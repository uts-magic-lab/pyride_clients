// PyrideRemoteAssembly.h

#pragma once
#include "PyrideRemoteDataHandler.h"

using namespace System;
using namespace pyride;
using namespace pyride_remote;

namespace Pyride {

  public ref class PyrideRemote
	{
  public:
    ~PyrideRemote();

    Boolean Connect( String ^ ipaddr, String ^ acode );
    Void Disconnect();
    Void TakeControl();
    Void ReleaseControl();
    Boolean HasControl();
    Boolean IsConnected();
    Void IssueCommand( int cmd, String ^ argument );
    String ^ GetActiveCamera();
    array<String ^> ^ GetCameraList();
    Void EnableTelemetry();
    Void DisableTelemetry();

    static property PyrideRemote ^ Instance {
      PyrideRemote ^ get() {
        if (instance_ == nullptr)
          instance_ = gcnew PyrideRemote();

        return instance_; 
      }
    };

  private:
    PyrideRemote();
    PyrideRemoteDataHandler * dataHandler_;


    static PyrideRemote ^ instance_;
	};
}
