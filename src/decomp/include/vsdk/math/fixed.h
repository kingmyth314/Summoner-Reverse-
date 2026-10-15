// vsdk/math/fixed.h - 16-bit fixed-point helpers of the effect code (vsdk\math in sum.symbols; directory
// from the module column, file name ours). Not original source.
//
// All inline (DECISIONS.md D4004): their only out-of-line copies are COMDATs at the ends of the two
// vsdk\geom objects built without inline expansion that call them (float_to_fixed8 0x51e060 and
// fixed8::set 0x51e040 after effect_mem.cpp's record constructors; fixed4_to_float 0x520360 and
// fixed8_to_float 0x520400 among the attached-light unit's COMDATs).
#ifndef VSDK_MATH_FIXED_H
#define VSDK_MATH_FIXED_H

#include "vsdk/vsdk.h"

// cdecl, `fld; fmul 256.0; jmp _ftol`: the caller stores AX.
inline int float_to_fixed8(float f)                   // 0x51e060
{ return (int)(f * 256.0f); }
// (short): `movsx; fild; fmul 1/256`.
inline float fixed8_to_float(short v)                 // 0x520400
{ return v * (1.0f / 256.0f); }
// (short): `movsx; fild; fmul 1/16` (the key channels of the effect tracks).
inline float fixed4_to_float(short v)                 // 0x520360
{ return v * (1.0f / 16.0f); }

// An 8.8 fixed-point value (2 bytes; class name from the symbol fixed8__set).
class fixed8 {
public:
    void set(float f)                                 // 0x51e040 fixed8__set (thiscall, ret 4)
    { v = (short)float_to_fixed8(f); }
    short v;
};
LAYOUT_CHECK(fixed8, sizeof(fixed8) == 2);

#endif
