// pathfinding_visibility.cpp - Sum.exe v1.40, code 0x4b77f0-0x4b87bd (Engine\ai\pathfinding_visibility.cpp)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Unit: the assert path "D:\projects\Summoner\pccode\Engine\ai\pathfinding_visibility.cpp" (0x59d7d8, lines
// 296 and 970) and the sum.symbols module column. It follows ai_think (0x4b7610, the AI group / turn code)
// and ends before the Pyrul initialisers (0x4b87c0). The algorithm is described in
// engine/ai/pathfinding_visibility.h; only determine_point_visibility is called from outside
// (pf_compute_endpoint_visibility, when Precalc_vis_method is set).
// Data: .data 0x59d7d8-0x59d933 (the path, the two error texts, the timing report); the constants 0.5,
// 100.0, +-0.0001 and 0.0 are pooled .rdata. .bss 0x247e968-0x247fcef (after the AI group formation
// tables, which end at 0x247e967).
//
// Profiling: every function but the pool helpers opens an EH frame whose only unwind action is the empty
// destructor 0x434650 (profile_label, vsdk/os/profile.h; records 0x5844b8 / 0x5844e0 / 0x584508 /
// 0x584530 / 0x584558), as in ai.cpp; the label texts are not in the binary, the names used here are the
// timing report's (ours). The read_tsc phase counters are separate statements.
//
// add_or_remove_edge (0x4b7e80) is compiled without optimisation (ebp frame, every access through memory,
// the `xor reg, reg / test / jne` of the list macros' while (0)): written between #pragma optimize("", off)
// and on (D2603).

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "vsdk/os/debug.h"
#include "vsdk/os/console.h"
#include "vsdk/os/profile.h"
#include "vsdk/os/timer.h"
#include "vsdk/math/vector.h"
#include "engine/ai/pathfinding.h"
#include "engine/ai/pathfinding_visibility.h"

#define PATHVIS_EPSILON 0.0001f       // 0x581930 / -0x581b74

// The list insertions of this file (names ours): before `pos` (pathvis_insert_sorted, pathvis_insert_by_side,
// add_or_remove_edge) and after it (add_or_remove_edge). Their store order differs from vsdk.h's
// list_insert, so they are separate macros.
#define pathvis_list_insert_before(pos, elem) do { (pos)->prev->next = (elem); (elem)->prev = (pos)->prev; \
                                                   (pos)->prev = (elem); (elem)->next = (pos); } while (0)
#define pathvis_list_insert_after(pos, elem)  do { (pos)->next->prev = (elem); (elem)->next = (pos)->next; \
                                                   (elem)->prev = (pos); (pos)->next = (elem); } while (0)

// ---- .bss (address order) -----------------------------------------------------------------------------
pathvis_edge pathvis_edge_lists[PF_MAX_LAYERS];         // 0x247e968
int pathvis_edge_cycles;                                // 0x247ec88
bool pathvis_ray_clear;                                 // 0x247ecdc
pathvis_edge *pathvis_pending_edge;                     // 0x247ecf0
int pathvis_tree_cycles;                                // 0x247ecf4
int pathvis_edge_tag;                                   // 0x247ecf8
pathvis_edge pathvis_edge_pool[PATHVIS_MAX_EDGES];      // 0x247ed00
int pathvis_sort_cycles;                                // 0x247fca0
vector2 *pathvis_origin;                                // 0x247fca8
vector2 *pathvis_target;                                // 0x247fcac
int pathvis_segment_cycles;                             // 0x247fcb8
pathvis_edge pathvis_free_edges;                        // 0x247fcc0
bool pathvis_alt_mode;                                  // 0x247fce8
int pathvis_edge_pool_inited;                           // 0x247fcec

