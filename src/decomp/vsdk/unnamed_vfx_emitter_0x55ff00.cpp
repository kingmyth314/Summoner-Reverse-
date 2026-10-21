// unnamed_vfx_emitter_0x55ff00.cpp - Sum.exe v1.40, code 0x55ff00-0x562a2b: the particle emitters of an
// effect instance (vsdk\geom in sum.symbols: vfx_emitter_instance__*, vfx_emitter__relocate,
// vfx_particle_*). The file name is a placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp66 NOTES.md): initialiser table entry 650 (0x55ff00) to the last COMDAT (0x562a2b).
// .bss 0x2f91a5c (gr_mode_vfx_flat_alpha), no .data. Built without inline expansion: the accessors of
// vfx_emitter_desc, fx_recb0 and fx_rec28 (vfx.h, effect_mem.h), fixed12_to_float / float_to_fixed12
// (fixed.h), vector3::operator/ and the four particle helpers below are COMDATs at 0x5622b0-0x562a2b.
// Times: t / last_t are effect frames (15 a second), dt and the particle ages seconds.

#include <math.h>
#include "vsdk/geom/effect_mem.h"
#include "vsdk/geom/vfx.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/unnamed_gr_3d_0x50d9b0.h"
#include "vsdk/gr/unnamed_lights_0x51e070.h"
#include "vsdk/gr/directx/gr_direct3d.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/math/ix.h"
#include "engine/rendering/levelrender.h"      // perlin_noise3 (0x5470e0)

// ---- .bss -------------------------------------------------------------------------------------------
// 0x55ff00 init_gr_mode_2f91a5c (the `jmp` thunk to 0x55ff10, which runs the inline gr::Mode constructor
// 0x508160). Confidence: exact.
gr::Mode gr_mode_vfx_flat_alpha(0, 0, 0, 3, 1, 0);   // 0x2f91a5c

// The forces at one frame, then steps of at most 1/30 s until `total` seconds are covered. Written as
// macros: the code repeats them inline at each use (the object has no inline expansion, so they were not
// functions). Game bug (NOTES.md): the step counter advances by 0.5 s, not by the step, so a frame of
// more than 1/30 s only moves the particles 1/30 s per half second.
#define EMITTER_FORCE_TRANSFORMS(frame)                                                            \
    for (i = 0; i < (int)desc->num_links; i++) {                                                   \
        if (links[i] != NULL && desc->links[i] != NULL) {                                          \
            links[i]->get_transform((vfx_force_desc *)desc->links[i], &forces[i], frame);           \
            forces[i].pos.unrotate(m28);                                                           \
            forces[i].pos += v1c;                                                                  \
            if ((char)((vfx_force_desc *)desc->links[i])->header.frame_word != 1)                  \
                forces[i].dir.unrotate(m28);                                                       \
        }                                                                                          \
    }
#define EMITTER_SUBSTEPS(total)                                                                    \
    for (elapsed = 0.0f; elapsed < (total); elapsed += 0.5f) {                                     \
        step = min(1.0f / 30.0f, (total) - elapsed);                                               \
        for (p = particles; p != NULL; ) {                                                         \
            apply_forces(desc, p, forces, desc->num_links, step);                                  \
            p = p->next;                                                                           \
            if (p == particles)                                                                    \
                break;                                                                             \
        }                                                                                          \
    }
#define EMITTER_DRIFT(total)                                                                       \
    for (p = particles; p != NULL; ) {                                                             \
        apply_forces(desc, p, NULL, 0, total);                                                     \
        p = p->next;                                                                               \
        if (p == particles)                                                                        \
            break;                                                                                 \
    }

// ---- inline helpers (COMDATs at the end of the object) -------------------------------------------

// 0x562550 vfx_particle_get_bitmaps  Confidence: exact. The two layers' bitmaps at the particle's age
// (life_anim: over its lifetime); returns the material.
inline vfx_material *fx_recb0::get_bitmaps(vfx_emitter_desc *desc, fx_rec28 *p, int *bitmaps)
{
    vfx_material *mat = &desc->base->materials[desc->material];

    if (desc->life_anim) {
        float f = p->get_age() / p->get_lifetime();
        bitmaps[0] = mat->layers[0].get_frame_bitmap_frac(f);
        bitmaps[1] = mat->layers[1].get_frame_bitmap_frac(f);
    } else {
        float f = p->get_age() * 15.0f;
        bitmaps[0] = mat->layers[0].get_frame_bitmap(f);
        bitmaps[1] = mat->layers[1].get_frame_bitmap(f);
    }
    return mat;
}

