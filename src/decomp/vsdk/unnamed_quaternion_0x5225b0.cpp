// unnamed_quaternion_0x5225b0.cpp - Sum.exe v1.40, code 0x5225b0-0x523f29: quaternion and cquat
// (vsdk\math in sum.symbols). The file name is a placeholder: no path string, no strings.tbl block;
// Summoner 2 has no unit with these functions.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range. The two static initialisers (static_initializers entries 596 / 597, after the sound
// unit's 595) open the object; the regular functions run to cquat::set (0x523cc0); the inline members
// it calls close it as COMDATs (0x523d20-0x523f29, listed at the end). The vector unit's initialiser
// (entry 598) follows at 0x523f30. .bss 0x2cbf498-0x2cbf4af, no .data.
//
// Build flags: no inline expansion for the class members (vector3 cross / scaled / + / dot, the
// quaternion constructor are called); the length computations are written out in the source (FSQRT
// inline, so /Oi). vc6-flags.txt: /Ox /Ob0 /GX /MT /Ow (the library's /Ox /Ow, D7502; /Ob0 for this unit,
// D7900).

#include <math.h>
#include "vsdk/math/quaternion.h"
#include "vsdk/math/matrix.h"

#define QUAT_EPSILON 1.0e-6f          // 0x581b80 (slerp) and the w floor of slerp / squad
#define QUAT_HALF_PI 1.5707964f       // 0x581920

// The identity constants (0x5225b0 / 0x5225e0) are defined at the end of the file (D7900).

// 0x522610  Confidence: exact (the case order of the switch; m.uvec.y + m.fvec.z is kept for case 0).
// Shoemake's conversion: from the trace when it is >= 0, else from the largest diagonal element.
quaternion::quaternion(const matrix &m)
{
    float trace, s;
    int i;

    trace = m.rvec.x + (m.uvec.y + m.fvec.z);
    if (trace >= 0.0f) {
        s = (float)sqrt(trace + 1.0f);
        w = s * 0.5f;
        s = 0.5f / s;
        x = (m.fvec.y - m.uvec.z) * s;
        y = (m.rvec.z - m.fvec.x) * s;
        z = (m.uvec.x - m.rvec.y) * s;
        return;
    }
    i = 0;
    if (m.uvec.y > m.rvec.x)
        i = 1;
    if (m.fvec.z > (&m.rvec.x)[i * 4])
        i = 2;
    switch (i) {
    case 0:
        s = (float)sqrt(m.rvec.x - (m.uvec.y + m.fvec.z) + 1.0f);
        x = s * 0.5f;
        s = 0.5f / s;
        y = (m.uvec.x + m.rvec.y) * s;
        z = (m.fvec.x + m.rvec.z) * s;
        w = (m.fvec.y - m.uvec.z) * s;
        break;
    case 1:
        s = (float)sqrt(m.uvec.y - (m.rvec.x + m.fvec.z) + 1.0f);
        y = s * 0.5f;
        s = 0.5f / s;
        z = (m.fvec.y + m.uvec.z) * s;
        x = (m.uvec.x + m.rvec.y) * s;
        w = (m.rvec.z - m.fvec.x) * s;
        break;
    case 2:
        s = (float)sqrt(m.fvec.z - (m.uvec.y + m.rvec.x) + 1.0f);
        z = s * 0.5f;
        s = 0.5f / s;
        x = (m.fvec.x + m.rvec.z) * s;
        y = (m.fvec.y + m.uvec.z) * s;
        w = (m.uvec.x - m.rvec.y) * s;
        break;
    }
}

