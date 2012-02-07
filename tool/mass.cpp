#include "gadget/GadgetFile.hpp"
#include "gadget/Grid.hpp"
#include "gadget/kernel.hpp"
#include "gadget/Octree.hpp"
#include "gadget/SmoothParticle.hpp"
#include "gadget/PagedGrid.hpp"

using namespace gadget;

#include "arguments.hpp"

#include <omp.h>

int mass(Arguments &arguments) {
	unsigned int bins = arguments.getInt("-bins", 10);
	std::cout << "Bins: " << bins << std::endl;

	float size = arguments.getFloat("-size", 240000);
	std::cout << "Size: " << size << std::endl;

	std::string output = arguments.getString("-o", "mass.raw");
	std::cout << "Output: " << output << std::endl;

	float h = arguments.getFloat("-h", 0.7);
	std::cout << "h:              " << h << std::endl;

	std::cout << "Create Grid" << std::endl;
	Grid<float> grid;
	grid.create(bins, size);
	grid.reset(0.0);

	bool verbose = arguments.hasFlag("-v");

	std::vector < std::string > files;
	arguments.getVector("-f", files);
	for (size_t iArg = 0; iArg < files.size(); iArg++) {
		std::cout << "Open " << files[iArg] << std::endl;

		GadgetFile file;
		file.open(files[iArg]);
		if (file.good() == false) {
			std::cerr << "Failed to open file " << files[iArg] << std::endl;
			return 1;
		}

		file.readHeader();
		int pn = file.getHeader().particleNumberList[0];
		std::cout << "Number of SmoothParticles: " << pn << std::endl;

		std::vector<float> pos;
		if (file.readFloatBlock("POS ", pos) == false) {
			std::cerr << "Failed to read POS block" << std::endl;
			return 1;
		}

		std::vector<float> rho;
		if (file.readFloatBlock("RHO ", rho) == false) {
			std::cerr << "Failed to read RHO block" << std::endl;
			return 1;
		}

		std::vector<float> hsml;
		if (file.readFloatBlock("HSML", hsml) == false) {
			std::cerr << "Failed to read HSML block" << std::endl;
			return 1;
		}

#pragma omp parallel for
		for (int iP = 0; iP < pn; iP++) {
			if (iP % (pn / 100) == 0 && verbose) {
#ifdef _OPENMP
				std::cerr << "[" << omp_get_thread_num() << "] ";
#endif
				std::cerr << iP << " / " << pn << std::endl;
			}
			float hsml2 = hsml[iP] / h;
			float vol = 1.0 / M_PI / hsml2 / hsml2 / hsml2;
			float pX = (pos[iP * 3] - size / 2) / h + size / 2;
			float pY = (pos[iP * 3 + 1] - size / 2) / h + size / 2;
			float pZ = (pos[iP * 3 + 2] - size / 2) / h + size / 2;
			int steps = ceil(hsml2 * 2 / grid.getCellLength());
			for (int iStepX = -steps; iStepX <= steps; iStepX++) {
				float x = iStepX * grid.getCellLength();
				for (int iStepY = -steps; iStepY <= steps; iStepY++) {
					float y = iStepY * grid.getCellLength();
					for (int iStepZ = -steps; iStepZ <= steps; iStepZ++) {
						float z = iStepZ * grid.getCellLength();
						float r = sqrt(x * x + y * y + z * z);
						float w = kernel(r / hsml2) * rho[iP] * vol;
						float &f = grid.get(pX + x, pY + y, pZ + z);
#pragma omp atomic
						f += w;
					}
				}

			}
		}
	}

	if (arguments.hasFlag("-dump")) {
		if (arguments.hasFlag("-zyx")) {
			std::cout << "Dump ZYX grid" << std::endl;
			grid.dumpZYX(output);
		} else {
			std::cout << "Dump XYZ grid" << std::endl;
			grid.dump(output);
		}
	} else {
		grid.save(output);
	}

	return 0;
}
