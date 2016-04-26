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
#include <sstream>
#include "Constants.h"
#include "RTMPMessageFormats.h"
#include "RTMPMessenger.h"
#include "RTMPChunking.h"
#include <agents.h>
#include <cmath>

using namespace Windows::Networking;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;
using namespace std;
using namespace Concurrency;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Microsoft::Media::RTMP;

RTMPMessenger::RTMPMessenger(PublishProfile^ params) :
  _streamSocket(nullptr)
{
  _sessionManager = make_shared<RTMPSessionManager>(params);
}

RTMPMessenger::~RTMPMessenger()
{

}


task<void> RTMPMessenger::ConnectAsync()
{
  _streamSocket = ref new StreamSocket();


  _streamSocket->Control->KeepAlive = true;
  _streamSocket->Control->NoDelay = true;


  return create_task(
    _streamSocket->ConnectAsync(
      ref new HostName(
        ref new String(_sessionManager->GetHostName().data())),
      ref new String(_sessionManager->GetPortNumber().data())
      ));

}

void RTMPMessenger::Disconnect()
{
  delete _streamSocket;
}

task<void> RTMPMessenger::CloseAsync()
{
  return SendUnpublishAndCloseStreamAsync().then([](unsigned int i) { return; });
}

task<void> RTMPMessenger::HandshakeAsync()
{
  if (_sessionManager->GetServerType() == RTMPServerType::Azure)
    return HandshakeAsyncAzure();
  else if (_sessionManager->GetServerType() == RTMPServerType::Wowza)
    return HandshakeAsyncWowza();
  else
    throw std::invalid_argument("Unknown RTMP server type");

}

task<void> RTMPMessenger::HandshakeAsyncAzure()
{
  //LOG("Handshaking with Azure Media Services RTMP Ingest");
  return  SendC0C1Async()
    .then([this](task<unsigned int> antecedent)
  {

    antecedent.get();
    return ReceiveS0S1Async();
  })
    .then([this](task<void> antecedent)
  {

    antecedent.get();
    return SendC2Async();
  })
    .then([this](task<unsigned int> antecedent)
  {

    antecedent.get();
    return ReceiveS2Async();
  })
    .then([this](task<void> antecedent)
  {

    antecedent.get();
    return SendConnectAsync();
  })
    .then([this](task<unsigned int> antecedent)
  {

    antecedent.get();

    return ReceiveConnectResponseAsync();
  })
    .then([this](task<void> antecedent)
  {

    antecedent.get(); 
   
    return SendReleaseStreamAsync();
  })
    .then([this](task<unsigned int> antecedent)
  {

    antecedent.get();
    return ReceiveReleaseStreamResponseAsync();
  })

    /* Send FCPublish and CreateStream in a batch - for some reason in case of MBR 
    sending these interleaved with their  receive functions result in AMS sending 
    back the same Message Stream ID's for all connections*/

    .then([this](task<void> antecedent)
  {
    antecedent.get();
    //return SendFCPublishAsync();
    return SendFCPublishAsync().then([this](task<unsigned int> antecedent)
    {
      antecedent.get();
      return SendCreateStreamAsync();
    });
  })
    .then([this](task<unsigned int> antecedent)
  {

    antecedent.get();
    //return ReceiveFCPublishResponseAsync();
    return ReceiveFCPublishResponseAsync().then([this](task<void> antecedent)
    {
      antecedent.get();
      return  ReceiveCreateStreamResponseAsync();
    });
  })
  /*  .then([this](task<void> antecedent)
  {
    antecedent.get(); 

    return SendCreateStreamAsync();
  })
    .then([this](task<unsigned int> antecedent)
  {
    antecedent.get();
    return  ReceiveCreateStreamResponseAsync();
  })*/
    .then([this](task<void> antecedent)
  {
    antecedent.get();
    return SendPublishStreamAsync();
  })
    .then([this](task<unsigned int> antecedent)
  {

    antecedent.get();
    return ReceivePublishStreamResponseAsync();
  })
    .then([this](task<void> antecedent)
  {

    antecedent.get();

    if (_sessionManager->GetEncodingProfile()->Video == nullptr)
    {
      return SendSetDataFrameAsync(

        L"mp4a",
        _sessionManager->GetEncodingProfile()->Audio->SampleRate,
        _sessionManager->GetEncodingProfile()->Audio->ChannelCount,
        _sessionManager->GetEncodingProfile()->Audio->Bitrate / 1000);
    }
    else if (_sessionManager->GetEncodingProfile()->Audio == nullptr)
    {
      return SendSetDataFrameAsync(
        (double)_sessionManager->GetEncodingProfile()->Video->FrameRate->Numerator / (double)_sessionManager->GetEncodingProfile()->Video->FrameRate->Denominator,
        _sessionManager->GetEncodingProfile()->Video->Width,
        _sessionManager->GetEncodingProfile()->Video->Height,
        L"avc1",
        _sessionManager->GetEncodingProfile()->Video->Bitrate / 1000,
        _sessionManager->GetKeyframeInterval());
    }
    else
      return SendSetDataFrameAsync(
        (double)_sessionManager->GetEncodingProfile()->Video->FrameRate->Numerator / (double)_sessionManager->GetEncodingProfile()->Video->FrameRate->Denominator,
        _sessionManager->GetEncodingProfile()->Video->Width,
        _sessionManager->GetEncodingProfile()->Video->Height,
        L"avc1",
        _sessionManager->GetEncodingProfile()->Video->Bitrate / 1000,
        _sessionManager->GetKeyframeInterval(),
        L"mp4a",
        _sessionManager->GetEncodingProfile()->Audio->SampleRate,
        _sessionManager->GetEncodingProfile()->Audio->ChannelCount,
        _sessionManager->GetEncodingProfile()->Audio->Bitrate / 1000);
  })
    .then([this](task<unsigned int> antecedent)
  {

    antecedent.get();
    return SendSetChunkSizeAsync(_sessionManager->GetClientChunkSize());
  })
    .then([this](task<unsigned int> antecedent)
  {
    antecedent.get();

    _sessionManager->SetVideoChunkStreamID(_sessionManager->GetNextChunkStreamID());
    _sessionManager->SetAudioChunkStreamID(_sessionManager->GetNextChunkStreamID());
    _sessionManager->SetState(RTMPSessionState::RTMP_RUNNING);


  });
}

