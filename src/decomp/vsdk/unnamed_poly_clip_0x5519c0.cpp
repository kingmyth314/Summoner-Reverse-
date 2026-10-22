// unnamed_poly_clip_0x5519c0.cpp - Sum.exe v1.40, code 0x5519c0-0x5520c0: polygon clipping against
// planes and a 2D convex polygon, plane / sphere tests (vsdk\geom in sum.symbols). The file name is a
// placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout or
// scheduling risk, unsure = the source shape is a guess.
//
// Unit range: the static initialiser of the 2D point buffer opens the object (0x5519c0, table entry
// 0x588a20); it ends before the CRC-32 code (0x5520d0, vsdk/unnamed_crc32_0x5520d0.cpp, whose table
// is the next .bss block). .bss 0x2f13368-0x2f136e7, no .data. Built without inline expansion
// (vector2::dot / operator=, vector3 operators and plane::distance_to_point are called; the linker
// kept the COMDAT copies of other objects). Float constants: 0.0001f 0x581930, -0.0001f 0x581b74.

#include <math.h>
#include "vsdk/math/ix.h"
#include "vsdk/math/vector.h"

#define POLY_EPSILON 0.0001f          // 0x581930

// 0x5519c0 init_vector_buf_2f13368 (jmp 0x5519d0: vector constructor iterator, 64 x 8 bytes).
// Confidence: exact. The points polygon_clip_2d creates (never reset between edges).
vector2 geom_poly2d_clip_points[64];  // 0x2f13368
float geom_poly2d_clip_dists[32];     // 0x2f13568
vector2 *geom_poly2d_clip_list[32];   // 0x2f135e8
vector2 *geom_poly2d_clip_list2[32];  // 0x2f13668

// 0x5519f0  Confidence: likely (the tail shared by the two split paths; the order of the |x| + |y|
// sum; D24505: b = edges then `if (i + 1 < num_edges) b = a + 1`, dst = list2 then `if (list != list1)
// dst = list1` after the wrap stores, the first list filled through geom_poly2d_clip_list and list set
// after it; left: n / the init counter in ESI / EBX, the dot loop's two pointers). The edge normals are scaled by 1 / (|x| + |y|), not normalised; the normal is flipped for every
// edge when the clip polygon's third vertex lies in front of the first edge. Nothing bounds the point
// lists (32 entries, n + 1 used) or the point buffer (64 per call). No caller.
int polygon_clip_2d(int num_edges, const vector2 *edges, int n, vector2 *verts, vector2 *out,
                    vector2 **result)
{
    vector2 *newp = geom_poly2d_clip_points;
    vector2 **list;
    vector2 normal;
    bool flip;
    int i, k;

    *result = verts;
    flip = false;
    for (k = 0; k < n; k++)
        geom_poly2d_clip_list[k] = &verts[k];
    list = geom_poly2d_clip_list;

    for (i = 0; i < num_edges; i++) {
        const vector2 *a = &edges[i];
        const vector2 *b = edges;
        if (i + 1 < num_edges)
            b = a + 1;
        normal.x = b->y - a->y;
        normal.y = a->x - b->x;
        float len = (float)(fabs(normal.y) + fabs(normal.x));
        if (len < POLY_EPSILON)
            continue;
        normal.x = normal.x / len;
        normal.y = normal.y / len;
        float d = a->dot(normal);
        if (i == 0 && b[1].dot(normal) - d > POLY_EPSILON)
            flip = true;
        if (flip) {
            d = -d;
            normal.x = -normal.x;
            normal.y = -normal.y;
        }

        int outside = 0;
        for (k = 0; k < n; k++) {
            geom_poly2d_clip_dists[k] = list[k]->dot(normal) - d;
            if (geom_poly2d_clip_dists[k] > POLY_EPSILON)
                outside++;
        }
        if (outside == n)
            return 0;
        if (outside == 0)
            continue;

        vector2 **dst = geom_poly2d_clip_list2;
        geom_poly2d_clip_dists[n] = geom_poly2d_clip_dists[0];
        list[n] = list[0];
        if (list != geom_poly2d_clip_list)
            dst = geom_poly2d_clip_list;
        vector2 **p = dst;
        for (k = 0; k < n; k++) {
            if (geom_poly2d_clip_dists[k] < POLY_EPSILON) {
                *p++ = list[k];
                if (!(geom_poly2d_clip_dists[k] < -POLY_EPSILON && geom_poly2d_clip_dists[k + 1] > POLY_EPSILON))
                    continue;
            } else if (!(geom_poly2d_clip_dists[k + 1] < -POLY_EPSILON)) {
                *result = out;
                continue;
            }
            float t = geom_poly2d_clip_dists[k] / (geom_poly2d_clip_dists[k] - geom_poly2d_clip_dists[k + 1]);
            vector2 *p1 = list[k + 1];
            vector2 *p0 = list[k];
            *p++ = newp;
            newp->x = (p1->x - p0->x) * t + p0->x;
            newp->y = (p1->y - p0->y) * t + p0->y;
            newp++;
            *result = out;
        }
        n = p - dst;
        list = dst;
    }

    if (n < 3)
        return 0;
    if (*result != verts) {
        for (k = 0; k < n; k++)
            out[k] = *list[k];
    }
    return n;
}

