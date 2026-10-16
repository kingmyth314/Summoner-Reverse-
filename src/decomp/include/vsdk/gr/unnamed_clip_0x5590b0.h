// vsdk/gr/unnamed_clip_0x5590b0.h - the 3D clipper and the bounding box helpers (vsdk\gr, 0x5590b0-0x559da7;
// source at src/decomp/vsdk/unnamed_clip_0x5590b0.cpp, decomp66; file name a placeholder). Not original
// source.
#ifndef VSDK_GR_UNNAMED_CLIP_0X5590B0_H
#define VSDK_GR_UNNAMED_CLIP_0X5590B0_H

#include "vsdk/vsdk.h"
#include "vsdk/gr/gr.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/math/ix.h"

// Clip codes of gr_vertex::codes (gr_code_vertex 0x50e890): the view frustum is x, y within +-z.
#define GR_CLIP_FAR     0x01          // z beyond gr_far_clip_z (when gr_far_clip_enabled)
#define GR_CLIP_LEFT    0x02          // x < -z
#define GR_CLIP_RIGHT   0x04          // x > z
#define GR_CLIP_BOTTOM  0x08          // y < -z
#define GR_CLIP_TOP     0x10          // y > z
#define GR_CLIP_BEHIND  0x20          // z <= 0 (never clipped against)

// gr_vertex::_unk1a: what a vertex carries besides the position (the clipper interpolates these).
#define GR_VERTEX_UV    0x01
#define GR_VERTEX_UV2   0x02
#define GR_VERTEX_RGB   0x04
#define GR_VERTEX_ALPHA 0x08

// gr_far_clip_z, gr_code_vertex, the inline gr_clip_free_temp_verts and the extern of gr_clip_num_temp_verts
// (0x2f54700, defined in this unit) are declared in vsdk/gr/unnamed_gr_3d_0x50d9b0.h (decomp65, included
// by gr.h), and so are this unit's bbox_get_corner (0x559960) and bbox_corner_indices_from_normal
// (0x559a80); decomp66 corrected their comments there. The function gr_clip_plane (0x5593a0) shares its
// name with decomp65's class gr_clip_plane: after this header the class needs its `class` tag.

// ---- the clipper (.bss 0x2f53b00-0x2f54703) ----------------------------------------------------------
#define GR_CLIP_MAX_TEMP_VERTS 64
extern gr_vertex gr_clip_temp_verts[GR_CLIP_MAX_TEMP_VERTS];   // 0x2f53b00
// (plane bit, inside vertex, outside vertex): the crossing point, a new temporary vertex with its codes.
gr_vertex *gr_clip_edge(int plane, gr_vertex *on, gr_vertex *off);   // 0x5590e0
gr_vertex *gr_clip_alloc_temp_vertex();                   // 0x559330 (no bound check)
// (&p0, &p1, codes of both or'ed): clips the line in place.
void gr_clip_line(gr_vertex **p0, gr_vertex **p1, ubyte codes);   // 0x559350
// (plane bit, src[n + 2], dst, &n, codes {or, and}): the vertex count written to dst.
int gr_clip_plane(int plane, gr_vertex **src, gr_vertex **dst, int *nv, ubyte *codes);   // 0x5593a0
// (src, temp, &n, codes {or, and}): clips against every plane in codes[0], ping-ponging between the two
// lists; returns the list that holds the result (stops early when codes[1], the and, becomes non-zero).
gr_vertex **gr_clip_polygon(gr_vertex **src, gr_vertex **tmp, int *nv, ubyte *codes);   // 0x559490
// (view-space centre, radius) against the four 45-degree side planes and z = 0: -1 outside, 1 inside,
// 0 crossing.
int gr_classify_sphere_view(const vector3 *p, float r);   // 0x5594f0

// ---- bounding boxes ---------------------------------------------------------------------------------
// bbox_from_points (0x5595e0) is declared in vsdk/math/ix.h.
// (min, max, ray {origin, dir}, length): the box of origin .. origin + dir * length.
void bbox_from_segment(vector3 *min, vector3 *max, const ix_ray *seg, float len);   // 0x559660
bool bbox_overlap(const vector3 *amin, const vector3 *amax, const vector3 *bmin, const vector3 *bmax);   // 0x5596b0
void bbox_add_point(vector3 *min, vector3 *max, const vector3 *p);   // 0x559710 (no symbol, name ours; no caller)
void bbox_union(vector3 *min, vector3 *max, const vector3 *bmin, const vector3 *bmax);   // 0x559790
// bbox_expand_by_vector (0x559810) is declared in vsdk/math/ix.h.
// (out[6], eye, min, max): the box corners of the outline seen from eye; 0 when eye is inside.
int bbox_silhouette_corners(vector3 *out, const vector3 *p, const vector3 *min, const vector3 *max);   // 0x559890
bool bbox_straddles_plane(const vector3 *min, const vector3 *max, plane *p);   // 0x559b80 (the plane is the third argument, D6802)
// (orient, pos, min, max, out min, out max): the axis-aligned box of a rotated and moved box.
void bbox_transform(matrix *m, const vector3 *pos, vector3 *min, vector3 *max,
                    vector3 *out_min, vector3 *out_max);   // 0x559c20 (no symbol, name ours; no caller)

#endif