task<void> RTMPMessenger::HandshakeAsyncWowza()
{
  //LOG("Handshaking with Wowza Media Server RTMP Ingest");
  return  SendC0C1Async()
    .then([this](task<unsigned int> antecedent)
  {

    antecedent.get();
    return ReceiveS0S1Async();
  })
    .then([this](task<void> antecedent)
  {

    antecedent.get();
    return SendC2Async();
  })
    .then([this](task<unsigned int> antecedent)
  {

    antecedent.get();
    return ReceiveS2Async();
  })
    .then([this](task<void> antecedent)
  {

    antecedent.get();
    return SendConnectAsync();
  })
    .then([this](task<unsigned int> antecedent)
  {

    antecedent.get();
    return ReceiveConnectResponseAsync();
  })
    .then([this](task<void> antecedent)
  {

    antecedent.get();
    return SendReleaseStreamAsync();
  })
    .then([this](task<unsigned int> antecedent)
  {
    antecedent.get();
    return SendFCPublishAsync();
  })
    .then([this](task<unsigned int> antecedent)
  {

    antecedent.get();
    return ReceiveFCPublishResponseAsync();
  })
    .then([this](task<void> antecedent)
  {
    antecedent.get(); 
    return SendCreateStreamAsync();
  })
    .then([this](task<unsigned int> antecedent)
  {
    antecedent.get();
    return  ReceiveCreateStreamResponseAsync();
  })
    .then([this](task<void> antecedent)
  {
    antecedent.get();
    return SendPublishStreamAsync();
  })
    .then([this](task<unsigned int> antecedent)
  {

    antecedent.get();
    return ReceivePublishStreamResponseAsync();
  })
    .then([this](task<void> antecedent)
  {

    antecedent.get();
    if (_sessionManager->GetEncodingProfile()->Video == nullptr)
    {
      return SendSetDataFrameAsync(

        L"mp4a",
        _sessionManager->GetEncodingProfile()->Audio->SampleRate,
        _sessionManager->GetEncodingProfile()->Audio->ChannelCount,
        _sessionManager->GetEncodingProfile()->Audio->Bitrate / 1000);
    }
    else if (_sessionManager->GetEncodingProfile()->Audio == nullptr)
    {
      return SendSetDataFrameAsync(
        (double)_sessionManager->GetEncodingProfile()->Video->FrameRate->Numerator / (double)_sessionManager->GetEncodingProfile()->Video->FrameRate->Denominator,
        _sessionManager->GetEncodingProfile()->Video->Width,
        _sessionManager->GetEncodingProfile()->Video->Height,
        L"avc1",
        _sessionManager->GetEncodingProfile()->Video->Bitrate / 1000,
        _sessionManager->GetKeyframeInterval());
    }
    else
      return SendSetDataFrameAsync(
        (double)_sessionManager->GetEncodingProfile()->Video->FrameRate->Numerator / (double)_sessionManager->GetEncodingProfile()->Video->FrameRate->Denominator,
        _sessionManager->GetEncodingProfile()->Video->Width,
        _sessionManager->GetEncodingProfile()->Video->Height,
        L"avc1",
        _sessionManager->GetEncodingProfile()->Video->Bitrate / 1000,
        _sessionManager->GetKeyframeInterval(),
        L"mp4a",
        _sessionManager->GetEncodingProfile()->Audio->SampleRate,
        _sessionManager->GetEncodingProfile()->Audio->ChannelCount,
        _sessionManager->GetEncodingProfile()->Audio->Bitrate / 1000);
  })
    .then([this](task<unsigned int> antecedent)
  {

    antecedent.get();
    return SendSetChunkSizeAsync(_sessionManager->GetClientChunkSize());
  })
    .then([this](task<unsigned int> antecedent)
  {
    antecedent.get();

    _sessionManager->SetVideoChunkStreamID(_sessionManager->GetNextChunkStreamID());
    _sessionManager->SetAudioChunkStreamID(_sessionManager->GetNextChunkStreamID());
    _sessionManager->SetState(RTMPSessionState::RTMP_RUNNING);


  });
}


