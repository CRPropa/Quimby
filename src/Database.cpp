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
	Vector3f lower, upper;
public:
	size_t count;
	_CollectVisitor(vector<SmoothParticle> &particles, const Vector3f &lower,
			const Vector3f &upper) :
			particles(particles), lower(lower), upper(upper), count(0) {
	}

	void begin(const Database &db) {
		count = 0;
	}

	void visit(const SmoothParticle &particle) {
		count++;
		particles.push_back(particle);
	}

	bool intersects(const Vector3f &lower, const Vector3f &upper,
			float margin) {
		return true;
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
				0), xmax(N - 1), ymin(0), ymax(N - 1), zmin(0), zmax(N - 1), box(
				offset, offset + Vector3f(size)) {
	cell = size / N;
}

SimpleSamplingVisitor::SimpleSamplingVisitor(Grid<Vector3f> &grid,
		const Vector3f &offset, float size) :
		data(grid.elements.data()), N(grid.bins), offset(offset), size(size), progress(false), count(0), xmin(
				0), xmax(N - 1), ymin(0), ymax(N - 1), zmin(0), zmax(N - 1), box(
				offset, offset + Vector3f(size)) {
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
	box.min = offset + Vector3f(cell * this->xmin, cell * this->ymin, cell * this->zmin);
	box.max = offset + Vector3f(cell * this->xmax, cell * this->ymax, cell * this->zmax);
}

bool SimpleSamplingVisitor::intersects(const Vector3f &lower,
		const Vector3f &upper, float margin) {
	return box.intersects(lower - Vector3f(margin), upper + Vector3f(margin));
}

void SimpleSamplingVisitor::showProgress(bool progress) {
	this->progress = progress;
}

void SimpleSamplingVisitor::begin(const Database &db) {
	count = 0;
}

size_t SimpleSamplingVisitor::getCount() {
	return count;
}

