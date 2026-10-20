// gr_opengl.cpp - Sum.exe v1.40, code 0x54d5c0-0x550658 (vsdk\gr\opengl\gr_opengl.cpp), the OpenGL back
// end (renderer 0x68). The retail game never selects it; SummonerFix patches LoadSettings to use it and
// hooks four of the API calls made here (see NOTES.md).
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Like the other vsdk objects this unit calls the header inlines out of line (gr::Mode's accessors,
// vector3's constructors through `vector constructor iterator`), so it was built without inline
// expansion (DECISIONS.md D500, D1300).
//
// Data of this unit: .data 0x5ad7c8-0x5ad9df (the globals in two groups, the string literals in function
// order between them; "Invalid window_mode passed to gr_direct3d_init" is the unit's own copy-paste),
// .bss 0x2f13318-0x2f13363. One static initialiser (table 0x588a1c): gr_opengl_current_mode.
//
// Not source: 0x5504f0 is the linker's import thunk for glFinish (jmp [0x57e318], module "-"); only
// gr_finish in gr.cpp reaches it, this unit calls the import directly.

#include <string.h>
#include <stdio.h>
#include "vsdk/os/debug.h"
#include "vsdk/os/console.h"
#include "vsdk/os/memory.h"
#include "vsdk/os/registry.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/opengl/gl_min.h"
#include "vsdk/gr/opengl/gr_opengl.h"

// Original __FILE__ of this unit (0x5ad91c and 0x5ad988; string pooling is off).
#define GR_OPENGL_CPP "D:\\projects\\Summoner\\pccode\\vsdk\\gr\\opengl\\gr_opengl.cpp"

// Unsigned 16.16 steps of the resampling converters (a whole texel: they copy 1:1).
#define GL_STEP_ONE 65536

// ---- .data, first group (0x5ad7c8-0x5ad7ef) ---------------------------------------------------------
int gr_opengl_bound_texture[2] = { -1, -1 };          // 0x5ad7c8 / 0x5ad7cc (D1301)
int gl_unused_5ad7d0 = -1;                            // 0x5ad7d0 no reference (a third stage?)
int gl_min_texture_w = 1;                             // 0x5ad7d4
int gl_min_texture_h = 1;                             // 0x5ad7d8
int gl_unused_5ad7dc = 4;                             // 0x5ad7dc no reference
int gl_unused_5ad7e0 = 1;                             // 0x5ad7e0 no reference
int gr_opengl_packed_pixels = 1;                      // 0x5ad7e4
int gl_flag_5ad7e8 = 1;                               // 0x5ad7e8
int gr_opengl_next_texture_id = 1;                    // 0x5ad7ec

// ---- .bss, in address order ------------------------------------------------------------------------
// gr_opengl_current_mode (0x2f1333c) is defined further down with its initialiser.
int gl_flag_2f13318;                                  // 0x2f13318
gl_tcache_entry *gl_tcache_entries;                   // 0x2f1331c
int gl_unused_2f13320;                                // 0x2f13320 no reference
int gl_frame_count;                                   // 0x2f13324
int gl_texture_mem_used;                              // 0x2f13328
int gl_texture_mem_frame;                             // 0x2f1332c
int gl_square_textures;                               // 0x2f13330
ubyte *gl_convert_buffer;                             // 0x2f13334
bool gl_context_initialized;                          // 0x2f13338
HGLRC gl_hglrc;                                       // 0x2f13340
gl_active_texture_proc gl_active_texture_arb;         // 0x2f13344
gl_active_texture_proc gl_client_active_texture_arb;  // 0x2f13348
int gl_unused_2f1334c;                                // 0x2f1334c no reference
HWND gl_hwnd;                                         // 0x2f13350
HDC gl_hdc;                                           // 0x2f13354
int gl_pixel_format;                                  // 0x2f13358
bool gl_packed_pixels_supported;                      // 0x2f1335c
float gl_scale_2f13360;                               // 0x2f13360

// ---- texture conversion -------------------------------------------------------------------------------
// All take (tex_h, tex_w, bm_h, bm_w, src, dst): the bitmap (bm_w x bm_h) goes into the top left corner
// of the power-of-two texture (tex_w x tex_h) and the rest is zero. Two styles: the per-texel test
// (x < bm_w && y < bm_h) and the 16.16 "resampling" loops whose step is a whole texel.

// 0x54d5c0  Confidence: likely (differs under cl 8966 in match26).
// BM_FORMAT_8_ALPHA: white with the gamma-corrected alpha.
void gl_convert_a8_to_rgba8888(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst)
{
    uint *d = (uint *)dst;
    for (int y = 0; y < tex_h; y++) {
        for (int x = 0; x < tex_w; x++) {
            if (x < bm_w && y < bm_h)
                *d = (gr_gamma_ramp[src[x]] << 24) | 0xffffff;
            else
                *d = 0;
            d++;
        }
        src += bm_w;
    }
}

// 0x54d630  Confidence: likely (loop shape). No caller.
// 888 (B, G, R bytes) to RGBA; pure green (0, 255, 0) becomes transparent black.
void gl_convert_888_to_rgba8888_scaled(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst)
{
    uint *row = (uint *)dst;
    int x, y;
    int v = 0;
    for (y = 0; y < bm_h; y++) {
        uint *d = row;
        int u = 0;
        for (x = 0; x < bm_w; x++) {
            ubyte *s = src + ((v / GL_STEP_ONE) * bm_w + u / GL_STEP_ONE) * 3;
            uint r, g, b, a;
            if (s[0] == 0 && s[1] == 0xff && s[2] == 0) {
                r = 0;
                g = 0;
                b = 0;
                a = 0;
            } else {
                r = s[2];
                g = s[1];
                b = s[0];
                a = 0xff;
            }
            *d++ = (((((a << 8) | b) << 8) | g) << 8) | r;
            u += GL_STEP_ONE;
        }
        for (; x < tex_w; x++)
            *d++ = 0;
        v += GL_STEP_ONE;
        row += tex_w;
    }
    uint *pad = (uint *)dst + bm_h * tex_w;
    for (y = bm_h; y < tex_h; y++) {
        for (x = 0; x < tex_w; x++)
            pad[x] = 0;
        pad += tex_w;
    }
}

// 0x54d780  Confidence: exact.
// BM_FORMAT_565 to R, G, B bytes (GL_RGB / GL_UNSIGNED_BYTE).
void gl_convert_565_to_rgb888(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst)
{
    ushort *s = (ushort *)src;
    for (int y = 0; y < tex_h; y++) {
        for (int x = 0; x < tex_w; x++) {
            if (x < bm_w && y < bm_h) {
                uint p = *s;
                *dst++ = (ubyte)((int)((p >> 11) * 255) / 31);
                *dst++ = (ubyte)((int)(((p >> 5) & 0x3f) * 255) / 63);
                *dst++ = (ubyte)((int)((p & 0x1f) * 255) / 31);
                s++;
            } else {
                *dst++ = 0;
                *dst++ = 0;
                *dst++ = 0;
            }
        }
    }
}

// 0x54d860  Confidence: exact.
// BM_FORMAT_888 (B, G, R) to R, G, B bytes.
void gl_convert_888_to_rgb888(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst)
{
    ubyte *s = src;
    for (int y = 0; y < tex_h; y++) {
        for (int x = 0; x < tex_w; x++) {
            if (x < bm_w && y < bm_h) {
                *dst++ = s[2];
                *dst++ = s[1];
                *dst++ = s[0];
                s += 3;
            } else {
                *dst++ = 0;
                *dst++ = 0;
                *dst++ = 0;
            }
        }
    }
}

// 0x54d8d0  Confidence: likely (loop shape as 0x54d630).
// BM_FORMAT_4444 (A in the top nibble, then R, G, B) to RGBA bytes.
void gl_convert_4444_to_rgba8888(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst)
{
    ushort *s = (ushort *)src;
    uint *row = (uint *)dst;
    int x, y;
    int v = 0;
    for (y = 0; y < bm_h; y++) {
        uint *d = row;
        int u = 0;
        for (x = 0; x < bm_w; x++) {
            uint p = s[(v / GL_STEP_ONE) * bm_w + u / GL_STEP_ONE];
            int a = (int)((p >> 12) * 255) / 15;
            int b = (int)((p & 0xf) * 255) / 15;
            int g = (int)(((p >> 4) & 0xf) * 255) / 15;
            int r = (int)(((p >> 8) & 0xf) * 255) / 15;
            *d++ = (((((a << 8) | b) << 8) | g) << 8) | r;
            u += GL_STEP_ONE;
        }
        for (; x < tex_w; x++)
            *d++ = 0;
        v += GL_STEP_ONE;
        row += tex_w;
    }
    uint *pad = (uint *)dst + bm_h * tex_w;
    for (y = bm_h; y < tex_h; y++) {
        for (x = 0; x < tex_w; x++)
            pad[x] = 0;
        pad += tex_w;
    }
}

// 0x54da80  Confidence: likely (loop shape as 0x54d630).
// BM_FORMAT_1555 to RGBA bytes. A set top bit means transparent (alpha 0), as in gl_convert_1555_to_rgba5551.
void gl_convert_1555_to_rgba8888(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst)
{
    ushort *s = (ushort *)src;
    uint *row = (uint *)dst;
    int x, y;
    int v = 0;
    for (y = 0; y < bm_h; y++) {
        uint *d = row;
        int u = 0;
        for (x = 0; x < bm_w; x++) {
            ushort p = s[(v / GL_STEP_ONE) * bm_w + u / GL_STEP_ONE];
            ubyte a = (p & 0x8000) ? 0 : 0xff;
            int b = (int)((p & 0x1f) * 255) / 31;
            int g = (int)(((p >> 5) & 0x1f) * 255) / 31;
            int r = (int)(((p >> 10) & 0x1f) * 255) / 31;
            *d++ = (((((a << 8) | b) << 8) | g) << 8) | r;
            u += GL_STEP_ONE;
        }
        for (; x < tex_w; x++)
            *d++ = 0;
        v += GL_STEP_ONE;
        row += tex_w;
    }
    uint *pad = (uint *)dst + bm_h * tex_w;
    for (y = bm_h; y < tex_h; y++) {
        for (x = 0; x < tex_w; x++)
            pad[x] = 0;
        pad += tex_w;
    }
}

// 0x54dc20  Confidence: likely (loop shape as 0x54d630). No caller.
// sum.symbols calls it the 1555 variant, but it decodes 565 (6-bit green at bit 5, red at bit 11) to
// opaque RGBA (D1303).
void gl_convert_1555_to_rgba8888_scaled(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst)
{
    ushort *s = (ushort *)src;
    uint *row = (uint *)dst;
    int x, y;
    int v = 0;
    for (y = 0; y < bm_h; y++) {
        uint *d = row;
        int u = 0;
        for (x = 0; x < bm_w; x++) {
            uint p = s[(v / GL_STEP_ONE) * bm_w + u / GL_STEP_ONE];
            int g = (int)(((p >> 5) & 0x3f) * 255) / 63;
            int b = (int)((p & 0x1f) * 255) / 31;
            int r = (int)((p >> 11) * 255) / 31;
            *d++ = (((((0xff << 8) | b) << 8) | g) << 8) | r;
            u += GL_STEP_ONE;
        }
        for (; x < tex_w; x++)
            *d++ = 0;
        v += GL_STEP_ONE;
        row += tex_w;
    }
    uint *pad = (uint *)dst + bm_h * tex_w;
    for (y = bm_h; y < tex_h; y++) {
        for (x = 0; x < tex_w; x++)
            pad[x] = 0;
        pad += tex_w;
    }
}

// 0x54dda0  Confidence: likely (loop shape as 0x54d630).
// BM_FORMAT_8888 (B, G, R, A bytes) to RGBA bytes.
void gl_convert_8888_to_rgba8888(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst)
{
    uint *row = (uint *)dst;
    int x, y;
    int v = 0;
    for (y = 0; y < bm_h; y++) {
        uint *d = row;
        int u = 0;
        for (x = 0; x < bm_w; x++) {
            ubyte *s = src + ((v / GL_STEP_ONE) * bm_w + u / GL_STEP_ONE) * 4;
            *d++ = (((((s[3] << 8) | s[0]) << 8) | s[1]) << 8) | s[2];
            u += GL_STEP_ONE;
        }
        for (; x < tex_w; x++)
            *d++ = 0;
        v += GL_STEP_ONE;
        row += tex_w;
    }
    uint *pad = (uint *)dst + bm_h * tex_w;
    for (y = bm_h; y < tex_h; y++) {
        for (x = 0; x < tex_w; x++)
            pad[x] = 0;
        pad += tex_w;
    }
}

// 0x54deb0  Confidence: likely (differs under cl 8966 in match26).
// BM_FORMAT_4444 with packed pixels: ARGB4444 rotated to RGBA4444 (GL_UNSIGNED_SHORT_4_4_4_4).
void gl_convert_4444_to_rgba4444(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst)
{
    ushort *s = (ushort *)src;
    ushort *d = (ushort *)dst;
    for (int y = 0; y < tex_h; y++) {
        for (int x = 0; x < tex_w; x++) {
            if (x < bm_w && y < bm_h) {
                ushort p = *s;
                *d++ = (ushort)((p >> 12) | (p << 4));
                s++;
            } else {
                *d++ = 0;
            }
        }
    }
}

// 0x54df20  Confidence: likely (differs under cl 8966 in match26).
// BM_FORMAT_1555 with packed pixels: colour bits up one, the inverted alpha bit at bit 0 (5551).
void gl_convert_1555_to_rgba5551(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst)
{
    ushort *s = (ushort *)src;
    ushort *d = (ushort *)dst;
    for (int y = 0; y < tex_h; y++) {
        for (int x = 0; x < tex_w; x++) {
            if (x < bm_w && y < bm_h) {
                ushort p = *s;
                *d++ = (ushort)(((ushort)~p >> 15) | (p << 1));
                s++;
            } else {
                *d++ = 0;
            }
        }
    }
}

// 0x54df90  Confidence: likely (differs under cl 8966 in match26).
// BM_FORMAT_888 with packed pixels: 5551 with alpha 1; pure green is transparent.
void gl_convert_888_to_rgba5551(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst)
{
    ushort *d = (ushort *)dst;
    for (int y = 0; y < tex_h; y++) {
        for (int x = 0; x < tex_w; x++) {
            if (x < bm_w && y < bm_h) {
                uint r, g, b, a;
                if (src[0] == 0 && src[1] == 0xff && src[2] == 0) {
                    b = 0;
                    g = 0;
                    r = 0;
                    a = 0;
                } else {
                    r = src[2] >> 3;
                    g = src[1] >> 3;
                    b = src[0] >> 3;
                    a = 1;
                }
                *d++ = (ushort)((((((r << 5) | g) << 5) | b) << 1) | a);
                src += 3;
            } else {
                *d++ = 0;
            }
        }
    }
}

// 0x54e050  Confidence: likely (differs under cl 8966 in match26).
// BM_FORMAT_8888 with packed pixels: the top nibbles as RGBA4444.
void gl_convert_8888_to_rgba4444(int tex_h, int tex_w, int bm_h, int bm_w, ubyte *src, ubyte *dst)
{
    ushort *d = (ushort *)dst;
    for (int y = 0; y < tex_h; y++) {
        for (int x = 0; x < tex_w; x++) {
            if (x < bm_w && y < bm_h) {
                *d = (ushort)((((((((ushort)(src[2] >> 4) << 4) | (ushort)(src[1] >> 4)) << 4)
                                 | (ushort)(src[0] >> 4)) << 4) | (ushort)(src[3] >> 4)));
                d++;
                src += 4;
            } else {
                *d = 0;
                d++;
            }
        }
    }
}

// 0x54e0e0  Confidence: likely (the three filter loops' index arithmetic).
// Copies the bitmap's stored levels to dst, then box-filters each further level from the previous one
// until a 1x1 level is written. Only 565, 888 and 8888 are filtered (other formats get no extra
// levels written but the loop still runs down to 1x1). The 565 filter never skips the second source row:
// its source pointer only moves on by two texels per output texel (0x54e5a8), so every output row
// averages source rows k and k + 1 and the bottom half of each level is never used.
ubyte *gl_build_mipmaps(int format, int levels, int w, int h, ubyte *src, ubyte *dst)
{
    int bpp = bm_bytes_per_pixel(format);
    ubyte *prev = src;
    int i, x, y;
    for (i = 0; i < levels; i++) {
        int size = h * w * bpp;
        prev = src;
        memcpy(dst, src, size);
        dst += size;
        src += size;
        w /= 2;
        h /= 2;
    }
    int pw = w + w;
    int ph = h + h;
    for (;;) {
        ubyte *level = dst;
        switch (format) {
        case BM_FORMAT_565: {
            ushort *s = (ushort *)prev;
            ushort *d = (ushort *)dst;
            for (y = 0; y < ph; y += 2) {
                for (x = 0; x < pw; x += 2) {
                    uint p0 = s[0];
                    uint p1 = s[1];
                    uint p2 = s[pw];
                    uint p3 = s[pw + 1];
                    int g = (int)(((p3 >> 5) & 0x3f) + ((p0 >> 5) & 0x3f) + ((p1 >> 5) & 0x3f)
                                  + ((p2 >> 5) & 0x3f)) / 4;
                    int r = (int)((p3 >> 11) + (p0 >> 11) + (p1 >> 11) + (p2 >> 11)) / 4;
                    int b = (int)((p3 & 0x1f) + (p0 & 0x1f) + (p1 & 0x1f) + (p2 & 0x1f)) / 4;
                    *d++ = (ushort)((((r << 6) | g) << 5) | b);
                    s += 2;
                    dst += 2;
                }
            }
            break;
        }
        case BM_FORMAT_888:
            for (y = 0; y < h; y++) {
                for (x = 0; x < w; x++) {
                    ubyte *s0 = prev + y * pw * 6 + x * 6;
                    ubyte *s1 = s0 + pw * 3;
                    dst[0] = (ubyte)((s0[0] + s0[3] + s1[0] + s1[3]) / 4);
                    dst[1] = (ubyte)((s0[1] + s0[4] + s1[1] + s1[4]) / 4);
                    dst[2] = (ubyte)((s0[2] + s0[5] + s1[2] + s1[5]) / 4);
                    dst += 3;
                }
            }
            break;
        case BM_FORMAT_8888:
            for (y = 0; y < h; y++) {
                for (x = 0; x < w; x++) {
                    ubyte *s0 = prev + y * pw * 8 + x * 8;
                    ubyte *s1 = s0 + pw * 4;
                    dst[0] = (ubyte)((s0[0] + s0[4] + s1[0] + s1[4]) / 4);
                    dst[1] = (ubyte)((s0[1] + s0[5] + s1[1] + s1[5]) / 4);
                    dst[2] = (ubyte)((s0[2] + s0[6] + s1[2] + s1[6]) / 4);
                    dst[3] = (ubyte)((s0[3] + s0[7] + s1[3] + s1[7]) / 4);
                    dst += 4;
                }
            }
            break;
        }
        pw = w;
        ph = h;
        w /= 2;
        h /= 2;
        if (w < 1)
            w = 1;
        if (h < 1)
            h = 1;
        prev = level;
        if (pw == 1 && ph == 1)
            break;
    }
    return dst;
}

// ---- texture cache -----------------------------------------------------------------------------------

// 0x54e640  Confidence: exact.
void gr_opengl_tcache_close()
{
    gr_opengl_tcache_flush();
    gl_texture_mem_used = 0;
    gl_texture_mem_frame = 0;
    if (gl_tcache_entries) {
        delete gl_tcache_entries;
        gl_tcache_entries = NULL;
    }
    if (gl_convert_buffer) {
        delete gl_convert_buffer;
        gl_convert_buffer = NULL;
    }
}

// 0x54e690  Confidence: exact (operator new spelled out: the size is the byte count of the cache).
// Called once, at the end of the first gr_opengl_init_context. The scratch buffer is
// GL_MAX_TEXTURE_SIZE^2 * 4 bytes: 1 GB on drivers that report 16384 (SummonerFix caps the query).
void gr_opengl_texture_init()
{
    GLint max_size;
    os_registry_read_dword(NULL, "PackedPixels", (DWORD *)&gr_opengl_packed_pixels, 1);
    gl_tcache_entries = (gl_tcache_entry *)operator new(MAX_BITMAPS * sizeof(gl_tcache_entry));
    if (gl_tcache_entries == NULL)
        exit(1);
    gl_tcache_entry *t = gl_tcache_entries;
    for (int i = 0; i < MAX_BITMAPS; i++) {
        t[i].gl_id = -1;
        t[i].bitmap = -1;
        t[i].size = 0;
        t[i].used = 0;
    }
    gr_opengl_bound_texture[0] = -1;
    gr_opengl_bound_texture[1] = -1;
    gl_texture_mem_used = 0;
    gl_texture_mem_frame = 0;
    gr_opengl_next_texture_id = 1;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
#ifdef REBUILT_OPENGL
    // Standalone test builds only, as SummonerFix's glfix.c: a modern driver's 16384 makes the scratch buffer
    // below 1 GB, which fails in a 32-bit process; the game's textures are far smaller.
    if (max_size > 2048)
        max_size = 2048;
#endif
    gr_screen.max_bitmap_w = max_size;
    gr_screen.max_bitmap_h = max_size;
    gl_convert_buffer = (ubyte *)operator new(max_size * max_size * 4);
}

// 0x54e740  Confidence: exact.
// Leaves a slot used this frame alone (so a flush can leave textures resident).
void gr_opengl_delete_texture(gl_tcache_entry *e)
{
    if (e->bitmap > -1 && e->used == 0) {
        GLuint id = e->gl_id;
        glDeleteTextures(1, &id);
        if (gr_opengl_bound_texture[0] == e->bitmap)
            gr_opengl_bound_texture[0] = -1;
        if (gr_opengl_bound_texture[1] == e->bitmap)
            gr_opengl_bound_texture[1] = -1;
        gl_texture_mem_used -= e->size;
        e->bitmap = -1;
    }
}

// 0x54e7a0  Confidence: exact.
// The warning is only formatted into string_buffer, never shown.
void gr_opengl_tcache_flush()
{
    glFinish();
    for (int i = 0; i < MAX_BITMAPS; i++)
        gr_opengl_delete_texture(&gl_tcache_entries[i]);
    if (gl_texture_mem_used) {
        sprintf(string_buffer, "WARNING: VRAM is at %d instead of zero after flushing!\n", gl_texture_mem_used);
        gl_texture_mem_used = 0;
    }
    gr_opengl_bound_texture[0] = -1;
    gr_opengl_bound_texture[1] = -1;
}

// 0x54e800  Confidence: exact.
// Tail of gr_opengl_flip.
void gr_opengl_tcache_frame()
{
    gl_tcache_entry *t;
    int i;

    gr_opengl_bound_texture[0] = -1;
    gr_opengl_bound_texture[1] = -1;
    gl_texture_mem_frame = 0;
    gl_frame_count++;
    t = gl_tcache_entries;
    for (i = 0; i < MAX_BITMAPS; i++, t++)
        t->used = 0;
}

// 0x54e840  Confidence: likely (the format switch; register use and the level loop match).
// Uploads bitmap bm into the bound texture of slot e: rounds the size up to a power of two (clamped to
// gl_min_texture_w/h and GL_MAX_TEXTURE_SIZE), converts each level into gl_convert_buffer and calls
// glTexImage2D. With update set, level 0 is also sent with glTexSubImage2D first (both calls are made).
// The memory counted is 2 bytes per texel of the last level uploaded, whatever the format.
void gr_opengl_create_texture(int bm, gl_tcache_entry *e, int update, int unused)
{
    int w, h, num_pixels, levels;
    ubyte *data, *palette;
    int tex_w, tex_h, i;
    GLenum type, format_gl;
    GLint internal_format;
    ubyte *src, *dst;

    Mempool *old_pool = get_current_mempool();
    set_current_mempool(NULL);
    if (e->used)
        glFinish();
    if (!update)
        gr_opengl_delete_texture(e);
    bm_get_mipmap_info(bm, &w, &h, &num_pixels, &levels);
    int format = bm_lock(bm, &data, &palette);
    int bpp = bm_bytes_per_pixel(format);
    if (!format) {
        set_current_mempool(old_pool);
        sprintf(string_buffer, "Couldn't lock bitmap %d\n", bm);
        return;
    }

    tex_w = w;
    tex_h = h;
    for (i = 0; i < 16; i++) {
        if (w > (1 << i) && w <= (1 << (i + 1))) {
            tex_w = 1 << (i + 1);
            break;
        }
    }
    for (i = 0; i < 16; i++) {
        if (h > (1 << i) && h <= (1 << (i + 1))) {
            tex_h = 1 << (i + 1);
            break;
        }
    }
    if (tex_w < gl_min_texture_w)
        tex_w = gl_min_texture_w;
    else if (tex_w > gr_screen.max_bitmap_w)
        tex_w = gr_screen.max_bitmap_w;
    if (tex_h < gl_min_texture_h)
        tex_h = gl_min_texture_h;
    else if (tex_h > gr_screen.max_bitmap_h)
        tex_h = gr_screen.max_bitmap_h;
    if (gl_square_textures) {
        if (tex_w > tex_h)
            tex_h = tex_w;
        tex_w = tex_h;
    }
    if (tex_w < 1 || tex_h < 1) {
        bm_unlock(bm);                                // the memory pool is not restored here
        return;
    }

    e->u_scale = (float)w / (float)tex_w;
    e->v_scale = (float)h / (float)tex_h;
    if (levels > 1) {
        src = gl_convert_buffer;
        dst = gl_build_mipmaps(format, levels, w, h, data, gl_convert_buffer);
    } else {
        src = data;
        dst = gl_convert_buffer;
    }

    int level = 0;
    for (;;) {
        switch (format) {
        case BM_FORMAT_8_ALPHA:
            gl_convert_a8_to_rgba8888(tex_h, tex_w, h, w, src, dst);
            type = GL_UNSIGNED_BYTE;
            format_gl = GL_RGBA;
            internal_format = GL_RGBA;
            break;
        case BM_FORMAT_888:
            if (gl_packed_pixels_supported && gr_opengl_packed_pixels) {
                gl_convert_888_to_rgba5551(tex_h, tex_w, h, w, src, dst);
                type = GL_UNSIGNED_SHORT_5_5_5_1;
                format_gl = GL_RGBA;
                internal_format = GL_RGB5_A1;
            } else {
                gl_convert_888_to_rgb888(tex_h, tex_w, h, w, src, dst);
                type = GL_UNSIGNED_BYTE;
                format_gl = GL_RGB;
                internal_format = GL_RGB;
            }
            break;
        case BM_FORMAT_4444:
            if (gl_packed_pixels_supported && gr_opengl_packed_pixels) {
                gl_convert_4444_to_rgba4444(tex_h, tex_w, h, w, src, dst);
                type = GL_UNSIGNED_SHORT_4_4_4_4;
                format_gl = GL_RGBA;
                internal_format = GL_RGBA4;
            } else {
                gl_convert_4444_to_rgba8888(tex_h, tex_w, h, w, src, dst);
                type = GL_UNSIGNED_BYTE;
                format_gl = GL_RGBA;
                internal_format = GL_RGBA;
            }
            break;
        case BM_FORMAT_1555:
            if (gl_packed_pixels_supported && gr_opengl_packed_pixels) {
                gl_convert_1555_to_rgba5551(tex_h, tex_w, h, w, src, dst);
                type = GL_UNSIGNED_SHORT_5_5_5_1;
                format_gl = GL_RGBA;
                internal_format = GL_RGB5_A1;
            } else {
                gl_convert_1555_to_rgba8888(tex_h, tex_w, h, w, src, dst);
                type = GL_UNSIGNED_BYTE;
                format_gl = GL_RGBA;
                internal_format = GL_RGBA;
            }
            break;
        case BM_FORMAT_565:
            gl_convert_565_to_rgb888(tex_h, tex_w, h, w, src, dst);
            type = GL_UNSIGNED_BYTE;
            format_gl = GL_RGB;
            internal_format = GL_RGB;
            break;
        case BM_FORMAT_8888:
            if (gl_packed_pixels_supported && gr_opengl_packed_pixels) {
                gl_convert_8888_to_rgba4444(tex_h, tex_w, h, w, src, dst);
                type = GL_UNSIGNED_SHORT_4_4_4_4;
                format_gl = GL_RGBA;
                internal_format = GL_RGBA4;
            } else {
                gl_convert_8888_to_rgba8888(tex_h, tex_w, h, w, src, dst);
                type = GL_UNSIGNED_BYTE;
                format_gl = GL_RGBA;
                internal_format = GL_RGBA;
            }
            break;
        default:
            type = GL_UNSIGNED_BYTE;
            format_gl = GL_RGBA;
            internal_format = GL_RGBA;
            break;
        }
        if (level == 0) {
            if (update) {
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_w, tex_h, format_gl, type, dst);
            } else {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                levels != 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
            }
        }
        glTexImage2D(GL_TEXTURE_2D, level, internal_format, tex_w, tex_h, 0, format_gl, type, dst);
        if (levels == 1)
            break;
        if (w == 1 && h == 1)
            break;
        src += bpp * w * h;
        tex_w /= 2;
        tex_h /= 2;
        if (tex_w < 1)
            tex_w = 1;
        if (tex_h < 1)
            tex_h = 1;
        w /= 2;
        h /= 2;
        if (w < 1)
            w = 1;
        if (h < 1)
            h = 1;
        level++;
    }

    bm_unlock(bm);
    int size = tex_h * tex_w * 2;
    e->bitmap = bm;
    e->size = size;
    gl_texture_mem_frame += size;
    if (!update)
        gl_texture_mem_used += size;
    set_current_mempool(old_pool);
}

