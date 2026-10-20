// vsdk/math/ix.h - intersection tests and the plane (vsdk\math module of sum.symbols: ix_ray_plane
// 0x53ce30, ix_point_linesegment 0x53da80, Plane__distance_to_point 0x50f860; file name ours, decomp35).
// Not original source.
#ifndef VSDK_MATH_IX_H
#define VSDK_MATH_IX_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"               // plane::transform (decomp65)

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
    // decomp65 (D4502): the members sum.symbols calls Plane__* are inline: their copies are COMDATs at the
    // end of the 3D view object (0x50f7d0-0x50f8f3), which was built without inline expansion; the ix
    // code (ix_ray_plane, ix_ray_quad, ...) calls those copies. distance_to_point was out of line.
    plane(const vector3 *p0, const vector3 *p1, const vector3 *p2)   // 0x50f7d0 Plane__Plane_points
    { set_from_points(p0, p1, p2); }
    // normal = the normalised edge cross product, d = -(normal . p0).
    void set_from_points(const vector3 *p0, const vector3 *p1, const vector3 *p2)   // 0x50f800 Plane__set_from_points
    {
        vector3_perp(&normal, p0, p1, p2);
        normal.normalize();
        d = normal.dot(*p0) * -1.0f;
    }
    float distance_to_point(const vector3 *p)        // 0x50f860 Plane__distance_to_point
    { return p->dot(normal) + d; }
    // The plane in the frame at pos / orient (gr_cull_bounding_box_transformed). Returned by value
    // (D16401): the caller's local is the hidden result (no constructor call there), and `return p`
    // copy-constructs into it through the implicit copy constructor, the COMDAT 0x4c0a20 (four dword
    // moves; folded with the projector object's plane assignment, D4502).
    plane transform(const vector3 *pos, const matrix *orient)   // 0x50f880 Plane__transform
    {
        plane p;
        vector3 pt = normal * d;
        pt += *pos;
        p.d = normal.dot(pt);
        p.normal = normal.rotated(*orient);
        return p;
    }

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
// decomp70 (D5104): offset is a float (plane_classify_point loads it with FLD; was int).
int   polygon_touches_planes(int n, const vector3 *pts, const plane *planes, int num_planes, float offset);   // 0x551f90
int   polygon_clip_to_planes(int n, const vector3 *in, vector3 *out, const plane *planes, int num_planes);   // 0x551e90
// decomp70: the rest of the polygon clip unit (vsdk/unnamed_poly_clip_0x5519c0.cpp; cdecl). Point against
// plane at distance offset: 1 in front (> offset + 0.0001), 2 behind (< offset - 0.0001), 3 on it.
int   plane_classify_point(const plane *pl, const vector3 *p, float offset);   // 0x551e00
// One Sutherland-Hodgman pass (the part behind the plane is kept); returns the new count.
int   polygon_clip_to_plane(int n, const vector3 *in, vector3 *out, const plane *pl);   // 0x551ca0
int   polygon_any_point_in_front(int n, const vector3 *pts, const plane *pl);   // 0x551e50 (no caller)
// (point, radius, planes, count, offset): every plane distance < radius + offset - 0.0001 / < offset -
// 0.0001 - radius (no callers).
bool  sphere_intersects_plane_volume(const vector3 *p, float radius, plane *planes, int num_planes, float offset);   // 0x552010
bool  sphere_inside_plane_volume(const vector3 *p, float radius, plane *planes, int num_planes, float offset);   // 0x552070
// (num_edges, clip polygon, n, verts, out, result): clips the 2D polygon verts against the convex
// polygon (its edge normals face away from its third vertex); *result = out when a vertex changed (then
// the vertices are copied to out). Returns the count, 0 when fewer than 3 remain (no caller).
int   polygon_clip_2d(int num_edges, const vector2 *edges, int n, vector2 *verts, vector2 *out,
                      vector2 **result);   // 0x5519f0
