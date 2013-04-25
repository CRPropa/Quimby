#pragma once

#include "Vector3.h"

#include <iostream>

namespace quimby {

template<typename T>
class AABB {
public:
	Vector3<T> min;
	Vector3<T> max;

	AABB() {
	}

	AABB(const T &ax, const T &ay, const T &az, const T &bx, const T &by,
			const T &bz) :
			min(ax, ay, az), max(bx, by, bz) {

	}

	AABB(const AABB<T> &a) :
			min(a.min), max(a.max) {

	}

	AABB(const Vector3<T> &Min, const Vector3<T> &Max) :
			min(Min), max(Max) {

	}

	bool intersects(const AABB<T> &a) const {
		if ((min.x > a.max.x) || (max.x < a.min.x)) {
			return false;
		}

		if ((min.y > a.max.y) || (max.y < a.min.y)) {
			return false;
		}

		if ((min.z > a.max.z) || (max.z < a.min.z)) {
			return false;
		}

		return true;
	}

	bool contains(const Vector3<T> &v) const {
		if ((min.x > v.x) || (max.x < v.x)) {
			return false;
		}

		if ((min.y > v.y) || (max.y < v.y)) {
			return false;
		}

		if ((min.z > v.z) || (max.z < v.z)) {
			return false;
		}

		return true;
	}

	bool contains(const AABB<T> &a) const {
		return (contains(a.min) && contains(a.max));
	}
};

} // namespace

template<typename T>
std::ostream &operator <<(std::ostream &out, const quimby::AABB<T> &a) {
	out << a.min << " - " << a.max;
	return out;
}
