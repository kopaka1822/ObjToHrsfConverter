#include "TextureConverter.h"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

#include "../gli/gli/gli.hpp"
#include "../gli/gli/generate_mipmaps.hpp"

TextureConverter::TextureConverter(path srcPath, path dstPath, bool writeFiles)
	:
m_srcRoot(srcPath),
m_dstRoot(dstPath),
m_writeFiles(writeFiles)
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

uint32_t CountMips(uint32_t width, uint32_t height)
{
	if (width == 0 || height == 0)
		return 0;

	uint32_t count = 1;
	while (width > 1 || height > 1)
	{
		width >>= 1;
		height >>= 1;
		count++;
	}
	return count;
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
		CountMips(x, y), 
		gli::texture::swizzles_type()
	);

	const auto size = gliTex.size(0);
	if (size != size_t(x * y * comp))
	{
		throw std::runtime_error("expected other gli size");
	}

	memcpy(gliTex.data(0, 0, 0), data, size);
	stbi_image_free(data);

	// RGB formats are deprecated => convert to rgba
	if (comp != 3) return gliTex;

	return gli::convert(gliTex, expectSrgb?getSrbFormat(4):getUnormFormat(4));
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

	// add new entry
	m_convertedMap[srcPath] = dstPath;

	if (!m_writeFiles) return dstPath;

	// assure that the directory is available
	std::filesystem::create_directories(dstPath.parent_path());

	// input is relative path
	if(filename.extension() == ".dds")
	{
		// assume already converted
		std::filesystem::copy_file(srcPath, dstPath);
		return dstPath;
	}

	// load png, jpg etc. with stb
	auto tex = loadStbiImage(srcPath.string(), expectSrgb);
	// create levels for mipmaps
	
	// generate mip maps
	auto mipTex = gli::generate_mipmaps(tex, gli::filter::FILTER_LINEAR);

	auto dstPathString = dstPath.string();
	if(!gli::save_dds(mipTex, dstPathString.c_str()))
	{
		throw std::runtime_error("could not save " + dstPathString);
	}

	return dstPath;
}
