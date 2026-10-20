// unnamed_pfgeom_0x4a4ee0.cpp - Sum.exe v1.40, code 0x4a4ee0-0x4a670f: the pathfinding geometry unit
// (selected graph, nav-wall and obstacle segment tests, layer-transition edges, 2D segment intersection,
// BSP cell lookup, the wall hash grid). FILE NAME NOT KNOWN: a placeholder (no path string, no
// strings.tbl block; docs/sum-exe/source-map.md "Units without a file name", hand cut 0x4a4ee0-0x4a6600).
// Written from the Sum.exe disassembly (tools/asm.mjs; floats and the jump table read from Sum.exe).
// Target: VC++ 6.0 (cl 12.00.8966). Optimised (no frame pointer except pf_grid_cells_along_segment's
// aligned frame), but the small helpers are called, not expanded: grid_hash_bucket / grid_linear_index
// and vector2::from_xz are calls, so the unit was built without inline expansion of non-inline
// functions (/Ob1, like the others).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: vector2::from_xz (0x4a4ee0, the first COMDAT this unit uses) to pf_layer_from_edges
// (0x4a6600-0x4a6700); the avoidance unit's initialisers start at 0x4a6710. The shadow / obstacle-debug
// unit ends with vector2::to_vector3 (0x4a4ec0) before it.
// Profiling: pf_segment_hits_walls, pf_trace_edge_crossings and pf_bsp_find_cell open an EH frame whose
// only unwind action is the empty destructor 0x434650 (FuncInfo 0x584170 / 0x584198 and the next one):
// a profile_label local as in pathfinding.cpp; the label texts are not in the binary. The local segments
// are 0x10-byte pf_edge records (the frames have room for two more dwords per segment, D6402).
//
// Data: no .data; .bss 0x245f3a0-0x2460cbb (defined here, D1203; pathfinding.cpp uses most of it).
// Floats: 0 (0x57eb84), 0.5 (0x57e6b0), 1 (0x57e990), +-0.0001 (0x581930 / 0x581b74), +-0.003
// (0x581b78 / 0x581b7c), 1e-6 (0x581b80), 3.0 pushed as 0x40400000, 9.9e10 (0x51b866e1, an immediate).

#include <math.h>
#include <float.h>
#include "engine/ai/pathfinding.h"
#include "engine/ai/ai.h"
#include "vsdk/os/profile.h"
#include "vsdk/math/vector.h"

// ---- .bss (0x245f3a0-0x2460cbb), address order -----------------------------------------------------
int pf_cell_pool_remaining;                   // 0x245f3a0
float seg2d_denom;                            // 0x245f3a4
float seg2d_cross;                            // 0x245f3a8
float seg2d_param;                            // 0x245f3ac
int pf_cur_layer;                             // 0x245f3b0
int pf_vis_node_pool_remaining;               // 0x245f3b4
pf_layer_list pf_layer_lists[PF_MAX_LAYERS];  // 0x245f3b8
int pf_cur_size;                              // 0x245f458
pf_graph pf_graphs[PF_NUM_SIZES][PF_MAX_LAYERS];   // 0x245f460
int pf_dword_pool_remaining;                  // 0x2460ae0
pf_graph *pf_cur_graph;                       // 0x2460ae4
int pf_row_pool_remaining;                    // 0x2460ae8
int pf_link_pool_remaining;                   // 0x2460aec
int pf_bsp_pool_remaining;                    // 0x2460af0
int pf_region_pool_remaining;                 // 0x2460af4
int pf_word_pool_remaining;                   // 0x2460af8
pf_grid_cell *pf_grid_cell_results[100];      // 0x2460afc
int pf_edge_ref_pool_remaining;               // 0x2460c8c
int pf_unused_count_c90;                      // 0x2460c90
int pf_grid_cell_count;                       // 0x2460c94
int pf_node_count;                            // 0x2460c98
int pf_edge_count;                            // 0x2460c9c
int pf_border_edge_count;                     // 0x2460ca0
int pf_wall_count;                            // 0x2460ca4
vector2 *pf_nodes;                            // 0x2460ca8
void *pf_unk_cac;                             // 0x2460cac
pf_edge *pf_edges;                            // 0x2460cb0
pf_edge *pf_border_edges;                     // 0x2460cb4
pf_wall *pf_walls;                            // 0x2460cb8

