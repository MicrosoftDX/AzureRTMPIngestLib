
#pragma once

#define LOGGER_INCL
#include <pch.h>
#include <memory>
#include <ppltasks.h>
#include <sstream>
#include <string> 
#include <windows.storage.h>  
#include <windows.applicationmodel.core.h>
#include <windows.ui.core.h> 
#include <vector>
#include <windows.foundation.diagnostics.h>
#include <mutex>

using namespace std;
using namespace Concurrency;
using namespace Windows::Foundation::Diagnostics;

#if defined(_VSLOG) && defined(LOGGER_INCL) && defined(_DEBUG)


#define STREAM(s)\
{\
  wostringstream _log;\
  _log << s;\
}\

#define LOG(s) \
{\
  SYSTEMTIME systime;\
  GetSystemTime(&systime);\
  std::wostringstream _log;\
  _log <<"["<<systime.wHour<<":"<<systime.wMinute<<":"<<systime.wSecond<<":"<<systime.wMilliseconds<<",Thread: "<<GetCurrentThreadId()<<"] "<<s <<"\r\n"; \
  OutputDebugString(_log.str().data());\
}\

#define LOGIF(cond,s) \
{\
if(cond)\
{\
  SYSTEMTIME systime;\
  GetSystemTime(&systime);\
  std::wostringstream _log;\
  _log << "[" << systime.wHour << ":" << systime.wMinute << ":" << systime.wSecond << ":" << systime.wMilliseconds << ",Thread: " << GetCurrentThreadId() << "] " << s << "\r\n"; \
  OutputDebugString(_log.str().data());\
}\
}\

#define LOGIIF(cond,s1,s2) \
{\
if(cond)\
{\
  SYSTEMTIME systime;\
  GetSystemTime(&systime);\
  std::wostringstream _log;\
  _log << "[" << systime.wHour << ":" << systime.wMinute << ":" << systime.wSecond << ":" << systime.wMilliseconds << ",Thread: " << GetCurrentThreadId() << "] " << s1 << "\r\n"; \
  OutputDebugString(_log.str().data());\
}\
  else\
{\
  SYSTEMTIME systime;\
  GetSystemTime(&systime);\
  std::wostringstream _log;\
  _log << "[" << systime.wHour << ":" << systime.wMinute << ":" << systime.wSecond << ":" << systime.wMilliseconds << ",Thread: " << GetCurrentThreadId() << "] " << s2 << "\r\n"; \
  OutputDebugString(_log.str().data());\
}\
}\

#define LOGIFNOT(cond,s) \
{\
if(!(cond))\
{\
  SYSTEMTIME systime;\
  GetSystemTime(&systime);\
  std::wostringstream _log;\
  _log << "[" << systime.wHour << ":" << systime.wMinute << ":" << systime.wSecond << ":" << systime.wMilliseconds << ",Thread: " << GetCurrentThreadId() << "] " << s << "\r\n"; \
  OutputDebugString(_log.str().data());\
}\
}\

#define BINLOG(nm,b,s) 

#elif defined(_FILELOG) && defined(LOGGER_INCL)


#define LOG(s) \
{\
  SYSTEMTIME systime;\
  GetSystemTime(&systime);\
  std::wostringstream _log;\
  _log <<"["<<systime.wHour<<":"<<systime.wMinute<<":"<<systime.wSecond<<":"<<systime.wMilliseconds<<"] "<<s <<"\r\n"; \
  FileLogger::Current()->OutputFileLog(_log.str().data());\
}\

#define LOGIF(cond,s) \
{\
if(cond)\
{\
  SYSTEMTIME systime;\
  GetSystemTime(&systime);\
  std::wostringstream _log;\
  _log <<"["<<systime.wHour<<":"<<systime.wMinute<<":"<<systime.wSecond<<":"<<systime.wMilliseconds<<"] "<<s <<"\r\n"; \
  FileLogger::Current()->OutputFileLog(_log.str().data());\
}\
}\

