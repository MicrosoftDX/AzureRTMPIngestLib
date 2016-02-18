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
#include "BitOp.h" 
#include "Constants.h"


using namespace std;
using namespace std::chrono;

namespace Microsoft
{
  namespace Media
  {
    namespace RTMP
    {

      class HandshakeMessageC0S0
      {
      public:

        static const unsigned short RTMP_VERSION = 3;

        HandshakeMessageC0S0(unsigned short version) :_version(version)
        {}


        std::shared_ptr<vector<BYTE>> ToBitstream()
        {
          shared_ptr<vector<BYTE>> retval = std::make_shared<std::vector<BYTE>>();
          BitOp::AddToBitstream<unsigned short>(_version, retval, true, 1U);
          return retval;
        }

        void ToBitstream(std::shared_ptr<vector<BYTE>>& retval)
        {
          if (retval == nullptr)
            retval = std::make_shared<std::vector<BYTE>>();
          BitOp::AddToBitstream<unsigned short>(_version, retval, true, 1U);
          return;
        }


        static std::shared_ptr<HandshakeMessageC0S0> TryParse(const BYTE* data, unsigned int size)
        {
          if (size != 1) return nullptr; //we are expecting a single byte
          return make_shared<HandshakeMessageC0S0>(data[0]);
        }


        unsigned short GetVersion()
        {
          return _version;
        }

      private:

        unsigned short _version = 0U;

      };






      class HandshakeMessageC1C2S1S2
      {
      public:
        //used when constructing C1
        HandshakeMessageC1C2S1S2(unsigned int baseEpoch, shared_ptr<vector<BYTE>> randomBytes) :
          _baseEpoch(baseEpoch),
          _randomBytes(randomBytes)
        {

        }

        //used when constructing C2
        HandshakeMessageC1C2S1S2(unsigned int baseEpoch, unsigned int parseTimestamp, shared_ptr<vector<BYTE>> randomBytes) :
          _baseEpoch(baseEpoch),
          _parseTimestamp(parseTimestamp),
          _randomBytes(randomBytes)
        {

        }

        HandshakeMessageC1C2S1S2()
        {
          _randomBytes = make_shared<vector<BYTE>>();
        }


        //Parse S1 or S2 messages
        static std::shared_ptr<HandshakeMessageC1C2S1S2> TryParse(const BYTE* data, unsigned int size)
        {
          if (size != 1536) return nullptr; //we are expecting exactly 1536 bytes

          auto retval = make_shared<HandshakeMessageC1C2S1S2>();
          retval->_baseEpoch = BitOp::ToInteger<unsigned int>(data, 4, true);
          retval->_randomBytes->resize(1528);
          memcpy_s(&(*(retval->_randomBytes->begin())), 1528, data + 8, 1528);


          //timestamp when we parsed this packet - only useful when parsing S1 - since we will need this to pack into C2
          retval->_parseTimestamp = (unsigned int) (chrono::system_clock::now().time_since_epoch().count() * TICKSTOMILLIS);

          return retval;
        }

        std::shared_ptr<vector<BYTE>> ToBitstream()
        {
          shared_ptr<vector<BYTE>> retval = std::make_shared<std::vector<BYTE>>();

          BitOp::AddToBitstream(_baseEpoch, retval);
          BitOp::AddToBitstream(_parseTimestamp, retval);

          auto oldsize = retval->size();
          retval->resize(oldsize + _randomBytes->size());
          copy(_randomBytes->begin(), _randomBytes->end(), retval->begin() + oldsize);

          return retval;
        }

        void ToBitstream(std::shared_ptr<vector<BYTE>>& retval)
        {
          if (retval == nullptr)
            retval = std::make_shared<std::vector<BYTE>>();

          BitOp::AddToBitstream(_baseEpoch, retval);
          BitOp::AddToBitstream(_parseTimestamp, retval);

          auto oldsize = retval->size();
          retval->resize(oldsize + _randomBytes->size());
          copy(_randomBytes->begin(), _randomBytes->end(), retval->begin() + oldsize);

          return;
        }

        shared_ptr<vector<BYTE>> GetRandomBytes()
        {
          return _randomBytes;
        }


        unsigned int GetBaseEpoch()
        {
          return _baseEpoch;
        }