// 0x4a4ee0  Confidence: exact. COMDAT of the inline member (thiscall, ret 4).
void vector2::from_xz(const vector3 *v)
{
    x = v->x;
    y = v->z;
}

// 0x4a4f00  Confidence: exact. Caller make_initial_edge_tree (pathfinding_visibility.cpp).
float float_lerp_range(float x, float x0, float y0, float x1, float y1)
{
    return (y1 - y0) / (x1 - x0) * (x - x0) + y0;
}

// 0x4a4f30  Confidence: exact.
void pf_select_graph(int size, int layer)
{
    pf_cur_size = size;
    pf_cur_layer = layer;
    pf_cur_graph = &pf_graphs[size][layer];
}

// 0x4a4f60  Confidence: exact (D11400: the edge normal as a vector2 built first, then the dot product). The flags argument is not read.
bool seg2d_point_left_of(const vector2 *p, const seg2d *s, int flags)
{
    vector2 n(s->b->y - s->a->y, s->a->x - s->b->x);

    return (p->x - s->a->x) * n.x + (p->y - s->a->y) * n.y > 0.0f;
}

// 0x4a4fb0  Confidence: likely (the four corner sides are or-ed as 1 / 2; the obstacle counter is
// raised before the bounds test, as the code does). True when the segment a-b is clear: flags 1 tests
// the nav walls of every cell of the selected graph whose bounds meet the segment's and whose corners
// lie on both sides of it; flags 2 / 4 the prohibit / avoid obstacles of the current size and layer.
bool pf_segment_hits_walls(const vector2 *a, const vector2 *b, int layer, int flags)
{
    profile_label label("pf_segment_hits_walls");
    pf_edge seg;
    vector2 c;
    pf_cell *cell;
    pf_edge *e;
    avoid_obstacle *ob;
    float min_x, max_x, min_y, max_y;
    float e_min_x, e_max_x, e_min_y, e_max_y;
    int prohibit, avoid, n, found, sides, i, r;

    seg.a = (vector2 *)a;
    seg.b = (vector2 *)b;
    if (layer == -1)
        layer = pf_cur_layer;
    min_x = a->x < b->x ? a->x : b->x;
    max_x = a->x > b->x ? a->x : b->x;
    min_y = a->y < b->y ? a->y : b->y;
    max_y = a->y > b->y ? a->y : b->y;
    if (flags & 1) {
        for (cell = pf_graphs[pf_cur_size][layer].cells; cell; cell = cell->next) {
            if (!(min_x < cell->bounds.max_x) || !(max_x > cell->bounds.min_x)
                || !(min_y < cell->bounds.max_y) || !(max_y > cell->bounds.min_y))
                continue;
            c.x = cell->bounds.min_x;
            c.y = cell->bounds.max_y;
            sides = seg2d_point_left_of(&c, &seg, 0) ? 1 : 2;
            c.x = cell->bounds.max_x;
            c.y = cell->bounds.max_y;
            sides |= seg2d_point_left_of(&c, &seg, 0) ? 1 : 2;
            c.x = cell->bounds.min_x;
            c.y = cell->bounds.min_y;
            sides |= seg2d_point_left_of(&c, &seg, 0) ? 1 : 2;
            c.x = cell->bounds.max_x;
            c.y = cell->bounds.min_y;
            sides |= seg2d_point_left_of(&c, &seg, 0) ? 1 : 2;
            if (sides != 3)
                continue;
            e = cell->edges;
            for (i = 0; i < cell->num_edges; i++, e++) {
                e_min_x = e->a->x < e->b->x ? e->a->x : e->b->x;
                e_max_x = e->a->x > e->b->x ? e->a->x : e->b->x;
                e_min_y = e->a->y < e->b->y ? e->a->y : e->b->y;
                e_max_y = e->a->y > e->b->y ? e->a->y : e->b->y;
                if (e_min_x < max_x && e_max_x > min_x && e_min_y < max_y && e_max_y > min_y) {
                    r = seg2d_segment_intersect(&seg, e, NULL);
                    if (r == 1 || r == 4)
                        return false;
                }
            }
        }
    }
    prohibit = flags & 2;
    avoid = flags & 4;
    n = 0;
    if (prohibit)
        n = avoid_obstacle_counts[pf_cur_size * PF_MAX_LAYERS + layer];
    if (avoid)
        n += avoid_obstacle_counts_b[pf_cur_size * PF_MAX_LAYERS + layer];
    found = 0;
    for (ob = avoid_obstacles; ob < &avoid_obstacles[AVOID_NUM_OBSTACLES]; ob++) {
        if (found == n)
            break;
        if (!(ob->flags & AVOID_OB_USED) || ob->layer != layer || ob->size != pf_cur_size)
            continue;
        if (!prohibit && (ob->flags & AVOID_OB_PROHIBIT))
            continue;
        if (!avoid && (ob->flags & AVOID_OB_AVOID))
            continue;
        found++;
        if (max_y < ob->bounds.min_y || min_y > ob->bounds.max_y || max_x < ob->bounds.min_x
            || min_x > ob->bounds.max_x)
            continue;
        if (avoid_segment_hits_prim(ob, a, b, 0.0f))
            return false;
    }
    return true;
}

