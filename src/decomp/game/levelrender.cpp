// levelrender.cpp - Sum.exe v1.40, code 0x48db30-0x492af0 (Engine\rendering\levelrender.cpp, path
// literal 0x59c524). Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0
// (cl 12.00.8966). Confidence per function: exact = expect identical code, likely = same semantics,
// small layout risk, unsure = the source shape is a guess.
//
// Unit range: sum.symbols tags 39 functions from 0x48db30 (init_array_23df188_thunk) to 0x492af0
// (atexit_nop_492af0) with levelrender.cpp. Optimised build (/O2).
//
// decomp76 wrote the three back-end draw lists (levelrender_draw_list_glide, _d3d and _opengl, each with
// two function-local static vector3 arrays whose empty atexit handlers are the compiler's) and their
// .bss, and replaced the DirectX 7 SDK's <d3d.h> with vsdk/gr/directx/d3d_min.h, so that the unit
// compiles with VC6 (D5800).

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <windows.h>
#include "vsdk/gr/directx/gr_direct3d.h"   // d3d_min.h, not the DirectX 7 SDK's <d3d.h> (D5800)
#include "vsdk/gr/glide/gr_glide.h"
#include "vsdk/gr/opengl/gr_opengl.h"
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
// The gr_info timing counters FrameEnd prints and resets (summoner.h; D3104): this unit's .bss by address,
// five right after levelrender_gl_vert_lit (0x2395cf0 + 4000) and two right after levelrender_gl_vertex_array
// (0x2419570 + 4512 * 12). Only the reset writes the first five; the last two are written as esi-indexed
// elements by levelrender_draw_list_opengl (game-bugs.md).
int gr_info_total_time;                       // 0x2396c90
int gr_info_backface_time;                    // 0x2396c94
int gr_info_point_rotation_time;              // 0x2396c98
int gr_info_everything_else_time;             // 0x2396c9c
int gr_info_polygon_time;                     // 0x2396ca0
int gr_info_smooth_lighting_time;             // 0x24268f0
int gr_info_zsort_time;                       // 0x24268f4

// The inline constructor of attached_light (reset()) over the 16 records (D805).
// 0x48db30 init_array_23df188_thunk (jmp, static initialiser entry 253). Confidence: exact.
// 0x48db40 init_array_23df188. Confidence: exact.
attached_light levelrender_lights[16];        // 0x23df188 (name ours; not referenced by address elsewhere)

// Static initialiser entries 254-259: six thunks to empty initialisers, i.e. six objects with empty
// inline constructors (vector3 arrays, D500) defined here. Two of them are probably the wave arrays
// below; the other four are among the draw lists' vector3 arrays: decomp76 defines two more globals
// (levelrender_view_verts, levelrender_gl_vertex_array) and makes the six per-function projected and
// colour arrays function-local statics (the three guards); two remain open (D5803).
// The assignment of the six to objects is unsure:
// 0x48db60 init_empty_48db70_thunk (jmp, entry 254). Confidence: exact.
// 0x48db70 init_empty_48db70 (a bare ret). Confidence: unsure (which object).
// 0x48db80 init_empty_48db90_thunk (jmp, entry 255). Confidence: exact.
// 0x48db90 init_empty_48db90 (a bare ret). Confidence: unsure (which object).
// 0x48dba0 init_empty_48dbb0_thunk (jmp, entry 256). Confidence: exact.
// 0x48dbb0 init_empty_48dbb0 (a bare ret). Confidence: unsure (which object).
// 0x48dbc0 init_empty_48dbd0_thunk (jmp, entry 257). Confidence: exact.
// 0x48dbd0 init_empty_48dbd0 (a bare ret). Confidence: unsure (which object).
// 0x48dbe0 init_empty_48dbf0_thunk (jmp, entry 258). Confidence: exact.
// 0x48dbf0 init_empty_48dbf0 (a bare ret). Confidence: unsure (which object).
// 0x48dc00 init_empty_48dc10_thunk (jmp, entry 259). Confidence: exact.
// 0x48dc10 init_empty_48dc10 (a bare ret). Confidence: unsure (which object).
vector3 levelrender_wave_vertex_normals[LEVELRENDER_MAX_WAVE_NORMALS];   // 0x2396ca8
vector3 levelrender_wave_vertices[LEVELRENDER_MAX_VERTS];               // 0x23ec658

// ---- console commands --------------------------------------------------------------------------------
void cmd_time_mult();
void cmd_space_mult();
void cmd_small_mesh_threshold();

// 0x48dc20 init_ccmd_time_mult_thunk (jmp). Confidence: exact.
// 0x48dc30 init_ccmd_time_mult (console_command constructor). Confidence: exact.
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

// 0x48dce0 init_ccmd_space_mult_thunk (jmp). Confidence: exact.
// 0x48dcf0 init_ccmd_space_mult (console_command constructor). Confidence: exact.
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

// 0x48dda0 init_ccmd_small_mesh_threshold_thunk (jmp). Confidence: exact.
// 0x48ddb0 init_ccmd_small_mesh_threshold (console_command constructor). Confidence: exact.
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

// 0x48e440 vector3::operator+=(const vector3 &) (vsdk/math/vector.h): the COMDAT of the inline operator,
// emitted after its first out-of-line use (levelrender_wave_lobj). Confidence: exact (compiler-generated
// from the header).

// 0x48e460 atexit_nop_48e460: the empty atexit destructor of levelrender_wave_lobj's static GameTimer
// (0x23eb688; GameTimer has no destructor, so the guard's handler is a bare ret). Confidence: exact
// (compiler-generated).

// ---- Direct3D vertex buffer ----------------------------------------------------------------------

// 0x48e470  Confidence: exact (VC6 byte match, match11; was likely) (the descriptor's store order). 8512 vertices of FVF 0x2c4 (XYZRHW,
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
        if (IDirect3D7_CreateVertexBuffer(D3D_DIRECT3D, &desc,
                (IDirect3DVertexBuffer7 **)&levelrender_d3d_vertex_buffer, 0) < 0)
            VSDK_ERROR(LEVELRENDER_CPP, 0x450, "Unable to create level vertex buffer for D3d!");
    }
}

// 0x48e4f0  Confidence: exact. Caller levelrender_level_unload.
void levelrender_free_d3d_vertex_buffer()
{
    if (gr_screen.mode == 0x66 && levelrender_d3d_vertex_buffer) {
        ((IDirect3DVertexBuffer7 *)levelrender_d3d_vertex_buffer)->lpVtbl->Release(
            (IDirect3DVertexBuffer7 *)levelrender_d3d_vertex_buffer);
        levelrender_d3d_vertex_buffer = NULL;
    }
}