        unsigned int GetParseTimestamp()
        {
          return _parseTimestamp;
        }


        bool AreRandomBytesEqual(shared_ptr<vector<BYTE>> src)
        {
          if (src->size() != _randomBytes->size()) return false;
          if (src == _randomBytes) return true;
          return std::equal(begin(*_randomBytes), end(*_randomBytes), begin(*src));
        }

      private:

        shared_ptr<vector<BYTE>> _randomBytes;
        unsigned int _baseEpoch = 0U;
        unsigned int _parseTimestamp = 0U;
      };






      class RTMPMessage
      {
      public:
        RTMPMessage(
          unsigned int timestamp,
          unsigned int messageLength,
          BYTE messageTypeID,
          unsigned int messageStreamID,
          BYTE* payload = nullptr,
          bool isTimestampDelta = false) :
          _timestamp(timestamp),
          _messageLength(messageLength),
          _messageTypeID(messageTypeID),
          _messageStreamID(messageStreamID),
          _isTimetampDelta(isTimestampDelta)
        {
          _payload = make_shared<vector<BYTE>>();

          if (messageLength > 0 && payload != nullptr)
          {

            _payload->resize(messageLength);
            memcpy_s(&*(_payload->begin()), messageLength, payload, messageLength);
          }

        }

        unsigned int GetTimestamp() {
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

        unsigned int IsTimestampDelta() {
          return _isTimetampDelta;
        }


        shared_ptr<vector<BYTE>> GetPayload() {
          return _payload;
        }

      protected:
        unsigned int _timestamp = 0U;
        unsigned int _messageLength = 0U;
        BYTE _messageTypeID = 0;
        unsigned int _messageStreamID = 0U;
        bool _extendedTimestamp = false;
        shared_ptr<vector<BYTE>> _payload;
        bool _isTimetampDelta = false;
      };



      class ProtoSetChunkSizeMessage : public RTMPMessage
      {
      public:

        ProtoSetChunkSizeMessage(unsigned int chunkSize) :
          RTMPMessage(0, sizeof(unsigned int), RTMPMessageType::PROTOSETCHUNKSIZE, 0)
        {
          if (chunkSize > 0x7FFFFFFF)
            throw invalid_argument("Chunk size cannot be greater than 2147483647 bytes");


          BitOp::AddToBitstream(chunkSize, this->_payload);
        }



        unsigned int GetChunkSize()
        {
          return BitOp::ToInteger<unsigned int>(&(*(_payload->begin())), 4);
        }

        static unsigned int GetChunkSize(const BYTE* bs)
        {
          return BitOp::ToInteger<unsigned int>(bs, 4);
        }

      };






      class ProtoAbortMessage : public RTMPMessage
      {
      public:

        ProtoAbortMessage(unsigned int chunkStreamID) :
          RTMPMessage(0, sizeof(unsigned int), RTMPMessageType::PROTOABORT, 0)
        {
          BitOp::AddToBitstream(chunkStreamID, this->_payload);
        }

        unsigned int GetChunkStreamID()
        {
          return BitOp::ToInteger<unsigned int>(&(*(_payload->begin())), 4);
        }

        static unsigned int GetChunkStreamID(const BYTE* bs)
        {
          return BitOp::ToInteger<unsigned int>(bs, 4);
        }
      };






      class ProtoAcknowledgementMessage : public RTMPMessage
      {
      public:

        ProtoAcknowledgementMessage(unsigned int sequenceNumber) :
          RTMPMessage(0, sizeof(unsigned int), RTMPMessageType::PROTOACKNOWLEDGEMENT, 0)
        {
          BitOp::AddToBitstream(sequenceNumber, this->_payload);
        }

        unsigned int GetSequenceNumber()
        {
          return BitOp::ToInteger<unsigned int>(&(*(_payload->begin())), 4);
        }

        static unsigned int GetSequenceNumber(const BYTE* bs)
        {
          return BitOp::ToInteger<unsigned int>(bs, 4);
        }
      };





      class ProtoAckWindowSizeMessage : public RTMPMessage
      {
      public:

        ProtoAckWindowSizeMessage(unsigned int windowSize) :
          RTMPMessage(0, sizeof(unsigned int), RTMPMessageType::PROTOACKWINDOWSIZE, 0)
        {
          BitOp::AddToBitstream(windowSize, this->_payload);
        }


        unsigned int GetWindowSize()
        {
          return BitOp::ToInteger<unsigned int>(&(*(_payload->begin())), 4);
        }

        static unsigned int GetWindowSize(const BYTE* bs)
        {
          return BitOp::ToInteger<unsigned int>(bs, 4);
        }

      };






      class ProtoSetPeerBandwidthMessage : public RTMPMessage
      {
      public:

        ProtoSetPeerBandwidthMessage(unsigned int bandwidth,
          BYTE bandwidthLimitType) :
          RTMPMessage(0, sizeof(unsigned int) + 1, RTMPMessageType::PROTOSETPEERBANDWIDTH, 0)
        {
          if (bandwidthLimitType > BandwidthLimitType::Dynamic || bandwidthLimitType < BandwidthLimitType::Soft)
            throw invalid_argument("Invalid Bandwidth limit type");


          BitOp::AddToBitstream(bandwidth, this->_payload);
          BitOp::AddToBitstream(bandwidthLimitType, this->_payload);

        }


        unsigned int GetBandwidth()
        {
          return BitOp::ToInteger<unsigned int>(&(*(_payload->begin())), 4);
        }

        BYTE GetBandwidthLimitType()
        {
          return (*_payload)[4];
        }

        static unsigned int GetBandwidth(const BYTE* bs)
        {
          return BitOp::ToInteger<unsigned int>(bs, 4);
        }

        static BYTE GetBandwidthLimitType(const BYTE* bs)
        {
          return bs[4];
        }

      };


      class UserControlMessage : public RTMPMessage
      {
      public:
        UserControlMessage(BYTE* payload, unsigned int payloadlen) : RTMPMessage(0, payloadlen, RTMPMessageType::USERCONTROL, 0, payload)
        {

        }

        unsigned short GetType()
        {
          return BitOp::ToInteger<unsigned short>(&(*(_payload->begin())), 2);
        }

        const BYTE* GetData()
        {
          return &(*(_payload->begin() + 2));
        }
      };



      class AMF0Entity
      {
      public:

        AMF0Entity(BYTE type) : _type(type)
        {

        }

        AMF0Entity(double numberValue) : _type(AMF0TypeMarker::Number), _numberValue(numberValue)
        {

        }

        AMF0Entity(wstring stringValue) : _stringValue(stringValue)
        {
          if (stringValue.size() <= 65535)
            _type = AMF0TypeMarker::String;
          else
            _type = AMF0TypeMarker::LongString;
        }

        AMF0Entity(bool booleanValue) : _type(AMF0TypeMarker::Boolean), _booleanValue(booleanValue)
        {

        }

        AMF0Entity(shared_ptr<std::vector<tuple<wstring, shared_ptr<AMF0Entity>>>>  propmap) : _type(AMF0TypeMarker::Object), _propertyMap(propmap)
        {

        }



        static void EncodeString(std::wstring str, shared_ptr<vector<BYTE>> bs)
        {
          BitOp::AddToBitstream<BYTE>(AMF0TypeMarker::String, bs, false);

          if (str.size() <= 65535)
            BitOp::AddToBitstream<unsigned short>((unsigned short) str.size(), bs, true);
          else
            BitOp::AddToBitstream<unsigned int>((unsigned int) str.size(), bs, true);

          if (str.size() > 0)
            BitOp::AddToBitstream(str, bs);
        }

        static void EncodeName(std::wstring str, shared_ptr<vector<BYTE>> bs)
        {

          if (str.size() <= 65535)
            BitOp::AddToBitstream<unsigned short>((unsigned short) str.size(), bs, true);
          else
            BitOp::AddToBitstream<unsigned int>((unsigned int) str.size(), bs, true);

          if (str.size() > 0)
            BitOp::AddToBitstream(str, bs);
        }


        static void EncodeNumber(double num, shared_ptr<vector<BYTE>> bs)
        {
          BitOp::AddToBitstream<BYTE>(AMF0TypeMarker::Number, bs, false);
          BitOp::AddToBitstream<unsigned long long>(BitOp::FromDoubleToFPRep(num), bs, true);
        }

