// unnamed_lobj_raycast_0x4ae890.cpp - Sum.exe v1.40, code 0x4ae890-0x4af335 (rays and swept spheres
// against the level objects: the object's node tree, its faces, the nearest hit).
// FILE NAME NOT KNOWN (placeholder). No assert path, no strings.tbl block, no reference name; module "-".
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit: the game tracker client ends with gtrack_send_channel_query (0x4ae830, code to 0x4ae882);
// multiplayerdata.cpp starts at 0x4af340. No .data; .bss 0x247e81c-0x247e823 (two words written by the face
// tests, never read). Constants: 0.0001f (0x581930), 1/3 (0x581b9c), -0.03f (0x581ba0), 2/1023 (0x581ba4)
// and 0.03f (0x5819c0, among objects.cpp's constants), 0.0f (0x57eb84), 1.0f (0x57e990) - shared .rdata.
//
// The ray is moved into the object's frame (lobj_ray::local_start / local_dir, dir as a segment: t in 0..1).
// A radius below 0.0001 is a thin ray (plane hit, then point in the triangle), otherwise a swept sphere (the
// plane, then the edges and vertices). The geometry's node tree (s3d_geom_block) is a BSP of 12-byte nodes
// with a 3 x 10-bit plane normal; leaves name one face or a face list.

#include <float.h>
#include <string.h>
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/math/ix.h"
#include "engine/s3d/s3d.h"
#include "engine/rendering/levelrender.h"

// One node of the geometry's tree (12 bytes; s3d_geom_block +0x04 is the array). Type and names ours.
struct lobj_bsp_node {
    unsigned int plane;               // +0 normal x, y, z in bits 0-9, 10-19, 20-29 (n = v * 2/1023 - 1);
                                      //    bit 31 a front child, bit 30 a back child
    float d;                          // +4 plane offset
    unsigned int link;                // +8 bits 0-15 the first child (back = first + 1 when both exist);
                                      //    bits 16-30 the face, or with bit 31 the offset of a face list in
                                      //    s3d_geom_block +0x00 ({ushort count, ushort faces[count]})
};

#define LOBJ_BSP_STACK 0x80
#define LOBJ_BSP_EPSILON 0.03f

// ---- .bss --------------------------------------------------------------------------------------------
s3d_face *lobj_last_hit_face;               // 0x247e81c
int lobj_last_hit_index;                    // 0x247e820

// 0x4ae890  Confidence: likely (the order of the two rotated temporaries). Tests one level object: its
// flags against the ray's require / exclude masks, then the one face of ray->face (flag 2) or the tree.
// An object whose tree is empty (block +0x08 == 0) is skipped.
void lobj_raycast(int lobj_index, lobj_ray *ray, lobj_hit *hit, bool clear)
{
    lobj *l = &lobjs[lobj_index];

    if (clear == 1) {
        hit->num_hits = 0;
        hit->_unk20 = 0;
        hit->_unk28 = 0;
    }
    if ((ray->require_flags & l->flags) != ray->require_flags)
        return;
    if (ray->exclude_flags & l->flags)
        return;
    s3d_geometry *g = l->mesh;
    if (g->block._unk08 == 0)
        return;

    ray->local_start = (ray->start - l->pos).rotated(l->orient);
    vector3 end = ray->start + ray->dir;
    ray->local_dir = (end - l->pos).rotated(l->orient) - ray->local_start;

    if (ray->flags & 2)
        lobj_ray_test_geometry(ray->face, g, lobj_index, ray, hit);
    else
        lobj_raycast_bsp(&g->block, g->block._unk04, g, lobj_index, ray, hit);
}

// 0x4aea00  Confidence: exact.
void lobj_ray_test_geometry(s3d_face *face, s3d_geometry *g, int lobj_index, lobj_ray *ray, lobj_hit *hit)
{
    if (ray->_unk18 < 0.0001f)
        lobj_ray_test_faces(face, g, lobj_index, ray, hit);
    else
        lobj_sphere_test_faces(face, g, lobj_index, ray, hit);
}

