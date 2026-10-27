// vsdk/gr/opengl/gr_opengl.h - the OpenGL back end (vsdk\gr\opengl\gr_opengl.cpp, 0x54d5c0-0x550658,
// decomp33): renderer 0x68, never selected by the retail game (SummonerFix patches LoadSettings to pick it).
// gr.cpp dispatches to it on gr_screen.mode == GR_OPENGL; the character mesh renderer (vsdk\ca) and
// levelrender_draw_list_opengl use the state functions, the texture cache and the multitexture entry
// points directly. Not original source.
#ifndef VSDK_GR_OPENGL_GR_OPENGL_H
#define VSDK_GR_OPENGL_GR_OPENGL_H

#include "vsdk/vsdk.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/opengl/gl_min.h"
#include "modern/gl_redirect.h"   // 64-bit MODERN_BUILD: VK_REDIRECT, the Vulkan back end (L4); empty otherwise

// One texture cache slot per bitmap entry (MAX_BITMAPS of them in gl_tcache_entries, 0x13240 bytes, indexed
// by bm_get_current_frame). Layout from gr_opengl_texture_init / create_texture / tcache_set; names ours.
struct gl_tcache_entry {
    int   gl_id;                     // +0x00 texture name for glBindTexture, -1 until first use
    float u_scale;                   // +0x04 bitmap width / texture width (power-of-two padding)
    float v_scale;                   // +0x08
    int   bitmap;                    // +0x0C the bitmap handle uploaded, -1 when not resident
    int   size;                      // +0x10 bytes counted in gl_texture_mem_used
    int   used;                      // +0x14 uses this frame (cleared by gr_opengl_tcache_frame)
    bool  dirty;                     // +0x18 gr_opengl_mark_texture_dirty: re-upload on the next set
    ubyte _pad19[3];
};
LAYOUT_CHECK(gl_tcache_entry, sizeof(gl_tcache_entry) == 0x1c);

// ---- data ------------------------------------------------------------------------------------------
// sum.symbols: gr_opengl_bound_texture0 (0x5ad7c8) / gr_opengl_bound_texture1 (0x5ad7cc); one array,
// indexed by the stage in gr_opengl_tcache_set (DECISIONS.md D1301). Holds bitmap handles.
extern int gr_opengl_bound_texture[2];                // 0x5ad7c8
extern int gl_min_texture_w;                          // 0x5ad7d4 (1; name ours)
extern int gl_min_texture_h;                          // 0x5ad7d8 (1; name ours)
extern int gr_opengl_packed_pixels;                   // 0x5ad7e4 registry "PackedPixels" (default 1)
extern int gl_flag_5ad7e8;                            // 0x5ad7e8 (1; written only; name ours)
extern int gr_opengl_next_texture_id;                 // 0x5ad7ec
extern int gl_blend_add_one_one;                      // 0x5ad854 (1): blend 1 is GL_ONE, GL_ONE (name ours)
extern bool gr_opengl_vertex_color;                   // 0x5ad858 colour source TEXTURE: vertices white
extern bool gr_opengl_vertex_alpha;                   // 0x5ad859 alpha source NONE / TEXTURE: alpha 255
extern float gr_opengl_texture_u_scale;               // 0x5ad85c of the bitmap last paged in
extern float gr_opengl_texture_v_scale;               // 0x5ad860
extern int gl_flag_2f13318;                           // 0x2f13318 (written only; name ours)
extern gl_tcache_entry *gl_tcache_entries;            // 0x2f1331c
extern int gl_frame_count;                            // 0x2f13324 (name ours)
extern int gl_texture_mem_used;                       // 0x2f13328
extern int gl_texture_mem_frame;                      // 0x2f1332c
extern int gl_square_textures;                        // 0x2f13330 never set (name ours)
extern ubyte *gl_convert_buffer;                      // 0x2f13334 max_texture_size^2 * 4 bytes
extern bool gl_context_initialized;                   // 0x2f13338
extern gr::Mode gr_opengl_current_mode;               // 0x2f1333c
extern HGLRC gl_hglrc;                                // 0x2f13340
extern gl_active_texture_proc gl_active_texture_arb;          // 0x2f13344 glActiveTextureARB
extern gl_active_texture_proc gl_client_active_texture_arb;   // 0x2f13348 glClientActiveTextureARB
extern HWND gl_hwnd;                                  // 0x2f13350
extern HDC gl_hdc;                                    // 0x2f13354
extern int gl_pixel_format;                           // 0x2f13358
extern bool gl_packed_pixels_supported;               // 0x2f1335c OpenGL 1.2 or GL_APPLE_packed_pixel (name ours)
extern float gl_scale_2f13360;                        // 0x2f13360 (written only; name ours)

