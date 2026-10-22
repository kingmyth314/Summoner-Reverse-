// unnamed_d3d_tcache_0x54bb30.cpp - Sum.exe v1.40, code 0x54bb30-0x54d5b5: the Direct3D texture cache (one
// managed DirectDraw texture per bitmap section, two texture stages; vsdk\gr\directx in sum.symbols). The
// file name is a placeholder: no path string, no strings.tbl block (source-map.md: "tags now
// vsdk\gr\directx folder only").
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp69 NOTES.md, D5023): gr_glide.cpp ends with gr_glide_flush_pipeline (0x54bad0); this
// object opens with its initialiser (0x54bb30, table 0x588a18, after gr_glide.cpp's 0x588a0c-0x588a14),
// ends with the header inlines of its stage records (0x54d470-0x54d5b5, D5020 / D5021), and
// gr_opengl.cpp follows at 0x54d5c0. Data: the message literal 0x5ad7a4 (after gr_glide.cpp's .data, which
// ends 0x5ad79b) and .bss 0x2f132d8-0x2f13317 (after gr_glide.cpp's 0x2f132d3; gr_opengl.cpp's starts at
// 0x2f13318).
//
// Like the other vsdk objects this one calls the header inlines out of line (the slot members), so it was
// built without inline expansion. The DirectX calls are the SDK's C macros (d3d_min.h, D1600).
//
// Bugs (NOTES.md): gr_d3d_copy_bitmap_to_surface copies bm_w x bm_h texels from the section's corner for
// every format except 8-bit alpha, not the section's w x h: a bitmap larger than the largest texture
// (several sections) overruns each texture row and the surface; the 8-bit alpha path ignores the section
// position and reads tex_w texels per bitmap row. The mip level Lock has no DDLOCK_WAIT, and its failure
// path leaks the top surface reference and leaves the bitmap locked.

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "vsdk/os/console.h"
#include "vsdk/os/memory.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/directx/d3d_min.h"
#include "vsdk/gr/directx/gr_direct3d.h"

// 16.16 fixed-point step of the texel walk (signed division, as the Glide back end's GLIDE_STEP_ONE).
#define D3D_STEP_ONE 0x10000

// ---- .bss, in address order ------------------------------------------------------------------------------
// 0x54bb30 init_d3d_tcache_slots_2f132d8 (jmp 0x54bb40: `vector constructor iterator` over the two slots
// with their constructor 0x54d470): compiler-generated. Confidence: likely.
d3d_tcache_slot d3d_tcache_slots[D3D_TCACHE_NUM_SLOTS];   // 0x2f132d8
d3d_tcache_entry *d3d_tcache_entries;                   // 0x2f13308
int d3d_tcache_unused_2f1330c;                          // 0x2f1330c no reference (name ours)
int d3d_tcache_unused_2f13310;                          // 0x2f13310 no reference (name ours)
int d3d_texture_mem_used;                               // 0x2f13314

// 0x54bb60  Confidence: exact. No NULL check of the allocation.
void gr_d3d_tcache_init()
{
    int i;
    d3d_tcache_entry *e;

    e = (d3d_tcache_entry *)operator new(MAX_BITMAPS * sizeof(d3d_tcache_entry));
    d3d_tcache_entries = e;
    for (i = 0; i < MAX_BITMAPS; i++) {
        e[i].bitmap = -1;
        e[i].num_sections = 0;
        e[i].sections = NULL;
    }
    d3d_tcache_slots[0].init_stage(0);
    d3d_tcache_slots[1].init_stage(1);
}

// 0x54bbb0  Confidence: exact.
void gr_d3d_tcache_close()
{
    int i;

    gr_d3d_tcache_flush();
    for (i = 0; i < MAX_BITMAPS; i++)
        gr_d3d_tcache_free_entry(&d3d_tcache_entries[i]);
    delete d3d_tcache_entries;
    d3d_tcache_entries = NULL;
}

