// unnamed_gr_3d_0x50d9b0.cpp - Sum.exe v1.40, code 0x50d9b0-0x50f9cf: the 3D view state of the renderer
// (vsdk\gr in sum.symbols): gr_setup_3d and the view frustum, the instance stack, point / vertex
// transforms, clip codes and projection, frustum culling, the saved 3D state, four console commands,
// and a few small helpers that ended up in the same file (the vfx mesh sort, matrix43_compose, the
// model light record). The file name is a placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range. The static initialisers open the object (table 0x5888a8-0x5888ec, after the character
// mesh unit's; matrix.cpp's identity_matrix at 0x5888f0 starts the next object), the regular functions
// run from gr_setup_3d (0x50dec0) to gr_light_record::read (0x50f490), and the implicit members and
// header inlines the unit uses close it as COMDATs (0x50f590-0x50f9cf, listed at the end).
// .data 0x5a7294-0x5a747f (six floats, then the literals: the four command objects' strings first, then
// those of the four command functions; no string pooling), .bss 0x2682998-0x2692e9f.
//
// The .bss also holds buffers other objects use: vfx_mesh_sort_buf / vfx_mesh_sort_count (vfx_mesh_render)
// and the two vertex lists of the polygon clipper (gr_clip_and_project_poly). sort_by_float_key is the
// sort vfx_mesh_render calls.
//
// Build flags: no inline expansion (the vector3 / matrix operators, the plane members and the clip plane
// setters are called), FPU intrinsics on (atan, tan, fabs).

#include <stdio.h>
#include <math.h>
#include "vsdk/os/console.h"
#include "vsdk/vfile/file.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/directx/gr_direct3d.h"

#define GR_DEG_TO_RAD 0.017453292f                     // 0x580e30 (pi / 180 as a float; strict check D9100)
#define GR_RAD_TO_DEG 57.295776f                       // 0x580e24 (180 / pi as a float)

// ---- .data (0x5a7294-0x5a72ab) ------------------------------------------------------------------------
float Gr_far_min = 31.25f;                            // 0x5a7294 console "far_min"
float Gr_far_max = 250.0f;                            // 0x5a7298 console "far_max"
float gr_depth_scale = 0.0001f;                       // 0x5a729c
float Gr_3d_zmult = 0.2f;                             // 0x5a72a0 console "zmult"
float Gr_3d_zmax = 0.998f;                            // 0x5a72a4 console "zmax"
float gr_view_zfar = 1000.0f;                         // 0x5a72a8

// ---- the 3D state (dynamic initialisers, table 0x5888a8-0x5888ec) --------------------------------------
// Each initialiser is two compiler-generated functions: the table entry (`jmp` to the next one) and the
// body (the constructor call). One sum.symbols entry covers both.
// 0x50d9b0 init_gr_view_matrix (body 0x50d9c0)  Confidence: exact.
matrix gr_view_matrix;                                // 0x26924d8
// 0x50d9d0 init_gr_view_pos (body 0x50d9e0)  Confidence: exact.
vector3 gr_view_pos;                                  // 0x26924a8
// 0x50d9f0 init_gr_light_matrix (body 0x50da00)  Confidence: exact.
matrix gr_light_matrix;                               // 0x2692800
// 0x50da10 init_gr_light_base (body 0x50da20)  Confidence: exact.
vector3 gr_light_base;                                // 0x26927f0
// 0x50da30 init_gr_eye_matrix (body 0x50da40)  Confidence: exact.
matrix gr_eye_matrix;                                 // 0x26927c0
// 0x50da50 init_gr_eye_pos (body 0x50da60)  Confidence: exact.
vector3 gr_eye_pos;                                   // 0x2682a80
// 0x50da70 init_gr_matrix_scale (body 0x50da80)  Confidence: exact.
vector3 gr_matrix_scale;                              // 0x2692e78

// ---- console commands ----------------------------------------------------------------------------------
void cmd_far_min();
void cmd_far_max();
void cmd_zmult();
void cmd_zmax();

// 0x50da90 init_ccmd_far_min_thunk  Confidence: exact. Generated: the initialiser table entry, jmp to the next.
// 0x50daa0 init_ccmd_far_min  Confidence: exact.
console_command ccmd_far_min("far_min", "Sets Gr_far_min", cmd_far_min);   // 0x2692880

// 0x50dac0  Confidence: exact. The console's float command macro.
void cmd_far_min()
{
    if (calling_a_command_function) {
        process_command(ARG_FLOAT);
        Gr_far_min = command_argument_float;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [float]\nSets %s to the specified value.\n", "far_min", "Gr_far_min");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %f\n", "far_min", Gr_far_min);
        print_to_console(string_buffer, 0);
    }
}

