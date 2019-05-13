#include "Converter.h"
#include "FileHelper.h"
#include <algorithm>
#include "glm.h"
#include <iostream>
#include "Console.h"
#include "TextureConverter.h"

Converter::Converter()
	:
UseNormals(true),
UseTexcoords(true),
RemoveDuplicates(false),
RemoveTolerance(0.00001f)
{

}

void Converter::convert(std::filesystem::path src, std::filesystem::path dst)
{
	m_texConvert = TextureConverter(src.parent_path(), dst.parent_path());
	load(src);	
	save(dst);
}

void Converter::load(std::filesystem::path src)
{
	Console::info("loading " + src.string());

	const auto inputDirectory = src.parent_path();
	std::string warnings;
	std::string errors;

	auto srcString = src.string();
	auto dirString = inputDirectory.string();
	bool res = tinyobj::LoadObj(&m_attrib, &m_shapes, &m_materials, &warnings, &errors, srcString.c_str(),
		dirString.c_str(), true);
	if (!res || !errors.empty())
		throw std::runtime_error("obj loader: " + errors);

	Console::info("# of vertices  = " + std::to_string(static_cast<int>(m_attrib.vertices.size()) / 3));
	Console::info("# of normals   = " + std::to_string(static_cast<int>(m_attrib.normals.size()) / 3));
	Console::info("# of texcoords = " + std::to_string(static_cast<int>(m_attrib.texcoords.size()) / 2));
	Console::info("# of materials = " + std::to_string(static_cast<int>(m_materials.size())));
	Console::info("# of shapes    = " + std::to_string(static_cast<int>(m_shapes.size())));

	// count triangles
	size_t numIndices = 0;
	for (const auto& s : m_shapes)
	{
		numIndices += s.mesh.indices.size();
	}
	Console::info("# of indices   = " + std::to_string(numIndices));
	Console::info("# of triangles = " + std::to_string(numIndices / 3));

	if (m_attrib.vertices.empty())
		throw std::runtime_error("no vertices found");
}

void Converter::save(std::filesystem::path dst)
{
	Console::info("converting to hrsf");
	hrsf::SceneFormat scene(convertMesh(), getCamera(), getLights(), getMaterials(), getEnvironment());
	scene.verify();

	Console::info("removing unused materials");
	scene.removeUnusedMaterials();

	Console::info("writing to " + dst.string());
	scene.save(dst);
}

