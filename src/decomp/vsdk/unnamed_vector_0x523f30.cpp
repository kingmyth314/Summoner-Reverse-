// unnamed_vector_0x523f30.cpp - Sum.exe v1.40, code 0x523f30-0x525710: vector3 / vector2 math (vsdk\math
// in sum.symbols). The file name is a placeholder: no path string, no strings.tbl block; Summoner 2's
// unit with the same functions in the same order (unit_00196B10) has no name either.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range. Two static initialisers open the object (static_initializers entries 598 / 599, after
// the quaternion unit's 597); the regular functions run to vector2_segment_circle_intersect (0x5254d0); the
// inline members it calls close it as COMDATs (0x525650-0x525710, listed at the end). The File unit's
// initialisers follow at 0x525720 (entries 600-603). .data 0x5a8b4c-0x5a8b63 (the axis table; the File
// unit's data follows at 0x5a8b64), .bss 0x2cbf4b0-0x2cbf4c3.
//
// Build flags: no inline expansion for the class members (the vector operators, len, len_sq, dot,
// set_from_unit_forward are called); the lengths of the normalisers are written out (FSQRT inline).

#include <math.h>
#include <float.h>
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"

// The other two axes of each major axis, for a positive component (swapped for a negative one).
static int vector3_major_axis_pairs[3][2] = { { 2, 1 }, { 0, 2 }, { 1, 0 } };   // 0x5a8b4c (label ours)

// 0x523f30 init_vector3_zero (jmp 0x523f40: vector3(0, 0, 0)). Confidence: exact.
vector3 vector3_zero(0.0f, 0.0f, 0.0f);           // 0x2cbf4b8
// 0x523f60 init_vector2_zero (jmp 0x523f70: vector2(0, 0)). Confidence: exact.
vector2 vector2_zero(0.0f, 0.0f);                 // 0x2cbf4b0

// 0x523f80  Confidence: exact (a copy of the components in registers).
float vector2::len() const
{
    return (float)sqrt(x * x + y * y);
}

// 0x523fa0  Confidence: exact.
float vector2::len_sq() const
{
    return x * x + y * y;
}

// 0x523fc0  Confidence: exact. (1, 0) with length 1 at length 0.
float vector2::normalize()
{
    float l = len();

    if (l == 0.0f) {
        x = 1.0f;
        y = 0.0f;
        return 1.0f;
    }
    x /= l;
    y /= l;
    return l;
}

// 0x524000  Confidence: exact.
void vector2::negate()
{
    x = -x;
    y = -y;
}

// 0x524010  Confidence: exact (the |x| of the local copy stays in a register).
void vector3::get_major_axis(int *axis, int *u, int *v) const
{
    vector3 a;
    float c;

    a.x = (float)fabs(x);
    a.y = (float)fabs(y);
    a.z = (float)fabs(z);
    if (a.x > a.y) {
        if (a.x > a.z) {
            *axis = 0;
            c = x;
        } else {
            *axis = 2;
            c = z;
        }
    } else if (a.y > a.z) {
        *axis = 1;
        c = y;
    } else {
        *axis = 2;
        c = z;
    }
    if (c > 0.0f) {
        *u = vector3_major_axis_pairs[*axis][0];
        *v = vector3_major_axis_pairs[*axis][1];
    } else {
        *u = vector3_major_axis_pairs[*axis][1];
        *v = vector3_major_axis_pairs[*axis][0];
    }
}

// 0x5240e0  Confidence: likely. The absolute components sorted a >= b >= c:
// a + 1.5 (b / 4 + c / 8).
float vector3::length_quick() const
{
    float a, b, c, t;

    a = x < 0.0f ? -x : x;
    b = y < 0.0f ? -y : y;
    c = z < 0.0f ? -z : z;
    if (a < b) {
        t = a;
        a = b;
        b = t;
    }
    if (b < c) {
        t = b;
        b = c;
        c = t;
        if (a < b) {
            t = a;
            a = b;
            b = t;
        }
    }
    t = c * 0.125f + b * 0.25f;
    return t + t * 0.5f + a;
}

// 0x5241c0  Confidence: exact (the term order: y, z, x).
float vector3::dot_xyz(float x, float y, float z) const
{
    return y * this->y + z * this->z + x * this->x;
}

// 0x5241e0  Confidence: exact.
int vector3::is_zero() const
{
    if (x == 0.0f && y == 0.0f && z == 0.0f)
        return 1;
    return 0;
}