// 0x54ed50  Confidence: exact. Not in sum.symbols: a hidden start after create_texture's jump table
// (at 0x54ed38), no reference. The slot's u / v scale of a bitmap without binding it. Name ours.
bool gr_opengl_get_texture_scale(int bm, float *u_scale, float *v_scale)
{
    if (bm < 0)
        return false;
    gl_tcache_entry *e = &gl_tcache_entries[bm_get_current_frame(bm)];
    *u_scale = e->u_scale;
    *v_scale = e->v_scale;
    return true;
}

// 0x54ed90  Confidence: exact.
// The early-out compares the handle passed (not the frame) with the stage's bound bitmap and the slot's
// bitmap; bound textures are reset every flip (gr_opengl_tcache_frame).
int gr_opengl_tcache_set(int bm, float *u_scale, float *v_scale, int stage)
{
    if (bm < 0) {
        gr_opengl_bound_texture[stage] = -1;
        return 0;
    }
    gl_tcache_entry *e = &gl_tcache_entries[bm_get_current_frame(bm)];
    if (gr_opengl_bound_texture[stage] == bm && e->bitmap == bm) {
        e->used++;
        *u_scale = e->u_scale;
        *v_scale = e->v_scale;
        return 1;
    }
    if (e->gl_id < 0)
        e->gl_id = gr_opengl_next_texture_id++;
    gl_active_texture_arb(GL_TEXTURE0_ARB + stage);
    gl_client_active_texture_arb(GL_TEXTURE0_ARB + stage);
    glBindTexture(GL_TEXTURE_2D, e->gl_id);
    if (e->bitmap < 0) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
        gr_opengl_create_texture(bm, e, 0, 0);
        e->dirty = false;
    }
    if (e->dirty) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
        gr_opengl_create_texture(bm, e, 1, 0);
        e->dirty = false;
    }
    *u_scale = e->u_scale;
    *v_scale = e->v_scale;
    gr_opengl_bound_texture[stage] = e->bitmap;
    e->used++;
    gr_opengl_get_current_mode()->invalidate();
    gl_active_texture_arb(GL_TEXTURE0_ARB);
    gl_client_active_texture_arb(GL_TEXTURE0_ARB);
    return 1;
}

