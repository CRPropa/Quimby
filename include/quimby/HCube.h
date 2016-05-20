#pragma once

#include "Vector3.h"
#include "Database.h"
#include "MagneticField.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

namespace quimby {

class invalid_position : std::exception {
public:
    virtual const char* what() const throw() {
        return "Invalid position.";
    }
};

struct HCubeInitFlags {
	HCubeInitFlags() : presampling(1024) {
	}
	Vector3f offsetKpc;
	float sizeKpc;
	float error;
	float threshold;
	size_t maxdepth;
	size_t target_depth;
	size_t presampling;
};

class HCubeInitCheckpoint {
	typedef std::pair<Vector3f, float> id_t;
	typedef std::map< id_t, size_t > map_t;
	typedef map_t::iterator iter_t;
	map_t indices;
	
	std::ofstream outfile;

public:
	
	
	HCubeInitCheckpoint(const std::string &filename) {
		init(filename);
	}
	
	void init(const std::string &filename) {
		// load idx pairs
		std::ifstream infile(filename.c_str(), std::ios::binary);

		while (infile.good()) {
			Vector3f v;
			float s;
			size_t e;
			infile.read((char *)&v, sizeof(Vector3f));
			infile.read((char *)&s, sizeof(float));
			infile.read((char *)&e, sizeof(size_t));
			if (infile) {
				indices[std::make_pair(v, s)] = e;
			}
		}
		infile.ignore(std::numeric_limits < std::streamsize > ::max(), '\n');
		infile.close();
	
		// truncate invalid pairs

		// prepare write
		outfile.open(filename.c_str(), std::ios::app | std::ios::binary);
	}
	
	size_t end(const Vector3f &v, float s) {
		iter_t i = indices.find(std::make_pair(v, s));
		if (i != indices.end())
			return i->second;
		else
			return 0;
	}
	
	void done(Vector3f v, float s, size_t end) {
		outfile.write((char*)&v, sizeof(Vector3f));
		outfile.write((char*)&s, sizeof(float));
		outfile.write((char*)&end, sizeof(size_t));
		outfile.flush();
	}
	