// 0x54bbf0  Confidence: exact (the two exits clear the count and the pointer separately).
void gr_d3d_tcache_free_entry(d3d_tcache_entry *e)
{
    int i;

    if (e->num_sections != 0) {
        for (i = 0; i < e->num_sections; i++) {
            if (e->sections[i].surface != NULL) {
                IDirectDrawSurface7_Release(e->sections[i].surface);
                e->sections[i].surface = NULL;
                d3d_texture_mem_used -= e->sections[i].size;
                e->sections[i].size = 0;
            }
        }
        e->num_sections = 0;
        delete e->sections;
        e->sections = NULL;
    } else {
        e->num_sections = 0;
        e->sections = NULL;
    }
}

// 0x54bc60  Confidence: exact. Evicts every managed texture and forgets both stages.
void gr_d3d_tcache_flush()
{
    int i;

    IDirect3D7_EvictManagedTextures(D3D_DIRECT3D);
    for (i = 0; i < D3D_TCACHE_NUM_SLOTS; i++) {
        d3d_tcache_slots[i].reset();
        d3d_tcache_slots[i].d3d_unbind();
    }
}

// 0x54bc90  Confidence: exact. Empty; gr_d3d_flip calls it once per frame.
void gr_d3d_tcache_frame()
{
}

// 0x54bca0  Confidence: likely (the pow2 search and the clamps match; the order of the descriptor stores
// is the compiler's). The texture size: the section size, rounded up to a power of two when the device
// needs it, clamped to the device minimum and gr_screen's maximum, square when the device needs it. The
// caps set before the level loop are dead: the loop clears the descriptor before each Lock, and
// GetAttachedSurface gets the caps Lock returned. m-1021e w18 D23903 (141 -> 15): the texture size read
// first and the section size copied from it, the two scales from float locals (the binary converts both
// operands with fild, no fidiv), the mipmap caps stored before the size product, and the section written
// through `e->sections[section]` at the end (no pointer local). Left: the section width and height have
// each other's frame slots (the binary stores the height first, and level / size share them the other way).
int gr_d3d_create_texture_section(int bm, d3d_tcache_entry *e, int section, ubyte *data, ubyte *palette,
                                  int bm_w, int bm_h, int format, int num_levels)
{
    DDSURFACEDESC2 ddsd;
    IDirectDrawSurface7 *level, *top, *prev;
    int bpp, i, w, h, tex_w, tex_h, size;
    float u_scale, v_scale;

    bpp = bm_bits_per_pixel(format);
    tex_w = e->sections[section].w;
    tex_h = e->sections[section].h;
    w = tex_w;
    h = tex_h;
    if (d3d_pow2_textures_only) {
        for (i = 0; i < 16; i++) {
            if (tex_w > (1 << i) && tex_w <= (1 << (i + 1))) {
                tex_w = 1 << (i + 1);
                break;
            }
        }
        for (i = 0; i < 16; i++) {
            if (tex_h > (1 << i) && tex_h <= (1 << (i + 1))) {
                tex_h = 1 << (i + 1);
                break;
            }
        }
    }
    if (tex_w < d3d_min_texture_w)
        tex_w = d3d_min_texture_w;
    else if (tex_w > gr_screen.max_bitmap_w)
        tex_w = gr_screen.max_bitmap_w;
    if (tex_h < d3d_min_texture_h)
        tex_h = d3d_min_texture_h;
    else if (tex_h > gr_screen.max_bitmap_h)
        tex_h = gr_screen.max_bitmap_h;
    if (d3d_square_textures_only) {
        if (tex_w <= tex_h)
            tex_w = tex_h;
        tex_h = tex_w;
    }
    float fw = (float)w, ftw = (float)tex_w;
    u_scale = fw / ftw;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    float fh = (float)h, fth = (float)tex_h;
    v_scale = fh / fth;
    level = NULL;
    if (num_levels > 1) {
        ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
    } else {
        ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    }
    ddsd.dwMipMapCount = num_levels;
    ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
    ddsd.dwHeight = tex_h;
    ddsd.dwWidth = tex_w;
    if (format == BM_FORMAT_8_ALPHA)
        ddsd.ddpfPixelFormat = d3d_texture_format_b;
    else if (format == BM_FORMAT_8_PALETTED)
        ddsd.ddpfPixelFormat = d3d_texture_format_b;
    else if (format == BM_FORMAT_888)
        ddsd.ddpfPixelFormat = d3d_texture_format_a;
    else if (format == BM_FORMAT_565)
        ddsd.ddpfPixelFormat = d3d_texture_format_a;
    else if (format == BM_FORMAT_1555)
        ddsd.ddpfPixelFormat = d3d_texture_format_b;
    else if (format == BM_FORMAT_8888)
        ddsd.ddpfPixelFormat = d3d_texture_format_b;
    else if (format == BM_FORMAT_4444)
        ddsd.ddpfPixelFormat = d3d_texture_format_b;
    else if (format == BM_FORMAT_88)
        ddsd.ddpfPixelFormat = d3d_texture_format_c;
    if (IDirectDraw7_CreateSurface(d3d_ddraw, &ddsd, &level, NULL) != DD_OK || level == NULL) {
        bm_unlock(bm);
        return -1;
    }
    top = level;
    IDirectDrawSurface7_AddRef(level);
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;
    size = tex_h * tex_w * 2;
    while (num_levels) {
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        if (IDirectDrawSurface7_Lock(level, NULL, &ddsd, 0, NULL) != DD_OK) {
            IDirectDrawSurface7_Release(level);
            return -1;
        }
        gr_d3d_copy_bitmap_to_surface(&ddsd.ddpfPixelFormat, e, section, &ddsd, format, tex_h, tex_w, bm_h, bm_w,
                                      data, palette);
        IDirectDrawSurface7_Unlock(level, NULL);
        prev = level;
        if (--num_levels) {
            IDirectDrawSurface7_GetAttachedSurface(level, &ddsd.ddsCaps, &level);
            data += (bpp / 8) * bm_w * bm_h;
            bm_w /= 2;
            bm_h /= 2;
            tex_w = bm_w;
            tex_h = bm_h;
        }
        IDirectDrawSurface7_Release(prev);
    }
    e->sections[section].surface = top;
    d3d_texture_mem_used += size;
    e->sections[section].size = size;
    e->sections[section].u_scale = u_scale;
    e->sections[section].v_scale = v_scale;
    return 1;
}

