#ifdef WIN32
#else
#include <unistd.h>
#endif
#include "VideoStreamController.h"

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
  toDecode_( true ),
  grabWaitTime_( 100000 ), // default to 10FPS
#ifdef WIN32
  dataThread_( 0 ),
#else
  dataThread_( (pthread_t)NULL ),
#endif
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
  int rawDataSize = 0;
  bool dataSizeChanged = false;

  if (!dataStream_)
    return false;

  rawDataSize = dataStream_->grabData( &rawData, dataSizeChanged );

  data = rawData;
  data_size = rawDataSize;

  //DEBUG_MSG( "Got video data %d.\n", dataSize );
  if (data_size == 0)
    return false;

  if (decode) {
    jpeg_mem_src( &cinfo_, data, data_size );
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
    data = imageData_;
    data_size = imageDataSize_;
  }
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
      dataThread_ = 0;
#else
      pthread_join( dataThread_, NULL ); // allow thread to exit
      dataThread_ = (pthread_t)NULL;
#endif
    }
  }
}

void VideoStreamController::dataThreadProc()
{
  unsigned char * data = NULL;
  int dataSize = 0;
  while (isStreaming_) {
    if (grabVideoStreamData( data, dataSize, toDecode_ ) && delegate_) {
      delegate_->onVideoDataInput( data, dataSize );
    }
#ifdef WIN32
    Sleep( grabWaitTime_ * 1E3);
#else
    ::usleep( grabWaitTime_ );
#endif
  }
}

}