	bool empty() {
		return (indices.size() == 0);
	}
};

template<size_t N>
class HCube {
	Vector3f elements[N * N * N];

public:
	void init(MagneticField *field, const Vector3f &offsetKpc, float sizeKpc,
			size_t depth, size_t &idx, const HCubeInitFlags &flags) {
		const size_t N3 = N * N * N;
		const size_t N2 = N * N;

		const float s = sizeKpc / N;
		const Vector3f o = offsetKpc + Vector3f(s / 2.);
		
        size_t thisidx = idx;
        idx++;

		if (depth == flags.maxdepth) {
			for (size_t iX = 0; iX < N; iX++) {
				for (size_t iY = 0; iY < N; iY++) {
					for (size_t iZ = 0; iZ < N; iZ++) {
						size_t idx_target = iX * N2 + iY * N + iZ;
						field->getField(o + Vector3f(iX *s, iY * s, iZ * s), elements[idx_target]);
					}
				}
			}
    	} else {
			for (size_t n = 0; n < N3; n++) {
				if (depth == 0)
					std::cout << "\n " << n;
				else if(depth == 1)
					std::cout << "." << n;
				std::cout.flush();
				HCube<N> *hc = this + (idx - thisidx);
				size_t i = n / N2;
				size_t j = (n % N2) / N;
				size_t k = n % N;
				size_t tmpidx = idx;
				hc->init(field, offsetKpc + Vector3f(i * s, j * s, k * s), s,
						depth + 1, tmpidx, flags);
				Vector3f mean;
				if (hc->collapse(mean, flags.error, flags.threshold)) {
					setValue(i, j, k, mean);
				} else {
					setCube(i, j, k, idx - thisidx);
					idx = tmpidx;
				}
			}

		}
	}

	
	void init(Database *db, const Vector3f &offsetKpc, float sizeKpc,
			size_t depth, size_t &idx, const HCubeInitFlags &flags, HCubeInitCheckpoint &checkpoint) {
		const size_t N3 = N * N * N;
		const size_t N2 = N * N;

		size_t start_idx = idx;
#ifdef DEBUG
		std:: cout << "Start " << offsetKpc << ", " << sizeKpc << "." << std::endl;
#endif
		size_t end_idx = checkpoint.end(offsetKpc, sizeKpc);
		if (end_idx) {
			std:: cout << "Skipping " << offsetKpc << ", " << sizeKpc << " -> " << end_idx << "." << std::endl;
			idx = end_idx;
			return;
		}
		
		size_t desired_depth = int(log2(flags.presampling) / log2(N));
		size_t remaining_depth = 1 + flags.maxdepth - depth;
		//std::cout << "init: desired_depth=" << desired_depth << ", remaining_depth=" << remaining_depth << ", offset="<< offsetKpc << ", idx=" << idx << std::endl;
		if (remaining_depth <= desired_depth) {
			size_t sample_depth = std::min(remaining_depth, desired_depth);
			size_t starting_depth = std::max(flags.maxdepth - sample_depth, 0ul);

			//std::cout << "Sample. N=" << N << ", depth=" << depth << ", remaining_depth=" << remaining_depth << std::endl;
			// sample data with max depth resolution
			size_t n = N;
			for (size_t i = 1; i < sample_depth; i++)
				n *= N;
			size_t n3 = n*n*n;
			Vector3f *data = new Vector3f[n3];

			// zero
			memset(data, 0, sizeof(Vector3f) * n3);

			// create visitor
			Vector3f lower = offsetKpc;
			//Vector3f upper = lower + Vector3f(sizeKpc, sizeKpc, sizeKpc);

			//std::cout << "Sample: " << lower << " - " << sizeKpc << std::endl;
			SimpleSamplingVisitor v(data, n, lower, sizeKpc);
			//v.showProgress(true);

			db->accept(v);

			// call this->init with sampled data
			init(data, n, sizeKpc, Vector3f(0, 0, 0), sizeKpc, depth, idx, flags);

			delete[] data;
		} else {
			const float s = sizeKpc / N;
			size_t thisidx = idx;
			idx++;
			for (size_t n = 0; n < N3; n++) {
				if (depth == 0)
					std::cout << "\n " << n;
				else if(depth == 1)
					std::cout << "." << n;
				std::cout.flush();
				HCube<N> *hc = this + (idx - thisidx);
				size_t i = n / N2;
				size_t j = (n % N2) / N;
				size_t k = n % N;
				size_t tmpidx = idx;
				hc->init(db, offsetKpc + Vector3f(i * s, j * s, k * s), s,
						depth + 1, tmpidx, flags, checkpoint);
				Vector3f mean;
				if (hc->collapse(mean, flags.error, flags.threshold) || (depth >= flags.target_depth)) {
					setValue(i, j, k, mean);
				} else {
					setCube(i, j, k, idx - thisidx);
					idx = tmpidx;
				}
			}

		}
		
		checkpoint.done(offsetKpc, sizeKpc, idx);
#ifdef DEBUG
		std:: cout << "Done " << offsetKpc << ", " << sizeKpc << " -> " << idx << "." << std::endl;
#endif

	}

