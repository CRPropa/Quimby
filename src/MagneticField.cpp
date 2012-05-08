#include "gadget/MagneticField.h"
#include "gadget/Index3.h"

#include <stdint.h>

namespace gadget {

template<class T>
T clamp(const T &value, const T &min, const T&max) {
	if (value < min)
		return min;
	else if (value > max)
		return max;
	else
		return value;
}

MagneticField::MagneticField() :
		_sizeKpc(0) {

}

const float &MagneticField::getSize() const {
	return _sizeKpc;
}

const Vector3f &MagneticField::getOrigin() const {
	return _originKpc;
}

void MagneticField::checkPosition(const Vector3f &positionKpc) const {
	if (positionKpc.x <= _originKpc.x)
		throw invalid_position(positionKpc);
	if (positionKpc.y <= _originKpc.y)
		throw invalid_position(positionKpc);
	if (positionKpc.z <= _originKpc.z)
		throw invalid_position(positionKpc);

	if (positionKpc.x >= _originKpc.x + _sizeKpc)
		throw invalid_position(positionKpc);
	if (positionKpc.y >= _originKpc.y + _sizeKpc)
		throw invalid_position(positionKpc);
	if (positionKpc.z >= _originKpc.z + _sizeKpc)
		throw invalid_position(positionKpc);
}

//----------------------------------------------------------------------------
// SampledMagneticField
//----------------------------------------------------------------------------

SampledMagneticField::SampledMagneticField(size_t samples) :
		_stepsizeKpc(0), _samples(samples), _broadeningFactor(0) {
	_grid.create(samples, _stepsizeKpc);
}

size_t SampledMagneticField::toLowerIndex(double x) {
	return (size_t) clamp((int) ::floor(x / _stepsizeKpc), (int) 0,
			(int) _samples - 1);
}

size_t SampledMagneticField::toUpperIndex(double x) {
	return (size_t) clamp((int) ::ceil(x / _stepsizeKpc), (int) 0,
			(int) _samples - 1);
}

class ApplyVisitor: public Database::Visitor {
	SampledMagneticField *field;
public:
	ApplyVisitor(SampledMagneticField *field) :
			field(field) {

	}

	void begin() {

	}

	void visit(const SmoothParticle &p) {
		field->sampleParticle(p);
	}