// ---- level lights --------------------------------------------------------------------------------

// 0x48e520  Confidence: exact (VC6 byte match, match11; was likely). The level ambient and lights, scaled by level_light_scale. Spot lights
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
                      level_light_scale * level_s3d.ambient.z);
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
            if (lt->texture != -1) {
                if (lt->texture < 0 || lt->texture >= level_num_textures)
                    break;
                bm = level_texture_bms[lt->texture];
            } else
                bm = -1;
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

// ---- the draw lists (decomp76, D5801-D5804) ------------------------------------------------------
// levelrender_draw_list (0x4858f0) calls one of the three back ends by gr_screen.mode after it has entered
// the level object's frame. Each walks the geometry's render groups (s3d_render_group: the faces and
// vertices are stored group after group, one texture per group) and has two paths:
//  - lightmapped (no tint, the first face has a lightmap, flag 0x80000 clear): texture stage 0 the group
//    texture, stage 1 the face's lightmap; per-vertex point light colours are added in a second additive
//    pass (gr_mode_level_alpha_add) when any vertex of the batch was lit;
//  - plain (tint, or no lightmap): flat or textured alpha mode by the group's texture, the vertex colour
//    from the object's vertex RGB (grey times the tint, or brightened by the point lights).
// Every vertex is transformed once per group (a vertex whose vertex_dup is >= 0 copies an earlier one),
// coded against the frustum and projected when unclipped; a face with a clipped corner goes through
// levelrender_face_vertex_colors (gr_clip_and_project_poly), the others use the projected vertices.
// The fog alpha uses the alpha fog range for small meshes (radius <= Small_mesh_threshold) and the vertex
// range for the others; objects with flag 0x200400 get their 1/z pushed back by a tenth of the far clip.

// The per-vertex arrays (4000 entries: LEVELRENDER_MAX_VERTS; the Glide polygon and OpenGL client arrays
// hold 4512, 0x11a0, the spacing of their neighbours). sum.symbols calls the clip-code bytes "vert_lit";
// they hold gr_code_point's codes of each vertex (D5802).
#define LEVELRENDER_MAX_POLY_VERTS 4512
#define LEVELRENDER_MAX_INDICES 9000          // the two index lists (0x4650 bytes between neighbours)
#define LEVELRENDER_D3D_VB_LIMIT 8000         // 0x1f40 (the vertex buffer holds 0x2140)
#define LEVELRENDER_GL_BATCH_LIMIT 4000       // 0xfa0
#define LEVELRENDER_SNAP 786432.0f            // 0x581ad0, gr_glide.cpp's GLIDE_SNAP

vector3 levelrender_view_verts[LEVELRENDER_MAX_VERTS];   // 0x2400ec8 view-space vertices of all three (name ours)
GrVertex levelrender_glide_poly_verts[LEVELRENDER_MAX_POLY_VERTS];   // 0x2319070
ubyte levelrender_glide_vert_lit[LEVELRENDER_MAX_VERTS];             // 0x2437a74 clip codes
ubyte levelrender_glide_vert_point_light[LEVELRENDER_MAX_VERTS];     // 0x240ca48
ubyte levelrender_d3d_vert_lit[LEVELRENDER_MAX_VERTS];               // 0x2432484 clip codes
ubyte levelrender_d3d_vert_point_light[LEVELRENDER_MAX_VERTS];       // 0x23eb690
ushort levelrender_d3d_indices[LEVELRENDER_MAX_INDICES];             // 0x2433424 (name ours)
int levelrender_d3d_vertex_buffer_pos;                               // 0x240d9e8
ubyte levelrender_gl_vert_lit[LEVELRENDER_MAX_VERTS];                // 0x2395cf0 clip codes (name ours)
ubyte levelrender_gl_vert_point_light[LEVELRENDER_MAX_VERTS];        // 0x23c5b3c
float levelrender_gl_texcoords0[LEVELRENDER_MAX_POLY_VERTS][4];      // 0x2378770
float levelrender_gl_texcoords1[LEVELRENDER_MAX_POLY_VERTS][4];      // 0x2366d70
ubyte levelrender_gl_color_array[LEVELRENDER_MAX_POLY_VERTS][4];     // 0x23fc844 r, g, b, a
vector3 levelrender_gl_vertex_array[LEVELRENDER_MAX_POLY_VERTS];     // 0x2419570
ushort levelrender_gl_indices[LEVELRENDER_MAX_INDICES];              // 0x23f81e4 (name ours)
bool levelrender_lights_dirty;                // 0x243e038
bool levelrender_lights_initialized;          // 0x243e039

// The draw lists' inline pieces (macros ours). The clip codes are gr_code_point's (0x50e810) expanded.
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
// The screen position of an unclipped view-space point (w = 1 / z).
#define LEVELRENDER_PROJECT_XY(p, w)                                                                   \
    {                                                                                                  \
        (p).x = ((w) * (p).x + 1.0f) * gr_projection_xmul + gr_screen.offset_x;                        \
        (p).y = (1.0f - (w) * (p).y) * gr_projection_ymul + gr_screen.offset_y;                        \
    }
// The alpha after the object's fog range (locals fog_near, fog_far, fog_inv_range and the alpha argument).
#define LEVELRENDER_FOG_ALPHA(z)                                                                       \
    ((z) > fog_near ? (int)(alpha * float_max(0.0f, (fog_far - (z)) * fog_inv_range)) : alpha)
// 1 / z pushed back by a tenth of the far clip (level objects with flag 0x200400).
#define LEVELRENDER_FAR_FADE(z) (1.0f / ((z) - gr_scale_by_far_clip(0.1f)))
#define LEVELRENDER_VB ((IDirect3DVertexBuffer7 *)levelrender_d3d_vertex_buffer)

// gr_calc_depth (unnamed_gr_3d_0x50d9b0.h) with the products in the other order: several draw-list sites
// multiply the depth scale by w first (fld gr_depth_scale; fmul w; fmul Gr_3d_zmult). Inline, ours.
inline float levelrender_depth(float w)
{
    float z = gr_depth_scale * w * Gr_3d_zmult;
    if (Gr_3d_zmax < z)
        z = Gr_3d_zmax;
    return z;
}