#define LOGIIF(cond,s1,s2) \
{\
if(cond)\
{\
  SYSTEMTIME systime;\
  GetSystemTime(&systime);\
  std::wostringstream _log;\
  _log <<"["<<systime.wHour<<":"<<systime.wMinute<<":"<<systime.wSecond<<":"<<systime.wMilliseconds<<"] "<<s1 <<"\r\n"; \
  FileLogger::Current()->OutputFileLog(_log.str().data());\
}\
  else\
{\
  SYSTEMTIME systime;\
  GetSystemTime(&systime);\
  std::wostringstream _log;\
  _log <<"["<<systime.wHour<<":"<<systime.wMinute<<":"<<systime.wSecond<<":"<<systime.wMilliseconds<<"] "<<s2 <<"\r\n"; \
  FileLogger::Current()->OutputFileLog(_log.str().data());\
}\
}\

#define LOGIFNOT(cond,s) \
{\
if(!(cond))\
{\
  SYSTEMTIME systime;\
  GetSystemTime(&systime);\
  std::wostringstream _log;\
  _log <<"["<<systime.wHour<<":"<<systime.wMinute<<":"<<systime.wSecond<<":"<<systime.wMilliseconds<<"] "<<s <<"\r\n"; \
  FileLogger::Current()->OutputFileLog(_log.str().data());\
}\
}\

/*


#define LOG(s) \
{\
  SYSTEMTIME systime;\
  GetSystemTime(&systime);\
  std::wostringstream _log;\
  _log <<"["<<systime.wHour<<":"<<systime.wMinute<<":"<<systime.wSecond<<":"<<systime.wMilliseconds<<"] "<<s <<"\r\n"; \
  FileLogging::Current()->LogMessage(ref new Platform::String(_log.str().data()));\
}\

#define LOGIF(cond,s) \
{\
if(cond)\
{\
  SYSTEMTIME systime;\
  GetSystemTime(&systime);\
  std::wostringstream _log;\
  _log <<"["<<systime.wHour<<":"<<systime.wMinute<<":"<<systime.wSecond<<":"<<systime.wMilliseconds<<"] "<<s <<"\r\n"; \
  FileLogging::Current()->LogMessage(ref new Platform::String(_log.str().data()));\
}\
}\

#define LOGIIF(cond,s1,s2) \
{\
if(cond)\
{\
  SYSTEMTIME systime;\
  GetSystemTime(&systime);\
  std::wostringstream _log;\
  _log <<"["<<systime.wHour<<":"<<systime.wMinute<<":"<<systime.wSecond<<":"<<systime.wMilliseconds<<"] "<<s1 <<"\r\n"; \
  FileLogging::Current()->LogMessage(ref new Platform::String(_log.str().data()));\
}\
  else\
{\
  SYSTEMTIME systime;\
  GetSystemTime(&systime);\
  std::wostringstream _log;\
  _log <<"["<<systime.wHour<<":"<<systime.wMinute<<":"<<systime.wSecond<<":"<<systime.wMilliseconds<<"] "<<s2 <<"\r\n"; \
  FileLogging::Current()->LogMessage(ref new Platform::String(_log.str().data()));\
}\
}\

#define LOGIFNOT(cond,s) \
{\
if(!(cond))\
{\
  SYSTEMTIME systime;\
  GetSystemTime(&systime);\
  std::wostringstream _log;\
  _log <<"["<<systime.wHour<<":"<<systime.wMinute<<":"<<systime.wSecond<<":"<<systime.wMilliseconds<<"] "<<s <<"\r\n"; \
  FileLogging::Current()->LogMessage(ref new Platform::String(_log.str().data()));\
}\
}\


*/

#define BINLOG(nm,b,s) 

#elif defined(_BINLOG) && defined(LOGGER_INCL) && defined(_DEBUG)


#define BINLOG(nm,b,s) \
{\
  std::wostringstream _log;\
  _log << nm;\
  FileLogger::Current()->OutputFullFile(_log.str().data(),b,s);\
}\

#define LOG(s) 
#define LOGIF(cond,s)
#define LOGIIF(cond,s1,s2)
#define LOGIFNOT(cond,s) 

