// vsdk/gr/unnamed_gr_3d_0x50d9b0.h - the 3D view state of the renderer (vsdk\gr in sum.symbols; one object
// 0x50d9b0-0x50f9cf, source src/decomp/vsdk/unnamed_gr_3d_0x50d9b0.cpp; file name a placeholder, no path
// string or strings.tbl block; decomp65). Included at the end of vsdk/gr/gr.h, which keeps the 3D
// prototypes earlier units declared (gr_setup_3d, gr_rotate_vertex, gr_cull_sphere, ...). Not original
// source. Field and global names ours unless sum.symbols names them.
#ifndef VSDK_GR_UNNAMED_GR_3D_0X50D9B0_H
#define VSDK_GR_UNNAMED_GR_3D_0X50D9B0_H

#include "vsdk/vsdk.h"
#include "vsdk/math/matrix.h"
#include "vsdk/math/ix.h"
#include "vsdk/gr/gr.h"

class File;

// ---- calls into other vsdk\gr objects ------------------------------------------------------------------
// decomp66: the two bbox functions and gr_clip_num_temp_verts belong to the clip unit (0x5590b0-0x559da7,
// src/decomp/vsdk/unnamed_clip_0x5590b0.cpp, header vsdk/gr/unnamed_clip_0x5590b0.h); they stay declared
// here, where they were first.
// (normal, near index, far index): the box corners nearest to / farthest along the normal (0..7).
void bbox_corner_indices_from_normal(const vector3 *normal, int *near_index, int *far_index);   // 0x559a80
// (out, index 0..7, mins, maxs): decomp66: not a bit pattern but an eight-case switch: 0 (max, max, min),
// 1 (max, min, min), 2 (min, min, min), 3 (min, max, min), 4 (max, max, max), 5 (max, min, max),
// 6 (min, min, max), 7 (min, max, max).
void bbox_get_corner(vector3 *out, int index, const vector3 *mins, const vector3 *maxs);       // 0x559960
void gr_poly_reset_clip_verts();                      // 0x545120
extern bool gr_poly_clip_verts_ready;                 // 0x2f076c4 (name ours) gr_setup_3d sets it before the first reset
extern int gr_clip_num_temp_verts;                    // 0x2f54700 the temporary clip vertices in use

// ---- .data (0x5a7294-0x5a72ab, then the literals) -----------------------------------------------------
// Gr_far_min (31.25) and Gr_far_max (250.0) are declared in gr.h.
extern float gr_depth_scale;                          // 0x5a729c 0.0001 = 1 / far (gr_set_far_clip)
extern float Gr_3d_zmult;                             // 0x5a72a0 0.2, console "zmult"
extern float Gr_3d_zmax;                              // 0x5a72a4 0.998, console "zmax"
extern float gr_view_zfar;                            // 0x5a72a8 1000.0; 8000.0 with Glide

// ---- structures ---------------------------------------------------------------------------------------
// A clipping plane (0x1c bytes): the clip code bit it sets, the plane, and the box corners
// bbox_corner_indices_from_normal picks for it (gr_cull_bounding_box tests near_corner). The constructor
// and the copy assignment are the implicit ones (0x50f590 / 0x50f900, COMDATs of the 3D object); the two
// setters are header inlines (COMDATs 0x50f6c0 / 0x50f710) and take their vectors by value (ret 0x28 /
// 0x1c).
class gr_clip_plane {
public:
    void set_from_points(ubyte flag, vector3 p0, vector3 p1, vector3 p2)   // 0x50f6c0 gr_clip_plane__set_from_points
    {
        this->flag = flag;
        pl = plane(&p0, &p1, &p2);
        bbox_corner_indices_from_normal(&pl.normal, &near_corner, &far_corner);
    }
    void set_from_normal(ubyte flag, vector3 normal, vector3 point)        // 0x50f710 gr_clip_plane__set_from_normal
    {
        this->flag = flag;
        pl = plane(&normal, &point);
        bbox_corner_indices_from_normal(&pl.normal, &near_corner, &far_corner);
    }
    ubyte flag;                       // +0x00 clip code (gr_code_point): 2 left, 4 right, 8 bottom, 0x10 top, 0x20 behind, 1 far
    ubyte _pad01[3];
    plane pl;                         // +0x04
    int near_corner;                  // +0x14
    int far_corner;                   // +0x18
};
LAYOUT_CHECK(gr_clip_plane, sizeof(gr_clip_plane) == 0x1c);

