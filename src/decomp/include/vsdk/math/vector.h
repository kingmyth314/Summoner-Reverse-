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
    // decomp37 (living_entity.cpp): only seen expanded, in character_ai_move_frame (v = move / dt, the
    // divisor copied to a temporary) and monster_drop_loot (rvec + 0.5 per component). Names ours.
    vector3 operator/(float s) const
    { return vector3(x / s, y / s, z / s); }
    // decomp67: the out-of-line copies of the scalar operators are COMDATs at the end of the ix unit
    // (0x53de00-0x53debc; sum.symbols vector3__plus_scalar / minus_scalar / add_scalar_assign /
    // sub_scalar_assign), which was built without inline expansion.
    vector3 operator+(float s) const                  // 0x53de00 vector3__plus_scalar
    { return vector3(x + s, y + s, z + s); }
    vector3 operator-(float s) const                  // 0x53de40 vector3__minus_scalar (decomp67)
    { return vector3(x - s, y - s, z - s); }
    void operator+=(float s)                          // 0x53de80 vector3__add_scalar_assign (decomp67)
    { x += s; y += s; z += s; }
    void operator-=(float s)                          // 0x53dea0 vector3__sub_scalar_assign (decomp67)
    { x -= s; y -= s; z -= s; }
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
    // decomp69: inline; its only copy is a COMDAT at the end of the sound emitter object (0x546cc0, ret 4),
    // which calls it from sound_emitter_update_position.
    bool operator!=(const vector3 &v) const           // 0x546cc0 vector3__ne
    { return x != v.x || y != v.y || z != v.z; }
    // decomp25: cross product, a COMDAT after ca_render (0x50d7e0); matrix.cpp calls it.
    vector3 cross(const vector3 &v) const             // 0x50d7e0 vector3__cross
    { return vector3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }
    // D12900: rounded to float, then promoted and converted again for the return. The unoptimised
    // living_entity callers store both (fst cast temporary, fst result); every plain float spelling
    // gives one store. A device that reproduces the binary; the value is the plain float length.
    float len() const                                 // 0x4bc290
    { return (double)(float)sqrt(x * x + y * y + z * z); }
    // Inline in the original header (decomp20, DECISIONS.md C3): expanded in conversation_frame, called
    // as the COMDAT 0x4d9a80 vector3__len_sq from conversation_camera_compute_shot.
    float len_sq() const { return x * x + y * y + z * z; }
    // decomp60: inline; its out-of-line copy is the COMDAT 0x4b24f0 (vsdk\math), called by the light and
    // vector units (built without inline expansion). The code sums the z term first (D504).
    float dot(const vector3 &v) const                 // 0x4b24f0 vector3__dot
    { return x * v.x + y * v.y + z * v.z; }

    // decomp60: inline; its only copy is a COMDAT at the end of the vector unit (0x525670).
    void operator/=(float s)                          // 0x525670 vector3__operator_div_assign (times 1 / s)
    { float r = 1.0f / s; x *= r; y *= r; z *= r; }
    // decomp66: inline; its only copy is a COMDAT at the end of the clip unit (0x559da0, after
    // bbox_transform; src/decomp/vsdk/unnamed_clip_0x5590b0.cpp).
    float &operator[](int i)                          // 0x559da0 vector3__component_ptr
    { return (&x)[i]; }

    // Out of line, in the vector unit (0x523f30-0x525710, src/decomp/vsdk/unnamed_vector_0x523f30.cpp; decomp60).
    int  is_valid() const;                            // 0x524220
    int  is_zero() const;                             // 0x5241e0 vector3__is_zero (decomp37)
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
    float normalize_safe_length();                    // 0x524510 as normalize_safe, returns the old length (decomp54)
    void random_direction();                          // 0x5246a0 vector3__random_direction (twin of 0x524650)
    void rotate(const matrix &m);                     // 0x5245a0 vector3__rotate (in place, dot with each row; decomp28)
    float length_quick() const;                       // 0x5240e0 vector3__length_quick (sorted |components|, weighted; decomp32)
    // decomp60: the other members of the vector unit.
    // (axis, u, v): the index of the largest |component| and the two others in a fixed order, swapped
    // when that component is not positive.
    void get_major_axis(int *axis, int *u, int *v) const;   // 0x524010
    vector3 normalized_or_x() const;                  // 0x5242c0 (1, 0, 0) at length 0
    void negate();                                    // 0x524580
    // thiscall in Sum.exe (Summoner 2's free functions take the vector first): a random unit vector
    // with z in min_z..1, and the same around an axis (passed by value).
    void random_direction_in_cone(float min_z);       // 0x5246f0 random_direction_in_cone
    void random_direction_in_cone_axis(vector3 axis, float min_z);   // 0x524740 random_direction_in_cone_axis

    float x, y, z;
};
LAYOUT_CHECK(vector3, sizeof(vector3) == 0xc);

