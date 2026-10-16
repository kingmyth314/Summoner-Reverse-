// unnamed_vfx_mesh_0x562a30.cpp - Sum.exe v1.40, code 0x562a30-0x564d62: the vfx mesh object (the
// dump_mesh_frames console command, mesh instances, their animation and drawing, mesh relocation; vsdk\geom
// in sum.symbols). The file name is a placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout or
// scheduling risk, unsure = the source shape is a guess.
//
// Unit range (decomp66 NOTES-unnamed-vsdk-tail.md, D4611): static initialiser table entry 651
// (0x562a30, the console command) after the emitter object's last COMDAT (0x562a2b); the functions run to
// vfx_node_desc::load_bitmap (0x564761); the inline accessors of vsdk/geom/vfx.h follow as COMDATs
// (0x564770-0x564d62, ending with ptr_relocate). .bss 0x2f91a60-0x2f91a6f, .data 0x5ae104-0x5ae1db.
// The vertex and face-uv scratch arrays it fills (vfx_mesh_verts 0x272b900, vfx_mesh_face_uvs 0x2c98a90)
// lie in effect_mem.cpp's .bss (D5110).
//
// Build flags: no inline expansion (every vector3 / matrix / quaternion member, gr::Mode's constructor,
// float_min / float_max and the record accessors are called). Float constants: 1 / 15 0x581e68,
// 0.5f 0x57e6b0, 1.0f 0x57e990, -0.5f 0x580df4, 30.0f 0x581adc, 160.0f 0x581d5c, 61.0f 0x58233c,
// 255.0f 0x57eb80, 64.0f (pushed 0x42800000), 0.5f (pushed 0x3f000000).

#include <stdio.h>
#include <math.h>
#include "vsdk/geom/effect_mem.h"
#include "vsdk/geom/vfx.h"
#include "vsdk/ca/skeleton.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/unnamed_clip_0x5590b0.h"
#include "vsdk/gr/unnamed_gr_poly_0x5450c0.h"
#include "vsdk/gr/unnamed_gr_line_0x53e520.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/console.h"

void cmd_dump_mesh_frames();

// 0x562a30 init_ccmd_dump_mesh_frames_thunk (jmp, static initialiser entry 651). Confidence: exact.
// 0x562a40 init_ccmd_dump_mesh_frames (console_command constructor). Confidence: exact.
console_command ccmd_dump_mesh_frames("dump_mesh_frames", "Toggles Dump_mesh_frames", cmd_dump_mesh_frames);   // 0x2f91a60
bool Dump_mesh_frames;                // 0x2f91a6c (nothing reads it)

// 0x562a60  Confidence: exact (the console's bool command macro).
void cmd_dump_mesh_frames()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Dump_mesh_frames = true;
        else if (arg_type & ARG_FALSE)
            Dump_mesh_frames = false;
        else if (arg_type & ARG_NONE)
            Dump_mesh_frames ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it.\n",
                "dump_mesh_frames", "Dump_mesh_frames");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "dump_mesh_frames", Dump_mesh_frames ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x562b20  Confidence: likely (the three flag bits are one AND / OR in the code). Hidden until the first update_frame; not bound, not placed.
void fx_rec98::init(vfx_node_desc *desc)
{
    v00 = desc->pos;
    hidden = 1;
    placed = 0;
    bound = 0;
    m5c.make_identity();
    v20 = vector3(0.0f, 0.0f, 0.0f);
    m2c.make_identity();
    glow = 0;
}

// 0x562b90  Confidence: unsure (an empty function with four stack arguments, ret 0x10; no caller).
void fx_rec98::stub_ret10(int a, int b, int c, int d)
{
}

