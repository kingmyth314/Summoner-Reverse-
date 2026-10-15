// vsdk/bmpman/bmpman.h - the bitmap manager (vsdk\bmpman\bmpman.cpp, path references 0x501d92-0x503c68).
// Not original source. Layout: symbols/types.h bitmap_entry.
#ifndef VSDK_BMPMAN_BMPMAN_H
#define VSDK_BMPMAN_BMPMAN_H

#include "vsdk/vsdk.h"

// Bitmap manager entry, bm_bitmaps[0xaf0] at 0x2591c78; handle = index.
struct bitmap_entry {
    const char *name;        // +0x00 in bm_name_pool
    int  name_checksum;      // +0x04 bm_hash_name(name); -1 for user bitmaps
    ushort width;            // +0x08
    ushort height;           // +0x0A
    int  num_pixels;         // +0x0C all mip levels
    int  type;               // +0x10 0 none, 1 pcx, 2 tga, 3 user, 4 vbm, 5 m2v
    int  anim_type;          // +0x14 0 single, 1 animation header, 2 frame
    int  format;             // +0x18
    ubyte num_levels;        // +0x1C
    ubyte num_frames;        // +0x1D
    ubyte _pad1e[2];
    void *data;              // +0x20 locked pixels
    float frames_per_ms;     // +0x24
    void *palette;           // +0x28
    bitmap_entry *next;      // +0x2C used/free list
    bitmap_entry *prev;      // +0x30
    ubyte lock_count;        // +0x34
    ubyte _pad35[3];
    int  total_bytes;        // +0x38
    int  path_id;            // +0x3C
    ubyte reduced;           // +0x40
    ubyte _pad41[3];
    int  mark_count;         // +0x44
};
LAYOUT_CHECK(bitmap_entry, sizeof(bitmap_entry) == 0x48);

int  bm_find(const char *name);                       // 0x5025d0, -1 if none
int  bm_load(const char *name, int path_id, int unk); // 0x5026e0 (the third argument is -1 at every pilot call)
void bm_get_dimensions(int bm, int *w, int *h);       // 0x503500
int  bm_get_num_frames(int bm, float *secs_per_frame);  // 0x502500 (0 here)
// bm_lock returns the bitmap type (0 = failure), the pixel data through *data and the format through *format.
int  bm_lock(int bm, ubyte **data, int *format);      // 0x503650
void bm_unlock(int bm);                               // 0x503b50 bm_free_data(bm, 0)
// (format, w, h, data, palette, a6); gr_font_create passes (4, w, h, bits, 0, -1).
int  bm_create(int format, int w, int h, void *data, void *palette, int a6);   // 0x503cf0
void bm_set_preload_on_load(int on);                  // 0x503da0
int  progress_update(int check_keys);                 // 0x503e80 (called with 0; return type guessed)
void bm_release(int bm);                              // 0x503c80 (return unused)

// The format out parameter ui_container::set_mask_bitmap passes to bm_lock: the first dword after
// bm_bitmaps (0x2591c78 + n * 0x48), so a bmpman.cpp global. No name in sum.symbols.
extern int bm_lock_format_unused;                     // 0x25c2ff8

#endif
