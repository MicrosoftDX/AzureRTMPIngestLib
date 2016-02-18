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

#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <wrl.h>
#include <mutex>

using namespace Microsoft::WRL;

namespace Microsoft
{
  namespace Media
  {
    namespace RTMP
    {


      class MediaEventGeneratorImpl
      {


      public:

        MediaEventGeneratorImpl()
        {
          MFCreateEventQueue(&_cpEventQueue);
        }

        virtual ~MediaEventGeneratorImpl()
        {
          if (_cpEventQueue != nullptr)
            _cpEventQueue->Shutdown();
        }


        ///<summary>See IMFMediaEventGenerator in MSDN</summary>
        IFACEMETHODIMP BeginGetEvent(IMFAsyncCallback *pCallback, IUnknown *punkState)
        {
          HRESULT hr = S_OK;
          std::lock_guard<std::recursive_mutex> lock(_lockEvent);

          hr = _cpEventQueue->BeginGetEvent(pCallback, punkState);
          return hr;
        }
        ///<summary>See IMFMediaEventGenerator in MSDN</summary>
        IFACEMETHODIMP EndGetEvent(IMFAsyncResult *pResult, IMFMediaEvent **ppEvent)
        {
          HRESULT hr = S_OK;
          std::lock_guard<std::recursive_mutex> lock(_lockEvent);
          hr = _cpEventQueue->EndGetEvent(pResult, ppEvent);

          return hr;
        }
        ///<summary>See IMFMediaEventGenerator in MSDN</summary>
        IFACEMETHODIMP GetEvent(DWORD dwFlags, IMFMediaEvent **ppEvent)
        {
          HRESULT hr = S_OK;
          std::lock_guard<std::recursive_mutex> lock(_lockEvent);
          hr = _cpEventQueue->GetEvent(dwFlags, ppEvent);

          return hr;
        }
        ///<summary>See IMFMediaEventGenerator in MSDN</summary>
        IFACEMETHODIMP QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT *pvValue)
        {
          HRESULT hr = S_OK;
          std::lock_guard<std::recursive_mutex> lock(_lockEvent);
          hr = _cpEventQueue->QueueEventParamVar(met, guidExtendedType, hrStatus, pvValue);
          return hr;
        }

      private:

        std::recursive_mutex _lockEvent;
        //the event queue for this object
        ComPtr<IMFMediaEventQueue> _cpEventQueue = nullptr;
      };


    }
  }
} 
