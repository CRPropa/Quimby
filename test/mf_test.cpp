/*
 * mf_test.cpp
 *
 *  Created on: Aug 7, 2011
 *      Author: gmueller
 */

#include "gadget/MagneticField.hpp"
#include "gadget/SmoothParticle.hpp"

#include <memory>
#include <limits>
#include <stdexcept>

class AbstractTest {
protected:
	std::vector<SmoothParticle> particles;
	std::auto_ptr<DirectMagneticField> dmf;
	std::auto_ptr<SampledMagneticField> smf;
	std::vector<std::pair<Vector3f, float> > positions;
public:

	virtual void setup() = 0;

	void run() {
		for (size_t i = 0; i < positions.size(); i++)
			testPosition(positions[i].first, positions[i].second);
	}

	void testPosition(const Vector3f &position, float expected_error =
			std::numeric_limits<float>::epsilon()) {
		Vector3f df = dmf->getField(position);
		Vector3f sf = smf->getField(position);
		float error = (df - sf).length();
		std::cout << "Direct: " << df << ", Sampled: " << sf << ", Error: "
				<< error << std::endl;
		if (error > expected_error)
			throw std::runtime_error("unexpected deviation!");
	}

	void addPosition(const Vector3f &position, float expected_error =
			std::numeric_limits<float>::epsilon()) {
		positions.push_back(std::make_pair(position, expected_error));
	}
};

class SimpleTest: public AbstractTest {
public:

	void setup() {
		particles.resize(1);
		particles[0].bfield = Vector3f(0, 1, 0);
		particles[0].position = Vector3f(120000, 120000, 120000);
		particles[0].smoothingLength = 10000;
		particles[0].mass = 1;

		SmoothParticleHelper::updateRho(particles);

		Vector3f origin(100000, 100000, 100000);
		float size(40000);

		dmf.reset(new DirectMagneticField(origin, size));
		dmf->init(100);
		dmf->load(particles);

		smf.reset(new SampledMagneticField(origin, size));
		smf->init(500);
		smf->load(particles);

		addPosition(Vector3f(120000, 120000, 120000));
		addPosition(Vector3f(120500, 120500, 120500));
		addPosition(Vector3f(120550, 120550, 120550), 0.01);
	}

};

int main() {
	SimpleTest stest;
	stest.setup();
	stest.run();
}
