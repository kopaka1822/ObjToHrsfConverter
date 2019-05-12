#pragma once
#include <string>
#include <filesystem>
#include <map>

// converts all files from png, jpg... to dds format with appropriate mipmaps
class TextureConverter
{
public:
	using path = std::filesystem::path;

	TextureConverter(path srcPath, path dstPath);
	TextureConverter() = default;

	/// \param expectSrgb expects png, jpg to be srgb when loading
	path convertTexture(const path& filename, bool expectSrgb);
private:
	path m_srcRoot;
	path m_dstRoot;
	std::map<path, path> m_convertedMap;
};