// s * v: a free function, cdecl (hidden result, s, &v) (combat.cpp range, 0x468a90 vector3_scale_to).
vector3 operator*(float s, const vector3 &v);         // 0x468a90
float distance(const vector3 *a, const vector3 *b);   // 0x524810 |a - b|
int vector3_near_equal(const vector3 *a, const vector3 *b);   // 0x524910 each |a - b| component below 0x581930 (decomp25)
// (out, p0, p1, p2): cross product of two edges, not normalised (Volition vm_vec_perp; decomp28).
void vector3_perp(vector3 *out, const vector3 *p0, const vector3 *p1, const vector3 *p2);   // 0x524990
extern vector3 vector3_zero;                          // 0x2cbf4b8 (0, 0, 0), static initialiser (decomp27)
// decomp60 (the sound unit): (out, a, b) out = a - b normalised; returns |a - b|.
float vector3_normalized_dir(vector3 *out, const vector3 *a, const vector3 *b);   // 0x5248d0
// decomp60: the free functions of the vector unit (cdecl).
float vector3_project_onto_normal(vector3 *out, const vector3 *v, const vector3 *n);   // 0x524780 out = n (v . n); returns v . n
void vector3_remove_normal_component(vector3 *out, const vector3 *v, const vector3 *n);   // 0x5247c0 out = v - n (v . n)
float distance_quick(const vector3 *a, const vector3 *b);   // 0x524870 (a - b).length_quick()
float dot_of_difference(const vector3 *a, const vector3 *b, const vector3 *c);   // 0x5248a0 b . (a - c)
void vector3_normal(vector3 *out, const vector3 *p0, const vector3 *p1, const vector3 *p2);   // 0x524960 vector3_perp, normalised
// (a, b, axis): the angle between a and b, negative when a x b points away from axis.
float vector3_angle_and_axis(const vector3 *a, const vector3 *b, const vector3 *axis);   // 0x5249e0
float direction_dot(const vector3 *dir, const vector3 *from, const vector3 *to);   // 0x524ab0 dir . normalised(to - from)
// (out, v, n, p): v moved along n onto the plane through p.
void project_onto_plane(vector3 *out, const vector3 *v, const vector3 *n, const vector3 *p);   // 0x524ae0
// (out, v, angle, origin, axis): v rotated by angle (the bank of a matrix built on axis) around origin.
void vector3_rotate_about_axis(vector3 *out, const vector3 *v, float angle, const vector3 *origin,
                               const vector3 *axis);  // 0x524b50
// (out, from, to, t): from turned towards to by t of the angle between them (normalised copies first).
void vector3_rotate_towards(vector3 *out, const vector3 *from, const vector3 *to, float t);   // 0x524c10
void vector3_rotate_towards_unit(vector3 *out, const vector3 *from, const vector3 *to, float t);   // 0x524c80
// (points, n, center, radius): Ritter's bounding sphere.
void bounding_sphere_from_points(const vector3 *pts, int n, vector3 *center, float *radius);   // 0x524d10
// (n, verts, centroid): the area-weighted centroid of a fan-triangulated polygon; returns the area.
float polygon_calc_centroid(int n, const vector3 *verts, vector3 *centroid);   // 0x525100
void vector3_world_to_local(vector3 *v, const vector3 *origin, const matrix *orient);   // 0x5252c0 (v - origin) rotated
void vector3_local_to_world(vector3 *v, const vector3 *origin, const matrix *orient);   // 0x525300 v unrotated + origin

int imax(int a, int b);                               // 0x51b750 (COMDAT in character_instance.cpp)
int imin(int a, int b);                               // 0x503dc0
float frand();                                        // 0x5426e0 rand() * (1/32768) (unit not known)
float frand_range(float lo, float hi);                // 0x542770 lo + frand() * (hi - lo) (unit not known; decomp39)
float distance_squared(const vector3 *a, const vector3 *b);   // 0x524840 |a - b|^2 (decomp41)
// (v, lo, hi): clamps *v in place (vsdk\math; the Glide tint code and the sound group volumes; decomp45).
// decomp65 (D4500): inline; its only copy is the COMDAT 0x50d980 at the end of the character mesh object
// (after float_max), which calls it from all three renderers.
inline void clamp_float(float *v, float lo, float hi) // 0x50d980
{
    if (*v < lo)
        *v = lo;
    else if (*v > hi)
        *v = hi;
}
// decomp60: inline (D4004). Out-of-line copies: float_max 0x50d960 (a COMDAT before clamp_float, in the
// gr unit), float_min 0x51faf0 (at the end of the light unit). Both cdecl, `fcomp` of a with b.
inline float float_max(float a, float b)              // 0x50d960
{ return a > b ? a : b; }
inline float float_min(float a, float b)              // 0x51faf0
{ return a < b ? a : b; }

