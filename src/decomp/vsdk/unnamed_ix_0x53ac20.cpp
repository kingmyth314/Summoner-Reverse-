// unnamed_ix_0x53ac20.cpp - Sum.exe v1.40, code 0x53ac20-0x53debc: the ix_* intersection tests (vsdk\math
// in sum.symbols; Dash Faction's rf/math/ix.h gives the RF module's names only). The file name is a
// placeholder: no path string (the only strings are the profile labels "ix_linesegment_box_oriented",
// "ix_ray_box_oriented", "ix_ray_boundingbox", "ix_ray_sphere"), no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: after gr_direct3d.cpp (its IsEqualGUID COMDAT 0x53ac00) and before cmdline.cpp's initialiser
// group (0x53dec0); .data 0x5abe9c-0x5abf05 (the axis pair table, then the four labels; cmdline.cpp's " "
// is at 0x5abf08), .bss 0x2dc6cc0-0x2dc6ea7 (the function-local statics and their guard bytes, after
// gr_direct3d.cpp's 0x2dc6cbb; cmdline_args 0x2dc6ea8 follows). docs/sum-exe/source-map.md: "one unit, no cut".
//
// Build flags: no inline expansion (the vector3 and plane members, float_max / float_min are called; the
// four scalar vector3 operators are the COMDATs 0x53de00-0x53debc at the end).
//
// Function-local statics: most functions keep their temporary vectors in `static vector3`s, built on
// first use behind a guard byte and registered with atexit; the registered handlers are bare `ret`s
// (one per static, listed with their functions). vector.h's vector3 has no destructor, which VC6 needs
// before it registers one (D4705): with this header the atexit calls and the 37 handlers are not
// generated.

#include <math.h>
#include "vsdk/math/ix.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/os/profile.h"

// The other two axes of a plane's largest normal component, swapped for a negative one (label ours).
static int ix_axis_pairs[3][2] = { { 2, 1 }, { 0, 2 }, { 1, 0 } };   // 0x5abe9c

// 0x53ac20  Confidence: exact. The segment p + t d (t in 0..1) against the plane through q with normal n.
// 0x53ad30 atexit_nop_53ad30: the atexit handler of tmp (compiler-generated). Confidence: exact.
bool ix_ray_polygon(ix_hit *out, const vector3 *p, const vector3 *d, const vector3 *q, const vector3 *n)
{
    static vector3 tmp;                               // 0x2dc6d78 ix_ray_polygon_tmp (guard 0x2dc6e44)
    float den, t;

    den = -n->dot(*d);
    if (den == 0.0f)
        return false;
    tmp = *p - *q;
    t = n->dot(tmp) / den;
    if (t < 0.0f)
        return false;
    if (t > 1.0f)
        return false;
    out->pos = *p + *d * t;
    out->t = t;
    return true;
}

// 0x53ad40  Confidence: exact. No caller.
bool ix_linesegment_plane_t(ix_hit *out, const vector3 *p, const vector3 *d, plane *pl)
{
    float den, t;

    den = -d->dot(pl->normal);
    if (den == 0.0f)
        return false;
    t = pl->distance_to_point(p) / den;
    out->t = t;
    if (t < 0.0f || t > 1.0f)
        return false;
    out->pos = *p + *d * t;
    return true;
}

// 0x53adf0  Confidence: exact.
bool ix_ray_plane_point(ix_ray *ray, vector3 **tri, plane *pl, vector3 *hit, float *t)
{
    if (!ix_ray_plane_t(ray, pl, t))
        return false;
    *hit = ray->origin + ray->dir * *t;
    return ix_point_in_polygon(hit, 3, tri, pl);
}

// 0x53ae60  Confidence: exact.
bool ix_linesegment_plane_front_t(const vector3 *p, const vector3 *d, plane *pl, float *t)
{
    float dist, den;

    dist = pl->distance_to_point(p);
    if (dist < 0.0f)
        return false;
    den = -d->dot(pl->normal);
    if (den < dist)
        return false;
    *t = dist / den;
    return true;
}

// 0x53aec0  Confidence: exact. No caller.
bool ix_linesegment_triangle_front(ix_ray *seg, vector3 **tri, plane *pl, vector3 *hit, float *t)
{
    if (!ix_linesegment_plane_front_t(&seg->origin, &seg->dir, pl, t))
        return false;
    *hit = seg->origin + seg->dir * *t;
    return ix_point_in_polygon(hit, 3, tri, pl);
}

// 0x53af40  Confidence: exact (VC6 byte match, m-1020d, D15902: the plane's two corners are `&(a + b)`
// temporaries, a VC6 extension, so the binary builds the third point first; one float t for both
// parameters; the edge point at s is a named local; the division of the second parameter is written
// with each side parenthesised, else 8966 stores the dot product before the two lea of perp). No
// caller. The hit must lie within the edge's span and on the positive side of b; *u is its distance
// from that edge over |b|.
bool ix_ray_parallelogram(ix_ray *ray, ix_ray *edge, const vector3 *b, vector3 *hit, float *u)
{
    float t;
#ifdef _MSC_VER
    plane pl(&edge->origin, &(edge->origin + edge->dir), &(edge->origin + *b));
#else
    vector3 c2 = edge->origin + *b;
    vector3 c1 = edge->origin + edge->dir;
    plane pl(&edge->origin, &c1, &c2);
#endif

    if (!ix_ray_plane_t(ray, &pl, &t))
        return false;
    if (t < 0.0f || t > 1.0f)
        return false;
    *hit = ray->origin + ray->dir * t;
    vector3 perp = edge->dir - (edge->dir.dot(*b) / b->dot(*b)) * *b;
    t = ((*hit - edge->origin).dot(perp)) / (perp.dot(perp));
    if (t < 0.0f || t > 1.0f)
        return false;
    vector3 q = edge->origin + edge->dir * t;
    vector3 r = *hit - q;
    if (r.dot(*b) <= 0.0f)
        return false;
    *u = r.len() / b->len();
    return *u >= 0.0f && *u <= 1.0f;
}

