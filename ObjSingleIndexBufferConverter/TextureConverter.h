#pragma once
#include <string>
#include <filesystem>
#include <map>
#include <set>


// converts all files from png, jpg... to dds format with appropriate mipmaps
class TextureConverter
{
public:
	using path = std::filesystem::path;

	/// \param writeFiles indicates if the textures should be converted and written to the destination
	TextureConverter(path srcPath, path dstPath, bool writeFiles);
	TextureConverter() = default;

	/// \param expectSrgb expects png, jpg to be srgb when loading
	path convertTexture(const path& filename);

	/// \params indicates if an already converted image had an alpha channel
	bool hasAlpha(const path& dstFilePath);
private:
	path m_srcRoot;
	path m_dstRoot;
	std::map<path, path> m_convertedMap;
	// only contains textures (destination path) that have a native alpha channel
	std::set<path> m_alphaMap;
	bool m_writeFiles;
};