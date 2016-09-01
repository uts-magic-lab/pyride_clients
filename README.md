# PyRIDE Remote Client Components (pyride_clients)
## pyride_remote: a Python extension module
This repository contains client components that can be used to remotely access PyRIDE 'server' running on a PR2 or NAO robot. Details on PyRIDE can be found at https://github.com/uts-magic-lab/pyride_pr2. Currently, only Python extension module *pyride_remote* is released under this repository. One can use this module to build Python based client applications and services to remote control and monitor PR2 or NAO robots through PyRIDE middleware. This README file gives a quick introduction on how one may compile and use *pyride_remote* module.

### Installation
#### Prerequisites
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

To enable real-time image streamming, set WITH_VIDEO_DATA environment variable before compiling the Python extension module. For example,

```
export WITH_VIDEO_DATA = 1
```

### Basic usages
#### Import and connect to PyRIDE
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

**NOTE:** PyRIDE can revoke the exclusive control of a remote client at anytime. When the control right is revoked, ```pyride_remote.onRobotControlOverride``` will be called on the client.

### Receive operational data from PyRIDE
*pyride_remote* can receive real-time operational data broadcast message from PyRIDE. Currently, operational data is the only mechanism for PyRIDE server to send/broadcast messages back to its remote clients. When the client receives a message from the server, ```pyride_remote.onRobotOperationData``` callback will be invoked.

```python
pyride_remote.enable_telemery() # to receive messages from PyRIDE server
pyride_remote.disable_telemery() # stop receiving messages from PyRIDE server
# pyride_remote.onRobotTelemetryStatus is called when the client starts or stops receiving messages from the server.
```

### Receive real-time image data from PyRIDE (for OpenCV image processing)
Real-time image data is provided through callback mechanism. You need to define a callback function that takes a raw image data byte array. *pyride_remote* does not receive live image data by default, you need to enable the service by providing the callback function to ```pyride_remote.register_image_data``` method call.

```python
import cv2 as cv
import numpy as np
import pyride_remote as pr

cv.startWindowThread()
cv.namedWindow( 'preview' )

def imagedata( data ):
  img = np.array( data ).reshape( 480, 640, 3 )
  cv.imshow( 'preview', img )

pr.connect( 'robot.ip.address', 'access code' )
pr.register_image_data( imagedata, True ) #the second parameter ensures the received (jpeg) image data is decoded to raw pixel data.

#stop receive image data from PyRIDE server with the following
#pr.register_image_data( None )
```

## TiNRemote: a client utility
**TiNRemote** is a utility tool that has been developed to validate and demonstrate the capabilities of PyRIDE client server infrastructure. It is a simple remote client that can retrieve real-time video and audio data feeds from a PyRIDE robot server. TiNRemote can issue text messages to the robot's Text-to-Speech system, allowing the robot to speak on behalf of the client. Finally, TiNRemote can remote control both head and body movement of the connected robot. TiNRemote is available on OS X, Windows and Linux. There is also an iOS version of TinRemote available in [AppStore](https://itunes.apple.com/au/app/tinremote/id1057118291?mt=8) with slightly different functionalities. Its user instructions is [here](#ios).

<img src="https://cloud.githubusercontent.com/assets/6646691/10659064/9251e944-78e8-11e5-88ef-3290278de13b.png" width="400">

**Figure 1. TiNRemote Login.**

Again, you need a valid access code in order to connect to the PyRIDE server (See Figure 1). Once connected, you can click the ```play``` button in the middle of the screen to initiate video and audio streaming. If the robot is configured with multiple cameras, you will be able to see  ```< >``` buttons that allow you to switch between the cameras. While video is streaming, you can control the robot head movement by double clicking the image area.

<img src="https://cloud.githubusercontent.com/assets/6646691/10659068/96dc9cac-78e8-11e5-9d12-3e2a446400cd.png"
width="500">

**Figure 2. TiNRemote Main Console.**

Type any text into the textbox and press ```RETURN``` key will send the text to the robot to speak. Click the ```Body Motion``` checkbox will bring up a set of arrow buttons that allow you to move the robot forward, backward, turn left and right. **NOTE** Clicking the ```Body Motion``` checkbox will take the exclusive control of the robot. Other connected remote client will not be able to perform any task on the robot except the text to speech and head movement. PyRIDE server can override you exclusive control anytime. When your client lose or unable to gain exclusive control of the robot (because another client has taken the control), the ```Body Motion``` checkbox will turn to grey.

### Client binary download and installation
Click the following link to download TiNRemote for your platform.

* [Windows x64 Installer](http://experimentdata.themagiclab.org/static/pyride_clients/TiNRemote_Windows_Setup.exe) Double click the TiNRemote_Windows_Setup.exe. Require [Microsoft SQL Server Compact](https://www.microsoft.com/en-us/download/details.aspx?id=17876) installed.
* [OS X AppStore](https://itunes.apple.com/au/app/tinremote/id1050361034?mt=12).
* [Linux Ubuntu 12.04 Debian package](http://experimentdata.themagiclab.org/static/pyride_clients/TiNRemote_Ubuntu_12.04.deb). Run ```sudo dpkg -i TiNRemote_Ubuntu_12.04.debian```.

### <a name="ios"></a>TiNRemote for iOS
TiNRemote for iOS has slightly different functionalities compared with TiNRemote for PCs. Once you start the app, tap the **TiN** icon to bring up the login dialogue box shown in Figure 3.

<img src="https://cloud.githubusercontent.com/assets/6646691/13273581/a0ed258c-daf8-11e5-84b7-03984ee75f42.png"
width="250">

**Figure 3. TiNRemote for iOS Login.**

Enter the IP address of the robot and a valid access code. **NOTE:** If you have already successfully used TiNRemote for iOS to access a number of robot PyRIDE servers, the app will bring up a list of robot PyRIDE servers that have been used previously.

Once you have successfully logged into the server, the main console is shown as in Figure 4.

<img src="https://cloud.githubusercontent.com/assets/6646691/13275579/b1fd1604-db0a-11e5-859a-137ebe8b5794.png"
width="600">

**Figure 4. TiNRemote for iOS main screen.**

Tap play button to start or pause real-time image streaming from (one of) robot camera(s). If the robot is equipped with multiple cameras, tap next or previous camera button to switch streaming between cameras. Tap the Text-to-Speech button to bring up a text field windows for entering text to the robot. The top middle icon indicates whether the app client can gain exclusive control of the robot. Its disappearance means you cannot remotely control the robot. Tap exit button to log out of the robot PyRIDE server.

#### Remote robot control
If the exclusive control indicator is present, you can remotely drive the robot. Put three fingers along the middle of the screen and long press for three seconds, a circular control display will appear (Figure 5):

<img src="https://cloud.githubusercontent.com/assets/6646691/13275806/fd5a0218-db0c-11e5-9772-0a3057301184.png"
width="600">

**Figure 5. TiNRemote for iOS remote control drive display.**

Three fingers long press for three seconds again will disable remote control drive. While the remote control drive is enabled, one finger press and move of the inner circle, similar to moving a joystick will drive the robot to the desired direction and speed. Press left/right turn button to turn the robot left or right respectively.

While real-time images are streaming from a robot head camera, you can move the robot head by double tapping the point on the video display where you want robot head point to. **DONOT** double tap again until the robot head movement is completed.
