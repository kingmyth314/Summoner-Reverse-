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

// decomp53: compiled-out debug hooks next to DebugError (unit not known). The net rate hook takes
// (0, "INC R TYPE_NM" style name, packet bytes, the type's display flag) and returns 0, so every caller's
// "Unable to add net rate info" sprintf runs.
int  debug_return_zero(int a, const char *name, int size, int display);   // 0x531c50
void debug_noop_mp_frame();                                         // 0x531c60 bare ret (mp_game_frame)
void debug_noop_disp_rate(int a, const char *name, int display);   // 0x531c90 bare ret (cmd_disp_rate)

#endif