// 0x54c0a0  Confidence: likely (each format's pixel expression; the order of the or'ed terms is the one of
// the binary, the zero fills are loops the compiler turned into rep stos). Converts the bitmap into the
// locked surface: shift and 255 / max of each mask of the format (alpha 0 / 256 without an alpha mask),
// then per bitmap format.
void gr_d3d_copy_bitmap_to_surface(DDPIXELFORMAT *pf, d3d_tcache_entry *e, int section, DDSURFACEDESC2 *ddsd,
                                   int format, int tex_h, int tex_w, int bm_h, int bm_w, ubyte *data,
                                   ubyte *palette)
{
    ushort pal16[256];
    DWORD m;
    int r_shift = 0, r_scale = 0, g_shift = 0, g_scale = 0, b_shift = 0, b_scale = 0, a_shift, a_scale;
    int i, x, y, u, u0, v, a;
    ushort *d;
    ubyte *row, *src, *p;
    ushort pix;
    d3d_tcache_section *s;

    if (pf->dwRBitMask) {
        for (m = pf->dwRBitMask; !(m & 1); m >>= 1)
            r_shift++;
        r_scale = 255 / (pf->dwRBitMask >> r_shift);
    }
    if (pf->dwGBitMask) {
        for (m = pf->dwGBitMask; !(m & 1); m >>= 1)
            g_shift++;
        g_scale = 255 / (pf->dwGBitMask >> g_shift);
    }
    if (pf->dwBBitMask) {
        for (m = pf->dwBBitMask; !(m & 1); m >>= 1)
            b_shift++;
        b_scale = 255 / (pf->dwBBitMask >> b_shift);
    }
    if ((pf->dwFlags & DDPF_ALPHAPIXELS) && pf->dwRGBAlphaBitMask) {
        a_shift = 0;
        for (m = pf->dwRGBAlphaBitMask; !(m & 1); m >>= 1)
            a_shift++;
        a_scale = 255 / (pf->dwRGBAlphaBitMask >> a_shift);
    } else {
        a_shift = 0;
        a_scale = 256;
    }

    if (format == BM_FORMAT_8_ALPHA) {
        // White in the target format with the (gamma-corrected) alpha; the section position is not used.
        row = (ubyte *)ddsd->lpSurface;
        src = data;
        for (y = 0; y < tex_h; y++) {
            d = (ushort *)row;
            for (x = 0; x < tex_w; x++)
                *d++ = (ushort)(((gr_gamma_ramp[src[x]] / a_scale) << a_shift) | ((255 / r_scale) << r_shift)
                                | ((255 / g_scale) << g_shift) | ((255 / b_scale) << b_shift));
            row += ddsd->lPitch;
            src += bm_w;
        }
    } else if (format == BM_FORMAT_8_PALETTED) {
        p = palette;
        for (i = 0; i < 256; i++) {
            pal16[i] = (ushort)(((p[2] / b_scale) << b_shift) | ((p[0] / r_scale) << r_shift)
                                | ((p[1] / g_scale) << g_shift) | ((255 / a_scale) << a_shift));
            p += 3;
        }
        s = &e->sections[section];
        v = s->y << 16;
        u0 = s->x << 16;
        row = (ubyte *)ddsd->lpSurface;
        for (y = 0; y < bm_h; y++) {
            d = (ushort *)row;
            u = u0;
            for (x = 0; x < bm_w; x++) {
                *d++ = pal16[data[(v / D3D_STEP_ONE) * bm_w + u / D3D_STEP_ONE]];
                u += D3D_STEP_ONE;
            }
            for (; x < tex_w; x++)
                *d++ = 0;
            v += D3D_STEP_ONE;
            row += ddsd->lPitch;
        }
        for (y = bm_h; y < tex_h; y++) {
            d = (ushort *)((ubyte *)ddsd->lpSurface + y * ddsd->lPitch);
            for (x = 0; x < tex_w; x++)
                *d++ = 0;
        }
    } else if (format == BM_FORMAT_888) {
        s = &e->sections[section];
        u0 = s->x << 16;
        v = s->y << 16;
        row = (ubyte *)ddsd->lpSurface;
        for (y = 0; y < bm_h; y++) {
            d = (ushort *)row;
            u = u0;
            for (x = 0; x < bm_w; x++) {
                p = &data[((v / D3D_STEP_ONE) * bm_w + u / D3D_STEP_ONE) * 3];
                *d++ = (ushort)(((p[1] / g_scale) << g_shift) | ((p[2] / r_scale) << r_shift)
                                | ((p[0] / b_scale) << b_shift) | ((255 / a_scale) << a_shift));
                u += D3D_STEP_ONE;
            }
            for (; x < tex_w; x++)
                *d++ = 0;
            v += D3D_STEP_ONE;
            row += ddsd->lPitch;
        }
        for (y = bm_h; y < tex_h; y++) {
            d = (ushort *)((ubyte *)ddsd->lpSurface + y * ddsd->lPitch);
            for (x = 0; x < tex_w; x++)
                *d++ = 0;
        }
    } else if (format == BM_FORMAT_4444) {
        s = &e->sections[section];
        u0 = s->x << 16;
        v = s->y << 16;
        row = (ubyte *)ddsd->lpSurface;
        for (y = 0; y < bm_h; y++) {
            d = (ushort *)row;
            u = u0;
            for (x = 0; x < bm_w; x++) {
                pix = ((ushort *)data)[(v / D3D_STEP_ONE) * bm_w + u / D3D_STEP_ONE];
                *d++ = (ushort)(((((pix >> 4) & 0xf) * 255 / 15 / g_scale) << g_shift)
                                | ((((pix >> 8) & 0xf) * 255 / 15 / r_scale) << r_shift)
                                | (((pix >> 12) * 255 / 15 / a_scale) << a_shift)
                                | (((pix & 0xf) * 255 / 15 / b_scale) << b_shift));
                u += D3D_STEP_ONE;
            }
            for (; x < tex_w; x++)
                *d++ = 0;
            v += D3D_STEP_ONE;
            row += ddsd->lPitch;
        }
        for (y = bm_h; y < tex_h; y++) {
            d = (ushort *)((ubyte *)ddsd->lpSurface + y * ddsd->lPitch);
            for (x = 0; x < tex_w; x++)
                *d++ = 0;
        }
    } else if (format == BM_FORMAT_1555) {
        // The bitmaps' top bit means transparent (as the Glide and OpenGL back ends read it).
        s = &e->sections[section];
        u0 = s->x << 16;
        v = s->y << 16;
        row = (ubyte *)ddsd->lpSurface;
        for (y = 0; y < bm_h; y++) {
            d = (ushort *)row;
            u = u0;
            for (x = 0; x < bm_w; x++) {
                pix = ((ushort *)data)[(v / D3D_STEP_ONE) * bm_w + u / D3D_STEP_ONE];
                a = (pix & 0x8000) ? 0 : 255;
                *d++ = (ushort)(((((pix >> 5) & 0x1f) * 255 / 31 / g_scale) << g_shift)
                                | ((((pix >> 10) & 0x1f) * 255 / 31 / r_scale) << r_shift)
                                | (((pix & 0x1f) * 255 / 31 / b_scale) << b_shift)
                                | ((a / a_scale) << a_shift));
                u += D3D_STEP_ONE;
            }
            for (; x < tex_w; x++)
                *d++ = 0;
            v += D3D_STEP_ONE;
            row += ddsd->lPitch;
        }
        for (y = bm_h; y < tex_h; y++) {
            d = (ushort *)((ubyte *)ddsd->lpSurface + y * ddsd->lPitch);
            for (x = 0; x < tex_w; x++)
                *d++ = 0;
        }
    } else if (format == BM_FORMAT_565) {
        s = &e->sections[section];
        u0 = s->x << 16;
        v = s->y << 16;
        row = (ubyte *)ddsd->lpSurface;
        for (y = 0; y < bm_h; y++) {
            d = (ushort *)row;
            u = u0;
            for (x = 0; x < bm_w; x++) {
                pix = ((ushort *)data)[(v / D3D_STEP_ONE) * bm_w + u / D3D_STEP_ONE];
                *d++ = (ushort)(((((pix >> 5) & 0x3f) * 255 / 63 / g_scale) << g_shift)
                                | (((pix >> 11) * 255 / 31 / r_scale) << r_shift)
                                | (((pix & 0x1f) * 255 / 31 / b_scale) << b_shift)
                                | ((255 / a_scale) << a_shift));
                u += D3D_STEP_ONE;
            }
            for (; x < tex_w; x++)
                *d++ = 0;
            v += D3D_STEP_ONE;
            row += ddsd->lPitch;
        }
        for (y = bm_h; y < tex_h; y++) {
            d = (ushort *)((ubyte *)ddsd->lpSurface + y * ddsd->lPitch);
            for (x = 0; x < tex_w; x++)
                *d++ = 0;
        }
    } else if (format == BM_FORMAT_8888) {
        s = &e->sections[section];
        u0 = s->x << 16;
        v = s->y << 16;
        row = (ubyte *)ddsd->lpSurface;
        for (y = 0; y < bm_h; y++) {
            d = (ushort *)row;
            u = u0;
            for (x = 0; x < bm_w; x++) {
                p = &data[((v / D3D_STEP_ONE) * bm_w + u / D3D_STEP_ONE) * 4];
                *d++ = (ushort)(((p[3] / a_scale) << a_shift) | ((p[1] / g_scale) << g_shift)
                                | ((p[2] / r_scale) << r_shift) | ((p[0] / b_scale) << b_shift));
                u += D3D_STEP_ONE;
            }
            for (; x < tex_w; x++)
                *d++ = 0;
            v += D3D_STEP_ONE;
            row += ddsd->lPitch;
        }
        for (y = bm_h; y < tex_h; y++) {
            d = (ushort *)((ubyte *)ddsd->lpSurface + y * ddsd->lPitch);
            for (x = 0; x < tex_w; x++)
                *d++ = 0;
        }
    } else if (format == BM_FORMAT_88) {
        // The du/dv bump bitmaps: copied as they are.
        s = &e->sections[section];
        u0 = s->x << 16;
        v = s->y << 16;
        row = (ubyte *)ddsd->lpSurface;
        for (y = 0; y < bm_h; y++) {
            d = (ushort *)row;
            u = u0;
            for (x = 0; x < bm_w; x++) {
                *d++ = ((ushort *)data)[(v / D3D_STEP_ONE) * bm_w + u / D3D_STEP_ONE];
                u += D3D_STEP_ONE;
            }
            for (; x < tex_w; x++)
                *d++ = 0;
            v += D3D_STEP_ONE;
            row += ddsd->lPitch;
        }
        for (y = bm_h; y < tex_h; y++) {
            d = (ushort *)((ubyte *)ddsd->lpSurface + y * ddsd->lPitch);
            for (x = 0; x < tex_w; x++)
                *d++ = 0;
        }
    }
}

