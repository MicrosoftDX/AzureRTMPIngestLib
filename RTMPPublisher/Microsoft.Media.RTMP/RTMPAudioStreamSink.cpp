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
#include <wrl.h>
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include <codecapi.h>
#include "WorkItem.h"
#include "RTMPPublishSession.h"
#include "RTMPPublisherSink.h"
#include "RTMPAudioStreamSink.h"
#include "ProfileState.h"

using namespace Microsoft::Media::RTMP;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Microsoft::WRL;


HRESULT RTMPAudioStreamSink::CreateMediaType(MediaEncodingProfile^ encodingProfile)
{
  _majorType = MFMediaType_Audio;

  try
  {

    //ensure this is audio or video
    if (!encodingProfile->Audio->Properties->HasKey(MF_MT_MAJOR_TYPE) || !encodingProfile->Audio->Properties->HasKey(MF_MT_SUBTYPE))
      throw E_INVALIDARG;

    GUID majorType = safe_cast<IPropertyValue^>(encodingProfile->Audio->Properties->Lookup(MF_MT_MAJOR_TYPE))->GetGuid();
    if (majorType != MFMediaType_Audio)
      throw E_INVALIDARG;
    GUID subType = safe_cast<IPropertyValue^>(encodingProfile->Audio->Properties->Lookup(MF_MT_SUBTYPE))->GetGuid();


    if (!IsAggregating() && subType != MFAudioFormat_AAC)
      throw E_INVALIDARG;

    ThrowIfFailed(ToMediaType(encodingProfile->Audio, &(this->_currentMediaType)));

    _currentSubtype = safe_cast<IPropertyValue^>(encodingProfile->Audio->Properties->Lookup(MF_MT_SUBTYPE))->GetGuid();

    if (!IsAggregating())
    {
      ThrowIfFailed(_currentMediaType->SetUINT32(MF_MT_AAC_PAYLOAD_TYPE, 0));

      _sampleInterval = (LONGLONG)round(10000000 / ((encodingProfile->Audio->SampleRate) / 1000));
    }



    //Set CBR
    ThrowIfFailed(_currentMediaType->SetUINT32(CODECAPI_AVEncCommonRateControlMode, eAVEncCommonRateControlMode::eAVEncCommonRateControlMode_CBR));

    LOG("Audio media type created")
      return S_OK;
  }
  catch (const HRESULT& hr)
  {
    return hr;
  }

}


IFACEMETHODIMP RTMPAudioStreamSink::ProcessSample(IMFSample *pSample)
{

  std::lock_guard<std::recursive_mutex> lock(_lockSink);

  try
  {

    if (IsState(SinkState::UNINITIALIZED))
      throw ref new COMException(MF_E_NOT_INITIALIZED, "Audio Sink uninitialized");
    if (IsState(SinkState::SHUTDOWN))
      throw ref new COMException(MF_E_SHUTDOWN, "Audio Sink has been shut down");
    if (IsState(SinkState::PAUSED) || IsState(SinkState::STOPPED))
      throw ref new COMException(MF_E_INVALIDREQUEST, "Audio Sink is in an invalid state");

    if (IsState(SinkState::EOS)) //drop
      return S_OK;

    if (!IsAggregating())
    {
      LONGLONG sampleTime = 0;

      ThrowIfFailed(pSample->GetSampleTime(&sampleTime), MF_E_NO_SAMPLE_TIMESTAMP, L"Could not get timestamp from audio sample");

      if (sampleTime < _clockStartOffset) //we do not process samples predating the clock start offset;
      {
        LOG("Not processing audio sample")
          return S_OK;
      }

      ComPtr<WorkItem> wi;

      ThrowIfFailed(MakeAndInitialize<WorkItem>(
        &wi,
        make_shared<MediaSampleInfo>(ContentType::AUDIO,
          pSample)
        ));

      ThrowIfFailed(BeginProcessNextWorkitem(wi));

#if defined(_DEBUG)
      LOG("Dispatched audio sample - " << _streamsinkname);
#endif
    }
    else
    {
      
      //LOG("AudioStreamSink" << (IsAggregating() ? "(Aggregating)" : "") << "::Audio Sample : Original PTS = " << (unsigned int) round(msi->GetSampleTimestamp() * TICKSTOMILLIS)
      //);
      for (auto profstate : _targetProfileStates)
      {
        DWORD sinkidx = 0;

        try
        {
          ThrowIfFailed(profstate->DelegateSink->GetStreamSinkIndex(this->_majorType, sinkidx));
          ThrowIfFailed(profstate->DelegateWriter->WriteSample(sinkidx, pSample));
        }
        catch (...)
        {
          continue;
        }
      }
    }
    if (SinkState::RUNNING)
      ThrowIfFailed(NotifyStreamSinkRequestSample());

  }
  catch (const std::exception& ex)
  {
    wstringstream s;
    s << ex.what();
    _session->RaisePublishFailed(ref new FailureEventArgs(E_FAIL, ref new String(s.str().data())));
    return E_FAIL;
  }
  catch (COMException^ cex)
  {
    _session->RaisePublishFailed(ref new FailureEventArgs(cex->HResult, cex->Message));
    return cex->HResult;
  }
  catch (const HRESULT& hr)
  {
    _session->RaisePublishFailed(ref new FailureEventArgs(hr));
    return hr;
  }
  catch (...)
  {
    _session->RaisePublishFailed(ref new FailureEventArgs(E_FAIL));
    return E_FAIL;
  }
  return S_OK;
}

