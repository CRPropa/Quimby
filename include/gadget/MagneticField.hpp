#ifndef GADGET_MAGNETIC_FIELD_H_
#define GADGET_MAGNETIC_FIELD_H_

#include "gadget/Grid.hpp"
#include "gadget/Vector3.hpp"
#include "gadget/SmoothParticle.hpp"

#include <math.h>
#include <vector>

class MagneticField {
protected:
	float _sizeKpc;
	Vector3f _originKpc;
public:
	MagneticField(const Vector3f &originKpc, double sizeKpc);
	const float &getSize() const;
	const Vector3f &getOrigin() const;

	virtual Vector3f getField(const Vector3f &position) const = 0;
};

class SampledMagneticField: public MagneticField {
	typedef Vector3<float> vector_t;
	typedef Grid<vector_t> grid_t;
	double _stepsizeKpc;
	size_t _samples;
	grid_t _grid;
	size_t toLowerIndex(double x);
	size_t toUpperIndex(double x);

	void apply(const SmoothParticle &particle);
public:
	SampledMagneticField(const Vector3f &originKpc, float sizeKpc);
	Vector3f getField(const Vector3f &position) const;
	void init(float stepsizeKpc);
	void load(const std::string &filename);
	void load(const std::vector<SmoothParticle> &particles);

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
	};
private:
	mutable Statistics _statistics;
	Grid<std::vector<size_t> > _grid;
	std::vector<SmoothParticle> _particles;

	void index(size_t i);

public:
	DirectMagneticField(const Vector3f &originKpc, float sizeKpc);
	Vector3f getField(const Vector3f &position) const;
	void init(size_t gid_size);
	void load(const std::string &filename);
	void load(const std::vector<SmoothParticle> &particles);

	const Statistics &getStatistics() const;
};

#endif