// 0x4b77f0  Confidence: likely (the four quadrant heads as an array; the countdown loop `while (i--)`;
// the graph is re-indexed every pass because the node stores may alias pf_cur_size). Each point goes into
// the list of its quadrant (seen from pt: down-left, right-down, up-right, left-up), sorted by the slope
// key, equal keys nearer first; the lists are joined down-left, left-up, up-right, right-down (clockwise).
// The heads' f is 0, so a point with key 0 after the head is marked collinear too; the first point of each
// list is cleared again at the end.
pf_vis_node *sort_points_clockwise(const vector2 *pt)
{
    profile_label label("sort_points_clockwise");
    pf_vis_node quad[4];
    pf_vis_node *n, *q, *prev, *p, *tail;
    float dx, dy, pdx, pdy;
    int layer, i;

    pathvis_sort_cycles = read_tsc();
    quad[2].next_open = NULL;
    quad[3].next_open = NULL;
    quad[0].next_open = NULL;
    quad[1].next_open = NULL;
    quad[2].f = 0.0f;
    quad[3].f = 0.0f;
    quad[0].f = 0.0f;
    quad[1].f = 0.0f;

    for (layer = 0; layer < PF_MAX_LAYERS; layer++) {
        if (layer != pf_cur_layer)
            continue;
        i = pf_graphs[pf_cur_size][layer].count;
        while (i--) {
            n = &pf_graphs[pf_cur_size][layer].points[i];
            n->flags &= ~PFN_COLLINEAR;
            if (n->pos == pt)
                continue;
            dx = n->pos->x - pt->x;
            dy = n->pos->y - pt->y;
            if (dx == 0.0f && dy == 0.0f)
                continue;

            if (dx > 0.0f && dy <= 0.0f) {
                q = &quad[1];
                n->f = -(dy / dx);
            } else if (dx <= 0.0f && dy < 0.0f) {
                q = &quad[0];
                n->f = dx / dy;
            } else if (dx < 0.0f && dy >= 0.0f) {
                q = &quad[3];
                n->f = -(dy / dx);
            } else {
                q = &quad[2];
                n->f = dx / dy;
            }

            if (q->next_open)
                q->next_open->flags &= ~PFN_COLLINEAR;
            for (prev = q; (p = prev->next_open) != NULL; prev = p) {
                if (p->f > n->f)
                    break;
                if (p->f == n->f) {
                    pdx = p->pos->x - pt->x;
                    pdy = p->pos->y - pt->y;
                    if (dy * dy + dx * dx < pdy * pdy + pdx * pdx)
                        break;
                }
            }
            n->next_open = prev->next_open;
            prev->next_open = n;
            if (prev->f == n->f)
                n->flags |= PFN_COLLINEAR;
            if (n->next_open && n->next_open->f == n->f)
                n->next_open->flags |= PFN_COLLINEAR;
        }
    }

    if (quad[1].next_open)
        quad[1].next_open->flags &= ~PFN_COLLINEAR;
    if (quad[0].next_open)
        quad[0].next_open->flags &= ~PFN_COLLINEAR;
    if (quad[3].next_open)
        quad[3].next_open->flags &= ~PFN_COLLINEAR;
    if (quad[2].next_open)
        quad[2].next_open->flags &= ~PFN_COLLINEAR;
    for (tail = &quad[0]; tail->next_open; tail = tail->next_open)
        ;
    tail->next_open = quad[3].next_open;
    for (; tail->next_open; tail = tail->next_open)
        ;
    tail->next_open = quad[2].next_open;
    for (; tail->next_open; tail = tail->next_open)
        ;
    tail->next_open = quad[1].next_open;
    pathvis_sort_cycles = read_tsc() - pathvis_sort_cycles;
    return quad[0].next_open;
}

