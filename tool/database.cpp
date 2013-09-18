#include "arguments.h"

#include "quimby/Database.h"
#include "quimby/GadgetFile.h"

#include <iostream>
#include <stdexcept>

using namespace quimby;
using namespace std;

const char database_usage[] =
		"create database file from GADGET files.\n"
				"\nOptions:\n\n"
				"-f     list of input files, space seperated\n"
				"-o     filename of the database\n"
				"-h     Hubble constant to use, default: use value from file\n"
				"-px, -py, -pz\n"
				"       x, y, z of the pivot point for hubble streching, default: 120000\n"
				"-bins  number of bins used for database lookup, default: 100\n";

int database(Arguments &arguments) {
	vector<string> files;
	arguments.getVector("-f", files);
	string output = arguments.getString("-o", "");
	float hubble = arguments.getFloat("-h", 0);
	Vector3f pivot;
	pivot.x = arguments.getFloat("-px", 0);
	pivot.y = arguments.getFloat("-py", 0);
	pivot.z = arguments.getFloat("-pz", 0);
	size_t bins = arguments.getFloat("-bins", 100);

	if ((files.size() == 0) || (output.size() == 0)) {
		cout << database_usage << endl;
		return 1;
	}

	cout << "Output:   " << output << endl;
	cout << "Pivot:    " << pivot << " kpc" << endl;
	cout << "Bins:     " << bins << endl;

	vector<SmoothParticle> particles;
	for (size_t iArg = 0; iArg < files.size(); iArg++) {
		cout << "Load " << files[iArg] << " (" << (iArg + 1) << "/"
				<< files.size() << ")" << endl;

		GadgetFile file;
		file.open(files[iArg]);
		if (file.good() == false) {
			throw runtime_error("Failed to open file " + files[iArg]);
		}

		file.readHeader();
		int pn = file.getHeader().particleNumberList[0];
		if (pn < 1)
			return 0;

		float h = hubble;
		if (h == 0)
			h = file.getHeader().hubble;
		cout << "  Hubble: " << h << endl;

		float constMass = file.getHeader().massList[0];

		cout << "  Read POS Block..." << endl;
		vector<float> pos;
		if (file.readFloatBlock("POS ", pos) == false) {
			throw runtime_error("Failed read POS from file " + files[iArg]);
		}

		cout << "  Read BFLD Block..." << endl;
		vector<float> bfld;
		if (file.readFloatBlock("BFLD", bfld) == false) {
			throw runtime_error("Failed read BFLD from file " + files[iArg]);
		}

		cout << "  Read HSML Block..." << endl;
		vector<float> hsml;
		if (file.readFloatBlock("HSML", hsml) == false) {
			throw runtime_error("Failed read HSML from file " + files[iArg]);
		}

		cout << "  Read RHO Block..." << endl;
		vector<float> rho;
		if (file.readFloatBlock("RHO ", rho) == false) {
			throw runtime_error("Failed read RHO from file " + files[iArg]);
		}

		vector<float> mass;
		if (constMass == 0) {
			cout << "  Read MASS Block..." << endl;
			if (file.readFloatBlock("MASS ", mass) == false) {
				throw runtime_error(
						"Failed read MASS from file " + files[iArg]);
			}
		} else {
			cout << "  Using constant mass: " << constMass << endl;

		}

		for (int iP = 0; iP < pn; iP++) {
			SmoothParticle particle;
			particle.smoothingLength = hsml[iP];
			particle.position.x = pos[iP * 3];
			particle.position.y = pos[iP * 3 + 1];
			particle.position.z = pos[iP * 3 + 2];

			particle.bfield.x = bfld[iP * 3];
			particle.bfield.y = bfld[iP * 3 + 1];
			particle.bfield.z = bfld[iP * 3 + 2];

			if (constMass == 0)
				particle.mass = mass[iP];
			else
				particle.mass = constMass;

			particle.rho = rho[iP];

			particle.toKpc(h, pivot);

			particles.push_back(particle);
		}
	}

	cout << "create database with " << particles.size() << " particles."
			<< endl;
	FileDatabase::create(particles, output, bins, true);

	cout << "done." << endl;

	FileDatabase db;
	db.open(output);
	cout << "Resulting Database:" << endl;
	cout << " count: " << db.getCount() << endl;
	cout << " lower: " << db.getLowerBounds() << endl;
	cout << " upper: " << db.getUpperBounds() << endl;

	return 0;
}
