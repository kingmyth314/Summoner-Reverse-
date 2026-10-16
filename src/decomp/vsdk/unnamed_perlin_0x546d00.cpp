// unnamed_perlin_0x546d00.cpp - Sum.exe v1.40, code 0x546d00-0x54738e: Ken Perlin's reference coherent noise
// (noise1 / noise2 / noise3 with lazy init; module "-" in sum.symbols). The file name is a placeholder: no
// path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// The functions follow the shape of Perlin's published reference code, in float: the vector arguments
// are by value (perlin_noise3's callers store three floats on the stack), the CRT rand() replaces
// random(). Constants: 4096.0f (0x581eb4), 1.0f (0x57e990), 3.0f (0x57ef1c), 1/256 (0x57f02c, the
// division by B as a multiplication).
//
// Unit range (decomp69 NOTES.md): the sound emitter object ends with its vector3::operator!= COMDAT
// (0x546cc0); this one has its own .data word (0x5aca88, after the emitter's message 0x5aca48) and
// .bss block (0x2f0b170-0x2f0dd9b, after sound_emitters_enabled 0x2f0b16c), no initialiser; gr_glide.cpp
// starts at 0x547390 (.data 0x5aca8c).

#include <stdlib.h>
#include <math.h>
#include "vsdk/math/unnamed_perlin_0x546d00.h"

// The reference code's macros. s_curve: the compiler evaluates (3 - 2t) first, then times t twice.
#define PERLIN_S_CURVE(t) ((t) * (t) * (3.0f - 2.0f * (t)))
#define PERLIN_LERP(t, a, b) ((a) + (t) * ((b) - (a)))
#define PERLIN_SETUP(x, b0, b1, r0, r1) \
    t = (x) + PERLIN_N;                 \
    b0 = ((int)t) & PERLIN_BM;          \
    b1 = (b0 + 1) & PERLIN_BM;          \
    r0 = t - (int)t;                    \
    r1 = r0 - 1.0f;
#define PERLIN_AT2(rx, ry) ((rx) * q[0] + (ry) * q[1])
#define PERLIN_AT3(rx, ry, rz) ((rx) * q[0] + (ry) * q[1] + (rz) * q[2])

// ---- .data -----------------------------------------------------------------------------------------------
int perlin_needs_init = 1;                              // 0x5aca88

// ---- .bss ------------------------------------------------------------------------------------------------
int perlin_perm[PERLIN_B + PERLIN_B + 2];               // 0x2f0b170
float perlin_g1[PERLIN_B + PERLIN_B + 2];               // 0x2f0b978
float perlin_g2[PERLIN_B + PERLIN_B + 2][2];            // 0x2f0c180
float perlin_g3[PERLIN_B + PERLIN_B + 2][3];            // 0x2f0d190

// 0x546d00  Confidence: likely (the order of the s-curve products). No caller.
float perlin_noise1(float arg)
{
    int bx0, bx1;
    float rx0, rx1, sx, t, u, v;

    if (perlin_needs_init) {
        perlin_needs_init = 0;
        perlin_init();
    }
    PERLIN_SETUP(arg, bx0, bx1, rx0, rx1);
    sx = PERLIN_S_CURVE(rx0);
    u = rx0 * perlin_g1[perlin_perm[bx0]];
    v = rx1 * perlin_g1[perlin_perm[bx1]];
    return PERLIN_LERP(sx, u, v);
}

// 0x546d80  Confidence: exact.
void perlin_init()
{
    int i, j, k;

    for (i = 0; i < PERLIN_B; i++) {
        perlin_perm[i] = i;
        perlin_g1[i] = (float)((rand() % (PERLIN_B + PERLIN_B)) - PERLIN_B) / PERLIN_B;
        for (j = 0; j < 2; j++)
            perlin_g2[i][j] = (float)((rand() % (PERLIN_B + PERLIN_B)) - PERLIN_B) / PERLIN_B;
        perlin_normalize2(perlin_g2[i]);
        for (j = 0; j < 3; j++)
            perlin_g3[i][j] = (float)((rand() % (PERLIN_B + PERLIN_B)) - PERLIN_B) / PERLIN_B;
        perlin_normalize3(perlin_g3[i]);
    }
    while (--i) {
        k = perlin_perm[i];
        perlin_perm[i] = perlin_perm[j = rand() % PERLIN_B];
        perlin_perm[j] = k;
    }
    for (i = 0; i < PERLIN_B + 2; i++) {
        perlin_perm[PERLIN_B + i] = perlin_perm[i];
        perlin_g1[PERLIN_B + i] = perlin_g1[i];
        for (j = 0; j < 2; j++)
            perlin_g2[PERLIN_B + i][j] = perlin_g2[i][j];
        for (j = 0; j < 3; j++)
            perlin_g3[PERLIN_B + i][j] = perlin_g3[i][j];
    }
}

