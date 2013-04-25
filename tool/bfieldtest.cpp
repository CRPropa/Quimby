#include "arguments.h"

#include "quimby/PagedGrid.h"
#include "quimby/Vector3.h"

using namespace quimby;

class TestVisitor1: public PagedGrid<Vector3f>::Visitor {
public:
	Index3 center;
	void visit(PagedGrid<Vector3f> &grid, size_t x, size_t y, size_t z,
			Vector3f &value) {
		value.x = (float) center.x - x;
		value.y = (float) center.y - y;
		value.z = (float) center.z - z;
	}
};

class TestVisitor2: public PagedGrid<Vector3f>::Visitor {
public:
	Index3 center;
	void visit(PagedGrid<Vector3f> &grid, size_t x, size_t y, size_t z,
			Vector3f &value) {
		value.x -= (float) center.x - x;
		//value.y -= (float) center.y - y;
		value.z -= (float) center.z - z;
	}
};

void step1(Arguments &arguments) {
	BinaryPageIO<Vector3f> io;
	io.setPrefix(arguments.getString("-f", "bfieldtest"));
	io.setOverwrite(true);
	io.setElementsPerFile(50);

	LastAccessPagingStrategy<Vector3f> strategy;

	PagedGrid<Vector3f> grid;
	grid.setIO(&io);
	grid.setStrategy(&strategy);
	grid.setSize(100);
	grid.setPageSize(10);
	grid.setPageCount(1000);

	TestVisitor1 v;
	v.center = Index3(50, 40, 30);
	std::cout << "accept..." << std::endl;
	grid.accept(v, Index3(0), Index3(100));
	std::cout << "flush..." << std::endl;
	grid.flush();
	std::cout << "done..." << std::endl;
}

void step2(Arguments &arguments) {
	BinaryPageIO<Vector3f> io;
	io.setPrefix(arguments.getString("-f", "bfieldtest"));
	//io.setOverwrite(true);
	io.setElementsPerFile(50);

	LastAccessPagingStrategy<Vector3f> strategy;

	PagedGrid<Vector3f> grid;
	grid.setIO(&io);
	grid.setStrategy(&strategy);
	grid.setSize(100);
	grid.setPageSize(10);
	grid.setPageCount(1000);

	TestVisitor2 v;
	v.center = Index3(50, 40, 30);
	std::cout << "accept..." << std::endl;
	grid.accept(v, Index3(0), Index3(100));
	std::cout << "flush..." << std::endl;
	grid.flush();
	std::cout << "done..." << std::endl;
}

int bfieldtest(Arguments &arguments) {
	step1(arguments);
	step2(arguments);
	return 0;
}
