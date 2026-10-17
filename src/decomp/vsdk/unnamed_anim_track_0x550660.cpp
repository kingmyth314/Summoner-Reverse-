// unnamed_anim_track_0x550660.cpp - Sum.exe v1.40, code 0x550660-0x5519b9: anim_track, the key tracks
// of skeletons, cutscene cameras and vfx meshes (vsdk\ca in sum.symbols). The file name is a
// placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout or
// scheduling risk, unsure = the source shape is a guess.
//
// Unit range: after gr_opengl.cpp's last helper (clamp_ubyte 0x550640) and before the polygon clipper's
// static initialiser (0x5519c0, vsdk/unnamed_poly_clip_0x5519c0.cpp). No .data, no .bss. The regular
// functions run to anim_track::relocate (0x551700); the inline accessors of vsdk/ca/skeleton.h follow
// as COMDATs (0x551770-0x55195d) and quaternion::unpack_short4 (0x551960, vsdk/math/quaternion.h)
// closes the object.
//
// Build flags: no inline expansion (every vector3 / cquat operator and the accessors are called, the
// cquat default constructor 0x505530 is called for each local). Float constants: 0.0f 0x57eb84,
// 0.5f 0x57e6b0, 1.0f 0x57e990, 2.0f 0x580200, 3.0f (pushed 0x40400000), 1 / 127 0x581eb8.

#include <math.h>
#include "vsdk/ca/skeleton.h"
#include "vsdk/geom/vfx.h"

// 0x550660  Confidence: exact. b = value + out tangent, a = in tangent + value (old version files store
// the tangents relative to the key).
void anim_track::calc_pos_controls(int key)
{
    *pos_control_b(key) = *pos_key_out(key) + *pos_key_value(key);
    *pos_control_a(key) = *pos_key_value(key) + *pos_key_in(key);
}

// 0x5506e0  Confidence: likely (the source shape of the three interval branches and the order of the
// weight products; the first block's key >= 0 test is in the code). The Kochanek-Bartels tangents of
// rotation key i in log space, from the neighbours (the shorter way round), weighted by the key's
// tension / continuity / bias bytes, turned into the squad inner quaternions a and b.
// unused: a ninth cquat local (0x60 in the frame) is constructed and never used.
void anim_track::calc_rot_tangents(int key, bool loop)
{
    cquat prev_q, next_q, unused, log_next, log_prev, tan_a, tan_b, exp_a, exp_b;
    float c, ac, t, b1, b2, tc, t2c, k1, k2, k3, k4;
    float adj_in, adj_out, half;

    if (key >= 0) {
        if (key == 0) {
            if (loop)
                prev_q = *rot_key_quat(num_rot_keys - 1);
            else
                prev_q = *rot_key_quat(0);
        } else
            prev_q = *rot_key_quat(key - 1);
        if (cquat_dot(&prev_q, rot_key_quat(key)) < 0.0f)
            prev_q = -prev_q;
        log_prev = cquat_log_of_delta(&prev_q, rot_key_quat(key));
    }
    next_q = *rot_key_quat(key == num_rot_keys - 1 ? (loop ? 0 : key) : key + 1);
    if (cquat_dot(&next_q, rot_key_quat(key)) < 0.0f)
        next_q = -next_q;
    log_next = cquat_log_of_delta(rot_key_quat(key), &next_q);

    c = 1.0f - rot_key_continuity(key);
    if (key == 0) {
        int first, last;
        get_rot_time_range(&first, &last);
        // (last - first) - time[n - 1] + time[0] is 0 for any track: the wrap interval is empty.
        float gap = (float)(last - first - rot_key_time(num_rot_keys - 1) + rot_key_time(key));
        half = ((float)rot_key_time(1) + gap - (float)rot_key_time(key)) * 0.5f;
        adj_in = gap / half;
        adj_out = (float)(rot_key_time(1) - rot_key_time(key)) / half;
    } else if (key == num_rot_keys - 1) {
        int first, last;
        get_rot_time_range(&first, &last);
        float gap = (float)(last - first - rot_key_time(num_rot_keys - 1) + rot_key_time(0));
        half = ((float)rot_key_time(num_rot_keys - 1) + gap - (float)rot_key_time(num_rot_keys - 2)) * 0.5f;
        adj_in = gap / half;
        adj_out = (float)(rot_key_time(num_rot_keys - 1) - rot_key_time(num_rot_keys - 2)) / half;
    } else {
        half = (float)(rot_key_time(key + 1) - rot_key_time(key - 1)) * 0.5f;
        adj_in = (float)(rot_key_time(key) - rot_key_time(key - 1)) / half;
        adj_out = (float)(rot_key_time(key + 1) - rot_key_time(key)) / half;
    }

    ac = (float)fabs(rot_key_continuity(key));
    adj_in = ac + adj_in - ac * adj_in;
    adj_out = ac + adj_out - ac * adj_out;
    t = (1.0f - rot_key_tension(key)) * 0.5f;
    b1 = 1.0f - rot_key_bias(key);
    b2 = 2.0f - b1;
    tc = t * c;
    t2c = (2.0f - c) * t;
    k1 = 1.0f - tc * b2 * adj_in;
    k2 = -(t2c * b1 * adj_in);
    k3 = t2c * b2 * adj_out;
    k4 = b1 * tc * adj_out - 1.0f;

    tan_a.x = (short)((log_prev.x * k3 + log_next.x * k4) * 0.5f);
    tan_b.x = (short)((log_prev.x * k1 + log_next.x * k2) * 0.5f);
    tan_a.y = (short)((log_prev.y * k3 + log_next.y * k4) * 0.5f);
    tan_b.y = (short)((log_prev.y * k1 + log_next.y * k2) * 0.5f);
    tan_a.z = (short)((log_prev.z * k3 + log_next.z * k4) * 0.5f);
    tan_b.z = (short)((log_prev.z * k1 + log_next.z * k2) * 0.5f);
    tan_a.w = (short)((log_prev.w * k3 + log_next.w * k4) * 0.5f);
    tan_b.w = (short)((log_prev.w * k1 + log_next.w * k2) * 0.5f);

    exp_a = tan_a.exp();
    exp_b = tan_b.exp();
    *rot_inner_a(key) = rot_key_quat(key)->multiply(exp_a);
    *rot_inner_b(key) = rot_key_quat(key)->multiply(exp_b);
}