// 0x562600 vfx_particle_get_color  Confidence: exact (VC6 byte match, cl 8966, /Ox /Ow (D8000), match21; was likely; v kept on the FPU). White, or the material's
// curve 44 at t raised to the ambient light per channel.
inline void fx_recb0::get_color(vfx_emitter_desc *desc, vfx_material *mat, float t, ubyte *r, ubyte *g, ubyte *b)
{
    float v;

    if (desc->white) {
        *r = 255;
        *g = 255;
        *b = 255;
        return;
    }
    v = mat->eval_curve44(t);
    *r = (ubyte)(int)(max(v, light_ambient_r) * 255.0f);
    *g = (ubyte)(int)(max(v, light_ambient_g) * 255.0f);
    *b = (ubyte)(int)(max(v, light_ambient_b) * 255.0f);
}

// 0x5626b0 vfx_particle_get_fade  Confidence: likely. (desc, age, lifetime, out size, out size fade,
// out alpha): the size (4 * the size value) faded in and out by the bytes +0x2c / +0x2d, the alpha (the
// instance's alpha byte) by the 8.8 fractions +0x2e / +0x30.
inline void fx_recb0::get_fade(vfx_emitter_desc *desc, float age, float life, float *size, float *fade, float *alpha)
{
    age = age / life;
    *size = get_fixed12_90() * 4.0f;
    if (desc->get_fade_in() > age)
        *fade = age / desc->get_fade_in();
    else if (desc->get_fade_out() < age)
        *fade = (1.0f - age) / (1.0f - desc->get_fade_out());
    else
        *fade = 1.0f;
    *size = *size * *fade;
    *alpha = (float)this->alpha;
    if (desc->get_fixed8_2e() > age)
        *alpha *= age / desc->get_fixed8_2e();
    else if (desc->get_fixed8_30() < age)
        *alpha *= (1.0f - age) / (1.0f - desc->get_fixed8_30());
}

// 0x562840 vfx_particle_get_billboard_axes  Confidence: exact. A flat particle's triangle: two corners
// across the view at +-size / 4, the tip behind along the velocity. The first tip (scaled by fade: the
// binary pushes [esp+0xc0], the last argument, 0x562947) is overwritten at once by one without it, so
// fade has no effect (D13601).
inline void fx_recb0::get_billboard_axes(vfx_emitter_desc *desc, fx_rec28 *p, vector3 *a, vector3 *b, vector3 *c,
                                         float size, float fade)
{
    vector3 eye;
    matrix eye_m;

    gr_get_eye(&eye, &eye_m);
    vector3 vel = p->get_vel();
    vector3 pos = p->get_pos();
    vector3 side = eye_m.fvec.cross(vel);
    side.normalize_safe();
    *a = pos + side * size * 0.25f;
    *b = pos - side * size * 0.25f;
    *c = pos - vel / 15.0f * 1.6f * fade * desc->get_fixed8_32();
    *c = pos - vel / 15.0f * 1.6f * desc->get_fixed8_32();
}

// ---- the emitter instance ---------------------------------------------------------------------------

// 0x55ff30  Confidence: likely (prev is an integer copy of last_t, ceil reads last_t, a rate local in
// three sums: D13604; the binary stores prev after the fstp of the ceil argument, ours before). Adds 320 * rate per frame passed since
// last_t to spawn_accum (fractions of frames at the ends), then starts that many free particles, spread
// over the step (frac) with their ages.
void fx_recb0::spawn(vfx_emitter_desc *desc, float t, float dt, const vector3 &vel)
{
    float prev;
    int first;
    int last;
    int i;
    int n;
    int count;
    float frac;
    fx_rec28 *p;
    float rate;

    if (desc->end_frame > t && desc->start_frame < t) {
        prev = last_t;
        first = (int)ceil(last_t);
        last = (int)floor(t);
        if (prev < t && (first > t || last < prev)) {
            rate = desc->get_rate(max((int)floor(prev), 0));
            spawn_accum += (t - prev) * rate * 320.0f;
        } else {
            if (prev < first) {
                rate = desc->get_rate(max((int)floor(prev), 0));
                spawn_accum += (first - prev) * rate * 320.0f;
            }
            for (i = first % desc->end_frame; i < last % desc->end_frame; i = (i + 1) % desc->end_frame)
                spawn_accum += desc->get_rate(max(i, 0)) * 320.0f;
            if (last < t) {
                rate = desc->get_rate(max(last, 0));
                spawn_accum += (t - last) * rate * 320.0f;
            }
        }
        n = (int)floor(spawn_accum);
        count = n;
        if (n > 0) {
            spawn_accum -= n;
            p = particles;
            while (p != NULL) {
                if (p->get_age() == -1.0f) {
                    frac = (float)n / count;
                    init_particle(desc, p, frac, (1.0f - frac) * dt, vel);
                    n--;
                    if (n <= 0)
                        break;
                }
                p = p->next;
                if (p == particles)
                    break;
            }
        }
    }
}

