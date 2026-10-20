// unnamed_vif_0x5329f0.cpp - Sum.exe v1.40, code 0x5329f0-0x533dae. Written from the Sum.exe disassembly
// (tools/asm.mjs; the floats 0x57e990 = 1.0, 0x57eb84 = 0.0, 0x580e00 = 0.25, 0x581c2c = 9.8,
// 0x57f030 = 1/255, 0x57ef2c = 128 read from the image). Target: VC++ 6.0 (cl 12.00.8966), /O2 with the
// inline members called out of line (every vector3 / matrix constructor and operator is a call to its COMDAT
// copy, as /Ob0 compiles them; D5317). Confidence per function: exact (control flow, calls, constants and
// globals match), likely (one detail of statement order or expression form is a guess), unsure (the shape
// itself is a guess).
//
// The vif particles: the particle systems of the effect files' +0x2c records (vfx_desc21c). 100 instance
// records (vfx_inst21c) on a free ring; their particles come from two halves of a 3000-particle pool (static
// effects; compacted into the other half when full) or from level_mempool (level effects). Per frame each record
// spawns at its interval curve's rate around its origin (a point, a box, or along a spawn path), applies
// gravity and a force, ages and kills its particles; the draw fades size and alpha in and out.
// FILE NAME NOT KNOWN (D5317): no path string, no assert, no strings.tbl block; a placeholder. The unit is
// cut by its static initialiser (table 0x5889c4) after timer.cpp, and file_packfile.cpp follows (0x533db0).
// .data 0x5a99d8 (vif_particle_top) and the literals 0x5a99dc-0x5a9aab; .bss 0x2cbffe0-0x2d0bf93.
//
// Header: vsdk/geom/vfx.h (the types and functions), summoner/summoner.h (vif_records_reset, vif_reset_all).

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/os/frametime.h"
#include "vsdk/os/memory.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/unnamed_gr_3d_0x50d9b0.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/geom/vfx.h"
#include "vsdk/math/bezier.h"
#include "summoner/summoner.h"

// ---- .data --------------------------------------------------------------------------------------------
vif_particle *vif_particle_top = vif_particles[0];   // 0x5a99d8

// ---- .bss (0x2cbffe0-0x2d0bf93) ------------------------------------------------------------------------
vif_particle vif_particles[2][VIF_MAX_PARTICLES];    // 0x2cbffe0
int vif_data_top;                                    // 0x2d064e0
vfx_inst21c *vif_record_list;                        // 0x2d064e4
// 0x5329f0  Confidence: exact. init_vif_records: "jmp 0x532a00" (initialiser table 0x5889c4), then
// `eh vector constructor iterator`(vif_records, 0xe8, 100, vfx_inst21c's implicit constructor).
vfx_inst21c vif_records[VIF_NUM_RECORDS];            // 0x2d064e8
int vif_particle_half;                               // 0x2d0bf8c
int vif_particle_used;                               // 0x2d0bf90

// 0x532a20  Confidence: likely (the list pointer fixes as written; the two halves' base computed twice is
// the code's). Copies the live particles of every static record into the other half, packed from 0, and
// moves the particles' and the two sentinels' links with them (links to the sentinels stay).
void vif_compact_particles()
{
    vif_particle *old_base = vif_particles[vif_particle_half];
    vif_particle *new_base;
    vfx_inst21c *r;
    int i;

    vif_particle_half = vif_particle_half == 0;
    new_base = vif_particles[vif_particle_half];
    vif_particle_top = vif_particles[vif_particle_half];
    vif_particle_used = 0;
    for (r = vif_records; r < &vif_records[VIF_NUM_RECORDS]; r++) {
        int shift;
        int offset;

        if (!(r->flags & 1) || (r->flags & 0x20))
            continue;
        shift = r->first - vif_particle_used;
        offset = vif_particle_used * sizeof(vif_particle);
        memcpy(&new_base[vif_particle_used], &old_base[r->first], r->count * sizeof(vif_particle));
        for (i = vif_particle_used; i < vif_particle_used + r->count; i++) {
            vif_particle *p = &new_base[i];
            if (p->prev && p->prev != &r->live && p->prev != &r->free)
                p->prev = &new_base[(p->prev - old_base) - shift];
            if (p->next && p->next != &r->live && p->next != &r->free)
                p->next = &new_base[(p->next - old_base) - shift];
        }
        if (r->live.prev && r->live.prev != &r->live)
            r->live.prev = &new_base[(r->live.prev - old_base) - shift];
        if (r->live.next && r->live.next != &r->live)
            r->live.next = &new_base[(r->live.next - old_base) - shift];
        if (r->free.prev && r->free.prev != &r->free)
            r->free.prev = &new_base[(r->free.prev - old_base) - shift];
        if (r->free.next && r->free.next != &r->free)
            r->free.next = &new_base[(r->free.next - old_base) - shift];
        r->first = (ushort)vif_particle_used;
        r->particles = (vif_particle *)((char *)vif_particle_top + offset);
        vif_particle_used += r->count;
    }
}

