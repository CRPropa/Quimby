#include "arguments.h"

#include "quimby/GadgetFile.h"
#include "quimby/Grid.h"
#include "quimby/Octree.h"
#include "quimby/SmoothParticle.h"
#include "quimby/Database.h"

#include <omp.h>

using namespace quimby;

class MassVisitor: public DatabaseVisitor {
public:
	Grid<float> grid;
	bool cells;
	Vector3f offset;
	size_t i;

	void begin(const Database &db) {
		i = 0;
	}

	void visit(const SmoothParticle &p) {
		i++;
		if (i % 10000 == 0) {
			std::cout << ".";
			std::cout.flush();
		}
		if (i % 1000000 == 0) {
			std::cout << " " << (i / 1000000) << std::endl;
		}

		Vector3f pos = p.position - offset;
		Vector3f sl = Vector3f(p.smoothingLength, p.smoothingLength,
				p.smoothingLength);
		float spacing = grid.getSize() / (grid.getBins() - 1);

		Vector3f rLower = ((pos - sl) / spacing).floor();
		rLower.clamp(0, (float) grid.getBins());

		Vector3f rUpper = ((pos + sl) / spacing).ceil();
		rUpper.clamp(0, (float) grid.getBins());

		for (int iStepX = rLower.x; iStepX < rUpper.x; iStepX++) {
			float x = iStepX * spacing;
			for (int iStepY = rLower.y; iStepY < rUpper.y; iStepY++) {
				float y = iStepY * spacing;
				for (int iStepZ = rLower.z; iStepZ < rUpper.z; iStepZ++) {
					float z = iStepZ * spacing;
					float k = p.kernel(offset + Vector3f(x, y, z));
					if (k < 0.00001)
						continue;
					float w = k * p.mass * p.weight();
					float &f = grid.get(iStepX, iStepY, iStepZ);
					f += w;
				}
			}
		}
	}

	void end() {
		std::cout << std::endl << "Total: " << i << std::endl;
	}
};

int mass(Arguments &arguments) {
	MassVisitor visitor;

	unsigned int bins = arguments.getInt("-bins", 10);
	std::cout << "Bins: " << bins << std::endl;

	visitor.cells = arguments.hasFlag("-cell");
	std::cout << "Cells: " << (visitor.cells ? "yes" : "no") << std::endl;

	float size = arguments.getFloat("-size", 240000);
	std::cout << "Size: " << size << " kpc" << std::endl;

	visitor.offset.x = arguments.getFloat("-ox", 0);
	visitor.offset.y = arguments.getFloat("-oy", 0);
	visitor.offset.z = arguments.getFloat("-oz", 0);
	std::cout << "Offset:         " << visitor.offset << " kpc" << std::endl;

	std::string output = arguments.getString("-o", "mass.raw");
	std::cout << "Output: " << output << std::endl;

	std::string database = arguments.getString("-db", "sph.db");
	std::cout << "Database:       " << database << std::endl;

	bool verbose = arguments.hasFlag("-v");

	FileDatabase db;
	if (!db.open(database)) {
		std::cout << "Failed to open database!" << std::endl;
		return 1;
	}

	visitor.grid.create(bins, size);
	visitor.grid.reset(0.0);

	Vector3f lower = visitor.offset;
	Vector3f upper = visitor.offset + Vector3f(size, size, size);
	db.accept(lower, upper, visitor);

	if (arguments.hasFlag("-dump")) {
		if (arguments.hasFlag("-zyx")) {
			std::cout << "Dump ZYX grid" << std::endl;
			visitor.grid.dumpZYX(output);
		} else {
			std::cout << "Dump XYZ grid" << std::endl;
			visitor.grid.dump(output);
		}
	} else {
		std::cout << "Save grid" << std::endl;
		visitor.grid.save(output);
	}

	std::cout << "done" << std::endl;

	return 0;
}
