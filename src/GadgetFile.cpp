#include "quimby/GadgetFile.h"

#include <stdexcept>

namespace quimby {

const GadgetFile::Header &GadgetFile::getHeader() const {
	return header;
}

bool GadgetFile::open(const std::string &filename) {
	swap = false;
	file.open(filename.c_str(), std::ios::binary);
	return file.good();
}

void GadgetFile::swapBytes(char *data, int itemCount, int itemSize) {
	if (swap == false)
		return;

	for (int count = 0; count < itemCount; count++) {
		for (int i = 0; i < itemSize / 2; i++) {
			std::swap(data[count * itemSize + i],
					data[count * itemSize + itemSize - 1 - i]);
		}
	}
}

void GadgetFile::readBlockSize() {
	read(&blksize);
}

bool GadgetFile::good() {
	return file.good();
}

size_t GadgetFile::findBlock(const std::string &label) {
	int blocksize = 0;
	std::string blocklabel;

	file.seekg(0, std::ios::beg);
	while (!file.eof() && blocksize == 0) {
		readBlockSize();
		if (!good())
			break;
		if (blksize != 8) {
#ifdef DEBUG_GADGET_FILE
			std::cerr << "[GadgetFile] Enable ENDIAN swapping" << std::endl;
#endif
			swap = !swap;
			swapBytes((char*) &blksize, 1, 4);
		}

		if (blksize != 8) {
			throw std::runtime_error(
					"[GadgetFile] incorrect format blksize=" + blksize);
		}

		blocklabel.resize(4);
		file.read(&blocklabel.at(0), 4 * sizeof(char));
		read(&blocksize);
#ifdef DEBUG_GADGET_FILE
		std::cerr << "[GadgetFile] Found Block <" << blocklabel
		<< "> with " << blocksize << " bytes" << std::endl;
#endif
		readBlockSize();
		if (label != blocklabel) {
			file.seekg(blocksize, std::ios::cur);
			blocksize = 0;
		} else {
			return (blocksize - 8);
		}

	}

	return 0;
}

void GadgetFile::printBlocks() {
	int blocksize = 0;
	std::string blocklabel;

	file.seekg(0, std::ios::beg);
	while (file.good() && blocksize == 0) {
		readBlockSize();
		if (!good())
			break;

		if (blksize != 8) {
#ifdef DEBUG_GADGET_FILE
			std::cerr << "[GadgetFile] Enable ENDIAN swapping" << std::endl;
#endif
			swap = !swap;
			swapBytes((char*) &blksize, 1, 4);
		}

		if (blksize != 8) {
			break;
		}

		blocklabel.resize(4);

		file.read(&blocklabel.at(0), 4 * sizeof(char));
		read(&blocksize);

		std::cout << "[GadgetFile] Found Block <" << blocklabel << "> with "
				<< blocksize << " bytes" << std::endl;

		readBlockSize();
		file.seekg(blocksize, std::ios::cur);
		blocksize = 0;
	}
}

bool GadgetFile::readHeader() {
	size_t blocksize = findBlock("HEAD");
	if (blocksize <= 0) {
		return false;
	}

	readBlockSize();
	blocksize -= read(header.particleNumberList, 6);
	// if the value in massList is 0, then an array with per particle masses is available.
	// Unit: 10^10 sun masses per h
	blocksize -= read(header.massList, 6);
	blocksize -= read(&header.timeOfSnapshot);
	blocksize -= read(&header.redshift);
	int tmpi[10];
	blocksize -= read(tmpi, 10);
	double tmpd[3];
	blocksize -= read(tmpd, 3);
	blocksize -= read(&header.hubble);
	file.seekg(blocksize, std::ios::cur);
	readBlockSize();

	return true;
}

bool GadgetFile::readFloatBlock(const std::string &label,
		std::vector<float> &data) {
	size_t blocksize = findBlock(label);
	if (blocksize == 0) {
		return false;
	}

	size_t count = blocksize / sizeof(float);
	data.resize(count);
	readBlockSize();
	read(&data.front(), count);
	readBlockSize();

	return true;
}

} // namespace
