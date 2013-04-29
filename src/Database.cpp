#include "quimby/Database.h"

#include <stdexcept>

namespace quimby {

using namespace std;

template<class T>
inline T clamp(const T &value, const T &min, const T&max) {
	if (value < min)
		return min;
	else if (value > max)
		return max;
	else
		return value;
}

class _CollectVisitor: public DatabaseVisitor {
	vector<SmoothParticle> &particles;

public:
	size_t count;
	_CollectVisitor(vector<SmoothParticle> &particles) :
			particles(particles), count(0) {
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

size_t SimpleSamplingVisitor::toLowerIndex(double x) {
	return (size_t) clamp((int) ::floor(x / cell), (int) 0, (int) N - 1);
}

size_t SimpleSamplingVisitor::toUpperIndex(double x) {
	return (size_t) clamp((int) ::ceil(x / cell), (int) 0, (int) N - 1);
}

SimpleSamplingVisitor::SimpleSamplingVisitor(Vector3f *data, size_t N,
		const Vector3f &offset, float size) :
		data(data), N(N), offset(offset), size(size), progress(false), count(0), xmin(
				0), xmax(N - 1), ymin(0), ymax(N - 1), zmin(0), zmax(N - 1) {
	cell = size / N;
}

void SimpleSamplingVisitor::limit(size_t xmin, size_t xmax, size_t ymin,
		size_t ymax, size_t zmin, size_t zmax) {
	this->xmin = clamp(xmin, (size_t) 0, N - 1);
	this->xmax = clamp(xmax, (size_t) 0, N - 1);
	this->ymin = clamp(ymin, (size_t) 0, N - 1);
	this->ymax = clamp(ymax, (size_t) 0, N - 1);
	this->zmin = clamp(zmin, (size_t) 0, N - 1);
	this->zmax = clamp(zmax, (size_t) 0, N - 1);
}

void SimpleSamplingVisitor::showProgress(bool progress) {
	this->progress = progress;
}

void SimpleSamplingVisitor::begin() {
	count = 0;
}

void SimpleSamplingVisitor::visit(const SmoothParticle &part) {
	const size_t N2 = N * N;

	SmoothParticle particle = part;
//			particle.smoothingLength += _broadeningFactor
//					* _grid.getCellLength();

	Vector3f value = particle.bfield * particle.weight() * particle.mass
			/ particle.rho;
	float r = particle.smoothingLength + cell;

	Vector3f relativePosition = particle.position - offset;
	size_t x_min = toLowerIndex(relativePosition.x - r);
	size_t x_max = toUpperIndex(relativePosition.x + r);
	x_min = clamp(x_min, xmin, xmax);
	x_max = clamp(x_max, xmin, xmax);

	size_t y_min = toLowerIndex(relativePosition.y - r);
	size_t y_max = toUpperIndex(relativePosition.y + r);
	y_min = clamp(y_min, ymin, ymax);
	y_max = clamp(y_max, ymin, ymax);

	size_t z_min = toLowerIndex(relativePosition.z - r);
	size_t z_max = toUpperIndex(relativePosition.z + r);
	z_min = clamp(z_min, zmin, zmax);
	z_max = clamp(z_max, zmin, zmax);

#pragma omp parallel for
	for (size_t x = x_min; x <= x_max; x++) {
		Vector3f p;
		p.x = x * cell;
		for (size_t y = y_min; y <= y_max; y++) {
			p.y = y * cell;
			for (size_t z = z_min; z <= z_max; z++) {
				p.z = z * cell;
				float k = particle.kernel(offset + p);
				data[x * N2 + y * N + z] += value * k;
			}
		}
	}

	if (progress) {
		count++;
		if (count % 10000 == 0) {
			cout << ".";
			cout.flush();
		}
		if (count % 1000000 == 0)
			cout << " " << count << endl;
	}

}

void SimpleSamplingVisitor::end() {

}

size_t Database::getParticles(const Vector3f &lower, const Vector3f &upper,
		vector<SmoothParticle> &particles) {
	_CollectVisitor v(particles);
	accept(lower, upper, v);
	return v.count;
}

FileDatabase::FileDatabase() :
		count(0), blocks_per_axis(0) {
}

FileDatabase::FileDatabase(const string &filename) :
		count(0), blocks_per_axis(0) {
	if (!open(filename))
		throw runtime_error("[FileDatabase] could not open database file!");
}

bool FileDatabase::open(const string &filename) {
	this->filename = filename;
	ifstream in(filename.c_str(), ios::binary);
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

	return in.good();
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

void FileDatabase::accept(const Vector3f &l, const Vector3f &u,
		DatabaseVisitor &visitor) {
	if (count == 0)
		return;

	ifstream in(filename.c_str(), ios::binary);
	in.seekg(data_pos, ios::beg);
	if (!in)
		return;

	AABB<float> box(l, u);
	Vector3f blockSize = (upper - lower) / blocks_per_axis;
	Vector3f box_lower, box_upper;

	visitor.begin();

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
						ios::beg);

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

	visitor.end();
}

void FileDatabase::accept(DatabaseVisitor &visitor) {
	if (count == 0)
		return;

	ifstream in(filename.c_str(), ios::binary);
	in.seekg(data_pos, ios::beg);
	if (!in)
		return;

	visitor.begin();

	SmoothParticle particle;
	for (size_t i = 0; i < count; i++) {
		in.read((char*) &particle, sizeof(SmoothParticle));
		visitor.visit(particle);
	}

	visitor.end();
}

class XSorter {
public:
	bool operator()(const SmoothParticle &i, const SmoothParticle &j) {
		return (i.position.x < j.position.x);
	}
};

void FileDatabase::create(vector<SmoothParticle> &particles,
		const string &filename, size_t blocks_per_axis, bool verbose) {

	if (verbose)
		cout << "Create FileDatabase '" << filename << "' ..." << endl;

	// set count
	unsigned int count = particles.size();

	if (verbose)
		cout << "  sort paticles" << endl;

	// sort the particles using position.x
	sort(particles.begin(), particles.end(), XSorter());

	if (verbose)
		cout << "  find bounds" << endl;

	// find lower, upper bounds
	Vector3f lower(numeric_limits<float>::max()), upper(
			numeric_limits<float>::min());
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
		maxSL = max(maxSL, particles[i].smoothingLength);
	}

	// write meta information
	ofstream out(filename.c_str(), ios::binary);
	out.write((char*) &count, sizeof(count));
	out.write((char*) &lower, sizeof(lower));
	out.write((char*) &upper, sizeof(upper));
	out.write((char*) &blocks_per_axis, sizeof(blocks_per_axis));

	// write dummy Blocks. Fill with data later.
	vector<Block> blocks;
	blocks.resize(blocks_per_axis * blocks_per_axis * blocks_per_axis);
	ifstream::pos_type block_pos = out.tellp();
	for (size_t i = 0; i < blocks.size(); i++)
		out.write((char*) &blocks[i], sizeof(Block));

	if (verbose)
		cout << "  write particles" << endl;

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
			if (verbose && (iY > 0)) {
				cout << ".";
				cout.flush();
			}

			box_lower.y = lower.y + iY * blockSize.y;
			box_upper.y = box_lower.y + blockSize.y;

			vector<size_t> indices;
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
					block.margin = max(block.margin, p.smoothingLength);
					out.write((char*) &p, sizeof(SmoothParticle));
				}
				particleOffet += block.count;
			}
		}
		if (verbose)
			cout << " " << iX << endl;
	}

	out.seekp(block_pos, ios::beg);
	size_t total_count = 0;
	for (size_t i = 0; i < blocks.size(); i++) {
		out.write((char*) &blocks[i], sizeof(Block));
		total_count += blocks[i].count;
	}
}

} // namespace
