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
UseSingleFile(false),
UseTexcoords(true),
RemoveDuplicates(false),
GenerateTextures(true),
RemoveTolerance(0.00001f)
{

}

void Converter::convert(std::filesystem::path src, std::filesystem::path dst)
{
	m_texConvert = TextureConverter(src.parent_path(), dst.parent_path(), GenerateTextures);
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

	// fix material texture paths
	for(auto& m : m_materials)
	{
		fixPath(m.diffuse_texname);
		fixPath(m.alpha_texname);
		fixPath(m.ambient_texname);
		fixPath(m.bump_texname);
		fixPath(m.displacement_texname);
		fixPath(m.metallic_texname);
		fixPath(m.normal_texname);
		fixPath(m.reflection_texname);
		fixPath(m.roughness_texname);
		fixPath(m.sheen_texname);
		fixPath(m.specular_highlight_texname);
		fixPath(m.specular_texname);
	}
}

void Converter::save(std::filesystem::path dst)
{
	Console::info("converting to hrsf");

	std::vector<hrsf::Material> materials;
	// materials are also required for mesh splitting
	if(OutComponents & hrsf::Component::Mesh || OutComponents & hrsf::Component::Material)
		materials = getMaterials();

	std::vector<hrsf::Mesh> mesh;
	if(OutComponents & hrsf::Component::Mesh)
	{
		mesh = convertMesh(materials);
	}
	
	hrsf::SceneFormat scene(std::move(mesh), getCamera(), getLights(), move(materials), getEnvironment());
	scene.verify();

	Console::info("removing unused materials");
	scene.removeUnusedMaterials();

	Console::info("writing to " + dst.string());
	scene.save(dst, UseSingleFile, OutComponents);
}

