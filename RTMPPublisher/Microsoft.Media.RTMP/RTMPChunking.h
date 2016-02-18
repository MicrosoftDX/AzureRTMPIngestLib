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
#include <cmath>
#include <windef.h>
#include <vector>
#include <algorithm>
#include <chrono> 
#include <limits>
#include <memory>
#include "BitOp.h" 
#include "RTMPMessageFormats.h"

using namespace std;
using namespace std::chrono;

namespace Microsoft
{
  namespace Media
  {
    namespace RTMP
    {

      class ChunkMessage
      {

      public:
        ChunkMessage() {
          _payload = make_shared<vector<BYTE>>();

        };

        ChunkMessage(
          unsigned int chunkStreamID,
          unsigned int timestamp,
          unsigned int messageLength,
          BYTE messageTypeID,
          unsigned int messageStreamID,
          BYTE* payload = nullptr,
          unsigned int payloadlen = 0U) :
          _chunkType(RTMPChunkType::Type0),
          _chunkStreamID(chunkStreamID),
          _timestamp(timestamp),
          _messageLength(messageLength),
          _messageTypeID(messageTypeID),
          _messageStreamID(messageStreamID)
        {
          if (payloadlen > 0 && payload != nullptr)
          {
            _payload = make_shared<vector<BYTE>>();
            _payload->resize(payloadlen);
            memcpy_s(&*(_payload->begin()), payloadlen, payload, payloadlen);
          }

        }

        ChunkMessage(
          unsigned int chunkStreamID,
          unsigned int timestampDelta,
          unsigned int messageLength,
          BYTE messageTypeID,
          BYTE* payload = nullptr,
          unsigned int payloadlen = 0U) :
          _chunkType(RTMPChunkType::Type1),
          _chunkStreamID(chunkStreamID),
          _timestamp(timestampDelta),
          _messageLength(messageLength),
          _messageTypeID(messageTypeID)
        {
          if (payloadlen > 0 && payload != nullptr)
          {
            _payload = make_shared<vector<BYTE>>();
            _payload->resize(payloadlen);
            memcpy_s(&*(_payload->begin()), payloadlen, payload, payloadlen);
          }

        }

        ChunkMessage(
          unsigned int chunkStreamID,
          unsigned int timestampDelta,
          BYTE* payload = nullptr,
          unsigned int payloadlen = 0U) :
          _chunkType(RTMPChunkType::Type2),
          _chunkStreamID(chunkStreamID),
          _timestamp(timestampDelta)
        {
          if (payloadlen > 0 && payload != nullptr)
          {
            _payload = make_shared<vector<BYTE>>();
            _payload->resize(payloadlen);
            memcpy_s(&*(_payload->begin()), payloadlen, payload, payloadlen);
          }

        }

        ChunkMessage(
          unsigned int chunkStreamID,
          BYTE* payload = nullptr,
          unsigned int payloadlen = 0U) :
          _chunkType(RTMPChunkType::Type3),
          _chunkStreamID(chunkStreamID)
        {
          if (payloadlen > 0 && payload != nullptr)
          {
            _payload = make_shared<vector<BYTE>>();
            _payload->resize(payloadlen);
            memcpy_s(&*(_payload->begin()), payloadlen, payload, payloadlen);
          }

        }

        unsigned int GetHeaderSize()
        {
          unsigned int basicheadersize = 0;
          if (_chunkStreamID <= 63) //1 byte format
          {
            basicheadersize = 1;
          }
          else if (_chunkStreamID <= 319) //2 byte format
          {
            basicheadersize = 2;

          }
          else if (_chunkStreamID <= ChunkStreamIDValue::MAX) //3 byte format
          {
            basicheadersize = 3;
          }

          if (_chunkType == RTMPChunkType::Type0)
          {
            return 11 + basicheadersize;
          }
          else if (_chunkType == RTMPChunkType::Type1)
          {
            return 7 + basicheadersize;
          }
          else if (_chunkType == RTMPChunkType::Type2)
          {
            return 3 + basicheadersize;
          }
          else
            return basicheadersize;
        }

        unsigned int GetSize()
        {

          return GetHeaderSize() + (unsigned int) (GetPayload()->size());
        }