IFACEMETHODIMP RTMPAudioStreamSink::PlaceMarker(MFSTREAMSINK_MARKER_TYPE eMarkerType, const PROPVARIANT *pvarMarkerValue, const PROPVARIANT *pvarContextValue)
{

  std::lock_guard<std::recursive_mutex> lock(_lockSink);

  try
  {
    LOG("Audio Marker Arrived");

    if (IsState(SinkState::UNINITIALIZED))
      throw ref new COMException(MF_E_NOT_INITIALIZED, "Audio Sink uninitialized");
    if (IsState(SinkState::SHUTDOWN))
      throw ref new COMException(MF_E_SHUTDOWN, "Audio Sink has been shut down");
    if (IsState(SinkState::PAUSED) || IsState(SinkState::STOPPED))
      throw ref new COMException(MF_E_INVALIDREQUEST, "Audio Sink is in an invalid state");

    ComPtr<WorkItem> wi;

    auto markerInfo = make_shared<MarkerInfo>(ContentType::AUDIO,
      eMarkerType,
      pvarMarkerValue,
      pvarContextValue

      );
    if (!IsAggregating())
    {

      if (markerInfo->GetMarkerType() == MFSTREAMSINK_MARKER_TYPE::MFSTREAMSINK_MARKER_ENDOFSEGMENT)
      {
        SetState(SinkState::EOS);
        LOG("AudioStreamSink" << (IsAggregating() ? "(Aggregating)" : "") << "::Audio stream end of segment");
        NotifyStreamSinkMarker(markerInfo->GetContextValue());
        create_task([this]() { _mediasinkparent->StopPresentationClock(); });
      }
      else
      {
        ThrowIfFailed(MakeAndInitialize<WorkItem>(
          &wi,
          markerInfo
          ));


        ThrowIfFailed(BeginProcessNextWorkitem(wi));
        ThrowIfFailed(NotifyStreamSinkMarker(markerInfo->GetContextValue()));

      }
    }
    else
    {

      if (markerInfo->GetMarkerType() == MFSTREAMSINK_MARKER_TYPE::MFSTREAMSINK_MARKER_ENDOFSEGMENT)
      {
        for (auto profstate : _targetProfileStates)
        {
          DWORD sinkidx = 0;

          try
          {
            ThrowIfFailed(profstate->DelegateSink->GetStreamSinkIndex(this->_majorType, sinkidx));
            ThrowIfFailed(profstate->DelegateWriter->Flush(sinkidx));
            ThrowIfFailed(profstate->DelegateWriter->NotifyEndOfSegment(sinkidx));
          }
          catch (...)
          {
            continue;
          }
        }


        SetState(SinkState::EOS);
        NotifyStreamSinkMarker(markerInfo->GetContextValue());
        LOG("AudioStreamSink" << (IsAggregating() ? "(Aggregating)" : "") << "::Audio stream end of segment");
      }
      else
      {

        LOG("AudioStreamSink" << (IsAggregating() ? "(Aggregating)" : "") << "::Audio stream marker");
        for (auto profstate : _targetProfileStates)
        {
          DWORD sinkidx = 0;

          try
          {
            ThrowIfFailed(profstate->DelegateSink->GetStreamSinkIndex(this->_majorType, sinkidx));
            ThrowIfFailed(profstate->DelegateWriter->PlaceMarker(sinkidx, nullptr));
          }
          catch (...)
          {
            continue;
          }
        }

        ThrowIfFailed(NotifyStreamSinkMarker(markerInfo->GetContextValue()));
      }
    }


  }
  catch (const std::exception& ex)
  {
    wstringstream s;
    s << ex.what();
    _session->RaisePublishFailed(ref new FailureEventArgs(E_FAIL, ref new String(s.str().data())));
    return E_FAIL;
  }
  catch (COMException^ cex)
  {
    _session->RaisePublishFailed(ref new FailureEventArgs(cex->HResult, cex->Message));
    return cex->HResult;
  }
  catch (const HRESULT& hr)
  {
    _session->RaisePublishFailed(ref new FailureEventArgs(hr));
    return hr;
  }
  catch (...)
  {
    _session->RaisePublishFailed(ref new FailureEventArgs(E_FAIL));
    return E_FAIL;
  }

  return S_OK;
}

