#pragma once

#include <inttypes.h>
#include <algorithm>
#include <iostream>

namespace quimby {

inline uint32_t spread3(uint32_t x) {
	x = (0xF0000000 & x) | ((0x0F000000 & x) >> 8) | (x >> 16);
	x = (0xC00C00C0 & x) | ((0x30030030 & x) >> 4);
	x = (0x82082082 & x) | ((0x41041041 & x) >> 2);
	return x;
}

static inline uint32_t morton(const uint32_t &x, const uint32_t &y,
		const uint32_t &z) {
	return spread3(x) | (spread3(y) >> 1) | (spread3(z) >> 2);
}

struct Index3 {
	typedef uint32_t index_t;
	index_t x, y, z;

	Index3() :
			x(0), y(0), z(0) {

	}

	Index3(const Index3 &v) :
			x(v.x), y(v.y), z(v.z) {
	}

	explicit Index3(index_t i) :
			x(i), y(i), z(i) {
	}

	explicit Index3(const index_t &X, const index_t &Y, const index_t &Z) :
			x(X), y(Y), z(Z) {
	}

	bool operator <(const Index3 &v) const {
		if (x > v.x)
			return false;
		else if (x < v.x)
			return true;
		if (y > v.y)
			return false;
		else if (y < v.y)
			return true;
		if (z >= v.z)
			return false;
		else
			return true;
	}

	bool operator ==(const Index3 &v) const {
		if (x != v.x)
			return false;
		if (y != v.y)
			return false;
		if (z != v.z)
			return false;
		return true;
	}

	Index3 operator -(const Index3 &v) const {
		return Index3(x - v.x, y - v.y, z - v.z);
	}

	Index3 operator +(const Index3 &v) const {
		return Index3(x + v.x, y + v.y, z + v.z);
	}

	Index3 operator *(const index_t &v) const {
		return Index3(x * v, y * v, z * v);
	}

	Index3 operator /(const index_t &f) const {
		return Index3(x / f, y / f, z / f);
	}

	Index3 &operator /=(const index_t &f) {
		x /= f;
		y /= f;
		z /= f;
		return *this;
	}

	Index3 operator %(const index_t &f) const {
		return Index3(x % f, y % f, z % f);
	}

	Index3 &operator %=(const index_t &f) {
		x %= f;
		y %= f;
		z %= f;
		return *this;
	}

	Index3 &operator *=(const index_t &f) {
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}

	Index3 &operator +=(const Index3 &v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	Index3 &operator -=(const Index3 &v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	Index3 &operator =(const Index3 &v) {
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}

	Index3 minByElement(const Index3 &a) const {
		return Index3(std::min(x, a.x), std::min(y, a.y), std::min(z, a.z));
	}

	Index3 maxByElement(const Index3 &a) const {
		return Index3(std::max(x, a.x), std::max(y, a.y), std::max(z, a.z));
	}

	uint32_t morton() const {
		return quimby::morton(x, y, z);
	}
};

} // namespace

inline std::ostream &operator <<(std::ostream &out, const quimby::Index3 &v) {
	out << v.x << " " << v.y << " " << v.z;
	return out;
}
