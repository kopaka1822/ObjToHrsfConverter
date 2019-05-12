#include "ArgumentSet.h"
#include <iostream>
#include "Converter.h"
#include "Console.h"

int main(int argc, char** argv) try
{
	if (argc < 3)
		throw std::runtime_error("please provide input obj as first parameter and output file as second parameter");
	std::string inputFilename = argv[1];
	std::string outputFilename = argv[2];

	util::ArgumentSet args;
	args.init(argc - 3, argv + 3);

	Converter converter;
	converter.convert(inputFilename, outputFilename);
	converter.printStats();

	return 0;
}
catch (const std::exception& e)
{
	Console::error(e.what());
	return -1;
}