// A 2D vector (conversation_frame passes the x / z offset; decomp20), (x, z of a ground position in spell_place_ground_effect). len as in decomp20's copy.
class vector2 {
public:
    // decomp57 (D3706): the default constructor is empty and folded with vector3s (0x505530): the
    // avoidance tables static initialisers 0x4a6720 / 0x4a6760 / 0x4a67a0 pass 0x505530 to `vector
    // constructor iterator` for their vector2 arrays (count 30 / 8, size 8).
    vector2() {}
    // decomp60: inline; the COMDATs 0x525650 / 0x5256a0 / 0x525700 at the end of the vector unit.
    vector2(float x, float y)                         // 0x525650 vector2__vector2 (ret 8)
    { this->x = x; this->y = y; }
    vector2 operator-(const vector2 &v) const         // 0x5256a0 vector2__operator_sub
    { return vector2(x - v.x, y - v.y); }
    float dot(const vector2 &v) const                 // 0x525700 vector2__dot
    { return x * v.x + y * v.y; }
    // decomp57: inline members like vector3s (D500); their only out-of-line copies are the COMDATs the
    // avoidance unit (built without inline expansion) calls: operator= returns nothing (EAX not set,
    // ret 4), operator+ returns through the hidden pointer (ret 8).
    void operator=(const vector2 &v)                  // 0x4aa3a0 vector2__assign
    { x = v.x; y = v.y; }
    // Through the float constructor (D9701): the COMDAT computes y first (arguments right to left) and
    // stores both after; avoid_area_add's inline copies match only in this form.
    vector2 operator+(const vector2 &v) const         // 0x4aa3c0 vector2__operator_add
    { return vector2(x + v.x, y + v.y); }
    float len() const;                                // 0x523f80 vector2__len
    float normalize();                                // 0x523fc0 vector2__normalize: returns the length, (1, 0) at 0
    float len_sq() const;                             // 0x523fa0 vector2__len_sq (in the vector unit, so out of line; decomp29)
    // decomp32 (pathfinding.cpp): out-of-line members, called with ECX = this.
    void negate();                                    // 0x524000 vector2__negate (x = -x, y = -y)
    // decomp57: nearly_equal and to_vector3 are COMDATs of the shadow / obstacle-debug unit
    // (game/unnamed_shadows_0x4a37d0.cpp), from_xz one of the geometry unit (game/unnamed_pfgeom_0x4a4ee0.cpp).
    vector3 to_vector3() const;                       // 0x4a4ec0 vector2__to_vector3 (x, 0, y); hidden result
    void from_xz(const vector3 *v);                   // 0x4a4ee0 vector2__from_xz (x = v.x, y = v.z); ret 4, no this
    // cdecl (two pushed pointers, add esp 8): both components within 0.001 (0x581938 / 0x58193c).
    static bool nearly_equal(const vector2 *a, const vector2 *b);   // 0x4a4e60 vector2__nearly_equal
    float x, y;
};
LAYOUT_CHECK(vector2, sizeof(vector2) == 0x8);
extern vector2 vector2_zero;                          // 0x2cbf4b0 (0, 0), static initialiser (decomp28)
// decomp60: s * v, inline (cdecl, hidden result; the COMDAT 0x5256d0 at the end of the vector unit).
inline vector2 operator*(float s, const vector2 &v)  // 0x5256d0 vector2_scaled
{ return vector2(s * v.x, s * v.y); }
float vector2_distance(const vector2 *a, const vector2 *b);           // 0x525340 |b - a|
float vector2_distance_squared(const vector2 *a, const vector2 *b);   // 0x525370
// (p, a, b): the squared distance of p from the line through a and b.
float vector2_dist_sq_to_line(const vector2 *p, const vector2 *a, const vector2 *b);   // 0x5253a0
// decomp25 (cdecl, called from matrix.cpp; vsdk\math, unit not known):
float atan2_fast(float y, float x);                   // 0x542790 fpatan with quadrant fix-up
float cross_2d(float ax, float ay, float bx, float by);   // 0x542b50 ax*by - ay*bx
// decomp32 (pathfinding.cpp; vsdk\math, unit not known). Closest point to p on segment a-b (t clamped to
// 0..1) into out; returns the squared distance (pf_sweep_test_wall compares it with 0.0009 = 0.03^2).
float vector2_closest_point_on_segment(const vector2 *p, const vector2 *a, const vector2 *b, vector2 *out);   // 0x525410
// decomp60: a segment / circle test (Summoner 2: segment_hits_circle): true with t = 0 when a is within
// margin of p, else the first t in 0..1 where a + t (b - a) is at distance margin from p; false when the
// segment misses the circle.
bool vector2_segment_circle_intersect(const vector2 *a, const vector2 *b, const vector2 *p, float margin, float *t);   // 0x5254d0

#endif
