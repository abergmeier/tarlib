tarlib
======
( near neighbor of [zlib](http://www.zlib.net/), [libbzip2](http://www.bzip.org/) and [ziplib](https://github.com/abergmeier/ziplib) )

A passive, non blocking, in memory tar inflate library. Inspired by [zlib](http://www.zlib.net/). Big brother to [ziplib](https://github.com/abergmeier/ziplib).

Building:
---------
Contained in the project is a CMake project for building the library as well as a test program.
I would recommend ninja for building.
Following my command line:

    mkdir build
    cd build
    cmake –GNinja ..
    ninja

Usage:
------
_tarlib_ works very similar to [zlib](http://www.zlib.net/).
This means it is your duty to put data into a `tar_stream` and invoke `tar_inflate` until all input data was processed.
As a direct result you can use _tarlib_ for streaming and do not need a file.


For usage see _test/test.cpp_.

Limitations:
------------
Since I currently have no need to create tars, _tarlib_ only extracts tars.
Also you will need a C++ compiler to compile and a C++ stdlib for running tarlib.
The C interface is mainly to enable usage from C and to resemble [zlib](http://www.zlib.net/).

License:
--------
Apache License (see _LICENSE_ file)

Plan:
-----
Currently _tarlib_ is for testing the interface and see what needs to be ironed out.
If anyone wants to extend _tarlib_, pull requests are very welcome.
