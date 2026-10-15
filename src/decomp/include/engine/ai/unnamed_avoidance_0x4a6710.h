// engine/ai/unnamed_avoidance_0x4a6710.h - obstacle avoidance: avoidance areas ("AA"), their obstacle
// primitives, the merged avoidance polygons, the segment lists the pathfinder links in, and the avoidance
// splines (code 0x4a6710-0x4aa77f; source game/unnamed_avoidance_0x4a6710.cpp). The file name is a
// placeholder: no path string, no strings.tbl block (docs/sum-exe/source-map.md); the directory follows
// engine/ai/pathfinding.h, whose unit is the only user of most of it. Not original source.
// decomp57 moved the avoidance block of engine/ai/ai.h here (ai.h includes this header) and replaced its
// partial layouts with the full ones (D3701-D3704). Type and field names are ours unless sum.symbols
// names them.
#ifndef ENGINE_AI_UNNAMED_AVOIDANCE_0X4A6710_H
#define ENGINE_AI_UNNAMED_AVOIDANCE_0X4A6710_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/os/console.h"
#include "engine/ai/pathfinding.h"

// An avoidance-area primitive ("AA"; 0x30 bytes on the caller's stack, avoid_area_add reads it).
// Layout from the three spells.cpp builders (decomp23) and avoid_area_add (decomp57): an oriented box or
// a circle (an octagon) around (cx, cz).
struct avoid_area_desc {
    int _unk00;                       // +0x00 0 for the spell areas (soft avoidance, obstacle flag 4); 1 from
                                      //       summon_spawn_effect (0x48cbd7): a prohibited area (flag 2)
    int shape;                        // +0x04 0 box, 1 circle (spell_spawn_hit_fx)
    float cx, cz;                     // +0x08 centre (x, z)
    float ax, az;                     // +0x10 first axis
    float bx, bz;                     // +0x18 second axis
    float extent_a, extent_b;         // +0x20 half sizes along the axes (extent_a is the circle radius)
    float _unk28;                     // +0x28 0.5 at every caller: the extra margin of the size class 2 copy
    int owner;                        // +0x2C the layer of the obstacles (avoid_area_add passes it to
                                      //       avoid_add_primitive as the layer, D3702; spells.cpp's name kept)
};
LAYOUT_CHECK(avoid_area_desc, sizeof(avoid_area_desc) == 0x30);

// ---- Obstacles (0x88, 14 at 0x246a080; sum.symbols avoid_obstacles) -----------------------------------
#define AVOID_OB_USED     0x01
#define AVOID_OB_PROHIBIT 0x02        // counted in avoid_obstacle_counts; blocks walks (pf_segment_hits_walls,
                                      // avoid_point_blocked)
#define AVOID_OB_AVOID    0x04        // counted in avoid_obstacle_counts_b; only adds path cost
#define AVOID_OB_BOX      0x08        // oriented box (else circle: an octagon of 8 points)
#define AVOID_OB_CIRCLE   0x10
#define AVOID_OB_SEGMENT  0x20        // a segment primitive (avoid_prim_from_segment, 0x21 with USED)
// The obstacle and the segment primitive avoid_prim_from_segment builds are the same record: every test
// reads both through the same offsets (avoid_prim_overlap, D3701).
struct avoid_obstacle {
    ushort flags;                     // +0x00 AVOID_OB_*
    short num_points;                 // +0x02 4 (box), 8 (circle), 2 (segment)
    short size;                       // +0x04 size class
    short layer;                      // +0x06
    vector2 centre;                   // +0x08
    vector2 axis_a;                   // +0x10 unit axes; a circle has four (0, 45, 90, 135 degrees)
    vector2 axis_b;                   // +0x18
    vector2 axis_c;                   // +0x20
    vector2 axis_d;                   // +0x28
    float half_a;                     // +0x30 half size along axis_a / radius
    float half_b;                     // +0x34 half size along axis_b / radius
    vector2 points[8];                // +0x38 outline (static initialiser 0x4a6760 constructs them)
    rect2d bounds;                    // +0x78
};
LAYOUT_CHECK(avoid_obstacle, sizeof(avoid_obstacle) == 0x88);
typedef avoid_obstacle avoid_prim;    // decomp32's name for the segment primitive (D3701)
#define AVOID_NUM_OBSTACLES 14
extern avoid_obstacle avoid_obstacles[AVOID_NUM_OBSTACLES];   // 0x246a080
extern char avoid_obstacle_counts[PF_NUM_SIZES * PF_MAX_LAYERS];     // 0x246a040 prohibit obstacles per size * 20 + layer
extern char avoid_obstacle_counts_b[PF_NUM_SIZES * PF_MAX_LAYERS];   // 0x246b708 avoid obstacles (name ours)