// 0x524220  Confidence: exact.
int vector3::is_valid() const
{
    if (!_isnan(x) && !_isnan(y) && !_isnan(z))
        return 1;
    return 0;
}

// 0x524270  Confidence: exact.
vector3 vector3::normalized() const
{
    float s = 1.0f / (float)sqrt(x * x + y * y + z * z);

    return vector3(x * s, y * s, z * s);
}

// 0x5242c0  Confidence: exact.
vector3 vector3::normalized_or_x() const
{
    float l = (float)sqrt(x * x + y * y + z * z);
    float s;

    if (l <= 0.0f)
        return vector3(1.0f, 0.0f, 0.0f);
    s = 1.0f / l;
    return vector3(x * s, y * s, z * s);
}

// 0x524340  Confidence: exact (operator-'s body, out of line).
vector3 vector3::negated_b() const
{
    return vector3(-x, -y, -z);
}

// 0x524370  Confidence: exact (VC6 8966 byte match with the rows bound to references: 8966 orders the
// terms of each dot product as an inlined dot() does). The dot product with each row.
vector3 vector3::rotated(const matrix &m) const
{
    const vector3 &r = m.rvec, &u = m.uvec, &f = m.fvec;

    return vector3(x * r.x + y * r.y + z * r.z,
                   x * u.x + y * u.y + z * u.z,
                   x * f.x + y * f.y + z * f.z);
}

// 0x5243d0  Confidence: likely. The transpose: the dot product with each column.
vector3 vector3::unrotated(const matrix &m) const
{
    return vector3(x * m.rvec.x + y * m.uvec.x + z * m.fvec.x,
                   x * m.rvec.y + y * m.uvec.y + z * m.fvec.y,
                   x * m.rvec.z + y * m.uvec.z + z * m.fvec.z);
}

// 0x524430  Confidence: exact. No zero check.
void vector3::normalize()
{
    float s = 1.0f / (float)sqrt(x * x + y * y + z * z);

    x *= s;
    y *= s;
    z *= s;
}

// 0x524470  Confidence: exact.
float vector3::normalize_length()
{
    float l = (float)sqrt(x * x + y * y + z * z);
    float s = 1.0f / l;

    x *= s;
    y *= s;
    z *= s;
    return l;
}

// 0x5244b0  Confidence: exact.
void vector3::normalize_safe()
{
    float l = (float)sqrt(x * x + y * y + z * z);
    float s;

    if (l <= 0.0f) {
        x = 1.0f;
        y = 0.0f;
        z = 0.0f;
        return;
    }
    s = 1.0f / l;
    x *= s;
    y *= s;
    z *= s;
}

// 0x524510  Confidence: exact. A zero vector becomes (1, 0, 0) and the result is 1, not 0.
float vector3::normalize_safe_length()
{
    float l = (float)sqrt(x * x + y * y + z * z);
    float s;

    if (l <= 0.0f) {
        x = 1.0f;
        y = 0.0f;
        z = 0.0f;
        return 1.0f;
    }
    s = 1.0f / l;
    x *= s;
    y *= s;
    z *= s;
    return l;
}

// 0x524580  Confidence: exact.
void vector3::negate()
{
    x = -x;
    y = -y;
    z = -z;
}

// 0x5245a0  Confidence: exact (the components are loaded once).
void vector3::rotate(const matrix &m)
{
    float vx = x, vy = y, vz = z;

    x = vx * m.rvec.x + vy * m.rvec.y + vz * m.rvec.z;
    y = vx * m.uvec.x + vy * m.uvec.y + vz * m.uvec.z;
    z = vx * m.fvec.x + vy * m.fvec.y + vz * m.fvec.z;
}

// 0x5245f0  Confidence: exact.
void vector3::unrotate(const matrix &m)
{
    float vx = x, vy = y, vz = z;

    x = vx * m.rvec.x + vy * m.uvec.x + vz * m.fvec.x;
    y = vx * m.rvec.y + vy * m.uvec.y + vz * m.fvec.y;
    z = vx * m.rvec.z + vy * m.uvec.z + vz * m.fvec.z;
}

// 0x524640  Confidence: exact.
void vector3::zero()
{
    x = 0.0f;
    y = 0.0f;
    z = 0.0f;
}