// 0x562ba0  Confidence: exact. The frame is computed once per instance and frame (vfx_last_mesh_instance);
// pos / orient replace the local transform when given. The glow bitmap is drawn at the mesh's origin.
void fx_rec98::render(vfx_node_desc *desc, float t, const vector3 *pos, const matrix *orient)
{
    if (this != vfx_last_mesh_instance) {
        vfx_last_mesh_instance = this;
        update_frame(desc, t);
    }
    if (hidden)
        return;
    if (pos)
        v50 = *pos;
    if (orient)
        m5c = *orient;
    if (desc->is_billboard())
        render_billboards(desc, t);
    else
        render_mesh(desc, t);

    if (desc->glow_name) {
        vector3 p(v00);
        p.unrotate(m5c);
        p += v50;
        gr_set_texture(desc->glow_bm, -1);
        gr_set_color(255, 255, 255, gr_get_point_fog_value(255, &p));
        gr_bitmap_3d(&p, 0.0f, desc->glow_radius, gr_mode_particle_additive);
    }
}

// 0x562ca0  Confidence: likely (the order of the two pass blocks). The face material is the mesh's first
// material. A sprite is drawn as an oriented bitmap of width x height, a line as a quad of that width
// along the axis (always additive); material curve 34 adds a second layer pass.
void fx_rec98::render_billboards(vfx_node_desc *desc, float t)
{
    vfx_material *mat = &desc->base->materials[desc->materials[0]];
    float a = mat->eval_curve48(t) * alpha;

    if (desc->vis_flags & VFX_MESH_SPRITE) {
        vector3 p(v00);
        p.unrotate(m5c);
        p += v50;
        gr::Mode mode = (mat->flags & 0x40) ? gr_mode_particle_additive : gr_mode_particle_alpha;
        gr_set_texture(mat->layers[0].get_frame_bitmap(t), -1);
        gr_set_color(255, 255, 255, gr_get_point_fog_value((int)a, &p));
        gr_bitmap_3d_oriented(&p, 0.0f, width, height, mode);
        if (mat->curve34_mode == 1) {
            float c = mat->eval_curve34(t);
            gr_set_texture(mat->layers[1].get_frame_bitmap(t), -1);
            int k = (int)(c * 255.0f);
            gr_set_color(k, k, k, gr_get_point_fog_value((int)(c * a), &p));
            gr_bitmap_3d_oriented(&p, 0.0f, width, height, mode);
        }
        return;
    }
    if (desc->vis_flags & VFX_MESH_LINE) {
        vector3 p0 = v00 + v80 * height * 0.5f;
        p0.unrotate(m5c);
        p0 += v50;
        vector3 p1 = v00 - v80 * height * 0.5f;
        p1.unrotate(m5c);
        p1 += v50;
        gr_set_texture(mat->layers[0].get_frame_bitmap(t), -1);
        gr_set_color(255, 255, 255, gr_get_point_fog_value((int)a, &p1));
        gr_line_quad_3d(&p0, &p1, width, gr_mode_particle_additive);
        if (mat->curve34_mode == 1) {
            float c = mat->eval_curve34(t);
            gr_set_texture(mat->layers[1].get_frame_bitmap(t), -1);
            int k = (int)(c * 255.0f);
            gr_set_color(k, k, k, gr_get_point_fog_value((int)(c * a), &p1));
            gr_line_quad_3d(&p0, &p1, width, gr_mode_particle_additive);
        }
    }
}

