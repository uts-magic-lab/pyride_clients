/*
 *  PyRideRemotePyModule.cpp
 *  
 */

#include <openssl/sha.h>
#include "RemoteDataHandler.h"

using namespace pyride;
using namespace pyride_remote;

  /* Initialization function for the module (*must* be called initxx) */
#ifndef PyMODINIT_FUNC  /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

//PYRIDE_LOGGING_DECLARE( "pyride_remote.log" );
PYRIDE_NO_LOGGING;

PyDoc_STRVAR( PyRideRemote_doc, \
             "pyride_remote is a remote client extension module for PyRIDE." );
  
static PyRideRemoteDataHandler * s_datahandler = NULL;

/**
 *  Static methods in PyRideRemote extension module
 *
 */
static PyObject * PyModule_connect( PyObject * self, PyObject * args )
{
  char * ipaddr = NULL;
  char * authcode = NULL;
  
  if (s_datahandler->isConnected()) {
    PyErr_Format( PyExc_StandardError, "pyride_remote: we have already connect to a robot." );
    return NULL;
  }
  
  if (!PyArg_ParseTuple( args, "ss", &ipaddr, &authcode )) {
    // PyArg_ParseTuple will set the error status.
    return NULL;
  }

  unsigned char encauth[SHA256_DIGEST_LENGTH];
  memset( encauth, 0, SHA256_DIGEST_LENGTH );

  if (strlen( authcode ) == 44 && authcode[43] == '=') { // assume to be base64 encoded encrypted password
    size_t decodeLen = 0;
    unsigned char * decodedStr = ::decodeBase64( authcode, &decodeLen );
    if ((int)decodeLen == SHA256_DIGEST_LENGTH) {
      memcpy( encauth, decodedStr, SHA256_DIGEST_LENGTH );
    }
    free( decodedStr );
  }
  else {
    secureSHA256Hash( (unsigned char*)authcode, (int)strlen(authcode), encauth );
  }
  
  if (!ConsoleDataProcessor::instance()->logonToRobot( ipaddr, encauth )) {
    PyErr_Format( PyExc_ValueError, "pyride_remote: invalid IP or name for robot." );
    
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject * PyModule_IsConnected( PyObject *self )
{
  if (s_datahandler->isConnected()) {
    Py_RETURN_TRUE;
  }
  else {
    Py_RETURN_FALSE;
  }
}

static PyObject * PyModule_HasExclusiveControl( PyObject *self )
{
  if (s_datahandler->hasExclusiveControl()) {
    Py_RETURN_TRUE;
  }
  else {
    Py_RETURN_FALSE;
  }
}

static PyObject * PyModule_disconnect( PyObject *self )
{
  ConsoleDataProcessor::instance()->disconnectRobots();
  Py_RETURN_NONE;  
}

static PyObject * PyModule_LetTiNSpeak( PyObject *self, PyObject * args )
{
  char * tts = NULL;

  if (!s_datahandler->isConnected()) {
    PyErr_Format( PyExc_StandardError, "pyride_remote: we have not connected to a robot yet." );
    return NULL;
  }

  if (!PyArg_ParseTuple( args, "s", &tts )) {
    // PyArg_ParseTuple will set the error status.
    return NULL;
  }
  
  float volume = 1.0;
  int datalength = int(strlen( tts ) + sizeof( float ));
  unsigned char * data = new unsigned char[datalength];
  unsigned char * dataptr = data;
  
  memcpy( dataptr, &volume, sizeof( float ) ); dataptr += sizeof( float );
  memcpy( dataptr, tts, datalength - sizeof( float ) );
  
  ConsoleDataProcessor::instance()->issueExtendedCommand( s_datahandler->robotID(), SPEAK, data, datalength );
  delete [] data;

  Py_RETURN_NONE;
}

static PyObject * PyModule_EnableTelemery( PyObject *self )
{
  if (!s_datahandler->isConnected()) {
    PyErr_Format( PyExc_StandardError, "pyride_remote: we have not connected to a robot yet." );
    return NULL;
  }

  if (!s_datahandler->isTelemetryOn()) {
    ConsoleDataProcessor::instance()->startTelemetryStream( s_datahandler->robotID() );
  }
  Py_RETURN_NONE;
}

static PyObject * PyModule_DisableTelemery( PyObject *self )
{
  if (!s_datahandler->isConnected()) {
    PyErr_Format( PyExc_StandardError, "pyride_remote: we have not connected to a robot yet." );
    return NULL;
  }
  
  if (s_datahandler->isTelemetryOn()) {
    ConsoleDataProcessor::instance()->stopTelemetryStream();
  }
  Py_RETURN_NONE;
}

static PyObject * PyModule_TakeExclusiveControl( PyObject *self )
{
  if (!s_datahandler->isConnected()) {
    PyErr_Format( PyExc_StandardError, "pyride_remote: we have not connected to a robot yet." );
    return NULL;
  }
  
  if (!s_datahandler->canHaveExclusiveControl()) {
    PyErr_Format( PyExc_StandardError, "pyride_remote: we are not allowed to have the exclusive control to robot yet." );
    return NULL;
  }

  if (!s_datahandler->hasExclusiveControl()) {
    ConsoleDataProcessor::instance()->issueExtendedCommand( s_datahandler->robotID(), EXCLUSIVE_CTRL_REQUEST, NULL, 0 );
  }
  Py_RETURN_NONE;
}

static PyObject * PyModule_ReleaseExclusiveControl( PyObject *self )
{
  if (!s_datahandler->isConnected()) {
    PyErr_Format( PyExc_StandardError, "pyride_remote: we have not connected to a robot yet." );
    return NULL;
  }
  
  if (s_datahandler->hasExclusiveControl()) {
    ConsoleDataProcessor::instance()->issueExtendedCommand( s_datahandler->robotID(), EXCLUSIVE_CTRL_RELEASE, NULL, 0 );
  }
  Py_RETURN_NONE;
}

static PyObject * PyModule_IssueCustomCommand( PyObject *self, PyObject * args )
{
  int cmdID = -1;
  char * cmdtxt = NULL;
  
  if (!s_datahandler->isConnected()) {
    PyErr_Format( PyExc_StandardError, "pyride_remote: we have not connected to a robot yet." );
    return NULL;
  }
  
  if (!PyArg_ParseTuple( args, "is", &cmdID, &cmdtxt )) {
    // PyArg_ParseTuple will set the error status.
    return NULL;
  }

  if (cmdID < 25 || cmdID > 255) {
    PyErr_Format( PyExc_ValueError, "pyride_remote: invalid custom command ID. Must be within (25..255) range." );
    return NULL;
  }

  ConsoleDataProcessor::instance()->issueExtendedCommand( s_datahandler->robotID(), (PyRideExtendedCommand)cmdID, (unsigned char*)cmdtxt, int(strlen( cmdtxt )) );

  Py_RETURN_NONE;
}

static PyMethodDef PyModule_methods[] = {
  { "connect", (PyCFunction)PyModule_connect, METH_VARARGS,
    "Connect to a robot." },
  { "isconnected", (PyCFunction)PyModule_IsConnected, METH_NOARGS,
    "Check remote robot connection status." },
  { "tosay", (PyCFunction)PyModule_LetTiNSpeak, METH_VARARGS,
    "Make robot to speak." },
  { "enabletelemery", (PyCFunction)PyModule_EnableTelemery, METH_NOARGS,
    "Enable robot Telemetry streaming." },
  { "disabletelemery", (PyCFunction)PyModule_DisableTelemery, METH_NOARGS,
    "Disable robot Telemetry streaming." },
  { "takecontrol", (PyCFunction)PyModule_TakeExclusiveControl, METH_NOARGS,
    "Take exclusive control of the connected robot." },
  { "releasecontrol", (PyCFunction)PyModule_ReleaseExclusiveControl, METH_NOARGS,
    "Release exclusive control of the connected robot." },
  { "hascontrol", (PyCFunction)PyModule_HasExclusiveControl, METH_NOARGS,
    "Check whether we have exclusive control of robot." },
  { "issuecommand", (PyCFunction)PyModule_IssueCustomCommand, METH_VARARGS,
    "Send exclusive control command to the robot." },
  { "disconnect", (PyCFunction)PyModule_disconnect, METH_NOARGS,
    "Disconnect from a robot." },
  { NULL, NULL, 0, NULL }           /* sentinel */
};

static void finiModule()
{
  if (s_datahandler) {
    delete s_datahandler;
    s_datahandler = NULL;
  }
}

PyMODINIT_FUNC initpyride_remote( void )
{
  //PYRIDE_LOGGING_INIT;
  PyEval_InitThreads();
  PyObject * pyModule = Py_InitModule3( "pyride_remote", PyModule_methods, PyRideRemote_doc );

  if (pyModule == NULL)
    return;

  Py_INCREF( pyModule );
  s_datahandler = new PyRideRemoteDataHandler( pyModule );
  atexit( finiModule );
  //delete cmd
}
