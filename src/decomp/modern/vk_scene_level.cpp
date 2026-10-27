// vk_scene_level.cpp - the Vulkan back end's level draw list (rung L4, docs/future/vulkan-renderer.md). Not original
// source. 64-bit MODERN_BUILD with the Vulkan SDK only. See modern/vk_internal.h for the units.
//
// Follows levelrender_draw_list_opengl (game/levelrender.cpp, 0x491500) line by line: the same lighting, colours,
// texture coordinates, clipping, batches and state calls. It fills the OpenGL list's own client arrays
// (levelrender_gl_vertex_array, _color_array, _texcoords0 / 1, _indices: globals of levelrender.cpp), so what they
// keep between calls (colours not rewritten, vertices of clipped-away corners) is the same as under OpenGL. Each
// glDrawArrays / glDrawElements becomes vk_draw of the vertices it reads, converted at the moment of the draw.

#if defined(_WIN64) && defined(MODERN_VULKAN)
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "vk_internal.h"
#include "engine/rendering/levelrender.h"
#include "engine/s3d/s3d.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/gr/unnamed_gr_3d_0x50d9b0.h"

// levelrender.cpp's sizes and arrays (not in its header).
#define LEVELRENDER_MAX_VERTS 4000
#define LEVELRENDER_MAX_POLY_VERTS 4512
#define LEVELRENDER_MAX_INDICES 9000
#define LEVELRENDER_GL_BATCH_LIMIT 4000
extern int *levelrender_fan_tables[9];
extern float Small_mesh_threshold;
extern vector3 levelrender_view_verts[LEVELRENDER_MAX_VERTS];
extern ubyte levelrender_gl_vert_lit[LEVELRENDER_MAX_VERTS];
extern ubyte levelrender_gl_vert_point_light[LEVELRENDER_MAX_VERTS];
extern float levelrender_gl_texcoords0[LEVELRENDER_MAX_POLY_VERTS][4];
extern float levelrender_gl_texcoords1[LEVELRENDER_MAX_POLY_VERTS][4];
extern ubyte levelrender_gl_color_array[LEVELRENDER_MAX_POLY_VERTS][4];
extern vector3 levelrender_gl_vertex_array[LEVELRENDER_MAX_POLY_VERTS];
extern ushort levelrender_gl_indices[LEVELRENDER_MAX_INDICES];

// levelrender.cpp's inline pieces, copied as they are there.
#define LEVELRENDER_CODE_POINT(c, p)                                                                    \
    {                                                                                                  \
        c = 0;                                                                                         \
        if ((p).x > (p).z)                                                                             \
            c = 4;                                                                                     \
        if ((p).y > (p).z)                                                                             \
            c |= 0x10;                                                                                 \
        if (-(p).z > (p).x)                                                                            \
            c |= 2;                                                                                    \
        if (-(p).z > (p).y)                                                                            \
            c |= 8;                                                                                    \
        if ((p).z <= 0.0f)                                                                             \
            c |= 0x20;                                                                                 \
        if (gr_far_clip_enabled && (p).z > gr_far_clip_z)                                              \
            c |= 1;                                                                                    \
    }
#define LEVELRENDER_PROJECT_XY(p, w)                                                                   \
    {                                                                                                  \
        (p).x = ((w) * (p).x + 1.0f) * gr_projection_xmul + gr_screen.offset_x;                        \
        (p).y = (1.0f - (w) * (p).y) * gr_projection_ymul + gr_screen.offset_y;                        \
    }
#define LEVELRENDER_FOG_ALPHA(z)                                                                       \
    ((z) > fog_near ? (int)(alpha * float_max(0.0f, (fog_far - (z)) * fog_inv_range)) : alpha)
#define LEVELRENDER_FAR_FADE(z) (1.0f / ((z) - gr_scale_by_far_clip(0.1f)))

static inline float levelrender_depth(float w)
{
    float z = gr_depth_scale * w * Gr_3d_zmult;
    if (Gr_3d_zmax < z)
        z = Gr_3d_zmax;
    return z;
}

