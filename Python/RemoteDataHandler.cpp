/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * pyride_remote
 * Copyright (C) Xun Wang 2013,2015 <wang.xun@gmail.com>
 * 
 */

#include "RemoteDataHandler.h"

namespace pyride_remote {

#ifdef WIN32
void data_thread( void * proc )
#else
void * data_thread( void * proc )
#endif
{
  ConsoleDataProcessor::instance()->processingData();
#ifndef WIN32
  return NULL;
#endif
}

PyRideRemoteDataHandler::PyRideRemoteDataHandler( PyObject * pyModule ) :
#ifndef WIN32
  runThread_( (pthread_t)NULL ),
#endif
  robotID_( -1 ),
  isTelemetryOn_( false ),
  hasExclusiveControl_( false ),
  canHaveExclusiveControl_( false ),
  finalShutdown_( false ),
  pPyModule_( pyModule )
{
  ConsoleDataProcessor::instance()->init( this );
#ifdef WIN32
  _beginthread( data_thread, 0, NULL );
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
  
  if (finalShutdown_)
    return;

  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  
  PyObject * arg = Py_BuildValue( "(i)", rinfo->type );

  this->invokeCallback( "onTiNLogon", arg );
  
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
  
  if (finalShutdown_)
    return;
  
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  
  this->invokeCallback( "onTiNLogoff", NULL );
  
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
  
  this->invokeCallback( "onTiNTelemetryStatus", arg );
  
  Py_DECREF( arg );
  
  PyGILState_Release( gstate );
}

void PyRideRemoteDataHandler::onVideoStreamControl( bool isStart, const char cID )
{
  if (cID != robotID_)
    return;

}

void PyRideRemoteDataHandler::onVideoStreamSwitch( const char cID, const VideoSettings * vsettings )
{
}

void PyRideRemoteDataHandler::onOperationalData( const char cID, const int status,
                                  const unsigned char * optionalData,
                                  const int optionalDataLength )
{
  if (robotID_ != cID)
    return;

  switch (status)
  {
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

          this->invokeCallback( "onTiNOperationData", arg );

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
      
        this->invokeCallback( "onTiNControlOverride", NULL );
      
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
      
      this->invokeCallback( "onTiNControlStatus", arg );
      
      Py_DECREF( arg );
      
      PyGILState_Release( gstate );
    }
  }
}
  
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

} // namespace pyride_remote