// 0x4a53d0  Confidence: exact (VC6 byte match; D15401: dy is a copy of dx's line, which also gives the
// binary's dx spill into the dead `hit` slot). Nearest crossing of seg with a layer-transition
// edge (pf_walls) that leaves `layer` on seg's side: *hit and *hit_layer (the layer on the other side).
// Game bug: the squared distance is dx*dx + dx*dx (x twice, the z term is missing), so "nearest" is by x
// alone.
bool pf_first_edge_crossing(const seg2d *seg, int layer, vector2 *hit, int *hit_layer)
{
    pf_wall *w;
    pf_edge ws;
    vector2 pt;
    float best, d, dx, dy;
    int from, to;
    bool found;

    found = false;
    best = 9.9e10f;                                // 0x51b866e1
    for (w = pf_walls; w; w = w->next) {
        ws.a = w->a;
        ws.b = w->b;
        if (seg2d_segment_intersect(seg, &ws, &pt) != 1)
            continue;
        if (seg2d_point_left_of(seg->a, &ws, 0)) {
            from = w->layer_b;
            to = w->layer_a;
        } else {
            from = w->layer_a;
            to = w->layer_b;
        }
        if (from != layer)
            continue;
        dx = seg->a->x - pt.x;
        dy = seg->a->x - pt.x;                     // sic: x again (copy of the line above)
        d = dx * dx + dy * dy;
        if (!found || d < best) {
            best = d;
            *hit = pt;
            *hit_layer = to;
            found = true;
        }
    }
    return found;
}

// 0x4a54b0  Confidence: exact (VC6 byte match) (the hit point and layer live in the argument slots). Follows a move from
// pos to dest across every layer-transition edge it crosses; returns the layer at dest.
int pf_trace_edge_crossings(const vector3 *pos, int layer, vector3 dest)
{
    profile_label label("pf_trace_edge_crossings");
    vector2 cur;
    vector2 to;
    pf_edge seg;
    vector2 hit;
    int hit_layer;

    cur.x = pos->x;
    cur.y = pos->z;
    to.x = dest.x;
    to.y = dest.z;
    seg.a = &cur;
    seg.b = &to;
    while (pf_first_edge_crossing(&seg, layer, &hit, &hit_layer)) {
        cur = hit;
        layer = hit_layer;
    }
    return layer;
}

