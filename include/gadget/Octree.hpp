#ifndef GADGET_OCTREE_HPP_
#define GADGET_OCTREE_HPP_

#include "AABC.hpp"

#include <vector>

namespace gadget {

template<typename FLOAT, typename ELEMENT>
class OctreeNode {
public:

	typedef ELEMENT element_t;
	typedef FLOAT float_t;

	// Top/Bottom Left/Right Front/Back
	enum {
		xyz = 0, Xyz, xYz, XYz, xyZ, XyZ, xYZ, XYZ
	};

	class Settings {
	public:
		virtual ~Settings() {
		}
		virtual AABC<float_t> calculateBounds(const element_t &element) = 0;
		virtual unsigned int getLeafSize() = 0;
		virtual float_t getLeafExtend() = 0;
	};

	class QueryCallback {
	public:
		virtual void visit(OctreeNode *leaf) = 0;
	};

	void insert(Settings &settings, const element_t &element) {
		AABC<float_t> bounds = settings.calculateBounds(element);
		insert(settings, bounds, element);
	}

	void query(QueryCallback &callback, Settings &settings,
			const AABC<float_t> &bounds) {
		if (bounds.intersects(box) == false)
			return;

		if (isLeaf()) {
			callback.visit(this);
		} else {
			for (int i = 0; i < 8; i++)
				children[i]->query(callback, settings, bounds);
		}
	}

	const OctreeNode *queryLeafNode(const Settings &settings, const Vector3<
			float_t> &v) {
		if (box.contains(v) == false) {
			return 0;
		}

		if (isLeaf()) {
			return this;
		} else {
			const OctreeNode *node = 0;
			for (int i = 0; i < 8; i++) {
				node = children[i].queryLeafNode(settings, v);
				if (node)
					return node;
			}

			return node;
		}
	}

	void setBoundingVolume(const AABC<float_t> aabb) {
		box = aabb;
	}

	void stats(unsigned int &maxdepth, unsigned int &count,
			unsigned int &element_count, unsigned int depth = 0) {
		count++;
		element_count += elements.size();
		if (depth + 1 > maxdepth)
			maxdepth = depth + 1;
		for (int i = 0; i < children.size(); i++)
			children[i].stats(maxdepth, count, element_count, depth + 1);
	}

	const std::vector<element_t> &getElements() const {
		return elements;
	}
private:
	AABC<float_t> box;
	std::vector<OctreeNode<float_t, element_t> > children;
	std::vector<element_t> elements;

	bool isLeaf() {
		return children.size() == 0;
	}

	void createChildren(Settings &settings) {
		float_t halfExtend = box.extend / 2;
		Vector3<float_t> halfExtendX = Vector3<float_t> (box.extend, 0.0, 0.0);
		Vector3<float_t> halfExtendY = Vector3<float_t> (0.0, box.extend, 0.0);
		Vector3<float_t> halfExtendZ = Vector3<float_t> (0.0, 0.0, box.extend);
		Vector3<float_t> min = box.center - Vector3<float_t> (box.extend / 2,
				box.extend / 2, box.extend / 2);
		children.resize(8);
		children[xyz].setBoundingVolume(AABC<float_t> (min, halfExtend));
		children[Xyz].setBoundingVolume(AABC<float_t> (min + halfExtendX,
				halfExtend));
		children[xYz].setBoundingVolume(AABC<float_t> (min + halfExtendY,
				halfExtend));
		children[XYz].setBoundingVolume(AABC<float_t> (min + halfExtendX
				+ halfExtendY, halfExtend));
		min = min + halfExtendZ;
		children[xyZ].setBoundingVolume(AABC<float_t> (min, halfExtend));
		children[XyZ].setBoundingVolume(AABC<float_t> (min + halfExtendX,
				halfExtend));
		children[xYZ].setBoundingVolume(AABC<float_t> (min + halfExtendY,
				halfExtend));
		children[XYZ].setBoundingVolume(AABC<float_t> (min + halfExtendX
				+ halfExtendY, halfExtend));

		for (int i = 0; i < elements.size(); i++) {
			AABC<float_t> aabc = settings.calculateBounds(elements[i]);
			insertIntoChildren(settings, aabc, elements[i]);
		}
		elements.clear();
		// make sure the memory is released
		std::vector<element_t>().swap(elements);
	}

	void insert(Settings &settings, const AABC<float_t> &aabb,
			const element_t &element) {
		if (box.intersects(aabb) == false)
			return;

		if (isLeaf()) {
			if (elements.size() < settings.getLeafSize() || box.extend
					< settings.getLeafExtend()) {
				elements.push_back(element);
			} else {
				createChildren(settings);
				insertIntoChildren(settings, aabb, element);
			}
		} else {
			insertIntoChildren(settings, aabb, element);
		}
	}

	void insertIntoChildren(Settings &settings, const AABC<float_t> &aabb,
			const element_t &element) {
		for (int i = 0; i < 8; i++)
			children[i].insert(settings, aabb, element);
	}

};

} // namespace gadget

#endif /* GADGET_OCTREE_HPP_ */
