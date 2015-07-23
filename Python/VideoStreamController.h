#pragma once
#include <pthread.h>
#include <stdio.h>
#include <jpeglib.h>
#include "RTPDataReceiver.h"
#include "PyRideCommon.h"

namespace pyride_remote {

class VideoStreamController;

class VideoStreamControllerDelegate
{
public:
  virtual ~VideoStreamControllerDelegate() {}
  virtual void onVideoDataInput( const unsigned char * data, const int dataSize ) = 0;

  friend class VideoStreamController;
};

class VideoStreamController
{
public:
  VideoStreamController();
  ~VideoStreamController();

  void setVideoSource( const char * host, const VideoSettings * vsettings );
  void setDelegate( VideoStreamControllerDelegate * delegate );
  void decodeImage( bool decode ) { toDecode_ = decode; }

  void processVideoStream( bool isStart );
  bool isStreaming() { return isStreaming_; }

  void dataThreadProc();

private:
  bool isStreaming_;
  bool toDecode_;
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