void RTMPMessenger::QueueAudioVideoMessage(BYTE type,
  unsigned int timestamp,
  std::vector<BYTE>& payload,
  bool useTimestampAsDelta)
{
  

  auto msg = make_shared<RTMPMessage>(
    timestamp,
    (unsigned int)payload.size(),
    type,
    _sessionManager->GetMessageStreamID(),
    (&(*(payload.begin())))
    , useTimestampAsDelta);

  auto chunkedbs = ChunkProcessor::ToChunkedBitstream(
    type == RTMPMessageType::VIDEO ? _sessionManager->GetVideoChunkStreamID() : _sessionManager->GetAudioChunkStreamID(),
    _sessionManager->GetClientChunkSize(),
    msg
    );

  dw->WriteBytes(ref new Array<BYTE>(&(*(chunkedbs.get()->begin())), (unsigned int)chunkedbs->size()));

  try {
    create_task(dw->StoreAsync()).wait();
  }
  catch (COMException^ ex)
  {
    throw;
  }
  catch (...)
  {
    throw;
  }


}


void RTMPMessenger::ProcessQueue()
{
  while (_sessionManager->GetState() == RTMPSessionState::RTMP_RUNNING)
  {
    std::unique_lock<std::mutex> lock(_mtxQueueNotifier);
    _cvQueueNotifier.wait(lock, [this]() {return !_messageQueue.empty(); });

    vector<shared_ptr<RTMPMessage>> items;

    {
      std::lock_guard<std::recursive_mutex> lock(_rmtxMessageQueue);
      while (_messageQueue.empty() == false)
      {
        items.push_back(_messageQueue.front());
        _messageQueue.pop_front();
      }
    }



    for (auto& itemptr : items)
    {

      auto bs = ChunkProcessor::ToChunkedBitstream(
        itemptr->GetMessageTypeID() == RTMPMessageType::VIDEO ? _sessionManager->GetVideoChunkStreamID() : _sessionManager->GetAudioChunkStreamID(),
        _sessionManager->GetClientChunkSize(),
        itemptr);


      dw->WriteBytes(ref new Array<BYTE>(&(*(bs.get()->begin())), (unsigned int)bs->size()));
    }

    try {
      create_task(dw->StoreAsync())
        .then([this](task<unsigned int> antecedent) { antecedent.get(); return dw->FlushAsync(); })
        .then([this](task<bool> antecedent) { antecedent.get(); }).wait();
    }
    catch (Exception^ ex)
    {
      throw ex;
    }
    catch (...)
    {
      throw;
    }

  }
}


