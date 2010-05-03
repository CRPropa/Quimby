/*
 * Vector3.hpp
 *
 *  Created on: 02.02.2010
 *      Author: gmueller
 */

#ifndef VECTOR3_HPP_
#define VECTOR3_HPP_

#include <iostream>

template <typename T>
class Vector3 {
public:
	T x, y, z;

	Vector3() {
	}

	Vector3(const Vector3<T> &v) :
		x(v.x), y(v.y), z(v.z) {
	}

	Vector3(const double *v) :
		x(v[0]), y(v[1]), z(v[2]) {
	}

	Vector3(const float *v) :
		x(v[0]), y(v[1]), z(v[2]) {
	}

	Vector3(const double &X, const double &Y, const double &Z) :
		x(X), y(Y), z(Z) {
	}

	bool operator <(const Vector3<T> &v) const {
		if (x > v.x)
			return false;
		if (y > v.y)
			return false;
		if (z > v.z)
			return false;
		return true;
	}

	bool operator >(const Vector3<T> &v) const {
		if (x < v.x)
			return false;
		if (y < v.y)
			return false;
		if (z < v.z)
			return false;
		return true;
	}

	Vector3<T> operator -(const Vector3<T> &v) const {
		return Vector3(x - v.x, y - v.y, z - v.z);
	}

	Vector3<T> operator +(const Vector3<T> &v) const {
		return Vector3(x + v.x, y + v.y, z + v.z);
	}

	Vector3<T> operator /(const T &f) const {
		return Vector3(x / f, y / f, z / f);
	}

	Vector3<T> &operator /=(const T &f) {
		x /= f;
		y /= f;
		z /= f;
		return *this;
	}

	Vector3<T> &operator *=(const T &f) {
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}

	Vector3<T> &operator +=(const Vector3<T> &v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}
};

template <typename T>
std::ostream &operator <<(std::ostream &out, const Vector3<T> &v) {
	out << v.x << ", " << v.y << ", " << v.z;
	return out;
}

typedef  Vector3<double> Vector3d;
typedef  Vector3<float> Vector3f;

#endif /* VECTOR3_HPP_ */
