// matrix.cpp - Sum.exe v1.40, code 0x50f9d0-0x511c06 (vsdk\math\matrix.cpp), followed by the six header
// inline functions this object emitted out of line (0x511c10-0x511d73: vector3::set / operator==,
// matrix43::transform / set_position / get_position / set_rotation; their bodies are in vector.h and
// matrix.h, DECISIONS.md D500). Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs).
// Target: VC++ 6.0 (cl 12.00.8966). Confidence per function: exact = expect identical code, likely =
// same semantics, small layout or scheduling risk, unsure = the source shape is a guess.
//
// Build: optimised (no frame pointer, x87 values kept on the stack) but without inline expansion: every
// header inline (vector3's constructors, operator=, cross, len, matrix(), matrix::operator=) is called.
//
// Floating point: VC6 without /Op keeps intermediates on the x87 stack (no rounding to float between
// operations) and, as the products below show, commutes and even reassociates sums: matrix43::operator*
// adds the four terms of the three columns of one row in three different orders. So the order of the
// x87 instructions cannot be read back into the source; the expressions are written in natural index
// order (x, y, z terms left to right) and the term order of the disassembly is not reproduced.
//
// Row convention: rows rvec / uvec / fvec (m[0..2] of matrix43, position m[3]); a point is a row vector,
// p' = p.x * rvec + p.y * uvec + p.z * fvec (+ position).

#include <math.h>
#include <string.h>
#include "vsdk/math/matrix.h"
#include "vsdk/os/debug.h"

#define MATRIX_CPP "D:\\projects\\Summoner\\pccode\\vsdk\\math\\matrix.cpp"

// ---- .data (0x5a7480-0x5a74fb) ----------------------------------------------------------------------
// The implicit fourth column of a matrix43 (0, 0, 0, 1) for operator* (Summoner 2 name). set_product_4x4
// has its own copy at 0x5a7490 (its loop ends at 0x5a74a0), defined there as a function static. Then the
// two literals of get_axis_angle's error (0x5a74a0 message, 0x5a74cc path).
float Homogeneous_column[4] = { 0.0f, 0.0f, 0.0f, 1.0f };    // 0x5a7480

// ---- .bss (0x2692ea0-0x2692ef7) --------------------------------------------------------------------
// 0x50f9d0 thunk / 0x50f9e0 and
// 0x50fa30 init_identity_matrix43 (thunk) / 0x50fa40: the dynamic initialisers (initialiser table
// 0x5888f0 / 0x5888f4; no destructor, so no atexit). Confidence: likely (generated).
matrix identity_matrix(vector3(1.0f, 0.0f, 0.0f), vector3(0.0f, 1.0f, 0.0f), vector3(0.0f, 0.0f, 1.0f));   // 0x2692ea0
matrix43 identity_matrix43(vector3(1.0f, 0.0f, 0.0f), vector3(0.0f, 1.0f, 0.0f), vector3(0.0f, 0.0f, 1.0f),
                           vector3(0.0f, 0.0f, 0.0f));                                                     // 0x2692ec8

// The epsilon of rows_match / matrix43::equals (0x581d44, 5e-7).
#define MATRIX_ROW_EPSILON 5e-7f

// ==== matrix (3x3) ===================================================================================

// 0x50faa0  Confidence: exact (the rows' default constructors, then a 9-dword rep movsd; EAX = this is
// set before the copy).
matrix::matrix(const matrix &m)
{
    memcpy(this, &m, sizeof(matrix));
}

// 0x50fad0  Confidence: exact.
matrix::matrix(const vector3 &r, const vector3 &u, const vector3 &f)
{
    rvec = r;
    uvec = u;
    fvec = f;
}

// 0x50fb30  Confidence: exact.
matrix::matrix(float sp, float cp, float sb, float cb, float sh, float ch)
{
    set_from_sincos(sp, cp, sb, cb, sh, ch);
}

// 0x50fb80  Confidence: exact.
matrix::matrix(float p, float b, float h)
{
    set_from_angles(p, b, h);
}

// 0x50fbc0  Confidence: likely (the term order of each sum is the compiler's, see the file comment).
// result.R.c = m.R.x * rvec.c + m.R.y * uvec.c + m.R.z * fvec.c: each row of m transformed by this
// ("m, then this" in the row convention). The row temporaries are built f, u, r (arguments right to
// left).
matrix matrix::operator*(const matrix &m) const
{
    return matrix(vector3(m.rvec.x * rvec.x + m.rvec.y * uvec.x + m.rvec.z * fvec.x,
                          m.rvec.x * rvec.y + m.rvec.y * uvec.y + m.rvec.z * fvec.y,
                          m.rvec.x * rvec.z + m.rvec.y * uvec.z + m.rvec.z * fvec.z),
                  vector3(m.uvec.x * rvec.x + m.uvec.y * uvec.x + m.uvec.z * fvec.x,
                          m.uvec.x * rvec.y + m.uvec.y * uvec.y + m.uvec.z * fvec.y,
                          m.uvec.x * rvec.z + m.uvec.y * uvec.z + m.uvec.z * fvec.z),
                  vector3(m.fvec.x * rvec.x + m.fvec.y * uvec.x + m.fvec.z * fvec.x,
                          m.fvec.x * rvec.y + m.fvec.y * uvec.y + m.fvec.z * fvec.y,
                          m.fvec.x * rvec.z + m.fvec.y * uvec.z + m.fvec.z * fvec.z));
}

