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
#include <windows.media.mediaproperties.h>
#include <codecapi.h>
#include <limits>
#include <cmath>

#include "WorkItem.h"
#include "RTMPPublisherSink.h"
#include "RTMPVideoStreamSink.h"
#include "RTMPPublishSession.h"
#include "AVCParser.h"
#include "ProfileState.h"

using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Microsoft::Media::RTMP;
using namespace Microsoft::WRL;


HRESULT RTMPVideoStreamSink::CreateMediaType(MediaEncodingProfile^ encodingProfile)
{

  _majorType = MFMediaType_Video;

  try
  {

    //ensure this is audio or video
    if (!encodingProfile->Video->Properties->HasKey(MF_MT_MAJOR_TYPE) || !encodingProfile->Video->Properties->HasKey(MF_MT_SUBTYPE))
      return E_INVALIDARG;

    GUID majorType = safe_cast<IPropertyValue^>(encodingProfile->Video->Properties->Lookup(MF_MT_MAJOR_TYPE))->GetGuid();
    if (majorType != MFMediaType_Video)
      return E_INVALIDARG;

    GUID subType = safe_cast<IPropertyValue^>(encodingProfile->Video->Properties->Lookup(MF_MT_SUBTYPE))->GetGuid();


    if (!IsAggregating() && subType != MFVideoFormat_H264)
      return E_INVALIDARG;


    ThrowIfFailed(ToMediaType(encodingProfile->Video, &(this->_currentMediaType)));

    _currentSubtype = safe_cast<IPropertyValue^>(encodingProfile->Video->Properties->Lookup(MF_MT_SUBTYPE))->GetGuid();

    if (!IsAggregating())
    {
      _sampleInterval = (LONGLONG) round(10000000 / (encodingProfile->Video->FrameRate->Numerator / encodingProfile->Video->FrameRate->Denominator));

      ThrowIfFailed(_currentMediaType->SetUINT32(CODECAPI_AVEncMPVGOPSize, _targetProfileStates[0]->PublishProfile->KeyFrameInterval));
    }

    //Set CBR
    ThrowIfFailed(_currentMediaType->SetUINT32(CODECAPI_AVEncCommonRateControlMode, eAVEncCommonRateControlMode::eAVEncCommonRateControlMode_CBR));


#if defined(_DEBUG)
    _streamsinkname = L"videosink:" + to_wstring((int) encodingProfile->Video->Bitrate);
#endif

    LOG("Video media type created")
      return S_OK;
  }
  catch (const HRESULT& hr)
  {
    return hr;
  }


}