task<unsigned int> Microsoft::Media::RTMP::RTMPMessenger::SendC0C1Async()
{
  dw = ref new DataWriter(_streamSocket->OutputStream);
  HandshakeMessageC0S0 c0{ HandshakeMessageC0S0::RTMP_VERSION };
  auto bitstreamc0 = c0.ToBitstream();
  dw->WriteBytes(ref new Array<BYTE>(&(*(bitstreamc0.get()->begin())), (unsigned int)bitstreamc0->size()));


  auto c1 = make_shared<HandshakeMessageC1C2S1S2>(_sessionManager->GetBaseEpoch(), _sessionManager->GetC1RandomBytes());
  auto bitstreamc1 = c1->ToBitstream();
  dw->WriteBytes(ref new Array<BYTE>(&(*(bitstreamc1.get()->begin())), (unsigned int)bitstreamc1->size()));

  return create_task(dw->StoreAsync());
}

task<void> Microsoft::Media::RTMP::RTMPMessenger::ReceiveS0S1Async()
{
  dr = ref new DataReader(_streamSocket->InputStream);
  return create_task(dr->LoadAsync(1537))
    .then([this](task<unsigned int> antecedent)
  {
    antecedent.get();
    //receive S0 & S1
    auto Buff = dr->ReadBuffer(1537);
    auto vec = BitOp::BufferToVector(Buff);

    if (vec.size() == 0)
      throw std::exception("RTMP S0S1 not received");

    auto s0 = HandshakeMessageC0S0::TryParse(&(*(vec.begin())), 1);

    if (s0 == nullptr || s0->GetVersion() != HandshakeMessageC0S0::RTMP_VERSION)
      throw std::exception("RTMP Handshake Failed - Message S0");

    auto s1 = HandshakeMessageC1C2S1S2::TryParse(&(*(vec.begin() + 1)), 1536);

    if (s1 == nullptr)
      throw std::exception("RTMP Handshake Failed : Message S1");

    _sessionManager->SetServerBaseEpoch(s1->GetBaseEpoch());
    _sessionManager->SetS1ParseTimestamp(s1->GetParseTimestamp());
    _sessionManager->SetS1RandomBytes(s1->GetRandomBytes());
  });
}

task<unsigned int> Microsoft::Media::RTMP::RTMPMessenger::SendC2Async()
{
  //send C2 - timestamp will be that sent in S1
  auto c2 = make_shared<HandshakeMessageC1C2S1S2>(_sessionManager->GetServerBaseEpoch(), _sessionManager->GetS1ParseTimestamp(), _sessionManager->GetS1RandomBytes());
  auto bitstream = c2->ToBitstream();
  dw->WriteBytes(ref new Array<BYTE>(&(*(bitstream.get()->begin())), (unsigned int)bitstream->size()));
  return create_task(dw->StoreAsync());
}

task<void> Microsoft::Media::RTMP::RTMPMessenger::ReceiveS2Async()
{
  dr->InputStreamOptions = InputStreamOptions::Partial;

  return create_task(dr->LoadAsync(1537))
    .then([this](task<unsigned int> antecedent)
  {
    unsigned int ret = antecedent.get();

    //receive S2
    auto Buff = dr->ReadBuffer(1536);
    auto vec = BitOp::BufferToVector(Buff);

    if (vec.size() == 0)
      throw std::exception("RTMP S2 not received");

    auto s2 = HandshakeMessageC1C2S1S2::TryParse(&(*(vec.begin())), 1536);

    if (s2 == nullptr || s2->GetBaseEpoch() != _sessionManager->GetBaseEpoch() || !s2->AreRandomBytesEqual(_sessionManager->GetC1RandomBytes()))
      throw std::exception("RTMP Handshake Failed : Message S2");
  });

}

