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
#include <algorithm>
#include <windows.storage.h>
#include <windows.storage.streams.h>
#include <windows.system.threading.h>


#include "PublishProfile.h"
#include "RTMPPublisherSink.h"
#include "RTMPAudioStreamSink.h"
#include "RTMPVideoStreamSink.h"
#include "RTMPPublishSession.h"
#include "ProfileState.h"
#include "SinkWriterCallbackImpl.h"

using namespace Microsoft::WRL;
using namespace Windows::Media::MediaProperties;
using namespace ABI::Windows::Foundation::Collections;
using namespace Microsoft::Media::RTMP;
using namespace concurrency;
using namespace Windows::System::Threading;

IFACEMETHODIMP RTMPPublisherSink::SetProperties(IPropertySet *props)
{
  return S_OK;
}



/** IMFMediaSink implementation **/
IFACEMETHODIMP RTMPPublisherSink::AddStreamSink(DWORD dwStreamSinkIdentifier, IMFMediaType *pMediaType, IMFStreamSink **ppStreamSink)
{

  if (IsState(SinkState::SHUTDOWN))
    return MF_E_SHUTDOWN;

  return MF_E_STREAMSINKS_FIXED;
}

IFACEMETHODIMP RTMPPublisherSink::GetCharacteristics(DWORD *pdwCharacteristics)
{


  *pdwCharacteristics = MEDIASINK_FIXED_STREAMS | MEDIASINK_RATELESS;

  return S_OK;
}

IFACEMETHODIMP RTMPPublisherSink::GetPresentationClock(IMFPresentationClock **ppPresentationClock)
{
  std::lock_guard<std::recursive_mutex> lock(SinkMutex);



  if (IsState(SinkState::SHUTDOWN))
    return MF_E_SHUTDOWN;

  return _clock.Get()->QueryInterface(IID_PPV_ARGS(ppPresentationClock));
}

IFACEMETHODIMP RTMPPublisherSink::GetStreamSinkById(DWORD dwStreamSinkIdentifier, IMFStreamSink **ppStreamSink)
{

  std::lock_guard<std::recursive_mutex> lock(SinkMutex);

  if (IsState(SinkState::SHUTDOWN))
    return MF_E_SHUTDOWN;

  if (dwStreamSinkIdentifier > 1)
    return MF_E_INVALIDSTREAMNUMBER;

  try
  {
    if (dwStreamSinkIdentifier == 0)
    {
      if (_audioStreamSink == nullptr && _videoStreamSink == nullptr)
        return E_FAIL;
      else if (_audioStreamSink == nullptr)
        ThrowIfFailed(_videoStreamSink.Get()->QueryInterface(IID_PPV_ARGS(ppStreamSink)));
      else
        ThrowIfFailed(_audioStreamSink.Get()->QueryInterface(IID_PPV_ARGS(ppStreamSink)));
    }
    else if (dwStreamSinkIdentifier == 1)
    {
      if (_videoStreamSink == nullptr)
        return E_FAIL;
      else
        ThrowIfFailed(_videoStreamSink.Get()->QueryInterface(IID_PPV_ARGS(ppStreamSink)));
    }
  }
  catch (const HRESULT& hr)
  {
    return hr;
  }

  return S_OK;
}

IFACEMETHODIMP RTMPPublisherSink::GetStreamSinkByIndex(DWORD  dwIndex, IMFStreamSink **ppStreamSink)
{
  std::lock_guard<std::recursive_mutex> lock(SinkMutex);



  if (IsState(SinkState::SHUTDOWN))
    return MF_E_SHUTDOWN;

  if (dwIndex > 1)
    return MF_E_INVALIDINDEX;

  try
  {
    if (dwIndex == 0)
    {
      if (_audioStreamSink == nullptr && _videoStreamSink == nullptr)
        return E_FAIL;
      else if (_audioStreamSink == nullptr)
        ThrowIfFailed(_videoStreamSink.Get()->QueryInterface(IID_PPV_ARGS(ppStreamSink)));
      else
        ThrowIfFailed(_audioStreamSink.Get()->QueryInterface(IID_PPV_ARGS(ppStreamSink)));

    }
    else if (dwIndex == 1)
    {
      if (_videoStreamSink == nullptr)
        return E_FAIL;
      else
        ThrowIfFailed(_videoStreamSink.Get()->QueryInterface(IID_PPV_ARGS(ppStreamSink)));
    }
  }
  catch (const HRESULT& hr)
  {
    return hr;
  }


  return S_OK;
}

