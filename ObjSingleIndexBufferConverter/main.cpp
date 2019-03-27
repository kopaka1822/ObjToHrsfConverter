#include "ArgumentSet.h"
#include <iostream>
#include "Converter.h"

int main(int argc, char** argv) try
{
	if (argc < 3)
		throw std::runtime_error("please provide input obj as first parameter and output file as second parameter");
	std::string inputFilename = argv[1];
	std::string outputFilename = argv[2];

	util::ArgumentSet args;
	args.init(argc - 3, argv + 3);

	Converter converter;
	converter.load(inputFilename);
	converter.convert();
	converter.printStats();
	converter.save(outputFilename);

	return 0;
}
catch (const std::exception& e)
{
	std::cerr << "ERR: " << e.what();
	return -1;
}
