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

#include <wrl.h>
#include <mfapi.h>
#include <mfidl.h>
#include <Mferror.h>
#include <mfidl.h>
#include <wtypes.h>
#include <vector>

using namespace Microsoft::WRL;
using namespace std;

namespace Microsoft
{
  namespace Media
  {
    namespace RTMP
    {


      //supports single media type - equivalent to MFCreateSimpleTypeHandler
      class MediaTypeHandlerImpl : public RuntimeClass<RuntimeClassFlags<RuntimeClassType::ClassicCom>, IMFMediaTypeHandler, FtmBase>
      {
      public:
        MediaTypeHandlerImpl()
        {}

        HRESULT RuntimeClassInitialize(IMFMediaType *pSupportedMediaType)
        {
          _supportedMediaType = pSupportedMediaType;
          _supportedMediaType->GetGUID(MF_MT_MAJOR_TYPE, &_supportedMajorType);
          _supportedMediaType->GetGUID(MF_MT_SUBTYPE, &_supportedSubType);
          return S_OK;
        }




      private:
        ComPtr<IMFMediaType> _currentMediaType = nullptr;
        ComPtr<IMFMediaType> _supportedMediaType = nullptr;
        GUID _supportedMajorType = GUID_NULL;
        GUID _supportedSubType = GUID_NULL;

        GUID _currentMajorType = GUID_NULL;
        GUID _currentSubtype = GUID_NULL;


      };
    }
  }
}