// 0x546f00  Confidence: exact.
void perlin_normalize2(float v[2])
{
    float s;

    s = (float)sqrt(v[0] * v[0] + v[1] * v[1]);
    v[0] = v[0] / s;
    v[1] = v[1] / s;
}

// 0x546f30  Confidence: exact.
void perlin_normalize3(float v[3])
{
    float s;

    s = (float)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    v[0] = v[0] / s;
    v[1] = v[1] / s;
    v[2] = v[2] / s;
}

// 0x546f70  Confidence: likely (ry0 is formed with fild / fsubr instead of fisub; the dot products sum the
// y term first). No caller.
float perlin_noise2(vector2 vec)
{
    int bx0, bx1, by0, by1, b00, b10, b01, b11;
    float rx0, rx1, ry0, ry1, *q, sx, sy, a, b, t, u, v;
    int i, j;

    if (perlin_needs_init) {
        perlin_needs_init = 0;
        perlin_init();
    }
    PERLIN_SETUP(vec.x, bx0, bx1, rx0, rx1);
    PERLIN_SETUP(vec.y, by0, by1, ry0, ry1);
    i = perlin_perm[bx0];
    j = perlin_perm[bx1];
    b00 = perlin_perm[i + by0];
    b10 = perlin_perm[j + by0];
    b01 = perlin_perm[i + by1];
    b11 = perlin_perm[j + by1];
    sx = PERLIN_S_CURVE(rx0);
    sy = PERLIN_S_CURVE(ry0);
    q = perlin_g2[b00];
    u = PERLIN_AT2(rx0, ry0);
    q = perlin_g2[b10];
    v = PERLIN_AT2(rx1, ry0);
    a = PERLIN_LERP(sx, u, v);
    q = perlin_g2[b01];
    u = PERLIN_AT2(rx0, ry1);
    q = perlin_g2[b11];
    v = PERLIN_AT2(rx1, ry1);
    b = PERLIN_LERP(sx, u, v);
    return PERLIN_LERP(sy, a, b);
}

// 0x5470e0  Confidence: likely (as perlin_noise2; the z dot term is summed first).
float perlin_noise3(vector3 vec)
{
    int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
    float rx0, rx1, ry0, ry1, rz0, rz1, *q, sy, sz, a, b, c, d, t, u, v;
    int i, j;

    if (perlin_needs_init) {
        perlin_needs_init = 0;
        perlin_init();
    }
    PERLIN_SETUP(vec.x, bx0, bx1, rx0, rx1);
    PERLIN_SETUP(vec.y, by0, by1, ry0, ry1);
    PERLIN_SETUP(vec.z, bz0, bz1, rz0, rz1);
    i = perlin_perm[bx0];
    j = perlin_perm[bx1];
    b00 = perlin_perm[i + by0];
    b10 = perlin_perm[j + by0];
    b01 = perlin_perm[i + by1];
    b11 = perlin_perm[j + by1];
    t = PERLIN_S_CURVE(rx0);
    sy = PERLIN_S_CURVE(ry0);
    sz = PERLIN_S_CURVE(rz0);
    q = perlin_g3[b00 + bz0];
    u = PERLIN_AT3(rx0, ry0, rz0);
    q = perlin_g3[b10 + bz0];
    v = PERLIN_AT3(rx1, ry0, rz0);
    a = PERLIN_LERP(t, u, v);
    q = perlin_g3[b01 + bz0];
    u = PERLIN_AT3(rx0, ry1, rz0);
    q = perlin_g3[b11 + bz0];
    v = PERLIN_AT3(rx1, ry1, rz0);
    b = PERLIN_LERP(t, u, v);
    c = PERLIN_LERP(sy, a, b);
    q = perlin_g3[b00 + bz1];
    u = PERLIN_AT3(rx0, ry0, rz1);
    q = perlin_g3[b10 + bz1];
    v = PERLIN_AT3(rx1, ry0, rz1);
    a = PERLIN_LERP(t, u, v);
    q = perlin_g3[b01 + bz1];
    u = PERLIN_AT3(rx0, ry1, rz1);
    q = perlin_g3[b11 + bz1];
    v = PERLIN_AT3(rx1, ry1, rz1);
    b = PERLIN_LERP(t, u, v);
    d = PERLIN_LERP(sy, a, b);
    return PERLIN_LERP(sz, c, d);
}
