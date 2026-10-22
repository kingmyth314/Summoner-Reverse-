// unnamed_avoidance_0x4a6710.cpp - Sum.exe v1.40, code 0x4a6710-0x4aa77f: obstacle avoidance. Spells
// register avoidance areas ("AA", avoid_area_add); each becomes one obstacle per size class (a box or an
// octagon), overlapping obstacles are merged into avoidance polygons, each polygon is clipped against
// the navigation walls into a segment list whose corners join the A* graph (pathfinding.cpp's
// pf_graph_search links them); the avoidance splines are extra walls for pf_sweep_segment.
// FILE NAME NOT KNOWN: a placeholder (no path string, no strings.tbl block; docs/sum-exe/source-map.md
// hand cut 0x4a6710-0x4aa760). Written from the Sum.exe disassembly (tools/asm.mjs; floats and strings
// read from Sum.exe). Target: VC++ 6.0 (cl 12.00.8966), optimised (no frame pointer except the aligned
// frame of avoid_clip_segments) but without inline expansion: vector2::operator= / operator+ are called
// (their COMDATs 0x4aa3a0 / 0x4aa3c0 sit at the end of the unit), so the vector2 copies the code does
// inline are written component by component here.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: static_init_vector3_arrays_246a804 (0x4a6710, initialiser table entry 298) to
// avoid_record_axis_at (0x4aa760-0x4aa776); multi_door.cpp starts at 0x4aa780.
// Data: .data 0x59ce88-0x59d34c (the four switches, then this unit's strings in reverse order of use,
// D3709); .bss 0x2460cc0-0x2476bb8 (the source-map row's 0x246b92c end misses the spline points and the
// link blocks). The objects with static initialisers come first, in initialiser order (entries 298-301);
// the plain .bss follows in address order.
// The 0x14 / 0x44 / 0x8 element types get their `vector constructor iterator` calls from their vector2
// members (D3706).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "engine/ai/unnamed_avoidance_0x4a6710.h"
#include "engine/ai/pathfinding.h"
#include "vsdk/os/console.h"
#include "vsdk/os/timer.h"
#include "vsdk/math/vector.h"

#define PI_F 3.14159265f                          // 0x57f028 (0x40490fdb)

// ---- objects with static initialisers (entries 298-301) ------------------------------------------------
// 0x4a6710 static_init_vector3_arrays_246a804 (jmp). Confidence: exact.
// 0x4a6720 init_vector3_arrays_246a804  Confidence: exact. 14 x `vector constructor iterator`(points,
// 8, 30, 0x505530).
avoid_poly avoid_polys[AVOID_NUM_POLYS];                  // 0x246a7f0
// 0x4a6750 static_init_vector3_arrays_246a0b8 (jmp). Confidence: exact.
// 0x4a6760 init_vector3_arrays_246a0b8: 14 x iterator(points, 8, 8). Confidence: exact.
avoid_obstacle avoid_obstacles[AVOID_NUM_OBSTACLES];      // 0x246a080
// 0x4a6790 static_init_vector3_arrays_2461020 (jmp). Confidence: exact.
// 0x4a67a0 init_vector3_arrays_2461020  Confidence: exact. 14 x (iterator(segs, 0x14, 30),
// iterator(corners, 0x44, 30)).
avoid_seglist avoid_seglists[AVOID_NUM_SEGLISTS];         // 0x2460db0
// 0x4a67e0 static_init_ccmd_avoidance (jmp). Confidence: exact.
// 0x4a67f0 init_ccmd_avoidance (console_command constructor on 0x246b918). Confidence: exact.
console_command ccmd_avoidance("avoidance", "Sets avoidance area parameters", cmd_avoidance);   // 0x246b918

// ---- .bss without initialisers (address order) ---------------------------------------------------------
avoid_spline avoid_splines[AVOID_NUM_SPLINES];            // 0x2460cc0
char avoid_obstacle_counts[PF_NUM_SIZES * PF_MAX_LAYERS]; // 0x246a040
char avoid_obstacle_counts_b[PF_NUM_SIZES * PF_MAX_LAYERS];   // 0x246b708
int avoid_backup_seg_count;                               // 0x246b748
pf_edge *avoid_backup_segs[100];                          // 0x246b74c
char avoid_spline_point_counts[PF_NUM_SIZES * PF_MAX_LAYERS]; // 0x246b8dc
avoid_area avoid_areas[AVOID_NUM_AREAS];                  // 0x246b928
avoid_spline_point avoid_spline_points[AVOID_NUM_SPLINE_POINTS];   // 0x246b998
avoid_link_block avoid_link_blocks[AVOID_NUM_LINK_BLOCKS];   // 0x246c7a8 (name ours)

// ---- .data ---------------------------------------------------------------------------------------------
int avoidance_enabled = 1;                                // 0x59ce88
int avoidance_no_remove = 0;                              // 0x59ce8c
int avoidance_area_type = 1;                              // 0x59ce90
int avoidance_prohibit = 1;                               // 0x59ce94

// The float min / max of the original are inline functions with by-value arguments (D7500): the binary keeps
// the field value in a stack temporary (fst / fcomp st(1)), which the min / max macros do not give
// (rect2d_add_point matches with them; avoid_merge_obstacle_polys and avoid_add_primitive use them too).
static inline float min_f(float a, float b) { return a < b ? a : b; }
static inline float max_f(float a, float b) { return a > b ? a : b; }

// 0x4a6810  Confidence: exact (D21203: one `checking_status_for_command = 0;` after the keyword chain; 8966 copies it
// into every branch and no longer threads the help branch past the doing_help test). The empty buffer gets the
// argument through strcat, as the code does.
void cmd_avoidance()
{
    char buf[512];

    buf[0] = 0;
    if (calling_a_command_function) {
        process_command(ARG_NONE | ARG_STRING);
        if (arg_type & ARG_STRING) {
            strcat(buf, next_arg);
            if (!_stricmp(buf, "enable")) {
                if (!avoidance_enabled) {
                    avoidance_enabled = 1;
                    print_to_console("Avoidance areas enabled", 0);
                } else {
                    avoidance_enabled = 0;
                    print_to_console("Avoidance areas disabled", 0);
                }
            } else if (!_stricmp(buf, "no_remove")) {
                if (!avoidance_no_remove) {
                    avoidance_no_remove = 1;
                    print_to_console("Avoidance areas will not be removed", 0);
                } else {
                    avoidance_no_remove = 0;
                    print_to_console("Avoidance areas will be removed", 0);
                }
            } else if (!_stricmp(buf, "reset")) {
                avoid_clear_segment_flags();
                avoid_reset();
                print_to_console("Avoidance areas have been reset", 0);
            } else if (!_stricmp(buf, "rect")) {
                avoidance_area_type = 1;
                print_to_console("Avoidance areas type rectangle selected", 0);
            } else if (!_stricmp(buf, "circle")) {
                avoidance_area_type = 0;
                print_to_console("Avoidance areas type circle selected", 0);
            } else if (!_stricmp(buf, "prohibit")) {
                avoidance_prohibit = 1;
                print_to_console("Avoidance areas type prohibite selected", 0);
            } else if (!_stricmp(buf, "avoid")) {
                avoidance_prohibit = 0;
                print_to_console("Avoidance areas parameter not supported", 0);
            } else {
                doing_help_for_comand = 1;
            }
            checking_status_for_command = 0;
        } else {
            checking_status_for_command = 1;
        }
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "enable: %d", avoidance_enabled);
        print_to_console(string_buffer, 0);
        sprintf(string_buffer, "no_remove: %d", avoidance_no_remove);
        print_to_console(string_buffer, 0);
        if (avoidance_area_type)
            print_to_console("type: prohibit rect", 0);
        else
            print_to_console("type: prohibit circle", 0);
    }
    if (doing_help_for_comand) {
        print_to_console("Usage: avoidance enable -- enable avoidance areas.", 0);
        print_to_console("       avoidance no_remove -- do not remove avoidance areas.", 0);
        print_to_console("       avoidance reset --  deletes all current avoidance areas.  initializes avoidance area.", 0);
        print_to_console("       avoidance rect -- select rectangle avoidance areas.", 0);
        print_to_console("       avoidance circle -- select circle avoidance areas.", 0);
        print_to_console("       avoidance prohibit -- select prohibit avoidance areas.", 0);
        print_to_console("       avoidance avoid -- [not supported yet] selects avoid (non-prohibit) avoidance areas.", 0);
    }
}

// 0x4a6b40  Confidence: exact. Drops the avoidance marks of every nav wall a used segment list refers to.
void avoid_clear_segment_flags()
{
    int k, i;

    for (k = 0; k < AVOID_NUM_SEGLISTS; k++) {
        avoid_seglist *sl = &avoid_seglists[k];
        if (!(sl->flags & AVOID_SEGLIST_USED))
            continue;
        for (i = 0; i < sl->num_segs; i++) {
            pf_edge *w = sl->segs[i].wall;
            if (w) {
                w->flags &= ~0x20;
                w->_pad0b = 0;
            }
        }
    }
}

// 0x4a6b90  Confidence: exact.
bool avoid_box_contains_point(const vector2 *p, const avoid_obstacle *ob, float margin)
{
    float dx = p->x - ob->centre.x;
    float dy = p->y - ob->centre.y;

    if (fabs(dy * ob->axis_a.y + dx * ob->axis_a.x) < margin + ob->half_a
        && fabs(dy * ob->axis_b.y + dx * ob->axis_b.x) < margin + ob->half_b)
        return true;
    return false;
}

