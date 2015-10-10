/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * pyride_remote
 * Copyright (C) Xun Wang 2013, 2015 <wang.xun@gmail.com>
 *
 */

#ifndef _REMOTE_DATA_HANDLER_H_
#define _REMOTE_DATA_HANDLER_H_

#include <Python.h>
#include <vector>
#ifdef WIN32
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <string>
#endif

#include "ConsoleDataProcessor.h"

#ifdef WITH_VIDEO_DATA
#include "VideoStreamController.h"
#endif

namespace pyride_remote {

class PyRideRemoteDataHandler : public PyRideConsoleCommandHandler
#ifdef WITH_VIDEO_DATA
, VideoStreamControllerDelegate
#endif
{
public:
  PyRideRemoteDataHandler( PyObject * pyModule );
  virtual ~PyRideRemoteDataHandler();

  bool isConnected() const { return (robotID_ != -1); }
  bool isTelemetryOn() const { return isTelemetryOn_; }
  bool hasExclusiveControl() const { return hasExclusiveControl_; }
  bool canHaveExclusiveControl() const { return canHaveExclusiveControl_; }

  int robotID() const { return robotID_; }

  int getCameraList( std::vector<std::string> & camlabels );
  int activeCamera() const { return activeCam_; }
  bool activeCamera( int camid );

#ifdef WITH_VIDEO_DATA
  void registerForImageData( PyObject * callback, bool decodeImage = true );
#endif

protected:
  void onRobotCreated( const char cID, const int ipAddr, const RobotInfo * rinfo,
                            const VideoSettings * vsettings, const AudioSettings * asettings,
                            const unsigned char * optLabel,
                            const int optLabelLength );

  void onRobotDestroyed( const char cID );
  void onTelemetryData( const char cID, const RobotPose * pose, const FieldObject * objects = NULL,
                                const int nofObjs = 0 );
  void onTelemetryStreamControl( bool isStart );
  void onVideoStreamControl( bool isStart, const char cID );
  void onVideoStreamSwitch( const char cID, const VideoSettings * vsettings );
  void onOperationalData( const char cID, const int status,
                                  const unsigned char * optionalData = NULL,
                                  const int optionalDataLength = 0 );
  void onExtendedCommandResponse( const char cID, const PyRideExtendedCommand command,
                                 const unsigned char * optionalData = NULL,
                                 const int optionalDataLength = 0 );

#ifdef WITH_VIDEO_DATA
  void onVideoDataInput( const unsigned char * data, const int dataSize );
#endif

private:
#ifdef WIN32
  HANDLE runThread_;
#else
  pthread_t runThread_;
#endif
  int robotID_;
  int activeCam_;
  int pendingCam_;
  std::vector<std::string> cameraLabels_;
  bool isTelemetryOn_;
  bool hasExclusiveControl_;
  bool canHaveExclusiveControl_;
  bool finalShutdown_;

  PyObject * pPyModule_;

#ifdef WITH_VIDEO_DATA
  VideoStreamController vsc_;
  PyObject * imageDataCB_;
#endif

  void invokeCallback( const char * fnName, PyObject * arg );
  void invokeCallback( PyObject * & cbObj, PyObject * arg );
};

} // namespace pyride_remote
#endif // _REMOTE_DATA_HANDLER_H_
