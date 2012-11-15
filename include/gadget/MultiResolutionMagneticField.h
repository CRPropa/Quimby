#ifndef MULTIRESOLUTIONMAGNETICFIELD_H_
#define MULTIRESOLUTIONMAGNETICFIELD_H_

#include "gadget/SmoothParticle.h"
#include "gadget/AABB.h"
#include "gadget/Grid.h"
#include "gadget/Database.h"
#include <vector>
#include <string>

namespace gadget {

class MultiResolutionMagneticFieldCell {
public:
	AABB<float> bounds;
	Grid<Vector3f> grid;
};

class MultiResolutionMagneticField {
public:
	static void create(Database &db, const std::string &filename, size_t bins =
			64, float minres = 10, float a = 0.1, float b = 0.08,
			float c = 0.04);
};

}

#endif /* MULTIRESOLUTIONMAGNETICFIELD_H_ */