// 0x4a6bf0  Confidence: exact (the 1.1717894 factor, about 1 / cos(22.5 deg) squared, as a literal; D15503:
// r2 as the sum squared gives the binary's `fld st(0) / fmulp`; D17202: the centre through a pointer local
// puts r in ob's dead argument slot, as in the binary).
// Inside the radius, or inside the octagon of the four axes when within the circumscribed radius.
bool avoid_circle_contains_point(const vector2 *p, const avoid_obstacle *ob, float margin)
{
    vector2 d;
    float lsq, r, r2;
    const vector2 *c = &ob->centre;

    d.x = p->x - c->x;
    d.y = p->y - c->y;
    lsq = d.len_sq();
    r = margin + ob->half_b;
    r2 = (margin + ob->half_b) * (margin + ob->half_b);
    if (lsq < r2)
        return true;
    if (lsq <= r2 * 1.1717894f) {
        margin += ob->half_a;
        if (fabsf(d.y * ob->axis_a.y + d.x * ob->axis_a.x) < margin
            && fabsf(d.y * ob->axis_b.y + d.x * ob->axis_b.x) < r
            && fabsf(d.y * ob->axis_c.y + d.x * ob->axis_c.x) < margin
            && fabsf(d.y * ob->axis_d.y + d.x * ob->axis_d.x) < r)
            return true;
    }
    return false;
}

// 0x4a6ce0  Confidence: exact.
bool avoid_box_contains_point_exact(const vector2 *p, const avoid_obstacle *ob)
{
    return avoid_box_contains_point(p, ob, 0.0f);
}

// 0x4a6d00  Confidence: exact.
bool avoid_circle_contains_point_exact(const vector2 *p, const avoid_obstacle *ob)
{
    return avoid_circle_contains_point(p, ob, 0.0f);
}

// 0x4a6d20  Confidence: exact.
bool avoid_prim_test(const vector2 *p, const avoid_obstacle *ob, float margin)
{
    if (ob->flags & AVOID_OB_BOX)
        return avoid_box_contains_point(p, ob, margin);
    if (ob->flags & AVOID_OB_CIRCLE)
        return avoid_circle_contains_point(p, ob, margin);
    return false;
}

// D13302: an inline setter with both values as arguments (as vector3::set; possibly the original's
// vector2 member): the arguments are evaluated right to left and stored straight into the target, which
// is the binary's `fld x / fld y / fchs / fstp / fstp` for axis_b. Name ours.
static inline void vec2_set(vector2 &v, float x, float y) { v.x = x; v.y = y; }

// 0x4a6d60  Confidence: exact (w10, D10901: the centre, axis and points through the vector2 operators;
// axis_b through the setter, D13302). A box of zero width
// around the segment a-b.
void avoid_prim_from_segment(avoid_prim *out, const vector2 *a, const vector2 *b)
{
    out->flags = AVOID_OB_SEGMENT | AVOID_OB_USED;
    out->centre = 0.5f * (*a + *b);
    out->axis_a = *b - out->centre;
    out->half_a = out->axis_a.normalize();
    vec2_set(out->axis_b, -out->axis_a.y, out->axis_a.x);
    out->half_b = 0.0f;
    out->points[0] = *a;
    out->points[1] = *b;
}

// 0x4a6e00  Confidence: likely (the order of the projections; D18503: ku through the pointer copy bo and the
// third test's two k terms as inline dots, the first assigned to the otherwise unused kw, give the binary's
// operand orders and its fxch pair: distance 27 -> 13; left: the first two projections' product order and
// the binary's dead fst of the third test's u term into b's argument slot). Separating-axis test of a (its two axes,
// or all four for a circle) against b (whose second half size is not used: b is a segment primitive at
// every caller); margin shrinks a.
bool avoid_prim_overlap(const avoid_obstacle *a, const avoid_prim *b, float margin)
{
    vector2 d;
    const vector2 *u, *v;
    float ku, kv, kw;
    const avoid_prim *bo = b;
    int pass;

    d.x = a->centre.x - b->centre.x;
    d.y = a->centre.y - b->centre.y;
    v = &a->axis_b;
    u = &a->axis_a;
    for (pass = 0; pass < 2; pass++) {
        ku = fabs(bo->axis_a.dot(*u));
        if (fabs(d.x * u->x + d.y * u->y) > ku * b->half_a + a->half_a - margin)
            return false;
        kv = fabs(b->axis_a.y * v->y + b->axis_a.x * v->x);
        if (fabs(d.x * v->x + d.y * v->y) > kv * b->half_a + a->half_b - margin)
            return false;
        if (fabs(d.y * b->axis_b.y + d.x * b->axis_b.x) > (kw = fabs(b->axis_b.dot(*v))) * a->half_b
                + fabs(b->axis_b.dot(*u)) * a->half_a - margin)
            return false;
        if (fabs(d.y * b->axis_a.y + d.x * b->axis_a.x) > ku * a->half_a + kv * a->half_b + b->half_a - margin)
            return false;
        if (a->flags & AVOID_OB_BOX)
            break;
        v = &a->axis_d;
        u = &a->axis_c;
    }
    return true;
}

// 0x4a6f50  Confidence: exact.
bool avoid_segment_hits_prim(const avoid_obstacle *ob, const vector2 *a, const vector2 *b, float margin)
{
    avoid_prim seg;

    avoid_prim_from_segment(&seg, a, b);
    return avoid_prim_overlap(ob, &seg, margin);
}

// 0x4a6f90  Confidence: exact (D15502: d through vector2::operator-, three of the k products as inline dots,
// gives the binary's registers and k slots; D17203: b's half sizes in the first two tests through the pointer
// copy bo and k_aa's receiver through ao give the binary's operand order of those sums and of k_ab's and k_aa's
// x products; 8966 seems to order commutative operands by symbol, not by source order). Separating-axis test of two
// oriented boxes.
bool avoid_box_box_overlap(const avoid_obstacle *a, const avoid_obstacle *b)
{
    vector2 d;
    const avoid_obstacle *bo = b;
    const avoid_obstacle *ao = a;
    float k_aa, k_ab, k_ba, k_bb;

    d = a->centre - b->centre;
    k_bb = fabs(b->axis_b.y * a->axis_b.y + b->axis_b.x * a->axis_b.x);
    k_ab = fabs(b->axis_a.dot(a->axis_b));
    k_ba = fabs(a->axis_a.dot(b->axis_b));
    k_aa = fabs(ao->axis_a.dot(b->axis_a));
    if (fabs(d.y * a->axis_b.y + d.x * a->axis_b.x) > k_bb * bo->half_b + k_ab * bo->half_a + a->half_b)
        return false;
    if (fabs(d.y * a->axis_a.y + d.x * a->axis_a.x) > k_ba * bo->half_b + k_aa * bo->half_a + a->half_a)
        return false;
    if (fabs(d.y * b->axis_b.y + d.x * b->axis_b.x) > k_ba * a->half_a + k_bb * a->half_b + b->half_b)
        return false;
    if (fabs(d.y * b->axis_a.y + d.x * b->axis_a.x) > k_aa * a->half_a + k_ab * a->half_b + b->half_a)
        return false;
    return true;
}

// 0x4a70e0  Confidence: exact.
bool avoid_poly_points_in_box(const avoid_obstacle *box, const avoid_obstacle *other)
{
    int i;

    for (i = 0; i < other->num_points; i++) {
        if (avoid_box_contains_point_exact(&other->points[i], box))
            return true;
    }
    return false;
}

// 0x4a7120  Confidence: exact.
bool avoid_poly_points_in_circle(const avoid_obstacle *circle, const avoid_obstacle *other)
{
    int i;

    for (i = 0; i < other->num_points; i++) {
        if (avoid_circle_contains_point_exact(&other->points[i], circle))
            return true;
    }
    return false;
}

// 0x4a7160  Confidence: exact.
bool rect2d_overlap(rect2d a, rect2d b)
{
    if (a.max_y < b.min_y || a.min_y > b.max_y || a.max_x < b.min_x || a.min_x > b.max_x)
        return false;
    return true;
}

// 0x4a71b0  Confidence: exact. Both parameters inside [0.0002, 0.9998].
bool seg_params_in_range(const float *t, const float *u)
{
    if (*t < 0.0002f)
        return false;
    if (*t > 0.9998f)
        return false;
    if (*u < 0.0002f)
        return false;
    if (*u > 0.9998f)
        return false;
    return true;
}

// 0x4a7210  Confidence: exact (D11400: the polygon edge loop `j = i++`, no early return, `return best !=
// FLT_MAX`; the hit point through the vector2 operators, w10 D10901). Nearest crossing of
// p-q with an edge (points[i - 1], points[i]) of the polygon: *edge = i, *hit the point.
bool avoid_poly_nearest_edge_hit(const vector2 *p, const vector2 *q, const avoid_poly *poly, int *edge,
                                 vector2 *hit)
{
    float best, t, u;
    int i, j;

    best = FLT_MAX;
    for (i = 0, j = poly->num_points - 1; i < poly->num_points; j = i++) {
        if (seg2d_intersect_params(p, q, &poly->points[j], &poly->points[i], &t, &u) && seg_params_in_range(&t, &u)
            && t < best) {
            best = t;
            *edge = i;
            *hit = *p + t * (*q - *p);
        }
    }
    return best != FLT_MAX;
}

// 0x4a7320  Confidence: exact. A one-obstacle polygon (boxes and circles only).
bool avoid_poly_from_obstacle(avoid_poly *out, const avoid_obstacle *ob)
{
    int i;

    if (!(ob->flags & (AVOID_OB_BOX | AVOID_OB_CIRCLE)))
        return false;
    for (i = 0; i < ob->num_points; i++) {
        out->points[i] = ob->points[i];
    }
    out->size = ob->size;
    out->layer = ob->layer;
    out->num_points = ob->num_points;
    out->count = 1;
    out->obstacles[0] = (char)(ob - avoid_obstacles);
    out->bounds = ob->bounds;
    out->flags = ob->flags & 7;
    return true;
}

