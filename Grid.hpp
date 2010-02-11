/*
 * Grid.hpp
 *
 *  Created on: 02.02.2010
 *      Author: gmueller
 */

#ifndef GRID_HPP_
#define GRID_HPP_

#include <typeinfo>
#include <vector>
#include <fstream>

template<class T>
class Grid {
public:
	typedef T element_t;
	class Visitor {
	public:
		virtual void visit(const size_t &x, const size_t &y, const size_t &z,
				const element_t &value) = 0;
	};
	std::vector<element_t> elements;
	float size, cellLength;
	unsigned int bins;
public:
	unsigned int getBins() const {
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

	void create(unsigned int bins, float size) {
		this->bins = bins;
		cellLength = size / (float) bins;
		elements.resize(bins * bins * bins);
	}

	element_t &get(unsigned int x, unsigned int y, unsigned int z) {
		return elements[x * bins * bins + y * bins + z];
	}

	const element_t &get(unsigned int x, unsigned int y, unsigned int z) const {
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

	float getCellLength() {
		return cellLength;
	}

	void dump(const std::string &filename) {
		std::ofstream outfile(filename.c_str(), std::ios::binary);
		outfile.write((char *) &elements[0], sizeof(element_t)
				* elements.size());
	}

	void dumpZYX(const std::string &filename) {
		std::ofstream outfile(filename.c_str(), std::ios::binary);
		for (unsigned int iZ = 0; iZ < bins; iZ++) {
			for (unsigned int iY = 0; iY < bins; iY++) {
				for (unsigned int iX = 0; iX < bins; iX++) {
					outfile.write((char *) &elements[iX * bins * bins + iY
							* bins + iZ], sizeof(element_t));
				}
			}
		}
	}

	void acceptXYZ(Visitor &v) {
		for (unsigned int iX = 0; iX < bins; iX++) {
			for (unsigned int iY = 0; iY < bins; iY++) {
				for (unsigned int iZ = 0; iZ < bins; iZ++) {
					v.visit(iX, iY, iZ, elements[iX * bins * bins + iY * bins
							+ iZ]);
				}
			}
		}
	}

	void acceptZYX(Visitor &v) {
		for (unsigned int iZ = 0; iZ < bins; iZ++) {
			for (unsigned int iY = 0; iY < bins; iY++) {
				for (unsigned int iX = 0; iX < bins; iX++) {
					v.visit(iX, iY, iZ, elements[iX * bins * bins + iY * bins
							+ iZ]);
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

	void visit(const size_t &x, const size_t &y, const size_t &z,
			const element_t &value) {
		outfile.write((char *) &value, sizeof(element_t));
	}
};

template<class T>
std::ostream &operator <<(std::ostream &stream, const Grid<T> &grid) {
	stream << "#bins: " << grid.getBins() << std::endl;
	stream << "#size: " << grid.getSize() << std::endl;

	for (unsigned int iX = 0; iX < grid.getBins(); iX++) {
		for (unsigned int iY = 0; iY < grid.getBins(); iY++) {
			for (unsigned int iZ = 0; iZ < grid.getBins(); iZ++) {
				stream << grid.get(iX, iY, iZ) << std::endl;
			}
		}
	}
	return stream;
}

#endif /* GRID_HPP_ */
