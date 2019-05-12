#include "TextureConverter.h"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

#include "../gli/gli/gli.hpp"
#include "../gli/gli/generate_mipmaps.hpp"

TextureConverter::TextureConverter(path srcPath, path dstPath)
	:
m_srcRoot(srcPath),
m_dstRoot(dstPath)
{}

gli::texture2d::format_type getSrbFormat(int components)
{
	switch (components)
	{
	case 1: return gli::format::FORMAT_R8_SRGB_PACK8;
	case 2: return gli::format::FORMAT_RG8_SRGB_PACK8;
	case 3: return gli::format::FORMAT_RGB8_SRGB_PACK8;
	case 4: return gli::format::FORMAT_RGBA8_SRGB_PACK8;
	}
	return gli::FORMAT_UNDEFINED;
}

gli::texture2d::format_type getUnormFormat(int components)
{
	switch (components)
	{
	case 1: return gli::format::FORMAT_R8_UNORM_PACK8;
	case 2: return gli::format::FORMAT_RG8_UNORM_PACK8;
	case 3: return gli::format::FORMAT_RGB8_UNORM_PACK8;
	case 4: return gli::format::FORMAT_RGBA8_UNORM_PACK8;
	}
	return gli::FORMAT_UNDEFINED;
}

gli::texture2d loadStbiImage(const std::string& filename, bool expectSrgb)
{
	int x, y, comp;
	auto data = stbi_load(filename.c_str(), &x, &y, &comp, 0);
	if (data == nullptr)
		throw std::runtime_error("could not load " + filename);

	// convert to gli image
	gli::texture2d gliTex(
		expectSrgb?getSrbFormat(comp):getUnormFormat(comp),
		gli::extent2d(x, y),
		1, gli::texture::swizzles_type()
	);

	const auto size = gliTex.size(0);
	if (size != size_t(x * y * comp))
	{
		throw std::runtime_error("expected other gli size");
	}

	memcpy(gliTex.data(0, 0, 0), data, size);
	stbi_image_free(data);

	return gliTex;
}

TextureConverter::path TextureConverter::convertTexture(const path& filename, bool expectSrgb)
{
	if (filename.empty()) return "";

	auto srcPath = m_srcRoot / filename;
	auto dstPath = (m_dstRoot / filename).replace_extension(".dds");

	auto it = m_convertedMap.find(srcPath);
	if(it != m_convertedMap.end())
	{
		return it->second;
	}

	// assure that the directory is available
	std::filesystem::create_directories(dstPath.parent_path());
	// add new entry
	m_convertedMap[srcPath] = dstPath;

	// input is relative path
	if(filename.extension() == ".dds")
	{
		// assume already converted
		std::filesystem::copy_file(srcPath, dstPath);
		return dstPath;
	}

	// load png, jpg etc. with stb
	auto tex = loadStbiImage(srcPath.string(), expectSrgb);
	// generate mip maps
	auto mipTex = gli::generate_mipmaps(tex, gli::filter::FILTER_LINEAR);

	auto dstPathString = dstPath.string();
	if(!gli::save_dds(mipTex, dstPathString.c_str()))
	{
		throw std::runtime_error("could not save " + dstPathString);
	}

	return dstPath;
}
