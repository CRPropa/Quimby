#include "quimby/SmoothParticle.h"
#include "quimby/MagneticField.h"

#if GADGET_ROOT_ENABLED
#	include "TNtuple.h"
#	include "TFile.h"
#	include "Compression.h"
#endif

#include <fstream>
#include <cmath>
#include <cstdlib>
#include <memory>

using namespace quimby;

const Vector3f ComaPositionKpc(119717, 221166, 133061);
const float ComaRadiusKpc = 2693.1857;

class Test {

	std::auto_ptr<DirectMagneticField> dmf;
	std::auto_ptr<FileDatabase> db;
public:
	void init() {
		std::cout << "** Load Particles" << std::endl;
		db.reset(new FileDatabase);
		db->open("test/coma-mhd_z.db");

		std::cout << "** Load DirectMagneticField" << std::endl;
		float size = ComaRadiusKpc * 3;
		Vector3f origin = ComaPositionKpc - Vector3f(size / 2);
		dmf.reset(new DirectMagneticField(50));
		dmf->init(origin, size, *db.get());
	}

	class MedianVisitor: public DatabaseVisitor {
	public:
		std::ofstream out;
		size_t i;
		void begin(const Database &db) {
			i = 0;
		}
		void visit(const SmoothParticle &p) {
			if (i % 10000 == 0) {
				std::cout << ".";
				std::cout.flush();
			}

			float r = p.position.distanceTo(ComaPositionKpc);
			float b = p.bfield.length();
			out << r << " " << b << std::endl;
		}

		bool intersects(const Vector3f &lower, const Vector3f &upper, float margin) {
			return true;
		}

		void end() {
			out.close();
		}
	};

	void writeMedianProfile() {
		std::cout << "** Write Median Profile" << std::endl;
		MedianVisitor v;
		v.out.open("coma_profile_median.csv");
		db->accept(v);
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

#if GADGET_ROOT_ENABLED
		TFile *file = new TFile("coma_profile_volume_weighted.root", "RECREATE",
				"Rho Weighted");
		if (file->IsZombie())
		throw std::runtime_error(
				"Root output file cannot be properly opened");
		TNtuple *ntuple = new TNtuple("events", "Rho", "r:B");
#endif
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
				Vector3f b;
				bool isGood = dmf->getField(p, b);
				avgB += b.length(); // / nPoints;
			}
			avgB /= nPoints;

#if GADGET_ROOT_ENABLED
			ntuple->Fill(r, avgB);
#endif
			out << r << " " << avgB << std::endl;

		}

#if GADGET_ROOT_ENABLED
		file->Write();
		file->Close();
#endif
		std::cout << std::endl;
	}

	class WriteRhoVisitor: public DatabaseVisitor {
	public:
#if GADGET_ROOT_ENABLED
		TFile *file;
		TNtuple *ntuple;
#endif
		std::ofstream out;
		void begin(const Database &db) {

		}
		void visit(const SmoothParticle &p) {
			size_t overlaps;
			//float directRho = dmf->getRho(particles[i].position, overlaps);
			//float r = particles[i].position.distanceTo(ComaPositionKpc);
#if GADGET_ROOT_ENABLED
				ntuple->Fill(r, particles[i].rho, directRho, overlaps);
#endif
			//out << r << " " << particles[i].rho << " " << directRho << " "
			//		<< overlaps << std::endl;
		}

		bool intersects(const Vector3f &lower, const Vector3f &upper, float margin) {
			return true;
		}
		void end() {
		}
	};

	void writeRhoTest() {
		std::cout << "** Write Rho Values" << std::endl;

		WriteRhoVisitor v;
#if GADGET_ROOT_ENABLED
		v.file = new TFile("coma_profile_rho.root", "RECREATE", "Rho");
		if (v.file->IsZombie())
		throw std::runtime_error(
				"Root output file cannot be properly opened");
		v.ntuple = new TNtuple("events", "Rho",
				"r:rho_i:rho_at_r_i:overlaps");
#endif
		v.out.open("coma_profile_rho.csv");
		v.out << "r rho_i rho_at_r_i overlaps" << std::endl;

		db->accept(v);

#if GADGET_ROOT_ENABLED
		v.file->Write();
		v.file->Close();
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