// 0x522790  Confidence: likely. The same conversion, normalised at the end, with two differences
// reproduced from the code: the largest-diagonal test compares m.fvec.z with m.uvec.y (not with the
// element chosen so far), and case 2 reads m.rvec.y (m[0][1]) where m.rvec.x belongs (game bug,
// NOTES.md).
quaternion::quaternion(const matrix *m)
{
    float trace, s;
    int i;

    trace = m->rvec.x + (m->uvec.y + m->fvec.z);
    if (trace >= 0.0f) {
        s = (float)sqrt(trace + 1.0f);
        w = s * 0.5f;
        s = 0.5f / s;
        x = (m->fvec.y - m->uvec.z) * s;
        y = (m->rvec.z - m->fvec.x) * s;
        z = (m->uvec.x - m->rvec.y) * s;
        normalize();
        return;
    }
    i = 0;
    if (m->uvec.y > m->rvec.x)
        i = 1;
    if (m->fvec.z > m->uvec.y)
        i = 2;
    switch (i) {
    case 0:
        s = (float)sqrt(m->rvec.x - (m->uvec.y + m->fvec.z) + 1.0f);
        x = s * 0.5f;
        s = 0.5f / s;
        y = (m->uvec.x + m->rvec.y) * s;
        z = (m->fvec.x + m->rvec.z) * s;
        w = (m->fvec.y - m->uvec.z) * s;
        break;
    case 1:
        s = (float)sqrt(m->uvec.y - (m->rvec.x + m->fvec.z) + 1.0f);
        y = s * 0.5f;
        s = 0.5f / s;
        z = (m->fvec.y + m->uvec.z) * s;
        x = (m->uvec.x + m->rvec.y) * s;
        w = (m->rvec.z - m->fvec.x) * s;
        normalize();
        return;
    case 2:
        s = (float)sqrt(m->fvec.z - (m->rvec.y + m->uvec.y) + 1.0f);
        z = s * 0.5f;
        s = 0.5f / s;
        x = (m->fvec.x + m->rvec.z) * s;
        y = (m->fvec.y + m->uvec.z) * s;
        w = (m->uvec.x - m->rvec.y) * s;
        normalize();
        return;
    }
    normalize();
}

// 0x522930  Confidence: likely (the order of the temporaries). (w1 w2 - v1 . v2, w1 v2 + w2 v1 + v1 x v2).
quaternion quaternion::multiply(const quaternion &q) const
{
    vector3 v1;
    vector3 v2;
    float w1, w2;

    v1.x = x;
    v1.y = y;
    v1.z = z;
    v2.x = q.x;
    v2.y = q.y;
    v2.z = q.z;
    w1 = w;
    w2 = q.w;
    vector3 c = v1.cross(v2);
    vector3 v = v2 * w1 + v1 * w2 + c;
    quaternion r;
    r.w = w1 * w2 - v1.dot(v2);
    r.x = v.x;
    r.y = v.y;
    r.z = v.z;
    return quaternion(r);
}

// 0x522a40  Confidence: likely (D15801: the cross product in a named local; the conversions' order differs). The same product in floats (components / 16383), rounded back.
cquat cquat::multiply(const cquat &q) const
{
    vector3 v1;
    vector3 v2;
    float w1, w2;

    v1.x = x * CQUAT_TO_FLOAT;
    v1.y = y * CQUAT_TO_FLOAT;
    v1.z = z * CQUAT_TO_FLOAT;
    v2.x = q.x * CQUAT_TO_FLOAT;
    v2.y = q.y * CQUAT_TO_FLOAT;
    v2.z = q.z * CQUAT_TO_FLOAT;
    w1 = w * CQUAT_TO_FLOAT;
    w2 = q.w * CQUAT_TO_FLOAT;
    vector3 c = v1.cross(v2);
    vector3 v = v2 * w1 + v1 * w2 + c;
    cquat r;
    r.w = (short)((w1 * w2 - v1.dot(v2)) * 16383.0f);
    r.x = (short)(v.x * 16383.0f);
    r.y = (short)(v.y * 16383.0f);
    r.z = (short)(v.z * 16383.0f);
    return cquat(r);
}

// 0x522bf0  Confidence: exact (D18205: w read through a pointer local, so 8966 loads w first in w * z and
// w * x; the position zeros through a volatile pointer keep their stores after the rows). The
// rotation rows into a matrix43, its position zeroed (D4011).
void quaternion::to_matrix(matrix *out) const
{
    matrix43 *m = (matrix43 *)out;
    float yy2 = y * y * 2.0f;
    float zz2 = z * z * 2.0f;
    float xy, wz, wy, xz, xx, yz, wx;
    const float *pw = &w;

    m->m[0][0] = 1.0f - yy2 - zz2;
    xy = x * y;
    wz = *pw * z;
    m->m[0][1] = xy * 2.0f - wz * 2.0f;
    wy = w * y;
    xz = z * x;
    m->m[0][2] = (xz + wy) * 2.0f;
    m->m[1][0] = (wz + xy) * 2.0f;
    xx = 1.0f - x * x * 2.0f;
    m->m[1][1] = xx - zz2;
    yz = z * y;
    wx = *pw * x;
    m->m[1][2] = yz * 2.0f - wx * 2.0f;
    m->m[2][0] = xz * 2.0f - wy * 2.0f;
    m->m[2][1] = (wx + yz) * 2.0f;
    m->m[2][2] = xx - yy2;
    volatile float *p = m->m[3];
    p[0] = 0.0f;
    p[1] = 0.0f;
    p[2] = 0.0f;
}