// ---- Avoidance polygons (0x114, 14 at 0x246a7f0; sum.symbols avoid_polys) -----------------------------
// The union outline of up to 8 overlapping obstacles of one size class and layer (avoid_merge_obstacle_polys).
struct avoid_poly {
    short flags;                      // +0x00 the obstacles' flags & 7 (used, prohibit, avoid)
    short count;                      // +0x02 obstacles merged in (decomp32's name)
    short num_points;                 // +0x04
    short seglist;                    // +0x06 avoid_seglists index (avoid_clip_segments), -1 none
    char obstacles[8];                // +0x08 avoid_obstacles indices (-1 = merged away)
    short size;                       // +0x10
    short layer;                      // +0x12
    vector2 points[30];               // +0x14 (static initialiser 0x4a6720 constructs them)
    rect2d bounds;                    // +0x104
};
LAYOUT_CHECK(avoid_poly, sizeof(avoid_poly) == 0x114);
#define AVOID_NUM_POLYS 14
extern avoid_poly avoid_polys[AVOID_NUM_POLYS];     // 0x246a7f0

// ---- Segment lists (0xa78, 14 at 0x2460db0; sum.symbols avoid_seglists) ------------------------------
// One per polygon: its outline clipped against the navigation walls (avoid_clip_segments), as segments
// between corners; each corner is also a visibility node the A* search links in (pf_graph_search).
struct avoid_seg_ref {                // 0x14, segment list +0x18
    short a;                          // +0x00 corner indices
    short b;                          // +0x02
    pf_edge *wall;                    // +0x04 the nav wall the segment ends on, NULL inside
    int side;                         // +0x08 1: the second end lies on `wall`
    vector2 normal;                   // +0x0c set by the corner offset code
};
LAYOUT_CHECK(avoid_seg_ref, sizeof(avoid_seg_ref) == 0x14);
// A block of 20 visibility links (0x54, 500 at 0x246c7a8; sum.symbols avoid_link_block_alloc / _free).
struct avoid_link_block {
    pf_vis_node *links[20];           // +0x00
    uint flags;                       // +0x50 bit 0 used
};
LAYOUT_CHECK(avoid_link_block, sizeof(avoid_link_block) == 0x54);
#define AVOID_NUM_LINK_BLOCKS 500
// A corner (0x44, segment list +0x270): starts with its point; the pf_vis_node at +0x18 points at `pos`
// (prohibit lists) or at `pushed` (pos moved 0.03 along the miter offset) (D3703).
struct avoid_corner {
    vector2 pos;                      // +0x00
    vector2 pushed;                   // +0x08
    vector2 offset;                   // +0x10 miter offset (avoid_seglist_calc_corner_offset)
    pf_vis_node node;                 // +0x18 links = an avoid_link_block
    int offset_state;                 // +0x40 -1 computed for one segment, then 1 = done (0 = not yet)
};
LAYOUT_CHECK(avoid_corner, sizeof(avoid_corner) == 0x44);
#define AVOID_SEGLIST_USED 0x1
#define AVOID_SEGLIST_DOOR 0x2        // a prohibit list: blocks instead of adding search nodes
                                      // (pf_add_avoidance_nodes; avoid_clip_segments sets it unless the
                                      // polygon has flag 4; name decomp32's)
#define AVOID_SEGLIST_AVOID 0x4       // a soft (cost) list
#define AVOID_SEGLIST_OFF  0x8        // superseded while a merged polygon is built; skipped by
                                      // pf_mark_nodes_visible_from_point