// ---- texture conversion (unit-private in use; (tex_h, tex_w, bm_h, bm_w, src, dst), padding with 0) ----
void gl_convert_a8_to_rgba8888(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst);   // 0x54d5c0
void gl_convert_888_to_rgba8888_scaled(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst);   // 0x54d630
void gl_convert_565_to_rgb888(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst);    // 0x54d780
void gl_convert_888_to_rgb888(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst);    // 0x54d860
void gl_convert_4444_to_rgba8888(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst); // 0x54d8d0
void gl_convert_1555_to_rgba8888(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst); // 0x54da80
// sum.symbols: gl_convert_1555_to_rgba8888_scaled; the body decodes 565 (D1303).
void gl_convert_1555_to_rgba8888_scaled(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst);  // 0x54dc20
void gl_convert_8888_to_rgba8888(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst); // 0x54dda0
void gl_convert_4444_to_rgba4444(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst); // 0x54deb0
void gl_convert_1555_to_rgba5551(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst); // 0x54df20
void gl_convert_888_to_rgba5551(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst);  // 0x54df90
void gl_convert_8888_to_rgba4444(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst); // 0x54e050
// Copies the stored levels to dst, box-filters the rest down to 1x1 (formats 565, 888, 8888); returns
// the end of what it wrote.
ubyte *gl_build_mipmaps(int format, int levels, int w, int h, ubyte *src, ubyte *dst);  // 0x54e0e0

// ---- texture cache --------------------------------------------------------------------------------
void gr_opengl_tcache_close();                        // 0x54e640
void gr_opengl_texture_init();                        // 0x54e690
void gr_opengl_delete_texture(gl_tcache_entry *e);    // 0x54e740
void gr_opengl_tcache_flush();                        // 0x54e7a0
void gr_opengl_tcache_frame();                        // 0x54e800
// (bm, entry, update, unused): update re-uploads into the existing texture (dirty bitmaps).
void gr_opengl_create_texture(int bm, gl_tcache_entry *e, int update, int unused);   // 0x54e840
bool gr_opengl_get_texture_scale(int bm, float *u_scale, float *v_scale);   // 0x54ed50 (no symbol, no caller; name ours)
// Binds bm on a texture stage (0 / 1), uploading it when needed; -1 unbinds. Returns 1 when bound.
int  gr_opengl_tcache_set(int bm, float *u_scale, float *v_scale, int stage);   // 0x54ed90
void gr_opengl_mark_texture_dirty(int bm);            // 0x54eed0
void gr_opengl_tcache_free(int bm);                   // 0x54ef20
void gr_opengl_set_flag_5ad7e8(bool on);              // 0x54ef90 (no caller)
void gr_opengl_set_flag_2f13318(bool on);             // 0x54efa0 (no caller)

// ---- state ----------------------------------------------------------------------------------------
gr::Mode *gr_opengl_get_current_mode();               // 0x54efd0
void gr_opengl_set_mode(gr::Mode mode);               // 0x54efe0
void gr_opengl_page_in(int bm, int stage);            // 0x54f320
void gr_opengl_set_bitmap(gr::Mode mode, int bm, int stage);   // 0x54f350
void gl_init_multitexture();                          // 0x54f370
bool gl_extension_supported(const char *name, const char *extensions);   // 0x54f3f0

// ---- init / frame ---------------------------------------------------------------------------------
void gr_opengl_init_context(HWND hwnd);               // 0x54f410 (also gr_init_context)
int  gr_opengl_set_pixel_format(HDC hdc);             // 0x54f680 1 on success
void gr_opengl_init(HWND hwnd);                       // 0x54f6f0
void gr_opengl_close();                               // 0x54f790
void gr_opengl_flip();                                // 0x54f7c0
void gr_opengl_clear();                               // 0x54f7e0
void gr_opengl_zbuffer_clear();                       // 0x54f870
void gr_opengl_set_viewport();                        // 0x54f8c0 (gr::set_clip)

// ---- drawing --------------------------------------------------------------------------------------
void gr_opengl_apply_tint(ubyte *rgb);                // 0x54f9a0
void gr_opengl_bitmap(int bm, int x, int y, int w, int h, int sx, int sy, int sw, int sh,
                      gr::Mode mode);                 // 0x54fa30
void gr_opengl_set_color();                           // 0x54fe10 empty
void gr_opengl_set_scale_2f13360(int v);              // 0x54fe20 (no caller)
void gr_opengl_line(float x1, float y1, float x2, float y2, gr::Mode mode);   // 0x54fe40
void gr_opengl_line_3d(gr_vertex *v1, gr_vertex *v2, gr::Mode mode);           // 0x54ff00
void gl_setup_multitexture_state();                   // 0x54ffa0
void gr_opengl_tmapper(int nv, gr_vertex **verts, gr::Mode mode);              // 0x550080
int  gr_opengl_read_backbuffer(int x, int y, int w, int h, ubyte *buf);        // 0x5502c0 returns 7 (8888)
void gr_opengl_invalidate_mode();                     // 0x5503b0 (no caller)
void gr_opengl_set_fog(int set_range);                // 0x5503c0 gr_set_fog passes 1
void gr_opengl_setup_view();                          // 0x550500 (no caller)
void gl_set_vertex_arrays(float *verts, int unused, ubyte *colors);   // 0x5505d0 (no caller)
void gr_opengl_set_antialias(bool on);                // 0x550630 empty (bool: gr_set_antialias pushes the dword unmasked, D6801)
void clamp_ubyte(ubyte *v, ubyte lo, ubyte hi);       // 0x550640

#endif