task<unsigned int> Microsoft::Media::RTMP::RTMPMessenger::SendConnectAsync()
{
  //send connect
  std::shared_ptr<std::vector<BYTE>> bs_commandconnect = nullptr;
  if (_sessionManager->GetEncodingProfile()->Audio == nullptr)
  {
    bs_commandconnect = ChunkProcessor::ToChunkedBitstream(
      _sessionManager->GetNextChunkStreamID(),
      _sessionManager->GetDefaultChunkSize(),
      make_shared<Command_Connect>(_sessionManager->GetNextTransactionID(),
        _sessionManager->GetServerAppName(),
        _sessionManager->GetRTMPUri(), 
        _sessionManager->GetSupportedRTMPVideoCodecFlags(),false));
  }
  else if (_sessionManager->GetEncodingProfile()->Video == nullptr)
  {
    bs_commandconnect = ChunkProcessor::ToChunkedBitstream(
      _sessionManager->GetNextChunkStreamID(),
      _sessionManager->GetDefaultChunkSize(),
      make_shared<Command_Connect>(_sessionManager->GetNextTransactionID(),
        _sessionManager->GetServerAppName(),
        _sessionManager->GetRTMPUri(),
        _sessionManager->GetSupportedRTMPAudioCodecFlags(),
        true));
  }
  else
  {
    bs_commandconnect = ChunkProcessor::ToChunkedBitstream(
      _sessionManager->GetNextChunkStreamID(),
      _sessionManager->GetDefaultChunkSize(),
      make_shared<Command_Connect>(_sessionManager->GetNextTransactionID(),
        _sessionManager->GetServerAppName(),
        _sessionManager->GetRTMPUri(),
        _sessionManager->GetSupportedRTMPAudioCodecFlags(),
        _sessionManager->GetSupportedRTMPVideoCodecFlags()));
  }
 

  dw->WriteBytes(ref new Array<BYTE>(&(*(bs_commandconnect.get()->begin())), (unsigned int)bs_commandconnect->size()));
  return create_task(dw->StoreAsync());
}

task<void> Microsoft::Media::RTMP::RTMPMessenger::ReceiveConnectResponseAsync()
{


  return create_task(dr->LoadAsync(3300))
    .then([this](task<unsigned int> antecedent)
  {
    unsigned int ret = antecedent.get();


    auto Buff = dr->ReadBuffer(dr->UnconsumedBufferLength);
    auto vec = BitOp::BufferToVector(Buff);

    if (vec.size() == 0)
      throw std::exception("RTMP Connect : Response not received");

    auto messages = ChunkProcessor::TryParse(&(*(vec.begin())), (unsigned int)vec.size(), (unsigned int)_sessionManager->GetServerChunkSize());

    //read response
    for (auto& msg : messages)
    {
      if (msg->GetMessageTypeID() == RTMPMessageType::PROTOACKWINDOWSIZE)
      {
        _sessionManager->SetAcknowledgementWindowSize((static_cast<ProtoAckWindowSizeMessage*>(msg.get()))->GetWindowSize());
      }
      else if (msg->GetMessageTypeID() == RTMPMessageType::PROTOSETCHUNKSIZE)
      {
        _sessionManager->SetServerChunkSize((static_cast<ProtoSetChunkSizeMessage*>(msg.get()))->GetChunkSize());
      }
      else if (msg->GetMessageTypeID() == RTMPMessageType::PROTOSETPEERBANDWIDTH)
      {
        _sessionManager->SetPeerBandwidthLimit((static_cast<ProtoSetPeerBandwidthMessage*>(msg.get()))->GetBandwidth());
        _sessionManager->SetBandwidthLimitType((static_cast<ProtoSetPeerBandwidthMessage*>(msg.get()))->GetBandwidthLimitType());
      }
      else if (msg->GetMessageTypeID() == RTMPMessageType::COMMANDAMF0)
      {
        auto cmd = (static_cast<AMF0EncodedCommandOrData*>(msg.get()));
        if (cmd->GetCommandName() == L"_error")
        {
          throw std::exception("RTMP Connect failed");
        }

      }
      else if (msg->GetMessageTypeID() == RTMPMessageType::USERCONTROL)
      {
        //do nothing
      }
    }
  });

}