// 0x550d10  Confidence: exact. As calc_pos_controls for the scale keys.
void anim_track::calc_scale_controls(int key)
{
    *scale_control_b(key) = *scale_key_out(key) + *scale_key_value(key);
    *scale_control_a(key) = *scale_key_value(key) + *scale_key_in(key);
}

// 0x550d90  Confidence: exact. The cubic Bezier of interval k0 -> k1 (k0 < 0: key k1's value). After
// init_tangents the key's out / in fields hold the control points.
void anim_track::eval_scale_bezier(vector3 *out, int k0, int k1, float t)
{
    if (k0 < 0) {
        *out = *scale_key_value(k1);
        return;
    }
    float u = 1.0f - t;
    *out = *scale_key_value(k0) * u * u * u + *scale_key_out(k0) * 3.0f * t * u * u
         + *scale_key_in(k1) * 3.0f * t * t * u + *scale_key_value(k1) * t * t * t;
}

// 0x550f60  Confidence: exact.
void anim_track::get_rot_time_range(int *first, int *last)
{
    anim_rot_key *k = rot_keys;
    *first = k[0].time;
    *last = k[num_rot_keys - 1].time;
}

// 0x550f90  Confidence: exact. Turns the stored tangents into absolute control points (key out = b,
// key in = a), then the squad quaternions.
void anim_track::init_tangents(bool loop)
{
    int i;

    for (i = 0; i < num_pos_keys; i++)
        calc_pos_controls(i);
    for (i = 0; i < num_pos_keys; i++) {
        *pos_key_out(i) = *pos_control_b(i);
        *pos_key_in(i) = *pos_control_a(i);
    }
    for (i = 0; i < num_rot_keys; i++)
        calc_rot_tangents(i, loop);
    for (i = 0; i < num_scale_keys; i++)
        calc_scale_controls(i);
    for (i = 0; i < num_scale_keys; i++) {
        *scale_key_out(i) = *scale_control_b(i);
        *scale_key_in(i) = *scale_control_a(i);
    }
}

static void anim_track_eval_pos_bezier(anim_track *track, vector3 *out, int k0, int k1, float t);

// 0x551090  Confidence: exact. (0, 0, 0) without keys; clamped to the first / last key.
vector3 anim_track::get_position(int time)
{
    if (num_pos_keys == 0)
        return vector3(0.0f, 0.0f, 0.0f);
    if (time <= pos_keys[0].time)
        return pos_keys[0].pos;
    if (time >= pos_keys[num_pos_keys - 1].time)
        return pos_keys[num_pos_keys - 1].pos;

    int i;
    for (i = 0; i < num_pos_keys && pos_keys[i].time <= time; i++)
        ;
    float t = (float)((float)(time - pos_keys[i - 1].time) / (double)(pos_keys[i].time - pos_keys[i - 1].time));
    vector3 v;
    anim_track_eval_pos_bezier(this, &v, i - 1, i, t);
    return v;
}

// 0x551190  Confidence: exact. cdecl (sum.symbols anim_track_eval_pos_bezier); as eval_scale_bezier.
static void anim_track_eval_pos_bezier(anim_track *track, vector3 *out, int k0, int k1, float t)
{
    if (k0 < 0) {
        *out = *track->pos_key_value(k1);
        return;
    }
    float u = 1.0f - t;
    *out = *track->pos_key_value(k0) * u * u * u + *track->pos_key_out(k0) * 3.0f * t * u * u
         + *track->pos_key_in(k1) * 3.0f * t * t * u + *track->pos_key_value(k1) * t * t * t;
}

static void anim_track_interp_rotation(anim_track *track, quaternion *out, int k0, int k1, float t,
                                       bool squad);