// 0x53b130  Confidence: likely (D15904: the retries build the plane from tri[0..2] as the binary reloads them; still differs in the scheduling of the final mean; D20301: copies vv / uu of v and u for the mean, 10 -> 4). No caller. The quad p0 p1 p3 p2 is tried as two triangle pairs; the hit
// is the ray point at the mean of the two t, *w the mean of its positions along p0-p2 and p1-p3.
// 0x53b3e0 atexit_nop_53b3e0: the atexit handler of db (compiler-generated). Confidence: exact.
// 0x53b3f0 atexit_nop_53b3f0: the atexit handler of da (compiler-generated). Confidence: exact.
bool ix_ray_quad(ix_ray *ray, vector3 *p0, vector3 *p1, vector3 *p2, vector3 *p3, vector3 *hit, float *w)
{
    static vector3 da;                                // 0x2dc6d38 ix_53b130_tmp_a (guard 0x2dc6e54)
    static vector3 db;                                // 0x2dc6dd8 ix_53b130_tmp_b
    float t1, t2, u, v;
    vector3 *tri[3];
    plane pl;

    tri[0] = p0;
    tri[1] = p1;
    tri[2] = p2;
    pl.set_from_points(p0, p1, p2);
    if (!ix_ray_plane_point(ray, tri, &pl, hit, &t1)) {
        tri[0] = p3;
        pl.set_from_points(tri[0], tri[1], tri[2]);
        if (!ix_ray_plane_point(ray, tri, &pl, hit, &t1))
            return false;
    }
    tri[0] = p0;
    tri[1] = p1;
    tri[2] = p3;
    pl.set_from_points(p0, p1, p3);
    if (!ix_ray_plane_point(ray, tri, &pl, hit, &t2)) {
        tri[1] = p2;
        pl.set_from_points(tri[0], tri[1], tri[2]);
        if (!ix_ray_plane_point(ray, tri, &pl, hit, &t2))
            return false;
    }
    *hit = ray->origin + ray->dir * ((t2 + t1) * 0.5f);
    da = *hit - *p0;
    db = *p2 - *p0;
    u = da.dot(db) / db.dot(db);
    da = *hit - *p1;
    db = *p3 - *p1;
    v = da.dot(db) / db.dot(db);
    float vv = v;                                     // D20301
    float uu = u;
    *w = (vv + uu) * 0.5f;
    *w = float_min(float_max(*w, 0.0f), 1.0f);
    return true;
}

// 0x53b400  Confidence: exact (one shared false exit: the range test with ||, the hit test as an if
// around the rest; the zero-length case measures into t, which takes t's slot as the binary, D12703).
// No caller. The first point of a-b at distance r from the centre (a itself when the sphere holds it); a
// zero-length segment only tests a.
// 0x53b6b0 atexit_nop_53b6b0: the atexit handler of closest (compiler-generated). Confidence: exact.
// 0x53b6c0 atexit_nop_53b6c0: the atexit handler of to_c (compiler-generated). Confidence: exact.
// 0x53b6d0 atexit_nop_53b6d0: the atexit handler of unit (compiler-generated). Confidence: exact.
// 0x53b6e0 atexit_nop_53b6e0: the atexit handler of dir (compiler-generated). Confidence: exact.
bool ix_linesegment_sphere(vector3 *out, const vector3 *a, const vector3 *b, const vector3 *centre, float r)
{
    static vector3 dir;                               // 0x2dc6cc8 ix_53b400_tmp_a (guard 0x2dc6dc4)
    static vector3 unit;                              // 0x2dc6d48 ix_53b400_tmp_b
    static vector3 to_c;                              // 0x2dc6df8 ix_53b400_tmp_c
    static vector3 closest;                           // 0x2dc6e18 ix_53b400_tmp_d
    float len, proj, d, t;

    dir = *b - *a;
    to_c = *centre - *a;
    len = dir.len();
    if (len <= 0.0f) {
        t = to_c.len();
        *out = *a;
        return t < r;
    }
    unit.x = dir.x / len;
    unit.y = dir.y / len;
    unit.z = dir.z / len;
    proj = unit.dot(to_c);
    if (-r > proj || len + r < proj)
        return false;
    closest = *a + unit * proj;
    d = distance(&closest, centre);
    if (d < r) {
        t = proj - (float)sqrt(r * r - d * d);
        if (t <= len && t >= 0.0f) {
            *out = *a + unit * t;
            return true;
        }
        *out = *a;
        return true;
    }
    return false;
}

