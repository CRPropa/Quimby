#ifndef _GADGET_GADGETFILE_H_
#define _GADGET_GADGETFILE_H_

#include <fstream>
#include <iostream>
#include <vector>

namespace gadget {

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
	void printBlocks();
};

} // namespace gadget

#endif /* _GADGET_GADGETFILE_H_ */