// 0x4aea50  Confidence: likely (the operand order of the dot product and of the centroid sum; the centroid
// and the vertex list share stack slots, so they are in separate scopes). A face seen from behind counts
// only with face flag 0x200 (D5215), with the normal turned. Keeps the nearest hit.
void lobj_ray_test_faces(s3d_face *face, s3d_geometry *g, int lobj_index, lobj_ray *ray, lobj_hit *hit)
{
    vector3 n;
    ix_hit ixh;

    if (face->normal.dot(ray->local_dir) <= 0.0f) {
        n = face->normal;
    } else {
        if (!(face->flags & 0x200))
            return;
        n = -face->normal;
    }
    {
        vector3 *p0 = &g->vertices[face->v[0]];
        vector3 *p1 = &g->vertices[face->v[1]];
        vector3 *p2 = &g->vertices[face->v[2]];
        vector3 centre((p2->x + p1->x + p0->x) * (1.0f / 3.0f), (p2->y + p1->y + p0->y) * (1.0f / 3.0f),
                       (p2->z + p1->z + p0->z) * (1.0f / 3.0f));
        if (!ix_ray_polygon(&ixh, &ray->local_start, &ray->local_dir, &centre, &n))
            return;
    }
    if (hit->num_hits != 0 && !(ixh.t < hit->t))
        return;
    vector3 *verts[3];
    verts[0] = &g->vertices[face->v[0]];
    verts[1] = &g->vertices[face->v[1]];
    verts[2] = &g->vertices[face->v[2]];
    if (!ix_point_in_polygon(&ixh.pos, 3, verts, (plane *)&n))
        return;

    hit->t = ixh.t;
    hit->pos = ixh.pos;
    hit->normal = n;
    hit->face = lobj_index;
    hit->_unk20 = 0;
    hit->face_ptr = face;
    hit->num_hits++;
    lobj_last_hit_index = lobj_index;
    lobj_last_hit_face = face;
}

// 0x4aec20  Confidence: likely (the in-range flag: VC6 left a dead second compare of t with 1.0 in the
// t < 0 path; the parameter slots are reused for t, span and the second t). The sphere against the face's
// plane (inside the triangle), else against its edges and vertices (hit +0x20 = 1; no normal, the two
// globals are not written). A plane hit that is farther than the best one falls through to the edge test;
// an edge hit that is farther still counts (num_hits + 1).
void lobj_sphere_test_faces(s3d_face *face, s3d_geometry *g, int lobj_index, lobj_ray *ray, lobj_hit *hit)
{
    vector3 n;
    vector3 touch;
    vector3 *verts[3];
    float t, span, t2;
    int inside;

    if (face->normal.dot(ray->local_dir) <= 0.0f) {
        n = face->normal;
    } else {
        if (!(face->flags & 0x20))
            return;
        n = -face->normal;
    }
    vector3 *p0 = &g->vertices[face->v[0]];
    vector3 *p1 = &g->vertices[face->v[1]];
    vector3 *p2 = &g->vertices[face->v[2]];
    vector3 centre((p2->x + p1->x + p0->x) * (1.0f / 3.0f), (p2->y + p1->y + p0->y) * (1.0f / 3.0f),
                   (p2->z + p1->z + p0->z) * (1.0f / 3.0f));
    if (!ix_sphere_polygon_plane(&touch, &ray->local_start, &ray->local_dir, ray->_unk18, &n, &centre, &t, &span))
        return;

    verts[0] = &g->vertices[face->v[0]];
    verts[1] = &g->vertices[face->v[1]];
    verts[2] = &g->vertices[face->v[2]];
    inside = 1;
    if (t < 0.0f || t > 1.0f)
        inside = 0;

    if ((hit->num_hits == 0 || t < hit->t) && inside && ix_point_in_polygon(&touch, 3, verts, (plane *)&n)) {
        hit->t = t;
        hit->pos = touch;
        hit->normal = n;
        hit->face = lobj_index;
        hit->_unk20 = 0;
        hit->face_ptr = face;
        hit->num_hits++;
        lobj_last_hit_face = face;
        lobj_last_hit_index = lobj_index;
        return;
    }

    if (t > 1.0f)
        return;
    if (t + span < 0.0f)
        return;
    if (!ix_sphere_polygon(&touch, &ray->local_start, &ray->local_dir, ray->_unk18, 3, verts, &t2))
        return;
    if (hit->num_hits != 0 && !(t2 < hit->t)) {
        hit->num_hits++;
        return;
    }
    hit->t = t2;
    hit->pos = touch;
    hit->face = lobj_index;
    hit->_unk20 = 1;
    hit->face_ptr = face;
    hit->num_hits++;
}