	void end() {

	}
};

void SampledMagneticField::init(const Vector3f &originKpc, float sizeKpc) {
	_originKpc = originKpc;
	_sizeKpc = sizeKpc;
	_stepsizeKpc = sizeKpc / (_samples - 1);
	_grid.create(_samples, _sizeKpc);
	_grid.reset(Vector3f(0, 0, 0));
}

void SampledMagneticField::init(const Vector3f &originKpc, float sizeKpc,
		Database &db) {
	init(originKpc, sizeKpc);
	ApplyVisitor v(this);
	db.accept(originKpc, originKpc + Vector3f(sizeKpc, sizeKpc, sizeKpc), v);
}

void SampledMagneticField::init(const Vector3f &originKpc, float sizeKpc,
		const std::vector<SmoothParticle> &particles) {
	init(originKpc, sizeKpc);
	size_t s = particles.size();
	for (size_t i = 0; i < s; i++) {
		sampleParticle(particles[i]);
	}
}

void SampledMagneticField::dump(const std::string &dumpfilename) {
	// dumps the field
	_grid.dump(dumpfilename);
}

bool SampledMagneticField::restore(const std::string &dumpfilename) {
	// tries to load the field from a dumpfile, returns true if successful
	return _grid.restore(dumpfilename);
}

void SampledMagneticField::sampleParticle(const SmoothParticle &part) {
	SmoothParticle particle = part;
	particle.smoothingLength += _broadeningFactor * _grid.getCellLength();

	Vector3f value = particle.bfield * particle.weight() * particle.mass
			/ particle.rho;
	float r = particle.smoothingLength + _stepsizeKpc;

	Vector3f relativePosition = particle.position - _originKpc;
	size_t x_min = toLowerIndex(relativePosition.x - r);
	size_t x_max = toUpperIndex(relativePosition.x + r);

	size_t y_min = toLowerIndex(relativePosition.y - r);
	size_t y_max = toUpperIndex(relativePosition.y + r);

	size_t z_min = toLowerIndex(relativePosition.z - r);
	size_t z_max = toUpperIndex(relativePosition.z + r);
#if 1

	Vector3f p;
	for (size_t x = x_min; x <= x_max; x++) {
		p.x = x * _stepsizeKpc;
		for (size_t y = y_min; y <= y_max; y++) {
			p.y = y * _stepsizeKpc;
			for (size_t z = z_min; z <= z_max; z++) {
				p.z = z * _stepsizeKpc;
				float k = particle.kernel(_originKpc + p);
				_grid.get(x, y, z) += value * k;
			}
		}
	}
#else

	size_t nx = x_max - x_min + 1;
	size_t ny = y_max - y_min + 1;
	size_t count = nx * ny;
#pragma omp parallel for if (count > 2000) schedule(dynamic, 1000)
	for (size_t i = 0; i < count; i++) {
		Vector3f p;
		size_t x = x_min + i % nx;
		size_t y = y_min + i / nx;
		p.x = x * _stepsizeKpc;
		p.y = y * _stepsizeKpc;
		for (size_t z = z_min; z <= z_max; z++) {
			p.z = z * _stepsizeKpc;
			float k = particle.kernel(_originKpc + p);
			_grid.get(x, y, z) += value * k;
		}
	}
#endif
}

Vector3f SampledMagneticField::getField(const Vector3f &positionKpc) const {
	checkPosition(positionKpc);

	// check: http://paulbourke.net/miscellaneous/interpolation/
	Vector3f r = (positionKpc - _originKpc) / _stepsizeKpc;

	if (r.x >= (_samples - 1) || r.y >= (_samples - 1) || r.z >= (_samples - 1)
			|| r.x <= 0 || r.y <= 0 || r.z <= 0)
		std::cerr << "[gadget::DirectMagneticField] invalid position: "
				<< positionKpc << std::endl;

	int ix = clamp((int) floor(r.x), 0, int(_samples - 2));
	int iX = ix + 1;
	double fx = r.x - ix;
	double fX = 1 - fx;

	int iy = clamp((int) floor(r.y), 0, int(_samples - 2));
	int iY = iy + 1;
	double fy = r.y - iy;
	double fY = 1 - fy;

	int iz = clamp((int) floor(r.z), 0, int(_samples - 2));
	int iZ = iz + 1;
	double fz = r.z - iz;
	double fZ = 1 - fz;

	Vector3f b(0.f);

// V000 (1 - x) (1 - y) (1 - z) +
	b += _grid.get(ix, iy, iz) * fX * fY * fZ;
//V100 x (1 - y) (1 - z) +
	b += _grid.get(iX, iy, iz) * fx * fY * fZ;
//V010 (1 - x) y (1 - z) +
	b += _grid.get(ix, iY, iz) * fX * fy * fZ;
//V001 (1 - x) (1 - y) z +
	b += _grid.get(iy, iy, iZ) * fX * fY * fz;
//V101 x (1 - y) z +
	b += _grid.get(iX, iy, iZ) * fx * fY * fz;
//V011 (1 - x) y z +
	b += _grid.get(ix, iY, iZ) * fX * fy * fz;
//V110 x y (1 - z) +
	b += _grid.get(iX, iY, iz) * fx * fy * fZ;
//V111 x y z
	b += _grid.get(iX, iY, iZ) * fx * fy * fz;

	return b;
}

void SampledMagneticField::setBroadeningFactor(double broadening) {
	_broadeningFactor = broadening;
}

//----------------------------------------------------------------------------
// DirectMagneticField
//----------------------------------------------------------------------------

DirectMagneticField::DirectMagneticField(size_t grid_size) {
	_grid.create(grid_size, 0);
}

void DirectMagneticField::index(size_t i) {
	SmoothParticle &particle = _particles[i];
	double cl = _grid.getCellLength();

	Vector3f l = particle.position - Vector3f(particle.smoothingLength)
			- _originKpc;
	l.clamp(0.0, _sizeKpc);

	Vector3f u = particle.position + Vector3f(particle.smoothingLength)
			- _originKpc;
	u.clamp(0.0, _sizeKpc);

	Index3 lower;
	lower.x = (uint32_t) std::floor(l.x / cl);
	lower.y = (uint32_t) std::floor(l.y / cl);
	lower.z = (uint32_t) std::floor(l.z / cl);

	Index3 upper;
	upper.x = (uint32_t) std::ceil(u.x / cl);
	upper.y = (uint32_t) std::ceil(u.y / cl);
	upper.z = (uint32_t) std::ceil(u.z / cl);

//	if (particle.rho == 0)
//		std::cerr << "Warning: particle has rho = 0" << std::endl;
//	else
//		particle.bfield *= particle.weight() * particle.mass / particle.rho;

	for (size_t x = lower.x; x < upper.x; x++)
		for (size_t y = lower.y; y < upper.y; y++)
			for (size_t z = lower.z; z < upper.z; z++)
				_grid.get(x, y, z).push_back(i);
}

void DirectMagneticField::init(const Vector3f &originKpc, float sizeKpc,
		Database &db) {
	_particles.clear();
	db.getParticles(originKpc, originKpc + Vector3f(sizeKpc), _particles);
#ifdef DEBUG
	std::cout << "DEBUG [gadget::DirectMagneticField] database with "
	<< db.getCount() << " particles." << std::endl;
#endif
	init(originKpc, sizeKpc);
}

void DirectMagneticField::init(const Vector3f &originKpc, float sizeKpc,
		const std::vector<SmoothParticle> &particles) {
	_particles = particles;
	init(originKpc, sizeKpc);
}

void DirectMagneticField::init(const Vector3f &originKpc, float sizeKpc) {
	_originKpc = originKpc;
	_sizeKpc = sizeKpc;
	_grid.create(_grid.getBins(), _sizeKpc);
	_grid.reset(std::vector<size_t>());

	for (size_t i = 0; i < _particles.size(); i++) {
		index(i);
	}

#ifdef DEBUG
	std::cout << "DEBUG [gadget::DirectMagneticField] indexed "
	<< _particles.size() << " particles." << std::endl;
#endif
}

Vector3f DirectMagneticField::getField(const Vector3f &positionKpc) const {
	checkPosition(positionKpc);

	// get index list
	Vector3f relativePosition = positionKpc - _originKpc;
	const std::vector<size_t> &idx = _grid.get(relativePosition.x,
			relativePosition.y, relativePosition.z);

	// calculate field from overlapping particles
	// see eq. 22 in diploma thesis by Rüdiger Pakmor TU Munich
	Vector3f b(0, 0, 0);
	for (size_t i = 0; i < idx.size(); i++) {
		const SmoothParticle &sp = _particles[idx[i]];
		double k = sp.kernel(positionKpc);
		if (k != 0) {
			b += sp.bfield * (sp.weight() * k) * (sp.mass / sp.rho);
			_statistics.actualSum++;
		}
		_statistics.totalSum++;
	}

	_statistics.totalCount++;
	_statistics.actualCount++;

	return b;
}

float DirectMagneticField::getRho(const Vector3f &positionKpc,
		size_t &overlaps) const {
	checkPosition(positionKpc);

	// get index list
	Vector3f relativePosition = positionKpc - _originKpc;
	const std::vector<size_t> &idx = _grid.get(relativePosition.x,
			relativePosition.y, relativePosition.z);

	// calculate field from overlapping particles
	// see eq. 22 in diploma thesis by Rüdiger Pakmor TU Munich
	float rho = 0;
	overlaps = 0;
	for (size_t i = 0; i < idx.size(); i++) {
		const SmoothParticle &sp = _particles[idx[i]];
		double k = sp.kernel(positionKpc);
		if (k != 0) {
			rho += sp.mass * k * sp.weight();
			overlaps++;
		}
	}

	return rho;
}

} // namespace gadget