// One level of the instance stack (0x64 bytes): what gr_start_instance replaces and gr_stop_instance
// puts back. Implicit constructor 0x50f5a0 and copy assignment 0x50f930.
struct gr_instance_entry {
    int lighting;                     // +0x00 gr_start_instance's third argument: the light frame moves too
    vector3 view_pos;                 // +0x04
    matrix view_matrix;               // +0x10
    vector3 light_base;               // +0x34
    matrix light_matrix;              // +0x40
};
LAYOUT_CHECK(gr_instance_entry, sizeof(gr_instance_entry) == 0x64);

#define GR_MAX_INSTANCE_DEPTH  5
#define GR_MAX_CLIP_PLANES     8

// The whole 3D state as gr_save_3d_state copies it (0x494 bytes; gr_saved_3d_state is the only object).
// Implicit constructor 0x50f5d0.
struct gr_3d_state {
    matrix view_matrix;               // +0x000
    vector3 view_pos;                 // +0x024
    matrix light_matrix;              // +0x030
    vector3 light_base;               // +0x054
    matrix eye_matrix;                // +0x060
    vector3 eye_pos;                  // +0x084
    vector3 matrix_scale;             // +0x090
    float one_over_matrix_scale_z;    // +0x09c
    int clip_width;                   // +0x0a0 gr_view_clip_width
    int clip_height;                  // +0x0a4 gr_view_clip_height
    float projection_xmul;            // +0x0a8
    float projection_ymul;            // +0x0ac
    float projection_xadd;            // +0x0b0
    float projection_yadd;            // +0x0b4
    float projection_xmul_copy;       // +0x0b8
    float projection_ymul_neg;        // +0x0bc
    gr_clip_plane user_clip_planes[GR_MAX_CLIP_PLANES];   // +0x0c0
    int num_user_clip_planes;         // +0x1a0
    bool far_clip_enabled;            // +0x1a4
    ubyte _pad1a5[3];
    float far_clip_dist;              // +0x1a8
    float far_clip_z;                 // +0x1ac
    float view_zfar;                  // +0x1b0
    float view_depth_scale;           // +0x1b4
    gr_instance_entry instance_stack[GR_MAX_INSTANCE_DEPTH];   // +0x1b8
    int instance_depth;               // +0x3ac
    gr_clip_plane frustum_planes[GR_MAX_CLIP_PLANES];          // +0x3b0
    int num_frustum_planes;           // +0x490
};
LAYOUT_CHECK(gr_3d_state, sizeof(gr_3d_state) == 0x494);

// An 8-byte sort record {key, value} (vfx_mesh_render fills vfx_mesh_sort_buf; name ours).
struct gr_sort_entry {
    float key;                        // +0x0
    union {
        int value;                    // +0x4 vfx_mesh_render stores a 16-bit index
        ushort index;                 //      decomp70: the face index it stores and reads back (word moves)
    };
};

