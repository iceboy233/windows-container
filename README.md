Windows Container
=================

This project provides a lightweight sandbox for Windows.

Build Instructions
------------------

Use Visual Studio 2015.

To build the python binding, set the `PYTHON_INCLUDE` and `PYTHON_LIB`
environment variable to the `include` and `libs` directory of a python
installation in your system. The build configuration needs to match the
python installation (Debug/Release, 32/64bit). Both python 2 and 3 are
supported.

Supported Operating Systems
---------------------------

The following versions of operating systems are supported:
* Windows Vista / 7 / 8 / 8.1 / 10
* Windows Server 2008 / 2008 R2 / 2012 / 2012 R2 / 2016
