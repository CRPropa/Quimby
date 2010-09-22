/*
 * paged_grid.cpp
 *
 *  Created on: 06.05.2010
 *      Author: gmueller
 */

#include "arguments.hpp"

#include "gadget/PagedGrid.hpp"
#include "gadget/SmoothParticle.hpp"
#include "gadget/GadgetFile.hpp"
#include "gadget/Vector3.hpp"

#include <ctime>
#include <limits>
#include <algorithm>
#include <omp.h>

class PagedSPVisitor: public PagedGrid<Vector3f>::Visitor {
public:
	SmoothParticle particle;
	size_t lastX, lastY, lastZ;
	Vector3f b;
	float cellLength;

	PagedSPVisitor() :
		lastX(-1), lastY(-1), lastZ(-1) {
	}

	void visit(PagedGrid<Vector3f> &grid, size_t x, size_t y, size_t z,
			Vector3f &value) {
		if (x != lastX) {
			b.x = SmoothParticle::kernel(particle.bfield.x, toCellCenter(x),
					particle.position.x, particle.smoothingLength);
		}
		lastX = x;

		if (y != lastY) {
			b.y = SmoothParticle::kernel(particle.bfield.y, toCellCenter(y),
					particle.position.y, particle.smoothingLength);
		}
		lastY = y;

		if (z != lastZ) {
			b.z = SmoothParticle::kernel(particle.bfield.z, toCellCenter(z),
					particle.position.z, particle.smoothingLength);
		}
		lastZ = z;

		value += b;
	}

	float toCellCenter(size_t x) {
		float a = (float) x + 0.5f;
		return cellLength * a;
	}
};

inline size_t limit(const float &x, const size_t &lower, const size_t &upper) {
	size_t r;

	if (x < 0)
		r = 0;
	else
		r = (size_t)x;

	if (r < lower)
		r = lower;
	else if (r > upper)
		r = upper;

	return r;
}

size_t dround(double d) {
	return size_t(d + 0.5);
}

