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

namespace gadget {

template<size_t N>
class HCube {
	Vector3f elements[N * N * N];

	void init(Database *db, const Vector3f &offsetKpc, float sizeKpc,
			float error, size_t maxdepth, size_t depth) {
		const float s = sizeKpc / N;
		const size_t N3 = N * N * N;
		const size_t N2 = N * N;
		if (depth == maxdepth) {
			SamplingVisitor visitor(*this, offsetKpc, sizeKpc);
			db->accept(offsetKpc, offsetKpc + Vector3f(sizeKpc), visitor);
		} else if (depth == 0) {
			HCube<N> hc;
#pragma omp parallel for schedule(dynamic, 1) private(hc)
			for (size_t n = 0; n < N3; n++) {
				size_t i = n / N2;
				size_t j = (n % N2) / N;
				size_t k = n % N;
#pragma omp critical
				std::cout << depth << ", " << n << " / " << N3 << ": " << i
						<< " " << j << " " << k << std::endl;
				hc.init(db, offsetKpc + Vector3f(i * s, j * s, k * s), s, error,
						maxdepth, depth + 1);
				Vector3f mean;
				if (hc.collapse(mean, error)) {
					setValue(i, j, k, mean);
				} else {
					setCube(i, j, k, new HCube<N>(hc));
				}
			}
		} else {
			HCube<N> hc;
			for (size_t n = 0; n < N3; n++) {
				size_t i = n / N2;
				size_t j = (n % N2) / N;
				size_t k = n % N;
				hc.init(db, offsetKpc + Vector3f(i * s, j * s, k * s), s, error,
						maxdepth, depth + 1);
				Vector3f mean;
				if (hc.collapse(mean, error)) {
					setValue(i, j, k, mean);
				} else {
					setCube(i, j, k, new HCube<N>(hc));
				}
			}

		}
	}

	void save(std::ofstream &out, size_t &idx) {
		idx += 1;

		// prepare data
		HCube<N> hc(*this);
		size_t tmp_idx = idx;
		const size_t N3 = N * N * N;
		for (size_t i = 0; i < N3; i++) {
			if (!isCube(hc.elements[i]))
				continue;
			HCube<N> *h = toCube(hc.elements[i]);
			setCube(hc.elements[i], (HCube<N> *) tmp_idx);
			tmp_idx += h->getCubeCount();
		}

		// write data
		out.write((const char *) &hc.elements[0], N3 * sizeof(Vector3f));

		// write sub cubes
		for (size_t i = 0; i < N3; i++) {
			if (!isCube(elements[i]))
				continue;
			HCube<N> *h = toCube(elements[i]);
			h->save(out, idx);
		}

		if (tmp_idx != idx)
			std::cout << "wrong number of idx: " << tmp_idx << " of " << idx
					<< std::endl;
	}

	void load(std::ifstream &in, size_t idx) {
		const size_t N3 = N * N * N;

		// read data
		in.seekg(sizeof(uint32_t) + N3 * sizeof(Vector3f) * idx, std::ios::beg);
		in.read((char *) &elements[0], N3 * sizeof(Vector3f));

		// read sub cubes
		for (size_t i = 0; i < N3; i++) {
			if (!isCube(elements[i]))
				continue;
			size_t sub_idx = (size_t) toCube(elements[i]);
			HCube<N> *h = new HCube<N>;
			h->load(in, sub_idx);
			setCube(elements[i], h);
		}
	}

public:

	size_t getN() {
		return N;
	}

	Vector3f &at(size_t i, size_t j, size_t k) {
		return elements[i * N * N + j * N + k];
	}

	void setValue(size_t i, size_t j, size_t k, const Vector3f &f) {
		at(i, j, k) = f;
	}

	static bool isCube(const Vector3f &v) {
		return (v.x != v.x);
	}

	static HCube<N> *toCube(const Vector3f &v) {
		uint32_t *a = (uint32_t *) &v.y;
		uint32_t *b = (uint32_t *) &v.z;
		uint64_t c = (uint64_t) *a | (uint64_t) (*b) << 32;
		return (HCube *) c;
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
		for (size_t i = 0; i < n; i++) {
			Vector3f d = (elements[i] - mean);
			d.x /= mean.x;
			d.y /= mean.y;
			d.z /= mean.z;
			if (d.x > error || d.y > error || d.z > error)
				return false;
		}

		return true;
	}

	void setCube(size_t i, size_t j, size_t k, HCube *cube) {
		setCube(at(i, j, k), cube);
	}

	static void setCube(Vector3f &v, HCube *cube) {
		v.x = std::numeric_limits<float>::quiet_NaN();
		uint64_t c = (uint64_t) cube;
		uint32_t *a = (uint32_t *) &v.y;
		*a = (c & 0xffffffff);
		uint32_t *b = (uint32_t *) &v.z;
		*b = (c >> 32 & 0xffffffff);
	}

	Vector3f &getValue(const Vector3f &position, float size) {
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
		Vector3f &v = at(i, j, k);
		if (isCube(v)) {
			HCube *c = toCube(v);
			return c->getValue(position - Vector3f(i * s, j * s, k * s), s);
		} else {
			return v;
		}
	}

	size_t getDepth(const Vector3f &position, float size, size_t depth = 0) {
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
		Vector3f &v = at(i, j, k);
		if (isCube(v)) {
			HCube *c = toCube(v);
			return c->getDepth(position - Vector3f(i * s, j * s, k * s), s,
					depth + 1);
		} else {
			return depth;
		}
	}

	size_t getCubeCount() {
		size_t n = N * N * N;
		size_t count = 1;
		for (size_t i = 0; i < n; i++) {
			if (isCube(elements[i])) {
				HCube *c = toCube(elements[i]);
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

	void init(Database *db, const Vector3f &offsetKpc, float sizeKpc,
			float error, size_t maxdepth) {
		init(db, offsetKpc, sizeKpc, error, maxdepth, 0);
	}

	void save(const std::string &filename) {
		std::ofstream outfile(filename.c_str(), std::ios::binary);
		uint32_t n = N;
		outfile.write((char *) &n, sizeof(uint32_t));
		size_t idx = 0;
		save(outfile, idx);
	}

	bool load(const std::string &filename) {
		std::ifstream infile(filename.c_str(), std::ios::binary);
		if (infile.bad())
			return false;

		uint32_t n;
		infile.read((char *) &n, sizeof(uint32_t));

		if (n != N) {
			throw std::runtime_error("Wrong HCube size");
		}

		load(infile, 0);

		return true;
	}
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
}

#endif /* HCUBE_H_ */