// 0x5601f0  Confidence: likely (the two shape blocks share their tail in the binary). Shape 1: a sphere
// shell (radius width, speed outward); shape 0: a box (width x depth) in the emitter's frame, the
// velocity down by speed with a spread. The start point is interpolated over the step, the particle
// advanced by its age.
void fx_recb0::init_particle(vfx_emitter_desc *desc, fx_rec28 *p, float frac, float age, const vector3 &vel)
{
    vector3 unused;                           // constructed, never read
    vector3 pos;
    matrix rm;
    float spread;

    switch (desc->shape) {
    case 0: {
        vector3 v;
        if (get_fixed12_94() == 0.0f)
            v.set(0.0f, -get_fixed12_92(), 0.0f);
        else
            v.set(frand_range(-get_fixed12_94(), get_fixed12_94()),
                  frand_range(-get_fixed12_94(), get_fixed12_94()) - get_fixed12_92(),
                  frand_range(-get_fixed12_94(), get_fixed12_94()));
        q7c.to_matrix_b(&rm);
        v.unrotate(rm);
        v.unrotate(m28);
        if (desc->add_velocity)
            v += vel;
        p->set_vel(v);
        pos.x = frand_range(get_fixed12_8c() * -0.5f, get_fixed12_8c() * 0.5f);
        pos.z = frand_range(get_fixed12_8e() * -0.5f, get_fixed12_8e() * 0.5f);
        pos.y = 0.0f;
        pos.unrotate(rm);
        pos.unrotate(m28);
        pos += (v4c - v64) + (v58 - v4c) * frac;
        p->life.set(age);
        p->set_pos(pos + v * age);
        spread = desc->frames * desc->rate;
        p->set_lifetime((desc->frames + frand_range(-spread, spread)) * (1.0f / 4800.0f));
        break;
    }
    case 1: {
        pos.random_unit_vector();
        vector3 v = pos * (frand_range(-get_fixed12_94(), get_fixed12_94()) + get_fixed12_92());
        if (desc->add_velocity)
            v += vel;
        p->set_vel(v);
        pos *= get_fixed12_8c();
        pos += (v4c - v64) + (v58 - v4c) * frac;
        p->life.set(age);
        p->set_pos(pos + v * age);
        spread = desc->frames * desc->rate;
        p->set_lifetime((desc->frames + frand_range(-spread, spread)) * (1.0f / 4800.0f));
        break;
    }
    }
    if (desc->random_rot)
        p->set_size(frand_range(0.0f, 6.2831855f));
    else
        p->set_size(0.0f);
    num_live++;
}