// 0x53b6f0  Confidence: exact (VC6 byte match; D20300: beta copied into a block local `b` for the last
// test, so 8966 reloads it and adds alpha from the FPU stack. The local reuse; `inside` is only ever set by
// the last test; D11602: a do / while, the first triangle is tested before the count, as the binary). A fan
// of triangles from verts[0], projected on the two axes ix_axis_pairs gives for the plane's largest
// normal component; the parameters of p in each triangle.
bool ix_point_in_polygon(const vector3 *p, int n, vector3 **verts, plane *pl)
{
    vector3 an;
    int axis, u, v, i;
    bool inside;
    const float *v0, *v1, *v2;
    float pu, pv, e1u, e1v, e2u, e2v, alpha, beta;

    an.x = (float)fabs(pl->normal.x);
    an.y = (float)fabs(pl->normal.y);
    an.z = (float)fabs(pl->normal.z);
    if (an.x > an.y) {
        if (an.x > an.z)
            axis = 0;
        else
            axis = 2;
    } else {
        if (an.y > an.z)
            axis = 1;
        else
            axis = 2;
    }
    if ((&pl->normal.x)[axis] > 0.0f) {
        u = ix_axis_pairs[axis][0];
        v = ix_axis_pairs[axis][1];
    } else {
        u = ix_axis_pairs[axis][1];
        v = ix_axis_pairs[axis][0];
    }
    inside = false;
    v0 = &verts[0]->x;
    pu = (&p->x)[u] - v0[u];
    pv = (&p->x)[v] - v0[v];
    i = 2;
    do {
        v1 = &verts[i - 1]->x;
        e1u = v1[u] - v0[u];
        e1v = v1[v] - v0[v];
        v2 = &verts[i]->x;
        e2u = v2[u] - v0[u];
        e2v = v2[v] - v0[v];
        if (e1u > -0.0001f && e1u < 0.0001f) {
            beta = pu / e2u;
            if (beta < 0.0f || beta > 1.0f)
                continue;
            alpha = (pv - beta * e2v) / e1v;
        } else {
            beta = (e1u * pv - e1v * pu) / (e2v * e1u - e2u * e1v);
            if (beta < 0.0f || beta > 1.0f)
                continue;
            alpha = (pu - beta * e2u) / e1u;
        }
        float b = beta;                               // D20300
        inside = alpha >= 0.0f && b + alpha <= 1.0f;
        if (inside)
            break;
    } while (++i < n);
    return inside;
}

// 0x53b8f0  Confidence: exact. No caller.
bool ix_sphere_sphere(const vector3 *a, const vector3 *b, float ra, float rb)
{
    float s = ra + rb;

    return distance_squared(a, b) < s * s;
}

// 0x53b930  Confidence: exact. The times the moving sphere touches (t) and leaves the plane, sorted; when
// it starts touching inside the move, out is the touch point moved onto the plane. True when the span
// overlaps the move (t < 1, leaving after 0).
// 0x53bac0 atexit_nop_53bac0: the handler of touch (compiler-generated). Confidence: exact.
bool ix_sphere_polygon_plane(vector3 *out, const vector3 *p0, const vector3 *d, float r, const vector3 *n,
                             const vector3 *pp, float *t, float *span)
{
    float pd, s, v, t0, t1, tmp;

    pd = -n->dot(*pp);
    s = n->dot(*p0);
    v = n->dot(*d);
    if ((float)fabs(v) > 1e-30f) {
        t0 = (r - (s + pd)) / v;
        t1 = (-pd - s - r) / v;
        if (t1 < t0) {
            tmp = t0;
            t0 = t1;
            t1 = tmp;
        }
        *t = t0;
        if (t0 > 0.0f && t0 < 1.0f) {
            static vector3 touch;                         // 0x2dc6da8 ix_sphere_polygon_plane_tmp (guard 0x2dc6cc0)
            touch = *p0 + *d * t0;
            project_onto_plane(out, &touch, n, pp);
        }
        *span = t1 - t0;
        return t0 < 1.0f && t1 > 0.0f;
    }
    return false;
}

// 0x53bad0  Confidence: exact. No caller. The sphere's nearest point to the plane against the plane.
bool ix_sphere_plane_front(const vector3 *c, const vector3 *d, float r, plane *pl, float *t, vector3 *hit)
{
    vector3 p = *c - pl->normal * r;

    if (!ix_linesegment_plane_front_t(&p, d, pl, t))
        return false;
    *hit = p + *d * *t;
    return true;
}

// 0x53bb50  Confidence: unsure (the branch shape around the early times). The moving sphere against the
// cylinder around a-b: an entry time in -0.05..0 counts as 1e-6, an earlier one (starting inside the
// cylinder) or an entry outside the edge falls back to the sphere around a. The hit is the edge point.
// 0x53bf40 atexit_nop_53bf40: the atexit handler of centre_pt (compiler-generated). Confidence: exact.
// 0x53bf50 atexit_nop_53bf50: the atexit handler of near_pt (compiler-generated). Confidence: exact.
bool ix_sphere_polygon_edge(vector3 *hit, const vector3 *p0, const vector3 *d, float r, const vector3 *a,
                            const vector3 *b, float *t, float best)
{
    static vector3 near_pt;                           // 0x2dc6e38 ix_sphere_polygon_edge_tmp_a (guard 0x2dc6dd4)
    static vector3 centre_pt;                         // 0x2dc6d68 ix_sphere_polygon_edge_tmp_b
    vector3 e = *b - *a;
    vector3 w = *p0 - *a;
    float we, wd, ww, ed, ee, dd, qa, qb, disc, s, t0, t1, tmp, u;

    we = w.dot(e);
    wd = w.dot(*d);
    ww = w.len_sq();
    ed = e.dot(*d);
    ee = e.len_sq();
    dd = d->len_sq();
    qa = ed * ed - dd * ee;
    qb = (ed * we - ee * wd) * 2.0f;
    if (qa != 0.0f) {
        disc = qb * qb - (ee * r * r + we * we - ee * ww) * qa * 4.0f;
        if (disc <= 0.0f)
            return false;
        s = (float)sqrt(disc);
        t0 = (s - qb) / (qa + qa);
        t1 = (-qb - s) / (qa + qa);
        if (t1 < t0) {
            tmp = t1;
            t1 = t0;
            t0 = tmp;
        }
        if (t0 >= -0.05f && t0 < 0.0f) {
            t0 = 1e-6f;
        } else if (t0 < 0.0f) {
            if (t0 > 1.0f || t1 < 0.0f)
                return false;
            goto vertex;
        } else if (t0 > 1.0f) {
            return false;
        }
        if (!(t0 < best))
            return false;
        centre_pt = *p0 + *d * t0;
        u = (centre_pt - *a).dot(e) / ee;
        if (u >= 0.0f && u <= 1.0f) {
            near_pt = *a + e * u;
            *t = t0;
            *hit = near_pt;
            return true;
        }
    }
vertex:
    qb = wd * 2.0f;
    disc = qb * qb - (ww - r * r) * dd * 4.0f;
    if (disc <= 0.0f)
        return false;
    s = (float)sqrt(disc);
    t0 = (s - qb) / (dd + dd);
    t1 = (-qb - s) / (dd + dd);
    if (t0 > t1)
        t0 = t1;
    if (t0 < 0.0f || t0 > 1.0f || !(t0 < best))
        return false;
    centre_pt = *p0 + *d * t0;
    near_pt = *a;
    *t = t0;
    *hit = near_pt;
    return true;
}

