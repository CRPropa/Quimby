#include "arguments.h"

#include "gadget/Database.h"
#include "gadget/GadgetFile.h"

#include <iostream>
#include <stdexcept>

using namespace gadget;

int lmf(Arguments &arguments) {
	int n1 = arguments.getFloat("-n1", 256);
	std::cout << "n1:   " << n1 << std::endl;
	int n2 = arguments.getFloat("-n2", 32);
	std::cout << "n2:   " << n2 << std::endl;

	std::string output = arguments.getString("-o", "field.lmf");
	std::cout << "Output:   " << output << std::endl;

	std::string dbname = arguments.getString("-db", "");
	std::cout << "Database: " << dbname << std::endl;

	FileDatabase fdb;
	if (!fdb.open(dbname))
		return 1;

	float s1 = 220000. / n1;
	size_t nx = 0;
	size_t ny = 0;
	size_t nDetailed = 0;

	std::ofstream out(output.c_str(), std::ios::binary);
	for (size_t nx = 0; nx < n1; nx++) {
		std::cout << "\r - " << nx << std::endl;
		for (size_t ny = 0; ny < n1; ny++) {
			std::vector<SmoothParticle> particles;
			Vector3f l(10000 + nx * s1, 10000 + ny * s1, 120000);
			Vector3f u = l + Vector3f(s1);
			fdb.getParticles(l, u, particles);
			float f = 1e32;
			for (size_t i = 0; i < particles.size(); i++) {
				f = std::min(particles[i].smoothingLength, f);
			}
			if (f < s1)
				nDetailed++;

			out.write((const char *) &f, sizeof(f));
		}
	}

	std::cout << "Details required: " << nDetailed << " -> "
			<< (n1 * nDetailed * 0.7 * n2 * n2 * n2 * 12) / 1000 / 1000 << " MB"
			<< std::endl;
	std::cout << "done." << std::endl;

	return 0;
}