// 0x50fcf0  Confidence: likely (term order). result[i][c] = column c of this . column i of m (m^T * this).
// Only caller rotate_between.
matrix matrix::multiply_by_transpose(const matrix &m) const
{
    return matrix(vector3(rvec.x * m.rvec.x + uvec.x * m.uvec.x + fvec.x * m.fvec.x,
                          rvec.y * m.rvec.x + uvec.y * m.uvec.x + fvec.y * m.fvec.x,
                          rvec.z * m.rvec.x + uvec.z * m.uvec.x + fvec.z * m.fvec.x),
                  vector3(rvec.x * m.rvec.y + uvec.x * m.uvec.y + fvec.x * m.fvec.y,
                          rvec.y * m.rvec.y + uvec.y * m.uvec.y + fvec.y * m.fvec.y,
                          rvec.z * m.rvec.y + uvec.z * m.uvec.y + fvec.z * m.fvec.y),
                  vector3(rvec.x * m.rvec.z + uvec.x * m.uvec.z + fvec.x * m.fvec.z,
                          rvec.y * m.rvec.z + uvec.y * m.uvec.z + fvec.y * m.fvec.z,
                          rvec.z * m.rvec.z + uvec.z * m.uvec.z + fvec.z * m.fvec.z));
}

// 0x50fe20  Confidence: likely (term order). result[i][c] = m row i . this row c (m * this^T); caller
// gr_start_instance.
matrix matrix::multiply_transposed(const matrix &m) const
{
    return matrix(vector3(m.rvec.x * rvec.x + m.rvec.y * rvec.y + m.rvec.z * rvec.z,
                          m.rvec.x * uvec.x + m.rvec.y * uvec.y + m.rvec.z * uvec.z,
                          m.rvec.x * fvec.x + m.rvec.y * fvec.y + m.rvec.z * fvec.z),
                  vector3(m.uvec.x * rvec.x + m.uvec.y * rvec.y + m.uvec.z * rvec.z,
                          m.uvec.x * uvec.x + m.uvec.y * uvec.y + m.uvec.z * uvec.z,
                          m.uvec.x * fvec.x + m.uvec.y * fvec.y + m.uvec.z * fvec.z),
                  vector3(m.fvec.x * rvec.x + m.fvec.y * rvec.y + m.fvec.z * rvec.z,
                          m.fvec.x * uvec.x + m.fvec.y * uvec.y + m.fvec.z * uvec.z,
                          m.fvec.x * fvec.x + m.fvec.y * fvec.y + m.fvec.z * fvec.z));
}

// 0x50ff50  Confidence: exact.
// True when every row of this has a dot product within 5e-7 of 1 with the same row of m.
bool matrix::rows_match(const matrix &m) const
{
    if ((float)fabs(m.rvec.dot_xyz(rvec.x, rvec.y, rvec.z) - 1.0f) > MATRIX_ROW_EPSILON)
        return false;
    if ((float)fabs(m.uvec.dot_xyz(uvec.x, uvec.y, uvec.z) - 1.0f) > MATRIX_ROW_EPSILON)
        return false;
    if ((float)fabs(m.fvec.dot_xyz(fvec.x, fvec.y, fvec.z) - 1.0f) > MATRIX_ROW_EPSILON)
        return false;
    return true;
}

// 0x50fff0  Confidence: exact.
matrix::matrix(const vector3 &axis, float angle)
{
    set_from_axis_angle(axis, angle);
}

// 0x510020  Confidence: exact (sbsh and cbch stay on the x87 stack, cbsh and sbch are spilled; store
// order as written).
void matrix::set_from_sincos(float sp, float cp, float sb, float cb, float sh, float ch)
{
    float sbsh, cbch, cbsh, sbch;

    sbsh = sb * sh;
    cbch = cb * ch;
    cbsh = cb * sh;
    sbch = sb * ch;

    rvec.x = cbch + sp * sbsh;
    uvec.z = sbsh + sp * cbch;
    uvec.x = sp * cbsh - sbch;
    rvec.z = sp * sbch - cbsh;
    fvec.x = sh * cp;
    rvec.y = sb * cp;
    uvec.y = cb * cp;
    fvec.z = ch * cp;
    fvec.y = -sp;
}