// 0x54eed0  Confidence: exact.
void gr_opengl_mark_texture_dirty(int bm)
{
    gl_tcache_entries[bm_get_current_frame(bm)].dirty = true;
    if (gr_opengl_bound_texture[0] == bm)
        gr_opengl_bound_texture[0] = -1;
    if (gr_opengl_bound_texture[1] == bm)
        gr_opengl_bound_texture[1] = -1;
}

// 0x54ef20  Confidence: exact.
void gr_opengl_tcache_free(int bm)
{
    int frame = bm_get_current_frame(bm);
    if (gr_opengl_bound_texture[0] == bm)
        gr_opengl_bound_texture[0] = -1;
    if (gr_opengl_bound_texture[1] == bm)
        gr_opengl_bound_texture[1] = -1;
    gr_opengl_delete_texture(&gl_tcache_entries[frame]);
    gl_tcache_entries[frame].bitmap = -1;
}

// 0x54ef90  Confidence: exact. No caller; nothing reads gl_flag_5ad7e8.
void gr_opengl_set_flag_5ad7e8(bool on)
{
    gl_flag_5ad7e8 = on;
}

// 0x54efa0  Confidence: exact. No caller; nothing reads gl_flag_2f13318.
void gr_opengl_set_flag_2f13318(bool on)
{
    gl_flag_2f13318 = on;
}

