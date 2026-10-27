// modern/gl_redirect.h - the OpenGL back end's entry points, redirected so that the Vulkan back end can take its
// place (docs/future/vulkan-renderer.md, rung L4). Not original source. 64-bit MODERN_BUILD only.
//
// The game selects its renderer by gr_screen.mode (gr.cpp and ~45 other sites). Vulkan does not get a fourth mode:
// with [Display] Renderer=Vulkan it runs in GR_OPENGL's slot, so no dispatch site changes (their bytes are part of
// the VC6 match) and everything that tests for GR_OPENGL (widescreen, hi-res, movies, vsync) applies to it.
//
// How: vsdk/gr_opengl.cpp includes this header first under VK_REDIRECT, which renames each entry point there to
// <name>_gl. The original names are then defined by modern/vk_dispatch.cpp, which calls vk_<name> when the Vulkan
// back end is active and <name>_gl otherwise. levelrender_draw_list_opengl and gr_opengl_render_character_mesh,
// which draw with OpenGL themselves, are renamed around their definitions only (their units also call them).
//
// GL_REDIRECT_ENTRIES(X): X(return type, name, (parameters), (arguments)) for the entry points called from outside
// gr_opengl.cpp. gr_opengl_init_context and gr_opengl_init are renamed as well but dispatched by hand (they choose
// the back end). The unit's helpers without outside callers (the converters, gl_init_multitexture, the no-caller
// setters) are not redirected.
#ifndef MODERN_GL_REDIRECT_H
#define MODERN_GL_REDIRECT_H

#if defined(MODERN_BUILD) && defined(_WIN64)
#define VK_REDIRECT 1

#define GL_REDIRECT_ENTRIES(X) \
    X(void, gr_opengl_tcache_close, (), ()) \
    X(void, gr_opengl_texture_init, (), ()) \
    X(void, gr_opengl_tcache_flush, (), ()) \
    X(void, gr_opengl_tcache_frame, (), ()) \
    X(int, gr_opengl_tcache_set, (int bm, float *u_scale, float *v_scale, int stage), (bm, u_scale, v_scale, stage)) \
    X(void, gr_opengl_mark_texture_dirty, (int bm), (bm)) \
    X(void, gr_opengl_tcache_free, (int bm), (bm)) \
    X(gr::Mode *, gr_opengl_get_current_mode, (), ()) \
    X(void, gr_opengl_set_mode, (gr::Mode mode), (mode)) \
    X(void, gr_opengl_page_in, (int bm, int stage), (bm, stage)) \
    X(void, gr_opengl_set_bitmap, (gr::Mode mode, int bm, int stage), (mode, bm, stage)) \
    X(void, gr_opengl_close, (), ()) \
    X(void, gr_opengl_flip, (), ()) \
    X(void, gr_opengl_clear, (), ()) \
    X(void, gr_opengl_zbuffer_clear, (), ()) \
    X(void, gr_opengl_set_viewport, (), ()) \
    X(void, gr_opengl_apply_tint, (ubyte *rgb), (rgb)) \
    X(void, gr_opengl_bitmap, (int bm, int x, int y, int w, int h, int sx, int sy, int sw, int sh, gr::Mode mode), \
      (bm, x, y, w, h, sx, sy, sw, sh, mode)) \
    X(void, gr_opengl_set_color, (), ()) \
    X(void, gr_opengl_line, (float x1, float y1, float x2, float y2, gr::Mode mode), (x1, y1, x2, y2, mode)) \
    X(void, gr_opengl_line_3d, (gr_vertex *v1, gr_vertex *v2, gr::Mode mode), (v1, v2, mode)) \
    X(void, gr_opengl_tmapper, (int nv, gr_vertex **verts, gr::Mode mode), (nv, verts, mode)) \
    X(int, gr_opengl_read_backbuffer, (int x, int y, int w, int h, ubyte *buf), (x, y, w, h, buf)) \
    X(void, gr_opengl_set_fog, (int set_range), (set_range)) \
    X(void, gr_opengl_set_antialias, (bool on), (on))

// vsdk/gr_opengl.cpp: its definitions (and its own calls) use the _gl names.
#ifdef GL_REDIRECT_RENAME
#define gr_opengl_tcache_close gr_opengl_tcache_close_gl
#define gr_opengl_texture_init gr_opengl_texture_init_gl
#define gr_opengl_tcache_flush gr_opengl_tcache_flush_gl
#define gr_opengl_tcache_frame gr_opengl_tcache_frame_gl
#define gr_opengl_tcache_set gr_opengl_tcache_set_gl
#define gr_opengl_mark_texture_dirty gr_opengl_mark_texture_dirty_gl
#define gr_opengl_tcache_free gr_opengl_tcache_free_gl
#define gr_opengl_get_current_mode gr_opengl_get_current_mode_gl
#define gr_opengl_set_mode gr_opengl_set_mode_gl
#define gr_opengl_page_in gr_opengl_page_in_gl
#define gr_opengl_set_bitmap gr_opengl_set_bitmap_gl
#define gr_opengl_init_context gr_opengl_init_context_gl
#define gr_opengl_init gr_opengl_init_gl
#define gr_opengl_close gr_opengl_close_gl
#define gr_opengl_flip gr_opengl_flip_gl
#define gr_opengl_clear gr_opengl_clear_gl
#define gr_opengl_zbuffer_clear gr_opengl_zbuffer_clear_gl
#define gr_opengl_set_viewport gr_opengl_set_viewport_gl
#define gr_opengl_apply_tint gr_opengl_apply_tint_gl
#define gr_opengl_bitmap gr_opengl_bitmap_gl
#define gr_opengl_set_color gr_opengl_set_color_gl
#define gr_opengl_line gr_opengl_line_gl
#define gr_opengl_line_3d gr_opengl_line_3d_gl
#define gr_opengl_tmapper gr_opengl_tmapper_gl
#define gr_opengl_read_backbuffer gr_opengl_read_backbuffer_gl
#define gr_opengl_set_fog gr_opengl_set_fog_gl
#define gr_opengl_set_antialias gr_opengl_set_antialias_gl
#endif

#endif   // MODERN_BUILD && _WIN64
#endif