std::vector<BYTE> RTMPAudioStreamSink::PreparePayload(MediaSampleInfo* pSampleInfo, bool firstPacket)
{
  std::vector<BYTE> retval;

  BYTE audiodata = (0X0a /* AAC */ << 4) | 0x0F; /*(bit pattern 1111 - first 2 bits indicate decimal 03 == 44 Khz sampling, 3rd bit of 1 indicate 16 bit samples, and 4thh bit of 1 indicate stereo - always use this pattern for AAC)*/



  BitOp::AddToBitstream(audiodata, retval, false);

  if (firstPacket)
  {
    //we add AACPacketType == 0, AudioSpecificConfig
    auto audioconfigrecord = MakeAudioSpecificConfig(pSampleInfo);
    retval.push_back((BYTE)0);
    auto oldsize = retval.size();
    retval.resize(oldsize + audioconfigrecord.size());
    memcpy_s(&(*(retval.begin() + oldsize)), (unsigned int)audioconfigrecord.size(), &(*(audioconfigrecord.begin())), (unsigned int)audioconfigrecord.size());
  }
  else
  {
    //we add AACPacketType == 1,and AAC Frame Data
    retval.push_back((BYTE)1);

    auto frames = pSampleInfo->GetSampleData();//we get raw aac frames per our output media type setting

    auto oldsize = retval.size();
    retval.resize(oldsize + frames.size());
    memcpy_s(&(*(retval.begin() + oldsize)), (unsigned int)frames.size(), &(*(frames.begin())), (unsigned int)frames.size());
  }
  return retval;
}


void RTMPAudioStreamSink::PrepareTimestamps(MediaSampleInfo* sampleInfo, LONGLONG& PTS, LONGLONG& TSDelta)
{

  _lastOriginalPTS = sampleInfo->GetSampleTimestamp();

  auto publishoffset = _targetProfileStates[0]->PublishProfile->AudioTimestampBase > 0 ? _targetProfileStates[0]->PublishProfile->AudioTimestampBase + _sampleInterval : 0;
  PTS = _lastOriginalPTS + publishoffset;

  if (_startPTS < 0) //first audio sample
  {
    _startPTS = PTS;
  }

  if (_gaplength > 0)
    _gaplength = 0;

  TSDelta = PTS - _lastPTS;
  _lastPTS = PTS;

  /*LOG("AudioStreamSink" << (IsAggregating() ? "(Agg)" : "")
  << "::PTS = "<< "[" << _lastOriginalPTS << "] "<< ToRTMPTimestamp(_lastOriginalPTS)
  << ",C. PTS = "<< "[" << PTS << "] " << ToRTMPTimestamp(PTS)
  << ", Size = "<< sampleInfo->GetTotalDataLength() << " bytes");*/
}

std::vector<BYTE> RTMPAudioStreamSink::MakeAudioSpecificConfig(MediaSampleInfo* pSampleInfo)
{
  //has the media type been updated by the encoder with the config info ?
  std::vector<BYTE> retval;


  try
  {
    std::vector<BYTE> tempval;
    unsigned int blobsize = 0;
    ThrowIfFailed(_currentMediaType->GetBlobSize(MF_MT_USER_DATA, &blobsize));
    tempval.resize(blobsize);
    ThrowIfFailed(_currentMediaType->GetBlob(MF_MT_USER_DATA, &(*(tempval.begin())), (unsigned int)tempval.size(), &blobsize));

    //this corrsponds to the portion of HEAACWAVEINFO past the WAVEFORMATEX (wfx) member. So we drop the next 5 members (payload type (WORD), profile level ind(WORD), struct type(WORD), reserved1(WORD) and reserved 2(DWORD)) and take the rest - this will be the audiospecificconfig 
    //see 
    if (tempval.size() > 12)
    {
      retval.resize(blobsize - 12);
      memcpy_s(&(*(retval.begin())), blobsize - 12, &(*(tempval.begin() + 12)), blobsize - 12);
      return retval;
    }

  }
  catch (...)
  {
    retval.resize(0);
  }

  //if not we build it by hand


  BYTE SFI_CC;
  if (_samplingFrequencyIndex.find(_mediasinkparent->GetProfileState()->PublishProfile->TargetEncodingProfile->Audio->SampleRate) != _samplingFrequencyIndex.end())
  {
    SFI_CC = (_samplingFrequencyIndex[_mediasinkparent->GetProfileState()->PublishProfile->TargetEncodingProfile->Audio->SampleRate] << 4);//
  }
  else
    SFI_CC = (0x4 << 4);//assume sampling frequency index for 44.1 khz = 0x4

  SFI_CC |= (BYTE)_mediasinkparent->GetProfileState()->PublishProfile->TargetEncodingProfile->Audio->ChannelCount;

  BYTE gaSpecificConfig = 0x0; //frame length flag = 0 for frame length = 1024 PCM samples per ISO 14496-3,all other fields are zero as well

  retval.push_back(SFI_CC);
  retval.push_back(gaSpecificConfig);

  return retval;
}