// 0x522cc0  Confidence: exact (D18206: w through a pointer local, as to_matrix). The same rows into a
// 3x3 matrix.
void quaternion::to_matrix_b(matrix *out) const
{
    float yy2 = y * y * 2.0f;
    float zz2 = z * z * 2.0f;
    float xy, wz, wy, xz, xx, yz, wx;
    const float *pw = &w;

    out->rvec.x = 1.0f - yy2 - zz2;
    xy = x * y;
    wz = *pw * z;
    out->rvec.y = xy * 2.0f - wz * 2.0f;
    wy = w * y;
    xz = z * x;
    out->rvec.z = (xz + wy) * 2.0f;
    out->uvec.x = (wz + xy) * 2.0f;
    xx = 1.0f - x * x * 2.0f;
    out->uvec.y = xx - zz2;
    yz = z * y;
    wx = *pw * x;
    out->uvec.z = yz * 2.0f - wx * 2.0f;
    out->fvec.x = xz * 2.0f - wy * 2.0f;
    out->fvec.y = (wx + yz) * 2.0f;
    out->fvec.z = xx - yy2;
}

// 0x522d90  Confidence: exact.
quaternion quaternion::normalized() const
{
    float s = (float)sqrt(1.0f / (x * x + y * y + z * z + w * w));

    return quaternion(x * s, y * s, z * s, w * s);
}

// 0x522e00  Confidence: exact.
quaternion quaternion::conjugate() const
{
    return quaternion(-x, -y, -z, w);
}

// 0x522e30  Confidence: exact (atan2 as the FPATAN intrinsic).
quaternion quaternion::log() const
{
    float s = (float)sqrt(x * x + y * y + z * z);

    if (s > 0.0f)
        s = (float)atan2(s, w) / s;
    return quaternion(x * s, y * s, z * s, 0.0f);
}

// 0x522ea0  Confidence: exact (D18202: the length into s, copied into a; the binary compares the
// register copy and reads a from memory).
quaternion quaternion::exp() const
{
    float s = (float)sqrt(x * x + y * y + z * z);
    float a = s;

    if (a > 1.0e-10f)
        s = (float)sin(a) / a;
    else
        s = 1.0f;
    return quaternion(x * s, y * s, z * s, (float)cos(a));
}

// 0x522f20  Confidence: exact.
void quaternion::normalize()
{
    float s = (float)sqrt(1.0f / (x * x + y * y + z * z + w * w));

    x *= s;
    y *= s;
    z *= s;
    w *= s;
}

// 0x522f70  Confidence: exact.
void quaternion::negate_xyz()
{
    x = -x;
    y = -y;
    z = -z;
}

// 0x522f90  Confidence: exact.
void quaternion::log_in_place()
{
    float s = (float)sqrt(x * x + y * y + z * z);

    if (s > 0.0f)
        s = (float)atan2(s, w) / s;
    x *= s;
    w = 0.0f;
    y *= s;
    z *= s;
}

// 0x522ff0  Confidence: exact.
void quaternion::exp_in_place()
{
    float a = (float)sqrt(x * x + y * y + z * z);
    float s;

    if (a > 1.0e-10f)
        s = (float)sin(a) / a;
    else
        s = 1.0f;
    x *= s;
    y *= s;
    z *= s;
    w = (float)cos(a);
}

