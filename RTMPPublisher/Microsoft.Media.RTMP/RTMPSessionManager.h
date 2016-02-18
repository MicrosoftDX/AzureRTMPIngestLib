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

#include <wtypes.h>
#include <string>
#include <vector>
#include <atomic>
#include <memory> 
#include <windows.media.mediaproperties.h>
#include "Constants.h"
#include "PublishProfile.h"
#include "Uri.h"
#include "Constants.h"

using namespace std;
using namespace Windows::Media::MediaProperties;

namespace Microsoft
{
  namespace Media
  {
    namespace RTMP
    {


      class RTMPSessionManager
      {
      public:

        static std::wstring GetNewGUIDAsString()
        {
          WCHAR buff[128];
          GUID id = GUID_NULL;
          CoCreateGuid(&id);
          int len = ::StringFromGUID2(id, buff, 128);

          auto retval = std::wstring(buff);
          retval = retval.substr(1, retval.size() - 2);
          std::replace(retval.begin(), retval.end(), L'-', L'0');
          return retval;
          //return std::wstring(buff);

        }

        RTMPSessionManager(PublishProfile^ params) :
          _encodingProfile(params->TargetEncodingProfile),
          _baseEpoch(params->BaseEpoch),
          _rtmpUri(params->EndpointUri->Data()),
          _streamName(params->StreamName->Data()),
          _clientChunkSize(params->ClientChunkSize),
          _serverType(params->ServerType),
          _keyframeinterval(params->KeyFrameInterval)
        {
          auto uri = Microsoft::Media::RTMP::Uri::Parse(_rtmpUri);

          if (uri.Host().empty() || uri.Port().empty())
            throw std::invalid_argument("Malformed URI");

          _hostName = uri.Host();
          _portNumber = uri.Port();
          _serverappname = uri.Path(); 

          auto seed = GetNewGUIDAsString();
          wstring sranddata = GetNewGUIDAsString() + wstring(1528 - seed.size(), '\0');
          _C1randomBytes = make_shared<vector<BYTE>>();
          _C1randomBytes->resize(sranddata.size());
          memcpy_s(&(*(_C1randomBytes->begin())), _C1randomBytes->size(), sranddata.data(), _C1randomBytes->size());

        }


        wstring GetHostName()
        {
          return _hostName;
        }

        wstring GetPortNumber()
        {
          return _portNumber;
        }

        wstring GetServerAppName()
        {
          return _serverappname;
        }

        wstring GetStreamName()
        {
          return _streamName;
        }

        wstring GetRTMPUri()
        {
          return _rtmpUri;
        }

        unsigned int GetBaseEpoch()
        {
          return _baseEpoch;
        }

        void SetBaseEpoch(unsigned int val)
        {
          _baseEpoch = val;
        }

        unsigned int GetServerBaseEpoch()
        {
          return _serverBaseEpoch;
        }

        void SetServerBaseEpoch(unsigned int val)
        {
          _serverBaseEpoch = val;
        }

        unsigned int GetDefaultChunkSize()
        {
          return 128U;
        }

        unsigned int GetKeyframeInterval()
        {
          return _keyframeinterval;;
        }

        unsigned int GetClientChunkSize()
        {
          return _clientChunkSize;
        }

        unsigned int GetServerChunkSize()
        {
          return _serverChunkSize;
        }

        void SetServerChunkSize(unsigned int val)
        {
          _serverChunkSize = val;
        }

        unsigned int GetAcknowldgementWindowSize()
        {
          return _acknowledgementWindowSize;
        }

        void SetAcknowledgementWindowSize(unsigned int val)
        {
          _acknowledgementWindowSize = val;
        }

        unsigned int GetPeerBandwidthLimit()
        {
          return _peerBandwidthLimit;
        }

        void SetPeerBandwidthLimit(unsigned int val)
        {
          _peerBandwidthLimit = val;
        }

        BYTE GetBandwidthLimitType()
        {
          return _bandwidthLimitType;
        }

        void SetBandwidthLimitType(BYTE val)
        {
          _bandwidthLimitType = val;
        }

        unsigned int GetNextChunkStreamID()
        {
          return _chunkStreamID++;
        }

        unsigned int GetNextTransactionID()
        {
          return _transactionID++;
        }

        unsigned int GetVideoChunkStreamID()
        {
          return _videoChunkStreamID;
        }

        void SetVideoChunkStreamID(unsigned int val)
        {
          _videoChunkStreamID = val;
        }

        unsigned int GetAudioChunkStreamID()
        {
          return _audioChunkStreamID;
        }

        void SetAudioChunkStreamID(unsigned int val)
        {
          _audioChunkStreamID = val;
        }

        unsigned int GetMessageStreamID()
        {
          return _messageStreamID;
        }