// 0x5100c0  Confidence: exact (fsin / fcos intrinsics; arguments evaluated right to left, cos(h) first).
void matrix::set_from_angles(float p, float b, float h)
{
    set_from_sincos((float)sin(p), (float)cos(p), (float)sin(b), (float)cos(b), (float)sin(h), (float)cos(h));
}

// 0x510110  Confidence: exact (the products are computed in the order declared; 2 * a is compiled as
// a + a; 2yy, 2zz and 1 - 2xx are reused).
// The rotation of the unit quaternion (axis * sin(angle / 2), cos(angle / 2)).
void matrix::set_from_axis_angle(const vector3 &axis, float angle)
{
    float half = angle * 0.5f;
    float s = (float)sin(half);
    float x = s * axis.x;
    float y = s * axis.y;
    float z = s * axis.z;
    float w = (float)cos(half);
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = y * x, xz = z * x, yz = z * y;
    float wx = w * x, wy = w * y, wz = w * z;

    rvec.x = 1.0f - 2.0f * yy - 2.0f * zz;
    rvec.y = 2.0f * (wz + xy);
    rvec.z = 2.0f * xz - 2.0f * wy;
    uvec.x = 2.0f * xy - 2.0f * wz;
    uvec.y = 1.0f - 2.0f * xx - 2.0f * zz;
    uvec.z = 2.0f * (wx + yz);
    fvec.x = 2.0f * (wy + xz);
    fvec.y = 2.0f * yz - 2.0f * wx;
    fvec.z = 1.0f - 2.0f * xx - 2.0f * yy;
}

// 0x510240  Confidence: exact (fvec.x goes through a stack copy before the first compare; the source
// of that copy is not known).
// Heading from fvec.x / fvec.z, pitch from -fvec.y, bank from rvec.y / uvec.y. Game bug (latent): the
// cos(pitch) estimate MULTIPLIES by sin(h) / cos(h) (`fmul` at 0x5102b2 / 0x5102bd) where dividing
// would recover cos(pitch) (fvec.x = cos p sin h, fvec.z = cos p cos h): the value is cos p * sin^2 h
// (or cos^2 h), so the pitch is wrong unless h is a multiple of 90 degrees or fvec.y is 0 (upright
// characters: mp_send_obj_update, character_save_state). The bank only uses the ratio and is right.
void matrix::extract_angles(float *p, float *b, float *h) const
{
    float sinh, cosh, cosp, sinb, cosb;

    if (fvec.x == 0.0f && fvec.z == 0.0f)
        *h = 0.0f;
    else
        *h = atan2_fast(fvec.x, fvec.z);

    sinh = (float)sin(*h);
    cosh = (float)cos(*h);

    if ((float)fabs(sinh) > (float)fabs(cosh))
        cosp = sinh * fvec.x;
    else
        cosp = cosh * fvec.z;

    if (cosp == 0.0f && fvec.y == 0.0f)
        *p = 0.0f;
    else
        *p = atan2_fast(-fvec.y, cosp);

    if (cosp == 0.0f) {
        *b = 0.0f;
        return;
    }

    sinb = rvec.y / cosp;
    cosb = uvec.y / cosp;
    if (sinb == 0.0f && cosb == 0.0f) {
        *b = 0.0f;
        return;
    }
    *b = atan2_fast(sinb, cosb);
}

// 0x510390  Confidence: likely (the order of the index tests and of the case blocks).
// The rotation axis and angle. Near the identity: axis (1, 0, 0), angle 0. Near a half turn the axis
// comes from the largest diagonal element (angle pi); otherwise from the antisymmetric part and acos.
void matrix::get_axis_angle(vector3 *axis, float *angle) const
{
    float c = (rvec.x + uvec.y + fvec.z - 1.0f) * 0.5f;

    if (c > 0.9999999f) {
        *angle = 0.0f;
        axis->set(1.0f, 0.0f, 0.0f);
        return;
    }
    if (c > -0.9999999f) {
        *angle = (float)acos(c);
        axis->x = uvec.z - fvec.y;
        axis->y = fvec.x - rvec.z;
        axis->z = rvec.y - uvec.x;
        axis->normalize();
        return;
    }

    *angle = 3.14159274f;     // 0x40490fdb
    int i = 0;
    if (uvec.y > rvec.x) {
        i = 1;
        if (fvec.z > uvec.y)
            i = 2;
    } else if (fvec.z > rvec.x) {
        i = 2;
    }

    float s;
    switch (i) {
    case 0:
        s = (float)sqrt(rvec.x + 1.0f);
        axis->x = s;
        axis->y = rvec.y / s;
        axis->z = rvec.z / s;
        break;
    case 1:
        s = (float)sqrt(uvec.y + 1.0f);
        axis->y = s;
        axis->x = uvec.x / s;
        axis->z = uvec.z / s;
        break;
    case 2:
        s = (float)sqrt(fvec.z + 1.0f);
        axis->z = s;
        axis->x = fvec.x / s;
        axis->y = fvec.y / s;
        break;
    default:
        VSDK_ERROR(MATRIX_CPP, 505, "invalid index - this should never happen");
    }
    axis->normalize();
}

