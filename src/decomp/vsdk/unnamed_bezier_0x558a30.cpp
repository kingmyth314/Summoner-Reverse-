// unnamed_bezier_0x558a30.cpp - Sum.exe v1.40, code 0x558a30-0x5590a2: Bezier curves and the Catmull-Rom
// spline (vsdk\math in sum.symbols). The file name is a placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp66 NOTES.md): after LineFile's last function (0x558a10), before the clip unit's
// static initialiser (0x5590b0, initialiser table entry 648). No .data, no .bss; the constants are shared
// .rdata. Built without inline expansion: vector3's operators are calls to their COMDATs.

#include <math.h>
#include "vsdk/math/bezier.h"

// 0x558a30  Confidence: exact (a four-entry jump table at 0x558a98).
float bezier_bernstein3(int i, float t)
{
    float s;

    switch (i) {
    case 0:
        s = 1.0f - t;
        return s * s * s;
    case 1:
        s = 1.0f - t;
        return s * s * t * 3.0f;
    case 2:
        return (1.0f - t) * t * t * 3.0f;
    case 3:
        return t * t * t;
    }
    return 0.0f;
}

// 0x558ab0  Confidence: exact.
bezier4::bezier4()
{
    pts[0] = &p0;
    pts[1] = &p1;
    pts[2] = &p2;
    pts[3] = &p3;
}

// 0x558af0  Confidence: exact.
bezier4::bezier4(const vector3 *a, const vector3 *b, const vector3 *c, const vector3 *d)
{
    p0 = *a;
    p1 = *b;
    p2 = *c;
    p3 = *d;
    pts[0] = &p0;
    pts[1] = &p1;
    pts[2] = &p2;
    pts[3] = &p3;
}

// 0x558b80  Confidence: exact.
void bezier4::eval(vector3 *out, float t)
{
    int i;
    float b;

    if (out == NULL)
        return;
    out->zero();
    for (i = 0; i < 4; i++) {
        b = bezier_bernstein3(i, t);
        out->x += b * pts[i]->x;
        out->y += b * pts[i]->y;
        out->z += b * pts[i]->z;
    }
}

// 0x558be0  Confidence: exact.
int factorial(int n)
{
    int r = 1;
    int i;

    for (i = 1; i <= n; i++)
        r *= i;
    return r;
}

// 0x558c00  Confidence: exact.
bezier::bezier()
{
    int i;

    for (i = 0; i < BEZIER_MAX_POINTS; i++)
        points[i] = vector3_zero;
    count = 0;
}

// 0x558c50  Confidence: exact.
bezier::bezier(int n, vector3 **pts)
{
    set_points(n, pts);
}

// 0x558c80  Confidence: exact.
void bezier::set_points(int n, vector3 **pts)
{
    int i;

    count = n;
    for (i = 0; i < n; i++) {
        if (pts[i] != NULL)
            points[i] = *pts[i];
    }
}

// 0x558cd0  Confidence: exact.
void bezier::set_points_array(int n, const vector3 *pts)
{
    int i;

    count = n;
    for (i = 0; i < n; i++)
        points[i] = pts[i];
}

// 0x558d10  Confidence: likely (the two pow results go through float locals).
float bezier::blend(int i, int n, float t)
{
    float coef;
    float a;
    float b;

    coef = (float)factorial(n) / (float)(factorial(n - i) * factorial(i));
    a = (float)pow((double)(1.0f - t), (double)(n - i));   // _CIpow: a double exponent (VC6's int overload would multiply)
    b = (float)pow((double)t, (double)i);
    return coef * a * b;
}

// 0x558d90  Confidence: exact.
void bezier::eval(vector3 *out, float t)
{
    int i;
    float b;

    if (out == NULL)
        return;
    out->x = 0.0f;
    out->y = 0.0f;
    out->z = 0.0f;
    for (i = 0; i < count; i++) {
        b = blend(i, count - 1, t);
        out->x += b * points[i].x;
        out->y += b * points[i].y;
        out->z += b * points[i].z;
    }
}

// 0x558e00  Confidence: exact.
spline::spline()
{
}

// 0x558e20  Confidence: exact.
spline::spline(int n, const vector3 *pts)
{
    set_points(n, pts);
}

// 0x558e50  Confidence: exact. The end points are pushed out by a tenth of the end segments.
void spline::set_points(int n, const vector3 *pts)
{
    int i;

    if (n > SPLINE_MAX_POINTS) {
        num_points = 0;
        return;
    }
    num_points = n;
    for (i = 0; i < n; i++)
        points[i + 1] = pts[i];
    points[0] = points[1] + (points[1] - points[2]) * 0.1f;
    points[num_points + 1] = points[num_points] + (points[num_points] - points[num_points - 1]) * 0.1f;
}

// 0x558f40  Confidence: likely (VC6 commutes the first sum; the weights stay in the FPU / frame).
void spline::eval(vector3 *out, float t)
{
    int i;
    float u, u2, u3;
    float b0, b1, b2, b3;
    vector3 *p;

    if (num_points <= 0) {
        *out = vector3_zero;
        return;
    }
    if ((int)t >= num_points) {
        *out = points[num_points];
        return;
    }
    i = (int)floor(t);
    u = t - i;
    u2 = u * u;
    u3 = u2 * u;
    b0 = -0.5f * u3 + u2 - 0.5f * u;
    b1 = 1.5f * u3 + -2.5f * u2 + 1.0f;
    b2 = -1.5f * u3 + (u2 + u2) + 0.5f * u;
    b3 = 0.5f * u3 - 0.5f * u2;
    p = &points[i + 1];
    out->x = b3 * p[2].x + b2 * p[1].x + b0 * p[-1].x + b1 * p[0].x;
    out->y = b3 * p[2].y + b2 * p[1].y + b1 * p[0].y + b0 * p[-1].y;
    out->z = b3 * p[2].z + b2 * p[1].z + b1 * p[0].z + b0 * p[-1].z;
}
