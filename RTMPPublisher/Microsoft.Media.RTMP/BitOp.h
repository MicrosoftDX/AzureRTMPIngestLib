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
#include <memory>
#include <vector>
#include <assert.h>
#include <wrl.h>
#include <robuffer.h>
#include <Windows.Foundation.h>
#include <windows.storage.streams.h>

using namespace Windows::Storage::Streams;
using namespace Microsoft::WRL;

namespace Microsoft
{
  namespace Media
  {
    namespace RTMP
    {

      ///<summary>Bit manipulation operations</summary>
      class BitOp
      {
      public:


        ///<summary>Converts an integral type to a byte array and appends the result to the supplied byte array</summary>    
        ///<param name='in'>value to convert</param>
        ///<param name='bitstream'>byte array to append to</param>
        ///<param name='networkByteOrder'>change to big-endian if set to true</param>
        ///<param name='bitstream'>number of bytes to take from the converted value starting from least significant byte</param>

        ///<returns>void</returns>
        template<class T>
        static void AddToBitstream(T in,
          std::shared_ptr<std::vector<BYTE>> bitstream,
          bool networkByteOrder = true,
          unsigned int numbytes = 0)
        {
          if (bitstream == nullptr)
            throw invalid_argument("Null pointer");

          if (numbytes == 0 || numbytes > sizeof(T)) //clamp
            numbytes = sizeof(T);



          if (!networkByteOrder)
          {
            for (unsigned int i = 0; i < numbytes; i++)
            {
              bitstream->push_back(in & 0xFF);
              in >>= 8;
            }

          }
          else
          {
            vector<BYTE> temp;
            for (unsigned int i = 0; i < numbytes; i++)
            {
              temp.push_back(in & 0xFF);
              in >>= 8;
            }

            for (auto itr = rbegin(temp); itr != rend(temp); itr++)
              bitstream->push_back(*itr);
          }

        }

        template<class T>
        static void AddToBitstream(T in,
          std::vector<BYTE>& bitstream,
          bool networkByteOrder = true,
          unsigned int numbytes = 0)
        {

          if (numbytes == 0 || numbytes > sizeof(T)) //clamp
            numbytes = sizeof(T);



          if (!networkByteOrder)
          {
            for (unsigned int i = 0; i < numbytes; i++)
            {
              bitstream.push_back(in & 0xFF);
              in >>= 8;
            }

          }
          else
          {
            vector<BYTE> temp;
            for (unsigned int i = 0; i < numbytes; i++)
            {
              temp.push_back(in & 0xFF);
              in >>= 8;
            }

            for (auto itr = rbegin(temp); itr != rend(temp); itr++)
              bitstream.push_back(*itr);
          }

        }


        static void AddToBitstream(std::wstring in,
          std::vector<BYTE>& bitstream)
        {
          auto oldsize = bitstream.size();

          auto buffsize = ::WideCharToMultiByte(CP_UTF8, 0, in.c_str(), -1, nullptr, 0, nullptr, nullptr);
          std::vector<CHAR> buffer;
          buffer.resize(buffsize);
          ::WideCharToMultiByte(CP_UTF8, 0, in.c_str(), -1, &(*(buffer.begin())), buffsize, nullptr, nullptr);

          bitstream.resize(oldsize + buffsize);


          if (buffer[buffer.size() - 1] == '\0')
          {
            bitstream.resize(oldsize + buffsize - 1);
            memcpy_s(&(*(bitstream.begin() + oldsize)), buffsize - 1, &(*(buffer.begin())), buffsize - 1); //we do not want the terminating null
          }
          else
          {
            bitstream.resize(oldsize + buffsize);
            memcpy_s(&(*(bitstream.begin() + oldsize)), buffsize, &(*(buffer.begin())), buffsize);
          }
          return;
        }

        static void AddToBitstream(std::wstring in,
          std::shared_ptr<std::vector<BYTE>> bitstream)
        {
          if (bitstream == nullptr)
            throw invalid_argument("Null pointer");
          auto oldsize = bitstream->size();

          auto buffsize = ::WideCharToMultiByte(CP_UTF8, 0, in.c_str(), -1, nullptr, 0, nullptr, nullptr);
          std::vector<CHAR> buffer;
          buffer.resize(buffsize);
          ::WideCharToMultiByte(CP_UTF8, 0, in.c_str(), -1, &(*(buffer.begin())), buffsize, nullptr, nullptr);



          if (buffer[buffer.size() - 1] == '\0')
          {
            bitstream->resize(oldsize + buffsize - 1);
            memcpy_s(&(*(bitstream->begin() + oldsize)), buffsize - 1, &(*(buffer.begin())), buffsize - 1); //we do not want the terminating null
          }
          else
          {
            bitstream->resize(oldsize + buffsize);
            memcpy_s(&(*(bitstream->begin() + oldsize)), buffsize, &(*(buffer.begin())), buffsize);
          }
          return;
        }