        static void EncodeBoolean(bool val, shared_ptr<vector<BYTE>> bs)
        {
          BitOp::AddToBitstream<BYTE>(AMF0TypeMarker::Boolean, bs, false);
          BitOp::AddToBitstream<BYTE>(val ? 1 : 0, bs, false);
        }

        static void EncodeNull(shared_ptr<vector<BYTE>> bs)
        {
          BitOp::AddToBitstream<BYTE>(AMF0TypeMarker::Null, bs, false);
        }


        static void EncodeObject(shared_ptr<std::vector<tuple<wstring, shared_ptr<AMF0Entity>>>> propmap, shared_ptr<vector<BYTE>> bs)
        {
          BitOp::AddToBitstream(AMF0TypeMarker::Object, bs);

          for (auto& t : *propmap)
          {
            auto name = std::get<0>(t);

            AMF0Entity::EncodeName(name, bs);


            auto entity = std::get<1>(t);

            if (entity->GetType() == AMF0TypeMarker::String || entity->GetType() == AMF0TypeMarker::LongString)
            {
              AMF0Entity::EncodeString(entity->GetStringValue(), bs);
            }
            else if (entity->GetType() == AMF0TypeMarker::Number)
            {
              AMF0Entity::EncodeNumber(entity->GetNumberValue(), bs);
            }
            else if (entity->GetType() == AMF0TypeMarker::Boolean)
            {
              AMF0Entity::EncodeBoolean(entity->GetBooleanValue(), bs);
            }
            else if (entity->GetType() == AMF0TypeMarker::Object)
            {
              AMF0Entity::EncodeObject(entity->GetPropertyMap(), bs); //does AMF0 allow object nesting ???
            }
            else if (entity->GetType() == AMF0TypeMarker::Null)
            {
              AMF0Entity::EncodeNull(bs);
            }
          }
          AMF0Entity::EncodeName(L"", bs);
          BitOp::AddToBitstream(AMF0TypeMarker::ObjectEnd, bs);
        }

        BYTE GetType() {
          return _type;
        }

        double GetNumberValue() {
          if (_type != AMF0TypeMarker::Number) throw exception("Not a number");
          return _numberValue;
        }

        wstring GetStringValue() {
          if (_type != AMF0TypeMarker::String) throw exception("Not a string");
          return _stringValue;
        }

        bool GetBooleanValue() {
          if (_type != AMF0TypeMarker::Boolean) throw exception("Not a boolean");
          return _booleanValue;
        }

        shared_ptr<std::vector<tuple<wstring, shared_ptr<AMF0Entity>>>> GetPropertyMap()
        {
          if (_type != AMF0TypeMarker::Object) throw exception("Not an object");
          if (_propertyMap == nullptr)
            _propertyMap = make_shared<std::vector<tuple<wstring, shared_ptr<AMF0Entity>>>>();
          return _propertyMap;
        }





