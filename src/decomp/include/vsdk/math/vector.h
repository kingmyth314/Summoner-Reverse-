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

    float x, y, z;
};
LAYOUT_CHECK(vector3, sizeof(vector3) == 0xc);

int imax(int a, int b);                               // 0x51b750 (COMDAT in character_instance.cpp)
int imin(int a, int b);                               // 0x503dc0
float frand();                                        // 0x5426e0 rand() * (1/32768) (unit not known)

#endif
