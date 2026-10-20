// unnamed_vfx_channel_0x55b440.cpp - Sum.exe v1.40, code 0x55b440-0x55bd77: the force state of an effect
// instance and the key channels of the effect files (vsdk\geom in sum.symbols). The file name is a
// placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp66 NOTES.md): after the HTTP getter (0x55b431), before the BitArray code (0x55bd80).
// No .data, no .bss (the 2000.0 at 0x5822f8 is a .rdata constant). Built without inline expansion like
// the other vsdk\geom objects: the vector, quaternion and matrix operators and fixed8_to_float are calls
// to their COMDATs. Whether BitArray (0x55bd80-0x55bf88) belongs to this object cannot be told (no data,
// no COMDAT between them); it is kept apart (unnamed_bitarray_0x55bd80.cpp).

#include <math.h>
#include <string.h>
#include "vsdk/geom/effect_mem.h"
#include "vsdk/geom/vfx.h"
#include "vsdk/ca/character_instance.h"

// 0x55b440  Confidence: exact (VC6 merges the three bit clears into one AND 0x1fffffff).
void fx_rec4c::clear()
{
    time = 0.0f;
    fixed = 0;
    on_bone = 0;
    _bit31 = 0;
}

// 0x55b460  Confidence: likely (floor is called twice, as written; the order of the temporaries).
void fx_rec4c::update(vfx_force_desc *f, float t)
{
    int n;

    if (time > 0.0f) {
        prev_pos = pos;
        prev_rot = rot;
    }
    if (!fixed) {
        int f0 = (int)floor(t);
        int f1 = f0 + 1;
        float frac;

        if (f1 >= f->header.frame_word >> 8)
            f1 = f0;
        frac = t - (int)floor(t);
        pos = f->get_pos(f0) * (1.0f - frac) + f->get_pos(f1) * frac;
        quaternion q0;
        quaternion q1;
        q0.unpack_short4(f->get_orient(f0));
        q1.unpack_short4(f->get_orient(f1));
        quaternion_slerp(&rot, &q0, &q1, frac);
        if (on_bone) {
            vector3 bpos;
            matrix brot;
            matrix43 rm;
            matrix43 bm = owner->compute_bone_matrix(bone);
            bm.get_rotation(&brot);
            bm.get_position(&bpos);
            matrix43 world(bpos, brot);
            pos = world.transform(pos);
            rm.set(rot, pos);
            rm.set_product_4x4(world, rm);
            quaternion rq(*(matrix *)&rm);
            rot = rq;
        }
    }
    if (time <= 0.0f) {
        prev_pos = pos;
        prev_rot = rot;
    }
    n = f->header.frame_word >> 8;
    if (!((float)n > t))
        time = (float)(n - 1);
    else
        time = t;
}

// 0x55b730  Confidence: exact (the order of the blends' two products; the frame count read from the header in
// place: the binary has no call to num_frames in this /Ob0 unit, D12704).
void fx_rec4c::get_transform(vfx_force_desc *f, vfx_force_params *out, float t)
{
    int f0 = (int)floor(t);
    int f1 = f0 + 1;
    quaternion q;
    matrix m;
    float frac;
    float inv;

    if (f1 >= f->header.frame_word >> 8)
        f1 = f0;
    frac = t - (int)floor(t);
    inv = 1.0f - frac;
    out->pos = prev_pos * inv + pos * frac;
    quaternion_slerp(&q, &prev_rot, &rot, frac);
    q.to_matrix_b(&m);
    out->dir = vector3(0.0f, 1.0f, 0.0f);
    out->dir.unrotate(m);
    out->dir.normalize_safe();
    out->strength = f->get_channel64(f1) * frac + f->get_channel64(f0) * inv;
    out->v1c = f->get_channel70(f1) * frac + f->get_channel70(f0) * inv;
    out->v20 = f->get_channel7c(f1) * frac + f->get_channel7c(f0) * inv;
    out->v24 = f->get_channel88(f1) * frac + f->get_channel88(f0) * inv;
    out->v28 = f->get_channel94(f1) * frac + f->get_channel94(f0) * inv;
}