// 0x54cfe0  Confidence: exact.
int gr_d3d_get_section_count(int bm)
{
    return d3d_tcache_entries[bm_get_current_frame(bm)].num_sections;
}

// 0x54d000  Confidence: exact (the section read in place, no local pointer).
void gr_d3d_get_section_rect(int bm, int section, int *x, int *y, int *w, int *h)
{
    d3d_tcache_entry *e = &d3d_tcache_entries[bm_get_current_frame(bm)];

    *x = e->sections[section].x;
    *y = e->sections[section].y;
    *w = e->sections[section].w;
    *h = e->sections[section].h;
}

// 0x54d050  Confidence: exact. FreeSpace's gr_d3d_tcache_set in shape: the stage keeps the section bound
// last; a missing or dirty texture is (re)created first.
int gr_d3d_tcache_set(int stage, int bm, int section, float *u_scale, float *v_scale, bool bind)
{
    d3d_tcache_entry *e;
    int frame;

    if (bm < 0) {
        d3d_tcache_slots[stage].d3d_unbind();
        return 1;
    }
    frame = bm_get_current_frame(bm);
    if (!d3d_tcache_slots[stage].is_bound(frame, section, u_scale, v_scale)) {
        e = &d3d_tcache_entries[frame];
        if (e->bitmap < 0 || e->bitmap != bm) {
            if (!gr_d3d_create_texture(bm, e))
                return 0;
            e->dirty = false;
        }
        if (e->dirty) {
            if (!gr_d3d_create_texture(bm, e))
                return 0;
        }
        if (bind)
            d3d_tcache_slots[stage].d3d_bind(e, section, u_scale, v_scale);
    }
    return 1;
}

