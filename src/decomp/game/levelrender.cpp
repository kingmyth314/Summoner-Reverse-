// levelrender.cpp - Sum.exe v1.40, code 0x48db30-0x492af0 (Engine\rendering\levelrender.cpp, path
// literal 0x59c524). Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0
// (cl 12.00.8966). Confidence per function: exact = expect identical code, likely = same semantics,
// small layout risk, unsure = the source shape is a guess.
//
// Unit range: sum.symbols tags 39 functions from 0x48db30 (init_array_23df188_thunk) to 0x492af0
// (atexit_nop_492af0) with levelrender.cpp. Optimised build (/O2).
//
// PARTIAL (decomp28): the three back-end draw lists are not written yet:
//   0x48e6c0 levelrender_draw_list_glide  (0x48e6c0-0x48f8bf, with atexit_nop_48fd90 / _48fda0)
//   0x48fdb0 levelrender_draw_list_d3d    (0x48fdb0-0x4914df, with atexit_nop_4914e0 / _4914f0)
//   0x491500 levelrender_draw_list_opengl (0x491500-0x492adf, with atexit_nop_492ae0 / _492af0)
// Each holds two function-local statics (guard bytes 0x23eb68c, 0x23d3600, 0x23c5b38) whose empty
// atexit handlers are the compiler's; their per-vertex arrays (see NOTES.md) are not defined here.

#include <stdio.h>
#include <math.h>
#include <windows.h>
#include <d3d.h>
#include "engine/rendering/levelrender.h"
#include "engine/s3d/s3d.h"
#include "vsdk/os/console.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/gr/gr.h"
#include "vsdk/geom/vfx.h"

// Original __FILE__ of this unit (literal 0x59c524).
#define LEVELRENDER_CPP "D:\\projects\\Summoner\\pccode\\Engine\\rendering\\levelrender.cpp"

#define LEVELRENDER_MAX_VERTS 4000            // the per-vertex arrays (0xbb80 bytes of vector3; ours)
#define LEVELRENDER_MAX_WAVE_NORMALS 12000    // levelrender_wave_lobj's last loop bound (ours)

// ---- .rdata ----------------------------------------------------------------------------------------
// 0.001f (0x581938), 255.0f (0x57eb80), -255.0f (0x581ad4), 0.75f (0x580e04), 1.0f (0x57e990), 0.0f
// (0x57eb84) are shared __real@ constants; 20000.0f is an immediate (0x469c4000).

// ---- .data (0x59c358-0x59c487; the literals 0x59c488-0x59c5a3 follow) ------------------------------
float level_light_scale = 1.0f;               // 0x59c358
// Fan triangulations of 3- to 8-sided polygons, three corner indices per triangle, read by the D3D and
// OpenGL draw lists through levelrender_fan_tables[n] (names ours).
int levelrender_fan3[3] = { 0, 1, 2 };                                            // 0x59c35c
int levelrender_fan4[6] = { 0, 1, 2, 0, 2, 3 };                                   // 0x59c368
int levelrender_fan5[9] = { 0, 1, 2, 2, 3, 4, 2, 4, 0 };                          // 0x59c380
int levelrender_fan6[12] = { 0, 1, 2, 2, 3, 4, 2, 4, 0, 4, 5, 0 };                // 0x59c3a4
int levelrender_fan7[15] = { 0, 1, 2, 2, 3, 4, 4, 5, 6, 0, 2, 6, 2, 4, 6 };       // 0x59c3d4
int levelrender_fan8[18] = { 0, 1, 5, 1, 2, 4, 2, 3, 4, 4, 5, 1, 5, 6, 7, 7, 0, 5 };   // 0x59c410
int *levelrender_fan_tables[9] = {                                                // 0x59c458
    NULL, NULL, NULL, levelrender_fan3, levelrender_fan4, levelrender_fan5, levelrender_fan6,
    levelrender_fan7, levelrender_fan8
};
float Small_mesh_threshold = 3.0f;            // 0x59c47c
float Time_mult = 5.0f;                       // 0x59c480
float Space_mult = 20.0f;                     // 0x59c484