IFACEMETHODIMP RTMPPublisherSink::GetStreamSinkCount(DWORD *pcStreamSinkCount)
{
  HRESULT hr = S_OK;
  std::lock_guard<std::recursive_mutex> lock(SinkMutex);

  if (_audioStreamSink != nullptr && _videoStreamSink != nullptr)
    *pcStreamSinkCount = 2;
  else
    *pcStreamSinkCount = 1;

  return hr;
}

IFACEMETHODIMP RTMPPublisherSink::RemoveStreamSink(DWORD dwStreamSinkIdentifier)
{
  std::lock_guard<std::recursive_mutex> lock(SinkMutex);

  if (IsState(SinkState::SHUTDOWN))
    return MF_E_SHUTDOWN;

  return MF_E_STREAMSINKS_FIXED;
}

IFACEMETHODIMP RTMPPublisherSink::SetPresentationClock(IMFPresentationClock *pPresentationClock)
{

  LOG("RTMPPublisherSink" << (IsAggregating() ? "(Aggregating)" : (IsAggregated() ? "(Child)" : "")) << "::SetPresentationClock()");

  if (IsState(SinkState::SHUTDOWN))
    return MF_E_SHUTDOWN;

  std::lock_guard<std::recursive_mutex> lock(SinkMutex);

  _clock = pPresentationClock;

  //if this is a parent sink we also set the input types of the children sink writers 
  //to match the output types of the parent sink as set by the pipeline

  if (_audioStreamSink != nullptr)
  {
    _audioStreamSink->SetClock(_clock.Get());

    if (IsAggregating())
    {
      DWORD audioSinkIndex = 0;
      ComPtr<IMFMediaType> mtype;
      _audioStreamSink->GetCurrentMediaType(&mtype);

      for (auto profstate : _targetProfileStates)
      {        
        ThrowIfFailed(profstate->DelegateSink->GetStreamSinkIndex(MFMediaType_Audio, audioSinkIndex));     
        ThrowIfFailed(profstate->DelegateWriter->SetInputMediaType(audioSinkIndex, mtype.Get(), nullptr));

      }
 
    }
  }

  if (_videoStreamSink != nullptr)
  {
    _videoStreamSink->SetClock(_clock.Get());
    if (IsAggregating())
    {
      DWORD videoSinkIndex = 0;
      ComPtr<IMFMediaType> mtype;
      _videoStreamSink->GetCurrentMediaType(&mtype);

      for (auto profstate : _targetProfileStates)
      {       
        ThrowIfFailed(profstate->DelegateSink->GetStreamSinkIndex(MFMediaType_Video, videoSinkIndex));      
        ThrowIfFailed(profstate->DelegateWriter->SetInputMediaType(videoSinkIndex, mtype.Get(), nullptr));

      }
 
    }
  }

  _clock->AddClockStateSink(this);

  return S_OK;
}

IFACEMETHODIMP RTMPPublisherSink::Shutdown()
{

  std::unique_lock<std::recursive_mutex> plock(GetParentSink()->SinkMutex, std::defer_lock);
  std::unique_lock<std::recursive_mutex> lock(SinkMutex, std::defer_lock);

  if (IsAggregated())
    plock.lock();
  else
    lock.lock();

  if (IsAggregating())
  {
    for (auto prof : _targetProfileStates)
    {
      prof->DelegateWriter->Finalize();
      prof->DelegateWriter.Reset();
      prof->DelegateSink.Reset();
    }
 
  }

  if (dxgimgr != nullptr)
    MFUnlockDXGIDeviceManager();

  LOG("RTMPPublisherSink" << (IsAggregating() ? "(Aggregating)" : (IsAggregated() ? "(Child)" : "")) << "::Shutdown()");

  SetState(SinkState::SHUTDOWN);

  if (_sampleProcessingQueue != 0)
    MFUnlockWorkQueue(_sampleProcessingQueue);

  return S_OK;
}

