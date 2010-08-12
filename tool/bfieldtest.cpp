#include "gadget/PagedGrid.hpp"
#include "gadget/Vector3.hpp"
#include "arguments.hpp"

class TestVisitor: public PagedGrid<Vector3f>::Visitor {
public:
	index3_t center;
	void visit(PagedGrid<Vector3f> &grid, size_t x, size_t y, size_t z,
			Vector3f &value) {
		value.x = center.x - x;
		value.y = center.y - y;
		value.z = center.z - z;
	}
};

int bfieldtest(Arguments &arguments) {

	BinaryPageIO<Vector3f> io;
	io.prefix = arguments.getString("-f", "bfieldtest");
	io.overwrite = true;
	io.fileSize = 100;

	LastAccessPagingStrategy<Vector3f> strategy;

	PagedGrid<Vector3f> grid;
	grid.setIO(&io);
	grid.setSize(100);
	grid.setPageCount(1000);
	grid.setPageSize(10);
	grid.setStrategy(&strategy);

	TestVisitor v;
	v.center = index3_t(50);
	grid.accept(v, index3_t(0), index3_t(100));
	grid.flush();
	return 0;
}
