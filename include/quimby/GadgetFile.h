#pragma once

#include <fstream>
#include <iostream>
#include <vector>

#include "quimby/Referenced.h"

namespace quimby {

struct GadgetFileHeader {
	std::vector<unsigned int> particleNumberList;
	std::vector<double> massList;
	double timeOfSnapshot;
	double redshift;
	double hubble;
};

class GadgetFile : public Referenced {

private:
	std::ifstream file;
	bool swapEndian;
	int blksize;
	bool swap;
	GadgetFileHeader header;

public:

	const GadgetFileHeader &getHeader() const;

	bool open(const std::string &filename);

	void swapBytes(char *data, int itemCount, int itemSize);

	template<typename T>
	size_t read(T *i, int count = 1) {
		size_t size = sizeof(T) * count;
		file.read((char *) i, size);
		swapBytes((char *) i, count, sizeof(T));
		return size;
	}

	void readBlockSize();
	bool good();
	size_t findBlock(const std::string &label);
	bool readHeader();
	bool readFloatBlock(const std::string &label, std::vector<float> &data);
	void printBlocks();
};

} // namespace