// 0x50db50 init_ccmd_far_max_thunk  Confidence: exact. Generated: the initialiser table entry.
// 0x50db60 init_ccmd_far_max  Confidence: exact.
console_command ccmd_far_max("far_max", "Sets Gr_far_max", cmd_far_max);   // 0x2692d28

// 0x50db80  Confidence: exact.
void cmd_far_max()
{
    if (calling_a_command_function) {
        process_command(ARG_FLOAT);
        Gr_far_max = command_argument_float;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [float]\nSets %s to the specified value.\n", "far_max", "Gr_far_max");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %f\n", "far_max", Gr_far_max);
        print_to_console(string_buffer, 0);
    }
}

// 0x50dc10 init_gr_user_clip_planes (body 0x50dc20)  Confidence: exact.
gr_clip_plane gr_user_clip_planes[GR_MAX_CLIP_PLANES];   // 0x26829a0

// 0x50dc40 init_ccmd_zmult_thunk  Confidence: exact. Generated: the initialiser table entry.
// 0x50dc50 init_ccmd_zmult  Confidence: exact.
console_command ccmd_zmult("zmult", "Sets Gr_3d_zmult", cmd_zmult);   // 0x2682a90

// 0x50dc70  Confidence: exact.
void cmd_zmult()
{
    if (calling_a_command_function) {
        process_command(ARG_FLOAT);
        Gr_3d_zmult = command_argument_float;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [float]\nSets %s to the specified value.\n", "zmult", "Gr_3d_zmult");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %f\n", "zmult", Gr_3d_zmult);
        print_to_console(string_buffer, 0);
    }
}

// 0x50dd00 init_ccmd_zmax_thunk  Confidence: exact. Generated: the initialiser table entry.
// 0x50dd10 init_ccmd_zmax  Confidence: exact.
console_command ccmd_zmax("zmax", "Sets Gr_3d_zmax", cmd_zmax);   // 0x2692e18

// 0x50dd30  Confidence: exact.
void cmd_zmax()
{
    if (calling_a_command_function) {
        process_command(ARG_FLOAT);
        Gr_3d_zmax = command_argument_float;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [float]\nSets %s to the specified value.\n", "zmax", "Gr_3d_zmax");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %f\n", "zmax", Gr_3d_zmax);
        print_to_console(string_buffer, 0);
    }
}

// 0x50ddc0 init_gr_instance_stack (body 0x50ddd0)  Confidence: exact.
gr_instance_entry gr_instance_stack[GR_MAX_INSTANCE_DEPTH];   // 0x2692500
// Two arrays of five 16-byte objects whose constructor is the folded "vector3 at +0" one (0x5081c0,
// gr__Vertex__Vertex in sum.symbols, the same code as plane()). Nothing else touches them; the type is a
// guess (labels ours).
// 0x50ddf0 init_gr_vertex_buf_2692e28 (body 0x50de00)  Confidence: likely (type).
plane gr_plane_buf_2692e28[5];                        // 0x2692e28
// 0x50de20 init_gr_vertex_buf_2692830 (body 0x50de30)  Confidence: likely (type).
plane gr_plane_buf_2692830[5];                        // 0x2692830
// 0x50de50 init_gr_frustum_planes (body 0x50de60)  Confidence: exact.
gr_clip_plane gr_frustum_planes[GR_MAX_CLIP_PLANES];  // 0x2692d38
// 0x50de80 init_gr_clip_plane_26924b8 (body 0x50de90)  Confidence: exact.
gr_clip_plane gr_frustum_fog_plane;                  // 0x26924b8
// 0x50dea0 init_gr_3d_state_saved (body 0x50deb0)  Confidence: exact.
gr_3d_state gr_saved_3d_state;                        // 0x2692890

