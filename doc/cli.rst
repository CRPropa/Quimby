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


