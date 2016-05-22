%module(directors="1") quimby

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

%template(FloatVector) std::vector<float>;
%template(DoubleVector) std::vector<double>;
%template(UnsignedIntVector) std::vector<unsigned int>;


%{
#include "quimby/Grid.h"
#include "quimby/Database.h"
#include "quimby/MagneticField.h"
#include "quimby/Referenced.h"
#include "quimby/SmoothParticle.h"
#include "quimby/Vector3.h"
#include "quimby/MMapFile.h"
#include "quimby/HCube.h"
#include "quimby/HCubeMagneticField.h"
#include "quimby/GadgetFile.h"
%}

%exception
{
	try
	{
		$action
	}
	catch( Swig::DirectorException &e ) {
		PyErr_Print();
		SWIG_exception(SWIG_RuntimeError, e.getMessage());
	}
	catch (const std::exception& e) {
		SWIG_exception(SWIG_RuntimeError, e.what());
	}
    catch (...) {
		SWIG_exception(SWIG_RuntimeError, "unknown exception");
	}
}

%define TPL_REF_PTR(name, type)
%template(name) type;
%implicitconv quimby::ref_ptr< type >;
%template(name ## RefPtr) quimby::ref_ptr< type >;
%enddef

%define REF_PTR(name, type)
%implicitconv quimby::ref_ptr< type >;
%template(name ## RefPtr) quimby::ref_ptr< type >;
%enddef

%ignore operator<<;
%ignore operator>>;
%ignore *::operator=;
%ignore *::operator!;


%feature("ref") quimby::Referenced "$this->addReference();"
%feature("unref") quimby::Referenced "$this->removeReference();"

%include "quimby/Vector3.h"
%template(Vector3d) quimby::Vector3<double>;
%template(Vector3f) quimby::Vector3<float>;

%include "quimby/Grid.h"
%template(Vector3dGrid) quimby::Grid< quimby::Vector3<double> >;
%template(Vector3fGrid) quimby::Grid< quimby::Vector3<float> >;

%include "quimby/Referenced.h"
%include "quimby/SmoothParticle.h"

%template(SmoothParticleVector) std::vector<quimby::SmoothParticle>;

%include "quimby/MMapFile.h"

%feature("director") DatabaseVisitor;
REF_PTR(Database, quimby::Database)
%include "quimby/Database.h"

%apply float &INOUT { float &rho };
REF_PTR(MagneticField, quimby::MagneticField)
REF_PTR(SampledMagneticField, quimby::SampledMagneticField)
REF_PTR(DirectMagneticField, quimby::DirectMagneticField)
%include "quimby/MagneticField.h"


%include "quimby/HCube.h"
%template(HCube2) quimby::HCube<2>;
%template(HCube4) quimby::HCube<4>;
%template(HCube8) quimby::HCube<8>;
%template(HCube16) quimby::HCube<16>;
%template(HCube32) quimby::HCube<32>;
%template(HCube64) quimby::HCube<64>;

TPL_REF_PTR(HCubeFile2, quimby::HCubeFile<2>)
TPL_REF_PTR(HCubeFile4, quimby::HCubeFile<4>)
TPL_REF_PTR(HCubeFile8, quimby::HCubeFile<8>)
TPL_REF_PTR(HCubeFile16, quimby::HCubeFile<16>)
TPL_REF_PTR(HCubeFile32, quimby::HCubeFile<32>)
TPL_REF_PTR(HCubeFile64, quimby::HCubeFile<64>)


%include "quimby/HCubeMagneticField.h"
TPL_REF_PTR(HCubeMagneticField2, quimby::HCubeMagneticField<2>)
TPL_REF_PTR(HCubeMagneticField4, quimby::HCubeMagneticField<4>)
TPL_REF_PTR(HCubeMagneticField8, quimby::HCubeMagneticField<8>)
TPL_REF_PTR(HCubeMagneticField16, quimby::HCubeMagneticField<16>)
TPL_REF_PTR(HCubeMagneticField32, quimby::HCubeMagneticField<32>)
TPL_REF_PTR(HCubeMagneticField64, quimby::HCubeMagneticField<64>)

%include "quimby/GadgetFile.h"
REF_PTR(GadgetFile, quimby::GadgetFile)

%pythoncode %{

def loadHCubeMagneticField(cfgfile, modus=Auto):
    import json, os
    cfg = json.load(open(cfgfile))
    offset = Vector3f(float(cfg["offset"][0]),float(cfg["offset"][1]),float(cfg["offset"][2]))
    size = float(cfg["size"])
    n = cfg["n"]
    filename = u"%s.hc%d" % (cfgfile[:-5], n)
    if "filename" in cfg:
        filename = cfg["filename"]
        filename = os.path.join(os.path.dirname(cfgfile), filename)
    hcf = globals()['HCubeFile%d'%n](filename.encode("utf-8"), modus)
    hcm = globals()['HCubeMagneticField%d'%n](hcf, offset, size)
    return hcm  

%}