IFACEMETHODIMP RTMPPublisherSink::OnClockStart(MFTIME hnsSystemTime, LONGLONG llClockStartOffset)
{
  std::lock_guard<std::recursive_mutex> lock(SinkMutex);

  LOG("RTMPPublisherSink" << (IsAggregating() ? "(Aggregating)" : (IsAggregated() ? "(Child)" : "")) << "::OnClockStart() : " << llClockStartOffset);

  SetState(SinkState::RUNNING);

  try
  {

    if (_videoStreamSink != nullptr)
      _videoStreamSink->StreamSinkClockStart(hnsSystemTime, llClockStartOffset);
    if (_audioStreamSink != nullptr)
      _audioStreamSink->StreamSinkClockStart(hnsSystemTime, llClockStartOffset);

    if (IsAggregating())
    {
      for (auto profstate : _targetProfileStates)
      {
        ThrowIfFailed(profstate->DelegateWriter->BeginWriting());
      }
    }
 

  }
  catch (const std::exception& ex)
  {
    wostringstream s;
    s << L"FAILED: (RTMPPublisherSink::OnClockStart) : " << ex.what();
    _session->RaisePublishFailed(ref new FailureEventArgs(E_FAIL, ref new String(s.str().data())));
    return E_FAIL;
  }
  catch (const HRESULT& hr)
  {
    _session->RaisePublishFailed(ref new FailureEventArgs(hr, L"FAILED: RTMPPublisherSink::OnClockStart"));
    return E_FAIL;
  }
  catch (...)
  {
    _session->RaisePublishFailed(ref new FailureEventArgs(E_FAIL, L"FAILED: RTMPPublisherSink::OnClockStart"));
    return E_FAIL;
  }

  return S_OK;
}

IFACEMETHODIMP RTMPPublisherSink::OnClockStop(MFTIME hnsSystemTime)
{
  std::unique_lock<std::recursive_mutex> plock(GetParentSink()->SinkMutex, std::defer_lock);
  std::unique_lock<std::recursive_mutex> lock(SinkMutex, std::defer_lock);

  if (IsAggregated())
    plock.lock();
  else
    lock.lock();

  LOG("RTMPPublisherSink" << (IsAggregating() ? "(Aggregating)" : (IsAggregated() ? "(Child)" : "")) << "::OnClockStop()");

  SetState(SinkState::STOPPED);

  try
  {


    if (_audioStreamSink != nullptr)
      _audioStreamSink->StreamSinkClockStop(hnsSystemTime);
    if (_videoStreamSink != nullptr)
      _videoStreamSink->StreamSinkClockStop(hnsSystemTime);

    if (IsStandalone() || IsAggregating())
      _session->Close();
    else
    {
      Shutdown(); //call shutdown on child sink
      GetParentSink()->StopPresentationClock();
    }


  }
  catch (...)
  {
    return E_FAIL;
  }


  return S_OK;
}

IFACEMETHODIMP RTMPPublisherSink::OnClockPause(MFTIME hnsSystemTime)
{
  std::lock_guard<std::recursive_mutex> lock(SinkMutex);


  return S_OK;
}

IFACEMETHODIMP RTMPPublisherSink::OnClockRestart(MFTIME hnsSystemTime)
{
  HRESULT hr = S_OK;

  return hr;
}

IFACEMETHODIMP RTMPPublisherSink::OnClockSetRate(MFTIME hnsSystemTime, float flRate)
{
  HRESULT hr = S_OK;

  return hr;
}


void RTMPPublisherSink::StopPresentationClock()
{
  bool initiateshutdown = false;

  std::unique_lock<std::recursive_mutex> plock(GetParentSink()->SinkMutex, std::defer_lock);
  std::unique_lock<std::recursive_mutex> lock(SinkMutex, std::defer_lock);

  if (IsAggregated())
    plock.lock();
  else
    lock.lock();

  if (!IsState(SinkState::RUNNING))
    return;

  LOG("RTMPPublisherSink" << (IsAggregating() ? "(Aggregating)" : (IsAggregated() ? "(Child)" : "")) << "::StopPresentationClock()");

  size_t waitingtoshutdown = std::count_if(begin(_targetProfileStates), end(_targetProfileStates), [this](shared_ptr<ProfileState> ps)
  {
    return ps->DelegateSink == nullptr || (ps->DelegateSink != nullptr && ps->DelegateSink->IsState(SinkState::SHUTDOWN) == false);
  });

  LOGIF(IsAggregating(), "RTMPPublisherSink::Waiting to shut down " << waitingtoshutdown << " children sinks");

  //if aggregating - check to see if all the children sinks have stopped properly
  if (IsAggregating() && waitingtoshutdown > 0)
    return; //not time yet;


  if (_audioStreamSink != nullptr && _videoStreamSink != nullptr && _videoStreamSink->IsState(SinkState::EOS) && _audioStreamSink->IsState(SinkState::EOS))
    initiateshutdown = true;
  else if (_audioStreamSink != nullptr && _videoStreamSink == nullptr && _audioStreamSink->IsState(SinkState::EOS))
    initiateshutdown = true;
  else if (_videoStreamSink != nullptr && _audioStreamSink == nullptr && _videoStreamSink->IsState(SinkState::EOS))
    initiateshutdown = true;

  if (initiateshutdown)
  {
    if (_rtmpMessenger != nullptr)
    {
      _rtmpMessenger->CloseAsync().get();
      _rtmpMessenger->Disconnect();
      _rtmpMessenger.reset();
    }


    MFCLOCK_STATE state;
    if (_clock != nullptr)
    {
      try
      {
        ThrowIfFailed(_clock->GetState(0, &state));
        if (state != MFCLOCK_STATE::MFCLOCK_STATE_INVALID)
          _clock->Stop();
        else if (IsAggregating() && IsState(SinkState::RUNNING))
          OnClockStop(0);
      }
      catch (...)
      {

      }
    }
  }



}


