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

class DumpVectorGridVisitor: public Grid<Vector3f>::Visitor {
public:
	std::ofstream outfile;
	DumpVectorGridVisitor(const std::string &filename) {
		outfile.open(filename.c_str(), std::ios::binary);
	}

	void visit(Grid<Vector3f> &grid, size_t x, size_t y, size_t z,
			Vector3f &value) {
		outfile.write((char *) &value, sizeof(Vector3f));
	}
};

class SPHVisitor: public Grid<Vector3f>::Visitor {
	size_t lastX, lastY, lastZ;
	Vector3f b;
	SmoothParticle particle;
	size_t reslution_kpc;
	Index3 offset;
public:

	SPHVisitor(const SmoothParticle &particle, size_t reslution_kpc) :
		lastX(-1), lastY(-1), lastZ(-1), particle(particle),
				reslution_kpc(reslution_kpc) {
	}

	void visit(Grid<Vector3f> &grid, size_t x, size_t y, size_t z,
			Vector3f &value) {
		if (x != lastX) {
			b.x = SmoothParticle::kernel(particle.bfield.x, x * reslution_kpc,
					particle.position.x, particle.smoothingLength);
			lastX = x;
		}

		if (y != lastY) {
			b.y = SmoothParticle::kernel(particle.bfield.y, y * reslution_kpc,
					particle.position.y, particle.smoothingLength);
			lastY = y;
		}

		if (z != lastZ) {
			b.z = SmoothParticle::kernel(particle.bfield.z, z * reslution_kpc,
					particle.position.z, particle.smoothingLength);
			lastZ = z;
		}

		value += b;
	}
};

template<class T>
T clamp(const T &value, const T &min, const T&max) {
	if (value < min)
		return min;
	else if (value > max)
		return max;
	else
		return value;
}

int sph_dump(Arguments &arguments) {
	int resolution_kpc = arguments.getInt("-resolution", 100);
	std::cout << "Resolution: " << resolution_kpc << " kpc" << std::endl;

	int size_kpc = arguments.getFloat("-size", 20000);
	std::cout << "Size: " << size_kpc << " kpc" << std::endl;

	std::string output = arguments.getString("-o", "output.raw");
	std::cout << "Output: " << output << std::endl;

	Index3 offset_kpc;
	offset_kpc.x = arguments.getInt("-ox", 0);
	offset_kpc.y = arguments.getInt("-oy", 0);
	offset_kpc.z = arguments.getInt("-oz", 0);
	std::cout << "Origin: " << offset_kpc.x << ", " << offset_kpc.y << ", "
			<< offset_kpc.z << " kpc" << std::endl;

	size_t samples = size_kpc / resolution_kpc + 1;
	std::cout << "Create Grid" << std::endl;
	Grid<Vector3f> grid;
	grid.create(samples, size_kpc);
	grid.reset(Vector3f(0, 0, 0));

	bool verbose = arguments.hasFlag("-v");

	std::string filename = arguments.getString("-f", "inputs.raw");
	std::cout << "Open " << filename << std::endl;
	std::ifstream file;
	file.open(filename.c_str(), std::ios::binary);
	if (file.good() == false) {
		std::cerr << "Failed to open file " << filename << std::endl;
		return 1;
	}

	uint32_t s;
	file.read((char *) &s, sizeof(uint32_t));
	std::cout << "Read " << s << " particles" << std::endl;
	std::vector<SmoothParticle> sphs;
	sphs.resize(s);
	std::cout << "Read " << (sizeof(SmoothParticle) * s) << " bytes of data" << std::endl;
	file.read((char *) sphs.data(), sizeof(SmoothParticle) * s);

	std::cout << "Fill grid" << std::endl;
	Vector3f origin(offset_kpc.x, offset_kpc.y, offset_kpc.z);
	size_t cells = 0;
	size_t pc = 10;
	for (int iP = 0; iP < s; iP++) {
		if (iP % 10000 == 0) {
			std::cout << iP << std::endl;
		}
		SmoothParticle &sp = sphs[iP];

		sp.position -= origin;
		float r = sp.smoothingLength * 1.8;
		float r2 = r * r;

		Vector3f b, p;
		size_t x_min = clamp((int)::floor((sp.position.x - r) / resolution_kpc), 0, (int)samples-1);
		size_t x_max = clamp((int)::ceil((sp.position.x + r) / resolution_kpc), 0, (int)samples-1);
		for (size_t x = x_min; x <= x_max; x++) {
			p.x = x * resolution_kpc;
			float x2 = pow(p.x - sp.position.x, 2);
			float r_yz2 = r2 - x2;

			if (r_yz2 <= 0)
				continue;

			b.x = SmoothParticle::kernel(sp.bfield.x, p.x, sp.position.x, sp.smoothingLength);
			float r_yz = sqrt(r_yz2);

			size_t y_min = clamp((int)::floor((sp.position.y - r_yz) / resolution_kpc), 0, (int)samples - 1);
			size_t y_max = clamp((int)::ceil((sp.position.y + r_yz) / resolution_kpc), 0, (int)samples - 1);
			for (size_t y = y_min; y <= y_max; y++) {
				p.y = y * resolution_kpc;
				float y2 = pow(p.y - sp.position.y, 2);
				float r_z2 = r2 - x2 - y2;
				if (r_z2 <= 0)
					continue;

				b.y = SmoothParticle::kernel(sp.bfield.y, p.y, sp.position.y, sp.smoothingLength);
				float r_z = sqrt(r_z2);

				size_t z_min = clamp((int)::floor((sp.position.z - r_z) / resolution_kpc), 0, (int)samples-1);
				size_t z_max = clamp((int)::ceil((sp.position.z + r_z) / resolution_kpc), 0, (int)samples-1);
				for (size_t z = z_min; z <= z_max; z++) {
					p.z = z * resolution_kpc;
					b.z = SmoothParticle::kernel(sp.bfield.z, p.z, sp.position.z, sp.smoothingLength);
					grid.get(x, y, z) += b;
				}
			}
		}
	}

	DumpVectorGridVisitor visitor(output);
	std::cout << "Dump ZYX vector grid" << std::endl;
	grid.acceptZYX(visitor);

	return 0;
}
