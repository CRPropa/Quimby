#include "gadget/Database.h"

namespace gadget {

class _CollectVisitor: public Database::Visitor {
	std::vector<SmoothParticle> &particles;

public:
	size_t count;
	_CollectVisitor(std::vector<SmoothParticle> &particles) :
			particles(particles) {
	}

	void begin() {
		count = 0;
	}

	void visit(const SmoothParticle &particle) {
		count++;
		particles.push_back(particle);
	}

	void end() {

	}
};

size_t Database::getParticles(const Vector3f &lower, const Vector3f &upper,
		std::vector<SmoothParticle> &particles) {
	_CollectVisitor v(particles);
	accept(lower, upper, v);
	return v.count;
}

FileDatabase::FileDatabase() :
		count(0) {
}

void FileDatabase::open(const std::string &filename) {
	this->filename = filename;
	std::ifstream in(filename.c_str(), std::ios::binary);
	in.read((char*) &count, sizeof(count));
	in.read((char*) &lower, sizeof(lower));
	in.read((char*) &upper, sizeof(upper));
	in.read((char*) &blocks_per_axis, sizeof(blocks_per_axis));
	blocks.resize(blocks_per_axis * blocks_per_axis * blocks_per_axis);
	for (size_t i = 0; i < blocks.size(); i++)
		in.read((char*) &blocks[i], sizeof(Block));

	data_pos = in.tellg();
	if (in.bad()) {
		this->filename.clear();
		count = 0;
	}
}

Vector3f FileDatabase::getLowerBounds() {
	return lower;
}

Vector3f FileDatabase::getUpperBounds() {
	return upper;
}
size_t FileDatabase::getCount() {
	return count;
}

void FileDatabase::accept(const Vector3f &lower, const Vector3f &upper,
		Database::Visitor &visitor) {
	if (count == 0)
		return;

	std::ifstream in(filename.c_str(), std::ios::binary);
	in.seekg(data_pos, std::ios::beg);
	if (!in)
		return;

	AABB<float> box(lower, upper);
	Vector3f blockSize = (upper - lower) / blocks_per_axis;
	Vector3f box_lower, box_upper;

	for (size_t iX = 0; iX < blocks_per_axis; iX++) {
		box_lower.x = lower.x + iX * blockSize.x;
		box_upper.x = box_lower.x + blockSize.x;
		for (size_t iY = 0; iY < blocks_per_axis; iY++) {
			box_lower.y = lower.y + iY * blockSize.y;
			box_upper.y = box_lower.y + blockSize.y;
			for (size_t iZ = 0; iZ < blocks_per_axis; iZ++) {
				box_lower.z = lower.z + iZ * blockSize.z;
				box_upper.z = box_lower.z + blockSize.z;
				Block &block = blocks[iX * blocks_per_axis * blocks_per_axis
						+ iY * blocks_per_axis + iZ];

				AABB<float> block_box(box_lower - Vector3f(block.margin),
						box_upper + Vector3f(block.margin));

				if (!block_box.intersects(box))
					continue;

				in.seekg(block.start * sizeof(SmoothParticle) + data_pos,
						std::ios::beg);

				for (size_t i = 0; i < block.count; i++) {
					SmoothParticle particle;
					in.read((char*) &particle, sizeof(SmoothParticle));
					Vector3f l = particle.position
							- Vector3f(particle.smoothingLength);
					Vector3f u = particle.position
							+ Vector3f(particle.smoothingLength);
					AABB<float> v(l, u);
					if (!v.intersects(box))
						continue;
					visitor.visit(particle);
				}
			}
		}
	}
}

class XSorter {
public:
	bool operator()(const SmoothParticle &i, const SmoothParticle &j) {
		return (i.position.x < j.position.x);
	}
};

void FileDatabase::create(std::vector<SmoothParticle> &particles,
		const std::string &filename, size_t blocks_per_axis) {
	// set count
	unsigned int count = particles.size();

	// sort the particles using position.x
	std::sort(particles.begin(), particles.end(), XSorter());

	// find lower, upper bounds
	Vector3f lower(std::numeric_limits<float>::max()), upper(
			std::numeric_limits<float>::min());
	float maxSL = 0;
	for (size_t i = 0; i < count; i++) {
		Vector3f l = particles[i].position
				- Vector3f(particles[i].smoothingLength);
		lower.setLower(l);
		upper.setUpper(l);
		Vector3f u = particles[i].position
				+ Vector3f(particles[i].smoothingLength);
		lower.setLower(u);
		upper.setUpper(u);
		maxSL = std::max(maxSL, particles[i].smoothingLength);
	}

	// write meta information
	std::ofstream out(filename.c_str(), std::ios::binary);
	out.write((char*) &count, sizeof(count));
	out.write((char*) &lower, sizeof(lower));
	out.write((char*) &upper, sizeof(upper));
	out.write((char*) &blocks_per_axis, sizeof(blocks_per_axis));

	// write dummy Blocks. Fill with data later.
	std::vector<Block> blocks;
	blocks.resize(blocks_per_axis * blocks_per_axis * blocks_per_axis);
	std::ifstream::pos_type block_pos = out.tellp();
	for (size_t i = 0; i < blocks.size(); i++)
		out.write((char*) &blocks[i], sizeof(Block));

	unsigned int particleOffet = 0;
	Vector3f blockSize = (upper - lower) / blocks_per_axis;
	Vector3f box_lower, box_upper;
	for (size_t iX = 0; iX < blocks_per_axis; iX++) {
		box_lower.x = lower.x + iX * blockSize.x;
		box_upper.x = box_lower.x + blockSize.x;

		// find all particles in X bin, limit number of particles in nested loop
		size_t first_in = 0, first_out = count;
		float bin_lower = box_lower.x - 2 * maxSL;
		for (size_t i = 0; i < count; i++) {
			float px = particles[i].position.x;
			if (px < bin_lower && i > first_in)
				first_in = i;
			if (px > box_upper.x && i < first_out)
				first_out = i;
		}

		for (size_t iY = 0; iY < blocks_per_axis; iY++) {
			box_lower.y = lower.y + iY * blockSize.y;
			box_upper.y = box_lower.y + blockSize.y;

			std::vector<size_t> indices;
			for (size_t i = first_in; i < first_out; i++) {
				Vector3f pl = particles[i].position
						- Vector3f(particles[i].smoothingLength);
				Vector3f pu = particles[i].position
						+ Vector3f(particles[i].smoothingLength);
				if (pl.x > box_upper.x)
					continue;
				if (pu.x < box_lower.x)
					continue;
				if (pl.y > box_upper.y)
					continue;
				if (pu.y < box_lower.y)
					continue;
				indices.push_back(i);
			}

			for (size_t iZ = 0; iZ < blocks_per_axis; iZ++) {
				box_lower.z = lower.z + iZ * blockSize.z;
				box_upper.z = box_lower.z + blockSize.z;
				AABB<float> block_box(box_lower, box_upper);
				Block &block = blocks[iX * blocks_per_axis * blocks_per_axis
						+ iY * blocks_per_axis + iZ];
				block.margin = 0;
				block.start = particleOffet;
				block.count = 0;
				for (size_t i = 0; i < indices.size(); i++) {
					SmoothParticle &p = particles[indices[i]];
					if (!block_box.contains(p.position))
						continue;
					block.count++;
					block.margin = std::max(block.margin, p.smoothingLength);
					out.write((char*) &p, sizeof(SmoothParticle));
				}
				particleOffet += block.count;
			}
		}
	}

	out.seekp(block_pos, std::ios::beg);
	size_t total_count = 0;
	for (size_t i = 0; i < blocks.size(); i++) {
		out.write((char*) &blocks[i], sizeof(Block));
		total_count += blocks[i].count;
	}
}

} // namespace gadget