// 0x48e6c0  Confidence: likely (unsure for the function-local statics, D5803). Nothing is drawn while the
// Glide window is closed. Lightmapped: one grDrawPlanarPolygonVertexList per face from the projected
// vertices (or the clipped polygon), a second additive pass when the face's corners had point lights.
// Plain: the projected vertices stay at their vertex index in levelrender_glide_poly_verts, clipped
// polygons append theirs after the mesh's vertices (never reset for the object, D5804), and each face is
// one grDrawPlanarPolygon of indices.
void levelrender_draw_list_glide(lobj *l, int alpha, unsigned int num_lights, attached_light **lights)
{
    bool far_fade;
    bool same_uv;
    vector3 *verts;
    float u_scale, v_scale;
    float lm_u_scale, lm_v_scale;
    float fog_near, fog_far, fog_inv_range;
    int n;
    int vbase;
    int face_base;
    int next;
    int g;
    int i;
    int f;
    int k;
    int v;
    int cur;
    int flat;
    int bm;
    int lit_sum;
    bool clipped;
    ubyte c, c0, c1, c2;
    s3d_render_group *grp;
    short *dup;
    ubyte *rgb;
    s3d_face *face;
    lobj_face *lf;
    s3d_box *box;
    gr_vertex **clip;
    gr_vertex *gv;
    GrVertex *o;
    vector3 p;
    int idx[28];
    float s;

    if (glide_close_on_deactivate == 1 && glide_pending_deactivations == -1)
        return;
    static vector3 levelrender_glide_proj_verts[LEVELRENDER_MAX_VERTS];    // 0x23c7a80 (guard 0x23eb68c)
    static vector3 levelrender_glide_vert_colors[LEVELRENDER_MAX_VERTS];   // 0x23d3608
    far_fade = (l->flags & 0x200400) != 0;
    if (l->flags & 0x20) {
        levelrender_wave_lobj(l, levelrender_wave_vertices);
        verts = levelrender_wave_vertices;
    } else
        verts = l->mesh->vertices;
    u_scale = 0;
    v_scale = 0;
    lm_u_scale = 0;
    lm_v_scale = 0;
    n = 0;
    vbase = 0;
    same_uv = (l->flags >> 23) & 1;
    fog_inv_range = gr_fog_alpha_inv_range;
    face_base = 0;
    clip = NULL;
    if (l->mesh->_unk30 <= Small_mesh_threshold) {
        fog_near = gr_fog_alpha_near;
        fog_far = gr_fog_alpha_far;
    } else {
        fog_near = gr_fog_vertex_near;
        fog_far = gr_fog_vertex_far;
    }

    if (!gr_tint_enabled && l->faces->lightmap >= 0 && !(l->flags & 0x80000)) {
        cur = -1;
        gr_glide_set_mode(gr_mode_lightmap_multitexture);
        for (g = 0; g < l->mesh->num_groups; g++) {
            grp = &l->mesh->groups[g];
            gr_glide_tcache_set(0, level_texture_bms[l->faces[face_base].texture - 1], 0, &u_scale, &v_scale, 1);
            dup = &l->mesh->vertex_dup[vbase];
            for (i = vbase; i < vbase + grp->num_vertices; i++, dup++) {
                if (*dup >= 0) {
                    levelrender_glide_vert_lit[i] = levelrender_glide_vert_lit[i - *dup];
                    levelrender_view_verts[i] = levelrender_view_verts[i - *dup];
                    if (num_lights) {
                        levelrender_glide_vert_colors[i] = levelrender_glide_vert_colors[i - *dup];
                        levelrender_glide_vert_point_light[i] = levelrender_glide_vert_point_light[i - *dup];
                    }
                    if (!levelrender_glide_vert_lit[i])
                        levelrender_glide_proj_verts[i] = levelrender_glide_proj_verts[i - *dup];
                } else {
                    if (num_lights)
                        levelrender_glide_vert_point_light[i] = (ubyte)levelrender_sum_point_lights(
                            &levelrender_glide_vert_colors[i], &verts[i], &l->mesh->normals[i], num_lights,
                            (gr_light **)lights);
                    levelrender_glide_proj_verts[i] = verts[i] - gr_view_pos;
                    levelrender_glide_proj_verts[i].rotate(gr_view_matrix);
                    levelrender_view_verts[i] = levelrender_glide_proj_verts[i];
                    LEVELRENDER_CODE_POINT(c, levelrender_glide_proj_verts[i]);
                    levelrender_glide_vert_lit[i] = c;
                    if (!c) {
                        float w = 1.0f / levelrender_glide_proj_verts[i].z;
                        LEVELRENDER_PROJECT_XY(levelrender_glide_proj_verts[i], w);
                        if (far_fade)
                            levelrender_glide_proj_verts[i].z = LEVELRENDER_FAR_FADE(levelrender_glide_proj_verts[i].z);
                        else
                            levelrender_glide_proj_verts[i].z = w;
                    }
                }
            }
            for (f = face_base; f < face_base + grp->num_faces; f++) {
                lf = &l->faces[f];
                box = &l->lightmap_boxes[f];
                face = &l->mesh->faces[f];
                if (!(face->flags & 0x200) && !gr_check_normal_facing(&face->normal, &verts[face->v[0]]))
                    continue;
                c0 = levelrender_glide_vert_lit[face->v[0]];
                c1 = levelrender_glide_vert_lit[face->v[1]];
                c2 = levelrender_glide_vert_lit[face->v[2]];
                if (c0 & c1 & c2)
                    continue;
                lit_sum = 0;
                if (c0 | c1 | c2) {
                    n = 3;
                    clip = levelrender_face_vertex_colors(l, face, (int)lf, (float *)box, levelrender_glide_vert_lit,
                                                          levelrender_view_verts, &n,
                                                          levelrender_glide_vert_colors, num_lights);
                    if (n < 3)
                        continue;
                    if (num_lights)
                        lit_sum = levelrender_glide_vert_point_light[face->v[0]]
                                  + levelrender_glide_vert_point_light[face->v[1]]
                                  + levelrender_glide_vert_point_light[face->v[2]];
                    clipped = true;
                } else
                    clipped = false;
                bm = level_face_bitmaps[lf->lightmap];
                if (bm != cur) {
                    gr_glide_tcache_set(1, bm, 0, &lm_u_scale, &lm_v_scale, 1);
                    cur = bm;
                }
                if (clipped) {
                    for (k = 0; k < n; k++) {
                        gv = clip[k];
                        o = &levelrender_glide_poly_verts[k];
                        o->x = gv->sx + LEVELRENDER_SNAP;
                        o->y = gv->sy + LEVELRENDER_SNAP;
                        if (far_fade)
                            gv->sw = LEVELRENDER_FAR_FADE(gv->z);
                        o->oow = gv->sw;
                        if (num_lights) {
                            o->r = gv->r;
                            o->g = gv->g;
                            o->b = gv->b;
                        }
                        o->a = (float)LEVELRENDER_FOG_ALPHA(gv->z);
                        o->tmuvtx[0].sow = u_scale * gv->u * o->oow;
                        o->tmuvtx[0].tow = v_scale * gv->v * o->oow;
                        o->tmuvtx[1].sow = lm_u_scale * gv->u2 * o->oow;
                        o->tmuvtx[1].tow = lm_v_scale * gv->v2 * o->oow;
                    }
                } else {
                    n = 3;
                    for (k = 0; k < 3; k++) {
                        v = face->v[k];
                        o = &levelrender_glide_poly_verts[k];
                        o->x = levelrender_glide_proj_verts[v].x + LEVELRENDER_SNAP;
                        o->oow = levelrender_glide_proj_verts[v].z;
                        o->y = levelrender_glide_proj_verts[v].y + LEVELRENDER_SNAP;
                        if (num_lights) {
                            o->r = levelrender_glide_vert_colors[v].x * 255.0f;
                            o->g = levelrender_glide_vert_colors[v].y * 255.0f;
                            o->b = levelrender_glide_vert_colors[v].z * 255.0f;
                            lit_sum += levelrender_glide_vert_point_light[v];
                        }
                        o->a = (float)LEVELRENDER_FOG_ALPHA(levelrender_view_verts[v].z);
                        o->tmuvtx[0].sow = l->mesh->uvs[v].x * o->oow * u_scale;
                        o->tmuvtx[0].tow = l->mesh->uvs[v].y * o->oow * v_scale;
                        o->tmuvtx[1].sow = lm_u_scale * o->oow * ((float *)box)[k];
                        o->tmuvtx[1].tow = ((float *)box)[k + 3] * o->oow * lm_v_scale;
                    }
                }
                glide_grDrawPlanarPolygonVertexList(n, levelrender_glide_poly_verts);
                if (num_lights && lit_sum) {
                    gr_glide_set_mode(gr_mode_level_alpha_add);
                    glide_grDrawPlanarPolygonVertexList(n, levelrender_glide_poly_verts);
                    gr_glide_set_mode(gr_mode_lightmap_multitexture);
                }
            }
            face_base += grp->num_faces;
            vbase += grp->num_vertices;
        }
        return;
    }

    next = l->mesh->num_vertices;
    cur = -1;
    for (g = 0; g < l->mesh->num_groups; g++) {
        grp = &l->mesh->groups[g];
        flat = !l->faces[face_base].texture;
        if (flat != cur) {
            gr_glide_set_mode(l->faces[face_base].texture ? gr_mode_mesh_alpha : gr_mode_flat_alpha);
            cur = flat;
        }
        if (l->faces[face_base].texture) {
            gr_glide_tcache_set(0, level_texture_bms[l->faces[face_base].texture - 1], 0, &u_scale, &v_scale, 1);
            if (same_uv)
                v_scale = u_scale;
        }
        dup = &l->mesh->vertex_dup[vbase];
        rgb = &l->vertex_rgb[vbase * 3];
        for (i = vbase; i < vbase + grp->num_vertices; i++, dup++, rgb += 3) {
            o = &levelrender_glide_poly_verts[i];
            if (*dup >= 0) {
                levelrender_glide_vert_lit[i] = levelrender_glide_vert_lit[i - *dup];
                levelrender_view_verts[i] = levelrender_view_verts[i - *dup];
                if (num_lights)
                    levelrender_glide_vert_colors[i] = levelrender_glide_vert_colors[i - *dup];
                if (levelrender_glide_vert_lit[i])
                    continue;
                o->x = o[-*dup].x;
                o->y = o[-*dup].y;
                o->oow = o[-*dup].oow;
            } else {
                // The light count is not kept on this path (the vertex colours are).
                if (num_lights)
                    levelrender_sum_point_lights(&levelrender_glide_vert_colors[i], &verts[i], &l->mesh->normals[i],
                                                 num_lights, (gr_light **)lights);
                p = verts[i] - gr_view_pos;
                p.rotate(gr_view_matrix);
                levelrender_view_verts[i] = p;
                LEVELRENDER_CODE_POINT(c, p);
                levelrender_glide_vert_lit[i] = c;
                if (c)
                    continue;
                float w = 1.0f / p.z;
                LEVELRENDER_PROJECT_XY(p, w);
                if (far_fade)
                    p.z = LEVELRENDER_FAR_FADE(p.z);
                else
                    p.z = w;
                o->x = p.x + LEVELRENDER_SNAP;
                o->y = p.y + LEVELRENDER_SNAP;
                o->oow = p.z;
            }
            o->tmuvtx[0].sow = l->mesh->uvs[i].x * o->oow * u_scale;
            o->tmuvtx[0].tow = l->mesh->uvs[i].y * o->oow * v_scale;
            if (gr_tint_enabled) {
                s = (float)(rgb[0] + rgb[1] + rgb[2]);
                if (s < 0.0f)
                    s = 0.0f;
                else if (s > 255.0f)
                    s = 255.0f;
                o->r = s * gr_tint_r;
                o->g = s * gr_tint_g;
                o->b = s * gr_tint_b;
            } else if (num_lights) {
                o->r = min(levelrender_glide_vert_colors[i].x * 255.0f + rgb[0], 255.0f);
                o->g = min(levelrender_glide_vert_colors[i].y * 255.0f + rgb[1], 255.0f);
                o->b = min(levelrender_glide_vert_colors[i].z * 255.0f + rgb[2], 255.0f);
            } else {
                o->r = rgb[0];
                o->g = rgb[1];
                o->b = rgb[2];
            }
            o->a = (float)LEVELRENDER_FOG_ALPHA(levelrender_view_verts[i].z);
        }
        for (f = face_base; f < face_base + grp->num_faces; f++) {
            face = &l->mesh->faces[f];
            lf = &l->faces[f];
            if (!(face->flags & 0x200) && !gr_check_normal_facing(&face->normal, &verts[face->v[0]]))
                continue;
            c0 = levelrender_glide_vert_lit[face->v[0]];
            c1 = levelrender_glide_vert_lit[face->v[1]];
            c2 = levelrender_glide_vert_lit[face->v[2]];
            if (c0 & c1 & c2)
                continue;
            n = 3;
            if (c0 | c1 | c2) {
                clip = levelrender_face_vertex_colors(l, face, (int)lf, NULL, levelrender_glide_vert_lit,
                                                      levelrender_view_verts, &n, levelrender_glide_vert_colors,
                                                      num_lights);
                if (n < 3)
                    continue;
                for (k = 0; k < n; k++) {
                    gv = clip[k];
                    if (gv->flags & 4) {
                        idx[k] = next;
                        o = &levelrender_glide_poly_verts[next++];
                        o->x = gv->sx + LEVELRENDER_SNAP;
                        o->y = gv->sy + LEVELRENDER_SNAP;
                        if (far_fade)
                            gv->sw = LEVELRENDER_FAR_FADE(gv->z);
                        o->oow = gv->sw;
                        o->r = gv->r;
                        o->g = gv->g;
                        o->b = gv->b;
                        o->a = (float)LEVELRENDER_FOG_ALPHA(gv->z);
                        o->tmuvtx[0].sow = gv->u * o->oow * u_scale;
                        o->tmuvtx[0].tow = gv->v * o->oow * v_scale;
                    } else
                        idx[k] = face->v[gv->index];
                }
            } else {
                idx[0] = face->v[0];
                idx[1] = face->v[1];
                idx[2] = face->v[2];
            }
            glide_grDrawPlanarPolygon(n, idx, levelrender_glide_poly_verts);
        }
        face_base += grp->num_faces;
        vbase += grp->num_vertices;
    }
}

