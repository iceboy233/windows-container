# Windows Container

This project provides a lightweight sandbox for Windows.

## Build Instructions

Use Visual Studio 2015.

To build the python binding, set the `PYTHON_INCLUDE` and `PYTHON_LIB`
environment variable to the `include` and `libs` directory of a python
installation in your system. The build configuration needs to match the
python installation (Debug/Release, 32/64bit). Both python 2 and 3 are
supported.

Use `setup.py`

For python 3.5+, use `python setup.py build` to build and `python setup.py install` to install

## Python Binding

Example: launch cmd in a container.

    import winc
    container = winc.Container()
    target = container.spawn(u'C:\\Windows\\system32\\cmd.exe')
    target.start().wait_for_process()

Try `whoami` in the launched cmd, should get `ERROR: Access is denied.`.

You may enable the current user's identity and try again.

    container.add_restricted_sid(container.logon.user_sid)

## Supported Operating Systems

The following versions of operating systems are supported:
* Windows Vista / 7 / 8 / 8.1 / 10
* Windows Server 2008 / 2008 R2 / 2012 / 2012 R2 / 2016
