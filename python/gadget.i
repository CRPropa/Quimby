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

%template(HCubeFile2) gadget::HCubeFile<2>;
%template(HCubeFile2RefPtr) gadget::ref_ptr<gadget::HCubeFile2>;
%template(HCubeFile4) gadget::HCubeFile<4>;
%template(HCubeFile4RefPtr) gadget::ref_ptr<gadget::HCubeFile4>;
%template(HCubeFile8) gadget::HCubeFile<8>;
%template(HCubeFile8RefPtr) gadget::ref_ptr<gadget::HCubeFile8>;
%template(HCubeFile16) gadget::HCubeFile<16>;
%template(HCubeFile16RefPtr) gadget::ref_ptr<gadget::HCubeFile16>;
%template(HCubeFile32) gadget::HCubeFile<32>;
%template(HCubeFile32RefPtr) gadget::ref_ptr<gadget::HCubeFile32>;
%template(HCubeFile64) gadget::HCubeFile<64>;
%template(HCubeFile64RefPtr) gadget::ref_ptr<gadget::HCubeFile64>;

%template(MagneticFieldRefPtr) gadget::ref_ptr<gadget::MagneticField>;
%template(SampledMagneticFieldRefPtr) gadget::ref_ptr<gadget::SampledMagneticField>;
%template(DirectMagneticFieldRefPtr) gadget::ref_ptr<gadget::DirectMagneticField>;

%include "gadget/MagneticField.h"
%template(HCubeMagneticField2) gadget::HCubeMagneticField<2>;
%template(HCubeMagneticField2RefPtr) gadget::ref_ptr<gadget::HCubeMagneticField2>;
%template(HCubeMagneticField4) gadget::HCubeMagneticField<4>;
%template(HCubeMagneticField4RefPtr) gadget::ref_ptr<gadget::HCubeMagneticField4>;
%template(HCubeMagneticField8) gadget::HCubeMagneticField<8>;
%template(HCubeMagneticField8RefPtr) gadget::ref_ptr<gadget::HCubeMagneticField8>;
%template(HCubeMagneticField16) gadget::HCubeMagneticField<16>;
%template(HCubeMagneticField16RefPtr) gadget::ref_ptr<gadget::HCubeMagneticField16>;
%template(HCubeMagneticField32) gadget::HCubeMagneticField<32>;
%template(HCubeMagneticField32RefPtr) gadget::ref_ptr<gadget::HCubeMagneticField32>;
%template(HCubeMagneticField64) gadget::HCubeMagneticField<64>;
%template(HCubeMagneticField64RefPtr) gadget::ref_ptr<gadget::HCubeMagneticField64>;
