/*
 * paged_grid.cpp
 *
 *  Created on: 06.05.2010
 *      Author: gmueller
 */

#include "arguments.hpp"

#include "gadget/SPHGrid.hpp"
#include "gadget/Index3.hpp"
#include "gadget/SmoothParticle.hpp"
#include "gadget/GadgetFile.hpp"
#include "gadget/Vector3.hpp"

#include <ctime>
#include <limits>
#include <algorithm>
#include <omp.h>
#include <sstream>
#include <fstream>

void updateRho(std::vector<SmoothParticle> &sph) {
	const size_t s = sph.size();
#pragma omp parallel for
	for (size_t i = 0; i < s; i++) {
		sph[i].updateRho(sph);
	}
}

int sph(Arguments &arguments) {

	int size = arguments.getInt("-size", 240000);
	std::cout << "Size:           " << size << " kpc" << std::endl;

	Vector3f offset;
	offset.x = arguments.getFloat("-ox", 0);
	offset.y = arguments.getFloat("-oy", 0);
	offset.z = arguments.getFloat("-oz", 0);
	std::cout << "Offset:         " << offset << " kpc" << std::endl;

	float h = arguments.getFloat("-h", 0.7);
	std::cout << "h:              " << h << std::endl;

	size_t fileSizeKpc = arguments.getInt("-fileSize", 20000);
	if (fileSizeKpc > size)
		fileSizeKpc = size;
	std::cout << "FileSize:       " << fileSizeKpc << " kpc " << std::endl;

	size_t marginKpc = arguments.getInt("-margin", 1000);
	std::cout << "Margin:         " << marginKpc << " kpc " << std::endl;

	std::string prefix = arguments.getString("-prefix", "sph");
	std::cout << "Prefix:         " << prefix << std::endl;

	bool skipRho = arguments.hasFlag("-skipRho");
	bool verbose = arguments.hasFlag("-v");
	bool veryverbose = arguments.hasFlag("-vv");
	if (veryverbose)
		verbose = true;

	size_t bins = size / fileSizeKpc;
	std::cout << "Bins:           " << bins << std::endl;

	SPHGrid grid(bins, size);
	grid.setOffset(offset);
	grid.setMargin(marginKpc);
	std::vector<std::string> files;
	arguments.getVector("-f", files);
	for (size_t iArg = 0; iArg < files.size(); iArg++) {
		std::cout << "Open " << files[iArg] << " (" << (iArg + 1) << "/"
				<< files.size() << ")" << std::endl;

		GadgetFile file;
		file.open(files[iArg]);
		if (file.good() == false) {
			std::cerr << "Failed to open file " << files[iArg] << std::endl;
			return 1;
		}

		file.readHeader();
		int pn = file.getHeader().particleNumberList[0];
		std::cout << "  Number of SmoothParticles: " << pn << std::endl;

		std::cout << "  Read POS block" << std::endl;
		std::vector<float> pos;
		if (file.readFloatBlock("POS ", pos) == false) {
			std::cerr << "Failed to read POS block" << std::endl;
			return 1;
		}

		std::cout << "  Read BFLD block" << std::endl;
		std::vector<float> bfld;
		if (file.readFloatBlock("BFLD", bfld) == false) {
			std::cerr << "Failed to read BFLD block" << std::endl;
			return 1;
		}

		std::cout << "  Read HSML block" << std::endl;
		std::vector<float> hsml;
		if (file.readFloatBlock("HSML", hsml) == false) {
			std::cerr << "Failed to read HSML block" << std::endl;
			return 1;
		}

		std::cout << "  Read RHO block" << std::endl;
		std::vector<float> rho;
		if (file.readFloatBlock("RHO ", rho) == false) {
			std::cerr << "Failed to read RHO block" << std::endl;
			return 1;
		}

		for (int iP = 0; iP < pn; iP++) {
			SmoothParticle particle;
			particle.smoothingLength = hsml[iP] / h;
			Vector3f p(pos[iP * 3], pos[iP * 3 + 1], pos[iP * 3 + 2]);
			particle.position = p.scale(1 / h,
					Vector3f(120000, 120000, 120000));

			particle.bfield.x = bfld[iP * 3];
			particle.bfield.y = bfld[iP * 3 + 1];
			particle.bfield.z = bfld[iP * 3 + 2];

			particle.mass = rho[iP];

			grid.add(particle);
		}
	}

	std::cout << "Write output" << std::endl;

	for (size_t x = 0; x < bins; x++) {
		if (verbose) {
			std::cout << "x = " << x << std::endl;
		}

		for (size_t y = 0; y < bins; y++)
			for (size_t z = 0; z < bins; z++) {
				std::stringstream sstr;
				sstr << prefix << "-" << x << "-" << y << "-" << z << ".raw";
				std::ofstream out(sstr.str().c_str(), std::ofstream::binary);
				if (!skipRho)
					updateRho(grid.get(x, y, z));
				uint32_t s = grid.get(x, y, z).size();
				if (verbose)
					std::cout << s << std::endl;
				out.write((const char *) &s, sizeof(uint32_t));
				out.write((const char *) &grid.get(x, y, z)[0],
						sizeof(SmoothParticle) * s);
			}

	}

	return 0;
}