// ---- .bss without initialisers (listed in address order) ------------------------------------------------
int gr_view_clip_height;                              // 0x2682998
gr_sort_entry vfx_mesh_sort_buf[8000];                // 0x2682aa0
int gr_num_user_clip_planes;                          // 0x26924a0
float gr_projection_xadd;                             // 0x26924a4
float gr_projection_yadd;                             // 0x26924fc
gr_vertex *gr_clip_vlist_a[25];                       // 0x26926f4
gr_vertex *gr_clip_vlist_b[25];                       // 0x2692758
int gr_view_clip_width;                               // 0x26927bc
float gr_projection_xmul_copy;                        // 0x26927e4
float gr_projection_ymul;                             // 0x26927e8
int vfx_mesh_sort_count;                              // 0x26927fc
float gr_projection_xmul;                             // 0x2692824
float gr_projection_ymul_neg;                         // 0x2692828
float gr_one_over_matrix_scale_z;                     // 0x2692d34
bool gr_far_clip_enabled;                             // 0x2692e84
float gr_far_clip_dist;                               // 0x2692e88
float gr_far_clip_z;                                  // 0x2692e8c
float gr_view_depth_scale;                            // 0x2692e90
int gr_instance_depth;                                // 0x2692e94
int gr_num_frustum_planes;                            // 0x2692e98
// 0x2692e9c: the "" literal gr_light_record::read passes as its default name (VC6 put it in .bss, as
// console.cpp's two).

// 0x50dec0  Confidence: likely (statement order around the far plane; the fov self-copy is the compiler
// reusing the argument slot for the tangent).
void gr_setup_3d(matrix *orient, vector3 *pos, float fov, bool zbuffer, int a5)
{
    gr_instance_depth = 0;
    gr_view_clip_width = gr_screen.clip_width;
    gr_view_clip_height = gr_screen.clip_height;
    float w = (float)gr_screen.clip_width;
    gr_projection_xmul = w * 0.5f;
    gr_projection_ymul = (float)gr_screen.clip_height * 0.5f;
    float ratio = (float)gr_screen.clip_height * gr_screen.aspect / w;
    gr_setup_3d_free_clip_verts();
    if (!gr_poly_clip_verts_ready) {
        gr_poly_clip_verts_ready = true;
        gr_poly_reset_clip_verts();
    }
    gr_zbuffer_clear(zbuffer);

    float t;
    if (fov < 2.0f)
        t = fov;
    else
    {
        float a = fov * GR_DEG_TO_RAD;
        t = (float)tan(a * 0.5f);
    }

    gr_view_pos = *pos;
    gr_view_matrix = *orient;
    gr_eye_matrix = gr_view_matrix;
    gr_eye_pos = gr_view_pos;
    gr_light_matrix.make_identity();
    gr_light_base.zero();

    gr_view_depth_scale = 0.98f;
    if (gr_screen.mode == GR_GLIDE)
        gr_view_depth_scale = 65536.0f;
    if (gr_screen.mode == GR_GLIDE) {
        gr_view_zfar = 8000.0f;
    } else {
        float old_zfar = gr_view_zfar;
        gr_view_zfar = min(max(Gr_far_min, gr_far_clip_dist), Gr_far_max);
        if (gr_screen.mode == GR_DIRECT3D && old_zfar != gr_view_zfar)
            gr_d3d_set_default_transforms(d3d_device, gr_view_zfar);
    }

    float s = gr_view_depth_scale / gr_view_zfar;
    gr_matrix_scale.x = s / t;
    gr_matrix_scale.y = s / (t * ratio);
    gr_matrix_scale.z = s;
    gr_one_over_matrix_scale_z = 1.0f / s;
    gr_view_matrix.rvec *= gr_matrix_scale.x;
    gr_view_matrix.uvec *= gr_matrix_scale.y;
    gr_view_matrix.fvec *= gr_matrix_scale.z;
    gr_projection_ymul_neg = -gr_projection_ymul;
    gr_projection_xmul_copy = gr_projection_xmul;
    gr_projection_xadd = gr_screen.offset_x + gr_projection_xmul;
    gr_projection_yadd = gr_screen.offset_y + gr_projection_ymul;
    gr_setup_frustum();
}

