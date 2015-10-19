#include "StdAfx.h"
#include "PyrideRemoteDataHandler.h"

using namespace System::Threading;
using namespace System;
using namespace System::Runtime::InteropServices;

namespace pyride_remote {

PyrideRemoteDataHandler::PyrideRemoteDataHandler(void) :
  robotID_( -1 )
{
  ConsoleDataProcessor::instance()->init( this );

  Thread ^ dataThread = gcnew Thread( gcnew ThreadStart( &PyrideRemoteDataHandler::DataThreadProc ) );
  dataThread->IsBackground = true;
  dataThread->Start();
}

PyrideRemoteDataHandler::~PyrideRemoteDataHandler(void)
{
  ConsoleDataProcessor::instance()->fini();
}

void PyrideRemoteDataHandler::onRobotCreated( const char cID, const int ipAddr, const RobotInfo * rinfo,
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

  //portalfrm_->setRobotInfo( robotID_, rinfo, vsettings, asettings, optLabel, optLabelLength );
}

void PyrideRemoteDataHandler::onRobotDestroyed( const char cID )
{
  if (robotID_ == -1) {
    //portalfrm_->updateGUI( false );
    return;
  }
  if (robotID_ != cID)
    return;

  robotID_ = -1;
  //portalfrm_->updateGUI( false );
  //lfrm_->updateGUI( true );
}

void PyrideRemoteDataHandler::DataThreadProc()
{
  ConsoleDataProcessor::instance()->processingData();
}

bool PyrideRemoteDataHandler::activeCamera( int camid )
{
  /*
  if (camid < 0 || camid >= (int)cameraLabels_.size())
    return false;
    */
  pendingCam_ = camid;
  ConsoleDataProcessor::instance()->switchCamera( robotID_, pendingCam_ );
  return true;
}

void PyrideRemoteDataHandler::onVideoStreamControl( bool isStart, const char cID )
{
  if (robotID_ != cID)
    return;

  //portalfrm_->updateVideoStreamStatus( isStart );
}

void PyrideRemoteDataHandler::onVideoStreamSwitch( const char cID, const VideoSettings * vsettings )
{
  if (robotID_ != cID)
    return;

  //portalfrm_->onVideoStreamSwitch( vsettings );
}

void PyrideRemoteDataHandler::onVideoDataInput( const unsigned char * data, const int dataSize )
{

}

void PyrideRemoteDataHandler::onOperationalData( const char cID, const int status, const unsigned char * optionalData,
                                  const int optionalDataLength )
{
  if (robotID_ != cID)
    return;

  switch (status) {
    case CUSTOM_STATE:
    {
      String ^ text = nullptr;

      if (optionalData && optionalDataLength > 0) {
        text = gcnew String( reinterpret_cast<const char*>(optionalData), 0, optionalDataLength );
        //portalfrm_->onTiNNotify( text );
      }
    }
    break;
    case EXCLUSIVE_CONTROL:
    case NORMAL_CONTROL:
    case EXCLUSIVE_CONTROL_OVERRIDE:
      //portalfrm_->onTiNCtrlStatusChange( status );
      break;
    default:
      break;
  }
}

void PyrideRemoteDataHandler::onExtendedCommandResponse( const char cID, const PyRideExtendedCommand command,
                                 const unsigned char * optionalData,
                                 const int optionalDataLength )
{
  if (robotID_ != cID)
    return;

  if (optionalDataLength != 1) {
    ERROR_MSG( "Invalid robot response\n" );
    return;
  }
  
  switch (command) {
    case EXCLUSIVE_CTRL_REQUEST:
      //hasExclusiveControl_ = (*optionalData == 1); //TODO: should handle reject correctly.
      break;
    case EXCLUSIVE_CTRL_RELEASE:
      //hasExclusiveControl_ = (*optionalData != 1);
    default:
      break;
  }
}

void PyrideRemoteDataHandler::onTelemetryData( const char cID, const RobotPose * pose, const FieldObject * objects,
  const int nofObjs )
{
}

void PyrideRemoteDataHandler::onTelemetryStreamControl( bool isStart )
{
}

}
