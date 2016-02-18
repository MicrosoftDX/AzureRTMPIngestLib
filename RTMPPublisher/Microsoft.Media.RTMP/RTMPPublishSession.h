/****************************************************************************************************************************

RTMP Live Publishing Library

Copyright (c) Microsoft Corporation

All rights reserved.

MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
files (the ""Software""), to deal in the Software without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


*****************************************************************************************************************************/

#pragma once

#include <windows.foundation.h>
#include <windows.media.mediaproperties.h>
#include <windows.media.h>
#include <wrl.h>
#include <memory>
#include <vector>
#include "PublishProfile.h"
#include "RTMPPublisherSink.h"
#include "EventArgs.h"
#include "RTMPMessenger.h"
#include <atomic>

using namespace Microsoft::WRL;
using namespace Windows::Media::MediaProperties;
using namespace std;
using namespace Windows::Media;
using namespace Windows::Foundation;
using namespace Platform;

namespace Microsoft
{
  namespace Media
  {
    namespace RTMP
    {

      ref class RTMPPublishSession;

      public delegate void PublishFailedHandler(RTMPPublishSession^ sender, FailureEventArgs^ args);
      public delegate void SessionClosedHandler(RTMPPublishSession^ sender, SessionClosedEventArgs^ args);

      [Windows::Foundation::Metadata::Threading(Windows::Foundation::Metadata::ThreadingModel::Both)]
      [Windows::Foundation::Metadata::MarshalingBehavior(Windows::Foundation::Metadata::MarshalingType::Agile)]
      public ref class RTMPPublishSession sealed
      {

        friend class RTMPPublisherSink;

      public:
 
        RTMPPublishSession(Windows::Foundation::Collections::IVector<PublishProfile^>^ targetProfiles);

        RTMPPublishSession(Windows::Foundation::Collections::IVector<PublishProfile^>^ targetProfiles, 
         VideoEncodingProperties^ interimVideoProfile, AudioEncodingProperties^ interimAudioProfile);



        Windows::Foundation::IAsyncOperation<IMediaExtension^>^ GetCaptureSinkAsync();


        event PublishFailedHandler^ PublishFailed
        {
          Windows::Foundation::EventRegistrationToken add(PublishFailedHandler^ handler)
          {

            ++_subcount_publishFailed;
            return _publishFailed += handler;
          }

          void remove(Windows::Foundation::EventRegistrationToken token)
          {
            --_subcount_publishFailed;
            _publishFailed -= token;
          }

          void raise(RTMPPublishSession^ sender, FailureEventArgs^ args)
          {
            return _publishFailed(sender, args);
          }
        }

        event SessionClosedHandler^ SessionClosed
        {
          Windows::Foundation::EventRegistrationToken add(SessionClosedHandler^ handler)
          {

            ++_subcount_sessionClosed;
            return _sessionClosed += handler;
          }

          void remove(Windows::Foundation::EventRegistrationToken token)
          {
            --_subcount_sessionClosed;
            _sessionClosed -= token;
          }

          void raise(RTMPPublishSession^ sender, SessionClosedEventArgs^ args)
          {
            return _sessionClosed(sender, args);
          }
        }

        property Windows::Foundation::Collections::IVectorView<PublishProfile^>^ Parameters
        {
          Windows::Foundation::Collections::IVectorView<PublishProfile^>^ get()
          {
            return ref new Platform::Collections::VectorView<PublishProfile^>(begin(_params),end(_params));
          }
        }

      internal:
        event PublishFailedHandler^ _publishFailed;

        event SessionClosedHandler^ _sessionClosed;

        void Close(bool OnError = false);

        void RaisePublishFailed(FailureEventArgs^ args);

        void RaiseSessionClosed(SessionClosedEventArgs^ args);
      private:

        ComPtr<RTMPPublisherSink> _sink = nullptr;

        std::vector<PublishProfile^> _params;


        std::atomic_uint _subcount_publishFailed = 0;
        std::atomic_uint _subcount_sessionClosed = 0;

        std::vector<PublishProfile^> CheckProfiles(Windows::Foundation::Collections::IVector<PublishProfile^>^ targetProfiles);
        
        bool EnsureUniqueStreamNames(std::vector<PublishProfile^> profiles);

        

      };
    }
  }
}