// 0x50e150  Confidence: exact (D11400: z in a local, -hx / -hy as locals right after each quotient,
// scale.z * far_clip_dist). The four side planes through the eye and the corners of the view window
// one unit ahead, the plane behind the eye, and with far clipping the far plane (plus
// gr_frustum_fog_plane, the far plane pulled in by Gr_level_fog_multiplier).
void gr_setup_frustum()
{
    gr_far_clip_z = gr_matrix_scale.z * gr_far_clip_dist;
    vector3 eye, ul, ur, ll, lr;
    eye = gr_view_pos;
    float z = gr_matrix_scale.z;
    float hx = z / gr_matrix_scale.x;
    float nhx = -hx;
    float hy = z / gr_matrix_scale.y;
    float nhy = -hy;
    vector3 c = gr_eye_matrix.fvec * 1.0f + gr_view_pos;
    ur = gr_eye_matrix.rvec * hx + gr_eye_matrix.uvec * hy + c;
    ul = gr_eye_matrix.rvec * nhx + gr_eye_matrix.uvec * hy + c;
    ll = gr_eye_matrix.rvec * nhx + gr_eye_matrix.uvec * nhy + c;
    lr = gr_eye_matrix.rvec * hx + gr_eye_matrix.uvec * nhy + c;

    gr_num_frustum_planes = 1;
    gr_frustum_planes[0].set_from_points(2, eye, ll, ul);
    gr_frustum_planes[gr_num_frustum_planes++].set_from_points(4, eye, ur, lr);
    gr_frustum_planes[gr_num_frustum_planes++].set_from_points(8, eye, lr, ll);
    gr_frustum_planes[gr_num_frustum_planes++].set_from_points(0x10, eye, ul, ur);
    gr_frustum_planes[gr_num_frustum_planes++].set_from_normal(0x20, -1.0f * gr_eye_matrix.fvec, gr_view_pos);
    if (gr_far_clip_enabled) {
        gr_frustum_planes[gr_num_frustum_planes++].set_from_normal(
            1, gr_eye_matrix.fvec, gr_view_pos + gr_eye_matrix.fvec * gr_far_clip_dist);
        gr_frustum_fog_plane.set_from_normal(
            1, gr_eye_matrix.fvec, gr_view_pos + gr_eye_matrix.fvec * (gr_far_clip_dist / Gr_level_fog_multiplier));
    }
}

// 0x50e5a0  Confidence: exact.
void gr_get_view_fov(float *h_fov, float *v_fov)
{
    float a;

    a = (float)atan(gr_matrix_scale.z / gr_matrix_scale.x) * 2.0f;
    *h_fov = a * GR_RAD_TO_DEG;
    a = (float)atan(gr_matrix_scale.z / gr_matrix_scale.y) * 2.0f;
    *v_fov = a * GR_RAD_TO_DEG;
}

// 0x50e5e0  Confidence: exact (all in the FPU; a fov below 2.0 is a tangent, as in gr_setup_3d).
float gr_fov_to_vertical(float fov)
{
    float a;

    if (fov < 2.0f)
        a = (float)atan(fov) * 2.0f;
    else
        a = fov * GR_DEG_TO_RAD;
    float t = (float)tan(a * 0.5f);
    float w = (float)gr_screen.clip_width;
    a = (float)atan(t * (1.0f / ((float)gr_screen.clip_height * gr_screen.aspect / w))) * 2.0f;
    return a * GR_RAD_TO_DEG;
}

// 0x50e640  Confidence: exact. The light frame moves with the instance only when lighting is set.
void gr_start_instance(const vector3 *pos, const matrix *orient, int lighting)
{
    gr_instance_entry *e = &gr_instance_stack[gr_instance_depth];

    e->view_pos = gr_view_pos;
    e->view_matrix = gr_view_matrix;
    gr_view_pos -= *pos;
    gr_view_pos.rotate(*orient);
    gr_view_matrix = orient->multiply_transposed(gr_view_matrix);
    e->lighting = lighting;
    if (lighting) {
        e->light_base = gr_light_base;
        e->light_matrix = gr_light_matrix;
        gr_light_base = pos->unrotated(e->light_matrix) + e->light_base;
        gr_light_matrix = e->light_matrix * *orient;
    }
    gr_instance_depth++;
}

// 0x50e750  Confidence: exact.
void gr_stop_instance()
{
    gr_instance_depth--;
    gr_instance_entry *e = &gr_instance_stack[gr_instance_depth];

    gr_view_pos = e->view_pos;
    gr_view_matrix = e->view_matrix;
    if (e->lighting) {
        gr_light_base = e->light_base;
        gr_light_matrix = e->light_matrix;
    }
}

// 0x50e7c0  Confidence: exact.
void gr_set_far_clip(float dist)
{
    if (dist > 1.0f) {
        float d;

        gr_far_clip_enabled = true;
        d = Gr_level_fog_multiplier * dist;
        gr_far_clip_dist = d;
        gr_depth_scale = 1.0f / d;
    } else {
        gr_far_clip_enabled = false;
    }
    gr_setup_frustum();
}

// 0x50e810  Confidence: exact.
ubyte gr_code_point(const vector3 *p)
{
    ubyte codes = 0;

    if (p->x > p->z)
        codes = 4;
    if (p->y > p->z)
        codes |= 0x10;
    if (-p->z > p->x)
        codes |= 2;
    if (-p->z > p->y)
        codes |= 8;
    if (p->z <= 0.0f)
        codes |= 0x20;
    if (gr_far_clip_enabled && p->z > gr_far_clip_z)
        codes |= 1;
    return codes;
}