// 0x532cb0  Confidence: exact (VC6 byte match, cl 8966, /Ox /Ob0 /Ow, D12600: the byte count tested in place,
// n read after the single-point test, the found segment in `seg` (-1 when none), the index converted into a
// float local, the line case as an else block with a named direction).
// The spawn origin at time t: the single point; the last point outside the keys; a straight line between
// two points; a spline through more (parameter = segment + fraction).
void vif_emitter_spawn_point(vector3 *out, const vif_spawn_path *path, float t)
{
    int i;

    if (path->count == 1) {
        *out = path->points[0];
        return;
    }
    int n = path->count;
    int seg = -1;
    for (i = 0; i < n - 1; i++)
        if (t >= path->times[i] && t <= path->times[i + 1]) {
            seg = i;
            break;
        }
    if (seg < 0) {
        *out = path->points[n - 1];
        return;
    }
    if (path->count > 2) {
        spline s(path->count, path->points);
        float fi = (float)i;
        s.eval(out, (t - path->times[i]) / (path->times[i + 1] - path->times[i]) + fi);
    } else {
        float d = path->times[i + 1] - path->times[i];
        vector3 dir = path->points[i + 1] - path->points[i];
        *out = path->points[i] + dir * ((t - path->times[i]) / d);
    }
}

// 0x532e30  Confidence: exact. The smallest key value that is not 0 (0 when all are); the index it keeps is
// not returned. No caller.
float vif_min_nonzero_float(const vif_curve *c)
{
    float best = 0.0f;
    int best_index = -1;
    int n = c->count;
    int i;

    for (i = 0; i < n; i++) {
        if (c->values[i] != 0.0f && (best_index == -1 || c->values[i] < best)) {
            best = c->values[i];
            best_index = i;
        }
    }
    return best;
}

// 0x532e80  Confidence: exact (VC6 byte match, cl 8966, /Ox /Ow (D8000), match21). LoadSettings: every record unused.
void vif_records_reset()
{
    for (int i = 0; i < VIF_NUM_RECORDS; i++)
        vif_records[i].flags = 0;
}

// 0x532ea0  Confidence: exact (VC6 byte match, cl 8966, /Ox /Ow (D8000), match21; was likely; the ring append as written; the list head is kept in a register).
// level_load: all records back on the free ring in order, the pool empty, every particle dead.
void vif_reset_all()
{
    vfx_inst21c *r;
    int i;

    vif_data_top = 0;
    vif_record_list = NULL;
    for (i = 0; i < VIF_NUM_RECORDS; i++) {
        r = &vif_records[i];
        r->flags = 0;
        plist_append(vif_record_list, r);
    }
    vif_particle_used = 0;
    vif_particle_half = 0;
    vif_particle_top = vif_particles[0];
    for (i = 0; i < VIF_MAX_PARTICLES; i++) {
        vif_particles[0][i].flags = 0;
        vif_particles[1][i].flags = 0;
    }
}