// 0x5605f0 vfx_particle_apply_forces  Confidence: likely (the plane force's point is the address of the
// sum, through a const reference: D13602). Gravity, then each force: a point force
// pushes along the particle's offset from it, a plane force along its normal; both scaled by the
// strength, damped by exp(-falloff * distance), plus Perlin turbulence; then a midpoint step.
void fx_recb0::apply_forces(vfx_emitter_desc *desc, fx_rec28 *p, vfx_force_params *forces, int count, float dt)
{
    vector3 pos = p->get_pos();
    float dist;
    int i;

    if (p->get_age() >= 0.0f) {
        vector3 acc(0.0f, 0.0f, 0.0f);
        if (desc->gravity)
            acc.y -= 9.8f;
        for (i = 0; i < count; i++) {
            if (links[i] != NULL && desc->links[i] != NULL) {
                vector3 d;
                vector3 f;
                if ((char)((vfx_force_desc *)desc->links[i])->header.frame_word == 1) {
                    d = pos + v64 - forces[i].pos;
                    dist = d.len();
                    f = d;
                    f.normalize_safe();
                    f *= forces[i].strength;
                } else {
                    plane pl(&forces[i].dir, &forces[i].pos);
                    dist = pl.distance_to_point(&static_cast<const vector3 &>(pos + v64));
                    d = forces[i].dir * dist;
                    f = forces[i].dir * forces[i].strength;
                }
                if (forces[i].v1c != 0.0f)
                    f *= (float)exp(-(forces[i].v1c * dist));
                if (forces[i].v20 != 0.0f) {
                    vector3 n;
                    vector3 q;
                    vector3 base = d / 0.0254f;
                    float s = forces[i].v24 * time * 48.0f;
                    n = base;
                    n.x = s;
                    q.x = perlin_noise3(n * forces[i].v28);
                    n = base;
                    n.y = s;
                    q.y = perlin_noise3(n * forces[i].v28);
                    n = base;
                    n.z = s;
                    q.z = perlin_noise3(n * forces[i].v28);
                    f += forces[i].v20 * q;
                }
                acc += f * 0.0001f * 0.0625f;
            }
        }
        vector3 vel = p->get_vel();
        vector3 nvel = vel + acc * dt;
        vector3 npos = pos + 0.5f * (vel + nvel) * dt;
        p->set_pos(npos);
        p->set_vel(nvel);
    }
}

// 0x560a00  Confidence: likely (the macros; the loop counter is `first` itself; D16703: ceil reads last_t,
// as in spawn, which gives the binary's frame; left: spawn's prev store order and one load in the second
// force block). Without forces one step
// of dt. Otherwise the part of the first frame, the whole frames (modulo the loop length) and the part
// of the last frame each get the forces at their frame and a 1/15 s budget of steps; once the effect has
// wrapped and the emitter is stopped, frames past the end only drift.
void fx_recb0::move_particles(vfx_emitter_desc *desc, float t, float dt)
{
    float prev;
    int first;
    int last;
    bool wrapped;
    bool stop;
    float total;
    float elapsed;
    float step;
    int i;
    fx_rec28 *p;

    if (desc->num_links > 0) {
        prev = last_t;
        first = (int)ceil(last_t);
        last = (int)floor(t);
        vfx_force_params forces[3];
        wrapped = t < prev;
        stop = stopped;
        if (prev < t && (first > t || last < prev)) {
            EMITTER_FORCE_TRANSFORMS((int)floor(prev));
            total = (t - prev) * (1.0f / 15.0f);
            EMITTER_SUBSTEPS(total);
            return;
        }
        if (prev < first) {
            EMITTER_FORCE_TRANSFORMS((int)floor(prev));
            total = (first - prev) * (1.0f / 15.0f);
            EMITTER_SUBSTEPS(total);
        }
        for (; first % desc->end_frame < last % desc->end_frame; first++) {
            if (wrapped && stop && first >= desc->end_frame) {
                total = (last - first) * (1.0f / 15.0f);
                EMITTER_DRIFT(total);
                break;
            }
            EMITTER_FORCE_TRANSFORMS(first % desc->end_frame);
            EMITTER_SUBSTEPS(1.0f / 15.0f);
        }
        if (last < t) {
            if (wrapped && stop && last >= desc->end_frame) {
                total = (t - last) * (1.0f / 15.0f);
                EMITTER_DRIFT(total);
                return;
            }
            EMITTER_FORCE_TRANSFORMS(last);
            total = (t - last) * (1.0f / 15.0f);
            EMITTER_SUBSTEPS(total);
        }
    } else {
        EMITTER_DRIFT(dt);
    }
}

// 0x561190  Confidence: exact (the header part is relocated as a vfx_node_header, D4606).
void vfx_emitter_desc::relocate(char *header_base, char *base)
{
    int i;

    ((vfx_node_header *)this)->relocate(header_base, base);
    offset.relocate(base);
    orient.relocate(base);
    ch_a0.relocate(base);
    ch_ac.relocate(base);
    ch_b8.relocate(base);
    ch_c4.relocate(base);
    ch_d0.relocate(base);
    rate_ch.relocate(base);
    for (i = 0; i < 3; i++)
        ptr_relocate(&links[i], base);
}

