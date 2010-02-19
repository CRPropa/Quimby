/*
 * test.cpp
 *
 *  Created on: 29.01.2010
 *      Author: gmueller
 */

#include "GadgetFile.hpp"
#include "Grid.hpp"
#include "kernel.hpp"
#include "Octree.hpp"

#include <cmath>
#include <ctime>
#include <cstdlib>
#include <string>

#include <omp.h>

class Arguments {
	int argc;
	const char **argv;
public:
	Arguments(int c, const char **v) :
		argc(c), argv(v) {

	}

	int getCount() {
		return argc;
	}

	bool hasFlag(const std::string &flag) {
		int i;
		for (i = 0; i < argc; i++) {
			if (flag == argv[i])
				return true;
		}

		return false;
	}

	int getInt(const std::string &flag, int def) {
		int i;
		for (i = 0; i < argc; i++) {
			if (flag == argv[i] && (i + 1 < argc))
				return atoi(argv[i + 1]);
		}

		return def;
	}

	int getFloat(const std::string &flag, float def) {
		int i;
		for (i = 0; i < argc; i++) {
			if (flag == argv[i] && (i + 1 < argc))
				return atof(argv[i + 1]);
		}

		return def;
	}
	std::string getString(const std::string &flag) {
		int i;
		for (i = 0; i < argc; i++) {
			if (flag == argv[i] && (i + 1 < argc))
				return std::string(argv[i + 1]);
		}

		return std::string();
	}

	void getVector(const std::string &flag, std::vector<std::string> &v) {
		int i;

		// find flag
		for (i = 0; i < argc; i++) {
			if (flag == argv[i])
				break;
		}

		for (i++; i < argc; i++) {
			if (argv[i][0] != '-')
				v.push_back(argv[i]);
		}

	}

};

#ifdef DEBUG_OCTREE
OctreeNode *root = new OctreeNode(0, AABB(Vector3(0.0, 0.0, 0.0), Vector3(
						1000.0, 1000.0, 1000.0)));
SPH *sphs = new SPH[10000000];
for (int i = 0; i < 10000000; i++) {
	sphs[i].position.x = (float) rand() * 1000.0 / (float) RAND_MAX;
	sphs[i].position.y = (float) rand() * 1000.0 / (float) RAND_MAX;
	sphs[i].position.z = (float) rand() * 1000.0 / (float) RAND_MAX;
	sphs[i].smoothingLength = (float) rand() * 10.0 / (float) RAND_MAX;
	Vector3 min = sphs[i].position + Vector3(sphs[i].smoothingLength,
			sphs[i].smoothingLength, sphs[i].smoothingLength);
	Vector3 max = sphs[i].position + Vector3(sphs[i].smoothingLength,
			sphs[i].smoothingLength, sphs[i].smoothingLength);
	root->insert(&sphs[i], AABB(min, max));
}
{
	// add special node
	SPH *sph = new SPH();
	sph->position.x = 350;
	sph->position.y = 350;
	sph->position.z = 350;
	sph->smoothingLength = 10;
	Vector3 min = sph->position - Vector3(sph->smoothingLength,
			sph->smoothingLength, sph->smoothingLength);
	Vector3 max = sph->position + Vector3(sph->smoothingLength,
			sph->smoothingLength, sph->smoothingLength);
	root->insert(sph, AABB(min, max));
}
NullQueryCallback pc = NullQueryCallback();
std::cout << "start queries..." << std::endl;
clock_t start = clock();
for (int x = 0; x < 100; x++)
for (int y = 0; y < 100; y++)
for (int z = 0; z < 100; z++)
root->query(
		pc,
		AABB(Vector3(10.0 * x, 10.0 * y, 10.0 * z), Vector3(
						10.0 * (x + 1), 10.0 * (y + 1), 10.0 * (z + 1))));
clock_t end = clock();
std::cout << ((end - start) / (float) CLOCKS_PER_SEC / 100 / 100 / 100)
<< std::endl;

PrintQueryCallback ppc = PrintQueryCallback();
root->query(ppc, Vector3(355.0, 349.0, 346.0));