// ---- state ---------------------------------------------------------------------------------------------

// 0x54efb0 init_gr_opengl_current_mode  Confidence: likely (generated: initialiser thunk, table entry 0x588a1c)
// 0x54efc0  Confidence: likely (generated: gr::Mode() on gr_opengl_current_mode, a jmp to 0x5368a0)
// The state cache of gr_opengl_set_mode; invalid (-1) so the first set_mode applies everything.
gr::Mode gr_opengl_current_mode;                      // 0x2f1333c

// 0x54efd0  Confidence: exact.
gr::Mode *gr_opengl_get_current_mode()
{
    return &gr_opengl_current_mode;
}

// ---- .data, second group (0x5ad854-0x5ad863, after create_texture's literal) --------------------------
int gl_blend_add_one_one = 1;                         // 0x5ad854
bool gr_opengl_vertex_color = true;                   // 0x5ad858
bool gr_opengl_vertex_alpha = true;                   // 0x5ad859
float gr_opengl_texture_u_scale = 1.0f;               // 0x5ad85c
float gr_opengl_texture_v_scale = 1.0f;               // 0x5ad860

// 0x54efe0  Confidence: likely (differs under cl 8966 in match26).
// Field values as Dash Faction's gr::Mode enums: texture source 0 none, 1 wrap, 2 clamp, 3 clamp without
// filtering, 4+ two textures; colour source 0 vertex, 1 texture, 2 vertex * texture; alpha source 0
// vertex, 1 none, 2 texture, 3 vertex * texture; zbuffer 0 none, 1 read, 2 read equal, 3 write, 4 full;
// fog 0-2 allowed, 3 not allowed. Filtering is not touched (clamp without filtering is plain clamp).
void gr_opengl_set_mode(gr::Mode mode)
{
    gr::Mode *cur = gr_opengl_get_current_mode();
    if (mode == *cur)
        return;
    *cur = mode;
    int texture_source = mode.get_texture_source();
    int color_source = mode.get_color_source();
    int alpha_source = mode.get_alpha_source();
    int alpha_blend = mode.get_alpha_blend();
    int zbuffer_type = mode.get_zbuffer_type();
    int fog_type = mode.get_fog_type();

    if (texture_source < 4) {
        gl_active_texture_arb(GL_TEXTURE1_ARB);
        gl_client_active_texture_arb(GL_TEXTURE1_ARB);
        glDisable(GL_TEXTURE_2D);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        gl_active_texture_arb(GL_TEXTURE0_ARB);
        gl_client_active_texture_arb(GL_TEXTURE0_ARB);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    } else {
        gl_active_texture_arb(GL_TEXTURE1_ARB);
        gl_client_active_texture_arb(GL_TEXTURE1_ARB);
        glEnable(GL_TEXTURE_2D);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        gl_active_texture_arb(GL_TEXTURE0_ARB);
        gl_client_active_texture_arb(GL_TEXTURE0_ARB);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    switch (texture_source) {
    case 0:
        glDisable(GL_TEXTURE_2D);
        gr_opengl_tcache_set(-1, NULL, NULL, 0);
        break;
    case 1:
    case 4:
        glEnable(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        break;
    case 2:
    case 3:
        glEnable(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        break;
    }

    switch (alpha_blend) {
    case 0:
        glBlendFunc(GL_ONE, GL_ZERO);
        break;
    case 1:
        if (gl_blend_add_one_one)
            glBlendFunc(GL_ONE, GL_ONE);
        else
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        break;
    case 2:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        break;
    case 3:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case 4:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
        break;
    case 5:
        glBlendFunc(GL_DST_COLOR, GL_ZERO);
        break;
    }

    switch (color_source) {
    case 0:
    case 2:
        gr_opengl_vertex_color = false;
        break;
    case 1:
        gr_opengl_vertex_color = true;
        break;
    }

    switch (alpha_source) {
    case 0:
    case 3:
        gr_opengl_vertex_alpha = false;
        break;
    case 1:
    case 2:
        gr_opengl_vertex_alpha = true;
        break;
    }

    switch (zbuffer_type) {
    case 0:
        glDisable(GL_DEPTH_TEST);
        glDepthMask(0);
        break;
    case 1:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(0);
        break;
    case 2:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_EQUAL);
        glDepthMask(0);
        break;
    case 3:
        glDisable(GL_DEPTH_TEST);
        glDepthMask(1);
        break;
    case 4:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(1);
        break;
    }

    switch (fog_type) {
    case 0:
    case 1:
    case 2:
        if (gr_fog_enabled)
            glEnable(GL_FOG);
        else
            glDisable(GL_FOG);
        break;
    case 3:
        glDisable(GL_FOG);
        break;
    }
}

// 0x54f320  Confidence: exact.
void gr_opengl_page_in(int bm, int stage)
{
    if (bm > -1)
        gr_opengl_tcache_set(bm, &gr_opengl_texture_u_scale, &gr_opengl_texture_v_scale, stage);
}

// 0x54f350  Confidence: exact.
void gr_opengl_set_bitmap(gr::Mode mode, int bm, int stage)
{
    gr_opengl_page_in(bm, stage);
    gr_opengl_set_mode(mode);
}

// 0x54f370  Confidence: exact.
// Without GL_ARB_multitexture the two entry points stay NULL, and gl_setup_multitexture_state (and every
// later state change) calls them anyway.
void gl_init_multitexture()
{
    if (gl_extension_supported("GL_ARB_multitexture", (const char *)glGetString(GL_EXTENSIONS))) {
        GLint num_units;
        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &num_units);
        if (num_units < 2) {
            MessageBoxA(NULL, "OpenGl Driver or Card does not support multitexturing\n", "Error", MB_OK);
            exit(-1);
        }
        gl_active_texture_arb = (gl_active_texture_proc)wglGetProcAddress("glActiveTextureARB");
        gl_client_active_texture_arb = (gl_active_texture_proc)wglGetProcAddress("glClientActiveTextureARB");
    }
    gl_setup_multitexture_state();
}

// 0x54f3f0  Confidence: exact.
bool gl_extension_supported(const char *name, const char *extensions)
{
    if (extensions == NULL)
        return false;
    if (strstr(extensions, name))
        return true;
    return false;
}

// ---- init / frame ----------------------------------------------------------------------------------------

// 0x54f410  Confidence: likely (the ortho / viewport block is written out as in gr_opengl_set_viewport;
// the client rectangle is computed and never used).
// Also reached from gr_init_context (0x5061b0) when the window changes. The first call sets up
// multitexturing and the texture cache.
void gr_opengl_init_context(HWND hwnd)
{
    RECT rect;
    PIXELFORMATDESCRIPTOR pfd;

    gr_opengl_current_mode.invalidate();
    if (gl_context_initialized) {
        gr_opengl_tcache_set(-1, NULL, NULL, 0);
        gr_opengl_tcache_set(-1, NULL, NULL, 1);
    }
    if (gl_hdc) {
        wglMakeCurrent(NULL, NULL);
        ReleaseDC(gl_hwnd, gl_hdc);
    }
    gl_hwnd = hwnd;
    gl_hdc = GetDC(hwnd);
    GetClientRect(gl_hwnd, &rect);
    ClientToScreen(hwnd, (POINT *)&rect.left);
    ClientToScreen(hwnd, (POINT *)&rect.right);
    if (!gr_opengl_set_pixel_format(gl_hdc))
        exit(1);
    if (!gl_hglrc)
        gl_hglrc = wglCreateContext(gl_hdc);
    wglMakeCurrent(gl_hdc, gl_hglrc);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(gr_screen.offset_x + gr_screen.clip_left,
            gr_screen.clip_width + gr_screen.offset_x + gr_screen.clip_left,
            gr_screen.clip_height + gr_screen.offset_y + gr_screen.clip_top,
            gr_screen.offset_y + gr_screen.clip_top, -1.0, 1.0);
    glViewport(gr_screen.offset_x + gr_screen.clip_left,
               gr_screen.max_h - gr_screen.clip_height - gr_screen.offset_y - gr_screen.clip_top,
               gr_screen.clip_width, gr_screen.clip_height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glAlphaFunc(GL_GREATER, 0.0f);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glDisable(GL_DITHER);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glCullFace(GL_FRONT);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    if (!gl_context_initialized) {
        gl_init_multitexture();
        DescribePixelFormat(gl_hdc, gl_pixel_format, sizeof(pfd), &pfd);
        gl_context_initialized = true;
        gr_opengl_texture_init();
    }
}

// 0x54f680  Confidence: exact.
// Double-buffered RGBA, 16-bit colour and depth.
int gr_opengl_set_pixel_format(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 16;
    pfd.cDepthBits = 16;
    gl_pixel_format = ChoosePixelFormat(hdc, &pfd);
    if (!gl_pixel_format)
        return 0;
    return SetPixelFormat(hdc, gl_pixel_format, &pfd) != FALSE;
}

// 0x54f6f0  Confidence: exact.
// Only window_mode 200 (windowed) creates the context. 201 (fullscreen) stops with the error SummonerFix
// works around; 202 (desktop size) falls through without a context, so glGetString returns NULL and the
// strstr below faults. The default message names gr_direct3d_init (copied from the Direct3D code).
void gr_opengl_init(HWND hwnd)
{
    switch (gr_screen.window_mode) {
    case 200:
        gr_opengl_init_context(hwnd);
        break;
    case 201:
        VSDK_ERROR(GR_OPENGL_CPP, 0x291, "OpenGl can only be used in windowed modes for now.\n");
    case 202:
        break;
    default:
        VSDK_ERROR(GR_OPENGL_CPP, 0x29c, "Invalid window_mode passed to gr_direct3d_init\n");
    }
    const char *version = (const char *)glGetString(GL_VERSION);
    const char *extensions = (const char *)glGetString(GL_EXTENSIONS);
    if (strstr(version, "1.2") || strstr(extensions, "GL_APPLE_packed_pixel"))
        gl_packed_pixels_supported = true;
    else
        gl_packed_pixels_supported = false;
}

// 0x54f790  Confidence: exact.
void gr_opengl_close()
{
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(gl_hglrc);
    ReleaseDC(gl_hwnd, gl_hdc);
    gr_opengl_tcache_close();
}

// 0x54f7c0  Confidence: exact.
void gr_opengl_flip()
{
    if (gl_hdc)
        SwapBuffers(gl_hdc);
    if (gl_context_initialized)
        gr_opengl_tcache_frame();
}

// 0x54f7e0  Confidence: exact.
// Black rectangle over the clip rectangle (no glClear).
void gr_opengl_clear()
{
    gr_opengl_set_mode(gr::Mode(0, 0, 0, 0, 0, 0));
    glColor4ub(0, 0, 0, 255);
    glDisable(GL_BLEND);
    glRecti(gr_screen.clip_left + gr_screen.offset_x, gr_screen.clip_top + gr_screen.offset_y,
            gr_screen.clip_right + gr_screen.offset_x + 1, gr_screen.clip_bottom + gr_screen.offset_y + 1);
    glEnable(GL_BLEND);
}

// 0x54f870  Confidence: exact.
void gr_opengl_zbuffer_clear()
{
    gr_opengl_set_mode(gr::Mode(0, 0, 0, 0, 4, 0));
    glClearDepth(1.0);
    glDepthRange(0.0, 1.0);
    glClear(GL_DEPTH_BUFFER_BIT);
}

// 0x54f8c0  Confidence: likely (differs under cl 8966 in match26).
// Pixel-space projection of the clip rectangle (gr::set_clip). SummonerFix's glViewport hook rescales it.
void gr_opengl_set_viewport()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(gr_screen.offset_x + gr_screen.clip_left,
            gr_screen.clip_width + gr_screen.offset_x + gr_screen.clip_left,
            gr_screen.clip_height + gr_screen.offset_y + gr_screen.clip_top,
            gr_screen.offset_y + gr_screen.clip_top, -1.0, 1.0);
    glViewport(gr_screen.offset_x + gr_screen.clip_left,
               gr_screen.max_h - gr_screen.clip_height - gr_screen.offset_y - gr_screen.clip_top,
               gr_screen.clip_width, gr_screen.clip_height);
    glMatrixMode(GL_MODELVIEW);
}

// ---- drawing ------------------------------------------------------------------------------------------

// 0x54f9a0  Confidence: likely (operand order of the sum).
// Each channel becomes (r + g + b) * tint factor. The result is truncated to a byte before
// clamp_ubyte runs, so the clamp never changes anything and bright colours wrap around.
void gr_opengl_apply_tint(ubyte *rgb)
{
    float sum = (float)(rgb[0] + rgb[2] + rgb[1]);
    rgb[0] = (ubyte)(sum * gr_tint_r);
    clamp_ubyte(&rgb[0], 0, 255);
    rgb[1] = (ubyte)(sum * gr_tint_g);
    clamp_ubyte(&rgb[1], 0, 255);
    rgb[2] = (ubyte)(sum * gr_tint_b);
    clamp_ubyte(&rgb[2], 0, 255);
}

// 0x54fa30  Confidence: likely (local layout; statement order of the four corners).
// (bm, x, y, w, h, sx, sy, sw, sh, mode): the back end of gr_bitmap / _ex / _stretched; one textured
// quad (GL_POLYGON) from arrays. The bitmap is set before and after filling the arrays.
void gr_opengl_bitmap(int bm, int x, int y, int w, int h, int sx, int sy, int sw, int sh, gr::Mode mode)
{
    ubyte r, g, b, a;
    int bw, bh;

    if (w < 1 || h < 1)
        return;
    gr_opengl_set_bitmap(mode, bm, 0);
    if (gr_opengl_vertex_color) {
        r = 255;
        g = 255;
        b = 255;
    } else {
        r = gr_color.r;
        g = gr_color.g;
        b = gr_color.b;
    }
    if (gr_opengl_vertex_alpha)
        a = 255;
    else
        a = gr_color.a;
    bm_get_dimensions(bm, &bw, &bh);
    float u0 = (float)sx * gr_opengl_texture_u_scale / (float)bw;
    float v0 = (float)sy * gr_opengl_texture_v_scale / (float)bh;
    float u1 = (float)(sx + sw) * gr_opengl_texture_u_scale / (float)bw;
    float v1 = (float)(sy + sh) * gr_opengl_texture_v_scale / (float)bh;
    float x0 = (float)(gr_screen.offset_x + x);
    float y0 = (float)(y + gr_screen.offset_y);
    float x1 = (float)(gr_screen.offset_x + x + w);
    float y1 = (float)(y + gr_screen.offset_y + h);
    vector3 pts[4];
    ubyte colors[4][4];
    float tc[4][4];

    pts[0].x = x0;
    pts[0].y = y0;
    pts[0].z = -1.0f;
    colors[0][0] = r;
    colors[0][1] = g;
    colors[0][2] = b;
    colors[0][3] = a;
    tc[0][0] = u0;
    tc[0][1] = v0;
    tc[0][2] = 0.0f;
    tc[0][3] = 1.0f;
    if (gr_tint_enabled)
        gr_opengl_apply_tint(colors[0]);

    pts[1].x = x1;
    pts[1].y = y0;
    pts[1].z = -1.0f;
    colors[1][0] = r;
    colors[1][1] = g;
    colors[1][2] = b;
    colors[1][3] = a;
    tc[1][0] = u1;
    tc[1][1] = v0;
    tc[1][2] = 0.0f;
    tc[1][3] = 1.0f;
    if (gr_tint_enabled)
        gr_opengl_apply_tint(colors[1]);

    pts[2].x = x1;
    pts[2].y = y1;
    pts[2].z = -1.0f;
    colors[2][0] = r;
    colors[2][1] = g;
    colors[2][2] = b;
    colors[2][3] = a;
    tc[2][0] = u1;
    tc[2][1] = v1;
    tc[2][2] = 0.0f;
    tc[2][3] = 1.0f;
    if (gr_tint_enabled)
        gr_opengl_apply_tint(colors[2]);

    pts[3].x = x0;
    pts[3].y = y1;
    pts[3].z = -1.0f;
    colors[3][0] = r;
    colors[3][1] = g;
    colors[3][2] = b;
    colors[3][3] = a;
    tc[3][0] = u0;
    tc[3][1] = v1;
    tc[3][2] = 0.0f;
    tc[3][3] = 1.0f;
    if (gr_tint_enabled)
        gr_opengl_apply_tint(colors[3]);

    gl_active_texture_arb(GL_TEXTURE0_ARB);
    gl_client_active_texture_arb(GL_TEXTURE0_ARB);
    glTexCoordPointer(4, GL_FLOAT, 0, tc);
    glVertexPointer(3, GL_FLOAT, 0, pts);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
    gr_opengl_set_bitmap(mode, bm, 0);
    glDrawArrays(GL_POLYGON, 0, 4);
}

// 0x54fe10  Confidence: exact.
void gr_opengl_set_color()
{
}

// 0x54fe20  Confidence: exact. No caller; nothing reads gl_scale_2f13360.
void gr_opengl_set_scale_2f13360(int v)
{
    float s = (float)v * 0.0625f;

    gl_scale_2f13360 = s * 0.001f;
}

// 0x54fe40  Confidence: exact.
// The colour lives in the mode argument's stack slot (VC6 reuses it once the mode is passed on).
void gr_opengl_line(float x1, float y1, float x2, float y2, gr::Mode mode)
{
    gr_opengl_set_bitmap(mode, -1, 0);
    ubyte c[4];
    c[0] = gr_color.r;
    c[1] = gr_color.g;
    c[2] = gr_color.b;
    c[3] = gr_color.a;
    if (gr_tint_enabled)
        gr_opengl_apply_tint(c);
    glColor4ubv(c);
    glBegin(GL_LINE_STRIP);
    glVertex2i((int)(gr_screen.offset_x + x1), (int)(gr_screen.offset_y + y1));
    glVertex2i((int)(gr_screen.offset_x + x2), (int)(gr_screen.offset_y + y2));
    glEnd();
}

// 0x54ff00  Confidence: exact.
void gr_opengl_line_3d(gr_vertex *v1, gr_vertex *v2, gr::Mode mode)
{
    gr_opengl_set_bitmap(mode, -1, 0);
    ubyte c[4];
    c[0] = gr_color.r;
    c[1] = gr_color.g;
    c[2] = gr_color.b;
    c[3] = gr_color.a;
    if (gr_tint_enabled)
        gr_opengl_apply_tint(c);
    glColor4ubv(c);
    glBegin(GL_LINE_STRIP);
    glVertex3f(v1->sx, v1->sy, gr_vertex_depth(v1));
    glVertex3f(v2->sx, v2->sy, gr_vertex_depth(v2));
    glEnd();
}

// 0x54ffa0  Confidence: exact.
// Stage 1 clamped, linear, modulate, with its texture coordinate array; stage 0 textured, modulate.
void gl_setup_multitexture_state()
{
    gl_active_texture_arb(GL_TEXTURE1_ARB);
    gl_client_active_texture_arb(GL_TEXTURE1_ARB);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    gl_active_texture_arb(GL_TEXTURE0_ARB);
    gl_client_active_texture_arb(GL_TEXTURE0_ARB);
    glEnable(GL_TEXTURE_2D);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

// 0x550080  Confidence: likely (the texture coordinate statements' operand order).
// (nv, verts, mode) with gr_texture: one GL_POLYGON from up to 30 projected vertices. Vertex flags
// (+0x1a): 1 texture coordinates (projective: u * w, v * w, 0, w), 4 vertex colour (through
// gr_color_gamma_lut), 8 vertex alpha; otherwise gr_color. Coordinates of vertices without flag 1 are
// left unset.
void gr_opengl_tmapper(int nv, gr_vertex **verts, gr::Mode mode)
{
    ubyte colors[30][4];
    vector3 pts[30];
    float tc[30][4];

    gr_opengl_set_bitmap(mode, gr_texture, 0);
    for (int i = 0; i < nv; i++) {
        gr_vertex *v = verts[i];
        if (gr_opengl_vertex_alpha)
            colors[i][3] = 255;
        else if (v->_unk1a & 8)
            colors[i][3] = v->a;
        else
            colors[i][3] = gr_color.a;
        if (gr_opengl_vertex_color) {
            colors[i][0] = 255;
            colors[i][1] = 255;
            colors[i][2] = 255;
        } else if (v->_unk1a & 4) {
            colors[i][0] = (ubyte)gr_color_gamma_lut[v->r];
            colors[i][1] = (ubyte)gr_color_gamma_lut[v->g];
            colors[i][2] = (ubyte)gr_color_gamma_lut[v->b];
        } else {
            colors[i][0] = gr_color.r;
            colors[i][1] = gr_color.g;
            colors[i][2] = gr_color.b;
        }
        if (gr_tint_enabled)
            gr_opengl_apply_tint(colors[i]);
        if (verts[i]->_unk1a & 1) {
            tc[i][3] = v->sw;
            tc[i][2] = 0.0f;
            tc[i][0] = tc[i][3] * v->u * gr_opengl_texture_u_scale;
            tc[i][1] = gr_opengl_texture_v_scale * v->v * tc[i][3];
        }
        pts[i].x = v->sx;
        pts[i].y = v->sy;
        pts[i].z = gr_vertex_depth(v);
    }
    gl_active_texture_arb(GL_TEXTURE0_ARB);
    gl_client_active_texture_arb(GL_TEXTURE0_ARB);
    glTexCoordPointer(4, GL_FLOAT, 0, tc);
    glVertexPointer(3, GL_FLOAT, 0, pts);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
    gr_opengl_set_bitmap(mode, gr_texture, 0);
    glDrawArrays(GL_POLYGON, 0, nv);
}

// 0x5502c0  Confidence: likely (byte order inside the swap).
// Reads the back buffer as R, G, B, A bytes, flips it top to bottom (alpha forced to 255 on the swapped
// rows) and reports format 7 (BM_FORMAT_8888, whose byte order elsewhere in this unit is B, G, R, A).
// SummonerFix hooks the glReadPixels call (savegame thumbnails).
int gr_opengl_read_backbuffer(int x, int y, int w, int h, ubyte *buf)
{
    glReadBuffer(GL_BACK);
    glReadPixels(x, gr_screen.max_h - y - h, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    for (int i = 0; i < h / 2; i++) {
        ubyte *top = buf + i * w * 4;
        ubyte *bottom = buf + (h - 1 - i) * w * 4;
        for (int j = 0; j < w; j++) {
            ubyte t;
            bottom[3] = 0xff;
            top[3] = 0xff;
            t = top[2];
            top[2] = bottom[2];
            bottom[2] = t;
            t = top[1];
            top[1] = bottom[1];
            bottom[1] = t;
            t = top[0];
            top[0] = bottom[0];
            bottom[0] = t;
            top += 4;
            bottom += 4;
        }
    }
    return BM_FORMAT_8888;
}

// 0x5503b0  Confidence: exact. No reference.
void gr_opengl_invalidate_mode()
{
    gr_opengl_current_mode.invalidate();
}

// 0x5503c0  Confidence: exact.
// Linear fog; with set_range (gr_set_fog passes 1) the start / end depths from gr_fog_near and
// gr_fog_far (end * 1.01) through the far clip scale and gr_vertex_depth.
void gr_opengl_set_fog(int set_range)
{
    float color[4];

    gr_opengl_current_mode.invalidate();
    if (!gr_fog_enabled) {
        glDisable(GL_FOG);
        return;
    }
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glHint(GL_FOG_HINT, GL_NICEST);
    if (set_range) {
        vector3 p(0.0f, 0.0f, gr_scale_by_far_clip(gr_fog_near));
        glFogf(GL_FOG_START, gr_vertex_depth((gr_vertex *)&p));
        p.z = gr_scale_by_far_clip(gr_fog_far) * 1.01f;
        glFogf(GL_FOG_END, gr_vertex_depth((gr_vertex *)&p));
    }
    color[0] = gr_fog_color.r * (1.0f / 255.0f);
    color[1] = gr_fog_color.g * (1.0f / 255.0f);
    color[2] = gr_fog_color.b * (1.0f / 255.0f);
    color[3] = 1.0f;
    glFogfv(GL_FOG_COLOR, color);
}

// 0x5504f0 glFinish: not source. The linker's import thunk `jmp [0x57e318]` (OPENGL32.dll!glFinish) for
// gr.cpp's gr_finish, which calls glFinish without __declspec(dllimport); this unit calls the import
// directly. Confidence: exact.

// 0x550500  Confidence: exact. No reference.
// Pushes a gluLookAt / gluPerspective projection. Never used, and wrong as written: the look-at target is
// the forward vector itself (not eye + forward) and the near plane is 0.
void gr_opengl_setup_view()
{
    float h_fov, v_fov;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    gluLookAt(gr_eye_pos.x, gr_eye_pos.y, gr_eye_pos.z,
              gr_eye_matrix.fvec.x, gr_eye_matrix.fvec.y, gr_eye_matrix.fvec.z,
              gr_eye_matrix.uvec.x, gr_eye_matrix.uvec.y, gr_eye_matrix.uvec.z);
    gr_get_view_fov(&h_fov, &v_fov);
    float aspect = gr_get_aspect();
    gluPerspective(v_fov, aspect, 0.0, gr_far_clip_dist);
}

// 0x5505d0  Confidence: exact. No reference.
// One triangle of element 0 from 3-float positions and 3-byte colours (the second argument is unused).
void gl_set_vertex_arrays(float *verts, int unused, ubyte *colors)
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, verts);
    glColorPointer(3, GL_UNSIGNED_BYTE, 0, colors);
    glBegin(GL_TRIANGLES);
    glArrayElement(0);
    glEnd();
}

// 0x550630  Confidence: exact.
void gr_opengl_set_antialias(bool on)
{
}

// 0x550640  Confidence: exact.
void clamp_ubyte(ubyte *v, ubyte lo, ubyte hi)
{
    if (*v < lo)
        *v = lo;
    else if (*v > hi)
        *v = hi;
}
