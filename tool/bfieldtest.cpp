#include "gadget/PagedGrid.hpp"
#include "gadget/Vector3.hpp"
#include "arguments.hpp"

class TestVisitor: public PagedGrid<Vector3f>::Visitor {
public:
	index3_t center;
	void visit(PagedGrid<Vector3f> &grid, size_t x, size_t y, size_t z,
			Vector3f &value) {
		value.x = (float)center.x - x;
		value.y = (float)center.y - y;
		value.z = (float)center.z - z;
	}
};

int bfieldtest(Arguments &arguments) {
	BinaryPageIO<Vector3f> io;
	io.setPrefix(arguments.getString("-f", "bfieldtest"));
	io.setOverwrite(true);
	io.setElemetsPerFile(50);

	LastAccessPagingStrategy<Vector3f> strategy;

	PagedGrid<Vector3f> grid;
	grid.setIO(&io);
	grid.setStrategy(&strategy);
	grid.setSize(100);
	grid.setPageCount(1000);
	grid.setPageSize(10);

	TestVisitor v;
	v.center = index3_t(50);
	std::cout << "accept..." << std::endl;
	grid.accept(v, index3_t(0), index3_t(100));
	std::cout << "flush..." << std::endl;
	grid.flush();
	std::cout << "done..." << std::endl;

	return 0;
}