// 0x53bf60  Confidence: exact (VC6 byte match, m-1020: te is a loop-body local, which 8966 puts in r's dead
// argument slot as the binary does). Each edge whose swept box (ix_linesegment_boundingbox) the move can reach
// is tested; the earliest hit wins.
// 0x53c1b0 atexit_nop_53c1b0: the atexit handler of box_max (compiler-generated). Confidence: exact.
// 0x53c1c0 atexit_nop_53c1c0: the atexit handler of box_min (compiler-generated). Confidence: exact.
// 0x53c1d0 atexit_nop_53c1d0: the atexit handler of hit_pt (compiler-generated). Confidence: exact.
// 0x53c1e0 atexit_nop_53c1e0: the atexit handler of edge_b (compiler-generated). Confidence: exact.
// 0x53c1f0 atexit_nop_53c1f0: the atexit handler of edge_a (compiler-generated). Confidence: exact.
bool ix_sphere_polygon(vector3 *hit, const vector3 *p0, const vector3 *d, float r, int n, vector3 **verts,
                       float *t)
{
    static vector3 edge_a;                            // 0x2dc6e98 ix_sphere_polygon_tmp_a (guard 0x2dc6cf4)
    static vector3 edge_b;                            // 0x2dc6d18 ix_sphere_polygon_tmp_b
    float best = 3.4028235e38f;
    int i;

    for (i = 0; i < n; i++) {
        float te;                                     // the binary keeps it in r's argument slot (r stays in EDI)
        edge_a = *verts[i];
        if (i + 1 == n)
            edge_b = *verts[0];
        else
            edge_b = *verts[i + 1];
        static vector3 hit_pt;                        // 0x2dc6e68 ix_sphere_polygon_tmp_c
        static vector3 box_min;                       // 0x2dc6d88 ix_sphere_polygon_bbox_min
        static vector3 box_max;                       // 0x2dc6d98 ix_sphere_polygon_bbox_max
        vector3 lo = *p0 - r;
        vector3 hi = *p0 + r;
        bbox_from_points(&box_min, &box_max, &lo, &hi);
        bbox_expand_by_vector(&box_min, &box_max, d);
        if (ix_linesegment_boundingbox(&box_min, &box_max, &edge_a, &edge_b, &hit_pt)
            && ix_sphere_polygon_edge(&hit_pt, p0, d, r, &edge_a, &edge_b, &te, best)
            && te < best) {
            best = te;
            *hit = hit_pt;
        }
    }
    if (best <= 1.0f) {
        *t = best;
        return true;
    }
    return false;
}

// 0x53c200  Confidence: exact. No caller.
bool ix_boundingbox_boundingbox(const vector3 *min1, const vector3 *max1, const vector3 *min2,
                                const vector3 *max2)
{
    if (min1->x <= max2->x && min1->y <= max2->y && min1->z <= max2->z
        && max1->x >= min2->x && max1->y >= min2->y && max1->z >= min2->z)
        return true;
    return false;
}

// 0x53c260  Confidence: exact.
bool ix_point_boundingbox(const vector3 *p, const vector3 *min, const vector3 *max)
{
    if (p->x >= min->x && p->x <= max->x && p->y >= min->y && p->y <= max->y
        && p->z >= min->z && p->z <= max->z)
        return true;
    return false;
}

// 0x53c2c0  Confidence: exact. No caller.
// 0x53c3e0 atexit_nop_53c3e0: the atexit handler of hi (compiler-generated). Confidence: exact.
// 0x53c3f0 atexit_nop_53c3f0: the atexit handler of lo (compiler-generated). Confidence: exact.
// 0x53c400 atexit_nop_53c400: the atexit handler of local (compiler-generated). Confidence: exact.
bool ix_point_box_oriented(const vector3 *p, const vector3 *centre, const matrix *orient, const vector3 *size)
{
    static vector3 local;                             // 0x2dc6e88 ix_53c2c0_tmp_a (guard 0x2dc6e04)
    local = *p - *centre;
    local.rotate(*orient);
    static vector3 lo;                                // 0x2dc6d58 ix_53c2c0_tmp_b
    static vector3 hi;                                // 0x2dc6e08 ix_53c2c0_tmp_c
    lo = *size * -0.5f;
    hi = *size * 0.5f;
    return ix_point_boundingbox(&local, &lo, &hi);
}

