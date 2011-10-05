#include "gadget/MagneticField.hpp"
#include "gadget/Index3.hpp"

#include <stdint.h>

template<class T>
T clamp(const T &value, const T &min, const T&max) {
	if (value < min)
		return min;
	else if (value > max)
		return max;
	else
		return value;
}

MagneticField::MagneticField(const Vector3f &originKpc, double sizeKpc) :
		_originKpc(originKpc), _sizeKpc(sizeKpc) {

}

const float &MagneticField::getSize() const {
	return _sizeKpc;
}

const Vector3f &MagneticField::getOrigin() const {
	return _originKpc;
}

void MagneticField::checkPosition(const Vector3f &positionKpc) const {
	if (positionKpc.x < _originKpc.x)
		throw invalid_position(positionKpc);
	if (positionKpc.y < _originKpc.y)
		throw invalid_position(positionKpc);
	if (positionKpc.z < _originKpc.z)
		throw invalid_position(positionKpc);

	if (positionKpc.x > _originKpc.x + _sizeKpc)
		throw invalid_position(positionKpc);
	if (positionKpc.y > _originKpc.y + _sizeKpc)
		throw invalid_position(positionKpc);
	if (positionKpc.z > _originKpc.z + _sizeKpc)
		throw invalid_position(positionKpc);
}

//MagneticField::MagneticField(const char* aFileName) :
//		TXmlParam(aFileName) {
//	SetType(MAGFIELD_LSS);
//
//	_originKpc = TVector3D(0.0);
//	TiXmlElement* lpXmlField = XmlExtract().GetElement("MagneticField");
//	TiXmlElement* lpXmlFile = lpXmlField->FirstChildElement("File");
//	_filename = lpXmlFile->Attribute("name");
//	std::cout << "[TSphMagField] file: " << _filename << std::endl;
//
//	TiXmlElement* lpXmlSize = lpXmlField->FirstChildElement("Size_Mpc");
//	_fSize = 240;
//	if (lpXmlSize) {
//		lpXmlSize->Attribute("value", &_fSize);
//	}
//	std::cout << "[TSphMagField] size (Mpc): " << _fSize << std::endl;
//	_sizeKpc = _fSize * 1000;
//	_fSize *= Mpc;
//
//	TiXmlElement* lpXmlStepsize = lpXmlField->FirstChildElement("Stepsize_Mpc");
//	_fStepsize = 0.1;
//	if (lpXmlStepsize) {
//		lpXmlStepsize->Attribute("value", &_fStepsize);
//	}
//	_stepsizeKpc = _fStepsize * 1000;
//
//	std::cout << "[TSphMagField] stepsize (Mpc): " << _fStepsize << std::endl;
//	_fStepsize *= Mpc;
//
//	TiXmlElement* lpOrigin = lpXmlField->FirstChildElement("Origin");
//	if (lpOrigin) {
//		TiXmlElement* lpX = lpOrigin->FirstChildElement("X_Mpc");
//		TiXmlElement* lpY = lpOrigin->FirstChildElement("Y_Mpc");
//		TiXmlElement* lpZ = lpOrigin->FirstChildElement("Z_Mpc");
//		if (!lpX || !lpY || !lpZ)
//			throw TXmlErr("MagField origin missing.");
//		lpX->Attribute("value", &_originKpc[0]);
//		_originKpc[0] *= Mpc;
//		lpY->Attribute("value", &_originKpc[1]);
//		_originKpc[1] *= Mpc;
//		lpZ->Attribute("value", &_originKpc[2]);
//		_originKpc[2] *= Mpc;
//	}
//
//	std::cout << "[TSphMagField] Size kpc: " << _sizeKpc << std::endl;
//	std::cout << "[TSphMagField] Stepsize kpc: " << _stepsizeKpc << std::endl;
//
//	_fAvgSphTotalSum = 0;
//	_fAvgSphTotalCount = 0;
//	_fAvgSphActualSum = 0;
//	_fAvgSphActualCount = 0;
//
//	if (_fStepsize)
//		createGrid();
//	else
//		createSmoothParticleGrid();
//}

