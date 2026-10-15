// vsdk/math/vector.h - vector3 (vsdk\math; unit file name guessed, matrix.cpp is the only math path in
// the binary). Not original source. Layout: symbols/types.h vector3.
#ifndef VSDK_MATH_VECTOR_H
#define VSDK_MATH_VECTOR_H

#include "vsdk/vsdk.h"

class matrix;

// All members are out of line in Sum.exe (the default constructor 0x505530 is a bare "mov eax, ecx; ret"
// shared with matrix43() / quaternion() by identical-code folding). The operators return by value
// through a hidden pointer.
class vector3 {
public:
    vector3();                                        // 0x505530 vector3__vector3_default (empty)
    vector3(float x, float y, float z);               // 0x43b580
    vector3(const vector3 &v);                        // 0x410cd0 vector3__vector3_copy
    vector3 operator=(const vector3 &v);              // 0x413190 vector3__assign (returns a copy)
    vector3 operator+(const vector3 &v) const;        // 0x45b680
    vector3 operator-(const vector3 &v) const;        // 0x45b6b0
    vector3 operator*(float s) const;                 // 0x50f790 vector3__scaled
    vector3 operator-() const;                        // 0x51b700 vector3__negated (COMDAT in character_instance.cpp)
    void operator+=(const vector3 &v);                // 0x48e440 vector3__add_assign
    void operator*=(float s);                         // 0x50f6a0 vector3__scale
    float len() const;                                // 0x4bc290
    int  is_valid() const;                            // 0x524220
    void unrotate(const matrix &m);                   // 0x5245f0
    void normalize_safe();                            // 0x5244b0 vector3__normalize_safe ((1,0,0) at length 0)
    // Inline in the original header (decomp20, DECISIONS.md C3): expanded in conversation_frame, called
    // as the COMDAT 0x4d9a80 vector3__len_sq from conversation_camera_compute_shot.
    float len_sq() const { return x * x + y * y + z * z; }
    vector3 rotated(const matrix &m) const;           // 0x524370 vector3__rotated (dot with each row)
    vector3 unrotated(const matrix &m) const;         // 0x5243d0 vector3__unrotated (M^T v)
    void normalize();                                 // 0x524430 vector3__normalize (no zero check)
    void zero();                                      // 0x524640 vector3__zero
    void random_unit_vector();                        // 0x524650 vector3__random_unit_vector

    float x, y, z;
};
LAYOUT_CHECK(vector3, sizeof(vector3) == 0xc);

// s * v: a free function, cdecl (hidden result, s, &v) (combat.cpp range, 0x468a90 vector3_scale_to).
vector3 operator*(float s, const vector3 &v);         // 0x468a90
float distance(const vector3 *a, const vector3 *b);   // 0x524810 |a - b|

int imax(int a, int b);                               // 0x51b750 (COMDAT in character_instance.cpp)
int imin(int a, int b);                               // 0x503dc0
float frand();                                        // 0x5426e0 rand() * (1/32768) (unit not known)

// A 2D vector (conversation_frame passes the x / z offset; decomp20), (x, z of a ground position in spell_place_ground_effect). len as in decomp20's copy.
class vector2 {
public:
    float len() const;                                // 0x523f80 vector2__len
    float normalize();                                // 0x523fc0 vector2__normalize: returns the length, (1, 0) at 0
    float x, y;
};
LAYOUT_CHECK(vector2, sizeof(vector2) == 0x8);

#endif
