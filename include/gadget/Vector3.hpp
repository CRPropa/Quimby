/*
 * Vector3.hpp
 *
 *  Created on: 02.02.2010
 *      Author: gmueller
 */

#ifndef VECTOR3_HPP_
#define VECTOR3_HPP_

#include <iostream>
#include <cmath>
#include <vector>

template<typename T>
class Vector3 {
public:
	T x, y, z;

	Vector3() {
	}

	Vector3(const Vector3<T> &v) :
		x(v.x), y(v.y), z(v.z) {
	}

	explicit Vector3(const double *v) :
		x(v[0]), y(v[1]), z(v[2]) {
	}

	explicit Vector3(const float *v) :
		x(v[0]), y(v[1]), z(v[2]) {
	}

	explicit Vector3(const double &X, const double &Y, const double &Z) :
		x(X), y(Y), z(Z) {
	}

	explicit Vector3(T t) :
		x(t), y(t), z(t) {
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

	bool operator ==(const Vector3<T> &v) const {
		if (x != v.x)
			return false;
		if (y != v.y)
			return false;
		if (z != v.z)
			return false;
		return true;
	}

	Vector3<T> operator -(const Vector3<T> &v) const {
		return Vector3(x - v.x, y - v.y, z - v.z);
	}

	Vector3<T> operator +(const Vector3<T> &v) const {
		return Vector3(x + v.x, y + v.y, z + v.z);
	}

	Vector3<T> operator *(const Vector3<T> &v) const {
		return Vector3(x * v.x, y * v.y, z * v.z);
	}

	Vector3<T> operator *(const T &v) const {
		return Vector3(x * v, y * v, z * v);
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

	Vector3<T> &operator -=(const Vector3<T> &v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	Vector3<T> &operator =(const Vector3<T> &v) {
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}

	T length() {
		return std::sqrt(x * x + y * y + z * z);
	}
//
//	bool operator <(const Vector3<T> &p) const {
//		if (x < p.x)
//			return true;
//		else if (x > p.x)
//			return false;
//		if (y < p.y)
//			return true;
//		else if (y > p.y)
//			return false;
//		if (z < p.z)
//			return true;
//		else
//			return false;
//	}

};

#if 0
template<typename T>
Vector3<T> interpolate(const Vector3<T> &a, const Vector3<T> &b) {
	T la = a.length();
	T lb = b.length();
	T d = la + lb;
	Vector3<T> v = (d - la) * a + (d - lb) * b;
	v /= d;
	return v;
}

template<typename T>
Vector3<T> interpolate(const std::vector< T > &distances, const vector< Vector3<T> > &values) {
	if (distances.size() != values.size())
	throw std::runtime_error("interpolate: no. of positions not equal no. of values!");

	T l = 0;
	for (size_t i = 0; i < distances.size(); i++)
	l += distances.size();

	Vector3<T> v(0);
	for (size_t i = 0; i < distances.size(); i++)
	v += (l - distances[i]) * values[i];

	v /= l;

	return v;
}

template<typename T>
Vector3<T> interpolate(const Vector3<T> &a, const Vector3<T> &b, const Vector3<
		T> &c, const Vector3<T> &d, const Vector3<T> &e, const Vector3<T> &f,
		const Vector3<T> &g, const Vector3<T> &h) {
	T la = a.length();
	T lb = b.length();
	T lc = c.length();
	T ld = d.length();
	T le = e.length();
	T lf = f.length();
	T lg = g.length();
	T lh = h.length();

	T l = la + lb + lc + ld + le + lf + lg + lh;

	Vector3<T> v(0);
	v += (l - la) * a;
	v += (l - lb) * b;
	v += (l - lc) * c;
	v += (l - ld) * d;
	v += (l - le) * e;
	v += (l - lf) * f;
	v += (l - lg) * g;
	v += (l - lh) * h;

	v /= l;

	return v;
}
#endif
template<typename T>
std::ostream &operator <<(std::ostream &out, const Vector3<T> &v) {
	out << v.x << " " << v.y << " " << v.z;
	return out;
}

template<typename T>
std::istream &operator >>(std::istream &in, Vector3<T> &v) {
	in >> v.x >> v.y >> v.z;
	return in;
}

typedef Vector3<double> Vector3d;
typedef Vector3<float> Vector3f;

#endif /* VECTOR3_HPP_ */
