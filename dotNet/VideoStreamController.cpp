#include "StdAfx.h"
#include "PyRideCommon.h"
#include "VideoStreamController.h"
#include "RTPDataReceiver.h"

namespace pyride_remote {

using namespace System::Drawing;
using namespace System::Drawing::Imaging;
using namespace System::Threading;
using namespace System::Windows::Media::Imaging;

VideoStreamController::VideoStreamController( int fps, VideoStreamControllerDelegate ^ delegate )
{

  isStreaming_ = false;
  grabWaitTime_ = 1000; // default to 10FPS
  delegate_ = delegate;
  dataStream_ = new RTPDataReceiver();
  dataStream_->init( PYRIDE_VIDEO_STREAM_BASE_PORT, true );
  if (fps > 1 || fps <= 30) {
    grabWaitTime_ = 1000 / fps;
  }
}

VideoStreamController::~VideoStreamController()
{
  dataStream_->fini();
  isStreaming_ = false;
  delete dataStream_;
}

void VideoStreamController::setVideoSource( const char * host, const VideoSettings * vsettings )
{
  grabWaitTime_ = 1000 / vsettings->fps;
}

Stream ^ VideoStreamController::grabVideoStreamData()
{
  unsigned char * rawData = NULL;
  unsigned char * data = NULL;
  int dataSize = 0, rawDataSize = 0;
  bool dataSizeChanged = false;

  if (!dataStream_)
    return nullptr;

  rawDataSize = dataStream_->grabData( &rawData, dataSizeChanged );

  data = rawData;
  dataSize = rawDataSize;

  //DEBUG_MSG( "Got video data %d.\n", dataSize );
  if (dataSize == 0)
    return nullptr;
  
  UnmanagedMemoryStream ^ mystream = gcnew UnmanagedMemoryStream( data, dataSize );
  return mystream;
}

void VideoStreamController::processVideoStream( bool isStart )
{
  isStreaming_ = isStart;

  if (isStreaming_) {
      // start thread to grab.
    Thread ^ dataThread = gcnew Thread( gcnew ThreadStart( this, &VideoStreamController::dataThreadProc ) );
    dataThread->IsBackground = true;
    dataThread->Start();
  }
}

void VideoStreamController::dataThreadProc()
{
  while (isStreaming_) {
    Stream ^ data = grabVideoStreamData();
        
    if (data) {
      JpegBitmapDecoder^ decoder = nullptr;
      try {
        decoder = gcnew JpegBitmapDecoder(data, BitmapCreateOptions::PreservePixelFormat,
          BitmapCacheOption::Default);
      }
      catch (...) {
        data->Close();
        Sleep( grabWaitTime_ / 2 );
        continue;
      }
      BitmapSource^ bitmapSource = decoder->Frames[0];

      // Draw the Image
      Bitmap ^ bmp = gcnew Bitmap( bitmapSource->PixelWidth, bitmapSource->PixelHeight, PixelFormat::Format24bppRgb );
      BitmapData ^ bmpdata = bmp->LockBits( System::Drawing::Rectangle( Point::Empty, bmp->Size ),
          ImageLockMode::WriteOnly, PixelFormat::Format24bppRgb );
      bitmapSource->CopyPixels( System::Windows::Int32Rect::Empty, bmpdata->Scan0, bmpdata->Height * bmpdata->Stride, bmpdata->Stride );
      bmp->UnlockBits( bmpdata );
      if (delegate_) {
        delegate_->onVideoDataInput( bmp );
      }
      data->Close();
    }
    Sleep( grabWaitTime_ / 2 );
  }
}

}