// 0x510510  Confidence: exact.
// True when a row or a column is exactly (0, 0, 0).
int matrix::has_zero_axis() const
{
    if (fvec.x == 0.0f && fvec.y == 0.0f && fvec.z == 0.0f)
        return 1;
    if (rvec.x == 0.0f && rvec.y == 0.0f && rvec.z == 0.0f)
        return 1;
    if (uvec.x == 0.0f && uvec.y == 0.0f && uvec.z == 0.0f)
        return 1;
    if (fvec.x == 0.0f && rvec.x == 0.0f && uvec.x == 0.0f)
        return 1;
    if (fvec.y == 0.0f && rvec.y == 0.0f && uvec.y == 0.0f)
        return 1;
    if (fvec.z == 0.0f && rvec.z == 0.0f && uvec.z == 0.0f)
        return 1;
    return 0;
}

// 0x510660  Confidence: exact.
int matrix::is_valid() const
{
    return fvec.is_valid() && uvec.is_valid() && rvec.is_valid();
}

// 0x5106a0  Confidence: likely (term and factor order are the compiler's).
float matrix::determinant() const
{
    return rvec.x * uvec.y * fvec.z + rvec.z * uvec.x * fvec.y + rvec.y * uvec.z * fvec.x
         - rvec.z * uvec.y * fvec.x - rvec.x * uvec.z * fvec.y - rvec.y * uvec.x * fvec.z;
}

// 0x5106e0  Confidence: exact (same body as orthogonalize below, returning the copy).
matrix matrix::orthogonalized_from_fvec() const
{
    matrix m;

    m.fvec = fvec.normalized();
    float ulen = uvec.len();
    float rlen = rvec.len();
    if (ulen <= 0.0f) {
        if (rlen <= 0.0f) {
            if (m.fvec.x == 0.0f && m.fvec.z == 0.0f && m.fvec.y != 0.0f)
                m.uvec.set(0.0f, 0.0f, 1.0f);
            else
                m.uvec.set(0.0f, 1.0f, 0.0f);
        } else {
            m.uvec = m.fvec.cross(rvec);
        }
    } else {
        m.uvec = uvec.normalized();
    }
    m.rvec = m.uvec.cross(m.fvec);
    m.uvec = m.fvec.cross(m.rvec);
    return m;
}

// 0x510830  Confidence: exact (VC6 8966 byte match: the zero-length test first, as the binary lays it
// out; no caller).
// orthogonalize_b on a copy of this.
matrix matrix::orthogonalized() const
{
    matrix m(*this);

    float flen = m.fvec.len();
    float ulen = m.uvec.len();
    float rlen = m.rvec.len();
    if (flen <= 0.0f) {
        if (ulen > 0.0f) {
            if (rlen > 0.0f && !vector3_near_equal(&m.uvec, &m.rvec)) {
                m.fvec = m.uvec.cross(m.rvec);
                m.fvec.normalize();
            } else if (m.uvec.x == 0.0f && m.uvec.y == 0.0f && m.uvec.z != 0.0f) {
                m.fvec.set(1.0f, 0.0f, 0.0f);
            } else {
                m.fvec.set(0.0f, 0.0f, 1.0f);
            }
        }
    } else {
        m.fvec.normalize();
    }
    if (ulen > 0.0f && !vector3_near_equal(&m.fvec, &m.uvec)) {
        m.uvec.normalize();
    } else if (rlen > 0.0f && !vector3_near_equal(&m.fvec, &m.rvec)) {
        m.uvec = m.fvec.cross(m.rvec);
        m.uvec.normalize();
    } else if (m.fvec.x == 0.0f && m.fvec.y != 0.0f && m.fvec.z == 0.0f) {
        m.uvec.set(0.0f, 0.0f, -1.0f);
    } else {
        m.uvec.set(0.0f, 1.0f, 0.0f);
    }
    m.rvec = m.uvec.cross(m.fvec);
    m.rvec.normalize();
    m.uvec = m.fvec.cross(m.rvec);
    return m;
}

// 0x510a80  Confidence: exact (VC6 8966 byte match with the stores written u, r, f).
matrix matrix::transpose() const
{
    vector3 r, u, f;

    u.x = rvec.y; u.y = uvec.y; u.z = fvec.y;
    r.x = rvec.x; r.y = uvec.x; r.z = fvec.x;
    f.x = rvec.z; f.y = uvec.z; f.z = fvec.z;
    return matrix(r, u, f);
}

