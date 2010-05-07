/*
 * test.cpp
 *
 *  Created on: 29.01.2010
 *      Author: gmueller
 */

#include "gadget/GadgetFile.hpp"
#include "gadget/Grid.hpp"
#include "gadget/kernel.hpp"
#include "gadget/Octree.hpp"
#include "gadget/SmoothParticle.hpp"
#include "gadget/PagedGrid.hpp"

#include "arguments.hpp"

#include <cmath>
#include <ctime>
#include <cstdlib>
#include <string>
#include <ctime>

#include <omp.h>

int paged_grid(Arguments &arguments);

int mass(Arguments &arguments) {
	unsigned int bins = arguments.getInt("-bins", 10);
	std::cout << "Bins: " << bins << std::endl;

	float size = arguments.getFloat("-size", 10);
	std::cout << "Size: " << size << std::endl;

	std::string output = arguments.getString("-o", "mass.raw");
	std::cout << "Output: " << output << std::endl;

	std::cout << "Create Grid" << std::endl;
	Grid<float> grid;
	grid.create(bins, size);
	grid.reset(0.0);

	bool verbose = arguments.hasFlag("-v");

	std::vector<std::string> files;
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
			float vol = M_PI / hsml[iP] / hsml[iP] / hsml[iP];
			float pX = pos[iP * 3];
			float pY = pos[iP * 3 + 1];
			float pZ = pos[iP * 3 + 2];
			int steps = ceil(hsml[iP] * 2 / grid.getCellLength());
			for (int iStepX = -steps; iStepX <= steps; iStepX++) {
				float x = iStepX * grid.getCellLength();
				for (int iStepY = -steps; iStepY <= steps; iStepY++) {
					float y = iStepY * grid.getCellLength();
					for (int iStepZ = -steps; iStepZ <= steps; iStepZ++) {
						float z = iStepZ * grid.getCellLength();
						float r = sqrt(x * x + y * y + z * z);
						float w = kernel(r / hsml[iP]) * rho[iP] * vol;
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

int av(int argc, const char **argv) {
	if (argc < 3) {
		std::cout << "missing filename!" << std::endl;
		return -1;
	}

	GadgetFile file;
	file.open(argv[2]);
	if (file.good() == false) {
		std::cerr << "Failed to open file " << argv[2] << std::endl;
		return 1;
	}

	file.readHeader();
	size_t pn = file.getHeader().particleNumberList[0];
	std::cerr << "Number of #0 Particles: " << pn << std::endl;

	std::vector<float> bfield;
	if (file.readFloatBlock("BFLD", bfield) == false) {
		std::cerr << "Failed to read BFLD block" << std::endl;
		return 1;
	}

	if ((bfield.size() / 3) != pn) {
		std::cerr << "BFLD size mismatch. BFLD count: " << (bfield.size() / 3)
				<< " Particle count: " << pn << std::endl;
		return 1;
	}

	float avgBx = 0.0, avgBy = 0.0, avgBz = 0.0;
	for (size_t i = 0; i < pn; i++) {
		avgBx += fabs(bfield[i * 3]);
		avgBy += fabs(bfield[i * 3 + 1]);
		avgBz += fabs(bfield[i * 3 + 2]);
	}

	std::cout << "Average Bx = " << avgBx / pn << std::endl;
	std::cout << "Average By = " << avgBy / pn << std::endl;
	std::cout << "Average Bz = " << avgBz / pn << std::endl;

	return 0;
}

class DumpMagnitudeGridVisitor: public Grid<Vector3f>::Visitor {
private:
	std::ofstream outfile;
	bool logarithm;
public:
	DumpMagnitudeGridVisitor(const std::string &filename, bool logarithm) {
		outfile.open(filename.c_str(), std::ios::binary);
		this->logarithm = logarithm;
	}

	void visit(Grid<Vector3f> &grid, size_t x, size_t y, size_t z,
			Vector3f &value) {
		float mag = std::sqrt(value.x * value.x + value.y * value.y + value.z
				* value.z);
		if (logarithm) {
			mag = std::log(mag + 1e-32);
		}
		outfile.write((char *) &mag, sizeof(float));
	}
};

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

int bfield(Arguments &arguments) {
	unsigned int bins = arguments.getInt("-bins", 10);
	std::cout << "Bins: " << bins << std::endl;

	float size = arguments.getFloat("-size", 10);
	std::cout << "Size: " << size << std::endl;

	std::string output = arguments.getString("-o", "bfield.raw");
	std::cout << "Output: " << output << std::endl;

	std::cout << "Create Grid" << std::endl;
	Grid<Vector3f> grid;
	grid.create(bins, size);
	Vector3f zero;
	zero.x = 0.0;
	zero.y = 0.0;
	zero.z = 0.0;
	grid.reset(zero);

	bool verbose = arguments.hasFlag("-v");

	std::vector<std::string> files;
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

		std::cerr << "Read POS block" << std::endl;
		std::vector<float> pos;
		if (file.readFloatBlock("POS ", pos) == false) {
			std::cerr << "Failed to read POS block" << std::endl;
			return 1;
		}

		std::cerr << "Read BFLD block" << std::endl;
		std::vector<float> bfld;
		if (file.readFloatBlock("BFLD", bfld) == false) {
			std::cerr << "Failed to read BFLD block" << std::endl;
			return 1;
		}

		std::cerr << "Read HSML block" << std::endl;
		std::vector<float> hsml;
		if (file.readFloatBlock("HSML", hsml) == false) {
			std::cerr << "Failed to read HSML block" << std::endl;
			return 1;
		}

		std::cout << "Fill grid" << std::endl;

#pragma omp parallel for schedule(dynamic, 10000)
		for (int iP = 0; iP < pn; iP++) {
			if (iP % (pn / 100) == 0 && verbose) {
				std::cerr << clock() << " ";
#ifdef _OPENMP
				std::cerr << "[" << omp_get_thread_num() << "] ";
#endif
				std::cerr << iP << " / " << pn << std::endl;
			}
			float sl = hsml[iP];
			float vol = M_PI / sl / sl / sl;
			float pX = pos[iP * 3];
			float pY = pos[iP * 3 + 1];
			float pZ = pos[iP * 3 + 2];
			float bX = bfld[iP * 3] * vol;
			float bY = bfld[iP * 3 + 1] * vol;
			float bZ = bfld[iP * 3 + 2] * vol;
			int steps = ceil(hsml[iP] * 2 / grid.getCellLength());
			int iX = pX / grid.getCellLength();
			int iY = pY / grid.getCellLength();
			int iZ = pZ / grid.getCellLength();
			for (int iStepX = -steps; iStepX <= steps; iStepX++) {
				float x = iStepX * grid.getCellLength() / sl;
				float vx = kernel(fabs(x)) * bX;
				for (int iStepY = -steps; iStepY <= steps; iStepY++) {
					float y = iStepY * grid.getCellLength() / sl;
					float vy = kernel(fabs(y)) * bY;
					for (int iStepZ = -steps; iStepZ <= steps; iStepZ++) {
						float z = iStepZ * grid.getCellLength() / sl;
						float vz = kernel(fabs(z)) * bZ;

						Vector3f &v = grid.get(iX + iStepX, iY + iStepY, iZ
								+ iStepZ);
#pragma omp critical
						{
							v.x += vx;
							v.y += vy;
							v.z += vz;
						}
					}
				}

			}
		}
	}

	if (arguments.hasFlag("-mag")) {
		std::cout << "Dump ZYX magnitude grid" << std::endl;
		DumpMagnitudeGridVisitor visitor(output, arguments.hasFlag("-log"));
		if (arguments.hasFlag("-zyx"))
			grid.acceptZYX(visitor);
		else
			grid.acceptXYZ(visitor);
	} else {
		DumpVectorGridVisitor visitor(output);
		if (arguments.hasFlag("-header")) {
			int intbins = bins;
			visitor.outfile.write((char *) &intbins, sizeof(intbins));
			visitor.outfile.write((char *) &intbins, sizeof(intbins));
			visitor.outfile.write((char *) &intbins, sizeof(intbins));
			double d = grid.getCellLength();
			visitor.outfile.write((char *) &d, sizeof(d));
			d = 0.0;
			visitor.outfile.write((char *) &d, sizeof(double));
			visitor.outfile.write((char *) &d, sizeof(double));
			visitor.outfile.write((char *) &d, sizeof(double));
		}
		if (arguments.hasFlag("-zyx")) {
			std::cout << "Dump ZYX vector grid" << std::endl;
			grid.acceptZYX(visitor);
		} else {
			std::cout << "Dump XYZ vector grid" << std::endl;
			grid.acceptXYZ(visitor);
		}
	}

	return 0;
}

struct GridCell {
	GridCell() :
		ref_count(0) {
	}
	std::vector<float> data;
	int64_t ref_count;
};

#if 0
void ranges(size_t &start, size_t &end, size_t n, float x, float h, float s) {
	int lowerX = floor((x - 2*h) / s);
	if (lowerX < 0)
	lowerX = 0;
	else if (lowerX > n)
	lowerX = n;
	int upperX = ceil((x + 2*h) / s);
	if (upperX < 0)
	upperX = 0;
	else if (upperX > n)
	upperX = n;
	start = lowerX;
	end = upperX;
}
void TFloatMagField::readCellData(GridCell &cell, const Vector3<float> &offset, std::ifstream &in, unsigned int count) const {
	Vector3<float> pos;
	Vector3<float> bfld;
	float hsml;
	for (size_t i = 0; i < count; i++) {
		in.read((char *)&pos.x, sizeof(float));
		in.read((char *)&pos.y, sizeof(float));
		in.read((char *)&pos.z, sizeof(float));
		in.read((char *)&bfld.x, sizeof(float));
		in.read((char *)&bfld.y, sizeof(float));
		in.read((char *)&bfld.z, sizeof(float));
		in.read((char *)&hsml, sizeof(float));
		float vol = M_PI / hsml / hsml / hsml;
		pos = pos - offset;
		bfld /= vol;
		size_t bX, eX, bY, eY, bZ, eZ;
		ranges(bX, eX, _fN, pos.x, hsml, _fStepsize);
		ranges(bY, eY, _fN, pos.y, hsml, _fStepsize);
		ranges(bZ, eZ, _fN, pos.z, hsml, _fStepsize);
		for (size_t x = bX; x < eX; x++) {
			float fx = x * _fStepsize - pos.x;
			float bx = kernel(fabs(fx)/hsml) * bfld.x;
			for (size_t y = bY; y < eY; y++) {
				float fy = y * _fStepsize - pos.y;
				float by = kernel(fabs(fy)/hsml) * bfld.y;
				for (size_t z = bZ; z < eZ; z++) {
					float fz = z * _fStepsize - pos.z;
					float bz = kernel(fabs(fz)/hsml) * bfld.z;
					size_t n = x*_fN*_fN + y*_fN + z;
					cell.data[n*3] += bx;
					cell.data[n*3+1] += by;
					cell.data[n*3+2] += bz;
				}
			}
		}
	}
}

#endif

class SPVisitor: public Grid<Vector3f>::Visitor {
public:
	SmoothParticle particle;
	size_t lastX, lastY, lastZ;
	Vector3f b;

	SPVisitor() :
		lastX(-1), lastY(-1), lastZ(-1) {
	}

	void visit(Grid<Vector3f> &grid, size_t x, size_t y, size_t z,
			Vector3f &value) {
		if (x != lastX) {
			b.x = kernel(particle.bfield.x, grid.toCellCenter(x),
					particle.position.x, particle.smoothingLength);
		}
		lastX = x;

		if (y != lastY) {
			b.y = kernel(particle.bfield.y, grid.toCellCenter(y),
					particle.position.y, particle.smoothingLength);
		}
		lastY = y;

		if (z != lastZ) {
			b.z = kernel(particle.bfield.z, grid.toCellCenter(z),
					particle.position.z, particle.smoothingLength);
		}
		lastZ = z;

#pragma omp critical
		value += b;
	}
};

class DumpBFieldGridVisitor: public Grid<Vector3f>::Visitor {
private:
	std::ostream &out;
public:

	DumpBFieldGridVisitor(std::ostream &out) :
		out(out) {
	}

	void visit(Grid<Vector3f> &grid, size_t x, size_t y, size_t z,
			Vector3f &value) {
		out.write((char *) &value.x, sizeof(float));
		out.write((char *) &value.y, sizeof(float));
		out.write((char *) &value.z, sizeof(float));
	}
};

void bfield_grid(std::vector<float> &pos, std::vector<float> &bfld,
		std::vector<float>&hsml, Vector3f &offset, Grid<Vector3f> &grid,
		size_t iP) {

}

int block(Arguments &arguments) {
	unsigned int bins = arguments.getInt("-bins", 100);
	std::cout << "Bins: " << bins << std::endl;

	float size = arguments.getFloat("-size", 40000);
	std::cout << "Size: " << size << std::endl;

	std::string output = arguments.getString("-o", "block");
	std::cout << "Output: " << output << std::endl;

	std::cout << "Create Grid" << std::endl;
	Grid<Vector3f> grid;
	grid.create(bins, size);
	grid.reset(Vector3f(0, 0, 0));

	std::cout << "Cell Size: " << grid.getCellLength() << std::endl;

	Vector3f offset(arguments.getFloat("-offX", 0), arguments.getFloat("-offY",
			0), arguments.getFloat("-offZ", 0));
	std::cout << "Offset: " << offset << std::endl;

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

		std::cerr << "  Read POS block" << std::endl;
		std::vector<float> pos;
		if (file.readFloatBlock("POS ", pos) == false) {
			std::cerr << "Failed to read POS block" << std::endl;
			return 1;
		}

		std::cerr << "  Read BFLD block" << std::endl;
		std::vector<float> bfld;
		if (file.readFloatBlock("BFLD", bfld) == false) {
			std::cerr << "Failed to read BFLD block" << std::endl;
			return 1;
		}

		std::cerr << "  Read HSML block" << std::endl;
		std::vector<float> hsml;
		if (file.readFloatBlock("HSML", hsml) == false) {
			std::cerr << "Failed to read HSML block" << std::endl;
			return 1;
		}

		time_t start = std::time(0);
#pragma omp parallel for schedule(dynamic, 10000)
		for (int iP = 0; iP < pn; iP++) {
			if (iP == 0) {
				std::cout << "  Fill grid";
#ifdef _OPENMP
				std::cout << " (" << omp_get_num_threads() << " threads)";
#endif
				std::cout << std::endl;

			}
			if (iP % (pn / 100) == 0 && verbose) {
				std::cout << "  ";

				std::cout << iP << ": " << (iP * 100) / pn << "%\r";
				std::cout.flush();
			}

			SPVisitor v;
			v.particle.smoothingLength = hsml[iP];
			v.particle.position.x = pos[iP * 3] - offset.x;
			v.particle.position.y = pos[iP * 3 + 1] - offset.y;
			v.particle.position.z = pos[iP * 3 + 2] - offset.z;

			float norm = 1.0 / M_PI / pow(v.particle.smoothingLength, 3);
			v.particle.bfield.x = bfld[iP * 3] * norm;
			v.particle.bfield.y = bfld[iP * 3 + 1] * norm;
			v.particle.bfield.z = bfld[iP * 3 + 2] * norm;

			AABC<float> aabc(v.particle.position, v.particle.smoothingLength
					* 2);
			grid.acceptZYX(v, aabc);
		}

		std::cout << "  Done                 " << std::endl;
	}

	std::cout << grid.get((size_t) bins / 2, (size_t) bins / 2, (size_t) bins
			/ 2) << std::endl;

	std::cout << "Write output" << std::endl;

	std::ofstream outfile;
	outfile.open(output.c_str(), std::ios::binary);
	outfile.write((const char *) &bins, sizeof(bins));
	outfile.write((const char *) &size, sizeof(size));
	outfile.write((const char *) &offset.x, sizeof(float));
	outfile.write((const char *) &offset.y, sizeof(float));
	outfile.write((const char *) &offset.z, sizeof(float));
	DumpBFieldGridVisitor visitor(outfile);
	grid.acceptZYX(visitor);

	return 0;
}

struct file_sphs {
	std::vector<float> pos;
	std::vector<float> bfld;
	std::vector<float> hsml;
};

void extract(file_sphs &fs, const std::string &filename) {
	std::cout << "Open " << filename << std::endl;

	GadgetFile file;
	file.open(filename);
	if (file.good() == false) {
		std::cerr << "Failed to open file " << filename << std::endl;
		return;
	}

	file.readHeader();
	int pn = file.getHeader().particleNumberList[0];
	std::cout << " Number of SmoothParticles: " << pn << std::endl;

	std::cout << " Read POS block" << std::endl;
	if (file.readFloatBlock("POS ", fs.pos) == false) {
		std::cerr << "Failed to read POS block" << std::endl;
		return;
	}

	std::cout << " Read BFLD block" << std::endl;
	if (file.readFloatBlock("BFLD", fs.bfld) == false) {
		std::cerr << "Failed to read BFLD block" << std::endl;
		return;
	}

	std::cout << " Read HSML block" << std::endl;
	if (file.readFloatBlock("HSML", fs.hsml) == false) {
		std::cerr << "Failed to read HSML block" << std::endl;
		return;
	}
}

void dump(const std::vector<file_sphs> &fs, size_t x, size_t y, size_t z,
		float cellSize, std::ostream &output) {
	std::cout << "dump " << x << "/" << y << "/" << z << std::endl;
	std::vector<std::vector<size_t> > indices;
	indices.resize(fs.size());

	// find intersecting particles
	Vector3<float> center(x * cellSize + cellSize / 2, y * cellSize + cellSize
			/ 2, z * cellSize + cellSize / 2);
	AABC<float> aabc(center, cellSize / 2);
	std::cout << " bounding box: " << aabc << std::endl;
	for (size_t iFile = 0; iFile < fs.size(); iFile++) {
		for (size_t iSPH = 0; iSPH < fs[iFile].pos.size() / 3; iSPH++) {
			Vector3<float> v(fs[iFile].pos[iSPH * 3], fs[iFile].pos[iSPH * 3
					+ 1], fs[iFile].pos[iSPH * 3 + 2]);
			float l = fs[iFile].hsml[iSPH];
			AABC<float> bb(v, l);
			if (aabc.intersects(bb)) {
				indices[iFile].push_back(iSPH);
			}
		}
	}

	std::cout << " write to file" << std::endl;

	// dump particles
	unsigned int tmp = x;
	output.write((const char *) &tmp, sizeof(unsigned int));
	tmp = y;
	output.write((const char *) &tmp, sizeof(unsigned int));
	tmp = z;
	output.write((const char *) &tmp, sizeof(unsigned int));

	unsigned int total = 0;
	for (size_t iFile = 0; iFile < fs.size(); iFile++) {
		total += indices[iFile].size();
	}
	std::cout << " " << x << "/" << y << "/" << z << ": " << total << std::endl;
	tmp = total;
	output.write((const char *) &tmp, sizeof(unsigned int));

	for (size_t iFile = 0; iFile < fs.size(); iFile++) {
		for (size_t iSPH = 0; iSPH < indices[iFile].size(); iSPH++) {
			size_t index = indices[iFile][iSPH];
			output.write((const char *) &fs[iFile].pos[index * 3],
					sizeof(float));
			output.write((const char *) &fs[iFile].pos[index * 3 + 1],
					sizeof(float));
			output.write((const char *) &fs[iFile].pos[index * 3 + 2],
					sizeof(float));
			output.write((const char *) &fs[iFile].bfld[index * 3],
					sizeof(float));
			output.write((const char *) &fs[iFile].bfld[index * 3 + 1],
					sizeof(float));
			output.write((const char *) &fs[iFile].bfld[index * 3 + 2],
					sizeof(float));
			output.write((const char *) &fs[iFile].hsml[index], sizeof(float));
		}
	}

}

int pp(Arguments &arguments) {
	std::string filename = arguments.getString("-o", "prepared.sph");
	std::cout << "Output: " << filename << std::endl;

	std::ofstream output(filename.c_str());

	unsigned int bins = arguments.getInt("-bins", 10);
	std::cout << "Bins: " << bins << std::endl;

	float size = arguments.getFloat("-size", 10);
	std::cout << "Size: " << size << std::endl;

	output.write((const char *) &size, sizeof(float));
	output.write((const char *) &bins, sizeof(unsigned int));

	std::vector<std::string> files;
	arguments.getVector("-f", files);

	std::vector<file_sphs> fs;
	fs.resize(files.size());
	for (size_t iFile = 0; iFile < files.size(); iFile++) {
		extract(fs[iFile], files[iFile]);
	}

	for (size_t x = 0; x < bins; x++) {
		for (size_t y = 0; y < bins; y++) {
			for (size_t z = 0; z < bins; z++) {
				dump(fs, x, y, z, size / bins, output);
			}
		}
	}

	return 0;
}

int main(int argc, const char **argv) {
	try {
		Arguments arguments(argc, argv);
		if (arguments.getCount() < 2) {
			std::cout << "Functions:" << std::endl;
			std::cout << "  mass        mass grid" << std::endl;
			std::cout << "  bfield      bfield grid" << std::endl;
			std::cout << "  av          average bfield" << std::endl;
			std::cout << "  pp          preprocess for use in CRPRopa"
					<< std::endl;
			std::cout << "  writetest   grid write test" << std::endl;
			std::cout << "  readtest    grid read test" << std::endl;
			return 1;
		}

		std::string function = argv[1];
		if (function == "mass")
			return mass(arguments);
		else if (function == "bfield")
			return bfield(arguments);
		else if (function == "av")
			return av(argc, argv);
		else if (function == "pp")
			return pp(arguments);
		else if (function == "pg")
			return paged_grid(arguments);
		else if (function == "block")
			return block(arguments);
		else if (function == "writetest") {
			if (arguments.hasFlag("-float")) {
				Grid<float> fg;
				fg.create(2, 1.0);
				fg.save("ls_float.dat");
			} else if (arguments.hasFlag("-vector")) {
				Grid<Vector3<float> > fg;
				fg.create(2, 1.0);
				fg.save("ls_vector.dat");
			}
		} else if (function == "readtest") {
			if (arguments.hasFlag("-float")) {
				Grid<float> fg;
				fg.load("ls_float.dat");
				std::cout << "Bins: " << fg.getBins() << std::endl;
				std::cout << "Size: " << fg.getSize() << std::endl;
			} else if (arguments.hasFlag("-vector")) {
				Grid<Vector3<float> > fg;
				fg.load("ls_vector.dat");
				std::cout << "Bins: " << fg.getBins() << std::endl;
				std::cout << "Size: " << fg.getSize() << std::endl;
			}
		}
	} catch (std::exception &e) {
		std::cerr << "EXCEPTION: " << e.what() << std::endl;
	}
	return 0;
}