// 0x4b7af0  Confidence: unsure for the pathvis_alt_mode loop form (the edge walk through +0x14 is never
// taken), exact for the rest (VC6 byte match, D20603; `i` and `e` are initialised once at the top). For
// each layer: the edges of every cell of the graph that cross the vertical line x = pt->x below the point,
// nearest first, with a = the end with the larger x.
// D20603 (byte match): e and i declared (and initialised) before the label, a_left cleared before a_right, the
// straddle test as an if / else of `goto next` (the binary tests a_right first), and the tag stored before dist.
void make_initial_edge_tree(const vector2 *pt)
{
    pf_edge *e = NULL;
    int i = 0;
    profile_label label("make_initial_edge_tree");
    pathvis_edge *head;
    pathvis_edge *n;
    pf_cell *cell;
    int layer;
    bool a_left, a_right;
    float d;

    pathvis_tree_cycles = read_tsc();
    for (layer = 0; layer < PF_MAX_LAYERS; layer++) {
        head = &pathvis_edge_lists[layer];
        list_init(head);
        for (cell = pf_graphs[pf_cur_size][layer].cells; cell; cell = cell->next) {
            if (pathvis_alt_mode)
                e = cell->edges;
            else
                i = 0;
            for (;;) {
                if (pathvis_alt_mode) {
                    if (!e)
                        break;
                } else {
                    if (i >= cell->num_edges)
                        break;
                    e = &cell->edges[i];
                }

                a_left = false;
                a_right = false;
                if (e->a->x <= pt->x)
                    a_left = true;
                else
                    a_right = true;
                if (e->b->x <= pt->x) {
                    if (!a_right)
                        goto next;
                } else if (!a_left)
                    goto next;
                if (e->a->x != e->b->x) {
                    d = float_lerp_range(pt->x, e->a->x, e->a->y, e->b->x, e->b->y) - pt->y;
                    if (d < 0.0f) {
                        n = pathvis_add_edge_node();
                        if (e->a->x > e->b->x) {
                            n->a = e->a;
                            n->b = e->b;
                        } else {
                            n->a = e->b;
                            n->b = e->a;
                        }
                        n->tag = pathvis_edge_tag;
                        n->dist = -d;
                        n->dx = (n->a->x - n->b->x) / n->dist;
                        n->dy = (n->a->y - n->b->y) / n->dist;
                        pathvis_insert_sorted(n, layer);
                    }
                }

            next:
                if (pathvis_alt_mode)
                    e = *(pf_edge **)((ubyte *)e + 0x14);
                else
                    i++;
            }
        }
    }
    pathvis_tree_cycles = read_tsc() - pathvis_tree_cycles;
}

// 0x4b7d30  Confidence: likely (marked exact earlier; differs under cl 12.00.8966, match31). Line 296 (0x128).
pathvis_edge *pathvis_add_edge_node()
{
    pathvis_edge *e;

    if (!pathvis_edge_pool_inited)
        pathvis_edge_pool_init();
    e = pathvis_free_edges.next;
    if (e == &pathvis_free_edges) {
        sprintf(string_buffer, "Edge tree node limit of %d reached", PATHVIS_MAX_EDGES);
        VSDK_ERROR("D:\\projects\\Summoner\\pccode\\Engine\\ai\\pathfinding_visibility.cpp", 296, string_buffer);
    }
    list_remove(e);
    return e;
}

// 0x4b7da0  Confidence: exact (VC6 byte match, cl 8966, match21; was likely; the tail is a register: written as a local, the head's prev is stored
// once after the loop; the head's own prev store of list_init is absent).
void pathvis_edge_pool_init()
{
    pathvis_edge *tail;
    int i;

    if (pathvis_edge_pool_inited)
        return;
    pathvis_free_edges.next = &pathvis_free_edges;
    tail = &pathvis_free_edges;
    for (i = 0; i < PATHVIS_MAX_EDGES; i++) {
        pathvis_edge_pool[i].prev = tail;
        pathvis_edge_pool[i].next = &pathvis_free_edges;
        tail->next = &pathvis_edge_pool[i];
        tail = &pathvis_edge_pool[i];
    }
    pathvis_free_edges.prev = tail;
    pathvis_edge_pool_inited = 1;
}