// 0x4aeef0  Confidence: unsure (the shape of the walk: an explicit stack of the straddled nodes and their
// far children, popped to test the node's faces; the two copies of the hit merge are in the binary). The
// segment's end distances from each node plane pick the near child; a node it crosses (within 0.03) is
// pushed with its far child, and on the way back its faces are tested and the far child walked. The first
// leaf with a hit ends the walk: the hit replaces the caller's when it is nearer (the hit counts add up),
// otherwise only the caller's count goes up. A full stack (128) drops the node and its near child (D5216).
void lobj_raycast_bsp(s3d_geom_block *block, void *root, s3d_geometry *g, int lobj_index, lobj_ray *ray,
                      lobj_hit *hit)
{
    lobj_bsp_node *far_stack[LOBJ_BSP_STACK];
    lobj_bsp_node *node_stack[LOBJ_BSP_STACK];
    lobj_hit local;
    unsigned int depth;
    lobj_bsp_node *node = (lobj_bsp_node *)root;
    int near_i, far_i;

    memset(&local, 0, sizeof(local));
    depth = 0;
    for (;;) {
        unsigned int bits = node->plane;
        vector3 n((float)(bits & 0x3ff) * (2.0f / 1023.0f) - 1.0f,
                  (float)((bits >> 10) & 0x3ff) * (2.0f / 1023.0f) - 1.0f,
                  (float)((bits >> 20) & 0x3ff) * (2.0f / 1023.0f) - 1.0f);
        float d0 = n.dot(ray->local_start) + node->d;
        vector3 end = ray->local_dir + ray->local_start;
        float d1 = end.dot(n) + node->d;
        float lo = min(d0, d1);
        float hi = max(d0, d1);
        int front = (bits & 0x80000000) ? (node->link & 0xffff) : 0;
        int back = 0;
        if (bits & 0x40000000)
            back = (node->link & 0xffff) + ((bits & 0x80000000) ? 1 : 0);

        if (d0 < 0.0f || _isnan(d0)) {
            near_i = back;
            far_i = front;
        } else {
            near_i = front;
            far_i = back;
        }

        if (lo <= LOBJ_BSP_EPSILON && hi >= -LOBJ_BSP_EPSILON) {
            if (depth < LOBJ_BSP_STACK) {
                far_stack[depth] = far_i ? &((lobj_bsp_node *)block->_unk04)[far_i] : NULL;
                node_stack[depth] = node;
                depth++;
                if (near_i) {
                    node = &((lobj_bsp_node *)block->_unk04)[near_i];
                    continue;
                }
            }
        } else if (near_i) {
            node = &((lobj_bsp_node *)block->_unk04)[near_i];
            continue;
        }

        // Pop: test the faces of the crossed node, then walk its far child.
        for (;;) {
            if (depth == 0)
                return;
            depth--;
            unsigned int link = node_stack[depth]->link;
            if ((link >> 31) & 1) {
                ushort *list = block->_unk00 + ((link >> 16) & 0x7fff);
                unsigned int count = *list;
                ushort *faces = list + 1;
                for (unsigned int i = 0; i < count; i++, faces++) {
                    lobj_ray_test_geometry(&g->faces[*faces], g, lobj_index, ray, &local);
                    if (local.num_hits) {
                        if (hit->num_hits != 0 && hit->t < local.t) {
                            hit->num_hits++;
                            return;
                        }
                        local.num_hits += hit->num_hits;
                        *hit = local;
                        return;
                    }
                }
            } else {
                lobj_ray_test_geometry(&g->faces[(link >> 16) & 0x7fff], g, lobj_index, ray, &local);
                if (local.num_hits) {
                    if (hit->num_hits != 0 && hit->t < local.t) {
                        hit->num_hits++;
                        return;
                    }
                    local.num_hits += hit->num_hits;
                    *hit = local;
                    return;
                }
            }
            node = far_stack[depth];
            if (node)
                break;
        }
    }
}

// 0x4af2c0  Confidence: exact (VC6 byte match, match24; was likely) (the object count is read once: a local copy). Flag 1 stops at the first
// object with a hit, flag 4 at a hit nearer than ray->stop_t.
void lobj_raycast_all(lobj_ray *ray, lobj_hit *hit)
{
    hit->num_hits = 0;
    hit->_unk20 = 0;
    hit->_unk28 = 0;
    int count = num_lobjs;
    for (int i = 0; i < count; i++) {
        lobj_raycast(i, ray, hit, false);
        if ((ray->flags & 1) && hit->num_hits > 0)
            return;
        if ((ray->flags & 4) && hit->t < ray->stop_t)
            return;
    }
}

// 0x4af320  Confidence: exact. Full brightness everywhere (the lighting sample is compiled out).
void lighting_sample_stub(int lobj, int face, const float *point, ubyte *r, ubyte *g, ubyte *b)
{
    *r = 0xff;
    *g = 0xff;
    *b = 0xff;
}
