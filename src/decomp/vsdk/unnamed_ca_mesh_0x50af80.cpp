// unnamed_ca_mesh_0x50af80.cpp - Sum.exe v1.40, code 0x50af80-0x50d9ab: the character mesh renderer
// (vsdk\ca in sum.symbols): the skinning and morph work buffers, ca_render_mesh and its Glide, Direct3D
// and OpenGL back ends, which skin, light, project and draw one .vim mesh of an animated character.
// The file name is a placeholder: no path string, no strings.tbl block, no Summoner 2 counterpart.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range. Nine static initialisers open the object (table 0x588880-0x5888a0, after console.cpp's),
// the regular functions run to ca_render_stub_ret (0x50d7d0), and the inline functions the unit calls
// close it as COMDATs (0x50d7e0-0x50d9ab: vector3::cross, the five fog / depth helpers, float_max,
// clamp_float; D4500). The 3D view object follows with its initialisers at 0x50d9b0 (table 0x5888a8).
// .data 0x5a7280-0x5a7293 (gr_reflection_map_bm, "refmap01.tga"), .bss 0x25db930-0x2682997.
//
// Build flags: like the other vsdk objects, no inline expansion (the vector3 operators, cross,
// normalize, unrotate and the fog helpers are called), but the bone transforms, the view rotation and
// the projection are written out per component.
//
// Vertex layout of a sub-mesh: see the comment in vsdk/ca/unnamed_ca_mesh_0x50af80.h. The renderers
// write the skinned model-space position to ca_model_verts (for lighting and the back-face test), the
// projected position to ca_skin_positions (x, y on screen, z = 1 / view z) and the lit colour to
// ca_vertex_rgb / ca_gl_mesh_colors; ca_vertex_done marks the vertices that projected.

#include <string.h>
#include <math.h>
#include "vsdk/ca/character_instance.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/math/bezier.h"
#include "vsdk/geom/vfx.h"
#include "vsdk/gr/glide/gr_glide.h"
#include "vsdk/gr/directx/gr_direct3d.h"
#include "vsdk/gr/opengl/gr_opengl.h"

// ---- .data (0x5a7280-0x5a7293) ------------------------------------------------------------------------
int gr_reflection_map_bm = -1;                        // 0x5a7280 (the literal 0x5a7284 follows)

// ---- the vector3 buffers (dynamic initialisers, table 0x588880-0x5888a0) -----------------------------
// Each initialiser is two compiler-generated functions: the table entry (`jmp` to the next one) and the
// body (`vector constructor iterator` with vector3's empty constructor 0x505530). One sum.symbols entry
// covers both.
// 0x50af80 init_ca_vertex_buf_266f108 (body 0x50af90)  Confidence: exact.
vector3 ca_morph_vertices[CA_MAX_MESH_VERTS];          // 0x266f108
// 0x50afb0 init_ca_vertex_buf_25e74b0 (body 0x50afc0)  Confidence: exact.
vector3 ca_skin_positions[CA_MAX_MESH_VERTS];          // 0x25e74b0 skinned, then projected (sx, sy, 1/z)
// 0x50afe0 init_ca_vertex_buf_2622e48 (body 0x50aff0)  Confidence: exact.
vector3 ca_skin_normals[CA_MAX_MESH_VERTS];            // 0x2622e48
// 0x50b010 init_ca_vertex_buf_2605988 (body 0x50b020)  Confidence: exact.
vector3 ca_vertex_buf_2605988[CA_MAX_MESH_VERTS];      // 0x2605988 (label ours) no reference
// 0x50b040 init_ca_vertex_buf_260b748 (body 0x50b050)  Confidence: exact.
vector3 ca_model_verts[4000];                          // 0x260b748 (name ours) skinned model-space positions
// 0x50b070 init_ca_vertex_buf_25db930 (body 0x50b080)  Confidence: exact.
vector3 ca_vertex_buf_25db930[4000];                   // 0x25db930 (label ours) no reference
// 0x50b0a0 init_ca_vertex_buf_25ed2e8 (body 0x50b0b0)  Confidence: exact.
vector3 ca_vertex_buf_25ed2e8[4000];                   // 0x25ed2e8 (label ours) no reference
// 0x50b0d0 init_ca_vertex_buf_2628c08 (body 0x50b0e0)  Confidence: exact.
vector3 ca_vertex_buf_2628c08[4000];                   // 0x2628c08 (label ours) no reference
// 0x50b100 init_ca_vertex_buf_2675698 (body 0x50b110)  Confidence: exact.
vector3 ca_vertex_buf_2675698[4000];                   // 0x2675698 (label ours) no reference

// ---- .bss without initialisers (listed in address order; VC6 does not keep the definition order) -----
ubyte ca_bss_25f8e68[0x9c40];                         // 0x25f8e68 (label ours) no reference; size from the gap
ubyte ca_gl_mesh_colors[CA_MAX_MESH_VERTS][4];        // 0x2602aa8 r, g, b, a (glColorPointer)
ushort ca_mesh_indices[CA_MAX_MESH_VERTS];            // 0x26049e8 (name ours) Direct3D / OpenGL index list
ubyte ca_bss_26172c8[0x3e80];                         // 0x26172c8 (label ours) no reference; size from the gap
float ca_gl_mesh_texcoords[CA_MAX_MESH_VERTS][4];     // 0x261b148 (name ours) s, t, r, q (glTexCoordPointer)
GrVertex ca_glide_mesh_verts[4000];                   // 0x2634788
ubyte ca_vertex_done[CA_MAX_MESH_VERTS];              // 0x2674ec8
ubyte ca_vertex_rgb[CA_MAX_MESH_VERTS][3];            // 0x2681218 (name ours) the lit colour
vim *ca_render_current_mesh;                          // 0x2682988
ca_character *ca_render_character;                    // 0x268298c
character_instance *ca_render_instance;               // 0x2682990
int ca_render_flags;                                  // 0x2682994

// The eighth argument of lights_shade_vertices is not read (D4006); the renderers push the float 255.0.
#define CA_SHADE_UNUSED_255 0x437f0000

// 0x50b130  Confidence: exact. vsdk\gr in sum.symbols, but its data lies in this object's .data.
void gr_load_reflection_map()
{
    gr_reflection_map_bm = bm_load("refmap01.tga", -1, -1);
}