task<unsigned int> Microsoft::Media::RTMP::RTMPMessenger::SendReleaseStreamAsync()
{

  _sessionManager->SetStreamCreateReleaseChunkStreamID(_sessionManager->GetNextChunkStreamID());

  auto bs_commandrelease = ChunkProcessor::ToChunkedBitstream(
    _sessionManager->GetStreamCreateReleaseChunkStreamID(),
    _sessionManager->GetDefaultChunkSize(),
    make_shared<Command_ReleaseStream>(
      _sessionManager->GetNextTransactionID(),
      _sessionManager->GetStreamName()));

  dw->WriteBytes(ref new Array<BYTE>(&(*(bs_commandrelease.get()->begin())), (unsigned int)bs_commandrelease->size()));
  return create_task(dw->StoreAsync());
}

task<void> Microsoft::Media::RTMP::RTMPMessenger::ReceiveReleaseStreamResponseAsync()
{


  return create_task(dr->LoadAsync(500))
    .then([this](task<unsigned int> antecedent)
  {
    antecedent.get();

    auto Buff = dr->ReadBuffer(dr->UnconsumedBufferLength);
    auto vec = BitOp::BufferToVector(Buff);

    if (vec.size() > 0) {

      auto messages = ChunkProcessor::TryParse(&(*(vec.begin())), (unsigned int)vec.size(), (unsigned int)_sessionManager->GetServerChunkSize());

      for (auto& msg : messages)
      {
        if (msg->GetMessageTypeID() == RTMPMessageType::COMMANDAMF0)
        {
          auto cmd = (static_cast<AMF0EncodedCommandOrData*>(msg.get()));
          if (cmd->GetCommandName() == L"_error")
          {
            throw std::exception("RTMP Release Stream response");
          }

        }
      }

    }
  });

}


task<unsigned int> Microsoft::Media::RTMP::RTMPMessenger::SendFCPublishAsync()
{
  auto bs_commandrelease = ChunkProcessor::ToChunkedBitstream(
    _sessionManager->GetStreamCreateReleaseChunkStreamID(),
    _sessionManager->GetDefaultChunkSize(),
    make_shared<Command_FCPublish>(
      _sessionManager->GetNextTransactionID(),
      _sessionManager->GetStreamName()));

  dw->WriteBytes(ref new Array<BYTE>(&(*(bs_commandrelease.get()->begin())), (unsigned int)bs_commandrelease->size()));
  return create_task(dw->StoreAsync());
}

task<void> Microsoft::Media::RTMP::RTMPMessenger::ReceiveFCPublishResponseAsync()
{
  return create_task(dr->LoadAsync(500))
    .then([this](task<unsigned int> antecedent)
  {
    antecedent.get();

    auto Buff = dr->ReadBuffer(dr->UnconsumedBufferLength);
    auto vec = BitOp::BufferToVector(Buff);

    if (vec.size() > 0)
    {
      auto messages = ChunkProcessor::TryParse(&(*(vec.begin())), (unsigned int)vec.size(), (unsigned int)_sessionManager->GetServerChunkSize());

      for (auto& msg : messages)
      {
        if (msg->GetMessageTypeID() == RTMPMessageType::COMMANDAMF0)
        {
          auto cmd = (static_cast<AMF0EncodedCommandOrData*>(msg.get()));
          if (cmd->GetCommandName() == L"_error")
          {
            throw std::exception("RTMP FCPublish response");
          }

        }
      }
    }
  });

}

task<unsigned int> Microsoft::Media::RTMP::RTMPMessenger::SendCreateStreamAsync()
{
  auto bs_commandcreate = ChunkProcessor::ToChunkedBitstream(
    _sessionManager->GetStreamCreateReleaseChunkStreamID(),
    _sessionManager->GetDefaultChunkSize(),
    make_shared<Command_CreateStream>(_sessionManager->GetNextTransactionID()));

  dw->WriteBytes(ref new Array<BYTE>(&(*(bs_commandcreate.get()->begin())), (unsigned int)bs_commandcreate->size()));
  return create_task(dw->StoreAsync());

}


