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
 catch( Swig::DirectorException &e ){
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

%feature("ref")   gadget::Referenced "$this->addReference();"
%feature("unref") gadget::Referenced "$this->removeReference();"

%include "gadget/Vector3.h"
%template(Vector3d) gadget::Vector3<double>;
%template(Vector3f) gadget::Vector3<float>;

%include "gadget/Referenced.h"
%include "gadget/SmoothParticle.h"

%feature("director") DatabaseVisitor;      
%template(DatabaseRefPtr) gadget::ref_ptr<gadget::Database>;
%include "gadget/Database.h"

%template(MagneticFieldRefPtr) gadget::ref_ptr<gadget::MagneticField>;
%template(SampledMagneticFieldRefPtr) gadget::ref_ptr<gadget::SampledMagneticField>;
%template(DirectMagneticFieldRefPtr) gadget::ref_ptr<gadget::DirectMagneticField>;
%include "gadget/MagneticField.h"

%include "gadget/HCube.h"
%template(HCube2) gadget::HCube<2>;
%template(HCube4) gadget::HCube<4>;
%template(HCube8) gadget::HCube<8>;
%template(HCube16) gadget::HCube<16>;
%template(HCube32) gadget::HCube<32>;
%template(HCube64) gadget::HCube<64>;
%template(HCube128) gadget::HCube<128>;
%template(HCube256) gadget::HCube<256>;

/*
%implicitconv mpc::ref_ptr<mpc::MagneticField>;
%template(MagneticFieldRefPtr) mpc::ref_ptr<mpc::MagneticField>;
%include "mpc/magneticField/MagneticField.h"

%include "mpc/Grid.h"
%include "mpc/GridTools.h"

%implicitconv mpc::ref_ptr<mpc::Grid<mpc::Vector3<float> > >;
%template(VectorGridRefPtr) mpc::ref_ptr<mpc::Grid<mpc::Vector3<float> > >;
%template(VectorGrid) mpc::Grid<mpc::Vector3<float> >;

%implicitconv mpc::ref_ptr<mpc::Grid<float> >;
%template(ScalarGridRefPtr) mpc::ref_ptr<mpc::Grid<float> >;
%template(ScalarGrid) mpc::Grid<float>;


%include "mpc/magneticField/MagneticFieldGrid.h"
%include "mpc/magneticField/SPHMagneticField.h"
%include "mpc/magneticField/JF2012Field.h"

%include "mpc/ExplicitRungeKutta.h"
%include "mpc/PhasePoint.h"

%include "mpc/module/BreakCondition.h"
%include "mpc/module/Boundary.h"
%include "mpc/module/Observer.h"
%include "mpc/module/SimplePropagation.h"
%include "mpc/module/DeflectionCK.h"
%include "mpc/module/Output.h"
%include "mpc/module/ElectronPairProduction.h"
%include "mpc/module/StochasticInteraction.h"
%include "mpc/module/NuclearDecay.h"
%include "mpc/module/PhotoPionProduction.h"
%include "mpc/module/PhotoDisintegration.h"
%include "mpc/module/Redshift.h"
%include "mpc/module/Tools.h"

%template(SourceRefPtr) mpc::ref_ptr<mpc::Source>;
%include "mpc/Source.h"

%template(ModuleListRefPtr) mpc::ref_ptr<mpc::ModuleList>;
%include "mpc/ModuleList.h"
*/