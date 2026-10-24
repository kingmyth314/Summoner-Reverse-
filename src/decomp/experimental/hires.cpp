// [Experimental] HiResInterface (docs/future/texture-upscale.md): upscaled interface images and fonts on the OpenGL
// renderer. Not part of Sum.exe: compiled only for link-build --config experimental.
//
// The game finds a loose file on disk before the packfiles (File::find), so a texture pack is loose TGAs. 3D drawing
// uses texture coordinates from 0 to 1 and needs nothing. The interface and the fonts draw a rectangle of the
// bitmap given in pixels (gr_bitmap / gr_bitmap_ex), and gr_opengl_bitmap turns those pixels into texture
// coordinates by dividing by bm_get_dimensions. With an upscaled file that division uses the new size, and the
// rectangle covers only part of the area it should (seen on the main menu, 2026-10-23).
//
// The fix: when a bitmap is loaded from a loose file and the packfile has a copy of the same name, the copy's
// header gives the size the game was made for (the nominal size). With the switch on and the OpenGL renderer active,
// bm_get_dimensions returns the nominal size, so every caller (the interface layout, the fonts, the bitmap
// drawing) works as with the original file. The texture upload reads bm_get_mipmap_info, which keeps the real
// size, so the GPU gets every pixel. The two places that read pixels themselves (the interface click masks,
// ui_container::get_mask_pixel, and the projected-light sample, light_sample_projected_bitmap) take the real size
// through exp_bm_real_point. Direct3D and Glide split textures by the real size, so there the switch does nothing.
#include <windows.h>
#include <string.h>
#include "experimental/experimental.h"

#ifdef EXPERIMENTAL_BUILD

#include "vsdk/vsdk.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/gr/gr.h"
#include "vsdk/vfile/packfile/file_packfile.h"
#include "vsdk/vfile/file.h"
#include "vsdk/gr/gr_font.h"

// Per bm_bitmaps index: the nominal size, 0 when the entry uses its real size.
static unsigned short hires_nominal_w[MAX_BITMAPS];
static unsigned short hires_nominal_h[MAX_BITMAPS];
static int hires_enabled = -1;     // -1 = not read yet

static bool hires_on()
{
    if (hires_enabled < 0) {
        hires_enabled = exp_ini_int("Experimental", "HiResInterface", 0) != 0;
        if (hires_enabled)
            exp_log("hires: HiResInterface on");
    }
    return hires_enabled != 0;
}

// The width and height in the packfile copy's header; false when there is no copy or the format is not known.
static bool hires_packfile_size(const char *filename, int *w, int *h)
{
    PackfileEntry *entry = packfile_find(filename);
    unsigned char hdr[20];
    const char *dot = strrchr(filename, '.');
    int got;

    if (entry == NULL || dot == NULL)
        return false;
    if (!packfile_fopen(entry))     // fails when the entry is open already
        return false;
    got = packfile_fread(hdr, 1, sizeof(hdr), entry, 0);
    packfile_fclose(entry);
    if (got != (int)sizeof(hdr))
        return false;
    if (_stricmp(dot, ".tga") == 0) {
        *w = hdr[12] | (hdr[13] << 8);
        *h = hdr[14] | (hdr[15] << 8);
        return true;
    }
    if (_stricmp(dot, ".vbm") == 0) {
        *w = hdr[8] | (hdr[9] << 8) | (hdr[10] << 16) | (hdr[11] << 24);
        *h = hdr[12] | (hdr[13] << 8) | (hdr[14] << 16) | (hdr[15] << 24);
        return true;
    }
    return false;
}

void exp_bm_entries_allocated(int index, int count)
{
    int i;

    for (i = index; i < index + count && i < MAX_BITMAPS; i++)
        hires_nominal_w[i] = hires_nominal_h[i] = 0;
}

void exp_bm_loaded(int index, int count, const char *filename, int w, int h, bool reduced)
{
    int nw, nh, i;

    exp_bm_entries_allocated(index, count);
    if (!hires_on() || reduced)
        return;
    if (!hires_packfile_size(filename, &nw, &nh))
        return;
    // Only a true upscale: larger in both directions and by the same factor (the layout scales evenly).
    if (nw <= 0 || nh <= 0 || w <= nw || h <= nh || w * nh != h * nw)
        return;
    for (i = index; i < index + count && i < MAX_BITMAPS; i++) {
        hires_nominal_w[i] = (unsigned short)nw;
        hires_nominal_h[i] = (unsigned short)nh;
    }
    exp_log("hires: %s %dx%d drawn as %dx%d", filename, w, h, nw, nh);
}

bool exp_bm_nominal_size(int index, int *w, int *h)
{
    if (index < 0 || index >= MAX_BITMAPS || hires_nominal_w[index] == 0)
        return false;
    if (gr_screen.mode != GR_OPENGL)
        return false;
    *w = hires_nominal_w[index];
    *h = hires_nominal_h[index];
    return true;
}

// Fonts: gr_font_create packs a font's characters into a 64, 128 or 256-pixel square bitmap of its own
// (bm_create, so there is no packfile copy to compare with). tools/texture-upscale.mjs writes <font>-hires.tga with
// the same layout at 2x or 4x. Loaded in its place, with the atlas's size as the nominal size, gr_string's pixel
// rectangles (char_x_coords / char_y_coords, widths, height) draw from it unchanged. The game's own atlas bitmap is
// kept (gr_close_fonts frees its pixels with the font); the font just points at the other one. OpenGL only.
void exp_font_hires(struct Font *f)
{
    char name[64];
    File file;
    int bm, index, w, h, num_pixels, num_levels;

    if (!hires_on() || gr_screen.mode != GR_OPENGL)
        return;
    strncpy(name, f->filename, sizeof name - 16);
    name[sizeof name - 16] = 0;
    char *dot = strrchr(name, '.');
    if (dot)
        *dot = 0;
    strcat(name, "-hires.tga");
    if (!file.find(name, PATH_ANY))
        return;
    bm = bm_load(name, -1, -1);
    index = bm_get_current_frame(bm);
    if (bm < 0 || index < 0)
        return;
    bm_get_mipmap_info(bm, &w, &h, &num_pixels, &num_levels);
    if (!((w == f->bm_width * 2 && h == f->bm_height * 2) || (w == f->bm_width * 4 && h == f->bm_height * 4))) {
        exp_log("hires: %s is %dx%d, not 2x or 4x the font's %dx%d atlas; not used", name, w, h, f->bm_width, f->bm_height);
        return;
    }
    hires_nominal_w[index] = (unsigned short)f->bm_width;
    hires_nominal_h[index] = (unsigned short)f->bm_height;
    f->bm_handle = bm;
    exp_log("hires: font %s draws from %s (%dx%d as %dx%d)", f->filename, name, w, h, f->bm_width, f->bm_height);
}

int exp_bm_real_point(int bm, int *x, int *y, int nominal_w)
{
    int index = bm_get_current_frame(bm);
    int nw, nh;

    if (index < 0 || !exp_bm_nominal_size(index, &nw, &nh))
        return nominal_w;
    *x = *x * bm_bitmaps[index].width / nw;
    *y = *y * bm_bitmaps[index].height / nh;
    return bm_bitmaps[index].width;
}

#endif // EXPERIMENTAL_BUILD