// 0x4a5570  Confidence: exact.
bool segments_intersect_xz(const vector3 *a, const vector3 *b, const vector3 *c, const vector3 *d)
{
    vector2 pa, pb, pc, pd;
    pf_edge s1, s2;

    pa.from_xz(a);
    pb.from_xz(b);
    pc.from_xz(c);
    pd.from_xz(d);
    s1.a = &pa;
    s1.b = &pb;
    s2.a = &pc;
    s2.b = &pd;
    bool r = seg2d_segment_intersect(&s1, &s2, NULL) == 1;
    return r;
}

// 0x4a55f0  Confidence: exact (cur + hit in the midpoint sums, D10805; the crossing layer is kept in a's argument slot). Straight walk from a
// (on a_layer) to b (on b_layer): every piece between layer-edge crossings must be clear
// (pf_segment_hits_walls with flags) and have its midpoint in a BSP cell; the walk must end on b_layer.
bool pf_straight_walk_clear(const vector2 *a, int a_layer, const vector2 *b, int b_layer, int flags)
{
    vector2 cur;
    vector2 hit;
    vector3 mid;
    pf_edge seg;
    int hit_layer;

    cur = *a;
    seg.a = &cur;
    seg.b = (vector2 *)b;
    while (pf_first_edge_crossing(&seg, a_layer, &hit, &hit_layer)) {
        if (!pf_segment_hits_walls(&cur, &hit, a_layer, flags))
            return false;
        mid.x = (cur.x + hit.x) * 0.5f;
        mid.y = 0.0f;
        mid.z = (cur.y + hit.y) * 0.5f;
        if (!pf_bsp_find_cell(&mid, a_layer))
            return false;
        cur = hit;
        a_layer = hit_layer;
    }
    if (a_layer != b_layer)
        return false;
    mid.x = (cur.x + b->x) * 0.5f;
    mid.y = 0.0f;
    mid.z = (cur.y + b->y) * 0.5f;
    if (!pf_bsp_find_cell(&mid, a_layer))
        return false;
    return pf_segment_hits_walls(&cur, b, b_layer, flags);
}

