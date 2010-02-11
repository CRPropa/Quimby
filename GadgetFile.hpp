/*
 * GadgetFile.hpp
 *
 *  Created on: 02.02.2010
 *      Author: gmueller
 */

#ifndef GADGETFILE_HPP_
#define GADGETFILE_HPP_

#include <fstream>
#include <iostream>
#include <vector>

class GadgetFile {

public:
	struct Header {
		int particleNumberList[6];
		double massList[6];
		double timeOfSnapshot;
		double redshift;
	};

private:
	std::ifstream file;
	bool swapEndian;
	int blksize;
	bool swap;
	Header header;

public:

	const Header &getHeader() const;

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
};

#endif /* GADGETFILE_HPP_ */
