Quimby Manual
=============

:Author: Gero Müller <gero.mueller@physik.rwth-aachen.de>
:Version: $Revision: initial draft $
:Date: 07/01/2013

.. contents:: Table of Contents
   :backlinks: top


Introduction
------------

Quimby is a C++ library to access and process smooth particles, spezialized on the GADGET_ file format.
Its main purpose is to provide fast access to the magnetic field from SPH simulations.


Features
--------

Quimby consists of a C++ Library, a command line tool, Python_ bindings and scripts.

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
    
* Python_ bindings

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

* ROOT_, used in tests and examples
* Python_ and SWIG_, for Python_ bindings and tools

Configure and Build
~~~~~~~~~~~~~~~~~~~

Quimby uses CMake for the build process. The typical procedure on posix systems is

.. code-block:: bash

    mkdir build && cd build
    cmake
        -DCMAKE_INSTALL_PREFIX=$HOME/quimby
        -DCMAKE_BUILD_TYPE=Release
        -DQUIMBY_ENABLE_ROOT=True
        -DQUIMBY_ENABLE_PYTHON=True
        -DQUIMBY_ENABLE_TESTING=True
        ..
    make && make install
    
In the first line a directoy called build is created. This way all files created during
build are inside this folder. Simply remove this folder to cleanup all files.
CMake takes many parameters, and the most importat ones are shown.

CMAKE_INSTALL_PREFIX
  defines the directory where all files are installed. For a system-wide installation use '/usr/local'.
    
CMAKE_BUILD_TYPE
  defines the build type. Typically 'Debug' or 'Release'.

QUIMBY_ENABLE_ROOT
  enables the use of ROOT_ in some tests and tools.

QUIMBY_ENABLE_PYTHON
  enables the Python_ bindings.  

QUIMBY_ENABLE_TESTING
  enables testing methods and build unit tests.  

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
    
Used kernel function [Dolag2008]

.. code-block:: c++

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

Many features of quimby use smooth particles organized in a database. it provides fast and easy access to smooth particles.
Use the cli to convert snapshot files to a database. E.g. a 240 Mpc snapshot, with the center at 120Mpc.

.. code-block:: bash

    quimby db -h 0.7 -px 120000 -py 120000 -pz 120000 -bins 100 -o snap.db -f snap*

The database stores the particles and a regular grid for indexing. Uses DatabaseVisitor to access data.

HCube
~~~~~

Hierarchy Cube (HCube) is a multi resolution regular vector grid.
The top most cube in the hierarchy has the lowest resolution.
It has the index 0 is located at the beginning of the memory block.  
Each cell of the cube contains either the value for this region, or a relative index to the higher resolution cube.
This index is relative to the index of the referencing cube. 
References are encoded using nan for the x component of the vector while the y and z components contain the offset to the next cube.
This way the tree is encoded into the data.
The HCube itself does not know its origin or size to reduce the required memory. 


Examples
--------

Access SmoothParticles from FileDatabase in C++
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Start by including the Database headers

.. code-block:: c++
    
    #include "quimby/Database.h"   

Now in your code declare a FileDatabase_ object and open a file

.. code-block:: c++
    
    quimby::FileDatabase db;
    db.open("test.db");

All Database classes provide a function to get all SmoothParticles overlapping a certain region as a list.
To get all particles inside a 1 Mpc wide box
    
.. code-block:: c++
    
    std::vector< quimby::SmoothParticle > particles;
    quimby::Vector3f lower(-500, -500, -500);
    quimby::Vector3f upper(500, 500, 500);
    db.getParticles(lower, upper, particles);

By default the Database is is accessed via the Visitor design pattern.
The user provides a C++ class and for each matching particle the visit method is called.
For example, to calculate the average magnetic field vector of all particles overlapping the 1 Mpc box one would create the following visitor

.. code-block:: c++

    class AverageVisitor : public quimby::DatabaseVisitor {
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
    
Now this visitor can be applied to the database

.. code-block:: c++

    AverageVisitor avg;
    db.accept(lower, upper, avg);
    std::cout << "Average magnetic field of " << avg.count;
    std::cout << " particles: " << avg.average << std::endl;
    
Note however, that this example is numerically not stable: http://en.wikipedia.org/wiki/Numerical_stability

Access magnetic field in C++
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This time include the magnetic field headers

.. code-block:: c++

    #include "quimby/MagneticField.h"
    using namespace quimby;

Be careful not to use 'using namespace' in headers!
The magnetic field has its origin in 0, 0, 0 and has a size of 120 Mpc. 

.. code-block:: c++

    Vector3f originKpc = Vector3f(0, 0, 0);
    float siuzKpc = 120000;

In this example we use a precomputed HCube file with 4^3 samples per cube.

.. code-block:: c++
    
    ref_ptr<HCubeFile4> hf4 = new HCubeFile4("test.hc4");
    ref_ptr<HCubeMagneticField4> hm4 = new HCubeMagneticField4(hf4, originKpc, sizeKpc);

Now we can access the magnetic field:


Command Line Interface
----------------------

the quimby utility provides many functions.
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

    quimby db -o galaxy.db -f galaxy0.snap galaxy1.snap

Function: bigfield
~~~~~~~~~~~~~~~~~~

Options:

Example::

    quimby bigfield

Function: hc
~~~~~~~~~~~~

Options:

Example::

    quimby hc


References
----------

.. [Dolag2008] `arXiv:0807.3553 [astro-ph]`__
__ http://arxiv.org/abs/0807.3553v2

.. _GADGET: http://www.mpa-garching.mpg.de/galform/gadget/
.. _ROOT: http://root.cern.ch/
.. _Python: http://www.python.org/
.. _SWIG: http://www.swig.org/
