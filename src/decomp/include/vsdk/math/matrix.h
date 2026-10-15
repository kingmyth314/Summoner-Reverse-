// vsdk/math/matrix.h - 3x3 rotation matrix, 4x3 transform and quaternion (vsdk\math\matrix.cpp, path
// reference 0x510474). Not original source. Layouts: symbols/types.h matrix3 / matrix43 / quaternion.
// decomp25: matrix.cpp written (0x50f9d0-0x511c06); the inline members below are the COMDATs of
// DECISIONS.md D500 (their bodies are written here, not in matrix.cpp).
#ifndef VSDK_MATH_MATRIX_H
#define VSDK_MATH_MATRIX_H

#include "vsdk/math/vector.h"

class matrix43;

// 3x3 rotation, rows right / up / forward. sum.symbols calls the methods matrix3__*; the class name is
// Summoner 2's. matrix3 (types.h, sum.symbols) and matrix3_t (the buysell pilot's wrapper) are the same
// type (DECISIONS.md D9).
class matrix {
public:
    matrix() {}                                       // 0x50f660 matrix__matrix_default (COMDAT in the gr unit)
    matrix(const matrix &m);                          // 0x50faa0 matrix3__matrix3_copy
    matrix(const vector3 &r, const vector3 &u, const vector3 &f);    // 0x50fad0 matrix3__matrix3_ruf
    matrix(float sp, float cp, float sb, float cb, float sh, float ch); // 0x50fb30 matrix3__matrix3_sincos
    matrix(float p, float b, float h);                // 0x50fb80 matrix3__matrix3_angles
    matrix(const vector3 &axis, float angle);         // 0x50fff0 matrix3__matrix3_axis_angle
    // 0x4a4320 matrix3__copy, a COMDAT of an optimised unit (the shadow code) with the three row copies
    // expanded; objects.cpp expands this operator and calls 0x413190 for the rows (DECISIONS.md D500).
    void operator=(const matrix &m)
    { rvec = m.rvec; uvec = m.uvec; fvec = m.fvec; }
    matrix operator*(const matrix &m) const;          // 0x50fbc0 matrix3__multiply
    matrix multiply_by_transpose(const matrix &m) const;   // 0x50fcf0 (m^T * this)
    matrix multiply_transposed(const matrix &m) const;     // 0x50fe20 (m * this^T)
    bool rows_match(const matrix &m) const;           // 0x50ff50
    void set_from_sincos(float sp, float cp, float sb, float cb, float sh, float ch);   // 0x510020
    void set_from_angles(float p, float b, float h);  // 0x5100c0
    void set_from_axis_angle(const vector3 &axis, float angle);   // 0x510110 matrix3__set_from_axis_angle
    void extract_angles(float *p, float *b, float *h) const;      // 0x510240 matrix3__extract_angles
    // decomp20 (conversationctl.cpp): the three pass the vector by address.
    void get_axis_angle(vector3 *axis, float *angle) const;       // 0x510390 matrix3__get_axis_angle
    int has_zero_axis() const;                        // 0x510510
    int is_valid() const;                             // 0x510660
    float determinant() const;                        // 0x5106a0
    matrix orthogonalized_from_fvec() const;          // 0x5106e0 (orthogonalize on a copy)
    matrix orthogonalized() const;                    // 0x510830 (orthogonalize_b on a copy)
    matrix transpose() const;                         // 0x510a80
    matrix inverse() const;                           // 0x510b10
    void orthogonalize();                             // 0x510b40 (forward kept)
    void orthogonalize_b();                           // 0x510c90 (forward, then up, then right)
    void transpose_in_place();                        // 0x510e70
    void set_random();                                // 0x510ea0
    void invert();                                    // 0x510ed0
    void make_identity();                             // 0x511050 matrix3__make_identity
    void set_from_forward(const vector3 &fwd);        // 0x511080 matrix3__set_from_forward
    void set_from_unit_forward(const vector3 &fwd);   // 0x511140
    void set_from_forward_up(const vector3 &fwd, const vector3 &up);         // 0x511200
    void set_from_forward_right(const vector3 &fwd, const vector3 &right);   // 0x5112d0
    void rotate_between(const matrix &a, const matrix &b);                   // 0x511a40

    vector3 rvec, uvec, fvec;
};
LAYOUT_CHECK(matrix, sizeof(matrix) == 0x24);
typedef matrix matrix3;
typedef matrix matrix3_t;
// new_status.cpp calls make_identity through this shim from the C-struct days (DECISIONS.md D9).
#define MATRIX3_M(m) (m)