        static wstring ToWideString(char* mbstr, unsigned int len)
        {

          /*  if (mbstr[len - 1] == '\0')
            {*/
          auto buffsize = ::MultiByteToWideChar(CP_UTF8, 0, mbstr, len, nullptr, 0);
          std::vector<wchar_t> buffer;
          buffer.resize(buffsize);
          ::MultiByteToWideChar(CP_UTF8, 0, mbstr, len, &(*(buffer.begin())), buffsize);

          return wstring(&(*(buffer.begin())), buffsize);
          /*}
          else
          {
            std::vector<char> terminated;
            terminated.resize(len + 1,'\0');
            memcpy_s(&(*(terminated.begin())), len, mbstr, len);

            auto buffsize = ::MultiByteToWideChar(CP_UTF8, 0, &(*(terminated.begin())), terminated.size(), nullptr, 0);
            std::vector<wchar_t> buffer;
            buffer.resize(buffsize);
            ::MultiByteToWideChar(CP_UTF8, 0, &(*(terminated.begin())), terminated.size(), &(*(buffer.begin())), buffsize);

            return wstring(&(*(buffer.begin())), buffsize);
          }*/
        }


        ///<summary>Converts a byte array to a specific integral type T</summary>    
        ///<param name='data'>Byte array</param>
        ///<param name='size'>array length</param>
        ///<returns>A value of type T</returns>
        template<typename T>
        static T ToInteger(const BYTE* data, unsigned short size, bool SwitchByteOrder = false)
        {
          T ret = 0;
          if (SwitchByteOrder)
          {
            for (unsigned short i = size; i > 0; i--)
            {
              if (i < size && ret != 0) ret <<= 8;
              memcpy_s(&ret, 1, data + (i - 1), 1);
            }
          }
          else
          {
            for (unsigned short i = 0; i < size; i++)
            {
              if (i > 0 && ret != 0) ret <<= 8;
              memcpy_s(&ret, 1, data + i, 1);
            }
          }

          return ret;
        }



        ///<summary>Pads a byte array with leading zero's</summary>
        ///<param name='data'>Byte array</param>
        ///<param name='numBytes'>Array size</param>
        ///<param name='targetNumBytes'>Desired array size after padding</param>
        ///<returns>A vector of BYTE</returns>
        static std::shared_ptr<std::vector<BYTE>> PadZeroLeading(BYTE *data, unsigned int numBytes, unsigned int targetNumBytes)
        {
          auto ret = std::make_shared<std::vector<BYTE>>();
          for (unsigned int num = 0; num < targetNumBytes; num++)
          {
            if (num < targetNumBytes - numBytes)
              ret->push_back(0);
            else
              ret->push_back(data[num - (targetNumBytes - numBytes)]);
          }

          return ret;
        }

        ///<summary>Extracts a specified number of bits from a value of integral type T starting at a specified position</summary>
        ///<param name='in'>Value of type T</param>
        ///<param name='startat'>Position to start extraction at</param>
        ///<param name='numbits'>Number of bits to extract</param>
        ///<returns>The resultant bits as a value of the source type</returns>
        template<typename T>
        static T ExtractBits(T in, unsigned short startat, unsigned short numbits)
        {
          unsigned short size = sizeof(in) * 8;

          assert((numbits > 0) && (startat >= 0) && (startat + numbits <= size));

          if (startat == 0)
          {
            if (startat + numbits == size) //need the whole thing
              return in;
            else
              return in >> (size - numbits); //need the first numbits bits
          }
          else
          {
            if (startat + numbits == size) //we need last numbits bits
              return in ^ ((in >> numbits) << numbits);
            else
            {
              T temp = in >> (size - (startat + numbits));
              return temp ^ ((temp >> numbits) << numbits);
            }
          }
        }

        static std::vector<BYTE> BufferToVector(IBuffer^ buffer)
        {
          ComPtr<IBufferByteAccess> cpbufferbytes;
          std::vector<BYTE> retvec;

          if (buffer->Length == 0)
            return retvec;
          if (SUCCEEDED(reinterpret_cast<IInspectable*>(buffer)->QueryInterface(IID_PPV_ARGS(&cpbufferbytes))))
          {

            retvec.resize(buffer->Length);
            BYTE* tmpbuff = nullptr;
            cpbufferbytes->Buffer(&tmpbuff);
            memcpy_s(&(*(retvec.begin())), buffer->Length, tmpbuff, buffer->Length);

            cpbufferbytes.Reset();
          }

          return retvec;
        }