// 0x524650  Confidence: likely. z uniform in -1..1, the angle uniform: uniform on the sphere.
void vector3::random_unit_vector()
{
    float a, r;

    z = frand_range(-1.0f, 1.0f);
    a = frand() * 6.2831855f;
    r = (float)sqrt(1.0f - z * z);
    x = (float)cos(a) * r;
    y = (float)sin(a) * r;
}

// 0x5246a0  Confidence: likely (the same body as random_unit_vector, not folded).
void vector3::random_direction()
{
    float a, r;

    z = frand_range(-1.0f, 1.0f);
    a = frand() * 6.2831855f;
    r = (float)sqrt(1.0f - z * z);
    x = (float)cos(a) * r;
    y = (float)sin(a) * r;
}

// 0x5246f0  Confidence: likely (thiscall, ret 4).
void vector3::random_direction_in_cone(float min_z)
{
    float a, r;

    z = frand_range(min_z, 1.0f);
    a = frand() * 6.2831855f;
    r = (float)sqrt(1.0f - z * z);
    x = (float)cos(a) * r;
    y = (float)sin(a) * r;
}

// 0x524740  Confidence: exact (thiscall, ret 0x10: the axis by value).
void vector3::random_direction_in_cone_axis(vector3 axis, float min_z)
{
    matrix m;

    m.set_from_unit_forward(axis);
    random_direction_in_cone(min_z);
    unrotate(m);
}

// 0x524780  Confidence: exact (the dot product lives in the n argument's slot).
float vector3_project_onto_normal(vector3 *out, const vector3 *v, const vector3 *n)
{
    float d = v->dot(*n);

    *out = *n * d;
    return d;
}

// 0x5247c0  Confidence: exact.
void vector3_remove_normal_component(vector3 *out, const vector3 *v, const vector3 *n)
{
    float d = v->dot(*n);

    *out = *v;
    *out += *n * -d;
}

// 0x524810  Confidence: exact.
float distance(const vector3 *a, const vector3 *b)
{
    return (*a - *b).len();
}

// 0x524840  Confidence: exact.
float distance_squared(const vector3 *a, const vector3 *b)
{
    return (*a - *b).len_sq();
}

// 0x524870  Confidence: exact.
float distance_quick(const vector3 *a, const vector3 *b)
{
    return (*a - *b).length_quick();
}

// 0x5248a0  Confidence: exact.
float dot_of_difference(const vector3 *a, const vector3 *b, const vector3 *c)
{
    return b->dot(*a - *c);
}

// 0x5248d0  Confidence: exact.
float vector3_normalized_dir(vector3 *out, const vector3 *a, const vector3 *b)
{
    *out = *a - *b;
    return out->normalize_length();
}

// 0x524910  Confidence: exact.
int vector3_near_equal(const vector3 *a, const vector3 *b)
{
    if ((float)fabs(a->x - b->x) < 0.0001f && (float)fabs(a->y - b->y) < 0.0001f && (float)fabs(a->z - b->z) < 0.0001f)
        return 1;
    return 0;
}

// 0x524960  Confidence: exact.
void vector3_normal(vector3 *out, const vector3 *p0, const vector3 *p1, const vector3 *p2)
{
    vector3_perp(out, p0, p1, p2);
    out->normalize();
}

// 0x524990  Confidence: exact. (p1 - p0) x (p2 - p1).
void vector3_perp(vector3 *out, const vector3 *p0, const vector3 *p1, const vector3 *p2)
{
    *out = (*p1 - *p0).cross(*p2 - *p1);
}

// 0x5249e0  Confidence: likely. The angle between a and b, its sign from (a x b) . axis (the cross
// product of the original vectors).
float vector3_angle_and_axis(const vector3 *a, const vector3 *b, const vector3 *axis)
{
    vector3 na;
    vector3 nb;
    vector3 nc;
    float angle;

    nb = a->normalized();
    nc = b->normalized();
    na = axis->normalized();
    angle = (float)acos(nb.dot(nc));
    if (a->cross(*b).dot(na) < 0.0f)
        return -angle;
    return angle;
}

// 0x524ab0  Confidence: exact.
float direction_dot(const vector3 *dir, const vector3 *from, const vector3 *to)
{
    return dir->dot((*to - *from).normalized());
}

// 0x524ae0  Confidence: exact (VC6 8966 byte match: the distance is negated in place before the
// scale; it lives in the n argument's slot).
void project_onto_plane(vector3 *out, const vector3 *v, const vector3 *n, const vector3 *p)
{
    float d = -p->dot(*n);

    d = v->dot(*n) + d;
    *out = *v;
    d = -d;
    *out += *n * d;
}

