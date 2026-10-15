// engine/ai/pathfinding_visibility.h - point visibility over the nav graph (engine\ai\pathfinding_visibility.cpp,
// assert path 0x59d7d8; code 0x4b77f0-0x4b87bd, .data 0x59d7d8-0x59d933, .bss 0x247e968-0x247fcef). Not
// original source. decomp32 declared determine_point_visibility; decomp47 wrote the unit and added the rest.
//
// A rotational sweep round the query point: sort_points_clockwise sorts the graph points of the current size
// class and layer by direction ("sort_points_clockwise()" in the timing report), make_initial_edge_tree
// collects per layer the cell edges the ray straight down (-y) from the point crosses, nearest first
// ("make_initial_edge_tree()"), and determine_point_visibility walks the sorted points, testing each against
// the nearest edge of its layer ("do_single_vis_check()", do_single_vis_check) and swapping the edges
// that meet at the point in or out ("add_or_remove_edge()", add_or_remove_edge). The quoted names are the
// original function names from the report strings; the pathvis_* names are sum.symbols'.
#ifndef ENGINE_AI_PATHFINDING_VISIBILITY_H
#define ENGINE_AI_PATHFINDING_VISIBILITY_H

#include "vsdk/vsdk.h"
#include "engine/ai/pathfinding.h"

class vector2;

#define PATHVIS_MAX_EDGES   100       // edge node pool ("Edge tree node limit of %d reached")
#define PATHVIS_MAX_PENDING 50        // points on one sweep ray ("Too many points on same sweep line")

// One edge of a sweep list (0x28; pool pathvis_edge_pool, list heads are whole records). a / b are the
// pf_edge end points (make_initial_edge_tree stores the end with the larger x in a; add_or_remove_edge
// the end at the swept point). Type and field names ours.
struct pathvis_edge : public seg2d {
    int _unk08;                       // +0x08 never written
    int _unk0c;                       // +0x0c
    float dist;                       // +0x10 list order: distance below the point (initial lists) or length
    float dx;                         // +0x14 (a - b) / dist
    float dy;                         // +0x18
    int tag;                          // +0x1c pathvis_edge_tag (always 0)
    pathvis_edge *prev;               // +0x20
    pathvis_edge *next;               // +0x24
};
LAYOUT_CHECK(pathvis_edge, sizeof(pathvis_edge) == 0x28);

// .bss (address order; names ours except pathvis_segment_cycles)
extern pathvis_edge pathvis_edge_lists[PF_MAX_LAYERS];  // 0x247e968 one sorted edge list per layer
extern int pathvis_edge_cycles;                         // 0x247ec88 read_tsc total of the edge updates
extern bool pathvis_ray_clear;                          // 0x247ecdc the ray to the current point is still open
extern pathvis_edge *pathvis_pending_edge;              // 0x247ecf0 first edge of a point, placed by side
extern int pathvis_tree_cycles;                         // 0x247ecf4 read_tsc of make_initial_edge_tree
extern int pathvis_edge_tag;                            // 0x247ecf8 never written
extern pathvis_edge pathvis_edge_pool[PATHVIS_MAX_EDGES];   // 0x247ed00
extern int pathvis_sort_cycles;                         // 0x247fca0 read_tsc of sort_points_clockwise
extern vector2 *pathvis_origin;                         // 0x247fca8 the query point
extern vector2 *pathvis_target;                         // 0x247fcac the point being swept
extern int pathvis_segment_cycles;                      // 0x247fcb8
extern pathvis_edge pathvis_free_edges;                 // 0x247fcc0 free list head
// 0x247fce8: never written (false). When set, make_initial_edge_tree would walk a cell's edges as a list
// (link at +0x14) and a 51st point on one sweep ray would be fatal (DebugError line 970).
extern bool pathvis_alt_mode;                           // 0x247fce8
extern int pathvis_edge_pool_inited;                    // 0x247fcec

// Returns the graph points (graph pf_cur_size, layer pf_cur_layer) as one list through next_open, by
// direction from pt; PFN_COLLINEAR marks a point on the same ray as the one before it.
pf_vis_node *sort_points_clockwise(const vector2 *pt);             // 0x4b77f0
void make_initial_edge_tree(const vector2 *pt);               // 0x4b7af0
pathvis_edge *pathvis_add_edge_node();                          // 0x4b7d30 from the free list
void pathvis_edge_pool_init();                                  // 0x4b7da0 once
void pathvis_insert_sorted(pathvis_edge *e, int layer);         // 0x4b7df0 by dist (D2602)
// Adds the nav edge (one of the two meeting at pos) to the layer's list, or removes it when it is there.
void add_or_remove_edge(pf_edge *edge, int layer, vector2 *pos);   // 0x4b7e80 (compiled unoptimised)
void pathvis_free_edge_node(pathvis_edge *e);                   // 0x4b8150
void pathvis_insert_by_side(pathvis_edge *e, int layer);        // 0x4b8190 (D2602)
// 1 when the walk from `from` to `to` is blocked (always while pathvis_ray_clear is false), 0 when clear;
// all: test every edge of the layer, else only the nearest (D2603).
int  do_single_vis_check(pf_vis_node *from, pf_vis_node *to, bool all);   // 0x4b8230
bool pathvis_bsp_contains(const vector2 *pt, pf_bsp *node);     // 0x4b86e0 (D2602)

// Marks the graph nodes of the current size class that the point sees (PFN_VISIBLE), with the
// edge-list sweep; prints "determine_point_visibility() took %d ms.".
void determine_point_visibility(const vector2 *pt, int layer);   // 0x4b8350

#endif