int paged_grid(Arguments &arguments) {

	int pageSize = arguments.getInt("-pageSize", 100);
	std::cout << "PageSize:       " << pageSize << " kpc " << std::endl;

	int res = arguments.getInt("-res", 100);
	std::cout << "Resolution:     " << res << " kpc" << std::endl;

	int pageLength = pageSize / res;
	std::cout << "PageLength:     " << pageLength << std::endl;

	int skip = arguments.getInt("-skip", 0);

	int size = arguments.getInt("-size", 40000);
	std::cout << "Size:           " << size << " kpc" << std::endl;

	float h = arguments.getFloat("-h", 0.7);
	std::cout << "h:              " << h << std::endl;

	BinaryPageIO<Vector3f> io;
	std::string prefix = arguments.getString("-prefix", "paged_grid");
	io.setPrefix(prefix);
	io.setForceDump(true);
	std::cout << "Output Prefix:  " << prefix << std::endl;

	index3_t lowerLimit, upperLimit;
	lowerLimit.x = arguments.getInt("-lx", 0) / res;
	lowerLimit.y = arguments.getInt("-ly", 0) / res;
	lowerLimit.z = arguments.getInt("-lz", 0) / res;

	upperLimit.x = arguments.getInt("-ux", size) / res;
	upperLimit.y = arguments.getInt("-uy", size) / res;
	upperLimit.z = arguments.getInt("-uz", size) / res;

	std::cout << "Lower limit:    " << lowerLimit * res << " kpc" << std::endl;
	std::cout << "Upper limit:    " << upperLimit * res << " kpc" << std::endl;

	int memory = arguments.getInt("-memory", 512);
	size_t page_byte_size = pageLength * pageLength * pageLength
			* sizeof(Vector3f);
	int pageCount = std::max((size_t) 1, (memory * 1024 * 1024)
			/ page_byte_size);
	std::cout << "Memory:         " << memory << " MiB -> " << pageCount
			<< " pages" << std::endl;

	io.setDefaultValue(Vector3f(0.0f));
	io.setOverwrite(true);

	size_t fileSizeKpc = arguments.getInt("-fileSize", 10000);
	size_t fileSize = fileSizeKpc / res;
	io.setElementsPerFile(fileSize);
	size_t pages_per_file = (fileSize / pageLength) * (fileSize / pageLength) * (fileSize / pageLength);
	std::cout << "FileSize:       " << fileSizeKpc << " kpc "
			<< (pages_per_file * page_byte_size / 1024 / 1024) << " MiB -> "
			<< pages_per_file << " pages" << std::endl;

	LastAccessPagingStrategy<Vector3f> strategy;
	PagedGrid<Vector3f> grid;
	grid.setSize(size / res);
	grid.setPageSize(pageLength);
	grid.setStrategy(&strategy);
	grid.setIO(&io);
	grid.setPageCount(pageCount);

	Vector3f offset(arguments.getFloat("-offX", 0), arguments.getFloat("-offY",
			0), arguments.getFloat("-offZ", 0));
	std::cout << "Offset:         " << offset << std::endl;

	bool verbose = arguments.hasFlag("-v");

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

		std::cout << "  Fill grid" << std::endl;

		time_t start = std::time(0);
		time_t last = std::time(0);
		index3_t totalMin(std::numeric_limits<uint32_t>::max()), totalMax(size_t(0));
		float avgSL = 0.0;
		size_t lastN = 0;
		for (int iP = skip; iP < pn; iP++) {
			time_t now = std::time(0);
			if ((now - last >= 1) && verbose && iP) {
				time_t elapsed = now - last;
				size_t n = iP - lastN;
				float pps = (float) n / std::min((time_t) 1, elapsed);
				std::cout << "\r  " << iP << ": " << (iP * 100) / pn
						<< "%, pages: " << grid.getActivePageCount() << " ("
						<< io.getLoadedPages() << " loaded), throughput: "
						<< pps << "               \r";
				std::cout.flush();

				last = now;
				lastN = iP;
				avgSL = 0.0;
			}

			avgSL += hsml[iP] / h;

			PagedSPVisitor v;
			v.particle.smoothingLength = hsml[iP] / h;
			v.particle.position.x = pos[iP * 3] / h - offset.x;
			v.particle.position.y = pos[iP * 3 + 1] / h - offset.y;
			v.particle.position.z = pos[iP * 3 + 2] / h - offset.z;

			float norm = 1.0 / M_PI / pow(v.particle.smoothingLength, 3);
			v.particle.bfield.x = bfld[iP * 3] * norm;
			v.particle.bfield.y = bfld[iP * 3 + 1] * norm;
			v.particle.bfield.z = bfld[iP * 3 + 2] * norm;

			v.cellLength = res;

			index3_t lower, upper;
			lower.x = std::max(lowerLimit.x, (uint32_t)std::floor((v.particle.position.x
					- v.particle.smoothingLength * 2) / res));
			lower.y = std::max(lowerLimit.y, (uint32_t)std::floor((v.particle.position.y
					- v.particle.smoothingLength * 2) / res));
			lower.z = std::max(lowerLimit.z, (uint32_t)std::floor((v.particle.position.z
					- v.particle.smoothingLength * 2) / res));

			upper.x = std::min(upperLimit.x - 1, (uint32_t)std::ceil(
					(v.particle.position.x + v.particle.smoothingLength * 2)
							/ res));
			upper.y = std::min(upperLimit.y - 1, (uint32_t)std::ceil(
					(v.particle.position.y + v.particle.smoothingLength * 2)
							/ res));
			upper.z = std::min(upperLimit.z - 1, (uint32_t)std::ceil(
					(v.particle.position.z + v.particle.smoothingLength * 2)
							/ res));

			grid.accept(v, lower, upper);

			totalMin.x = std::min(totalMin.x, lower.x);
			totalMin.y = std::min(totalMin.y, lower.y);
			totalMin.z = std::min(totalMin.z, lower.z);

			totalMax.x = std::max(totalMax.x, upper.x);
			totalMax.y = std::max(totalMax.y, upper.y);
			totalMax.z = std::max(totalMax.z, upper.z);
		}

		size_t duration = time(0) - start;
		std::cout << "\r  Done: " << duration / 3600 << "h " << (duration
				% 3600) / 60 << "m " << duration % 60
				<< "s                                                  "
				<< std::endl;
		totalMin *= res;
		totalMax *= res;
		std::cout << "  min: " << totalMin << " kpc" << std::endl;
		std::cout << "  max: " << totalMax << " kpc" << std::endl;
		std::cout << "  pages: " << grid.getActivePageCount() << " ("
				<< io.getLoadedPages() << " loaded)" << std::endl;

	}

	std::cout << "Write output" << std::endl;
	grid.flush();

	return 0;
}
