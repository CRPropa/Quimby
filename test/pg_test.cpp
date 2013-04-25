#include <string>

#include "quimby/PagedGrid.h"

using namespace quimby;

void write() {
	BinaryPageIO<int> io;
	io.setPrefix("pg_test");
	io.setForceDump(true);
	io.setDefaultValue(0);
	io.setOverwrite(true);
	io.setElementsPerFile(100);

	LastAccessPagingStrategy<int> strategy;

	PagedGrid<int> grid;
	grid.setSize(1000);
	grid.setPageSize(10);
	grid.setPageCount(50);
	grid.setStrategy(&strategy);
	grid.setIO(&io);

	for (size_t i = 0; i < 1000; i++) {
		grid.getReadWrite(Index3(i, i, i)) = i + 1;
	}

	grid.flush();
}

void read() {
	BinaryPageIO<int> io;
	io.setPrefix("pg_test");
	io.setForceDump(true);
	io.setDefaultValue(0);
	io.setReadOnly(true);
	io.setElementsPerFile(100);

	LastAccessPagingStrategy<int> strategy;

	PagedGrid<int> grid;
	grid.setSize(1000);
	grid.setPageSize(10);
	grid.setPageCount(50);
	grid.setStrategy(&strategy);
	grid.setIO(&io);

	for (int i = 0; i < 1000; i++) {
		const int &v = grid.getReadOnly(Index3(i, i, i));
		if (v != (i + 1))
			exit(1);
	}
}

int main(int argc, char **args) {
	write();
	read();
	return 0;
}
