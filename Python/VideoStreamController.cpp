#include "PyRideCommon.h"
#include "VideoStreamController.h"
#include "RTPDataReceiver.h"

namespace pyride_remote {

#ifdef WIN32
unsigned __stdcall video_thread( void * processor )
#else
void * video_thread( void * processor )
#endif
{
  ((VideoStreamController *)processor)->dataThreadProc();
  return NULL;
}


VideoStreamController::VideoStreamController() :
  isStreaming_( false ),
  grabWaitTime_( 100000 ), // default to 10FPS
  dataThread_( (pthread_t)NULL ),
  imageData_( NULL ),
  imageDataSize_( 0 ),
  delegate_( NULL )
{
  dataStream_ = new RTPDataReceiver();
  dataStream_->init( PYRIDE_VIDEO_STREAM_BASE_PORT, true );
}

VideoStreamController::~VideoStreamController()
{
  if (imageData_) {
    delete [] imageData_;
    jpeg_destroy_decompress( &cinfo_ );
  }
  dataStream_->fini();
  isStreaming_ = false;
  delete dataStream_;
}

void VideoStreamController::setDelegate( VideoStreamControllerDelegate * delegate )
{
  delegate_ = delegate;
}

void VideoStreamController::setVideoSource( const char * host, const VideoSettings * vsettings )
{
  grabWaitTime_ = 1000000 / vsettings->fps / 2.0;
  int newImageDataSize = kSupportedCameraQuality[(int)vsettings->resolution].width *
      kSupportedCameraQuality[(int)vsettings->resolution].height * 3;
  if (newImageDataSize != imageDataSize_) {
    if (imageData_) {
      delete [] imageData_;

      jpeg_destroy_decompress( &cinfo_ );

    }
    imageDataSize_ = newImageDataSize;
    imageData_ = new unsigned char[imageDataSize_];
    cinfo_.err = jpeg_std_error( &jerr_ );
    jpeg_create_decompress( &cinfo_ );
  }
}

bool VideoStreamController::grabVideoStreamData( unsigned char * & data, int & data_size, bool decode )
{
  unsigned char * rawData = NULL;
  unsigned char * data = NULL;
  int dataSize = 0, rawDataSize = 0;
  bool dataSizeChanged = false;

  if (!dataStream_)
    return false;

  rawDataSize = dataStream_->grabData( &rawData, dataSizeChanged );

  data = rawData;
  dataSize = rawDataSize;

  //DEBUG_MSG( "Got video data %d.\n", dataSize );
  if (dataSize == 0)
    return false;
  
  jpeg_mem_src( &cinfo_, data, dataSize );
  jpeg_read_header( &cinfo_, true );
  jpeg_start_decompress( &cinfo_ );
  int rowStride = cinfo_.output_width * cinfo_.output_components;

  if (imageDataSize_ != rowStride * cinfo_.output_height) {
    printf( "invalid input image format, skip!\n" );
    return false;
  }
  unsigned char *buffer_array[1];

  while (cinfo_.output_scanline < cinfo_.output_height) {
    buffer_array[0] = imageData_ + (cinfo_.output_scanline) * rowStride;

    jpeg_read_scanlines( &cinfo_, buffer_array, 1 );
  }

  jpeg_finish_decompress( &cinfo_ );
  return true;
}

void VideoStreamController::processVideoStream( bool isStart )
{
  isStreaming_ = isStart;

  if (isStreaming_) {
    if (!dataThread_) {
      // start thread to grab.
#ifdef WIN32
      dataThread_ = (HANDLE)_beginthreadex( NULL, 0, &video_thread, this, 0, NULL );
      if (dataThread_ == 0) {
        ERROR_MSG( "Unable to create thread to grab video stream.\n" );
        return;
      }
#else
      if (pthread_create( &dataThread_, NULL, video_thread, this ) ) {
        ERROR_MSG( "Unable to create thread to grab video stream.\n" );
        return;
      }
#endif
    }
  }
  else {
    if (dataThread_) {
#ifdef WIN32
      WaitForSingleObject( dataThread_, INFINITE );;
      CloseHandle( dataThread_ );
#else
      pthread_join( dataThread_, NULL ); // allow thread to exit
#endif
      dataThread_ = (pthread_t)NULL;
    }
  }
}

void VideoStreamController::dataThreadProc()
{
  unsigned char * data = NULL;
  int dataSize = 0;
  while (isStreaming_) {
    if (grabVideoStreamData( data, dataSize ) && delegate_) {
      delegate_->onVideoDataInput( data, dataSize );
    }
    ::usleep( grabWaitTime_ );
  }
}

}
