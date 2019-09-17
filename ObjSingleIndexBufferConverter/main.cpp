#include "ArgumentSet.h"
#include <iostream>
#include "Converter.h"
#include "Console.h"

// params: 
// -notextures => skips texture conversion / generation
// -singlefile => saves camera etc. in a single file
// -nomaterial => skips material write
// -nocamera => skips camera write
// -nolight => skips light write
// -noenv => skips env write
// -nomesh => skips mesh generation
// -transparent texture1 texture2 ... => forces textures to be seen as transparent ()
int main(int argc, char** argv) try
{
	if (argc < 3)
		throw std::runtime_error("please provide input obj as first parameter and output file as second parameter");
	std::string inputFilename = argv[1];
	std::string outputFilename = argv[2];

	util::ArgumentSet args;
	args.init(argc - 3, argv + 3);

	Converter converter;

	if (args.has("notextures") || args.has("nomaterial"))
		converter.GenerateTextures = false;
	if (args.has("singlefile"))
		converter.UseSingleFile = true;
	if (args.has("nomaterial"))
		converter.removeComponent(hrsf::Component::Material);
	if (args.has("nocamera"))
		converter.removeComponent(hrsf::Component::Camera);
	if (args.has("noenv"))
		converter.removeComponent(hrsf::Component::Environment);
	if (args.has("nomesh"))
		converter.removeComponent(hrsf::Component::Mesh);
	if(args.has("transparent"))
	{
		auto textures = args.getVector<std::string>("transparent");
		for (const auto& t : textures)
			converter.getTexConverter().setAlphaTexture(t);
	}

	converter.convert(inputFilename, outputFilename);
	converter.printStats();

	return 0;
}
catch (const std::exception& e)
{
	Console::error(e.what());
	return -1;
}
