Setup
-----

Requirements
~~~~~~~~~~~~

* C++ Compiler
* CMake_

Optional:

* ROOT_, used in tests and examples
* Python_ and SWIG_, for Python_ bindings and tools


Download
~~~~~~~~

Either download the latest source archive from 
https://forge.physik.rwth-aachen.de/hg/quimby/archive/tip.tar.gz
or use Merurial_ to create a clone of the repository:
hg clone https://forge.physik.rwth-aachen.de/hg/quimby/

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

Finally run 'make' and 'make install' to compile and install Quimby.

.. _GADGET: http://www.mpa-garching.mpg.de/galform/gadget/
.. _ROOT: http://root.cern.ch/
.. _Python: http://www.python.org/
.. _SWIG: http://www.swig.org/
.. _CMake: http://www.cmake.org/