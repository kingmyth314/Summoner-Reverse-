// vsdk/math/ix.h - intersection tests and the plane (vsdk\math module of sum.symbols: ix_ray_plane
// 0x53ce30, ix_point_linesegment 0x53da80, Plane__distance_to_point 0x50f860; file name ours, decomp35).
// Not original source.
#ifndef VSDK_MATH_IX_H
#define VSDK_MATH_IX_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"

// A plane {normal, d} with dot(normal, p) + d = 0 on it (0x10 bytes; type name ours). set_normal_point
// is inline: its COMDATs are 0x43b5a0 (gametarget.cpp, whose .rdata holds the -1.0f at 0x580e1c) and
// 0x4a42e0 (the shadow code). The copy assignment is the implicit one (world_pick_under_mouse calls
// vector3::operator= for the normal, then copies d).
class plane {
public:
    void set_normal_point(const vector3 *n, const vector3 *p)   // 0x43b5a0 plane__set_normal_point
    { normal = *n; d = (p->y * n->y + p->z * n->z + p->x * n->x) * -1.0f; }
    float distance_to_point(const vector3 *p);       // 0x50f860 Plane__distance_to_point

    vector3 normal;                                  // +0x00
    float d;                                         // +0x0c
};
LAYOUT_CHECK(plane, sizeof(plane) == 0x10);

// A ray {origin, direction} (gr_screen_to_world_ray's output has the same layout). Type name ours.
struct ix_ray {
    vector3 origin;                                  // +0x00
    vector3 dir;                                     // +0x0c
};
LAYOUT_CHECK(ix_ray, sizeof(ix_ray) == 0x18);

// false when the ray is parallel to the plane; else out = origin + dir * t (t may be negative).
bool  ix_ray_plane(ix_ray *ray, plane *pl, vector3 *out);    // 0x53ce30
// (point, segment start, segment end, out closest point): the distance (pick_test_character drops it).
float ix_point_linesegment(const vector3 *p, const vector3 *a, const vector3 *b, vector3 *closest);   // 0x53da80

#endif
