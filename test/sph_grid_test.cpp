/*
 * sph_grid_test.cpp
 *
 *  Created on: Aug 10, 2011
 *      Author: gmueller
 */

#include "gadget/SPHGrid.hpp"

#include <iostream>

class SPHGridTest {
	SPHGrid grid;
	size_t expectedCount;
	float margin;
public:
	SPHGridTest() :
			grid(1, 10000), expectedCount(0), margin(100) {
		grid.setOffset(Vector3f(20000, 20000, 20000));
	}

	void add(const Vector3f &p, float sl, bool expect) {
		std::cout << p << " - " << sl << std::endl;
		SmoothParticle particle;
		particle.position = p;
		particle.smoothingLength = sl;

		if (expect)
			expectedCount++;
		grid.add(particle, margin);
		if (grid.get(0, 0, 0).size() != expectedCount)
			throw std::runtime_error("wrong number");
	}
};

int main() {
	SPHGridTest test;
	Vector3f pos;

	test.add(Vector3f(1000, 1000, 1000), 100, false);
	test.add(Vector3f(19000, 1000, 1000), 2000, false);
	test.add(Vector3f(19000, 19000, 1000), 2000, false);
	test.add(Vector3f(19000, 19000, 19000), 2000, true);
	test.add(Vector3f(19000, 33000, 19000), 2000, false);
	test.add(Vector3f(19000, 31000, 19000), 2000, true);
	test.add(Vector3f(25000, 25000, 25000), 2000, true);

	test.add(Vector3f(17850, 25000, 25000), 2000, false);
	test.add(Vector3f(17950, 25000, 25000), 2000, true);

	test.add(Vector3f(25000, 17850, 25000), 2000, false);
	test.add(Vector3f(25000, 17950, 25000), 2000, true);

	test.add(Vector3f(25000, 25000, 17850), 2000, false);
	test.add(Vector3f(25000, 25000, 17950), 2000, true);

	std::cout << "done" << std::endl;

	return 0;
}