// 0x551350  Confidence: exact. Identity without keys. Before the first key it interpolates keys 0 and
// 1 at t = 0, which reads key 1 of a one-key track (past the array; see NOTES.md).
quaternion anim_track::get_rotation(int time, bool squad)
{
    if (num_rot_keys == 0)
        return quaternion(0.0f, 0.0f, 0.0f, 1.0f);

    int i;
    float t;
    if (time <= rot_keys[0].time) {
        i = 1;
        t = 0.0f;
    } else if (time >= rot_keys[num_rot_keys - 1].time) {
        i = num_rot_keys - 1;
        t = 1.0f;
    } else {
        for (i = 0; i < num_rot_keys && rot_keys[i].time <= time; i++)
            ;
        t = (float)(time - rot_keys[i - 1].time) / (rot_keys[i].time - rot_keys[i - 1].time);
    }
    quaternion q;
    anim_track_interp_rotation(this, &q, i - 1, i, t, squad);
    return q;
}

// 0x551430  Confidence: exact. cdecl. The interval parameter is eased by the keys' ease from / ease to
// (unless k0 == k1); squad through the inner quaternions, else slerp of the two keys.
static void anim_track_interp_rotation(anim_track *track, quaternion *out, int k0, int k1, float t,
                                       bool squad)
{
    if (k0 < 0) {
        out->unpack_short4(*track->rot_key_quat(k1));
        return;
    }
    if (k0 != k1)
        t = tcb_ease(t, track->rot_key_ease_from(k0), track->rot_key_ease_to(k1));
    if (squad) {
        cquat_squad(out, track->rot_key_quat(k0), track->rot_inner_a(k0), track->rot_inner_b(k1),
                    track->rot_key_quat(k1), t);
        return;
    }
    cquat q;
    cquat_slerp(&q, track->rot_key_quat(k0), track->rot_key_quat(k1), t);
    out->unpack_short4(q);
}

// 0x551520  Confidence: exact. As get_position for the scale keys (the interval divisor is an int:
// FIDIV, where get_position converts both).
vector3 anim_track::get_scale(int time)
{
    if (num_scale_keys == 0)
        return vector3(0.0f, 0.0f, 0.0f);
    if (time <= scale_keys[0].time)
        return scale_keys[0].pos;
    if (time >= scale_keys[num_scale_keys - 1].time)
        return scale_keys[num_scale_keys - 1].pos;

    int i;
    for (i = 0; i < num_scale_keys && scale_keys[i].time <= time; i++)
        ;
    float t = (float)(time - scale_keys[i - 1].time) / (scale_keys[i].time - scale_keys[i - 1].time);
    vector3 v;
    eval_scale_bezier(&v, i - 1, i, t);
    return v;
}

// 0x551610  Confidence: exact (t + t for 2t). 3D Studio's ease: t unchanged at 0 and 1 or without ease; the two ease
// fractions are scaled down when they add up to more than 1; quadratic in the ease ranges, linear
// between.
float tcb_ease(float t, float ease_from, float ease_to)
{
    if (t == 0.0f || t == 1.0f)
        return t;
    float s = ease_from + ease_to;
    if (s == 0.0f)
        return t;
    if (s > 1.0f) {
        ease_from = ease_from / s;
        ease_to = ease_to / s;
    }
    float k = 1.0f / (2.0f - ease_from - ease_to);
    if (t < ease_from)
        return k / ease_from * t * t;
    if (1.0f - ease_to > t)
        return k * (t + t - ease_from);
    float u = 1.0f - t;
    return 1.0f - k / ease_to * u * u;
}

// 0x551700  Confidence: exact. The key and control point arrays of a track inside a loaded vfx file.
void anim_track::relocate(char *base)
{
    ptr_relocate(&pos_keys, base);
    ptr_relocate(&rot_keys, base);
    ptr_relocate(&scale_keys, base);
    ptr_relocate(&pos_aux_a, base);
    ptr_relocate(&pos_aux_b, base);
    ptr_relocate(&rot_aux_a, base);
    ptr_relocate(&rot_aux_b, base);
    ptr_relocate(&scale_aux_a, base);
    ptr_relocate(&scale_aux_b, base);
}

// COMDATs (inline members, vsdk/ca/skeleton.h and vsdk/math/quaternion.h), exact, one per line:
// 0x551770 anim_track::pos_key_value
// 0x551790 anim_track::pos_key_in
// 0x5517b0 anim_track::pos_key_out
// 0x5517d0 anim_track::pos_control_a
// 0x5517e0 anim_track::pos_control_b
// 0x5517f0 anim_track::rot_key_time
// 0x551800 anim_track::rot_key_quat
// 0x551820 anim_track::rot_key_continuity
// 0x551840 anim_track::rot_key_tension
// 0x551860 anim_track::rot_key_bias
// 0x551880 anim_track::rot_inner_a
// 0x551890 anim_track::rot_inner_b
// 0x5518a0 anim_track::scale_key_value
// 0x5518c0 anim_track::scale_key_in
// 0x5518e0 anim_track::scale_key_out
// 0x551900 anim_track::scale_control_a
// 0x551910 anim_track::scale_control_b
// 0x551920 anim_track::rot_key_ease_to
// 0x551940 anim_track::rot_key_ease_from
// 0x551960 quaternion::unpack_short4