#else

#define LOG(s) 
#define LOGIF(cond,s)
#define LOGIIF(cond,s1,s2)
#define LOGIFNOT(cond,s)
#define BINLOG(nm,b,s) 

#endif
 
class FileLogger
{
public:

  FileLogger()
  {

    SYSTEMTIME systime;
    ::GetSystemTime(&systime);
    std::wostringstream fnm;
    fnm << systime.wYear << "_" << systime.wMonth << "_" << systime.wDay << "_" << systime.wHour << "_" << systime.wMinute << "_" << systime.wSecond << L"_rtsp" << ".log";



    pFile = Concurrency::create_task(Windows::Storage::KnownFolders::VideosLibrary->CreateFileAsync(ref new Platform::String(fnm.str().data()),
      Windows::Storage::CreationCollisionOption::GenerateUniqueName)).get();

    return;


  }


public:

  Windows::Storage::IStorageFile^ pFile;
  static shared_ptr<FileLogger> pFileLogger;
  Windows::UI::Core::ICoreDispatcher^ cordisp;
  std::recursive_mutex _lock;
  wstring linebuffer;
  unsigned int sizewritten = 0;

  static shared_ptr<FileLogger> Current()
  {
    if (pFileLogger == nullptr)
      pFileLogger = std::make_shared<FileLogger>();
    return pFileLogger;
  }

  void OutputFileLog(const WCHAR *Message)
  {
    if (pFile == nullptr || linebuffer.size() < 1024 * 5)
    {
      linebuffer += Message;
    }

    if (pFile != nullptr && linebuffer.size() >= 1024 * 5) {

      if (sizewritten < 1024 * 500)
      {
        {
         // std::lock_guard<recursive_mutex> lock(_lock);
          Concurrency::create_task(Windows::Storage::FileIO::AppendTextAsync(pFile, ref new Platform::String(linebuffer.data()))).get();
        }
        sizewritten += (unsigned int)linebuffer.size();
        linebuffer = L"";
      }
      else
      {
        {
         // std::lock_guard<recursive_mutex> lock(_lock);
          pFile = Concurrency::create_task(Windows::Storage::FileIO::AppendTextAsync(pFile, ref new Platform::String(linebuffer.data())))
            .then([this](task<void> t)
          {
            t.get();

            linebuffer = L"";
            sizewritten = 0;
            SYSTEMTIME systime;
            ::GetSystemTime(&systime);
            std::wostringstream fnm;
            fnm << systime.wYear << "_" << systime.wMonth << "_" << systime.wDay << "_" << systime.wHour << "_" << systime.wMinute << "_" << systime.wSecond << L"_rtsp" << ".log";


            return Windows::Storage::KnownFolders::VideosLibrary->CreateFileAsync(ref new Platform::String(fnm.str().data()),
              Windows::Storage::CreationCollisionOption::GenerateUniqueName);

          }).get();
        }
      }

    }
  }

  void OutputBinLog(BYTE *Message, unsigned int size)
  {
    Platform::Array<BYTE>^ arr = ref new Platform::Array<BYTE>(size);
    for (unsigned int i = 0; i < size; i++)
      arr->set(i, Message[i]);
    Windows::Storage::FileIO::WriteBytesAsync(pFile, arr);


  }

  void OutputFullFile(wstring FileName, BYTE *Message, unsigned int size)
  {
    HRESULT hr = S_OK;

    Windows::Storage::IStorageFile^ pFile;
    Concurrency::create_task(Windows::Storage::KnownFolders::VideosLibrary->CreateFileAsync(ref new Platform::String(FileName.data()),
      Windows::Storage::CreationCollisionOption::GenerateUniqueName)).then([this, Message, size](Windows::Storage::IStorageFile^ fl)
    {
      Platform::Array<BYTE>^ arr = ref new Platform::Array<BYTE>(size);
      for (unsigned int i = 0; i < size; i++)
        arr->set(i, Message[i]);
      Concurrency::create_task(Windows::Storage::FileIO::WriteBytesAsync(fl, arr));
    });

  }

};