// 0x4a5750  Confidence: likely (the order of the shared-endpoint tests and of the collinear cases).
// Classifies the crossing of segments a and b: 3 a shared end point, -1 parallel or collinear and
// apart, 2 collinear and overlapping (out = an end point inside), 1 a proper crossing, 4 a touch at an
// end point, 0 none. The denominator and the two numerators stay in seg2d_denom / seg2d_cross /
// seg2d_param (callers divide them). Game bug: for a->b == b->a the reported point is b->b, the far end
// of b, not the shared point.
int seg2d_segment_intersect(const seg2d *a, const seg2d *b, vector2 *out)
{
    vector2 *p, *q, *lo, *hi, *t;
    float dax, day, dbx, dby, cx, cy;

    p = a->a;
    q = b->a;
    if (p->x == q->x && p->y == q->y) {
        if (out)
            *out = *p;
        return 3;
    }
    t = b->b;
    if (p->x == t->x && p->y == t->y) {
        if (out)
            *out = *p;
        return 3;
    }
    p = a->b;
    if (p->x == q->x && p->y == q->y) {
        if (out)
            *out = *t;
        return 3;
    }
    if (p->x == t->x && p->y == t->y) {
        if (out)
            *out = *p;
        return 3;
    }
    dax = a->b->x - a->a->x;
    day = a->b->y - a->a->y;
    dbx = b->b->x - b->a->x;
    dby = b->b->y - b->a->y;
    cx = b->a->x - a->a->x;
    cy = b->a->y - a->a->y;
    seg2d_denom = dby * dax - dbx * day;
    seg2d_cross = cx * day - cy * dax;
    seg2d_param = cx * dby - cy * dbx;
    if (seg2d_denom == 0.0f) {
        if (seg2d_cross != 0.0f)
            return -1;
        lo = b->a;
        hi = b->b;
        if (dax == 0.0f) {
            if (lo->y > hi->y) {
                lo = b->b;
                hi = b->a;
            }
            p = a->a;
            if (p->y > lo->y && p->y < hi->y) {
                if (out)
                    *out = *p;
                return 2;
            }
            if (p->y < lo->y && a->b->y < lo->y)
                return -1;
            if (p->y > hi->y && a->b->y > hi->y)
                return -1;
            if (abs((int)(a->b->y - lo->y)) < abs((int)(a->b->y - hi->y))) {
                if (out)
                    *out = *lo;
                return 2;
            }
        } else {
            if (lo->x > hi->x) {
                lo = b->b;
                hi = b->a;
            }
            p = a->a;
            if (p->x > lo->x && p->x < hi->x) {
                if (out)
                    *out = *p;
                return 2;
            }
            if (p->x < lo->x && a->b->x < lo->x)
                return -1;
            if (p->x > hi->x && a->b->x > hi->x)
                return -1;
            if (abs((int)(a->b->x - lo->x)) < abs((int)(a->b->x - hi->x))) {
                if (out)
                    *out = *lo;
                return 2;
            }
        }
        if (out)
            *out = *hi;
        return 2;
    }
    if (seg2d_denom > 0.0f) {
        if (seg2d_cross < 0.0f || seg2d_cross > seg2d_denom)
            return 0;
        if (seg2d_param < 0.0f || seg2d_param > seg2d_denom)
            return 0;
    } else {
        if (seg2d_cross > 0.0f || seg2d_cross < seg2d_denom)
            return 0;
        if (seg2d_param > 0.0f || seg2d_param < seg2d_denom)
            return 0;
    }
    if (out) {
        out->x = seg2d_param * dax / seg2d_denom + a->a->x;
        out->y = seg2d_param * day / seg2d_denom + a->a->y;
    }
    if (seg2d_cross != 0.0f && seg2d_cross != seg2d_denom && seg2d_param != 0.0f && seg2d_param != seg2d_denom)
        return 1;
    return 4;
}

// 0x4a5bd0  Confidence: exact. Caller seg2d_closest_point (bias 0).
void seg2d_line_intersect(const seg2d *s, const seg2d *line, vector2 *out, float bias)
{
    float dx, dy, lx, ly, cx, cy;

    dx = s->b->x - s->a->x;
    dy = s->b->y - s->a->y;
    lx = line->b->x - line->a->x;
    ly = line->b->y - line->a->y;
    cx = line->a->x - s->a->x;
    cy = line->a->y - s->a->y;
    seg2d_denom = ly * dx - lx * dy;
    seg2d_param = (cx * ly - cy * lx) / seg2d_denom + bias;
    out->x = seg2d_param * dx + s->a->x;
    out->y = seg2d_param * dy + s->a->y;
}

