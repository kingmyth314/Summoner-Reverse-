// engine/ai/pathfinding.h - navigation mesh queries and AI paths (Engine\ai\pathfinding.cpp, the path
// reference that gives engine/ai/ its directory; pf_* / ai_path_* in sum.symbols). Not original source.
// decomp32 wrote pathfinding.cpp (0x49c840-0x4a04af) and extended this header with the .pfg graph types
// (layouts from pathfinding_load_graph and its readers) and the geometry unit that follows boss_ai.cpp
// (0x4a4ee0-0x4a6600: graph globals, BSP cell lookup, grid hash, seg2d tests; file not known). Type and
// field names are ours unless sum.symbols names them.
#ifndef ENGINE_AI_PATHFINDING_H
#define ENGINE_AI_PATHFINDING_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"

class File;

#define PFG_VERSION      0x6d          // pathfinding_load_graph accepts exactly 109
#define PF_NUM_SIZES     3             // character size classes (char_info +0x90)
#define PF_MAX_LAYERS    20
#define AI_PATH_MAX_NODES 1000         // ai_path_nodes 0x2452d58..0x245c998, 0x28 bytes each

// ---- 2D primitives ----------------------------------------------------------------------------------
// A segment as two point pointers (seg2d_segment_intersect, pf_first_edge_crossing take one).
struct seg2d {
    vector2 *a;                       // +0x00
    vector2 *b;                       // +0x04
};
// 2D bounds in the order the .pfg cells store them (cell +0x0c..+0x18; the reader fills min x, max x,
// max y, min y in that order). y is the world z. rect2d_overlap takes two by value; pf_bounds is one
// (D1200).
struct rect2d {
    float max_y;                      // +0x00
    float min_y;                      // +0x04
    float min_x;                      // +0x08
    float max_x;                      // +0x0c
};

// A nav wall / cell edge (0x10, pf_edges): pf_read_edges reads two node indices and two uint16.
#define PF_EDGE_SIDE 0x1              // the blocked side is the other one (normal negated)
#define PF_EDGE_DOOR 0x2              // belongs to a door: ignored while pf_door_index_is_open
struct pf_edge : public seg2d {
    ushort flags;                     // +0x08 PF_EDGE_*
    ubyte door;                       // +0x0a door index (out of range counts as open)
    ubyte _pad0b;                     // +0x0b 0 on load; avoidance reference count (avoid_clip_segments raises it
                                      //        with flag 0x20, avoid_age_segments / avoid_seglist_free lower it)
    int search_id;                    // +0x0c pf_next_search_id stamp (each wall is tested once)
};
// A layer-transition edge (0x14, pf_walls; sum.symbols name): pf_layer_links gets both directions.
// decomp57 (D3700): derives from seg2d; pf_layer_from_edges passes the record as the segment.
struct pf_wall : public seg2d {
    int layer_a;                      // +0x08
    int layer_b;                      // +0x0c
    pf_wall *next;                    // +0x10 next record of the array, NULL for the last
};
LAYOUT_CHECK(pf_wall, sizeof(pf_wall) == 0x14);

// A visibility-graph node (0x28; boss_ai.cpp's pf_point_ref, D1200). The first 0x18 bytes come from the
// .pfg file, +0x18..+0x27 are pf_graph_search's A* state. The avoidance corners (avoid_corner, ai.h)
// start with the same fields.
#define PFN_VISIBLE      0x0001       // straight walk from the query point clear
#define PFN_COLLINEAR    0x0002       // sort_points_clockwise: same direction from the point as the node before
                                      // it in the sweep (name ours, decomp47)
#define PFN_FROM_START   0x0004       // visible from the search start
#define PFN_FROM_GOAL    0x0008       // visible from the search goal
#define PFN_IN_REACH     0x0010       // inside the query box and on a reachable layer
#define PFN_LAYER_TESTED 0x0020
#define PFN_WALK_TESTED  0x0040
#define PFN_NO_EDGES     0x0080       // .pfg >= 0x6c: no wall edges recorded
#define PFN_SKIP_DOOR_LISTS 0x0200    // pf_add_avoidance_nodes skips the door-type avoidance lists
struct pf_vis_node {
    vector2 *pos;                     // +0x00 point (pf_nodes)
    ushort flags;                     // +0x04 PFN_*
    short layer;                      // +0x06
    short num_links;                  // +0x08
    short _pad0a;
    pf_vis_node **links;              // +0x0c
    pf_edge *edges[2];                // +0x10 the wall edges meeting at the point
    pf_vis_node *next_open;           // +0x18 open list, sorted by f
    pf_vis_node *parent;              // +0x1c came-from (non-NULL = reached)
    float g;                          // +0x20 cost from the start
    float f;                          // +0x24 g + straight distance to the goal
};
LAYOUT_CHECK(pf_vis_node, sizeof(pf_vis_node) == 0x28);
typedef pf_vis_node pf_point_ref;     // decomp29's name (boss_mount_pick_flee_point)

