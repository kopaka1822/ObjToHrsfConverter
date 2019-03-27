#pragma once
#include <string>
#include <vector>
#include <fstream>

class BinaryMeshLoader
{
public:
	static void load(
		const std::string& filename, 
		std::vector<float>& vertices, 
		std::vector<std::vector<int>>& shapes, 
		bool& hasNormals, 
		bool& hasTexcoords)
	{
		std::fstream file(filename, std::ios::in | std::ios::binary);

		char sig[3];
		file >> sig[0];
		file >> sig[1];
		file >> sig[2];

		if (sig[0] != 'M' || sig[1] != 'S' || sig[2] != 'H')
			throw std::runtime_error("invalid file signature. expected MSH");

		size_t version;
		file >> version;
		if (version != 0) throw std::runtime_error("invalid version");

		char c;
		file >> c; hasNormals = c != 0;
		file >> c; hasTexcoords = c != 0;

		size_t num;
		file >> num;
		shapes.assign(num, std::vector<int>());

		// shapes
		for(auto& s : shapes)
		{
			file >> num;
			s.resize(num);
			file.read(reinterpret_cast<char*>(s.data()), num * sizeof(s[0]));
		}

		// vertices
		file >> num;
		vertices.resize(num);
		file.read(reinterpret_cast<char*>(vertices.data()), num * sizeof(vertices[0]));

		file.close();
	}

	static void save(
		const std::string& filename, 
		const std::vector<float>& vertices, 
		const std::vector<std::vector<int>>& shapeIndices, 
		bool hasNormals, 
		bool hasTexcoords)
	{
		std::fstream file(filename + ".bin", std::ios::out | std::ios::binary);

		// signature
		file << 'M' << 'S' << 'H'; // Mesh signature
		file << size_t(0); // version

		file << char(hasNormals?1:0);
		file << char(hasTexcoords?1:0);

		// num shapes
		file << size_t(shapeIndices.size());
		// for all shapes the indices
		for (const auto& s : shapeIndices)
		{
			file << size_t(s.size());
			file.write(reinterpret_cast<const char*>(s.data()), s.size() * sizeof(s[0]));
		}
		// num vertices
		file << size_t(vertices.size());
		file.write(reinterpret_cast<const char*>(vertices.data()), vertices.size() * sizeof(vertices[0]));

		file.close();
	}
};
