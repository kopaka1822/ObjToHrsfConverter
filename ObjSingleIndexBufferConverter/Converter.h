#pragma once
#include <string>
#include "../tinyobj/tiny_obj_loader.h"
#include "tinyobjhash.h"
#include <unordered_map>
#include "../props/property.h"
#define BMF_GENERATORS
#include <hrsf/SceneFormat.h>
#include "TextureConverter.h"
#include <unordered_set>

using namespace prop;

class Converter
{
public:
	Converter();
	void convert(std::filesystem::path src, std::filesystem::path dst);
	
	void printStats() const;

	hrsf::Component OutComponents = hrsf::Component::All;
	void removeComponent(hrsf::Component component);
	void setTransparentMaterial(const std::string& name);

	TextureConverter& getTexConverter();

	// properties
	DefaultGetterSetter<bool> UseNormals;
	DefaultGetterSetter<bool> UseSingleFile;
	DefaultGetterSetter<bool> UseTexcoords;
	// reduces duplicate vertices
	DefaultGetterSetter<bool> RemoveDuplicates;
	DefaultGetterSetter<bool> GenerateTextures;
	DefaultGetterSetter<float> RemoveTolerance;
private:
	void load(std::filesystem::path src);
	void save(std::filesystem::path dst);

	std::vector<hrsf::Mesh> convertMesh(const std::vector<hrsf::Material>& materials) const;
	hrsf::Camera getCamera() const;
	std::vector<hrsf::Light> getLights() const;
	std::vector<hrsf::Material> getMaterials() const;
	hrsf::Environment getEnvironment() const;
private:
	tinyobj::attrib_t m_attrib;
	std::vector<tinyobj::shape_t> m_shapes;
	std::vector<tinyobj::material_t> m_materials;
	std::unordered_set<std::string> m_transparentMaterials;

	size_t m_normalsGenerated = 0;
	size_t m_texcoordsGenerated = 0;
	size_t m_verticesRemoved = 0;
	size_t m_normalsRemoved = 0;
	size_t m_texcoordsRemoved = 0;

	mutable TextureConverter m_texConvert;
};
