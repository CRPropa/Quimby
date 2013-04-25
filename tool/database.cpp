#include "arguments.h"

#include "quimby/Database.h"
#include "quimby/GadgetFile.h"

#include <iostream>
#include <stdexcept>

using namespace quimby;

const char database_usage[] = "create database file from GADGET files.\n"
		"\nOptions:\n\n"
		"-f     list of input files, space seperated\n"
		"-o     filename of the database\n"
		"-h     Hubble constant to use, default: 0.7\n"
		"-px, -py, -pz\n"
		"       x, y, z of the pivot point for hubble streching, default: 120000\n"
		"-bins  number of bins used for database lookup, default: 100\n";

int database(Arguments &arguments) {
	std::vector<std::string> files;
	arguments.getVector("-f", files);
	std::string output = arguments.getString("-o", "");
	float h = arguments.getFloat("-h", 0.7);
	Vector3f pivot;
	pivot.x = arguments.getFloat("-px", 120000);
	pivot.y = arguments.getFloat("-py", 120000);
	pivot.z = arguments.getFloat("-pz", 120000);
	size_t bins = arguments.getFloat("-bins", 100);

	if ((files.size() == 0) || (output.size() == 0)) {
		std::cout << database_usage << std::endl;
		return 1;
	}

	std::cout << "Output:   " << output << std::endl;
	std::cout << "h:        " << h << std::endl;
	std::cout << "Pivot:    " << pivot << " kpc" << std::endl;
	std::cout << "bins:     " << bins << std::endl;

	std::vector<SmoothParticle> particles;
	for (size_t iArg = 0; iArg < files.size(); iArg++) {
		std::cout << "Open " << files[iArg] << " (" << (iArg + 1) << "/"
				<< files.size() << ")" << std::endl;

		GadgetFile file;
		file.open(files[iArg]);
		if (file.good() == false) {
			throw std::runtime_error("Failed to open file " + files[iArg]);
		}

		file.readHeader();
		int pn = file.getHeader().particleNumberList[0];
		if (pn < 1)
			return 0;

		std::vector<float> pos;
		if (file.readFloatBlock("POS ", pos) == false) {
			throw std::runtime_error(
					"Failed read POS from file " + files[iArg]);
		}

		std::vector<float> bfld;
		if (file.readFloatBlock("BFLD", bfld) == false) {
			throw std::runtime_error(
					"Failed read BFLD from file " + files[iArg]);
		}

		std::vector<float> hsml;
		if (file.readFloatBlock("HSML", hsml) == false) {
			throw std::runtime_error(
					"Failed read HSML from file " + files[iArg]);
		}

		std::vector<float> rho;
		if (file.readFloatBlock("RHO ", rho) == false) {
			throw std::runtime_error(
					"Failed read RHO from file " + files[iArg]);
		}

		for (int iP = 0; iP < pn; iP++) {
			SmoothParticle particle;
			particle.smoothingLength = hsml[iP];
			particle.position.x = pos[iP * 3];
			particle.position.y = pos[iP * 3 + 1];
			particle.position.z = pos[iP * 3 + 2];

			particle.bfield.x = bfld[iP * 3];
			particle.bfield.y = bfld[iP * 3 + 1];
			particle.bfield.z = bfld[iP * 3 + 2];

			particle.mass = file.getHeader().massList[0];
			particle.rho = rho[iP];

			particle.toKpc(h, pivot);

			particles.push_back(particle);
		}
	}

	std::cout << "create database with " << particles.size() << " particles."
			<< std::endl;
	FileDatabase::create(particles, output, bins);

	std::cout << "done." << std::endl;

	FileDatabase db;
	db.open(output);
	std::cout << "Resulting Database:" << std::endl;
	std::cout << " count: " << db.getCount() << std::endl;
	std::cout << " lower: " << db.getLowerBounds() << std::endl;
	std::cout << " upper: " << db.getUpperBounds() << std::endl;

	return 0;
}
