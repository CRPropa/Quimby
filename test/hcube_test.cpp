#include "gadget/MagneticField.h"
#include "gadget/SmoothParticle.h"
#include "gadget/HCube.h"

using namespace gadget;

int main() {
	ref_ptr<FileDatabase> db = new FileDatabase;
	db->open("/home/gmueller/Workspaces/Tracking/gadget/test/coma-mhd_z.db");
	float size = (db->getUpperBounds() - db->getLowerBounds()).length();

	//hc.init(db, gadget.Vector3f(117200, 118600, 130500), 5000, 0.1, 0)
	HCubeFile4::create(db, db->getLowerBounds(), size, 0.5, 1e-14, 2,
			"test.hc4");

	HCubeFile4 hf4("test.hc4");
	const HCube4 *hc = hf4.get();
	for (size_t i = 0; i < 10; i++) {
		std::cout << hc->getValue(Vector3f(i * size / 10), size) << std::endl;
	}
}