// 0x50b150  Confidence: likely (the key search: the cached key +8 first, then from key 1).
void ca_render_morph_vertices(vim_submesh *sm)
{
    int anim = ca_render_instance->action_anim;

    if (anim == -1)
        anim = ca_render_instance->state_anim;
    memcpy(ca_morph_vertices, sm->block0, sm->size0 * sizeof(vector3));

    skeleton *s = ca_render_character->skeletons[anim];
    for (int i = 0; i < s->num_morphs; i++) {
        int v = ((short *)sm->vertex_extra)[s->morph_ids[i]];
        if (v < 0)
            continue;
        skeleton_morph *m = &s->morphs[i];
        float frame = (float)(ca_render_instance->times[anim] - s->start) / (s->end - s->start)
                      * s->num_morph_frames;
        if ((int)frame >= s->num_morph_frames - 1) {
            anim_vec_key *last = &m->keys[m->num_keys - 1];
            ca_morph_vertices[v].x = last->v.x;
            ca_morph_vertices[v].y = last->v.y;
            ca_morph_vertices[v].z = last->v.z;
            continue;
        }

        int key;
        for (key = m->_unk08; key < m->num_keys; key++) {
            if (m->keys[key].time >= frame && m->keys[key - 1].time < frame)
                break;
        }
        if (key == m->num_keys) {
            for (key = 1; key <= m->_unk08 && m->keys[key].time < frame; key++)
                ;
        }
        m->_unk08 = key;

        anim_vec_key *k = &m->keys[key];
        float t = (k->time - frame) / (k->time - k[-1].time);
        float u = 1.0f - t;
        ca_morph_vertices[v].x = u * k->v.x + t * k[-1].v.x;
        ca_morph_vertices[v].y = u * k->v.y + t * k[-1].v.y;
        ca_morph_vertices[v].z = u * k->v.z + t * k[-1].v.z;
    }
}

// 0x50b380 init_ca_offset_table_25ed270 (body 0x50b390)  Confidence: exact. Ten points (x, 0, 0).
vector3 ca_ramp_points[10] = {                        // 0x25ed270 (name ours)
    vector3(0.0f, 0.0f, 0.0f),   vector3(0.0001f, 0.0f, 0.0f), vector3(0.005f, 0.0f, 0.0f),
    vector3(0.01f, 0.0f, 0.0f),  vector3(0.03f, 0.0f, 0.0f),   vector3(0.1f, 0.0f, 0.0f),
    vector3(0.15f, 0.0f, 0.0f),  vector3(0.2f, 0.0f, 0.0f),    vector3(0.375f, 0.0f, 0.0f),
    vector3(0.55f, 0.0f, 0.0f)
};

// 0x50b450  Confidence: exact.
int ca_ramp_curve_eval(float t)
{
    bezier curve;
    vector3 *points[10];

    for (int i = 0; i < 10; i++)
        points[i] = &ca_ramp_points[i];
    curve.set_points(10, points);
    vector3 out;
    curve.eval(&out, t);
    return (int)(out.x * 255.0f);
}