// The OpenGL list's function-local statics (0x240d9f0, 0x24268f8; D5803).
static vector3 levelrender_gl_proj_verts[LEVELRENDER_MAX_VERTS];
static vector3 levelrender_gl_vert_colors[LEVELRENDER_MAX_VERTS];

// ---- the client arrays to vk_vertex ----
static vk_vertex vk_level_verts[LEVELRENDER_MAX_INDICES];   // a draw's vertices (at most one per index)
static ushort vk_level_idx[LEVELRENDER_MAX_INDICES];
// glDrawElements: each array index used gets one slot in vk_level_verts; a stamp per index marks this draw's.
static unsigned int vk_level_stamp[65536];
static ushort vk_level_slot[65536];
static unsigned int vk_level_gen;

// One element of the client arrays. Stage 1's coordinates only while its array is enabled (two-texture modes);
// zeros otherwise, as the shaders then do not read them.
static void vk_level_vertex(vk_vertex *o, int i, bool stage1)
{
    o->x = levelrender_gl_vertex_array[i].x;
    o->y = levelrender_gl_vertex_array[i].y;
    o->z = levelrender_gl_vertex_array[i].z;
    o->color = vk_pack_color(levelrender_gl_color_array[i]);
    memcpy(o->uv0, levelrender_gl_texcoords0[i], sizeof o->uv0);
    if (stage1)
        memcpy(o->uv1, levelrender_gl_texcoords1[i], sizeof o->uv1);
    else
        memset(o->uv1, 0, sizeof o->uv1);
}

// glDrawArrays(GL_TRIANGLES, start, n).
static void vk_level_draw_arrays(int start, int n, bool stage1)
{
    if (n <= 0)
        return;
    if (n > LEVELRENDER_MAX_INDICES)
        n = LEVELRENDER_MAX_INDICES;
    for (int i = 0; i < n; i++) {
        vk_level_vertex(&vk_level_verts[i], start + i, stage1);
        vk_level_idx[i] = (ushort)i;
    }
    vk_draw(VK_PRIM_TRIANGLES, vk_level_verts, n, vk_level_idx, n);
}

// glDrawElements(GL_TRIANGLES, n, GL_UNSIGNED_SHORT, idx): only the elements the indices name are converted.
static void vk_level_draw_elements(const ushort *idx, int n, bool stage1)
{
    if (n <= 0)
        return;
    if (n > LEVELRENDER_MAX_INDICES)
        n = LEVELRENDER_MAX_INDICES;
    if (++vk_level_gen == 0) {
        memset(vk_level_stamp, 0, sizeof vk_level_stamp);
        vk_level_gen = 1;
    }
    int nv = 0;
    for (int k = 0; k < n; k++) {
        ushort e = idx[k];
        if (vk_level_stamp[e] != vk_level_gen) {
            vk_level_stamp[e] = vk_level_gen;
            vk_level_slot[e] = (ushort)nv;
            vk_level_vertex(&vk_level_verts[nv++], e, stage1);
        }
        vk_level_idx[k] = vk_level_slot[e];
    }
    vk_draw(VK_PRIM_TRIANGLES, vk_level_verts, nv, vk_level_idx, n);
}