// 0x510b10  Confidence: exact.
matrix matrix::inverse() const
{
    matrix m(*this);

    m.invert();
    return m;
}

// 0x510b40  Confidence: exact.
// Keeps the direction of fvec; uvec from the old up (or right, or a world axis when both are zero),
// rvec = u x f, then uvec = f x r.
void matrix::orthogonalize()
{
    matrix m;

    m.fvec = fvec.normalized();
    float ulen = uvec.len();
    float rlen = rvec.len();
    if (ulen <= 0.0f) {
        if (rlen <= 0.0f) {
            if (m.fvec.x == 0.0f && m.fvec.z == 0.0f && m.fvec.y != 0.0f)
                m.uvec.set(0.0f, 0.0f, 1.0f);
            else
                m.uvec.set(0.0f, 1.0f, 0.0f);
        } else {
            m.uvec = m.fvec.cross(rvec);
        }
    } else {
        m.uvec = uvec.normalized();
    }
    m.rvec = m.uvec.cross(m.fvec);
    m.uvec = m.fvec.cross(m.rvec);
    *this = m;
}

// 0x510c90  Confidence: exact (VC6 8966 byte match: the zero-length test first; no caller).
// In place: fvec from itself (or u x r, or a world axis), uvec from itself unless parallel to fvec (or
// f x r, or a world axis), then rvec = u x f normalized and uvec = f x r. With fvec and uvec both zero
// fvec stays zero.
void matrix::orthogonalize_b()
{
    float flen = fvec.len();
    float ulen = uvec.len();
    float rlen = rvec.len();
    if (flen <= 0.0f) {
        if (ulen > 0.0f) {
            if (rlen > 0.0f && !vector3_near_equal(&uvec, &rvec)) {
                fvec = uvec.cross(rvec);
                fvec.normalize();
            } else if (uvec.x == 0.0f && uvec.y == 0.0f && uvec.z != 0.0f) {
                fvec.set(1.0f, 0.0f, 0.0f);
            } else {
                fvec.set(0.0f, 0.0f, 1.0f);
            }
        }
    } else {
        fvec.normalize();
    }
    if (ulen > 0.0f && !vector3_near_equal(&fvec, &uvec)) {
        uvec.normalize();
    } else if (rlen > 0.0f && !vector3_near_equal(&fvec, &rvec)) {
        uvec = fvec.cross(rvec);
        uvec.normalize();
    } else if (fvec.x == 0.0f && fvec.y != 0.0f && fvec.z == 0.0f) {
        uvec.set(0.0f, 0.0f, -1.0f);
    } else {
        uvec.set(0.0f, 1.0f, 0.0f);
    }
    rvec = uvec.cross(fvec);
    rvec.normalize();
    uvec = fvec.cross(rvec);
}

// 0x510e70  Confidence: exact (no temporary on the stack: the compiler keeps it in a register).
void matrix::transpose_in_place()
{
    float t;

    t = uvec.x; uvec.x = rvec.y; rvec.y = t;
    t = fvec.x; fvec.x = rvec.z; rvec.z = t;
    t = fvec.y; fvec.y = uvec.z; uvec.z = t;
}

// 0x510ea0  Confidence: exact.
void matrix::set_random()
{
    vector3 v;

    v.random_direction();
    set_from_unit_forward(v);
}

// 0x510ed0  Confidence: exact (the adjugate is formed in a local matrix and divided by the
// determinant; the ninth element is never stored to the local, which the compiler may do on its own).
// Nothing happens for a singular matrix.
void matrix::invert()
{
    matrix t;
    float det = determinant();

    if (det == 0.0f)
        return;

    t.rvec.x =  cross_2d(uvec.y, uvec.z, fvec.y, fvec.z);
    t.rvec.y = -cross_2d(rvec.y, rvec.z, fvec.y, fvec.z);
    t.rvec.z =  cross_2d(rvec.y, rvec.z, uvec.y, uvec.z);
    t.uvec.x = -cross_2d(uvec.x, uvec.z, fvec.x, fvec.z);
    t.uvec.y =  cross_2d(rvec.x, rvec.z, fvec.x, fvec.z);
    t.uvec.z = -cross_2d(rvec.x, rvec.z, uvec.x, uvec.z);
    t.fvec.x =  cross_2d(uvec.x, uvec.y, fvec.x, fvec.y);
    t.fvec.y = -cross_2d(rvec.x, rvec.y, fvec.x, fvec.y);
    t.fvec.z =  cross_2d(rvec.x, rvec.y, uvec.x, uvec.y);

    rvec.x = t.rvec.x / det;
    rvec.y = t.rvec.y / det;
    rvec.z = t.rvec.z / det;
    uvec.x = t.uvec.x / det;
    uvec.y = t.uvec.y / det;
    uvec.z = t.uvec.z / det;
    fvec.x = t.fvec.x / det;
    fvec.y = t.fvec.y / det;
    fvec.z = t.fvec.z / det;
}