// A convex nav cell (0x20): its edges and bounds; the cells of a graph form a list.
struct pf_cell {
    int num_edges;                    // +0x00
    pf_edge *edges;                   // +0x04 pf_read_edges
    int _unk08;
    rect2d bounds;                    // +0x0c
    pf_cell *next;                    // +0x1c
};
LAYOUT_CHECK(pf_cell, sizeof(pf_cell) == 0x20);

// A navigation BSP node (0x10, pf_read_typed_refs). Type 1 splits at the x of a pf_nodes point (data;
// front: x below it), 2 splits by the line of a pf_edge (data; front: the side with the cross product <= 0),
// 3 empty (the file's index is dropped). pf_bsp_node_find and pathvis_bsp_contains test both sides of a
// point within 0.0001 of the split (decomp47, D2602; type 1 was read as a leaf before).
struct pf_bsp {
    int type;                         // +0x00
    void *data;                       // +0x04
    pf_bsp *front;                    // +0x08
    pf_bsp *back;                     // +0x0c
};
LAYOUT_CHECK(pf_bsp, sizeof(pf_bsp) == 0x10);

// The wall hash grid of a graph (graph +0x24, pf_read_region).
struct pf_grid_cell {                 // 0x0c (pf_read_edge_list)
    int _unk00;
    ushort num_walls;                 // +0x04
    ushort _pad06;
    pf_edge **walls;                  // +0x08
};
LAYOUT_CHECK(pf_grid_cell, sizeof(pf_grid_cell) == 0xc);
struct pf_grid_row {                  // 0x08
    ushort num_cells;                 // +0x00
    ushort _pad02;                    // +0x02 0
    pf_grid_cell *cells;              // +0x04
};
LAYOUT_CHECK(pf_grid_row, sizeof(pf_grid_row) == 0x8);
// decomp57: bounds[0] / bounds[2] the grid origin x / z, bounds[4] / bounds[5] the cell sizes, _unk18 /
// _unk1c the width / height in cells, num_rows the bucket count of the hash (pf_grid_hash_find).
struct pf_grid {                      // 0x2c
    float bounds[6];                  // +0x00
    int _unk18;                       // +0x18
    int _unk1c;                       // +0x1c
    int num_rows;                     // +0x20 pf_cells_along_segment tests it (graph +0x44)
    pf_grid_row *rows;                // +0x24
    int _unk28;                       // +0x28
};
LAYOUT_CHECK(pf_grid, sizeof(pf_grid) == 0x2c);

// One graph per size class and layer (0x60, pf_graphs[3][20] at 0x245f460; decomp29's pf_point_list,
// whose field names count / points are kept, D1200).
struct pf_graph {
    int count;                        // +0x00 visibility nodes
    int count_b;                      // +0x04 second node list (linked among itself)
    int _unk08;
    int num_bsp;                      // +0x0c
    pf_vis_node *points;              // +0x10
    pf_vis_node *points_b;            // +0x14
    int _unk18;
    pf_bsp *bsp;                      // +0x1c root (pf_bsp_find_cell)
    pf_cell *cells;                   // +0x20
    pf_grid grid;                     // +0x24
    ubyte _unk50[0x10];
};
LAYOUT_CHECK(pf_graph, sizeof(pf_graph) == 0x60);
typedef pf_graph pf_point_list;       // decomp29's name
struct pf_layer_list {                // 0x245f3b8, one per layer
    int count;
    int *list;
};