	void init(const Vector3f *data, size_t dataN, float dataSize,
			const Vector3f &offsetKpc, float sizeKpc, size_t depth, size_t &idx, const HCubeInitFlags &flags) {
		const float s = sizeKpc / N;
		const size_t N3 = N * N * N;
		const size_t N2 = N * N;
		size_t thisidx = idx;
		idx++;
		if (depth == flags.maxdepth) {
			//std::cout << "init data final" << std::endl;
			float dataStep = dataSize / dataN;
			size_t oX = offsetKpc.x / dataStep;
			size_t oY = offsetKpc.y / dataStep;
			size_t oZ = offsetKpc.z / dataStep;
			const size_t dataN2 = dataN * dataN;
			for (size_t iX = 0; iX < N; iX++) {
				for (size_t iY = 0; iY < N; iY++) {
					for (size_t iZ = 0; iZ < N; iZ++) {
						size_t idx_data = (iX + oX) * dataN2 + (iY + oY) * dataN + (iZ + oZ);
						size_t idx_target = iX * N2 + iY * N + iZ;
						elements[idx_target] = data[idx_data];
					}
				}
			}
		} else {

			for (size_t n = 0; n < N3; n++) {
				HCube<N> *hc = this + (idx - thisidx);
				size_t i = n / N2;
				size_t j = (n % N2) / N;
				size_t k = n % N;
				size_t tmpidx = idx;
				hc->init(data, dataN, dataSize,
						offsetKpc + Vector3f(i * s, j * s, k * s), s, depth + 1, tmpidx, flags);
				Vector3f mean;
				if (hc->collapse(mean, flags.error, flags.threshold) || (depth >= flags.target_depth)) {
					setValue(i, j, k, mean);
				} else {
					setCube(i, j, k, idx - thisidx);
					idx = tmpidx;
				}
			}

		}
	}

	void load(std::istream &in, size_t dataN, float dataSize,
			const Vector3f &offsetKpc, float sizeKpc, float threshold) {
		const float s = sizeKpc / N;
		const size_t N3 = N * N * N;
		const size_t N2 = N * N;

		float dataStep = dataSize / dataN;
		size_t oX = offsetKpc.x / dataStep;
		size_t oY = offsetKpc.y / dataStep;
		size_t oZ = offsetKpc.z / dataStep;
		const size_t dataN2 = dataN * dataN;
		for (size_t iX = 0; iX < N; iX++) {
			for (size_t iY = 0; iY < N; iY++) {
				size_t offsetN = (iX + oX) * dataN2 + (iY + oY) * dataN + oZ;
				in.seekg(offsetN * sizeof(Vector3f), std::ios::beg);
				in.read((char *) (elements + (iX * N2 + iY * N)),
						N * sizeof(Vector3f));
			}
		}
//		const float t2 = threshold * threshold;
//		for (size_t n = 0; n < N3; n++) {
//			if (elements[n].length2() < t2)
//				elements[n] = Vector3f(0, 0, 0);
//		}
	}