// 0x561220  Confidence: exact (one AND for each group of bit clears).
void fx_recb0::init(vfx_emitter_desc *desc)
{
    flags &= 0xff000000;
    bound = 0;
    placed = 0;
    hidden = 0;
    v1c.zero();
    m28.make_identity();
    time = 0.0f;
    if (desc->start_frame < 0)
        last_t = desc->start_frame;
    else
        last_t = 0.0f;
    spawn_accum = 0.0f;
    v64 = desc->offset.sample(0);
    _bit31 = 0;
}

// 0x5612b0  Confidence: likely (the frame tests are made on the FPU, so one side is a float in the
// source). The offset and rotation blended between two frames (unless placed), the five shape values
// blended in 8-bit steps, the alpha from the material's curve 48; through the model's bone when bound.
void fx_recb0::get_transform(vfx_emitter_desc *desc, float t, bool wrapped)
{
    int frame;
    int next;
    float frac;
    int w1;
    int w0;

    frame = (int)t;
    next = frame + 1;
    if ((float)frame >= (float)desc->end_frame) {
        if (wrapped)
            frame = frame % desc->end_frame;
        else
            frame = desc->end_frame - 1;
    }
    if ((float)next >= (float)desc->end_frame) {
        if (wrapped)
            next = next % desc->end_frame;
        else
            next = frame;
    }
    frac = t - frame;
    w1 = (int)(frac * 255.0f);
    w0 = 255 - w1;
    if (!placed) {
        v70 = desc->get_offset(frame) * (1.0f - frac) + desc->get_offset(next) * frac;
        quaternion qa;
        quaternion qb;
        qa.unpack_short4(desc->get_orient(frame));
        qb.unpack_short4(desc->get_orient(next));
        quaternion_slerp(&q7c, &qa, &qb, frac);
    }
    size = (short)((desc->eval_channel_b8(frame) * w0 + desc->eval_channel_b8(next) * w1) >> 8);
    width = (short)((desc->eval_channel_a0(frame) * w0 + desc->eval_channel_a0(next) * w1) >> 8);
    depth = (short)((desc->eval_channel_ac(frame) * w0 + desc->eval_channel_ac(next) * w1) >> 8);
    speed = (short)((desc->eval_channel_c4(frame) * w0 + desc->eval_channel_c4(next) * w1) >> 8);
    speed_var = (short)((desc->eval_channel_d0(frame) * w0 + desc->eval_channel_d0(next) * w1) >> 8);
    alpha = (int)(desc->base->materials[desc->material].eval_curve48(t) * 255.0f);
    if (!placed && bound) {
        matrix43 m;
        vector3 bpos;
        matrix brot;
        matrix43 bm = ci->compute_bone_matrix(tag);
        bm.get_rotation(&brot);
        bm.get_position(&bpos);
        matrix43 world(bpos, brot);
        v70 = world.transform(v70);
        m.set(q7c, v70);
        m.set_product_4x4(m, world);
        q7c = quaternion(*(matrix *)&m);
    }
}

// 0x561670  Confidence: exact. (desc, frame, position, orientation, wrapped): the emission points
// before and after, the transform and the forces (skipped while bound and stopped), the ages (a
// particle past its lifetime is freed), then the moves and the new particles.
void fx_recb0::update(vfx_emitter_desc *desc, float t, const vector3 *pos, const matrix *orient, bool wrapped)
{
    float dt;
    int n;
    int i;
    float age;
    fx_rec28 *p;

    if (t != last_t) {
        if (!bound || !stopped) {
            if (time > 0.0f) {
                v4c = v70;
                v4c.unrotate(m28);
                v4c += v1c;
            }
            if (pos != NULL)
                v1c = *pos;
            else
                v1c.zero();
            if (orient != NULL)
                m28 = *orient;
            if (!_bit31) {
                _bit31 = 1;
                v64 += v1c;
            }
            get_transform(desc, t, wrapped);
            v58 = v70;
            v58.unrotate(m28);
            v58 += v1c;
            if (time == 0.0f)
                v4c = v58;
            for (i = 0; i < (int)desc->num_links; i++) {
                if (links[i] != NULL && desc->links[i] != NULL)
                    links[i]->update((vfx_force_desc *)desc->links[i], t);
            }
        }
        dt = (wrapped ? desc->end_frame - last_t + t : t - last_t) * (1.0f / 15.0f);
        time += last_t < 0.0f ? t * (1.0f / 15.0f) : dt;
        n = 0;
        for (p = particles; p != NULL; ) {
            age = p->get_age();
            if (age >= 0.0f) {
                n++;
                p->life.set(age + dt);
                if ((ushort)p->life.v >= p->lifetime) {
                    p->life.set(-1.0f);
                    num_live--;
                }
            }
            p = p->next;
            if (p == particles)
                break;
        }
        if (n == 0)
            num_live = 0;
        vector3 vel = (v58 - v4c) / dt;
        move_particles(desc, t, dt);
        if (!stopped)
            spawn(desc, t, dt, vel);
        last_t = t;
    }
}