// 0x50b4c0  Confidence: unsure (the shape of the per-vertex code and the three triangle passes; the
// arithmetic, tests and stores follow the disassembly). This back end indexes mesh->submeshes[i] each
// time (no sub-mesh pointer is kept across the calls).
void gr_glide_render_character_mesh(vim *mesh, vector3 *pos, matrix *orient, gr_light_desc *light,
                                    int a5, int has_instance, int a7, int a8)
{
    if (glide_close_on_deactivate == 1 && glide_pending_deactivations == -1)
        return;
    if (mesh == NULL)
        return;

    int hide_mask = (int)light->model_prefix << 6;
    vector3 center(mesh->center);
    center.unrotate(*orient);
    center += *pos;
    gr_start_instance(pos, orient, 1);
    lights_select_nearest(-1, &center, mesh->radius);
    lights_transform_selection();
    float u_scale = 0.0f;
    float v_scale = 0.0f;
    gr_glide_set_mode(gr_mode_mesh_alpha);

    for (int i = 0; i < mesh->num_submeshes; i++) {
        vector3 *normals = (vector3 *)mesh->submeshes[i].block1;
        vector3 *verts = (vector3 *)mesh->submeshes[i].block0;
        if (mesh->submeshes[i].size1 == 0 || mesh->submeshes[i]._unk18 < 0)
            continue;
        if (!gr_glide_tcache_set(0, light->appearance ? ((int *)light->appearance)[mesh->submeshes[i]._unk18]
                                                      : mesh->textures[mesh->submeshes[i]._unk18],
                                 0, &u_scale, &v_scale, true))
            continue;
        if (has_instance) {
            int anim = ca_render_instance->action_anim > -1 ? ca_render_instance->action_anim
                                                            : ca_render_instance->state_anim;
            if (anim > -1 && anim < ca_render_character->num_anims
                && ca_render_character->skeletons[anim]->num_morphs != 0) {
                ca_render_morph_vertices(&mesh->submeshes[i]);
                verts = ca_morph_vertices;
            }
        }

        vector3 bone_pos;
        vector3 bone_nrm;
        float alpha = (float)light->_unk08;
        short *dup = (short *)mesh->submeshes[i].block4;
        int j;
        for (j = 0; j < mesh->submeshes[i].size0; j++) {
            if (dup[j] >= 0) {
                int d = dup[j];
                ca_vertex_done[j] = ca_vertex_done[j - d];
                if (!ca_vertex_done[j])
                    continue;
                memcpy(&ca_glide_mesh_verts[j], &ca_glide_mesh_verts[j - d], 9 * sizeof(float));
                ca_model_verts[j] = ca_model_verts[j - d];
            } else {
                ca_skin_normals[j].x = 0.0f;
                ca_skin_positions[j].x = 0.0f;
                ca_skin_positions[j].y = 0.0f;
                ca_skin_normals[j].y = 0.0f;
                ca_skin_positions[j].z = 0.0f;
                ca_skin_normals[j].z = 0.0f;
                if (ca_render_instance) {
                    matrix43 *skin = ca_render_instance->skin_matrices;
                    for (int k = 0; k < 4; k++) {
                        if (((ubyte *)mesh->submeshes[i].block5)[j * 8 + k] == 0)
                            continue;
                        float wt = (float)((ubyte *)mesh->submeshes[i].block5)[j * 8 + k];
                        matrix43 *m = &skin[((ubyte *)mesh->submeshes[i].block5)[j * 8 + k + 4]];
                        bone_pos.x = verts[j].x * m->m[0][0] + verts[j].y * m->m[1][0]
                                     + verts[j].z * m->m[2][0] + m->m[3][0];
                        bone_pos.y = verts[j].x * m->m[0][1] + verts[j].y * m->m[1][1]
                                     + verts[j].z * m->m[2][1] + m->m[3][1];
                        bone_pos.z = verts[j].x * m->m[0][2] + verts[j].y * m->m[1][2]
                                     + verts[j].z * m->m[2][2] + m->m[3][2];
                        // The normals get the bone translation too (D4505).
                        m = &skin[((ubyte *)mesh->submeshes[i].block5)[j * 8 + k + 4]];
                        bone_nrm.x = normals[j].x * m->m[0][0] + normals[j].y * m->m[1][0]
                                     + normals[j].z * m->m[2][0] + m->m[3][0];
                        bone_nrm.y = normals[j].x * m->m[0][1] + normals[j].y * m->m[1][1]
                                     + normals[j].z * m->m[2][1] + m->m[3][1];
                        bone_nrm.z = normals[j].x * m->m[0][2] + normals[j].y * m->m[1][2]
                                     + normals[j].z * m->m[2][2] + m->m[3][2];
                        ca_skin_positions[j].x += bone_pos.x * wt;
                        ca_skin_positions[j].y += bone_pos.y * wt;
                        ca_skin_positions[j].z += bone_pos.z * wt;
                        ca_skin_normals[j].x += bone_nrm.x * wt;
                        ca_skin_normals[j].y += bone_nrm.y * wt;
                        ca_skin_normals[j].z += bone_nrm.z * wt;
                    }
                    ca_skin_positions[j].x *= 0.0625f;
                    ca_skin_positions[j].y *= 0.0625f;
                    ca_skin_positions[j].z *= 0.0625f;
                    ca_skin_normals[j].x *= 0.0625f;
                    ca_skin_normals[j].y *= 0.0625f;
                    ca_skin_normals[j].z *= 0.0625f;
                } else {
                    ca_skin_positions[j].x = verts[j].x;
                    ca_skin_positions[j].y = verts[j].y;
                    ca_skin_positions[j].z = verts[j].z;
                    ca_skin_normals[j].x = normals[j].x;
                    ca_skin_normals[j].y = normals[j].y;
                    ca_skin_normals[j].z = normals[j].z;
                }

                ca_model_verts[j].x = ca_skin_positions[j].x;
                ca_model_verts[j].y = ca_skin_positions[j].y;
                ca_model_verts[j].z = ca_skin_positions[j].z;
                ca_vertex_done[j] = 1;
                ca_skin_positions[j].x -= gr_view_pos.x;
                ca_skin_positions[j].y -= gr_view_pos.y;
                ca_skin_positions[j].z -= gr_view_pos.z;
                float z = ca_skin_positions[j].z;
                float vy = gr_view_matrix.uvec.x * ca_skin_positions[j].x
                           + gr_view_matrix.uvec.y * ca_skin_positions[j].y + gr_view_matrix.uvec.z * z;
                float vz = gr_view_matrix.fvec.x * ca_skin_positions[j].x
                           + gr_view_matrix.fvec.y * ca_skin_positions[j].y + gr_view_matrix.fvec.z * z;
                ca_skin_positions[j].x = gr_view_matrix.rvec.x * ca_skin_positions[j].x
                                         + gr_view_matrix.rvec.y * ca_skin_positions[j].y
                                         + gr_view_matrix.rvec.z * z;
                ca_skin_positions[j].y = vy;
                ca_skin_positions[j].z = 1.0f / vz;
                ca_skin_positions[j].x = (ca_skin_positions[j].z * ca_skin_positions[j].x + 1.0f)
                                         * gr_projection_xmul + gr_screen.offset_x;
                ca_skin_positions[j].y = (1.0f - ca_skin_positions[j].z * ca_skin_positions[j].y)
                                         * gr_projection_ymul + gr_screen.offset_y;
                if (!(fabs(ca_skin_positions[j].x) < 1224.0f && fabs(ca_skin_positions[j].y) < 1000.0f
                      && ca_skin_positions[j].z >= 0.0f)) {
                    ca_vertex_done[j] = 0;
                    continue;
                }

                if (ca_render_instance)
                    ca_skin_normals[j].normalize();
                lights_shade_vertices(&ca_vertex_rgb[j][0], &ca_vertex_rgb[j][1], &ca_vertex_rgb[j][2],
                                      &ca_model_verts[j], &ca_skin_normals[j], true, false,
                                      CA_SHADE_UNUSED_255);
                if (gr_tint_enabled == 1) {
                    float grey = (ca_vertex_rgb[j][0] + ca_vertex_rgb[j][1] + ca_vertex_rgb[j][2]) * 0.75f;
                    clamp_float(&grey, 0.0f, 255.0f);
                    ca_vertex_rgb[j][0] = (ubyte)(int)(gr_tint_r * grey);
                    ca_vertex_rgb[j][1] = (ubyte)(int)(gr_tint_g * grey);
                    ca_vertex_rgb[j][2] = (ubyte)(int)(gr_tint_b * grey);
                }
                ca_glide_mesh_verts[j].x = ca_skin_positions[j].x + 786432.0f;
                ca_glide_mesh_verts[j].y = ca_skin_positions[j].y + 786432.0f;
                ca_glide_mesh_verts[j].r = ca_vertex_rgb[j][0];
                ca_glide_mesh_verts[j].g = ca_vertex_rgb[j][1];
                ca_glide_mesh_verts[j].b = ca_vertex_rgb[j][2];
                ca_glide_mesh_verts[j].oow = ca_skin_positions[j].z;
                ca_glide_mesh_verts[j].a = alpha;
            }
            float *uv = (float *)mesh->submeshes[i].block2 + j * 2;
            ca_glide_mesh_verts[j].tmuvtx[0].sow = ca_glide_mesh_verts[j].oow * uv[0] * u_scale;
            ca_glide_mesh_verts[j].tmuvtx[0].tow = uv[1] * ca_glide_mesh_verts[j].oow * v_scale;
            if (vfx_render_bitmap >= 0)
                ca_glide_mesh_verts[j].tmuvtx[1] = ca_glide_mesh_verts[j].tmuvtx[0];
        }

        int t;
        for (t = 0; t < mesh->submeshes[i].size1; t++) {
            ca_mesh_tri *tri = &((ca_mesh_tri *)mesh->submeshes[i].block3)[t];
            if (hide_mask & tri->flags)
                continue;
            if (!ca_vertex_done[tri->v[0]] || !ca_vertex_done[tri->v[1]] || !ca_vertex_done[tri->v[2]])
                continue;
            if (!(tri->flags & CA_TRI_TWO_SIDED)) {
                vector3 n = (ca_model_verts[tri->v[1]] - ca_model_verts[tri->v[0]])
                                .cross(ca_model_verts[tri->v[2]] - ca_model_verts[tri->v[1]]);
                if (!gr_check_normal_facing(&n, &ca_model_verts[tri->v[0]]))
                    continue;
            }
            glide_grDrawPlanarPolygon(3, tri->v, ca_glide_mesh_verts);
        }

        if (gr_tint_enabled) {
            gr_glide_set_mode(gr_mode_particle_additive);
            for (t = 0; t < mesh->submeshes[i].size1; t++) {
                ca_mesh_tri *tri = &((ca_mesh_tri *)mesh->submeshes[i].block3)[t];
                if (hide_mask & tri->flags)
                    continue;
                if (!ca_vertex_done[tri->v[0]] || !ca_vertex_done[tri->v[1]] || !ca_vertex_done[tri->v[2]])
                    continue;
                if (!(tri->flags & CA_TRI_TWO_SIDED)) {
                    vector3 n = (ca_model_verts[tri->v[1]] - ca_model_verts[tri->v[0]])
                                    .cross(ca_model_verts[tri->v[2]] - ca_model_verts[tri->v[1]]);
                    if (!gr_check_normal_facing(&n, &ca_model_verts[tri->v[0]]))
                        continue;
                }
                glide_grDrawPlanarPolygon(3, tri->v, ca_glide_mesh_verts);
            }
            gr_glide_set_mode(gr_mode_mesh_alpha);
        }

        if (vfx_render_bitmap >= 0 && gr_glide_tcache_set(0, vfx_render_bitmap, 0, &u_scale, &v_scale, true)) {
            int n = mesh->submeshes[i].size0;
            if (n > 0) {
                float r = (float)light->tint[0];
                float g = (float)light->tint[1];
                float b = (float)light->tint[2];
                float a = vfx_render_alpha * (1.0f / 255.0f);
                for (j = 0; j < n; j++) {
                    ca_glide_mesh_verts[j].r = r;
                    ca_glide_mesh_verts[j].g = g;
                    ca_glide_mesh_verts[j].b = b;
                    ca_glide_mesh_verts[j].a *= a;
                }
            }
            for (t = 0; t < mesh->submeshes[i].size1; t++) {
                ca_mesh_tri *tri = &((ca_mesh_tri *)mesh->submeshes[i].block3)[t];
                if (hide_mask & tri->flags)
                    continue;
                if (!ca_vertex_done[tri->v[0]] || !ca_vertex_done[tri->v[1]] || !ca_vertex_done[tri->v[2]])
                    continue;
                if (!(tri->flags & CA_TRI_TWO_SIDED)) {
                    vector3 n = (ca_model_verts[tri->v[1]] - ca_model_verts[tri->v[0]])
                                    .cross(ca_model_verts[tri->v[2]] - ca_model_verts[tri->v[1]]);
                    if (!gr_check_normal_facing(&n, &ca_model_verts[tri->v[0]]))
                        continue;
                }
                glide_grDrawPlanarPolygon(3, tri->v, ca_glide_mesh_verts);
            }
        }
    }
    lights_pop_selection();
    gr_stop_instance();
}