// 0x50e890  Confidence: exact.
ubyte gr_code_vertex(gr_vertex *v)
{
    ubyte codes = 0;

    if (v->x > v->z)
        codes = 4;
    if (v->y > v->z)
        codes |= 0x10;
    if (-v->z > v->x)
        codes |= 2;
    if (-v->z > v->y)
        codes |= 8;
    if (v->z <= 0.0f)
        codes |= 0x20;
    if (gr_far_clip_enabled && v->z > gr_far_clip_z)
        codes |= 1;
    v->codes = codes;
    return codes;
}

// 0x50e910  Confidence: exact (VC6 byte match, match30; D8902: the difference in a named local).
ubyte gr_rotate_vertex(gr_vertex *out, const vector3 *p)
{
    vector3 d = *p - gr_view_pos;
    *(vector3 *)out = d.rotated(gr_view_matrix);
    out->flags = 0;
    return gr_code_vertex(out);
}

// 0x50e960  Confidence: exact.
void gr_rotate_delta_vec(vector3 *out, const vector3 *in)
{
    *out = in->rotated(gr_view_matrix);
}

// 0x50e990  Confidence: exact (VC6 byte match, match30; D8902: the difference in a named local).
void gr_rotate_vertices(gr_vertex *out, const vector3 *in, int count)
{
    while (count--) {
        vector3 d = *in - gr_view_pos;
        *(vector3 *)out = d.rotated(gr_view_matrix);
        out->flags = 0;
        gr_code_vertex(out);
        in++;
        out++;
    }
}

// 0x50ea00  Confidence: exact (D11400: the old z through a float local, loaded again for the divide).
void gr_offset_point_depth(gr_vertex *v, float d)
{
    float z = v->z / gr_vector_scaled_z(v) * d + v->z;
    float oz = v->z;
    float s = z / oz;

    if (z > 0.004f) {
        v->x = s * v->x;
        v->y = s * v->y;
        v->z = z;
    }
}

// 0x50ea50  Confidence: likely. Screen x / y clamped to the 0..2 range of the normalised position.
ubyte gr_project_vertex(gr_vertex *v)
{
    if (v->flags & 3)
        return v->flags;
    if (v->z <= 0.0f) {
        v->flags |= 2;
        return v->flags;
    }

    float w = 1.0f / v->z;
    v->flags |= 1;
    v->sw = w;
    float x = w * v->x + 1.0f;
    float y = 1.0f - w * v->y;
    if (x <= 0.0f)
        x = 0.0f;
    else if (x >= 2.0f)
        x = 2.0f;
    if (y <= 0.0f)
        y = 0.0f;
    else if (y >= 2.0f)
        y = 2.0f;
    v->sx = gr_projection_xmul * x + gr_screen.offset_x;
    v->sy = gr_projection_ymul * y + gr_screen.offset_y;
    return v->flags;
}

// 0x50eb30  Confidence: exact.
bool gr_check_normal_facing(const vector3 *normal, const vector3 *point)
{
    return (gr_view_pos - *point).dot(*normal) > 0.0f;
}

// 0x50eb70  Confidence: exact.
bool gr_plane_faces_viewer(plane *p)
{
    return p->distance_to_point(&gr_view_pos) > 0.0f;
}

// 0x50eba0  Confidence: exact.
float gr_get_normal_facing_dot(const vector3 *normal, const vector3 *point)
{
    vector3 v = gr_view_pos - *point;
    v.normalize();
    return v.dot(*normal);
}

// 0x50ebe0  Confidence: exact (VC6 byte match, match30; was likely; D8902: x computed into dir.x). Earlier note: (statement order; the screen point is not offset by the clip origin).
void gr_screen_to_world_ray(ix_ray *out, float sx, float sy)
{
    vector3 dir;

    dir.z = 1.0f;
    dir.x = (sx - gr_projection_xmul) / gr_projection_xmul;
    dir.y = -((sy - gr_projection_ymul) / gr_projection_ymul);
    dir.x = gr_matrix_scale.z * dir.x / gr_matrix_scale.x;
    dir.y = gr_matrix_scale.z * dir.y / gr_matrix_scale.y;
    dir.normalize();
    dir.unrotate(gr_eye_matrix);
    out->origin = gr_eye_pos;
    out->dir = dir;
}

// 0x50ec90  Confidence: exact.
float gr_project_size(const gr_vertex *v, const float *size)
{
    return gr_matrix_scale.x * *size * gr_projection_xmul / v->z;
}