// 0x53c410  Confidence: exact. No caller. (sum.symbols: "(center, min, max, radius)"; the radius is the
// second argument.)
bool ix_sphere_boundingbox(const vector3 *p, float r, const vector3 *min, const vector3 *max)
{
    vector3 lo = *min - r;
    vector3 hi = *max + r;

    if (p->x >= lo.x && p->x <= hi.x && p->y >= lo.y && p->y <= hi.y && p->z >= lo.z && p->z <= hi.z)
        return true;
    return false;
}

// 0x53c4a0  Confidence: exact. No caller.
// 0x53c5e0 atexit_nop_53c5e0: the atexit handler of hi (compiler-generated). Confidence: exact.
// 0x53c5f0 atexit_nop_53c5f0: the atexit handler of lo (compiler-generated). Confidence: exact.
// 0x53c600 atexit_nop_53c600: the atexit handler of local (compiler-generated). Confidence: exact.
bool ix_sphere_box_oriented(const vector3 *p, float r, const vector3 *centre, const matrix *orient,
                            const vector3 *size)
{
    static vector3 local;                             // 0x2dc6ce8 ix_53c4a0_tmp_a (guard 0x2dc6d04)
    local = *p - *centre;
    local.rotate(*orient);
    static vector3 lo;                                // 0x2dc6d28 ix_53c4a0_tmp_b
    static vector3 hi;                                // 0x2dc6cd8 ix_53c4a0_tmp_c
    lo = *size * -0.5f;
    hi = *size * 0.5f;
    lo -= r;
    hi += r;
    return ix_point_boundingbox(&local, &lo, &hi);
}

// 0x53c610  Confidence: likely; tests 3, 5 and 8: unsure. The 15 separating-axis tests of two oriented
// boxes (B is the rotation of the second box in the first's frame, rows 16 bytes apart; T the translation;
// a / b the half extents), each term in the binary's order. Tests 3, 5, 8 and 9 use an extent of the
// wrong axis (see NOTES decomp67): test 3 multiplies Bf[1][0] by b[2] instead of b[0] (the binary adds
// Bf[1][2] + Bf[1][0] first), test 5 Bf[0][1] by a[2] instead of a[0], test 8 Bf[0][2] by b[2] instead of
// b[0], test 9 Bf[0][1] by b[2] instead of b[0] (the product shared with test 7).
int ix_box_oriented_box_oriented(const float B[3][4], const vector3 *T, const vector3 *a, const vector3 *b)
{
    float Bf[3][3];
    float t;
    int r;

    Bf[0][0] = (float)fabs(B[0][0]) + 1e-6f;
    Bf[0][1] = (float)fabs(B[0][1]) + 1e-6f;
    Bf[0][2] = (float)fabs(B[0][2]) + 1e-6f;
    Bf[1][0] = (float)fabs(B[1][0]) + 1e-6f;
    Bf[1][1] = (float)fabs(B[1][1]) + 1e-6f;
    Bf[1][2] = (float)fabs(B[1][2]) + 1e-6f;
    Bf[2][0] = (float)fabs(B[2][0]) + 1e-6f;
    Bf[2][1] = (float)fabs(B[2][1]) + 1e-6f;
    Bf[2][2] = (float)fabs(B[2][2]) + 1e-6f;

    r = 1;
    t = (float)fabs(T->x);
    r &= (t <= Bf[0][2] * b->z + Bf[0][1] * b->y + Bf[0][0] * b->x + a->x);
    if (!r)
        return 1;
    t = (float)fabs(T->z * B[2][0] + T->y * B[1][0] + B[0][0] * T->x);
    r &= (t <= Bf[0][0] * a->x + Bf[2][0] * a->z + Bf[1][0] * a->y + b->x);
    if (!r)
        return 2;
    t = (float)fabs(T->y);
    r &= (t <= (Bf[1][2] + Bf[1][0]) * b->z + Bf[1][1] * b->y + a->y);
    if (!r)
        return 3;
    t = (float)fabs(T->z);
    r &= (t <= Bf[2][2] * b->z + Bf[2][1] * b->y + Bf[2][0] * b->x + a->z);
    if (!r)
        return 4;
    t = (float)fabs(T->z * B[2][1] + T->y * B[1][1] + T->x * B[0][1]);
    r &= (t <= (Bf[2][1] + Bf[0][1]) * a->z + Bf[1][1] * a->y + b->y);
    if (!r)
        return 5;
    t = (float)fabs(T->z * B[2][2] + T->y * B[1][2] + B[0][2] * T->x);
    r &= (t <= Bf[0][2] * a->x + Bf[2][2] * a->z + Bf[1][2] * a->y + b->z);
    if (!r)
        return 6;
    t = (float)fabs(T->z * B[1][0] - T->y * B[2][0]);
    r &= (t <= Bf[0][2] * b->y + Bf[2][0] * a->y + Bf[1][0] * a->z + Bf[0][1] * b->z);
    if (!r)
        return 7;
    t = (float)fabs(T->z * B[1][1] - T->y * B[2][1]);
    r &= (t <= (Bf[0][2] + Bf[0][0]) * b->z + Bf[2][1] * a->y + Bf[1][1] * a->z);
    if (!r)
        return 8;
    t = (float)fabs(T->z * B[1][2] - T->y * B[2][2]);
    r &= (t <= Bf[0][0] * b->y + Bf[2][2] * a->y + Bf[1][2] * a->z + Bf[0][1] * b->z);
    if (!r)
        return 9;
    t = (float)fabs(B[2][0] * T->x - T->z * B[0][0]);
    r &= (t <= Bf[1][2] * b->y + Bf[1][1] * b->z + Bf[2][0] * a->x + Bf[0][0] * a->z);
    if (!r)
        return 10;
    t = (float)fabs(T->x * B[2][1] - T->z * B[0][1]);
    r &= (t <= Bf[1][0] * b->z + Bf[2][1] * a->x + Bf[1][2] * b->x + Bf[0][1] * a->z);
    if (!r)
        return 11;
    t = (float)fabs(B[2][2] * T->x - T->z * B[0][2]);
    r &= (t <= Bf[1][0] * b->y + Bf[2][2] * a->x + Bf[1][1] * b->x + Bf[0][2] * a->z);
    if (!r)
        return 12;
    t = (float)fabs(T->y * B[0][0] - T->x * B[1][0]);
    r &= (t <= Bf[2][2] * b->y + Bf[2][1] * b->z + Bf[1][0] * a->x + Bf[0][0] * a->y);
    if (!r)
        return 13;
    t = (float)fabs(T->y * B[0][1] - B[1][1] * T->x);
    r &= (t <= Bf[2][0] * b->z + Bf[1][1] * a->x + Bf[2][2] * b->x + Bf[0][1] * a->y);
    if (!r)
        return 14;
    t = (float)fabs(T->y * B[0][2] - T->x * B[1][2]);
    r &= (t <= Bf[2][0] * b->y + Bf[1][2] * a->x + Bf[2][1] * b->x + Bf[0][2] * a->y);
    if (!r)
        return 15;
    return 0;
}