        static shared_ptr<ChunkMessage> TryParse(const BYTE *data, unsigned int len, unsigned int currentChunkSize, unsigned int startAt = 0)
        {
          auto ctr = startAt;
          auto retval = make_shared<ChunkMessage>();


          retval.get()->_chunkType = BitOp::ExtractBits<BYTE>(data[ctr], 0, 2);
          if (retval.get()->_chunkType < RTMPChunkType::Type0 || retval.get()->_chunkType > RTMPChunkType::Type3)
            return nullptr; //Chunk type has to be within bounds

          auto csidhdrind = BitOp::ExtractBits<BYTE>(data[ctr], 2, 6);

          if (csidhdrind == 1)
          {
            retval.get()->_chunkStreamID = data[ctr + 2] * 256 + data[ctr + 1] + 64;
            ctr += 3;
          }
          else if (csidhdrind == 0)
          {
            retval.get()->_chunkStreamID = data[ctr + 1] + 64;
            ctr += 2;
          }
          else
          {
            retval.get()->_chunkStreamID = csidhdrind;
            ctr += 1;
          }

          if (retval.get()->_chunkType == RTMPChunkType::Type0)
          {
            retval.get()->_timestamp = BitOp::ToInteger<unsigned int>(data + ctr, 3);
            ctr += 3;
            retval.get()->_messageLength = BitOp::ToInteger<unsigned int>(data + ctr, 3);
            ctr += 3;
            retval.get()->_messageTypeID = data[ctr];
            ctr += 1;
            retval.get()->_messageStreamID = BitOp::ToInteger<unsigned int>(data + ctr, 4);
            ctr += 4;

          }
          else if (retval.get()->_chunkType == RTMPChunkType::Type1)
          {
            retval.get()->_timestamp = BitOp::ToInteger<unsigned int>(data + ctr, 3);
            ctr += 3;
            retval.get()->_messageLength = BitOp::ToInteger<unsigned int>(data + ctr, 3);
            ctr += 3;
            retval.get()->_messageTypeID = data[ctr];
            ctr += 1;

          }
          else if (retval.get()->_chunkType == RTMPChunkType::Type2)
          {
            retval.get()->_timestamp = BitOp::ToInteger<unsigned int>(data + ctr, 3);
            ctr += 3;

          }

          auto payloadlen = min(len - ctr, currentChunkSize);
          retval.get()->GetPayload()->resize(payloadlen);
          memcpy_s(&(*(retval.get()->GetPayload()->begin())), payloadlen, data + ctr, payloadlen);
          return retval;

        }


        virtual shared_ptr<vector<BYTE>> ToBitstream()
        {
          auto retval = make_shared<vector<BYTE>>();

          //basic header

          if (_chunkStreamID <= 63) //1 byte format
          {
            BYTE ret = 0;
            ret |= _chunkType;
            ret <<= 6;
            ret |= _chunkStreamID;
            retval->push_back(ret);
          }
          else if (_chunkStreamID <= 319) //2 byte format
          {
            BYTE ret = 0;
            ret |= _chunkType;
            ret <<= 6;
            retval->push_back(ret);
            retval->push_back(_chunkStreamID - 64);

          }
          else if (_chunkStreamID <= ChunkStreamIDValue::MAX) //3 byte format
          {
            BitOp::AddToBitstream(_chunkStreamID - 64, retval, false);
            if (retval->size() > 2)
              retval->resize(2);

            BYTE ret = 0;
            ret |= _chunkType;
            ret <<= 6;
            ret |= 1;
            retval->insert(begin(*retval), ret);
          }

          //message header

          if (_chunkType == RTMPChunkType::Type0) //format : timestamp(3 bytes NBO)+messagelength(3 bytes NBO)+messagetypeid(1 byte)+messagestreamid(4 bytes)
          {
            BitOp::AddToBitstream<unsigned int>(_extendedTimestamp ? 0xFFFFFF : _timestamp, retval, true, 3);
            BitOp::AddToBitstream(_messageLength, retval, true, 3);
            BitOp::AddToBitstream(_messageTypeID, retval);
            BitOp::AddToBitstream(_messageStreamID, retval, false);
            if (_extendedTimestamp)
            {
              BitOp::AddToBitstream(_timestamp, retval);
            }
          }
          else if (_chunkType == RTMPChunkType::Type1)
          {
            BitOp::AddToBitstream<unsigned int>(_extendedTimestamp ? 0xFFFFFF : _timestamp, retval, true, 3);
            BitOp::AddToBitstream(_messageLength, retval, true, 3);
            BitOp::AddToBitstream(_messageTypeID, retval);
            if (_extendedTimestamp)
            {
              BitOp::AddToBitstream(_timestamp, retval);
            }
          }
          else if (_chunkType == RTMPChunkType::Type2)
          {
            BitOp::AddToBitstream<unsigned int>(_extendedTimestamp ? 0xFFFFFF : _timestamp, retval, true, 3);
            if (_extendedTimestamp)
            {
              BitOp::AddToBitstream(_timestamp, retval);
            }
          }
          else if (_chunkType == RTMPChunkType::Type3)
          {
            if (_extendedTimestamp)
            {
              BitOp::AddToBitstream(_timestamp, retval);
            }
          }

          if (_payload != nullptr)
          {
            auto oldsize = retval->size();
            retval->resize(oldsize + _payload->size());
            memcpy_s(&(*(retval->begin() + oldsize)), _payload->size(), &(*(_payload->begin())), _payload->size());
          }


          return retval;
        }