#define AVOID_SEGLIST_MAX  30
struct avoid_seglist {
    int _unk00;
    uint flags;                       // +0x04 AVOID_SEGLIST_*
    int poly;                         // +0x08 avoid_polys index (-1 from avoid_seglist_alloc)
    short num_segs;                   // +0x0c
    short num_corners;                // +0x0e
    int size;                         // +0x10
    int layer;                        // +0x14
    avoid_seg_ref segs[AVOID_SEGLIST_MAX];     // +0x18
    avoid_corner corners[AVOID_SEGLIST_MAX];   // +0x270 (static initialiser 0x4a67a0)
    rect2d bounds;                    // +0xa68
};
LAYOUT_CHECK(avoid_seglist, sizeof(avoid_seglist) == 0xa78);
#define AVOID_NUM_SEGLISTS 14
extern avoid_seglist avoid_seglists[AVOID_NUM_SEGLISTS];   // 0x2460db0
// The nav walls whose avoidance reference count (pf_edge::_pad0b) was raised (0x246b74c, count 0x246b748).
extern int avoid_backup_seg_count;                 // 0x246b748 (names ours)
extern pf_edge *avoid_backup_segs[100];            // 0x246b74c "ran out of backup segs"

// ---- Avoidance areas (8 bytes, 14 at 0x246b928) --------------------------------------------------------
struct avoid_area {
    short flags;                      // +0x00 bit 0 used
    short count;                      // +0x02
    char obstacles[4];                // +0x04 avoid_obstacles indices (one per size class pass)
};
LAYOUT_CHECK(avoid_area, sizeof(avoid_area) == 0x8);
#define AVOID_NUM_AREAS 14
extern avoid_area avoid_areas[AVOID_NUM_AREAS];    // 0x246b928 (name ours)

// ---- Avoidance splines (0xc, 20 at 0x2460cc0) and their points (0x24, 100 at 0x246b998) ----------------
// The description avoid_spline_create reads: a count and per point the two cell bytes and two 2D end
// points (avoid_spline_point_create(&p0[i], &p1[i], cell_x[i], cell_y[i]); decomp34, D1402). The cell
// bytes are the size class and the layer of the spline wall (they index avoid_spline_point_counts).
struct avoid_spline_def {
    int count;                        // +0x00
    int cell_x[8];                    // +0x04
    int cell_y[8];                    // +0x24
    vector2 p0[8];                    // +0x44
    vector2 p1[8];                    // +0x84
};
LAYOUT_CHECK(avoid_spline_def, sizeof(avoid_spline_def) == 0xc4);
struct avoid_spline {
    short flags;                      // +0x00 bit 0 used
    short count;                      // +0x02
    char points[8];                   // +0x04 avoid_spline_point_get indices
};
LAYOUT_CHECK(avoid_spline, sizeof(avoid_spline) == 0xc);
#define AVOID_NUM_SPLINES 20
// A spline point: a wall pf_sweep_segment tests like a pf_edge, between its two own end points.
struct avoid_spline_point {
    ushort flags;                     // +0x00 bit 0 used
    char size;                        // +0x02
    char layer;                       // +0x03
    pf_edge wall;                     // +0x04 a / b point at p0 / p1
    vector2 p0;                       // +0x14
    vector2 p1;                       // +0x1c
};
LAYOUT_CHECK(avoid_spline_point, sizeof(avoid_spline_point) == 0x24);
#define AVOID_NUM_SPLINE_POINTS 100
extern avoid_spline avoid_splines[AVOID_NUM_SPLINES];                 // 0x2460cc0
extern avoid_spline_point avoid_spline_points[AVOID_NUM_SPLINE_POINTS];   // 0x246b998 (name ours)
extern char avoid_spline_point_counts[PF_NUM_SIZES * PF_MAX_LAYERS];   // 0x246b8dc per size * 20 + layer

// ---- .data and the console command ----------------------------------------------------------------------
extern int avoidance_enabled;         // 0x59ce88 1
extern int avoidance_no_remove;       // 0x59ce8c 0
extern int avoidance_area_type;       // 0x59ce90 1 (rect), 0 (circle); only printed
extern int avoidance_prohibit;        // 0x59ce94 1; set by "prohibit", cleared by "avoid"; never read (name ours)
extern console_command ccmd_avoidance;   // 0x246b918