// 0x50ecb0  Confidence: exact.
float gr_scale_by_far_clip(float f)
{
    return gr_matrix_scale.z * f;
}

// 0x50ecc0  Confidence: exact.
float gr_vector_scaled_z(const gr_vertex *v)
{
    return gr_one_over_matrix_scale_z * v->z;
}

// 0x50ecd0  Confidence: exact (the first plane is tested before the count, a do-while). With far
// clipping on the far plane is replaced by gr_frustum_fog_plane; that plane is also tested when far
// clipping is off (D4505).
bool gr_cull_sphere(const vector3 *pos, float radius, bool ignore_far)
{
    int n = gr_num_frustum_planes;
    int i = 0;

    if (gr_far_clip_enabled && !ignore_far)
        n--;
    do {
        if (gr_frustum_planes[i].pl.distance_to_point(pos) > radius)
            return true;
        i++;
    } while (i < n);
    if (!ignore_far && gr_frustum_fog_plane.pl.distance_to_point(pos) > radius)
        return true;
    return false;
}

// 0x50ed60  Confidence: likely.
void gr_get_frustum_planes(plane *out, ubyte *flags)
{
    int i, n;

    for (i = 0; i < gr_num_frustum_planes; i++)
        out[i] = gr_frustum_planes[i].pl;
    if (flags) {
        n = gr_num_frustum_planes;
        for (i = 0; i < n; i++)
            flags[i] = gr_frustum_planes[i].flag;
    }
}

// 0x50edc0  Confidence: exact (do-while as in gr_cull_sphere; the plane count is read once).
bool gr_cull_bounding_box(vector3 *mins, vector3 *maxs)
{
    vector3 corner;
    int n = gr_num_frustum_planes;
    int i = 0;

    do {
        bbox_get_corner(&corner, gr_frustum_planes[i].near_corner, mins, maxs);
        if (gr_frustum_planes[i].pl.distance_to_point(&corner) > 0.0f)
            return true;
        i++;
    } while (i < n);
    return false;
}

// 0x50ee70  Confidence: likely (the transformed plane local has no constructor call in the binary).
bool gr_cull_bounding_box_transformed(const vector3 *pos, const matrix *orient, const vector3 *mins,
                                      const vector3 *maxs)
{
    vector3 corner;

    for (int i = 0; i < gr_num_frustum_planes; i++) {
        plane p;
        int near_index, far_index;
        gr_frustum_planes[i].pl.transform(&p, pos, orient);
        bbox_corner_indices_from_normal(&p.normal, &near_index, &far_index);
        bbox_get_corner(&corner, near_index, mins, maxs);
        if (p.distance_to_point(&corner) > 0.0f)
            return true;
    }
    return false;
}

// 0x50ef30  Confidence: exact.
void gr_get_eye(vector3 *pos, matrix *orient)
{
    *pos = gr_eye_pos;
    *orient = gr_eye_matrix;
}

// 0x50ef60  Confidence: exact (the order of the scalar copies is the compiler's).
void gr_save_3d_state()
{
    int i;

    gr_saved_3d_state.view_matrix = gr_view_matrix;
    gr_saved_3d_state.view_pos = gr_view_pos;
    gr_saved_3d_state.light_matrix = gr_light_matrix;
    gr_saved_3d_state.light_base = gr_light_base;
    gr_saved_3d_state.eye_matrix = gr_eye_matrix;
    gr_saved_3d_state.eye_pos = gr_eye_pos;
    gr_saved_3d_state.matrix_scale = gr_matrix_scale;
    gr_saved_3d_state.one_over_matrix_scale_z = gr_one_over_matrix_scale_z;
    gr_saved_3d_state.clip_width = gr_view_clip_width;
    gr_saved_3d_state.clip_height = gr_view_clip_height;
    gr_saved_3d_state.projection_xmul = gr_projection_xmul;
    gr_saved_3d_state.projection_ymul = gr_projection_ymul;
    gr_saved_3d_state.projection_xadd = gr_projection_xadd;
    gr_saved_3d_state.projection_yadd = gr_projection_yadd;
    gr_saved_3d_state.projection_xmul_copy = gr_projection_xmul_copy;
    gr_saved_3d_state.projection_ymul_neg = gr_projection_ymul_neg;
    gr_saved_3d_state.num_user_clip_planes = gr_num_user_clip_planes;
    for (i = 0; i < gr_num_user_clip_planes; i++)
        gr_saved_3d_state.user_clip_planes[i] = gr_user_clip_planes[i];
    gr_saved_3d_state.far_clip_enabled = gr_far_clip_enabled;
    gr_saved_3d_state.far_clip_dist = gr_far_clip_dist;
    gr_saved_3d_state.far_clip_z = gr_far_clip_z;
    gr_saved_3d_state.view_zfar = gr_view_zfar;
    gr_saved_3d_state.view_depth_scale = gr_view_depth_scale;
    gr_saved_3d_state.instance_depth = gr_instance_depth;
    for (i = 0; i < gr_instance_depth; i++)
        gr_saved_3d_state.instance_stack[i] = gr_instance_stack[i];
    gr_saved_3d_state.num_frustum_planes = gr_num_frustum_planes;
    for (i = 0; i < gr_num_frustum_planes; i++)
        gr_saved_3d_state.frustum_planes[i] = gr_frustum_planes[i];
}

