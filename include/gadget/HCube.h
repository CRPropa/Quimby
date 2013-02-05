/*
 * hcube.h
 *
 *  Created on: 18.12.2012
 *      Author: gmueller
 */

#ifndef HCUBE_H_
#define HCUBE_H_

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

namespace gadget {

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
			const float t2 = threshold * threshold;
			for (size_t n = 0; n < N3; n++) {
				if (elements[n].length2() < t2)
					elements[n] = Vector3f(0, 0, 0);
			}
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
				if (hc->collapse(mean, error)) {
					setValue(i, j, k, mean);
				} else {
					setCube(i, j, k, idx - thisidx);
					idx = tmpidx;
				}
			}

		}
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

	bool collapse(Vector3f &mean, float error) {
		const size_t n = N * N * N;
		mean = Vector3f(0, 0, 0);
		for (size_t i = 0; i < n; i++) {
			if (isCube(elements[i]))
				return false;
			mean += elements[i];
		}

		mean /= n;
		const float e2 = error * error * mean.length2();
		for (size_t i = 0; i < n; i++) {
			float d2 = (elements[i] - mean).length2();
			if (d2 > e2)
				return false;
		}

		return true;
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
			std::cout << "invalid k: " << k << " z" << position.z << " size: "
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

		void begin() {
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
};

typedef HCube<2> HCube2;
typedef HCube<4> HCube4;
typedef HCube<8> HCube8;
typedef HCube<16> HCube16;
typedef HCube<32> HCube32;
typedef HCube<64> HCube64;
typedef HCube<128> HCube128;
typedef HCube<256> HCube256;
typedef HCube<512> HCube512;
typedef HCube<1024> HCube1024;

template<int N>
class HCubeFile {
private:
	int _fd;
	off_t _size;
	void *_buffer;
public:

	HCubeFile() :
			_fd(-1), _size(0), _buffer(0) {

	}

	HCubeFile(const std::string &filename) :
			_fd(-1), _size(0), _buffer(0) {
		open(filename);
	}

	~HCubeFile() {
		close();
	}

	bool open(const std::string &filename) {
		_fd = ::open(filename.c_str(), O_RDWR);
		_size = lseek(_fd, 0, SEEK_END);
		//printf("Opened %ld b file\n", _size);

		_buffer = mmap(NULL, _size, PROT_READ, MAP_SHARED, _fd, 0);
		if (_buffer == MAP_FAILED ) {
			close();
			return false;
		}
		//printf("Done mmap - returned 0x0%lx\n", (unsigned long) _buffer);

		return true;
	}

	const HCube<N> *get() {
		return (const HCube<N> *) _buffer;
	}

	void close() {
		if (_buffer) {
			int result = munmap(_buffer, _size);
//			if (result < 0) {
//				printf("Error unmapping 0x0%lx of size %ld\n",
//						(unsigned long) _buffer, _size);
//			}
			_buffer = 0;
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

		// calculate max size
		size_t n = N * N * N;
		off_t size = n;
		for (size_t i = 1; i < maxdepth; i++)
			size *= n;
		size += 1;
		size *= sizeof(HCube<N> );

		int fd = ::open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
		ftruncate(fd, size);
		//printf("Created %ld b sparse file\n", size);

		void* buffer = mmap(NULL, (size_t) size, PROT_READ | PROT_WRITE,
				MAP_SHARED, fd, 0);
		if (buffer == MAP_FAILED ) {
			perror("mmap");
			exit(1);
		}
		//printf("Done mmap - returned 0x0%lx\n", (unsigned long) buffer);

		HCube<N> *hcube = new (buffer) HCube<N>;
		size_t idx = 0;
		hcube->init(db, offsetKpc, sizeKpc, error, threshold, maxdepth, 0, idx);

		off_t rsize = hcube->getCubeCount() * sizeof(HCube<N> );

		if (munmap(buffer, (size_t) size) < 0) {
			perror("munmap");
			exit(1);
		}

		ftruncate(fd, rsize);
		//printf("Truncated sparse file at %ld b\n", rsize);

		::close(fd);

		return 0;
	}
};

typedef HCubeFile<2> HCubeFile2;
typedef HCubeFile<4> HCubeFile4;
typedef HCubeFile<8> HCubeFile8;
typedef HCubeFile<16> HCubeFile16;
typedef HCubeFile<32> HCubeFile32;
typedef HCubeFile<64> HCubeFile64;
typedef HCubeFile<128> HCubeFile128;
typedef HCubeFile<256> HCubeFile256;
typedef HCubeFile<512> HCubeFile512;
typedef HCubeFile<1024> HCubeFile1024;

}

#endif /* HCUBE_H_ */
