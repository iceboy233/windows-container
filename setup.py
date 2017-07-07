from distutils.core import setup, Extension
from os import path
from glob import glob

setup(
    name = 'windows-container',
    ext_modules = [
        Extension('winc',
            define_macros = [
                # see https://msdn.microsoft.com/en-us/library/windows/desktop/ms683219(v=vs.85).aspx
                ('PSAPI_VERSION', '1'),
                # FIXME: _HAS_EXCEPTIONS only has effect when /MT
                # But python builds it with /MD
                # ('_HAS_EXCEPTIONS', '0'),
                # For future?
                ('BINDING_PYTHON_EXPORTS', None),
                # Use UNICODE
                ('_UNICODE', None),
                ('UNICODE', None)],
            include_dirs = [
                'include',
                '.'],
            libraries = [
                'Advapi32',
                'Ntdll',
                'User32',
                'Psapi'],
            sources = glob(path.join('core', '*.cc')) +
                      glob(path.join('bindings', 'binding_python', '*.cc')))])
