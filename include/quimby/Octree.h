#pragma once

#include "AABB.h"

#include <vector>

namespace quimby {

template<typename FLOAT, typename ELEMENT>
class OctreeNode {
public:

	typedef ELEMENT element_t;
	typedef FLOAT float_t;
	typedef Vector3<FLOAT> vector_t;
	typedef AABB<FLOAT> aabb_t;

	// Top/Bottom Left/Right Front/Back
	enum eSector {
		xyz = 0, Xyz, xYz, XYz, xyZ, XyZ, xYZ, XYZ
	};

	class Settings {
	public:
		virtual ~Settings() {
		}
		virtual aabb_t calculateBounds(const element_t &element) = 0;
		virtual unsigned int getLeafSize() = 0;
		virtual float_t getLeafExtend() = 0;
	};

	class QueryCallback {
	public:
		QueryCallback() :
				abortQuery(false) {

		}

		virtual ~QueryCallback() {

		}
		bool abortQuery;
		virtual void visit(OctreeNode *leaf) = 0;
	};

	void insert(Settings &settings, const element_t &element) {
		aabb_t bounds = settings.calculateBounds(element);
		insert(settings, bounds, element);
	}

	void query(QueryCallback &callback, Settings &settings,
			const aabb_t &bounds) {
		if (callback.abortQuery)
			return;

		if (bounds.intersects(box) == false)
			return;

		if (isLeaf()) {
			callback.visit(this);
		} else {
			for (int i = 0; i < 8; i++)
				children[i].query(callback, settings, bounds);
		}
	}

	const OctreeNode *queryLeafNode(const Settings &settings,
			const vector_t &v) {
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

	void setBoundingVolume(const aabb_t aabb) {
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

//	aabb_t subVolume(eSector s) {
//		switch (s) {
//		case xyz:
//			return aabb_t(box.min, (box.max + box.min) / 2);
//		case Xyz:
//			return aabb_t(box.min, (box.max + box.min) / 2);
//		case xyz:
//			return aabb_t(box.min, (box.max + box.min) / 2);
//		case xyz:
//			return aabb_t(box.min, (box.max + box.min) / 2);
//		case xyz:
//			return aabb_t(box.min, (box.max + box.min) / 2);
//		}
//	}

private:
	aabb_t box;
	std::vector<OctreeNode<float_t, element_t> > children;
	std::vector<element_t> elements;

	bool isLeaf() {
		return children.size() == 0;
	}

	void createChildren(Settings &settings) {
		vector_t c = (box.max + box.min) / 2;
		vector_t &l = box.min;
		vector_t &u = box.max;
		children.resize(8);
		children[xyz].setBoundingVolume(aabb_t(l.x, l.y, l.z, c.x, c.y, c.z));
		children[Xyz].setBoundingVolume(aabb_t(c.x, l.y, l.z, u.x, c.y, c.z));
		children[xYz].setBoundingVolume(aabb_t(l.x, c.y, l.z, c.x, u.y, c.z));
		children[XYz].setBoundingVolume(aabb_t(c.x, c.y, l.z, u.x, u.y, c.z));
		children[xyZ].setBoundingVolume(aabb_t(l.x, l.y, c.z, c.x, c.y, u.z));
		children[XyZ].setBoundingVolume(aabb_t(c.x, l.y, c.z, u.x, c.y, u.z));
		children[xYZ].setBoundingVolume(aabb_t(l.x, c.y, c.z, c.x, u.y, u.z));
		children[XYZ].setBoundingVolume(aabb_t(c.x, c.y, c.z, u.x, u.y, u.z));

		for (int i = 0; i < elements.size(); i++) {
			aabb_t AABB = settings.calculateBounds(elements[i]);
			insertIntoChildren(settings, AABB, elements[i]);
		}
		elements.clear();
		// make sure the memory is released
		std::vector<element_t>().swap(elements);
	}

	void insert(Settings &settings, const aabb_t &aabb,
			const element_t &element) {
		if (box.intersects(aabb) == false)
			return;

		if (isLeaf()) {
			if (elements.size() < settings.getLeafSize()
					|| (box.min - box.max).length()
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

	void insertIntoChildren(Settings &settings, const aabb_t &aabb,
			const element_t &element) {
		for (int i = 0; i < 8; i++)
			children[i].insert(settings, aabb, element);
	}

};

} // namespace
