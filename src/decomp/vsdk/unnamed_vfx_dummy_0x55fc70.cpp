// unnamed_vfx_dummy_0x55fc70.cpp - Sum.exe v1.40, code 0x55fc70-0x55fefa: the dummies of an effect
// instance (vsdk\geom in sum.symbols: vfx_dummy_state__reset / __update, vfx_dummy__relocate). The file
// name is a placeholder: no path string, no strings.tbl block, no data.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp66 NOTES.md): after the polygon split unit (0x55fc6f), before the emitter unit's
// initialiser (table entry 650, 0x55ff00). No .data / .bss of its own; it may be the head of the emitter
// object (no COMDAT block between them), kept apart because nothing proves either way.

#include <math.h>
#include "vsdk/geom/effect_mem.h"
#include "vsdk/geom/vfx.h"
#include "vsdk/ca/character_instance.h"

// 0x55fc70  Confidence: exact (one AND 0x1fffffff for the three bit clears).
void fx_rec2c::reset()
{
    bound = 0;
    placed = 0;
    hidden = 0;
    v08.zero();
    q14.make_identity();
}

// 0x55fc90  Confidence: likely (the two unused-looking locals constructed before the bone test; their
// order). Game bug (decomp66 NOTES.md): on a bone the rotation is never turned into the bone's frame:
// the product is built in m, then q14.to_matrix(&m) overwrites it, q14 stays the effect-frame rotation
// (the emitter unit makes the quaternion from the product, 0x56164f). The descriptor is read as a vfx_node
// (get_orient 0x55ba30) and its header as a vfx_node_header (D4606).
void fx_rec2c::update(vfx_dummy_desc *d, float t)
{
    int frame;
    int next;
    float frac;

    frame = (int)floor(t);
    next = frame + 1;
    if (!(next < (d->flags & 0x7fff)))
        next = frame;
    frac = t - (int)floor(t);
    if (!placed) {
        v08 = d->get_position(frame) * (1.0f - frac) + d->get_position(next) * frac;
        quaternion qa;
        qa.unpack_short4(((vfx_node *)d)->get_orient(frame));
        quaternion qb;
        qb.unpack_short4(((vfx_node *)d)->get_orient(next));
        quaternion_slerp(&q14, &qa, &qb, frac);
    }
    matrix43 unused;                          // constructed, never read
    matrix43 m;
    if (!placed && bound) {
        vector3 bpos;
        matrix brot;
        matrix43 bm = ci->compute_bone_matrix(tag);
        bm.get_rotation(&brot);
        bm.get_position(&bpos);
        matrix43 world(bpos, brot);
        v08 = world.transform(v08);
        m.set(q14, v08);
        m.set_product_4x4(world, m);
        q14.to_matrix((matrix *)&m);
    }
}

// 0x55fed0  Confidence: exact.
void vfx_dummy_desc::relocate(char *header_base, char *base)
{
    ((vfx_node_header *)this)->relocate(header_base, base);
    pos.relocate(base);
    orient.relocate(base);
}