// ---- .bss (0x2319070-0x243e03f, partial) --------------------------------------------------------------
// Only the objects of the functions written here; the draw lists' arrays are listed in NOTES.md.
float levelrender_wave_vertex_face_count[LEVELRENDER_MAX_WAVE_NORMALS];   // 0x238a170
gr_vertex levelrender_face_verts[3];          // 0x23c5aa8 (name ours)
gr_vertex *levelrender_face_vert_list[10];    // 0x23ec630
void *levelrender_d3d_vertex_buffer;          // 0x243e03c

// 0x48db30 init_array_23df188_thunk / 0x48db40 init_array_23df188: the inline constructor of
// attached_light (reset()) over the 16 records (D805). Static initialiser entry 253. Confidence: exact.
attached_light levelrender_lights[16];        // 0x23df188 (name ours; not referenced by address elsewhere)

// Static initialiser entries 254-259 (0x48db60/0x48db70 .. 0x48dc00/0x48dc10): six thunks to empty
// initialisers, i.e. six objects with empty inline constructors (vector3 arrays, D500) defined here.
// Two of them are probably the wave arrays below; the other four are among the draw lists' vector3
// arrays (which ones is open, NOTES.md). Confidence: unsure (the assignment of the six).
vector3 levelrender_wave_vertex_normals[LEVELRENDER_MAX_WAVE_NORMALS];   // 0x2396ca8
vector3 levelrender_wave_vertices[LEVELRENDER_MAX_VERTS];               // 0x23ec658

// ---- console commands --------------------------------------------------------------------------------
void cmd_time_mult();
void cmd_space_mult();
void cmd_small_mesh_threshold();

// 0x48dc20 init_ccmd_time_mult_thunk / 0x48dc30 init_ccmd_time_mult. Confidence: exact.
console_command ccmd_time_mult("time_mult", "Sets Time_mult", cmd_time_mult);   // 0x23fc838

// 0x48dc50  Confidence: exact. The console's float command macro (as cmd_gamma in summoner.cpp).
void cmd_time_mult()
{
    if (calling_a_command_function) {
        process_command(ARG_FLOAT);
        Time_mult = command_argument_float;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [float]\nSets %s to the specified value.\n", "time_mult", "Time_mult");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %f\n", "time_mult", Time_mult);
        print_to_console(string_buffer, 0);
    }
}

// 0x48dce0 init_ccmd_space_mult_thunk / 0x48dcf0 init_ccmd_space_mult. Confidence: exact.
console_command ccmd_space_mult("space_mult", "Sets Space_mult", cmd_space_mult);   // 0x2432478

// 0x48dd10  Confidence: exact.
void cmd_space_mult()
{
    if (calling_a_command_function) {
        process_command(ARG_FLOAT);
        Space_mult = command_argument_float;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [float]\nSets %s to the specified value.\n", "space_mult", "Space_mult");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %f\n", "space_mult", Space_mult);
        print_to_console(string_buffer, 0);
    }
}

// 0x48dda0 init_ccmd_small_mesh_threshold_thunk / 0x48ddb0 init_ccmd_small_mesh_threshold. Confidence: exact.
console_command ccmd_small_mesh_threshold("small_mesh_threshold", "Sets Small_mesh_threshold",
                                          cmd_small_mesh_threshold);   // 0x23f81d8

// 0x48ddd0  Confidence: exact.
void cmd_small_mesh_threshold()
{
    if (calling_a_command_function) {
        process_command(ARG_FLOAT);
        Small_mesh_threshold = command_argument_float;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [float]\nSets %s to the specified value.\n", "small_mesh_threshold",
                "Small_mesh_threshold");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %f\n", "small_mesh_threshold", Small_mesh_threshold);
        print_to_console(string_buffer, 0);
    }
}

// ---- waving level objects ----------------------------------------------------------------------------

