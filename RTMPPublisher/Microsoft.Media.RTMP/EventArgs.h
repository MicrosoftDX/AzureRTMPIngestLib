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

using namespace Platform;

namespace Microsoft
{
  namespace Media
  {
    namespace RTMP
    {
      [Windows::Foundation::Metadata::Threading(Windows::Foundation::Metadata::ThreadingModel::Both)]
      [Windows::Foundation::Metadata::MarshalingBehavior(Windows::Foundation::Metadata::MarshalingType::Agile)]
      public ref class FailureEventArgs sealed
      {
      public:

        property String^ Message
        {
          String^ get()
          {
            return _message;
          }
        }

        property int ErrorCode
        {
          int get()
          {
            return _errcode;
          }
        }


      internal:
        FailureEventArgs(int errcode) : _errcode(errcode) {}
        FailureEventArgs(int errcode, String^ message) : _errcode(errcode), _message(message) {}
        FailureEventArgs(COMException^ ex) : _errcode(ex->HResult), _message(ex->Message) {}
      private:

        String^ _message = L"Unknown Error";
        int _errcode = 0;

      };

      [Windows::Foundation::Metadata::Threading(Windows::Foundation::Metadata::ThreadingModel::Both)]
      [Windows::Foundation::Metadata::MarshalingBehavior(Windows::Foundation::Metadata::MarshalingType::Agile)]
      public ref class SessionClosedEventArgs sealed
      {
      public:

        property String^ Endpoint
        {
          String^ get()
          {
            return _endpointUri;
          }

        }

        property String^ StreamName
        {
          String^ get()
          {
            return _streamName;
          }

        }

        property LONGLONG LastVideoTimestamp
        {
          LONGLONG get()
          {
            return _lastvideoTS;
          }
        }

        property LONGLONG LastAudioTimestamp
        {
          LONGLONG get()
          {
            return _lastaudioTS;
          }

        }

      internal:
        SessionClosedEventArgs(String^ endpoint, String^ streamname, LONGLONG lastvideots, LONGLONG lastaudiots)
          :_endpointUri(endpoint), _streamName(streamname), _lastvideoTS(lastvideots), _lastaudioTS(lastaudiots)
        {

        }


      private:

        String^ _streamName = nullptr;

        LONGLONG _lastvideoTS = -1;

        LONGLONG _lastaudioTS = -1;

        String^ _endpointUri = nullptr;

      };
    }
  }
}
