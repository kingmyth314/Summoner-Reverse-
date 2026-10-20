// unnamed_debug_stubs_0x531c10.cpp - Sum.exe v1.40, code 0x531c10-0x531c90. Written from the Sum.exe
// disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2. Confidence per function: exact
// (control flow, calls, constants and globals match), likely (one detail of statement order or expression
// form is a guess), unsure (the shape itself is a guess).
//
// A _stricmp wrapper and seven compiled-out debug hooks (bare RET or `xor eax, eax / ret`).
// FILE NAME NOT KNOWN (D5314): no data, no string, no reference name. They sit between the two -1 stubs of
// unnamed_os_0x531bf0.cpp and DebugError (debug.cpp, 0x531ca0); they may be the head of debug.cpp or the tail
// of the os stubs' object, so they are a placeholder unit of their own. The empty hooks are not folded into
// empty_function (0x4f13e0), so they were linked without identical-code folding for this object, or are
// distinct in a way the bytes do not show.
//
// Header: vsdk/os/debug.h, summoner/summoner.h (the hooks' callers' declarations).

#include <string.h>
#include "vsdk/os/debug.h"
#include "summoner/summoner.h"

// 0x531c10  Confidence: exact (no direct caller found in the code; the signature from its body).
int stricmp_wrapper(const char *a, const char *b)
{
    return _stricmp(a, b);
}

// 0x531c30  Confidence: exact (bare RET, no reference; the signature is unknown).
void os_stub_ret_531c30()
{
}

// 0x531c40  Confidence: exact (bare RET, no reference).
void os_stub_ret_531c40()
{
}

// 0x531c50  Confidence: exact. The net rate hook (multi_process_packet, multi_send_packet...): always 0.
int debug_return_zero(int a, const char *name, int size, unsigned char display)
{
    return 0;
}

// 0x531c60  Confidence: exact (mp_game_frame).
void debug_noop_mp_frame()
{
}

// 0x531c70  Confidence: exact (FrameEnd passes 0, 10, 10).
void debug_noop_frame_end(int a, int b, int c)
{
}

// 0x531c80  Confidence: exact (game_cleanup).
void debug_noop_cleanup()
{
}

// 0x531c90  Confidence: exact (cmd_disp_rate).
void debug_noop_disp_rate(int a, const char *name, unsigned char display)
{
}