// 0x561900  Confidence: unsure (the corner arithmetic is scheduled across the six vertices; the order of
// the statements is a guess; the lock flags test vb_pos == 0, D13605).
// No caller: the Direct3D batch path of render. Two triangles per particle
// into the shared vertex buffer, flushed every 1000 vertices. Game bugs (NOTES.md): particles in the top
// 100 pixels are culled (the y test is against +100, x against -100); rhw ends up as the view z.
void fx_recb0::render_d3d(vfx_emitter_desc *desc, float t)
{
    int bitmaps[2];
    vfx_material *mat;
    float u_scale;
    float v_scale;
    fx_rec28 *p;
    int vb_pos;
    int count;
    int end;
    void *data;
    d3d_tlvertex *v;
    float age;
    float life;
    float size;
    float fade;
    float alpha;
    float hw;
    float hh;
    int w;
    int h;
    int j;

    mat = get_bitmaps(desc, particles, bitmaps);
    gr_d3d_set_state((mat->flags & 0x40) || desc->white ? gr_mode_particle_additive : gr_mode_particle_alpha);
    gr_d3d_tcache_set(0, bitmaps[0], 0, &u_scale, &v_scale, 1);
    p = particles;
    vb_pos = d3d_vertex_buffer_pos;
    while (p != NULL) {
        count = 0;
        IDirect3DVertexBuffer7_Lock(d3d_vertex_buffer, vb_pos == 0 ? gr_d3d_lock_discard : gr_d3d_lock_nooverwrite,
                                    &data, NULL);
        v = (d3d_tlvertex *)data + vb_pos;
        for (end = vb_pos + 6; end < 1000 && p != NULL; p = p->next != particles ? p->next : NULL) {
            age = p->get_age();
            life = p->get_lifetime();
            if (!(age >= 0.0f && age < life))
                continue;
            get_fade(desc, age, life, &size, &fade, &alpha);
            if (alpha < 0.03f)
                continue;
            if (bitmaps[0] > -1) {
                bm_get_dimensions(gr_texture, &w, &h);
                if (w < h) {
                    hw = size;
                    hh = h * size / w;
                } else if (w > h) {
                    hh = size;
                    hw = w * size / h;
                } else {
                    hw = hh = size;
                }
            } else {
                hw = hh = size;
            }
            vector3 rel = p->get_pos() - gr_view_pos;
            rel.rotate(gr_view_matrix);
            if (!(rel.z > 0.0f))
                continue;
            alpha *= gr_get_fog_value(255, rel.z) * (1.0f / 255.0f);
            vector3 scr = rel;
            scr.z = 1.0f / scr.z;
            scr.x = (scr.x * scr.z + 1.0f) * gr_projection_xmul + gr_screen.offset_x;
            scr.y = (1.0f - scr.y * scr.z) * gr_projection_ymul + gr_screen.offset_y;
            if (scr.x < -100.0f || !(gr_screen.max_w + 100.0f > scr.x) || scr.y < 100.0f ||
                !(gr_screen.max_h + 100.0f > scr.y))
                continue;
            float angle = p->get_size();
            float sn = (float)sin(angle);
            float cs = (float)cos(angle);
            ubyte r, g, b;
            get_color(desc, mat, t, &r, &g, &b);
            D3DCOLOR color = (((((int)alpha << 8) | r) << 8 | g) << 8) | b;
            float ay;
            float by;
            v[0].sx = (sn * hh - cs * hw) * gr_matrix_scale.x + rel.x;
            v[0].tu = 0.0f;
            v[0].tv = 1.0f;
            v[1].tu = 1.0f;
            v[1].tv = 1.0f;
            ay = sn * hw;
            by = cs * hh;
            v[0].sy = (-ay - by) * gr_matrix_scale.y + rel.y;
            v[1].sx = (cs * hw + sn * hh) * gr_matrix_scale.x + rel.x;
            v[1].sy = (ay - by) * gr_matrix_scale.y + rel.y;
            v[2].tu = 1.0f;
            v[2].tv = 0.0f;
            v[3] = v[0];
            v[2].sx = (cs * hw - sn * hh) * gr_matrix_scale.x + rel.x;
            v[2].sy = (by + ay) * gr_matrix_scale.y + rel.y;
            v[4] = v[2];
            v[5].tu = 0.0f;
            v[5].tv = 0.0f;
            v[5].sx = (-(cs * hw) - sn * hh) * gr_matrix_scale.x + rel.x;
            v[5].sy = (by - ay) * gr_matrix_scale.y + rel.y;
            float rhw = scr.z;
            for (j = 0; j < 6; j++) {
                v[j].rhw = rhw;
                v[j].sx = (rhw * v[j].sx + 1.0f) * gr_projection_xmul + gr_screen.offset_x;
                v[j].sy = (1.0f - rhw * v[j].sy) * gr_projection_ymul + gr_screen.offset_y;
                v[j].sz = gr_calc_depth(rhw);
                v[j].rhw = rel.z;
                v[j].color = color;
            }
            v += 6;
            count += 6;
            end += 6;
        }
        IDirect3DVertexBuffer7_Unlock(d3d_vertex_buffer);
        IDirect3DDevice7_DrawPrimitiveVB(d3d_device, D3DPT_TRIANGLELIST, d3d_vertex_buffer, vb_pos, count, 0);
        vb_pos += count;
        if (vb_pos + 6 >= 1000)
            vb_pos = 0;
    }
    d3d_vertex_buffer_pos = vb_pos;
}

