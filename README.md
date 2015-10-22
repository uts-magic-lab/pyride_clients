# PyRIDE Remote Client Components (pyride_clients)
## Introduction
This repository contains client components that can be used to remotely access PyRIDE 'server' running on a PR2 or NAO robot. Details on PyRIDE can be found at https://github.com/uts-magic-lab/pyride_pr2. Currently, only Python extension module *pyride_remote* is released under this repository. One can use this module to build Python based client applications and services to remote control and monitor PR2 or NAO robots through PyRIDE middleware. This README file gives a quick introduction on how one may compile and use *pyride_remote* module.

## Installation
### Prerequisites
*pyride_remote* by default can be compiled and run on Windows, Linux and OS X platforms. It only requires standard Python 2.7.x installation and the latest C/C++ compiler toolchains for the underlying platform and OpenSSL development library.

Additional third-party libraries (ccRtp and jpeg) are required, if you need *pyride_remote* to able to retrieve real-time video image data feed from the robot cameras through PyRIDE 'server'. **NOTE:** This feature is only fully tested on Ubuntu Linux.

**On Ubuntu Linux**

Install the following packages using ```apt-get``` if they are missing:
* python-dev
* libssl-dev
* libccrtp-dev (optional)
* libjpeg-dev or libjpeg-turbo8-dev (optional)

**On OS X**

Assuming ```brew``` is in use, you need to install ```openssl``` and ```jpeg``` formula and (optionally) compile and install ```ccRtp``` and ```commoncpp2``` GNU libraries.

**NOTE:** source code for ```ccRtp``` and ```commoncpp2``` can be downloaded [here](http://ftp.gnu.org/gnu/ccrtp/ccrtp-1.8.0.tar.gz) and [here](https://ftp.gnu.org/gnu/commoncpp/commoncpp2-1.8.1.tar.gz) respectively. Follow their INSTALL manual to compile the libraries. Make sure you compile and install ```commoncpp2``` library first.

**On Windows**
You need first install Microsoft VC++ for python in addition to Visual Studio. Since compiling the required third-party libraries is particular tricky on Windows. We have a precompiled binaries that you can use immediately. Simply download [this package](http://experimentdata.themagiclab.org/static/pyride_clients/pyride_client_thirdparty_windows.zip) and extract the package under under the repository.

### Compile source code
Open a terminal (on Windows platform use VS20XX command prompt) and change the working directory to ```Python``` subdirectory. Execute the following build command:

```
python pyride_remote_setup.py build
```
To install the extension library, run
```
python pyride_remote_setup.py install
```
You may require the admin privilege to install.

## Basic *pyride_remote* usages
### Import and connect to PyRIDE
```python
import pyride_remote
# access code is the password assigned to you
# instead of putting plain-text access code into the method call, you can use the 44 character base64/SHA256 based password digest to better security.
pyride_remote.connect( 'robot.ip.address', 'access code' )
# when the connection is successfully established, pyride_remote.onRobotConnected callback is invoked.
pyride_remote.is_connected() # return True, when it is connected.
pyride_remote.disconnect()
# When the client is disconnected from PyRIDE server, pyride_remote.onRobotDisconnected callback will be called.
```
### Issue custom command to PyRIDE
Before the client can issue commands to PyRIDE server, it must first to take the exclusive control of the PyRIDE. Only one client can take the control of the server at a time.
```python
pyride_remote.take_control()
pyride_remote.has_control() # return True, when the client has the exclusive control of the PyRIDE server.
pyride_remote.release_control()
# pyride_remote.onRobotControlStatus is called, when the client successfully takes or release the control.

# A custom command is consist of an integer command ID followed by a free form command string.
pyride_remote.issue_command( cmd_id, cmd_string )
# On the PyRIDE server side, onRemoteCommand callback will be executed with these two input arguments.
```
**NOTE::** PyRIDE can revoke the exclusive control of a remote client at anytime. When the control right is revoked, ```pyride_remote.onRobotControlOverride``` will be called on the client.

### Receive operational data from PyRIDE
*pyride_remote* can receive real-time operational data broadcast message from PyRIDE. Currently, operational data is the only mechanism for PyRIDE server to send/broadcast messages back to its remote clients. When the client receives a message from the server, ```pyride_remote.onRobotOperationData``` callback will be invoked.

```python
pyride_remote.enable_telemery() # to receive messages from PyRIDE server
pyride_remote.disable_telemery() # stop receiving messages from PyRIDE server
# pyride_remote.onRobotTelemetryStatus is called when the client starts or stops receiving messages from the server.
```

## TiN(Remote): a client utility tool
**TiN(Remote)** is a simple utility tool that has been developed to validate and demonstrate the capabilities of PyRIDE client server infrastructure. It is a remote client that can retrieve real-time video and audio data feeds from a PyRIDE server. TiN can issue simple text to the robot's Text-to-Speech system, allowing it speak on behalf of the client. Finally, TiN can remote control both head and body movement of the connected robot.

<img src="https://cloud.githubusercontent.com/assets/6646691/10659064/9251e944-78e8-11e5-88ef-3290278de13b.png" width="400">

**Figure 1. TiN Remote Login Windows.**

Again, you need a valid access code in order to connect to the PyRIDE server (See Figure 1). Once connected, you can click the ```play``` button in the middle of the screen to initiate video and audio streaming. If the robot is configured with multiple cameras, you will be able to see  ```< >``` buttons that allow you to switch between the cameras. While video is streaming, you can control the robot head movement by double clicking the image area.

<img src="https://cloud.githubusercontent.com/assets/6646691/10659068/96dc9cac-78e8-11e5-9d12-3e2a446400cd.png"
width="500">

**Figure 2. TiN Main Console.**

Type any text into the textbox and press ```RETURN``` key will send the text to the robot to speak. Click the ```Body Motion``` checkbox will bring up a set of arrow buttons that allow you to move the robot forward, backward, turn left and right. **NOTE** Clicking the ```Body Motion``` checkbox will take the exclusive control of the robot. Other connected remote client will not be able to perform any task on the robot except the text to speech and head movement. PyRIDE server can override you exclusive control anytime. When your client lose or unable to gain exclusive control of the robot (because another client has taken the control), the ```Body Motion``` checkbox will turn to grey.

### Client binary download and installation
Click the following link to download TiN(Remote) for your platform.

* [Windows x64 Installer](http://experimentdata.themagiclab.org/static/pyride_clients/TiNSetup.exe) Double click the TiNSetup.exe.
* [OS X 10.11 Binary](http://experimentdata.themagiclab.org/static/pyride_clients/TiN_OSX.zip). Unzip ```TiN_OSX.zip``` and move the app into ```Application``` folder.
* [Linux Ubuntu 12.04 Debian package](http://experimentdata.themagiclab.org/static/pyride_clients/TiN_Ubuntu_12.04.deb). Run ```sudo dpkg -i TiN_Ubuntu_12.04.debian```.
