// vsdk/math/unnamed_perlin_0x546d00.h - Ken Perlin's reference coherent noise (0x546d00-0x54738e; source at
// src/decomp/vsdk/unnamed_perlin_0x546d00.cpp, decomp69; module "-" in sum.symbols, file name a placeholder).
// The float version of the reference code: noise1 takes and returns a float, noise2 / noise3 take their
// vector by value. Not original source.
#ifndef VSDK_MATH_UNNAMED_PERLIN_0X546D00_H
#define VSDK_MATH_UNNAMED_PERLIN_0X546D00_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"

#define PERLIN_B  0x100               // gradient table size
#define PERLIN_BM 0xff
#define PERLIN_N  0x1000              // 4096.0f is added before the integer part is taken (0x581eb4)

// ---- data (.data 0x5aca88, .bss 0x2f0b170-0x2f0dd9b) ---------------------------------------------------
extern int perlin_needs_init;                         // 0x5aca88 (1) the reference code's `start`
extern int perlin_perm[PERLIN_B + PERLIN_B + 2];      // 0x2f0b170 p
extern float perlin_g1[PERLIN_B + PERLIN_B + 2];      // 0x2f0b978
extern float perlin_g2[PERLIN_B + PERLIN_B + 2][2];   // 0x2f0c180
extern float perlin_g3[PERLIN_B + PERLIN_B + 2][3];   // 0x2f0d190

float perlin_noise1(float arg);                       // 0x546d00 (no caller)
void perlin_init();                                   // 0x546d80
void perlin_normalize2(float v[2]);                   // 0x546f00
void perlin_normalize3(float v[3]);                   // 0x546f30
float perlin_noise2(vector2 v);                       // 0x546f70 (no caller)
// Moved here from engine/rendering/levelrender.h (decomp69), which includes this header.
float perlin_noise3(vector3 v);                       // 0x5470e0

#endif