// 0x561ec0  Confidence: likely (gr_vertex has no constructor here, the original runs gr::Vertex's
// 0x5081c0 on the three vertices, D1605). Point particles through gr_bitmap_3d, flat ones as a
// triangle (the tip black) through gr_poly, in the instance frame at the origin v64. D19900: the ring walk
// as `for (...; p = p->next)` with `if (p->next == particles) break;` gives the binary's desc / p registers.
void fx_recb0::render(vfx_emitter_desc *desc, float t)
{
    vfx_material *mat;
    fx_rec28 *p;
    float age;
    float life;
    float size;
    float fade;
    float alpha;
    int i;

    if (!hidden) {
        gr_start_instance(&v64, &identity_matrix, 1);
        mat = &desc->base->materials[desc->material];
        gr::Mode mode;
        if (!desc->flat)
            mode = (mat->flags & 0x40) || desc->white ? gr_mode_particle_additive : gr_mode_particle_alpha;
        else
            mode = (mat->flags & 0x40) ? gr_mode_vfx_flat_additive : gr_mode_vfx_flat_alpha;
        for (p = particles; p != NULL; p = p->next) {
            age = p->get_age();
            life = p->get_lifetime();
            if (age >= 0.0f && age < life) {
                get_fade(desc, age, life, &size, &fade, &alpha);
                if (alpha >= 0.03f) {
                    vector3 pos = p->get_pos();
                    if (!desc->flat) {
                        int bitmaps[2];
                        ubyte r, g, b;
                        ubyte a;
                        get_bitmaps(desc, p, bitmaps);
                        get_color(desc, mat, t, &r, &g, &b);
                        a = (ubyte)(int)alpha;
                        gr_set_texture(bitmaps[0], -1);
                        gr_set_color(r, g, b, a);
                        gr_bitmap_3d(&pos, p->get_size(), size, mode);
                    } else {
                        vector3 va;
                        vector3 vb;
                        vector3 vc;
                        get_billboard_axes(desc, p, &va, &vb, &vc, size, fade);
                        gr_vertex v0;
                        gr_vertex v1;
                        gr_vertex v2;
                        gr_rotate_vertex(&v0, &va);
                        gr_rotate_vertex(&v1, &vb);
                        gr_rotate_vertex(&v2, &vc);
                        gr_project_vertex(&v0);
                        gr_project_vertex(&v1);
                        gr_project_vertex(&v2);
                        ubyte r, g, b;
                        get_color(desc, mat, t, &r, &g, &b);
                        gr_vertex *verts[3];
                        verts[0] = &v0;
                        verts[1] = &v1;
                        verts[2] = &v2;
                        for (i = 0; i < 3; i++) {
                            gr_vertex *vt = verts[i];
                            vt->_unk1a = 0xc;
                            if (i == 2) {
                                vt->r = 0;
                                vt->g = 0;
                                vt->b = 0;
                            } else {
                                vt->r = (ubyte)(int)(mat->color[0] * r * (1.0f / 255.0f));
                                vt->g = (ubyte)(int)(mat->color[1] * g * (1.0f / 255.0f));
                                vt->b = (ubyte)(int)(mat->color[2] * b * (1.0f / 255.0f));
                            }
                            vt->a = (ubyte)(int)alpha;
                        }
                        gr_poly(3, verts, mode, 0, 0);
                    }
                }
            }
            if (p->next == particles)
                break;
        }
        gr_stop_instance();
    }
}