// 0x55b960  Confidence: exact.
void vfx_force_desc::relocate(char *header_base, char *base)
{
    header.relocate(header_base, base);
    pos.relocate(base);
    orient.relocate(base);
    ch64.relocate(base);
    ch70.relocate(base);
    ch7c.relocate(base);
    ch88.relocate(base);
    ch94.relocate(base);
}

// 0x55b9c0  Confidence: exact.
vector3 vfx_node::get_pos(int frame)
{
    return pos.eval(frame);
}

// 0x55b9e0  Confidence: exact (the constructor's arguments are evaluated right to left).
vector3 vfx_vec_channel::eval(int frame)
{
    return vector3(fixed8_to_float(x.eval(frame)), fixed8_to_float(y.eval(frame)),
                   fixed8_to_float(z.eval(frame)));
}

// 0x55ba30  Confidence: exact.
cquat vfx_node::get_orient(int frame)
{
    return orient.eval(frame);
}

// 0x55ba50  Confidence: exact.
cquat vfx_quat_channel::eval(int frame)
{
    return cquat(x.eval(frame), y.eval(frame), z.eval(frame), w.eval(frame));
}

// 0x55baa0  Confidence: exact.
float vfx_force_desc::get_channel64(int frame)
{
    return ch64.eval(frame) * 2000.0f;
}

// 0x55bad0  Confidence: exact.
float vfx_force_desc::get_channel70(int frame)
{
    return fixed8_to_float(ch70.eval(frame));
}

// 0x55baf0  Confidence: exact.
float vfx_force_desc::get_channel7c(int frame)
{
    return fixed8_to_float(ch7c.eval(frame));
}

// 0x55bb10  Confidence: exact.
float vfx_force_desc::get_channel88(int frame)
{
    return fixed8_to_float(ch88.eval(frame));
}

// 0x55bb30  Confidence: exact.
float vfx_force_desc::get_channel94(int frame)
{
    return fixed8_to_float(ch94.eval(frame));
}

// 0x55bb50  Confidence: exact.
void vfx_vec_channel::relocate(char *base)
{
    x.relocate(base);
    y.relocate(base);
    z.relocate(base);
}

// 0x55bb80  Confidence: exact.
void vfx_quat_channel::relocate(char *base)
{
    x.relocate(base);
    y.relocate(base);
    z.relocate(base);
    w.relocate(base);
}

// 0x55bbb0  Confidence: exact (D6303: a memset of the 12 bytes, expanded as three dword stores of 0).
void vfx_channel::clear()
{
    memset(this, 0, sizeof(*this));
}

// 0x55bbc0  Confidence: exact.
vfx_channel::vfx_channel()
{
    clear();
}

// 0x55bbd0  Confidence: exact.
void vfx_node_header::relocate(char *header_base, char *base)
{
    reloc_ptr(&name, header_base);
    reloc_ptr(&link, header_base);
    ptr_relocate(&data, base);
}

// 0x55bc00  Confidence: likely (the found-in-run case is laid out after the loop by the compiler).
// The cache (cursor, frame) moves forward only; an earlier frame restarts from the first key.
short vfx_channel::eval(int t)
{
    ushort f = (ushort)t;
    ubyte *p;
    int n;

    if (done)
        return 4000;
    if (f < frame) {
        frame = 0;
        cursor = 0;
    }
    while (cursor < size) {
        p = (ubyte *)data + cursor;
        if (p[0] & 1) {
            n = p[1];
            if (n == 0)
                goto bad;
            if (frame <= f && f < frame + n) {
                p = (ubyte *)data + cursor;
                if (p[0] != 1)
                    return 0;
                if (bytes)
                    return base_value + p[2];
                return base_value + (p[2] | (p[3] << 8));
            }
            if (p[0] != 1)
                goto bad;
            frame += n;
            cursor += bytes ? 3 : 4;
        } else {
            if (f == frame)
                return base_value + (p[1] << 8) + p[0];
            frame++;
            cursor += bytes ? 1 : 2;
        }
    }
    return 0;
bad:
    done = 1;
    return 4000;
}
