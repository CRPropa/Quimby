/*
 * Particle.hpp
 *
 *  Created on: 28.04.2010
 *      Author: gmueller
 */

#ifndef SMOOTH_PARTICLE_HPP_
#define SMOOTH_PARTICLE_HPP_

#include <ostream>
#include <vector>
#include "gadget/Vector3.hpp"

class SmoothParticle {
public:
	Vector3f position;
	Vector3f bfield;
	float smoothingLength;
	float mass;

	static float kernel(float r) {
		if (r < 0.5) {
			return 1.0 + 6 * r * r * (r - 1);
		} else if (r < 1.0) {
			float x = (1 - r);
			return 2 * x * x * x;
		} else {
			return 0.0;
		}
	}

	static float kernel(float value, float center, float position, float hsml) {
		return value * kernel(std::fabs((center - position) / hsml));
	}

	static float kernel(float value, const Vector3f &center,
			const Vector3f &position, float hsml) {
		return value * kernel(std::fabs((center - position).length() / hsml));
	}

	float kernel(const Vector3f &center) const {
		return kernel(std::fabs((center - position).length() / smoothingLength));
	}

	float weight() const {
		8 / (M_PI * smoothingLength * smoothingLength * smoothingLength);
	}

	float calculateRho(const std::vector<SmoothParticle> &particles) {
		float rho = 0;
		for (size_t i = 0; i < particles.size(); i++) {
			rho += particles[i].mass * particles[i].weight() * particles[i].kernel(position);
		}
		return rho;
	}
};

inline std::ostream &operator <<(std::ostream &out, const SmoothParticle &v) {
	out << "(" << v.position.x << "," << v.position.x << "," << v.position.x
			<< "), " << "(" << v.bfield.x << "," << v.bfield.x << ","
			<< v.bfield.x << "), " << v.smoothingLength;
	return out;
}

#endif /* PARTICLE_HPP_ */
