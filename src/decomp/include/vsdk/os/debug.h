// vsdk/os/debug.h - the vsdk error report (DebugError 0x531cb0; unit path not in the binary, placed by
// address next to the timer code 0x530bf0-0x5329b1). Not original source.
#ifndef VSDK_OS_DEBUG_H
#define VSDK_OS_DEBUG_H

#include "vsdk/vsdk.h"

// PC version: message box, then exit(1). Not declared noreturn: every call site in Sum.exe re-pushes the
// arguments and calls it again in an endless loop (jmp back to the call), so the vsdk error macro is a
// loop around the call. The callers write the literal path and line so the strings and pushes match.
void DebugError(const char *file, int line, const char *msg);   // 0x531cb0
#define VSDK_ERROR(file, line, msg) for (;;) DebugError(file, line, msg)

#endif
