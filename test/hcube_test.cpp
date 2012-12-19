#include "gadget/MagneticField.h"
#include "gadget/SmoothParticle.h"
#include "gadget/HCube.h"

using namespace gadget;

int main() {
	ref_ptr<FileDatabase> db = new FileDatabase;
	db->open("/home/gmueller/Workspaces/Tracking/gadget/test/coma-mhd_z.db");
	float size = (db->getUpperBounds() - db->getLowerBounds()).length();

	HCube2 hc;
	//hc.init(db, gadget.Vector3f(117200, 118600, 130500), 5000, 0.1, 0)
	hc.init(db, db->getLowerBounds(), size, 0.1, 1);

	for (size_t i = 0; i < 10; i++) {
		std::cout << hc.getValue(Vector3f(i * size / 10), size) << std::endl;
	}

	hc.save("test.hc");
}
