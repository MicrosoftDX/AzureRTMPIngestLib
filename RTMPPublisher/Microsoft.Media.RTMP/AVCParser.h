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

#include "pch.h"
#include <memory>
#include <vector>
#include <wtypes.h>
#include "BitOp.h"


using namespace std;


namespace Microsoft
{
  namespace Media
  {
    namespace RTMP
    {


      enum NALUType : BYTE
      {
        NALUTYPE_UNSPECIFIED = 0,
        NALUTYPE_CODEDSLICE_NONIDR = 1,
        NALUTYPE_CODEDSLICE_DATAPART_A = 2,
        NALUTYPE_CODEDSLICE_DATAPART_B = 3,
        NALUTYPE_CODEDSLICE_DATAPART_C = 4,
        NALUTYPE_CODEDSLICEIDR = 5,
        NALUTYPE_SEI = 6,
        NALUTYPE_SPS = 7,
        NALUTYPE_PPS = 8,
        NALUTYPE_AUD = 9,
        NALUTYPE_EOSEQ = 10,
        NALUTYPE_EOSTM = 11,
        NALUTYPE_FILLER = 12,
        NALUTYPE_SPS_EST = 13,
        NALUTYPE_PREFIX = 14,
        NALUTYPE_SUBSET_SPS = 15,
        NALUTYPE_SLICELAYER_NOPART = 19,
        NALUTYPE_SLICE_EXT = 20,
      };

      enum SEIType : short
      {
        SEITYPE_ITUT_T35 = 4, SEITYPE_NOTRELEVANT = 0
      };

      class NALUnit
      {
      protected:

        std::vector<BYTE> _naluDataEmulPrevBytesRemoved;
        unsigned int _length;
        NALUType _nalutype;
        const BYTE* _naluData;
      public:

        NALUType GetType()
        {
          return _nalutype;
        }

        unsigned int GetLength()
        {
          return _naluDataEmulPrevBytesRemoved.size() > 0 ? (unsigned int) _naluDataEmulPrevBytesRemoved.size() : _length;
        }

        const BYTE* GetData()
        {
          return _naluDataEmulPrevBytesRemoved.size() > 0 ? &(*(_naluDataEmulPrevBytesRemoved.begin())) : _naluData;
        }



        NALUnit(NALUType type, const BYTE *data, unsigned int len) :
          _nalutype(type), _length(len), _naluData(data)
        {
          //CleanRBSPOfEmulPreventionBytes();
        }


        void CleanRBSPOfEmulPreventionBytes()
        {
          std::vector<BYTE> vecnaluDataEmulByteRemoved;
          unsigned int readctr = 0;
          while (_naluData[readctr++] != 0x01); //skip prefix
          readctr++;//skip NALU header;
          bool foundEmulPreventByte = false;
          while (readctr < _length)
          {
            if (readctr + 2 < _length && BitOp::ToInteger<unsigned int>(_naluData + readctr, 3) == 0x000003)//emulation prevention
            {
              vecnaluDataEmulByteRemoved.push_back(_naluData[readctr++]);
              vecnaluDataEmulByteRemoved.push_back(_naluData[readctr++]);
              readctr++;//emulation prevention byte skipped;
              _length--;
              foundEmulPreventByte = true;
            }
            else
              vecnaluDataEmulByteRemoved.push_back(_naluData[readctr++]);
          }
          if (!foundEmulPreventByte)
            vecnaluDataEmulByteRemoved.clear();

        }

      };



      class AVCParser
      {
      public:
        static std::vector<shared_ptr<NALUnit>> Parse(std::vector<BYTE>& sampledata)
        {

          std::vector<shared_ptr<NALUnit>> retval;

          const BYTE* elemData = &(*(sampledata.begin()));


          unsigned int size = (unsigned int) sampledata.size();
          unsigned int readbytecount = 0;
          bool EOS = false;

          //skip leading zero bytes and position on first prefix
          while (elemData[readbytecount] == 0x00 && BitOp::ToInteger<unsigned int>(elemData + readbytecount, 3) != 0x000001)
            ++readbytecount;

          if (readbytecount + 3 >= size) //nothing to parse
            return retval;

          while (true)
          {

            unsigned int NumBytesInNALUnit = 0;
            unsigned int BitSequenceMatchPos = 0;
            //find the next prefix match - after this prefix
            if (FindNextMatchingBitSequence(0x000001, 3, readbytecount + 3, elemData, size, BitSequenceMatchPos) == true)
              //if found - find the number of bytes in the NALU
              NumBytesInNALUnit = BitSequenceMatchPos - readbytecount - 3;
            else
              NumBytesInNALUnit = size - readbytecount - 3;
            //process NALU here

            //read a byte (after the prefix) and skip over first 3 bits(forbidden bit(1 bit) + nal_ref_idc(2 bits) to get nal_unit_type
            auto nal_unit_type = BitOp::ExtractBits(elemData[readbytecount + 3], 3, 5);


            retval.push_back(std::make_shared<NALUnit>((NALUType) nal_unit_type, elemData + readbytecount + 3, NumBytesInNALUnit));

            //done processing
            readbytecount += NumBytesInNALUnit + 3;

            if (BitSequenceMatchPos == size || size - readbytecount <= 3) //we did not find the start of another NALU last time we checked - or nothing else left to parse
              break;
          }
          return retval;

        }



        static bool FindNextMatchingBitSequence(unsigned int sequencevalue, unsigned short numbits, unsigned int startat, const BYTE *data, unsigned int size, unsigned int& MatchPos)
        {
          MatchPos = size;
          if (size < numbits) return false;

          while (startat + numbits <= size)
          {
            if (BitOp::ToInteger<unsigned int>(data + startat, numbits) != sequencevalue)
              ++startat;
            else
            {
              MatchPos = startat;
              break;
            }
          }

          return (startat + numbits <= size);
        }
      };
    }
  }
}
