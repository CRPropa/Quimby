/*
 * paged_grid.cpp
 *
 *  Created on: 06.05.2010
 *      Author: gmueller
 */

#include "arguments.hpp"

#include "gadget/Grid.hpp"
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
	std::cout << "FileSize:       " << fileSizeKpc << " kpc " << std::endl;

	size_t marginKpc = arguments.getInt("-margin", 1000);
	std::cout << "Margin:         " << marginKpc << " kpc " << std::endl;

	std::string prefix = arguments.getString("-prefix", "sph");
	std::cout << "Prefix:         " << prefix << std::endl;

	bool verbose = arguments.hasFlag("-v");
	bool veryverbose = arguments.hasFlag("-vv");
	if (veryverbose)
		verbose = true;

	size_t bins = size / fileSizeKpc;
	Grid<std::vector<SmoothParticle> > grid(bins, size);

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
			particle.position.x = (pos[iP * 3] - size / 2) / h + size / 2;
			particle.position.y = (pos[iP * 3 + 1] - size / 2) / h + size / 2;
			particle.position.z = (pos[iP * 3 + 2] - size / 2) / h + size / 2;

			particle.bfield.x = bfld[iP * 3];
			particle.bfield.y = bfld[iP * 3 + 1];
			particle.bfield.z = bfld[iP * 3 + 2];

			particle.mass = rho[iP];

			Vector3f relativePosition = particle.position - offset;
			Vector3f radius = Vector3f(particle.smoothingLength + marginKpc);
			Vector3f l = relativePosition - radius;
			l.clamp(0.0, size);

			Vector3f u = relativePosition + radius;
			u.clamp(0.0, size);

			Index3 lower, upper;
			lower.x = (uint32_t) std::floor(l.x / fileSizeKpc);
			lower.y = (uint32_t) std::floor(l.y / fileSizeKpc);
			lower.z = (uint32_t) std::floor(l.z / fileSizeKpc);

			upper.x = (uint32_t) std::ceil(u.x / fileSizeKpc);
			upper.y = (uint32_t) std::ceil(u.y / fileSizeKpc);
			upper.z = (uint32_t) std::ceil(u.z / fileSizeKpc);

			if ((verbose && (iP % 100000 == 0)) || veryverbose) {
				std::cout << "position:         " << particle.position
						<< std::endl;
				std::cout << "magnetic field:   " << particle.bfield
						<< std::endl;
				std::cout << "mass:             " << particle.mass << std::endl;
				std::cout << "smoothing length: " << particle.smoothingLength
						<< std::endl;
				std::cout << "lower:            " << lower << std::endl;
				std::cout << "upper:            " << upper << std::endl;
			}

			for (size_t x = lower.x; x < upper.x; x++)
				for (size_t y = lower.y; y < upper.y; y++)
					for (size_t z = lower.z; z < upper.z; z++)
						grid.get(x, y, z).push_back(particle);
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

