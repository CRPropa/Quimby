#pragma once

#include "Vector3.h"
#include "Database.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

namespace quimby {

template<size_t N>
class HCube {
	Vector3f elements[N * N * N];

public:

	void init(Database *db, const Vector3f &offsetKpc, float sizeKpc,
			float error, float threshold, size_t maxdepth, size_t depth,
			size_t &idx) {
		const float s = sizeKpc / N;
		const size_t N3 = N * N * N;
		const size_t N2 = N * N;
		size_t thisidx = idx;
		idx++;
		if (depth == maxdepth) {
			SamplingVisitor visitor(*this, offsetKpc, sizeKpc);
			db->accept(offsetKpc, offsetKpc + Vector3f(sizeKpc), visitor);
		} else {

			for (size_t n = 0; n < N3; n++) {
				HCube<N> *hc = this + (idx - thisidx);
				size_t i = n / N2;
				size_t j = (n % N2) / N;
				size_t k = n % N;
				size_t tmpidx = idx;
				hc->init(db, offsetKpc + Vector3f(i * s, j * s, k * s), s,
						error, threshold, maxdepth, depth + 1, tmpidx);
				Vector3f mean;
				if (hc->collapse(mean, error, threshold)) {
					setValue(i, j, k, mean);
				} else {
					setCube(i, j, k, idx - thisidx);
					idx = tmpidx;
				}
			}

		}
	}

	void init(const Vector3f *data, size_t dataN, float dataSize,
			const Vector3f &offsetKpc, float sizeKpc, float error,
			float threshold, size_t maxdepth, size_t depth, size_t &idx) {
		const float s = sizeKpc / N;
		const size_t N3 = N * N * N;
		const size_t N2 = N * N;
		size_t thisidx = idx;
		idx++;
		if (depth == maxdepth) {
			float dataStep = dataSize / dataN;
			size_t oX = offsetKpc.x / dataStep;
			size_t oY = offsetKpc.y / dataStep;
			size_t oZ = offsetKpc.z / dataStep;
			const size_t dataN2 = dataN * dataN;
			for (size_t iX = 0; iX < N; iX++) {
				for (size_t iY = 0; iY < N; iY++) {
					for (size_t iZ = 0; iZ < N; iZ++) {
						elements[iX * N2 + iY * N + iZ] = data[(iX + oX)
								* dataN2 + (iY + oY) * dataN + (iZ + oZ)];
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
						offsetKpc + Vector3f(i * s, j * s, k * s), s, error,
						threshold, maxdepth, depth + 1, tmpidx);
				Vector3f mean;
				if (hc->collapse(mean, error, threshold)) {
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
			const Vector3f &offsetKpc, float sizeKpc, float error,
			float threshold, size_t maxdepth, size_t depth, size_t &idx) {
		const float s = sizeKpc / N;
		const size_t N3 = N * N * N;
		const size_t N2 = N * N;
		size_t thisidx = idx;
		idx++;
		if (depth == maxdepth) {
			load(in, dataN, dataSize, offsetKpc, sizeKpc, threshold);
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
						offsetKpc + Vector3f(i * s, j * s, k * s), s, error,
						threshold, maxdepth, depth + 1, tmpidx);
				Vector3f mean;
				if (hc->collapse(mean, error, threshold)) {
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
			std::cout << "invalid i: " << i << " x:" << position.x << " size: "
					<< size << std::endl;
			throw std::runtime_error("invalid x");
		}
		if (j >= N) {
			std::cout << "invalid j: " << j << " y:" << position.y << " size: "
					<< size << std::endl;
			throw std::runtime_error("invalid y");
		}
		if (k >= N) {
			std::cout << "invalid k: " << k << " z:" << position.z << " size: "
					<< size << std::endl;
			throw std::runtime_error("invalid z");
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
			std::cout << "invalid i: " << i << " x:" << position.x << " size: "
					<< size << std::endl;
			throw std::runtime_error("invalid x");
		}
		if (j >= N) {
			std::cout << "invalid j: " << j << " y:" << position.y << " size: "
					<< size << std::endl;
			throw std::runtime_error("invalid y");
		}
		if (k >= N) {
			std::cout << "invalid k: " << k << " z" << position.z << " size: "
					<< size << std::endl;
			throw std::runtime_error("invalid z");
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

	MappedWriteFile(const std::string filename, size_t size) :
			_file(-1), _data(MAP_FAILED), _data_size(size) {
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

		_fd = ::open(filename.c_str(), O_RDWR);
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
			float error, float threshold, size_t maxdepth,
			const std::string &filename) {

		MappedWriteFile mapping(filename, HCube<N>::memoryUsage(maxdepth));

		HCube<N> *hcube = new (mapping.data()) HCube<N>;
		size_t idx = 0;
		hcube->init(db, offsetKpc, sizeKpc, error, threshold, maxdepth, 0, idx);

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
		hcube->init(data, dataN, sizeKpc, offsetKpc, sizeKpc, error, threshold,
				maxdepth, 0, idx);

		off_t rsize = hcube->getCubeCount() * sizeof(HCube<N> );

		mapping.unmap();
		mapping.truncate(rsize);

		return true;
	}

	static bool createFromRaw(const std::string rawfilename, size_t dataN,
			const Vector3f &offsetKpc, float sizeKpc, float error,
			float threshold, size_t maxdepth, const std::string &filename) {
		std::ifstream in(rawfilename.c_str(), std::ios::binary);
		MappedWriteFile mapping(filename, HCube<N>::memoryUsage(maxdepth));

		HCube<N> *hcube = new (mapping.data()) HCube<N>;
		size_t idx = 0;
		hcube->init(in, dataN, sizeKpc, offsetKpc, sizeKpc, error, threshold,
				maxdepth, 0, idx);

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