	void init(std::istream &in, size_t dataN, float dataSize,
			const Vector3f &offsetKpc, float sizeKpc, size_t depth, size_t &idx, const HCubeInitFlags &flags) {
		const float s = sizeKpc / N;
		const size_t N3 = N * N * N;
		const size_t N2 = N * N;
		size_t thisidx = idx;
		idx++;
		if (depth == flags.maxdepth) {
			load(in, dataN, dataSize, offsetKpc, sizeKpc, flags.threshold);
		} else {
			for (size_t n = 0; n < N3; n++) {
				if (depth == 0) {
					std::cout << (n + 1) << "/" << N3 << std::endl;
				} else if (depth == 1) {
					if (n && (n % N == 0)) {
						std::cout << ".";
						if (n % N2 == 0)
							std::cout << std::endl;
						else
							std::cout.flush();
					}
				}
				HCube<N> *hc = this + (idx - thisidx);
				size_t i = n / N2;
				size_t j = (n % N2) / N;
				size_t k = n % N;
				size_t tmpidx = idx;
				hc->init(in, dataN, dataSize,
						offsetKpc + Vector3f(i * s, j * s, k * s), s, depth + 1, tmpidx, flags);
				Vector3f mean;
				if (hc->collapse(mean, flags.error, flags.threshold)) {
					setValue(i, j, k, mean);
				} else {
					setCube(i, j, k, idx - thisidx);
					idx = tmpidx;
				}
			}
			if (depth == 1) {
				std::cout << "." << std::endl;
			}
		}
	}
#if 0
	static void create(HCube<N> &hc, std::ostream &out, std::istream &in,
			size_t dataN, float dataSize, const Vector3f &offsetKpc,
			float sizeKpc, float error, float threshold, size_t maxdepth,
			size_t depth, size_t &idx) {
		const float s = sizeKpc / N;
		const size_t N3 = N * N * N;
		const size_t N2 = N * N;
		HCube<N> thishc;
		size_t thisidx = idx;
		idx++;
		for (size_t n = 0; n < N3; n++) {
			if (depth == 0) {
				std::cout << (n + 1) << "/" << N3 << std::endl;
			} else if (depth == 1) {
				if (n && (n % N == 0)) {
					std::cout << ".";
					if (n % N2 == 0)
					std::cout << std::endl;
					else
					std::cout.flush();
				}
			}
			size_t i = n / N2;
			size_t j = (n % N2) / N;
			size_t k = n % N;
			if (depth == (maxdepth - 1)) {
				HCube<N> hc; //  = this + (idx - thisidx)
				hc.load(in, dataN, dataSize,
						offsetKpc + Vector3f(i * s, j * s, k * s), s,
						threshold);
				Vector3f mean;
				if (hc.collapse(mean, error)) {
					thishc.setValue(i, j, k, mean);
				} else {
					out.seekp(idx * sizeof(HCube<N> ), std::ios::beg);
					out.write((char *) &hc, sizeof(HCube<N> ));
					thishc.setCube(i, j, k, idx - thisidx);
					idx++;
				}
			} else {
				size_t tmpidx = idx;

			}
		}

		if (depth == 0) {
			out.seekp(thisidx * sizeof(HCube<N> ), std::ios::beg);
			out.write((char *) &thishc, sizeof(HCube<N> ));
		}

		if (depth == 1) {
			std::cout << "." << std::endl;
		}

	}
#endif
	static void create(HCube<N> &thishc, std::ostream &out, std::istream &in,
			size_t dataN, float dataSize, const Vector3f &offsetKpc,
			float sizeKpc, float error, float threshold, size_t maxdepth,
			size_t depth, size_t &idx) {
		const float s = sizeKpc / N;
		const size_t N3 = N * N * N;
		const size_t N2 = N * N;
		size_t thisidx = idx;
		for (size_t n = 0; n < N3; n++) {
			if (depth == 0) {
				std::cout << (n + 1) << "/" << N3 << std::endl;
			} else if (depth == 1) {
				if (n && (n % N == 0)) {
					std::cout << ".";
					if (n % N2 == 0)
						std::cout << std::endl;
					else
						std::cout.flush();
				}
			}
			size_t i = n / N2;
			size_t j = (n % N2) / N;
			size_t k = n % N;
			HCube<N> hc;
			size_t tmpidx = idx + 1;
			size_t nextidx = tmpidx;
			Vector3f subOffsetKpc = offsetKpc + Vector3f(i * s, j * s, k * s);
			if (depth == (maxdepth - 1)) {
				hc.load(in, dataN, dataSize, subOffsetKpc, s, threshold);
			} else {
				create(hc, out, in, dataN, dataSize, subOffsetKpc, s, error,
						threshold, maxdepth, depth + 1, tmpidx);
			}
			Vector3f mean;
			if (hc.collapse(mean, error, threshold)) {
				thishc.setValue(i, j, k, mean);
			} else {
				idx = tmpidx;
				out.seekp(nextidx * sizeof(HCube<N> ), std::ios::beg);
				out.write((char *) &hc, sizeof(HCube<N> ));
				thishc.setCube(i, j, k, nextidx - thisidx);
			}
		}

		if (depth == 1) {
			std::cout << "." << std::endl;
		}

	}

	static void create(std::ostream &out, std::istream &in, size_t dataN,
			float dataSize, const Vector3f &offsetKpc, float sizeKpc,
			float error, float threshold, size_t maxdepth) {
		size_t idx = 0;
		HCube<N> hc;
		create(hc, out, in, dataN, dataSize, offsetKpc, sizeKpc, error,
				threshold, maxdepth, 0, idx);
		out.seekp(0, std::ios::beg);
		out.write((char *) &hc, sizeof(HCube<N> ));
	}

	size_t getN() {
		return N;
	}

	Vector3f &at(size_t i, size_t j, size_t k) {
		return elements[i * N * N + j * N + k];
	}

