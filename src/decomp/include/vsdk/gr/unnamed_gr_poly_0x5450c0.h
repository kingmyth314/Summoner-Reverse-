// vsdk/gr/unnamed_gr_poly_0x5450c0.h - the polygon builder of the renderer (vsdk\gr, 0x5450c0-0x54614a; source
// src/decomp/vsdk/unnamed_gr_poly_0x5450c0.cpp, decomp69; file name a placeholder): polygons are given
// vertex by vertex (begin / vertex / set_texture / uv / rgb / alpha / end), drawn at once through
// gr_tmapper_multipass, or recorded into a command buffer that gr_poly_buffer_flush sorts far to near and
// replays (the alpha-blended polygons, Use_buffered_alpha_polys). Each set_texture adds a pass (at most
// four); uv / rgb / alpha fill the current pass of successive vertices. gr_poly_begin / vertex /
// set_texture / uv / alpha / end and gr_poly_buffer_flush are declared in gr.h, gr_poly_reset_clip_verts
// and gr_poly_clip_verts_ready in vsdk/gr/unnamed_gr_3d_0x50d9b0.h. Not original source.
#ifndef VSDK_GR_UNNAMED_GR_POLY_0X5450C0_H
#define VSDK_GR_UNNAMED_GR_POLY_0X5450C0_H

#include "vsdk/vsdk.h"
#include "vsdk/gr/gr.h"

#define GR_POLY_MAX_PASSES      4     // gr_poly_texture_slots
#define GR_POLY_MAX_VERTS       32    // gr_poly_vertices
#define GR_POLY_MAX_CLIP_VERTS  64    // the clip vertex pool
#define GR_POLY_BUFFER_SIZE     0x20000
#define GR_POLY_BUFFER_MAX_POLYS 0x3ff   // gr_poly_begin flushes at this count (the array has 0x400)

// Command buffer opcodes (one byte, then padding to 4 bytes; names ours).
#define GR_POLY_OP_BEGIN        1     // byte a2, float z
#define GR_POLY_OP_BEGIN_DEPTH  2     // byte a2, float z, float depth
#define GR_POLY_OP_VERTEX       3     // byte codes, vector3 position
#define GR_POLY_OP_TEXTURE      4     // byte stage, gr::Mode, int bitmap, bytes r g b a
#define GR_POLY_OP_UV           5     // float u, float v
#define GR_POLY_OP_RGB          6     // bytes r g b (no padding)
#define GR_POLY_OP_ALPHA        7     // byte a
#define GR_POLY_OP_END          8

// A vertex as gr_tmapper_multipass takes it, with room for the four passes (0x34 bytes; gr.h's
// gr_multipass_vertex is the same record declared with one pass, D5011).
struct gr_poly_vertex_rec {
    gr_vertex *v;                     // +0x00
    gr_pass_vertex_data pass[GR_POLY_MAX_PASSES];   // +0x04
};
LAYOUT_CHECK(gr_poly_vertex_rec, sizeof(gr_poly_vertex_rec) == 0x34);