// 0x4a73d0  Confidence: exact.
bool avoid_prim_test_exact(const vector2 *p, const avoid_obstacle *ob)
{
    return avoid_prim_test(p, ob, 0.0f);
}

// 0x4a73f0  Confidence: likely (the walk's pointer bookkeeping; D22803: the point copies as vector2
// assignments, distance 263 -> 233; left: frame 0x250 against 0x22c, register choice in the walk). Adds an obstacle to a polygon of the
// same kind: a polygon wholly inside the obstacle takes the obstacle's outline; else the outline of the
// union is walked from the first polygon point outside the obstacle, switching between the two outlines
// at every edge crossing (at most 30 points; an odd number of switches fails).
bool avoid_merge_obstacle_polys(avoid_poly *poly, const avoid_obstacle *ob)
{
    avoid_poly other;
    vector2 out[30];
    vector2 hit;
    const avoid_poly *cur, *alt, *tmp;
    const vector2 *p;
    int start, next, count, switches, edge, i;
    bool done;

    if ((poly->flags ^ ob->flags) & 7)
        return false;
    other.count = 0;
    other.num_points = 0;
    switches = 0;
    avoid_poly_from_obstacle(&other, ob);
    cur = poly;
    alt = &other;
    start = 0;
    for (i = 0; i < poly->num_points; i++) {
        if (!avoid_prim_test_exact(&poly->points[i], ob))
            break;
    }
    if (i >= poly->num_points) {
        poly->bounds = ob->bounds;
        poly->num_points = ob->num_points;
        poly->obstacles[poly->count] = (char)(ob - avoid_obstacles);
        poly->count++;
        for (i = 0; i < poly->num_points; i++) {
            poly->points[i] = ob->points[i];
        }
        return true;
    }
    start = i;
    out[0].x = poly->points[i].x;
    out[0].y = poly->points[i].y;
    next = (i + 1) % poly->num_points;
    count = 1;
    done = false;
    p = &poly->points[i];
    for (;;) {
        if (count >= 30)
            return false;
        if (avoid_poly_nearest_edge_hit(p, &cur->points[next], alt, &edge, &hit)) {
            tmp = cur;
            cur = alt;
            alt = tmp;
            switches++;
            out[count] = hit;
            count++;
            p = &out[count - 1];
            next = edge;
            continue;
        }
        if (next == start && cur == poly) {
            done = true;
        } else {
            out[count] = cur->points[next];
            count++;
            p = &out[count - 1];
        }
        next = (next + 1) % cur->num_points;
        if (done)
            break;
    }
    if (switches <= 0 || switches % 2 != 0)
        return false;
    for (i = 0; i < count; i++) {
        poly->points[i].x = out[i].x;
        poly->points[i].y = out[i].y;
    }
    poly->bounds.max_y = max_f(poly->bounds.max_y, other.bounds.max_y);
    poly->bounds.min_y = min_f(poly->bounds.min_y, other.bounds.min_y);
    poly->bounds.max_x = max_f(poly->bounds.max_x, other.bounds.max_x);
    poly->bounds.min_x = min_f(poly->bounds.min_x, other.bounds.min_x);
    poly->obstacles[poly->count] = (char)(ob - avoid_obstacles);
    poly->count++;
    poly->num_points = (short)count;
    return true;
}

// 0x4a7720  Confidence: exact.
avoid_link_block *avoid_link_block_alloc()
{
    avoid_link_block *b = NULL;
    int i;

    for (i = 0; i < AVOID_NUM_LINK_BLOCKS; i++) {
        if (!(avoid_link_blocks[i].flags & 1)) {
            avoid_link_blocks[i].flags |= 1;
            b = &avoid_link_blocks[i];
            break;
        }
    }
    return b;
}

// 0x4a7760  Confidence: exact.
void avoid_link_block_free(avoid_link_block *b)
{
    if (b)
        avoid_link_blocks[b - avoid_link_blocks].flags = 0;
}

// 0x4a77a0  Confidence: exact.
avoid_spline_point *avoid_spline_point_alloc()
{
    int i;

    for (i = 0; i < AVOID_NUM_SPLINE_POINTS; i++) {
        if (!(avoid_spline_points[i].flags & 1)) {
            avoid_spline_points[i].flags = 0;
            return &avoid_spline_points[i];
        }
    }
    return NULL;
}

// 0x4a77d0  Confidence: exact. D17200: the range test through a bool local (8966 then places the NULL return
// right after the tests, as in the binary).
avoid_spline_point *avoid_spline_point_get(int i)
{
    bool ok = i >= 0 && i < AVOID_NUM_SPLINE_POINTS;
    if (!ok)
        return NULL;
    return (avoid_spline_points[i].flags & 1) ? &avoid_spline_points[i] : NULL;
}

// 0x4a7800  Confidence: exact.
avoid_spline *avoid_spline_alloc()
{
    int i;

    for (i = 0; i < AVOID_NUM_SPLINES; i++) {
        if (!(avoid_splines[i].flags & 1)) {
            avoid_spline *p = &avoid_splines[i];
            p->flags = 1;
            p->count = 0;
            return p;
        }
    }
    return NULL;
}

// 0x4a7830  Confidence: exact. D17200: the range test through a bool local, as avoid_spline_point_get.
avoid_spline *avoid_spline_get(int i)
{
    bool ok = i >= 0 && i < AVOID_NUM_SPLINES;
    if (!ok)
        return NULL;
    return (avoid_splines[i].flags & 1) ? &avoid_splines[i] : NULL;
}

// 0x4a7860  Confidence: exact.
void avoid_spline_point_free(avoid_spline_point *pt)
{
    char *count = &avoid_spline_point_counts[pt->size * PF_MAX_LAYERS + pt->layer];
    if (*count > 0)
        (*count)--;
    pt->flags = 0;
    pt->size = -1;
    pt->layer = -1;
}

// 0x4a7890  Confidence: exact.
void avoid_spline_point_free_index(int i)
{
    if (i >= 0 && i < AVOID_NUM_SPLINE_POINTS)
        avoid_spline_point_free(&avoid_spline_points[i]);
}

// 0x4a78b0  Confidence: exact.
void avoid_spline_free(int index)
{
    avoid_spline *sp;
    int i;

    if (index == -1)
        return;
    if (index < 0 || index >= AVOID_NUM_SPLINES)
        return;
    sp = &avoid_splines[index];
    for (i = 0; i < sp->count; i++)
        avoid_spline_point_free_index(sp->points[i]);
    sp->flags = 0;
    sp->count = 0;
}

// 0x4a7900  Confidence: exact (D17201). A spline wall from a to b of size class `size` on `layer`.
int avoid_spline_point_create(const vector2 *a, const vector2 *b, int size, int layer)
{
    avoid_spline_point *pt = avoid_spline_point_alloc();
    if (!pt)
        return -1;
    // D17201: the two points copied as the plain 8-byte avoid_spline_end (D13300); vector2's inline
    // operator= keeps p1.y's store ahead of wall.a's.
    *(avoid_spline_end *)&pt->p0 = *(const avoid_spline_end *)a;
    *(avoid_spline_end *)&pt->p1 = *(const avoid_spline_end *)b;
    pt->wall.a = &pt->p0;
    pt->wall.b = &pt->p1;
    pt->size = (char)size;
    pt->layer = (char)layer;
    pt->flags = 1;
    pt->wall.door = 0;
    pt->wall.flags = 0;
    avoid_spline_point_counts[pt->size * PF_MAX_LAYERS + pt->layer]++;
    return pt - avoid_spline_points;
}

// 0x4a7990  Confidence: exact. Points that fail to allocate are skipped.
int avoid_spline_create(avoid_spline_def *def)
{
    avoid_spline *sp;
    int n, i, r;

    sp = avoid_spline_alloc();
    if (!sp)
        return -1;
    n = 0;
    for (i = 0; i < def->count; i++) {
        r = avoid_spline_point_create((const vector2 *)&def->p0[i], (const vector2 *)&def->p1[i], def->cell_x[i],
                                      def->cell_y[i]);
        if (r != -1)
            sp->points[n++] = (char)r;
    }
    sp->count = (short)n;
    return sp - avoid_splines;
}

// 0x4a7a30  Confidence: exact. Does not mark the list used (avoid_clip_segments does).
avoid_seglist *avoid_seglist_alloc()
{
    int i;

    for (i = 0; i < AVOID_NUM_SEGLISTS; i++) {
        if (!(avoid_seglists[i].flags & AVOID_SEGLIST_USED)) {
            avoid_seglist *p = &avoid_seglists[i];
            p->num_corners = 0;
            p->num_segs = 0;
            p->poly = -1;
            return p;
        }
    }
    return NULL;
}

// 0x4a7a80  Confidence: exact. Does not mark the polygon used (the obstacle copy's flags do).
avoid_poly *avoid_poly_alloc()
{
    int i;

    for (i = 0; i < AVOID_NUM_POLYS; i++) {
        if (!(avoid_polys[i].flags & 1)) {
            avoid_poly *p = &avoid_polys[i];
            p->flags = 0;
            p->num_points = 0;
            p->count = 0;
            return p;
        }
    }
    return NULL;
}

// 0x4a7ac0  Confidence: exact.
avoid_obstacle *avoid_obstacle_alloc()
{
    int i;

    for (i = 0; i < AVOID_NUM_OBSTACLES; i++) {
        if (!(avoid_obstacles[i].flags & AVOID_OB_USED)) {
            avoid_obstacle *p = &avoid_obstacles[i];
            p->num_points = 0;
            return p;
        }
    }
    return NULL;
}

