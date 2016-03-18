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
#include <wtypes.h> 
#include <string>
#include "Constants.h"

using namespace std;
using namespace Platform;
using namespace Windows::Media::MediaProperties;

namespace Microsoft
{
  namespace Media
  {
    namespace RTMP
    {

      [Windows::Foundation::Metadata::Threading(Windows::Foundation::Metadata::ThreadingModel::Both)]
      [Windows::Foundation::Metadata::MarshalingBehavior(Windows::Foundation::Metadata::MarshalingType::Agile)]
      public ref class PublishProfile sealed
      {

      public:
        virtual ~PublishProfile() {}

        PublishProfile(RTMPServerType rtmpServerType) :
          _RTMPServerType(rtmpServerType)
        {
          WCHAR buff[128];
          GUID id = GUID_NULL;
          CoCreateGuid(&id);
          int len = ::StringFromGUID2(id, buff, 128);

          auto retval = std::wstring(buff);
          retval = retval.substr(1, retval.size() - 2);
          std::replace(retval.begin(), retval.end(), L'-', L'0');

          _streamName = ref new String(retval.c_str());
        }

        PublishProfile(RTMPServerType rtmpServerType, String^ endpointUri) :
          _RTMPServerType(rtmpServerType), _endpointUri(endpointUri)
        {
          WCHAR buff[128];
          GUID id = GUID_NULL;
          CoCreateGuid(&id);
          int len = ::StringFromGUID2(id, buff, 128);

          auto retval = std::wstring(buff);
          retval = retval.substr(1, retval.size() - 2);
          std::replace(retval.begin(), retval.end(), L'-', L'0');

          _streamName = ref new String(retval.c_str());
        }


        PublishProfile(RTMPServerType rtmpServerType, String^ endpointUri, MediaEncodingProfile^ targetEncodingProfile) :
          PublishProfile(rtmpServerType, endpointUri)
        {
          _targetEncodingProfile = targetEncodingProfile;
        }

        PublishProfile(RTMPServerType rtmpServerType, String^ endpointUri, MediaEncodingProfile^ targetEncodingProfile , unsigned int keyframeinterval) :
          PublishProfile(rtmpServerType, endpointUri,targetEncodingProfile)
        {
          _keyframeinterval = keyframeinterval;
        }

        PublishProfile(RTMPServerType rtmpServerType, String^ endpointUri, MediaEncodingProfile^ targetEncodingProfile, unsigned int keyframeinterval, unsigned int clientChunkSize) :
          PublishProfile(rtmpServerType, endpointUri,targetEncodingProfile, keyframeinterval)
        {
          _clientChunkSize = clientChunkSize;
        }

        PublishProfile(RTMPServerType rtmpServerType, String^ endpointUri, MediaEncodingProfile^ targetEncodingProfile, unsigned int keyframeinterval, unsigned int clientChunkSize, String^ streamName) :
          PublishProfile(rtmpServerType, endpointUri, targetEncodingProfile, keyframeinterval, clientChunkSize)
        {
          _streamName = streamName;
        }

        PublishProfile(RTMPServerType rtmpServerType, String^ endpointUri, MediaEncodingProfile^ targetEncodingProfile, unsigned int keyframeinterval, unsigned int clientChunkSize, LONGLONG lastvideots, LONGLONG lastaudiots) :
          PublishProfile(rtmpServerType, endpointUri, targetEncodingProfile,keyframeinterval, clientChunkSize)
        {
          _videoTSBase = lastvideots;
          _audioTSBase = lastaudiots;
        }

        PublishProfile(RTMPServerType rtmpServerType, String^ endpointUri, MediaEncodingProfile^ targetEncodingProfile, unsigned int keyframeinterval, unsigned int clientChunkSize, LONGLONG lastvideots, LONGLONG lastaudiots, String^ streamName) :
          PublishProfile(rtmpServerType, endpointUri, targetEncodingProfile, keyframeinterval, clientChunkSize, lastvideots, lastaudiots)
        {
          _streamName = streamName;
        }

       

        property String^ EndpointUri
        {
          String^ get()
          {
            return _endpointUri;
          }
          void set(String^ val)
          {
            _endpointUri = val;
          }
        }

        property String^ StreamName
        {
          String^ get()
          {
            return _streamName;
          }
          void set(String^ val)
          {
            _streamName = val;
          }
        }

        property unsigned int ClientChunkSize
        {
          unsigned int get()
          {
            return _clientChunkSize;
          }

          void set(unsigned int val)
          {
            _clientChunkSize = val < 128 ? 128 : val;
          }
        }

        property RTMPServerType ServerType
        {
          RTMPServerType get()
          {
            return _RTMPServerType;
          }

          void set(RTMPServerType val)
          {
            _RTMPServerType = val;
          }
        }

        property unsigned int  KeyFrameInterval
        {
          unsigned int get()
          {
            return _keyframeinterval;
          }

          void set(unsigned int val)
          {
            _keyframeinterval = val;
          }
        }

        property LONGLONG VideoTimestampBase
        {
          LONGLONG get()
          {
            return _videoTSBase;
          }

          void set(LONGLONG val)
          {
            _videoTSBase = val;
          }
        }

        property LONGLONG AudioTimestampBase
        {
          LONGLONG get()
          {
            return _audioTSBase;
          }

          void set(LONGLONG val)
          {
            _audioTSBase = val;
          }
        }

        property bool EnableLowLatency
        {
          bool get()
          {
            return _enableLowLatency;
          }
          void set(bool val)
          {
            _enableLowLatency = val;
          }
        }

        property bool DisableThrottling
        {
          bool get()
          {
            return _disableThrottling;
          }
          void set(bool val)
          {
            _disableThrottling = val;
          }
        }



        property MediaEncodingProfile^ TargetEncodingProfile
        {
          MediaEncodingProfile^ get()
          {
            return _targetEncodingProfile;
          }

          void set(MediaEncodingProfile^ val)
          {
            _targetEncodingProfile = val;
          }
        }

        property unsigned int BaseEpoch
        {
          unsigned int get()
          {
            return _baseEpoch;
          }

          void set(unsigned int val)
          {
            _baseEpoch = val;
          }
        }

      internal:
        PublishProfile()
        {
          WCHAR buff[128];
          GUID id = GUID_NULL;
          CoCreateGuid(&id);
          int len = ::StringFromGUID2(id, buff, 128);

          auto retval = std::wstring(buff);
          retval = retval.substr(1, retval.size() - 2);
          std::replace(retval.begin(), retval.end(), L'-', L'0');

          _streamName = ref new String(retval.c_str());
        }


      private:

        String^ _endpointUri = nullptr;

        String^ _streamName = nullptr;

        unsigned int _clientChunkSize = 128;

        RTMPServerType _RTMPServerType = RTMPServerType::Azure;

        unsigned int _keyframeinterval = 120; //120 frames = 4 seconds at 30 fps

        LONGLONG _videoTSBase = -1;

        LONGLONG _audioTSBase = -1;

        MediaEncodingProfile^ _targetEncodingProfile = nullptr;

        unsigned int _baseEpoch = 0U;

        bool _enableLowLatency = false;

        bool _disableThrottling = false;
      };

    }
  }
}