// levelrender_draw_list_opengl. The client array setup (glVertexPointer etc.) and the texture unit selection
// (gl_active_texture_arb, gl_client_active_texture_arb) have no counterpart: the arrays are read at each draw, and
// gr_opengl_set_mode and tcache_set address their stage themselves.
void vk_levelrender_draw_list(lobj *l, int alpha, unsigned int num_lights, attached_light **lights)
{
    bool far_fade;
    bool clipped;
    vector3 *verts;
    float u_scale, v_scale;
    float fog_near, fog_far, fog_inv_range;
    float w;
    int n;
    int vbase;
    int face_base;
    int g;
    int i;
    int f;
    int k;
    int v;
    int cur;
    int flat;
    int bm;
    int lit_sum;
    int count;
    int start;
    int face_count;
    int num_idx;
    int col;
    int *fan;
    ubyte c, c0, c1, c2;
    s3d_render_group *grp;
    short *dup;
    ubyte *rgb;
    s3d_face *face;
    lobj_face *lf;
    s3d_box *box;
    gr_vertex **clip;
    gr_vertex *gv;
    vector3 *vp;
    ubyte *cp;
    float *t0;
    float *t1;
    ushort *ip;
    float s;

    far_fade = (l->flags & 0x200400) != 0;
    if (l->flags & 0x20) {
        levelrender_wave_lobj(l, levelrender_wave_vertices);
        verts = levelrender_wave_vertices;
    } else
        verts = l->mesh->vertices;
    u_scale = 0;
    v_scale = 0;
    n = 0;
    fog_inv_range = gr_fog_alpha_inv_range;
    vbase = 0;
    face_base = 0;
    clip = NULL;
    if (l->mesh->_unk30 <= Small_mesh_threshold) {
        fog_near = gr_fog_alpha_near;
        fog_far = gr_fog_alpha_far;
    } else {
        fog_near = gr_fog_vertex_near;
        fog_far = gr_fog_vertex_far;
    }

    // Lightmapped: batches of triangles, drawn when the lightmap changes, before 4000 vertices, and after each group;
    // with point lights on a batch, a second, additive pass of it.
    if (!gr_tint_enabled && l->faces->lightmap >= 0 && !(l->flags & 0x80000)) {
        vk_gr_opengl_set_mode(gr_mode_lightmap_multitexture);
        cur = -1;
        start = 0;
        count = 0;
        for (g = 0; g < l->mesh->num_groups; g++) {
            grp = &l->mesh->groups[g];
            vk_gr_opengl_tcache_set(level_texture_bms[l->faces[face_base].texture - 1], &u_scale, &v_scale, 0);
            dup = &l->mesh->vertex_dup[vbase];
            for (i = vbase; i < vbase + grp->num_vertices; i++, dup++) {
                if (*dup >= 0) {
                    levelrender_gl_vert_lit[i] = levelrender_gl_vert_lit[i - *dup];
                    levelrender_view_verts[i] = levelrender_view_verts[i - *dup];
                    if (num_lights) {
                        levelrender_gl_vert_colors[i] = levelrender_gl_vert_colors[i - *dup];
                        levelrender_gl_vert_point_light[i] = levelrender_gl_vert_point_light[i - *dup];
                    }
                    if (!levelrender_gl_vert_lit[i])
                        levelrender_gl_proj_verts[i] = levelrender_gl_proj_verts[i - *dup];
                } else {
                    if (num_lights)
                        levelrender_gl_vert_point_light[i] = (ubyte)levelrender_sum_point_lights(
                            &levelrender_gl_vert_colors[i], &verts[i], &l->mesh->normals[i], num_lights,
                            (gr_light **)lights);
                    levelrender_gl_proj_verts[i] = verts[i] - gr_view_pos;
                    levelrender_gl_proj_verts[i].rotate(gr_view_matrix);
                    levelrender_view_verts[i] = levelrender_gl_proj_verts[i];
                    LEVELRENDER_CODE_POINT(c, levelrender_gl_proj_verts[i]);
                    levelrender_gl_vert_lit[i] = c;
                    if (!c) {
                        w = 1.0f / levelrender_gl_proj_verts[i].z;
                        LEVELRENDER_PROJECT_XY(levelrender_gl_proj_verts[i], w);
                        if (far_fade)
                            levelrender_gl_proj_verts[i].z = LEVELRENDER_FAR_FADE(levelrender_gl_proj_verts[i].z);
                        else
                            levelrender_gl_proj_verts[i].z = w;
                    }
                }
            }
            lit_sum = 0;
            cp = levelrender_gl_color_array[count];
            vp = &levelrender_gl_vertex_array[count];
            t0 = levelrender_gl_texcoords0[count];
            t1 = levelrender_gl_texcoords1[count];
            for (f = face_base; f < face_base + grp->num_faces; f++) {
                lf = &l->faces[f];
                box = &l->lightmap_boxes[f];
                face = &l->mesh->faces[f];
                if (!(face->flags & 0x200) && !gr_check_normal_facing(&face->normal, &verts[face->v[0]]))
                    continue;
                c0 = levelrender_gl_vert_lit[face->v[0]];
                c1 = levelrender_gl_vert_lit[face->v[1]];
                c2 = levelrender_gl_vert_lit[face->v[2]];
                if (c0 & c1 & c2)
                    continue;
                face_count = 3;
                if (c0 | c1 | c2) {
                    n = 3;
                    clip = levelrender_face_vertex_colors(l, face, (int)(intptr_t)lf, (float *)box,
                                                          levelrender_gl_vert_lit, levelrender_view_verts, &n,
                                                          levelrender_gl_vert_colors, num_lights);
                    if (n < 3 || n > 8)
                        continue;
                    clipped = true;
                    face_count = 3 * (n - 2);
                } else
                    clipped = false;
                bm = level_face_bitmaps[lf->lightmap];
                if ((bm != cur && count > start) || count + face_count > LEVELRENDER_GL_BATCH_LIMIT) {
                    vk_level_draw_arrays(start, count - start, true);
                    if (num_lights && lit_sum) {
                        vk_gr_opengl_set_mode(gr_mode_level_alpha_add);
                        vk_level_draw_arrays(start, count - start, false);
                        vk_gr_opengl_set_mode(gr_mode_lightmap_multitexture);
                        cur = -1;
                    }
                    // The batch restarts only when three more vertices would pass the limit (D5804).
                    if (count + 3 > LEVELRENDER_GL_BATCH_LIMIT)
                        count = 0;
                    vp = &levelrender_gl_vertex_array[count];
                    cp = levelrender_gl_color_array[count];
                    t0 = levelrender_gl_texcoords0[count];
                    start = count;
                    lit_sum = 0;
                    t1 = levelrender_gl_texcoords1[count];
                }
                if (num_lights)
                    lit_sum += levelrender_gl_vert_point_light[face->v[0]] + levelrender_gl_vert_point_light[face->v[1]]
                               + levelrender_gl_vert_point_light[face->v[2]];
                if (bm != cur) {
                    vk_gr_opengl_tcache_set(bm, &u_scale, &v_scale, 1);
                    cur = bm;
                }
                if (clipped) {
                    fan = levelrender_fan_tables[n];
                    for (k = 0; k < face_count; k++, vp++, cp += 4, t0 += 4, t1 += 4) {
                        gv = clip[fan[k]];
                        w = gv->sw;
                        vp->x = gv->sx;
                        vp->y = gv->sy;
                        if (far_fade)
                            gv->sw = LEVELRENDER_FAR_FADE(gv->z);
                        vp->z = levelrender_depth(gv->sw);
                        cp[3] = (ubyte)LEVELRENDER_FOG_ALPHA(gv->z);
                        if (num_lights) {
                            cp[0] = gv->r;
                            cp[1] = gv->g;
                            cp[2] = gv->b;
                        }
                        t0[0] = w * gv->u;
                        t0[1] = w * gv->v;
                        t0[2] = 0;
                        t0[3] = w;
                        t1[0] = w * gv->u2;
                        t1[1] = w * gv->v2;
                        t1[2] = 0;
                        t1[3] = w;
                    }
                } else {
                    for (k = 0; k < 3; k++, vp++, cp += 4, t0 += 4, t1 += 4) {
                        v = face->v[k];
                        w = levelrender_gl_proj_verts[v].z;
                        vp->x = levelrender_gl_proj_verts[v].x;
                        vp->y = levelrender_gl_proj_verts[v].y;
                        if (far_fade)
                            vp->z = levelrender_depth(LEVELRENDER_FAR_FADE(levelrender_view_verts[v].z));
                        else
                            vp->z = levelrender_depth(levelrender_gl_proj_verts[v].z);
                        cp[3] = (ubyte)LEVELRENDER_FOG_ALPHA(levelrender_view_verts[v].z);
                        if (num_lights) {
                            // The original's: all three bytes from the red component (D5804).
                            cp[0] = (ubyte)(int)(levelrender_gl_vert_colors[v].x * 255.0f);
                            cp[1] = (ubyte)(int)(levelrender_gl_vert_colors[v].x * 255.0f);
                            cp[2] = (ubyte)(int)(levelrender_gl_vert_colors[v].x * 255.0f);
                        }
                        t0[0] = w * l->mesh->uvs[v].x;
                        t0[1] = w * l->mesh->uvs[v].y;
                        t0[2] = 0;
                        t0[3] = w;
                        t1[0] = w * ((float *)box)[k];
                        t1[1] = w * ((float *)box)[k + 3];
                        t1[2] = 0;
                        t1[3] = w;
                    }
                }
                count += face_count;
            }
            if (count > start) {
                vk_level_draw_arrays(start, count - start, true);
                if (num_lights && lit_sum) {
                    vk_gr_opengl_set_mode(gr_mode_level_alpha_add);
                    vk_level_draw_arrays(start, count - start, false);
                    vk_gr_opengl_set_mode(gr_mode_lightmap_multitexture);
                    cur = -1;
                }
                start = count;
            }
            face_base += grp->num_faces;
            vbase += grp->num_vertices;
        }
        return;
    }

    // Plain: the group's vertices at their vertex index, clipped polygons appended after the mesh's vertices (for
    // the whole object), one indexed draw per group.
    cur = -1;
    count = l->mesh->num_vertices;
    for (g = 0; g < l->mesh->num_groups; g++) {
        grp = &l->mesh->groups[g];
        flat = !l->faces[face_base].texture;
        if (flat != cur) {
            vk_gr_opengl_set_mode(l->faces[face_base].texture ? gr_mode_mesh_alpha : gr_mode_flat_alpha);
            cur = flat;
        }
        if (l->faces[face_base].texture)
            vk_gr_opengl_tcache_set(level_texture_bms[l->faces[face_base].texture - 1], &u_scale, &v_scale, 0);
        rgb = &l->vertex_rgb[vbase * 3];
        dup = &l->mesh->vertex_dup[vbase];
        t0 = levelrender_gl_texcoords0[vbase];
        cp = levelrender_gl_color_array[vbase];
        vp = &levelrender_gl_vertex_array[vbase];
        for (i = vbase; i < vbase + grp->num_vertices; i++, vp++, cp += 4, t0 += 4, dup++, rgb += 3) {
            if (*dup >= 0) {
                levelrender_view_verts[i] = levelrender_view_verts[i - *dup];
                levelrender_gl_vert_lit[i] = levelrender_gl_vert_lit[i - *dup];
                if (num_lights)
                    levelrender_gl_vert_colors[i] = levelrender_gl_vert_colors[i - *dup];
                if (levelrender_gl_vert_lit[i])
                    continue;
                *vp = vp[-*dup];
            } else {
                // The light count is not kept on this path (the vertex colours are).
                if (num_lights)
                    levelrender_sum_point_lights(&levelrender_gl_vert_colors[i], &verts[i], &l->mesh->normals[i],
                                                 num_lights, (gr_light **)lights);
                *vp = verts[i] - gr_view_pos;
                vp->rotate(gr_view_matrix);
                levelrender_view_verts[i] = *vp;
                LEVELRENDER_CODE_POINT(c, *vp);
                levelrender_gl_vert_lit[i] = c;
                if (c)
                    continue;
                w = 1.0f / vp->z;
                if (far_fade)
                    vp->z = vp->z - gr_scale_by_far_clip(0.1f);
                vp->z = levelrender_depth(1.0f / vp->z);
                LEVELRENDER_PROJECT_XY(*vp, w);
            }
            w = 1.0f / levelrender_view_verts[i].z;
            t0[0] = w * l->mesh->uvs[i].x;
            t0[1] = w * l->mesh->uvs[i].y;
            t0[2] = 0;
            t0[3] = w;
            cp[3] = (ubyte)LEVELRENDER_FOG_ALPHA(levelrender_view_verts[i].z);
            if (gr_tint_enabled) {
                s = (float)(rgb[0] + rgb[1] + rgb[2]);
                if (s < 0.0f)
                    s = 0.0f;
                else if (s > 255.0f)
                    s = 255.0f;
                cp[0] = (ubyte)(int)(s * gr_tint_r);
                cp[1] = (ubyte)(int)(s * gr_tint_g);
                cp[2] = (ubyte)(int)(s * gr_tint_b);
            } else if (num_lights) {
                col = rgb[0] - (int)(levelrender_gl_vert_colors[i].x * -255.0f);
                if (col > 255)
                    col = 255;
                cp[0] = (ubyte)col;
                col = rgb[1] - (int)(levelrender_gl_vert_colors[i].y * -255.0f);
                if (col > 255)
                    col = 255;
                cp[1] = (ubyte)col;
                col = rgb[2] - (int)(levelrender_gl_vert_colors[i].z * -255.0f);
                if (col > 255)
                    col = 255;
                cp[2] = (ubyte)col;
            } else {
                cp[0] = rgb[0];
                cp[1] = rgb[1];
                cp[2] = rgb[2];
            }
        }
        ip = levelrender_gl_indices;
        num_idx = 0;
        for (f = face_base; f < face_base + grp->num_faces; f++) {
            face = &l->mesh->faces[f];
            lf = &l->faces[f];
            if (!(face->flags & 0x200) && !gr_check_normal_facing(&face->normal, &verts[face->v[0]]))
                continue;
            c0 = levelrender_gl_vert_lit[face->v[0]];
            c1 = levelrender_gl_vert_lit[face->v[1]];
            c2 = levelrender_gl_vert_lit[face->v[2]];
            if (c0 & c1 & c2)
                continue;
            face_count = 3;
            if (c0 | c1 | c2) {
                n = 3;
                clip = levelrender_face_vertex_colors(l, face, (int)(intptr_t)lf, NULL, levelrender_gl_vert_lit,
                                                      levelrender_view_verts, &n, levelrender_gl_vert_colors,
                                                      num_lights);
                if (n < 3)
                    continue;
                face_count = 3 * (n - 2);
                fan = levelrender_fan_tables[n];
                for (k = 0; k < face_count; k++) {
                    gv = clip[fan[k]];
                    if (gv->flags & 4) {
                        ip[k] = (ushort)count;
                        cp = levelrender_gl_color_array[count];
                        vp = &levelrender_gl_vertex_array[count];
                        t0 = levelrender_gl_texcoords0[count];
                        count++;
                        vp->x = gv->sx;
                        vp->y = gv->sy;
                        if (far_fade)
                            vp->z = levelrender_depth(LEVELRENDER_FAR_FADE(gv->z));
                        else
                            vp->z = levelrender_depth(gv->sw);
                        cp[3] = (ubyte)LEVELRENDER_FOG_ALPHA(gv->z);
                        cp[0] = gv->r;
                        cp[1] = gv->g;
                        cp[2] = gv->b;
                        t0[0] = gv->u * gv->sw;
                        t0[1] = gv->v * gv->sw;
                        t0[2] = 0;
                        t0[3] = gv->sw;
                    } else
                        ip[k] = face->v[gv->index];
                }
            } else
                memcpy(ip, face->v, 3 * sizeof(ushort));
            ip += face_count;
            num_idx += face_count;
        }
        vbase += grp->num_vertices;
        face_base += grp->num_faces;
        vk_level_draw_elements(levelrender_gl_indices, num_idx, false);
    }
}
#endif
