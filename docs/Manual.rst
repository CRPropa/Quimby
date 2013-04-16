libGadget Manual
================

:Author: Gero Müller <gero.mueller@physik.rwth-aachen.de>
:Version: $Revision: initial draft $
:Date: 07/01/2013

.. contents::

Introduction
------------

libGadget is a C++ library to access and process smooth particles, spezialized on the GADGET_ file format.
Its main purpose is to provide fast access to the magnetic field from SPH simulations.
 
Features
--------

libGadget consists of a C++ Library, a command line tool, python bindings and scripts.

C++ Library
~~~~~~~~~~~

* Simple GADGET_ file acccess
* SmoothParticle class, incl. FileDatabase_
* Spatial partitioning classes
* MagneticField classes

  - Directly on SmoothParticles. Very slow but full resolution
  - Sampled. The magnetic field is sampled onto a regular grid. 
  
    + Grid. Simple grid, fast for small fields.
    + PagedGrid. Only currently used parts of the field are kept in memory.
    + MultiResolutionMagneticField. The magnetic field is sampled using different resulutions depending on the local turbulence.
    
* Python bindings

Command Line Interface
~~~~~~~~~~~~~~~~~~~~~~

* Create raw magentic field from smooth particles
* Create raw density field from smooth particles
* Create database from gagdet files.
* Create multi resolution magnetic field from raw field

Installation
------------

Requirements
~~~~~~~~~~~~

* C++ Compiler
* CMake

Optional:

* ROOT, used in tests and examples
* Python and SWIG, for python bindings and tools

Configuring
~~~~~~~~~~~

libGadget uses CMake for the build process. The typical procedure on posix systems is::

    mkdir build && cd build
    cmake -DCMAKE_INSTALL_PREFIX=$HOME/gadget -DCMAKE_BUILD_TYPE=Release -DENABLE_ROOT=True -DENABLE_PYTHON=True ..
    make && make install
	
In the first line a directoy called build is created. This way all files created during
build are inside this folder. Simply remove this folder to cleanup all files.
CMake takes many parameters, and the most importat ones are shown.

* CMAKE_INSTALL_PREFIX defines the directory where all files are installed. For a system-wide installation use '/usr/local'.
* CMAKE_BUILD_TYPE defines the build type. Typically 'Debug' or 'Release'.
* ENABLE_ROOT enables the use of ROOT_ in some tests and tools.
* ENABLE_PYTHON enables the Python_ bindings.  

Finally 'make' and 'make install' compile and install GADGET_.

Concepts
--------

SmoothParticle
~~~~~~~~~~~~~~

SmoothParticles have the following properties: 

* position: The supergalactic position of the particle in kpc
* bfield: The magnetic field vector carried by the particle in Gauss.
* smoothingLength: The size of the particle in kpc.
* mass: The mass value of the particle in 10^10 Msol.
* rho: The mass density value calculated from the clostest neighbours.
    
kernel::

    // see http://arxiv.org/abs/0807.3553v2
    static float_t kernel(float_t r) {
        if (r < 0.5) {
            return 1.0 + 6 * r * r * (r - 1);
        } else if (r < 1.0) {
            float_t x = (1 - r);
            return 2 * x * x * x;
        } else {
            return 0.0;
        }
    }

FileDatabase
~~~~~~~~~~~~

Many features of gadget use smooth particles organized in a database. it provides fast and easy access to smooth particles.
Use the cli to convert snapshot files to a database. E.g. a 240 Mpc snapshot, with the center at 120Mpc. ::

    gadget db -h 0.7 -px 120000 -py 120000 -pz 120000 -bins 100 -o snap.db -f snap*

The database stores the particles and a regular grid for indexing. Uses DatabaseVisitor to access data.

HCube
~~~~~

Hierarchy Cube (HCube) is a multi resolution regular vector grid.
The top most cube in the hierachy has the lowest resolution.
It has the index 0 is located at the beginning of the memory block.  
Each cell of the cube contains either the value for this region, or an index to the higher resolution cube.
This index is relative to the index of the referencing cube. 
References are encoded using nan for the x component of the vector while the y and z components contain the offset to the next cube.
This way the tree is encoded into the data.
The HCube itself does not know its origin or size to reduce the required memory. 

Examples
--------

Access SmoothParticles from FileDatabase in C++
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Start by including the Database headers::

    #include "gadget/Database.h"
    
Now in your code declare a FileDatabase_ object and open a file::

    gadget::FileDatabase db;
    db.open("test.db");
    
All Database classes provide a function to get all SmoothParticles overlapping a certain region as a list.
To get all particles inside a 1 Mpc wide box::
    
    std::vector<gadget::SmoothParticle> particles;
    gadget::Vector3f lower(-500, -500, -500);
    gadget::Vector3f upper(500, 500, 500);
    db.getParticles(lower, upper, particles);
    
By default the Database is is accessed via the Visitor design pattern.
The user provides a C++ class and for each matching particle the visit method is called.
For example, to calculate the average magnetic field vector of all particles overlapping the 1 Mpc box one would create the following visitor::

    class AverageVisitor : public gadget::DatabaseVisitor {
    public:
        Vector3f average;
        size_t count;
        
        void begin() {
            average = Vector3f(0,0,0);
            count = 0;
        }
        
        void visit(const SmoothParticle &p) {
            average += p.bfield;
            count += 1;
        }
        
        void end() {
            average /= count;
        }   
    };
    
Now this visitor can be applied to the database::

    AverageVisitor avg;
    db.accept(lower, upper, avg);
    std::cout << "Average magnetic field of " << avg.count;
    std::cout << " particles: " << avg.average << std::endl;
    
	 
Command Line Interface
----------------------

the gadget utility provides many functions.
Like the git, hg or svn tools, the first paramter is the function name, followed by options for this function.

Function: db
~~~~~~~~~~~~

create database file from GADGET_ files.

Options:

-f     list of input GADGET_ files, space seperated
-o     filename of the database
-h     Hubble constant to use, default: 0.7
-px, -py, -pz
       x, y, z of the pivot point for hubble streching, default: 120000
-bins  number of bins used for database lookup, default: 100

Example:::

    gadget db -o galaxy.db -f galaxy0.snap galaxy1.snap

Function: bigfield
~~~~~~~~~~~~~~~~~~

Options:

Example::

    gadget bigfield

Function: hc
~~~~~~~~~~~~

Options:

Example::

    gadget hc
    
.. _GADGET: http://www.mpa-garching.mpg.de/galform/gadget/
.. _ROOT: http://root.cern.ch/
.. _Python: http://www.python.org/