// 0x50c130  Confidence: unsure (as the Glide back end; vertex buffer handling and the index list exact).
void gr_d3d_render_character_mesh(vim *mesh, vector3 *pos, matrix *orient, gr_light_desc *light,
                                  int a5, int has_instance, int a7, int a8)
{
    if (mesh == NULL)
        return;

    int hide_mask = (int)light->model_prefix << 6;
    vector3 center(mesh->center);
    center.unrotate(*orient);
    center += *pos;
    gr_start_instance(pos, orient, 1);
    lights_select_nearest(-1, &center, mesh->radius);
    lights_transform_selection();
    gr_d3d_set_state(gr_mode_mesh_alpha);
    int vb_pos = d3d_vertex_buffer_pos;
    d3d_tlvertex *vb_data = NULL;
    float u_scale, v_scale;

    for (int i = 0; i < mesh->num_submeshes; i++) {
        vim_submesh *sm = &mesh->submeshes[i];
        if (sm->_unk18 < 0 || sm->size1 == 0)
            continue;
        if (!gr_d3d_tcache_set(0, light->appearance ? ((int *)light->appearance)[sm->_unk18]
                                                    : mesh->textures[sm->_unk18],
                               0, &u_scale, &v_scale, 1))
            continue;
        float *uv = (float *)sm->block2;
        vector3 *verts = (vector3 *)sm->block0;
        vector3 *normals = (vector3 *)sm->block1;
        if (has_instance) {
            int anim = ca_render_instance->action_anim > -1 ? ca_render_instance->action_anim
                                                            : ca_render_instance->state_anim;
            if (anim > -1 && anim < ca_render_character->num_anims
                && ca_render_character->skeletons[anim]->num_morphs != 0) {
                ca_render_morph_vertices(sm);
                verts = ca_morph_vertices;
            }
        }

        DWORD lock_flags;
        if (sm->size0 + vb_pos > 1000) {
            lock_flags = gr_d3d_lock_discard;
            vb_pos = 0;
        } else if (vb_pos == 0) {
            lock_flags = gr_d3d_lock_discard;
        } else {
            lock_flags = gr_d3d_lock_nooverwrite;
        }
        IDirect3DVertexBuffer7_Lock(d3d_vertex_buffer, lock_flags, (void **)&vb_data, NULL);

        vector3 bone_pos;
        vector3 bone_nrm;
        short *dup = (short *)sm->block4;
        d3d_tlvertex *v = vb_data + vb_pos;
        int j;
        for (j = 0; j < sm->size0; j++, v++, uv += 2) {
            if (dup[j] >= 0) {
                int d = dup[j];
                ca_vertex_done[j] = ca_vertex_done[j - d];
                if (!ca_vertex_done[j])
                    continue;
                // The fog alpha of a seam copy comes from the normal's z, not the depth (D4505).
                v->specular_bytes[3] = (ubyte)gr_get_fog_alpha(ca_skin_normals[j - d].z);
                v->sx = ca_skin_positions[j - d].x;
                v->sy = ca_skin_positions[j - d].y;
                v->sz = gr_calc_depth(ca_skin_positions[j - d].z);
                v->rhw = ca_skin_positions[j - d].z;
                ca_model_verts[j] = ca_model_verts[j - d];
                v->color = RGBA_MAKE(ca_vertex_rgb[j - d][0], ca_vertex_rgb[j - d][1], ca_vertex_rgb[j - d][2],
                                     light->_unk08);
                v->tu = uv[0];
                v->tv = uv[1];
                continue;
            }

            if (ca_render_instance) {
                matrix43 *skin = ca_render_instance->skin_matrices;
                ca_skin_positions[j].x = 0.0f;
                ca_skin_normals[j].x = 0.0f;
                ca_skin_positions[j].y = 0.0f;
                ca_skin_normals[j].y = 0.0f;
                ca_skin_positions[j].z = 0.0f;
                ca_skin_normals[j].z = 0.0f;
                for (int k = 0; k < 4; k++) {
                    if (((ubyte *)sm->block5)[j * 8 + k] == 0)
                        continue;
                    float wt = (float)((ubyte *)sm->block5)[j * 8 + k];
                    matrix43 *m = &skin[((ubyte *)sm->block5)[j * 8 + k + 4]];
                    bone_pos.x = verts[j].x * m->m[0][0] + verts[j].y * m->m[1][0] + verts[j].z * m->m[2][0]
                                 + m->m[3][0];
                    bone_pos.y = verts[j].x * m->m[0][1] + verts[j].y * m->m[1][1] + verts[j].z * m->m[2][1]
                                 + m->m[3][1];
                    bone_pos.z = verts[j].x * m->m[0][2] + verts[j].y * m->m[1][2] + verts[j].z * m->m[2][2]
                                 + m->m[3][2];
                    m = &skin[((ubyte *)sm->block5)[j * 8 + k + 4]];
                    bone_nrm.x = normals[j].x * m->m[0][0] + normals[j].y * m->m[1][0]
                                 + normals[j].z * m->m[2][0] + m->m[3][0];
                    bone_nrm.y = normals[j].x * m->m[0][1] + normals[j].y * m->m[1][1]
                                 + normals[j].z * m->m[2][1] + m->m[3][1];
                    bone_nrm.z = normals[j].x * m->m[0][2] + normals[j].y * m->m[1][2]
                                 + normals[j].z * m->m[2][2] + m->m[3][2];
                    ca_skin_positions[j].x += bone_pos.x * wt;
                    ca_skin_positions[j].y += bone_pos.y * wt;
                    ca_skin_positions[j].z += bone_pos.z * wt;
                    ca_skin_normals[j].x += bone_nrm.x * wt;
                    ca_skin_normals[j].y += bone_nrm.y * wt;
                    ca_skin_normals[j].z += bone_nrm.z * wt;
                }
                ca_skin_positions[j].x *= 0.0625f;
                ca_skin_positions[j].y *= 0.0625f;
                ca_skin_positions[j].z *= 0.0625f;
                ca_skin_normals[j].x *= 0.0625f;
                ca_skin_normals[j].y *= 0.0625f;
                ca_skin_normals[j].z *= 0.0625f;
            } else {
                ca_skin_positions[j].x = verts[j].x;
                ca_skin_positions[j].y = verts[j].y;
                ca_skin_positions[j].z = verts[j].z;
                ca_skin_normals[j].x = normals[j].x;
                ca_skin_normals[j].y = normals[j].y;
                ca_skin_normals[j].z = normals[j].z;
            }

            ca_model_verts[j].x = ca_skin_positions[j].x;
            ca_model_verts[j].y = ca_skin_positions[j].y;
            ca_model_verts[j].z = ca_skin_positions[j].z;
            ca_skin_positions[j].x -= gr_view_pos.x;
            ca_skin_positions[j].y -= gr_view_pos.y;
            ca_skin_positions[j].z -= gr_view_pos.z;
            float z = ca_skin_positions[j].z;
            float vy = gr_view_matrix.uvec.x * ca_skin_positions[j].x
                       + gr_view_matrix.uvec.y * ca_skin_positions[j].y + gr_view_matrix.uvec.z * z;
            float vz = gr_view_matrix.fvec.y * ca_skin_positions[j].y + gr_view_matrix.fvec.z * z
                       + gr_view_matrix.fvec.x * ca_skin_positions[j].x;
            ca_skin_positions[j].x = gr_view_matrix.rvec.x * ca_skin_positions[j].x
                                     + gr_view_matrix.rvec.y * ca_skin_positions[j].y + gr_view_matrix.rvec.z * z;
            ca_skin_positions[j].y = vy;
            ca_skin_positions[j].z = vz;
            v->specular_bytes[3] = (ubyte)gr_get_fog_alpha(ca_skin_positions[j].z);
            ca_skin_positions[j].z = 1.0f / ca_skin_positions[j].z;
            ca_skin_positions[j].x = (ca_skin_positions[j].z * ca_skin_positions[j].x + 1.0f)
                                     * gr_projection_xmul + gr_screen.offset_x;
            ca_skin_positions[j].y = (1.0f - ca_skin_positions[j].z * ca_skin_positions[j].y)
                                     * gr_projection_ymul + gr_screen.offset_y;
            ca_vertex_done[j] = 1;
            if (!(fabs(ca_skin_positions[j].x) < 2048.0f && fabs(ca_skin_positions[j].y) < 2048.0f
                  && ca_skin_positions[j].z >= 0.0f)) {
                ca_vertex_done[j] = 0;
                continue;
            }

            if (ca_render_instance)
                ca_skin_normals[j].normalize();
            lights_shade_vertices(&ca_vertex_rgb[j][0], &ca_vertex_rgb[j][1], &ca_vertex_rgb[j][2],
                                  &ca_model_verts[j], &ca_skin_normals[j], true, false, CA_SHADE_UNUSED_255);
            if (gr_tint_enabled == 1) {
                float grey = (ca_vertex_rgb[j][0] + ca_vertex_rgb[j][1] + ca_vertex_rgb[j][2]) * 0.75f;
                clamp_float(&grey, 0.0f, 255.0f);
                ca_vertex_rgb[j][0] = (ubyte)(int)(gr_tint_r * grey);
                ca_vertex_rgb[j][1] = (ubyte)(int)(gr_tint_g * grey);
                ca_vertex_rgb[j][2] = (ubyte)(int)(gr_tint_b * grey);
            }
            v->sx = ca_skin_positions[j].x;
            v->sy = ca_skin_positions[j].y;
            v->sz = gr_calc_depth(ca_skin_positions[j].z);
            v->rhw = ca_skin_positions[j].z;
            v->color = RGBA_MAKE(ca_vertex_rgb[j][0], ca_vertex_rgb[j][1], ca_vertex_rgb[j][2], light->_unk08);
            v->tu = uv[0];
            v->tv = uv[1];
        }
        IDirect3DVertexBuffer7_Unlock(d3d_vertex_buffer);

        int num_indices = 0;
        for (int t = 0; t < sm->size1; t++) {
            ca_mesh_tri *tri = &((ca_mesh_tri *)sm->block3)[t];
            if (hide_mask & tri->flags)
                continue;
            if (!ca_vertex_done[tri->v[0]] || !ca_vertex_done[tri->v[1]] || !ca_vertex_done[tri->v[2]])
                continue;
            if (!(tri->flags & CA_TRI_TWO_SIDED)) {
                vector3 n = (ca_model_verts[tri->v[1]] - ca_model_verts[tri->v[0]])
                                .cross(ca_model_verts[tri->v[2]] - ca_model_verts[tri->v[1]]);
                if (!gr_check_normal_facing(&n, &ca_model_verts[tri->v[0]]))
                    continue;
            }
            ca_mesh_indices[num_indices] = (ushort)tri->v[0];
            ca_mesh_indices[num_indices + 1] = (ushort)tri->v[1];
            ca_mesh_indices[num_indices + 2] = (ushort)tri->v[2];
            num_indices += 3;
        }

        if (num_indices != 0) {
            IDirect3DDevice7_DrawIndexedPrimitiveVB(d3d_device, D3DPT_TRIANGLELIST, d3d_vertex_buffer, vb_pos,
                                                    sm->size0, ca_mesh_indices, num_indices, 0);
            if (gr_tint_enabled) {
                gr_d3d_set_state(gr_mode_particle_additive);
                IDirect3DDevice7_DrawIndexedPrimitiveVB(d3d_device, D3DPT_TRIANGLELIST, d3d_vertex_buffer,
                                                        vb_pos, sm->size0, ca_mesh_indices, num_indices, 0);
                gr_d3d_set_state(gr_mode_mesh_alpha);
            }
            if (vfx_render_bitmap >= 0) {
                gr_d3d_tcache_set(0, vfx_render_bitmap, 0, &u_scale, &v_scale, 1);
                IDirect3DVertexBuffer7_Lock(d3d_vertex_buffer, DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_NOSYSLOCK,
                                            (void **)&vb_data, NULL);
                D3DCOLOR c = RGBA_MAKE(light->tint[0], light->tint[1], light->tint[2], vfx_render_alpha);
                int n = sm->size0;
                for (j = 0; j < n; j++)
                    vb_data[vb_pos + j].color = c;
                IDirect3DVertexBuffer7_Unlock(d3d_vertex_buffer);
                IDirect3DDevice7_DrawIndexedPrimitiveVB(d3d_device, D3DPT_TRIANGLELIST, d3d_vertex_buffer,
                                                        vb_pos, sm->size0, ca_mesh_indices, num_indices, 0);
            }
        }
        vb_pos += sm->size0;
    }
    d3d_vertex_buffer_pos = vb_pos;
    lights_pop_selection();
    gr_stop_instance();
}

