#pragma once
#include "ConsoleDataProcessor.h"

using namespace pyride;

namespace pyride_remote {

class PyrideRemoteDataHandler : public PyRideConsoleCommandHandler
{
public:
  PyrideRemoteDataHandler(void);
  virtual ~PyrideRemoteDataHandler(void);

  static void DataThreadProc();

  bool isConnected() const { return (robotID_ != -1); }
  bool isTelemetryOn() const { return isTelemetryOn_; }
  bool hasExclusiveControl() const { return hasExclusiveControl_; }
  bool canHaveExclusiveControl() const { return canHaveExclusiveControl_; }

  int robotID() const { return robotID_; }

  //int getCameraList( std::vector<std::string> & camlabels );
  int activeCamera() const { return activeCam_; }
  bool activeCamera( int camid );

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

  void onVideoDataInput( const unsigned char * data, const int dataSize );

private:
  HANDLE runThread_;

  int robotID_;
  int activeCam_;
  int pendingCam_;
  //std::vector<std::string> cameraLabels_;
  bool isTelemetryOn_;
  bool hasExclusiveControl_;
  bool canHaveExclusiveControl_;
};

}