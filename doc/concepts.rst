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
    
Used kernel function [Dolag2008]_

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

.. image:: /hcube.svg

.. rubric:: Bibliography

.. [Dolag2008] `2009MNRAS.398.1678D Dolag, K., \& Stasyszyn, F.\ 2009, \mnras, 398, 1678`__
__ http://adsabs.harvard.edu/abs/2009MNRAS.398.1678D
