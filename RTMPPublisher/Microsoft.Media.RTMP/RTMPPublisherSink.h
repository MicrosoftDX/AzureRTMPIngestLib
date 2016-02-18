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


#pragma comment(lib,"mf.lib")
#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mfuuid.lib")

#include <wrl.h>
#include <mfapi.h>
#include <mfidl.h>
#include <Mferror.h> 
#include <mfreadwrite.h>
#include <wtypes.h> 
#include <windows.media.h>
#include <windows.foundation.collections.h>
#include <windows.media.mediaproperties.h>
#include <memory>
#include <mutex>
#include <atomic>
#include <agile.h>
#include <deque>
#include <vector>
#include <map>

#include "Constants.h"
#include "PublishProfile.h"
#include "RTMPMessenger.h"
#include "Workitem.h"
#include "RTMPAudioStreamSink.h"
#include "RTMPVideoStreamSink.h" 



using namespace Microsoft::WRL;
using namespace Windows::Media::MediaProperties;

namespace Microsoft
{
  namespace Media
  {
    namespace RTMP
    {

      ref class RTMPPublishSession;

      class ProfileState;

      class DECLSPEC_UUID("C4DE71DE-F234-4105-9D26-843B5DA7A89A") RTMPPublisherSink :
        public RuntimeClass<
        RuntimeClassFlags<RuntimeClassType::WinRtClassicComMix>,
        ABI::Windows::Media::IMediaExtension,
        FtmBase,
        IMFMediaSink,
        IMFClockStateSink >
      {
        InspectableClass(L"Microsoft.RTMP.Publisher.RTMPPublisherSink", BaseTrust);

      public:

        RTMPPublisherSink()
        {

          MFAllocateSerialWorkQueue(MFASYNC_CALLBACK_QUEUE_MULTITHREADED, &_sampleProcessingQueue);
          return;
        }



        /** IMediaExtension implementation **/
        IFACEMETHOD(SetProperties)(ABI::Windows::Foundation::Collections::IPropertySet *props);

        /** IMFMediaSink implementation **/
        IFACEMETHOD(AddStreamSink)(DWORD dwStreamSinkIdentifier, IMFMediaType *pMediaType, IMFStreamSink **ppStreamSink);

        IFACEMETHOD(GetCharacteristics)(DWORD *pdwCharacteristics);

        IFACEMETHOD(GetPresentationClock)(IMFPresentationClock **ppPresentationClock);

        IFACEMETHOD(GetStreamSinkById)(DWORD dwStreamSinkIdentifier, IMFStreamSink **ppStreamSink);

        IFACEMETHOD(GetStreamSinkByIndex)(DWORD  dwIndex, IMFStreamSink **ppStreamSink);

        IFACEMETHOD(GetStreamSinkCount)(DWORD *pcStreamSinkCount);

        IFACEMETHOD(RemoveStreamSink)(DWORD dwStreamSinkIdentifier);

        IFACEMETHOD(SetPresentationClock)(IMFPresentationClock *pPresentationClock);

        IFACEMETHOD(Shutdown)();


        /** IMFClockStateSink implementation  **/
        IFACEMETHOD(OnClockStart)(MFTIME hnsSystemTime, LONGLONG llClockStartOffset);

        IFACEMETHOD(OnClockStop)(MFTIME hnsSystemTime);

        IFACEMETHOD(OnClockPause)(MFTIME hnsSystemTime);

        IFACEMETHOD(OnClockRestart)(MFTIME hnsSystemTime);

        IFACEMETHOD(OnClockSetRate)(MFTIME hnsSystemTime, float flRate);

        void Initialize(std::vector<PublishProfile^> targetPublishProfiles,
          VideoEncodingProperties^ interimVideoProfile,
          AudioEncodingProperties^ interimAudioProfile,
          RTMPPublishSession^ session,
          RTMPPublisherSink* aggregatingParentSink = nullptr);
 
        task<void> ConnectRTMPAsync();

        void StopPresentationClock();

        HRESULT GetStreamSinkIndex(const GUID& MajorMediaType, DWORD& dwsinkidx)
        {
          if (MajorMediaType == MFMediaType_Audio)
          {
            if (this->_audioStreamSink == nullptr)
              return E_NOTIMPL;
            else
              dwsinkidx = 0;

          }
          else if (MajorMediaType == MFMediaType_Video)
          {
            if (this->_videoStreamSink == nullptr)
              return E_NOTIMPL;
            else if (this->_audioStreamSink == nullptr)
              dwsinkidx = 0;
            else
              dwsinkidx = 1;
          }

          return S_OK;
        }


        inline shared_ptr<RTMPMessenger> GetMessenger()
        {
          return _rtmpMessenger;
        }

        inline ComPtr<RTMPAudioStreamSink> GetAudioSink()
        {
          return _audioStreamSink;
        }

        inline ComPtr<RTMPVideoStreamSink> GetVideoSink()
        {
          return _videoStreamSink;
        }

        inline bool IsAggregating()
        {
          return _isAggregating;
        }

        inline bool IsAggregated()
        {
          return !_isAggregating && _aggregatingParentSink != nullptr;
        }

        inline bool IsStandalone()
        {
          return !_isAggregating && _aggregatingParentSink == nullptr;
        }

        inline void MakeAggregating()
        {
          _isAggregating = true;
        }

        inline void MakeAggregated(RTMPPublisherSink* aggregator)
        {
          _isAggregating = false;
          _aggregatingParentSink = aggregator;
        }

        inline shared_ptr<ProfileState> GetProfileState(int idx = 0)
        {
          return _targetProfileStates[idx];
        }

        inline bool IsState(SinkState state)
        {
          return (SinkState) _sinkState == state;
        }

        inline RTMPPublisherSink* GetParentSink()
        {
          return _aggregatingParentSink;
        }

        inline void SetState(SinkState state)
        {
          if (_audioStreamSink != nullptr)
            _audioStreamSink->SetState(state);
          if (_videoStreamSink != nullptr)
            _videoStreamSink->SetState(state);
          _sinkState = state;
        }

        std::recursive_mutex SinkMutex;

      
      private:

        MediaEncodingProfile^ CreateInterimProfile(MediaEncodingProfile^ targetProfile);

        RTMPPublishSession^ _session;

        ComPtr<RTMPAudioStreamSink> _audioStreamSink = nullptr;

        ComPtr<RTMPVideoStreamSink> _videoStreamSink = nullptr;

        ComPtr<IMFPresentationClock> _clock = nullptr;

        DWORD _sampleProcessingQueue = 0; 

        std::atomic<SinkState> _sinkState = SinkState::UNINITIALIZED; 

        std::vector<shared_ptr<ProfileState>> _targetProfileStates;

        bool _isAggregating = false;

        shared_ptr<RTMPMessenger> _rtmpMessenger = nullptr;

        RTMPPublisherSink* _aggregatingParentSink = nullptr;

        ComPtr<IMFDXGIDeviceManager> dxgimgr = nullptr;
      };




      ActivatableClass(RTMPPublisherSink)
    }
  }
}