// 0x532f40  Confidence: unsure (the whole: the locals' construction order, the spawn count arithmetic and the
// per-component loops are from the code; which parts were helpers or macros is not visible). (record, desc,
// spawn, effect position, effect orientation, effect time): nothing unless the record is in use and the desc
// shown (0x180) or VIF_SHOWN_ALONE. The origin (the desc position or the spawn path point), carried by the
// bone matrix when the record is bound to a tag, then into the world by the effect transform. With spawn set
// the interval curve gives the spawn count of the frame; each new particle takes a random lifetime and a
// position and velocity in a box (VIF_SPAWN_BOX) or a sphere. Every live particle ages (dying ones go back to
// the free list), falls (VIF_GRAVITY), is pushed (VIF_FORCE) and moves. The time is kept for the next frame.
// The last argument is the effect time's raw dword (D4704).
void vif_emitter_update(vfx_inst21c *em, vfx_desc21c *rec, int flag, const vector3 *pos,
                        const matrix *orient, int unk0c)
{
    float elapsed = *(float *)&unk0c;
    float force;
    float interval;
    float remaining;
    int spawn;
    vif_particle *p;
    vif_particle *next;
    int i;

    if (!(em->flags & 1))
        return;
    vector3 dir;
    vector3 world;
    vector3 attract;
    vector3 to_attract;
    vector3 origin;
    matrix m;
    force = 0.0f;
    if (!(rec->vis_flags & 0x180) && !(rec->vis_flags & VIF_SHOWN_ALONE))
        return;
    if (pos)
        em->emit_pos = *pos;
    if (orient)
        em->emit_orient = *orient;
    if (rec->vis_flags & VIF_SPAWN_PATH)
        vif_emitter_spawn_point(&origin, &rec->path, elapsed);
    else
        origin = rec->pos;
    if (!(em->flags & 2) && (em->flags & 4)) {
        vector3 bone_pos;
        matrix bone_orient;
        matrix43 bone = em->ci->compute_bone_matrix(em->tag);
        bone.get_rotation(&bone_orient);
        bone.get_position(&bone_pos);
        matrix43 frame(bone_pos, bone_orient);
        em->pos = frame.transform(origin);
        em->orient = bone_orient * rec->orient;
    } else {
        em->pos = origin;
        em->orient = rec->orient;
    }
    m = em->orient;
    world = em->pos;
    world.unrotate(em->emit_orient);
    world += em->emit_pos;
    if (rec->vis_flags & VIF_FORCE) {
        if (rec->vis_flags & VIF_FORCE_ATTRACT) {
            attract.x = rec->attract_offset.x + em->emit_pos.x;
            attract.y = rec->attract_offset.y + em->emit_pos.y;
            attract.z = rec->attract_offset.z + em->emit_pos.z;
        }
        force = vif_curve__evaluate(&rec->force_curve, em->time, false) * reported_frametime;
    }
    interval = vif_curve__evaluate(&rec->interval_curve, em->time, true);
    if (em->interval <= 0.0f && interval > 0.0f)
        em->spawn_timer = interval;
    remaining = float_min(em->spawn_timer, interval) - reported_frametime;
    em->interval = interval;
    em->spawn_timer = remaining;

    if (flag && remaining <= 0.0f && interval > 0.0f) {
        float late = (float)fabs(remaining);
        int n = (int)(late / interval);
        spawn = 1;
        if (late > interval)
            spawn = n;
        if (late > interval)
            late -= n * interval;
        em->spawn_timer = interval - late;
        for (i = 0; i < spawn; i++) {
            float speed;

            p = em->free.next;
            if (p == &em->free)
                break;
            p->prev->next = p->next;
            p->next->prev = p->prev;
            p->age = 0.0f;
            p->next = em->live.next;
            em->live.next->prev = p;
            p->prev = &em->live;
            em->live.next = p;
            p->flags |= VIF_PARTICLE_ALIVE;
            p->lifetime = frand_range(-rec->time_b, rec->time_b) + rec->time_a;
            p->seed = rand();
            em->num_live++;
            if (rec->vis_flags & VIF_SPAWN_BOX) {
                float rx = frand_range(-rec->spread_x, rec->spread_x);
                float ry = frand_range(-rec->spread_y, rec->spread_y);
                p->x = m.fvec.x * ry + m.rvec.x * rx + world.x;
                p->y = m.fvec.y * ry + m.rvec.y * rx + world.y;
                p->z = m.fvec.z * ry + m.rvec.z * rx + world.z;
                speed = rec->speed_spread;
                if (speed == 0.0f) {
                    p->vx = m.uvec.x * rec->speed;
                    p->vy = m.uvec.y * rec->speed;
                    p->vz = m.uvec.z * rec->speed;
                } else {
                    speed = frand_range(-speed, speed) + rec->speed;
                    p->vx = speed * m.uvec.x;
                    p->vy = speed * m.uvec.y;
                    p->vz = speed * m.uvec.z;
                }
            } else {
                dir.random_unit_vector();
                p->x = dir.x * rec->spread_x * 2.0f + world.x;
                p->y = dir.y * rec->spread_x * 2.0f + world.y;
                p->z = dir.z * rec->spread_x * 2.0f + world.z;
                speed = rec->speed_spread;
                if (speed == 0.0f) {
                    p->vx = dir.x * rec->speed;
                    p->vy = dir.y * rec->speed;
                    p->vz = dir.z * rec->speed;
                } else {
                    speed = frand_range(-speed, speed) + rec->speed;
                    p->vx = dir.x * speed;
                    p->vy = dir.y * speed;
                    p->vz = dir.z * speed;
                }
            }
        }
    }

    for (p = em->live.next, next = p->next; p != &em->live; p = next, next = next->next) {
        if (p->age >= p->lifetime) {
            p->prev->next = p->next;
            p->next->prev = p->prev;
            p->next = em->free.next;
            em->num_live--;
            em->free.next->prev = p;
            p->prev = &em->free;
            em->free.next = p;
            p->flags &= ~VIF_PARTICLE_ALIVE;
            continue;
        }
        if (rec->vis_flags & VIF_GRAVITY)
            p->vy -= reported_frametime * 9.8f;
        if (rec->vis_flags & VIF_FORCE) {
            if (rec->vis_flags & VIF_FORCE_ATTRACT) {
                to_attract.x = p->x - attract.x;
                to_attract.y = p->y - attract.y;
                to_attract.z = p->z - attract.z;
                to_attract.normalize();
                p->vx += to_attract.x * force;
                p->vy += to_attract.y * force;
                p->vz += to_attract.z * force;
            } else {
                p->vx += force * rec->force_dir.x;
                p->vy += force * rec->force_dir.y;
                p->vz += force * rec->force_dir.z;
            }
        }
        p->x += reported_frametime * p->vx;
        p->y += reported_frametime * p->vy;
        p->z += reported_frametime * p->vz;
        p->age += reported_frametime;
        p->age = float_min(p->lifetime, p->age);
    }
    em->time = elapsed;
}