IFACEMETHODIMP RTMPVideoStreamSink::ProcessSample(IMFSample *pSample)
{

  std::lock_guard<std::recursive_mutex> lock(_lockSink);

  try
  {
    if (IsState(SinkState::UNINITIALIZED))
      throw ref new COMException(MF_E_NOT_INITIALIZED, "Video Sink uninitialized");
    if (IsState(SinkState::SHUTDOWN))
      throw ref new COMException(MF_E_SHUTDOWN, "Video Sink has been shut down");
    if (IsState(SinkState::PAUSED) || IsState(SinkState::STOPPED))
      throw ref new COMException(MF_E_INVALIDREQUEST, "Video Sink is in an invalid state");

    if (IsState(SinkState::EOS)) //drop
      return S_OK;


    if (!IsAggregating())
    {
      LONGLONG sampleTime = 0;

      ThrowIfFailed(pSample->GetSampleTime(&sampleTime), MF_E_NO_SAMPLE_TIMESTAMP, L"Could not get timestamp from video sample");

      if (sampleTime < _clockStartOffset) //we do not process samples predating the clock start offset;
      {
        LOG("Not processing video sample")
          return S_OK;
      }


      ComPtr<WorkItem> wi;

      ThrowIfFailed(MakeAndInitialize<WorkItem>(
        &wi,
        make_shared<MediaSampleInfo>(ContentType::VIDEO,
          pSample)
        ));


      ThrowIfFailed(BeginProcessNextWorkitem(wi));

#if defined(_DEBUG)
      LOG("Dispatched video sample - videosink:"<<_streamsinkname);
#endif
    }
    else
    {

      auto msi = make_shared<MediaSampleInfo>(ContentType::VIDEO,
        pSample);

     /* LOG("VideoStreamSink" << (IsAggregating() ? "(Aggregating)" : "") << "::Video Sample : Original PTS = " << (unsigned int) round(msi->GetSampleTimestamp() * TICKSTOMILLIS)
        << ", Original DTS = " << (unsigned int) round(msi->GetDecodeTimestamp() * TICKSTOMILLIS));*/

      for (auto profstate : _targetProfileStates)
      {
        DWORD sinkidx = 0;

        try
        {
          ThrowIfFailed(profstate->DelegateSink->GetStreamSinkIndex(this->_majorType, sinkidx));
          ThrowIfFailed(profstate->DelegateWriter->WriteSample(sinkidx, pSample));
          LOG("Sent video sample to sink writer");
        }
        catch (...)
        {
          LOG("Error sending video sample to sink writer");
          continue;
        }
      }

     

      if (SinkState::RUNNING)
        ThrowIfFailed(NotifyStreamSinkRequestSample());
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

IFACEMETHODIMP RTMPVideoStreamSink::PlaceMarker(MFSTREAMSINK_MARKER_TYPE eMarkerType, const PROPVARIANT *pvarMarkerValue, const PROPVARIANT *pvarContextValue)
{

  std::lock_guard<std::recursive_mutex> lock(_lockSink);

  try
  {

    if (IsState(SinkState::UNINITIALIZED))
      throw ref new COMException(MF_E_NOT_INITIALIZED, "Video Sink uninitialized");
    if (IsState(SinkState::SHUTDOWN))
      throw ref new COMException(MF_E_SHUTDOWN, "Video Sink has been shut down");
    if (IsState(SinkState::PAUSED) || IsState(SinkState::STOPPED))
      throw ref new COMException(MF_E_INVALIDREQUEST, "Video Sink is in an invalid state");

    ComPtr<WorkItem> wi;

    auto markerInfo = make_shared<MarkerInfo>(ContentType::VIDEO,
      eMarkerType,
      pvarMarkerValue,
      pvarContextValue
      );

    if (!IsAggregating())
    {

      if (markerInfo->GetMarkerType() == MFSTREAMSINK_MARKER_TYPE::MFSTREAMSINK_MARKER_ENDOFSEGMENT)
      {
        
        SetState(SinkState::EOS);
        LOG("VideoStreamSink"<<(IsAggregating() ? "(Aggregating)" : "")<<"::Video stream end of segment");
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
        LOG("VideoStreamSink" << (IsAggregating() ? "(Aggregating)" : "") << "::Video stream end of segment");
        NotifyStreamSinkMarker(markerInfo->GetContextValue());

        //if aggregating - the sink will call this on the aggregating parent
      //  create_task([this]() { _mediasinkparent->StopPresentationClock(); });
      }
      else
      {
        LOG("VideoStreamSink" << (IsAggregating() ? "(Aggregating)" : "") << "::Video stream marker");

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

std::vector<BYTE> RTMPVideoStreamSink::PreparePayload(MediaSampleInfo* pSampleInfo, unsigned int compositionTimeOffset, bool firstPacket)
{
  std::vector<BYTE> retval;

  BYTE videodata = pSampleInfo->IsKeyFrame() ? 0x07 /*H.264*/ | (0x01 /* Key Frame */ << 4) //first 4 bits - frame type, last 4 bits - encoding type 
    : 0x07 /*H.264*/ | (0x02 /* Inter Frame */ << 4); //first 4 bits - frame type, last 4 bits - encoding type 

  BitOp::AddToBitstream(videodata, retval, false);

  if (firstPacket)
  {
    //we add AVCPacketType == 0, composition time offset = 0, and decoder config record
    auto decoderconfigrecord = MakeDecoderConfigRecord(pSampleInfo);
    BitOp::AddToBitstream((BYTE) 0, retval, false);
    BitOp::AddToBitstream<unsigned int>((unsigned int) 0, retval, true, 3);

    auto oldsize = retval.size();
    retval.resize(oldsize + decoderconfigrecord.size());
    memcpy_s(&(*(retval.begin() + oldsize)), (unsigned int) decoderconfigrecord.size(), &(*(decoderconfigrecord.begin())), (unsigned int) decoderconfigrecord.size());
  }
  else
  {
    //we add AVCPacketType == 1, composition time offset (3 bytes), and NALU's
    BitOp::AddToBitstream((BYTE) 1, retval, false);
    BitOp::AddToBitstream<unsigned int>(compositionTimeOffset, retval, true, 3);

    auto sample = MakeAVCSample(pSampleInfo);
    auto oldsize = retval.size();
    retval.resize(oldsize + sample.size());
    memcpy_s(&(*(retval.begin() + oldsize)), (unsigned int) sample.size(), &(*(sample.begin())), (unsigned int) sample.size());
  }
  return retval;
}

void RTMPVideoStreamSink::PrepareTimestamps(MediaSampleInfo* sampleInfo, LONGLONG& PTS, LONGLONG& DTS, LONGLONG& TSDelta)
{
  auto originalDTS = sampleInfo->GetDecodeTimestamp();
  auto originalPTS = sampleInfo->GetSampleTimestamp();

  if (originalDTS < 0)
    originalDTS = originalPTS;

  auto publishoffset = _targetProfileStates[0]->PublishProfile->VideoTimestampBase > 0 ?
    _targetProfileStates[0]->PublishProfile->VideoTimestampBase + _sampleInterval : 0;

  if (_startPTS < 0) //first video sample
  {

    //if video timestamp does not start at clock start - we offset it to start at clock start
    if (originalDTS > _clockStartOffset)
    {
      DTS = _clockStartOffset + publishoffset;
      PTS = DTS;
    }
    else
    {
      DTS = originalDTS + publishoffset;
      PTS = originalPTS + publishoffset;
    }

    _startPTS = PTS;

  }
  else
  {

    if (_gaplength > 0)
    {
      auto frameoffset = (originalDTS - _lastOriginalDTS) - _gaplength;     
      DTS = _lastDTS + frameoffset;
      PTS = originalPTS + publishoffset;
      _gaplength = 0;
    }
    else
    {
      DTS = _lastDTS + (originalDTS - _lastOriginalDTS);
      PTS = originalPTS + publishoffset;
    }

  }

  _lastOriginalPTS = originalPTS;
  _lastOriginalDTS = originalDTS;
  TSDelta = DTS - _lastDTS;
  _lastPTS = PTS;
  _lastDTS = DTS;

 /* LOG("VideoStreamSink" << (IsAggregating() ? "(Agg)" : "") 
    << ":: PTS = " << "[" <<_lastOriginalPTS << "] "<<ToRTMPTimestamp(_lastOriginalPTS)
    << ", DTS = " << "[" << _lastOriginalDTS << "] " << ToRTMPTimestamp(_lastOriginalDTS)
    << ", D. PTS = " << "[" << PTS << "] " << ToRTMPTimestamp(PTS)
    << ", D. DTS = " << "[" << DTS << "] " << ToRTMPTimestamp(DTS)
    << ", Delta = " << "[" << TSDelta << "] " << ToRTMPTimestamp(TSDelta)
    << ", C. Offset = " << "[" << PTS - DTS << "] " << ToRTMPTimestamp(PTS - DTS)
    << ", Size = " << sampleInfo->GetTotalDataLength() << " bytes"
    );*/
}

/*
void RTMPVideoStreamSink::PrepareTimestamps(MediaSampleInfo* sampleInfo, LONGLONG& PTS, LONGLONG& DTS, LONGLONG& TSDelta)
{
  _lastOriginalDTS = sampleInfo->GetDecodeTimestamp();
  _lastOriginalPTS = sampleInfo->GetSampleTimestamp();
  if (_lastOriginalDTS < 0)
    _lastOriginalDTS = _lastOriginalPTS;

  if (_startPTS < 0) //first video sample
  {

    if (_publishparams->VideoTimestampBase > 0) //we are trying to continue from a previous publish effort
    {
      DTS = _publishparams->VideoTimestampBase + _sampleInterval;
      PTS = _publishparams->VideoTimestampBase + _sampleInterval;
    }
    else
    {
      //if video timestamp does not start at clock start - we offset it to start at clock start
      if (_lastOriginalDTS > _clockStartOffset)
        DTS = _clockStartOffset;
      else
        DTS = _lastOriginalDTS;

      PTS = _lastOriginalPTS;
    }
    _startPTS = PTS;

  }
  else
  {
    if (_gaplength > 0)
    {
      DTS = _lastDTS + _sampleInterval;
      PTS = _lastPTS + _sampleInterval + _gaplength;
      _gaplength = 0;
    }
    else
    {
      DTS = _lastDTS + _sampleInterval;
      PTS = _lastPTS + _sampleInterval;
    }

  }

  TSDelta = DTS - _lastDTS;
  _lastPTS = PTS;
  _lastDTS = DTS;

  LOG("Video Sample : Original PTS = " << (unsigned int)round(_lastOriginalPTS * TICKSTOMILLIS)
    << ", Original DTS = " << (unsigned int)round(_lastOriginalDTS * TICKSTOMILLIS)
    << ", Derived PTS = " << (unsigned int)round(PTS * TICKSTOMILLIS)
    << ", Derived DTS = " << (unsigned int)round(DTS * TICKSTOMILLIS)
    << ", Composition Offset = " << (unsigned int)round((PTS - DTS) * TICKSTOMILLIS)
    << ", Gap Length = " << (unsigned int)round(_gaplength * TICKSTOMILLIS)
    );
}
*/

std::vector<BYTE> RTMPVideoStreamSink::MakeDecoderConfigRecord(MediaSampleInfo* pSampleInfo)
{
  //has the media type been updated by the encoder with the sequence header ?
  std::vector<BYTE> retval;
  //try
  //{
  //  unsigned int blobsize = 0;
  //  ThrowIfFailed(_currentMediaType->GetBlobSize(MF_MT_MPEG_SEQUENCE_HEADER, &blobsize));
  //  retval.resize(blobsize);
  //  ThrowIfFailed(_currentMediaType->GetBlob(MF_MT_MPEG_SEQUENCE_HEADER, &(*(retval.begin())), (unsigned int)retval.size(), &blobsize));

  //  if (retval.size() > 0)
  //    return retval;
  //}
  //catch (...)
  //{
  //  retval.resize(0);
  //}

  //if not we build it by hand
  retval.push_back(1);

  auto sampledata = pSampleInfo->GetSampleData();
  auto nalus = AVCParser::Parse(sampledata);

  std::vector<BYTE> ppsbs;
  std::vector<BYTE> spsbs;
  BYTE spscount = 0;
  BYTE ppscount = 0;
  BYTE profile_idc = 0;
  BYTE level_idc = 0;
  BYTE profile_compat = 0;

  for (auto& nalu : nalus)
  {
    if (nalu->GetType() == NALUType::NALUTYPE_PPS)
    {
      ++ppscount;

      BitOp::AddToBitstream((unsigned short) nalu->GetLength(), ppsbs);
      auto oldsize = ppsbs.size();
      ppsbs.resize(nalu->GetLength() + oldsize);
      memcpy_s(&(*(ppsbs.begin() + oldsize)), nalu->GetLength(), nalu->GetData(), nalu->GetLength());
    }
    else if (nalu->GetType() == NALUType::NALUTYPE_SPS)
    {
      ++spscount;

      BitOp::AddToBitstream((unsigned short) nalu->GetLength(), spsbs);
      auto oldsize = spsbs.size();
      spsbs.resize(nalu->GetLength() + oldsize);
      memcpy_s(&(*(spsbs.begin() + oldsize)), nalu->GetLength(), nalu->GetData(), nalu->GetLength());

      if (spscount == 1)
      {
        retval.push_back(nalu->GetData()[1]);//profile_idc
        retval.push_back(nalu->GetData()[2]);//profile compat byte
        retval.push_back(nalu->GetData()[3]);//level_idc
      }

    }
  }

  ppsbs.insert(ppsbs.begin(), ppscount);
  spscount |= 0xE0; //3 bits '111'
  spsbs.insert(spsbs.begin(), spscount);

  retval.push_back(255);//lengthSizeMinusOne = 255. Assuming 4 bytes to represent nalu length - 1('11') + 6 bits of '111111' = '111111111'

  auto oldsize = retval.size();
  retval.resize(spsbs.size() + oldsize);
  memcpy_s(&(*(retval.begin() + oldsize)), (unsigned int) spsbs.size(), &(*(spsbs.begin())), (unsigned int) spsbs.size());

  oldsize = retval.size();
  retval.resize(ppsbs.size() + oldsize);
  memcpy_s(&(*(retval.begin() + oldsize)), (unsigned int) ppsbs.size(), &(*(ppsbs.begin())), (unsigned int) ppsbs.size());

  return retval;
}

std::vector<BYTE> RTMPVideoStreamSink::MakeAVCSample(MediaSampleInfo* pSampleInfo)
{

  std::vector<BYTE> retval;


  auto sampledata = pSampleInfo->GetSampleData();
  auto nalus = AVCParser::Parse(sampledata);

  for (auto& nalu : nalus)
  {


    unsigned int nalusize = nalu->GetLength();
    BitOp::AddToBitstream(nalusize, retval); //add the length field as 4 bytes

    auto oldsize = retval.size();
    auto newsize = nalusize + oldsize;

    retval.resize(newsize);
    memcpy_s(&(*(retval.begin() + oldsize)), nalusize, nalu->GetData(), nalusize);
  }

  return retval;
}



HRESULT RTMPVideoStreamSink::CompleteProcessNextWorkitem(IMFAsyncResult *pAsyncResult)
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


 /* ComPtr<WorkItem> workitem;
  ThrowIfFailed(pAsyncResult->GetState(&workitem));*/
  ComPtr<WorkItem> workitem((WorkItem*)pAsyncResult->GetStateNoAddRef());
  if (workitem == nullptr)
    throw E_INVALIDARG;


  if (workitem->GetWorkItemInfo()->GetWorkItemType() == WorkItemType::MEDIASAMPLE)
  {
    auto sampleInfo = static_cast<MediaSampleInfo*>(workitem->GetWorkItemInfo().get());
    auto sampleTimestamp = sampleInfo->GetSampleTimestamp();


    firstpacket = (_startPTS < 0); //first video packet 

    PrepareTimestamps(sampleInfo, PTS, DTS, TSDelta);

    unsigned int uiPTS = ToRTMPTimestamp(PTS);
    unsigned int uiDTS = ToRTMPTimestamp(DTS);

    unsigned int compositiontimeoffset = uiPTS - uiDTS;

    if (firstpacket)
    {
      auto decoderconfigpayload = PreparePayload(sampleInfo, compositiontimeoffset, true);

      _mediasinkparent->GetMessenger()->QueueAudioVideoMessage(
        RTMPMessageType::VIDEO,
        uiDTS,
        decoderconfigpayload);

      auto framepayload = PreparePayload(sampleInfo, compositiontimeoffset, false);

      _mediasinkparent->GetMessenger()->QueueAudioVideoMessage(
        RTMPMessageType::VIDEO,
        uiDTS,
        framepayload);

    }
    else
    {

      unsigned int uiTSDelta = ToRTMPTimestamp(TSDelta);

      auto framepayload = PreparePayload(sampleInfo, compositiontimeoffset, false);

    /*  _mediasinkparent->GetMessenger()->QueueAudioVideoMessage(
        RTMPMessageType::VIDEO,
        uiTSDelta,
        framepayload,
        true);*/

      _mediasinkparent->GetMessenger()->QueueAudioVideoMessage(
        RTMPMessageType::VIDEO,
        uiDTS,
        framepayload);

    }



#if defined(_DEBUG)
    LOG("Queued Video Sample @ " << uiDTS << " - " << _streamsinkname);
#endif

    if (SinkState::RUNNING)
      ThrowIfFailed(NotifyStreamSinkRequestSample());
  }
  else //marker
  {
    auto markerInfo = static_cast<MarkerInfo*>(workitem->GetWorkItemInfo().get());

    if (markerInfo->GetMarkerType() == MFSTREAMSINK_MARKER_TYPE::MFSTREAMSINK_MARKER_TICK)
    {
      //_gaplength += _sampleInterval; 
      auto tick = markerInfo->GetMarkerTick();
      _gaplength += (tick - _lastOriginalPTS);

      LOG("VideoStreamSink" << (IsAggregating() ? "(Aggregating)" : "") << "::Video stream gap at : " << tick << ", Last Original PTS : " << _lastOriginalPTS << ", gaplength :" << _gaplength);

      _lastOriginalPTS = tick;

    }

    ThrowIfFailed(NotifyStreamSinkMarker(markerInfo->GetContextValue()));

  }

  workitem.Reset();
  return S_OK;
}


LONGLONG RTMPVideoStreamSink::GetLastDTS()
{
  if (!IsAggregating())
    return _lastDTS;
  else
  {
    return _targetProfileStates[0]->DelegateSink->GetVideoSink()->GetLastDTS();
  }
}

LONGLONG RTMPVideoStreamSink::GetLastPTS()
{
  if (!IsAggregating())
    return _lastPTS;
  else
  {
    return _targetProfileStates[0]->DelegateSink->GetVideoSink()->GetLastPTS();
  }
}