// 0x4b7df0  Confidence: exact (VC6 byte match, cl 8966; was likely: the empty-list append before the loop; the
// loop's first test is kept as a
// dead `je`). (edge, layer): before the first entry that is not nearer.
void pathvis_insert_sorted(pathvis_edge *e, int layer)
{
    pathvis_edge *p;

    if (pathvis_edge_lists[layer].next == &pathvis_edge_lists[layer]) {
        list_append(&pathvis_edge_lists[layer], e);
        return;
    }
    for (p = pathvis_edge_lists[layer].next; p != &pathvis_edge_lists[layer]; p = p->next) {
        if (e->dist <= p->dist) {
            pathvis_list_insert_before(p, e);
            return;
        }
    }
    list_append(&pathvis_edge_lists[layer], e);
}

#ifdef _MSC_VER
#pragma optimize("", off)
#endif
// 0x4b7e80  Confidence: likely (unoptimised; locals declared in frame order, first = highest address:
// e ebp-0x10, the label -0x14, then -0x18..-0x30; `e->dist = len = sqrt(...)` from the fst / fstp pair).
// An edge already in the list (either direction) is removed; an edge parallel to the sweep ray is
// skipped. The first edge of a point is placed by side (pathvis_insert_by_side) and kept pending; the
// second goes before or after it by its direction against the ray.
void add_or_remove_edge(pf_edge *edge, int layer, vector2 *pos)
{
    pathvis_edge *e;
    profile_label label("add_or_remove_edge");
    float ray_dx;
    float edge_dx;
    float ray_dy;
    float edge_dy;
    float proj_new;
    float proj_pending;
    float len;

    for (e = pathvis_edge_lists[layer].next; e != &pathvis_edge_lists[layer]; e = e->next) {
        if ((e->a == edge->a && e->b == edge->b) || (e->a == edge->b && e->b == edge->a)) {
            list_remove(e);
            pathvis_free_edge_node(e);
            return;
        }
    }

    edge_dx = edge->a->x - edge->b->x;
    edge_dy = edge->a->y - edge->b->y;
    ray_dx = pathvis_origin->x - pathvis_target->x;
    ray_dy = pathvis_origin->y - pathvis_target->y;
    if (edge_dx * ray_dy - edge_dy * ray_dx == 0.0f)
        return;

    e = pathvis_add_edge_node();
    if (edge->a == pos) {
        e->a = edge->a;
        e->b = edge->b;
    } else {
        e->a = edge->b;
        e->b = edge->a;
        edge_dx = -edge_dx;
        edge_dy = -edge_dy;
    }
    e->dist = len = (float)sqrt(edge_dx * edge_dx + edge_dy * edge_dy);
    e->dx = edge_dx / e->dist;
    e->dy = edge_dy / e->dist;

    if (pathvis_pending_edge) {
        proj_new = ray_dx * e->dx + ray_dy * e->dy;
        proj_pending = ray_dx * pathvis_pending_edge->dx + ray_dy * pathvis_pending_edge->dy;
        if (proj_new < proj_pending)
            pathvis_list_insert_before(pathvis_pending_edge, e);
        else
            pathvis_list_insert_after(pathvis_pending_edge, e);
        pathvis_pending_edge = NULL;
    } else {
        pathvis_insert_by_side(e, layer);
        pathvis_pending_edge = e;
    }
}
#ifdef _MSC_VER
#pragma optimize("", on)
#endif

// 0x4b8150  Confidence: exact.
void pathvis_free_edge_node(pathvis_edge *e)
{
    if (!pathvis_edge_pool_inited)
        pathvis_edge_pool_init();
    list_append(&pathvis_free_edges, e);
}

// 0x4b8190  Confidence: exact (VC6 byte match, cl 8966; was likely: as pathvis_insert_sorted). (edge, layer):
// before the first entry that has
// the edge's a on its left (seg2d_point_left_of).
void pathvis_insert_by_side(pathvis_edge *e, int layer)
{
    pathvis_edge *p;

    if (pathvis_edge_lists[layer].next == &pathvis_edge_lists[layer]) {
        list_append(&pathvis_edge_lists[layer], e);
        return;
    }
    for (p = pathvis_edge_lists[layer].next; p != &pathvis_edge_lists[layer]; p = p->next) {
        if (seg2d_point_left_of(e->a, p, 0)) {
            pathvis_list_insert_before(p, e);
            return;
        }
    }
    list_append(&pathvis_edge_lists[layer], e);
}

