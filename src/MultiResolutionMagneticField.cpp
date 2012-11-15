#include "gadget/MultiResolutionMagneticField.h"
#include "gadget/Octree.h"

#include <set>
#include <iostream>

namespace gadget {

using namespace std;

bool slsort(const SmoothParticle &a, const SmoothParticle &b) {
	return (a.smoothingLength < b.smoothingLength);
}

typedef OctreeNode<float, MultiResolutionMagneticFieldCell *> MultiResolutionMagneticFieldCellOctree;
class MultiResolutionMagneticFieldCellOctreeSettings: public MultiResolutionMagneticFieldCellOctree::Settings {

	AABB<MultiResolutionMagneticFieldCellOctree::float_t> calculateBounds(
			const MultiResolutionMagneticFieldCellOctree::element_t &element) {
		return element->bounds;
	}
	unsigned int getLeafSize() {
		return 100;
	}
	MultiResolutionMagneticFieldCellOctree::float_t getLeafExtend() {
		return 100;
	}
};

class _InclusionQuery: public MultiResolutionMagneticFieldCellOctree::QueryCallback {
public:
	AABB<float> vol;
	bool inCell;
	_InclusionQuery(AABB<float> aabb) :
			MultiResolutionMagneticFieldCellOctree::QueryCallback(), vol(aabb), inCell(
					false) {

	}

	void visit(MultiResolutionMagneticFieldCellOctree *leaf) {
		const std::vector<MultiResolutionMagneticFieldCell *> &cells =
				leaf->getElements();
		for (size_t i = 0; i < cells.size(); i++) {
			if (cells[i]->bounds.contains(vol)) {
				inCell = true;
				abortQuery = true;
				break;
			}
		}
	}
};

void MultiResolutionMagneticField::create(Database &db, const string &filename,
		size_t bins, float minres, float a, float b, float c) {
	vector<SmoothParticle> particles;
	vector<MultiResolutionMagneticFieldCell *> cells;
	MultiResolutionMagneticFieldCellOctree octree;
	MultiResolutionMagneticFieldCellOctreeSettings octree_settings;
	octree.setBoundingVolume(
			AABB<float>(db.getLowerBounds(), db.getUpperBounds()));
	cout << " - load all particles" << endl;
	particles.reserve(db.getCount());
	db.getParticles(db.getLowerBounds(), db.getUpperBounds(), particles);
	float extend = (db.getUpperBounds() - db.getLowerBounds()).length();
	//db.getParticles(Vector3f(70000), Vector3f(170000), particles);

	cout << " - sort particles" << endl;
	sort(particles.begin(), particles.end(), slsort);

	cout << " - create cells" << endl;
	size_t idx = 0;
	size_t kbytes = 0;
	while (idx < particles.size()) {

		SmoothParticle &p = particles[idx];

		AABB<float> aabb(p.position - Vector3f(p.smoothingLength * 0.8),
				p.position + Vector3f(p.smoothingLength * 0.8));
		_InclusionQuery inclusion_query(aabb);

		octree.query(inclusion_query, octree_settings, aabb);
		if (inclusion_query.inCell) {
			idx++;
			continue;
		}
		//cout << idx << endl;
		// build cell
		MultiResolutionMagneticFieldCell *cell =
				new MultiResolutionMagneticFieldCell;

		float relSL = p.smoothingLength / particles.back().smoothingLength;
		float res = max(minres, p.smoothingLength);
		int b = bins;
		if (relSL > a)
			b = bins / 8;
		else if (relSL > b)
			b = bins / 4;
		else if (relSL > c)
			b = bins / 2;

		cell->bounds.min = p.position - Vector3f(b * res / 2.);
		cell->bounds.max = p.position + Vector3f(b * res / 2.);

		cells.push_back(cell);
		octree.insert(octree_settings, cell);

		//cout << "  Resolution: " << p.smoothingLength / factor << endl;
		cerr << idx << " " << cell->bounds.min << " "
				<< (cell->bounds.max - cell->bounds.min) << " " << res << " "
				<< b << endl;
		idx++;

		kbytes += (bins * bins * bins) * 12 / 1000;

	}

	std::cout << "Memory: " << kbytes / 1000 / 1000 << "."
			<< (kbytes / 1000) % 1000 << "." << (kbytes % 1000) << "kB"
			<< std::endl;
}

}
