// vsdk/math/bezier.h - cubic and n-point Bezier curves and a Catmull-Rom spline (0x558a30-0x5590a2,
// vsdk\math in sum.symbols; source at src/decomp/vsdk/unnamed_bezier_0x558a30.cpp, decomp66; file name
// ours). decomp65 declared bezier with the three members the character mesh unit calls; decomp66 adds the
// rest. Class names from sum.symbols (bezier4__, bezier__, spline__). Not original source.
#ifndef VSDK_MATH_BEZIER_H
#define VSDK_MATH_BEZIER_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"

// (i, t): the cubic Bernstein weight of control point i (0 outside 0..3).
float bezier_bernstein3(int i, float t);          // 0x558a30
int factorial(int n);                             // 0x558be0 (1 for n < 1)

// Four control points and pointers to them (0x40 bytes).
class bezier4 {
public:
    bezier4();                                    // 0x558ab0 bezier4__ctor
    bezier4(const vector3 *p0, const vector3 *p1, const vector3 *p2, const vector3 *p3);   // 0x558af0
    void eval(vector3 *out, float t);             // 0x558b80 (nothing for out NULL)

    vector3 p0, p1, p2, p3;                       // +0x00 four members (four constructor calls, no array loop)
    vector3 *pts[4];                              // +0x30 &p0 .. &p3
};
LAYOUT_CHECK_32(bezier4, sizeof(bezier4) == 0x40);

// Up to 20 control points (0xf4 bytes).
#define BEZIER_MAX_POINTS 20
class bezier {
public:
    bezier();                                     // 0x558c00 bezier__ctor: all points zero
    bezier(int n, vector3 **pts);                 // 0x558c50 bezier__ctor_points
    void set_points(int n, vector3 **pts);        // 0x558c80 NULL entries are skipped; n is not checked
    void set_points_array(int n, const vector3 *pts);   // 0x558cd0
    // (i, n, t): C(n, i) t^i (1 - t)^(n - i).
    float blend(int i, int n, float t);           // 0x558d10
    void eval(vector3 *out, float t);             // 0x558d90

    vector3 points[BEZIER_MAX_POINTS];            // +0x00
    int count;                                    // +0xf0 (decomp65's name)
};
LAYOUT_CHECK(bezier, sizeof(bezier) == 0xf4);

// A Catmull-Rom spline through up to 20 points, with an extrapolated point at each end (0x10c bytes).
#define SPLINE_MAX_POINTS 20
class spline {
public:
    spline();                                     // 0x558e00 spline__ctor
    spline(int n, const vector3 *pts);            // 0x558e20 spline__ctor_points
    void set_points(int n, const vector3 *pts);   // 0x558e50 more than 20: num_points = 0
    // (out, t): t in points (0 .. num_points); past the end the last point.
    void eval(vector3 *out, float t);             // 0x558f40

    vector3 points[SPLINE_MAX_POINTS + 2];        // +0x000 [0] and [n + 1] extrapolated
    int num_points;                               // +0x108
};
LAYOUT_CHECK(spline, sizeof(spline) == 0x10c);

#endif
