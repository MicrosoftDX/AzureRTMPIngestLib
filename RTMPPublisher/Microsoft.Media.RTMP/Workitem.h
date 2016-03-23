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

#include <pch.h>
#include <mfidl.h>
#include <mfapi.h>
#include <Mferror.h>
#include <wrl.h>
#include <vector>
#include <wtypes.h>


using namespace Microsoft::WRL;
using namespace std;


namespace Microsoft
{
  namespace Media
  {
    namespace RTMP
    {

      class ContentType
      {
      public:
        static const unsigned int AUDIO = 1;
        static const unsigned int VIDEO = 2;
      };

      class WorkItemType
      {
      public:
        static const unsigned int MEDIASAMPLE = 1;
        static const unsigned int MARKER = 2;
      };


      class CustomMediaBuffer :
        public RuntimeClass < RuntimeClassFlags <RuntimeClassType::ClassicCom>,
        IMFMediaBuffer,
        FtmBase>
      {
      public:
        HRESULT RuntimeClassInitialize(DWORD maxlen)
        {
          _maxlen = maxlen;
          _bufferdata.resize(maxlen);
          return S_OK;
        }

        STDMETHODIMP GetCurrentLength(DWORD* pcbCurrentLength)
        {
          *pcbCurrentLength = (DWORD)_bufferdata.size();
          return S_OK;
        }

        STDMETHODIMP GetMaxLength(DWORD* pcbMaxLength)
        {
          *pcbMaxLength = _maxlen;
          return S_OK;
        }

        STDMETHODIMP SetCurrentLength(DWORD cbCurrentLength)
        {
          if (cbCurrentLength > _maxlen)
            return E_INVALIDARG;
          else
          {
            _bufferdata.resize((size_t)cbCurrentLength);
            return S_OK;
          }
        }

        STDMETHODIMP Lock(BYTE ** ppbBuffer, DWORD *pcbMaxLen, DWORD *pcbCurrentLength)
        {
          *ppbBuffer = &(*(begin(_bufferdata)));
          if(pcbMaxLen != nullptr)
            *pcbMaxLen = _maxlen;
          if (pcbCurrentLength != nullptr)
            *pcbCurrentLength = (DWORD)_bufferdata.size();

          return S_OK;
        }

        STDMETHODIMP Unlock()
        {
          return S_OK;
        }

      private:
        std::vector<BYTE> _bufferdata;
        DWORD _maxlen = 0;
         
      };
      struct WorkItemInfo
      {
        WorkItemInfo(unsigned int contenttype, unsigned int workitemtype) : _contenttype(contenttype), _workitemtype(workitemtype)
        {

        }
        unsigned int GetContentType()
        {
          return _contenttype;
        }

        unsigned int GetWorkItemType()
        {
          return _workitemtype;
        }
      private:
        unsigned int _contenttype = ContentType::AUDIO;
        unsigned int _workitemtype = WorkItemType::MEDIASAMPLE;
      };

      class MediaSampleInfo : public WorkItemInfo
      {
      public:
        MediaSampleInfo(const unsigned int ContentType, IMFSample *pSample) :
          WorkItemInfo(ContentType, WorkItemType::MEDIASAMPLE),
          _sample(pSample)
        {
        }

        ~MediaSampleInfo()
        {
          if (_sample != nullptr)
            _sample.Reset();
        }

        ComPtr<IMFSample> GetSample()
        {
          return _sample;
        }

        LONGLONG GetSampleTimestamp()
        {
          LONGLONG retval = 0;

          if (!(_sample == nullptr))
          {
            ThrowIfFailed(_sample->GetSampleTime(&retval));
          }
          else
            throw E_OUTOFMEMORY;

          return retval;
        }

        LONGLONG GetDecodeTimestamp()
        {
          UINT64 decodets = 0;
          if (FAILED(_sample->GetUINT64(MFSampleExtension_DecodeTimestamp, &decodets)))
          {
            return -1;
          }
          else
            return  decodets;
        }

        LONGLONG GetSampleDuration()
        {
          LONGLONG retval = 0;

          if (!(_sample == nullptr))
          {
            ThrowIfFailed(_sample->GetSampleDuration(&retval));
          }
          else
            throw E_OUTOFMEMORY;

          return retval;
        }

       
        bool IsKeyFrame()
        {

          if (_sample == nullptr)
            throw E_UNEXPECTED;

          UINT32 val = 0;
          ThrowIfFailed(_sample->GetUINT32(MFSampleExtension_CleanPoint, &val));
          return val == TRUE;
        }

        DWORD GetTotalDataLength()
        {
          if (_sample == nullptr)
            throw E_OUTOFMEMORY;

          DWORD totlen = 0;
          ThrowIfFailed(_sample->GetTotalLength(&totlen));
          return totlen;
        }

        std::vector<BYTE> GetSampleData()
        {

          if (_sample == nullptr)
            throw E_OUTOFMEMORY;


          std::vector<BYTE> retval;

          DWORD bufferCount = 0;
          ThrowIfFailed(_sample->GetBufferCount(&bufferCount));

          for (DWORD i = 0; i < bufferCount; i++)
          {
            ComPtr<IMFMediaBuffer> buffer = nullptr;
            ThrowIfFailed(_sample->GetBufferByIndex(i, &buffer));
            DWORD bufferLen = 0;
            ThrowIfFailed(buffer->GetCurrentLength(&bufferLen));

            auto oldsize = retval.size();

            if (bufferLen > 0)
            {
              retval.resize(oldsize + bufferLen);

              DWORD maxlen = 0;
              BYTE * bufferdata = nullptr;
              ThrowIfFailed(buffer->Lock(&bufferdata, &maxlen, &bufferLen));
              memcpy_s(&(*(retval.begin() + oldsize)), bufferLen, bufferdata, bufferLen);
              ThrowIfFailed(buffer->Unlock());
            }

          }

          return retval;
        }

