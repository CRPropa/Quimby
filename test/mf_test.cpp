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
#include <stdlib.h>

void test_value(float value, float expected) {
	if (std::fabs(value - expected) > 0.00001) {
		throw std::runtime_error("unexpected deviation!");
	}
}

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

	void getAverageFieldOnSphere(const Vector3f &center, float lower_radius,
			float delta_radius, float &avgSampled, float &avgDirect,
			float &avgDirectWeighted, float &avgInvRho) {
		avgDirect = 0;
		avgDirectWeighted = 0;
		avgSampled = 0;
		avgInvRho = 0;
		size_t count = 0;
#if 0
		float phi = 0;
		while (phi < 360) {
			float costheta = -1 + 0.05;
			while (costheta < 1) {
				Vector3f position = center;
//				try {
				position.x += radius * sin(phi)
				* ::sqrt(1. - costheta * costheta);
				position.y += radius * cos(phi)
				* ::sqrt(1. - costheta * costheta);
				position.z += radius * costheta;
				avgDirect += dmf->getField(position).length();
				avgSampled += smf->getField(position).length();
				count++;
//				} catch (...) {
//					std::cerr << "invalud"
//				}
				costheta += 0.05;
			}
			phi += 5;
		}
#endif

		for (size_t i = 0; i < 100000; i++) {
			Vector3f r;

			// trig method for random unit vectors on a sphere
			double z = drand48() * 2 - 1;
			double t = (drand48() * 2 - 1) * M_PI;
			r.x = sqrt(1 - z * z) * cos(t);
			r.y = sqrt(1 - z * z) * sin(t);
			r.z = z;

			r *= lower_radius + drand48() * delta_radius;
			r += center;
			Vector3f b = dmf->getField(r);
			size_t co;
			float rho = dmf->getRho(r, co);
			avgDirectWeighted += b.length() * rho;
			avgDirect += b.length();
			avgInvRho += rho;
			avgSampled += smf->getField(r).length();
			count++;
		}
		avgDirect /= count;
		avgDirectWeighted /= count;
		avgInvRho /= count;
		avgSampled /= count;
	}

	void runHaloTest(const Vector3f &center, float radius) {
		std::cout << ">> Halo Test around " << center << ", R_v: " << radius
				<< std::endl;
		std::cout << "  -     sampled direct directWeighted  invrho"
				<< std::endl;
		size_t steps = 100;
		float delta_radius = radius / steps;
		for (size_t i = 0; i < (steps + 1); i++) {
			float r = delta_radius * i;
			float avgSampled, avgDirect, avgDirectWeighted, avgInvRho;
			getAverageFieldOnSphere(center, r, delta_radius, avgSampled,
					avgDirect, avgDirectWeighted, avgInvRho);
			std::cout << r / radius << " " << avgSampled << " " << avgDirect
					<< " " << avgDirectWeighted << " " << avgInvRho
					<< std::endl;
		}

		std::cout << "DMF: " << dmf->getStatistics().getAverageActual() << "/ "
				<< dmf->getStatistics().getAverageTotal() << std::endl;
	}

	void runRhoTest() {
		for (size_t i = 0; i < particles.size(); i++) {
			try {
				size_t contrib;
				float directRho = dmf->getRho(particles[i].position, contrib);
				float relativeError = (particles[i].rho - directRho)
						/ particles[i].rho;
				if (std::fabs(relativeError) > 0.05) {
					std::cerr << "High Deviation: " << i << std::endl;
					std::cerr << " position:   " << particles[i].position
							<< std::endl;
					std::cerr << " calculated: " << directRho << std::endl;
					std::cerr << " particle:   " << particles[i].rho
							<< std::endl;
					std::cerr << " hsml:       " << particles[i].smoothingLength
							<< std::endl;
					std::cerr << " ratio:      " << directRho / particles[i].rho
							<< std::endl;
					std::cerr << " contrib:    " << contrib << std::endl;
					//throw std::runtime_error(
					//"unexpected deviation in RhoTest!");
				}
			} catch (invalid_position &e) {

			}
		}
	}

	void runBFieldTest() {
		for (size_t i = 0; i < particles.size(); i++) {
			try {
				size_t contrib;
				Vector3f directB = dmf->getField(particles[i].position);
				float relativeError = (particles[i].bfield.length()
						- directB.length()) / particles[i].bfield.length();
				if (std::fabs(relativeError) > 0.05) {
					std::cerr << "High Deviation: " << i << std::endl;
					std::cerr << " position:   " << particles[i].position
							<< std::endl;
					std::cerr << " calculated: " << directB << std::endl;
					std::cerr << " particle:   " << particles[i].bfield
							<< std::endl;
					std::cerr << " bratio:       "
							<< particles[i].bfield.length() / directB.length()
							<< std::endl;
					std::cerr << " hsml:       " << particles[i].smoothingLength
							<< std::endl;
				}
			} catch (invalid_position &e) {

			}
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
						Vector3f(0, 0.958897, 0), 0.000001));
		testPoints.push_back(
				TestPoint(Vector3f(120550, 120550, 120550),
						Vector3f(0, 0.950737, 0), 0.01));
	}

};

