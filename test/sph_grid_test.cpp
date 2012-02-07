#include "gadget/SPHGrid.hpp"
using namespace gadget;

#include <iostream>

class SPHGridTest: public SPHGrid {
public:
	SPHGridTest(size_t bins, float size) :
			SPHGrid(bins, size) {

	}
	void test(const Vector3f &p, float sl, size_t expect) {
		std::cout << p << " - " << sl << std::endl;
		SmoothParticle particle;
		particle.position = p;
		particle.smoothingLength = sl;

		size_t count = add(particle);
		if (count != expect)
			throw std::runtime_error("wrong number");
	}
};

int main() {
	SPHGridTest test(1, 10000);
	test.setOffset(Vector3f(20000, 20000, 20000));
	test.setMargin(100);
	Vector3f pos;

	test.test(Vector3f(1000, 1000, 1000), 100, 0);
	test.test(Vector3f(19000, 1000, 1000), 2000, 0);
	test.test(Vector3f(19000, 19000, 1000), 2000, 0);
	test.test(Vector3f(19000, 19000, 19000), 2000, 1);
	test.test(Vector3f(19000, 33000, 19000), 2000, 0);
	test.test(Vector3f(19000, 31000, 19000), 2000, 1);
	test.test(Vector3f(25000, 25000, 25000), 2000, 1);

	test.test(Vector3f(17850, 25000, 25000), 2000, 0);
	test.test(Vector3f(17950, 25000, 25000), 2000, 1);

	test.test(Vector3f(25000, 17850, 25000), 2000, 0);
	test.test(Vector3f(25000, 17950, 25000), 2000, 1);

	test.test(Vector3f(25000, 25000, 17850), 2000, 0);
	test.test(Vector3f(25000, 25000, 17950), 2000, 1);

	std::cout << "done" << std::endl;

	return 0;
}