// 0x533620  Confidence: likely (D12602: the fog point as operator+'s result, args register rotation left; the two ramps as nested ifs; the second ramp's middle value 1.0 is loaded
// after dropping the fraction). Each live particle as a bitmap of size * 2 scaled by the size ramp, white with
// the alpha ramp * alpha * the point fog of the record (* 128), additive or alpha blended.
void vif_draw_particles(vfx_inst21c *em, vfx_desc21c *rec)
{
    float size;
    float fog;
    float f;
    float s;
    float a;
    vif_particle *p;

    if (!(em->flags & 1))
        return;
    size = rec->size * 2.0f;
    fog = gr_get_point_fog_value(0xff, &static_cast<const vector3 &>(em->emit_pos + em->pos.unrotated(em->emit_orient))) * (1.0f / 255.0f);
    vector3 pt;
    for (p = em->live.next; p != &em->live; p = p->next) {
        if (!(p->flags & VIF_PARTICLE_ALIVE))
            continue;
        f = p->age / p->lifetime;
        if (f < rec->size_in)
            s = f / rec->size_in;
        else if (f > rec->size_out)
            s = (1.0f - f) / (1.0f - rec->size_out);
        else
            s = 1.0f;
        s *= size;
        if (f < rec->alpha_in)
            a = f / rec->alpha_in;
        else if (f > rec->alpha_out)
            a = (1.0f - f) / (1.0f - rec->alpha_out);
        else
            a = 1.0f;
        pt.x = p->x;
        pt.y = p->y;
        pt.z = p->z;
        gr_set_color(0xff, 0xff, 0xff, (int)(a * rec->alpha * fog * 128.0f) & 0xff);
        gr_set_texture(rec->bitmap, -1);
        gr_bitmap_3d(&pt, 0.0f, s, (rec->vis_flags & VIF_ADDITIVE) ? gr_mode_particle_additive : gr_mode_particle_alpha);
    }
}