// 0x523050  Confidence: exact. The scale is sqrt(16383^2 / |q|^2) with an integer division, so it
// is the square root of a whole number: 0 for any |q| above one unit (a cquat slightly too long
// collapses to zero), 1 near one unit, never a fine correction (D4012; game bug, NOTES.md).
cquat cquat::normalized() const
{
    int iw = w, iz = z, iy = y, ix = x;
    float s = (float)sqrt((float)((CQUAT_ONE * CQUAT_ONE) / (ix * ix + iy * iy + iz * iz + iw * iw)));

    return cquat((short)(ix * s), (short)(iy * s), (short)(iz * s), (short)(iw * s));
}

// 0x5230f0  Confidence: exact.
cquat cquat::conjugate() const
{
    return cquat(-x, -y, -z, w);
}

// 0x523120  Confidence: exact (D18201: the tx / ty / tz copies of D18200). As quaternion::log on the
// scaled components; the result keeps the 16383 scale (the raw components times atan2(|v|, w) / |v|).
cquat cquat::log() const
{
    float fx = (float)x;
    float sx = fx * CQUAT_TO_FLOAT;
    float fy = (float)y;
    float sy = fy * CQUAT_TO_FLOAT;
    float fz = (float)z;
    float sz = fz * CQUAT_TO_FLOAT;
    float s = (float)sqrt(sx * sx + sy * sy + sz * sz);

    if (s > 0.0f)
        s = (float)atan2(s, w * CQUAT_TO_FLOAT) / s;
    float tx = fx, ty = fy, tz = fz;
    return cquat((short)(tx * s), (short)(ty * s), (short)(tz * s), 0);
}

// 0x5231e0  Confidence: exact (D18203: the length into s copied into a, as quaternion::exp D18202,
// and the tx / ty / tz copies of D18200).
cquat cquat::exp() const
{
    float fx = (float)x;
    float sx = fx * CQUAT_TO_FLOAT;
    float fy = (float)y;
    float sy = fy * CQUAT_TO_FLOAT;
    float fz = (float)z;
    float sz = fz * CQUAT_TO_FLOAT;
    float s = (float)sqrt(sx * sx + sy * sy + sz * sz);
    float a = s;

    if (a > 1.0e-10f)
        s = (float)sin(a) / a;
    else
        s = 1.0f;
    float tx = fx, ty = fy, tz = fz;
    return cquat((short)(tx * s), (short)(ty * s), (short)(tz * s), (short)((float)cos(a) * 16383.0f));
}

// 0x5232b0  Confidence: exact (the integer division of cquat::normalized, D4012).
void cquat::normalize()
{
    int iw = w, iz = z, iy = y, ix = x;
    float s = (float)sqrt((float)((CQUAT_ONE * CQUAT_ONE) / (ix * ix + iy * iy + iz * iz + iw * iw)));

    x = (short)(ix * s);
    y = (short)(iy * s);
    z = (short)(iz * s);
    w = (short)(iw * s);
}

// 0x523350  Confidence: exact.
void cquat::negate_xyz()
{
    x = -x;
    y = -y;
    z = -z;
}

// 0x523370  Confidence: exact (D18200: the raw components copied into tx / ty / tz before the
// products; 8966 then keeps s on the FPU stack and reloads fy / fz as the binary does).
void cquat::log_in_place()
{
    float fx = (float)x;
    float sx = fx * CQUAT_TO_FLOAT;
    float fy = (float)y;
    float sy = fy * CQUAT_TO_FLOAT;
    float fz = (float)z;
    float sz = fz * CQUAT_TO_FLOAT;
    float s = (float)sqrt(sx * sx + sy * sy + sz * sz);

    if (s > 0.0f)
        s = (float)atan2(s, w * CQUAT_TO_FLOAT) / s;
    float tx = fx, ty = fy, tz = fz;
    x = (short)(tx * s);
    y = (short)(ty * s);
    z = (short)(tz * s);
    w = 0;
}

// 0x523430  Confidence: exact (D18204: as cquat::exp, D18203).
void cquat::exp_in_place()
{
    float fx = (float)x;
    float sx = fx * CQUAT_TO_FLOAT;
    float fy = (float)y;
    float sy = fy * CQUAT_TO_FLOAT;
    float fz = (float)z;
    float sz = fz * CQUAT_TO_FLOAT;
    float s = (float)sqrt(sx * sx + sy * sy + sz * sz);
    float a = s;

    if (a > 1.0e-10f)
        s = (float)sin(a) / a;
    else
        s = 1.0f;
    float tx = fx, ty = fy, tz = fz;
    x = (short)(tx * s);
    y = (short)(ty * s);
    z = (short)(tz * s);
    w = (short)((float)cos(a) * 16383.0f);
}