        void SetMessageStreamID(unsigned int val)
        {
          _messageStreamID = val;
        }

        unsigned int GetS1ParseTimestamp()
        {
          return _S1parseTimeStamp;
        }

        void SetS1ParseTimestamp(unsigned int val)
        {
          _S1parseTimeStamp = val;
        }

        unsigned int GetBytesSentSinceLastAcknowledgement()
        {
          return _bytesSentSinceLastAck;
        }

        void IncrementBytesSentSinceLastAcknowledgement(unsigned int val)
        {
          _bytesSentSinceLastAck += val;
        }

        void BytesSentSinceLastAcknowledgement()
        {
          _bytesSentSinceLastAck = 0;
        }

        shared_ptr<unsigned int> GetLastVideoTimestamp()
        {
          return _lastVideoTimestamp;
        }

        void SetLastVideoTimestamp(shared_ptr<unsigned int> ts)
        {
          _lastVideoTimestamp = ts;
        }

        shared_ptr<unsigned int> GetLastAudioTimestamp()
        {
          return _lastAudioTimestamp;
        }

        void SetLastAudioTimestamp(shared_ptr<unsigned int> ts)
        {
          _lastAudioTimestamp = ts;
        }

        shared_ptr<vector<BYTE>> GetS1RandomBytes()
        {
          return _S1randomBytes;
        }

        void SetS1RandomBytes(shared_ptr<vector<BYTE>> val)
        {
          _S1randomBytes = val;
        }

        shared_ptr<vector<BYTE>> GetC1RandomBytes()
        {
          return _C1randomBytes;
        }

        void SetC1RandomBytes(shared_ptr<vector<BYTE>> val)
        {
          _C1randomBytes = val;
        }

        unsigned int GetPublishChunkStreamID()
        {
          return _publishChunkStreamId;
        }

        void SetPublishChunkStreamID(unsigned int val)
        {
          _publishChunkStreamId = val;
        }

        unsigned int GetStreamCreateReleaseChunkStreamID()
        {
          return _publishChunkStreamId;
        }

        void SetStreamCreateReleaseChunkStreamID(unsigned int val)
        {
          _publishChunkStreamId = val;
        }

        unsigned short GetSupportedRTMPAudioCodecFlags()
        {
          return RTMPAudioCodecFlag::SUPPORT_SND_AAC;
        }

        RTMPServerType GetServerType()
        {
          return _serverType;
        }

        unsigned short GetSupportedRTMPVideoCodecFlags()
        {
          return RTMPVideoCodecFlag::SUPPORT_VID_H264;
        }

        RTMPSessionState GetState()
        {
          return _sessionState;
        }

        void SetState(RTMPSessionState state)
        {
          _sessionState = state;
        }

        MediaEncodingProfile^ GetEncodingProfile()
        {
          return _encodingProfile;
        }



      private:
        std::wstring _hostName = L"";
        std::wstring _portNumber = L"";
        std::wstring _serverappname = L"";
        std::wstring _streamName = L"";
        std::wstring _rtmpUri = L"";
        unsigned int _baseEpoch = 0U;
        unsigned int _serverBaseEpoch = 0U;
        unsigned int _clientChunkSize = 128U;
        unsigned int _serverChunkSize = 128U;
        unsigned int _acknowledgementWindowSize = 128U * 50;
        unsigned int _peerBandwidthLimit = 0;
        BYTE _bandwidthLimitType = BandwidthLimitType::Hard;


        BYTE _rtmpVideoPayloadKeyFrameHeader = 0;
        BYTE _rtmpVideoPayloadInterFrameHeader = 0;
        BYTE _rtmpAudioPayloadHeader = 0;

        unsigned int _streamCreateReleaseChunkStreamID = 0;
        unsigned int _publishChunkStreamId = 0;
        unsigned int _chunkStreamID = 3;
        unsigned int _videoChunkStreamID = 0;
        unsigned int _audioChunkStreamID = 0;
        unsigned int _transactionID = 1;
        unsigned int _messageStreamID = 0;

        unsigned int _bytesSentSinceLastAck = 0;

        std::shared_ptr<unsigned int> _lastVideoTimestamp = nullptr;
        std::shared_ptr<unsigned int> _lastAudioTimestamp = nullptr;

        unsigned int _keyframeinterval = 60;

        unsigned int _S1parseTimeStamp = 0;
        shared_ptr<vector<BYTE>> _S1randomBytes = nullptr;
        shared_ptr<vector<BYTE>> _C1randomBytes = nullptr;
        RTMPServerType _serverType = RTMPServerType::Azure;
        MediaEncodingProfile^ _encodingProfile = nullptr;
        std::atomic<RTMPSessionState> _sessionState = RTMPSessionState::RTMP_UNINITIALIZED;
      };
    }
  }
}