// 0x511050  Confidence: exact.
void matrix::make_identity()
{
    rvec.x = 1.0f;
    rvec.y = 0.0f;
    rvec.z = 0.0f;
    uvec.x = 0.0f;
    uvec.y = 1.0f;
    uvec.z = 0.0f;
    fvec.x = 0.0f;
    fvec.y = 0.0f;
    fvec.z = 1.0f;
}

// 0x511080  Confidence: exact (store order of the else branch).
// For a vertical forward the right vector is world x and up is -/+z; else right is horizontal.
void matrix::set_from_forward(const vector3 &fwd)
{
    fvec = fwd.normalized();
    if (fvec.x == 0.0f && fvec.z == 0.0f) {
        rvec.x = 1.0f;
        uvec.z = (fvec.y < 0.0f) ? 1.0f : -1.0f;
        rvec.y = rvec.z = uvec.x = uvec.y = 0.0f;
    } else {
        rvec.z = -fvec.x;
        rvec.x = fvec.z;
        rvec.y = 0.0f;
        rvec.normalize();
        uvec = fvec.cross(rvec);
    }
}

// 0x511140  Confidence: exact (as set_from_forward).
void matrix::set_from_unit_forward(const vector3 &fwd)
{
    fvec = fwd;
    if (fvec.x == 0.0f && fvec.z == 0.0f) {
        rvec.x = 1.0f;
        uvec.z = (fvec.y < 0.0f) ? 1.0f : -1.0f;
        rvec.y = rvec.z = uvec.x = uvec.y = 0.0f;
    } else {
        rvec.z = -fvec.x;
        rvec.x = fvec.z;
        rvec.y = 0.0f;
        rvec.normalize();
        uvec = fvec.cross(rvec);
    }
}

// 0x511200  Confidence: exact.
void matrix::set_from_forward_up(const vector3 &fwd, const vector3 &up)
{
    fvec = fwd.normalized();
    uvec = up;
    if (uvec.normalize_length() == 0.0f) {
        set_from_forward(fwd);
        return;
    }
    rvec = uvec.cross(fvec);
    if (rvec.normalize_length() == 0.0f) {
        set_from_forward(fwd);
        return;
    }
    uvec = fvec.cross(rvec);
}

// 0x5112d0  Confidence: exact.
void matrix::set_from_forward_right(const vector3 &fwd, const vector3 &right)
{
    fvec = fwd.normalized();
    rvec = right;
    if (rvec.normalize_length() == 0.0f) {
        set_from_forward(fwd);
        return;
    }
    uvec = fvec.cross(rvec);
    if (uvec.normalize_length() == 0.0f) {
        set_from_forward(fwd);
        return;
    }
    rvec = uvec.cross(fvec);
}

// ==== matrix43 =======================================================================================

// 0x5113a0  Confidence: exact.
matrix43::matrix43(const matrix &rot)
{
    m[0][0] = rot.rvec.x; m[0][1] = rot.rvec.y; m[0][2] = rot.rvec.z;
    m[1][0] = rot.uvec.x; m[1][1] = rot.uvec.y; m[1][2] = rot.uvec.z;
    m[2][0] = rot.fvec.x; m[2][1] = rot.fvec.y; m[2][2] = rot.fvec.z;
    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;
}

// 0x5113f0  Confidence: exact.
matrix43::matrix43(const vector3 &r, const vector3 &u, const vector3 &f, const vector3 &p)
{
    m[0][0] = r.x; m[0][1] = r.y; m[0][2] = r.z;
    m[1][0] = u.x; m[1][1] = u.y; m[1][2] = u.z;
    m[2][0] = f.x; m[2][1] = f.y; m[2][2] = f.z;
    m[3][0] = p.x; m[3][1] = p.y; m[3][2] = p.z;
}

// 0x511450  Confidence: exact.
matrix43::matrix43(const vector3 &pos, const matrix &rot)
{
    m[0][0] = rot.rvec.x; m[0][1] = rot.rvec.y; m[0][2] = rot.rvec.z;
    m[1][0] = rot.uvec.x; m[1][1] = rot.uvec.y; m[1][2] = rot.uvec.z;
    m[2][0] = rot.fvec.x; m[2][1] = rot.fvec.y; m[2][2] = rot.fvec.z;
    m[3][0] = pos.x; m[3][1] = pos.y; m[3][2] = pos.z;
}

