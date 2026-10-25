// engine/rendering/unnamed_decals_0x4bbe80.h - level decals (blood splots, the move-target marker, ground
// decals) projected onto the level objects: the unit 0x4bbe80-0x4bce55 (source at
// game/unnamed_decals_0x4bbe80.cpp, decomp56). File and directory names are placeholders: no path string,
// no strings.tbl block. The unit also holds s3d_geom_block's members (declared in engine/s3d/s3d.h).
// Not original source. Type and field names ours unless sum.symbols has one.
#ifndef ENGINE_RENDERING_UNNAMED_DECALS_0X4BBE80_H
#define ENGINE_RENDERING_UNNAMED_DECALS_0X4BBE80_H

#include "vsdk/vsdk.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/ix.h"
#include "engine/rendering/unnamed_projectors_0x4c0250.h"

struct lobj;
struct gr_vertex;

#define MAX_DECALS 32

// One decal (0x128 bytes; 32 at 0x2480a48). The flags word +0x24: bits 0-28 the lifetime in ms (0 = one
// frame: decals_recycle_unused frees it after the draw), bit 29 clipped (set after decal_build_clip_planes),
// bit 30 in use, bit 31 lit (light selection and per-vertex shading).
class decal_record {
public:
    decal_record();                   // 0x4bbed0 decal_record__ctor
    void set_frame(const plane &p);   // 0x4bc2c0 decal_set_frame: proj.set(&pos, radius, p)
    void set_u_axis(float scale);     // 0x4bc2e0 decal_set_u_axis: the last added plane gives u
    void set_v_axis(float scale);     // 0x4bc300 decal_set_v_axis: the last added plane gives v

    GameTimer timer;                  // +0x000 age (decal_place restarts it)
    int bitmap;                       // +0x004 -1 free
    vector3 pos;                      // +0x008 centre
    float radius;                     // +0x014 |u + v + depth|
    vector3 normal;                   // +0x018 u x v, normalised
    unsigned int lifetime : 29;       // +0x024
    unsigned int clipped : 1;
    unsigned int in_use : 1;
    unsigned int lit : 1;
    decal_record *next;               // +0x028 decal_active / decal_free ring
    decal_record *prev;               // +0x02c
    int colour;                       // +0x030 lights_shade_vertices' last argument
    float depth;                      // +0x034 projection depth, as a fraction of the larger axis
    projector_record proj;            // +0x038
    int u_plane;                      // +0x118 proj plane index for u
    int v_plane;                      // +0x11c proj plane index for v
    float u_scale;                    // +0x120 0.5 / |v|
    float v_scale;                    // +0x124 0.5 / |u|
};
LAYOUT_CHECK_32(decal_record, sizeof(decal_record) == 0x128);

// The age ring: one node per decal record (same index), sorted by the time left.
struct decal_node {
    decal_node *next;
    decal_node *prev;
};

extern decal_node decal_nodes[MAX_DECALS];            // 0x2480948
extern decal_record decal_records[MAX_DECALS];        // 0x2480a48
extern gr_vertex decal_verts[MAX_DECALS];             // 0x2482f48 one clipped polygon
extern int decal_2483548;                             // 0x2483548 never accessed (after decal_verts)
extern decal_record *decal_free;                      // 0x248354c
extern decal_record *decal_active;                    // 0x2483550
extern decal_node *decal_age;                         // 0x2483554 the first node expires first
extern gr_vertex *decal_vert_ptrs[MAX_DECALS];        // 0x59daf0 &decal_verts[i]

void decal_build_clip_planes(decal_record *d, const vector3 *pos, const vector3 *u, const vector3 *v);   // 0x4bbf10
// (bitmap, centre, u half-axis, v half-axis, lifetime ms (0 one frame), lit, colour, depth): the record
// index. Callers blood_splot_place, move_target_draw_decal, place_ground_decal.
int  decal_place(int bitmap, const vector3 *pos, const vector3 *u, const vector3 *v, int ms, bool lit,
                 int colour, float depth);        // 0x4bc320
void decal_recycle(decal_record *d, decal_node *n);   // 0x4bc560 (inline in the other users)
void decals_reset();                                  // 0x4bc640 (levelrender_level_load)
void decals_draw_for_lobj(int lobj_index, const vector3 *verts);   // 0x4bc6b0 (levelrender_draw_list)
// (lobj, out list, out count): the active decals touching the object; the list takes at most 0x40.
int  decals_collect_for_lobj(lobj *lo, projector_record **out, int *count);   // 0x4bcbb0
void decals_recycle_unused();                         // 0x4bcc20 (levelrender_render_scene)

#endif
