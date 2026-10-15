// vsdk/math/vector.h - vector3 (vsdk\math; unit file name guessed, matrix.cpp is the only math path in
// the binary). Not original source. Layout: symbols/types.h vector3.
#ifndef VSDK_MATH_VECTOR_H
#define VSDK_MATH_VECTOR_H

#include <math.h>
#include "vsdk/vsdk.h"

class matrix;

// Inline members (DECISIONS.md D500, which settles D42): every member defined in the class body below
// has its only out-of-line copy outside the vector unit (0x523f80-0x524930), as a COMDAT among the code
// of an object that called it (0x413190 operator= in the interface code, 0x43b580 in gametarget,
// 0x505530 in localize, 0x50f6a0 / 0x50f790 / 0x50f840 in the gr unit, 0x511c10 / 0x511c30 at the end
// of matrix.cpp, ...). The vsdk objects (matrix.cpp, character_instance.cpp, effect_mem.cpp) were built
// without inline expansion and call them; the optimised game units expand them (D42's 3-dword copies).
// The default constructor 0x505530 (`mov eax, ecx; ret`) is shared with matrix43() / quaternion() by
// identical-code folding. The operators return by value through a hidden pointer.
class vector3 {
public:
    vector3() {}                                      // 0x505530 vector3__vector3_default (empty)
    vector3(float x, float y, float z)                // 0x43b580
    { this->x = x; this->y = y; this->z = z; }
    vector3(const vector3 &v)                         // 0x410cd0 vector3__vector3_copy
    { x = v.x; y = v.y; z = v.z; }
    vector3 operator=(const vector3 &v)               // 0x413190 vector3__assign (returns a copy)
    { x = v.x; y = v.y; z = v.z; return *this; }
    vector3 operator+(const vector3 &v) const         // 0x45b680
    { return vector3(x + v.x, y + v.y, z + v.z); }
    vector3 operator-(const vector3 &v) const         // 0x45b6b0
    { return vector3(x - v.x, y - v.y, z - v.z); }
    vector3 operator*(float s) const                  // 0x50f790 vector3__scaled
    { return vector3(x * s, y * s, z * s); }
    vector3 operator-() const                         // 0x51b700 vector3__negated (COMDAT in character_instance.cpp)
    { return vector3(-x, -y, -z); }
    void operator+=(const vector3 &v)                 // 0x48e440 vector3__add_assign
    { x += v.x; y += v.y; z += v.z; }
    void operator-=(const vector3 &v)                 // 0x50f840 vector3__sub_assign (decomp25)
    { x -= v.x; y -= v.y; z -= v.z; }
    void operator*=(float s)                          // 0x50f6a0 vector3__scale
    { x *= s; y *= s; z *= s; }
    // decomp25: set and == are COMDATs after matrix.cpp's last function (0x511c10 / 0x511c30, D500).
    void set(float x, float y, float z)               // 0x511c10 vector3__set
    { this->x = x; this->y = y; this->z = z; }
    bool operator==(const vector3 &v) const           // 0x511c30 vector3__operator_eq (exact compare)
    { return x == v.x && y == v.y && z == v.z; }
    // decomp25: cross product, a COMDAT after ca_render (0x50d7e0); matrix.cpp calls it.
    vector3 cross(const vector3 &v) const             // 0x50d7e0 vector3__cross
    { return vector3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }
    float len() const                                 // 0x4bc290
    { return (float)sqrt(x * x + y * y + z * z); }
    // Inline in the original header (decomp20, DECISIONS.md C3): expanded in conversation_frame, called
    // as the COMDAT 0x4d9a80 vector3__len_sq from conversation_camera_compute_shot.
    float len_sq() const { return x * x + y * y + z * z; }

    // Out of line, in the vector unit (0x523f80-0x524930; not written yet).
    int  is_valid() const;                            // 0x524220
    void unrotate(const matrix &m);                   // 0x5245f0
    void normalize_safe();                            // 0x5244b0 vector3__normalize_safe ((1,0,0) at length 0)
    vector3 rotated(const matrix &m) const;           // 0x524370 vector3__rotated (dot with each row)
    vector3 unrotated(const matrix &m) const;         // 0x5243d0 vector3__unrotated (M^T v)
    void normalize();                                 // 0x524430 vector3__normalize (no zero check)
    void zero();                                      // 0x524640 vector3__zero
    void random_unit_vector();                        // 0x524650 vector3__random_unit_vector
    // decomp25 (called from matrix.cpp):
    float dot_xyz(float x, float y, float z) const;   // 0x5241c0 vector3__dot_xyz
    vector3 normalized() const;                       // 0x524270 1/sqrt, no zero check
    vector3 negated_b() const;                        // 0x524340 vector3__negated_b (operator-'s body; name ours)
    float normalize_length();                         // 0x524470 normalize, returns the length
    void random_direction();                          // 0x5246a0 vector3__random_direction (twin of 0x524650)
    void rotate(const matrix &m);                     // 0x5245a0 vector3__rotate (in place, dot with each row; decomp28)

    float x, y, z;
};
LAYOUT_CHECK(vector3, sizeof(vector3) == 0xc);

// s * v: a free function, cdecl (hidden result, s, &v) (combat.cpp range, 0x468a90 vector3_scale_to).
vector3 operator*(float s, const vector3 &v);         // 0x468a90
float distance(const vector3 *a, const vector3 *b);   // 0x524810 |a - b|
int vector3_near_equal(const vector3 *a, const vector3 *b);   // 0x524910 each |a - b| component below 0x581930 (decomp25)
// (out, p0, p1, p2): cross product of two edges, not normalised (Volition vm_vec_perp; decomp28).
void vector3_perp(vector3 *out, const vector3 *p0, const vector3 *p1, const vector3 *p2);   // 0x524990

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
extern vector2 vector2_zero;                          // 0x2cbf4b0 (0, 0), static initialiser (decomp28)
// decomp25 (cdecl, called from matrix.cpp; vsdk\math, unit not known):
float atan2_fast(float y, float x);                   // 0x542790 fpatan with quadrant fix-up
float cross_2d(float ax, float ay, float bx, float by);   // 0x542b50 ax*by - ay*bx

#endif