// 0x54d120  Confidence: exact (D19501: x = 0 before the row height, both clamps as if / else as in the Glide
// twin, and the x store through e->sections[section] before s is taken). Twin of gr_glide_create_texture: the sections are tiles of at most
// gr_screen.max_bitmap_w x max_bitmap_h; a mipmapped bitmap must be one texture of a texture size, else it
// is not drawn (the message is formatted, never printed). Allocates outside the level mempool.
int gr_d3d_create_texture(int bm, d3d_tcache_entry *e)
{
    int w, h, num_pixels, num_levels;
    ubyte *data;
    ubyte *palette;
    int i, n, nx, ny, x, y, row, col, sh, section, format;
    d3d_tcache_section *s;
    Mempool *old_pool;

    old_pool = get_current_mempool();
    set_current_mempool(NULL);
    gr_d3d_tcache_free_entry(e);
    bm_get_mipmap_info(bm, &w, &h, &num_pixels, &num_levels);
    e->bitmap = bm;
    nx = (gr_screen.max_bitmap_w + w - 1) / gr_screen.max_bitmap_w;
    ny = (gr_screen.max_bitmap_h + h - 1) / gr_screen.max_bitmap_h;
    e->num_sections = ny * nx;
    s = new d3d_tcache_section[e->num_sections];
    n = e->num_sections;
    e->sections = s;
    for (i = 0; i < n; i++)
        s[i].surface = NULL;
    if (num_levels > 1 && (!gr_d3d_is_texture_size(w, h) || e->num_sections > 1)) {
        sprintf(string_buffer, "D3D: Not drawing %dx%d bitmap '%s'\n", w, h, bm_get_filename(bm));
        set_current_mempool(old_pool);
        return 0;
    }
    format = bm_lock(bm, &data, &palette);
    section = 0;
    y = 0;
    for (row = 0; row < ny; row++) {
        x = 0;
        if (h - y > gr_screen.max_bitmap_h)
            sh = gr_screen.max_bitmap_h;
        else
            sh = h - y;
        for (col = 0; col < nx; col++) {
            e->sections[section].x = x;
            s = &e->sections[section];
            s->y = y;
            if (w - x > gr_screen.max_bitmap_w)
                s->w = gr_screen.max_bitmap_w;
            else
                s->w = w - x;
            x += s->w;
            s->h = sh;
            gr_d3d_create_texture_section(bm, e, section, data, palette, w, h, format, num_levels);
            section++;
        }
        y += sh;
    }
    bm_unlock(bm);
    set_current_mempool(old_pool);
    return 1;
}