extern matrix identity_matrix;                        // 0x2692ea0 (matrix.cpp)

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
    void to_matrix(matrix *out) const;                // 0x522bf0 quaternion__to_matrix
    void to_matrix_b(matrix *out) const;              // 0x522cc0 quaternion__to_matrix_b (doors; decomp24)
    void normalize();                                 // 0x522f20 quaternion__normalize (decomp25)
    void make_identity();                             // 0x523500 quaternion__make_identity (0, 0, 0, 1; decomp61)
    float x, y, z, w;
};
LAYOUT_CHECK(quaternion, sizeof(quaternion) == 0x10);
void quaternion_slerp(quaternion *out, const quaternion *a, const quaternion *b, float t);  // 0x5235a0

class matrix43 {                                      // rotation rows m[0..2] + position m[3]
public:
    matrix43() {}                                     // folded with 0x505530 (empty)
    matrix43(const vector3 &r, const vector3 &u, const vector3 &f, const vector3 &p);  // 0x5113f0
    matrix43(const matrix &rot);                      // 0x5113a0 matrix43__matrix43_rot (zero position; decomp20)
    matrix43(const vector3 &pos, const matrix &rot);  // 0x511450 matrix43__matrix43_pos_rot
    matrix43 operator*(const matrix43 &m) const;      // 0x5114b0 matrix43__multiply_4x4
    bool equals(const matrix43 &m) const;             // 0x511570
    matrix43 inverse() const;                         // 0x5116f0
    void make_identity();                             // 0x5117e0
    void set(const quaternion &q, const vector3 &pos);// 0x511840 matrix43__set_quat
    void set_product_4x4(const matrix43 &a, const matrix43 &b);   // 0x511ab0 (this = a * b)
    void get_rotation(matrix *out) const;             // 0x511b70
    void transpose_in_place();                        // 0x511bb0 (not a transpose; see matrix.cpp)

    // Inline (DECISIONS.md D500). 0x51b6c0 / 0x51b6e0 are COMDATs in character_instance.cpp; the next
    // four follow vector3::set / operator== after matrix.cpp's last function. transform is expanded by
    // the optimised conversationctl.cpp (conversation_camera_capture, same expression tree).
    void operator=(const matrix43 &m)                 // 0x51b6c0 matrix43__copy (rep movsd, no result)
    { memcpy((void *)this, &m, sizeof(matrix43)); }   // (void *): only quiets mingw's -Wnontrivial-memcall
    void get_position(vector3 *out) const             // 0x51b6e0 matrix43__get_position_b
    { out->x = m[3][0]; out->y = m[3][1]; out->z = m[3][2]; }
    vector3 transform(const vector3 &v) const         // 0x511c70 matrix43__transform_point
    {
        vector3 r;
        r.x = v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0] + m[3][0];
        r.y = v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1] + m[3][1];
        r.z = v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2] + m[3][2];
        return r;
    }
    void set_position(const vector3 &v)               // 0x511cf0
    { m[3][0] = v.x; m[3][1] = v.y; m[3][2] = v.z; }
    vector3 get_position() const                      // 0x511d10 matrix43__get_position (by value)
    { return vector3(m[3][0], m[3][1], m[3][2]); }
    void set_rotation(const matrix &rot)              // 0x511d30
    {
        m[0][0] = rot.rvec.x; m[0][1] = rot.rvec.y; m[0][2] = rot.rvec.z;
        m[1][0] = rot.uvec.x; m[1][1] = rot.uvec.y; m[1][2] = rot.uvec.z;
        m[2][0] = rot.fvec.x; m[2][1] = rot.fvec.y; m[2][2] = rot.fvec.z;
        m[3][0] = 0.0f;
        m[3][1] = 0.0f;
        m[3][2] = 0.0f;
    }
    float m[4][3];
};
LAYOUT_CHECK(matrix43, sizeof(matrix43) == 0x30);

extern matrix43 identity_matrix43;                    // 0x2692ec8 (matrix.cpp)
extern float Homogeneous_column[4];                   // 0x5a7480 {0, 0, 0, 1}

// (orient_a, pos_a, orient_b, pos_b): orient_b = orient_a * orient_b, pos_b = pos_b unrotated by
// orient_a + pos_a (spell_launch_missile; decomp23). In the gr unit before matrix.cpp, not written.
void matrix43_compose(const matrix *orient_a, const vector3 *pos_a, matrix *orient_b, vector3 *pos_b);  // 0x50f3a0

// cdecl, matrix.cpp (decomp25).
matrix43 matrix43_interpolate(const matrix43 &from, const matrix43 &to, float t);           // 0x511860
matrix43 matrix43_interpolate_quat(const quaternion &qa, const quaternion &qb,
                                   const vector3 &pa, const vector3 &pb, float t);          // 0x511920
void matrix43_set_interpolated(matrix43 *out, const quaternion &qa, const quaternion &qb,
                               const vector3 &pa, const vector3 &pb, float t);              // 0x5119c0

#endif
