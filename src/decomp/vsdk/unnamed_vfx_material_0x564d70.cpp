// unnamed_vfx_material_0x564d70.cpp - Sum.exe v1.40, code 0x564d70-0x56565c: the materials and texture
// layers of the effect files, the tag groups and the effect camera (vsdk\geom in sum.symbols:
// vfx_tex_layer__*, vfx_material__*, vfx_group__*, vfx_camera__*). The file name is a placeholder: no
// path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp66 NOTES.md): after the mesh object's COMDAT block (ptr_relocate 0x564d50 ends at
// 0x564d62), before libjpeg (jpeg_CreateCompress 0x565660). No initialiser, no .data; the empty-string
// literal of reset is 0x2f91a70, the last .bss slot after the mesh object's Dump_mesh_frames. It may be
// up to three objects (materials 0x564d70-0x5653f0, groups 0x565400-0x565461 with the COMDAT 0x565450,
// the camera 0x565470-0x56565c with the COMDATs 0x5655f0 / 0x565610): each ends in what look like its
// inline members. Kept as one file because nothing names a cut.

#include <string.h>
#include <math.h>
#include "vsdk/geom/vfx.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/quaternion.h"

// 0x564d70 / 0x564d80 vfx_tex_layer's empty constructor and destructor (vfx.h, inline; referenced only by
// vfx_material's constructor). Confidence: exact.

// 0x564d90  Confidence: likely (the order of the float factors). Frames run at the bitmap's frame count
// over its animation time, times the layer rate, from the first frame; wrap (modes 0 and 2) or clamp.
int vfx_tex_layer::get_frame_bitmap(float t)
{
    float secs;
    int n;
    int f;

    if (bitmap > 0) {
        n = bm_get_num_frames(bitmap, &secs);
        if (n == 1)
            return bitmap;
        f = imax(0, (int)floor((t - start) * (n / secs * rate) * (1.0f / 15.0f)));
        switch (mode) {
        case 0:
        case 2:
            if (f >= n)
                f %= n;
            break;
        case 1:
            if (f >= n)
                f = n - 1;
            break;
        }
        if (f >= n)
            f = n - 1;
        return bitmap + f + 1;
    }
    return bitmap;
}

// 0x564e40  Confidence: likely. The frame at fraction f of a particle's life (all frames over the life),
// counted from the first frame; negative frames wrap once, then clamp to 0.
int vfx_tex_layer::get_frame_bitmap_frac(float frac)
{
    float secs;
    int n;
    int f;

    if (bitmap > 0) {
        n = bm_get_num_frames(bitmap, &secs);
        if (n == 1)
            return bitmap;
        f = (int)floor(n * frac) - start;
        switch (mode) {
        case 0:
        case 2:
            if (f >= n)
                f %= n;
            break;
        case 1:
            if (f >= n)
                f = n - 1;
            break;
        }
        if (f < 0) {
            f += n;
            if (f < 0)
                f = 0;
        }
        if (f >= n)
            f = n - 1;
        return bitmap + f + 1;
    }
    return bitmap;
}

// 0x564ef0 sum.symbols vfx_tex_layer__ctor: six dwords zeroed, so a 0x18-byte record, not the 0x14-byte
// layer. No caller, no reference; the type is not known (name ours). Confidence: unsure.
struct vfx_zero_record18 {
    vfx_zero_record18();
    int v[6];
};
vfx_zero_record18::vfx_zero_record18()
{
    v[0] = 0;
    v[1] = 0;
    v[2] = 0;
    v[3] = 0;
    v[4] = 0;
    v[5] = 0;
}

// 0x564f10 sum.symbols vfx_stub_ret_564f10: an empty function (`ret`), no caller. Confidence: unsure.
void vfx_stub_564f10()
{
}

// 0x564f20  Confidence: likely (the EH state numbering). No caller: bases are loaded from the files and
// relocated, never constructed.
vfx_material::vfx_material()
{
    reset();
}

// 0x564f80  Confidence: likely (VC6 merges the bit-field stores of flags differently from the field
// order here).
void vfx_material::reset()
{
    int i;

    curve34_mode = -1;                        // the three bits set (or al, 7)
    blend = 0;
    color[0] = 255;
    color[1] = 255;
    color[2] = 255;
    for (i = 0; i < 2; i++)
        layers[i].bitmap = -1;
    num34 = 0;
    curve34 = NULL;
    rate = 15;
    _unk0b = 0;
    name = "";                                // 0x2f91a70
    bitmap = -1;
    _unk40 = 0.6f;
    num44 = 0;
    curve44 = NULL;
    num48 = 0;
    curve48 = NULL;
}

