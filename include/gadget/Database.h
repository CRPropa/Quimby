#ifndef _GADGET_DATABASE_H_
#define _GADGET_DATABASE_H_

#include "gadget/AABB.h"
#include "gadget/SmoothParticle.h"
#include "gadget/Referenced.h"

#include <algorithm>
#include <limits>

namespace gadget {

class DatabaseVisitor {
public:
	virtual ~DatabaseVisitor() {

	}
	virtual void begin() = 0;
	virtual void visit(const SmoothParticle &p) = 0;
	virtual void end() = 0;
};

class Database: public Referenced {
public:

	virtual ~Database() {
	}
	size_t getParticles(const Vector3f &lower, const Vector3f &upper,
			std::vector<SmoothParticle> &particles);

	virtual Vector3f getLowerBounds() = 0;

	virtual Vector3f getUpperBounds() = 0;

	virtual size_t getCount() = 0;

	virtual void accept(const Vector3f &lower, const Vector3f &upper,
			DatabaseVisitor &visitor) = 0;
	virtual void accept(DatabaseVisitor &visitor) = 0;
};

class FileDatabase: public Database {

	struct Block {
		float margin;
		unsigned int start, count;
	};

	unsigned int count;
	Vector3f lower, upper;
	size_t blocks_per_axis;
	std::vector<Block> blocks;
	std::string filename;
	std::ifstream::pos_type data_pos;

public:

	FileDatabase();
	FileDatabase(const std::string &filename);
	bool open(const std::string &filename);

	Vector3f getLowerBounds();

	Vector3f getUpperBounds();

	size_t getCount();

	static void create(std::vector<SmoothParticle> &particles,
			const std::string &filename, size_t blocks_per_axis = 100);

	void accept(const Vector3f &lower, const Vector3f &upper,
			DatabaseVisitor &visitor);
	void accept(DatabaseVisitor &visitor);
};

class SimpleSamplingVisitor: public DatabaseVisitor {
	Vector3f *data;
	size_t N, count;
	Vector3f offset;
	float size, cell;
	bool progress;
	size_t xmin, xmax, ymin, ymax, zmin, zmax;

	size_t toLowerIndex(double x);
	size_t toUpperIndex(double x);

public:

	SimpleSamplingVisitor(Vector3f *data, size_t N, const Vector3f &offset,
			float size);
	void begin();
	void visit(const SmoothParticle &part);
	void end();

	void limit(size_t xmin, size_t xmax, size_t ymin, size_t ymax, size_t zmin,
			size_t zmax);
	void showProgress(bool progress);
};

} // namespace gadget

#endif /* _GADGET_DATABASE_H_ */
