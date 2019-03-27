#include "Converter.h"
#include "FileHelper.h"
#include <algorithm>
#include "glm.h"
#include <iostream>
#include "VertexIterator.h"
#include <fstream>
#include "Loader.h"

Converter::Converter()
	:
UseNormals(true),
UseTexcoords(true),
RemoveDuplicates(false),
RemoveTolerance(0.00001f)
{

}

void Converter::load(const std::string& filename)
{
	const auto inputDirectory = getDirectory(filename);
	std::string warnings;
	std::string errors;
	bool res = tinyobj::LoadObj(&m_attrib, &m_shapes, &m_materials, &warnings, &errors, filename.c_str(),
		inputDirectory.c_str(), true);
	if (!res || !errors.empty())
		throw std::runtime_error("obj loader: " + errors);

	printf("# of vertices  = %d\n", static_cast<int>(m_attrib.vertices.size()) / 3);
	printf("# of normals   = %d\n", static_cast<int>(m_attrib.normals.size()) / 3);
	printf("# of texcoords = %d\n", static_cast<int>(m_attrib.texcoords.size()) / 2);
	printf("# of materials = %d\n", static_cast<int>(m_materials.size()));
	printf("# of shapes    = %d\n", static_cast<int>(m_shapes.size()));

	// count triangles
	size_t numIndices = 0;
	for (const auto& s : m_shapes)
	{
		numIndices += s.mesh.indices.size();
	}
	printf("# of indices   = %d\n", int(numIndices));
	printf("# of triangles = %d\n", int(numIndices / 3));

	if (m_attrib.vertices.empty())
		throw std::runtime_error("no vertices found");
}

void Converter::convert()
{
	if(UseNormals)
	{
		generateMissingNormals();
	}

	if(UseTexcoords)
	{
		generateMissingTexcoords();
	}

	if(RemoveDuplicates)
	{
		removeDuplicates();
	}

	for (const auto& s : m_shapes)
	{
		// add new shape indices
		m_outShapeIndices.emplace_back();
		
		for(auto index : s.mesh.indices)
		{
			const auto match = m_indexMap.find(index);
			if(match != m_indexMap.end())
			{
				// match
				m_outShapeIndices.back().push_back(match->second);
			}
			else
			{
				// add vertex
				m_outShapeIndices.back().push_back(addVertex(index));
			}
		}
	}
}

void Converter::printStats() const
{
	if(m_normalsGenerated)
		std::cerr << "generated " << m_normalsGenerated << " normals\n";
	if (m_texcoordsGenerated)
		std::cerr << "generated " << m_texcoordsGenerated << " texcoords\n";

	if (m_verticesRemoved)
		std::cerr << "removed " << m_verticesRemoved << " vertices\n";
	if (m_normalsRemoved)
		std::cerr << "removed " << m_normalsRemoved << " normals\n";
	if (m_texcoordsRemoved)
		std::cerr << "removed " << m_texcoordsRemoved << " texcoords\n";
}

void Converter::save(const std::string& filename) const
{
	std::cerr << "writing binary";

	BinaryMeshLoader::save(filename + ".bin", m_outVertices, m_outShapeIndices, UseNormals, UseTexcoords);
}

int Converter::addVertex(tinyobj::index_t idx)
{
	// position
	m_outVertices.push_back(m_attrib.vertices[idx.vertex_index * 3]);
	m_outVertices.push_back(m_attrib.vertices[idx.vertex_index * 3 + 1]);
	m_outVertices.push_back(m_attrib.vertices[idx.vertex_index * 3 + 2]);

	// normal
	if(UseNormals)
	{
		m_outVertices.push_back(m_attrib.normals[idx.normal_index * 3]);
		m_outVertices.push_back(m_attrib.normals[idx.normal_index * 3] + 1);
		m_outVertices.push_back(m_attrib.normals[idx.normal_index * 3] + 2);
	}

	// texcoords
	if(UseTexcoords)
	{
		m_outVertices.push_back(m_attrib.texcoords[idx.texcoord_index]);
		m_outVertices.push_back(m_attrib.texcoords[idx.texcoord_index] + 1);
	}

	return m_curIndex++;
}

