// unnamed_floatmath_0x542500.cpp - Sum.exe v1.40, code 0x542500-0x542cc2: float helpers (vsdk\math in
// sum.symbols): table square roots, random numbers, a quadrant-correct arctangent, a float formatter, the
// 1e-4 comparisons. The file name is a placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: after the exception handler unit (GetFilePart 0x5424e0, its strings end at 0x5ac8bd) and
// before the UI element object (0x542cd0, src/decomp/vsdk/unnamed_ui_element_0x542cd0.cpp): the two
// float_wrapper members 0x542cb0 / 0x542cc0 close this object as the vector3 scalar operators close the
// ix unit, and the UI object ends with its own compiler-generated COMDAT (0x543010). Its .data is
// at 0x5ac8c8-0x5ac8d9 (the formatter's "-", "0.0", "0", ".", "0"), .bss 0x2ee2518-0x2ee4523 (skeleton.cpp's .bss
// starts at 0x2ee4524). The .data dwords 0x5ac8c0 / 0x5ac8c4 before it are not referenced by code.
//
// The two empty-string literals of float_format_buf are .bss addresses (0x2ee451c read as one byte for
// `char digit[2] = ""`, 0x2ee4520 copied by strcpy): VC6 put the all-zero literals in .bss here (D4706).

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vsdk/math/vector.h"
#include "vsdk/math/unnamed_floatmath_0x542500.h"

// ---- .bss, in address order ------------------------------------------------------------------------------
unsigned int fast_inv_sqrt_table[1024];              // 0x2ee2518
unsigned int fast_sqrt_table[1024];                  // 0x2ee3518
int fast_sqrt_tables_ready;                          // 0x2ee4518

#define FLOAT_BITS(f) (*(unsigned int *)&(f))

// 0x542500  Confidence: likely (the expression shape).
float __stdcall fast_inv_sqrt(float x)
{
    FLOAT_BITS(x) = (((0xbe000000 - (FLOAT_BITS(x) & 0x7f800000)) >> 1) & 0x7f800000)
                    | fast_inv_sqrt_table[(FLOAT_BITS(x) >> 14) & 0x3ff];
    return x;
}

// 0x542540  Confidence: likely.
float __stdcall fast_sqrt_refined(float x)
{
    float y;

    FLOAT_BITS(y) = ((((FLOAT_BITS(x) & 0x7f800000) + 0x3f800000) >> 1) & 0x7f800000)
                    | fast_sqrt_table[(FLOAT_BITS(x) >> 14) & 0x3ff];
    return (3.0f - y * y / x) * y * 0.5f;
}

// 0x542590  Confidence: likely.
float __stdcall fast_sqrt(float x)
{
    FLOAT_BITS(x) = ((((FLOAT_BITS(x) & 0x7f800000) + 0x3f800000) >> 1) & 0x7f800000)
                    | fast_sqrt_table[(FLOAT_BITS(x) >> 14) & 0x3ff];
    return x;
}

// 0x5425d0  Confidence: exact (inline assembly: the EBP frame). The mingw branch only keeps the file
// compiling for the syntax check.
int __stdcall float_to_int_round(float x)
{
    int i;
#if defined(_MSC_VER)
    __asm {
        fld x
        fistp i
    }
#else
    i = (int)x;
#endif
    return i;
}

// 0x5425f0  Confidence: exact. 1 / sqrt and sqrt of 1 + i / 1024, keeping mantissa bits 13-22; 1.0 maps
// to the largest mantissa (1 / sqrt) or 0 (sqrt).
void fast_sqrt_tables_init()
{
    int i;
    float f, r;

    if (fast_sqrt_tables_ready)
        return;
    fast_sqrt_tables_ready = 1;
    for (i = 0; i < 1024; i++) {
        f = 1.0f;
        FLOAT_BITS(f) = (FLOAT_BITS(f) & 0xff003fff) | ((i & 0x3ff) << 14);
        r = 1.0f / (float)sqrt(f);
        if (r == 1.0f)
            fast_inv_sqrt_table[i] = 0x7fe000;
        else
            fast_inv_sqrt_table[i] = FLOAT_BITS(r) & 0x7fe000;
    }
    for (i = 0; i < 1024; i++) {
        f = 1.0f;
        FLOAT_BITS(f) = (FLOAT_BITS(f) & 0xff003fff) | ((i & 0x3ff) << 14);
        r = (float)sqrt(f);
        if (r == 1.0f)
            fast_sqrt_table[i] = 0;
        else
            fast_sqrt_table[i] = FLOAT_BITS(r) & 0x7fe000;
    }
}