// ---- Functions (address order) -------------------------------------------------------------------------
void cmd_avoidance();                                                      // 0x4a6810
void avoid_clear_segment_flags();                                          // 0x4a6b40
bool avoid_box_contains_point(const vector2 *p, const avoid_obstacle *ob, float margin);      // 0x4a6b90
bool avoid_circle_contains_point(const vector2 *p, const avoid_obstacle *ob, float margin);   // 0x4a6bf0
bool avoid_box_contains_point_exact(const vector2 *p, const avoid_obstacle *ob);             // 0x4a6ce0
bool avoid_circle_contains_point_exact(const vector2 *p, const avoid_obstacle *ob);          // 0x4a6d00
bool avoid_prim_test(const vector2 *p, const avoid_obstacle *ob, float margin);              // 0x4a6d20
void avoid_prim_from_segment(avoid_prim *out, const vector2 *a, const vector2 *b);           // 0x4a6d60
bool avoid_prim_overlap(const avoid_obstacle *a, const avoid_prim *b, float margin);         // 0x4a6e00
bool avoid_segment_hits_prim(const avoid_obstacle *ob, const vector2 *a, const vector2 *b, float margin);   // 0x4a6f50
bool avoid_box_box_overlap(const avoid_obstacle *a, const avoid_obstacle *b);                // 0x4a6f90
// (box, other): any of other's outline points inside the box / circle (D3705: decomp32's comment had
// the order the other way round).
bool avoid_poly_points_in_box(const avoid_obstacle *box, const avoid_obstacle *other);       // 0x4a70e0
bool avoid_poly_points_in_circle(const avoid_obstacle *circle, const avoid_obstacle *other); // 0x4a7120
bool rect2d_overlap(rect2d a, rect2d b);               // 0x4a7160 (eight floats on the stack)
bool seg_params_in_range(const float *t, const float *u);                 // 0x4a71b0
bool avoid_poly_nearest_edge_hit(const vector2 *p, const vector2 *q, const avoid_poly *poly, int *edge,
                                 vector2 *hit);                            // 0x4a7210
bool avoid_poly_from_obstacle(avoid_poly *out, const avoid_obstacle *ob); // 0x4a7320
bool avoid_prim_test_exact(const vector2 *p, const avoid_obstacle *ob);   // 0x4a73d0
bool avoid_merge_obstacle_polys(avoid_poly *poly, const avoid_obstacle *ob);   // 0x4a73f0
avoid_link_block *avoid_link_block_alloc();                                       // 0x4a7720
void avoid_link_block_free(avoid_link_block *b);                                  // 0x4a7760
avoid_spline_point *avoid_spline_point_alloc();                            // 0x4a77a0
avoid_spline_point *avoid_spline_point_get(int i);     // 0x4a77d0 NULL unless used
avoid_spline *avoid_spline_alloc();                                        // 0x4a7800
avoid_spline *avoid_spline_get(int i);                 // 0x4a7830 NULL unless used
void avoid_spline_point_free(avoid_spline_point *pt);                      // 0x4a7860
void avoid_spline_point_free_index(int i);                                 // 0x4a7890
void avoid_spline_free(int index);                    // 0x4a78b0
int  avoid_spline_point_create(const vector2 *a, const vector2 *b, int size, int layer);   // 0x4a7900 index or -1
int  avoid_spline_create(avoid_spline_def *def);      // 0x4a7990 spline index (< 20) or -1
avoid_seglist *avoid_seglist_alloc();                                      // 0x4a7a30
avoid_poly *avoid_poly_alloc();                                            // 0x4a7a80
avoid_obstacle *avoid_obstacle_alloc();                                    // 0x4a7ac0
avoid_area *avoid_area_alloc();                                            // 0x4a7b00
void avoid_obstacle_free(avoid_obstacle *ob);                              // 0x4a7b30
void avoid_seglist_mark_free(avoid_seglist *sl);                           // 0x4a7b70
// Clears the first word of an area slot; also used on polygons (avoid_poly_release and the failure
// paths), so it takes the record as void * (D3704).
void avoid_area_free(void *slot);                                          // 0x4a7b80
void avoid_seglists_clear_refs();                      // 0x4a7b90
void avoid_seglist_free(avoid_seglist *sl);                                // 0x4a7bd0
void avoid_poly_release(avoid_poly *poly);                                 // 0x4a7c70
void avoid_area_rebuild(avoid_obstacle *ob);                               // 0x4a7cb0
void avoid_seglist_calc_corner_offset(avoid_seglist *sl, avoid_seg_ref *seg, const pf_edge *wall, int corner,
                                      bool swap);                          // 0x4a7e30
