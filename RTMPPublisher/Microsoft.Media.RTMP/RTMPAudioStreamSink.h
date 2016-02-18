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


#include <mfidl.h>
#include <mfapi.h>
#include <Mferror.h>
#include <wrl.h>
#include <memory>
#include <map>
#include <windows.media.mediaproperties.h> 
#include "RTMPStreamSinkBase.h"

using namespace Microsoft::WRL;
using namespace Windows::Media::MediaProperties;
using namespace std;

namespace Microsoft
{
  namespace Media
  {
    namespace RTMP
    {

      class RTMPAudioStreamSink : public RTMPStreamSinkBase
      {
      public:
        RTMPAudioStreamSink()
        {
          _samplingFrequencyIndex = std::map<unsigned int, BYTE>{
            std::pair<unsigned int, BYTE>(96000, 0x0),
            std::pair<unsigned int, BYTE>(88200, 0x1),
            std::pair<unsigned int, BYTE>(64000, 0x2),
            std::pair<unsigned int, BYTE>(48000, 0x3),
            std::pair<unsigned int, BYTE>(44100, 0x4),
            std::pair<unsigned int, BYTE>(32000, 0x5),
            std::pair<unsigned int, BYTE>(24000, 0x6),
            std::pair<unsigned int, BYTE>(22050, 0x7),
            std::pair<unsigned int, BYTE>(16000, 0x8),
            std::pair<unsigned int, BYTE>(12000, 0x9),
            std::pair<unsigned int, BYTE>(11025, 0xa),
            std::pair<unsigned int, BYTE>(8000, 0xb),
            std::pair<unsigned int, BYTE>(7350, 0xc)
          };
        }

        IFACEMETHOD(ProcessSample)(IMFSample *pSample);

        IFACEMETHOD(PlaceMarker)(MFSTREAMSINK_MARKER_TYPE eMarkerType, const PROPVARIANT *pvarMarkerValue, const PROPVARIANT *pvarContextValue);

        virtual HRESULT CreateMediaType(MediaEncodingProfile^ encodingProfile) override;

        LONGLONG GetLastDTS() override;

        LONGLONG GetLastPTS() override;

      protected:
        std::vector<BYTE> PreparePayload(MediaSampleInfo* pSampleInfo, bool firstPacket = false);

        void RTMPAudioStreamSink::PrepareTimestamps(MediaSampleInfo* sampleInfo, LONGLONG& PTS, LONGLONG& TSDelta);

        HRESULT CompleteProcessNextWorkitem(IMFAsyncResult *pAsyncResult) override;

        std::vector<BYTE> MakeAudioSpecificConfig(MediaSampleInfo* pSampleInfo);

        std::map<unsigned int, BYTE> _samplingFrequencyIndex;
      };


    }
  }
}