// 0x523500  Confidence: exact (store order w first).
void quaternion::make_identity()
{
    w = 1.0f;
    x = 0.0f;
    y = 0.0f;
    z = 0.0f;
}

// 0x523520  Confidence: exact (the sum starts with w, D504).
float quaternion_dot(const quaternion *a, const quaternion *b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
}

// 0x523550  Confidence: exact.
float cquat_dot(const cquat *a, const cquat *b)
{
    return (a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w) * (1.0f / (16383.0f * 16383.0f));
}

// 0x5235a0  Confidence: exact. t wrapped into 0..1; b or -b, whichever is nearer a; nearly equal
// quaternions give b itself (weights 0 / 1, not a lerp), nearly opposite ones go through the
// perpendicular (-b.y, b.x, -b.w, b.z). A w of exactly 0 becomes 1e-6.
void quaternion_slerp(quaternion *out, const quaternion *a, const quaternion *b, float t)
{
    float d_diff, d_sum, cosom, omega, sinom_inv, scale0, scale1;

    while (t < 0.0f)
        t += 1.0f;
    while (t > 1.0f)
        t -= 1.0f;
    quaternion diff = *a - *b;
    d_diff = quaternion_dot(&diff, &diff);
    quaternion sum = *a + *b;
    d_sum = quaternion_dot(&sum, &sum);
    quaternion q;
    if (d_diff < d_sum)
        q = *b;
    else
        q = -*b;
    cosom = quaternion_dot(a, &q);
    if (1.0f + cosom > QUAT_EPSILON) {
        if (1.0f - cosom <= QUAT_EPSILON) {
            scale0 = 0.0f;
            scale1 = 1.0f;
        } else {
            omega = (float)acos(cosom);
            sinom_inv = 1.0f / (float)sin(omega);
            scale0 = (float)sin((1.0f - t) * omega) * sinom_inv;
            scale1 = (float)sin(omega * t) * sinom_inv;
        }
        out->x = scale0 * a->x + q.x * scale1;
        out->y = scale0 * a->y + q.y * scale1;
        out->z = scale0 * a->z + q.z * scale1;
        out->w = scale0 * a->w + q.w * scale1;
    } else {
        scale0 = (float)sin((1.0f - t) * QUAT_HALF_PI);
        scale1 = (float)sin(t * QUAT_HALF_PI);
        out->x = scale0 * a->x - q.y * scale1;
        out->y = scale0 * a->y + q.x * scale1;
        out->z = scale0 * a->z - q.w * scale1;
        out->w = scale0 * a->w + q.z * scale1;
    }
    if (out->w == 0.0f)
        out->w = QUAT_EPSILON;
}

// 0x5237c0  Confidence: exact. The same on cquats (the result rounded per component; w 0 becomes 1).
void cquat_slerp(cquat *out, const cquat *a, const cquat *b, float t)
{
    float d_diff, cosom, omega, sinom_inv, scale0, scale1;

    while (t < 0.0f)
        t += 1.0f;
    while (t > 1.0f)
        t -= 1.0f;
    cquat q;
    cquat diff = *a - *b;
    d_diff = cquat_dot(&diff, &diff);
    cquat sum = *a + *b;
    if (cquat_dot(&sum, &sum) > d_diff)
        q = *b;
    else
        q = -*b;
    cosom = cquat_dot(a, &q);
    if (1.0f + cosom > QUAT_EPSILON) {
        if (1.0f - cosom <= QUAT_EPSILON) {
            scale0 = 0.0f;
            scale1 = 1.0f;
        } else {
            omega = (float)acos(cosom);
            sinom_inv = 1.0f / (float)sin(omega);
            scale0 = (float)sin((1.0f - t) * omega) * sinom_inv;
            scale1 = (float)sin(omega * t) * sinom_inv;
        }
        out->x = (short)(a->x * scale0 + q.x * scale1);
        out->y = (short)(a->y * scale0 + q.y * scale1);
        out->z = (short)(a->z * scale0 + q.z * scale1);
        out->w = (short)(a->w * scale0 + q.w * scale1);
    } else {
        scale0 = (float)sin((1.0f - t) * QUAT_HALF_PI);
        scale1 = (float)sin(t * QUAT_HALF_PI);
        out->x = (short)(a->x * scale0 - q.y * scale1);
        out->y = (short)(a->y * scale0 + q.x * scale1);
        out->z = (short)(a->z * scale0 - q.w * scale1);
        out->w = (short)(a->w * scale0 + q.z * scale1);
    }
    if (out->w == 0)
        out->w = 1;
}

