#pragma once
#include "RTPDataReceiver.h"

namespace pyride_remote {
 
using namespace System::IO;

ref class VideoStreamControllerDelegate abstract
{
public:
  virtual ~VideoStreamControllerDelegate() {}
  virtual void onVideoDataInput( System::Drawing::Bitmap ^ bitmapData ) = 0;
};

ref class VideoStreamController
{
public:
  VideoStreamController( int fps, VideoStreamControllerDelegate ^ delegate );
  void setVideoSource( const char * host, const VideoSettings * vsettings );
  void processVideoStream( bool isStart );
  bool isStreaming() { return isStreaming_; }

  void dataThreadProc();

  ~VideoStreamController();

private:
  bool isStreaming_;
  unsigned int grabWaitTime_;

  RTPDataReceiver * dataStream_;

  Stream ^ grabVideoStreamData();
  VideoStreamControllerDelegate ^ delegate_;
};

}