// 0x48fd90 atexit_nop_48fd90 and
// 0x48fda0 atexit_nop_48fda0: the empty atexit destructors of the Glide draw list's two function-local
//          statics (guard 0x23eb68c). Confidence: exact (compiler-generated, bare rets).

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

// 0x48fdb0  Confidence: likely (unsure for the function-local statics, D5803). The vertices go into
// levelrender_d3d_vertex_buffer from levelrender_d3d_vertex_buffer_pos on (wrapped to 0 with a discarding
// lock once it reaches 8000 at entry). Lightmapped: each face's triangles (the clipped polygon as a fan,
// levelrender_fan_tables) are appended to a batch that is drawn with DrawPrimitiveVB when the lightmap
// changes or the buffer would pass 8000 (the batch then restarts with a discarding lock, D5804), and at
// the end of each group. Plain: the group's vertices are written at their vertex index, clipped polygons
// append theirs after the mesh's vertices, and each group is one DrawIndexedPrimitiveVB of
// levelrender_d3d_indices. Lock failures loop on DebugError (lines 0x6d9, 0x727, 0x7b1).
void levelrender_draw_list_d3d(lobj *l, int alpha, unsigned int num_lights, attached_light **lights)
{
    static vector3 levelrender_d3d_proj_verts[LEVELRENDER_MAX_VERTS];    // 0x23dfb08 (guard 0x23d3600)
    static vector3 levelrender_d3d_vert_colors[LEVELRENDER_MAX_VERTS];   // 0x235b1f0
    bool far_fade;
    bool clipped;
    vector3 *verts;
    float u_scale, v_scale;
    float fog_near, fog_far, fog_inv_range;
    float z;
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
    DWORD lock_flags;
    HRESULT hr;
    ubyte c, c0, c1, c2;
    s3d_render_group *grp;
    short *dup;
    ubyte *rgb;
    vector2 *uv;
    s3d_face *face;
    lobj_face *lf;
    s3d_box *box;
    gr_vertex **clip;
    gr_vertex *gv;
    vector3 *pv;
    vector3 *vv;
    d3d_tlvertex *data;
    d3d_tlvertex *o;
    ushort *ip;
    float s;

    far_fade = (l->flags & 0x200400) != 0;
    fog_inv_range = gr_fog_alpha_inv_range;
    if (l->mesh->_unk30 <= Small_mesh_threshold) {
        fog_near = gr_fog_alpha_near;
        fog_far = gr_fog_alpha_far;
    } else {
        fog_near = gr_fog_vertex_near;
        fog_far = gr_fog_vertex_far;
    }
    if (l->flags & 0x20) {
        levelrender_wave_lobj(l, levelrender_wave_vertices);
        verts = levelrender_wave_vertices;
    } else
        verts = l->mesh->vertices;
    u_scale = 0;
    v_scale = 0;
    vbase = 0;
    face_base = 0;
    clip = NULL;
    n = 0;

    if (!gr_tint_enabled && l->faces->lightmap >= 0 && !(l->flags & 0x80000)) {
        if (levelrender_d3d_vertex_buffer_pos >= LEVELRENDER_D3D_VB_LIMIT) {
            levelrender_d3d_vertex_buffer_pos = 0;
            lock_flags = gr_d3d_lock_discard;
        } else if (levelrender_d3d_vertex_buffer_pos == 0)
            lock_flags = gr_d3d_lock_discard;
        else
            lock_flags = gr_d3d_lock_nooverwrite;
        cur = -1;
        start = 0;
        count = 0;
        gr_d3d_set_state(gr_mode_lightmap_multitexture);
        for (g = 0; g < l->mesh->num_groups; g++) {
            grp = &l->mesh->groups[g];
            gr_d3d_tcache_set(0, level_texture_bms[l->faces[face_base].texture - 1], 0, &u_scale, &v_scale, 1);
            dup = &l->mesh->vertex_dup[vbase];
            for (i = vbase; i < vbase + grp->num_vertices; i++, dup++) {
                if (*dup >= 0) {
                    levelrender_d3d_vert_lit[i] = levelrender_d3d_vert_lit[i - *dup];
                    levelrender_view_verts[i] = levelrender_view_verts[i - *dup];
                    if (num_lights) {
                        levelrender_d3d_vert_colors[i] = levelrender_d3d_vert_colors[i - *dup];
                        levelrender_d3d_vert_point_light[i] = levelrender_d3d_vert_point_light[i - *dup];
                    }
                    if (!levelrender_d3d_vert_lit[i])
                        levelrender_d3d_proj_verts[i] = levelrender_d3d_proj_verts[i - *dup];
                } else {
                    if (num_lights)
                        levelrender_d3d_vert_point_light[i] = (ubyte)levelrender_sum_point_lights(
                            &levelrender_d3d_vert_colors[i], &verts[i], &l->mesh->normals[i], num_lights,
                            (gr_light **)lights);
                    levelrender_d3d_proj_verts[i] = verts[i] - gr_view_pos;
                    levelrender_d3d_proj_verts[i].rotate(gr_view_matrix);
                    levelrender_view_verts[i] = levelrender_d3d_proj_verts[i];
                    LEVELRENDER_CODE_POINT(c, levelrender_d3d_proj_verts[i]);
                    levelrender_d3d_vert_lit[i] = c;
                    if (!c) {
                        float w = 1.0f / levelrender_d3d_proj_verts[i].z;
                        LEVELRENDER_PROJECT_XY(levelrender_d3d_proj_verts[i], w);
                        if (far_fade)
                            levelrender_d3d_proj_verts[i].z = LEVELRENDER_FAR_FADE(levelrender_d3d_proj_verts[i].z);
                        else
                            levelrender_d3d_proj_verts[i].z = w;
                    }
                }
            }
            hr = IDirect3DVertexBuffer7_Lock(LEVELRENDER_VB, lock_flags, (void **)&data, NULL);
            lock_flags = gr_d3d_lock_nooverwrite;
            if (hr < 0)
                VSDK_ERROR(LEVELRENDER_CPP, 0x6d9, ddraw_error_string(hr));
            data += levelrender_d3d_vertex_buffer_pos;
            lit_sum = 0;
            o = &data[count];
            for (f = face_base; f < face_base + grp->num_faces; f++) {
                lf = &l->faces[f];
                box = &l->lightmap_boxes[f];
                face = &l->mesh->faces[f];
                if (!(face->flags & 0x200) && !gr_check_normal_facing(&face->normal, &verts[face->v[0]]))
                    continue;
                c0 = levelrender_d3d_vert_lit[face->v[0]];
                c1 = levelrender_d3d_vert_lit[face->v[1]];
                c2 = levelrender_d3d_vert_lit[face->v[2]];
                if (c0 & c1 & c2)
                    continue;
                face_count = 3;
                if (c0 | c1 | c2) {
                    n = 3;
                    clip = levelrender_face_vertex_colors(l, face, (int)lf, (float *)box, levelrender_d3d_vert_lit,
                                                          levelrender_view_verts, &n,
                                                          levelrender_d3d_vert_colors, num_lights);
                    if (n < 3 || n > 8)
                        continue;
                    clipped = true;
                    face_count = 3 * (n - 2);
                } else
                    clipped = false;
                bm = level_face_bitmaps[lf->lightmap];
                if ((bm != cur && count > start)
                    || levelrender_d3d_vertex_buffer_pos + face_count + count > LEVELRENDER_D3D_VB_LIMIT) {
                    IDirect3DVertexBuffer7_Unlock(LEVELRENDER_VB);
                    IDirect3DDevice7_DrawPrimitiveVB(d3d_device, D3DPT_TRIANGLELIST, LEVELRENDER_VB,
                                                     levelrender_d3d_vertex_buffer_pos + start, count - start, 0);
                    if (num_lights && lit_sum) {
                        gr_d3d_set_state(gr_mode_level_alpha_add);
                        IDirect3DDevice7_DrawPrimitiveVB(d3d_device, D3DPT_TRIANGLELIST, LEVELRENDER_VB,
                                                         levelrender_d3d_vertex_buffer_pos + start, count - start, 0);
                        gr_d3d_set_state(gr_mode_lightmap_multitexture);
                    }
                    if (levelrender_d3d_vertex_buffer_pos + face_count + count > LEVELRENDER_D3D_VB_LIMIT) {
                        lock_flags = gr_d3d_lock_discard;
                        count = 0;
                    }
                    start = count;
                    lit_sum = 0;
                    hr = IDirect3DVertexBuffer7_Lock(LEVELRENDER_VB, lock_flags, (void **)&data, NULL);
                    lock_flags = gr_d3d_lock_nooverwrite;
                    if (hr < 0)
                        VSDK_ERROR(LEVELRENDER_CPP, 0x727, ddraw_error_string(hr));
                    data += levelrender_d3d_vertex_buffer_pos;
                    o = &data[count];
                }
                if (num_lights)
                    lit_sum += levelrender_d3d_vert_point_light[face->v[0]] + levelrender_d3d_vert_point_light[face->v[1]]
                               + levelrender_d3d_vert_point_light[face->v[2]];
                if (bm != cur) {
                    gr_d3d_tcache_set(1, bm, 0, &u_scale, &v_scale, 1);
                    cur = bm;
                }
                if (clipped) {
                    fan = levelrender_fan_tables[n];
                    for (k = 0; k < face_count; k++, o++) {
                        gv = clip[fan[k]];
                        o->sx = gv->sx;
                        o->sy = gv->sy;
                        if (far_fade)
                            gv->sw = LEVELRENDER_FAR_FADE(gv->z);
                        o->sz = gr_calc_depth(gv->sw);
                        o->rhw = gv->sw;
                        o->color_bytes[3] = (ubyte)LEVELRENDER_FOG_ALPHA(gv->z);
                        if (num_lights) {
                            o->color_bytes[0] = gv->b;
                            o->color_bytes[1] = gv->g;
                            o->color_bytes[2] = gv->r;
                        }
                        o->specular_bytes[3] = (ubyte)gr_get_fog_alpha(gv->z);
                        o->tu = gv->u;
                        o->tv = gv->v;
                        o->tu2 = gv->u2;
                        o->tv2 = gv->v2;
                    }
                } else {
                    for (k = 0; k < 3; k++, o++) {
                        v = face->v[k];
                        o->sx = levelrender_d3d_proj_verts[v].x;
                        o->sy = levelrender_d3d_proj_verts[v].y;
                        o->sz = levelrender_depth(levelrender_d3d_proj_verts[v].z);
                        o->rhw = levelrender_d3d_proj_verts[v].z;
                        o->color_bytes[3] = (ubyte)LEVELRENDER_FOG_ALPHA(levelrender_view_verts[v].z);
                        if (num_lights) {
                            o->color_bytes[0] = (ubyte)(int)(levelrender_d3d_vert_colors[v].z * 255.0f);
                            o->color_bytes[1] = (ubyte)(int)(levelrender_d3d_vert_colors[v].y * 255.0f);
                            o->color_bytes[2] = (ubyte)(int)(levelrender_d3d_vert_colors[v].x * 255.0f);
                        }
                        o->specular_bytes[3] = (ubyte)gr_get_fog_alpha(levelrender_view_verts[v].z);
                        o->tu = l->mesh->uvs[v].x;
                        o->tv = l->mesh->uvs[v].y;
                        o->tu2 = ((float *)box)[k];
                        o->tv2 = ((float *)box)[k + 3];
                    }
                }
                count += face_count;
            }
            IDirect3DVertexBuffer7_Unlock(LEVELRENDER_VB);
            if (count > start) {
                IDirect3DDevice7_DrawPrimitiveVB(d3d_device, D3DPT_TRIANGLELIST, LEVELRENDER_VB,
                                                 levelrender_d3d_vertex_buffer_pos + start, count - start, 0);
                if (num_lights && lit_sum) {
                    gr_d3d_set_state(gr_mode_level_alpha_add);
                    IDirect3DDevice7_DrawPrimitiveVB(d3d_device, D3DPT_TRIANGLELIST, LEVELRENDER_VB,
                                                     levelrender_d3d_vertex_buffer_pos + start, count - start, 0);
                    gr_d3d_set_state(gr_mode_lightmap_multitexture);
                }
                start = count;
            }
            face_base += grp->num_faces;
            vbase += grp->num_vertices;
        }
        levelrender_d3d_vertex_buffer_pos += count;
        return;
    }

    count = l->mesh->num_vertices;
    if (count + levelrender_d3d_vertex_buffer_pos >= LEVELRENDER_D3D_VB_LIMIT) {
        levelrender_d3d_vertex_buffer_pos = 0;
        lock_flags = gr_d3d_lock_discard;
    } else if (levelrender_d3d_vertex_buffer_pos == 0)
        lock_flags = gr_d3d_lock_discard;
    else
        lock_flags = gr_d3d_lock_nooverwrite;
    cur = -1;
    for (g = 0; g < l->mesh->num_groups; g++) {
        grp = &l->mesh->groups[g];
        flat = !l->faces[face_base].texture;
        if (flat != cur) {
            gr_d3d_set_state(l->faces[face_base].texture ? gr_mode_mesh_alpha : gr_mode_flat_alpha);
            cur = flat;
        }
        if (l->faces[face_base].texture)
            gr_d3d_tcache_set(0, level_texture_bms[l->faces[face_base].texture - 1], 0, &u_scale, &v_scale, 1);
        hr = IDirect3DVertexBuffer7_Lock(LEVELRENDER_VB, lock_flags, (void **)&data, NULL);
        lock_flags = gr_d3d_lock_nooverwrite;
        if (hr < 0)
            VSDK_ERROR(LEVELRENDER_CPP, 0x7b1, ddraw_error_string(hr));
        data += levelrender_d3d_vertex_buffer_pos;
        rgb = &l->vertex_rgb[vbase * 3];
        o = &data[vbase];
        uv = &l->mesh->uvs[vbase];
        dup = &l->mesh->vertex_dup[vbase];
        for (i = vbase; i < vbase + grp->num_vertices; i++, o++, dup++, uv++, rgb += 3) {
            if (*dup >= 0) {
                levelrender_view_verts[i] = levelrender_view_verts[i - *dup];
                levelrender_d3d_vert_lit[i] = levelrender_d3d_vert_lit[i - *dup];
                if (num_lights)
                    levelrender_d3d_vert_colors[i] = levelrender_d3d_vert_colors[i - *dup];
                if (levelrender_d3d_vert_lit[i])
                    continue;
                pv = &levelrender_d3d_proj_verts[i - *dup];
                vv = &levelrender_view_verts[i - *dup];
                o->sx = pv->x;
                o->sy = pv->y;
                o->sz = gr_calc_depth(pv->z);
                o->rhw = pv->z;
                o->specular_bytes[3] = (ubyte)gr_get_fog_alpha(vv->z);
            } else {
                // The light count is not kept on this path (the vertex colours are).
                if (num_lights)
                    levelrender_sum_point_lights(&levelrender_d3d_vert_colors[i], &verts[i], &l->mesh->normals[i],
                                                 num_lights, (gr_light **)lights);
                pv = &levelrender_d3d_proj_verts[i];
                *pv = verts[i] - gr_view_pos;
                pv->rotate(gr_view_matrix);
                levelrender_view_verts[i] = *pv;
                LEVELRENDER_CODE_POINT(c, *pv);
                levelrender_d3d_vert_lit[i] = c;
                if (c)
                    continue;
                float w = 1.0f / pv->z;
                z = pv->z;
                LEVELRENDER_PROJECT_XY(*pv, w);
                o->specular_bytes[3] = (ubyte)gr_get_fog_alpha(z);
                if (far_fade)
                    pv->z = LEVELRENDER_FAR_FADE(pv->z);
                else
                    pv->z = w;
                o->sx = pv->x;
                o->sy = pv->y;
                o->sz = levelrender_depth(pv->z);
                o->rhw = pv->z;
            }
            o->tu = uv->x;
            o->tv = uv->y;
            o->color_bytes[3] = (ubyte)LEVELRENDER_FOG_ALPHA(levelrender_view_verts[i].z);
            if (gr_tint_enabled) {
                s = (float)(rgb[0] + rgb[1] + rgb[2]);
                if (s < 0.0f)
                    s = 0.0f;
                else if (s > 255.0f)
                    s = 255.0f;
                o->color_bytes[0] = (ubyte)(int)(s * gr_tint_b);
                o->color_bytes[1] = (ubyte)(int)(s * gr_tint_g);
                o->color_bytes[2] = (ubyte)(int)(s * gr_tint_r);
            } else if (num_lights) {
                col = rgb[2] - (int)(levelrender_d3d_vert_colors[i].z * -255.0f);
                if (col > 255)
                    col = 255;
                o->color_bytes[0] = (ubyte)col;
                col = rgb[1] - (int)(levelrender_d3d_vert_colors[i].y * -255.0f);
                if (col > 255)
                    col = 255;
                o->color_bytes[1] = (ubyte)col;
                col = rgb[0] - (int)(levelrender_d3d_vert_colors[i].x * -255.0f);
                if (col > 255)
                    col = 255;
                o->color_bytes[2] = (ubyte)col;
            } else {
                o->color_bytes[0] = rgb[2];
                o->color_bytes[1] = rgb[1];
                o->color_bytes[2] = rgb[0];
            }
        }
        ip = levelrender_d3d_indices;
        num_idx = 0;
        for (f = face_base; f < face_base + grp->num_faces; f++) {
            face = &l->mesh->faces[f];
            lf = &l->faces[f];
            if (!(face->flags & 0x200) && !gr_check_normal_facing(&face->normal, &verts[face->v[0]]))
                continue;
            c0 = levelrender_d3d_vert_lit[face->v[0]];
            c1 = levelrender_d3d_vert_lit[face->v[1]];
            c2 = levelrender_d3d_vert_lit[face->v[2]];
            if (c0 & c1 & c2)
                continue;
            face_count = 3;
            if (c0 | c1 | c2) {
                n = 3;
                clip = levelrender_face_vertex_colors(l, face, (int)lf, NULL, levelrender_d3d_vert_lit,
                                                      levelrender_view_verts, &n, levelrender_d3d_vert_colors,
                                                      num_lights);
                if (n < 3)
                    continue;
                face_count = 3 * (n - 2);
                fan = levelrender_fan_tables[n];
                for (k = 0; k < face_count; k++) {
                    gv = clip[fan[k]];
                    if (gv->flags & 4) {
                        ip[k] = (ushort)count;
                        o = &data[count++];
                        o->sx = gv->sx;
                        o->sy = gv->sy;
                        if (far_fade)
                            gv->sw = LEVELRENDER_FAR_FADE(gv->z);
                        o->sz = gr_calc_depth(gv->sw);
                        o->rhw = gv->sw;
                        o->color_bytes[0] = gv->b;
                        o->color_bytes[1] = gv->g;
                        o->color_bytes[2] = gv->r;
                        o->color_bytes[3] = (ubyte)LEVELRENDER_FOG_ALPHA(gv->z);
                        o->tu = gv->u;
                        o->tv = gv->v;
                        o->specular_bytes[3] = (ubyte)gr_get_fog_alpha(gv->z);
                    } else
                        ip[k] = face->v[gv->index];
                }
            } else
                memcpy(ip, face->v, 3 * sizeof(ushort));
            ip += face_count;
            num_idx += face_count;
        }
        IDirect3DVertexBuffer7_Unlock(LEVELRENDER_VB);
        vbase += grp->num_vertices;
        face_base += grp->num_faces;
        if (num_idx)
            IDirect3DDevice7_DrawIndexedPrimitiveVB(d3d_device, D3DPT_TRIANGLELIST, LEVELRENDER_VB,
                                                    levelrender_d3d_vertex_buffer_pos,
                                                    count == l->mesh->num_vertices ? vbase : count,
                                                    levelrender_d3d_indices, num_idx, 0);
    }
    levelrender_d3d_vertex_buffer_pos += count;
}

