#include "gadget/SmoothParticle.hpp"
#include "gadget/MagneticField.hpp"

#if GADGET_ROOT_ENABLED
#	include "TNtuple.h"
#	include "TFile.h"
#	include "Compression.h"
#endif

#include <fstream>
#include <cmath>
#include <cstdlib>
#include <memory>

const Vector3f ComaPositionKpc(119717, 221166, 133061);
const float ComaRadiusKpc = 2693.1857;

class Test {

	std::auto_ptr<DirectMagneticField> dmf;
	std::vector<SmoothParticle> particles;
public:
	void init() {
		std::cout << "** Load Particles" << std::endl;
		SmoothParticleHelper::read("test/coma.raw", particles);
		for (size_t i = 0; i < particles.size(); i++) {
			particles[i].smoothingLength;
		}

		std::cout << "** Load DirectMagneticField" << std::endl;
		float size = ComaRadiusKpc * 3;
		Vector3f origin = ComaPositionKpc - Vector3f(size / 2);
		dmf.reset(new DirectMagneticField(origin, size));
		dmf->init(50);
		dmf->load(particles);
	}

	void writeMedianProfile() {
		std::cout << "** Write Median Profile" << std::endl;
		std::ofstream out("coma_profile_median.csv");
		for (size_t i = 0; i < particles.size(); i++) {
			if (i % 10000 == 0) {
				std::cout << ".";
				std::cout.flush();
			}

			const SmoothParticle &sp = particles[i];
			float r = sp.position.distanceTo(ComaPositionKpc);
			float b = sp.bfield.length();
			out << r << " " << b << std::endl;
		}
		std::cout << std::endl;
	}

	Vector3f randomUnitVector() {
		double z = drand48() * 2 - 1;
		double t = (drand48() * 2 - 1) * M_PI;
		Vector3f u;
		u.x = sqrt(1 - z * z) * cos(t);
		u.y = sqrt(1 - z * z) * sin(t);
		u.z = z;
		return u;
	}

	void writeVolumeWeightedProfile() {
		std::cout << "** Write Volume Weighted Profile" << std::endl;

		const size_t steps = 50;
		const float rMin = 5;
		const float rMax = ComaRadiusKpc;
		const size_t nPoints = 10000;

		std::ofstream out("coma_profile_volume_weighted.csv");
		out << "r B" << std::endl;

		float stepLog = (log10(rMax) - log10(rMin)) / (steps - 1);
		for (size_t i = 0; i < steps; i++) {
			std::cout << ".";
			std::cout.flush();

			float r = pow(10, log10(rMin) + stepLog * i);

			float avgB = 0;
			for (size_t i = 0; i < nPoints; i++) {
				Vector3f p = randomUnitVector();
				p *= r;
				p += ComaPositionKpc;
				Vector3f b = dmf->getField(p);
				avgB += b.length(); // / nPoints;
			}
			avgB /= nPoints;
			out << r << " " << avgB << std::endl;
		}

		std::cout << std::endl;
	}

	void writeRhoTest() {
		std::cout << "** Write Rho Values" << std::endl;

#if GADGET_ROOT_ENABLED
		TFile *file = new TFile("coma_profile_rho.root", "RECREATE", "Rho");
		if (file->IsZombie())
			throw std::runtime_error(
					"Root output file cannot be properly opened");
		TNtuple *ntuple = new TNtuple("events", "Rho", "r:rho_i:rho_at_r_i:overlaps");
#else
		std::ofstream out("coma_profile_rho.csv");
		out << "r rho_i rho_at_r_i overlaps" << std::endl;
#endif

		for (size_t i = 0; i < particles.size(); i++) {
			try {
				size_t overlaps;
				float directRho = dmf->getRho(particles[i].position, overlaps);
				float r = particles[i].position.distanceTo(ComaPositionKpc);
#if GADGET_ROOT_ENABLED
				ntuple->Fill(r, particles[i].rho, directRho, overlaps);
#else
				out << r << " " << particles[i].rho << " " << directRho << " " << overlaps
				<< std::endl;
#endif
			} catch (invalid_position &e) {

			}
		}
#if GADGET_ROOT_ENABLED
		file->Write();
		file->Close();
#endif
		std::cout << std::endl;
	}

};

int main() {

	Test test;
	test.init();
	test.writeMedianProfile();
	test.writeVolumeWeightedProfile();
	test.writeRhoTest();

	std::cout << "** Finished" << std::endl;

	return 0;
}
