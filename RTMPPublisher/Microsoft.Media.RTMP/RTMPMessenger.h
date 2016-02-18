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

#include <windows.foundation.h>
#include <windows.media.mediaproperties.h>
#include <windows.networking.h>
#include <windows.networking.sockets.h>
#include <windows.storage.streams.h>
#include <vector>
#include <string>
#include <ppltasks.h>
#include <deque>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include "PublishProfile.h"
#include "RTMPMessageFormats.h" 
#include "RTMPSessionManager.h"
#include "RTMPChunking.h"
#include "Uri.h"


using namespace Windows::Networking;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;
using namespace std;
using namespace Concurrency;
using namespace Platform;

namespace Microsoft
{
  namespace Media
  {
    namespace RTMP
    {

      class RTMPMessenger
      {

      public:

        RTMPMessenger(PublishProfile^ params);

        virtual ~RTMPMessenger();


        task<void> ConnectAsync();

        void Disconnect();

        task<void> HandshakeAsync();

        task<void> CloseAsync();


        void QueueAudioVideoMessage(BYTE type, 
          unsigned int timestamp, 
          std::vector<BYTE>& payload, 
          bool useTimestampAsDelta = false);

        shared_ptr<RTMPSessionManager> GetSessionManager()
        {
          return _sessionManager;
        }

      private:

        std::shared_ptr<RTMPSessionManager> _sessionManager;

        StreamSocket^ _streamSocket;

        DataReader^ dr = nullptr;

        DataWriter^ dw = nullptr;

        std::vector<std::tuple<unsigned int, unsigned int>> _mstocs;

        std::deque<std::shared_ptr<RTMPMessage>> _messageQueue;

        std::recursive_mutex _rmtxMessageQueue;

        std::mutex _mtxQueueNotifier;

        std::condition_variable _cvQueueNotifier;

        shared_ptr<std::thread> _thrdProcessQueue;



        void ProcessQueue();

        task<void> HandshakeAsyncWowza();

        task<void> CloseAsyncWowza();

        task<void> HandshakeAsyncAzure();

        task<void> CloseAsyncAzure();

        task<unsigned int> SendC0C1Async();

        task<void> ReceiveS0S1Async();

        task<unsigned int> SendC2Async();

        task<void> ReceiveS2Async();

        task<unsigned int> SendConnectAsync();

        task<void> ReceiveConnectResponseAsync();

        task<unsigned int> SendReleaseStreamAsync();

        task<void> ReceiveReleaseStreamResponseAsync();

        task<unsigned int> SendFCPublishAsync();

        task<void> ReceiveFCPublishResponseAsync();

        task<unsigned int> SendCreateStreamAsync();

        task<void> ReceiveCreateStreamResponseAsync();

        task<unsigned int> SendPublishStreamAsync();

        task<void> ReceivePublishStreamResponseAsync();

        task<unsigned int> SendSetDataFrameAsync(
          double frameRate,
          unsigned int width,
          unsigned int height,
          wstring videoCodec,
          unsigned int videoBitrate,
          unsigned int videoKeyFrameIntervalTime,
          wstring audioCodec,
          unsigned int audioSampleRate,
          unsigned int audioChannelCount,
          unsigned int audioBitrate);

        task<unsigned int> SendSetDataFrameAsync(
          wstring audioCodec,
          unsigned int audioSampleRate,
          unsigned int audioChannelCount,
          unsigned int audioBitrate);

        task<unsigned int> SendSetDataFrameAsync(
          double frameRate,
          unsigned int width,
          unsigned int height,
          wstring videoCodec,
          unsigned int videoBitrate,
          unsigned int videoKeyFrameIntervalTime);

        task<unsigned int> SendSetChunkSizeAsync(unsigned int ChunkSize);

        task<unsigned int> SendUnpublishAndCloseStreamAsync();

        task<void> ReceiveUnpublishStreamResponseAsync();
        
      };
    }
  }
}