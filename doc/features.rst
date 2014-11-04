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
