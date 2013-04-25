#pragma once

#include "Grid.h"
#include "Vector3.h"
#include "SmoothParticle.h"
#include "Database.h"
#include "Referenced.h"
#include "HCube.h"

#include <vector>
#include <string>
#include <exception>
#include <sstream>
#include <iostream>

namespace quimby {

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
	bool getRho(const Vector3f &positionKpc, float &rho) const;

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
	ref_ptr<HCubeFile<N> > _hcfile;
public:
	HCubeMagneticField(ref_ptr<HCubeFile<N> > hcube, const Vector3f &originKpc,
			float sizeKpc) :
			_hcfile(hcube) {
		_originKpc = originKpc;
		_sizeKpc = sizeKpc;
	}

	~HCubeMagneticField() {

	}

	bool getField(const Vector3f &position, Vector3f &b) const {
		b = _hcfile->hcube()->getValue(position - _originKpc, _sizeKpc);
		return true;
	}

};

typedef HCubeMagneticField<2> HCubeMagneticField2;
typedef HCubeMagneticField<4> HCubeMagneticField4;
typedef HCubeMagneticField<8> HCubeMagneticField8;
typedef HCubeMagneticField<16> HCubeMagneticField16;
typedef HCubeMagneticField<32> HCubeMagneticField32;
typedef HCubeMagneticField<64> HCubeMagneticField64;

class MagneticFieldPerformanceTest: public Referenced {
public:
	size_t nSteps, nTrajectories, nThreads;
	size_t seed;
	float step;
	MagneticFieldPerformanceTest();
	float randomwalk(MagneticField *field);
};

} // namespace
