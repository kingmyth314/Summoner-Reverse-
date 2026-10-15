// vsdk/math/quaternion.h - quaternion and the 16-bit cquat (vsdk\math in sum.symbols; one object
// 0x5225b0-0x523f29, source src/decomp/vsdk/unnamed_quaternion_0x5225b0.cpp; file name a placeholder,
// decomp60). Not original source. quaternion moved here from matrix.h and cquat from vsdk/ca/skeleton.h
// (decomp60); matrix.h includes this header, so every user still sees both.
#ifndef VSDK_MATH_QUATERNION_H
#define VSDK_MATH_QUATERNION_H

#include "vsdk/math/vector.h"

class matrix;
class matrix43;
class cquat;

#define CQUAT_ONE      16383          // a cquat component of 1.0
#define CQUAT_TO_FLOAT (1.0f / 16383.0f)

class quaternion {                                    // w at +0xC (quaternion__to_matrix)
public:
    quaternion() {}                                   // folded with 0x505530 (empty)
    quaternion(float x, float y, float z, float w)    // 0x50f990 (COMDAT in the gr unit)
    { this->x = x; this->y = y; this->z = z; this->w = w; }
    quaternion(const quaternion &q)                   // 0x51b730 quaternion__assign_b (COMDAT in character_instance.cpp)
    { x = q.x; y = q.y; z = q.z; w = q.w; }
    // 0x50f9b0 quaternion__assign: EAX is left holding q.w, so the operator returns nothing (decomp25,
    // D501; the old declaration returned quaternion &).
    void operator=(const quaternion &q)
    { x = q.x; y = q.y; z = q.z; w = q.w; }
    quaternion(const matrix &m);                      // 0x522610 quaternion__set_from_matrix (returns this: a constructor)
    // 0x522790 quaternion__set_from_matrix_b also returns this (thiscall, ret 4) and s3d_read_navpoint
    // builds a temporary with it before an inline copy: a second constructor, by pointer (decomp28, D806).
    quaternion(const matrix *m);
    // decomp60 (D4011): fills a matrix43 (rotation rows, then the position zeroed: 0x30 bytes); the
    // parameter stays matrix * because matrix.cpp's callers cast their matrix43 to it.
    void to_matrix(matrix *out) const;                // 0x522bf0 quaternion__to_matrix
    void to_matrix_b(matrix *out) const;              // 0x522cc0 quaternion__to_matrix_b (doors; decomp24) 0x24 bytes
    void normalize();                                 // 0x522f20 quaternion__normalize (decomp25)
    // decomp60: the rest of the unit (thiscall; the by-value results through the hidden pointer).
    quaternion multiply(const quaternion &q) const;   // 0x522930 quaternion__multiply (this * q)
    quaternion normalized() const;                    // 0x522d90
    quaternion conjugate() const;                     // 0x522e00
    quaternion log() const;                           // 0x522e30 (w 0)
    quaternion exp() const;                           // 0x522ea0
    void negate_xyz();                                // 0x522f70
    void log_in_place();                              // 0x522f90
    void exp_in_place();                              // 0x522ff0
    void make_identity();                             // 0x523500
    // decomp66 (the vfx key readers): x, y, z, w = the cquat's components * CQUAT_TO_FLOAT (0x581d78).
    // Out of line in another vsdk\math unit (near skeleton.cpp; not written).
    void unpack_short4(const cquat &q);               // 0x551960 quaternion__unpack_short4 (ret 4)
    // Inline: their only copies are COMDATs at the end of the quaternion object.
    quaternion operator-() const                      // 0x523e70 quaternion__negated
    { return quaternion(-x, -y, -z, -w); }
    quaternion operator+(const quaternion &q) const   // 0x523eb0 quaternion__operator_add
    { return quaternion(q.x + x, q.y + y, q.z + z, q.w + w); }
    quaternion operator-(const quaternion &q) const   // 0x523ef0 quaternion__operator_sub
    { return quaternion(x - q.x, y - q.y, z - q.z, w - q.w); }
    float x, y, z, w;
};
LAYOUT_CHECK(quaternion, sizeof(quaternion) == 0x10);

// The rotation of a key: four int16 components scaled by 16383 (cquat_dot, cquat_slerp work on it).
// Empty constructor (folded with vector3() at 0x505530). decomp60: moved from vsdk/ca/skeleton.h; the
// other inline members are COMDATs at the end of the quaternion object.
class cquat {
public:
    cquat() {}
    cquat(const cquat &q)                             // 0x523d20 cquat__cquat_copy
    { x = q.x; y = q.y; z = q.z; w = q.w; }
    cquat(short x, short y, short z, short w)         // 0x523d50 cquat__cquat (ret 0x10)
    { this->x = x; this->y = y; this->z = z; this->w = w; }
    void operator=(const cquat &q)                    // 0x523d80 cquat__assign (no result)
    { x = q.x; y = q.y; z = q.z; w = q.w; }
    cquat operator-() const                           // 0x523db0 cquat__negated
    { return cquat(-x, -y, -z, -w); }
    cquat operator-(const cquat &q) const             // 0x523df0 cquat__operator_sub
    { return cquat(x - q.x, y - q.y, z - q.z, w - q.w); }
    cquat operator+(const cquat &q) const             // 0x523e30 cquat__operator_add
    { return cquat(q.x + x, q.y + y, q.z + z, q.w + w); }
    cquat multiply(const cquat &q) const;             // 0x522a40 cquat__multiply (through floats)
    cquat normalized() const;                         // 0x523050 (integer 16383^2 / |q|^2, D4012)
    cquat conjugate() const;                          // 0x5230f0
    cquat log() const;                                // 0x523120
    cquat exp() const;                                // 0x5231e0
    void normalize();                                 // 0x5232b0 (as normalized)
    void negate_xyz();                                // 0x523350
    void log_in_place();                              // 0x523370
    void exp_in_place();                              // 0x523430
    void set(const quaternion &q);                    // 0x523cc0 cquat__set_from_quaternion (components * 16383)
    short x, y, z, w;
};
LAYOUT_CHECK(cquat, sizeof(cquat) == 8);

extern cquat cquat_identity;                          // 0x2cbf498 (0, 0, 0, 16383), static initialiser
extern quaternion quaternion_identity;                // 0x2cbf4a0 (0, 0, 0, 1), static initialiser

float quaternion_dot(const quaternion *a, const quaternion *b);   // 0x523520
float cquat_dot(const cquat *a, const cquat *b);      // 0x523550 in units of 1 (sum / 16383^2)
// (out, a, b, t): t wrapped into 0..1, the shorter way round (b or -b).
void quaternion_slerp(quaternion *out, const quaternion *a, const quaternion *b, float t);  // 0x5235a0
void cquat_slerp(cquat *out, const cquat *a, const cquat *b, float t);   // 0x5237c0 (w 0 becomes 1)
// (out, p, a, b, q, t) = slerp(slerp(p, q, t), slerp(a, b, t), 2t(1 - t)).
void quaternion_squad(quaternion *out, const quaternion *p, const quaternion *a, const quaternion *b,
                      const quaternion *q, float t);  // 0x523a90
void cquat_squad(quaternion *out, const cquat *p, const cquat *a, const cquat *b, const cquat *q,
                 float t);                            // 0x523b10 (the result as floats)
// (a, b): log(normalized(conjugate(a)) * b), the tangent of the step a -> b.
quaternion quaternion_log_of_delta(const quaternion *a, const quaternion *b);   // 0x523c00
cquat cquat_log_of_delta(const cquat *a, const cquat *b);   // 0x523c60

#endif
