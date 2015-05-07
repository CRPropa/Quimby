#pragma once

#include "AABB.h"
#include "SmoothParticle.h"
#include "Referenced.h"
#include "Grid.h"

#include <algorithm>
#include <limits>
#include <set>

namespace quimby {

class Database;

class DatabaseVisitor {
public:
	virtual ~DatabaseVisitor() {

	}
	virtual void begin(const Database &db) = 0;
	virtual bool intersects(const Vector3f &lower, const Vector3f &upper, float margin) = 0;
	virtual void visit(const SmoothParticle &p) = 0;
	virtual void end() = 0;
};

class Database: public Referenced {
public:

	virtual ~Database() {
	}
	size_t getParticles(const Vector3f &lower, const Vector3f &upper,
			std::vector<SmoothParticle> &particles) const;

	virtual Vector3f getLowerBounds() const = 0;
	virtual Vector3f getUpperBounds() const = 0;
	virtual float getMargin() const = 0;

	virtual size_t getCount() const = 0;

	virtual void accept(DatabaseVisitor &visitor) const = 0;
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

	Vector3f getLowerBounds() const;
	Vector3f getUpperBounds() const;
	float getMargin() const;

	size_t getCount() const;

	static void create(std::vector<SmoothParticle> &particles,
			const std::string &filename, size_t blocks_per_axis = 100,
			bool verbose = false);

	void accept(DatabaseVisitor &visitor) const;
};

class Databases: public Database {

	unsigned int count;
	Vector3f lower, upper;
	typedef std::set<ref_ptr<Database> > set_t;
	typedef std::set<ref_ptr<Database> >::iterator iter_t;
	set_t databases;

	void update();
public:

	Databases();
	void add(ref_ptr<Database> db);
	void remove(ref_ptr<Database> db);

	Vector3f getLowerBounds() const;
	Vector3f getUpperBounds() const;
	size_t getCount() const;
	float getMargin() const;

	void accept(DatabaseVisitor &visitor) const;
};

class SimpleSamplingVisitor: public DatabaseVisitor {
	Vector3f *data;
	size_t N, count;
	Vector3f offset;
	float size, cell;
	bool progress;
	size_t xmin, xmax, ymin, ymax, zmin, zmax;
	AABB<float> box;
	size_t toLowerIndex(double x);
	size_t toUpperIndex(double x);

public:

	SimpleSamplingVisitor(Vector3f *data, size_t N, const Vector3f &offset,
			float size);
	SimpleSamplingVisitor(Grid<Vector3f> &grid, const Vector3f &offset,
			float size);
	void begin(const Database &db);
	bool intersects(const Vector3f &lower, const Vector3f &upper, float margin);
	void visit(const SmoothParticle &part);
	void end();

	void limit(size_t xmin, size_t xmax, size_t ymin, size_t ymax, size_t zmin,
			size_t zmax);
	void showProgress(bool progress);
};

} // namespace