#endif

//		OctreeNode *root = new OctreeNode(0, AABB(Vector3(0.0, 0.0, 0.0), Vector3(
//				1000.0, 1000.0, 1000.0)));
//		SPH *sph = new SPH();
//		sph->position.x = 350;
//		sph->position.y = 350;
//		sph->position.z = 350;
//		sph->smoothingLength = 10;
//		Vector3 min = sph->position + Vector3(sph->smoothingLength,
//				sph->smoothingLength, sph->smoothingLength);
//		Vector3 max = sph->position + Vector3(sph->smoothingLength,
//				sph->smoothingLength, sph->smoothingLength);
//		root->insert(sph, AABB(min, max));
//
//		PrintQueryCallback pc = PrintQueryCallback();
//		root->query(
//							pc,
//							Vector3(10.0 * x, 10.0 * y, 10.0 * z));


#if 0
Grid3D grid;
grid.create(50, 240000.0);

if (argc < 2) {
	printf("Please give a filename ...\n");
	exit(4);
}

for (int iArg = 1; iArg < argc; iArg++) {
	if (!(fd = fopen(argv[iArg], "r"))) {
		std::cerr << "Cant open file " << argv[iArg] << std::endl;
		continue;
	} else {
		std::cerr << "Opened file " << argv[iArg] << std::endl;
	}

	/*----------- RED HEADER TO GET GLOBAL PROPERTIES -------------*/
	n = read_gadget_head(npart, masstab, &time, &redshift, fd);

	ntot = 0;
	for (i = 0; i < 6; i++) {
		//printf("PartSpezies %d, anz=%d, masstab=%f\n", i, npart[i],
		//masstab[i]);
		ntot += npart[i];
	}
	//printf("Time of snapshot=%f, z=%f, ntot=%d\n", time, redshift, ntot);

	/*---------- ALLOCATE MEMORY ---------------------------------*/
	rho = (float *) malloc(npart[0] * sizeof(float));
	b = (vector *) malloc(3 * npart[0] * sizeof(float));
	pos = (vector *) malloc(3 * ntot * sizeof(float));

	/*---------- READ DATA BLOCKS --------------------------------*/
	n = read_gadget_float(rho, "RHO ", fd);
	n = read_gadget_float3((float*) pos, "POS ", fd);
	n = read_gadget_float3((float*) b, "BFLD", fd);

	for (i = 0; i < npart[0]; i++) {
		if (pos[i].x < minpos.x)
		minpos.x = pos[i].x;
		if (pos[i].y < minpos.y)
		minpos.y = pos[i].y;
		if (pos[i].z < minpos.z)
		minpos.z = pos[i].z;

		if (pos[i].x > maxpos.x)
		maxpos.x = pos[i].x;
		if (pos[i].y > maxpos.y)
		maxpos.y = pos[i].y;
		if (pos[i].z > maxpos.z)
		maxpos.z = pos[i].z;

		vector *g = grid.get(&pos[i]);
		g->x += b[i].x;
		g->y += b[i].y;
		g->z += b[i].z;
		if ((i % 1000) == 0)
		std::cerr << i << std::endl;
	}

	/*---------- CLOSE FILE AND FREE DATA ------------------------*/
	fclose(fd);
	free(rho);
	free(b);
	free(pos);

}

grid.print();

std::cerr << "Min: " << minpos.x << ", " << minpos.y << ", " << minpos.z
<< std::endl;
std::cerr << "Max: " << maxpos.x << ", " << maxpos.y << ", " << maxpos.z
<< std::endl;
#endif

#if 0
FILE *fd = 0;
int i, n, ntot;
int npart[6];
double masstab[6], redshift, time;
float *rho;
struct vector *pos, *b;
vector minpos, maxpos;

if (argc < 2) {
	printf("Please give a filename ...\n");
	exit(4);
}