// The swept move of pf_slide_move (0x48 on its stack).
struct pf_sweep {
    vector2 from;                     // +0x00
    vector2 to;                       // +0x08
    vector2 hit;                      // +0x10 nearest hit point (pf_sweep_segment)
    int size;                         // +0x18
    int layer;                        // +0x1c
    int search_id;                    // +0x20
    pf_edge *wall;                    // +0x24 wall under test
    pf_edge hit_wall;                 // +0x28 copy of the nearest wall
    float t;                          // +0x38 move parameter of the last hit
    int hit_kind;                     // +0x3c 0 wall, 1 wall end point (2: pf_sweep_segment's "none")
    vector2 *hit_corner;              // +0x40 end point of the last corner hit
    vector2 *last_corner;             // +0x44 corner of the nearest hit, skipped by the next test
};
LAYOUT_CHECK(pf_sweep, sizeof(pf_sweep) == 0x48);

// One edge of the A* search (pf_graph_search's stack, 0x14), filled by pf_add_avoidance_nodes.
struct pf_search_seg {
    pf_vis_node *a;                   // +0x00
    pf_vis_node *b;                   // +0x04
    bool blocked;                     // +0x08 a door-type avoidance list blocks it
    int size;                         // +0x0c
    float cost;                       // +0x10 extra cost round the obstacles
};
LAYOUT_CHECK(pf_search_seg, sizeof(pf_search_seg) == 0x14);

// An AI path node (0x28, pool ai_path_nodes). A path is the list from the first step to the goal.
struct ai_path {
    vector3 pos;                      // +0x00
    int layer;                        // +0x0c
    vector3 dir;                      // +0x10 normalised x / z step from the previous node (ai_path_find)
    float cost;                       // +0x1c cumulative length
    int flags;                        // +0x20 bit 1: keep (ai_path_smooth does not drop past it)
    ai_path *next;                    // +0x24 (also the free-list link)
};
LAYOUT_CHECK(ai_path, sizeof(ai_path) == 0x28);

// ---- pathfinding.cpp (0x49c840-0x4a04af) ---------------------------------------------------------------
int  pathfinding_load_graph(const char *level_name);                       // 0x49cdd0 File::close result
void ai_path_pool_init();                                                  // 0x49d930
int  pf_next_search_id();                                                  // 0x49d970
void pathfinding_init();                                                   // 0x49d980
void pathfinding_free_graph();                                             // 0x49d9f0
void seg2d_closest_point(vector2 *p, const seg2d *s, vector2 *out);        // 0x49db20
bool pf_door_index_is_open(int door, float fraction);                      // 0x49dbe0
// (from, to, out, size, layer): false when the move is clear (out not written), else out = the slid
// or the hit point, y from `from`.
bool pf_slide_move(const vector3 *from, const vector3 *to, vector3 *out, int size, int layer);   // 0x49e470
// (from, to, layer, size, max length): true when a closed door wall blocks the straight move.
bool pf_check_direct_walk(const vector3 *from, const vector3 *to, int layer, int size,
                          const float *max_len);                           // 0x49e620
// Moves *pos to the closest navigable point for a character of `size` (char_info +0x90) on `layer`;
// true when it had to move it.
bool pf_find_nearest_navigable_point(vector3 *pos, int size, int layer);   // 0x49e8c0
bool pf_find_cell_crossing(vector2 *out, vector2 *from, vector2 *to);      // 0x49ecb0
int  pf_mark_nodes_visible_from_point(const vector2 *pt, int layer, int walk_flags);   // 0x49ee80
void pf_compute_endpoint_visibility(const vector2 *start, int start_layer, const vector2 *end,
                                    int end_layer);                        // 0x49f340
ai_path *ai_path_smooth(ai_path *path, const vector3 *start, int start_layer);   // 0x49f650
bool pf_add_avoidance_nodes(pf_search_seg *s);                             // 0x49f810
// to is moved onto the mesh when it lies outside every cell (pf_find_cell_crossing).
ai_path *pf_graph_search(const vector3 *from, int from_layer, vector3 *to, int to_layer, int size,
                         float cost_bias);                                 // 0x49fa70
// cdecl, 6 arguments (add esp, 0x18 at 0x4da703): NULL when no path. The last argument is a float
// (pf_graph_search negates it into the goal node's cost; D1202).
ai_path *ai_path_find(const vector3 *from, int from_layer, vector3 *to, int to_layer, int size,
                      float cost_bias);                                    // 0x4a01d0
ai_path *ai_path_alloc_node();                                             // 0x4a02d0
ai_path *ai_path_node_create_b(vector3 pos, int layer, ai_path *next);     // 0x4a0330
ai_path *ai_path_node_create(vector2 pos, int layer, ai_path *next);       // 0x4a03c0
void ai_path_free_node(ai_path *node);                                     // 0x4a0440
ai_path *ai_path_pop_node(ai_path *node);                                  // 0x4a0460
void ai_path_free(ai_path *path);                                          // 0x4a0480
int  pf_node_index(const vector2 *node);                                   // 0x4a04a0