// 0x4a7b00  Confidence: exact.
avoid_area *avoid_area_alloc()
{
    int i;

    for (i = 0; i < AVOID_NUM_AREAS; i++) {
        if (!(avoid_areas[i].flags & 1)) {
            avoid_area *p = &avoid_areas[i];
            p->flags |= 1;
            p->count = 0;
            return p;
        }
    }
    return NULL;
}

// 0x4a7b30  Confidence: exact.
void avoid_obstacle_free(avoid_obstacle *ob)
{
    if (ob->flags & AVOID_OB_PROHIBIT)
        avoid_obstacle_counts[ob->size * PF_MAX_LAYERS + ob->layer]--;
    if (ob->flags & AVOID_OB_AVOID)
        avoid_obstacle_counts_b[ob->size * PF_MAX_LAYERS + ob->layer]--;
    ob->flags = 0;
}

// 0x4a7b70  Confidence: exact.
void avoid_seglist_mark_free(avoid_seglist *sl)
{
    sl->flags = 0;
}

// 0x4a7b80  Confidence: exact.
void avoid_area_free(void *slot)
{
    *(short *)slot = 0;
}

// 0x4a7b90  Confidence: exact. pathfinding_free_graph: the walls go away with the graph.
void avoid_seglists_clear_refs()
{
    int k, i;

    for (k = 0; k < AVOID_NUM_SEGLISTS; k++) {
        avoid_seglist *sl = &avoid_seglists[k];
        if (!(sl->flags & AVOID_SEGLIST_USED))
            continue;
        for (i = 0; i < sl->num_segs; i++) {
            if (sl->segs[i].wall)
                sl->segs[i].wall = NULL;
        }
    }
}

// 0x4a7bd0  Confidence: exact.
void avoid_seglist_free(avoid_seglist *sl)
{
    pf_edge *w;
    int index, i;

    index = sl - avoid_seglists;
    if (index < 0 || index >= AVOID_NUM_SEGLISTS || !(sl->flags & AVOID_SEGLIST_USED))
        return;
    for (i = 0; i < sl->num_corners; i++)
        avoid_link_block_free((avoid_link_block *)sl->corners[i].node.links);
    for (i = 0; i < sl->num_segs; i++) {
        if (sl->segs[i].wall) {
            sl->segs[i].wall->_pad0b--;
            if (sl->segs[i].wall->_pad0b == 0)
                sl->segs[i].wall->flags &= ~0x20;
        }
    }
    avoid_seglist_mark_free(sl);
}

// 0x4a7c70  Confidence: exact.
void avoid_poly_release(avoid_poly *poly)
{
    int index = poly - avoid_polys;
    if (index >= 0 && index < AVOID_NUM_POLYS && (poly->flags & 1))
        avoid_area_free(poly);
}

// 0x4a7cb0  Confidence: exact (m-1021c w9 D20201: the polygon search as for (;;) with the `++p >= N`
// return at the bottom of the body gives the binary's `jge` to the shared epilogue and `jmp` back). Takes an obstacle out of its polygon: frees the polygon, its segment
// list and the obstacle, re-adds the polygon's other obstacles one by one, then relinks the segment
// lists of the size class and layer (the timer_get(1000) results are discarded).
// GAME BUG (D7503): `others` holds 3 indices in the binary (frame 0x18, the array at +0xc), but a polygon
// merges up to 8 obstacles: removing one from a polygon of 5 or more writes the extra indices over the
// saved EDI / ESI / EBP / EBX of the caller (0x4a7d5d `mov [ebp], eax` in the copy loop).
void avoid_area_rebuild(avoid_obstacle *ob)
{
    avoid_poly *poly;
    int others[3];
    int index, size, layer, n, i, k, p;

    index = ob - avoid_obstacles;
    size = ob->size;
    layer = ob->layer;
    p = 0;
    for (;;) {
        poly = &avoid_polys[p];
        if ((poly->flags & 1) && poly->size == ob->size && poly->layer == ob->layer) {
            for (k = 0; k < poly->count; k++) {
                if (index == poly->obstacles[k])
                    goto found;
            }
        }
        if (++p >= AVOID_NUM_POLYS)
            return;
    }
found:
    n = 0;
    for (k = 0; k < poly->count; k++) {
        if (poly->obstacles[k] != index)
            others[n++] = poly->obstacles[k];
    }
    avoid_seglist_free(&avoid_seglists[poly->seglist]);
    avoid_poly_release(poly);
    avoid_obstacle_free(ob);
    for (i = 0; i < n; i++)
        avoid_create_obstacle_polys(&avoid_obstacles[others[i]], false);
    timer_get(1000);
    for (k = 0; k < AVOID_NUM_SEGLISTS; k++) {
        avoid_seglist *sl = &avoid_seglists[k];
        if ((sl->flags & AVOID_SEGLIST_USED) && sl->size == size && sl->layer == layer)
            avoid_do_poly_seg_list_links(sl);
    }
    timer_get(1000);
}

// 0x4a7e30  Confidence: exact (D11401: vector2 temporaries and in-place normals, one float for the cosine
// and the scale, the angle in float, swap an int). The miter offset at a corner where a list
// segment meets a nav wall: the bisector of the two normals scaled by 1 / sin(half the angle), clamped
// to 1.01..5 (1 when the corner is convex). swap exchanges the roles of the segment and the wall.
void avoid_seglist_calc_corner_offset(avoid_seglist *sl, avoid_seg_ref *seg, const pf_edge *wall, int corner,
                                      int swap)
{
    vector2 d1, d2, tmp;
    float dot, s;

    d1 = sl->corners[seg->b].pos - sl->corners[seg->a].pos;
    d1.normalize();
    vector2 n1(d1.y, -d1.x);
    seg->normal = n1;
    d2 = *wall->b - *wall->a;
    d2.normalize();
    if (wall->flags & PF_EDGE_SIDE)
        d2.negate();
    vector2 n2(d2.y, -d2.x);
    if (swap) {
        d1 = d2;
        tmp = n1;
        n1 = n2;
        n2 = tmp;
    }
    dot = n2.dot(d1);
    if (dot > 0.0f) {
        s = 1.0f;
    } else {
        s = -dot;
        if (s >= 1.0f)
            s = 1.0f;
        float ang;
        if (n2.dot(n1) > 0.0f)
            ang = PI_F - (float)asin(s);
        else
            ang = (float)asin(s);
        s = 1.0f / (float)sin(ang * 0.5f);
        if (s >= 5.0f)
            s = 5.0f;
        else if (s <= 1.01f)
            s = 1.01f;
    }
    vector2 bis = n2 + n1;
    bis.normalize();
    bis.x = bis.x * s;
    bis.y = bis.y * s;
    sl->corners[corner].offset = bis;
    sl->corners[corner].offset_state = -1;
}

// 0x4a8060  Confidence: exact (D11401, as avoid_seglist_calc_corner_offset). The same miter for a corner two segments share (of one list or of two
// lists): both corners get the offset and are marked done.
void avoid_seglist_calc_shared_corner_offset(avoid_seglist *sla, avoid_seglist *slb, avoid_seg_ref *sega,
                                             avoid_seg_ref *segb, int corner_a, int corner_b)
{
    vector2 da, db;
    float dot, s;

    da = sla->corners[sega->b].pos - sla->corners[sega->a].pos;
    da.normalize();
    vector2 na(da.y, -da.x);
    sega->normal = na;
    db = slb->corners[segb->b].pos - slb->corners[segb->a].pos;
    db.normalize();
    vector2 nb(db.y, -db.x);
    segb->normal = nb;
    dot = nb.dot(da);
    if (dot > 0.0f) {
        s = 1.0f;
    } else {
        s = -dot;
        if (s >= 1.0f)
            s = 1.0f;
        float ang;
        if (nb.dot(na) > 0.0f)
            ang = PI_F - (float)asin(s);
        else
            ang = (float)asin(s);
        s = 1.0f / (float)sin(ang * 0.5f);
        if (s >= 5.0f)
            s = 5.0f;
        else if (s <= 1.01f)
            s = 1.01f;
    }
    vector2 bis = nb + na;
    bis.normalize();
    bis.x = bis.x * s;
    bis.y = bis.y * s;
    sla->corners[corner_a].offset = bis;
    slb->corners[corner_b].offset = bis;
    sla->corners[corner_a].offset_state = 1;
    slb->corners[corner_b].offset_state = 1;
}

// 0x4a82a0  Confidence: exact (D11400: sl->corners[corner] indexed each time, no corner pointer). A corner on one segment gets offset (1, 1) (the
// pf_node_index lookup of the wall's end point is discarded); one on two segments the shared miter.
// Game bug: a corner on no segment reads segs[-1].
void avoid_seglist_ensure_corner_offset(avoid_seglist *sl, int corner)
{
    avoid_seg_ref *s1, *s2, *t;
    pf_edge *w;
    int first, second, i;

    if (sl->corners[corner].offset_state == 1)
        return;
    first = -1;
    second = -1;
    for (i = 0; i < sl->num_segs; i++) {
        if (sl->segs[i].a == corner || sl->segs[i].b == corner) {
            if (first == -1) {
                first = i;
            } else {
                second = i;
                break;
            }
        }
    }
    if (second == -1) {
        w = sl->segs[first].wall;
        if (sl->corners[corner].pos.x == w->a->x && sl->corners[corner].pos.y == w->a->y)
            pf_node_index(w->a);
        else
            pf_node_index(w->b);
        sl->corners[corner].offset.x = 1.0f;
        sl->corners[corner].offset.y = 1.0f;
        sl->corners[corner].offset_state = 1;
        return;
    }
    s1 = &sl->segs[first];
    s2 = &sl->segs[second];
    if (s1->b != corner) {
        t = s1;
        s1 = s2;
        s2 = t;
    }
    avoid_seglist_calc_shared_corner_offset(sl, sl, s1, s2, corner, corner);
}

