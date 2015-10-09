# PyRIDE Remote Client Components (pyride_clients)
## Introduction
This repository contains client components that can be used to remotely access PyRIDE 'server' running on a PR2 or NAO robot. Details on PyRIDE can be found at https://github.com/uts-magic-lab/pyride_pr2. Currently, only Python extension module *pyride_remote* is released under this repository. One can use this module to build Python based client applications and services to remote control and monitor PR2 or NAO robots through PyRIDE middleware. This README file gives a quick introduction on how one may compile and use *pyride_remote* module.

## Compile source code
*pyride_remote* by default can be compiled and run on Windows, Linux and OS X platforms. It only requires standard Python 2.7.x installation and the standard C/C++ compiler toolchains for underlying platform, e.g. Visual Studio C/C++ compiler on Windows.


*pyride_remote* can retrieve real-time video image data feed from the robot cameras through PyRIDE 'server'. The functionality depends on several third-party libraries that requires some
### Prerequisites