	const Vector3f &at(size_t i, size_t j, size_t k) const {
		return elements[i * N * N + j * N + k];
	}

	void setValue(size_t i, size_t j, size_t k, const Vector3f &f) {
		at(i, j, k) = f;
	}

	static bool isCube(const Vector3f &v) {
		return (v.x != v.x);
	}

	HCube<N> *toCube(const Vector3f &v) {
		uint32_t *a = (uint32_t *) &v.y;
		uint32_t *b = (uint32_t *) &v.z;
		uint64_t c = (uint64_t) *a | (uint64_t) (*b) << 32;
		HCube<N> *h = this;
		return (h + c);
	}

	const HCube<N> *toCube(const Vector3f &v) const {
		uint32_t *a = (uint32_t *) &v.y;
		uint32_t *b = (uint32_t *) &v.z;
		uint64_t c = (uint64_t) *a | (uint64_t) (*b) << 32;
		const HCube<N> *h = this;
		return (h + c);
	}

	bool collapse(Vector3f &mean, float error, float threshold) {
		const size_t N3 = N * N * N;
		mean = Vector3f(0, 0, 0);
		for (size_t i = 0; i < N3; i++) {
			if (isCube(elements[i]))
				return false;
			mean += elements[i];
		}
		mean /= N3;

		const float t2 = threshold * threshold;
		const float e2 = error * error * mean.length2();
		bool collapseByError = true;
		bool collapseByThreshold = true;

		for (size_t i = 0; i < N3; i++) {
			float d2 = (elements[i] - mean).length2();
			if (d2 > e2)
				collapseByError = false;
			if (d2 > t2)
				collapseByThreshold = false;
			if (!collapseByThreshold && !collapseByError)
				return false;
		}

		return (collapseByThreshold || collapseByError);
	}

	void setCube(size_t i, size_t j, size_t k, uint64_t cube) {
		setCube(at(i, j, k), cube);
	}

	static void setCube(Vector3f &v, uint64_t cube) {
		v.x = std::numeric_limits<float>::quiet_NaN();
		uint32_t *a = (uint32_t *) &v.y;
		*a = (cube & 0xffffffff);
		uint32_t *b = (uint32_t *) &v.z;
		*b = (cube >> 32 & 0xffffffff);
	}

	const Vector3f &getValue(const Vector3f &position, float size) const {
		float s = size / N;
		size_t i = position.x / s;
		size_t j = position.y / s;
		size_t k = position.z / s;
		if (i >= N) {
#ifdef DEBUG
			std::cout << "invalid i: " << i << " x:" << position.x << " size: "
					<< size << std::endl;
#endif
			throw invalid_position();
		}
		if (j >= N) {
#ifdef DEBUG
			std::cout << "invalid j: " << j << " y:" << position.y << " size: "
					<< size << std::endl;
#endif
			throw invalid_position();
		}
		if (k >= N) {
#ifdef DEBUG
			std::cout << "invalid k: " << k << " z:" << position.z << " size: "
					<< size << std::endl;
#endif
			throw invalid_position();
		}
		const Vector3f &v = at(i, j, k);
		if (isCube(v)) {
			const HCube *c = toCube(v);
			return c->getValue(position - Vector3f(i * s, j * s, k * s), s);
		} else {
			return v;
		}
	}

	size_t getDepth(const Vector3f &position, float size,
			size_t depth = 0) const {
		const float s = size / N;
		size_t i = position.x / s;
		size_t j = position.y / s;
		size_t k = position.z / s;
		if (i >= N) {
#ifdef DEBUG
            std::cout << "invalid i: " << i << " x:" << position.x << " size: "
					<< size << std::endl;
#endif
			throw invalid_position();
		}
		if (j >= N) {
#ifdef DEBUG
			std::cout << "invalid j: " << j << " y:" << position.y << " size: "
					<< size << std::endl;
#endif
			throw invalid_position();
		}
		if (k >= N) {
#ifdef DEBUG
			std::cout << "invalid k: " << k << " z" << position.z << " size: "
					<< size << std::endl;
#endif
			throw invalid_position();
		}
		const Vector3f &v = at(i, j, k);
		if (isCube(v)) {
			const HCube *c = toCube(v);
			return c->getDepth(position - Vector3f(i * s, j * s, k * s), s,
					depth + 1);
		} else {
			return depth;
		}
	}