// 0x4a5c60  Confidence: exact (D11400: the line case through a vector2 edge normal). Descends a navigation BSP to the leaf cell
// (type 3) holding pt; near a split (0.0001 in x, 0.003 for a line) the front side is searched first,
// then the back. Another type loops on the same node.
pf_bsp *pf_bsp_node_find(pf_bsp *node, const vector2 *pt)
{
    pf_bsp *r;
    vector2 *v;
    pf_edge *e;
    float d;
    vector2 n;

    while (node) {
        switch (node->type) {
        case 1:
            v = (vector2 *)node->data;
            d = pt->x - v->x;
            if (d > -0.0001f && d < 0.0001f) {
                r = pf_bsp_node_find(node->front, pt);
                if (r)
                    return r;
                node = node->back;
            } else if (pt->x < v->x) {
                node = node->front;
            } else {
                node = node->back;
            }
            break;
        case 2:
            e = (pf_edge *)node->data;
            n = vector2(e->b->y - e->a->y, e->a->x - e->b->x);
            d = (pt->x - e->a->x) * n.x + (pt->y - e->a->y) * n.y;
            if (d > -0.003f && d < 0.003f) {
                r = pf_bsp_node_find(node->front, pt);
                if (r)
                    return r;
                node = node->back;
            } else if (d > 0.0f) {
                node = node->back;
            } else {
                node = node->front;
            }
            break;
        case 3:
            return node;
        default:
            break;
        }
    }
    return NULL;
}

// 0x4a5d40  Confidence: exact (the profile label). The BSP root of the current graph (layer -1) or of
// the given layer of the current size class.
pf_bsp *pf_bsp_find_cell(const vector3 *pos, int layer)
{
    profile_label label("pf_bsp_find_cell");
    vector2 p;
    pf_bsp *root;

    p.x = pos->x;
    p.y = pos->z;
    if (layer == -1)
        root = pf_cur_graph->bsp;
    else
        root = pf_graphs[pf_cur_size][layer].bsp;
    return pf_bsp_node_find(root, &p);
}

// 0x4a5dc0  Confidence: exact.
bool pf_point_is_navigable(const vector3 *pos, int size, int layer)
{
    pf_select_graph(size, layer);
    if (pf_cur_graph->count == 0)
        return false;
    if (!pf_cur_graph->bsp)
        return true;
    return pf_bsp_find_cell(pos, -1) ? true : false;
}

// 0x4a5e00  Confidence: exact.
int grid_hash_bucket(int x, int y, int w, int n)
{
    return (x + y * w) % n;
}

// 0x4a5e20  Confidence: exact.
int grid_linear_index(int x, int y, int w)
{
    return x + y * w;
}

// 0x4a5e30  Confidence: exact. The cell (x, y) of the hashed wall grid, NULL when out of range or empty.
pf_grid_cell *pf_grid_hash_find(int x, int y, pf_grid *grid)
{
    pf_grid_row *row;
    int bucket, index, i;

    if (grid->num_rows == 0 || x < 0 || x >= grid->_unk18 || y < 0 || y >= grid->_unk1c)
        return NULL;
    bucket = grid_hash_bucket(x, y, grid->_unk18, grid->num_rows);
    index = grid_linear_index(x, y, grid->_unk18);
    row = &grid->rows[bucket];
    for (i = 0; i < row->num_cells; i++) {
        if (row->cells[i]._unk00 == index)
            return &row->cells[i];
    }
    return NULL;
}

// 0x4a5eb0  Confidence: exact. Parameters of the crossing of a-b with c-d (t on a-b, u on c-d); false
// when parallel (|denominator| < 1e-6) or when either is NaN.
bool seg2d_intersect_params(const vector2 *a, const vector2 *b, const vector2 *c, const vector2 *d,
                            float *t, float *u)
{
    float ex, ey, den;

    ey = d->y - c->y;
    ex = d->x - c->x;
    den = (b->x - a->x) * ey - (b->y - a->y) * ex;
    if ((float)fabs(den) < 1e-6f)
        return false;
    *t = ((a->y - c->y) * ex - (a->x - c->x) * ey) / den;
    *u = ((a->y - c->y) * (b->x - a->x) - (b->y - a->y) * (a->x - c->x)) / den;
    return !_isnan(*t) && !_isnan(*u);
}

