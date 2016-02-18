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

#include <mfidl.h>
#include <mfapi.h>
#include <Mferror.h>
#include <wrl.h>
#include <memory> 
#include <vector>
#include <map>
#include <mfreadwrite.h>
#include <windows.media.mediaproperties.h> 
#include "Constants.h"
#include "Workitem.h"
#include "RTMPMessenger.h"
#include "PublishProfile.h" 

using namespace Microsoft::WRL;
using namespace Windows::Media::MediaProperties;
using namespace std;

namespace Microsoft
{
  namespace Media
  {
    namespace RTMP
    {
      ref class RTMPPublishSession;
      class RTMPPublisherSink;
      class ProfileState;

      class RTMPStreamSinkBase :
        public RuntimeClass < RuntimeClassFlags <RuntimeClassType::ClassicCom>,
        IMFStreamSink,
        IMFAsyncCallback,
        FtmBase,
        IMFMediaEventGenerator,
        IMFMediaTypeHandler>
      {
        friend class RTMPPublisherSink;

      public:

        virtual HRESULT RuntimeClassInitialize(
          DWORD streamid,
          RTMPPublisherSink *mediasinkparent,
          MediaEncodingProfile ^outputProfile,
          DWORD serialWorkQueue,
          RTMPPublishSession^ session, 
          std::vector<shared_ptr<ProfileState>> targetProfileStates);



        ~RTMPStreamSinkBase()
        {
          if (_cpEventQueue != nullptr)
            _cpEventQueue->Shutdown();
        }

        //IMFStreamSink members 
        IFACEMETHOD(GetMediaSink)(IMFMediaSink **ppMediaSink);

        IFACEMETHOD(GetIdentifier)(DWORD *pdwIdentifier);

        IFACEMETHOD(GetMediaTypeHandler)(IMFMediaTypeHandler **ppHandler);

        IFACEMETHOD(Flush)();

        //IMFMediaEventGenerator members
        IFACEMETHOD(BeginGetEvent)(IMFAsyncCallback *pCallback, IUnknown *punkState);

        IFACEMETHOD(EndGetEvent)(IMFAsyncResult *pResult, IMFMediaEvent **ppEvent);

        IFACEMETHOD(GetEvent)(DWORD dwFlags, IMFMediaEvent **ppEvent);

        IFACEMETHOD(QueueEvent)(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT *pvValue);

        //IMFMediaTypeHandler members
        IFACEMETHOD(GetCurrentMediaType)(IMFMediaType **ppMediaType);

        IFACEMETHOD(GetMajorType)(GUID *pguidMajorType);

        IFACEMETHOD(GetMediaTypeByIndex)(DWORD dwIndex, IMFMediaType **ppType);

        IFACEMETHOD(GetMediaTypeCount)(DWORD *pdwTypeCount);

        IFACEMETHOD(IsMediaTypeSupported)(IMFMediaType *pMediaType, IMFMediaType **ppMediaType);

        IFACEMETHOD(SetCurrentMediaType)(IMFMediaType *pMediaType);

        IFACEMETHOD(Invoke)(IMFAsyncResult *pAsyncResult);

        virtual HRESULT CreateMediaType(MediaEncodingProfile^ encodingProfile) = 0;


        virtual HRESULT StreamSinkClockStart(MFTIME hnsSystemTime, LONGLONG llClockStartOffset);

        virtual HRESULT StreamSinkClockStop(MFTIME hnsSystemTime);

        virtual HRESULT StreamSinkClockPause(MFTIME hnsSystemTime);

        virtual HRESULT StreamSinkClockRestart(MFTIME hnsSystemTime);

        virtual HRESULT StreamSinkClockSetRate(MFTIME hnsSystemTime, float flRate);

        virtual HRESULT NotifyStreamSinkStarted();

        virtual HRESULT NotifyStreamSinkStopped();

        virtual HRESULT NotifyStreamSinkRequestSample();

        virtual HRESULT NotifyStreamSinkPaused();

        virtual HRESULT NotifyStreamSinkMarker(const PROPVARIANT *pVarContextValue);


        virtual LONGLONG GetLastDTS() = 0;        

        virtual LONGLONG GetLastPTS() = 0;

        IFACEMETHODIMP GetParameters(DWORD *pdwFlags, DWORD *pdwQueue)
        {
          *pdwFlags = 0;
          *pdwQueue = _serialWorkQueue;
          return S_OK;
        }

        inline virtual void SetClock(IMFPresentationClock *pClock)
        {
          _clock = pClock;

        }

        inline bool IsState(SinkState state)
        {
          return (SinkState) _sinkState == state;
        }

        inline void SetState(SinkState state)
        {
          _sinkState = state;
        }

 

        inline bool IsAggregating() { return _isAggregating; }
        inline void MakeAggregating() { _isAggregating = true; }

        static HRESULT ToMediaType(IMediaEncodingProperties^ mediaEncodingProps, IMFMediaType **ppMediaType, std::vector<GUID> keyfilter = std::vector<GUID>{});

      protected:

        DWORD _streamid = 0;

        RTMPPublishSession^ _session;

        std::recursive_mutex _lockSink;

        std::recursive_mutex _lockEvent;

        //the event queue for this object
        ComPtr<IMFMediaEventQueue> _cpEventQueue = nullptr;

        ComPtr<RTMPPublisherSink> _mediasinkparent = nullptr;

        ComPtr<IMFMediaType> _currentMediaType = nullptr;

        ComPtr<IMFPresentationClock> _clock = nullptr;

        DWORD _serialWorkQueue;

        LONGLONG wraparoundbound = (LONGLONG) (std::numeric_limits<unsigned int>::max)() + 1;

        LONGLONG _clockStartOffset = 0L;

        LONGLONG _sampleInterval = -1;

        LONGLONG _startPTS = -1;

        LONGLONG _lastPTS = -1;

        LONGLONG _lastOriginalPTS = -1;

        LONGLONG _lastOriginalDTS = -1;

        LONGLONG _lastDTS = -1;

        LONGLONG _gaplength = 0;

        MediaEncodingProfile ^_outputProfile = nullptr;

        shared_ptr<RTMPMessenger> _messenger = nullptr;

        GUID _majorType = GUID_NULL;

        GUID _currentSubtype = GUID_NULL;

        std::atomic<SinkState> _sinkState = SinkState::UNINITIALIZED;

        HRESULT BeginProcessNextWorkitem(ComPtr<WorkItem> pWorkItem);

        virtual HRESULT CompleteProcessNextWorkitem(IMFAsyncResult *pAsyncResult) = 0;


        inline unsigned int ToRTMPTimestamp(LONGLONG tsinticks)
        {
          LONGLONG tsinmillis = (LONGLONG) round(tsinticks * TICKSTOMILLIS);
          return (unsigned int) (tsinmillis % wraparoundbound);
        }

        std::vector<shared_ptr<ProfileState>> _targetProfileStates;

        bool _isAggregating = false; 


#if defined(_DEBUG)
        wstring _streamsinkname;
#endif


      };

    }
  }
}
