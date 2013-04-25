#pragma once

#include "Vector3.h"

#include <iostream>

namespace quimby {

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
		if ((center.x - extend > a.center.x + extend)
				|| (center.x + extend < a.center.x - extend)) {
			return false;
		}

		if ((center.y - extend > a.center.y + extend)
				|| (center.y + extend < a.center.y - extend)) {
			return false;
		}

		if ((center.z - extend > a.center.z + extend)
				|| (center.z + extend < a.center.z - extend)) {
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

	Vector3<T> lower() {
		return center - Vector3<T>(extend);
	}

	Vector3<T> upper() {
		return center + Vector3<T>(extend);
	}

	T lowerX() const {
		return center.x - extend;
	}

	T lowerY() const {
		return center.y - extend;
	}

	T lowerZ() const {
		return center.z - extend;
	}

	T upperX() const {
		return center.x + extend;
	}

	T upperY() const {
		return center.y + extend;
	}

	T upperZ() const {
		return center.z + extend;
	}

	static AABC<T> fromOriginSize(const Vector3<T> &origin, T size) {
		return AABC<T>(origin + Vector3<T>(size / 2.0), size / 2.0);
	}
};

} // namespace

template<typename T>
std::ostream &operator <<(std::ostream &out, const quimby::AABC<T> &a) {
	out << a.center << " - " << a.extend;
	return out;
}