//----------------------------------------------------------------------------
// SampledMagneticField
//----------------------------------------------------------------------------

SampledMagneticField::SampledMagneticField(const Vector3f &originKpc,
		float sizeKpc) :
		MagneticField(originKpc, sizeKpc) {

}

size_t SampledMagneticField::toLowerIndex(double x) {
	return (size_t) clamp((int) ::floor(x / _stepsizeKpc), (int) 0,
			(int) _samples - 1);
}

size_t SampledMagneticField::toUpperIndex(double x) {
	return (size_t) clamp((int) ::ceil(x / _stepsizeKpc), (int) 0,
			(int) _samples - 1);
}

void SampledMagneticField::init(float stepsizeKpc) {
	_stepsizeKpc = stepsizeKpc;
	_samples = _sizeKpc / _stepsizeKpc + 1;
	_grid.create(_samples, _sizeKpc);
	_grid.reset(Vector3f(0, 0, 0));
}

void SampledMagneticField::apply(const SmoothParticle &particle) {
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
}

#if 0
class DotProgress {
	double step, progress, line_progress, line_width;

	DotProgress(size_t total, size_t width = 80) :
	progress(0), line_progress(0) {
		step = 1. / total;
		line_width =
	}

	void next() {
		progress += step;
		line_progress += step;

		if ((s > 1000) && (i > 0) && ((i % (s / 1000)) == 0)) {
			dots++;
			std::cout << ".";
			if (dots > 100) {
				std::cout << " " << ((i * 100) / s) << " %" << std::endl;
				dots = 0;
			} else {
				std::cout.flush();
			}
		}

	}
};
#endif

