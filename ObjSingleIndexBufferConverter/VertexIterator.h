#pragma once
#include <vector>

// iterator adapter
class VertexIterator
{
public:
	using base_type = std::vector<float>::iterator;

	VertexIterator(base_type it, size_t stride)
		:
	m_value(it),
	m_stride(stride)
	{}

	VertexIterator& operator++()
	{
		m_value += m_stride;
		return *this;
	}

	bool operator==(const VertexIterator& other) const
	{
		return m_value == other.m_value;
	}

	bool operator!=(const VertexIterator& other) const
	{
		return m_value != other.m_value;
	}

	bool operator==(const base_type& other) const
	{
		return m_value == other;
	}

	bool operator!=(const base_type& other) const
	{
		return m_value != other;
	}

	void erase(std::vector<float>& vec)
	{
		m_value = vec.erase(m_value, m_value + m_stride);
	}

	size_t getIndex(const std::vector<float>& vec) const
	{
		return (m_value - vec.begin()) / m_stride;
	}

	float getDifference(const VertexIterator& other) const
	{
		float err = 0.0f;
		for (size_t i = 0; i < m_stride; ++i)
			err += std::abs(*(m_value + i) - *(other.m_value + i));

		return err;
	}
private:
	base_type m_value;
	size_t m_stride;
};