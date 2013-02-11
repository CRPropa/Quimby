#ifndef _GADGET_MAGNETIC_FIELD_H_
#define _GADGET_MAGNETIC_FIELD_H_

#include "gadget/Grid.h"
#include "gadget/Vector3.h"
#include "gadget/SmoothParticle.h"
#include "gadget/Database.h"
#include "gadget/Referenced.h"
#include "gadget/HCube.h"

#include <vector>
#include <string>
#include <exception>
#include <sstream>
#include <iostream>

namespace gadget {

class MagneticField: public Referenced {
protected:
	Vector3f _originKpc;
	float _sizeKpc;
public:
	MagneticField();
	virtual ~MagneticField() {
	}
	const float &getSize() const;
	const Vector3f &getOrigin() const;

	virtual bool getField(const Vector3f &position, Vector3f &b) const = 0;
};

class SampledMagneticField: public MagneticField {
	typedef Vector3<float> vector3_t;
	typedef Grid<vector3_t> grid_t;
	double _stepsizeKpc;
	size_t _samples;
	grid_t _grid;
	double _broadeningFactor;
	bool interpolate;
	size_t toLowerIndex(double x);
	size_t toUpperIndex(double x);
public:
	SampledMagneticField(size_t samples);
	bool getField(const Vector3f &position, Vector3f &b) const;
	void init(const Vector3f &originKpc, float sizeKpc);
	void init(const Vector3f &originKpc, float sizeKpc, Database &db);
	void init(const Vector3f &originKpc, float sizeKpc,
			const std::vector<SmoothParticle> &particles);

	void dump(const std::string &dumpfilename);
	bool restore(const std::string &dumpfilename);
	void sampleParticle(const SmoothParticle &particle);
	void setBroadeningFactor(double broadening);
	void setInterpolate(bool interpolate);
};

class DirectMagneticField: public MagneticField {
public:
	struct Statistics {
		size_t totalSum;
		size_t totalCount;
		size_t actualSum;
		size_t actualCount;

		void reset() {
			totalSum = 0;
			totalCount = 0;
			actualSum = 0;
			actualCount = 0;
		}

		double getAverageTotal() const {
			return (double) totalSum / (double) totalCount;
		}
		double getAverageActual() const {
			return (double) actualSum / (double) actualCount;
		}
	};
private:
	mutable Statistics _statistics;
	Grid<std::vector<size_t> > _grid;
	std::vector<SmoothParticle> _particles;
	void index(size_t i);
public:
	DirectMagneticField(size_t grid_size);
	bool badPosition(const Vector3f &positionKpc) const;
	bool getField(const Vector3f &position, Vector3f &field) const;
	bool getRho(const Vector3f &positionKpc, size_t &overlaps,
			float &rho) const;

	void init(const Vector3f &originKpc, float sizeKpc);
	void init(const Vector3f &originKpc, float sizeKpc, Database &db);
	void init(const Vector3f &originKpc, float sizeKpc,
			const std::vector<SmoothParticle> &particles);

	const std::vector<SmoothParticle> getParticles() const {
		return _particles;
	}
	const Statistics &getStatistics() const {
		return _statistics;
	}

};

template<size_t N>
class HCubeMagneticField: public MagneticField {
	HCube<N> *_hcube;
public:
	HCubeMagneticField(HCube<N> *hcube, const Vector3f &originKpc,
			float sizeKpc) :
			_hcube(hcube) {
		_originKpc = originKpc;
		_sizeKpc = sizeKpc;
	}

	~HCubeMagneticField() {

	}

	bool getField(const Vector3f &position, Vector3f &b) const {
		b = _hcube->getValue(position - _originKpc, _sizeKpc);
		return true;
	}

};

class MagneticFieldPerformanceTest: public Referenced {
public:
	size_t nSteps, nTrajectories, nThreads;
	size_t seed;
	float step;
	MagneticFieldPerformanceTest();
	float randomwalk(MagneticField *field);
};

} // namespace gadget

#endif // _GADGET_MAGNETIC_FIELD_H_