// COMDATs of this object not written above (inline in the headers; one per line, address order). The
// vfx_emitter_desc members are in vfx.h, fx_rec28 / fx_recb0 in effect_mem.h, the fixed-point helpers in
// fixed.h. Confidence: exact for all.
// 0x5622b0 vfx_emitter__get_rate: vfx_emitter_desc::get_rate(frame).
// 0x5622d0 fixed12_to_float.
// 0x5622f0 vfx_particle__set_lifetime: fx_rec28::set_lifetime.
// 0x562310 vfx_particle__set_size: fx_rec28::set_size (the angle word +4).
// 0x562330 float_to_fixed12.
// 0x562340 vfx_particle__set_velocity: fx_rec28::set_pos (+0x14; sum.symbols calls it the velocity, D4607).
// 0x562360 vfx_particle__set_pos: fx_rec28::set_vel (+8).
// 0x562380 vfx_emitter__get_fixed12_8c: fx_recb0::get_fixed12_8c.
// 0x562390 vfx_emitter__get_fixed12_8e: fx_recb0::get_fixed12_8e.
// 0x5623a0 vfx_emitter__get_fixed12_92: fx_recb0::get_fixed12_92.
// 0x5623b0 vfx_emitter__get_fixed12_94: fx_recb0::get_fixed12_94.
// 0x5623c0 vector3::operator/ (vector.h).
// 0x562400 vfx_particle__get_velocity: fx_rec28::get_pos (+0x14).
// 0x562420 vfx_particle__get_pos: fx_rec28::get_vel (+8).
// 0x562440 vfx_vec_pair__ctor: vfx_force_params' implicit constructor (two vector3()).
// 0x562460 vfx_emitter__get_offset: vfx_emitter_desc::get_offset(frame).
// 0x562480 vfx_emitter__get_orient: vfx_emitter_desc::get_orient(frame).
// 0x5624a0 vfx_emitter__eval_channel_a0: vfx_emitter_desc::eval_channel_a0.
// 0x5624c0 vfx_emitter__eval_channel_ac: vfx_emitter_desc::eval_channel_ac.
// 0x5624e0 vfx_emitter__eval_channel_b8: vfx_emitter_desc::eval_channel_b8.
// 0x562500 vfx_emitter__eval_channel_c4: vfx_emitter_desc::eval_channel_c4.
// 0x562520 vfx_emitter__eval_channel_d0: vfx_emitter_desc::eval_channel_d0.
// 0x562540 vfx_particle__get_size: fx_rec28::get_size.
// 0x5627d0 vfx_emitter__get_fade_in: vfx_emitter_desc::get_fade_in.
// 0x5627f0 vfx_emitter__get_fade_out: vfx_emitter_desc::get_fade_out.
// 0x562810 vfx_emitter__get_fixed8_2e: vfx_emitter_desc::get_fixed8_2e.
// 0x562820 vfx_emitter__get_fixed8_30: vfx_emitter_desc::get_fixed8_30.
// 0x562830 vfx_emitter__get_fixed12_90: fx_recb0::get_fixed12_90.
// 0x562a20 vfx_emitter__get_fixed8_32: vfx_emitter_desc::get_fixed8_32 (the last COMDAT).
