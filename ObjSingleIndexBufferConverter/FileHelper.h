#pragma once
#include <string>

// attempts to retrieve the file directory
inline std::string getDirectory(const std::string &filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos)
		return filepath.substr(0, filepath.find_last_of("/\\")) + "\\";
	return "";
}