%module(directors="1") gadget

%include stl.i
%include std_set.i
%include std_multiset.i
%include std_map.i
%include std_pair.i
%include std_multimap.i
%include std_vector.i
%include std_string.i
%include std_list.i
%include stdint.i
%include std_container.i
%include "exception.i"
%include "typemaps.i"

%{
#include "gadget/Database.h"
#include "gadget/MagneticField.h"
#include "gadget/Referenced.h"
#include "gadget/SmoothParticle.h"
#include "gadget/Vector3.h"
#include "gadget/HCube.h"
%}

%exception
{
	try
	{
		$action
	}
	catch (const std::exception& e) {
		SWIG_exception(SWIG_RuntimeError, e.what());
	}
	catch( Swig::DirectorException &e ) {
		PyErr_Print();
		SWIG_exception(SWIG_RuntimeError, e.getMessage());
	} catch (...) {
		SWIG_exception(SWIG_RuntimeError, "unknown exception");
	}
}

%define TPL_REF_PTR(name, type)
%template(name) type;
%implicitconv gadget::ref_ptr< type >;
%template(name ## RefPtr) gadget::ref_ptr< type >;
%enddef

%define REF_PTR(name, type)
%implicitconv gadget::ref_ptr< type >;
%template(name ## RefPtr) gadget::ref_ptr< type >;
%enddef

%ignore operator<<;
%ignore operator>>;
%ignore *::operator=;
%ignore *::operator!;
%ignore operator gadget::**;
/*
 %ignore operator mpc::Candidate*;
 %ignore operator mpc::Module*;
 %ignore operator mpc::ModuleList*;
 %ignore operator mpc::MagneticField*;
 */

%feature("ref") gadget::Referenced "$this->addReference();"
%feature("unref") gadget::Referenced "$this->removeReference();"

%include "gadget/Vector3.h"
%template(Vector3d) gadget::Vector3<double>;
%template(Vector3f) gadget::Vector3<float>;

%include "gadget/Referenced.h"
%include "gadget/SmoothParticle.h"

%feature("director") DatabaseVisitor;
%template(DatabaseRefPtr) gadget::ref_ptr<gadget::Database>;
%include "gadget/Database.h"

%include "gadget/HCube.h"
%template(HCube2) gadget::HCube<2>;
%template(HCube4) gadget::HCube<4>;
%template(HCube8) gadget::HCube<8>;
%template(HCube16) gadget::HCube<16>;
%template(HCube32) gadget::HCube<32>;
%template(HCube64) gadget::HCube<64>;

TPL_REF_PTR(HCubeFile2, gadget::HCubeFile<2>)
TPL_REF_PTR(HCubeFile4, gadget::HCubeFile<4>)
TPL_REF_PTR(HCubeFile8, gadget::HCubeFile<8>)
TPL_REF_PTR(HCubeFile16, gadget::HCubeFile<16>)
TPL_REF_PTR(HCubeFile32, gadget::HCubeFile<32>)
TPL_REF_PTR(HCubeFile64, gadget::HCubeFile<64>)

%apply float &INOUT { float &rho };
%include "gadget/MagneticField.h"
REF_PTR(MagneticField, gadget::MagneticField)
REF_PTR(SampledMagneticField, gadget::SampledMagneticField)
REF_PTR(DirectMagneticField, gadget::DirectMagneticField)
TPL_REF_PTR(HCubeMagneticField2, gadget::HCubeMagneticField<2>)
TPL_REF_PTR(HCubeMagneticField4, gadget::HCubeMagneticField<4>)
TPL_REF_PTR(HCubeMagneticField8, gadget::HCubeMagneticField<8>)
TPL_REF_PTR(HCubeMagneticField16, gadget::HCubeMagneticField<16>)
TPL_REF_PTR(HCubeMagneticField32, gadget::HCubeMagneticField<32>)
TPL_REF_PTR(HCubeMagneticField64, gadget::HCubeMagneticField<64>)