// 0x54d330  Confidence: exact. True when both dimensions are already a texture size: each is rounded up
// to 4, 8, ..., 128, and anything above 128 to 256.
bool gr_d3d_is_texture_size(int w, int h)
{
    int tw, th;

    if (w <= 4)
        tw = 4;
    else if (w <= 8)
        tw = 8;
    else if (w <= 16)
        tw = 16;
    else if (w <= 32)
        tw = 32;
    else if (w <= 64)
        tw = 64;
    else if (w <= 128)
        tw = 128;
    else
        tw = 256;
    if (h <= 4)
        th = 4;
    else if (h <= 8)
        th = 8;
    else if (h <= 16)
        th = 16;
    else if (h <= 32)
        th = 32;
    else if (h <= 64)
        th = 64;
    else if (h <= 128)
        th = 128;
    else
        th = 256;
    if (tw == w && th == h)
        return true;
    return false;
}

// 0x54d3f0  Confidence: exact.
void gr_d3d_mark_texture_dirty(int bm)
{
    int frame, i;

    frame = bm_get_current_frame(bm);
    d3d_tcache_entries[frame].dirty = true;
    for (i = 0; i < D3D_TCACHE_NUM_SLOTS; i++)
        d3d_tcache_slots[i].invalidate_bitmap(frame);
}

