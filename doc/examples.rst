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
    
.. note:: This example is numerically not stable: http://en.wikipedia.org/wiki/Numerical_stability

Access magnetic field in C++
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This time include the magnetic field headers

.. code-block:: c++

    #include "quimby/MagneticField.h"
    using namespace quimby;

.. warning:: Be careful not to use 'using namespace' in header files!

The magnetic field has its origin in 0, 0, 0 and has a size of 120 Mpc. 

.. code-block:: c++

    Vector3f originKpc = Vector3f(0, 0, 0);
    float sizeKpc = 120000;

In this example we use a precomputed HCube file with 4^3 samples per cube.

.. code-block:: c++

    ref_ptr<HCubeFile4> hf4 = new HCubeFile4("test.hc4");
    ref_ptr<HCubeMagneticField4> hm4 = new HCubeMagneticField4(hf4, originKpc, sizeKpc);

Now we can access the magnetic field:

.. code-block:: c++

    Vector3f bfield;
    if (hm4->getField(Vector3f(60000, 60000, 60000), bfield))
        std::cout << bfield << std::endl;
    else
        std::cout << "Failed to get B-Field!" << std::endl;


Access Gadget Files in Python
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

First include the package 'quimby', and create a GadgetFile instance:

.. code-block:: python

    import quimby
    f = quimby.GadgetFile()

Then open the file 'snapshot.0' and print information about the data blocks in the file.

.. code-block:: python

    f.open('snapshot.0')
    f.printBlocks()

Now get the header information and print the number of type 0 (barions) particles:

.. code-block:: python

    h = f.getHeader()
    print h.particleNumberList[0]

Finally get the float values from the "POS " block:

.. code-block:: python

    pos = quimby.FloatVector()
    f.readFloatBlock("POS ", pos)
