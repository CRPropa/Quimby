#include "arguments.h"

#include "gadget/MultiResolutionMagneticField.h"
#include "gadget/Database.h"
#include "gadget/GadgetFile.h"

#include <iostream>
#include <stdexcept>

using namespace gadget;

int mrmf(Arguments &arguments) {
	float fa = arguments.getFloat("-fa", 0.1f);
	std::cout << "a:   " << fa << std::endl;
	float fb = arguments.getFloat("-fb", 0.05f);
	std::cout << "b:   " << fb << std::endl;
	float fc = arguments.getFloat("-fc", 0.025f);
	std::cout << "c:   " << fc << std::endl;

	float minres = arguments.getFloat("-r", 10.0f);
	std::cout << "MinRes:   " << minres << std::endl;

	int bins = arguments.getInt("-b", 64);
	std::cout << "Bins:     " << bins << std::endl;

	std::string output = arguments.getString("-o", "field.mrmf");
	std::cout << "Output:   " << output << std::endl;

	std::string dbname = arguments.getString("-db", "");
	std::cout << "Database: " << dbname << std::endl;
	FileDatabase fdb;
	if (!fdb.open(dbname))
		return 1;

	std::cout << "Create MultiResolutionMagneticField with " << fdb.getCount()
			<< " particles." << std::endl;
	MultiResolutionMagneticField::create(fdb, output, bins, minres, fa, fb, fc);

	std::cout << "done." << std::endl;

	return 0;
}