// 0x5337a0  Confidence: likely (D12602: the static branch through used / total; the free-ring unlink as written). (desc, level effect): the next free record
// with desc->count particles, for a level effect from operator new in level_mempool (emptied at the next level load) or from
// the pool (compacted when full; NULL with a message when still short), all on the free list; the record
// leaves the free ring. NULL when no record is free.
vfx_inst21c *vif_instantiate_level(vfx_desc21c *desc, bool level)
{
    vfx_inst21c *r = vif_record_list;
    int i;

    if (!r)
        return NULL;
    r->flags = 0;
    if (level == true) {
        printf("VIF INSTANTIATE : LEVEL\n");
        r->flags |= 0x20;
        r->first = -1;
        r->particles = (vif_particle *)operator new(desc->count * sizeof(vif_particle));
        if (!r->particles)
            return NULL;
        vif_data_top += desc->count * sizeof(vif_particle);
    } else {
        printf("VIF INSTANTIATE : STATIC\n");
        int used = vif_particle_used;
        if (desc->count + used > VIF_MAX_PARTICLES) {
            printf("COMPACTING VIF PARTICLES\n");
            vif_compact_particles();
            printf("DONE COMPACTING\n");
            used = vif_particle_used;
        }
        int total = desc->count + used;
        if (total > VIF_MAX_PARTICLES) {
            printf("Skipping vif particle instantiation due to lack of static particles (needed : %d   available : %d)!\n",
                   desc->count, VIF_MAX_PARTICLES - used);
            return NULL;
        }
        r->first = used;
        vif_particle_used = total;
        r->particles = &vif_particle_top[used];
    }
    r->flags |= 1;
    r->live.next = &r->live;
    r->live.prev = &r->live;
    r->free.next = &r->free;
    r->free.prev = &r->free;
    r->count = desc->count;
    r->spawn_timer = 0.0f;
    r->emit_pos = vector3_zero;
    r->emit_orient.make_identity();
    r->num_live = 0;
    r->interval = 0.0f;
    r->time = 0.0f;
    for (i = 0; i < r->count; i++) {
        vif_particle *p = &r->particles[i];
        vif_particle *last = r->free.prev;
        r->free.prev = p;
        p->prev = last;
        p->next = &r->free;
        last->next = p;
    }
    if (vif_record_list == r) {
        if (r->next == r) {
            r->next = NULL;
            r->prev = NULL;
            vif_record_list = NULL;
            return r;
        }
        vif_record_list = r->next;
    }
    r->prev->next = r->next;
    r->next->prev = r->prev;
    r->next = NULL;
    r->prev = NULL;
    return r;
}

// 0x5339b0  Confidence: exact (VC6 byte match, cl 8966, /Ox /Ow (D8000), match21; was likely; the ring append as in vif_reset_all). Every particle of the record dead, the
// lists empty, the record unused and (when it is one of the 100) back at the end of the free ring. A level
// record only takes its bytes off vif_data_top; its heap particles are not freed.
void vif_record_release(vfx_inst21c *em, vfx_desc21c *desc)
{
    vif_particle *p;
    int index;

    if (em->flags & 0x20)
        vif_data_top -= desc->count * sizeof(vif_particle);
    for (p = em->live.next; p != &em->live; p = p->next)
        p->flags = 0;
    for (p = em->free.next; p != &em->free; p = p->next)
        p->flags = 0;
    em->live.next = &em->live;
    em->live.prev = &em->live;
    em->free.next = &em->free;
    em->free.prev = &em->free;
    em->particles = NULL;
    em->flags = 0;
    index = em - vif_records;
    if (index < 0 || index >= VIF_NUM_RECORDS)
        return;
    plist_append(vif_record_list, em);
}