// 0x524b50  Confidence: exact (VC6 byte match, m-1020, D11102: the angle goes through a float local, fld /
// fstp into the argument; the second vector is constructed but not used). Into the frame of a
// matrix built on axis, rotated by a bank of angle, back out.
void vector3_rotate_about_axis(vector3 *out, const vector3 *v, float angle, const vector3 *origin,
                               const vector3 *axis)
{
    vector3 p;
    vector3 unused;
    matrix frame;

    frame.set_from_forward(*axis);
    float a = angle;
    matrix spin(0.0f, a, 0.0f);
    p = *v - *origin;
    p.rotate(frame);
    p.rotate(spin);
    p.unrotate(frame);
    *out = p + *origin;
}

// 0x524c10  Confidence: exact.
void vector3_rotate_towards(vector3 *out, const vector3 *from, const vector3 *to, float t)
{
    vector3 a;
    vector3 b;

    a = from->normalized_or_x();
    b = to->normalized_or_x();
    vector3_rotate_towards_unit(out, &a, &b, t);
}

// 0x524c80  Confidence: exact (VC6 8966 byte match: the origin is a temporary, not a named local; the
// angle lives in the to argument's slot).
void vector3_rotate_towards_unit(vector3 *out, const vector3 *from, const vector3 *to, float t)
{
    vector3 axis;
    float angle;

    axis = from->cross(*to);
    axis.normalize();
    angle = -(float)acos(from->dot(*to));
    vector3_rotate_about_axis(out, from, angle * t, &(const vector3 &)vector3(0.0f, 0.0f, 0.0f), &axis);
}

// 0x524d10  Confidence: exact (the assignment temporaries). Ritter's sphere: the widest of the
// x / y / z extreme pairs gives the first sphere, which grows over every point outside it.
void bounding_sphere_from_points(const vector3 *pts, int n, vector3 *center, float *radius)
{
    vector3 d;
    vector3 xmin, xmax, ymin, ymax, zmin, zmax;
    vector3 dia1, dia2;
    float dx2, dy2, dz2, maxspan, rad, rad_sq, old_to_p_sq, old_to_p, old_to_new;
    int i;

    xmin.x = ymin.y = zmin.z = 1.0e8f;
    xmax.x = ymax.y = zmax.z = -1.0e8f;
    for (i = 0; i < n; i++) {
        if (pts[i].x < xmin.x)
            xmin = pts[i];
        if (pts[i].x > xmax.x)
            xmax = pts[i];
        if (pts[i].y < ymin.y)
            ymin = pts[i];
        if (pts[i].y > ymax.y)
            ymax = pts[i];
        if (pts[i].z < zmin.z)
            zmin = pts[i];
        if (pts[i].z > zmax.z)
            zmax = pts[i];
    }
    d = xmax - xmin;
    dx2 = d.len_sq();
    d = ymax - ymin;
    dy2 = d.len_sq();
    d = zmax - zmin;
    dz2 = d.len_sq();
    dia1 = xmin;
    dia2 = xmax;
    maxspan = dx2;
    if (dy2 > maxspan) {
        maxspan = dy2;
        dia1 = ymin;
        dia2 = ymax;
    }
    if (dz2 > maxspan) {
        dia1 = zmin;
        dia2 = zmax;
    }
    *center = dia1 + dia2;
    *center *= 0.5f;
    d = dia2 - *center;
    rad_sq = d.len_sq();
    rad = (float)sqrt(rad_sq);
    for (i = 0; i < n; i++) {
        d = pts[i] - *center;
        old_to_p_sq = d.len_sq();
        if (old_to_p_sq > rad_sq) {
            old_to_p = (float)sqrt(old_to_p_sq);
            rad = (rad + old_to_p) * 0.5f;
            rad_sq = rad * rad;
            old_to_new = old_to_p - rad;
            center->x = (rad * center->x + old_to_new * pts[i].x) / old_to_p;
            center->y = (old_to_new * pts[i].y + rad * center->y) / old_to_p;
            center->z = (rad * center->z + old_to_new * pts[i].z) / old_to_p;
        }
    }
    *radius = rad;
}

