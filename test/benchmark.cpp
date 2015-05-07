/*
 * benchmark.cpp
 *
 *  Created on: 04.05.2015
 *      Author: gmueller
 */

#include <iostream>
#include <sstream>

#include <sys/stat.h>
#include <sys/time.h>
#include <omp.h>

#include <quimby/Database.h>
#include <quimby/Grid.h>
#include <quimby/HCube.h>
#include <quimby/tga.h>

using namespace quimby;
using namespace std;

bool check_file(const std::string filename) {
	struct stat s;
	int r = stat(filename.c_str(), &s);
	return ((r == 0) && S_ISREG(s.st_mode) && (s.st_size > 0));
}

class Timer {
	struct timeval start;
public:
	Timer() {
		reset();
	}

	void reset() {
		::gettimeofday(&start, NULL);
	}

	size_t microns() {
		struct timeval now;
		::gettimeofday(&now, NULL);
		size_t ms = (now.tv_sec - start.tv_sec) * 1000000;
		ms += (now.tv_usec - start.tv_usec);
		return ms;

	}

	size_t millisecods() {
		return microns() / 1000;
	}

	double seconds() {
		struct timeval now;
		::gettimeofday(&now, NULL);
		return double(now.tv_sec - start.tv_sec)
				+ double(now.tv_usec - start.tv_usec) / 1000000;
	}

};

class Accessor {
public:
	virtual Vector3f get(const Vector3f &pos) {
		return Vector3f(0, 0, 0);
	}
};

class GridAccessor: public Accessor {
	Vector3f origin;
	Grid<Vector3f> &grid;
public:
	GridAccessor(const Vector3f &origin, Grid<Vector3f> &grid) :
			origin(origin), grid(grid) {

	}

	Vector3f get(const Vector3f &pos) {
		Vector3f r = pos - origin;
		return grid.get(r.x, r.y, r.z);
	}
};

template<size_t N>
class HCubeAccessor: public Accessor {
	Vector3f origin;
	float size;
	const HCube<N> *hcube;
public:
	HCubeAccessor(const Vector3f &origin, float size, const HCube<N> *hcube) :
			origin(origin), size(size), hcube(hcube) {

	}

	Vector3f get(const Vector3f &pos) {
		Vector3f r = pos - origin;
		return hcube->getValue(r, size);
	}
};

class RandomAccessBenchmark {
	Vector3f origin;
	float size;
public:
	RandomAccessBenchmark(Vector3f origin, float size) :
			origin(origin), size(size) {

	}
	double run(Accessor &accessor, size_t threads = 1,
			size_t samples = 10000000) {
		Timer timer;

		omp_set_num_threads(threads);

#pragma omp parallel
		{
			int seed = 1202107158 + omp_get_thread_num() * 1999;
			struct drand48_data drand_buf;
			srand48_r(seed, &drand_buf);

			float s = size * (1 - 1e-6);
#pragma omp parallel for
			for (size_t i = 0; i < samples; i++) {
				double x, y, z;
				drand48_r(&drand_buf, &x);
				drand48_r(&drand_buf, &y);
				drand48_r(&drand_buf, &z);
				accessor.get(origin + Vector3f(x, y, z) * s);
			}

		}
		return timer.seconds();
	}

};

class RandomWalkBenchmark {
	Vector3f origin;
	float size;
public:
	RandomWalkBenchmark(Vector3f origin, float size) :
			origin(origin), size(size) {

	}
	double run(Accessor &accessor, size_t threads = 1,
			size_t samples = 10000000) {
		Timer timer;

		omp_set_num_threads(threads);

		float step = size / 1000.;
		Vector3f end = origin + Vector3f(size, size, size) * (1 - 1e-6);

#pragma omp parallel
		{
			int seed = 1202107158 + omp_get_thread_num() * 1999;
			struct drand48_data drand_buf;
			srand48_r(seed, &drand_buf);

			Vector3f pos = origin + Vector3f(size / 2., size / 2., size / 2.);

#pragma omp parallel for
			for (size_t i = 0; i < samples; i++) {
				double x, y, z;
				drand48_r(&drand_buf, &x);
				drand48_r(&drand_buf, &y);
				drand48_r(&drand_buf, &z);
				pos += Vector3f(x-0.5, y-0.5, z-0.5) * step;
				pos.setUpper(origin);
				pos.setLower(end);
				accessor.get(pos);
				//cout << pos << endl;
			}

		}
		return timer.seconds();
	}

};