// decomp67: the result is the closest point's distance along a-b (clamped to 0..|b - a|), not its
// distance from p.
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
// decomp67: length is not read; with the ray origin inside the sphere hit = origin and t is not written.
bool  ix_ray_sphere(vector3 *hit, float *t, ix_ray *ray, float length, const vector3 *centre, float radius);   // 0x53d7a0
bool  ix_ray_box_oriented(ix_ray *ray, const vector3 *centre, const matrix *orient, const vector3 *size,
                          vector3 *hit);                                     // 0x53d100

// ---- decomp67: the rest of the ix unit (0x53ac20-0x53deb0, src/decomp/vsdk/unnamed_ix_0x53ac20.cpp) ----
// A hit point with its parameter (16 bytes: the two segment / plane tests store t at +0x0c; name ours).
struct ix_hit {
    vector3 pos;                                     // +0x00
    float t;                                         // +0x0c
};
LAYOUT_CHECK(ix_hit, sizeof(ix_hit) == 0x10);
// Segments are {start, direction} (ix_ray) or a start and a direction pointer; polygons are arrays of
// vertex pointers with their plane. The sum.symbols names are kept where the code does something else
// (NOTES decomp67): ix_ray_polygon is a segment / point-normal plane test, ix_line_line intersects two
// planes, ix_slab_clip computes the outcode of a point against a box, get_tag_transform hides tags.
// (out, p, d, q, n): p + t d (t in 0..1) on the plane through q with normal n; out->t only on a hit.
bool  ix_ray_polygon(ix_hit *out, const vector3 *p, const vector3 *d, const vector3 *q, const vector3 *n);   // 0x53ac20
// (out, p, d, plane): out->t is stored before the 0..1 test (no caller).
bool  ix_linesegment_plane_t(ix_hit *out, const vector3 *p, const vector3 *d, plane *pl);   // 0x53ad40
// (ray, triangle, its plane, out hit, out t): ix_ray_plane_t, then the hit inside the triangle.
bool  ix_ray_plane_point(ix_ray *ray, vector3 **tri, plane *pl, vector3 *hit, float *t);   // 0x53adf0
// (p, d, plane, out t): p in front of the plane and p + d reaching it.
bool  ix_linesegment_plane_front_t(const vector3 *p, const vector3 *d, plane *pl, float *t);   // 0x53ae60
bool  ix_linesegment_triangle_front(ix_ray *seg, vector3 **tri, plane *pl, vector3 *hit, float *t);   // 0x53aec0
// (ray, corner and first edge, second edge, out hit, out position along the second edge) (no caller).
bool  ix_ray_parallelogram(ix_ray *ray, ix_ray *edge, const vector3 *b, vector3 *hit, float *u);   // 0x53af40
// (ray, the four corners, out hit, out 0..1 position): two triangle pairs (no caller).
bool  ix_ray_quad(ix_ray *ray, vector3 *p0, vector3 *p1, vector3 *p2, vector3 *p3, vector3 *hit, float *w);   // 0x53b130
// (out entry point, a, b, centre, radius): out = a when a is inside (no caller).
bool  ix_linesegment_sphere(vector3 *out, const vector3 *a, const vector3 *b, const vector3 *centre, float r);   // 0x53b400
// (point, vertex count, vertex pointers, plane): a triangle-fan test in the plane of the largest normal
// component (lobj_ray_test_faces, lobj_sphere_test_faces).
bool  ix_point_in_polygon(const vector3 *p, int n, vector3 **verts, plane *pl);   // 0x53b6f0
bool  ix_sphere_sphere(const vector3 *a, const vector3 *b, float ra, float rb);   // 0x53b8f0 (no caller)
// (out touch point, start, move, radius, plane normal, plane point, out entry t, out t span): the moving
// sphere against the plane (lobj_sphere_test_faces).
bool  ix_sphere_polygon_plane(vector3 *out, const vector3 *p0, const vector3 *d, float r, const vector3 *n,
                              const vector3 *pp, float *t, float *span);   // 0x53b930
// (centre, move, radius, plane, out t, out hit) (no caller).
bool  ix_sphere_plane_front(const vector3 *c, const vector3 *d, float r, plane *pl, float *t, vector3 *hit);   // 0x53bad0
// (out hit, start, move, radius, edge a, edge b, out t, best t so far): the moving sphere against the
// edge's cylinder, then against vertex a.
bool  ix_sphere_polygon_edge(vector3 *hit, const vector3 *p0, const vector3 *d, float r, const vector3 *a,
                             const vector3 *b, float *t, float best);   // 0x53bb50
