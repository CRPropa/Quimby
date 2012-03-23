#ifndef _GADGET_DATABASE_H_
#define _GADGET_DATABASE_H_

#include "gadget/AABB.h"
#include "gadget/SmoothParticle.h"

#include <algorithm>
#include <limits>

namespace gadget {

class Database {
public:

	class Visitor {
	public:
		virtual ~Visitor() {

		}
		virtual void begin() = 0;
		virtual void visit(const SmoothParticle &p) = 0;
		virtual void end() = 0;
	};

	virtual ~Database() {
	}
	size_t getParticles(const Vector3f &lower, const Vector3f &upper,
			std::vector<SmoothParticle> &particles);

	virtual Vector3f getLowerBounds() =0;

	virtual Vector3f getUpperBounds() = 0;

	virtual size_t getCount() = 0;

	virtual void accept(const Vector3f &lower, const Vector3f &upper,
			Visitor &visitor) = 0;
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
	void open(const std::string &filename);

	Vector3f getLowerBounds();

	Vector3f getUpperBounds();

	size_t getCount();

	static void create(std::vector<SmoothParticle> &particles,
			const std::string &filename, size_t blocks_per_axis = 100);

	void accept(const Vector3f &lower, const Vector3f &upper, Visitor &visitor);
};

} // namespace gadget

#endif /* _GADGET_DATABASE_H_ */
