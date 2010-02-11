/*
 * AABB.hpp
 *
 *  Created on: 02.02.2010
 *      Author: gmueller
 */

#ifndef AABB_HPP_
#define AABB_HPP_

#include "Vector3.hpp"

#include <iostream>

template <typename T>
class AABB {
public:
	Vector3<T> min;
	Vector3<T> max;

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

};

template <typename T>
std::ostream &operator <<(std::ostream &out, const AABB<T> &a) {
	out << a.min << " - " << a.max;
	return out;
}

#endif /* AABB_HPP_ */
