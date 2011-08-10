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
public:
	struct TestPoint {
		TestPoint() :
				position(Vector3f(0, 0, 0)), value(Vector3f(0, 0, 0)), error(
						std::numeric_limits<float>::epsilon()) {

		}
		TestPoint(Vector3f position, Vector3f value, float error =
				std::numeric_limits<float>::epsilon()) :
				position(position), value(value), error(error) {

		}
		Vector3f position;
		Vector3f value;
		float error;
	};
protected:
	std::vector<SmoothParticle> particles;
	std::auto_ptr<DirectMagneticField> dmf;
	std::auto_ptr<SampledMagneticField> smf;
	std::vector<TestPoint> testPoints;
public:

	virtual void setup() = 0;

	void runTestPoints() {
		std::cout << ">> test points: " << testPoints.size() << std::endl;
		for (size_t i = 0; i < testPoints.size(); i++)
			test(testPoints[i]);
		std::cout << ">> done" << std::endl;
	}

	void runSweep(const Vector3f &from, const Vector3f &to, float stepSize,
			float allowedError) {
		std::cout << ">> Sweep from " << from << " to " << to << ", Step: "
				<< stepSize << std::endl;
		float length = (from - to).length();
		size_t steps = length / stepSize;
		Vector3f step = (to - from) * stepSize / length;
		Vector3f position = from;
		float minError = std::numeric_limits<float>::max();
		float maxError = std::numeric_limits<float>::min();
		for (size_t i = 0; i < steps; i++) {
			position += step;
			Vector3f df = dmf->getField(position);
			Vector3f sf = smf->getField(position);
			std::cout << "   -> " << df.length() << std::endl;
			float error = (df - sf).length();
			minError = std::min(error, minError);
			maxError = std::max(error, maxError);
			if (error > allowedError)
				throw std::runtime_error("unexpected deviation!");
		}
		std::cout << "   min error: " << minError << std::endl;
		std::cout << "   max error: " << maxError << std::endl;
		std::cout << "   average total: "
				<< dmf->getStatistics().getAverageTotal() << std::endl;
		std::cout << "   average actual: "
				<< dmf->getStatistics().getAverageActual() << std::endl;
		std::cout << ">> done" << std::endl;
	}

	void test(const TestPoint &testPoint) {
		std::cout << "   TestPoint: " << testPoint.position << " -> "
				<< testPoint.value << ", +- " << testPoint.error << std::endl;

		Vector3f df = dmf->getField(testPoint.position);
		float dfError = (df - testPoint.value).length();
		std::cout << "    - Direct: " << df << ", Error: " << dfError
				<< std::endl;

		Vector3f sf = smf->getField(testPoint.position);
		float sfError = (sf - testPoint.value).length();
		std::cout << "    - Samples: " << sf << ", Error: " << sfError
				<< std::endl;

		if (dfError > testPoint.error || sfError > testPoint.error)
			throw std::runtime_error("unexpected deviation!");
	}

	void getAverageFieldOnSphere(const Vector3f &center, float radius,
			float &avgDirect, float &avgSampled) {
		avgDirect = 0;
		avgSampled = 0;
		size_t count = 0;
		float phi = 0;
		while (phi < 360) {
			float theta = -90;
			while (theta <= 90) {
				theta += 10;
				Vector3f position = center;
				try {
					position.x += radius * sin(phi) * cos(theta);
					position.y += radius * cos(phi) * cos(theta);
					position.z += radius * sin(theta);
					avgDirect += dmf->getField(position).length();
					avgSampled += smf->getField(position).length();
					count++;
				} catch (...) {

				}
			}
			phi += 10;
		}

		avgDirect /= count;
		avgSampled /= count;
	}

	void runHaloTest(const Vector3f &center, float radius) {
		std::cout << ">> Halo Test around " << center << ", R_v: " << radius
				<< std::endl;
		std::cout << "  -     direct    sampled" << std::endl;
		for (size_t i = 0; i < 51; i++) {
			float r = (radius / 50) * i;
			float avgDirect, avgSampled;
			getAverageFieldOnSphere(center, r, avgDirect, avgSampled);
			std::cout << r/radius << " " << avgDirect << " " << avgSampled
					<< std::endl;
		}
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

		testPoints.push_back(
				TestPoint(Vector3f(120000, 120000, 120000), Vector3f(0, 1, 0)));
		testPoints.push_back(
				TestPoint(Vector3f(120500, 120500, 120500),
						Vector3f(0, 0.958897, 0)));
		testPoints.push_back(
				TestPoint(Vector3f(120550, 120550, 120550),
						Vector3f(0, 0.950737, 0), 0.01));
	}

};

// Coma
// 11   411121   214459 7.570E+14 6.634E+14 1885.23     119.8023  190.8159  129.1431  -361.72   566.41  -315.91  949.39
Vector3f ComaPosition(119717, 221164, 133061);

class HaloTest: public AbstractTest {
public:

	void setup() {
		SmoothParticleHelper::read("test/coma-0.7.raw", particles);

		Vector3f origin(99000, 219000, 119000);
		float size(22000);

		dmf.reset(new DirectMagneticField(origin, size));
		dmf->init(50);
		dmf->load(particles);

		smf.reset(new SampledMagneticField(origin, size));
		smf->init(100);
		smf->load(particles);

		testPoints.push_back(
				TestPoint(ComaPosition,
						Vector3f(-1.60984e-09, -3.48952e-10, 1.00858e-09), 1));
	}

};

int main() {
	SimpleTest stest;
	stest.setup();
	stest.runTestPoints();
	stest.runSweep(Vector3f(120000, 120000, 120000),
			Vector3f(120000, 110000, 120000), 10, 0.01);

	HaloTest htest;
	htest.setup();
	htest.runTestPoints();
	htest.runSweep(ComaPosition, ComaPosition + Vector3f(-1000, 1000, 500), 10,
			0.01);
	htest.runHaloTest(ComaPosition, 2700);
}