        static std::vector<shared_ptr<AMF0Entity>> TryParse(shared_ptr<vector<BYTE>> payload)
        {
          std::vector<shared_ptr<AMF0Entity>> retval;
          bool inObject = false;
          shared_ptr<AMF0Entity> curObject = nullptr;
          wstring curPropKey;
          for (auto itr = payload->begin(); itr != payload->end();)
          {

            if (inObject && curPropKey == L"")
            {
              auto strlen = BitOp::ToInteger<unsigned short>(&(*(itr)), 2);
              if (*itr == AMF0TypeMarker::ObjectEnd || strlen == 0)
              {
                inObject = false;
                retval.push_back(curObject);
                if (*itr == AMF0TypeMarker::ObjectEnd)
                  ++itr;
                else
                  itr += 3;
                continue;
              }
              //read string length 
              itr += 2;
              curPropKey = BitOp::ToWideString((char*) &(*(itr)), strlen);
              itr += strlen;
            }

            auto type = *itr;
            ++itr;

            if (type == AMF0TypeMarker::Boolean)
            {


              if (inObject)
              {
                if (curPropKey == L"")
                  throw std::logic_error("Parse Error : Missing object property name"); //gotta have a property name
                curObject->GetPropertyMap()->push_back(std::tuple<wstring, shared_ptr<AMF0Entity>>(curPropKey, make_shared<AMF0Entity>((*itr) == 1 ? true : false)));
                curPropKey = L"";
              }
              else
              {
                retval.push_back(make_shared<AMF0Entity>((*itr) == 1 ? true : false));
              }
              ++itr;
            }
            else if (type == AMF0TypeMarker::Number)
            {

              if (inObject)
              {
                if (curPropKey == L"")
                  throw std::logic_error("Parse Error : Missing object property name"); //gotta have a property name
                auto fprep = BitOp::ToInteger<unsigned long long>(&(*(itr)), sizeof(unsigned long long));
                double val = BitOp::FromFPRepToDouble(fprep);
                curObject->GetPropertyMap()->push_back(std::tuple<wstring, shared_ptr<AMF0Entity>>(curPropKey, make_shared<AMF0Entity>(val)));
                curPropKey = L"";
              }
              else
              {
                auto fprep = BitOp::ToInteger<unsigned long long>(&(*(itr)), sizeof(unsigned long long));
                double val = BitOp::FromFPRepToDouble(fprep);
                retval.push_back(make_shared<AMF0Entity>(val));
              }
              itr += sizeof(unsigned long long);
            }
            else if (type == AMF0TypeMarker::String)
            {

              //read string length
              auto strlen = BitOp::ToInteger<unsigned short>(&(*(itr)), 2);
              itr += 2;

              if (inObject)
              {
                if (curPropKey == L"")
                  throw std::logic_error("Parse Error : Missing object property name"); //gotta have a property name
                else
                {
                  curObject->GetPropertyMap()->push_back(std::tuple<wstring, shared_ptr<AMF0Entity>>(curPropKey, make_shared<AMF0Entity>(BitOp::ToWideString((char*) &(*(itr)), strlen))));
                  curPropKey = L"";
                }
              }
              else
              {
                retval.push_back(make_shared<AMF0Entity>(BitOp::ToWideString((char*) &(*(itr)), strlen)));
              }
              itr += strlen;
            }

            else if (type == AMF0TypeMarker::Null)
            {
              retval.push_back(make_shared<AMF0Entity>(type));

            }
            else if (type == AMF0TypeMarker::EcmaArray)
            {
              //we do not process arrays -no scenarios need it yet
              while (*itr != AMF0TypeMarker::ObjectEnd)
                ++itr;

              ++itr;//go past the object end marker

              if (inObject)
                curPropKey = L"";
            }
            else if (type == AMF0TypeMarker::Object)
            {

              inObject = true;
              curObject = make_shared<AMF0Entity>(type);

            }

          }
          return retval;
        }


      private:
        BYTE _type = AMF0TypeMarker::Unsupported;
        double  _numberValue = 0;
        wstring _stringValue = L"";
        bool _booleanValue = false;
        shared_ptr<std::vector<tuple<wstring, shared_ptr<AMF0Entity>>>> _propertyMap = nullptr;
      };



      class AMF0EncodedCommandOrData : public RTMPMessage
      {
      public:
        AMF0EncodedCommandOrData(wstring commandName, unsigned int messageStreamID, unsigned int transactionID) :
          RTMPMessage(0, 0, RTMPMessageType::COMMANDAMF0, messageStreamID)
        {
          _entities.push_back(make_shared<AMF0Entity>(commandName));
          _entities.push_back(make_shared<AMF0Entity>((double) transactionID));
        }

        AMF0EncodedCommandOrData(std::vector<shared_ptr<AMF0Entity>> entities, unsigned int messageLength, unsigned int messageStreamID) :
          RTMPMessage(0, messageLength, RTMPMessageType::COMMANDAMF0, messageStreamID), _entities(entities)
        {

        }

        AMF0EncodedCommandOrData(wstring commandName, unsigned int messageStreamID) :
          RTMPMessage(0, 0, RTMPMessageType::DATAAMF0, messageStreamID)
        {
          _entities.push_back(make_shared<AMF0Entity>(commandName));
        }



        wstring GetCommandName()
        {
          return _entities.begin()->get()->GetStringValue();
        }
        unsigned int GetTransactionID()
        {
          return (unsigned int) ((_entities.begin() + 1)->get()->GetNumberValue());
        }

