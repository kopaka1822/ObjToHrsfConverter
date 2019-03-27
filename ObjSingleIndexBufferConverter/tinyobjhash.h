#pragma once

namespace std
{
	template<>
	struct hash<tinyobj::index_t>
	{
		size_t operator()(const tinyobj::index_t& idx) const noexcept
		{
			return size_t(idx.vertex_index) ^
				(size_t(idx.normal_index) << 21) ^
				(size_t(idx.texcoord_index) << 42);
		}
	};

	template<>
	struct equal_to<tinyobj::index_t>
	{
		bool operator()(const tinyobj::index_t& left, const tinyobj::index_t& right) const
		{
			return memcmp(&left, &right, sizeof(left)) == 0;
		}
	};
}