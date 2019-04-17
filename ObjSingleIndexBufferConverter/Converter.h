#pragma once
#include <string>
#include "../tinyobj/tiny_obj_loader.h"
#include "tinyobjhash.h"
#include <unordered_map>
#include "../props/property.h"
#define BMF_GENERATORS
#include <hrsf/SceneFormat.h>

using namespace prop;

class Converter
{
public:
	Converter();

	void load(const std::string& filename);
	void convert(const std::string& filename);
	
	void printStats() const;

	// properties
	DefaultGetterSetter<bool> UseNormals;
	DefaultGetterSetter<bool> UseTexcoords;
	// reduces duplicate vertices
	DefaultGetterSetter<bool> RemoveDuplicates;
	DefaultGetterSetter<float> RemoveTolerance;
private:
	bmf::BinaryMesh convertMesh() const;
	hrsf::Camera getCamera() const;
	std::vector<hrsf::Light> getLights() const;
	std::vector<hrsf::Material> getMaterials() const;
	hrsf::Environment getEnvironment() const;
private:
	tinyobj::attrib_t m_attrib;
	std::vector<tinyobj::shape_t> m_shapes;
	std::vector<tinyobj::material_t> m_materials;

	size_t m_normalsGenerated = 0;
	size_t m_texcoordsGenerated = 0;
	size_t m_verticesRemoved = 0;
	size_t m_normalsRemoved = 0;
	size_t m_texcoordsRemoved = 0;
};