// 0x50cd00  Confidence: unsure (as the Glide back end). No screen bounds test (only w), no additive tint
// pass; the vertices go to the card as arrays (positions, colours, s / t / r / q with q = 1 / z).
void gr_opengl_render_character_mesh(vim *mesh, vector3 *pos, matrix *orient, gr_light_desc *light,
                                     int a5, int has_instance, int a7, int a8)
{
    if (mesh == NULL)
        return;

    int hide_mask = (int)light->model_prefix << 6;
    vector3 center(mesh->center);
    center.unrotate(*orient);
    center += *pos;
    gr_start_instance(pos, orient, 1);
    lights_select_nearest(-1, &center, mesh->radius);
    lights_transform_selection();
    gl_active_texture_arb(GL_TEXTURE0_ARB);
    gl_client_active_texture_arb(GL_TEXTURE0_ARB);
    glTexCoordPointer(4, GL_FLOAT, 0, ca_gl_mesh_texcoords);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, ca_gl_mesh_colors);
    glVertexPointer(3, GL_FLOAT, 0, ca_skin_positions);

    for (int i = 0; i < mesh->num_submeshes; i++) {
        vim_submesh *sm = &mesh->submeshes[i];
        if (sm->size1 == 0 || sm->_unk18 < 0)
            continue;
        gr_opengl_set_bitmap(gr_mode_mesh_alpha, light->appearance ? ((int *)light->appearance)[sm->_unk18]
                                                                   : mesh->textures[sm->_unk18], 0);
        vector3 *verts = (vector3 *)sm->block0;
        vector3 *normals = (vector3 *)sm->block1;
        if (has_instance) {
            int anim = ca_render_instance->action_anim > -1 ? ca_render_instance->action_anim
                                                            : ca_render_instance->state_anim;
            if (anim > -1 && anim < ca_render_character->num_anims
                && ca_render_character->skeletons[anim]->num_morphs != 0) {
                ca_render_morph_vertices(&mesh->submeshes[i]);
                verts = ca_morph_vertices;
            }
        }

        vector3 bone_pos;
        vector3 bone_nrm;
        float *uv = (float *)sm->block2;
        short *dup = (short *)sm->block4;
        int j;
        for (j = 0; j < sm->size0; j++) {
            float w;
            if (dup[j] >= 0) {
                int d = dup[j];
                ca_vertex_done[j] = ca_vertex_done[j - d];
                if (!ca_vertex_done[j])
                    continue;
                ca_model_verts[j] = ca_model_verts[j - d];
                ca_skin_positions[j] = ca_skin_positions[j - d];
                ca_gl_mesh_colors[j][3] = (ubyte)light->_unk08;
                ca_gl_mesh_colors[j][0] = ca_gl_mesh_colors[j - d][0];
                ca_gl_mesh_colors[j][1] = ca_gl_mesh_colors[j - d][1];
                ca_gl_mesh_colors[j][2] = ca_gl_mesh_colors[j - d][2];
                w = ca_gl_mesh_texcoords[j - d][3];
            } else {
                if (ca_render_instance) {
                    matrix43 *skin = ca_render_instance->skin_matrices;
                    ca_skin_normals[j].x = 0.0f;
                    ca_skin_normals[j].y = 0.0f;
                    ca_skin_positions[j].x = 0.0f;
                    ca_skin_positions[j].y = 0.0f;
                    ca_skin_positions[j].z = 0.0f;
                    ca_skin_normals[j].z = 0.0f;
                    for (int k = 0; k < 4; k++) {
                        if (((ubyte *)sm->block5)[j * 8 + k] == 0)
                            continue;
                        float wt = (float)((ubyte *)sm->block5)[j * 8 + k];
                        matrix43 *m = &skin[((ubyte *)sm->block5)[j * 8 + k + 4]];
                        bone_pos.x = verts[j].x * m->m[0][0] + verts[j].y * m->m[1][0]
                                     + verts[j].z * m->m[2][0] + m->m[3][0];
                        bone_pos.y = verts[j].x * m->m[0][1] + verts[j].y * m->m[1][1]
                                     + verts[j].z * m->m[2][1] + m->m[3][1];
                        bone_pos.z = verts[j].x * m->m[0][2] + verts[j].y * m->m[1][2]
                                     + verts[j].z * m->m[2][2] + m->m[3][2];
                        m = &skin[((ubyte *)sm->block5)[j * 8 + k + 4]];
                        bone_nrm.x = normals[j].x * m->m[0][0] + normals[j].y * m->m[1][0]
                                     + normals[j].z * m->m[2][0] + m->m[3][0];
                        bone_nrm.y = normals[j].x * m->m[0][1] + normals[j].y * m->m[1][1]
                                     + normals[j].z * m->m[2][1] + m->m[3][1];
                        bone_nrm.z = normals[j].x * m->m[0][2] + normals[j].y * m->m[1][2]
                                     + normals[j].z * m->m[2][2] + m->m[3][2];
                        ca_skin_positions[j].x += bone_pos.x * wt;
                        ca_skin_positions[j].y += bone_pos.y * wt;
                        ca_skin_positions[j].z += bone_pos.z * wt;
                        ca_skin_normals[j].x += bone_nrm.x * wt;
                        ca_skin_normals[j].y += bone_nrm.y * wt;
                        ca_skin_normals[j].z += bone_nrm.z * wt;
                    }
                    ca_skin_positions[j].x *= 0.0625f;
                    ca_skin_positions[j].y *= 0.0625f;
                    ca_skin_positions[j].z *= 0.0625f;
                    ca_skin_normals[j].x *= 0.0625f;
                    ca_skin_normals[j].y *= 0.0625f;
                    ca_skin_normals[j].z *= 0.0625f;
                } else {
                    ca_skin_positions[j].x = verts[j].x;
                    ca_skin_positions[j].y = verts[j].y;
                    ca_skin_positions[j].z = verts[j].z;
                    ca_skin_normals[j].x = normals[j].x;
                    ca_skin_normals[j].y = normals[j].y;
                    ca_skin_normals[j].z = normals[j].z;
                }

                ca_model_verts[j].x = ca_skin_positions[j].x;
                ca_model_verts[j].y = ca_skin_positions[j].y;
                ca_model_verts[j].z = ca_skin_positions[j].z;
                ca_vertex_done[j] = 1;
                ca_skin_positions[j].x -= gr_view_pos.x;
                ca_skin_positions[j].y -= gr_view_pos.y;
                ca_skin_positions[j].z -= gr_view_pos.z;
                float z = ca_skin_positions[j].z;
                float vy = gr_view_matrix.uvec.y * ca_skin_positions[j].y + gr_view_matrix.uvec.z * z
                           + gr_view_matrix.uvec.x * ca_skin_positions[j].x;
                float vz = gr_view_matrix.fvec.z * z + gr_view_matrix.fvec.x * ca_skin_positions[j].x
                           + gr_view_matrix.fvec.y * ca_skin_positions[j].y;
                ca_skin_positions[j].x = gr_view_matrix.rvec.y * ca_skin_positions[j].y
                                         + gr_view_matrix.rvec.z * z
                                         + gr_view_matrix.rvec.x * ca_skin_positions[j].x;
                ca_skin_positions[j].y = vy;
                ca_skin_positions[j].z = 1.0f / vz;
                ca_skin_positions[j].x = (ca_skin_positions[j].z * ca_skin_positions[j].x + 1.0f)
                                         * gr_projection_xmul + gr_screen.offset_x;
                ca_skin_positions[j].y = (1.0f - ca_skin_positions[j].y * ca_skin_positions[j].z)
                                         * gr_projection_ymul + gr_screen.offset_y;
                if (ca_skin_positions[j].z < 0.0f) {
                    ca_vertex_done[j] = 0;
                    continue;
                }

                if (ca_render_instance)
                    ca_skin_normals[j].normalize();
                lights_shade_vertices(&ca_gl_mesh_colors[j][0], &ca_gl_mesh_colors[j][1],
                                      &ca_gl_mesh_colors[j][2], &ca_model_verts[j], &ca_skin_normals[j], true,
                                      false, CA_SHADE_UNUSED_255);
                ca_gl_mesh_colors[j][3] = (ubyte)light->_unk08;
                if (gr_tint_enabled == 1) {
                    float grey = (ca_gl_mesh_colors[j][0] + ca_gl_mesh_colors[j][1] + ca_gl_mesh_colors[j][2])
                                 * 0.75f;
                    clamp_float(&grey, 0.0f, 255.0f);
                    ca_gl_mesh_colors[j][0] = (ubyte)(int)(gr_tint_r * grey);
                    ca_gl_mesh_colors[j][1] = (ubyte)(int)(gr_tint_g * grey);
                    ca_gl_mesh_colors[j][2] = (ubyte)(int)(gr_tint_b * grey);
                }
                w = ca_skin_positions[j].z;
                ca_skin_positions[j].z = 1.0f / w;
                ca_skin_positions[j].z = gr_vertex_depth((gr_vertex *)&ca_skin_positions[j]);
            }
            ca_gl_mesh_texcoords[j][2] = 0.0f;
            ca_gl_mesh_texcoords[j][3] = w;
            ca_gl_mesh_texcoords[j][0] = w * uv[j * 2];
            ca_gl_mesh_texcoords[j][1] = w * uv[j * 2 + 1];
        }

        int num_tris = 0;
        for (int t = 0; t < sm->size1; t++) {
            ca_mesh_tri *tri = &((ca_mesh_tri *)sm->block3)[t];
            if (hide_mask & tri->flags)
                continue;
            if (!ca_vertex_done[tri->v[0]] || !ca_vertex_done[tri->v[1]] || !ca_vertex_done[tri->v[2]])
                continue;
            if (!(tri->flags & CA_TRI_TWO_SIDED)) {
                vector3 n = (ca_model_verts[tri->v[1]] - ca_model_verts[tri->v[0]])
                                .cross(ca_model_verts[tri->v[2]] - ca_model_verts[tri->v[1]]);
                if (!gr_check_normal_facing(&n, &ca_model_verts[tri->v[0]]))
                    continue;
            }
            ca_mesh_indices[num_tris * 3] = (ushort)tri->v[0];
            ca_mesh_indices[num_tris * 3 + 1] = (ushort)tri->v[1];
            ca_mesh_indices[num_tris * 3 + 2] = (ushort)tri->v[2];
            num_tris++;
        }
        glDrawElements(GL_TRIANGLES, num_tris * 3, GL_UNSIGNED_SHORT, ca_mesh_indices);

        if (vfx_render_bitmap >= 0) {
            gr_opengl_set_bitmap(gr_mode_mesh_alpha, vfx_render_bitmap, 0);
            int n = sm->size0;
            if (n > 0) {
                ubyte r = light->tint[0];
                ubyte g = light->tint[1];
                ubyte b = light->tint[2];
                for (j = 0; j < n; j++) {
                    ca_gl_mesh_colors[j][0] = r;
                    ca_gl_mesh_colors[j][1] = g;
                    ca_gl_mesh_colors[j][2] = b;
                    ca_gl_mesh_colors[j][3] = vfx_render_alpha;
                }
            }
            glDrawElements(GL_TRIANGLES, num_tris * 3, GL_UNSIGNED_SHORT, ca_mesh_indices);
        }
    }
    lights_pop_selection();
    gr_stop_instance();
}

