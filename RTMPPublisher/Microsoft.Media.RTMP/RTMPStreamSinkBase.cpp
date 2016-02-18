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

#include <pch.h>
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include "RTMPPublisherSink.h"
#include "RTMPStreamSinkBase.h"
#include "RTMPPublishSession.h"
#include "ProfileState.h"

using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Microsoft::Media::RTMP;


HRESULT RTMPStreamSinkBase::RuntimeClassInitialize(DWORD streamid,
  RTMPPublisherSink *mediasinkparent,
  MediaEncodingProfile ^outputProfile,
  DWORD serialWorkQueue, 
  RTMPPublishSession^ session, std::vector<shared_ptr<ProfileState>> targetProfileStates)
{
  _streamid = streamid;
  _session = session;
  _mediasinkparent = mediasinkparent;
  _serialWorkQueue = serialWorkQueue;
  _outputProfile = outputProfile; 
  _targetProfileStates = targetProfileStates;
  if (_targetProfileStates.size() > 1)
    _isAggregating = true;

  MFCreateEventQueue(&_cpEventQueue);
  CreateMediaType(_outputProfile);
  return S_OK;
}

 

IFACEMETHODIMP RTMPStreamSinkBase::BeginGetEvent(IMFAsyncCallback *pCallback, IUnknown *punkState)
{
  HRESULT hr = S_OK;
  std::lock_guard<std::recursive_mutex> lock(_lockEvent);

  hr = _cpEventQueue->BeginGetEvent(pCallback, punkState);
  return hr;
}
IFACEMETHODIMP RTMPStreamSinkBase::EndGetEvent(IMFAsyncResult *pResult, IMFMediaEvent **ppEvent)
{
  HRESULT hr = S_OK;
  std::lock_guard<std::recursive_mutex> lock(_lockEvent);
  hr = _cpEventQueue->EndGetEvent(pResult, ppEvent);

  return hr;
}
IFACEMETHODIMP RTMPStreamSinkBase::GetEvent(DWORD dwFlags, IMFMediaEvent **ppEvent)
{
  HRESULT hr = S_OK;
  std::lock_guard<std::recursive_mutex> lock(_lockEvent);
  hr = _cpEventQueue->GetEvent(dwFlags, ppEvent);

  return hr;
}
IFACEMETHODIMP RTMPStreamSinkBase::QueueEvent(MediaEventType met, 
  REFGUID guidExtendedType, 
  HRESULT hrStatus, 
  const PROPVARIANT *pvValue)
{
  HRESULT hr = S_OK;
  std::lock_guard<std::recursive_mutex> lock(_lockEvent);
  hr = _cpEventQueue->QueueEventParamVar(met, guidExtendedType, hrStatus, pvValue);
  return hr;
}


HRESULT RTMPStreamSinkBase::NotifyStreamSinkStarted()
{
  PROPVARIANT pVar;
  try
  {
    ::PropVariantInit(&pVar);
    pVar.vt = VT_EMPTY;
    ThrowIfFailed(QueueEvent(MEStreamSinkStarted, GUID_NULL, S_OK, &pVar));
    ThrowIfFailed(::PropVariantClear(&pVar));
  }
  catch (const HRESULT& hr)
  {
    return hr;
  }

  return S_OK;
}

HRESULT RTMPStreamSinkBase::NotifyStreamSinkStopped()
{
  PROPVARIANT pVar;
  try
  {
    ::PropVariantInit(&pVar);
    pVar.vt = VT_EMPTY;
    ThrowIfFailed(QueueEvent(MEStreamSinkStopped, GUID_NULL, S_OK, &pVar));
    ThrowIfFailed(::PropVariantClear(&pVar));
  }
  catch (const HRESULT& hr)
  {
    return hr;
  }

  return S_OK;
}

HRESULT RTMPStreamSinkBase::NotifyStreamSinkRequestSample()
{
  PROPVARIANT pVar;
  try
  {
    ::PropVariantInit(&pVar);
    pVar.vt = VT_EMPTY;
    ThrowIfFailed(QueueEvent(MEStreamSinkRequestSample, GUID_NULL, S_OK, &pVar));
    ThrowIfFailed(::PropVariantClear(&pVar));
  }
  catch (const HRESULT& hr)
  {
    return hr;
  }

  //LOG("Notified Sample Request");
  return S_OK;
}

HRESULT RTMPStreamSinkBase::NotifyStreamSinkPaused()
{
  PROPVARIANT pVar;
  try
  {
    ::PropVariantInit(&pVar);
    pVar.vt = VT_EMPTY;
    ThrowIfFailed(QueueEvent(MEStreamSinkPaused, GUID_NULL, S_OK, &pVar));
    ThrowIfFailed(::PropVariantClear(&pVar));
  }
  catch (const HRESULT& hr)
  {
    return hr;
  }

  return S_OK;
}

