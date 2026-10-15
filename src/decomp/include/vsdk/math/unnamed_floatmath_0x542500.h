// vsdk/math/unnamed_floatmath_0x542500.h - the float helper unit 0x542500-0x542cc2 (vsdk\math in sum.symbols;
// source src/decomp/vsdk/unnamed_floatmath_0x542500.cpp, file name a placeholder: no path string, no
// strings.tbl block; decomp67). Not original source. frand, frand_range, atan2_fast and cross_2d (the
// members with callers) stay declared in vsdk/math/vector.h.
#ifndef VSDK_MATH_UNNAMED_FLOATMATH_0X542500_H
#define VSDK_MATH_UNNAMED_FLOATMATH_0X542500_H

#include "vsdk/vsdk.h"

// The table square roots (10 mantissa bits; __stdcall, ret 4). Nothing calls them or the table builder.
extern unsigned int fast_inv_sqrt_table[1024];        // 0x2ee2518
extern unsigned int fast_sqrt_table[1024];            // 0x2ee3518
extern int fast_sqrt_tables_ready;                     // 0x2ee4518 (label ours)
float __stdcall fast_inv_sqrt(float x);                // 0x542500
float __stdcall fast_sqrt_refined(float x);            // 0x542540 one Newton step on the table value
float __stdcall fast_sqrt(float x);                    // 0x542590
int   __stdcall float_to_int_round(float x);           // 0x5425d0 fistp in the current rounding mode
void  fast_sqrt_tables_init();                         // 0x5425f0 once
// (p, n): true when one of about n tries with chance p succeeds; the last fractional try counts in
// proportion (no caller).
int   frand_chance_in_n_tries(float p, float n);       // 0x542700
float sqrt_abs(float x);                               // 0x542800 sqrt(|x|) (no caller)
// (buf, value, size): "-", the integer digits (or "0"), ".", up to six fraction digits (or "0"),
// "0.0" for 0 (needs size 4); stops at size - 1 characters; returns the length (no caller).
int   float_format_buf(char *buf, float value, int size);   // 0x542830
// |a - b| < 1e-4 (the first three) and its negation (no callers).
int   float_ptr_near_value(const float *a, float b);         // 0x542b70
int   float_near_ptr_value(float a, const float *b);         // 0x542ba0
int   float_ptrs_near_equal(const float *a, const float *b); // 0x542bd0
int   float_ptr_not_near_equal_val(const float *a, float b); // 0x542c10
int   float_val_not_near_equal_ptr(float a, const float *b); // 0x542c40
int   float_ptrs_not_near_equal(const float *a, const float *b);   // 0x542c70

// A float in a class of its own (4 bytes; sum.symbols float_wrapper, conf I: no caller, type not known).
class float_wrapper {
public:
    float_wrapper *set(float v);                       // 0x542cb0 float_wrapper__set (returns this)
    float get();                                       // 0x542cc0 float_wrapper__get
    float value;                                       // +0x00
};
LAYOUT_CHECK(float_wrapper, sizeof(float_wrapper) == 4);

#endif