        std::vector<shared_ptr<AMF0Entity>> GetAllEntities()
        {
          return _entities;
        }


        shared_ptr<AMF0Entity> GetObjectPropertyValue(wstring PropertyName)
        {
          shared_ptr<AMF0Entity> retval = nullptr;
          for (auto& ent : _entities)
          {
            if (ent->GetType() == AMF0TypeMarker::Object)
            {
              auto propmap = ent->GetPropertyMap();
              auto match = std::find_if(propmap->begin(), propmap->end(), [&PropertyName](tuple<wstring, shared_ptr<AMF0Entity>> tpl)
              {
                wstring name = std::get<0>(tpl);
                return true;
              });

              if (match != propmap->end())
              {
                retval = std::get<1>(*match);
                break;
              }


            }
          }

          return retval;
        }

        virtual void Encode()
        {
          if (_payload == nullptr)
            _payload = make_shared<vector<BYTE>>();

          for (auto& entity : _entities)
          {
            if (entity->GetType() == AMF0TypeMarker::String || entity->GetType() == AMF0TypeMarker::LongString)
            {
              AMF0Entity::EncodeString(entity->GetStringValue(), _payload);
            }
            else if (entity->GetType() == AMF0TypeMarker::Number)
            {
              AMF0Entity::EncodeNumber(entity->GetNumberValue(), _payload);
            }
            else if (entity->GetType() == AMF0TypeMarker::Boolean)
            {
              AMF0Entity::EncodeBoolean(entity->GetBooleanValue(), _payload);
            }
            else if (entity->GetType() == AMF0TypeMarker::Null)
            {
              AMF0Entity::EncodeNull(_payload);
            }
            else if (entity->GetType() == AMF0TypeMarker::Object)
            {
              AMF0Entity::EncodeObject(entity->GetPropertyMap(), _payload);
            }
          }
        }
      protected:

        std::vector<shared_ptr<AMF0Entity>> _entities;
      };






      class  Command_Connect : public AMF0EncodedCommandOrData
      {
      public:
        Command_Connect(unsigned int transactionID, wstring serverapp, wstring tcUrl, unsigned short audioCodecs, unsigned short videoCodecs) :
          AMF0EncodedCommandOrData(L"connect", 0, transactionID)
        {
          shared_ptr<std::vector<tuple<wstring, shared_ptr<AMF0Entity>>>> props
            = make_shared<std::vector<tuple<wstring, shared_ptr<AMF0Entity>>>>();

          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"app", make_shared<AMF0Entity>(serverapp)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"tcUrl", make_shared<AMF0Entity>(tcUrl)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"audioCodecs", make_shared<AMF0Entity>((double) audioCodecs)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"videoCodecs", make_shared<AMF0Entity>((double) videoCodecs)));
          /*  props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"type", make_shared<AMF0Entity>(wstring(L"nonprivate"))));
            props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"flashVer", make_shared<AMF0Entity>(wstring(L"wirecast/FM 1.0 (compatible; MSS/1.0)"))));*/


          _entities.push_back(make_shared<AMF0Entity>(props));


          Encode();


          _messageLength = (unsigned int) _payload->size();
        }