// 0x4a5f70  Confidence: exact (VC6 byte match) (the order of the four floor calls). The hashed grid cells overlapping
// the square of half size `half` around pt, into pf_grid_cell_results; returns the count.
int pf_grid_cells_in_box(const vector2 *pt, float half, pf_grid *grid)
{
    pf_grid_cell *c;
    float fx0, fx1, fy0, fy1;
    int x0, x1, y0, y1, x, y;

    fx0 = (pt->x - half - grid->bounds[0]) / grid->bounds[4];
    fx1 = (half + pt->x - grid->bounds[0]) / grid->bounds[4];
    fy0 = (pt->y - half - grid->bounds[2]) / grid->bounds[5];
    fy1 = (half + pt->y - grid->bounds[2]) / grid->bounds[5];
    x0 = (int)floor(fx0);
    x1 = (int)floor(fx1);
    y0 = (int)floor(fy0);
    y1 = (int)floor(fy1);
    pf_grid_cell_count = 0;
    for (x = x0; x <= x1; x++) {
        for (y = y0; y <= y1; y++) {
            c = pf_grid_hash_find(x, y, grid);
            if (c)
                pf_grid_cell_results[pf_grid_cell_count++] = c;
        }
    }
    return pf_grid_cell_count;
}

// 0x4a6060  Confidence: likely (the four direction cases are written out; the compiler merges their
// tails). Walks the grid cells crossed by a-b (a DDA in cell units: fx / fy the position inside the
// current cell, px / py the position, bx / by the next boundary) into pf_grid_cell_results; returns
// the count. The step count is the cell distance in x plus the one in y.
int pf_grid_cells_along_segment(const vector2 *a, const vector2 *b, pf_grid *grid)
{
    pf_grid_cell *c;
    float ax, ay, ex, ey, dx, dy, fx, fy, px, py, s, sx, sy;
    int cx, cy, bx, by, n, dir;
    bool xpos, ypos;

    xpos = true;
    ax = (a->x - grid->bounds[0]) / grid->bounds[4];
    ay = (a->y - grid->bounds[2]) / grid->bounds[5];
    ex = (b->x - grid->bounds[0]) / grid->bounds[4];
    ey = (b->y - grid->bounds[2]) / grid->bounds[5];
    dx = ex - ax;
    dy = ey - ay;
    if (dx < 0.0f)
        xpos = false;
    ypos = true;
    if (dy < 0.0f)
        ypos = false;
    cx = (int)floor(ax);
    cy = (int)floor(ay);
    fx = ax - (float)cx;
    fy = ay - (float)cy;
    if (xpos)
        n = (int)floor(ex - (float)cx);
    else
        n = (int)ceil((float)cx - ex);
    if (ypos)
        n += (int)floor(ey - (float)cy);
    else
        n += (int)ceil((float)cy - ey);
    pf_grid_cell_count = 0;
    c = pf_grid_hash_find(cx, cy, grid);
    if (c)
        pf_grid_cell_results[pf_grid_cell_count++] = c;
    py = ay;
    px = ax;
    dir = 0;
    if (xpos)
        dir = 2;
    if (ypos)
        dir++;
    bx = cx + 1;
    by = cy + 1;
    while (n--) {
        switch (dir) {
        case 3:                                    // +x, +y
            sx = (1.0f - fy) * dx;
            sy = (1.0f - fx) * dy;
            if (sx < sy) {
                s = sx / dy;
                cy++;
                by++;
                fy = 0.0f;
                px += s;
                fx += s;
                py = (float)cy;
            } else {
                s = sy / dx;
                cx++;
                bx++;
                fx = 0.0f;
                py += s;
                fy += s;
                px = (float)cx;
            }
            break;
        case 2:                                    // +x, -y
            sx = fy * dx;
            sy = (1.0f - fx) * dy;
            if (sx < -sy) {
                s = -(sx / dy);
                px += s;
                fx += s;
                cy--;
                by--;
                fy = 1.0f;
                py = (float)by;
            } else {
                s = sy / dx;
                cx++;
                bx++;
                fx = 0.0f;
                py += s;
                fy += s;
                px = (float)cx;
            }
            break;
        case 1:                                    // -x, +y
            sx = (1.0f - fy) * dx;
            sy = fx * dy;
            if (-sx < sy) {
                s = sx / dy;
                cy++;
                by++;
                fy = 0.0f;
                px += s;
                fx += s;
                py = (float)cy;
            } else {
                s = -(sy / dx);
                py += s;
                fy += s;
                cx--;
                bx--;
                fx = 1.0f;
                px = (float)bx;
            }
            break;
        case 0:                                    // -x, -y
            sx = fy * dx;
            sy = fx * dy;
            if (-sy > -sx) {
                s = sx / dy;
                px -= s;
                fx -= s;
                cy--;
                by--;
                fy = 1.0f;
                py = (float)by;
            } else {
                s = sy / dx;
                py -= s;
                fy -= s;
                cx--;
                bx--;
                fx = 1.0f;
                px = (float)bx;
            }
            break;
        }
        c = pf_grid_hash_find(cx, cy, grid);
        if (c)
            pf_grid_cell_results[pf_grid_cell_count++] = c;
    }
    (void)px;
    (void)py;
    return pf_grid_cell_count;
}