void SampledMagneticField::load(const std::string &filename) {

#if CACHE
	size_t cells = _samples - 1;
	std::cout << "[TSphMagField] Samples: " << _samples << std::endl;

	std::string basename = _filename.substr(_filename.find_last_of('/') + 1);
	std::string dumpfile = basename + ".dump";
	std::cout << "[TSphMagField] checking previous field: " << dumpfile
	<< std::endl;
	if (_grid.restore(dumpfile) == false) {
#endif

	std::vector<SmoothParticle> particles;
	SmoothParticleHelper::read(filename, particles);

	size_t s = particles.size();
	for (size_t i = 0; i < s; i++) {
		apply(particles[i]);
	}

#if CACHE
	std::cout << "\n[TSphMagField] dump field" << std::endl;
	std::string tmpfile = basename + ".dump_";
	_grid.dump(tmpfile);
	::rename(tmpfile.c_str(), dumpfile.c_str());
#endif

#if STATS
	bool printStats = getenv("CRPROPA_SPH_STATS") != 0;
	std::ofstream stats;
	if (printStats) {
		stats.open("stats.csv");
		stats
		<< "x, y, z, h, m, rho, w, Bx_i, By_i, Bz_i, Bx_p, By_p, Bz_p\n";
	}
	float eMin = 1e30, eMax = 1e-30, eAvg = 0;
	for (size_t i = 0; i < s; i++) {
		SmoothParticle<double> &sp = _smoothParticles[i];
		Vector3f p = (sp.position + origin);
		TVector3D pos(p.x * kpc, p.y * kpc, p.z * kpc);
		if (pos.x() < _originKpc.x() || pos.x() > _originKpc.x() + _fSize)
		continue;
		if (pos.y() < _originKpc.y() || pos.y() > _originKpc.y() + _fSize)
		continue;
		if (pos.z() < _originKpc.z() || pos.z() > _originKpc.z() + _fSize)
		continue;
		TVector3D b = getField(pos);
		float dx = b.x() - sp.bfield.x * gauss;
		float dy = b.y() - sp.bfield.y * gauss;
		float dz = b.z() - sp.bfield.z * gauss;
		float e = sqrt(dx * dx + dy * dy + dz * dz) / b.mag();
		if (printStats) {
			stats << p.x << ", " << p.y << ", " << p.z << ", ";
			stats << sp.smoothingLength << ", " << sp.mass << ", " << sp.rho
			<< ", " << sp.weight() << ", ";
			stats << b.x() / nG << ", " << b.y() / nG << ", " << b.z() / nG
			<< ", ";
			stats << sp.bfield.x * gauss / nG << ", "
			<< sp.bfield.y * gauss / nG << ", "
			<< sp.bfield.z * gauss / nG << "\n";
		}
		if (e < eMin)
		eMin = e;
		if (e > eMax)
		eMax = e;
		eAvg += e;
	}
	if (s)
	eAvg /= s;
	std::cout << "[TSphMagField] error: " << eMin << " < " << eAvg << " < "
	<< eMax << std::endl;

	Vector3f vmax(0.f), vmin(1e31f);
	for (size_t ix = 0; ix < _samples; ix++) {
		for (size_t iy = 0; iy < _samples; iy++) {
			for (size_t iz = 0; iz < _samples; iz++) {
				Vector3f v = _fGrid.get(ix, iy, iz);
				if (v.length2() > vmax.length2())
				vmax = v;
				if (v.length2() < vmin.length2())
				vmin = v;
			}
		}
	}
	std::cout << "[TSphMagField] Min (nG): " << vmin / nG << std::endl;
	std::cout << "[TSphMagField] Max (nG): " << vmax / nG << std::endl;
#endif

}

void SampledMagneticField::load(const std::vector<SmoothParticle> &particles) {
	size_t s = particles.size();
	for (size_t i = 0; i < s; i++) {
		apply(particles[i]);
	}
}

Vector3f SampledMagneticField::getField(const Vector3f &positionKpc) const {
	checkPosition(positionKpc);

	// check: http://paulbourke.net/miscellaneous/interpolation/
	Vector3f r = (positionKpc - _originKpc) / _stepsizeKpc;

	if (r.x >= _samples || r.y >= _samples || r.z >= _samples || r.x <= 0
			|| r.y <= 0 || r.z <= 0)
		std::cerr << "[TSphMagField] invalid position: " << positionKpc
				<< std::endl;

	int ix = clamp((int) floor(r.x), 0, int(_samples - 1));
	int iX = ix + 1;
	double fx = r.x - ix;
	double fX = 1 - fx;

	int iy = clamp((int) floor(r.y), 0, int(_samples - 1));
	int iY = iy + 1;
	double fy = r.y - iy;
	double fY = 1 - fy;

	int iz = clamp((int) floor(r.z), 0, int(_samples - 1));
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

//----------------------------------------------------------------------------
// DirectMagneticField
//----------------------------------------------------------------------------

DirectMagneticField::DirectMagneticField(const Vector3f &originKpc,
		float sizeKpc) :
		MagneticField(originKpc, sizeKpc) {

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

void DirectMagneticField::init(size_t grid_size = 100) {
	_statistics.reset();
	_grid.create(grid_size, _sizeKpc);
}

void DirectMagneticField::load(const std::string &filename) {
	SmoothParticleHelper::read(filename, _particles);
	for (size_t i = 0; i < _particles.size(); i++) {
		index(i);
	}
}

void DirectMagneticField::load(const std::vector<SmoothParticle> &particles) {
	_particles = particles;
	for (size_t i = 0; i < _particles.size(); i++) {
		index(i);
	}
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

float DirectMagneticField::getRho(const Vector3f &positionKpc) const {
	checkPosition(positionKpc);

	// get index list
	Vector3f relativePosition = positionKpc - _originKpc;
	const std::vector<size_t> &idx = _grid.get(relativePosition.x,
			relativePosition.y, relativePosition.z);

// calculate field from overlapping particles
// see eq. 22 in diploma thesis by Rüdiger Pakmor TU Munich
	float rho = 0;
	for (size_t i = 0; i < idx.size(); i++) {
		const SmoothParticle &sp = _particles[idx[i]];
		double k = sp.kernel(positionKpc);
		if (k != 0) {
			rho += sp.mass * k * sp.weight();
		}
	}

	return rho;
}