// 0x5114b0  Confidence: exact (a loop over the four rows, the counter is the Homogeneous_column
// pointer; the term order of each sum is the compiler's). this * m with the implicit fourth column.
// The result is a local copied out with the implicit copy constructor (rep movsd).
matrix43 matrix43::operator*(const matrix43 &b) const
{
    matrix43 r;

    for (int i = 0; i < 4; i++) {
        r.m[i][0] = m[i][0] * b.m[0][0] + m[i][1] * b.m[1][0] + m[i][2] * b.m[2][0] + Homogeneous_column[i] * b.m[3][0];
        r.m[i][1] = m[i][0] * b.m[0][1] + m[i][1] * b.m[1][1] + m[i][2] * b.m[2][1] + Homogeneous_column[i] * b.m[3][1];
        r.m[i][2] = m[i][0] * b.m[0][2] + m[i][1] * b.m[1][2] + m[i][2] * b.m[2][2] + Homogeneous_column[i] * b.m[3][2];
    }
    return r;
}

// 0x511570  Confidence: exact (the bool of operator== is normalised again with test / setne).
// The rotation rows match (dot products within 5e-7 of 1) and the positions are exactly equal.
bool matrix43::equals(const matrix43 &o) const
{
    vector3 r, u, f;

    r = vector3(o.m[0][0], o.m[0][1], o.m[0][2]);
    u = vector3(o.m[1][0], o.m[1][1], o.m[1][2]);
    f = vector3(o.m[2][0], o.m[2][1], o.m[2][2]);
    float dr = (float)fabs(r.dot_xyz(m[0][0], m[0][1], m[0][2]) - 1.0f);
    float du = (float)fabs(u.dot_xyz(m[1][0], m[1][1], m[1][2]) - 1.0f);
    float df = (float)fabs(f.dot_xyz(m[2][0], m[2][1], m[2][2]) - 1.0f);
    if (dr > MATRIX_ROW_EPSILON)
        return false;
    if (du > MATRIX_ROW_EPSILON)
        return false;
    if (df > MATRIX_ROW_EPSILON)
        return false;
    return (vector3(o.m[3][0], o.m[3][1], o.m[3][2]) == vector3(m[3][0], m[3][1], m[3][2])) ? true : false;
}

// 0x5116f0  Confidence: likely (the transpose is a nested loop; -position goes through 0x524340, the
// out-of-line negation of the vector unit).
// Inverse of a rigid transform: transposed rotation, position -p rotated.
matrix43 matrix43::inverse() const
{
    matrix43 r(vector3(1.0f, 0.0f, 0.0f), vector3(0.0f, 1.0f, 0.0f), vector3(0.0f, 0.0f, 1.0f),
               vector3(0.0f, 0.0f, 0.0f));
    int i, j;

    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            r.m[i][j] = m[j][i];
    vector3 pos = get_position().negated_b();
    pos = r.transform(pos);
    r.set_position(pos);
    return r;
}

// 0x5117e0  Confidence: exact.
void matrix43::make_identity()
{
    set_rotation(matrix(vector3(1.0f, 0.0f, 0.0f), vector3(0.0f, 1.0f, 0.0f), vector3(0.0f, 0.0f, 1.0f)));
}

// 0x511840  Confidence: exact (the rotation rows of a matrix43 are passed as a matrix).
void matrix43::set(const quaternion &q, const vector3 &pos)
{
    q.to_matrix((matrix *)this);
    set_position(pos);
}

// 0x511860  Confidence: exact (declaration order of the locals from the constructor order).
// Slerp of the two rotations, linear blend of the positions.
matrix43 matrix43_interpolate(const matrix43 &from, const matrix43 &to, float t)
{
    matrix43 r;
    quaternion qa((const matrix &)from);
    quaternion qb((const matrix &)to);
    quaternion q;

    quaternion_slerp(&q, &qa, &qb, t);
    q.to_matrix((matrix *)&r);
    r.m[3][0] = (1.0f - t) * from.m[3][0] + t * to.m[3][0];
    r.m[3][1] = (1.0f - t) * from.m[3][1] + t * to.m[3][1];
    r.m[3][2] = (1.0f - t) * from.m[3][2] + t * to.m[3][2];
    return r;
}

// 0x511920  Confidence: likely (marked exact earlier; differs under cl 12.00.8966, match31). 
matrix43 matrix43_interpolate_quat(const quaternion &qa, const quaternion &qb,
                                   const vector3 &pa, const vector3 &pb, float t)
{
    matrix43 r;
    quaternion q;

    quaternion_slerp(&q, &qa, &qb, t);
    q.to_matrix((matrix *)&r);
    r.m[3][0] = (1.0f - t) * pa.x + t * pb.x;
    r.m[3][1] = (1.0f - t) * pa.y + t * pb.y;
    r.m[3][2] = (1.0f - t) * pa.z + t * pb.z;
    return r;
}