// 0x562f60  Confidence: unsure (the frame layout of this long function: which locals share slots and
// the order of the per-face blocks; the calls, their arguments and the control flow follow the code).
// sum.symbols vfx_mesh_render. Pass 1 lights each shared vertex once (the mean of its faces' normals,
// not below material curve 44), culls faces behind or outside one clip plane, and sorts the rest by
// their farthest corner plus 61 per material index (back to front; unsorted for level meshes). Pass 2
// draws each face: layer 0, layer 1 when material curve 34 is on (scaled colours, at t minus the mesh's
// start), and the environment map (the material bitmap, sphere-mapped, u wrapped next to the first
// corner's) with the material's +0x0b byte as alpha.
void fx_rec98::render_mesh(vfx_node_desc *desc, float t)
{
    if (desc->num_faces == 0)
        return;

    vector3 center(v00);
    center.unrotate(m5c);
    center += v50;
    gr_start_instance(&v50, &m5c, 1);
    lights_collect_enabled();
    lights_select_nearest(-1, &center, desc->glow_radius);
    lights_transform_selection();

    int i, k;
    for (i = 0; i < desc->num_vnormals; i++)
        desc->vnormals[i].done = 0;
    gr_rotate_vertices(gr_vertex_pool, vfx_mesh_verts, desc->num_verts);
    vfx_mesh_sort_count = 0;
    gr_sort_entry *sort = vfx_mesh_sort_buf;

    for (i = 0; i < desc->num_faces; i++) {
        vfx_face *face = &desc->faces[i];
        char msg[256];
        if (face->material < 0) {
            sprintf(msg, "Face is missing a material -- mesh %s\n", desc->name);
            continue;
        }
        vfx_material *mat = &desc->base->materials[desc->materials[face->material]];
        if (mat->layers[0].bitmap <= 0 && mat->layers[1].bitmap <= 0)
            continue;

        gr_vertex *gv[3];
        vfx_vnormal *vn[3];
        ubyte codes = 0xff;
        for (k = 0; k < 3; k++) {
            gv[k] = &gr_vertex_pool[face->v[k]];
            vn[k] = &desc->vnormals[face->vn[k]];
            codes &= gv[k]->codes;
        }
        if (codes)
            continue;

        vector3 normal;
        vector3_perp(&normal, &vfx_mesh_verts[face->v[0]], &vfx_mesh_verts[face->v[1]],
                     &vfx_mesh_verts[face->v[2]]);
        normal.normalize();
        if (!gr_check_normal_facing(&normal, &vfx_mesh_verts[face->v[0]]))
            continue;

        ubyte min_light = (ubyte)(int)(mat->eval_curve44(t) * 255.0f);
        for (k = 0; k < 3; k++) {
            vfx_vnormal *n = vn[k];
            if (n->done)
                continue;
            n->done = 1;
            if (!(desc->vis_flags & VFX_MESH_FULLBRIGHT) && min_light < 0xfe) {
                vector3 sum;
                sum.zero();
                int count = n->count;
                for (int j = 0; j < count; j++)
                    sum += desc->faces[n->faces[j]].get_normal();
                sum *= 1.0f / count;
                sum.normalize_safe();
                n->pack_normal(&sum);
                // the last argument is pushed as the bits of -1.0f (lights_shade_vertices does not read it)
                lights_shade_vertices(&n->r, &n->g, &n->b, &vfx_mesh_verts[n->vertex], &sum, 1, 0,
                                      (int)0xbf800000);
                if (n->r < min_light)
                    n->r = min_light;
                if (n->g < min_light)
                    n->g = min_light;
                if (n->b < min_light)
                    n->b = min_light;
            } else {
                n->r = 0xff;
                n->g = 0xff;
                n->b = 0xff;
            }
        }

        float bias = face->material * 61.0f;
        float z0 = bias + gv[0]->z;
        float z1 = bias + gv[1]->z;
        float z2 = bias + gv[2]->z;
        sort->key = float_max(float_max(z0, z1), float_max(z1, z2));
        sort->index = (ushort)i;
        sort++;
        vfx_mesh_sort_count++;
    }

    if (!level)
        sort_by_float_key(vfx_mesh_sort_buf, vfx_mesh_sort_count);
    int n = vfx_mesh_sort_count;
    gr_set_color(255, 255, 255, 255);
    for (sort = vfx_mesh_sort_buf; n; sort++, n--) {
        int fi = sort->index;
        vfx_face *face = &desc->faces[fi];
        if (face->material < 0 || face->material >= desc->num_materials)
            continue;
        vfx_material *mat = &desc->base->materials[desc->materials[face->material]];
        gr_vertex *g0 = &gr_vertex_pool[face->v[0]];
        gr_vertex *g1 = &gr_vertex_pool[face->v[1]];
        gr_vertex *g2 = &gr_vertex_pool[face->v[2]];
        vfx_vnormal *vn[3];
        vn[0] = &desc->vnormals[face->vn[0]];
        vn[1] = &desc->vnormals[face->vn[1]];
        vn[2] = &desc->vnormals[face->vn[2]];

        gr_poly_begin(level, *(int *)&sort->key, 0, 0, 0);
        gr_poly_vertex(g0);
        gr_poly_vertex(g1);
        gr_poly_vertex(g2);

        float eu[3], ev[3];
        if (mat->bitmap > 0) {
            for (k = 0; k < 3; k++) {
                vector3 nrm = vn[k]->unpack_normal();
                gr_env_map_uv(&eu[k], &ev[k], &vfx_mesh_verts[vn[k]->vertex], &nrm);
            }
        }
        if (mat->bitmap > 0) {
            if (eu[1] - eu[0] > 0.5f)
                eu[1] -= 1.0f;
            if (eu[1] - eu[0] < -0.5f)
                eu[1] += 1.0f;
            if (eu[2] - eu[0] > 0.5f)
                eu[2] -= 1.0f;
            if (eu[2] - eu[0] < -0.5f)
                eu[2] += 1.0f;
        }

        float a = mat->eval_curve48(t) * alpha;
        gr_poly_set_texture((mat->flags & 0x40) ? gr_mode_level_alpha_add : gr_mode_mesh_alpha,
                            mat->layers[0].get_frame_bitmap(t), 255, 255, 255, 255, 0);
        vfx_face_uv *uv = &vfx_mesh_face_uvs[fi];
        vector3 us(uv->get_u(0), uv->get_u(1), uv->get_u(2));
        vector3 vs(uv->get_v(0), uv->get_v(1), uv->get_v(2));
        gr_poly_uv(us.x, vs.x);
        gr_poly_uv(us.y, vs.y);
        gr_poly_uv(us.z, vs.z);
        gr_poly_rgb(vn[0]->r, vn[0]->g, vn[0]->b);
        gr_poly_rgb(vn[1]->r, vn[1]->g, vn[1]->b);
        gr_poly_rgb(vn[2]->r, vn[2]->g, vn[2]->b);
        ubyte ia = (ubyte)(int)a;
        if (level) {
            gr_poly_alpha(ia);
            gr_poly_alpha(ia);
            gr_poly_alpha(ia);
        } else {
            gr_poly_alpha(gr_get_fog_value(ia, g0->z));
            gr_poly_alpha(gr_get_fog_value(ia, g1->z));
            gr_poly_alpha(gr_get_fog_value(ia, g2->z));
        }

        if (mat->curve34_mode == 1) {
            float c = mat->eval_curve34(t - desc->start_time);
            int bm = mat->layers[1].get_frame_bitmap(t);
            if (bm_has_alpha(bm)) {
                gr_poly_set_texture(gr_mode_mesh_alpha, bm, 255, 255, 255,
                                    gr_get_point_fog_value((int)(c * a), &center), 0);
            } else {
                gr::Mode mode = (mat->flags & 0x40) ? gr_mode_level_alpha_add : gr_mode_vfx_mesh_opaque;
                gr_poly_set_texture(mode, bm, 255, 255, 255, float_round_to_int(c * a), 0);
            }
            gr_poly_uv(us.x, vs.x);
            gr_poly_uv(us.y, vs.y);
            gr_poly_uv(us.z, vs.z);
            gr_poly_rgb((int)(vn[0]->r * c), (int)(vn[0]->g * c), (int)(vn[0]->b * c));
            gr_poly_rgb((int)(vn[1]->r * c), (int)(vn[1]->g * c), (int)(vn[1]->b * c));
            gr_poly_rgb((int)(vn[2]->r * c), (int)(vn[2]->g * c), (int)(vn[2]->b * c));
        }

        if (mat->bitmap > 0) {
            int env_alpha = mat->_unk0b;
            gr_poly_set_texture(gr::Mode(1, 1, 0, 3, 1, 0), mat->bitmap, 255, 255, 255,
                                gr_get_point_fog_value(float_round_to_int(env_alpha * a), &center), 0);
            gr_poly_uv(eu[0], ev[0]);
            gr_poly_uv(eu[1], ev[1]);
            gr_poly_uv(eu[2], ev[2]);
        }
        gr_poly_end();
    }
    lights_pop_selection();
    gr_stop_instance();
}

