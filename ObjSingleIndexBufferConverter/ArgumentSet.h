#pragma once
#include <unordered_map>
#include <string>

namespace util
{

	class ArgumentSet
	{
	public:
		ArgumentSet() = default;
		ArgumentSet(ArgumentSet&& m) noexcept
			: m_params(move(m.m_params))
		{}
		ArgumentSet& operator=(ArgumentSet&& o) noexcept
		{
			m_params = move(o.m_params);
			return *this;
		}

		/**
		 * \brief initialises arguments
		 * \param argc argument count
		 * \param argv arguments
		 *			syntax: -parameter1 arg1 arg2 ... --parameter2
		 *			note: - if no argument is provided it will automatically hold the value "true"
		 *				  - same parameters will be overwritten by the last one
		 */
		void init(int argc, char** argv)
		{
			for (int c = 0; c < argc; ++c)
			{
				if (argv[c][0] == '-')
				{
					// parameter with - or -- ?
					std::string name = argv[c][1] == '-' ? argv[c] + 2 : argv[c] + 1;
					// read parameters
					std::vector<std::string> args;
					for (int i = c + 1; i < argc && argv[i][0] != '-'; ++i)
					{
						// add argument
						args.emplace_back(argv[i]);
					}
					if (!args.empty())
						c += int(args.size());
					else
						args.emplace_back("true");
					// create entry
					m_params[name] = args;
				}
			}
		}

		/**
		 * \brief returns the first argument to requested name
		 * \tparam T argument type
		 * \param name parameter name
		 * \param def default if argument can not be found
		 * \return the first argument to requested name. if it could not be found def is returned
		 */
		template <class T>
		T get(const std::string& name, T def) const
		{
			auto it = m_params.find(name);
			if (it != m_params.end())
			{
				return convertString<T>(it->second[0]);
			}
			return def;
		}

		/**
		 * \brief fills dst with first argument to requested name
		 * \tparam T argument type
		 * \param name parameter name
		 * \param dst argument destination (can be nullptr)
		 * \return true if the argument could be found
		 */
		template <class T>
		bool get(const std::string& name, T* dst) const
		{
			auto it = m_params.find(name);
			if (it != m_params.end())
			{
				if (dst)
					*dst = convertString<T>(it->second[0]);
				return true;
			}
			return false;
		}

		/**
		 * \brief fills the tuple width the arguments to the requested parameter
		 * \tparam T tuple types
		 * \param name parameter name
		 * \param dst argument destination (can be nullptr)
		 * \return true if the arguments could be found and the argument count matches the tuple size
		 */
		template <class... T>
		bool get(const std::string& name, std::tuple<T...>* dst)
		{
			auto it = m_params.find(name);
			if (it != m_params.end())
			{
				if (it->second.size() == std::tuple_size<std::tuple<T...>>::value)
				{
					if (dst)
					{
						// fill the tuple
						for_each_in_tuple(*dst, [&](auto& ref, auto copy, size_t idx)
						{
							const_cast<decltype(copy)&>(ref) = convertString<decltype(copy)>(it->second[idx]);
						});
					}
					return true;
				}
			}
			return false;
		}

		/**
		 * \brief overwrites a parameter with given arguments
		 * \param name parameter
		 * \param args arguments
		 */
		void set(const std::string& name, std::string args...)
		{
			std::vector<std::string> a = { args };
			m_params[name] = move(a);
		}
		/**
		* \brief checks if an entry for the parameter exists
		* \param name parameter name
		* \return true if the parameter exists
		*/
		bool has(const std::string& name) const
		{
			return get<std::string>(name, nullptr);
		}

		/**
		 * \brief checks if an argument value belongs to a parameter
		 * \tparam T argument type
		 * \param name parameter name
		 * \param value argument value
		 * \return true if the argument belongs to the parameter
		 */
		template<class T>
		bool includes(const std::string& name, T value) const
		{
			auto it = m_params.find(name);
			if (it != m_params.end())
			{
				for (const auto& v : it->second)
					if (convertString<T>(v) == value)
						return true;
			}
			return false;
		}

		template<>
		bool includes(const std::string& name, const char* value) const
		{
			return includes(name, std::string(value));
		}
		/**
		 * \brief returns arguments to the provided parameter
		 * \tparam T conversion type for the arguments
		 * \param name parameter name
		 * \return vector with the converted arguments or an empty vector if parameter not found
		 */
		template<class T>
		std::vector<T> getVector(const std::string& name) const
		{
			auto it = m_params.find(name);
			if (it != m_params.end())
			{
				std::vector<T> vals;
				vals.reserve(it->second.size());
				for (const auto& s : it->second)
					vals.push_back(convertString<T>(s));
				return vals;
			}
			return std::vector<T>();
		}

		template <class T>
		static T convertString(const std::string& s) noexcept = delete;

		template<>
		static std::string convertString(const std::string& s) noexcept
		{
			return s;
		}

		template<>
		static const char* convertString(const std::string& s) noexcept
		{
			return s.c_str();
		}

		template<>
		static float convertString(const std::string& s) noexcept
		{
			try
			{
				return stof(s);
			}
			catch (const std::exception&)
			{
				return 0.0f;
			}
		}

		template<>
		static double convertString(const std::string& s) noexcept
		{
			try
			{
				return stod(s);
			}
			catch (const std::exception&)
			{
				return 0.0;
			}
		}

		template<>
		static bool convertString(const std::string& s) noexcept
		{
			if (s == "true")
				return true;
			return false;
		}

		template<>
		static int convertString(const std::string& s) noexcept
		{
			try
			{
				return stoi(s);
			}
			catch (const std::exception&)
			{
				return 0;
			}
		}

		template<>
		static short convertString(const std::string& s) noexcept
		{
			return short(convertString<int>(s));
		}

		template<>
		static char convertString(const std::string& s) noexcept
		{
			return char(convertString<int>(s));
		}

		template<>
		static long convertString(const std::string& s) noexcept
		{
			try
			{
				return stol(s);
			}
			catch (const std::exception&)
			{
				return 0;
			}
		}
	private:
		template<class F, class...Ts, std::size_t...Is>
		void for_each_in_tuple(const std::tuple<Ts...> & tuple, F func, std::index_sequence<Is...>) {
			using expander = int[];
			(void)expander {
				0, ((void)func(std::get<Is>(tuple), std::get<Is>(tuple), Is), 0)...
			};
		}

		template<class F, class...Ts>
		void for_each_in_tuple(const std::tuple<Ts...> & tuple, F func) {
			for_each_in_tuple(tuple, func, std::make_index_sequence<sizeof...(Ts)>());
		}

	private:
		std::unordered_map<std::string, std::vector<std::string>> m_params;
	};

}