// 0x4b8230  Confidence: likely (the `all` test after the first edge as a break). Returns 1 = blocked (the
// sum.symbols name reads the other way, D2603): the ray is already closed, the midpoint of from-to is
// inside the layer's BSP solid (pathvis_bsp_contains), or the segment crosses the nearest edge of the
// layer (with `all`, any edge).
// D20602: the two later `return 1`s as a `goto blocked` (the binary shares the early return's epilogue) and
// seg.a stored right after a's copy (distance 76 -> 43); left: the binary has b (to) below a (from) in the frame.
int do_single_vis_check(pf_vis_node *from, pf_vis_node *to, bool all)
{
    profile_label label("do_single_vis_check");
    vector2 a, b, mid;
    pf_edge seg;
    pathvis_edge *e;
    int layer;

    if (!pathvis_ray_clear)
        return 1;
    layer = from->layer;
    a = *from->pos;
    seg.a = &a;
    b = *to->pos;
    seg.b = &b;
    mid.x = (b.x + a.x) * 0.5f;
    mid.y = (b.y + a.y) * 0.5f;
    if (pathvis_bsp_contains(&mid, pf_graphs[pf_cur_size][layer].bsp))
        goto blocked;
    for (e = pathvis_edge_lists[layer].next; e != &pathvis_edge_lists[layer]; e = e->next) {
        if (seg2d_segment_intersect(&seg, e, NULL) == 1)
            goto blocked;
        if (!all)
            break;
    }
    return 0;
blocked:
    return 1;
}