	size_t getCubeCount() const {
		size_t n = N * N * N;
		size_t count = 1;
		for (size_t i = 0; i < n; i++) {
			if (isCube(elements[i])) {
				const HCube *c = toCube(elements[i]);
				count += c->getCubeCount();
			}
		}
		return count;
	}

	class SamplingVisitor: public DatabaseVisitor {
		HCube<N> &cube;
		Vector3f offset;
		float size, cell;
		AABB<float> box;

		size_t toLowerIndex(double x) {
			return (size_t) clamp((int) ::floor(x / cell), (int) 0,
					(int) cube.getN() - 1);
		}

		size_t toUpperIndex(double x) {
			return (size_t) clamp((int) ::ceil(x / cell), (int) 0,
					(int) cube.getN() - 1);
		}

		template<class T>
		static T clamp(const T &value, const T &min, const T&max) {
			if (value < min)
				return min;
			else if (value > max)
				return max;
			else
				return value;
		}

	public:

		SamplingVisitor(HCube<N> &cube, const Vector3f &offset, float size) :
				cube(cube), offset(offset), size(size) {
			cell = size / cube.getN();
			box.min = offset;
			box.max = offset + Vector3f(size);
		}

		void begin(const Database &db) {
			for (size_t i = 0; i < N; i++) {
				for (size_t j = 0; j < N; j++) {
					for (size_t k = 0; k < N; k++) {
						cube.at(i, j, k) = Vector3f(0, 0, 0);
					}
				}
			}
		}

		bool intersects(const Vector3f &lower, const Vector3f &upper, float margin) {
			AABB<float> other(lower - Vector3f(margin), upper + Vector3f(margin));
			return box.intersects(other);
		}

		void visit(const SmoothParticle &part) {
			SmoothParticle particle = part;
//			particle.smoothingLength += _broadeningFactor
//					* _grid.getCellLength();

			Vector3f value = particle.bfield * particle.weight() * particle.mass
					/ particle.rho;
			float r = particle.smoothingLength + cell;

			Vector3f relativePosition = particle.position - offset;
			size_t x_min = toLowerIndex(relativePosition.x - r);
			size_t x_max = toUpperIndex(relativePosition.x + r);

			size_t y_min = toLowerIndex(relativePosition.y - r);
			size_t y_max = toUpperIndex(relativePosition.y + r);

			size_t z_min = toLowerIndex(relativePosition.z - r);
			size_t z_max = toUpperIndex(relativePosition.z + r);

			Vector3f p;
#pragma omp parallel for
			for (size_t x = x_min; x <= x_max; x++) {
				p.x = x * cell;
				for (size_t y = y_min; y <= y_max; y++) {
					p.y = y * cell;
					for (size_t z = z_min; z <= z_max; z++) {
						p.z = z * cell;
						float k = particle.kernel(offset + p);
						cube.at(x, y, z) += value * k;
					}
				}
			}

		}

		void end() {

		}
	};

	static size_t memoryUsage(size_t depth) {
		size_t n = N * N * N;
		size_t size = 0;
		for (size_t i = 0; i <= depth; i++)
			size += ::pow(n, i) * n * sizeof(Vector3f);
		return size;
	}
};

typedef HCube<2> HCube2;
typedef HCube<4> HCube4;
typedef HCube<8> HCube8;
typedef HCube<16> HCube16;
typedef HCube<32> HCube32;
typedef HCube<64> HCube64;

class MappedWriteFile {
private:
	int _file;
	void *_data;
	size_t _data_size;
public:

	MappedWriteFile(const std::string filename, size_t size, bool resume = false) :
			_file(-1), _data(MAP_FAILED), _data_size(size) {
		if (resume)
			_file = ::open(filename.c_str(), O_RDWR, 0666);
		else
			_file = ::open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
		if (_file == -1)
			throw std::runtime_error("[MappedFile] error opening file!");
		truncate(_data_size);

		_data = ::mmap(NULL, _data_size, PROT_READ | PROT_WRITE, MAP_SHARED,
				_file, 0);

		if (_data == MAP_FAILED)
			throw std::runtime_error("[MappedFile] error mapping file!");
	}

	void truncate(size_t size) {
		if (_file == -1)
			return;
		if (ftruncate(_file, size) == -1)
			throw std::runtime_error("[MappedFile] error truncating file!");
	}

	~MappedWriteFile() {
		unmap();
		close();
	}

	void unmap() {
		if (_data != MAP_FAILED) {
			::msync(_data, _data_size, MS_SYNC);
			::munmap(_data, _data_size);
			_data = MAP_FAILED;
		}

	}
	void close() {
		if (_file != -1) {
			::close(_file);
			_file = -1;
		}

	}

	void *data() {
		return _data;
	}
};

template<int N>
class HCubeFile: public Referenced {
private:
	int _fd;
	off_t _size;
	void *_data;

public:
	
	enum MappingType {
		Auto,
		OnDemand,
		ReadAhead	
	};

	HCubeFile() :
			_fd(-1), _size(0), _data(MAP_FAILED) {

	}

	HCubeFile(const std::string &filename, MappingType mtype = Auto) :
			_fd(-1), _size(0), _data(MAP_FAILED) {
		open(filename, mtype);
	}

	~HCubeFile() {
		close();
	}

	
	void open(const std::string &filename, MappingType mtype = Auto) {
		close();

		_fd = ::open(filename.c_str(), O_RDONLY);
		if (_fd == -1) {
			perror("HCubeFile");
			throw std::runtime_error("[HCubeFile] error opening file!");
		}
		_size = lseek(_fd, 0, SEEK_END);

		int flags = MAP_PRIVATE;
		if (mtype == ReadAhead)
			flags |= MAP_POPULATE;
		else if (mtype == Auto) {
			size_t mem = sysconf (_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE);
			if (_size < mem - 200) {
				#ifdef DEBUG
				std::cout << "[HCubeFile] Automatic mapping: ReadAhead, " << mem / 1000 / 1000 << " MB physical, " << _size / 1000 / 1000 << " MB required" << std::endl;
				#endif
				flags |= MAP_POPULATE;
			} else {
				#ifdef DEBUG
				std::cout << "[HCubeFile] Automatic mapping: OnDemand, " << mem / 1000 / 1000 << " MB physical, " << _size / 1000 / 1000 << " MB required" << std::endl;
				#endif
			}
		}
		_data = mmap(NULL, _size, PROT_READ, flags, _fd, 0);
		if (_data == MAP_FAILED) {
			close();
			throw std::runtime_error("[HCubeFile] error mapping file!");
		}
	}

	const HCube<N> *hcube() {
		return (const HCube<N> *) _data;
	}

	void close() {
		if (_data != MAP_FAILED) {
			int result = munmap(_data, _size);
//          if (result < 0) {
//              printf("Error unmapping 0x0%lx of size %ld\n",
//                      (unsigned long) _data, _size);
//          }
			_data = MAP_FAILED;
		}

		_size = 0;

		if (_fd >= 0) {
			::close(_fd);
			_fd = -1;
		}
	}

	static bool create(Database *db, const Vector3f &offsetKpc, float sizeKpc,
			float error, float threshold, size_t maxdepth, size_t target_depth,
			const std::string &filename) {

		size_t max_size = std::min((size_t)1<<40UL, HCube<N>::memoryUsage(maxdepth));

		HCubeInitCheckpoint checkpoint(filename + ".checkpoint");
		bool resume = !checkpoint.empty();
		MappedWriteFile mapping(filename, max_size, resume);

		HCube<N> *hcube = new (mapping.data()) HCube<N>;
		size_t idx = 0;
		HCubeInitFlags flags;
		flags.error = error;
		flags.maxdepth = maxdepth;
		flags.offsetKpc = offsetKpc;
		flags.sizeKpc = sizeKpc;
		flags.target_depth = target_depth;
		flags.threshold = threshold;
		flags.sizeKpc = sizeKpc;

		hcube->init(db, offsetKpc, sizeKpc, 0, idx, flags, checkpoint);

		
		off_t rsize = hcube->getCubeCount() * sizeof(HCube<N> );

		mapping.unmap();
		mapping.truncate(rsize);

		return true;
	}

