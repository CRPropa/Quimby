#include "quimby/MMapFile.h"

/*
#include <errno.h>
#include <math.h>
#include <string.h>
*/

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdexcept>
#include <iostream>

namespace quimby {

MMapFile::MMapFile() :
	_fd(-1), _size(0), _data((char *)MAP_FAILED) {

}

MMapFile::MMapFile(const std::string& filename, MappingType mtype) :
	_fd(-1), _size(0), _data((char *)MAP_FAILED) {
	open(filename, mtype);
}

MMapFile::~MMapFile() {
	close();
}


void MMapFile::open(const std::string& filename, MappingType mtype ) {
	close();

	_fd = ::open(filename.c_str(), O_RDONLY);

	if (_fd == -1) {
		perror("MMapFile");
		throw std::runtime_error("[MMapFile] error opening file!");
	}

	_size = ::lseek(_fd, 0, SEEK_END);

	int flags = MAP_SHARED;

	if (mtype == ReadAhead)
		flags |= MAP_POPULATE;
	else if (mtype == Auto) {
		size_t mem = sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE);

		if (_size < mem - 200) {
#ifdef DEBUG
			std::cout << "[MMapFile] Automatic mapping: ReadAhead, " << mem / 1000 / 1000 << " MB physical, " << _size / 1000 / 1000 << " MB required" << std::endl;
#endif
			flags |= MAP_POPULATE;
		} else {
#ifdef DEBUG
			std::cout << "[MMapFile] Automatic mapping: OnDemand, " << mem / 1000 / 1000 << " MB physical, " << _size / 1000 / 1000 << " MB required" << std::endl;
#endif
		}
	}

	_data = mmap(NULL, _size, PROT_READ, flags, _fd, 0);

	if (_data == MAP_FAILED) {
		perror("MMapFile");
		close();
		throw std::runtime_error("[HCubeFile] error mapping file: " + filename);
	}
}

void MMapFile::close() {
	if (_data != MAP_FAILED) {
		int result = munmap(_data, _size);
//          if (result < 0) {
//              printf("Error unmapping 0x0%lx of size %ld\n",
//                      (unsigned long) _data, _size);
//          }
		_data = MAP_FAILED;
	}

	_size = 0;

	if (_fd >= 0) {
		::close(_fd);
		_fd = -1;
	}
}


MMapFileWrite::MMapFileWrite(const std::string filename, size_t size, bool resume) :
	_file(-1), _data(MAP_FAILED), _data_size(size) {
	if (resume)
		_file = ::open(filename.c_str(), O_RDWR, 0666);
	else
		_file = ::open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);

	if (_file == -1)
		throw std::runtime_error("[MappedFile] error opening file!");

	truncate(_data_size);

	_data = ::mmap(NULL, _data_size, PROT_READ | PROT_WRITE, MAP_SHARED,
	               _file, 0);

	if (_data == MAP_FAILED) {
        std::cout << "[MappedFileWrite] " << _data_size << std::endl;
	    perror("[MappedFileWrite]");
		throw std::runtime_error("[MappedFileWrite] error mapping file: " + filename);
	}
}

void MMapFileWrite::truncate(size_t size) {
	if (_file == -1)
		return;

	if (ftruncate(_file, size) == -1)
		throw std::runtime_error("[MappedFile Write] error truncating file!");
}

MMapFileWrite::~MMapFileWrite() {
	unmap();
	close();
}

void MMapFileWrite::unmap() {
	if (_data != MAP_FAILED) {
		::msync(_data, _data_size, MS_SYNC);
		::munmap(_data, _data_size);
		_data = MAP_FAILED;
	}

}
void MMapFileWrite::close() {
	if (_file != -1) {
		::close(_file);
		_file = -1;
	}

}

void* MMapFileWrite::data() {
	return _data;
}

} // namespace quimby
