#pragma once

#include "MagneticField.h"
#include "HCube.h"

#include <sstream>

namespace quimby {

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
        try {
            b = _hcfile->hcube()->getValue(position - _originKpc, _sizeKpc);
            return true;
        } catch(invalid_position &e) {
            std::ostringstream str;
            str << "HCubeMagneticField: invalid position: (" << position << ") kpc.";
            str << "Origin: (" << _originKpc << ") kpc.";
            str << "Size: " << _sizeKpc << " kpc.";
            throw std::runtime_error(str.str());
        }  
	}

};

typedef HCubeMagneticField<2> HCubeMagneticField2;
typedef HCubeMagneticField<4> HCubeMagneticField4;
typedef HCubeMagneticField<8> HCubeMagneticField8;
typedef HCubeMagneticField<16> HCubeMagneticField16;
typedef HCubeMagneticField<32> HCubeMagneticField32;
typedef HCubeMagneticField<64> HCubeMagneticField64;

} // namespace quimby