// 0x4a8390  Confidence: exact (D21204: p through vec2_set, the door offset through one vector2 d (frame
// 0x24), and the other list's corner count in a local (the count-down loop); D22800: the door add as
// `p.x = p.x + d.x` and d.y through a float local added to p.y (p.y stays on the FPU), the corner's flags
// through sl->corners[k].node (the node pointer's bias), the other lists by an int index (signed end
// test), the out-of-memory exit as a break to the one restore, the graph point indexed in place).
// Links every corner of a segment list:
// its two neighbours along the outline, the visible corners of the other lists of the size class and
// layer, then the visible graph nodes, up to 20 links (the corner's link block).
void avoid_do_poly_seg_list_links(avoid_seglist *sl)
{
    avoid_seglist *o;
    avoid_corner *c;
    pf_vis_node *node, *other;
    pf_graph *g;
    vector2 p;
    int saved_size, layer, k, i, n, visible, found;

    saved_size = pf_cur_size;
    pf_cur_size = sl->size;
    layer = sl->layer;
    for (k = 0; k < sl->num_corners; k++) {
        node = &sl->corners[k].node;
        if (!node->links) {
            printf("do_poly_seg_list_links -- out of memory\n");
            break;
        }
        vec2_set(p, node->pos->x, node->pos->y);
        if (sl->flags & AVOID_SEGLIST_DOOR) {
            vector2 d = 0.03f * sl->corners[k].offset;
            p.x = p.x + d.x;
            float t = d.y;
            p.y = t + p.y;
        }
        visible = pf_mark_nodes_visible_from_point(&p, layer, 7);
        n = 0;
        if (sl->corners[k].node.flags & PFN_VISIBLE) {
            sl->corners[k].node.flags &= ~PFN_VISIBLE;
            visible--;
        }
        found = 0;
        for (i = 0; i < sl->num_segs; i++) {
            if (sl->segs[i].a == k) {
                found++;
                other = &sl->corners[sl->segs[i].b].node;
                node->links[n++] = other;
                node->num_links = (short)n;
                if (other->flags & PFN_VISIBLE)
                    other->flags &= ~PFN_VISIBLE;
                else
                    visible++;
            }
            if (sl->segs[i].b == k) {
                found++;
                other = &sl->corners[sl->segs[i].a].node;
                node->links[n++] = other;
                node->num_links = (short)n;
                if (other->flags & PFN_VISIBLE)
                    other->flags &= ~PFN_VISIBLE;
                else
                    visible++;
            }
            if (found == 2)
                break;
        }
        for (int j = 0; j < AVOID_NUM_SEGLISTS; j++) {
            o = &avoid_seglists[j];
            if (!(o->flags & AVOID_SEGLIST_USED) || (o->flags & AVOID_SEGLIST_OFF) || o->layer != layer
                || o->size != pf_cur_size)
                continue;
            int nc = o->num_corners;
            for (i = 0; i < nc; i++) {
                c = &o->corners[i];
                if ((c->node.flags & PFN_VISIBLE) && n < 20) {
                    node->links[n++] = &c->node;
                    node->num_links = (short)n;
                }
            }
        }
        g = &pf_graphs[pf_cur_size][layer];
        for (i = 0; i < g->count; i++) {
            if (g->points[i].flags & PFN_VISIBLE) {
                node->links[n++] = &g->points[i];
                node->num_links = (short)n;
                if (n >= 20 || n == visible)
                    break;
            }
        }
    }
    pf_cur_size = saved_size;
}

// 0x4a8600  Confidence: exact.
void rect2d_add_point(rect2d *r, const vector2 *p)
{
    r->min_x = min_f(r->min_x, p->x);
    r->max_x = max_f(r->max_x, p->x);
    r->max_y = max_f(r->max_y, p->y);
    r->min_y = min_f(r->min_y, p->y);
}

// 0x4a8680  Confidence: likely (the reuse of sl's argument slot for b's corner index; D22804: ws as a
// pf_edge, 264 -> 262; ib still does not take sl's slot). Appends the
// segment a-b (reusing corners at the same points). Soft lists compute the miter at the ends that lie on
// walls; prohibit lists add a closing segment along each wall to the wall's end point on a's side.
// Returns -1 when the 30 corners are used up.
int avoid_seglist_add_segment(const vector2 *a, const vector2 *b, pf_edge *wall_a, pf_edge *wall_b,
                              avoid_seglist *sl)
{
    pf_edge ws;
    vector2 *ep;
    int ia, ib, nc, ns, i, idx;

    ia = -1;
    ib = -1;
    nc = sl->num_corners;
    ns = sl->num_segs;
    for (i = nc - 1; i >= 0; i--) {
        if (a->x == sl->corners[i].pos.x && a->y == sl->corners[i].pos.y) {
            ia = i;
            break;
        }
    }
    for (i = nc - 1; i >= 0; i--) {
        if (b->x == sl->corners[i].pos.x && b->y == sl->corners[i].pos.y) {
            ib = i;
            break;
        }
    }
    if (ia == -1) {
        if (nc == AVOID_SEGLIST_MAX)
            return -1;
        ia = nc++;
        sl->corners[ia].pos.x = a->x;
        sl->corners[ia].pos.y = a->y;
    }
    if (ib == -1) {
        if (nc == AVOID_SEGLIST_MAX)
            return -1;
        ib = nc++;
        sl->corners[ib].pos.x = b->x;
        sl->corners[ib].pos.y = b->y;
    }
    sl->segs[ns].a = (short)ia;
    sl->segs[ns].b = (short)ib;
    sl->segs[ns].wall = NULL;
    ns++;
    rect2d_add_point(&sl->bounds, &sl->corners[ia].pos);
    rect2d_add_point(&sl->bounds, &sl->corners[ib].pos);
    if (sl->flags & AVOID_SEGLIST_AVOID) {
        if (wall_a)
            avoid_seglist_calc_corner_offset(sl, &sl->segs[ns - 1], wall_a, sl->segs[ns - 1].a, true);
        if (wall_b)
            avoid_seglist_calc_corner_offset(sl, &sl->segs[ns - 1], wall_b, sl->segs[ns - 1].b, false);
    } else {
        if (wall_a) {
            ws.a = (vector2 *)a;
            ws.b = (vector2 *)b;
            ep = seg2d_point_left_of(wall_a->a, &ws, 0) ? wall_a->a : wall_a->b;
            if (nc == AVOID_SEGLIST_MAX)
                return -1;
            idx = nc++;
            sl->corners[idx].pos.x = ep->x;
            sl->corners[idx].pos.y = ep->y;
            rect2d_add_point(&sl->bounds, &sl->corners[idx].pos);
            sl->segs[ns].a = (short)idx;
            sl->segs[ns].b = (short)ia;
            sl->segs[ns].side = 0;
            sl->segs[ns].wall = wall_a;
            ns++;
        }
        if (wall_b) {
            ws.a = (vector2 *)a;
            ws.b = (vector2 *)b;
            ep = seg2d_point_left_of(wall_b->a, &ws, 0) ? wall_b->a : wall_b->b;
            if (nc == AVOID_SEGLIST_MAX)
                return -1;
            idx = nc++;
            sl->corners[idx].pos.x = ep->x;
            sl->corners[idx].pos.y = ep->y;
            rect2d_add_point(&sl->bounds, &sl->corners[idx].pos);
            sl->segs[ns].a = (short)ib;
            sl->segs[ns].b = (short)idx;
            sl->segs[ns].side = 1;
            sl->segs[ns].wall = wall_b;
            ns++;
        }
    }
    sl->num_corners = (short)nc;
    sl->num_segs = (short)ns;
    return 0;
}

// 0x4a89b0  Confidence: exact.
bool ptr_array_contains(const void *value, void *const *array, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        if (array[i] == value)
            return true;
    }
    return false;
}

// 0x4a89e0  Confidence: exact. Every segment of the used prohibit lists of the size class and layer
// that ends on `wall`.
void avoid_collect_segment_refs(int *count, avoid_seg_ref_entry *refs, const pf_edge *wall, int size, int layer)
{
    int k, i;

    for (k = 0; k < AVOID_NUM_SEGLISTS; k++) {
        avoid_seglist *sl = &avoid_seglists[k];
        if (!(sl->flags & AVOID_SEGLIST_USED) || !(sl->flags & AVOID_SEGLIST_DOOR) || (sl->flags & AVOID_SEGLIST_OFF)
            || sl->layer != layer || sl->size != size)
            continue;
        for (i = 0; i < sl->num_segs; i++) {
            if (sl->segs[i].wall == wall) {
                avoid_seg_ref_entry *r = &refs[(*count)++];
                r->index = i;
                r->list = sl;
            }
        }
    }
}

// 0x4a8a60  Confidence: exact. qsort comparator of the leading float (never 0).
int qsort_cmp_float_asc(const void *a, const void *b)
{
    if (*(const float *)a < *(const float *)b)
        return -1;
    return 1;
}

// 0x4a8a80  Confidence: exact.
avoid_seg_ref *avoid_ref_entry(const avoid_seg_ref_entry *e)
{
    return &e->list->segs[e->index];
}

