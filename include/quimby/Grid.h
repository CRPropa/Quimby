#pragma once

#include "AABC.h"

#include <typeinfo>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <inttypes.h>

namespace quimby {

template<class T>
class Grid {

public:

	typedef T element_t;

	class Visitor {
	public:
		virtual ~Visitor() {
		}
		virtual void visit(Grid<element_t> &grid, size_t x, size_t y, size_t z,
				element_t &value) = 0;
	};

	std::vector<element_t> elements;
	double size, cellLength;
	size_t bins, bins2;

public:
	Grid() :
			size(0.0), cellLength(0.0), bins(0), bins2(0) {
	}

	Grid(size_t bins, double size) {
		create(bins, size);
	}
	size_t getBins() const {
		return bins;
	}

	double getSize() const {
		return size;
	}

	void reset(const element_t &value) {
		typename std::vector<element_t>::iterator i;
		for (i = elements.begin(); i != elements.end(); i++)
			*i = value;
	}

	void create(size_t bins, double size) {
		this->bins = bins;
		this->size = size;
		this->bins2 = bins * bins;
		cellLength = size / (double) bins;
		size_t count = bins * bins * bins;
		if (elements.size() != count) {
			try {
				elements.resize(count);
			} catch (...) {
				std::stringstream sstr;
				sstr << "Failed to allocate memory for Grid. Bins: " << bins;
				sstr << ", Elements size: " << sizeof(element_t);
				throw std::runtime_error(sstr.str());
			}
		}
	}

	element_t &get(size_t x, size_t y, size_t z) {
		return elements[x * bins2 + y * bins + z];
	}

	const element_t &get(size_t x, size_t y, size_t z) const {
		return elements[x * bins2 + y * bins + z];
	}

	element_t &get(int x, int y, int z) {
		return elements[x * bins2 + y * bins + z];
	}

	const element_t &get(int x, int y, int z) const {
		return elements[x * bins2 + y * bins + z];
	}

	element_t &get(double fx, double fy, double fz) {
		int x = ((int) (fx / cellLength)) % bins;
		int y = ((int) (fy / cellLength)) % bins;
		int z = ((int) (fz / cellLength)) % bins;
		int i = x * bins * bins + y * bins + z;
		return elements[i];
	}

	const element_t &get(double fx, double fy, double fz) const {
		int x = ((int) (fx / cellLength)) % bins;
		int y = ((int) (fy / cellLength)) % bins;
		int z = ((int) (fz / cellLength)) % bins;
		int i = x * bins * bins + y * bins + z;
		return elements[i];
	}

	size_t toIndex(double x) {
		if (x < 0)
			return 0;

		size_t i = (size_t) (x / cellLength);
		if (i >= bins)
			return bins - 1;

		return i;
	}

	double toCellCenter(double x) {
		double a = std::floor(x / cellLength) + 0.5;
		return cellLength * a;
	}

	double toCellCenter(size_t x) {
		double a = (double) x + 0.5f;
		return cellLength * a;
	}

	double getCellLength() {
		return cellLength;
	}

	bool dump(const std::string &filename) {
		std::ofstream outfile(filename.c_str(), std::ios::binary);
		if (!outfile)
			return false;
		outfile.write((char *) &elements[0],
				sizeof(element_t) * elements.size());
		if (!outfile)
			return false;
		return true;
	}

	bool restore(const std::string &filename) {
		std::ifstream infile(filename.c_str(), std::ios::binary);
		if (!infile)
			return false;
		infile.read((char *) &elements[0], sizeof(element_t) * elements.size());
		if (!infile)
			return false;
		return true;
	}

	void save(const std::string &filename) {
		std::ofstream outfile(filename.c_str(), std::ios::binary);
		std::string type = typeid(element_t).name();
		std::string::size_type type_size = type.size();
		outfile.write((char *) &type_size, sizeof(std::string::size_type));
		outfile.write((char *) &type.at(0),
				type.size() * sizeof(std::string::value_type));
		outfile.write((char *) &size, sizeof(double));
		outfile.write((char *) &bins, sizeof(size_t));
		outfile.write((char *) &elements[0],
				sizeof(element_t) * elements.size());
	}

	bool load(const std::string &filename) {
		std::ifstream infile(filename.c_str(), std::ios::binary);
		if (infile.bad())
			return false;

		std::string type;
		std::string::size_type type_size;
		infile.read((char *) &type_size, sizeof(std::string::size_type));
		type.resize(type_size);
		infile.read((char *) &type.at(0),
				type_size * sizeof(std::string::value_type));

		if (type != typeid(element_t).name()) {
			throw std::runtime_error(
					"Grid type mismatch. file: " + type + " this: "
							+ typeid(element_t).name());
		}

		double s;
		infile.read((char *) &s, sizeof(double));
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
					outfile.write(
							(char *) &elements[iX * bins * bins + iY * bins + iZ],
							sizeof(element_t));
				}
			}
		}
	}

	void acceptXYZ(Visitor &v) {
		for (size_t iX = 0; iX < bins; iX++) {
			for (size_t iY = 0; iY < bins; iY++) {
				for (size_t iZ = 0; iZ < bins; iZ++) {
					v.visit(*this, iX, iY, iZ,
							elements[iX * bins * bins + iY * bins + iZ]);
				}
			}
		}
	}

	void acceptZYX(Visitor &v) {
		for (size_t iZ = 0; iZ < bins; iZ++) {
			for (size_t iY = 0; iY < bins; iY++) {
				for (size_t iX = 0; iX < bins; iX++) {
					v.visit(*this, iX, iY, iZ,
							elements[iX * bins * bins + iY * bins + iZ]);
				}
			}
		}
	}

	template<class F>
	void acceptZYX(Visitor &v, const AABC<F> &aabc) {
		size_t zStart = toIndex(aabc.lowerZ());
		size_t zEnd = toIndex(aabc.upperZ());
		for (size_t iZ = zStart; iZ <= zEnd; iZ++) {
			size_t yStart = toIndex(aabc.lowerY());
			size_t yEnd = toIndex(aabc.upperY());
			for (size_t iY = yStart; iY <= yEnd; iY++) {
				size_t xStart = toIndex(aabc.lowerX());
				size_t xEnd = toIndex(aabc.upperX());
				for (size_t iX = xStart; iX <= xEnd; iX++) {
					v.visit(*this, iX, iY, iZ,
							elements[iX * bins * bins + iY * bins + iZ]);
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

} // namespace

template<class T>
std::ostream &operator <<(std::ostream &stream, const quimby::Grid<T> &grid) {
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