// 0x551ca0  Confidence: exact (D15007: out indexed by count, which 8966 strength-reduces into the out slot;
// the clip block entered by a goto from the inside branch; i stepped with next at the loop end). Keeps
// the vertices not in front of the plane and adds the crossing points.
int polygon_clip_to_plane(int n, const vector3 *in, vector3 *out, const plane *pl)
{
    int next = 1;
    int count = 0;
    int i;

    for (i = 0; i < n; ) {
        int inside = 1;
        if (plane_classify_point(pl, &in[i], 0.0f) == 1)
            inside = 0;
        int next_inside = 1;
        if (plane_classify_point(pl, &in[next], 0.0f) == 1)
            next_inside = 0;
        if (inside) {
            out[count] = in[i];
            count++;
            if (!next_inside)
                goto clip;
        } else if (next_inside) {
        clip:
            float d0 = -in[i].dot(pl->normal);
            float d1 = -in[next].dot(pl->normal);
            float t = (pl->d - d0) / (d1 - d0);
            out[count] = in[i] + (in[next] - in[i]) * t;
            count++;
        }
        i++;
        next++;
        if (next == n)
            next = 0;
    }
    return count;
}

// 0x551e00  Confidence: exact.
int plane_classify_point(const plane *pl, const vector3 *p, float offset)
{
    float d = ((plane *)pl)->distance_to_point(p);
    if (d > offset + POLY_EPSILON)
        return 1;
    if (d < offset - POLY_EPSILON)
        return 2;
    return 3;
}

// 0x551e50  Confidence: exact. No caller.
int polygon_any_point_in_front(int n, const vector3 *pts, const plane *pl)
{
    for (int i = 0; i < n; i++) {
        if (plane_classify_point(pl, &pts[i], 0.0f) == 1)
            return 1;
    }
    return 0;
}

// 0x551e90  Confidence: exact. n must be below 32 (the scratch polygon); the passes ping-pong between
// out and the scratch array, the result ends in out.
int polygon_clip_to_planes(int n, const vector3 *in, vector3 *out, const plane *planes, int num_planes)
{
    int i;

    if (n >= 32)
        return 0;
    vector3 tmp[32];
    for (i = 0; i < n; i++)
        out[i] = in[i];
    vector3 *src = out;
    vector3 *dst = tmp;
    for (i = 0; i < num_planes; i++) {
        n = polygon_clip_to_plane(n, src, dst, &planes[i]);
        if (n < 3)
            return 0;
        vector3 *swap = src;
        src = dst;
        dst = swap;
    }
    if (src != out) {
        for (i = 0; i < n; i++)
            out[i] = src[i];
    }
    return n;
}

// 0x551f90  Confidence: exact (the exit test after the inner loop). False when every point lies on or
// in front of some plane.
int polygon_touches_planes(int n, const vector3 *pts, const plane *planes, int num_planes, float offset)
{
    int p, i;

    for (p = 0; p < num_planes; p++) {
        for (i = 0; i < n; i++) {
            if (plane_classify_point(&planes[p], &pts[i], offset) == 2)
                break;
        }
        if (i == n)
            break;
    }
    if (p >= num_planes)
        return 1;
    return 0;
}

// 0x552010  Confidence: exact. No caller.
bool sphere_intersects_plane_volume(const vector3 *p, float radius, plane *planes, int num_planes, float offset)
{
    for (int i = 0; i < num_planes; i++) {
        if (!(planes[i].distance_to_point(p) < radius + offset - POLY_EPSILON))
            return false;
    }
    return true;
}

// 0x552070  Confidence: exact. No caller.
bool sphere_inside_plane_volume(const vector3 *p, float radius, plane *planes, int num_planes, float offset)
{
    for (int i = 0; i < num_planes; i++) {
        if (!(planes[i].distance_to_point(p) < offset - POLY_EPSILON - radius))
            return false;
    }
    return true;
}