void SimpleSamplingVisitor::visit(const SmoothParticle &part) {
	const size_t N2 = N * N;
	count++;

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

	Vector3f o = offset + Vector3f(cell / 2);
	
#pragma omp parallel for
	for (size_t x = x_min; x <= x_max; x++) {
		Vector3f p;
		p.x = x * cell;
		for (size_t y = y_min; y <= y_max; y++) {
			p.y = y * cell;
			for (size_t z = z_min; z <= z_max; z++) {
				p.z = z * cell;
				float k = particle.kernel(o + p);
				data[x * N2 + y * N + z] += value * k;
			}
		}
	}

	if (progress) {
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
		vector<SmoothParticle> &particles) const {
	_CollectVisitor v(particles, lower, upper);
	accept(v);
	return v.count;
}

FileDatabase::FileDatabase() :
		count(0), blocks_per_axis(0) {
}

FileDatabase::FileDatabase(const string &filename, MappingType mtype) :
		count(0), blocks_per_axis(0) {
	if (!open(filename, mtype))
		throw runtime_error("[FileDatabase] could not open database file!");
}

bool FileDatabase::open(const string &filename, MappingType mtype) {
	this->filename = filename;
	file.open(filename);
	size_t offset = 0;
	offset = file.read(count, offset);
	offset = file.read(lower, offset);
	offset = file.read(upper, offset);
	offset = file.read(blocks_per_axis, offset);
	blocks.resize(blocks_per_axis * blocks_per_axis * blocks_per_axis);
	for (size_t i = 0; i < blocks.size(); i++)
		offset = file.read(blocks[i], offset);

	particles = file.data<SmoothParticle>(offset);
	
	return true;
}

void FileDatabase::close() {
	file.close();
	count = 0;
	blocks_per_axis = 0;
}
	
Vector3f FileDatabase::getLowerBounds() const {
	return lower;
}

Vector3f FileDatabase::getUpperBounds() const {
	return upper;
}

float FileDatabase::getMargin() const {
	float margin = 0;
	for (size_t i = 0; i < blocks.size(); i++) {
		margin = std::max(margin, blocks[i].margin);
	}
	return margin;
}

size_t FileDatabase::getCount() const {
	return count;
}

void FileDatabase::accept(DatabaseVisitor &visitor) const {
	if (count == 0)
		return;

	Vector3f blockSize = (upper - lower) / blocks_per_axis;
	Vector3f box_lower, box_upper;

	visitor.begin(*this);

	for (size_t iX = 0; iX < blocks_per_axis; iX++) {
		box_lower.x = lower.x + iX * blockSize.x;
		box_upper.x = box_lower.x + blockSize.x;
		for (size_t iY = 0; iY < blocks_per_axis; iY++) {
			box_lower.y = lower.y + iY * blockSize.y;
			box_upper.y = box_lower.y + blockSize.y;
			for (size_t iZ = 0; iZ < blocks_per_axis; iZ++) {
				box_lower.z = lower.z + iZ * blockSize.z;
				box_upper.z = box_lower.z + blockSize.z;
				const Block &block = blocks[iX * blocks_per_axis
						* blocks_per_axis + iY * blocks_per_axis + iZ];

				if (!visitor.intersects(box_lower, box_upper, block.margin))
					continue;

				for (size_t i = 0; i < block.count; i++) {
					SmoothParticle particle = particles[block.start + i];
					Vector3f l = particle.position
							- Vector3f(particle.smoothingLength);
					Vector3f u = particle.position
							+ Vector3f(particle.smoothingLength);
					AABB<float> v(l, u);
					if (visitor.intersects(particle.position, particle.position,
							particle.smoothingLength))
						visitor.visit(particle);
				}
			}
		}
	}

	visitor.end();
}

class XSorter {
public:
	bool operator()(const SmoothParticle &i, const SmoothParticle &j) {
		return (i.position.x < j.position.x);
	}
};

class YSorter {
public:
	bool operator()(const SmoothParticle &i, const SmoothParticle &j) {
		return (i.position.y < j.position.y);
	}
};

class ZSorter {
public:
	bool operator()(const SmoothParticle &i, const SmoothParticle &j) {
		return (i.position.z < j.position.z);
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

	if (verbose) {
		cout << "  maxSL " << maxSL << endl;
		cout << "  lower " << lower << endl;
		cout << "  upper " << upper << endl;
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

	// sort the particles using position.x
	sort(particles.begin(), particles.end(), XSorter());
	size_t first_in_x = 0, first_out_x = 0;

	for (size_t iX = 0; iX < blocks_per_axis; iX++) {
		box_lower.x = lower.x + iX * blockSize.x;
		box_upper.x = box_lower.x + blockSize.x;

		first_in_x = first_out_x;

		// find all particles in X bin
		for (size_t i = first_in_x; i < count; i++) {
			float px = particles[i].position.x;
			if (px > box_upper.x) {
				first_out_x = i;
				break;
			}
		}

		// sort the particles using position.x
		sort(particles.begin() + first_in_x, particles.begin() + first_out_x,
				YSorter());
		size_t first_in_y = first_in_x, first_out_y = first_in_x;

		for (size_t iY = 0; iY < blocks_per_axis; iY++) {
			if (verbose && (iY > 0)) {
				cout << ".";
				cout.flush();
			}

			box_lower.y = lower.y + iY * blockSize.y;
			box_upper.y = box_lower.y + blockSize.y;

			first_in_y = first_out_y;

			// find all particles in Y bin
			for (size_t i = first_in_y; i < first_out_x; i++) {
				float py = particles[i].position.y;
				if (py > box_upper.y) {
					first_out_y = i;
					break;
				}
			}

			// sort the particles using position.x
			sort(particles.begin() + first_in_y,
					particles.begin() + first_out_y, ZSorter());
			size_t first_in_z = first_in_y, first_out_z = first_in_y;

			for (size_t iZ = 0; iZ < blocks_per_axis; iZ++) {
				box_lower.z = lower.z + iZ * blockSize.z;
				box_upper.z = box_lower.z + blockSize.z;

				first_in_z = first_out_z;

				// find all particles in Y bin
				for (size_t i = first_in_z; i < first_out_y; i++) {
					float pz = particles[i].position.z;
					if (pz > box_upper.z) {
						first_out_z = i;
						break;
					}
				}

				Block &block = blocks[iX * blocks_per_axis * blocks_per_axis
						+ iY * blocks_per_axis + iZ];
				block.margin = 0;
				block.start = particleOffet;
				block.count = first_out_z - first_in_z;
				for (size_t i = first_in_z; i < first_out_z; i++) {
					SmoothParticle &p = particles[i];
					block.margin = max(block.margin, p.smoothingLength);
				}
				out.write((char*) &particles[first_in_z],
						block.count * sizeof(SmoothParticle));
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

Databases::Databases() :
		count(0) {

}

void Databases::add(ref_ptr<Database> db) {
	databases.insert(db);
	update();
}

void Databases::remove(ref_ptr<Database> db) {
	databases.erase(db);
	update();
}

Vector3f Databases::getLowerBounds() const {
	return lower;
}

Vector3f Databases::getUpperBounds() const {
	return upper;
}
size_t Databases::getCount() const {
	return count;
}

void Databases::update() {
	lower = Vector3f(numeric_limits<float>::max());
	upper = Vector3f(numeric_limits<float>::min());
	count = 0;

	for (iter_t i = databases.begin(); i != databases.end(); i++) {
		Vector3f l = (*i)->getLowerBounds();
		lower.setLower(l);
		upper.setUpper(l);

		Vector3f u = (*i)->getUpperBounds();
		lower.setLower(u);
		upper.setUpper(u);

		count += (*i)->getCount();
	}
}

class DatabasesVisitorAdapter: public DatabaseVisitor {
	DatabaseVisitor &visitor;
public:
	DatabasesVisitorAdapter(DatabaseVisitor &visitor) :
			visitor(visitor) {

	}

	virtual void begin(const Database &db) {
	}

	virtual void visit(const SmoothParticle &p) {
		visitor.visit(p);
	}

	bool intersects(const Vector3f &lower, const Vector3f &upper,
			float margin) {
		return visitor.intersects(lower, upper, margin);
	}

	virtual void end() {
	}
};

void Databases::accept(DatabaseVisitor &visitor) const {
	visitor.begin(*this);
	DatabasesVisitorAdapter v(visitor);
	for (iter_t i = databases.begin(); i != databases.end(); i++) {
		Database *db = *i;
		if (v.intersects(db->getLowerBounds(), db->getUpperBounds(),
				db->getMargin()))
			db->accept(v);
	}
	visitor.end();
}

float Databases::getMargin() const {
	float margin = 0;
	for (iter_t i = databases.begin(); i != databases.end(); i++) {
		margin = std::max(margin, (*i)->getMargin());
	}
	return margin;
}
} // namespace
