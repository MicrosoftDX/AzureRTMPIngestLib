 

#include "pch.h"
#include "Logger.h"


shared_ptr<FileLogger> FileLogger::pFileLogger = nullptr;
FileLogging^ FileLogging::current = nullptr;
