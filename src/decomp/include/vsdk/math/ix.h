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
    // decomp56 (D3601): the constructor from a normal and a point is the COMDAT 0x4a42e0 (sum.symbols
    // plane__plane: the same body, returns this); the shadow and decal code build temporary
    // planes with it for projector_record::add_plane. The default constructor is empty.
    plane() {}
    plane(const vector3 *n, const vector3 *p)         // 0x4a42e0 plane__plane
    { normal = *n; d = (p->y * n->y + p->z * n->z + p->x * n->x) * -1.0f; }
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
// decomp56: polygon tests against plane sets (vsdkgeom in sum.symbols, file not known), used by the
// decal, shadow and projector code. (n, points, planes, count, offset): false when for some plane no
// point lies behind it. (n < 32, in, out, planes, count): clips in to out, the vertex count (< 3: gone).
int   polygon_touches_planes(int n, const vector3 *pts, const plane *planes, int num_planes, int offset);   // 0x551f90
int   polygon_clip_to_planes(int n, const vector3 *in, vector3 *out, const plane *planes, int num_planes);   // 0x551e90
float ix_point_linesegment(const vector3 *p, const vector3 *a, const vector3 *b, vector3 *closest);   // 0x53da80
// decomp64 (fx_find_spawn_point): the 2D segments a-b and c-d cross (both parameters in 0..1); the second
// segment is passed by value (six argument dwords, the body reads c and d at [esp+0xc]..[esp+0x18]).
class vector2;
bool  seg2d_intersect(const vector2 *a, const vector2 *b, vector2 c, vector2 d);   // 0x53d8f0
// decomp55 (the script editor's picking). (out hit, out t, ray, ray length, centre, radius): the ray within
// length hits the sphere; (ray, box centre, box orientation, box size, out hit): the oriented box. Which
// of the sphere test's first two outputs is the point is from the editor's use (it measures from the
// first); the parameter names are ours.
class matrix;
bool  ix_ray_sphere(vector3 *hit, float *t, ix_ray *ray, float length, const vector3 *centre, float radius);   // 0x53d7a0
bool  ix_ray_box_oriented(ix_ray *ray, const vector3 *centre, const matrix *orient, const vector3 *size,
                          vector3 *hit);                                     // 0x53d100

#endif
