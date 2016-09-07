/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * pyride_remote
 * Copyright (C) Xun Wang 2013,2015 <wang.xun@gmail.com>
 *
 */

#include "RemoteDataHandler.h"

namespace pyride_remote {

#ifdef WIN32
unsigned __stdcall data_thread(void * proc)
#else
void * data_thread( void * proc )
#endif
{
  ConsoleDataProcessor::instance()->processingData();
  return NULL;
}

PyRideRemoteDataHandler::PyRideRemoteDataHandler( PyObject * pyModule ) :
#ifdef WIN32
  runThread_( 0 ),
#else
  runThread_( (pthread_t)NULL ),
#endif
  robotID_( -1 ),
  activeCam_( -1 ),
  pendingCam_( -1 ),
  isTelemetryOn_( false ),
  hasExclusiveControl_( false ),
  canHaveExclusiveControl_( false ),
  finalShutdown_( false ),
  pPyModule_( pyModule )
#ifdef WITH_VIDEO_DATA
  ,imageDataCB_( NULL )
#endif
{
#ifdef WITH_VIDEO_DATA
  vsc_.setDelegate( this );
#endif
  ConsoleDataProcessor::instance()->init( this );
#ifdef WIN32
  //_beginthread( data_thread, 0, NULL );
  runThread_ = (HANDLE)_beginthreadex( NULL, 0, &data_thread, this, 0, NULL );
#else
  if (pthread_create( &runThread_, NULL, data_thread, this ) ) {
    ERROR_MSG( "Unable to create data thread for console data processor!\n" );
    return;
  }
#endif
}

PyRideRemoteDataHandler::~PyRideRemoteDataHandler()
{
  finalShutdown_ = true;
  ConsoleDataProcessor::instance()->fini();
#ifdef WIN32
  WaitForSingleObject(runThread_, 20);
  CloseHandle(runThread_);
  runThread_ = 0;
#else
  pthread_join(runThread_, NULL); // allow thread to exit
  runThread_ = (pthread_t)NULL;
#endif
}

void PyRideRemoteDataHandler::onRobotCreated( const char cID, const int ipAddr, const RobotInfo * rinfo,
                            const VideoSettings * vsettings, const AudioSettings * asettings,
                            const unsigned char * optLabel,
                            const int optLabelLength )
{
  if (robotID_ != -1)
    return;

  robotID_ = cID;

  if (vsettings->format != RGB) {
    ConsoleDataProcessor::instance()->setImageFormat( robotID_, RGB );
  }

  canHaveExclusiveControl_ = (rinfo->status == NORMAL_CONTROL);

  cameraLabels_.clear();
  activeCam_ = -1;
  pendingCam_ = -1;

  if (rinfo->nofcams > 0) {
    unsigned char * dataPtr = (unsigned char *)optLabel;
    for (int i = 0; i < rinfo->nofcams; ++i) {
      int length = (int)*dataPtr & 0xff; dataPtr++;
      cameraLabels_.push_back( std::string( (char *)dataPtr, length ) );
      dataPtr += length;
    }
    activeCam_ = 0;
#ifdef WITH_VIDEO_DATA
    vsc_.setVideoSource( NULL, vsettings );
#endif
  }

  if (finalShutdown_)
    return;

  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  PyObject * arg = Py_BuildValue( "(i)", rinfo->type );

  this->invokeCallback( "onRobotConnected", arg );

  Py_DECREF( arg );

  PyGILState_Release( gstate );
}

void PyRideRemoteDataHandler::onRobotDestroyed( const char cID )
{
  if (robotID_ == -1) {
    return;
  }
  if (robotID_ != cID)
    return;

  robotID_ = -1;

#ifdef WITH_VIDEO_DATA
  if (imageDataCB_) {
    vsc_.processVideoStream( false );
    Py_DECREF( imageDataCB_ );
    imageDataCB_ = NULL;
  }
#endif

  cameraLabels_.clear();
  activeCam_ = -1;

  if (finalShutdown_)
    return;

  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  this->invokeCallback( "onRobotDisconnected", NULL );

  PyGILState_Release( gstate );
}

void PyRideRemoteDataHandler::onTelemetryData( const char cID, const RobotPose * pose, const FieldObject * objects,
                                const int nofObjs )
{
}

void PyRideRemoteDataHandler::onTelemetryStreamControl( bool isStart )
{
  isTelemetryOn_ = isStart;

  if (finalShutdown_)
    return;

  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  PyObject * arg = Py_BuildValue( "(O)", isStart ? Py_True : Py_False );

  this->invokeCallback( "onRobotTelemetryStatus", arg );

  Py_DECREF( arg );

  PyGILState_Release( gstate );
}

void PyRideRemoteDataHandler::onVideoStreamControl( bool isStart, const char cID )
{
  if (cID != robotID_)
    return;

#ifdef WITH_VIDEO_DATA
  vsc_.processVideoStream( isStart );
#endif
}

void PyRideRemoteDataHandler::onVideoStreamSwitch( const char cID, const VideoSettings * vsettings )
{
  if (cID != robotID_)
    return;

  if (pendingCam_ >= 0) {
#ifdef WITH_VIDEO_DATA
    vsc_.setVideoSource( NULL, vsettings );
#endif
    if (!finalShutdown_) {
      PyGILState_STATE gstate;
      gstate = PyGILState_Ensure();

      PyObject * arg = Py_BuildValue( "(ii)", pendingCam_, activeCam_ );

      this->invokeCallback( "onRobotCameraSwitch", arg );

      Py_DECREF( arg );

      PyGILState_Release( gstate );
    }
    activeCam_ = pendingCam_;
    pendingCam_ = -1;
  }
}

void PyRideRemoteDataHandler::onOperationalData( const char cID, const int status,
                                  const unsigned char * optionalData,
                                  const int optionalDataLength )
{
  if (robotID_ != cID)
    return;

  switch (status) {
    case CUSTOM_STATE:
    {
      if (optionalData && optionalDataLength > 0) {
        unsigned char * data = new unsigned char[optionalDataLength+1];
        memcpy( data, optionalData, optionalDataLength );
        data[optionalDataLength] = '\0';

        if (!finalShutdown_) {
          PyGILState_STATE gstate;
          gstate = PyGILState_Ensure();

          PyObject * arg = Py_BuildValue( "(s)", data );

          this->invokeCallback( "onRobotOperationData", arg );

          Py_DECREF( arg );

          PyGILState_Release( gstate );
        }
        delete [] data;
      }
    }
      break;
    case EXCLUSIVE_CONTROL:
    case NORMAL_CONTROL:
      if (hasExclusiveControl_)
        return;

      canHaveExclusiveControl_ = (status == NORMAL_CONTROL);
      break;
    case EXCLUSIVE_CONTROL_OVERRIDE:
    {
      hasExclusiveControl_ = canHaveExclusiveControl_ = false;

      if (!finalShutdown_) {
        PyGILState_STATE gstate;
        gstate = PyGILState_Ensure();

        this->invokeCallback( "onRobotControlOverride", NULL );

        PyGILState_Release( gstate );
      }
    }
      break;
    default:
      break;
  }
}

void PyRideRemoteDataHandler::onExtendedCommandResponse( const char cID, const PyRideExtendedCommand command,
                                 const unsigned char * optionalData,
                                 const int optionalDataLength )
{
  if (robotID_ != cID)
    return;

  if (optionalDataLength != 1) {
    return;
  }

  bool hascontrol = hasExclusiveControl_;

  switch (command) {
    case EXCLUSIVE_CTRL_REQUEST:
      hascontrol = (*optionalData == 1); //TODO: should handle reject correctly.
      break;
    case EXCLUSIVE_CTRL_RELEASE:
      hascontrol = (*optionalData != 1);
      break;
    default:
      break;
  }

  if (hascontrol != hasExclusiveControl_) {
    hasExclusiveControl_ = hascontrol;

    if (!finalShutdown_) {
      PyGILState_STATE gstate;
      gstate = PyGILState_Ensure();

      PyObject * arg = Py_BuildValue( "(O)", hasExclusiveControl_ ? Py_True : Py_False );

      this->invokeCallback( "onRobotControlStatus", arg );

      Py_DECREF( arg );

      PyGILState_Release( gstate );
    }
  }
}

int PyRideRemoteDataHandler::getCameraList( std::vector<std::string> & camlabels )
{
  camlabels = cameraLabels_;
  return (int)camlabels.size();
}

bool PyRideRemoteDataHandler::activeCamera( int camid )
{
  if (camid < 0 || camid >= (int)cameraLabels_.size())
    return false;

  pendingCam_ = camid;
  ConsoleDataProcessor::instance()->switchCamera( robotID_, pendingCam_ );
  return true;
}

#ifdef WITH_VIDEO_DATA
void PyRideRemoteDataHandler::registerForImageData( PyObject * callback, bool decodeImage )
{
  if (callback) {
    Py_INCREF( callback );
    if (imageDataCB_) {
      Py_DECREF( imageDataCB_ );
    }
    else { // start streaming
      ConsoleDataProcessor::instance()->startCameraImageStream( robotID_ );
    }
    imageDataCB_ = callback;
    vsc_.decodeImage( decodeImage );
  }
  else if (imageDataCB_) { // stop streaming
    Py_DECREF( imageDataCB_ );
    imageDataCB_ = NULL;
    ConsoleDataProcessor::instance()->stopCameraImageStream( robotID_ );
  }
}

void PyRideRemoteDataHandler::onVideoDataInput( const unsigned char * data, const int dataSize )
{
  if (imageDataCB_ && !finalShutdown_) {
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject * barray = PyByteArray_FromStringAndSize( (char*)data, (Py_ssize_t)dataSize );
    PyObject * arg = Py_BuildValue( "(O)", barray );

    this->invokeCallback( imageDataCB_, arg );

    Py_DECREF( arg );
    Py_DECREF( barray );

    PyGILState_Release( gstate );
  }
}
#endif

void PyRideRemoteDataHandler::invokeCallback( const char * fnName, PyObject * arg )
{
  if (!pPyModule_)
    return;

  //DEBUG_MSG( "Attempt get callback function %s\n", fnName );

  PyObject * callbackFn = PyObject_GetAttrString( pPyModule_, const_cast<char *>(fnName) );
  if (!callbackFn) {
    PyErr_Clear();
    return;
  }
  else if (!PyCallable_Check( callbackFn )) {
    PyErr_Format( PyExc_TypeError, "%s is not callable object", fnName );
  }
  else {
    PyObject * pResult = PyObject_CallObject( callbackFn, arg );
    if (PyErr_Occurred()) {
      PyErr_Print();
    }
    Py_XDECREF( pResult );
  }
  Py_DECREF( callbackFn );
}

void PyRideRemoteDataHandler::invokeCallback( PyObject * & cbObj, PyObject * arg )
{
  if (cbObj) {
    PyObject * pResult = PyObject_CallObject( cbObj, arg );
    if (PyErr_Occurred()) {
      PyErr_Print();
    }
    Py_XDECREF( pResult );
  }
}

} // namespace pyride_remote
