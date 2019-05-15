#pragma once
#include <string>
#include <filesystem>
#include <map>

// converts all files from png, jpg... to dds format with appropriate mipmaps
class TextureConverter
{
public:
	using path = std::filesystem::path;

	/// \param writeFiles indicates if the textures should be converted and written to the destination
	TextureConverter(path srcPath, path dstPath, bool writeFiles);
	TextureConverter() = default;

	/// \param expectSrgb expects png, jpg to be srgb when loading
	path convertTexture(const path& filename, bool expectSrgb);
private:
	path m_srcRoot;
	path m_dstRoot;
	std::map<path, path> m_convertedMap;
	bool m_writeFiles;
};