HRESULT RTMPStreamSinkBase::NotifyStreamSinkMarker(const PROPVARIANT *pVarContextValue)
{
  try
  {

    if (pVarContextValue == nullptr)
    {
      PROPVARIANT pVar;
      ::PropVariantInit(&pVar);
      pVar.vt = VT_EMPTY;
      ThrowIfFailed(QueueEvent(MEStreamSinkMarker, GUID_NULL, S_OK, &pVar));
      ThrowIfFailed(::PropVariantClear(&pVar));
    }
    else
      ThrowIfFailed(QueueEvent(MEStreamSinkMarker, GUID_NULL, S_OK, pVarContextValue));
  }
  catch (const HRESULT& hr)
  {
    return hr;
  }

  //LOG("Notified Marker");

  return S_OK;
}


IFACEMETHODIMP RTMPStreamSinkBase::GetMediaSink(IMFMediaSink **ppMediaSink)
{

  if (_mediasinkparent == nullptr)
    return MF_E_NOT_INITIALIZED;

  return _mediasinkparent.Get()->QueryInterface(IID_PPV_ARGS(ppMediaSink));
}

IFACEMETHODIMP RTMPStreamSinkBase::GetIdentifier(DWORD *pdwIdentifier)
{
  *pdwIdentifier = _streamid;
  return S_OK;
}

IFACEMETHODIMP RTMPStreamSinkBase::GetMediaTypeHandler(IMFMediaTypeHandler **ppHandler)
{
  HRESULT hr = S_OK;

  if (_mediasinkparent == nullptr)
    return MF_E_NOT_INITIALIZED;

  if (_mediasinkparent->IsState(SinkState::SHUTDOWN))
    return MF_E_SHUTDOWN;

  this->QueryInterface(IID_PPV_ARGS(ppHandler));

  return hr;
}

IFACEMETHODIMP RTMPStreamSinkBase::Flush()
{
  HRESULT hr = S_OK;
  LOG("Flush()");
  return hr;
}


IFACEMETHODIMP RTMPStreamSinkBase::GetCurrentMediaType(IMFMediaType **ppMediaType)
{
  if (_currentMediaType == nullptr)
    return MF_E_NOT_INITIALIZED;

  _currentMediaType.Get()->QueryInterface(IID_PPV_ARGS(ppMediaType));

  return S_OK;
}

IFACEMETHODIMP RTMPStreamSinkBase::GetMajorType(GUID *pguidMajorType)
{
  *pguidMajorType = _majorType;
  return S_OK;
}

IFACEMETHODIMP RTMPStreamSinkBase::GetMediaTypeByIndex(DWORD dwIndex, IMFMediaType **ppType)
{
  if (dwIndex > 0)
    return MF_E_NO_MORE_TYPES;

  _currentMediaType.Get()->QueryInterface(IID_PPV_ARGS(ppType));

  return S_OK;
}

IFACEMETHODIMP RTMPStreamSinkBase::GetMediaTypeCount(DWORD *pdwTypeCount)
{
  *pdwTypeCount = 1;

  return S_OK;
}

IFACEMETHODIMP RTMPStreamSinkBase::IsMediaTypeSupported(IMFMediaType *pMediaType, IMFMediaType **ppMediaType)
{
  GUID tgtMajorType = GUID_NULL;
  GUID tgtSubtype = GUID_NULL;

  try
  {
    if (pMediaType == nullptr)
      throw E_INVALIDARG;

    ThrowIfFailed(pMediaType->GetGUID(MF_MT_MAJOR_TYPE, &tgtMajorType));

    if (tgtMajorType != _majorType)
      throw MF_E_INVALIDMEDIATYPE;

    ThrowIfFailed(pMediaType->GetGUID(MF_MT_SUBTYPE, &tgtSubtype));

    if (_currentMediaType != nullptr && tgtSubtype != _currentSubtype)
      throw MF_E_INVALIDMEDIATYPE;


    if (ppMediaType)
      *ppMediaType = nullptr;

    return S_OK;
  }
  catch (const HRESULT& hr)
  {
    return hr;
  }
}


IFACEMETHODIMP RTMPStreamSinkBase::SetCurrentMediaType(IMFMediaType *pMediaType)
{
  HRESULT hr = S_OK;

  GUID tgtMajorType = GUID_NULL;
  GUID tgtSubtype = GUID_NULL;

  try
  {

    if (_currentMediaType != nullptr)
      ThrowIfFailed(IsMediaTypeSupported(pMediaType, nullptr));

    ThrowIfFailed(::MFCreateMediaType(&_currentMediaType));

    ThrowIfFailed(pMediaType->CopyAllItems(_currentMediaType.Get()));

    ThrowIfFailed(pMediaType->GetGUID(MF_MT_SUBTYPE, &_currentSubtype));



    return S_OK;

  }
  catch (const HRESULT& hr)
  {
    return hr;
  }
}


