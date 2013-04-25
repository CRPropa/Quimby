#include "quimby/Database.h"

#include <vector>
#include <assert.h>
#include <iostream>

using namespace quimby;
using namespace std;

int main() {
	vector<SmoothParticle> particles(1);
	particles[0].position = Vector3f(1, 2, 3);
	particles[0].smoothingLength = 1;

	FileDatabase::create(particles, "database_test.db", 5);

	FileDatabase db;
	db.open("database_test.db");

	cout << db.getCount() << endl;
	cout << db.getLowerBounds() << endl;
	cout << db.getUpperBounds() << endl;

	particles.clear();
	db.getParticles(Vector3f(0, 0, 0), Vector3f(4, 4, 4), particles);
	if (particles.size() != 1) {
		cerr << "Error, not one particle!" << endl;
	}

	if (particles[0].position.x != 1)
		cerr << "Error, wriong x!" << endl;
	if (particles[0].position.y != 2)
		cerr << "Error, wriong y!" << endl;
	if (particles[0].position.z != 3)
		cerr << "Error, wriong z!" << endl;

	particles.clear();
	db.getParticles(Vector3f(5, 0, 0), Vector3f(6, 4, 4), particles);
	if (particles.size() != 0) {
		cerr << "Error, more particles!" << endl;
	}

	return 0;
}