        BYTE GetChunkType() {
          return _chunkType;
        }

        unsigned int GetChunkStreamID() {
          return _chunkStreamID;
        }

        unsigned int GetTimeStamp() {
          return _timestamp;
        }

        unsigned int GetMessageLength() {
          return _messageLength;
        }

        BYTE GetMessageTypeID() {
          return _messageTypeID;
        }

        unsigned int GetMessageStreamID() {
          return _messageStreamID;
        }


        shared_ptr<vector<BYTE>> GetPayload() {

          return _payload;
        }



        void TrimPayload(unsigned int len)
        {
          _payload->resize(len);

        }

        void AppendPayload(shared_ptr<vector<BYTE>> payload)
        {
          if (payload == nullptr || payload->size() == 0)
            return;

          if (_payload == nullptr)
            _payload = make_shared<vector<BYTE>>();

          auto oldsize = _payload->size();
          _payload->resize(payload->size() + oldsize);
          memcpy_s(&(*(_payload->begin() + oldsize)), payload->size(), &(*(payload->begin())), payload->size());
        }
      protected:

        BYTE _chunkType = 0;
        unsigned int _chunkStreamID = 0U;
        unsigned int _timestamp = 0U;
        unsigned int _messageLength = 0U;
        BYTE _messageTypeID = 0;
        unsigned int _messageStreamID = 0U;
        bool _extendedTimestamp = false;
        shared_ptr<vector<BYTE>> _payload;
      };



      class ChunkProcessor
      {
      public:

        static std::vector<shared_ptr<RTMPMessage>> TryParse(const BYTE* data, unsigned int len, unsigned int curChunkSize, unsigned int startAt = 0)
        {
          auto ctr = startAt;
          std::vector<shared_ptr<ChunkMessage>> chunkmessages;
          auto retval = std::vector<shared_ptr<RTMPMessage>>();
          shared_ptr<RTMPMessage> curMessage;

          unsigned int chunkSize = curChunkSize;

          //let's dechunk
          while (len > ctr)
          {
            auto chunkMessage = ChunkMessage::TryParse(data + ctr, len - ctr, chunkSize);

            if (chunkMessage == nullptr)
              break;

            if (chunkMessage->GetChunkType() < RTMPChunkType::Type0 || chunkMessage->GetChunkType() > RTMPChunkType::Type3)
              break;

            if (chunkMessage->GetChunkType() == RTMPChunkType::Type3)
            {
              if (chunkmessages.size() == 0)
                break;
              else
              {
                //find the last chunk message of types 0, 1, or 2 that has the same stream ID as this
                auto cmStart = std::find_if(chunkmessages.rbegin(), chunkmessages.rend(), [&chunkMessage](std::shared_ptr<ChunkMessage> cm)
                {
                  return (cm->GetChunkType() == RTMPChunkType::Type0 || cm->GetChunkType() == RTMPChunkType::Type1 || cm->GetChunkType() == RTMPChunkType::Type2) && chunkMessage->GetChunkStreamID() == cm->GetChunkStreamID();
                });
                if (cmStart == chunkmessages.rend())
                  break;

                chunkMessage->TrimPayload(min(chunkSize, (*cmStart)->GetMessageLength() - (unsigned int) ((*cmStart)->GetPayload()->size())));
                (*cmStart)->AppendPayload(chunkMessage->GetPayload());
              }

            }
            else if (chunkMessage->GetChunkType() == RTMPChunkType::Type0 || chunkMessage->GetChunkType() == RTMPChunkType::Type1 || chunkMessage->GetChunkType() == RTMPChunkType::Type2)
            {
              chunkMessage->TrimPayload(min(chunkSize, chunkMessage->GetMessageLength()));
              chunkmessages.push_back(chunkMessage);
            }

            ctr += chunkMessage->GetSize();

            if (chunkMessage->GetPayload()->size() == chunkMessage->GetMessageLength())
            {

              if (chunkMessage->GetMessageTypeID() == RTMPMessageType::COMMANDAMF0)
              {
                auto props = AMF0Entity::TryParse(chunkMessage->GetPayload());
                if (props.size() > 0 && props.front()->GetType() == AMF0TypeMarker::String)
                {

                  if (props.front()->GetStringValue() == L"_result")
                  {
                    retval.push_back(make_shared<Command_Result>(props, (unsigned int) chunkMessage->GetPayload()->size(), chunkMessage->GetMessageStreamID()));
                  }
                  else if (props.front()->GetStringValue() == L"_error")
                  {
                    retval.push_back(make_shared<Command_Error>(props, (unsigned int) chunkMessage->GetPayload()->size(), chunkMessage->GetMessageStreamID()));
                  }
                  else if (props.front()->GetStringValue() == L"onStatus")
                  {
                    retval.push_back(make_shared<Command_Status>(props, (unsigned int) chunkMessage->GetPayload()->size(), chunkMessage->GetMessageStreamID()));
                  }
                  else
                  {
                    retval.push_back(make_shared<AMF0EncodedCommandOrData>(props, (unsigned int) chunkMessage->GetPayload()->size(), chunkMessage->GetMessageStreamID()));
                  }

                }
              }
              else if (chunkMessage->GetMessageTypeID() == RTMPMessageType::PROTOABORT)
              {

                retval.push_back(make_shared<ProtoAbortMessage>(
                  ProtoAbortMessage::GetChunkStreamID(&(*(chunkMessage->GetPayload()->begin())))
                  ));

              }
              else if (chunkMessage->GetMessageTypeID() == RTMPMessageType::PROTOACKNOWLEDGEMENT)
              {
                retval.push_back(make_shared<ProtoAcknowledgementMessage>(
                  ProtoAcknowledgementMessage::GetSequenceNumber(&(*(chunkMessage->GetPayload()->begin())))
                  ));

              }
              else if (chunkMessage->GetMessageTypeID() == RTMPMessageType::PROTOACKWINDOWSIZE)
              {
                retval.push_back(make_shared<ProtoAckWindowSizeMessage>(
                  ProtoAckWindowSizeMessage::GetWindowSize(&(*(chunkMessage->GetPayload()->begin())))
                  ));

              }
              else if (chunkMessage->GetMessageTypeID() == RTMPMessageType::PROTOSETCHUNKSIZE)
              {
                chunkSize = ProtoSetChunkSizeMessage::GetChunkSize(&(*(chunkMessage->GetPayload()->begin())));
                retval.push_back(make_shared<ProtoSetChunkSizeMessage>(
                  chunkSize
                  ));

              }
              else if (chunkMessage->GetMessageTypeID() == RTMPMessageType::PROTOSETPEERBANDWIDTH)
              {
                retval.push_back(make_shared<ProtoSetPeerBandwidthMessage>(
                  ProtoSetPeerBandwidthMessage::GetBandwidth(&(*(chunkMessage->GetPayload()->begin()))),
                  ProtoSetPeerBandwidthMessage::GetBandwidthLimitType(&(*(chunkMessage->GetPayload()->begin())))
                  ));

              }
              else if (chunkMessage->GetMessageTypeID() == RTMPMessageType::USERCONTROL)
              {
                unsigned short usercontrolmessagetype = BitOp::ToInteger<unsigned short>(&(*(chunkMessage->GetPayload()->begin())), 2);
                unsigned short payloadlength = 6;
                if (usercontrolmessagetype == UserControlMessageType::SetBufferLength)
                  payloadlength = 10;
                retval.push_back(make_shared<UserControlMessage>(&(*(chunkMessage->GetPayload()->begin())), payloadlength));
              }
            }


          }


          return retval;
        }