// 0x533aa0  Confidence: likely (D12602: one argument-slot choice left; the box radius expression). (desc, out centre, out radius): the spawn area
// around the desc position; with a spawn path the centre of its points and the radius grown by the farthest.
void vif_record_get_bounds(vfx_desc21c *rec, vector3 *centre, float *radius)
{
    float r = rec->spread_x * 2.0f;

    if (rec->vis_flags & VIF_SPAWN_BOX) {
        float d = rec->spread_y * 2.0f;
        r = (float)sqrt(d * d * 0.25f + r * r * 0.25f);
    }
    *radius = r;
    *centre = rec->pos;
    if (rec->vis_flags & VIF_SPAWN_PATH) {
        vector3 c;
        float farthest;
        unsigned int i;

        c.zero();
        for (i = 0; i < rec->path.count; i++)
            c += rec->path.points[i];
        c *= 1.0f / rec->path.count;
        farthest = 0.0f;
        for (i = 0; i < rec->path.count; i++) {
            float l = (rec->path.points[i] - c).len();
            farthest = float_max(farthest, l);
        }
        *centre = c;
        *radius += farthest;
    }
}

// 0x533be0  Confidence: exact (thiscall, ret 4).
void vfx_desc21c::relocate(char *header_base)
{
    reloc_ptr(&name, header_base);
    reloc_ptr(&parent, header_base);
    reloc_ptr(&bitmap_name, header_base);
    bitmap = 0;
}

// 0x533c20  Confidence: exact (thiscall, ret 4). Retried with -1 when the first load returns 0.
void vfx_desc21c::load_bitmap(int flag)
{
    bitmap = bm_load(bitmap_name, -1, flag);
    if (!bitmap)
        bitmap = bm_load(bitmap_name, -1, -1);
}

// 0x533c60  Confidence: exact (bare RET, no reference; the signature is unknown).
void vif_stub_ret_533c60()
{
}

// 0x533c70  Confidence: exact. vif_record__ctor: vfx_inst21c's implicit constructor (the members' default
// constructors, called out of line), the function init_vif_records passes to the vector constructor
// iterator. Generated by the compiler from the declaration of vif_records above; no source of its own.

// 0x533ca0  Confidence: exact (VC6 byte match, cl 8966, /Ox /Ob0 /Ow, D12600: the found segment in `seg`, -1
// when none, so the loop's normal exit goes straight to the last key; that key indexed by c->count again).
// (curve, t, reciprocal): see vfx.h. With
// reciprocal set the fraction is (t - 1/v0) / (t1 - t0) instead of (t - t0) / (t1 - t0): the code subtracts
// the reciprocal it just computed (a game bug, see NOTES); written as the code computes it.
float vif_curve__evaluate(const vif_curve *c, float t, bool reciprocal)
{
    int i;

    if (c->count == 1)
        return c->values[0];
    int n = c->count;
    int seg = -1;
    for (i = 0; i < n - 1; i++)
        if (t >= c->times[i] && t <= c->times[i + 1]) {
            seg = i;
            break;
        }
    if (seg < 0)
        return c->values[c->count - 1];
    if (c->values[i] == 0.0f || c->values[i + 1] == 0.0f)
        return 0.0f;
    float dt = c->times[i + 1] - c->times[i];
    if (reciprocal) {
        float r0 = 1.0f / c->values[i];
        float u = (t - r0) / dt;
        return 1.0f / ((1.0f / c->values[i + 1] - r0) * u + r0);
    }
    return (c->values[i + 1] - c->values[i]) * ((t - c->times[i]) / dt) + c->values[i];
}

// 0x533d90  Confidence: exact. NULL stays, -1 becomes the base, anything else is an offset from it.
void reloc_ptr(void *p, char *base)
{
    char **pp = (char **)p;

    if (*pp == NULL)
        return;
    if (*pp == (char *)-1)
        *pp = base;
    else
        *pp += (int)base;
}