// 0x4a8aa0  Confidence: exact (D15500: the corner pointer as if / else). Sorts the references by where their wall-side corner lies along the
// wall (major axis; reversed for walls with PF_EDGE_SIDE).
void avoid_sort_refs_along_segment(int count, avoid_seg_ref_entry *refs)
{
    avoid_seg_ref *s;
    pf_edge *w;
    vector2 *p;
    float dx, dy, t;
    int use_x, i;

    w = avoid_ref_entry(refs)->wall;
    use_x = 0;
    dx = w->b->x - w->a->x;
    dy = w->b->y - w->a->y;
    if (fabsf(dx) > fabsf(dy))
        use_x = 1;
    for (i = 0; i < count; i++) {
        s = avoid_ref_entry(&refs[i]);
        if (s->side)
            p = &refs[i].list->corners[s->a].pos;
        else
            p = &refs[i].list->corners[s->b].pos;
        if (use_x)
            t = (p->x - w->a->x) / dx;
        else
            t = (p->y - w->a->y) / dy;
        if (w->flags & PF_EDGE_SIDE)
            t = 1.0f - t;
        refs[i].t = t;
    }
    qsort(refs, count, sizeof(avoid_seg_ref_entry), qsort_cmp_float_asc);
}

// 0x4a8b80  Confidence: likely (the "previous segment" index arithmetic, segs[index - 1] or
// segs[index - 2], is the code's; D22802: the corner point copied as one vector2 assignment, which lets
// 8966 cross-jump the two calc_shared_corner_offset calls as the binary does, 191 -> 128). Where several prohibit lists end on the same nav wall, the corners
// that lie between two neighbouring references along the wall are joined (the later one takes the
// earlier one's point) and get a shared miter.
void avoid_merge_shared_segments(avoid_seglist *sl)
{
    pf_edge *done[42];
    avoid_seg_ref_entry refs[39];
    avoid_seg_ref *e, *sp;
    avoid_seglist *l;
    avoid_corner *p;
    pf_edge *w;
    int n_done, count, i, k, c, cc;

    n_done = 0;
    for (i = 0; i < sl->num_segs; i++) {
        w = sl->segs[i].wall;
        if (!w || (char)w->_pad0b < 2 || ptr_array_contains(w, (void *const *)done, n_done))
            continue;
        count = 0;
        avoid_collect_segment_refs(&count, refs, w, sl->size, sl->layer);
        if (count != 1) {
            avoid_sort_refs_along_segment(count, refs);
            for (k = 0; k < count; k++) {
                e = avoid_ref_entry(&refs[k]);
                if (!e->side) {
                    if (k == 0)
                        continue;
                    c = avoid_ref_entry(&refs[k - 1])->a;
                    p = &refs[k - 1].list->corners[c];
                    cc = avoid_ref_entry(&refs[k])->a;
                    refs[k].list->corners[cc].pos = p->pos;
                    l = refs[k - 1].list;
                    sp = &l->segs[refs[k - 1].index - 1];
                    if (sp->b != c)
                        sp = &l->segs[refs[k - 1].index - 2];
                    avoid_seglist_calc_shared_corner_offset(l, refs[k].list, sp, avoid_ref_entry(&refs[k]), sp->b, cc);
                } else {
                    if (k == count - 1)
                        continue;
                    c = avoid_ref_entry(&refs[k + 1])->b;
                    p = &refs[k + 1].list->corners[c];
                    cc = avoid_ref_entry(&refs[k])->b;
                    refs[k].list->corners[cc].pos = p->pos;
                    l = refs[k + 1].list;
                    sp = &l->segs[refs[k + 1].index - 1];
                    if (sp->a != c)
                        sp = &l->segs[refs[k + 1].index - 2];
                    avoid_seglist_calc_shared_corner_offset(refs[k].list, l, avoid_ref_entry(&refs[k]), sp, cc, sp->a);
                }
            }
        }
        done[n_done++] = w;
    }
}

// The wall crossings of one polygon edge (8 bytes; avoid_clip_segments' stack, sorted by t).
struct avoid_clip_hit {
    float t;
    pf_edge *wall;
};

// 0x4a8df0  Confidence: likely (the two identical add_segment calls of the inside test are the code's;
// the hit array size is a guess). Clips the polygon outline against the nav walls of its size class and
// layer: the pieces inside the navigable area become the segments of a new segment list (pieces that end
// on a wall reference it and raise its avoidance count). An odd crossing count fails (-1). Then the
// corner miters, the corners' nodes (the pushed point for soft lists, the point itself for prohibit
// ones) and their link blocks, and with `link` the visibility links of every list of the size class and
// layer. Returns the list's index.
int avoid_clip_segments(avoid_poly *poly, bool link)
{
    avoid_clip_hit hits[98];
    avoid_seglist *sl, *o;
    avoid_seg_ref *s;
    avoid_corner *c;
    pf_grid_cell *cell;
    pf_edge *w, *wprev;
    vector2 *prev, *cur, *pp;
    vector2 from, hit;
    vector3 v;
    float t, u;
    int layer, size, total, nhits, n, id, i, j, k, m;
    bool inside;

    layer = poly->layer;
    size = poly->size;
    sl = avoid_seglist_alloc();
    if (!sl)
        return -1;
    sl->num_corners = 0;
    sl->num_segs = 0;
    sl->size = poly->size;
    sl->layer = poly->layer;
    sl->flags = AVOID_SEGLIST_USED;
    sl->poly = poly - avoid_polys;
    sl->bounds.min_x = FLT_MAX;
    sl->bounds.max_x = -FLT_MAX;
    sl->bounds.min_y = FLT_MAX;
    sl->bounds.max_y = -FLT_MAX;
    sl->flags = (poly->flags & 4) ? AVOID_SEGLIST_USED | AVOID_SEGLIST_AVOID : AVOID_SEGLIST_USED | AVOID_SEGLIST_DOOR;
    prev = &poly->points[poly->num_points - 1];
    v.x = prev->x;
    v.z = prev->y;
    total = 0;
    v.y = 0.0f;
    inside = pf_point_is_navigable(&v, size, layer) != 0;
    for (i = 0; i < poly->num_points; i++) {
        cur = &poly->points[i];
        nhits = 0;
        n = pf_cells_along_segment(layer, size, prev, cur);
        id = pf_next_search_id();
        for (j = 0; j < n; j++) {
            cell = pf_grid_cell_results[j];
            for (m = 0; m < cell->num_walls; m++) {
                w = cell->walls[m];
                if ((w->flags & PF_EDGE_DOOR) || w->search_id == id)
                    continue;
                w->search_id = id;
                if (seg2d_intersect_params(prev, cur, w->a, w->b, &t, &u) && seg_params_in_range(&t, &u)) {
                    hits[nhits].t = t;
                    hits[nhits].wall = w;
                    nhits++;
                    total++;
                }
            }
        }
        from.x = prev->x;
        from.y = prev->y;
        if (nhits == 0) {
            if (inside && avoid_seglist_add_segment(&from, cur, NULL, NULL, sl) == -1)
                goto fail;
        } else {
            if (nhits > 1)
                qsort(hits, nhits, sizeof(avoid_clip_hit), qsort_cmp_float_asc);
            for (k = 0; k < nhits; k++) {
                t = hits[k].t;
                hit.x = (cur->x - prev->x) * t + prev->x;
                hit.y = (cur->y - prev->y) * t + prev->y;
                wprev = k == 0 ? NULL : hits[k - 1].wall;
                if (inside) {
                    if (sl->flags & AVOID_SEGLIST_DOOR) {
                        if (avoid_seglist_add_segment(&from, &hit, wprev, hits[k].wall, sl) == -1)
                            goto fail;
                    } else {
                        if (avoid_seglist_add_segment(&from, &hit, wprev, hits[k].wall, sl) == -1)
                            goto fail;
                    }
                }
                from.x = hit.x;
                from.y = hit.y;
                inside = !inside;
            }
            if (inside && avoid_seglist_add_segment(&from, cur, hits[nhits - 1].wall, NULL, sl) == -1)
                goto fail;
        }
        prev = cur;
    }
    if (total % 2 != 0) {
fail:
        avoid_seglist_mark_free(sl);
        return -1;
    }
    for (k = 0; k < sl->num_segs; k++) {
        s = &sl->segs[k];
        if (s->wall) {
            s->wall->flags |= 0x20;
            s->wall->_pad0b++;
            if (avoid_backup_seg_count <= 99)
                avoid_backup_segs[avoid_backup_seg_count++] = s->wall;
            else
                printf("ran out of backup segs");
        }
        sl->corners[s->a].offset_state = sl->corners[s->a].offset_state == -1;
        sl->corners[s->b].offset_state = sl->corners[s->b].offset_state == -1;
    }
    if ((sl->flags & AVOID_SEGLIST_DOOR) && total > 0)
        avoid_merge_shared_segments(sl);
    for (k = 0; k < sl->num_segs; k++) {
        avoid_seglist_ensure_corner_offset(sl, sl->segs[k].a);
        avoid_seglist_ensure_corner_offset(sl, sl->segs[k].b);
    }
    for (k = 0; k < sl->num_corners; k++) {
        c = &sl->corners[k];
        if (sl->flags & AVOID_SEGLIST_DOOR) {
            pp = &c->pos;
        } else {
            c->pushed.x = c->offset.x * 0.03f + c->pos.x;
            c->pushed.y = c->offset.y * 0.03f + c->pos.y;
            pp = &c->pushed;
        }
        c->node.pos = pp;
        c->node._pad0a = 0;
        c->node.num_links = 0;
        c->node.layer = (short)layer;
        c->node.flags = PFN_SKIP_DOOR_LISTS;
        c->node.links = (pf_vis_node **)avoid_link_block_alloc();
    }
    if (link) {
        timer_get(1000);
        for (o = avoid_seglists; o < &avoid_seglists[AVOID_NUM_SEGLISTS]; o++) {
            if ((o->flags & AVOID_SEGLIST_USED) && !(o->flags & AVOID_SEGLIST_OFF) && o->size == sl->size
                && o->layer == sl->layer)
                avoid_do_poly_seg_list_links(o);
        }
        timer_get(1000);
    }
    return sl - avoid_seglists;
}