// 0x50f120  Confidence: exact (as gr_save_3d_state).
void gr_restore_3d_state()
{
    int i;

    gr_view_matrix = gr_saved_3d_state.view_matrix;
    gr_view_pos = gr_saved_3d_state.view_pos;
    gr_light_matrix = gr_saved_3d_state.light_matrix;
    gr_light_base = gr_saved_3d_state.light_base;
    gr_eye_matrix = gr_saved_3d_state.eye_matrix;
    gr_eye_pos = gr_saved_3d_state.eye_pos;
    gr_matrix_scale = gr_saved_3d_state.matrix_scale;
    gr_one_over_matrix_scale_z = gr_saved_3d_state.one_over_matrix_scale_z;
    gr_view_clip_width = gr_saved_3d_state.clip_width;
    gr_view_clip_height = gr_saved_3d_state.clip_height;
    gr_projection_xmul = gr_saved_3d_state.projection_xmul;
    gr_projection_ymul = gr_saved_3d_state.projection_ymul;
    gr_projection_xadd = gr_saved_3d_state.projection_xadd;
    gr_projection_yadd = gr_saved_3d_state.projection_yadd;
    gr_projection_xmul_copy = gr_saved_3d_state.projection_xmul_copy;
    gr_projection_ymul_neg = gr_saved_3d_state.projection_ymul_neg;
    gr_num_user_clip_planes = gr_saved_3d_state.num_user_clip_planes;
    for (i = 0; i < gr_num_user_clip_planes; i++)
        gr_user_clip_planes[i] = gr_saved_3d_state.user_clip_planes[i];
    gr_far_clip_enabled = gr_saved_3d_state.far_clip_enabled;
    gr_far_clip_dist = gr_saved_3d_state.far_clip_dist;
    gr_far_clip_z = gr_saved_3d_state.far_clip_z;
    gr_view_zfar = gr_saved_3d_state.view_zfar;
    gr_view_depth_scale = gr_saved_3d_state.view_depth_scale;
    gr_instance_depth = gr_saved_3d_state.instance_depth;
    for (i = 0; i < gr_instance_depth; i++)
        gr_instance_stack[i] = gr_saved_3d_state.instance_stack[i];
    gr_num_frustum_planes = gr_saved_3d_state.num_frustum_planes;
    for (i = 0; i < gr_num_frustum_planes; i++)
        gr_frustum_planes[i] = gr_saved_3d_state.frustum_planes[i];
}

// 0x50f2e0  Confidence: exact (the n > 0 guard before the gap loop).
void sort_by_float_key(gr_sort_entry *entries, int n)
{
    if (n <= 0)
        return;
    for (int gap = n / 2; gap > 0; gap /= 2) {
        for (int i = gap; i < n; i++) {
            for (int j = i - gap; j >= 0; j -= gap) {
                if (!float_key_less(&entries[j], &entries[j + gap]))
                    break;
                gr_sort_entry tmp = entries[j];
                entries[j] = entries[j + gap];
                entries[j + gap] = tmp;
            }
        }
    }
}

// 0x50f380  Confidence: exact.
bool float_key_less(const gr_sort_entry *a, const gr_sort_entry *b)
{
    return a->key < b->key;
}

// 0x50f3a0  Confidence: exact.
void matrix43_compose(const matrix *orient_a, const vector3 *pos_a, matrix *orient_b, vector3 *pos_b)
{
    *orient_b = *orient_a * *orient_b;
    pos_b->unrotate(*orient_a);
    *pos_b += *pos_a;
}

// 0x50f3e0  Confidence: exact (VC6 byte match, match30; was likely). The binary first runs vector3() on +0xc
// (a vector3 member there, which gr.h splits into _unk0c / appearance / f14 because the callers store a
// pointer at +0x10). D8900: the constructor is called explicitly on +0xc under _MSC_VER (as D7805); the
// /Ob0 unit calls it out of line as the binary does. Other compilers skip it (vector3() does nothing).
gr_light_desc::gr_light_desc()
{
#ifdef _MSC_VER
    ((vector3 *)&_unk0c)->vector3::vector3();
#endif
    reset();
}

