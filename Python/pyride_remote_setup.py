#
#  tinremote_ext_setup.py
#  A tinremote extension module build script
#

import os
from distutils.core import setup, Extension
#from setuptools import setup, find_packages, Extension
#import distutilscross

# Remove the "-Wstrict-prototypes" compiler option, which isn't valid for C++.
import distutils.sysconfig
cfg_vars = distutils.sysconfig.get_config_vars()
for key, value in cfg_vars.items():
    if type(value) == str:
        cfg_vars[key] = value.replace("-Wstrict-prototypes", "")
# ==================================

BIG_ENDIAN_ARCH = [ 'sparc', 'powerpc', 'ppc' ]

macro = [ ('PYRIDE_REMOTE_CLIENT', None), ('USE_ENCRYPTION', None),
         ('NO_AUTO_DISCOVERY', None) ]

lib = []

osname = os.name
if osname == 'nt':
  macro = macro + [('WIN32', None), ('WIN32_LEAN_AND_MEAN', None), ('NO_WINCOM', None)]
  lib = ['ws2_32', 'Kernel32', 'libeay32', 'advapi32', 'oleaut32', 'user32']
  lib = lib + ['ccext2', 'ccrtp1', 'ccgnu2', 'turbojpeg' ]
  inc_dirs = ['../Windows/include', '../Common']
  lib_dirs = ['../Windows/lib']
elif osname == 'posix':
  lib = ['pthread', 'ccext2', 'ccrtp1', 'ccgnu2','crypto', 'jpeg' ]
  #lib = [ 'pthread', 'crypto' ]
  inc_dirs = ['../Common']
  lib_dirs = []
  f = os.popen('uname -ms')
  (myos, myarch) = f.readline().split(' ')
  f.close()
  if myos == 'Darwin' or myos.endswith( 'BSD' ):
    macro.append(('BSD_COMPAT', None))
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
                    sources = ['RemotePyModule.cpp', 'RemoteDataHandler.cpp', 'VideoStreamController.cpp', '../Common/PyRideNetComm.cpp', '../Common/ConsoleDataProcessor.cpp', '../Common/PyRideCommon.cpp', '../Common/RTPDataReceiver.cpp'])

setup (name = 'pyride_remote',
       version = '1.0.0',
       description = 'This is a Python client extension module for PyRIDE.',
       author = 'Xun Wang',
       author_email = 'Wang.Xun@gmail.com',
       license = 'GPL V3',
       platforms = 'Linux, OS X, Windows',
       ext_modules = [module1])