// pathfinding.cpp .bss (0x2452cb8-0x245c9df)
extern uint pf_layer_links[PF_MAX_LAYERS];    // 0x2452cb8 bit l: layer l is reachable
extern pf_vis_node *pf_search_start_nodes[10];   // 0x2452d08
extern ai_path *ai_path_free_list;            // 0x2452d30
extern int ai_path_peak_used;                 // 0x2452d34
extern int pf_search_iterations;              // 0x2452d38
extern int ai_path_used;                      // 0x2452d4c
extern rect2d pf_bounds;                      // 0x245c998 (D1200)
#define pf_bounds_max_z (pf_bounds.max_y)     // 0x245c998 sum.symbols names (rect y is world z, D1200)
#define pf_bounds_min_z (pf_bounds.min_y)     // 0x245c99c
#define pf_bounds_min_x (pf_bounds.min_x)     // 0x245c9a0
#define pf_bounds_max_x (pf_bounds.max_x)     // 0x245c9a4
extern int pf_search_start_count;             // 0x245c9ac
extern bool Precalc_vis_method;               // 0x245c9d8

// ---- pathfinding geometry (0x4a4ee0-0x4a670f, file not known; .bss 0x245f3a0-0x2460cbb) ---------------
// decomp57: written as game/unnamed_pfgeom_0x4a4ee0.cpp (placeholder name); the declarations below are in
// address order where decomp57 added them.
void pf_select_graph(int size, int layer);                                 // 0x4a4f30
// (seg, layer, out point, out layer): nearest crossing of seg with a pf_walls edge leaving `layer`.
bool pf_first_edge_crossing(const seg2d *seg, int layer, vector2 *hit, int *hit_layer);   // 0x4a53d0
// Follows a move from *pos to dest across the layer-transition edges; returns the layer at dest.
// cdecl, (pos, layer, dest by value): add esp, 0x14 at 0x4da348 (unit not tagged in sum.symbols).
int  pf_trace_edge_crossings(const vector3 *pos, int layer, vector3 dest);  // 0x4a54b0
// (a, a layer, b, b layer, flags): ai_path_smooth passes the layers of the two points (D1201).
bool pf_straight_walk_clear(const vector2 *a, int a_layer, const vector2 *b, int b_layer, int flags);   // 0x4a55f0
int  seg2d_segment_intersect(const seg2d *a, const seg2d *b, vector2 *out);   // 0x4a5750 class, > 0 hit
// decomp41 (level_scripts_common.cpp): (point, segment, 0) the 2D cross product of (b - a) and
// (point - a) above the epsilon 0x57eb84; (a, b, c, d) the x / z segments a-b and c-d cross
// (seg2d_segment_intersect == 1).
bool seg2d_point_left_of(const vector2 *p, const seg2d *s, int flags);        // 0x4a4f60
// decomp47 (pathfinding_visibility.cpp): y0 + (x - x0) * (y1 - y0) / (x1 - x0).
float float_lerp_range(float x, float x0, float y0, float x1, float y1);      // 0x4a4f00
bool segments_intersect_xz(const vector3 *a, const vector3 *b, const vector3 *c, const vector3 *d);   // 0x4a5570
// (segment, line, out, bias): parameter along the segment + bias in seg2d_param (denominator seg2d_denom).
// decomp57 (D3708): the fourth argument is a float added to the parameter (fadd [esp+0x10] at 0x4a5c28),
// and nothing is returned (EAX only holds `out` for the stores).
void seg2d_line_intersect(const seg2d *seg, const seg2d *line, vector2 *out, float bias);   // 0x4a5bd0
pf_bsp *pf_bsp_find_cell(const vector3 *pos, int layer);                   // 0x4a5d40 layer -1 = current
bool pf_point_is_navigable(const vector3 *pos, int size, int layer);       // 0x4a5dc0
// (pos, layer, size class): the x/z point lies in a used obstacle of the avoidance table (unit after
// pathfinding; decomp35, world_click_move).
bool avoid_point_blocked(const vector3 *pos, int layer, int size);  // 0x4aa630
// (a, b, c, d, out t on a-b, out u on c-d).
bool seg2d_intersect_params(const vector2 *a, const vector2 *b, const vector2 *c, const vector2 *d,
                            float *t, float *u);                           // 0x4a5eb0