// 0x563a40  Confidence: likely (the frame layout and the source shape of the three branches; the calls
// and their order follow the code). t is in 1/15 s. Outside the mesh's frames (before start_time or past
// num_frames) it is hidden. Animated meshes take the anim record (billboards), the morph targets or the
// static vertices plus the origin; skinned ones the rest pose, then the anim_track (or frame 0 of the
// xform) at 30 * 160 ticks per second; the others blend two xform frames. Then the shown-frame bit, the
// face uvs (animated or static), the line axis and the world transform (placed, or the bound bone).
// In the xform branch the * 64 of VFX_MESH_X64 comes after the transform (see NOTES.md).
void fx_rec98::update_frame(vfx_node_desc *desc, float t)
{
    float tsec = t * (1.0f / 15.0f);
    int rate = desc->mesh_flags >> 6;
    float frame = (tsec - desc->start_time) * rate;
    int i, j;

    if (frame < 0.0f || desc->num_frames < frame) {
        hidden = 1;
        return;
    }
    int f0 = (int)frame;
    float frac = frame - f0;
    int f1 = f0 + 1;
    if (desc->vis_flags & VFX_MESH_NO_LERP)
        frac = 0.0f;
    if (f1 >= desc->num_frames)
        f1 = desc->num_frames - 1;

    if (desc->has_flag4()) {
        if (desc->is_billboard()) {
            float u = 1.0f - frac;
            v00 = desc->anim->get_vec(f0) * u + desc->anim->get_vec(f1) * frac;
            width = desc->anim->get_a(f0) * u + desc->anim->get_a(f1) * frac;
            height = desc->anim->get_b(f0) * u + desc->anim->get_b(f1) * frac;
        } else if (!(desc->mesh_flags & VFX_MESH_STATIC_VERTS)) {
            v00 = desc->morph->origin;
            float u = 1.0f - frac;
            for (i = 0; i < desc->num_verts; i++) {
                vfx_mesh_verts[i] = desc->morph->get_pos(i, f0) * u + desc->morph->get_pos(i, f1) * frac;
                if (desc->mesh_flags & VFX_MESH_X64)
                    vfx_mesh_verts[i] *= 64.0f;
                vfx_mesh_verts[i] += v00;
            }
        } else {
            v00 = desc->vertices->origin;
            for (i = 0; i < desc->num_verts; i++) {
                vfx_mesh_verts[i] = desc->vertices->get_vertex(i);
                if (desc->mesh_flags & VFX_MESH_X64)
                    vfx_mesh_verts[i] *= 64.0f;
                vfx_mesh_verts[i] += v00;
            }
        }
    } else if (desc->mesh_flags & VFX_MESH_SKINNED) {
        int ticks = (int)(tsec * 30.0f * 160.0f);
        int start = (int)(desc->start_time * 30.0f * 160.0f);
        vector3 rest_pos(desc->rest_pos);
        matrix rest_rot;
        quaternion rq;
        rq.unpack_short4(desc->rest_rot);
        rq.to_matrix_b(&rest_rot);
        vector3 rest_scale(desc->rest_scale);
        vector3 pos;
        if (desc->track->num_pos_keys)
            pos = desc->track->get_position(ticks - start);
        else
            pos = desc->xform->get_pos((desc->mesh_flags >> 3) & 1, 0);
        matrix rot;
        if (desc->track->num_rot_keys) {
            desc->track->get_rotation(ticks - start, false).to_matrix_b(&rot);
        } else {
            quaternion q;
            q.unpack_short4(desc->xform->get_rot(0));
            q.to_matrix_b(&rot);
        }
        vector3 scale;
        if (desc->track->num_scale_keys)
            scale = desc->track->get_scale(ticks - start);
        else
            scale = desc->xform->get_scale(0);
        if (desc->is_billboard())
            v00 = desc->sprite->get_pos();
        else
            v00 = desc->vertices->origin;

        if (!desc->is_billboard()) {
            for (i = 0; i < desc->num_verts; i++) {
                vector3 *v = &vfx_mesh_verts[i];
                *v = desc->vertices->get_vertex(i);
                if (desc->mesh_flags & VFX_MESH_X64)
                    *v *= 64.0f;
                *v += v00;
                v->x = rest_scale.x * v->x;
                v->y = rest_scale.y * v->y;
                v->z = rest_scale.z * v->z;
                v->unrotate(rest_rot);
                *v += rest_pos;
                v->x = scale.x * v->x;
                v->y = scale.y * v->y;
                v->z = scale.z * v->z;
                v->unrotate(rot);
                *v += pos;
            }
        } else {
            width = (float)fabs(desc->sprite->get_w() * scale.x * rest_scale.x);
            height = (float)fabs(desc->sprite->get_h() * scale.y * rest_scale.y);
        }
        v00.x = rest_scale.x * v00.x;
        v00.y = rest_scale.y * v00.y;
        v00.z = rest_scale.z * v00.z;
        v00.unrotate(rest_rot);
        v00 += rest_pos;
        v00.x = scale.x * v00.x;
        v00.y = scale.y * v00.y;
        v00.z = scale.z * v00.z;
        v00.unrotate(rot);
        v00 += pos;
    } else {
        matrix rot;
        quaternion q0, q1, q;
        q0.unpack_short4(desc->xform->get_rot(f0));
        q1.unpack_short4(desc->xform->get_rot(f1));
        quaternion_slerp(&q, &q0, &q1, frac);
        q.to_matrix_b(&rot);
        float u = 1.0f - frac;
        vector3 scale = desc->xform->get_scale(f0) * u + desc->xform->get_scale(f1) * frac;
        if (desc->is_billboard()) {
            width = (float)fabs(desc->sprite->get_w() * scale.x);
            height = (float)fabs(desc->sprite->get_h() * scale.y);
            v00 = desc->sprite->get_pos();
        } else {
            v00 = desc->vertices->origin;
        }
        v00.x = scale.x * v00.x;
        v00.y = scale.y * v00.y;
        v00.z = scale.z * v00.z;
        v00.unrotate(rot);
        vector3 pos = desc->xform->get_pos((desc->mesh_flags >> 3) & 1, f0) * u
                    + desc->xform->get_pos((desc->mesh_flags >> 3) & 1, f1) * frac;
        v00 += pos;
        if (!desc->is_billboard()) {
            for (i = 0; i < desc->num_verts; i++) {
                vector3 *v = &vfx_mesh_verts[i];
                *v = desc->vertices->get_vertex(i);
                v->x = scale.x * v->x;
                v->y = scale.y * v->y;
                v->z = scale.z * v->z;
                v->unrotate(rot);
                *v += pos;
                if (desc->mesh_flags & VFX_MESH_X64)
                    *v *= 64.0f;
            }
        }
    }

    hidden = !desc->frame_bit(f1);

    if (!desc->is_billboard()) {
        if ((desc->vis_flags & VFX_MESH_UV_ANIM) && !(desc->mesh_flags & VFX_MESH_STATIC_UVS)) {
            float u = 1.0f - frac;
            for (i = 0; i < desc->num_faces; i++) {
                for (j = 0; j < 3; j++) {
                    vfx_mesh_face_uvs[i].set_u(j, desc->uv_anims[i].get_u(j, f0) * u
                                                  + desc->uv_anims[i].get_u(j, f1) * frac);
                    vfx_mesh_face_uvs[i].set_v(j, desc->uv_anims[i].get_v(j, f0) * u
                                                  + desc->uv_anims[i].get_v(j, f1) * frac);
                }
            }
        } else {
            for (i = 0; i < desc->num_faces; i++) {
                for (j = 0; j < 3; j++) {
                    vfx_mesh_face_uvs[i].set_u(j, desc->face_uvs[i].get_u(j));
                    vfx_mesh_face_uvs[i].set_v(j, desc->face_uvs[i].get_v(j));
                }
            }
        }
    }
    if (desc->vis_flags & VFX_MESH_LINE)
        v80 = desc->line_axis;

    if (placed) {
        if (!desc->is_billboard()) {
            for (i = 0; i < desc->num_verts; i++) {
                vfx_mesh_verts[i].unrotate(m2c);
                vfx_mesh_verts[i] += v20;
            }
        }
        v00.unrotate(m2c);
        v00 += v20;
        if (desc->vis_flags & VFX_MESH_LINE)
            v80.unrotate(m2c);
        return;
    }
    if (bound) {
        matrix43 bone = ci->compute_bone_matrix(tag);
        if (!desc->is_billboard()) {
            for (i = 0; i < desc->num_verts; i++)
                vfx_mesh_verts[i] = bone.transform(vfx_mesh_verts[i]);
        }
        v00 = bone.transform(v00);
        if (desc->vis_flags & VFX_MESH_LINE) {
            matrix r;
            bone.get_rotation(&r);
            v80.unrotate(r);
        }
    }
}