void avoid_seglist_calc_shared_corner_offset(avoid_seglist *sla, avoid_seglist *slb, avoid_seg_ref *sega,
                                             avoid_seg_ref *segb, int corner_a, int corner_b);   // 0x4a8060
void avoid_seglist_ensure_corner_offset(avoid_seglist *sl, int corner);   // 0x4a82a0
void avoid_do_poly_seg_list_links(avoid_seglist *sl);                     // 0x4a8390
void rect2d_add_point(rect2d *r, const vector2 *p);                       // 0x4a8600
int  avoid_seglist_add_segment(const vector2 *a, const vector2 *b, pf_edge *wall_a, pf_edge *wall_b,
                               avoid_seglist *sl);                         // 0x4a8680 0 or -1 (full)
bool ptr_array_contains(const void *value, void *const *array, int count);   // 0x4a89b0
// A reference to a segment of a segment list (0xc; avoid_merge_shared_segments' stack).
struct avoid_seg_ref_entry {
    float t;                          // +0x00 parameter along the wall (avoid_sort_refs_along_segment)
    avoid_seglist *list;              // +0x04
    int index;                        // +0x08 segs index
};
void avoid_collect_segment_refs(int *count, avoid_seg_ref_entry *refs, const pf_edge *wall, int size, int layer);   // 0x4a89e0
int  qsort_cmp_float_asc(const void *a, const void *b);                   // 0x4a8a60
avoid_seg_ref *avoid_ref_entry(const avoid_seg_ref_entry *e);             // 0x4a8a80
void avoid_sort_refs_along_segment(avoid_seg_ref_entry *refs, int count); // 0x4a8aa0
void avoid_merge_shared_segments(avoid_seglist *sl);                      // 0x4a8b80
int  avoid_clip_segments(avoid_poly *poly, bool link);                    // 0x4a8df0 seglist index or -1
int  avoid_qsort_cmp_poly_word(const void *a, const void *b);             // 0x4a93c0
void avoid_age_segments();                                                // 0x4a9400
bool avoid_create_obstacle_polys(avoid_obstacle *ob, bool link);          // 0x4a9440
int  avoid_area_add(avoid_area_desc *d);               // 0x4a9b40 index ("AA - primative not added!!")
void avoid_reset();                                    // 0x4a9d60
void avoid_area_remove(int slot);                                         // 0x4a9df0
bool avoid_obstacles_overlap(const avoid_obstacle *a, const avoid_obstacle *b);   // 0x4a9e60
// (centre, AVOID_* shape and kind bits of avoid_area_add: 1 box, 2 circle, 4 prohibit, 8 avoid, half
// axis a, half axis b, size class, layer): obstacle index or -1.
int  avoid_add_primitive(const vector2 *centre, int flags, const vector2 *a, const vector2 *b, int size,
                         int layer);                                       // 0x4a9eb0
avoid_poly *avoid_poly_get(avoid_seglist *s);          // 0x4aa3e0
// Returns the extra path cost of going round the obstacle (pf_add_avoidance_nodes adds it).
float avoid_poly_add_search_nodes(const avoid_prim *seg, const avoid_obstacle *ob);   // 0x4aa400
// avoid_point_blocked (0x4aa630) is declared in engine/ai/pathfinding.h.
bool avoid_area_contains_point(const vector3 *pos, int layer, int size, int area);   // 0x4aa6b0
vector2 *avoid_record_axis_at(avoid_seglist *sl, int corner);   // 0x4aa760 &corners[corner].offset

#endif