// 0x48de60  Confidence: likely (unsure for the corner normals' temporaries). Callers: the three draw
// lists, with out = levelrender_wave_vertices, for level objects with flag 0x20. Each vertex is
// copied, then moved up by Perlin noise sampled at the world position (scaled by the record and
// Space_mult), animated along z by the elapsed time (Time_mult) and weighted per vertex. The vector
// members are D500's inlines: the code expands the first uses and calls the COMDATs for the later ones
// (vector3(x, y, z) 0x43b580, the copy 0x410cd0, += 0x48e440; s * v is the out-of-line 0x468a90),
// which looks like VC6's per-function inline budget running out. The second half accumulates one
// normal per face corner into levelrender_wave_vertex_normals: it reads the global array, not `out`,
// and the accumulators are never cleared and never read anywhere (NOTES.md). The function-local
// GameTimer's empty atexit handler is 0x48e460 (atexit_nop_48e460, compiler-generated).
void levelrender_wave_lobj(lobj *l, vector3 *out)
{
    static GameTimer timer;                   // 0x23eb688 levelrender_wave_timer (guard 0x2438a14)
    s3d_textured_record *rec;
    vector3 p;
    float t;
    int i;
    s3d_face *face;
    vector3 n0, n1, n2;

    rec = lobj_find_textured_record(l - lobjs);
    if (!timer.valid())
        timer.SetTimeout(0);
    t = (float)timer.GetElapsedTime() * 0.001f / rec->period;
    for (i = 0; i < l->mesh->num_vertices; i++) {
        p = l->mesh->vertices[i];
        p.rotate(l->orient);
        p += l->pos;
        out[i] = l->mesh->vertices[i];
        p.x /= rec->scale_x;
        p.z /= rec->scale_z;
        out[i].y += perlin_noise3(vector3(0.0f, 0.0f, Time_mult * t) + Space_mult * p) * rec->weights[i]
                    * rec->amplitude;
    }

    for (i = 0; i < l->mesh->num_faces; i++) {
        face = &l->mesh->faces[i];
        n0 = (levelrender_wave_vertices[face->v[1]] - levelrender_wave_vertices[face->v[0]])
                 .cross(levelrender_wave_vertices[face->v[2]] - levelrender_wave_vertices[face->v[0]]);
        n1 = (levelrender_wave_vertices[face->v[2]] - levelrender_wave_vertices[face->v[1]])
                 .cross(levelrender_wave_vertices[face->v[0]] - levelrender_wave_vertices[face->v[1]]);
        n2 = (levelrender_wave_vertices[face->v[0]] - levelrender_wave_vertices[face->v[2]])
                 .cross(levelrender_wave_vertices[face->v[1]] - levelrender_wave_vertices[face->v[2]]);
        n0.normalize();
        n1.normalize();
        n2.normalize();
        levelrender_wave_vertex_normals[face->v[0]] += n0;
        levelrender_wave_vertex_face_count[face->v[0]] += 1.0f;
        levelrender_wave_vertex_normals[face->v[1]] += n1;
        levelrender_wave_vertex_face_count[face->v[1]] += 1.0f;
        levelrender_wave_vertex_normals[face->v[2]] += n2;
        levelrender_wave_vertex_face_count[face->v[2]] += 1.0f;
    }
    for (i = 0; i < LEVELRENDER_MAX_WAVE_NORMALS; i++) {
        if (levelrender_wave_vertex_face_count[i] != 0.0f)
            levelrender_wave_vertex_normals[i].normalize();
    }
}

// ---- Direct3D vertex buffer ----------------------------------------------------------------------

// 0x48e470  Confidence: likely (the descriptor's store order). 8512 vertices of FVF 0x2c4 (XYZRHW,
// diffuse, specular, two texture sets); caps write-only | system memory, plus DONOTCLIP when the
// device uses vertex buffers. The vsdk error loop at line 0x450.
void levelrender_create_vertex_buffer()
{
    D3DVERTEXBUFFERDESC desc;

    if (gr_screen.mode == 0x66 && !levelrender_d3d_vertex_buffer) {
        desc.dwCaps = d3d_use_vertex_buffer ? 0x10801 : 0x10800;
        desc.dwSize = 0x10;
        desc.dwFVF = 0x2c4;
        desc.dwNumVertices = 0x2140;
        if (((IDirect3D7 *)d3d_direct3d)->CreateVertexBuffer(&desc,
                (LPDIRECT3DVERTEXBUFFER7 *)&levelrender_d3d_vertex_buffer, 0) < 0)
            VSDK_ERROR(LEVELRENDER_CPP, 0x450, "Unable to create level vertex buffer for D3d!");
    }
}

