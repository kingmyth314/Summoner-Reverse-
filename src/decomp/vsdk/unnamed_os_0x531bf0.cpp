// unnamed_os_0x531bf0.cpp - Sum.exe v1.40, code 0x531bf0-0x531c0f (sum.symbols module vsdk\os): two
// unreferenced functions that return -1.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit not known: they start a new object after the int3 gap that ends the os object (0x531beb-0x531bef)
// and are followed by stricmp_wrapper 0x531c10 and the bare-ret / return-0 debug stubs (module -) up to
// DebugError's helper 0x531ca0 (debug.cpp). They may share an object with those stubs; nothing (no data, no
// string) says which, so they get a placeholder file of their own.

#include "vsdk/os/os.h"

// 0x531bf0  Confidence: likely (or eax, -1 / ret; no reference anywhere, so the signature is unknown).
int os_stub_ret_m1_531bf0()
{
    return -1;
}

// 0x531c00  Confidence: likely (same body).
int os_stub_ret_m1_531c00()
{
    return -1;
}
