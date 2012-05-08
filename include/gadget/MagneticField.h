#ifndef _GADGET_MAGNETIC_FIELD_H_
#define _GADGET_MAGNETIC_FIELD_H_

#include "gadget/Grid.h"
#include "gadget/Vector3.h"
#include "gadget/SmoothParticle.h"
#include "gadget/Database.h"

#include <vector>
#include <string>
#include <exception>
#include <sstream>
#include <iostream>

namespace gadget {

class invalid_position: public std::exception {
	std::string _msg;
public:
	invalid_position(const Vector3f &v) {
		std::stringstream ss;
		ss << "Invalid position: " << v;
		_msg = ss.str();
	}

	virtual ~invalid_position() throw () {
	}

	virtual const char* what() const throw () {
		return _msg.c_str();
	}

};

class MagneticField {
protected:
	Vector3f _originKpc;
	float _sizeKpc;
	void checkPosition(const Vector3f &positionKpc) const;
public:
	MagneticField();
	virtual ~MagneticField() {
	}
	const float &getSize() const;
	const Vector3f &getOrigin() const;

	virtual Vector3f getField(const Vector3f &position) const = 0;
};

class SampledMagneticField: public MagneticField {
	typedef Vector3<float> vector3_t;
	typedef Grid<vector3_t> grid_t;
	double _stepsizeKpc;
	size_t _samples;
	grid_t _grid;
	double _broadeningFactor;
	size_t toLowerIndex(double x);
	size_t toUpperIndex(double x);
public:
	SampledMagneticField(size_t samples);
	Vector3f getField(const Vector3f &position) const;
	void init(const Vector3f &originKpc, float sizeKpc);
	void init(const Vector3f &originKpc, float sizeKpc, Database &db);
	void init(const Vector3f &originKpc, float sizeKpc,
			const std::vector<SmoothParticle> &particles);

	void dump(const std::string &dumpfilename);
	bool restore(const std::string &dumpfilename);
	void sampleParticle(const SmoothParticle &particle);
	void setBroadeningFactor(double broadening);
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
	Vector3f getField(const Vector3f &position) const;
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
	float getRho(const Vector3f &positionKpc, size_t &overlaps) const;

};

} // namespace gadget

#endif // _GADGET_MAGNETIC_FIELD_H_
