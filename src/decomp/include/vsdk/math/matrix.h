// vsdk/math/matrix.h - 3x3 rotation matrix, 4x3 transform and quaternion (vsdk\math\matrix.cpp, path
// reference 0x510474). Not original source. Layouts: symbols/types.h matrix3 / matrix43 / quaternion.
#ifndef VSDK_MATH_MATRIX_H
#define VSDK_MATH_MATRIX_H

#include "vsdk/math/vector.h"

// 3x3 rotation, rows right / up / forward. sum.symbols calls the methods matrix3__*; the class name is
// Summoner 2's. matrix3 (types.h, sum.symbols) and matrix3_t (the buysell pilot's wrapper) are the same
// type (DECISIONS.md D9).
class matrix {
public:
    matrix();                                         // 0x50f660 matrix__matrix_default (three vector3 ctors)
    matrix operator*(const matrix &m) const;          // 0x50fbc0 matrix3__multiply
    void operator=(const matrix &m);                  // 0x4a4320 matrix3__copy
    void make_identity();                             // 0x511050 matrix3__make_identity
    void extract_angles(float *p, float *b, float *h);// 0x510240 matrix3__extract_angles
    // decomp20 (conversationctl.cpp): the three pass the vector by address.
    void set_from_forward(const vector3 &fwd);        // 0x511080 matrix3__set_from_forward
    void get_axis_angle(vector3 *axis, float *angle); // 0x510390 matrix3__get_axis_angle
    void set_from_axis_angle(const vector3 &axis, float angle);   // 0x510110 matrix3__set_from_axis_angle

    vector3 rvec, uvec, fvec;
};
LAYOUT_CHECK(matrix, sizeof(matrix) == 0x24);
typedef matrix matrix3;
typedef matrix matrix3_t;
// new_status.cpp calls make_identity through this shim from the C-struct days (DECISIONS.md D9).
#define MATRIX3_M(m) (m)

class quaternion {                                    // w at +0xC (quaternion__to_matrix)
public:
    quaternion();                                     // folded with 0x505530 (empty)
    quaternion(float x, float y, float z, float w);   // 0x50f990
    quaternion(const quaternion &q);                  // 0x51b730 quaternion__assign_b (COMDAT in character_instance.cpp)
    quaternion &operator=(const quaternion &q);       // 0x50f9b0 quaternion__assign
    void to_matrix(matrix *out) const;                // 0x522bf0 quaternion__to_matrix
    void to_matrix_b(matrix *out) const;              // 0x522cc0 quaternion__to_matrix_b (doors)
    float x, y, z, w;
};
LAYOUT_CHECK(quaternion, sizeof(quaternion) == 0x10);
void quaternion_slerp(quaternion *out, const quaternion *a, const quaternion *b, float t);  // 0x5235a0

class matrix43 {                                      // rotation rows m[0..2] + position m[3]
public:
    matrix43();                                       // folded with 0x505530 (empty)
    matrix43(const vector3 &r, const vector3 &u, const vector3 &f, const vector3 &p);  // 0x5113f0
    matrix43(const matrix &rot);                      // 0x5113a0 matrix43__matrix43_rot (zero position; decomp20)
    matrix43 operator*(const matrix43 &m) const;      // 0x5114b0 matrix43__multiply_4x4
    void operator=(const matrix43 &m);                // 0x51b6c0 matrix43__copy (COMDAT in character_instance.cpp, no result)
    void set(const quaternion &q, const vector3 &pos);// 0x511840 matrix43__set_quat
    matrix43 inverse() const;                         // 0x5116f0
    void get_rotation(matrix *out) const;             // 0x511b70
    vector3 transform(const vector3 &v) const;        // 0x511c70 matrix43__transform_point
    void set_position(const vector3 &v);              // 0x511cf0
    void get_position(vector3 *out) const;            // 0x51b6e0 matrix43__get_position_b (COMDAT in character_instance.cpp)
    float m[4][3];
};
LAYOUT_CHECK(matrix43, sizeof(matrix43) == 0x30);

#endif