std::vector<hrsf::Mesh> Converter::convertMesh(const std::vector<hrsf::Material>& materials) const
{
	uint32_t requestedAttribs = bmf::Position;
	if(UseNormals)
		requestedAttribs |= bmf::Normal;

	if(UseTexcoords)
		requestedAttribs |= bmf::Texcoord0;



	Console::info("creating meshes");
	// convert all shapes into seperate binary meshes
	std::vector<bmf::BinaryMesh16> meshes;
	size_t curShape = 0;
	for (const auto& s : m_shapes)
	{
		++curShape;
		std::vector<bmf::BinaryMesh32> bigMeshes;
		std::vector<bmf::BinaryMesh16> smallMeshes;

		uint32_t attribs = bmf::Position;
		if (s.mesh.indices[0].normal_index >= 0 && UseNormals)
			attribs |= bmf::Normal;
		if (s.mesh.indices[0].texcoord_index >= 0 && UseTexcoords)
			attribs |= bmf::Texcoord0;

		const auto stride = bmf::getAttributeElementStride(attribs);

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

		if (s.mesh.material_ids.size() > 1 && !std::all_of(s.mesh.material_ids.begin(), s.mesh.material_ids.end(), [materialId](auto id)
			{
				return id == int(materialId);
			}))
		{
			Console::info("found multiple materials for one mesh");

			decltype(vertices) newVertices;
			decltype(indices) newIndices;
			std::vector<bmf::Shape> shapes;

			shapes.emplace_back(bmf::Shape{
				0, 0,
				0, 0,
				0
				});

			// split mesh based on materials
			auto lastMaterial = s.mesh.material_ids[0];
			uint32_t curIndex = 0;

			auto addShape = [&]()
			{
				shapes[0].indexCount = curIndex;
				shapes[0].vertexCount = curIndex;
				shapes[0].materialId = lastMaterial;
				curIndex = 0;
				bigMeshes.emplace_back(attribs, std::move(newVertices), std::move(newIndices), shapes);
				newVertices.clear();
				newIndices.clear();
			};

			for(size_t curFace = 0; curFace < indices.size() / 3; ++curFace)
			{
				if(s.mesh.material_ids[curFace] != lastMaterial)
				{
					// add this shape
					addShape();
					lastMaterial = s.mesh.material_ids[curFace];
				}

				// push back vertices
				for(size_t curVertex = 0; curVertex < 3; ++curVertex)
				{
					auto idx = indices[curFace * 3 + curVertex];
					for(size_t curStride = 0; curStride < stride; ++curStride)
					{
						newVertices.push_back(vertices[idx * stride + curStride]);
					}

				}
				newIndices.push_back(curIndex++);
				newIndices.push_back(curIndex++);
				newIndices.push_back(curIndex++);
			}

			if (newVertices.size()) addShape();

			for(auto& m : bigMeshes)
			{
				//m.verify();
				m.removeDuplicateVertices(); // a lot of duplicate vertices
				//m.verify();
			}
		}
		else
		{
			if (materialId == uint32_t(-1)) // not material => choose default material
				materialId = uint32_t(m_materials.size());

			std::vector<bmf::Shape> shapes;

			shapes.emplace_back(bmf::Shape{
			0,
			uint32_t(indices.size()),
			0,
			uint32_t(vertices.size() / stride),
			materialId
				});

			bigMeshes.emplace_back(attribs, std::move(vertices), std::move(indices), std::move(shapes));
		}
		
		// convert to 16 bit mesh
		for(auto& m : bigMeshes)
		{
			auto res = m.force16BitIndices();
			for(auto& sm : res)
			{
				smallMeshes.emplace_back(std::move(sm));
			}
		}

		if (smallMeshes.size() > bigMeshes.size())
			Console::info("forced 16 bit indices");

		for(auto& m : smallMeshes)
		{
			meshes.emplace_back(std::move(m));
		}
		
		Console::progress("meshes", curShape, m_shapes.size());
	}

	// missing attributes generators
	std::vector<std::unique_ptr<bmf::VertexGenerator>> generators;
	// normal generator
	generators.emplace_back(new bmf::FlatNormalGenerator());
	// texcoord generator
	float defTexCoord[] = { 0.0f, 0.0f };
	generators.emplace_back(new bmf::ConstantValueGenerator(bmf::ValueVertex(bmf::Attributes::Texcoord0, defTexCoord)));

	Console::info("removing duplicate vertices");
	size_t curCount = 0;
	size_t maxVertexCount = 0;
	for(auto& m : meshes)
	{
		m.removeDuplicateVertices();
		maxVertexCount = std::max(maxVertexCount, size_t(m.getNumVertices()));
		//m.centerShapes(); // center shapes to improve numerical stability for instances
		Console::progress("meshes", ++curCount, meshes.size());
	}
	Console::info("Max vertex count per shape: " + std::to_string(maxVertexCount));

	//Console::info("deinstancing shapes");
	//auto sizeBefore = meshes.size();
	//bmf::BinaryMesh::deinstanceShapes(meshes, 0.1f);
	//if(sizeBefore != meshes.size())
	//	Console::info("reduced shapes from " + std::to_string(sizeBefore) + " to " + std::to_string(meshes.size()));

	Console::info("generating missing attributes");
	curCount = 0;
	for (auto& m : meshes)
	{
		m.changeAttributes(requestedAttribs, generators);
		Console::progress("meshes", ++curCount, meshes.size());
	}

	if(!m_flips.empty())
	{
		Console::info("flipping geometry");

		const auto stride = bmf::getAttributeElementStride(requestedAttribs);
		const auto normalOffset = bmf::getAttributeElementOffset(requestedAttribs, bmf::Attributes::Normal);

		for(size_t i = 0; i < m_flips.size(); i += 2)
		{
			const auto axis1 = m_flips[i];
			const auto axis2 = m_flips[i + 1];

			for(auto& m : meshes)
			{
				auto& verts = m.getVertices();
				for(float* v = verts.data(), *end = verts.data() + verts.size(); v < end; v += stride)
				{
					std::swap(v[axis1], v[axis2]);
					std::swap(v[normalOffset + axis1], v[normalOffset + axis2]);
				}

				Console::progress("meshes (flip)", ++curCount, meshes.size());
			}
		}
	}

	Console::info("merging meshes");
	// all transparent meshes and all non transparent meshes belong together
	std::vector<bmf::BinaryMesh16> opaqueMeshes;
	opaqueMeshes.reserve(meshes.size());
	std::vector<bmf::BinaryMesh16> transMeshes;
	transMeshes.reserve(transMeshes.size());

	for(auto& m : meshes)
	{
		auto matId = m.getShapes()[0].materialId;
		if (materials.at(matId).data.flags & hrsf::MaterialData::Transparent)
			transMeshes.emplace_back(std::move(m));
		else
			opaqueMeshes.emplace_back(std::move(m));
	}

	// put into final vector
	std::vector<hrsf::Mesh> result;
	result.reserve(2);
	if(!opaqueMeshes.empty())
		result.emplace_back(bmf::BinaryMesh16::mergeShapes(opaqueMeshes));
	if(!transMeshes.empty())
		result.emplace_back(bmf::BinaryMesh16::mergeShapes(transMeshes));

	if (result.empty())
		throw std::runtime_error("no mesh available");

	Console::info("generating bounding volumes");
	for(auto& m : result)
	{
		m.triangle.generateBoundingVolumes();
	}

	return result;
}