// 0x53cb60  Confidence: exact (the squares through a register copy). No caller. Not two lines: the
// intersection line of two planes, its point solved in the plane of the direction's largest component.
// 0x53cd50 atexit_nop_53cd50: the handler of sq (compiler-generated). Confidence: exact.
bool ix_line_line(const plane *a, const plane *b, ix_ray *out)
{
    static vector3 sq;                                // 0x2dc6e78 ix_53cb60_sq (guard 0x2dc6d94)
    float inv;

    out->dir = a->normal.cross(b->normal);
    sq.x = out->dir.x * out->dir.x;
    sq.y = out->dir.y * out->dir.y;
    sq.z = out->dir.z * out->dir.z;
    if (sq.z > sq.y && sq.z > sq.x && sq.z > 1e-8f) {
        inv = 1.0f / out->dir.z;
        out->origin.set(a->normal.y * b->d - b->normal.y * a->d, a->d * b->normal.x - a->normal.x * b->d, 0.0f);
    } else if (sq.y > sq.x && sq.y > 1e-8f) {
        inv = 1.0f / out->dir.y;
        out->origin.set(b->normal.z * a->d - a->normal.z * b->d, 0.0f, a->normal.x * b->d - a->d * b->normal.x);
    } else if (sq.x > 1e-8f) {
        inv = 1.0f / out->dir.x;
        out->origin.set(0.0f, a->normal.z * b->d - b->normal.z * a->d, b->normal.y * a->d - a->normal.y * b->d);
    } else {
        return false;
    }
    out->origin *= inv;
    out->dir *= 1.0f / (float)sqrt(sq.z + sq.y + sq.x);
    return true;
}

// 0x53cd60  Confidence: exact. No caller.
bool ix_ray_plane_point_b(ix_ray *ray, plane *pl, vector3 *out)
{
    float den, t;

    den = ray->dir.dot(pl->normal);
    if (den == 0.0f)
        return false;
    t = -(pl->distance_to_point(&ray->origin) / den);
    *out = ray->origin + ray->dir * t;
    return true;
}

// 0x53cde0  Confidence: exact.
bool ix_ray_plane_t(ix_ray *ray, plane *pl, float *t)
{
    float den;

    den = ray->dir.dot(pl->normal);
    if (den == 0.0f)
        return false;
    *t = -(pl->distance_to_point(&ray->origin) / den);
    return true;
}

// 0x53ce30  Confidence: exact. out is written before the t test.
bool ix_ray_plane(ix_ray *ray, plane *pl, vector3 *out)
{
    float den, t;

    den = ray->dir.dot(pl->normal);
    if (den == 0.0f)
        return false;
    t = -(pl->distance_to_point(&ray->origin) / den);
    *out = ray->origin + ray->dir * t;
    if (t < 0.0f)
        return false;
    return true;
}

// 0x53ced0  Confidence: exact. No caller.
// 0x53d0c0 atexit_nop_53d0c0: the atexit handler of hi (compiler-generated). Confidence: exact.
// 0x53d0d0 atexit_nop_53d0d0: the atexit handler of lo (compiler-generated). Confidence: exact.
// 0x53d0e0 atexit_nop_53d0e0: the atexit handler of lb (compiler-generated). Confidence: exact.
// 0x53d0f0 atexit_nop_53d0f0: the atexit handler of la (compiler-generated). Confidence: exact.
bool ix_linesegment_box_oriented(const vector3 *centre, const matrix *orient, const vector3 *size,
                                 const vector3 *a, const vector3 *b, vector3 *out)
{
    static vector3 la;                                // 0x2dc6de8 ix_linesegment_box_oriented_tmp_a (guard 0x2dc6e34)
    static vector3 lb;                                // 0x2dc6e28 ix_linesegment_box_oriented_tmp_b
    profile_label label("ix_linesegment_box_oriented");
    bool hit;

    la = *a - *centre;
    la.rotate(*orient);
    lb = *b - *centre;
    lb.rotate(*orient);
    static vector3 lo;                                // 0x2dc6e48 ix_linesegment_box_oriented_tmp_c
    static vector3 hi;                                // 0x2dc6d08 ix_linesegment_box_oriented_tmp_d
    lo = *size * -0.5f;
    hi = *size * 0.5f;
    hit = ix_linesegment_boundingbox(&lo, &hi, &la, &lb, out);
    out->unrotate(*orient);
    *out += *centre;
    return hit;
}