// A light record read from a file (0x8c bytes; sum.symbols gr_light__reset / gr_light__read, but gr_light
// is the 0x98-byte attached-light layout, D4002, so the class name is ours: D4503). Nothing in Sum.exe
// calls either member.
class gr_light_record {
public:
    void reset();                     // 0x50f420 gr_light__reset
    int read(File *f);                // 0x50f490 gr_light__read (returns 0)
    ubyte type;                       // +0x00 4: one more float (+0x38) in the file
    ubyte _pad01[3];
    quaternion orient;                // +0x04
    vector3 pos;                      // +0x14
    float f20, f24, f28;              // +0x20
    float f2c;                        // +0x2c 1.0 after reset
    float f30, f34;                   // +0x30
    float f38;                        // +0x38 type 4 only
    bool flag3c;                      // +0x3c
    ubyte _pad3d[3];
    float f40;                        // +0x40
    float f44;                        // +0x44 40.0 after reset
    bool flag48;                      // +0x48
    char name[0x40];                  // +0x49
    ubyte _pad89[3];
};
LAYOUT_CHECK(gr_light_record, sizeof(gr_light_record) == 0x8c);

// ---- .bss (0x2682998-0x2692e9f; gr_view_pos, gr_light_base, gr_light_matrix and gr_instance_depth are
// declared in the light unit's header, gr_eye_pos / gr_eye_matrix / gr_far_clip_dist in gr.h) ----------
extern int gr_view_clip_height;                       // 0x2682998 (name ours) gr_screen.clip_height at gr_setup_3d
extern gr_clip_plane gr_user_clip_planes[GR_MAX_CLIP_PLANES];   // 0x26829a0 (only saved and restored)
extern gr_sort_entry vfx_mesh_sort_buf[8000];         // 0x2682aa0 (filled and sorted by vfx_mesh_render)
extern int gr_num_user_clip_planes;                   // 0x26924a0
extern float gr_projection_xadd;                      // 0x26924a4
extern gr_clip_plane gr_frustum_fog_plane;           // 0x26924b8 the far plane at far / Gr_level_fog_multiplier
extern matrix gr_view_matrix;                         // 0x26924d8 rows scaled by gr_matrix_scale
extern float gr_projection_yadd;                      // 0x26924fc
extern gr_instance_entry gr_instance_stack[GR_MAX_INSTANCE_DEPTH];   // 0x2692500
// The two vertex pointer lists of the polygon clipper (gr_clip_and_project_poly, gr_poly_project_to_screen;
// 25 entries of 4 bytes each; names ours).
extern gr_vertex *gr_clip_vlist_a[25];                // 0x26926f4
extern gr_vertex *gr_clip_vlist_b[25];                // 0x2692758
extern int gr_view_clip_width;                        // 0x26927bc (name ours)
extern float gr_projection_xmul_copy;                 // 0x26927e4 (name ours; written, saved, never read)
extern float gr_projection_ymul;                      // 0x26927e8 clip height / 2
extern int vfx_mesh_sort_count;                       // 0x26927fc
extern float gr_projection_xmul;                      // 0x2692824 clip width / 2
extern float gr_projection_ymul_neg;                  // 0x2692828
extern gr_3d_state gr_saved_3d_state;                 // 0x2692890
extern float gr_one_over_matrix_scale_z;              // 0x2692d34
extern gr_clip_plane gr_frustum_planes[GR_MAX_CLIP_PLANES];   // 0x2692d38
extern vector3 gr_matrix_scale;                       // 0x2692e78
extern bool gr_far_clip_enabled;                      // 0x2692e84
extern float gr_far_clip_z;                           // 0x2692e8c
extern float gr_view_depth_scale;                     // 0x2692e90 0.98, 65536 with Glide
extern int gr_num_frustum_planes;                     // 0x2692e98

