#pragma once

#include "Referenced.h"
#include <sys/types.h>
#include <string>

namespace quimby {

typedef enum MappingType_ {
    Auto,
    OnDemand,
    ReadAhead
} MappingType;


class MMapFile: public Referenced {
private:
	int _fd;
	off_t _size;
	void* _data;

public:


	MMapFile();
	MMapFile(const std::string& filename, MappingType mtype = Auto);
	~MMapFile();

	void open(const std::string& filename, MappingType mtype = Auto);
	void close();

	template<class T>
	const T* data(size_t offset = 0) {
		return (const T*)((char *)_data + offset);
	}

	template<class T>
	size_t read(T& v, size_t offset) {
		v = *((const T*)((char *)_data + offset));
		return offset + sizeof(T);
	}

};

class MMapFileWrite {
private:
	int _file;
	void* _data;
	size_t _data_size;
public:

	MMapFileWrite(const std::string filename, size_t size, bool resume = false);

	void truncate(size_t size) ;
	~MMapFileWrite();

	void unmap() ;
	void close();
	void* data();
};

} // namespace quimby