// 0x50d6c0  Confidence: exact.
void ca_render_set_instance(ca_character *character, character_instance *instance)
{
    ca_render_character = character;
    ca_render_instance = instance;
}

// 0x50d6e0  Confidence: exact.
void ca_render_set_mesh(void *mesh)
{
    ca_render_current_mesh = (vim *)mesh;
}

// 0x50d6f0  Confidence: exact. The light record's +8 (255 from its constructor) becomes the fog-scaled
// vertex alpha.
void ca_render_mesh(vector3 *pos, matrix *orient, gr_light_desc *light)
{
    if (ca_render_current_mesh == NULL)
        return;
    light->_unk08 = gr_get_point_fog_value(light->_unk08, pos);
    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_render_character_mesh(ca_render_current_mesh, pos, orient, light, 1, ca_render_instance != NULL,
                                     -1, -1);
        break;
    case GR_GLIDE:
        gr_glide_render_character_mesh(ca_render_current_mesh, pos, orient, light, 1, ca_render_instance != NULL,
                                       -1, -1);
        break;
    case GR_OPENGL:
        gr_opengl_render_character_mesh(ca_render_current_mesh, pos, orient, light, 1, ca_render_instance != NULL,
                                        -1, -1);
        break;
    }
}

// 0x50d7b0  Confidence: exact.
int ca_render_has_mesh()
{
    return ca_render_current_mesh != NULL;
}

// 0x50d7c0  Confidence: exact. Nothing reads ca_render_flags.
void ca_render_set_flags(int flags)
{
    ca_render_flags = flags;
}

// 0x50d7d0  Confidence: exact. Empty, no reference.
void ca_render_stub_ret()
{
}

// ---- COMDATs (0x50d7e0-0x50d9ab), the header inlines this object calls (D4500) -------------------------
// 0x50d7e0 vector3::cross (vsdk/math/vector.h)  Confidence: exact.
// 0x50d830 gr_get_fog_alpha (vsdk/gr/unnamed_gr_3d_0x50d9b0.h)  Confidence: exact.
// 0x50d880 gr_calc_depth (same header)  Confidence: likely (the clamp written with a local).
// 0x50d8b0 gr_vertex_depth (same header)  Confidence: exact.
// 0x50d8d0 gr_get_point_fog_value (same header)  Confidence: exact.
// 0x50d910 gr_get_fog_value (same header)  Confidence: exact.
// 0x50d960 float_max (vsdk/math/vector.h)  Confidence: exact.
// 0x50d980 clamp_float (vsdk/math/vector.h)  Confidence: exact.
