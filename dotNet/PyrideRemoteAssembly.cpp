// This is the main DLL file.

#include "stdafx.h"

#include "PyrideRemoteAssembly.h"
#include <openssl\sha.h>

using namespace System::Runtime::InteropServices;

namespace Pyride
{
  PyrideRemote::PyrideRemote()
  {
    dataHandler_ = new PyrideRemoteDataHandler();
  }

  Boolean PyrideRemote::Connect( String ^ ipaddr, String ^ acode )
  {
    Boolean retVal = false;
    char * addr = (char *)Marshal::StringToHGlobalAnsi( ipaddr ).ToPointer();
    char * authcode = (char *)Marshal::StringToHGlobalAnsi( acode ).ToPointer();
    unsigned char encAuth[SHA256_DIGEST_LENGTH];
    memset( encAuth, 0, SHA256_DIGEST_LENGTH );

    if (strlen( authcode ) == 44 && authcode[43] == '=') { // assume to be base64 encoded encrypted password
      size_t decodeLen = 0;
      unsigned char * decodedStr = ::decodeBase64( authcode, &decodeLen );
      if ((int)decodeLen == SHA256_DIGEST_LENGTH) {
        memcpy( encAuth, decodedStr, SHA256_DIGEST_LENGTH );
      }
      Marshal::FreeHGlobal( IntPtr( (void*)decodedStr ) );
    }
    else {
      secureSHA256Hash( (unsigned char*)authcode, acode->Length, encAuth );
    }

    retVal = ConsoleDataProcessor::instance()->logonToRobot( addr, encAuth );

    Marshal::FreeHGlobal( IntPtr( (void*)authcode ) );
    Marshal::FreeHGlobal( IntPtr( (void*)addr ) );
    return retVal;
  }

  Void PyrideRemote::Disconnect()
  {

  }

  Void PyrideRemote::TakeControl()
  {

  }

  Void PyrideRemote::ReleaseControl()
  {

  }

  Boolean PyrideRemote::HasControl()
  {

    return false;
  }

  Boolean PyrideRemote::IsConnected()
  {

    return false;
  }

  Void PyrideRemote::IssueCommand( int cmd, String ^ argument )
  {

  }

  String ^ PyrideRemote::GetActiveCamera()
  {
    return L"";
  }

  array<String ^> ^ PyrideRemote::GetCameraList()
  {
    return nullptr;
  }

  Void PyrideRemote::EnableTelemetry()
  {

  }

  Void PyrideRemote::DisableTelemetry()
  {

  }

  PyrideRemote::~PyrideRemote()
  {
    delete dataHandler_;
  }

}