// 0x5426e0  Confidence: exact.
float frand()
{
    return rand() * (1.0f / 32768.0f);
}

// 0x542700  Confidence: exact.
int frand_chance_in_n_tries(float p, float n)
{
    while ((n -= 1.0f) > 0.0f) {
        if (frand() < p)
            return 1;
    }
    return frand() < (n + 1.0f) * p;
}

// 0x542770  Confidence: exact.
float frand_range(float lo, float hi)
{
    return frand() * (hi - lo) + lo;
}

// 0x542790  Confidence: exact (atan is the fld1 / fpatan intrinsic).
float atan2_fast(float y, float x)
{
    float a;

    if (x == 0.0f) {
        if (y == 0.0f)
            return 0.0f;
        if (y > 0.0f)
            return 1.5707964f;
        return -1.5707964f;
    }
    a = (float)atan(y / x);
    if (x < 0.0f)
        a += 3.1415927f;
    return a;
}

// 0x542800  Confidence: exact.
float sqrt_abs(float x)
{
    if (x < 0.0f)
        return (float)sqrt(-x);
    return (float)sqrt(x);
}

// 0x542830  Confidence: likely (the digit conversion: floor, _ftol to int, back to float, _ftol to char;
// D14605: n declared first, so digit[1] takes the zero from n's EBX as in the binary).
// The integer digits go from the largest power of ten down; each fraction digit subtracts digit / 10^k.
int float_format_buf(char *buf, float value, int size)
{
    int n = 0;
    char digit[2] = "";
    float a, rem, p, x, d;
    int e, i;

    if (buf == NULL)
        return 0;
    strcpy(buf, "");
    a = (float)fabs(value);
    if (a != value) {
        strcpy(buf, "-");
        n = 1;
        if (size - 1 <= n)
            return n;
    }
    if (value == 0.0f) {
        if (size < 4)
            return 0;
        strcpy(buf, "0.0");
        return 3;
    }
    rem = a;
    if (rem >= 1.0f) {
        p = 0.1f;
        x = a;
        e = -1;
        while (x >= 1.0f) {
            p *= 10.0f;
            x *= 0.1f;
            e++;
        }
        for (; e >= 0; e--) {
            d = (float)(int)floor(rem / p);
            digit[0] = (char)d + '0';
            strcat(buf, digit);
            n++;
            if (n >= size - 1)
                return n;
            rem = rem - p * d;
            p *= 0.1f;
        }
    } else {
        strcat(buf, "0");
        n++;
    }
    strcat(buf, ".");
    n++;
    if (n >= size - 1)
        return n;
    if (rem <= 0.0f) {
        strcat(buf, "0");
        n++;
        return n;
    }
    p = 10.0f;
    for (i = 1; i < 7; i++) {
        d = (float)(int)floor(p * rem);
        digit[0] = (char)d + '0';
        strcat(buf, digit);
        n++;
        if (n >= size - 1)
            return n;
        rem = rem - d / p;
        p *= 10.0f;
    }
    return n;
}

// 0x542b50  Confidence: exact (matrix3::invert calls it).
float cross_2d(float ax, float ay, float bx, float by)
{
    return ax * by - ay * bx;
}

// 0x542b70  Confidence: exact.
int float_ptr_near_value(const float *a, float b)
{
    float d = *a - b;

    return d < 0.0001f && d > -0.0001f;
}

// 0x542ba0  Confidence: exact.
int float_near_ptr_value(float a, const float *b)
{
    float d = a - *b;

    return d < 0.0001f && d > -0.0001f;
}

// 0x542bd0  Confidence: exact.
int float_ptrs_near_equal(const float *a, const float *b)
{
    float d = *a - *b;

    return d < 0.0001f && d > -0.0001f;
}

// 0x542c10  Confidence: exact.
int float_ptr_not_near_equal_val(const float *a, float b)
{
    float d = *a - b;

    return d > 0.0001f || d < -0.0001f;
}

// 0x542c40  Confidence: exact.
int float_val_not_near_equal_ptr(float a, const float *b)
{
    float d = a - *b;

    return d > 0.0001f || d < -0.0001f;
}

// 0x542c70  Confidence: exact.
int float_ptrs_not_near_equal(const float *a, const float *b)
{
    float d = *a - *b;

    return d > 0.0001f || d < -0.0001f;
}

// 0x542cb0  Confidence: exact (out of line here; whether they are header inlines is not known).
float_wrapper *float_wrapper::set(float v)
{
    value = v;
    return this;
}

// 0x542cc0  Confidence: exact.
float float_wrapper::get()
{
    return value;
}
