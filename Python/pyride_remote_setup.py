#
#  tinremote_ext_setup.py
#  A tinremote extension module build script
#

import os
#from distutils.core import setup, Extension
from setuptools import setup, find_packages, Extension

# Remove the "-Wstrict-prototypes" compiler option, which isn't valid for C++.
import distutils.sysconfig
cfg_vars = distutils.sysconfig.get_config_vars()
for key, value in cfg_vars.items():
    if type(value) == str:
        cfg_vars[key] = value.replace("-Wstrict-prototypes", "")
# ==================================
with_video_data = 'WITH_VIDEO_DATA' in os.environ

BIG_ENDIAN_ARCH = [ 'sparc', 'powerpc', 'ppc' ]

macro = [ ('PYRIDE_REMOTE_CLIENT', None), ('USE_ENCRYPTION', None),
         ('NO_AUTO_DISCOVERY', None) ]

src_code = ['RemotePyModule.cpp', 'RemoteDataHandler.cpp', '../pyride_core/PyRideNetComm.cpp',
            '../pyride_core/ConsoleDataProcessor.cpp', '../pyride_core/PyRideCommon.cpp']
inc_dirs = ['../pyride_core']
lib = []
link_args = []
lib_dirs = []

if with_video_data:
  macro.append(('WITH_VIDEO_DATA', None))
  src_code = src_code + ['VideoStreamController.cpp', '../pyride_core/RTPDataReceiver.cpp']

osname = os.name
if osname == 'nt':
  macro = macro + [('WIN32', None), ('WIN32_LEAN_AND_MEAN', None), ('NO_WINCOM', None)]
  lib = ['ws2_32', 'Kernel32', 'libeay32', 'advapi32', 'oleaut32', 'user32', 'gdi32', # 'legacy_stdio_definitions',
    'ucrt', 'vcruntime']
  inc_dirs = inc_dirs + ['../Windows/include']
  lib_dirs = ['../Windows/lib/release']
  link_args = []
  if with_video_data:
    macro = macro + [('CCXX_STATIC', None), ('CCXX_NAMESPACES', None)]
    lib = lib + ['ccext2', 'ccrtp1', 'ccgnu2', 'jpeg-static' ]
elif osname == 'posix':
  if with_video_data:
    lib = ['pthread', 'ccext2', 'ccrtp1', 'ccgnu2','crypto', 'jpeg']
  else:
    lib = ['pthread']

  f = os.popen('uname -ms')
  (myos, myarch) = f.readline().split(' ')
  f.close()
  if myos == 'Darwin' or myos.endswith( 'BSD' ):
    macro.append(('BSD_COMPAT', None))
    inc_dirs.append( '/usr/local/opt/openssl/include' )
    lib_dirs.append( '/usr/local/opt/openssl/lib' )
  elif myos == 'SunOS':
    macro.append(('SOLARIS', None))

  for arch in BIG_ENDIAN_ARCH:
    if arch in myarch:
      macro.append(('WITH_BIG_ENDIAN', None))
      break
else:
  print "unknow platform. quit"
  exit( -1 )

module1 = Extension('pyride_remote',
                    define_macros = macro,
                    include_dirs = inc_dirs,
                    library_dirs = lib_dirs,
                    libraries = lib,
                    extra_link_args = link_args,
                    sources = src_code)

setup (name = 'pyride_remote',
       version = '0.1.0',
       description = 'This is a Python client extension module for PyRIDE.',
       url = 'https://github.com/uts-magic-lab/pyride_clients',
       author = 'Xun Wang',
       author_email = 'Wang.Xun@gmail.com',
       license = 'MIT',
       platforms = 'Linux, OS X, Windows',
       ext_modules = [module1])