// 0x53d100  Confidence: exact (the direction is rotated before the label is built).
// 0x53d2a0 atexit_nop_53d2a0: the atexit handler of hi (compiler-generated). Confidence: exact.
// 0x53d2b0 atexit_nop_53d2b0: the atexit handler of lo (compiler-generated). Confidence: exact.
// 0x53d2c0 atexit_nop_53d2c0: the atexit handler of lorigin (compiler-generated). Confidence: exact.
bool ix_ray_box_oriented(ix_ray *ray, const vector3 *centre, const matrix *orient, const vector3 *size,
                         vector3 *hit)
{
    static vector3 lorigin;                           // 0x2dc6e58 ix_ray_box_oriented_tmp_a (guard 0x2dc6ce4)
    vector3 ldir = ray->dir.rotated(*orient);
    profile_label label("ix_ray_box_oriented");
    bool r;

    lorigin = ray->origin - *centre;
    lorigin.rotate(*orient);
    static vector3 lo;                                // 0x2dc6cf8 ix_ray_box_oriented_tmp_b
    static vector3 hi;                                // 0x2dc6db8 ix_ray_box_oriented_tmp_c
    lo = *size * -0.5f;
    hi = *size * 0.5f;
    r = ix_ray_boundingbox(&lo, &hi, &lorigin, &ldir, hit);
    hit->unrotate(*orient);
    *hit += *centre;
    return r;
}

// 0x53d2d0  Confidence: exact (VC6 byte match, m-1020: `inside` is declared and set before the profile
// label, the binary sets EDI before the constructor call; the array indexing; the binary walks the components with pointer
// differences). Woo's test: the candidate planes of the outside axes, the largest parameter picks the
// plane; no upper limit on the parameter.
bool ix_ray_boundingbox(const vector3 *min, const vector3 *max, const vector3 *origin, const vector3 *dir,
                        vector3 *hit)
{
    int inside = 1;
    profile_label label("ix_ray_boundingbox");
    int quadrant[3];
    float max_t[3];
    float candidate[3];
    int i, which;
    const float *o = &origin->x;
    const float *d = &dir->x;
    const float *lo = &min->x;
    const float *hi = &max->x;
    float *h = &hit->x;

    for (i = 0; i < 3; i++) {
        if (o[i] < lo[i]) {
            quadrant[i] = 0;
            candidate[i] = lo[i];
            inside = 0;
        } else if (o[i] > hi[i]) {
            quadrant[i] = 0;
            candidate[i] = hi[i];
            inside = 0;
        } else {
            quadrant[i] = 1;
        }
    }
    if (inside) {
        *hit = *origin;
        return true;
    }
    for (i = 0; i < 3; i++) {
        if (quadrant[i] == 0 && d[i] != 0.0f)
            max_t[i] = (candidate[i] - o[i]) / d[i];
        else
            max_t[i] = -1.0f;
    }
    which = 0;
    for (i = 1; i < 3; i++) {
        if (max_t[which] < max_t[i])
            which = i;
    }
    if (max_t[which] < 0.0f)
        return false;
    for (i = 0; i < 3; i++) {
        if (which != i) {
            h[i] = d[i] * max_t[which] + o[i];
            if (h[i] < lo[i] || h[i] > hi[i])
                return false;
        } else {
            h[i] = candidate[i];
        }
    }
    return true;
}

// 0x53d4d0  Confidence: exact (D6305: the clip masks are unsigned, `> 0` gives the binary's test / ja;
// the two differences are computed once, the faces chosen with if / else; the two computed coordinates of
// each face are the same two locals u, v, which take the binary's stack slots, D12702). Only the faces a lies outside of are tried.
bool ix_linesegment_boundingbox(const vector3 *min, const vector3 *max, const vector3 *a, const vector3 *b,
                                vector3 *out)
{
    unsigned int ca, cb;
    float u, v, d, s;

    ca = ix_slab_clip(min, max, a);
    if (ca == 0) {
        *out = *a;
        return true;
    }
    cb = ix_slab_clip(min, max, b);
    if (cb == 0) {
        *out = *b;
        return true;
    }
    if ((ca & cb) > 0)
        return false;
    if (ca & 3) {
        if (ca & 1)
            out->x = max->x;
        else
            out->x = min->x;
        d = b->x - a->x;
        s = out->x - a->x;
        u = (b->y - a->y) * s / d + a->y;
        out->y = u;
        v = (b->z - a->z) * s / d + a->z;
        out->z = v;
        if (u <= max->y && u >= min->y && v <= max->z && v >= min->z)
            return true;
    }
    if (ca & 0xc) {
        if (ca & 4)
            out->y = max->y;
        else
            out->y = min->y;
        d = b->y - a->y;
        s = out->y - a->y;
        u = (b->x - a->x) * s / d + a->x;
        out->x = u;
        v = (b->z - a->z) * s / d + a->z;
        out->z = v;
        if (u <= max->x && u >= min->x && v <= max->z && v >= min->z)
            return true;
    }
    if (ca & 0x30) {
        if (ca & 0x20)
            out->z = max->z;
        else
            out->z = min->z;
        d = b->z - a->z;
        s = out->z - a->z;
        u = (b->x - a->x) * s / d + a->x;
        out->x = u;
        v = (b->y - a->y) * s / d + a->y;
        out->y = v;
        if (u <= max->x && u >= min->x && v <= max->y && v >= min->y)
            return true;
    }
    return false;
}

// 0x53d720  Confidence: exact. Not a slab clip: the point's outcode.
int ix_slab_clip(const vector3 *min, const vector3 *max, const vector3 *p)
{
    int code = 0;

    if (p->x > max->x)
        code = 1;
    else if (p->x < min->x)
        code = 2;
    if (p->y > max->y)
        code |= 4;
    else if (p->y < min->y)
        code |= 8;
    if (p->z > max->z)
        code |= 0x20;
    else if (p->z < min->z)
        code |= 0x10;
    return code;
}