// 0x48e4f0  Confidence: exact. Caller levelrender_level_unload.
void levelrender_free_d3d_vertex_buffer()
{
    if (gr_screen.mode == 0x66 && levelrender_d3d_vertex_buffer) {
        ((IDirect3DVertexBuffer7 *)levelrender_d3d_vertex_buffer)->Release();
        levelrender_d3d_vertex_buffer = NULL;
    }
}

// ---- level lights --------------------------------------------------------------------------------

// 0x48e520  Confidence: likely. The level ambient and lights, scaled by level_light_scale. Spot lights
// (types 1 and 4) point down (0, -1, 0; the inline constructor's stores follow to_matrix_b) through
// the light's orientation; a spot whose texture index is out of range is skipped. The count is read once (the loop runs on a down counter while the record is
// re-addressed from level_s3d.lights each time).
void level_lights_setup()
{
    int n;
    int i;
    s3d_light *lt;
    float range_a, range_b;
    matrix m;
    vector3 dir;
    int bm;

    light_reset_all();
    light_set_ambient(level_s3d.ambient.x * level_light_scale, level_s3d.ambient.y * level_light_scale,
                      level_s3d.ambient.z * level_light_scale);
    n = level_s3d.num_lights;
    for (i = 0; i < n; i++) {
        lt = &level_s3d.lights[i];
        if (lt->flags & 2) {
            range_a = lt->range_a;
            range_b = lt->range_b;
        } else {
            range_a = 20000.0f;
            range_b = 20000.0f;
        }
        switch (lt->type) {
        case 1:
        case 4: {
            lt->orient.to_matrix_b(&m);
            vector3 down(0.0f, -1.0f, 0.0f);
            dir = down.unrotated(m);
            if (lt->texture == -1)
                bm = -1;
            else {
                if (lt->texture < 0 || lt->texture >= level_num_textures)
                    break;
                bm = level_texture_bms[lt->texture];
            }
            light_create_spot(&lt->pos, &dir, lt->_unk30, lt->_unk34, range_a, range_b,
                              level_light_scale * lt->intensity, lt->r, lt->g, lt->b, 0, bm, &lt->orient);
            break;
        }
        case 0:
            light_create_point(&lt->pos, range_a, range_b, level_light_scale * lt->intensity, lt->r, lt->g,
                               lt->b, 0);
            break;
        }
    }
}

// 0x48e6c0 levelrender_draw_list_glide: not written yet (see the head of this file).

// ---- face helpers ----------------------------------------------------------------------------------