void print_slice(Accessor &accessor, size_t bins, size_t z,
		const Vector3f &origin, float size, const string &filename) {
	const float rl = 1e-15;
	const float ru = 1e-5;
	ofstream tgafile(filename.c_str());
	tga_write_header(tgafile, bins, bins);
	for (size_t x = 0; x < bins; x++) {
		for (size_t y = 0; y < bins; y++) {
			float value = accessor.get(
					origin + Vector3f(x, y, z) * (size / bins)).length();
			// normalize
			value = (log10(value) - log10(rl)) / (log10(ru) - log10(rl));
			tga_write_float(tgafile, value);
		}
	}

}

int main(int argc, const char **argv) {
	if (argc < 3) {
		cout << "benchmark <comaprofile.db> <bins>" << endl;
		return 1;
	}

	size_t bins = atoi(argv[2]);

	cout << "load coma db" << endl;
	FileDatabase fdb;
	fdb.open(argv[1]);

	cout << "  particles: " << fdb.getCount() << endl;
	cout << "  lower: " << fdb.getLowerBounds() << endl;
	cout << "  upper: " << fdb.getUpperBounds() << endl;

	Vector3f origin = fdb.getLowerBounds();

	Vector3f size3 = fdb.getUpperBounds() - fdb.getLowerBounds();
	float size = std::min(std::min(size3.x, size3.y), size3.z);
	cout << "  size: " << size << endl;


	cout << "create sample" << endl;
	Grid<Vector3f> grid(bins, size);

	stringstream gridfilename;
	gridfilename << "benchmark-" << bins << "-grid.grid";

	if (check_file(gridfilename.str())) {
		grid.load(gridfilename.str());

	} else {
		grid.reset(Vector3f(0, 0, 0));

		cout << "create visitor" << endl;
		SimpleSamplingVisitor v(grid, fdb.getLowerBounds(), size);
		v.showProgress(true);
		cout << "start sampling" << endl;
		fdb.accept(v);
		cout << endl;

		grid.save(gridfilename.str());
	}

	GridAccessor gridaccessor(fdb.getLowerBounds(), grid);

	cout << "print slices" << endl;
	for (size_t z = 0; z < bins; z += bins / 16) {
		stringstream filename;
		filename << "benchmark-" << bins << "-slice-grid-" << z << ".tga";
		print_slice(gridaccessor, bins, z, origin, size, filename.str());
	}

	size_t maxdepth = 0;

	while (pow(4, maxdepth + 1) < bins)
		maxdepth++;

	stringstream hcubefilename;
	hcubefilename << "benchmark-" << bins << "-hcube.hc4";
	if (!check_file(hcubefilename.str())) {
		cout << "create hc4, maxdepth=" << maxdepth << endl;

		HCubeFile4::create(grid, Vector3f(0, 0, 0), size, 0.1, 1e-19, maxdepth,
				hcubefilename.str());
	}

	cout << "open hc4" << endl;
	HCubeFile4 hcf;
	hcf.open(hcubefilename.str());
	const HCube4 *hcube = hcf.hcube();

	HCubeAccessor<4> hcubeaccessor(origin, size, hcube);

	cout << "print hcube slices" << endl;
	for (size_t z = 0; z < bins; z += bins / 16) {
		stringstream filename;
		filename << "benchmark-" << bins << "-slice-hcube-" << z << ".tga";
		print_slice(hcubeaccessor, bins, z, origin, size, filename.str());
	}


	RandomAccessBenchmark rab(origin, size);
	RandomWalkBenchmark rwb(origin, size);
	Accessor nullaccessor;

//ToDo: zorder
//ToDo: mmap grid
	cout << "accessor benchmark threads time" << endl;
	const size_t samples = 3;
	size_t max_threads = omp_get_max_threads();
	for (size_t threads = 1; threads <= max_threads; threads++) {

		for (size_t i = 0; i < samples; i++)
			cout << "0 0 " << threads << " " << rab.run(nullaccessor, threads) << endl;

		for (size_t i = 0; i < samples; i++)
			cout << "1 0 " << threads << " " << rab.run(gridaccessor, threads) << endl;

		for (size_t i = 0; i < samples; i++)
			cout << "2 0 " << threads << " " << rab.run(hcubeaccessor, threads) << endl;

		for (size_t i = 0; i < samples; i++)
			cout << "0 1 " << threads << " " << rwb.run(nullaccessor, threads) << endl;

		for (size_t i = 0; i < samples; i++)
			cout << "1 1 " << threads << " " << rwb.run(gridaccessor, threads) << endl;

		for (size_t i = 0; i < samples; i++)
			cout << "2 1 " << threads << " " << rwb.run(hcubeaccessor, threads) << endl;

	}

	cout << "done" << endl;
}