        static IBuffer^ VectorToBuffer(const std::vector<BYTE>& vec)
        {

          Buffer^ retbuff = ref new Buffer((unsigned int) vec.size());
          ComPtr<IBufferByteAccess> cpbufferbytes;

          if (SUCCEEDED(reinterpret_cast<IInspectable*>(retbuff)->QueryInterface(IID_PPV_ARGS(&cpbufferbytes))))
          {
            BYTE* tmpbuff = nullptr;
            cpbufferbytes->Buffer(&tmpbuff);
            memcpy_s(tmpbuff, vec.size(), &(*(vec.begin())), vec.size());

            cpbufferbytes.Reset();
          }

          return retbuff;
        }


        static unsigned long long FromDoubleToFPRep(double a)
        {

          if (a == 0)
            return 0;

          int whole = (int) trunc(abs(a));
          double fraction = abs(a) - whole;
          std::vector<BYTE> bin;

          while (whole != 0 && whole != 1)
          {
            bin.push_back(whole % 2);
            whole >>= 1;
          }
          bin.push_back(whole);
          std::reverse(bin.begin(), bin.end());
          while (bin.size() > 0 && bin.front() != 1)
            bin.erase(bin.begin()); //move to 1

          int wholedigits = (int) bin.size();

          double tmp = fraction * 2;

          while (trunc(tmp) != tmp)
          {
            bin.push_back((BYTE) trunc(tmp));
            if (bin.size() == 53) break;
            tmp -= trunc(tmp);
            tmp *= 2;
          }

          if (bin.size() < 53) bin.push_back((BYTE) trunc(tmp));

          unsigned long long exponent = 0;
          if (wholedigits > 0)
          {
            exponent = 1023 + wholedigits - 1;
            bin.erase(bin.begin()); //erase 1
          }
          else
          {
            int ctr = 0;
            while (bin.front() == 0)
            {
              bin.erase(bin.begin());
              ctr++;
            }
            bin.erase(bin.begin()); //erase 1
            exponent = 1023 - (ctr + 1);
          }




          //add zeros to make mantissa 52 bits
          while (bin.size() < 52)
            bin.push_back(0);

          unsigned long long mantissa = 0;
          int pos = 0;
          for (auto itr = bin.rbegin(); itr != bin.rend(); itr++, pos++)
            mantissa += (*itr) * (unsigned long long)pow(2, pos);

          unsigned long long rep = 0;
          rep |= mantissa;
          exponent <<= 52;
          rep |= exponent;


          if (a < 0)
          {
            unsigned long long sign = 1;
            sign <<= 63;
            rep |= sign;
          }

          return rep;
        }




        static double FromFPRepToDouble(unsigned long long b)
        {

          if (b == 0)
            return 0;

          double wholepart = 0;
          double fracpart = 0;

          BYTE sign = (BYTE) BitOp::ExtractBits<unsigned long long>(b, 0, 1);
          int exponent = (int) BitOp::ExtractBits<unsigned long long>(b, 1, 11) - 1023;

          unsigned long long mantissa = BitOp::ExtractBits<unsigned long long>(b, 12, 52);

          std::vector<BYTE> bin;
          //mantissa to binary
          while (bin.size() < 52)
          {
            bin.push_back(mantissa % 2);
            mantissa >>= 1;
          }



          if (exponent >= 0)
          {

            bin.push_back(1);
            std::reverse(bin.begin(), bin.end());


            for (int i = exponent; i >= 0; i--)
              wholepart += bin[i] * pow(2, exponent - i);

            auto size = (int) bin.size();

            for (int j = size - 1; j != exponent; j--)
            {
              fracpart += bin[j];
              fracpart /= 2;
            }
          }
          else if (exponent < 0)
          {
            bin.push_back(1);

            for (int i = 0; i < abs(exponent) - 1; i++)
              bin.push_back(0);

            std::reverse(bin.begin(), bin.end());

            auto size = (int) bin.size();

            for (int j = size - 1; j >= 0; j--)
            {
              fracpart += bin[j];
              fracpart /= 2;
            }


          }

          return sign == 1 ? -1 * (wholepart + fracpart) : wholepart + fracpart;
        }

      };
    }

  }
}