// (out hit, start, move, radius, vertex count, vertex pointers, out t): the polygon's edges and vertices.
bool  ix_sphere_polygon(vector3 *hit, const vector3 *p0, const vector3 *d, float r, int n, vector3 **verts,
                        float *t);                                       // 0x53bf60
bool  ix_boundingbox_boundingbox(const vector3 *min1, const vector3 *max1, const vector3 *min2,
                                 const vector3 *max2);                   // 0x53c200 (no caller)
bool  ix_point_boundingbox(const vector3 *p, const vector3 *min, const vector3 *max);   // 0x53c260
bool  ix_point_box_oriented(const vector3 *p, const vector3 *centre, const matrix *orient, const vector3 *size);   // 0x53c2c0
// decomp67: (point, radius, min, max); sum.symbols reads (center, min, max, radius).
bool  ix_sphere_boundingbox(const vector3 *p, float r, const vector3 *min, const vector3 *max);   // 0x53c410
bool  ix_sphere_box_oriented(const vector3 *p, float r, const vector3 *centre, const matrix *orient,
                             const vector3 *size);                       // 0x53c4a0
// (B: rotation rows 16 bytes apart, T: translation, a / b: half extents): 0 when no axis separates the
// boxes, else the number (1..15) of the first separating axis (no caller).
int   ix_box_oriented_box_oriented(const float B[3][4], const vector3 *T, const vector3 *a, const vector3 *b);   // 0x53c610
// (plane a, plane b, out line {point, unit direction}): false for parallel planes.
bool  ix_line_line(const plane *a, const plane *b, ix_ray *out);   // 0x53cb60
bool  ix_ray_plane_point_b(ix_ray *ray, plane *pl, vector3 *out);   // 0x53cd60 ix_ray_plane without the t >= 0 test
bool  ix_ray_plane_t(ix_ray *ray, plane *pl, float *t);            // 0x53cde0
// (box centre, orientation, size, a, b, out point): the segment in box space.
bool  ix_linesegment_box_oriented(const vector3 *centre, const matrix *orient, const vector3 *size,
                                  const vector3 *a, const vector3 *b, vector3 *out);   // 0x53ced0
// (min, max, origin, direction, out hit): Woo's slab test; the hit can be past the ray's length.
bool  ix_ray_boundingbox(const vector3 *min, const vector3 *max, const vector3 *origin, const vector3 *dir,
                         vector3 *hit);                                  // 0x53d2d0
// (min, max, a, b, out): a (or b) when inside, else the face of a's outcode that the segment crosses.
bool  ix_linesegment_boundingbox(const vector3 *min, const vector3 *max, const vector3 *a, const vector3 *b,
                                 vector3 *out);                          // 0x53d4d0
// (min, max, p): outcode 1 / 2 x above / below, 4 / 8 y, 0x20 / 0x10 z.
int   ix_slab_clip(const vector3 *min, const vector3 *max, const vector3 *p);   // 0x53d720
// (point, ray, out closest point): the ray parameter of the closest point (no caller).
float ix_ray_closest_point_t(const vector3 *p, ix_ray *ray, vector3 *out);   // 0x53d9d0
// (a, b, centre, radius): the length of a-b inside the sphere (no caller).
float ix_linesegment_sphere_chord_length(const vector3 *a, const vector3 *b, const vector3 *centre, float r);   // 0x53dbb0
// (a, b, radius, out x, out angle, out y): the tangent point from a to the sphere at b (no caller).
bool  ix_point_sphere_tangent(const vector3 *a, const vector3 *b, float r, float *x, float *angle, float *y);   // 0x53dcd0
// decomp67: calls into another vsdk\gr object (the bbox helpers next to bbox_get_corner; vsdk\gr in sum.symbols).
void  bbox_from_points(vector3 *mins, vector3 *maxs, const vector3 *a, const vector3 *b);   // 0x5595e0
void  bbox_expand_by_vector(vector3 *mins, vector3 *maxs, const vector3 *delta);           // 0x559810

#endif