// 0x4a93c0  Confidence: exact. qsort comparator of polygon indices: more points first.
int avoid_qsort_cmp_poly_word(const void *a, const void *b)
{
    return avoid_polys[*(const int *)b].num_points - avoid_polys[*(const int *)a].num_points;
}

// 0x4a9400  Confidence: exact. Undoes the reference counts raised since the last avoid_backup_seg_count
// reset (after a failed avoid_clip_segments).
void avoid_age_segments()
{
    pf_edge *w;
    int i;

    for (i = 0; i < avoid_backup_seg_count; i++) {
        w = avoid_backup_segs[i];
        if ((char)w->_pad0b > 0)
            w->_pad0b--;
        if (w->_pad0b == 0)
            w->flags &= ~0x20;
    }
}

// 0x4a9440  Confidence: likely (the repeated merge pass and the list bookkeeping). Builds the polygon of
// a new obstacle: alone when it overlaps no polygon of its kind, size class and layer; else merged with
// the overlapping polygons (at most 3 and at most 3 obstacles in all), which are freed once the new
// polygon's segment list exists. Game bug: when a merged polygon fails to clip, only the first merged
// polygon's list gets AVOID_SEGLIST_OFF back off; the others stay switched off.
bool avoid_create_obstacle_polys(avoid_obstacle *ob, bool link)
{
    avoid_poly *poly, *p, *q;
    avoid_obstacle *o;
    int prohibit_list[14], avoid_list[14];
    int *list;
    int prohibit, n_prohibit, n_avoid, n, total, i, k, m, c, r;

    prohibit = (char)~ob->flags & AVOID_OB_AVOID;
    prohibit >>= 2;
    n_prohibit = 0;
    n_avoid = 0;
    for (i = 0; i < AVOID_NUM_POLYS; i++) {
        p = &avoid_polys[i];
        if (!(p->flags & 1) || p->size != ob->size || p->layer != ob->layer || !rect2d_overlap(p->bounds, ob->bounds))
            continue;
        for (k = 0; k < p->count; k++) {
            o = &avoid_obstacles[(int)p->obstacles[k]];
            if (rect2d_overlap(o->bounds, ob->bounds) && avoid_obstacles_overlap(o, ob)) {
                if (p->flags & AVOID_OB_PROHIBIT)
                    prohibit_list[n_prohibit++] = i;
                else
                    avoid_list[n_avoid++] = i;
                break;
            }
        }
    }
    if (prohibit == 1) {
        n = n_prohibit;
        list = prohibit_list;
    } else {
        n = n_avoid;
        list = avoid_list;
    }
    poly = avoid_poly_alloc();
    if (!poly)
        return false;
    if (n == 0) {
        if (!avoid_poly_from_obstacle(poly, ob))
            return false;
        poly->seglist = (short)avoid_clip_segments(poly, link);
        if (poly->seglist == -1) {
            avoid_area_free(poly);
            return false;
        }
        return true;
    }
    if (n == 1) {
        if (avoid_polys[list[0]].count > 3)
            return false;
        memcpy((void *)poly, &avoid_polys[list[0]], sizeof(avoid_poly));
        avoid_backup_seg_count = 0;
        if (!avoid_merge_obstacle_polys(poly, ob)) {
            printf("add_primative_to_avoid_poly failed\n");
            avoid_area_free(poly);
            return false;
        }
        avoid_seglists[avoid_polys[list[0]].seglist].flags |= AVOID_SEGLIST_OFF;
        r = (short)avoid_clip_segments(poly, link);
        if (r < 0)
            goto clip_failed;
        goto clipped;
    }
    if (n > 3) {
        avoid_area_free(poly);
        return false;
    }
    total = 0;
    for (i = 0; i < n; i++)
        total += avoid_polys[list[i]].count;
    if (total > 3) {
        avoid_area_free(poly);
        return false;
    }
    qsort(list, n, sizeof(int), avoid_qsort_cmp_poly_word);
    memcpy((void *)poly, &avoid_polys[list[0]], sizeof(avoid_poly));
    if (!avoid_merge_obstacle_polys(poly, ob)) {
        avoid_area_free(poly);
        return false;
    }
    for (i = 1; i < n; i++) {
        q = &avoid_polys[list[i]];
        for (c = 0; c < q->count; c++) {
            for (k = 0; k < q->count; k++) {
                if (q->obstacles[k] == -1)
                    continue;
                o = &avoid_obstacles[(int)q->obstacles[k]];
                if (!rect2d_overlap(poly->bounds, o->bounds) || poly->count <= 0)
                    continue;
                for (m = 0; m < poly->count; m++) {
                    if (!rect2d_overlap(avoid_obstacles[(int)poly->obstacles[m]].bounds, o->bounds))
                        continue;
                    if (avoid_obstacles_overlap(&avoid_obstacles[(int)poly->obstacles[m]], o)) {
                        q->obstacles[k] = -1;
                        avoid_merge_obstacle_polys(poly, o);
                        break;
                    }
                    printf("bad intersection -- multiple avoid_polys\n");
                }
            }
        }
    }
    for (i = 0; i < n; i++)
        avoid_seglists[avoid_polys[list[i]].seglist].flags |= AVOID_SEGLIST_OFF;
    r = avoid_clip_segments(poly, link);
    if (r < 0)
        goto clip_failed;
clipped:
    poly->seglist = (short)r;
    for (i = 0; i < n; i++) {
        avoid_seglist_free(&avoid_seglists[avoid_polys[list[i]].seglist]);
        avoid_area_free(&avoid_polys[list[i]]);
    }
    return true;
clip_failed:
    printf("create_obstacle_poly_from_avoid_poly failed\n");
    avoid_age_segments();
    avoid_seglists[avoid_polys[list[0]].seglist].flags &= ~AVOID_SEGLIST_OFF;
    avoid_area_free(poly);
    return false;
}

// The x / z of a vector3 into a vector2 through float arguments: avoid_area_add copies them with fld / fstp
// (an inline with float parameters, as pathfinding.cpp's pf_set_xz, D8601; plain member copies are integer
// moves, a vector2(x, z) temporary is copied through EAX). Name ours.
static inline void avoid_set_xz(vector2 *v, float x, float z)
{
    v->x = x;
    v->y = z;
}

// 0x4a9b40  Confidence: exact (VC6 byte match, D9701: vector2 operators; the descriptor's centre and axes
// are read as vector2s, D8601). Registers an avoidance area: one obstacle for size class 1 (centre
// jittered by up to 0.3, first axis by up to 0.1, a square frame) and one for size class 2 (the
// description's axes, half sizes + _unk28). Returns the slot or -1.
int avoid_area_add(avoid_area_desc *d)
{
    avoid_area *area;
    vector3 rv;
    vector2 r, centre, axis, ha, hb;
    int flags, ob;

    area = avoid_area_alloc();
    if (!area)
        return -1;
    flags = (d->shape != 0) + 1;
    if (d->_unk00 == 0)
        flags |= 8;
    else
        flags |= 4;
    rv.random_direction();
    avoid_set_xz(&r, rv.x, rv.z);
    r.normalize();
    centre = 0.3f * r + *(vector2 *)&d->cx;
    rv.random_direction();
    avoid_set_xz(&r, rv.x, rv.z);
    r.normalize();
    axis = 0.1f * r + *(vector2 *)&d->ax;
    axis.normalize();
    vector2 perp(-axis.y, axis.x);
    ha = d->extent_a * axis;
    hb = d->extent_b * perp;
    ob = avoid_add_primitive(&centre, flags, &ha, &hb, 1, d->owner);
    if (ob >= 0)
        area->obstacles[area->count++] = (char)ob;
    else
        printf("AA - primative not added!!\n");
    ha = (d->_unk28 + d->extent_a) * *(vector2 *)&d->ax;
    hb = (d->_unk28 + d->extent_b) * *(vector2 *)&d->bx;
    ob = avoid_add_primitive(&centre, flags, &ha, &hb, 2, d->owner);
    if (ob >= 0)
        area->obstacles[area->count++] = (char)ob;
    else
        printf("AA - primative not added!!\n");
    if (area->count == 0) {
        avoid_area_free(area);
        return -1;
    }
    return area - avoid_areas;
}

// 0x4a9d60  Confidence: exact. pathfinding_load_graph and "avoidance reset".
void avoid_reset()
{
    memset(avoid_polys, 0, sizeof(avoid_polys));
    memset(avoid_obstacles, 0, sizeof(avoid_obstacles));
    memset(avoid_seglists, 0, sizeof(avoid_seglists));
    memset(avoid_link_blocks, 0, sizeof(avoid_link_blocks));
    memset(avoid_areas, 0, sizeof(avoid_areas));
    memset(avoid_obstacle_counts_b, 0, sizeof(avoid_obstacle_counts_b));
    memset(avoid_obstacle_counts, 0, sizeof(avoid_obstacle_counts));
    memset(avoid_spline_point_counts, 0, sizeof(avoid_spline_point_counts));
    memset(avoid_spline_points, 0, sizeof(avoid_spline_points));
    memset(avoid_splines, 0, sizeof(avoid_splines));
    avoid_backup_seg_count = 0;
}

// 0x4a9df0  Confidence: exact. fx_obj_destroy, summon_emerge_update. A slot with an obstacle that is
// already free is left in place (returns before avoid_area_free).
void avoid_area_remove(int slot)
{
    avoid_area *area;
    avoid_obstacle *ob;
    int i;

    if (slot < 0 || slot >= AVOID_NUM_AREAS)
        return;
    area = &avoid_areas[slot];
    if (!(area->flags & 1) || avoidance_no_remove)
        return;
    for (i = 0; i < area->count; i++) {
        ob = &avoid_obstacles[(int)area->obstacles[i]];
        if (!(ob->flags & AVOID_OB_USED))
            return;
        avoid_area_rebuild(ob);
    }
    avoid_area_free(area);
}

