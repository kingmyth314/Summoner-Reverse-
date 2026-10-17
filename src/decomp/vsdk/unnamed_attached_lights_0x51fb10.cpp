// unnamed_attached_lights_0x51fb10.cpp - Sum.exe v1.40, code 0x51fb10-0x520413: effect light tracks
// and the attached lights (vsdk\geom in sum.symbols). The file name is a placeholder: no path string,
// no strings.tbl block, no .data.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range. The static initialiser of attached_light_slots (entry 591) opens the object; the regular
// functions run to attached_light::reset (0x520210), then come the COMDATs of the inline members it
// calls (0x520280-0x520413, listed at the end). The sound unit's initialiser (entry 592) is at 0x5204c0;
// character_instance::render (0x520420) between them follows the COMDAT block, so it is taken as a unit
// of its own (unnamed_ca_render_0x520420.cpp, D4007). .bss: attached_light_slots 0x2cb45d0-0x2cb540f
// (snd_listener_vel follows).
//
// Build flags: no inline expansion (vector3 / matrix43 members and the track samplers are called).

#include <math.h>
#include "vsdk/geom/vfx.h"
#include "vsdk/ca/character_instance.h"

// 0x51fb10 init_attached_lights (jmp 0x51fb20: `vector constructor iterator` with the out-of-line
// constructor 0x520280). Confidence: exact.
attached_light attached_light_slots[MAX_ATTACHED_LIGHTS];   // 0x2cb45d0

// 0x51fb40  Confidence: exact (thiscall, ret 8). The header, the position channels, the range, the
// on / off bits, then the colour channels.
void vfx_light_track::relocate(char *header_base, char *base)
{
    header.relocate(header_base, base);
    pos.relocate(base);
    range.relocate(base);
    on.relocate(base);
    r.relocate(base);
    g.relocate(base);
    b.relocate(base);
}

// 0x51fb90  Confidence: exact (the four bitfield stores are merged into AND 0x7fffff / OR 0x1f800000).
// Places the light at the track's first frame and, unless no_light, gives it an attached light with the
// first frame's colour. The range is passed unscaled here (the 8.8 value), update_light scales it.
void vfx_light_desc::create_light(vfx_light_track *track, bool no_light)
{
    offset.zero();
    pos = track->get_position(0);
    slot = -1;
    on_bone = 0;
    fixed = 0;
    on = 0;
    if (!no_light) {
        slot = attached_light_create_from_vfx(&pos, 0.0f, (float)track->sample_range(0),
                                              track->sample_r(0) * (1.0f / 255.0f),
                                              track->sample_g(0) * (1.0f / 255.0f),
                                              track->sample_b(0) * (1.0f / 255.0f));
    }
}

// 0x51fc80  Confidence: exact.
void vfx_light_desc::free_light()
{
    if (slot >= 0)
        attached_light_free(slot);
}

// 0x51fcb0  Confidence: exact (thiscall, ret 0x10). Samples the track, then moves the attached light:
// position (unrotated by orient, plus the offset), range, on / off as intensity 1 / 0, colour.
void vfx_light_desc::update_light(void *track, float t, const vector3 *offset, const matrix *orient)
{
    attached_light *a;

    sample(track, t);
    if (offset != NULL)
        this->offset = *offset;
    vector3 p(pos);
    if (orient != NULL)
        p.unrotate(*orient);
    p += this->offset;
    if (slot >= 0) {
        a = &attached_light_slots[slot];
        a->pos = p;
        a->range_far = get_scale();
        a->intensity = on ? 1.0f : 0.0f;
        a->r = r * (1.0f / 255.0f);
        a->g = g * (1.0f / 255.0f);
        a->b = b * (1.0f / 255.0f);
    }
}

// 0x51fdc0  Confidence: exact (floor is called twice, as written; the weights are 0..255 integers).
// Linear interpolation between frame floor(t) and the next one (the same at the last frame): position
// (unless fixed), range and colour; the on / off bit of the next frame; then, for a light on a bone,
// the position through the owner's bone matrix.
void vfx_light_desc::sample(void *track_ptr, float t)
{
    vfx_light_track *track = (vfx_light_track *)track_ptr;
    int f0 = (int)floor(t);
    int f1 = f0 + 1;
    float frac, inv;
    int wf, wi;

    if (f1 >= (int)track->header.num_frames)
        f1 = f0;
    frac = t - (int)floor(t);
    inv = 1.0f - frac;
    wf = (int)(frac * 255.0f);
    wi = (int)(inv * 255.0f);
    if (!fixed)
        pos = track->get_position(f0) * inv + track->get_position(f1) * frac;
    range = (short)(((track->sample_range(f1) * wf) >> 8) + ((track->sample_range(f0) * wi) >> 8));
    r = (ubyte)(((track->sample_r(f1) * wf) >> 8) + ((track->sample_r(f0) * wi) >> 8));
    g = (ubyte)(((track->sample_g(f1) * wf) >> 8) + ((track->sample_g(f0) * wi) >> 8));
    b = (ubyte)(((track->sample_b(f1) * wf) >> 8) + ((track->sample_b(f0) * wi) >> 8));
    on = track->on.get(f1);
    if (!fixed && on_bone) {
        vector3 bone_pos;
        matrix bone_orient;
        matrix43 m = owner->compute_bone_matrix(bone);
        m.get_rotation(&bone_orient);
        m.get_position(&bone_pos);
        matrix43 world(bone_pos, bone_orient);
        pos = world.transform(pos);
    }
}

