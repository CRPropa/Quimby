#ifndef _GADGET_DATABASE_H_
#define _GADGET_DATABASE_H_

#include "gadget/AABB.h"
#include "gadget/SmoothParticle.h"

#include <algorithm>
#include <limits>

namespace gadget {

class Database {
public:
	virtual ~Database() {
	}
	virtual unsigned int getParticles(const Vector3f &lower,
			const Vector3f &upper, std::vector<SmoothParticle> &particles) = 0;

	virtual Vector3f getLowerBounds() =0;

	virtual Vector3f getUpperBounds() = 0;

	virtual unsigned int getCount() = 0;
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

	unsigned int getCount();

	unsigned int getParticles(const Vector3f &lower, const Vector3f &upper,
			std::vector<SmoothParticle> &particles);

	static void create(std::vector<SmoothParticle> &particles,
			const std::string &filename, size_t blocks_per_axis = 100);
};

} // namespace gadget

#endif /* _GADGET_DATABASE_H_ */
