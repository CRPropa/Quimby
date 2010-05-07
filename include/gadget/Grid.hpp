/*
 * Grid.hpp
 *
 *  Created on: 02.02.2010
 *      Author: gmueller
 */

#ifndef GRID_HPP_
#define GRID_HPP_

#include "AABC.hpp"

#include <typeinfo>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <cmath>
#include <inttypes.h>

template<class T>
class Grid {

public:

	typedef T element_t;

	class Visitor {
	public:
		virtual void visit(Grid<element_t> &grid, size_t x, size_t y, size_t z,
				element_t &value) = 0;
	};

	std::vector<element_t> elements;
	float size, cellLength;
	size_t bins;

public:
	Grid() :
		size(0.0), cellLength(0.0), bins(0) {
	}

	Grid(size_t bins, float size) {
		create(bins, size);
	}
	size_t getBins() const {
		return bins;
	}

	float getSize() const {
		return size;
	}

	void reset(const element_t &value) {
		typename std::vector<element_t>::iterator i;
		for (i = elements.begin(); i != elements.end(); i++)
			*i = value;
	}

	void create(size_t bins, float size) {
		this->bins = bins;
		this->size = size;
		cellLength = size / (float) bins;
		elements.resize(bins * bins * bins);
	}

	element_t &get(size_t x, size_t y, size_t z) {
		return elements[x * bins * bins + y * bins + z];
	}

	const element_t &get(size_t x, size_t y, size_t z) const {
		return elements[x * bins * bins + y * bins + z];
	}

	element_t &get(int x, int y, int z) {
		return elements[x * bins * bins + y * bins + z];
	}

	const element_t &get(int x, int y, int z) const {
		return elements[x * bins * bins + y * bins + z];
	}

	element_t &get(float fx, float fy, float fz) {
		int x = ((int) (fx / cellLength)) % bins;
		int y = ((int) (fy / cellLength)) % bins;
		int z = ((int) (fz / cellLength)) % bins;
		int i = x * bins * bins + y * bins + z;
		return elements[i];
	}

	const element_t &get(float fx, float fy, float fz) const {
		int x = ((int) (fx / cellLength)) % bins;
		int y = ((int) (fy / cellLength)) % bins;
		int z = ((int) (fz / cellLength)) % bins;
		int i = x * bins * bins + y * bins + z;
		return elements[i];
	}

	size_t toIndex(float x) {
		if (x < 0)
			return 0;

		size_t i = (size_t) (x / cellLength);
		if (i >= bins)
			return bins -1 ;

		return i;
	}

	float toCellCenter(float x) {
		float a = std::floor(x / cellLength) + 0.5;
		return cellLength * a;
	}

	float toCellCenter(size_t x) {
		float a = (float) x + 0.5f;
		return cellLength * a;
	}

	float getCellLength() {
		return cellLength;
	}

	void dump(const std::string &filename) {
		std::ofstream outfile(filename.c_str(), std::ios::binary);
		outfile.write((char *) &elements[0], sizeof(element_t)
				* elements.size());
	}

	void save(const std::string &filename) {
		std::ofstream outfile(filename.c_str(), std::ios::binary);
		std::string type = typeid(element_t).name();
		std::string::size_type type_size = type.size();
		outfile.write((char *) &type_size, sizeof(std::string::size_type));
		outfile.write((char *) &type.at(0), type.size()
				* sizeof(std::string::value_type));
		outfile.write((char *) &size, sizeof(float));
		outfile.write((char *) &bins, sizeof(size_t));
		outfile.write((char *) &elements[0], sizeof(element_t)
				* elements.size());
	}

	bool load(const std::string &filename) {
		std::ifstream infile(filename.c_str(), std::ios::binary);
		if (infile.bad())
			return false;

		std::string type;
		std::string::size_type type_size;
		infile.read((char *) &type_size, sizeof(std::string::size_type));
		type.resize(type_size);
		infile.read((char *) &type.at(0), type_size
				* sizeof(std::string::value_type));

		if (type != typeid(element_t).name()) {
			throw std::runtime_error("Grid type mismatch. file: " + type
					+ " this: " + typeid(element_t).name());
		}

		float s;
		infile.read((char *) &s, sizeof(float));
		size_t b;
		infile.read((char *) &b, sizeof(size_t));
		create(b, s);

		infile.read((char *) &elements[0], sizeof(element_t) * elements.size());

		return true;
	}

	void dumpZYX(const std::string &filename) {
		std::ofstream outfile(filename.c_str(), std::ios::binary);
		for (size_t iZ = 0; iZ < bins; iZ++) {
			for (size_t iY = 0; iY < bins; iY++) {
				for (size_t iX = 0; iX < bins; iX++) {
					outfile.write((char *) &elements[iX * bins * bins + iY
							* bins + iZ], sizeof(element_t));
				}
			}
		}
	}

	void acceptXYZ(Visitor &v) {
		for (size_t iX = 0; iX < bins; iX++) {
			for (size_t iY = 0; iY < bins; iY++) {
				for (size_t iZ = 0; iZ < bins; iZ++) {
					v.visit(*this, iX, iY, iZ, elements[iX * bins * bins + iY
							* bins + iZ]);
				}
			}
		}
	}

	void acceptZYX(Visitor &v) {
		for (size_t iZ = 0; iZ < bins; iZ++) {
			for (size_t iY = 0; iY < bins; iY++) {
				for (size_t iX = 0; iX < bins; iX++) {
					v.visit(*this, iX, iY, iZ, elements[iX * bins * bins + iY
							* bins + iZ]);
				}
			}
		}
	}

	void acceptZYX(Visitor &v, const AABC<float> &aabc) {
		size_t zStart = toIndex(aabc.lowerZ());
		size_t zEnd = toIndex(aabc.upperZ());
		for (size_t iZ = zStart; iZ <= zEnd; iZ++) {
			size_t yStart = toIndex(aabc.lowerY());
			size_t yEnd = toIndex(aabc.upperY());
			for (size_t iY = yStart; iY <= yEnd; iY++) {
				size_t xStart = toIndex(aabc.lowerX());
				size_t xEnd = toIndex(aabc.upperX());
				for (size_t iX = xStart; iX <= xEnd; iX++) {
					v.visit(*this, iX, iY, iZ, elements[iX * bins * bins + iY
							* bins + iZ]);
				}
			}
		}
	}
};

template<class T>
class DumpGridVisitor: public Grid<T>::Visitor {
public:
	typedef typename Grid<T>::element_t element_t;
	std::ofstream outfile;
public:
	DumpGridVisitor(const std::string &filename) {
		outfile.open(filename.c_str(), std::ios::binary);
	}

	void visit(Grid<T> &grid, size_t x, size_t y, size_t z,
			const element_t &value) {
		outfile.write((char *) &value, sizeof(element_t));
	}
};

template<class T>
std::ostream &operator <<(std::ostream &stream, const Grid<T> &grid) {
	stream << "#bins: " << grid.getBins() << std::endl;
	stream << "#size: " << grid.getSize() << std::endl;

	for (size_t iX = 0; iX < grid.getBins(); iX++) {
		for (size_t iY = 0; iY < grid.getBins(); iY++) {
			for (size_t iZ = 0; iZ < grid.getBins(); iZ++) {
				stream << grid.get(iX, iY, iZ) << std::endl;
			}
		}
	}
	return stream;
}

#endif /* GRID_HPP_ */