// 0x520070  Confidence: exact (pointer loop to the end of the array).
void attached_lights_init_slots()
{
    int i;

    for (i = 0; i < MAX_ATTACHED_LIGHTS; i++)
        attached_light_slots[i].type = -1;
}

// 0x520090  Confidence: exact (the same body as attached_lights_init_slots; not folded).
void attached_lights_clear_slots()
{
    int i;

    for (i = 0; i < MAX_ATTACHED_LIGHTS; i++)
        attached_light_slots[i].type = -1;
}

// 0x5200b0  Confidence: exact (the slot search returns -1 straight from the loop's normal exit, as
// light_alloc). No caller found.
int attached_light_create_b(const vector3 *pos, float intensity, float angle_a, float angle_b,
                            float range_near, float range_far, float r, float g, float b, bool kind)
{
    int i;
    attached_light *a;

    for (i = 0; i < MAX_ATTACHED_LIGHTS; i++) {
        if (attached_light_slots[i].type == -1)
            break;
    }
    if (i >= MAX_ATTACHED_LIGHTS)
        return -1;
    a = &attached_light_slots[i];
    a->pos = *pos;
    a->has_range = true;
    a->range_near = range_near;
    a->range_far = range_far;
    a->angle_a = angle_a;
    a->angle_b = angle_b;
    a->type = kind ? 1 : 4;
    a->intensity = intensity;
    a->r = r;
    a->g = g;
    a->b = b;
    return i;
}

// 0x520160  Confidence: exact (as attached_light_create_b).
int attached_light_create(const vector3 *pos, float intensity, float range_near, float range_far,
                          float r, float g, float b)
{
    int i;
    attached_light *a;

    for (i = 0; i < MAX_ATTACHED_LIGHTS; i++) {
        if (attached_light_slots[i].type == -1)
            break;
    }
    if (i >= MAX_ATTACHED_LIGHTS)
        return -1;
    a = &attached_light_slots[i];
    a->pos = *pos;
    a->has_range = true;
    a->range_near = range_near;
    a->range_far = range_far;
    a->type = 0;
    a->intensity = intensity;
    a->r = r;
    a->g = g;
    a->b = b;
    return i;
}

// 0x5201f0  Confidence: exact (VC6 byte match, match14; was exact).
void attached_light_free(int index)
{
    if (attached_light_slots[index].type != -1)
        attached_light_slots[index].type = -1;
}

// 0x520210  Confidence: exact (the quaternion and the vector are built as temporaries and assigned
// through the out-of-line operators).
void attached_light::reset()
{
    type = 0;
    orient = quaternion(0.0f, 0.0f, 0.0f, 1.0f);
    pos = vector3(0.0f, 0.0f, 0.0f);
    r = 0.0f;
    g = 0.0f;
    b = 0.0f;
    intensity = 1.0f;
    angle_a = 0.0f;
    angle_b = 0.0f;
    _unk44 = 0.0f;
    has_range = false;
    range_near = 0.0f;
    range_far = 40.0f;
    _unk54 = false;
}

// ---- COMDATs at the end of the object (bodies in vsdk/geom/vfx.h and vsdk/math/fixed.h) -----------
// 0x520280 attached_light__ctor: attached_light() { reset(); } (quaternion, pos, world_pos first). Confidence: exact.
// 0x5202b0 vfx_track__relocate = vfx_channel::relocate. Confidence: exact.
// 0x5202d0 vfx_ptr__relocate = BitArray::relocate. Confidence: exact.
// 0x5202f0 vfx_light_desc__get_position = vfx_light_track::get_position. Confidence: exact.
// 0x520310 vfx_vector_track__sample = vfx_vec_channel::sample. Confidence: exact.
// 0x520360 fixed4_to_float. Confidence: exact.
// 0x520380 vfx_light_desc__sample_34 = vfx_light_track::sample_range. Confidence: exact.
// 0x520390 vfx_light_desc__sample_40 = vfx_light_track::sample_r. Confidence: exact.
// 0x5203a0 vfx_light_desc__sample_4c = vfx_light_track::sample_g. Confidence: exact.
// 0x5203b0 vfx_light_desc__sample_58 = vfx_light_track::sample_b. Confidence: exact.
// 0x5203c0 attached_light_create_from_vfx (inline in vfx.h). Confidence: likely.
// 0x5203f0 vfx_node__get_scale = vfx_light_desc::get_scale. Confidence: exact.
// 0x520400 fixed8_to_float. Confidence: exact.