// 0x523a90  Confidence: exact.
void quaternion_squad(quaternion *out, const quaternion *p, const quaternion *a, const quaternion *b,
                      const quaternion *q, float t)
{
    quaternion pq;
    quaternion ab;

    quaternion_slerp(&pq, p, q, t);
    quaternion_slerp(&ab, a, b, t);
    quaternion_slerp(out, &pq, &ab, (1.0f - t) * t * 2.0f);
}

// 0x523b10  Confidence: likely. The result converted to floats; w 0 becomes 1e-6.
void cquat_squad(quaternion *out, const cquat *p, const cquat *a, const cquat *b, const cquat *q, float t)
{
    cquat pq;
    cquat ab;
    cquat r;

    cquat_slerp(&pq, p, q, t);
    cquat_slerp(&ab, a, b, t);
    cquat_slerp(&r, &pq, &ab, (1.0f - t) * t * 2.0f);
    out->x = r.x * CQUAT_TO_FLOAT;
    out->y = r.y * CQUAT_TO_FLOAT;
    out->z = r.z * CQUAT_TO_FLOAT;
    if (r.w == 0)
        out->w = QUAT_EPSILON;
    else
        out->w = r.w * CQUAT_TO_FLOAT;
}

// 0x523c00  Confidence: exact (r is constructed from the conjugate, the normalized quaternion is a named local).
quaternion quaternion_log_of_delta(const quaternion *a, const quaternion *b)
{
    quaternion r = a->conjugate();
    quaternion n = r.normalized();

    r = n.multiply(*b).log();
    return quaternion(r);
}

// 0x523c60  Confidence: exact.
cquat cquat_log_of_delta(const cquat *a, const cquat *b)
{
    cquat r = a->conjugate();
    cquat n = r.normalized();

    r = n.multiply(*b).log();
    return cquat(r);
}

// 0x523cc0  Confidence: exact (no result: EAX holds the last _ftol).
void cquat::set(const quaternion &q)
{
    x = (short)(q.x * 16383.0f);
    y = (short)(q.y * 16383.0f);
    z = (short)(q.z * 16383.0f);
    w = (short)(q.w * 16383.0f);
}

// The two static initialisers open the object in Sum.exe (0x5225b0, 0x5225e0). They are defined here, after
// the functions, so that the out-of-line quaternion(x, y, z, w) COMDAT is not the first compiled
// quaternion::quaternion: vc6-compare pairs same-named functions in compile order (D7900).
// 0x5225b0 init_cquat_identity (jmp 0x5225c0: cquat(0, 0, 0, 16383)). Confidence: exact.
cquat cquat_identity(0, 0, 0, CQUAT_ONE);         // 0x2cbf498
// 0x5225e0 init_quaternion_identity (jmp 0x5225f0: quaternion(0, 0, 0, 1)). Confidence: exact.
quaternion quaternion_identity(0.0f, 0.0f, 0.0f, 1.0f);   // 0x2cbf4a0

// ---- COMDATs at the end of the object (bodies in vsdk/math/quaternion.h) ---------------------------
// 0x523d20 cquat__cquat_copy. Confidence: exact.
// 0x523d50 cquat__cquat (x, y, z, w). Confidence: exact.
// 0x523d80 cquat__assign. Confidence: exact.
// 0x523db0 cquat__negated (unary -). Confidence: exact.
// 0x523df0 cquat__operator_sub. Confidence: exact.
// 0x523e30 cquat__operator_add. Confidence: exact.
// 0x523e70 quaternion__negated (unary -). Confidence: exact.
// 0x523eb0 quaternion__operator_add. Confidence: exact.
// 0x523ef0 quaternion__operator_sub. Confidence: exact.
