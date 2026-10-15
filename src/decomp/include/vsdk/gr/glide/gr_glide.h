// vsdk/gr/glide/gr_glide.h - the 3dfx Glide back end (vsdk\gr\glide\gr_glide.cpp, 0x5479c0-0x54bb5f in
// sum.symbols; renderer 0x67). Not written yet: only the entry points gr.cpp dispatches to are declared
// (decomp40), with the signatures read from gr.cpp's pushes. Not original source.
#ifndef VSDK_GR_GLIDE_GR_GLIDE_H
#define VSDK_GR_GLIDE_GR_GLIDE_H

#include "vsdk/vsdk.h"
#include "vsdk/gr/gr.h"

void gr_glide_tcache_flush();                         // 0x547ba0
void gr_glide_mark_texture_dirty(int bm);             // 0x5496e0
void gr_glide_tcache_free(int bm);                    // 0x549730
void gr_glide_close();                                // 0x549cc0
void gr_glide_init(HWND hwnd);                        // 0x549d10 (return not used by gr_init)
bool gr_glide_is_mode_supported(gr::Mode *mode);      // 0x54a0b0 (gr_is_mode_supported passes the pointer on)
int  gr_glide_get_video_memory_mb();                  // 0x54a0d0
void gr_glide_set_viewport();                         // 0x54a5b0
void gr_glide_clear();                                // 0x54a600
void gr_glide_zbuffer_clear();                        // 0x54a650
void gr_glide_deactivate();                           // 0x54a6b0
void gr_glide_finish();                               // 0x54a6d0
void gr_glide_flip();                                 // 0x54a6e0
void gr_glide_page_in(int bm, int flags);             // 0x54a780 (gr_page_in passes both arguments)
void gr_glide_bitmap(int bm, int x, int y, int w, int h, int sx, int sy, int sw, int sh,
                     gr::Mode mode);                  // 0x54a800
void gr_glide_line(float x1, float y1, float x2, float y2, gr::Mode mode);   // 0x54af10
void gr_glide_tmapper(int nv, gr_vertex **verts, gr::Mode mode);              // 0x54b240
void gr_glide_tmapper_multitexture(int nv, gr_multipass_vertex **verts, int npasses,
                                   gr_tmapper_pass *passes);                  // 0x54b3a0
int  gr_glide_read_backbuffer(int x, int y, int w, int h, void *buf);         // 0x54b980
void gr_glide_set_fog(int set_range);                 // 0x54b9b0 gr_set_fog passes 1
void gr_glide_set_antialias(bool on);                 // 0x54ba90 empty
void gr_glide_set_alpha_test(bool on);                // 0x54baa0
void gr_glide_set_gamma();                            // 0x54bac0
void gr_glide_flush_pipeline();                       // 0x54bad0

#endif
