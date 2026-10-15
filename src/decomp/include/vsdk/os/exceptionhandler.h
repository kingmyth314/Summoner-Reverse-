// vsdk/os/exceptionhandler.h - the crash report writer (sum.symbols module "exceptionhandler", 0x5419a0:
// Bruce Dawson's ExceptionHandler.cpp from Game Developer, 1999). Directory and file name guessed.
// Not original source.
#ifndef VSDK_OS_EXCEPTIONHANDLER_H
#define VSDK_OS_EXCEPTIONHANDLER_H

#include "vsdk/vsdk.h"
#include <windows.h>

// Appends a report to errorlog.txt beside Sum.exe; the result is the __except filter value.
int RecordExceptionInfo(PEXCEPTION_POINTERS data, const char *message);   // 0x5419a0

#endif
