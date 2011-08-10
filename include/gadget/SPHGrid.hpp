/*
 * SPHGrid.hpp
 *
 *  Created on: Aug 10, 2011
 *      Author: gmueller
 */

#ifndef GADGET_SPH_GRID_HPP_
#define GADGET_SPH_GRID_HPP_

#include "gadget/Grid.hpp"
#include "gadget/SmoothParticle.hpp"
#include "gadget/Index3.hpp"

#include <vector>

class SPHGrid: public Grid<std::vector<SmoothParticle> > {
	Vector3f offset;
	size_t c;
public:

	typedef Grid<std::vector<SmoothParticle> > base;

	SPHGrid() :
			base(), offset(0.f) {
	}

	SPHGrid(size_t bins, float size) :
			base(bins, size), offset(0.f) {

	}

	void setOffset(const Vector3f &offset) {
		this->offset = offset;
	}

	const Vector3f &getOffset() const {
		return offset;
	}

	size_t clamp(const double &v) {
		int32_t i = (int32_t) v;
		if (i < 0)
			return 0;
		if (i > bins)
			return bins;
		return (size_t) i;
	}

	Index3 clamp(const Vector3f &v) {
		return Index3(clamp(v.x), clamp(v.y), clamp(v.z));
	}

	void add(const SmoothParticle &particle, float marginKpc = 0) {
		Vector3f relativePosition = particle.position - offset;
		Vector3f radius = Vector3f(particle.smoothingLength + marginKpc);
		Vector3f l = (relativePosition - radius) / cellLength;
		Vector3f u = (relativePosition + radius) / cellLength;

		Index3 lower = clamp(l.floor());
		Index3 upper = clamp(u.ceil());

		for (size_t x = lower.x; x < upper.x; x++)
			for (size_t y = lower.y; y < upper.y; y++)
				for (size_t z = lower.z; z < upper.z; z++)
					get(x, y, z).push_back(particle);
	}
};

#endif /* SPHGRID_HPP_ */