// 0x525100  Confidence: exact. The fan triangles (v0, vi, vi+1): the total area first, then each
// triangle's centroid weighted by its share.
float polygon_calc_centroid(int n, const vector3 *verts, vector3 *centroid)
{
    vector3 normal;
    vector3 c;
    float area, a;
    int i;

    centroid->zero();
    vector3_perp(&normal, &verts[0], &verts[1], &verts[2]);
    area = normal.len() * 0.5f;
    for (i = 2; i < n - 1; i++) {
        vector3_perp(&normal, &verts[0], &verts[i], &verts[i + 1]);
        area += normal.len() * 0.5f;
    }
    vector3_perp(&normal, &verts[0], &verts[1], &verts[2]);
    a = normal.len() * 0.5f;
    c.zero();
    for (i = 0; i < 3; i++)
        c += verts[i];
    c /= 3.0f;
    *centroid += c * (a / area);
    for (i = 2; i < n - 1; i++) {
        vector3_perp(&normal, &verts[0], &verts[i], &verts[i + 1]);
        a = normal.len() * 0.5f;
        c.zero();
        c += verts[0];
        c += verts[i];
        c += verts[i + 1];
        c /= 3.0f;
        *centroid += c * (a / area);
    }
    return area;
}

// 0x5252c0  Confidence: exact.
void vector3_world_to_local(vector3 *v, const vector3 *origin, const matrix *orient)
{
    *v = *v - *origin;
    v->rotate(*orient);
}

// 0x525300  Confidence: exact.
void vector3_local_to_world(vector3 *v, const vector3 *origin, const matrix *orient)
{
    *v = v->unrotated(*orient) + *origin;
}

// 0x525340  Confidence: exact.
float vector2_distance(const vector2 *a, const vector2 *b)
{
    return (*b - *a).len();
}

// 0x525370  Confidence: exact.
float vector2_distance_squared(const vector2 *a, const vector2 *b)
{
    return (*b - *a).len_sq();
}

// 0x5253a0  Confidence: likely (the dot product written out; two results live in argument slots).
float vector2_dist_sq_to_line(const vector2 *p, const vector2 *a, const vector2 *b)
{
    vector2 ab = *b - *a;
    vector2 ap = *p - *a;
    float d = ab.x * ap.x + ab.y * ap.y;
    float l = ap.len_sq();

    d = d * d;
    return l - d / ab.len_sq();
}

// 0x525410  Confidence: likely (decomp32's description; the dot product written out).
float vector2_closest_point_on_segment(const vector2 *p, const vector2 *a, const vector2 *b, vector2 *out)
{
    vector2 ab = *b - *a;
    vector2 ap = *p - *a;
    float t = (ab.x * ap.x + ab.y * ap.y) / ab.len_sq();

    t = float_min(float_max(t, 0.0f), 1.0f);
    *out = *a + t * ab;
    return (*out - *p).len_sq();
}

// 0x5254d0  Confidence: exact (the root order swap; several values live in argument slots). The
// circle of radius margin around p against the segment a-b.
bool vector2_segment_circle_intersect(const vector2 *a, const vector2 *b, const vector2 *p, float margin, float *t)
{
    vector2 d;
    vector2 e;
    float c, r2, e2, k, disc, s, t1, t2, tmp;

    d = *a - *p;
    c = d.len_sq();
    r2 = margin * margin;
    if (c <= r2) {
        *t = 0.0f;
        return true;
    }
    e = *b - *a;
    e2 = e.len_sq();
    k = d.dot(e);
    disc = k * k - (c - r2) * e2;
    if (disc < 0.0f)
        return false;
    s = (float)sqrt(disc);
    t1 = (s - k) / e2;
    t2 = (-k - s) / e2;
    if (t2 < t1) {
        tmp = t1;
        t1 = t2;
        t2 = tmp;
    }
    if (t1 >= 0.0f && t1 <= 1.0f) {
        *t = t1;
        return true;
    }
    if (t2 >= 0.0f && t2 <= 1.0f) {
        *t = t2;
        return true;
    }
    return false;
}

// ---- COMDATs at the end of the object (bodies in vsdk/math/vector.h) -------------------------------
// 0x525650 vector2__vector2 (x, y). Confidence: exact.
// 0x525670 vector3__operator_div_assign. Confidence: exact.
// 0x5256a0 vector2__operator_sub. Confidence: exact.
// 0x5256d0 vector2_scaled = operator*(float, const vector2 &). Confidence: exact.
// 0x525700 vector2__dot. Confidence: exact.