bmf::BinaryMesh Converter::convertMesh() const
{
	uint32_t requestedAttribs = bmf::Position;
	if(UseNormals)
		requestedAttribs |= bmf::Normal;

	if(UseTexcoords)
		requestedAttribs |= bmf::Texcoord0;

	Console::info("creating meshes");
	// convert all shapes into seperate binary meshes
	std::vector<bmf::BinaryMesh> meshes;
	for (const auto& s : m_shapes)
	{
		uint32_t attribs = bmf::Position;
		if (s.mesh.indices[0].normal_index >= 0 && UseNormals)
			attribs |= bmf::Normal;
		if (s.mesh.indices[0].texcoord_index >= 0 && UseTexcoords)
			attribs |= bmf::Texcoord0;

		// for now brute force create mesh
		std::vector<float> vertices;
		std::vector<uint32_t> indices;

		indices.reserve(s.mesh.indices.size());
		vertices.reserve(s.mesh.indices.size() * bmf::getAttributeElementStride(attribs));
		for(const auto & i : s.mesh.indices)
		{
			indices.push_back(uint32_t(indices.size()));

			vertices.push_back(m_attrib.vertices[3 * i.vertex_index]);
			vertices.push_back(m_attrib.vertices[3 * i.vertex_index + 1]);
			vertices.push_back(m_attrib.vertices[3 * i.vertex_index + 2]);
			if(attribs & bmf::Normal)
			{
				vertices.push_back(m_attrib.normals[3 * i.normal_index]);
				vertices.push_back(m_attrib.normals[3 * i.normal_index + 1]);
				vertices.push_back(m_attrib.normals[3 * i.normal_index + 2]);
			}
			if(attribs & bmf::Texcoord0)
			{
				vertices.push_back(m_attrib.texcoords[2 * i.texcoord_index]);
				// directX reverses y coordinate
				vertices.push_back(1.0f - m_attrib.texcoords[2 * i.texcoord_index + 1]);
			}
		}

		uint32_t materialId;
		if (s.mesh.material_ids.empty()) // choose default material (will be added by getMaterials() later)
			materialId = uint32_t(m_materials.size());
		else
			materialId = uint32_t(s.mesh.material_ids[0]);

		if (s.mesh.material_ids.size() > 1 && Console::PrintWarning)
		{
			// are they all the same?
			if(!std::all_of(s.mesh.material_ids.begin(), s.mesh.material_ids.end(), [materialId](auto id)
			{
				return id == int(materialId);
			}))
				Console::warning("shape has more than one material. Only the first one will be used");
		}

		if (materialId == uint32_t(-1)) // not material => choose default material
			materialId = uint32_t(m_materials.size());

		std::vector<bmf::BinaryMesh::Shape> shapes;
		shapes.emplace_back(bmf::BinaryMesh::Shape{
			0,
			uint32_t(indices.size()),
			materialId
			});

		meshes.emplace_back(attribs, std::move(vertices), std::move(indices), std::move(shapes));

		Console::progress("meshes", meshes.size(), m_shapes.size());
	}

	// missing attributes generators
	std::vector<std::unique_ptr<bmf::VertexGenerator>> generators;
	// normal generator
	generators.emplace_back(new bmf::FlatNormalGenerator());
	// texcoord generator
	float defTexCoord[] = { 0.0f, 0.0f };
	generators.emplace_back(new bmf::ConstantValueGenerator(bmf::ValueVertex(bmf::Attributes::Texcoord0, defTexCoord)));

	Console::info("removing duplicate vertices and generating missing attributes");
	size_t curCount = 0;
	// beautify meshes
	for(auto& m : meshes)
	{
		m.removeDuplicateVertices();
		// generate new meshes if required
		m.changeAttributes(requestedAttribs, generators);

		Console::progress("meshes", ++curCount, meshes.size());
	}

	// merge together into one final mesh
	Console::info("merging meshes into one mesh");
	return bmf::BinaryMesh::mergeShapes(meshes);
}

hrsf::Camera Converter::getCamera() const
{
	return hrsf::Camera::Default(); // just use default camera for now
}

std::vector<hrsf::Light> Converter::getLights() const
{
	// just one light from the top
	std::vector<hrsf::Light> res;
	res.emplace_back();
	res.back().type = hrsf::Light::Directional;
	res.back().color = { 1.0f, 1.0f, 1.0f };
	res.back().direction = { 0.1f, -1.0f, 0.1f }; // from the top

	return res;
}

std::vector<hrsf::Material> Converter::getMaterials() const
{
	Console::info("converting materials");

	std::vector<hrsf::Material> res;
	res.reserve(m_materials.size() + 1);

	for(const auto& m : m_materials)
	{
		res.emplace_back();
		auto& mat = res.back();
		mat.name = m.name;
		// textures
		mat.textures.diffuse = m_texConvert.convertTexture(m.diffuse_texname, true);
		mat.textures.ambient = m_texConvert.convertTexture(m.ambient_texname, true);
		mat.textures.occlusion = m_texConvert.convertTexture(m.alpha_texname, true);
		mat.textures.specular = m_texConvert.convertTexture(m.specular_texname, true);
		// remaining stuff
		mat.data = hrsf::MaterialData::Default();
		std::copy(m.diffuse, m.diffuse + 3, mat.data.diffuse.begin());
		std::copy(m.ambient, m.ambient + 3, mat.data.ambient.begin());
		std::copy(m.specular, m.specular + 3, mat.data.specular.begin());
		mat.data.occlusion = m.dissolve;
		std::copy(m.emission, m.emission + 3, mat.data.emission.begin());
		mat.data.gloss = m.shininess;
		mat.data.roughness = m.roughness;
		mat.data.flags = 0;

		if (m.illum >= 3) // raytrace on flag
			mat.data.flags |= hrsf::MaterialData::Flags::Reflection;

		Console::progress("materials", res.size(), m_materials.size());
	}

	// add default material fallback (if some shape had no material it will use this)
	res.emplace_back();
	res.back().name = "missing_material";
	res.back().data = hrsf::MaterialData::Default();

	return res;
}

hrsf::Environment Converter::getEnvironment() const
{
	hrsf::Environment e;
	// default white background
	e.color = { 1.0f, 1.0f, 1.0f };
	return e;
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