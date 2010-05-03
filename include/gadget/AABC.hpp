/*
 * AABC.hpp
 *
 *  Created on: 11.02.2010
 *      Author: gmueller
 */

#ifndef AABC_HPP_
#define AABC_HPP_

#include "Vector3.hpp"

#include <iostream>

template<typename T>
class AABC {
public:
	Vector3<T> center;
	T extend;

	AABC() {
	}

	AABC(const AABC<T> &a) :
		center(a.center), extend(a.extend) {
	}

	AABC(const Vector3<T> &c, T e) :
		center(c), extend(e) {
	}

	bool intersects(const AABC<T> &a) const {
		if ((center.x - extend > a.center.x + extend) || (center.x + extend
				< a.center.x - extend)) {
			return false;
		}

		if ((center.y - extend > a.center.y + extend) || (center.y + extend
				< a.center.y - extend)) {
			return false;
		}

		if ((center.z - extend > a.center.z + extend) || (center.z + extend
				< a.center.z - extend)) {
			return false;
		}

		return true;
	}

	bool contains(const Vector3<T> &v) const {
		if ((center.x - extend > v.x) || (center.x + extend < v.x)) {
			return false;
		}

		if ((center.y - extend > v.y) || (center.y + extend < v.y)) {
			return false;
		}

		if ((center.z - extend > v.z) || (center.z + extend < v.z)) {
			return false;
		}

		return true;
	}

	T lowerX() const {
		center.x - extend;
	}

	T lowerY() const {
		center.y - extend;
	}

	T lowerZ() const {
		center.z - extend;
	}

	T upperX() const {
		center.x + extend;
	}

	T upperY() const {
		center.y + extend;
	}

	T upperZ() const {
		center.z + extend;
	}
};

template<typename T>
std::ostream &operator <<(std::ostream &out, const AABC<T> &a) {
	out << a.center << " - " << a.extend;
	return out;
}

#endif /* AABC_HPP_ */
