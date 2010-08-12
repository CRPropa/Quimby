/*
 * Particle.hpp
 *
 *  Created on: 28.04.2010
 *      Author: gmueller
 */

#ifndef SMOOTH_PARTICLE_HPP_
#define SMOOTH_PARTICLE_HPP_

#include <ostream>
#include "gadget/Vector3.hpp"

class SmoothParticle {
public:
	Vector3f position;
	Vector3f bfield;
	float smoothingLength;

	static float kernel(float r) {
		if (r < 1.0) {
			return 1.0 - 1.5 * r * r + 0.75 * r * r * r;
		} else if (r < 2.0) {
			float x = (2.0 - r);
			return 0.25 * x * x * x;
		} else {
			return 0.0;
		}
	}

	static float kernel(float value, float center, float position, float hsml) {
		return value * kernel(std::fabs((center - position) / hsml));
	}

};


inline std::ostream &operator <<(std::ostream &out, const SmoothParticle &v) {
	out << "(" << v.position.x << "," << v.position.x << "," << v.position.x
			<< "), " << "(" << v.bfield.x << "," << v.bfield.x << ","
			<< v.bfield.x << "), " << v.smoothingLength;
	return out;
}

#endif /* PARTICLE_HPP_ */