// 0x4a64b0  Confidence: exact. (layer, size): graph pf_graphs[size][layer] (D1201).
int pf_cells_along_segment(int layer, int size, const vector2 *a, const vector2 *b)
{
    pf_graph *g = &pf_graphs[size][layer];
    if (g->grid.num_rows == 0)
        return 0;
    return pf_grid_cells_along_segment(a, b, &g->grid);
}

// 0x4a64f0  Confidence: exact. Caller door_find_floor_point.
int pf_cells_in_box(int layer, int size, const vector2 *pt, float half)
{
    pf_graph *g = &pf_graphs[size][layer];
    if (g->grid.num_rows == 0)
        return 0;
    return pf_grid_cells_in_box(pt, half, &g->grid);
}

// 0x4a6530  Confidence: likely (8168 sorts the factors of the second dot product; int, not bool: D6401).
// 1 when pt projects within half the segment's length of its midpoint
// and lies closer than dist to its line.
int seg2d_point_near_middle(const vector2 *pt, const seg2d *s, float dist)
{
    vector2 mid;
    vector2 d;
    vector2 perp;
    float half;

    mid = 0.5f * (*s->a + *s->b);
    vector2 dir(s->b->x - mid.x, s->b->y - mid.y);
    half = dir.normalize();
    perp = vector2(-dir.y, dir.x);
    d.x = mid.x - pt->x;
    d.y = mid.y - pt->y;
    if ((float)fabs(d.y * dir.y + d.x * dir.x) < half && (float)fabs(d.x * perp.x + perp.y * d.y) < dist)
        return 1;
    return 0;
}

// 0x4a6600  Confidence: exact (VC6 byte match) (the two tests are written as two ifs). Which of two layers joined by
// a transition edge within 3.0 of pos (middle part of the edge) the point is on; layer_b when no edge.
int pf_layer_from_edges(int layer_a, int layer_b, const vector3 *pos)
{
    pf_wall *w;
    pf_edge s;
    vector2 p;
    int i;

    p.x = pos->x;
    p.y = pos->z;
    for (i = 0; i < pf_wall_count; i++) {
        w = &pf_walls[i];
        if (w->layer_a == layer_a && w->layer_b == layer_b && seg2d_point_near_middle(&p, w, 3.0f)) {
            s.a = w->a;
            s.b = w->b;
            if (seg2d_point_left_of(&p, &s, 0))
                return layer_b;
            return layer_a;
        }
        if (w->layer_a == layer_b && w->layer_b == layer_a && seg2d_point_near_middle(&p, w, 3.0f)) {
            s.a = w->a;
            s.b = w->b;
            if (seg2d_point_left_of(&p, &s, 0))
                return layer_a;
            return layer_b;
        }
    }
    return layer_b;
}
