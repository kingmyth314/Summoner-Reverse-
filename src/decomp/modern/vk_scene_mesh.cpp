// vk_scene_mesh.cpp - the Vulkan back end's character meshes (rung L4, docs/future/vulkan-renderer.md). Not original
// source. 64-bit MODERN_BUILD with the Vulkan SDK only. See modern/vk_internal.h for the units.
//
// Follows gr_opengl_render_character_mesh (vsdk/unnamed_ca_mesh_0x50af80.cpp, Sum.exe 0x50cd00) statement by
// statement: the same skinning, projection, lighting, tint, texture coordinates and passes, in the same work buffers
// (that unit's globals), so the vertices are the ones OpenGL gets. Only the hand-over differs: where OpenGL reads its
// client arrays at glDrawElements (positions from ca_skin_positions, colours from ca_gl_mesh_colors, s, t, r, q from
// ca_gl_mesh_texcoords on stage 0), this builds vk_vertex records from the same arrays at the same moment.

#if defined(_WIN64) && defined(MODERN_VULKAN)
#include <string.h>
#include <math.h>
#include "vk_internal.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/geom/vfx.h"

// The work buffers of vsdk/unnamed_ca_mesh_0x50af80.cpp (defined there, not in its header; the types as there).
extern vector3 ca_skin_positions[CA_MAX_MESH_VERTS];
extern vector3 ca_skin_normals[CA_MAX_MESH_VERTS];
extern vector3 ca_model_verts[4000];
extern ubyte ca_gl_mesh_colors[CA_MAX_MESH_VERTS][4];
extern float ca_gl_mesh_texcoords[CA_MAX_MESH_VERTS][4];
extern ubyte ca_vertex_done[CA_MAX_MESH_VERTS];

// That unit's #define: lights_shade_vertices' eighth argument, not read (D4006); the renderers push the float 255.0.
#define CA_SHADE_UNUSED_255 0x437f0000

// The index list. OpenGL's ca_mesh_indices has CA_MAX_MESH_VERTS entries but takes 3 per visible triangle, so a
// sub-mesh with more than 666 runs past it (into an unreferenced buffer in Sum.exe's layout; not so in this build's).
// This one has room for two triangles per vertex, and OpenGL draws what it wrote, so the output is the same.
#define VK_MESH_MAX_INDICES (CA_MAX_MESH_VERTS * 6)
static ushort vk_mesh_indices[VK_MESH_MAX_INDICES];
static vk_vertex vk_mesh_verts[CA_MAX_MESH_VERTS];   // ~96 KB, so not on the stack

// glDrawElements(GL_TRIANGLES, n, GL_UNSIGNED_SHORT, indices) with the client arrays as the function sets them.
// Only the vertices up to the largest index are read; the others may hold another sub-mesh's values, unused.
static void vk_mesh_draw(int num_indices, int max_index)
{
    if (num_indices <= 0 || max_index >= CA_MAX_MESH_VERTS)
        return;
    for (int j = 0; j <= max_index; j++) {
        vk_vertex *o = &vk_mesh_verts[j];
        o->x = ca_skin_positions[j].x;
        o->y = ca_skin_positions[j].y;
        o->z = ca_skin_positions[j].z;     // gr_vertex_depth's value, as gr_opengl_tmapper passes it
        o->color = vk_pack_color(ca_gl_mesh_colors[j]);
        memcpy(o->uv0, ca_gl_mesh_texcoords[j], sizeof o->uv0);
        memset(o->uv1, 0, sizeof o->uv1);  // stage 1 is off in gr_mode_mesh_alpha (texture source 1)
    }
    vk_draw(VK_PRIM_TRIANGLES, vk_mesh_verts, max_index + 1, vk_mesh_indices, num_indices);
}

void vk_render_character_mesh(vim *mesh, vector3 *pos, matrix *orient, gr_light_desc *light, int a5,
                              int has_instance, int a7, int a8)
{
    static bool warned;
    if (mesh == NULL)
        return;

    // +0x1c holds an int mask in a pointer field (D1711); through intptr_t for the 64-bit cast, the same low bits.
    int hide_mask = (int)(intptr_t)light->model_prefix << 6;
    vector3 center(mesh->center);
    center.unrotate(*orient);
    center += *pos;
    gr_start_instance(pos, orient, 1);
    lights_select_nearest(-1, &center, mesh->radius);
    lights_transform_selection();
    // OpenGL selects stage 0 here and points its client arrays at the work buffers; vk_mesh_draw reads the same.

    for (int i = 0; i < mesh->num_submeshes; i++) {
        vim_submesh *sm = &mesh->submeshes[i];
        if (sm->size1 == 0 || sm->_unk18 < 0)
            continue;
        vk_gr_opengl_set_bitmap(gr_mode_mesh_alpha, light->appearance ? ((int *)light->appearance)[sm->_unk18]
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
                        // The normal takes the bone's translation too, as in all three back ends.
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
                    ca_skin_positions[j].x *= 0.0625f;   // the weights sum to 16
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
                if (ca_skin_positions[j].z < 0.0f) {   // behind the viewer: its triangles are not drawn
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
        int max_index = -1;
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
            if (num_tris * 3 + 3 > VK_MESH_MAX_INDICES) {
                if (!warned) {
                    warned = true;
                    vk_log("vulkan: a character sub-mesh has more than %d visible triangles; the rest not drawn",
                           VK_MESH_MAX_INDICES / 3);
                }
                break;
            }
            vk_mesh_indices[num_tris * 3] = (ushort)tri->v[0];
            vk_mesh_indices[num_tris * 3 + 1] = (ushort)tri->v[1];
            vk_mesh_indices[num_tris * 3 + 2] = (ushort)tri->v[2];
            for (int c = 0; c < 3; c++) {
                if (tri->v[c] > max_index)
                    max_index = tri->v[c];
            }
            num_tris++;
        }
        vk_mesh_draw(num_tris * 3, max_index);

        // The second (environment) pass: the same triangles with the effect's bitmap, the light's tint and alpha.
        if (vfx_render_bitmap >= 0) {
            vk_gr_opengl_set_bitmap(gr_mode_mesh_alpha, vfx_render_bitmap, 0);
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
            vk_mesh_draw(num_tris * 3, max_index);
        }
    }
    lights_pop_selection();
    gr_stop_instance();
}
#endif