// ---- data (.data 0x5aca38-0x5aca47, .bss 0x2ee45d8-0x2f076eb) -------------------------------------------
extern ubyte *gr_poly_buffer_limit;                   // 0x5aca38 &gr_poly_buffer[size - 0x200]: flush beyond
extern ubyte *gr_poly_buffer_last;                    // 0x5aca3c &gr_poly_buffer[size - 1] (no reference)
extern ubyte *gr_poly_buffer_ptr;                     // 0x5aca40 write / read cursor
extern bool gr_poly_buffer_first_overflow;            // 0x5aca44 (1) cleared at the first full buffer
extern gr_tmapper_pass gr_poly_texture_slots[GR_POLY_MAX_PASSES];   // 0x2ee45d8
extern bool gr_poly_active;                           // 0x2ee4628 recording into the buffer
extern gr_poly_vertex_rec gr_poly_clip_records[GR_POLY_MAX_CLIP_VERTS];   // 0x2ee4630
extern ubyte *gr_poly_buffer_polys[GR_POLY_BUFFER_MAX_POLYS + 1];   // 0x2ee5330 start of each recorded polygon
extern float gr_poly_depth;                           // 0x2ee6330
extern gr_vertex gr_poly_clip_vertices[GR_POLY_MAX_CLIP_VERTS];   // 0x2ee6338
extern gr_poly_vertex_rec gr_poly_vertices[GR_POLY_MAX_VERTS];   // 0x2ee6f38
extern ubyte gr_poly_buffer[GR_POLY_BUFFER_SIZE];     // 0x2ee75b8
extern bool gr_poly_multitexture;                     // 0x2f075b8 a pass has a sectioned bitmap (split drawing)
extern gr_poly_vertex_rec *gr_poly_clip_vertex_ptrs[GR_POLY_MAX_CLIP_VERTS];   // 0x2f075bc
extern bool gr_poly_fixed_depth;                      // 0x2f076bc (name ours)
extern int gr_poly_clip_vertex_count;                 // 0x2f076c0
extern bool gr_poly_in_progress;                      // 0x2f076c5
extern int gr_poly_num_vertices;                      // 0x2f076c8
extern int gr_poly_num_textures;                      // 0x2f076cc
extern gr_tmapper_pass *gr_poly_cur_texture;          // 0x2f076d0
extern int gr_poly_cur_texture_index;                 // 0x2f076d4
extern gr_poly_vertex_rec *gr_poly_uv_cursor;         // 0x2f076d8
extern gr_poly_vertex_rec *gr_poly_rgb_cursor;        // 0x2f076dc
extern gr_poly_vertex_rec *gr_poly_alpha_cursor;      // 0x2f076e0
extern int gr_poly_arg2;                              // 0x2f076e4 gr_poly_begin's third argument (never read; name ours)
extern int gr_poly_buffer_count;                      // 0x2f076e8

// ---- functions --------------------------------------------------------------------------------------------
void gr_poly_buffer_skip(int n);                      // 0x545260
void gr_poly_buffer_put_byte(ubyte b);                // 0x545280
void gr_poly_buffer_put_float(float f);               // 0x5452a0
void gr_poly_buffer_put_vector(const vector3 *v);     // 0x545310
void gr_poly_buffer_put_int(int i);                   // 0x545550
void gr_poly_buffer_put_data(const void *src, int n); // 0x545570
// (r, g, b) the vertex colour of the current pass. decomp69 (D5012): with the tint on, the red result is
// computed from the green tint and the green byte is not tinted (bug, NOTES.md).
void gr_poly_rgb(ubyte r, ubyte g, ubyte b);          // 0x545610
// (codes {or, and}, list, temp list): clips when the or-code is set, projects, draws; true unless no
// texture was set.
bool gr_poly_clip_and_draw(ubyte *codes, gr_multipass_vertex **list, gr_multipass_vertex **tmp);   // 0x545870
gr_multipass_vertex **gr_poly_clip_planes(gr_multipass_vertex **src, gr_multipass_vertex **tmp, int *nv, ubyte *codes);   // 0x545970
int  gr_poly_clip_plane(int plane, gr_multipass_vertex **src, gr_multipass_vertex **dst, int *nv, ubyte *codes);   // 0x5459d0
gr_multipass_vertex *gr_poly_clip_edge(int plane, gr_multipass_vertex *on, gr_multipass_vertex *off);   // 0x545ac0
ubyte gr_poly_buffer_get_byte();                      // 0x546070
float gr_poly_buffer_get_float();                     // 0x546080
int  gr_poly_buffer_get_int();                        // 0x546090
void gr_poly_buffer_get_vector(vector3 *out);         // 0x5460b0
void gr_poly_buffer_get_data(void *dst, int n);       // 0x5460e0

// Header inlines: COMDATs at the end of the object after the pass record's implicit constructor (0x546110),
// in the order of their first use; the split unit (unnamed_poly_split_0x55f110.cpp) calls the second.
inline void gr_poly_free_clip_verts()                 // 0x546120
{ gr_poly_clip_vertex_count = 0; }
// The next vertex of the clip pool, its gr_vertex flags cleared (no bound check).
inline gr_multipass_vertex *gr_poly_alloc_clip_vertex()   // 0x546130
{
    gr_poly_vertex_rec *r = gr_poly_clip_vertex_ptrs[gr_poly_clip_vertex_count++];

    r->v->flags = 0;
    return (gr_multipass_vertex *)r;
}

#endif