// ---- functions of the unit (the others are in gr.h) ----------------------------------------------------
void gr_setup_frustum();                              // 0x50e150
// (pos, orient, lighting): enters the frame of an object at pos / orient.
void gr_start_instance(const vector3 *pos, const matrix *orient, int lighting);   // 0x50e640
void gr_stop_instance();                              // 0x50e750
void gr_set_far_clip(float dist);                     // 0x50e7c0 (dist <= 1: off)
ubyte gr_code_point(const vector3 *p);                // 0x50e810 (no caller)
ubyte gr_code_vertex(gr_vertex *v);                   // 0x50e890 stores the codes in v->codes
void gr_rotate_delta_vec(vector3 *out, const vector3 *in);   // 0x50e960 (no caller)
// (normal, point): the point's side of the plane faces the viewer. sum.symbols' "(pos, normal)" has the
// order reversed (the second argument is subtracted from gr_view_pos, D4504).
bool gr_check_normal_facing(const vector3 *normal, const vector3 *point);   // 0x50eb30 (callers test al)
bool gr_plane_faces_viewer(plane *p);                 // 0x50eb70 (no caller; shape of 0x50eb30)
float gr_get_normal_facing_dot(const vector3 *normal, const vector3 *point);   // 0x50eba0
float gr_project_size(const gr_vertex *v, const float *size);   // 0x50ec90
float gr_vector_scaled_z(const gr_vertex *v);         // 0x50ecc0 z / gr_matrix_scale.z
void gr_get_frustum_planes(plane *out, ubyte *flags); // 0x50ed60 (no caller)
bool gr_cull_bounding_box_transformed(const vector3 *pos, const matrix *orient, const vector3 *mins,
                                      const vector3 *maxs);   // 0x50ee70 (no caller)
void gr_save_3d_state();                              // 0x50ef60 (no caller)
void gr_restore_3d_state();                           // 0x50f120 (no caller)
// (entries, n): shell sort, largest key first (vfx_mesh_render).
void sort_by_float_key(gr_sort_entry *entries, int n);   // 0x50f2e0
bool float_key_less(const gr_sort_entry *a, const gr_sort_entry *b);   // 0x50f380

// Inline: the COMDATs after gr_light_record::read (0x50f680 / 0x50f690). The first only forwards (`jmp`).
inline void gr_clip_free_temp_verts()                 // 0x50f690
{ gr_clip_num_temp_verts = 0; }
inline void gr_setup_3d_free_clip_verts()             // 0x50f680 (gr_setup_3d)
{ gr_clip_free_temp_verts(); }

// ---- fog and depth helpers (D4500) ---------------------------------------------------------------------
// Header inlines: their only copies are COMDATs at the end of the character mesh object
// (0x50d830-0x50d95f, after vector3::cross), which calls all five; the Direct3D, OpenGL and vfx objects
// call those copies. They read gr_screen's fog fields (gr.h) and this unit's .data.
// (z): 255 with fog off or before the fog start, else 255 * the fog factor.
inline int gr_get_fog_alpha(float z)                  // 0x50d830
{
    if (gr_fog_enabled && z > gr_fog_z_near)
        return (int)(float_max(0.0f, (gr_fog_z_far - z) * gr_fog_z_inv_range) * 255.0f);
    return 255;
}
// (w): the z the back ends send, w / far * Gr_3d_zmult clamped to Gr_3d_zmax.
inline float gr_calc_depth(float w)                   // 0x50d880
{
    float z = gr_depth_scale * Gr_3d_zmult * w;
    if (Gr_3d_zmax < z)
        z = Gr_3d_zmax;
    return z;
}
// gr_calc_depth(1 / v->z); only x, y, z are read, so the fog code passes a vector3 cast to a vertex.
inline float gr_vertex_depth(const gr_vertex *v)      // 0x50d8b0
{ return gr_calc_depth(1.0f / v->z); }
// (value, z): value scaled by the second fog range (the vertex alpha of the effects).
inline int gr_get_fog_value(int value, float z)       // 0x50d910
{
    if (gr_fog_enabled && z > gr_fog_alpha_near)
        return (int)(float_max(0.0f, (gr_fog_alpha_far - z) * gr_fog_alpha_inv_range) * value);
    return value;
}
// (value, world point): gr_get_fog_value with the point's view-space z.
inline int gr_get_point_fog_value(int value, const vector3 *pos)   // 0x50d8d0
{
    vector3 v = *pos - gr_view_pos;
    v.rotate(gr_view_matrix);
    return gr_get_fog_value(value, v.z);
}

#endif