        static shared_ptr<vector<BYTE>> ToChunkedBitstream(unsigned int chunkStreamID, unsigned int chunkSize, shared_ptr<RTMPMessage> rtmpmsg)
        {
          auto retval = make_shared<vector<BYTE>>();
          unsigned int messagelen = rtmpmsg->GetMessageLength();
          int numchunks = (int) std::ceil((double) messagelen / (double) chunkSize);
          for (int i = 0; i < numchunks; i++)
          {
            shared_ptr<vector<BYTE>> bs;
            if (i == 0)
            {

              if (rtmpmsg->IsTimestampDelta() == false)
              {
                bs = ChunkMessage(chunkStreamID,
                  rtmpmsg->GetTimestamp(),
                  rtmpmsg->GetMessageLength(),
                  rtmpmsg->GetMessageTypeID(),
                  rtmpmsg->GetMessageStreamID(),
                  &(*(rtmpmsg->GetPayload()->begin() + (i * chunkSize))),
                  min(messagelen - (i * chunkSize), chunkSize)).ToBitstream();
              }
              else
              {
                bs = ChunkMessage(chunkStreamID,
                  rtmpmsg->GetTimestamp(),
                  rtmpmsg->GetMessageLength(),
                  rtmpmsg->GetMessageTypeID(),
                  &(*(rtmpmsg->GetPayload()->begin() + (i * chunkSize))),
                  min(messagelen - (i * chunkSize), chunkSize)).ToBitstream();
              }
            }
            else
            {
              bs = ChunkMessage(chunkStreamID,
                &(*(rtmpmsg->GetPayload()->begin() + ((i * chunkSize)))),
                min(messagelen - (i * chunkSize), chunkSize)).ToBitstream();
            }

            auto oldsize = retval->size();
            retval->resize(oldsize + bs->size());
            memcpy_s(&(*(retval->begin() + oldsize)), bs->size(), &(*(bs->begin())), bs->size());

          }
          return retval;
        }

        static std::vector<shared_ptr<vector<BYTE>>> ToChunkedBitstreams(unsigned int chunkStreamID, unsigned int chunkSize, shared_ptr<RTMPMessage> rtmpmsg)
        {
          std::vector<shared_ptr<vector<BYTE>>> retval;
          unsigned int messagelen = rtmpmsg->GetMessageLength();
          int numchunks = (int) std::ceil((double) messagelen / (double) chunkSize);
          for (int i = 0; i < numchunks; i++)
          {
            shared_ptr<vector<BYTE>> bs;
            if (i == 0)
            {

              if (rtmpmsg->IsTimestampDelta() == false)
              {
                bs = ChunkMessage(chunkStreamID,
                  rtmpmsg->GetTimestamp(),
                  rtmpmsg->GetMessageLength(),
                  rtmpmsg->GetMessageTypeID(),
                  rtmpmsg->GetMessageStreamID(),
                  &(*(rtmpmsg->GetPayload()->begin() + (i * chunkSize))),
                  min(messagelen - (i * chunkSize), chunkSize)).ToBitstream();
              }
              else
              {
                bs = ChunkMessage(chunkStreamID,
                  rtmpmsg->GetTimestamp(),
                  rtmpmsg->GetMessageLength(),
                  rtmpmsg->GetMessageTypeID(),
                  &(*(rtmpmsg->GetPayload()->begin() + (i * chunkSize))),
                  min(messagelen - (i * chunkSize), chunkSize)).ToBitstream();
              }


            }
            else
            {
              bs = ChunkMessage(chunkStreamID,
                &(*(rtmpmsg->GetPayload()->begin() + ((i * chunkSize)))),
                min(messagelen - (i * chunkSize), chunkSize)).ToBitstream();
            }

            retval.push_back(bs);

          }
          return retval;
        }



      };

    }
  }
}