// The curve sampler of the three eval functions (written out in each; the object has no inline
// expansion, so it was no function).
#define VFX_EVAL_CURVE(num, data)                                                                  \
    {                                                                                              \
        float f;                                                                                   \
        int i;                                                                                     \
        float v;                                                                                   \
        f = rate * t * (1.0f / 15.0f);                                                             \
        i = (int)floor(f);                                                                         \
        f -= i;                                                                                    \
        if (i + 1 >= num)                                                                          \
            v = data[num - 1] * (1.0f / 255.0f);                                                   \
        else                                                                                       \
            v = (data[i] * (1.0f - f) + data[i + 1] * f) * (1.0f / 255.0f);                       \
        return float_min(float_max(v, 0.0f), 1.0f);                                                \
    }

// 0x564ff0  Confidence: likely (the switch: modes 0 and 2 are separate cases in the binary).
float vfx_material::eval_curve34(float t)
{
    switch (curve34_mode) {
    case 0:
        break;
    case 1:
        VFX_EVAL_CURVE(num34, curve34)
    case 2:
        break;
    }
    return 0.0f;
}

// 0x5650e0  Confidence: likely.
float vfx_material::eval_curve44(float t)
{
    VFX_EVAL_CURVE(num44, curve44)
}

// 0x5651b0  Confidence: likely.
float vfx_material::eval_curve48(float t)
{
    VFX_EVAL_CURVE(num48, curve48)
}

// 0x565280  Confidence: exact.
void vfx_material::relocate(char *header_base, char *base)
{
    int i;

    for (i = 0; i < 2; i++)
        layers[i].relocate(header_base);
    reloc_ptr(&name, header_base);
    ptr_relocate(&curve34, base);
    ptr_relocate(&curve44, base);
    ptr_relocate(&curve48, base);
    bitmap = 0;
}

// 0x5652e0  Confidence: likely (strlen is the inline `repne scasb`). The material's own bitmap: none for
// an empty name or a "$" name; with the flags, else retried without.
void vfx_material::load_bitmaps(int flags)
{
    int i;

    for (i = 0; i < 2; i++)
        layers[i].load_bitmap(flags);
    if (name != NULL && strlen(name) != 0 && name[0] != '$') {
        bitmap = bm_load(name, -1, flags);
        if (bitmap == 0)
            bitmap = bm_load(name, -1, -1);
    } else {
        bitmap = 0;
    }
}

// 0x565360  Confidence: exact.
void vfx_tex_layer::relocate(char *header_base)
{
    reloc_ptr(&name, header_base);
    bitmap = 0;
}

// 0x565380  Confidence: likely (as vfx_material::load_bitmaps).
void vfx_tex_layer::load_bitmap(int flags)
{
    if (name != NULL && strlen(name) != 0 && name[0] != '$') {
        bitmap = bm_load(name, -1, flags);
        if (bitmap == 0)
            bitmap = bm_load(name, -1, -1);
    } else {
        bitmap = 0;
    }
}

// 0x5653e0 sum.symbols vfx_ptr_pair__ctor: two pointers zeroed; no caller, the type is not known (name
// ours). 0x5653f0: an empty function (`ret`) right after it, no caller (its destructor, perhaps).
// Confidence: unsure.
struct vfx_ptr_pair {
    vfx_ptr_pair();
    void *a;
    void *b;
};
vfx_ptr_pair::vfx_ptr_pair()
{
    a = NULL;
    b = NULL;
}
void vfx_stub_5653f0()
{
}

// 0x565400 vfx_group__relocate  Confidence: exact (the header part as a vfx_node_header, D4606).
void vfx_base_tag::relocate(char *header_base, char *base)
{
    int i;

    ((vfx_node_header *)this)->relocate(header_base, base);
    ptr_relocate(&refs, base);
    for (i = 0; i < count; i++)
        refs[i].relocate(base);
}
// 0x565450 vfx_group_entry__relocate = vfx_tag_ref::relocate (vfx.h, inline). Confidence: exact.

// 0x565470  Confidence: likely (t reused for the fraction). The position (4.12) and rotation blended
// between the two frames around t, frames counted from start_frame; false outside start..end - 1.
bool vfx_camera_desc::get_transform(float t, vector3 *pos, quaternion *rot)
{
    int frame;
    int next;

    frame = (int)floor(t);
    next = frame + 1;
    if (frame < start_frame || next >= end_frame)
        return false;
    frame -= start_frame;
    next -= start_frame;
    t -= (int)floor(t);
    *pos = get_pos(frame) * (1.0f - t) + get_pos(next) * t;
    quaternion qa;
    quaternion qb;
    qa.unpack_short4(((vfx_node *)this)->get_orient(frame));
    qb.unpack_short4(((vfx_node *)this)->get_orient(next));
    quaternion_slerp(rot, &qa, &qb, t);
    return true;
}

// 0x5655c0  Confidence: exact (the header part as a vfx_node_header, D4606).
void vfx_camera_desc::relocate(char *header_base, char *base)
{
    ((vfx_node_header *)this)->relocate(header_base, base);
    pos.relocate(base);
    orient.relocate(base);
}

// 0x5655f0 vfx_camera__get_pos = vfx_camera_desc::get_pos, 0x565610 vfx_vec_channel::eval_fixed12
// (vfx.h, inline). Confidence: exact.