void Converter::generateMissingNormals()
{
	std::cerr << "generating missing normals\n";
	auto nMissing = 0;
	for(auto& s : m_shapes)
	{
		// check normals for each triangle
		const bool hasNormals = std::all_of(s.mesh.indices.begin(), s.mesh.indices.end(), [](tinyobj::index_t i)
		{
			return i.normal_index != -1;
		});
		if (hasNormals) continue;

		++nMissing;
		// generate normals
		for(int i = 0, end = int(s.mesh.indices.size()); i < end; i += 3)
		{
			// retrieves points
			auto& i0 = s.mesh.indices[i];
			auto& i1 = s.mesh.indices[i + 1];
			auto& i2 = s.mesh.indices[i + 2];

			const auto p0 = glm::vec3(m_attrib.vertices[i0.vertex_index]);
			const auto p1 = glm::vec3(m_attrib.vertices[i1.vertex_index]);
			const auto p2 = glm::vec3(m_attrib.vertices[i2.vertex_index]);
			
			// calc edges
			const auto e1 = p1 - p0;
			const auto e2 = p2 - p0;

			// normal
			const auto n = glm::normalize(glm::cross(e1, e2));

			// add normal
			const auto nIndex = int(m_attrib.normals.size() / 3);
			m_attrib.normals.push_back(n.x);
			m_attrib.normals.push_back(n.y);
			m_attrib.normals.push_back(n.z);

			// overwrite indices
			i0.normal_index = nIndex;
			i1.normal_index = nIndex;
			i2.normal_index = nIndex;
		}

		m_normalsGenerated += s.mesh.indices.size();
	}

	std::cerr << "generated normals for " << nMissing << " shapes\n";
}

void Converter::generateMissingTexcoords()
{
	std::cerr << "generating missing texcoords\n";
	auto nMissing = 0;

	// use dummy texcoords
	if(m_attrib.texcoords.size() == 0)
	{
		// add dummy texcoords
		m_attrib.texcoords.push_back(0.0f);
		m_attrib.texcoords.push_back(0.0f);
	}

	// set dummy texcoords
	for(auto& s : m_shapes)
	{
		bool isMissing = false;
		for(auto& i : s.mesh.indices)
		{
			if (i.texcoord_index == -1)
			{
				i.texcoord_index = 0;
				m_texcoordsGenerated++;
				isMissing = true;
			}
		}
		if (isMissing) ++nMissing;
	}

	std::cerr << "generated texcoords for " << nMissing << " shapes\n";
}

void Converter::removeDuplicates()
{
	// remove duplicate vertices
	removeDuplicates(m_attrib.vertices, 3, RemoveTolerance,
	[this](size_t remove, size_t replace)
	{
		m_verticesRemoved++;
		this->removeIndex(remove, replace, [](tinyobj::index_t i)
		{
			return size_t(i.vertex_index);
		}, [](tinyobj::index_t& i, size_t value)
		{
			i.vertex_index = int(value);
		});
	});

	if (UseNormals)
		removeDuplicates(m_attrib.normals, 3, RemoveTolerance,
			[this](size_t remove, size_t replace)
		{
			m_normalsRemoved = 0;
			this->removeIndex(remove, replace, [](tinyobj::index_t i)
			{
				return size_t(i.normal_index);
			}, [](tinyobj::index_t& i, size_t value)
			{
				i.normal_index = int(value);
			});
		});

	if (UseTexcoords)
		removeDuplicates(m_attrib.texcoords, 2, RemoveTolerance,
			[this](size_t remove, size_t replace)
		{
			m_texcoordsRemoved = 0;
			this->removeIndex(remove, replace, [](tinyobj::index_t i)
			{
				return size_t(i.texcoord_index);
			}, [](tinyobj::index_t& i, size_t value)
			{
				i.texcoord_index = int(value);
			});
		});
}

void Converter::removeDuplicates(std::vector<float>& vec, size_t stride, float tolerance, 
	std::function<void(size_t, size_t)> removeIndex)
{
	std::vector<float> res;
	res.reserve(vec.size());
	std::cerr << "removing duplicates\n";

	auto index = 0;
	for(auto i = VertexIterator(vec.begin(), stride); i != vec.end(); )
	{
		if(index % 1000 == 0)
		{
			std::cerr << index << "/" << vec.size() / stride << '\n';
		}

		bool increaseIterator = true;

		// compare with all previous elements
		for(auto j = VertexIterator(vec.begin(), stride); j != i; ++j)
		{
			auto error = j.getDifference(i);
			if (error > tolerance) continue; // keep this vertex

			// remove this vertex
			removeIndex(i.getIndex(vec), j.getIndex(vec));
			i.erase(vec);
			increaseIterator = false;
			break;
		}

		if (increaseIterator)
		{
			++i;
			++index;
		}
	}
}

void Converter::removeIndex(size_t remove, size_t replaces, std::function<size_t(tinyobj::index_t)> getIndex,
	std::function<void(tinyobj::index_t&, size_t)> setIndex)
{
	std::cerr << "removing index\n";
	for(auto& s : m_shapes)
	{
		for(auto& i : s.mesh.indices)
		{
			auto idx = getIndex(i);
			if(idx == remove) // replace index
			{
				setIndex(i, replaces);
			}
			else if(idx > remove) // decrease index
			{
				setIndex(i, idx - 1);
			}
		}
	}
}
