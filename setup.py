from distutils.core import setup, Extension
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
            ],
            include_dirs = [
                'include',
                '.'
            ],
            libraries = [
                'advapi32',
                'ntdll',
                'user32',
                'psapi'
            ],
            sources = glob('core/*.cc') +
                      glob('bindings/binding_python/*.cc')
        )
    ]
)