// 0x4914e0 atexit_nop_4914e0 and
// 0x4914f0 atexit_nop_4914f0: the empty atexit destructors of the Direct3D draw list's two function-local
//          statics (guard 0x23d3600). Confidence: exact (compiler-generated, bare rets).

// 0x491500  Confidence: likely (unsure for the function-local statics, D5803). The OpenGL client arrays
// (vertex, colour, texture coordinates 0 and 1, projective: s * w, t * w, 0, w) are set up first.
// Lightmapped: as the Direct3D list, batches of triangles drawn with glDrawArrays when the lightmap
// changes or the batch would pass 4000 vertices, and after each group (the additive pass resets the
// bound lightmap). Plain: the group's vertices at their vertex index, clipped polygons appended after the
// mesh's vertices (for the whole object), one glDrawElements of levelrender_gl_indices per group, also
// when it is empty. With point lights on a lightmapped face that is not clipped, all three colour bytes
// take the red component (bug G-candidate, D5804).
void levelrender_draw_list_opengl(lobj *l, int alpha, unsigned int num_lights, attached_light **lights)
{
    static vector3 levelrender_gl_proj_verts[LEVELRENDER_MAX_VERTS];    // 0x240d9f0 (guard 0x23c5b38)
    static vector3 levelrender_gl_vert_colors[LEVELRENDER_MAX_VERTS];   // 0x24268f8
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
    glVertexPointer(3, GL_FLOAT, 0, levelrender_gl_vertex_array);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, levelrender_gl_color_array);
    gl_active_texture_arb(GL_TEXTURE0_ARB);
    gl_client_active_texture_arb(GL_TEXTURE0_ARB);
    glTexCoordPointer(4, GL_FLOAT, 0, levelrender_gl_texcoords0);

    if (!gr_tint_enabled && l->faces->lightmap >= 0 && !(l->flags & 0x80000)) {
        gl_active_texture_arb(GL_TEXTURE1_ARB);
        gl_client_active_texture_arb(GL_TEXTURE1_ARB);
        glTexCoordPointer(4, GL_FLOAT, 0, levelrender_gl_texcoords1);
        gr_opengl_set_mode(gr_mode_lightmap_multitexture);
        gl_active_texture_arb(GL_TEXTURE1_ARB);
        gl_client_active_texture_arb(GL_TEXTURE1_ARB);
        cur = -1;
        start = 0;
        count = 0;
        for (g = 0; g < l->mesh->num_groups; g++) {
            grp = &l->mesh->groups[g];
            gr_opengl_tcache_set(level_texture_bms[l->faces[face_base].texture - 1], &u_scale, &v_scale, 0);
            gl_active_texture_arb(GL_TEXTURE1_ARB);
            gl_client_active_texture_arb(GL_TEXTURE1_ARB);
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
                    clip = levelrender_face_vertex_colors(l, face, (int)lf, (float *)box, levelrender_gl_vert_lit,
                                                          levelrender_view_verts, &n,
                                                          levelrender_gl_vert_colors, num_lights);
                    if (n < 3 || n > 8)
                        continue;
                    clipped = true;
                    face_count = 3 * (n - 2);
                } else
                    clipped = false;
                bm = level_face_bitmaps[lf->lightmap];
                if ((bm != cur && count > start) || count + face_count > LEVELRENDER_GL_BATCH_LIMIT) {
                    glDrawArrays(GL_TRIANGLES, start, count - start);
                    if (num_lights && lit_sum) {
                        gl_active_texture_arb(GL_TEXTURE0_ARB);
                        gl_client_active_texture_arb(GL_TEXTURE0_ARB);
                        gr_opengl_set_mode(gr_mode_level_alpha_add);
                        glDrawArrays(GL_TRIANGLES, start, count - start);
                        gr_opengl_set_mode(gr_mode_lightmap_multitexture);
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
                    gr_opengl_tcache_set(bm, &u_scale, &v_scale, 1);
                    gl_active_texture_arb(GL_TEXTURE0_ARB);
                    gl_client_active_texture_arb(GL_TEXTURE0_ARB);
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
                glDrawArrays(GL_TRIANGLES, start, count - start);
                if (num_lights && lit_sum) {
                    gr_opengl_set_mode(gr_mode_level_alpha_add);
                    glDrawArrays(GL_TRIANGLES, start, count - start);
                    gr_opengl_set_mode(gr_mode_lightmap_multitexture);
                    cur = -1;
                }
                start = count;
            }
            face_base += grp->num_faces;
            vbase += grp->num_vertices;
        }
        return;
    }

    cur = -1;
    count = l->mesh->num_vertices;
    for (g = 0; g < l->mesh->num_groups; g++) {
        grp = &l->mesh->groups[g];
        flat = !l->faces[face_base].texture;
        if (flat != cur) {
            gr_opengl_set_mode(l->faces[face_base].texture ? gr_mode_mesh_alpha : gr_mode_flat_alpha);
            cur = flat;
        }
        if (l->faces[face_base].texture)
            gr_opengl_tcache_set(level_texture_bms[l->faces[face_base].texture - 1], &u_scale, &v_scale, 0);
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
                clip = levelrender_face_vertex_colors(l, face, (int)lf, NULL, levelrender_gl_vert_lit,
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
        glDrawElements(GL_TRIANGLES, num_idx, GL_UNSIGNED_SHORT, levelrender_gl_indices);
    }
}

// 0x492ae0 atexit_nop_492ae0 and
// 0x492af0 atexit_nop_492af0: the same for the OpenGL draw list (guard 0x23c5b38). Confidence: exact
//          (compiler-generated, bare rets).