// 0x4a9e60  Confidence: exact.
bool avoid_obstacles_overlap(const avoid_obstacle *a, const avoid_obstacle *b)
{
    int a_box = a->flags & AVOID_OB_BOX;
    int b_box = b->flags & AVOID_OB_BOX;

    if (a_box && b_box)
        return avoid_box_box_overlap(a, b);
    if (a_box)
        return avoid_poly_points_in_box(a, b);
    if (b_box)
        return avoid_poly_points_in_box(b, a);
    return avoid_poly_points_in_circle(a, b);
}

// 0x4a9eb0  Confidence: likely (the circle branch's last point and axes go through the vector2
// operators, as the code calls them). An obstacle from a centre and two half axes: a box (flags 1) or an
// octagon of radius |a| (flags 2), prohibit (flags 4) or avoid (else); then its polygon
// (avoid_create_obstacle_polys with links). Returns the obstacle index or -1.
int avoid_add_primitive(const vector2 *centre, int flags, const vector2 *a, const vector2 *b, int size,
                        int layer)
{
    avoid_obstacle *ob;
    vector2 ta, tb, kb, ca, sum;
    rect2d r;
    char *count;
    int i;
    bool ok;

    if (!avoidance_enabled)
        return -1;
    timer_get(1000);
    ob = avoid_obstacle_alloc();
    if (!ob)
        return -1;
    ob->centre.x = centre->x;
    ob->centre.y = centre->y;
    ob->size = (short)size;
    ob->layer = (short)layer;
    if (flags & 4) {
        ob->flags = AVOID_OB_PROHIBIT;
        count = &avoid_obstacle_counts[size * PF_MAX_LAYERS + layer];
    } else {
        ob->flags = AVOID_OB_AVOID;
        count = &avoid_obstacle_counts_b[size * PF_MAX_LAYERS + layer];
    }
    (*count)++;
    if (flags & 1) {
        ob->flags |= AVOID_OB_BOX | AVOID_OB_USED;
        ob->num_points = 4;
        ob->points[0].x = centre->x + a->x + b->x;
        ob->points[0].y = a->y + centre->y + b->y;
        ob->points[1].x = centre->x - a->x + b->x;
        ob->points[1].y = centre->y - a->y + b->y;
        ob->points[2].x = centre->x - a->x - b->x;
        ob->points[2].y = centre->y - a->y - b->y;
        ob->points[3].x = centre->x + a->x - b->x;
        ob->points[3].y = a->y + centre->y - b->y;
        ta.x = a->x;
        ta.y = a->y;
        tb.x = b->x;
        tb.y = b->y;
        ob->half_a = ta.normalize();
        ob->axis_a.x = ta.x;
        ob->axis_a.y = ta.y;
        ob->half_b = tb.normalize();
        ob->axis_b.x = tb.x;
        ob->axis_b.y = tb.y;
    } else {
        ob->flags |= AVOID_OB_CIRCLE | AVOID_OB_USED;
        ob->num_points = 8;
        ob->points[0].x = a->x * 0.4142135f + centre->x + b->x;
        ob->points[0].y = a->y * 0.4142135f + centre->y + b->y;
        ob->points[1].x = centre->x - a->x * 0.4142135f + b->x;
        ob->points[1].y = centre->y - a->y * 0.4142135f + b->y;
        ob->points[2].x = centre->x - a->x + b->x * 0.4142135f;
        ob->points[2].y = centre->y - a->y + b->y * 0.4142135f;
        ob->points[3].x = centre->x - a->x - b->x * 0.4142135f;
        ob->points[3].y = centre->y - a->y - b->y * 0.4142135f;
        ob->points[4].x = centre->x - a->x * 0.4142135f - b->x;
        ob->points[4].y = centre->y - a->y * 0.4142135f - b->y;
        ob->points[5].x = a->x * 0.4142135f + centre->x - b->x;
        ob->points[5].y = a->y * 0.4142135f + centre->y - b->y;
        ob->points[6].x = centre->x + a->x - b->x * 0.4142135f;
        ob->points[6].y = a->y + centre->y - b->y * 0.4142135f;
        kb.x = b->x * 0.4142135f;
        kb.y = b->y * 0.4142135f;
        ca.x = centre->x + a->x;
        ca.y = a->y + centre->y;
        ob->points[7] = ca + kb;
        ta = *a;
        ob->half_a = ob->half_b = ta.normalize();
        ob->axis_a = ta;
        ob->axis_b.x = -ta.y;
        ob->axis_b.y = ta.x;
        sum = ob->axis_a + ob->axis_b;
        sum.normalize();
        ob->axis_c = sum;
        ob->axis_d.x = -sum.y;
        ob->axis_d.y = sum.x;
    }
    r.max_x = -FLT_MAX;
    r.max_y = -FLT_MAX;
    r.min_x = FLT_MAX;
    r.min_y = FLT_MAX;
    for (i = 0; i < ob->num_points; i++) {
        r.max_x = max_f(r.max_x, ob->points[i].x);
        r.min_x = min_f(r.min_x, ob->points[i].x);
        r.max_y = max_f(r.max_y, ob->points[i].y);
        r.min_y = min_f(r.min_y, ob->points[i].y);
    }
    ob->bounds.max_y = r.max_y;
    ob->bounds.min_y = r.min_y;
    ob->bounds.min_x = r.min_x;
    ob->bounds.max_x = r.max_x;
    ok = avoid_create_obstacle_polys(ob, true);
    timer_get(1000);
    if (!ok) {
        avoid_obstacle_free(ob);
        return -1;
    }
    return ob - avoid_obstacles;
}

// 0x4aa3a0 vector2::operator=  Confidence: exact.
// 0x4aa3c0 vector2::operator+  Confidence: likely (y is computed first).
// COMDATs of the inline members of vsdk/math/vector.h (D3706), emitted here because this unit does not
// expand inline functions.

// 0x4aa3e0  Confidence: exact.
avoid_poly *avoid_poly_get(avoid_seglist *s)
{
    return &avoid_polys[s->poly];
}

// 0x4aa400  Confidence: likely (the hit count and the early exits; the hit point as vector2 operators,
// D9803; D22801: the previous point by an index j (`j = i` at the loop end), `hits[nhits++]`, and each
// length as `(p - q).len()` on its own temporary (three separate exits), distance 167 -> 8; left: the
// binary stores a float 0 into i's slot before the overlap call and its early exit loads it from there).
// The extra path length of an A* edge through an obstacle: the whole edge when both ends are inside,
// else the length of the part inside (two outline crossings, or the inside end to the one crossing); 0 when
// the edge misses the obstacle.
float avoid_poly_add_search_nodes(const avoid_prim *seg, const avoid_obstacle *ob)
{
    vector2 hits[2];
    const vector2 *a, *b;
    float t, u;
    int inside, nhits, i, j;
    bool a_inside;

    if (!avoid_prim_overlap(ob, seg, 0.0f))
        return 0.0f;
    a = &seg->points[0];
    inside = 0;
    a_inside = false;
    if (avoid_prim_test(a, ob, 0.0f)) {
        a_inside = true;
        inside = 1;
    }
    b = &seg->points[1];
    if (avoid_prim_test(b, ob, 0.0f))
        inside++;
    if (inside == 2)
        return seg->half_a;
    nhits = 0;
    j = ob->num_points - 1;
    for (i = 0; i < ob->num_points; i++) {
        if (seg2d_intersect_params(a, b, &ob->points[j], &ob->points[i], &t, &u) && u >= 0.0f && u <= 1.0f
            && t >= 0.0f && t <= 1.0f) {
            hits[nhits++] = t * (*b - *a) + *a;
            if (nhits == 2)
                break;
            if (inside == 1 && nhits == 1)
                break;
        }
        j = i;
    }
    if (nhits == 2) {
        return (hits[1] - hits[0]).len();
    }
    if (nhits == 1) {
        if (a_inside)
            return (hits[0] - *a).len();
        return (hits[0] - *b).len();
    }
    return 0.0f;
}

// 0x4aa630  Confidence: exact. world_click_move: the x / z point is inside a prohibit obstacle of the
// layer and size class (margin 0.05).
bool avoid_point_blocked(const vector3 *pos, int layer, int size)
{
    int i;
    vector2 p(pos->x, pos->z);

    for (i = 0; i < AVOID_NUM_OBSTACLES; i++) {
        avoid_obstacle *ob = &avoid_obstacles[i];
        if (((ubyte)ob->flags & AVOID_OB_USED) && ((ubyte)ob->flags & AVOID_OB_PROHIBIT) && ob->layer == layer
            && ob->size == size && avoid_prim_test(&p, ob, 0.05f))
            return true;
    }
    return false;
}

// 0x4aa6b0  Confidence: exact. fx_frame: the point is inside one of the area's obstacles of the layer
// and size class (margin -0.25).
bool avoid_area_contains_point(const vector3 *pos, int layer, int size, int area)
{
    avoid_area *aa;
    avoid_obstacle *ob;
    int i;

    vector2 p(pos->x, pos->z);
    aa = &avoid_areas[area];
    if (!(aa->flags & 1))
        return false;
    for (i = 0; i < aa->count; i++) {
        ob = &avoid_obstacles[(int)aa->obstacles[i]];
        if ((ob->flags & AVOID_OB_USED) && layer == ob->layer && size == ob->size
            && avoid_prim_test(&p, ob, -0.25f))
            return true;
    }
    return false;
}

// 0x4aa760  Confidence: exact. obstacles_debug_render.
vector2 *avoid_record_axis_at(avoid_seglist *sl, int corner)
{
    return &sl->corners[corner].offset;
}
