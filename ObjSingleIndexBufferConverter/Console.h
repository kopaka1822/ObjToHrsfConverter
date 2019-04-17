#pragma once
#include <string>

class Console
{
public:
	Console() = delete;
	/// \brief prints information if PrintInfo is true
	static void info(const std::string& text);
	/// \brief prints warning if PrintWarning is true
	static void warning(const std::string& text);
	/// \brief prints error if PrintError is true
	static void error(const std::string& text);

	/// \brief prints progress if PrintInfo is enabled: what curCount/totalCount
	/// outputs every 2 seconds.
	/// \param what identifier
	/// \param curCount starting with 1 and ending with totalCount
	/// \param totalCount maximum value of curCount
	static void progress(const char* what, size_t curCount, size_t totalCount);

	inline static bool PrintInfo = true;
	inline static bool PrintWarning = true;
	inline static bool PrintError = true;

private:
	static void write(std::string text);
};
