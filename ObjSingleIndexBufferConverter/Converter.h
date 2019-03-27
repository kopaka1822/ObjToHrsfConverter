#pragma once
#include <string>
#include "../tinyobj/tiny_obj_loader.h"
#include "tinyobjhash.h"
#include <unordered_map>
#include "../props/property.h"
using namespace prop;

class Converter
{
public:
	Converter();

	void load(const std::string& filename);
	void convert();
	void printStats() const;
	void save(const std::string& filename) const;

	// properties
	DefaultGetterSetter<bool> UseNormals;
	DefaultGetterSetter<bool> UseTexcoords;
	// reduces duplicate vertices
	DefaultGetterSetter<bool> RemoveDuplicates;
	DefaultGetterSetter<float> RemoveTolerance;
private:
	// adds vertex data and returns new global index
	int addVertex(tinyobj::index_t idx);
	void generateMissingNormals();
	void generateMissingTexcoords();
	void removeDuplicates();

	struct RemovedIndexInfo
	{
		int removedId;
		int replacedWithId;
	};

	/// \param removeIndex: first param: removed index, second param: the index it was replaces with
	static void removeDuplicates(std::vector<float>& vec, size_t stride, float tolerance, std::function<void(size_t, size_t)> removeIndex);
	void removeIndex(size_t remove, size_t replaces, std::function<size_t(tinyobj::index_t)> getIndex, std::function<void(tinyobj::index_t&, size_t)> setIndex);
private:
	tinyobj::attrib_t m_attrib;
	std::vector<tinyobj::shape_t> m_shapes;
	std::vector<tinyobj::material_t> m_materials;

	std::vector<float> m_outVertices;
	std::vector<std::vector<int>> m_outShapeIndices;
	std::unordered_map<tinyobj::index_t, int> m_indexMap;
	int m_curIndex = 0;

	size_t m_normalsGenerated = 0;
	size_t m_texcoordsGenerated = 0;
	size_t m_verticesRemoved = 0;
	size_t m_normalsRemoved = 0;
	size_t m_texcoordsRemoved = 0;
};
