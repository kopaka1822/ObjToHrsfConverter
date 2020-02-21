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
// -transparent material1 material2 ... => forces materials to be seen as transparent (must be the material name)
// -flipaxis axis1 axis2 .. => flips the position axes
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
	if (args.has("nolight"))
		converter.removeComponent(hrsf::Component::Lights);
	if(args.has("transparent"))
	{
		auto names = args.getVector<std::string>("transparent");
		for(const auto& name : names)
		{
			converter.setTransparentMaterial(name);
		}
	}
	if(args.has("flipaxis"))
	{
		auto swaps = args.getVector<int>("flipaxis");
		if (swaps.size() % 2 != 0) throw std::runtime_error("number of flips must be multiple of two");
		converter.setAxisFlips(move(swaps));
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

// san miguel args: D:\scenes\obj\miguel\san-miguel-low-poly.obj D:\scenes\converted\miguel\miguel -nocamera -nolight -noenv -nomaterial -transparent materialo materialn
// D:\scenes\obj\mine\mine.obj D:\scenes\converted\mine\mine -nocamera -nolight -noenv -nomaterial -transparent crystal1 crystal2 crystal3 crystal4 old_lamp_glass star_glass web_center web_center.001 web_corner web_hanging web_hanging.001 web_thicc
// bistro e: D:\scenes\obj\bistro\Exterior\exterior.obj D:\scenes\converted\bistro\bistro\exterior
// bistro e: D:\scenes\obj\bistro\Interior\interior.obj D:\scenes\converted\bistro\bistro\interior