task<void> Microsoft::Media::RTMP::RTMPMessenger::ReceiveCreateStreamResponseAsync()
{
  return create_task(dr->LoadAsync(500))
    .then([this](task<unsigned int> antecedent)
  {
    antecedent.get();

    auto Buff = dr->ReadBuffer(dr->UnconsumedBufferLength);
    auto vec = BitOp::BufferToVector(Buff);
    if (vec.size() == 0)
      throw std::exception("RTMP Create : Response not received");

    auto messages = ChunkProcessor::TryParse(&(*(vec.begin())), (unsigned int)vec.size(), (unsigned int)_sessionManager->GetServerChunkSize());

    for (auto& msg : messages)
    {
      if (msg->GetMessageTypeID() == RTMPMessageType::COMMANDAMF0)
      {
        auto cmd = (static_cast<AMF0EncodedCommandOrData*>(msg.get()));
        if (cmd->GetCommandName() == L"_error")
        {
          throw std::exception("RTMP Create failed");
        }
        else if (cmd->GetCommandName() == L"_result")
        {
          _sessionManager->SetMessageStreamID((unsigned int)cmd->GetAllEntities().back()->GetNumberValue());
        }

      }
    }
  });

}


task<unsigned int> Microsoft::Media::RTMP::RTMPMessenger::SendPublishStreamAsync()
{
  _sessionManager->SetPublishChunkStreamID(_sessionManager->GetNextChunkStreamID());
  auto bs_commandpublish = ChunkProcessor::ToChunkedBitstream(
    _sessionManager->GetPublishChunkStreamID(),
    _sessionManager->GetDefaultChunkSize(),
    make_shared<Command_PublishStream>(
      0,
      _sessionManager->GetMessageStreamID(),
      _sessionManager->GetStreamName(),
      RTMPPublishType::LIVE));

  dw->WriteBytes(ref new Array<BYTE>(&(*(bs_commandpublish.get()->begin())), (unsigned int)bs_commandpublish->size()));
  return create_task(dw->StoreAsync());
}



task<void> Microsoft::Media::RTMP::RTMPMessenger::ReceivePublishStreamResponseAsync()
{
  return create_task(dr->LoadAsync(500))
    .then([this](task<unsigned int> antecedent)
  {
    antecedent.get();

    auto Buff = dr->ReadBuffer(dr->UnconsumedBufferLength);
    auto vec = BitOp::BufferToVector(Buff);
    if (vec.size() == 0)
      throw std::exception("ERROR : RTMP Publish : Response not received");

    auto messages = ChunkProcessor::TryParse(&(*(vec.begin())), (unsigned int)vec.size(), (unsigned int)_sessionManager->GetServerChunkSize());

    for (auto& msg : messages)
    {
      if (msg->GetMessageTypeID() == RTMPMessageType::COMMANDAMF0)
      {
        auto cmd = (static_cast<AMF0EncodedCommandOrData*>(msg.get()));
        if (cmd->GetCommandName() == L"_error")
        {
          throw std::exception("ERROR : RTMP Publish failed");
        }
        else if (cmd->GetCommandName() == L"onStatus")
        {

        }

      }
      else if (msg->GetMessageTypeID() == RTMPMessageType::USERCONTROL)
      {
        //do nothing
      }
    }
  });

}


task<unsigned int> Microsoft::Media::RTMP::RTMPMessenger::SendUnpublishAndCloseStreamAsync()
{
  auto bs_commandunpublish = ChunkProcessor::ToChunkedBitstream(
    _sessionManager->GetPublishChunkStreamID(),
    _sessionManager->GetClientChunkSize(),
    make_shared<Command_UnpublishStream>(
      0,
      _sessionManager->GetMessageStreamID()));

  dw->WriteBytes(ref new Array<BYTE>(&(*(bs_commandunpublish.get()->begin())), (unsigned int)bs_commandunpublish->size()));

  auto bs_commandclose = ChunkProcessor::ToChunkedBitstream(
    _sessionManager->GetPublishChunkStreamID(),
    _sessionManager->GetClientChunkSize(),
    make_shared<Command_CloseStream>(
      0,
      _sessionManager->GetMessageStreamID()));

  dw->WriteBytes(ref new Array<BYTE>(&(*(bs_commandclose.get()->begin())), (unsigned int)bs_commandclose->size()));
  return create_task(dw->StoreAsync());
}