// 0x53d7a0  Confidence: exact (VC6 byte match, match29; was likely) (the label is built after the first three statements). The ray length is
// not read; t is not written when the origin is inside the sphere (hit = origin).
bool ix_ray_sphere(vector3 *hit, float *t, ix_ray *ray, float length, const vector3 *centre, float radius)
{
    vector3 l = *centre - ray->origin;
    float r2 = radius * radius;
    float c = l.dot(l);
    profile_label label("ix_ray_sphere");
    float tca, thc2;

    if (c <= r2) {
        *hit = ray->origin;
        return true;
    }
    tca = ray->dir.dot(l);
    if (tca < 0.0f)
        return false;
    thc2 = r2 - (c - tca * tca);
    if (thc2 < 0.0f)
        return false;
    tca = tca - (float)sqrt(thc2);
    *t = tca;
    *hit = ray->origin + ray->dir * tca;
    return true;
}

// 0x53d8f0  Confidence: likely (the second den == 0 test is in the binary, 0x53d98b).
bool seg2d_intersect(const vector2 *a, const vector2 *b, vector2 c, vector2 d)
{
    float dx1, dy1, ex, ey, den, fx, fy, s, u;

    dx1 = b->x - a->x;
    ey = d.y - c.y;
    dy1 = b->y - a->y;
    ex = d.x - c.x;
    den = ey * dx1 - ex * dy1;
    if (den == 0.0f)
        return false;
    fy = a->y - c.y;
    fx = a->x - c.x;
    s = (fy * ex - fx * ey) / den;
    if (s < 0.0f || s > 1.0f)
        return false;
    if (den == 0.0f)
        return false;
    u = (fy * dx1 - fx * dy1) / den;
    if (u < 0.0f || u > 1.0f)
        return false;
    return true;
}

// 0x53d9d0  Confidence: exact. No caller. (sum.symbols names the static seg2d_intersect_tmp.)
// 0x53da70 atexit_nop_53da70: the handler of rel (compiler-generated). Confidence: exact.
float ix_ray_closest_point_t(const vector3 *p, ix_ray *ray, vector3 *out)
{
    static vector3 rel;                               // 0x2dc6dc8 (guard 0x2dc6cc1)
    float t;

    rel = *p - ray->origin;
    t = rel.dot(ray->dir);
    *out = ray->origin + ray->dir * t;
    return t;
}

// 0x53da80  Confidence: exact. Returns the clamped distance of the closest point along a-b (0 for a
// zero-length segment), not the point's distance.
float ix_point_linesegment(const vector3 *p, const vector3 *a, const vector3 *b, vector3 *closest)
{
    vector3 d, w, n;
    float len, t;

    d = *b - *a;
    len = d.len();
    if (len == 0.0f) {
        *closest = *a;
        return 0.0f;
    }
    n = d;
    n *= 1.0f / len;
    w = *p - *a;
    t = n.dot(w);
    t = float_min(float_max(t, 0.0f), len);
    *closest = *a + n * t;
    return t;
}

// 0x53dbb0  Confidence: exact (VC6 byte match, match29; was likely). No caller.
float ix_linesegment_sphere_chord_length(const vector3 *a, const vector3 *b, const vector3 *centre, float r)
{
    float result = 0.0f;
    vector3 dir = *b - *a;
    float len = dir.normalize_safe_length();
    vector3 to = *centre - *a;
    float proj = dir.dot(to);
    float r2, disc, s, lo;

    if (!(-r > proj) && len + r > proj) {
        r2 = r * r;
        disc = r2 - (to.dot(to) - proj * proj);
        if (disc > 0.0f) {
            s = (float)sqrt(disc);
            lo = float_min(float_max(proj - s, 0.0f), len);
            result = float_min(float_max(s + proj, 0.0f), len);
            result -= lo;
        }
    }
    return result;
}

// 0x53dcd0  Confidence: exact (VC6 byte match, match29; was likely). No caller. With k = |a - b| / r: x = r sqrt(k^2 - 1) / k, y = r / k and the
// angle twice the one at (0, k) between the tangent point (x, y) and (0, y).
bool ix_point_sphere_tangent(const vector3 *a, const vector3 *b, float r, float *x, float *angle, float *y)
{
    float d, k;

    d = distance(a, b);
    if (d <= 0.0f || r <= 0.0f || d <= r)
        return false;
    k = d / r;
    *x = (float)sqrt(k * k - 1.0f) / k;
    *y = 1.0f / k;
    vector3 c(0.0f, k, 0.0f);
    vector3 p(0.0f, *y, 0.0f);
    vector3 q(*x, *y, 0.0f);
    vector3 u = p - c;
    vector3 v = q - c;
    u.normalize();
    v.normalize();
    *angle = (float)acos(u.dot(v)) * 2.0f;
    *x = r * *x;
    *y = r * *y;
    return true;
}

// ---- COMDATs at the end of the object (vsdk/math/vector.h inline members) -----------------------------
// 0x53de00 vector3::operator+(float) (sum.symbols vector3__plus_scalar). Confidence: likely (the binary
//          loads s first: commutative order).
// 0x53de40 vector3::operator-(float) (vector3__minus_scalar). Confidence: exact.
// 0x53de80 vector3::operator+=(float) (vector3__add_scalar_assign). Confidence: likely (as 0x53de00).
// 0x53dea0 vector3::operator-=(float) (vector3__sub_scalar_assign). Confidence: exact.
