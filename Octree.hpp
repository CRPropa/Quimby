/*
 * Octree.hpp
 *
 *  Created on: 02.02.2010
 *      Author: gmueller
 */

#ifndef OCTREE_HPP_
#define OCTREE_HPP_

#include "AABB.hpp"

#include <vector>

template<typename T, typename R>
class OctreeNode {
public:

	typedef R element_t;

	// Top/Bottom Left/Right Front/Back
	enum {
		xyz = 0, Xyz, xYz, XYz, xyZ, XyZ, xYZ, XYZ
	};

	template <typename U>
	class Settings {
	public:
		virtual ~Settings() {
		}
		virtual AABB<U> calculateAABB(element_t element) = 0;
		virtual unsigned int getLeafSize() = 0;
	};

	class QueryCallback {
	public:
		virtual void visit(OctreeNode *node, element_t element) = 0;
	};

	void insert(Settings<T> &settings, element_t element) {
		AABB<T> aabb = settings.calculateAABB(element);
		insert(settings, aabb, element);
	}

	void query(QueryCallback &callback, Settings<T> &settings, const AABB<T> &aabb) {
		if (aabb.intersects(box) == false)
			return;

		if (isLeaf()) {
			for (int i = 0; i < elements.size(); i++) {
				callback.visit(this, elements[i]);
			}
		} else {
			for (int i = 0; i < 8; i++)
				children[i]->query(callback, settings, aabb);
		}
	}

	void query(QueryCallback &callback, Settings<T> &settings, const Vector3<T> &v) {
		if (box.contains(v) == false) {
			return;
		}

		if (isLeaf()) {
			for (int i = 0; i < elements.size(); i++) {
				callback.visit(this, elements[i]);
			}
		} else {
			for (int i = 0; i < 8; i++)
				children[i]->query(callback, settings, v);
		}
	}

	void setAABB(const AABB<T> aabb) {
		box = aabb;
	}

private:
	AABB<T> box;
	std::vector< OctreeNode<T> > children;
	std::vector<element_t> elements;

	bool isLeaf() {
		return children.size() == 0;
	}

	void createChildren(Settings<T> &settings) {
		Vector3<T> halfExtend = (box.max - box.min) / 2;
		Vector3<T> halfExtendX = Vector3<T>(halfExtend.x, 0.0, 0.0);
		Vector3<T> halfExtendY = Vector3<T>(0.0, halfExtend.y, 0.0);
		Vector3<T> halfExtendZ = Vector3<T>(0.0, 0.0, halfExtend.z);
		Vector3<T> min = box.min;
		Vector3<T> max = box.min + halfExtend;
		children.resize(8);
		children[xyz].setAABB(AABB<T>(min, max));
		children[Xyz].setAABB(AABB<T>(min + halfExtendX, max + halfExtendX));
		children[xYz].setAABB(AABB<T>(min + halfExtendY, max + halfExtendY));
		children[XYz].setAABB(AABB<T>(min + halfExtendX + halfExtendY, max
				+ halfExtendX + halfExtendY));
		min = min + halfExtendZ;
		max = max + halfExtendZ;
		children[xyZ].setAABB(AABB<T>(min, max));
		children[XyZ].setAABB(AABB<T>(min + halfExtendX, max + halfExtendX));
		children[xYZ].setAABB(AABB<T>(min + halfExtendY, max + halfExtendY));
		children[XYZ].setAABB(AABB<T>(min + halfExtendX + halfExtendY, max
				+ halfExtendX + halfExtendY));

		for (int i = 0; i < elements.size(); i++) {
			insertIntoChildren(settings, settings.calculateAABB(elements[i]),
					elements[i]);
		}
		elements.clear();
		// make sure the memory is released
		std::vector<element_t>().swap(elements);
	}

	void insert(Settings<T> &settings, const AABB<T> &aabb, element_t element) {
		if (box.intersects(aabb) == false)
			return;

		if (isLeaf()) {
			if (elements.size() < settings.getLeafSize()) {
				elements.push_back(element);
			} else {
				createChildren(settings);
				insertIntoChildren(settings, aabb, element);
			}
		} else {
			insertIntoChildren(settings, aabb, element);
		}
	}

	void insertIntoChildren(Settings<T> &settings, const AABB<T> &aabb,
			element_t element) {
		for (int i = 0; i < 8; i++)
			children[i]->insert(settings, aabb, element);
	}

};

#endif /* OCTREE_HPP_ */