HRESULT RTMPStreamSinkBase::ToMediaType(IMediaEncodingProperties^ encodingProps, 
  IMFMediaType **ppMediaType, 
  std::vector<GUID> keyfilter) 
{
  try
  {
    ComPtr<IMFMediaType> mediaType;
    


    ThrowIfFailed(MFCreateMediaType(&mediaType));

    ComPtr<IMFAttributes> attr;
    ThrowIfFailed(mediaType.As(&attr));

    auto itr = encodingProps->Properties->First();
    while (itr->HasCurrent)
    {
      GUID key = itr->Current->Key;

      if (keyfilter.size() != 0 &&
        std::find_if(begin(keyfilter), end(keyfilter),
          [&key](GUID g) { return IsEqualGUID(g, key) == TRUE; }) == end(keyfilter))
      {
        itr->MoveNext();
        continue;
      }
        

      IPropertyValue^ val = safe_cast<IPropertyValue^>(itr->Current->Value);

      switch (val->Type)
      {
      case PropertyType::String:
        ThrowIfFailed(attr->SetString(key, val->GetString()->Data()));
        break;
      case PropertyType::UInt32:
        ThrowIfFailed(attr->SetUINT32(key, val->GetUInt32()));
        break;
      case PropertyType::UInt64:
        ThrowIfFailed(attr->SetUINT64(key, val->GetUInt64()));
        break;
      case PropertyType::Double:
        ThrowIfFailed(attr->SetDouble(key, val->GetDouble()));
        break;
      case PropertyType::Guid:
        ThrowIfFailed(attr->SetGUID(key, val->GetGuid()));
        break;
      case PropertyType::UInt8Array:

        Platform::Array<BYTE> ^arr;
        val->GetUInt8Array(&arr);
        ThrowIfFailed(attr->SetBlob(key, arr->Data, arr->Length));
        break;

      }

      itr->MoveNext();
    }
     

    *ppMediaType = mediaType.Detach();
    return S_OK;
  }
  catch (const HRESULT& hr)
  {
    return hr;
  }


}

HRESULT RTMPStreamSinkBase::StreamSinkClockStart(MFTIME hnsSystemTime, LONGLONG llClockStartOffset)
{
  try
  {
    LOG("Stream Sink started");
    _clockStartOffset = llClockStartOffset;
    SetState(SinkState::RUNNING);

    NotifyStreamSinkStarted();

    NotifyStreamSinkRequestSample();
  }
  catch (const HRESULT& hr)
  {
    return hr;
  }

  return S_OK;
}

HRESULT RTMPStreamSinkBase::StreamSinkClockStop(MFTIME hnsSystemTime)
{
  try
  {
    SetState(SinkState::STOPPED); 
    NotifyStreamSinkStopped();
  }
  catch (const HRESULT& hr)
  {
    return hr;
  }
  return S_OK;
}

HRESULT RTMPStreamSinkBase::StreamSinkClockPause(MFTIME hnsSystemTime)
{

  return S_OK;
}

HRESULT RTMPStreamSinkBase::StreamSinkClockRestart(MFTIME hnsSystemTime)
{

  return S_OK;
}

HRESULT RTMPStreamSinkBase::StreamSinkClockSetRate(MFTIME hnsSystemTime, float flRate)
{

  return S_OK;
}


HRESULT RTMPStreamSinkBase::BeginProcessNextWorkitem(ComPtr<WorkItem> pWorkItem)
{
  ComPtr<IMFAsyncResult> asyncResult;

  std::lock_guard<std::recursive_mutex> lock(_lockSink);

  try
  {

     
    ComPtr<IUnknown> pUnk;
    pWorkItem.Get()->QueryInterface(IID_PPV_ARGS(&pUnk));

    ThrowIfFailed(MFCreateAsyncResult(nullptr, this, pUnk.Detach(), &asyncResult));
    ThrowIfFailed(MFPutWorkItemEx2(_serialWorkQueue, 0, asyncResult.Detach()));
  }

  catch (...)
  {
    throw;
  }
 

  return S_OK;
}



IFACEMETHODIMP RTMPStreamSinkBase::Invoke(IMFAsyncResult *pAsyncResult)
{

  std::lock_guard<std::recursive_mutex> lock(_lockSink);

  FailureEventArgs^ failedargs = nullptr;
  try
  {

    ThrowIfFailed(CompleteProcessNextWorkitem(pAsyncResult));

  }
  catch (const std::exception& ex)
  {
    wstringstream s;
    s << ex.what();
    failedargs = ref new FailureEventArgs(E_FAIL, ref new String(s.str().data()));
  }
  catch (COMException^ cex)
  {
    failedargs = ref new FailureEventArgs(cex->HResult, cex->Message);
  }
  catch (const HRESULT& hr)
  {
    failedargs = ref new FailureEventArgs(hr);
  }
  catch (...)
  {
    failedargs = ref new FailureEventArgs(E_FAIL);
  }



  ComPtr<WorkItem> workitem = nullptr;
  pAsyncResult->GetState(&workitem);
  if (workitem != nullptr)
    workitem.Get()->Release();

  pAsyncResult->Release();

  if (failedargs != nullptr)
  {
    _session->RaisePublishFailed(failedargs);
    return E_FAIL;
  }
  else 
  {
    return S_OK;
  } 
}







