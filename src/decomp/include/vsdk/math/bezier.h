// vsdk/math/bezier.h - the n-point Bezier curve (vsdk\math in sum.symbols, 0x558a30-0x558df3; unit not
// written, file name ours, decomp65). Only what the character mesh unit calls (ca_ramp_curve_eval). Not
// original source.
#ifndef VSDK_MATH_BEZIER_H
#define VSDK_MATH_BEZIER_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"

#define BEZIER_MAX_POINTS 20

// A Bezier curve through up to 20 control points (0xf4 bytes; thiscall members).
class bezier {
public:
    bezier();                                         // 0x558c00 bezier__ctor (points zeroed, count 0)
    // (n, points): copies the n points (NULL entries skipped), count = n.
    void set_points(int n, vector3 **points);         // 0x558c80 bezier__set_points (ret 8)
    // (out, t): the point at t (Bernstein weights, bezier__blend); out untouched when NULL.
    void eval(vector3 *out, float t);                 // 0x558d90 bezier__eval
    vector3 points[BEZIER_MAX_POINTS];                // +0x00
    int count;                                        // +0xf0
};
LAYOUT_CHECK(bezier, sizeof(bezier) == 0xf4);

#endif