task<void> Microsoft::Media::RTMP::RTMPMessenger::ReceiveUnpublishStreamResponseAsync()
{
  return create_task(dr->LoadAsync(500))
    .then([this](task<unsigned int> antecedent)
  {
    antecedent.get();

    auto Buff = dr->ReadBuffer(dr->UnconsumedBufferLength);
    auto vec = BitOp::BufferToVector(Buff);

    auto messages = ChunkProcessor::TryParse(&(*(vec.begin())), (unsigned int)vec.size(), (unsigned int)_sessionManager->GetServerChunkSize());

    for (auto& msg : messages)
    {
      if (msg->GetMessageTypeID() == RTMPMessageType::COMMANDAMF0)
      {
        auto cmd = (static_cast<AMF0EncodedCommandOrData*>(msg.get()));
        if (cmd->GetCommandName() == L"onStatus")
        {

        }

      }

    }
  });

}

task<unsigned int> Microsoft::Media::RTMP::RTMPMessenger::SendSetDataFrameAsync(double frameRate,
  unsigned int width,
  unsigned int height,
  wstring videoCodec,
  unsigned int videoBitrate,
  unsigned int videoKeyFrameIntervalTime,
  wstring audioCodec,
  unsigned int audioSampleRate,
  unsigned int audioChannelCount,
  unsigned int audioBitrate)
{
  auto bs_commandDataFrame = ChunkProcessor::ToChunkedBitstream(
    _sessionManager->GetNextChunkStreamID(),
    _sessionManager->GetDefaultChunkSize(),
    make_shared<Command_SetDataFrame>(
      _sessionManager->GetNextTransactionID(),
      _sessionManager->GetMessageStreamID(), frameRate,
      width,
      height,
      videoCodec,
      videoBitrate,
      videoKeyFrameIntervalTime,
      audioCodec,
      audioSampleRate,
      audioChannelCount,
      audioBitrate

      ));

  dw->WriteBytes(ref new Array<BYTE>(&(*(bs_commandDataFrame.get()->begin())), (unsigned int)bs_commandDataFrame->size()));
  return create_task(dw->StoreAsync());
}


task<unsigned int> Microsoft::Media::RTMP::RTMPMessenger::SendSetDataFrameAsync(double frameRate,
  unsigned int width,
  unsigned int height,
  wstring videoCodec,
  unsigned int videoBitrate,
  unsigned int videoKeyFrameIntervalTime)
{
  auto bs_commandDataFrame = ChunkProcessor::ToChunkedBitstream(
    _sessionManager->GetNextChunkStreamID(),
    _sessionManager->GetDefaultChunkSize(),
    make_shared<Command_SetDataFrame>(
      _sessionManager->GetNextTransactionID(),
      _sessionManager->GetMessageStreamID(), frameRate,
      width,
      height,
      videoCodec,
      videoBitrate,
      videoKeyFrameIntervalTime 
      ));

  dw->WriteBytes(ref new Array<BYTE>(&(*(bs_commandDataFrame.get()->begin())), (unsigned int)bs_commandDataFrame->size()));
  return create_task(dw->StoreAsync());
}

task<unsigned int> Microsoft::Media::RTMP::RTMPMessenger::SendSetDataFrameAsync( 
  wstring audioCodec,
  unsigned int audioSampleRate,
  unsigned int audioChannelCount,
  unsigned int audioBitrate)
{
  auto bs_commandDataFrame = ChunkProcessor::ToChunkedBitstream(
    _sessionManager->GetNextChunkStreamID(),
    _sessionManager->GetDefaultChunkSize(),
    make_shared<Command_SetDataFrame>(
      _sessionManager->GetNextTransactionID(),
      _sessionManager->GetMessageStreamID(),  
      audioCodec,
      audioSampleRate,
      audioChannelCount,
      audioBitrate

      ));

  dw->WriteBytes(ref new Array<BYTE>(&(*(bs_commandDataFrame.get()->begin())), (unsigned int)bs_commandDataFrame->size()));
  return create_task(dw->StoreAsync());
}
task<unsigned int> Microsoft::Media::RTMP::RTMPMessenger::SendSetChunkSizeAsync(unsigned int ChunkSize)
{
  auto bs_SetChunkSize = ChunkProcessor::ToChunkedBitstream(
    _sessionManager->GetNextChunkStreamID(),
    _sessionManager->GetDefaultChunkSize(),
    make_shared<ProtoSetChunkSizeMessage>(
      ChunkSize
      ));

  dw->WriteBytes(ref new Array<BYTE>(&(*(bs_SetChunkSize.get()->begin())), (unsigned int)bs_SetChunkSize->size()));
  return create_task(dw->StoreAsync());
}