HRESULT RTMPAudioStreamSink::CompleteProcessNextWorkitem(IMFAsyncResult *pAsyncResult)
{
  std::lock_guard<std::recursive_mutex> lock(_lockSink);

  if (!IsState(SinkState::RUNNING)) //drop the sample
  {
    pAsyncResult->SetStatus(S_OK);
    return S_OK;
  }


  bool firstpacket = false;
  LONGLONG PTS = 0;
  LONGLONG DTS = 0;
  LONGLONG TSDelta = 0;


  //ComPtr<WorkItem> workitem;
  //  ThrowIfFailed(pAsyncResult->GetState(&workitem));
  ComPtr<WorkItem> workitem((WorkItem*)pAsyncResult->GetStateNoAddRef());
  if (workitem == nullptr)
    throw E_INVALIDARG;


  if (workitem->GetWorkItemInfo()->GetWorkItemType() == WorkItemType::MEDIASAMPLE)
  {
    auto sampleInfo = static_cast<MediaSampleInfo*>(workitem->GetWorkItemInfo().get());
    auto sampleTimestamp = sampleInfo->GetSampleTimestamp();


    firstpacket = (_startPTS < 0); //first video packet 

    PrepareTimestamps(sampleInfo, PTS, TSDelta);

    unsigned int uiPTS = ToRTMPTimestamp(PTS);

    if (firstpacket)
    {
      auto audioconfigpayload = PreparePayload(sampleInfo, true);
      auto framepayload = PreparePayload(sampleInfo, false);
      workitem.Reset();

      _mediasinkparent->GetMessenger()->QueueAudioVideoMessage(
        RTMPMessageType::AUDIO,
        uiPTS,
        audioconfigpayload);

     

      _mediasinkparent->GetMessenger()->QueueAudioVideoMessage(
        RTMPMessageType::AUDIO,
        uiPTS,
        framepayload);

    }
    else
    {

      unsigned int uiPTSDelta = ToRTMPTimestamp(TSDelta);
      auto framepayload = PreparePayload(sampleInfo, false);
      workitem.Reset();

      _mediasinkparent->GetMessenger()->QueueAudioVideoMessage(
        RTMPMessageType::AUDIO,
        uiPTS,
        framepayload);
    }

#if defined(_DEBUG)
    LOG("Queued Audio Sample @ " << uiPTS << " - " << _streamsinkname);
#endif

  }
  else //marker
  {
    auto markerInfo = static_cast<MarkerInfo*>(workitem->GetWorkItemInfo().get());

    if (markerInfo->GetMarkerType() == MFSTREAMSINK_MARKER_TYPE::MFSTREAMSINK_MARKER_TICK)
    {
      //_gaplength += _sampleInterval;
      auto tick = markerInfo->GetMarkerTick();
      _gaplength += (tick - _lastOriginalPTS);
      LOG("AudioStreamSink" << (IsAggregating() ? "(Aggregating)" : "") << "::Audio stream gap at : " << tick << ", Last Original PTS : " << _lastOriginalPTS << ", gaplength :" << _gaplength);
      _lastOriginalPTS = tick;

    }
    workitem.Reset();
  }

 
  pAsyncResult->SetStatus(S_OK);

  return S_OK;
}

LONGLONG RTMPAudioStreamSink::GetLastDTS()
{
  if (!IsAggregating())
    return _lastDTS;
  else
  {
    return _targetProfileStates[0]->DelegateSink->GetAudioSink()->GetLastDTS();
  }
}

LONGLONG RTMPAudioStreamSink::GetLastPTS()
{
  if (!IsAggregating())
    return _lastPTS;
  else
  {
    return _targetProfileStates[0]->DelegateSink->GetAudioSink()->GetLastPTS();
  }
}