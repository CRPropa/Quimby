#include "arguments.h"

#include "gadget/SPHGrid.h"
#include "gadget/Index3.h"
#include "gadget/SmoothParticle.h"
#include "gadget/GadgetFile.h"
#include "gadget/Vector3.h"

#include <ctime>
#include <limits>
#include <algorithm>
#include <omp.h>
#include <sstream>
#include <fstream>

using namespace gadget;

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

	int fileSize = arguments.getInt("-fileSize", 20000);
	if (fileSize > size)
		fileSize = size;
	std::cout << "FileSize:       " << fileSize << " kpc " << std::endl;

	size_t margin = arguments.getInt("-margin", 1000);
	std::cout << "Margin:         " << margin << " kpc " << std::endl;

	std::string prefix = arguments.getString("-prefix", "sph");
	std::cout << "Prefix:         " << prefix << std::endl;

	size_t bins = size / fileSize;
	std::cout << "Bins:           " << bins << std::endl;

	SPHGrid grid(bins, size);
	grid.setOffset(offset);
	grid.setMargin(margin);
	std::vector<std::string> files;
	arguments.getVector("-f", files);
	for (size_t iArg = 0; iArg < files.size(); iArg++) {
		std::cout << "Open " << files[iArg] << " (" << (iArg + 1) << "/"
				<< files.size() << ")" << std::endl;

		grid.loadFromGadgetFile(files[iArg], h,
				Vector3f(120000, 120000, 120000));
	}

	std::cout << "Write output" << std::endl;

	for (size_t x = 0; x < bins; x++) {
		for (size_t y = 0; y < bins; y++)
			for (size_t z = 0; z < bins; z++) {
				std::stringstream sstr;
				sstr << prefix << "-" << x << "-" << y << "-" << z << ".raw";
				SmoothParticleHelper::write(sstr.str(), grid.get(x, y, z));
			}

	}

	return 0;
}