for (int iArg = 1; iArg < argc; iArg++) {
	if (!(fd = fopen(argv[iArg], "r"))) {
		std::cerr << "Cant open file " << argv[iArg] << std::endl;
		continue;
	} else {
		std::cerr << "Opened file " << argv[iArg] << std::endl;
	}

	/*----------- RED HEADER TO GET GLOBAL PROPERTIES -------------*/
	n = read_gadget_head(npart, masstab, &time, &redshift, fd);

	ntot = 0;
	for (i = 0; i < 6; i++) {
		//printf("PartSpezies %d, anz=%d, masstab=%f\n", i, npart[i],
		//masstab[i]);
		ntot += npart[i];
	}
	//printf("Time of snapshot=%f, z=%f, ntot=%d\n", time, redshift, ntot);

	/*---------- ALLOCATE MEMORY ---------------------------------*/
	rho = (float *) malloc(npart[0] * sizeof(float));
	b = (vector *) malloc(3 * npart[0] * sizeof(float));
	pos = (vector *) malloc(3 * ntot * sizeof(float));

	/*---------- READ DATA BLOCKS --------------------------------*/
	n = read_gadget_float(rho, "HSML", fd);
	n = read_gadget_float3((float*) pos, "POS ", fd);
	n = read_gadget_float3((float*) b, "BFLD", fd);

	std::cout << pos[1500].x << std::endl;
	std::cout << pos[1500].y << std::endl;
	std::cout << pos[1500].z << std::endl;

	std::cout << b[1500].x << std::endl;
	std::cout << b[1500].y << std::endl;
	std::cout << b[1500].z << std::endl;

	std::cout << rho[1500] << std::endl;

	/*---------- CLOSE FILE AND FREE DATA ------------------------*/
	fclose(fd);
	free(rho);
	free(b);
	free(pos);

}

exit(0);
#endif

//class AbstractProcessor {
//	std::vector<Vector3f> pos;
//	std::vector<float> rho;
//	std::vector<Vector3f> bfield;
//	std::vector<float> hsml;
//	void init(Arguments &arguments);
//};
int mass(Arguments &arguments) {
	unsigned int bins = arguments.getInt("-bins", 10);
	std::cout << "Bins: " << bins << std::endl;

	float size = arguments.getFloat("-size", 10);
	std::cout << "Size: " << size << std::endl;

	std::string output = arguments.getString("-o");
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
						float w = kernel1d(r / hsml[iP]) * rho[iP] * vol;
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

	void visit(const size_t &x, const size_t &y, const size_t &z,
			const Vector3f &value) {
		float mag = sqrt(value.x * value.x + value.y * value.y + value.z
				* value.z);
		if (logarithm) {
			mag = log(mag + 1e-32);
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

	void visit(const size_t &x, const size_t &y, const size_t &z,
			const Vector3f &value) {
		outfile.write((char *) &value, sizeof(Vector3f));
	}
};

int bfield(Arguments &arguments) {
	unsigned int bins = arguments.getInt("-bins", 10);
	std::cout << "Bins: " << bins << std::endl;

	float size = arguments.getFloat("-size", 10);
	std::cout << "Size: " << size << std::endl;

	std::string output = arguments.getString("-o");
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
				float vx = kernel1d(fabs(x)) * bX;
				for (int iStepY = -steps; iStepY <= steps; iStepY++) {
					float y = iStepY * grid.getCellLength() / sl;
					float vy = kernel1d(fabs(y)) * bY;
					for (int iStepZ = -steps; iStepZ <= steps; iStepZ++) {
						float z = iStepZ * grid.getCellLength() / sl;
						float vz = kernel1d(fabs(z)) * bZ;

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

int main(int argc, const char **argv) {
	Arguments arguments(argc, argv);
	if (arguments.getCount() < 2) {
		std::cout << "Functions:" << std::endl;
		std::cout << "  mass        mass grid" << std::endl;
		std::cout << "  bfield      bfield grid" << std::endl;
		std::cout << "  av          average bfield" << std::endl;
		return 1;
	}

	std::string function = argv[1];
	if (function == "mass")
		return mass(arguments);
	else if (function == "bfield")
		return bfield(arguments);
	else if (function == "av")
		return av(argc, argv);
	else if (function == "test")
		return test(arguments);
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
	}
	else if (function == "readtest") {
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

	return 0;
}