MediaEncodingProfile^ RTMPPublisherSink::CreateInterimProfile(MediaEncodingProfile^ targetProfile)
{

  MediaEncodingProfile^ interimProfile = ref new MediaEncodingProfile();
  if (targetProfile->Video != nullptr)
  {
    interimProfile->Video = VideoEncodingProperties::CreateUncompressed(MediaEncodingSubtypes::Nv12, targetProfile->Video->Width, targetProfile->Video->Height);
    interimProfile->Video->FrameRate->Numerator = targetProfile->Video->FrameRate->Numerator;
    interimProfile->Video->FrameRate->Denominator = targetProfile->Video->FrameRate->Denominator;
    interimProfile->Video->PixelAspectRatio->Numerator = targetProfile->Video->PixelAspectRatio->Numerator;
    interimProfile->Video->PixelAspectRatio->Denominator = targetProfile->Video->PixelAspectRatio->Denominator;
    interimProfile->Video->Bitrate = targetProfile->Video->Bitrate;
  }
  if (targetProfile->Audio != nullptr)
  {
    interimProfile->Audio = targetProfile->Audio;
  }

  return interimProfile;
}
 

void RTMPPublisherSink::Initialize(std::vector<PublishProfile^> targetProfiles,
  VideoEncodingProperties^ interimVideoprofile,
  AudioEncodingProperties^ interimAudioProfile,
  RTMPPublishSession^  session,
  RTMPPublisherSink* aggregatingParentSink
  )
{

  std::lock_guard<std::recursive_mutex> lock(SinkMutex);

  try
  {
    _session = session;

    _targetProfileStates.resize(targetProfiles.size());

    std::transform(begin(targetProfiles), end(targetProfiles), begin(_targetProfileStates), [this](PublishProfile^ prof)
    {
      return make_shared<ProfileState>(prof, nullptr, nullptr);
    });

    if (_targetProfileStates.size() == 1) //Target SBR
    {

      if (aggregatingParentSink != nullptr)
        MakeAggregated(aggregatingParentSink);

      DWORD id = 0;
      if (_targetProfileStates[0]->PublishProfile->TargetEncodingProfile->Audio != nullptr)
        ThrowIfFailed(MakeAndInitialize<RTMPAudioStreamSink>(&_audioStreamSink,
          id++,
          this,
          _targetProfileStates[0]->PublishProfile->TargetEncodingProfile,
          _sampleProcessingQueue,
          _session,
          std::vector<shared_ptr<ProfileState>>{ _targetProfileStates[0] }));
      if (_targetProfileStates[0]->PublishProfile->TargetEncodingProfile->Video != nullptr)
        ThrowIfFailed(MakeAndInitialize<RTMPVideoStreamSink>(&_videoStreamSink,
          id,
          this,
          _targetProfileStates[0]->PublishProfile->TargetEncodingProfile,
          _sampleProcessingQueue,
          _session,
          std::vector<shared_ptr<ProfileState>>{ _targetProfileStates[0] }));
    }
    else //Target MBR
    { 
      MakeAggregating(); 

      MediaEncodingProfile^ interimprofile = ref new MediaEncodingProfile();

      interimprofile->Video = interimVideoprofile;
      interimprofile->Audio = interimAudioProfile;

      if (interimprofile->Video == nullptr || interimprofile->Audio == nullptr)
      {
        std::sort(begin(_targetProfileStates), end(_targetProfileStates), [this](shared_ptr<ProfileState> ps1, shared_ptr<ProfileState> ps2)
        {
          return ps1->PublishProfile->TargetEncodingProfile->Video != nullptr ?
            ps1->PublishProfile->TargetEncodingProfile->Video->Bitrate < ps2->PublishProfile->TargetEncodingProfile->Video->Bitrate :
            ps1->PublishProfile->TargetEncodingProfile->Audio->Bitrate < ps2->PublishProfile->TargetEncodingProfile->Audio->Bitrate;

        });

        auto temp = CreateInterimProfile(_targetProfileStates.back()->PublishProfile->TargetEncodingProfile);
        if (interimprofile->Video == nullptr)
          interimprofile->Video = temp->Video;
        if (interimprofile->Audio == nullptr)
          interimprofile->Audio = temp->Audio;
      }
      
      if (dxgimgr == nullptr)
      {
        UINT dxgimgrtoken = 0;
        ThrowIfFailed(MFLockDXGIDeviceManager(&dxgimgrtoken, &dxgimgr));
      }

      for (auto profstate : _targetProfileStates)
      {


        profstate->DelegateSink = Make<RTMPPublisherSink>();
        profstate->DelegateSinkCallback = Make<RTMPSinkWriterCallbackImpl>();     

        ComPtr<IMFAttributes> cpAttr;
        ThrowIfFailed(MFCreateAttributes(&cpAttr, 1U));
        cpAttr->SetUnknown(MF_SINK_WRITER_ASYNC_CALLBACK, profstate->DelegateSinkCallback.Get());
        cpAttr->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, TRUE);
        cpAttr->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
        cpAttr->SetUnknown(MF_SINK_WRITER_D3D_MANAGER, dxgimgr.Get());

        if(profstate->PublishProfile->EnableLowLatency)
          cpAttr->SetUINT32(MF_LOW_LATENCY, TRUE);

        profstate->DelegateSink->Initialize(std::vector<PublishProfile^>{profstate->PublishProfile}, nullptr,nullptr, session, this);
        ThrowIfFailed(MFCreateSinkWriterFromMediaSink(profstate->DelegateSink.Get(), cpAttr.Get(), &(profstate->DelegateWriter)));

        //We will set the input types of the child sink writers later to match the parent sink's output types
      }

     
      

      DWORD id = 0;

      if (std::find_if(begin(_targetProfileStates), end(_targetProfileStates), [this](shared_ptr<ProfileState> profstate)
      {
        return profstate->PublishProfile->TargetEncodingProfile->Audio != nullptr;
      }) != end(_targetProfileStates))
      {
        ThrowIfFailed(MakeAndInitialize<RTMPAudioStreamSink>(&_audioStreamSink,
          id++,
          this,
          interimprofile,
          _sampleProcessingQueue,
          _session,
          _targetProfileStates));
      }
      if (std::find_if(begin(_targetProfileStates), end(_targetProfileStates), [this](shared_ptr<ProfileState> profstate)
      {
        return profstate->PublishProfile->TargetEncodingProfile->Video != nullptr;
      }) != end(_targetProfileStates))
      {
        ThrowIfFailed(MakeAndInitialize<RTMPVideoStreamSink>(&_videoStreamSink,
          id,
          this,
          interimprofile,
          _sampleProcessingQueue,
          _session,
          _targetProfileStates));
      }
    }

    LOG("RTMPPublisherSink" << (IsAggregating() ? "(Aggregating)" : (IsAggregated() ? "(Child)" : "")) << "::Initialize()");
  }
  catch (const std::exception& ex)
  {
    throw ex;
  }
  catch (const HRESULT& hr)
  {
    throw hr;
  }
  catch (...)
  {
    throw E_FAIL;
  }
  return;
}



task<void> RTMPPublisherSink::ConnectRTMPAsync()
{
  if (!IsAggregating())
  {
    if (_rtmpMessenger == nullptr)
    {
      _rtmpMessenger = make_shared<RTMPMessenger>(_targetProfileStates[0]->PublishProfile);
    }
    return _rtmpMessenger->ConnectAsync().then([this]()
    {
      return _rtmpMessenger->HandshakeAsync();
    });
  }
  else
  {

 /*   return task<void>([this]()
    {
      for (auto itr = _targetProfileStates.begin(); itr != _targetProfileStates.end(); itr++)
      {
        
         
        ((*itr)->DelegateSink->ConnectRTMPAsync()).get();
         
      }
    });*/

    auto connectchain = (*_targetProfileStates.begin())->DelegateSink->ConnectRTMPAsync();

    for (auto itr = _targetProfileStates.begin() + 1; itr != _targetProfileStates.end(); itr++)
    {
      auto temp = connectchain;
      connectchain = temp.then([this, itr](task<void> antecedent)
      {
        antecedent.get();
        return ((*itr)->DelegateSink->ConnectRTMPAsync());
      });
    }

    return connectchain;
  }

}

 