int  pf_cells_along_segment(int layer, int size, const vector2 *a, const vector2 *b);   // 0x4a64b0 count
int  pf_layer_from_edges(int layer_a, int layer_b, const vector3 *pos);    // 0x4a6600
// decomp57: the rest of the unit.
// (a, b, layer or -1, flags): true when the 2D segment a-b crosses no nav wall of the selected graph
// (flags 1) and no prohibit (flags 2) / avoid (flags 4) obstacle of the size and layer (D3707: the name
// says the opposite of the result).
bool pf_segment_hits_walls(const vector2 *a, const vector2 *b, int layer, int flags);   // 0x4a4fb0
pf_bsp *pf_bsp_node_find(pf_bsp *node, const vector2 *pt);                 // 0x4a5c60
int  grid_hash_bucket(int x, int y, int w, int n);                         // 0x4a5e00
int  grid_linear_index(int x, int y, int w);                               // 0x4a5e20
pf_grid_cell *pf_grid_hash_find(int x, int y, pf_grid *grid);              // 0x4a5e30
int  pf_grid_cells_in_box(const vector2 *pt, float half, pf_grid *grid);   // 0x4a5f70 count
int  pf_grid_cells_along_segment(const vector2 *a, const vector2 *b, pf_grid *grid);   // 0x4a6060 count
int  pf_cells_in_box(int layer, int size, const vector2 *pt, float half);  // 0x4a64f0 count
bool seg2d_point_near_middle(const vector2 *pt, const seg2d *s, float dist);   // 0x4a6530

extern int pf_cell_pool_remaining;            // 0x245f3a0
extern float seg2d_denom;                     // 0x245f3a4
extern float seg2d_cross;                     // 0x245f3a8
extern float seg2d_param;                     // 0x245f3ac
extern int pf_cur_layer;                      // 0x245f3b0
extern int pf_vis_node_pool_remaining;        // 0x245f3b4
extern pf_layer_list pf_layer_lists[PF_MAX_LAYERS];   // 0x245f3b8
extern int pf_cur_size;                       // 0x245f458
extern pf_graph pf_graphs[PF_NUM_SIZES][PF_MAX_LAYERS];   // 0x245f460
#define pf_point_lists pf_graphs              // decomp29's name
extern int pf_dword_pool_remaining;           // 0x2460ae0
extern pf_graph *pf_cur_graph;                // 0x2460ae4
extern int pf_row_pool_remaining;             // 0x2460ae8
extern int pf_link_pool_remaining;            // 0x2460aec
extern int pf_bsp_pool_remaining;             // 0x2460af0
extern int pf_region_pool_remaining;          // 0x2460af4 grid cells (12 bytes)
extern int pf_word_pool_remaining;            // 0x2460af8
extern pf_grid_cell *pf_grid_cell_results[100];   // 0x2460afc
extern int pf_edge_ref_pool_remaining;        // 0x2460c8c
extern int pf_unused_count_c90;               // 0x2460c90 read from the .pfg, never used
extern int pf_grid_cell_count;                // 0x2460c94
extern int pf_node_count;                     // 0x2460c98
extern int pf_edge_count;                     // 0x2460c9c
extern int pf_border_edge_count;              // 0x2460ca0 the .pfg's first edge list
extern int pf_wall_count;                     // 0x2460ca4
extern vector2 *pf_nodes;                     // 0x2460ca8
extern void *pf_unk_cac;                      // 0x2460cac cleared by pathfinding_init only
extern pf_edge *pf_edges;                     // 0x2460cb0
extern pf_edge *pf_border_edges;              // 0x2460cb4
extern pf_wall *pf_walls;                     // 0x2460cb8

// Debug hooks of other units: a path freed at exit and referenced nowhere else (0x245cd8c, .bss between
// the handle-slot and obstacle-debug units), a .data int compared with 7 (0x59cd8c) and the per-step
// search hook, an empty function folded into empty_function (0x4f13e0; D907 style, no address).
extern ai_path *pf_debug_path;                // 0x245cd8c
extern int pf_debug_mode;                     // 0x59cd8c
void pf_debug_search_step(float sx, float sy, float gx, float gy, pf_vis_node *to, pf_vis_node *from,
                          int unused, float range);

#endif
