#ifndef _GADGET_SPH_GRID_H_
#define _GADGET_SPH_GRID_H_

#include "gadget/Grid.h"
#include "gadget/SmoothParticle.h"
#include "gadget/Index3.h"
#include "gadget/GadgetFile.h"

#include <vector>
#include <stdexcept>

namespace gadget {

class SPHGrid: public Grid<std::vector<SmoothParticle> > {
	Vector3f offset;
	float margin;
public:

	typedef Grid<std::vector<SmoothParticle> > base;

	SPHGrid() :
			base(), offset(0.f) {
	}

	SPHGrid(size_t bins, float size) :
			base(bins, size), offset(0.f), margin(0.f) {

	}

	void setOffset(const Vector3f &offset) {
		this->offset = offset;
	}

	const Vector3f &getOffset() const {
		return offset;
	}

	void setMargin(const float &margin) {
		this->margin = margin;
	}

	const float &getMargin() const {
		return margin;
	}

	size_t clamp(const double &v) {
		int32_t i = (int32_t) v;
		if (i < 0)
			return 0;
		if ((size_t) i > bins)
			return bins;
		return (size_t) i;
	}

	Index3 clamp(const Vector3f &v) {
		return Index3(clamp(v.x), clamp(v.y), clamp(v.z));
	}

	size_t add(const SmoothParticle &particle) {
		Vector3f relativePosition = particle.position - offset;
		Vector3f radius = Vector3f(particle.smoothingLength + margin);
		Vector3f l = (relativePosition - radius) / cellLength;
		Vector3f u = (relativePosition + radius) / cellLength;

		Index3 lower = clamp(l.floor());
		Index3 upper = clamp(u.ceil());

		size_t count = 0;
		for (size_t x = lower.x; x < upper.x; x++) {
			for (size_t y = lower.y; y < upper.y; y++) {
				for (size_t z = lower.z; z < upper.z; z++) {
					count++;
					get(x, y, z).push_back(particle);
				}
			}
		}

		return count;
	}

	size_t loadFromGadgetFile(const std::string &filename, float h,
			Vector3f pivot) {
		GadgetFile file;
		file.open(filename);
		if (file.good() == false) {
			throw std::runtime_error("Failed to open file " + filename);
		}

		file.readHeader();
		int pn = file.getHeader().particleNumberList[0];
		if (pn < 1)
			return 0;

		std::vector<float> pos;
		if (file.readFloatBlock("POS ", pos) == false) {
			throw std::runtime_error("Failed read POS from file " + filename);
		}

		std::vector<float> bfld;
		if (file.readFloatBlock("BFLD", bfld) == false) {
			throw std::runtime_error("Failed read BFLD from file " + filename);
		}

		std::vector<float> hsml;
		if (file.readFloatBlock("HSML", hsml) == false) {
			throw std::runtime_error("Failed read HSML from file " + filename);
		}

		std::vector<float> rho;
		if (file.readFloatBlock("RHO ", rho) == false) {
			throw std::runtime_error("Failed read RHO from file " + filename);
		}

		size_t overlaps = 0;
		for (int iP = 0; iP < pn; iP++) {
			SmoothParticle particle;
			particle.smoothingLength = hsml[iP];
			particle.position.x = pos[iP * 3];
			particle.position.y = pos[iP * 3 + 1];
			particle.position.z = pos[iP * 3 + 2];

			particle.bfield.x = bfld[iP * 3];
			particle.bfield.y = bfld[iP * 3 + 1];
			particle.bfield.z = bfld[iP * 3 + 2];

			particle.mass = file.getHeader().massList[0];
			particle.rho = rho[iP];

			particle.toKpc(h, pivot);

			overlaps += add(particle);
		}

		std::cout << "Overlaps: " << overlaps << std::endl;
		return pn;
	}
};

} // namespace gadget

#endif /* _GADGET_SPH_GRID_H_ */