        static ComPtr<IMFSample> CloneSample(IMFSample* pSample)
        {
          ComPtr<IMFSample> retval = nullptr;
          ThrowIfFailed(MFCreateSample(&retval));
          if (retval == nullptr)
            throw E_OUTOFMEMORY;

          DWORD maxlen = 0; 

          DWORD bufferCount = 0;
          ThrowIfFailed(pSample->GetBufferCount(&bufferCount));

          for (DWORD i = 0; i < bufferCount; i++)
          {
            ComPtr<IMFMediaBuffer> buffer = nullptr;
            ThrowIfFailed(pSample->GetBufferByIndex(i, &buffer));
            DWORD bufferLen = 0;
            ThrowIfFailed(buffer->GetCurrentLength(&bufferLen));

            
            if (bufferLen > 0)
            {
              ComPtr<IMFMediaBuffer> custombuff;
              ThrowIfFailed(MakeAndInitialize<CustomMediaBuffer>(&custombuff, bufferLen));
              
              DWORD maxlen = 0;
              BYTE * srcbufferdata = nullptr;
              BYTE* destbufferdata = nullptr;
              ThrowIfFailed(buffer->Lock(&srcbufferdata, &maxlen, &bufferLen));
              ThrowIfFailed(custombuff->Lock(&destbufferdata, &maxlen, &bufferLen)); 
              memcpy_s(destbufferdata, bufferLen, srcbufferdata, bufferLen);
              ThrowIfFailed(buffer->Unlock());
              ThrowIfFailed(custombuff->Unlock());
              ThrowIfFailed(custombuff->SetCurrentLength(bufferLen));
              ThrowIfFailed(retval->AddBuffer(custombuff.Get()));
            }

          }

          LONGLONG sampletime;
          LONGLONG sampledur; 

          ThrowIfFailed(pSample->GetSampleTime(&sampletime));
          ThrowIfFailed(retval->SetSampleTime(sampletime));
          ThrowIfFailed(pSample->GetSampleDuration(&sampledur));
          ThrowIfFailed(retval->SetSampleDuration(sampledur));

          UINT64 decodets = 0;
          if (SUCCEEDED(pSample->GetUINT64(MFSampleExtension_DecodeTimestamp, &decodets)))
          {
            ThrowIfFailed(retval->SetUINT64(MFSampleExtension_DecodeTimestamp,decodets));
          } 

          UINT32 val = 0;
          ThrowIfFailed(pSample->GetUINT32(MFSampleExtension_CleanPoint, &val));
          ThrowIfFailed(retval->SetUINT32(MFSampleExtension_CleanPoint,val));

          return retval;
        }
      private:
        ComPtr<IMFSample> _sample;

      };

      class MarkerInfo : public WorkItemInfo
      {
      public:
        MarkerInfo(const unsigned int ContentType, MFSTREAMSINK_MARKER_TYPE markerType, const PROPVARIANT *markerValue, const PROPVARIANT *markerContextValue) :
          WorkItemInfo(ContentType, WorkItemType::MARKER),
          _markerType(markerType)
        {
          if (_markerType == MFSTREAMSINK_MARKER_TYPE::MFSTREAMSINK_MARKER_TICK && markerValue != nullptr)
            _markerTick = markerValue->hVal.QuadPart;

          if (markerContextValue != nullptr)
          {
            _contextValue = make_unique<PROPVARIANT>();
            ::PropVariantInit(_contextValue.get());
            ThrowIfFailed(::PropVariantCopy(_contextValue.get(), markerContextValue));
          }
        }

        virtual ~MarkerInfo()
        {
          if (_contextValue != nullptr)
            ::PropVariantClear(_contextValue.get());
        }

        PROPVARIANT* GetContextValue()
        {
          if (_contextValue != nullptr)
            return _contextValue.get();
          else
            return nullptr;
        }

        MFSTREAMSINK_MARKER_TYPE GetMarkerType()
        {
          return _markerType;
        }

        LONGLONG GetMarkerTick()
        {
          return _markerTick;
        }

      private:
        MFSTREAMSINK_MARKER_TYPE _markerType;
        LONGLONG _markerTick = 0LL;
        unique_ptr<PROPVARIANT> _contextValue = nullptr;
      };

      class WorkItem : public RuntimeClass<RuntimeClassFlags<RuntimeClassType::ClassicCom>, FtmBase>
      {
      public:

        HRESULT RuntimeClassInitialize(shared_ptr<WorkItemInfo> wiInfo)
        {
          if (wiInfo == nullptr)
            return E_INVALIDARG;

          _workitemInfo = wiInfo;

          return S_OK;
        }

        shared_ptr<WorkItemInfo> GetWorkItemInfo()
        {
          return _workitemInfo;
        }

        ~WorkItem()
        {
          if (_workitemInfo != nullptr)
            _workitemInfo.reset();
        }

      private:
        shared_ptr<WorkItemInfo> _workitemInfo = nullptr;
      };


    }
  }
}