// Coma
// 11   411121   214459 7.570E+14 6.634E+14 1885.23     119.8023  190.8159  129.1431  -361.72   566.41  -315.91  949.39
Vector3f ComaPositionKpc(119717, 221164, 133061);
float ComaRadiusKpc = 2693.1857;
Vector3f ComaPosition(119802, 190816, 129143);
float ComaRadius = 1885.23;

//119802 +
class HaloTest: public AbstractTest {
public:

	void setup() {
		SmoothParticleHelper::read("test/coma.raw", particles);

		float size = ComaRadiusKpc * 2.1;
		Vector3f origin = ComaPositionKpc - Vector3f(size / 2);

		dmf.reset(new DirectMagneticField(origin, size));
		dmf->init(50);
		dmf->load(particles);

		smf.reset(new SampledMagneticField(origin, size));
		smf->init(100);
		smf->load(particles);

		testPoints.push_back(
				TestPoint(ComaPositionKpc,
						Vector3f(-1.60984e-09, -3.48952e-10, 1.00858e-09), 1));
	}

};

void KernelValueTest(float r, float v) {
	float res = SmoothParticle::kernel(r);
	if (std::fabs(res - v) > 0.0001) {
		std::cerr << "wrong kernel value! exptectd: " << v << " got: " << res
				<< " at position " << r << std::endl;
		throw std::runtime_error("unexpected deviation in KernelValueTest!");
	}
}

void KernelTest() {
	KernelValueTest(0.25, 0.71875);
	KernelValueTest(0.5, 0.25);
	KernelValueTest(0.75, 0.03125);
	KernelValueTest(2.0, 0.0);
}

int main() {
	SmoothParticle pa, pb;
	pa.bfield = Vector3f(0, 1, 0);
	pa.position = Vector3f(120000, 120000, 120000);
	pa.smoothingLength = 10000;
	pa.mass = 1;

	pb = pa;
	bool equal = pa.bfield == pb.bfield;
	if (!equal)
		throw std::runtime_error("unexpected deviation in pa and pb!");
	if (pa.bfield.length() != pb.bfield.length())
		throw std::runtime_error("unexpected deviation in pa and pb!");

	KernelTest();

	SimpleTest stest;
	stest.setup();
	stest.runTestPoints();
	stest.runSweep(Vector3f(120000, 120000, 120000),
			Vector3f(120000, 110000, 120000), 10, 0.01);
	float a, b, c, d;
	stest.getAverageFieldOnSphere(Vector3f(120000, 120000, 120000), 5000, 0, a,
			b, c, d);
	std::cout << "Average on Sphere: " << a << ", " << b << std::endl;
	//if (fabs(a - 1) > 0.01 || fabs(b - 0.25) > 0.01)
	//	throw std::runtime_error("unexpected deviation!");

	HaloTest htest;
	htest.setup();
//	htest.runTestPoints();
//	htest.runSweep(ComaPosition, ComaPosition + Vector3f(-1000, 1000, 500), 10,
//			0.01);
	htest.runRhoTest();
	htest.runBFieldTest();
	htest.runHaloTest(ComaPositionKpc, ComaRadiusKpc);
}
