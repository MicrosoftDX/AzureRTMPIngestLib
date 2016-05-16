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
#include <tuple>
#include <wrl.h>
#include <memory> 


#define MILLISTOTICKS 10000;
#define TICKSTOMILLIS 0.0001

namespace Microsoft
{
  namespace Media
  {
    namespace RTMP
    {


      class ChunkStreamIDValue
      {
      public:
        static const unsigned int MAX = 65599;
        static const unsigned int MIN = 3;

      };

      class RTMPMessageType
      {
      public:
        static const BYTE PROTOSETCHUNKSIZE = 1;
        static const BYTE PROTOABORT = 2;
        static const BYTE PROTOACKNOWLEDGEMENT = 3;
        static const BYTE PROTOACKWINDOWSIZE = 5;
        static const BYTE PROTOSETPEERBANDWIDTH = 6;
        static const BYTE USERCONTROL = 4;
        static const BYTE COMMANDAMF0 = 20;
        static const BYTE COMMANDAMF3 = 17;
        static const BYTE DATAAMF0 = 18;
        static const BYTE DATAAMF3 = 15;
        static const BYTE SHAREDOBJECTAMF0 = 19;
        static const BYTE SHAREDOBJECTAMF3 = 16;
        static const BYTE AUDIO = 8;
        static const BYTE VIDEO = 9;
        static const BYTE AGGREGATE = 22;

      };

      class UserControlMessageType
      {
      public:
        static const unsigned short StreamBegin = 0;
        static const unsigned short StreamEOF = 1;
        static const unsigned short StreamDry = 2;
        static const unsigned short SetBufferLength = 3;
        static const unsigned short StreamIsRecorded = 4;
        static const unsigned short PingRequest = 6;
        static const unsigned short PrinResponse = 7;

      };

      class RTMPChunkType
      {
      public:
        static const BYTE Type0 = 0;
        static const BYTE Type1 = 1;
        static const BYTE Type2 = 2;
        static const BYTE Type3 = 3;
      };


      class BandwidthLimitType
      {
      public:
        static const BYTE Hard = 0;
        static const BYTE Soft = 1;
        static const BYTE Dynamic = 2;
      };

      class AMF0TypeMarker
      {
      public:
        static const BYTE Number = 0x00;
        static const BYTE Boolean = 0x01;
        static const BYTE String = 0x02;
        static const BYTE Object = 0x03;
        static const BYTE MovieClip = 0x04;
        static const BYTE Null = 0x05;
        static const BYTE Undefined = 0x06;
        static const BYTE Reference = 0x07;
        static const BYTE EcmaArray = 0x08;
        static const BYTE ObjectEnd = 0x09;
        static const BYTE StrictArray = 0x0A;
        static const BYTE Date = 0x0B;
        static const BYTE LongString = 0x0C;
        static const BYTE Unsupported = 0x0D;
        static const BYTE Recordset = 0x0E;
        static const BYTE XmlDocument = 0x0F;
        static const BYTE TypedObject = 0x10;

      };

      class RTMPAudioCodecFlag
      {
      public:
        static const unsigned short SUPPORT_SND_NONE = 0x0001;
        static const unsigned short SUPPORT_SND_ADPCM = 0x0002;
        static const unsigned short SUPPORT_SND_MP3 = 0x0004;
        static const unsigned short SUPPORT_SND_AAC = 0x0400;
      };

      class RTMPVideoCodecFlag
      {
      public:
        static const unsigned short SUPPORT_VID_H264 = 0x0080;
      };

      class RTMPPublishType
      {
      public:
        static const wchar_t *LIVE;
        static const wchar_t *RECORD;
        static const wchar_t *APPEND;
      };

      enum RTMPSessionState : int
      {
        RTMP_UNINITIALIZED,
        RTMP_RUNNING

      };

      enum SinkState : unsigned short
      {
        UNINITIALIZED = 0, RUNNING, EOS, PAUSED, STOPPED, SHUTDOWN
      };

      public enum class RTMPServerType
      {
        Azure = 0, Wowza = 1
      };

      DEFINE_GUID(MF_XVP_DISABLE_FRC, 0x2c0afa19, 0x7a97, 0x4d5a, 0x9e, 0xe8, 0x16, 0xd4, 0xfc, 0x51, 0x8d, 0x8c);
    
    }
  }
} 