// 0x54d430  Confidence: exact. Takes the handle as the entry index (no frame lookup).
void gr_d3d_tcache_free(int bm)
{
    d3d_tcache_entry *e = &d3d_tcache_entries[bm];
    int i;

    for (i = 0; i < D3D_TCACHE_NUM_SLOTS; i++)
        d3d_tcache_slots[i].invalidate_bitmap(bm);
    gr_d3d_tcache_free_entry(e);
    e->bitmap = -1;
}

// Header inlines at the end of the object, in the order of their first use (not source here):
// 0x54d470 d3d_tcache_slot__ctor: d3d_tcache_slot::d3d_tcache_slot (gr_direct3d.h). Confidence: exact.
// 0x54d490 tcache_slot__init_stage: tcache_slot::init_stage (gr.h). Confidence: exact.
// 0x54d4b0 tcache_slot__reset: tcache_slot::reset (gr.h). Confidence: exact.
// 0x54d4c0 tcache_slot__d3d_unbind: d3d_tcache_slot::d3d_unbind (gr_direct3d.h). Confidence: exact.
// 0x54d4f0 tcache_slot__is_bound: tcache_slot::is_bound (gr.h). Confidence: exact.
// 0x54d530 tcache_slot__d3d_bind: d3d_tcache_slot::d3d_bind (gr_direct3d.h). Confidence: likely (store order).
// 0x54d5a0 tcache_slot__invalidate_bitmap: tcache_slot::invalidate_bitmap (gr.h). Confidence: exact.
