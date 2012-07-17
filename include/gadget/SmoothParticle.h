#ifndef _GADGET_SMOOTH_PARTICLE_H_
#define _GADGET_SMOOTH_PARTICLE_H_

#include "gadget/Vector3.h"

#include <fstream>
#include <vector>
#include <stdint.h>

namespace gadget {

class SmoothParticle {
public:
	typedef float float_t;
	typedef Vector3<float_t> vector_t;

	vector_t position;
	vector_t bfield;
	float_t smoothingLength;
	float_t mass;
	float_t rho;

	SmoothParticle() :
			mass(0), rho(0), smoothingLength(0) {
	}

	void toKpc(float_t h, vector_t pivot) {
		smoothingLength /= h;
		mass /= h;
		rho *= (h * h);
		position.scale(1. / h, pivot);
	}

	// see http://arxiv.org/abs/0807.3553v2
	static float_t kernel(float_t r) {
		if (r < 0.5) {
			return 1.0 + 6 * r * r * (r - 1);
		} else if (r < 1.0) {
			float_t x = (1 - r);
			return 2 * x * x * x;
		} else {
			return 0.0;
		}
	}

	static float_t kernel(float_t value, float_t center, float_t position,
			float_t hsml) {
		return value * kernel(std::fabs((center - position) / hsml));
	}

	static float_t kernel(float_t value, const vector_t &center,
			const vector_t &position, float_t hsml) {
		return value * kernel(std::fabs((center - position).length() / hsml));
	}

	float_t kernel(const vector_t &point) const {
		float_t distance = (point - position).length();
		float_t normalizedDistance = distance / smoothingLength;
		return kernel(normalizedDistance);
	}

	float_t weight() const {
		return 8. / (M_PI * smoothingLength * smoothingLength * smoothingLength);
	}

	void updateRho(const std::vector<SmoothParticle> &particles) {
		rho = 0;
		for (size_t i = 0; i < particles.size(); i++) {
			rho += particles[i].mass * particles[i].weight()
					* particles[i].kernel(position);
		}
	}
};

class SmoothParticleHelper {
public:
	static void updateRho(std::vector<SmoothParticle> &particles) {
		const size_t s = particles.size();
//#pragma omp parallel for
		for (size_t i = 0; i < s; i++) {
			particles[i].updateRho(particles);
		}
	}

	static bool read(const std::string &filename,
			std::vector<SmoothParticle> &particles) {
		std::ifstream in(filename.c_str(), std::ifstream::binary);
		uint32_t s;
		in.read((char *) &s, sizeof(uint32_t));
		if (!in)
			return false;
		particles.resize(s);
		size_t bytes = sizeof(SmoothParticle) * s;
		in.read((char *) &particles[0], bytes);
		if (!in)
			return false;
		return true;
	}

	static bool write(const std::string &filename,
			const std::vector<SmoothParticle> &particles) {
		std::ofstream out(filename.c_str(), std::ofstream::binary);
		uint32_t s = particles.size();
		out.write((const char *) &s, sizeof(uint32_t));
		out.write((const char *) &particles[0], sizeof(SmoothParticle) * s);
		if (!out)
			return false;
		return true;
	}
};

} // namespace gadget

template<typename T>
inline std::ostream &operator <<(std::ostream &out,
		const gadget::SmoothParticle &v) {
	out << "(" << v.position.x << "," << v.position.x << "," << v.position.x
			<< "), " << "(" << v.bfield.x << "," << v.bfield.x << ","
			<< v.bfield.x << "), " << v.smoothingLength;
	return out;
}

#endif /* _GADGET_SMOOTH_PARTICLE_H_ */