// 0x4b8350  Confidence: likely (the group flag as an if / else; the two read_tsc readings in two locals;
// the report's divisor converted once). The sweep: for each point in clockwise order, its visibility
// against the current edge lists, then its two wall edges swapped in or out. Points on one ray (a group)
// keep their edges pending until the group ends, so the farther ones are tested against the same edges.
// Overflow at 50 pending points: the point's edges go in at once and it is appended anyway (NOTES.md, bugs).
void determine_point_visibility(const vector2 *pt, int layer)
{
    profile_label label("determine_point_visibility");
    pf_vis_node *pending[PATHVIS_MAX_PENDING];
    pf_vis_node origin;
    pf_vis_node *n, *last;
    int num_pending;
    int start_ms, start_tsc, t, t2, cycles;
    int blocked;
    int n_layer;
    int i;
    bool group;
    pathvis_edge *head, *e;
    float total;

    num_pending = 0;
    start_ms = timer_get(1000);
    start_tsc = read_tsc();
    memset(pending, 0, sizeof(pending));
    pathvis_origin = (vector2 *)pt;
    origin.pos = (vector2 *)pt;
    origin.layer = (short)layer;
    last = &origin;
    pathvis_edge_cycles = 0;
    pathvis_segment_cycles = 0;
    n = sort_points_clockwise(pt);
    make_initial_edge_tree(pt);

    while (n) {
        n_layer = n->layer;
        if (!(n->flags & PFN_COLLINEAR)) {
            pathvis_ray_clear = true;
            last = &origin;
            if (n->next_open && (n->next_open->flags & PFN_COLLINEAR))
                group = true;
            else
                group = false;
        } else {
            group = true;
        }

        pathvis_target = n->pos;
        t = read_tsc();
        blocked = do_single_vis_check(last, n, group);
        pathvis_segment_cycles += read_tsc() - t;
        if (!blocked)
            n->flags |= PFN_VISIBLE;
        else
            n->flags &= ~PFN_VISIBLE;

        t2 = read_tsc();
        if (!(n->flags & PFN_NO_EDGES)) {
            if (!group) {
                pathvis_pending_edge = NULL;
                add_or_remove_edge(n->edges[0], n_layer, n->pos);
                add_or_remove_edge(n->edges[1], n_layer, n->pos);
            } else {
                if (num_pending == PATHVIS_MAX_PENDING) {
                    if (pathvis_alt_mode)
                        VSDK_ERROR("D:\\projects\\Summoner\\pccode\\Engine\\ai\\pathfinding_visibility.cpp", 970,
                                   "Too many points on same sweep line.  Get DaveA");
                    pathvis_pending_edge = NULL;
                    add_or_remove_edge(n->edges[0], n_layer, n->pos);
                    add_or_remove_edge(n->edges[1], n_layer, n->pos);
                }
                pending[num_pending++] = n;
            }
        }
        pathvis_edge_cycles += read_tsc() - t2;
        if (!blocked) {
            pathvis_ray_clear = true;
            last = n;
        } else {
            pathvis_ray_clear = false;
        }

        n = n->next_open;
        if (group) {
            if (!n)
                break;
            if (!(n->flags & PFN_COLLINEAR)) {
                for (i = 0; i < num_pending; i++) {
                    pathvis_pending_edge = NULL;
                    add_or_remove_edge(pending[i]->edges[0], pending[i]->layer, pending[i]->pos);
                    add_or_remove_edge(pending[i]->edges[1], pending[i]->layer, pending[i]->pos);
                    pending[i] = NULL;
                }
                num_pending = 0;
            }
        }
    }

    for (i = 0; i < PF_MAX_LAYERS; i++) {
        head = &pathvis_edge_lists[i];
        while (head->next != head) {
            e = head->next;
            list_remove(e);
            pathvis_free_edge_node(e);
        }
    }

    cycles = read_tsc() - start_tsc;
    sprintf(string_buffer, "determine_point_visibility() took %d ms.\n", timer_get(1000) - start_ms);
    total = (float)cycles;
    sprintf(string_buffer, "   sort_points_clockwise() = %.1f%%\n", pathvis_sort_cycles * 100.0f / total);
    sprintf(string_buffer, "   make_initial_edge_tree() = %.1f%%\n", pathvis_tree_cycles * 100.0f / total);
    sprintf(string_buffer, "   do_single_vis_check() = %.1f%%\n", pathvis_segment_cycles * 100.0f / total);
    sprintf(string_buffer, "   add_or_remove_edge() = %.1f%%\n", pathvis_edge_cycles * 100.0f / total);
}

// 0x4b86e0  Confidence: likely (the case bodies are laid out 2 before 1, so case 2 is written first; an
// unknown type loops for ever on the same node). Type 2 splits by the edge's line, type 1 by the point's x
// (pf_bsp's "leaf" comment in pathfinding.h, D2602); a point within 0.0001 of the split is tested on both
// sides (front must hold, then back). Type 3: false.
// D20601: the near-split recursion as one block after the switch (`goto both`) is closer (distance 50 -> 30);
// left: the binary keeps case 2's push of front and jumps into case 1's inline recursion at `push edi`.
bool pathvis_bsp_contains(const vector2 *pt, pf_bsp *node)
{
    while (node) {
        switch (node->type) {
        case 2: {
            pf_edge *split = (pf_edge *)node->data;
            vector2 n(split->b->y - split->a->y, split->a->x - split->b->x);
            float side = (pt->y - split->a->y) * n.y + (pt->x - split->a->x) * n.x;
            if (side > -PATHVIS_EPSILON && side < PATHVIS_EPSILON) {
                goto both;
            } else if (side <= 0.0f) {
                node = node->front;
            } else {
                node = node->back;
            }
            break;
        }
        case 1: {
            vector2 *p = (vector2 *)node->data;
            float d = pt->x - p->x;
            if (d > -PATHVIS_EPSILON && d < PATHVIS_EPSILON) {
                goto both;
            } else if (pt->x < p->x) {
                node = node->front;
            } else {
                node = node->back;
            }
            break;
        }
        case 3:
            return false;
        }
        continue;
    both:
        if (!pathvis_bsp_contains(pt, node->front))
            return false;
        node = node->back;
    }
    return true;
}