	static bool create(MagneticField *field, const Vector3f &offsetKpc, float sizeKpc,
			float error, float threshold, size_t maxdepth,
			const std::string &filename) {

		MappedWriteFile mapping(filename, HCube<N>::memoryUsage(maxdepth));

		HCube<N> *hcube = new (mapping.data()) HCube<N>;
		size_t idx = 0;
		HCubeInitFlags flags;
		flags.error = error;
		flags.maxdepth = maxdepth;
		flags.offsetKpc = offsetKpc;
		flags.sizeKpc = sizeKpc;
		flags.target_depth = maxdepth;
		flags.threshold = threshold;
		flags.sizeKpc = sizeKpc;
		hcube->init(field, offsetKpc, sizeKpc, 0, idx, flags);

        off_t rsize = hcube->getCubeCount() * sizeof(HCube<N> );

        mapping.unmap();
		mapping.truncate(rsize);

		return true;
	}
	
	static bool create(Vector3f *data, size_t dataN, const Vector3f &offsetKpc,
			float sizeKpc, float error, float threshold, size_t maxdepth,
			const std::string &filename) {

		MappedWriteFile mapping(filename, HCube<N>::memoryUsage(maxdepth));

		HCube<N> *hcube = new (mapping.data()) HCube<N>;
		size_t idx = 0;
		HCubeInitFlags flags;
		flags.error = error;
		flags.maxdepth = maxdepth;
		flags.offsetKpc = offsetKpc;
		flags.sizeKpc = sizeKpc;
		flags.target_depth = maxdepth;
		flags.threshold = threshold;
		flags.sizeKpc = sizeKpc;

		hcube->init(data, dataN, sizeKpc, offsetKpc, sizeKpc, 0, idx, flags);

		off_t rsize = hcube->getCubeCount() * sizeof(HCube<N> );

		mapping.unmap();
		mapping.truncate(rsize);

		return true;
	}

	static bool create(Grid<Vector3f> &grid, const Vector3f &offsetKpc,
			float sizeKpc, float error, float threshold, size_t maxdepth,
			const std::string &filename) {

		return create(grid.elements.data(), grid.bins, offsetKpc, sizeKpc,
				error, threshold, maxdepth, filename);
	}

	static bool createFromRaw(const std::string rawfilename, size_t dataN,
			const Vector3f &offsetKpc, float sizeKpc, float error,
			float threshold, size_t maxdepth, const std::string &filename) {
		std::ifstream in(rawfilename.c_str(), std::ios::binary);
		MappedWriteFile mapping(filename, HCube<N>::memoryUsage(maxdepth));

		HCube<N> *hcube = new (mapping.data()) HCube<N>;
		size_t idx = 0;
		
		HCubeInitFlags flags;
		flags.error = error;
		flags.maxdepth = maxdepth;
		flags.offsetKpc = offsetKpc;
		flags.sizeKpc = sizeKpc;
		flags.target_depth = maxdepth;
		flags.threshold = threshold;
		flags.sizeKpc = sizeKpc;
		hcube->init(in, dataN, sizeKpc, offsetKpc, sizeKpc, 0, idx, flags);

		off_t rsize = hcube->getCubeCount() * sizeof(HCube<N> );

		mapping.unmap();
		mapping.truncate(rsize);

		return true;
	}
};

typedef HCubeFile<2> HCubeFile2;
typedef HCubeFile<4> HCubeFile4;
typedef HCubeFile<8> HCubeFile8;
typedef HCubeFile<16> HCubeFile16;
typedef HCubeFile<32> HCubeFile32;
typedef HCubeFile<64> HCubeFile64;

} // namespace


