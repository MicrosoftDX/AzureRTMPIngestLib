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
#include <vector>
#include <algorithm>
#include <windows.media.mediaproperties.h>
#include "PublishProfile.h"
#include "RTMPPublishSession.h"


using namespace Microsoft::Media::RTMP;
using namespace Platform::Collections;
using namespace Windows::Foundation::Collections;

RTMPPublishSession::RTMPPublishSession(IVector<PublishProfile^>^ targetProfiles)
{

  _params = CheckProfiles(targetProfiles);

  _sink = Make<RTMPPublisherSink>();
  try
  {
    _sink->Initialize(_params, nullptr, nullptr, this);
  }
  catch (COMException^ ex)
  {
    throw ex;
  }
  catch (Exception^ ex)
  {
    throw ref new COMException(ex->HResult, ex->Message);
  }
  catch (const std::exception& stdex)
  {
    wstringstream s;
    s << stdex.what();
    throw ref new COMException(E_FAIL, ref new String(s.str().data()));
  }
  catch (const HRESULT& hr)
  {
    throw ref new COMException(hr, L"Failed to initialize capture sink");
  }
  catch (...)
  {
    throw ref new COMException(E_FAIL, L"Failed to initialize capture sink");
  }

}

RTMPPublishSession::RTMPPublishSession(IVector<PublishProfile^>^ targetProfiles, 
  VideoEncodingProperties^ interimVideoProfile, 
  AudioEncodingProperties^ interimAudioProfile)
{

  _params = CheckProfiles(targetProfiles);

  _sink = Make<RTMPPublisherSink>();
  try
  {
    _sink->Initialize(_params, interimVideoProfile, interimAudioProfile, this);
  }
  catch (COMException^ ex)
  {
    throw ex;
  }
  catch (Exception^ ex)
  {
    throw ref new COMException(ex->HResult, ex->Message);
  }
  catch (const std::exception& stdex)
  {
    wstringstream s;
    s << stdex.what();
    throw ref new COMException(E_FAIL, ref new String(s.str().data()));
  }
  catch (const HRESULT& hr)
  {
    throw ref new COMException(hr, L"Failed to initialize capture sink");
  }
  catch (...)
  {
    throw ref new COMException(E_FAIL, L"Failed to initialize capture sink");
  }

}

bool RTMPPublishSession::EnsureUniqueStreamNames(std::vector<PublishProfile^> profiles)
{
  if (profiles.size() <= 1) return true;

  std::vector<String^> streamnames;
  streamnames.resize(profiles.size());
  std::transform(begin(profiles), end(profiles), begin(streamnames), [this](PublishProfile^ prof) { return prof->StreamName; });
  for (auto sn : streamnames)
  {
    if (std::count_if(begin(streamnames), end(streamnames), [this, sn](String^ s) 
    { 
      return Platform::String::CompareOrdinal(sn, s) == 0; 
    }) > 1)
      return false;
  }

  return true;
}
std::vector<PublishProfile^> RTMPPublishSession::CheckProfiles(IVector<PublishProfile^>^ targetProfiles)
{
  //we need at least one target profile to encode to 
  if (targetProfiles->Size == 0)
    throw ref new COMException(E_INVALIDARG, L"At least one publish profile is required");

   

  std::vector<PublishProfile^> retval = std::vector<PublishProfile^>(begin(targetProfiles), end(targetProfiles));

  if(EnsureUniqueStreamNames(retval) == false)
    throw ref new COMException(E_INVALIDARG, L"Stream names need to be unique");

  for (auto prof : retval)
  {
    if (prof == nullptr)
      throw ref new COMException(E_INVALIDARG, L"Publish profile cannot be null");
    if (prof->EndpointUri == nullptr || prof->EndpointUri->Length() == 0)
      throw ref new COMException(E_INVALIDARG, L"Invalid RTMP Endpoint URL");
    if (prof->TargetEncodingProfile == nullptr)
      throw ref new COMException(E_INVALIDARG, L"Encoding profile cannot be null");
    if (prof->TargetEncodingProfile->Audio == nullptr && prof->TargetEncodingProfile->Video == nullptr)
      throw ref new COMException(E_INVALIDARG, L"Encoding profile needs at least audio or video encoding parameters");
  }

  return retval;

}

Windows::Foundation::IAsyncOperation<IMediaExtension^>^ RTMPPublishSession::GetCaptureSinkAsync()
{

  if (_sink == nullptr)
    throw ref new COMException(E_ACCESSDENIED, L"Session cannot be reused. Please create a new session.");

  return Concurrency::create_async([this]()
  {
    return _sink->ConnectRTMPAsync().then([this](task<void> t)
    {
      try
      {
        t.get();

        LOG("RTMP Connected");
        ComPtr<IInspectable> tempInsp(_sink.Get());
        return reinterpret_cast<IMediaExtension^>(tempInsp.Get());
      }
      catch (COMException^ ex)
      {
        throw ex;
      }
      catch (Exception^ ex)
      {
        throw ref new COMException(ex->HResult, ex->Message);
      }
      catch (const std::exception& stdex)
      {
        wstringstream s;
        s << stdex.what();
        throw ref new COMException(E_ACCESSDENIED, ref new String(s.str().data()));
      }
      catch (const HRESULT& hr)
      {
        throw ref new COMException(hr, L"Could not connect to RTMP endpoint");
      }
      catch (...)
      {
        throw ref new COMException(E_ACCESSDENIED, L"Could not connect to RTMP endpoint");
      }
    });
  });
}



void RTMPPublishSession::Close(bool onError)
{
  if (onError)
    _sink->StopPresentationClock();

  auto CloseLogic = [this, onError]()
  {

    LONGLONG _audiolastts = 0;
    LONGLONG _videolastts = 0;
    if (_sink != nullptr)
    {
      if (_sink->GetAudioSink() != nullptr)
        _audiolastts = _sink->GetAudioSink()->GetLastPTS();
      if (_sink->GetVideoSink() != nullptr)
        _videolastts = _sink->GetVideoSink()->GetLastDTS();


      RaiseSessionClosed(ref new SessionClosedEventArgs(_params[0]->EndpointUri, _params.size() > 0 ? "" : _params[0]->StreamName, _videolastts, _audiolastts));

      _sink->Shutdown();
      _sink.Reset();
    }

   
  };

  if (onError)
    CloseLogic();
  else
    create_task(CloseLogic);
}

void RTMPPublishSession::RaisePublishFailed(FailureEventArgs^ args)
{
  if (_sink != nullptr && _sink->IsState(SinkState::RUNNING)) //raise only once
  {
    _sink->SetState(SinkState::EOS);

    create_task([this, args]()
    {

      if (_subcount_publishFailed > 0)
        PublishFailed(this, args);

      Close(true);

    });
  }
}

void RTMPPublishSession::RaiseSessionClosed(SessionClosedEventArgs^ args)
{
  if (_subcount_sessionClosed > 0)
    SessionClosed(this, args);
}