// 0x5119c0  Confidence: exact.
void matrix43_set_interpolated(matrix43 *out, const quaternion &qa, const quaternion &qb,
                               const vector3 &pa, const vector3 &pb, float t)
{
    quaternion q;

    quaternion_slerp(&q, &qa, &qb, t);
    q.to_matrix((matrix *)out);
    out->m[3][0] = (1.0f - t) * pa.x + t * pb.x;
    out->m[3][1] = (1.0f - t) * pa.y + t * pb.y;
    out->m[3][2] = (1.0f - t) * pa.z + t * pb.z;
}

// 0x511a40  Confidence: exact (sum.symbols tags the name I).
// t = a.multiply_by_transpose(b), t = t.multiply_by_transpose(a), this = t * this (operator* above),
// then orthogonalize.
void matrix::rotate_between(const matrix &a, const matrix &b)
{
    matrix t;

    t = a.multiply_by_transpose(b);
    t = t.multiply_by_transpose(a);
    *this = t * *this;
    orthogonalize();
}

// 0x511ab0  Confidence: exact (as operator*; the result goes into this with a block copy, written as
// memcpy because matrix43::operator= is a called inline in this object).
void matrix43::set_product_4x4(const matrix43 &a, const matrix43 &b)
{
    static float Homogeneous_column[4] = { 0.0f, 0.0f, 0.0f, 1.0f };     // 0x5a7490
    matrix43 r;

    for (int i = 0; i < 4; i++) {
        r.m[i][0] = a.m[i][0] * b.m[0][0] + a.m[i][1] * b.m[1][0] + a.m[i][2] * b.m[2][0] + Homogeneous_column[i] * b.m[3][0];
        r.m[i][1] = a.m[i][0] * b.m[0][1] + a.m[i][1] * b.m[1][1] + a.m[i][2] * b.m[2][1] + Homogeneous_column[i] * b.m[3][1];
        r.m[i][2] = a.m[i][0] * b.m[0][2] + a.m[i][1] * b.m[1][2] + a.m[i][2] * b.m[2][2] + Homogeneous_column[i] * b.m[3][2];
    }
    memcpy(this, &r, sizeof(matrix43));
}

// 0x511b70  Confidence: exact.
void matrix43::get_rotation(matrix *out) const
{
    out->rvec.x = m[0][0]; out->rvec.y = m[0][1]; out->rvec.z = m[0][2];
    out->uvec.x = m[1][0]; out->uvec.y = m[1][1]; out->uvec.z = m[1][2];
    out->fvec.x = m[2][0]; out->fvec.y = m[2][1]; out->fvec.z = m[2][2];
}

// 0x511bb0  Confidence: unsure (the source form; the data movement is the disassembly's). No caller.
// Not a transpose: two five-element rotations that mix in the position row,
// m11 <- m01 <- m10 <- m30 <- m12 <- old m11 and m22 <- m02 <- m20 <- m21 <- m31 <- old m22; only m01
// and m02 end up transposed. The code writes four values (old m11, m30, m22, m21) to one stack slot that
// is never read back, so the source had a temporary assigned four times, probably mis-indexed swaps;
// the rotations below give the same result. Game bug (latent: unreferenced).
void matrix43::transpose_in_place()
{
    float t;

    t = m[1][1];
    m[1][1] = m[0][1];
    m[0][1] = m[1][0];
    m[1][0] = m[3][0];
    m[3][0] = m[1][2];
    m[1][2] = t;

    t = m[2][2];
    m[2][2] = m[0][2];
    m[0][2] = m[2][0];
    m[2][0] = m[2][1];
    m[2][1] = m[3][1];
    m[3][1] = t;
}

// ==== header inlines emitted out of line at the end of this object (D500) ===========================
// Bodies in vsdk/math/vector.h and matrix.h; this object was built without inline expansion, so its
// callers call these copies.
// 0x511c10 vector3__set: vector3::set(x, y, z), three dword stores, ret 0xc. Confidence: exact.
// 0x511c30 vector3__operator_eq: vector3::operator==, exact fcomp of x, y, z, returns AL. Confidence: exact.
// 0x511c70 matrix43__transform_point: matrix43::transform(v), rotation rows times v plus the position
// row, into the hidden result. Confidence: exact.
// 0x511cf0 matrix43__set_position: matrix43::set_position(v), the row +0x24. Confidence: exact.
// 0x511d10 matrix43__get_position: matrix43::get_position(), vector3(+0x24, +0x28, +0x2c) by value.
// Confidence: exact.
// 0x511d30 matrix43__set_rotation: matrix43::set_rotation(rot), the nine floats, then the position row
// zeroed (from EAX = 0). Confidence: exact.