// 0x50f400  Confidence: exact (the store order is the compiler's). +0x0c and the tint (+0x18) are left alone.
void gr_light_desc::reset()
{
    _unk04 = -1;
    _unk00 = 0;
    _unk08 = 0xff;
    appearance = NULL;
    f14 = -1.0f;
    model_prefix = NULL;
}

// 0x50f420  Confidence: exact.
void gr_light_record::reset()
{
    type = 0;
    orient = quaternion(0.0f, 0.0f, 0.0f, 1.0f);
    pos = vector3(0.0f, 0.0f, 0.0f);
    f20 = 0.0f;
    f24 = 0.0f;
    f28 = 0.0f;
    f2c = 1.0f;
    f30 = 0.0f;
    f34 = 0.0f;
    f38 = 0.0f;
    flag3c = false;
    f40 = 0.0f;
    f44 = 40.0f;
    flag48 = false;
}

// 0x50f490  Confidence: exact.
int gr_light_record::read(File *f)
{
    type = f->read_int8(0, 0);
    f->read_quaternion(&orient, 0, &file_identity_quaternion);
    f->read_vector(&pos, 0, &file_zero_vector);
    f20 = f->read_float(0, 0.0f);
    f24 = f->read_float(0, 0.0f);
    f28 = f->read_float(0, 0.0f);
    f2c = f->read_float(0, 0.0f);
    f30 = f->read_float(0, 0.0f);
    f34 = f->read_float(0, 0.0f);
    if (type == 4)
        f38 = f->read_float(0, 0.0f);
    flag3c = f->read_bool(0, true);
    f40 = f->read_float(0, 0.0f);
    f44 = f->read_float(0, 0.0f);
    flag48 = f->read_bool(0, true);
    f->read_string(name, 0x40, 0x30007, "");
    return 0;
}

// ---- COMDATs (0x50f590-0x50f9cf): implicit members and header inlines this object calls ---------------
// 0x50f590 gr_clip_plane::gr_clip_plane (implicit; the plane member's constructor is the folded 0x5081c0)  Confidence: exact.
// 0x50f5a0 gr_instance_entry::gr_instance_entry (implicit)  Confidence: exact.
// 0x50f5d0 gr_3d_state::gr_3d_state (implicit; `vector constructor iterator` for the arrays)  Confidence: exact.
// 0x50f660 matrix::matrix (vsdk/math/matrix.h)  Confidence: exact.
// 0x50f680 gr_setup_3d_free_clip_verts (vsdk/gr/unnamed_gr_3d_0x50d9b0.h; jmp to the next)  Confidence: likely (what the wrapper is in the source).
// 0x50f690 gr_clip_free_temp_verts (same header)  Confidence: exact.
// 0x50f6a0 vector3::operator*= (vsdk/math/vector.h)  Confidence: exact.
// 0x50f6c0 gr_clip_plane::set_from_points (vsdk/gr/unnamed_gr_3d_0x50d9b0.h)  Confidence: likely (the by-value vectors).
// 0x50f710 gr_clip_plane::set_from_normal (same header)  Confidence: likely (the by-value vectors).
// 0x50f760 plane::operator= (implicit, returns this; D4502)  Confidence: unsure (D4502).
// 0x50f790 vector3::operator* (vsdk/math/vector.h)  Confidence: exact.
// 0x50f7d0 plane::plane(p0, p1, p2) (vsdk/math/ix.h)  Confidence: exact.
// 0x50f800 plane::set_from_points (vsdk/math/ix.h)  Confidence: exact.
// 0x50f840 vector3::operator-= (vsdk/math/vector.h)  Confidence: exact.
// 0x50f860 plane::distance_to_point (vsdk/math/ix.h)  Confidence: exact.
// 0x50f880 plane::transform (vsdk/math/ix.h)  Confidence: likely.
// 0x50f900 gr_clip_plane::operator= (implicit; copies +0x18 before +0x14)  Confidence: likely (copy order).
// 0x50f930 gr_instance_entry::operator= (implicit)  Confidence: exact.
// 0x50f990 quaternion::quaternion(x, y, z, w) (vsdk/math/quaternion.h)  Confidence: exact.
// 0x50f9b0 quaternion::operator= (vsdk/math/quaternion.h)  Confidence: exact.
