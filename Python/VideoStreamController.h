#pragma once
#include <pthread.h>
#include "RTPDataReceiver.h"

namespace pyride_remote {

class VideoStreamControllerDelegate
{
public:
  virtual void onVideoDataInput( const unsigned char * data, const int data_size ) = 0;
};

class VideoStreamController
{
public:
  VideoStreamController();
  ~VideoStreamController();

  void setVideoSource( const char * host, const VideoSettings * vsettings );
  void setDelegate( VideoStreamControllerDelegate * delegate );

  void processVideoStream( bool isStart );
  bool isStreaming() { return isStreaming_; }

  void dataThreadProc();

private:
  bool isStreaming_;
  unsigned int grabWaitTime_;

#ifdef WIN32
  HANDLE dataThread_;
#else
  pthread_t dataThread_;
#endif

  RTPDataReceiver * dataStream_;

  // image data
  unsigned char * imageData_;
  int imageDataSize_;
  struct jpeg_decompress_struct cinfo_;
  struct jpeg_error_mgr jerr_;

  VideoStreamControllerDelegate * delegate_;

  bool grabVideoStreamData( unsigned char * & data, int & data_size, bool decode = true );
};

}