        Command_Connect(unsigned int transactionID, wstring serverapp, wstring tcUrl, unsigned short codec, bool AudioOnly) :
          AMF0EncodedCommandOrData(L"connect", 0, transactionID)
        {
          shared_ptr<std::vector<tuple<wstring, shared_ptr<AMF0Entity>>>> props
            = make_shared<std::vector<tuple<wstring, shared_ptr<AMF0Entity>>>>();

          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"app", make_shared<AMF0Entity>(serverapp)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"tcUrl", make_shared<AMF0Entity>(tcUrl)));
          if (AudioOnly)
            props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"audioCodecs", make_shared<AMF0Entity>((double) codec)));
          else
            props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"videoCodecs", make_shared<AMF0Entity>((double) codec)));
          /*  props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"type", make_shared<AMF0Entity>(wstring(L"nonprivate"))));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"flashVer", make_shared<AMF0Entity>(wstring(L"wirecast/FM 1.0 (compatible; MSS/1.0)"))));*/


          _entities.push_back(make_shared<AMF0Entity>(props));


          Encode();


          _messageLength = (unsigned int) _payload->size();
        }

      };





      class  Command_CreateStream : public AMF0EncodedCommandOrData
      {
      public:
        Command_CreateStream(unsigned int transactionID) :
          AMF0EncodedCommandOrData(L"createStream", 0, transactionID)
        {

          _entities.push_back(make_shared<AMF0Entity>(AMF0TypeMarker::Null));

          Encode();


          _messageLength = (unsigned int) _payload->size();
        }
      };





      class  Command_ReleaseStream : public AMF0EncodedCommandOrData
      {
      public:
        Command_ReleaseStream(unsigned int transactionID, wstring streamName) :
          AMF0EncodedCommandOrData(L"releaseStream", 0, transactionID)
        {
          _entities.push_back(make_shared<AMF0Entity>(AMF0TypeMarker::Null));
          _entities.push_back(make_shared<AMF0Entity>(streamName));

          Encode();


          _messageLength = (unsigned int) _payload->size();
        }
      };

      class  Command_FCPublish : public AMF0EncodedCommandOrData
      {
      public:
        Command_FCPublish(unsigned int transactionID, wstring streamName) :
          AMF0EncodedCommandOrData(L"FCPublish", 0, transactionID)
        {
          _entities.push_back(make_shared<AMF0Entity>(AMF0TypeMarker::Null));
          _entities.push_back(make_shared<AMF0Entity>(streamName));

          Encode();


          _messageLength = (unsigned int) _payload->size();
        }
      };





      class  Command_PublishStream : public AMF0EncodedCommandOrData
      {
      public:
        Command_PublishStream(unsigned int transactionID, unsigned int messageStreamID, wstring streamName, wstring publishType) :
          AMF0EncodedCommandOrData(L"publish", messageStreamID, transactionID)
        {
          _entities.push_back(make_shared<AMF0Entity>(AMF0TypeMarker::Null));
          _entities.push_back(make_shared<AMF0Entity>(streamName));
          _entities.push_back(make_shared<AMF0Entity>(publishType));

          Encode();


          _messageLength = (unsigned int) _payload->size();
        }
      };

      class  Command_UnpublishStream : public AMF0EncodedCommandOrData
      {
      public:
        Command_UnpublishStream(unsigned int transactionID, unsigned int messageStreamID) :
          AMF0EncodedCommandOrData(L"publish", messageStreamID, transactionID)
        {
          _entities.push_back(make_shared<AMF0Entity>(AMF0TypeMarker::Null));
          _entities.push_back(make_shared<AMF0Entity>(false));

          Encode();


          _messageLength = (unsigned int) _payload->size();
        }
      };

      class  Command_CloseStream : public AMF0EncodedCommandOrData
      {
      public:
        Command_CloseStream(unsigned int transactionID, unsigned int messageStreamID) :
          AMF0EncodedCommandOrData(L"closeStream", messageStreamID, transactionID)
        {
          _entities.push_back(make_shared<AMF0Entity>(AMF0TypeMarker::Null));
          Encode();
          _messageLength = (unsigned int) _payload->size();
        }
      };





      class Command_SetDataFrame : public AMF0EncodedCommandOrData
      {
      public:
        Command_SetDataFrame(unsigned int transactionID, unsigned int messageStreamID,
          double videoFrameRate,
          unsigned int videoFrameWidth,
          unsigned int videoFrameHeight,
          wstring videoCodec,
          unsigned int videoBitrate,
          unsigned int videoKeyFrameFrequency,
          wstring audioCodec,
          unsigned int audioSampleRate,
          unsigned int audioChannels,
          unsigned int audioBitrate) :
          AMF0EncodedCommandOrData(L"@setDataFrame", messageStreamID)
        {

          _entities.push_back(make_shared<AMF0Entity>(wstring(L"onMetaData")));

          shared_ptr<std::vector<tuple<wstring, shared_ptr<AMF0Entity>>>> props
            = make_shared<std::vector<tuple<wstring, shared_ptr<AMF0Entity>>>>();

          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"framerate", make_shared<AMF0Entity>(videoFrameRate)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"width", make_shared<AMF0Entity>((double) videoFrameWidth)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"height", make_shared<AMF0Entity>((double) videoFrameHeight)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"videocodecid", make_shared<AMF0Entity>(videoCodec)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"videodatarate", make_shared<AMF0Entity>((double) videoBitrate)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"videokeyframe_frequency", make_shared<AMF0Entity>((double) videoKeyFrameFrequency)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"audiocodecid", make_shared<AMF0Entity>(audioCodec)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"audiosamplerate", make_shared<AMF0Entity>((double) audioSampleRate)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"audiochannels", make_shared<AMF0Entity>((double) audioChannels)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"audiodatarate", make_shared<AMF0Entity>((double) audioBitrate)));

          _entities.push_back(make_shared<AMF0Entity>(props));

          Encode();


          _messageLength = (unsigned int) _payload->size();
        }

        Command_SetDataFrame(unsigned int transactionID, unsigned int messageStreamID,

          wstring audioCodec,
          unsigned int audioSampleRate,
          unsigned int audioChannels,
          unsigned int audioBitrate) :
          AMF0EncodedCommandOrData(L"@setDataFrame", messageStreamID)
        {

          _entities.push_back(make_shared<AMF0Entity>(wstring(L"onMetaData")));

          shared_ptr<std::vector<tuple<wstring, shared_ptr<AMF0Entity>>>> props
            = make_shared<std::vector<tuple<wstring, shared_ptr<AMF0Entity>>>>();


          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"audiocodecid", make_shared<AMF0Entity>(audioCodec)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"audiosamplerate", make_shared<AMF0Entity>((double) audioSampleRate)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"audiochannels", make_shared<AMF0Entity>((double) audioChannels)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"audiodatarate", make_shared<AMF0Entity>((double) audioBitrate)));

          _entities.push_back(make_shared<AMF0Entity>(props));

          Encode();


          _messageLength = (unsigned int) _payload->size();
        }


        Command_SetDataFrame(unsigned int transactionID, unsigned int messageStreamID,
          double videoFrameRate,
          unsigned int videoFrameWidth,
          unsigned int videoFrameHeight,
          wstring videoCodec,
          unsigned int videoBitrate,
          unsigned int videoKeyFrameFrequency) :
          AMF0EncodedCommandOrData(L"@setDataFrame", messageStreamID)
        {

          _entities.push_back(make_shared<AMF0Entity>(wstring(L"onMetaData")));

          shared_ptr<std::vector<tuple<wstring, shared_ptr<AMF0Entity>>>> props
            = make_shared<std::vector<tuple<wstring, shared_ptr<AMF0Entity>>>>();

          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"framerate", make_shared<AMF0Entity>(videoFrameRate)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"width", make_shared<AMF0Entity>((double) videoFrameWidth)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"height", make_shared<AMF0Entity>((double) videoFrameHeight)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"videocodecid", make_shared<AMF0Entity>(videoCodec)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"videodatarate", make_shared<AMF0Entity>((double) videoBitrate)));
          props->push_back(tuple<wstring, shared_ptr<AMF0Entity>>(L"videokeyframe_frequency", make_shared<AMF0Entity>((double) videoKeyFrameFrequency)));

          _entities.push_back(make_shared<AMF0Entity>(props));

          Encode();


          _messageLength = (unsigned int) _payload->size();
        }

      };





      class  Command_Result : public AMF0EncodedCommandOrData
      {
      public:
        Command_Result(std::vector<shared_ptr<AMF0Entity>> entities, unsigned int messageLength, unsigned int messageStreamID) :
          AMF0EncodedCommandOrData(entities, messageLength, messageStreamID)
        {
        }
      };





      class  Command_Error : public AMF0EncodedCommandOrData
      {
      public:
        Command_Error(std::vector<shared_ptr<AMF0Entity>> entities, unsigned int messageLength, unsigned int messageStreamID) :
          AMF0EncodedCommandOrData(entities, messageLength, messageStreamID)
        {
        }
      };





      class  Command_Status : public AMF0EncodedCommandOrData
      {
      public:
        Command_Status(std::vector<shared_ptr<AMF0Entity>> entities, unsigned int messageLength, unsigned int messageStreamID) :
          AMF0EncodedCommandOrData(entities, messageLength, messageStreamID)
        {
        }

      };

    }
  }
}