// 0x5645b0  Confidence: exact. The header (name, parent against header_base; base), the record
// pointers, the glow bitmap name; then the records. glow_bm is set to -1 and later to 0 (load_bitmap
// sets it when the mesh has a glow).
void vfx_node_desc::relocate(char *header_base, char *base)
{
    unsigned int i;

    ((BitArray *)&frames_data)->relocate(base);
    ((vfx_node_header *)this)->relocate(header_base, base);
    ptr_relocate(&faces, base);
    ptr_relocate(&vnormals, base);
    ptr_relocate(&materials, base);
    ptr_relocate(&morph, base);
    ptr_relocate(&anim, base);
    ptr_relocate(&uv_anims, base);
    ptr_relocate(&vertices, base);
    ptr_relocate(&sprite, base);
    ptr_relocate(&face_uvs, base);
    ptr_relocate(&track, base);
    ptr_relocate(&xform, base);
    reloc_ptr(&glow_name, header_base);
    glow_bm = -1;
    if (uv_anims != NULL) {
        for (i = 0; i < num_faces; i++)
            uv_anims[i].relocate(base);
    }
    if (vnormals != NULL) {
        for (i = 0; i < num_vnormals; i++)
            vnormals[i].relocate(base);
    }
    if (anim)
        anim->relocate(base);
    if (morph)
        morph->relocate(num_verts, base);
    if (vertices)
        vertices->relocate(base);
    if ((mesh_flags & VFX_MESH_SKINNED) && track)
        track->relocate(base);
    if (xform)
        xform->relocate(base);
    glow_bm = 0;
}

