#include "TextureConverter.h"
#include <iostream>
#include "../image/ImageFramework.h"

static ImageFramework::Model s_image("../image/ImageConsole.exe");

TextureConverter::TextureConverter(path srcPath, path dstPath, bool writeFiles)
	:
m_srcRoot(srcPath),
m_dstRoot(dstPath),
m_writeFiles(writeFiles)
{
	//s_image.SetExportQuality(20);
	s_image.SetExportQuality(90);
}

TextureConverter::path TextureConverter::convertTexture(const path& filename)
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

	// assure that the directory is available
	std::filesystem::create_directories(dstPath.parent_path());

	// open file
	s_image.ClearImages();
	s_image.OpenImage(srcPath.string());
	const char* dstFormat = "RGBA8_SRGB";
	if(s_image.IsAlpha())
	{
		m_alphaMap.insert(dstPath);
	}

	if(!std::filesystem::exists(dstPath))
	{
		s_image.GenMipmaps();
		s_image.Export(dstPath.string(), dstFormat);
	}

	return dstPath;
}

bool TextureConverter::hasAlpha(const path& dstFilePath)
{
	return m_alphaMap.find(dstFilePath) != m_alphaMap.end();
}