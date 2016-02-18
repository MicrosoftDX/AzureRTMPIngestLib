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

#include <string>
#include <algorithm>


namespace Microsoft
{
  namespace Media
  {
    namespace RTMP
    {

      class Uri
      {


      public:

        static const Uri Parse(std::wstring uristring)
        {
          Uri uri;
          auto scheme_sep_pos = uristring.find(L"://", 0);
          if (scheme_sep_pos == wstring::npos)
            uri._isRelative = true;

          if (uri._isRelative == false)
          {
            uri._scheme = uristring.substr(0, scheme_sep_pos);
            //host + port
            wstring host_port;
            auto host_port_sep_pos = uristring.find(L"/", scheme_sep_pos + 3);
            host_port = (host_port_sep_pos == wstring::npos) ?
              uristring.substr(scheme_sep_pos + 3, uristring.size() - scheme_sep_pos - 3) :
              uristring.substr(scheme_sep_pos + 3, host_port_sep_pos - scheme_sep_pos - 3);

            auto port_sep_pos = host_port.find(L":", 0);
            if (port_sep_pos != wstring::npos)
            {
              uri._host = host_port.substr(0, port_sep_pos);
              uri._port = host_port.substr(port_sep_pos + 1, host_port.size() - port_sep_pos - 1);
            }
            else
              uri._host = host_port;


            uristring = uristring.substr(host_port_sep_pos + 1, uristring.size() - host_port_sep_pos - 1);
          }
          else
          {
            if (uristring[0] == '/')
              uristring = uristring.substr(1, uristring.size() - 1);
          }

          //query
          auto query_sep = uristring.find(L"?");
          if (query_sep != wstring::npos)
          {
            uri._path = uristring.substr(0, query_sep);
            uri._query = uristring.substr(query_sep + 1, uristring.size() - query_sep - 1);
          }
          else
            uri._path = uristring;

          return uri;
        }


        virtual ~Uri()
        {

        }


        wstring Scheme()
        {
          return _scheme;
        }
        wstring Host()
        {
          return _host;
        }
        wstring Port()
        {
          return _port;
        }
        wstring Query()
        {
          return _query;
        }
        wstring Path()
        {
          return _path;
        }
        bool IsRelative()
        {
          return _isRelative;
        }

      private:
        Uri() {}
        Uri(wstring scheme, wstring host, wstring port, wstring query, bool isrelative) : _scheme(scheme), _host(host), _port(port), _query(query), _isRelative(isrelative)
        {

        }


        bool _isRelative = false;
        wstring _scheme;
        wstring _host;
        wstring _port;
        wstring _path;
        wstring _query;
      };
    }
  }
}