hrsf::Camera Converter::getCamera() const
{
	hrsf::Camera cam; // use default camera for now
	cam.data = hrsf::CameraData::Default();
	return cam;
}

std::vector<hrsf::Light> Converter::getLights() const
{
	// just one light from the top
	std::vector<hrsf::Light> res;
	res.emplace_back();
	res.back().data.type = hrsf::LightData::Directional;
	res.back().data.color = { 1.0f, 1.0f, 1.0f };
	res.back().data.direction = { 0.1f, -1.0f, 0.1f }; // from the top

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
		mat.textures.albedo = m_texConvert.convertTexture(m.diffuse_texname);
		mat.textures.occlusion = m_texConvert.convertTexture(m.alpha_texname);
		mat.textures.specular = m_texConvert.convertTexture(m.specular_texname);
		// remaining stuff
		mat.data = hrsf::MaterialData::Default();
		std::copy(m.diffuse, m.diffuse + 3, glm::value_ptr(mat.data.albedo));
		mat.data.specular = (m.specular[0] + m.specular[1] + m.specular[2]) / 3.0f;
		mat.data.occlusion = m.dissolve;
		std::copy(m.emission, m.emission + 3, glm::value_ptr(mat.data.emission));
		std::copy(m.transmittance, m.transmittance + 3, glm::value_ptr(mat.data.translucency));;
		mat.data.metalness = m.metallic;
		if(m.roughness != 0.0f)
		{
			mat.data.roughness = m.roughness;
		}
		else
		{
			// map shininess to roughness
			mat.data.roughness = sqrt(2.0f / (m.shininess + 2.0f));
		}
		mat.data.flags = 0;

		//if (m.illum >= 3) // raytrace on flag
			//mat.data.flags |= hrsf::MaterialData::Flags::Reflection;

		// is transparent?
		bool isTransparent = false;
		if (mat.data.occlusion < 1.0f) isTransparent = true;
		if (!mat.textures.occlusion.empty()) isTransparent |= true;
		if (!mat.textures.albedo.empty() && m_texConvert.hasAlpha(mat.textures.albedo))
			isTransparent |= true;

		// forced by user
		if (m_transparentMaterials.find(mat.name) != m_transparentMaterials.end())
			isTransparent |= true;

		if (isTransparent)
			mat.data.flags |= hrsf::MaterialData::Transparent;

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

void Converter::fixPath(std::string& path)
{
	if (path.empty()) return;
	if (path[0] == '/') path = path.substr(1);
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

void Converter::removeComponent(hrsf::Component component)
{
	OutComponents = hrsf::Component(uint32_t(OutComponents) & ~uint32_t(component));
}

void Converter::setTransparentMaterial(const std::string& name)
{
	m_transparentMaterials.insert(name);
}

TextureConverter& Converter::getTexConverter()
{
	return m_texConvert;
}
