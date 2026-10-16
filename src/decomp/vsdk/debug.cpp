// debug.cpp - Sum.exe v1.40, code 0x531ca0-0x531d16 (sum.symbols module debug.cpp: Summoner 2's debug.cpp has
// DebugError; the folder is not known). Written from the Sum.exe disassembly (tools/asm.mjs). Target:
// VC++ 6.0 (cl 12.00.8966), /O2. Confidence per function: exact (control flow, calls, constants and globals
// match), likely (one detail of statement order or expression form is a guess), unsure (the shape itself is
// a guess).
//
// The PC error report: a message box, then exit(1). The bare-ret / return-0 debug hooks before it
// (0x531c10-0x531c90) are a placeholder unit of their own (unnamed_debug_stubs_0x531c10.cpp, D5314); the
// free-movement unit's static initialisers start at 0x531d20. No data of its own (the literals
// 0x5a984c-0x5a98d3 only).
//
// Header: vsdk/os/debug.h.

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vsdk/os/debug.h"
#include "vsdk/gr/gr.h"

void debug_error_deactivate_gr();

// 0x531ca0  Confidence: likely (a function of its own that only tail-jumps to gr_deactivate; name ours from
// sum.symbols; whether it is a debug.cpp helper or a separately compiled inline is not visible).
void debug_error_deactivate_gr()
{
    gr_deactivate();
}

// 0x531cb0  Confidence: likely (the size of the text buffer: the frame is 0x418 bytes). (file, line,
// message): "Error: <message> / File: <file> - Line: <line>" and the fixed advice, in a task-modal box.
void DebugError(const char *file, int line, const char *msg)
{
    char text[0x418];

    sprintf(text, "Error: %s\nFile: %s - Line: %d\n\n", msg, file, line);
    strcat(text, "A critical error has occurred and the program cannot continue.\nPress OK to exit the program\n");
    debug_error_deactivate_gr();
    MessageBoxA(NULL, text, "Error!", MB_TASKMODAL | MB_SETFOREGROUND);
    exit(1);
}