// 0x564720  Confidence: exact. A handle of 0 is taken as a failure and loaded again (without the flags).
void vfx_node_desc::load_bitmap(int flags)
{
    if (glow_name) {
        glow_bm = bm_load(glow_name, -1, flags);
        if (glow_bm == 0)
            glow_bm = bm_load(glow_name, -1, -1);
    }
}

// COMDATs (inline members of vsdk/geom/vfx.h and vsdk/geom/effect_mem.h), exact unless noted:
// 0x564770 vfx_mesh_xform::relocate, 0x5647a0 vfx_mesh_morph::relocate, 0x5647e0 vfx_mesh_anim::relocate,
// 0x564810 vfx_face_uv_anim::relocate, 0x564840 vfx_mesh_vertices::relocate (vfx_ptr0_relocate), 0x564860
// vfx_node_desc::is_billboard, 0x564870 / 0x564890 vfx_face_uv::get_u / get_v, 0x5648b0 float_round_to_int,
// 0x5648c0 vfx_vnormal::unpack_normal, 0x564950 vfx_vnormal::pack_normal, 0x564a20 vfx_face::get_normal
// (vfx_unpack_vector_s8), 0x564aa0 / 0x564ac0 vfx_face_uv::set_u / set_v, 0x564ae0 vfx_mesh_xform::get_pos
// (likely: the conditional), 0x564b40 get_scale, 0x564b60 get_rot, 0x564b80 vfx_mesh_morph::get_pos,
// 0x564bb0 vfx_mesh_vertices::get_vertex, 0x564c00 / 0x564c20 / 0x564c40 vfx_mesh_anim::get_a / get_b /
// get_vec, 0x564c60 / 0x564c70 vfx_mesh_sprite::get_w / get_h (folded with fx_rec28::get_age /
// get_lifetime), 0x564c80 vfx_mesh_sprite::get_pos (vfx_get_fixed8_vector_4), 0x564cc0 / 0x564ce0
// vfx_face_uv_anim::get_u / get_v, 0x564d00 vfx_node_desc::frame_bit, 0x564d20 has_flag4 (likely: the bit
// read), 0x564d30 vfx_vnormal::relocate (vfx_ptr4_relocate), 0x564d50 ptr_relocate.
