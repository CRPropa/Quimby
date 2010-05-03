/*
 * Particle.hpp
 *
 *  Created on: 28.04.2010
 *      Author: gmueller
 */

#ifndef SMOOTH_PARTICLE_HPP_
#define SMOOTH_PARTICLE_HPP_

#include <ostream>

class SmoothParticle {
public:
	Vector3f position;
	Vector3f bfield;
	float smoothingLength;
};

inline std::ostream &operator <<(std::ostream &out, const SmoothParticle &v) {
	out << "(" << v.position.x << "," << v.position.x << "," << v.position.x
			<< "), " << "(" << v.bfield.x << "," << v.bfield.x << ","
			<< v.bfield.x << "), " << v.smoothingLength;
	return out;
}

#endif /* PARTICLE_HPP_ */
