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

#include <windows.media.mediaproperties.h>
#include <wrl.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h> 
#include "PublishProfile.h"

using namespace Windows::Media::MediaProperties;
using namespace Microsoft::WRL;

namespace Microsoft
{
  namespace Media
  {
    namespace RTMP
    {
      class RTMPPublisherSink;

      class ProfileState
      {

      public:
        ProfileState(PublishProfile^ profile, ComPtr<RTMPPublisherSink> delegateSink, ComPtr<IMFSinkWriter> delegateWriter) :
          PublishProfile(profile), 
          DelegateWriter(delegateWriter), 
          DelegateSink(delegateSink)
        {

        }
 

        PublishProfile^ PublishProfile;
        ComPtr<IMFSinkWriter> DelegateWriter;
        ComPtr<IMFSinkWriterCallback> DelegateSinkCallback;
        ComPtr<RTMPPublisherSink> DelegateSink; 
      };


    }
  }
}