// 0x48f8c0  Confidence: likely. Fills levelrender_face_verts from a face: the view-space position and
// clip code by the vertex index copied into a local array, the texture uv, lightmap uv and colour by
// the face's index read again. The colour: grey (sum of the object's vertex RGB, clamped to 0..255,
// times the tint) when the tint is on; the light colour times 255 with a lightmap; without a lightmap
// the object's RGB brightened by the light (rgb - (int)(c * -255), capped at 255 only from above);
// else the object's RGB. Then clips and projects the triangle.
gr_vertex **levelrender_face_vertex_colors(lobj *l, s3d_face *face, int unused, float *lightmap_uv,
                                           ubyte *codes, vector3 *verts, int *num_out,
                                           vector3 *colors, int lit)
{
    int idx[3];
    int i;
    int v;
    int c;
    float sum;
    gr_vertex *gv;
    ubyte *rgb;

    idx[0] = face->v[0];
    idx[1] = face->v[1];
    idx[2] = face->v[2];
    for (i = 0; i < 3; i++) {
        gv = &levelrender_face_verts[i];
        v = face->v[i];
        levelrender_face_vert_list[i] = gv;
        gv->x = verts[idx[i]].x;
        gv->y = verts[idx[i]].y;
        gv->z = verts[idx[i]].z;
        gv->u = l->mesh->uvs[v].x;
        gv->v = l->mesh->uvs[v].y;
        if (lightmap_uv) {
            gv->u2 = lightmap_uv[i];
            gv->v2 = lightmap_uv[i + 3];
        }
        if (gr_tint_enabled) {
            rgb = &l->vertex_rgb[v * 3];
            sum = (float)rgb[2] + (float)rgb[1] + (float)rgb[0];
            if (sum < 0.0f)
                sum = 0.0f;
            else if (sum > 255.0f)
                sum = 255.0f;
            gv->r = (ubyte)(int)(gr_tint_r * sum);
            gv->g = (ubyte)(int)(gr_tint_g * sum);
            gv->b = (ubyte)(int)(gr_tint_b * sum);
        } else if (lit) {
            if (lightmap_uv) {
                gv->r = (ubyte)(int)(colors[v].x * 255.0f);
                gv->g = (ubyte)(int)(colors[v].y * 255.0f);
                gv->b = (ubyte)(int)(colors[v].z * 255.0f);
            } else {
                c = l->vertex_rgb[v * 3] - (int)(colors[v].x * -255.0f);
                if (c >= 255)
                    c = 255;
                gv->r = (ubyte)c;
                c = l->vertex_rgb[v * 3 + 1] - (int)(colors[v].y * -255.0f);
                if (c >= 255)
                    c = 255;
                gv->g = (ubyte)c;
                c = l->vertex_rgb[v * 3 + 2] - (int)(colors[v].z * -255.0f);
                if (c >= 255)
                    c = 255;
                gv->b = (ubyte)c;
            }
        } else {
            gv->r = l->vertex_rgb[v * 3];
            gv->g = l->vertex_rgb[v * 3 + 1];
            gv->b = l->vertex_rgb[v * 3 + 2];
        }
        gv->a = 255;
        gv->_unk1a = 0xf;
        gv->flags = 0;
        gv->index = (ubyte)i;
        gv->codes = codes[idx[i]];
    }
    *num_out = gr_clip_and_project_poly(3, levelrender_face_vert_list);
    return levelrender_face_vert_list;
}

// 0x48fbb0  Confidence: likely (the code sums the z terms first: VC6 reordering, D504; the distance is
// vector3::len expanded). Lights
// closer than 1 count fully; otherwise by the cosine to the normal, faded between the light's near and
// far range when it has one, times intensity * 0.75, each channel's contribution capped at 1, the sums
// capped at 1.
int levelrender_sum_point_lights(vector3 *out, const vector3 *pos, const vector3 *normal, int count,
                                 gr_light **lights)
{
    int n;
    int i;
    gr_light *lt;
    vector3 d;
    float dist;
    float f;

    n = 0;
    out->x = 0;
    out->y = 0;
    out->z = 0;
    for (i = 0; i < count; i++) {
        lt = lights[i];
        d.x = lt->pos.x - pos->x;
        d.y = lt->pos.y - pos->y;
        d.z = lt->pos.z - pos->z;
        dist = d.len();
        if (lt->has_range && dist > lt->range_far)
            continue;
        if (dist < 1.0f)
            f = 1.0f;
        else {
            f = (d.x * normal->x + d.y * normal->y + d.z * normal->z) / dist;
            if (f <= 0.0f)
                continue;
        }
        if (lt->has_range && dist > lt->range_near)
            f *= (lt->range_far - dist) / (lt->range_far - lt->range_near);
        f = f * lt->intensity * 0.75f;
        out->x += min(1.0f, f * lt->r);
        out->y += min(1.0f, f * lt->g);
        out->z += min(1.0f, f * lt->b);
        n++;
    }
    out->x = min(out->x, 1.0f);
    out->y = min(out->y, 1.0f);
    out->z = min(out->z, 1.0f);
    return n;
}

// 0x48fdb0 levelrender_draw_list_d3d, 0x491500 levelrender_draw_list_opengl: not written yet.
