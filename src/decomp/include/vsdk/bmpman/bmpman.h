// vsdk/bmpman/bmpman.h - the bitmap manager (vsdk\bmpman\bmpman.cpp, 0x501b20-0x503db5, path references
// 0x501d92-0x503c68; decomp31). Not original source. Layout: symbols/types.h bitmap_entry.
#ifndef VSDK_BMPMAN_BMPMAN_H
#define VSDK_BMPMAN_BMPMAN_H

#include "vsdk/vsdk.h"

#define MAX_BITMAPS     0xaf0           // bm_bitmaps entries (2800); handle = index
#define BM_HASH_SIZE    0xaf1           // bm_hash_table slots (open addressing)
#define BM_NAME_POOL_SIZE 0x10000       // bm_name_pool bytes

// bitmap_entry::type (bm_get_type_from_ext; 3 = bm_create).
#define BM_TYPE_NONE    0
#define BM_TYPE_PCX     1
#define BM_TYPE_TGA     2
#define BM_TYPE_USER    3
#define BM_TYPE_VBM     4
#define BM_TYPE_M2V     5

// bitmap_entry::anim_type. An animation is a header entry followed by num_frames frame entries; a frame's
// num_frames is its frame number (1..n), so entry - num_frames is the header.
#define BM_ANIM_NONE    0
#define BM_ANIM_HEADER  1
#define BM_ANIM_FRAME   2

// bitmap_entry::format (pixel formats; names after Dash Faction's bm::Format, numbers checked against
// bm_bytes_per_pixel 0x502450 and bm_has_alpha 0x5035e0).
#define BM_FORMAT_NONE  0
#define BM_FORMAT_8_PALETTED 1
#define BM_FORMAT_8_ALPHA 2
#define BM_FORMAT_565   3
#define BM_FORMAT_4444  4
#define BM_FORMAT_1555  5
#define BM_FORMAT_888   6
#define BM_FORMAT_8888  7
#define BM_FORMAT_88    8

// Bitmap manager entry, bm_bitmaps[0xaf0] at 0x2591c78; handle = index.
struct bitmap_entry {
    const char *name;        // +0x00 in bm_name_pool ("USERBMAP" for bm_create, NULL for frames)
    int  name_checksum;      // +0x04 bm_hash_name(name); -1 for user bitmaps and frames
    ushort width;            // +0x08
    ushort height;           // +0x0A
    int  num_pixels;         // +0x0C all mip levels
    int  type;               // +0x10 BM_TYPE_*: 0 none, 1 pcx, 2 tga, 3 user, 4 vbm, 5 m2v
    int  anim_type;          // +0x14 BM_ANIM_*: 0 single, 1 animation header, 2 frame
    int  format;             // +0x18 BM_FORMAT_*
    ubyte num_levels;        // +0x1C
    ubyte num_frames;        // +0x1D header: frame count; frame: frame number (1..n)
    ubyte _pad1e[2];
    void *data;              // +0x20 locked pixels
    float frames_per_ms;     // +0x24 fps * 0.001
    void *palette;           // +0x28
    bitmap_entry *next;      // +0x2C used/free list
    bitmap_entry *prev;      // +0x30
    ubyte lock_count;        // +0x34 animation headers: frames still locked (bm_lock_vbm sets num_frames)
    ubyte _pad35[3];
    int  total_bytes;        // +0x38
    int  path_id;            // +0x3C
    ubyte reduced;           // +0x40 half size, one mip level dropped (bm_set_reduced)
    ubyte _pad41[3];
    int  mark_count;         // +0x44 bm_set_mark nesting: > 0 survives bm_release_from_mark
};
LAYOUT_CHECK(bitmap_entry, sizeof(bitmap_entry) == 0x48);

// ---- data (bmpman.cpp) ------------------------------------------------------------------------------
extern bool Load_mipmaps;                             // 0x5a5760 (.data, true) console "load_mipmaps"
extern bitmap_entry *bm_hash_table[BM_HASH_SIZE];     // 0x257f0b0
extern char bm_name_pool[BM_NAME_POOL_SIZE];          // 0x2581c74 the name pool's buffer
extern bitmap_entry bm_bitmaps[MAX_BITMAPS];          // 0x2591c78
extern bitmap_entry bm_used_list;                     // 0x25c3000 list sentinel
extern bitmap_entry *bm_hash_table_mark[BM_HASH_SIZE];   // 0x25c3054 copy taken by bm_set_mark
extern bitmap_entry bm_free_list;                     // 0x25c5c18 list sentinel
extern int  bm_mark_name_pool_length;                 // 0x25c5c60
extern ubyte bm_missing_bitmap_pixels[32 * 32 * 3];   // 0x25c5c8c 888 checker (bm_init)
extern bool bm_missing_created;                       // 0x25c688c
extern bool bm_reduce_textures;                       // 0x25c688d
extern bool bm_inited;                                // 0x25c688e
extern int  bm_mark_active;                           // 0x25c6890
extern int  bm_preload_on_load;                       // 0x25c6894

// The palette out parameter ui_container::set_mask_bitmap passes to bm_lock: the first dword after
// bm_bitmaps (0x2591c78 + n * 0x48), so a bmpman.cpp global. No name in sum.symbols. bm_lock stores the
// entry's palette pointer through it (D1100), hence the pointer type; the name is kept for ui_container.cpp.
extern ubyte *bm_lock_format_unused;                  // 0x25c2ff8

// ---- console commands ---------------------------------------------------------------------------------
void cmd_load_mipmaps();                              // 0x501b50
void cmd_change_texture_res();                        // 0x501c90

// ---- functions, in address order -------------------------------------------------------------------
int  bm_calc_num_pixels(int w, int h, int num_levels);   // 0x501ce0 sum of w * h over halving levels
// (name, &w, &h, &format, &num_frames, &num_levels, &fps, path_id); errors are fatal.
void bm_read_vbm_header(const char *name, int *w, int *h, int *format, int *num_frames, int *num_levels,
                        int *fps, int path_id);       // 0x501d20
void bm_set_mark();                                   // 0x501eb0
void bm_release_from_mark();                          // 0x501f20
void bm_hash_insert(bitmap_entry *e);                 // 0x502040
void bm_close();                                      // 0x502090
void bm_free_data(int bm, bool force);                // 0x5020f0
int  bm_handle_to_index(int bm);                      // 0x5021d0 identity
void bm_set_reduce_textures(bool reduce);             // 0x5021e0
void bm_set_reduced(int bm, bool reduce);             // 0x502240
bool bm_is_reduced(int bm);                           // 0x502300 (no caller)
void bm_init();                                       // 0x502320
void bm_free_entry(int index);                        // 0x5023e0
int  bm_bytes_per_pixel(int format);                  // 0x502450
int  bm_bits_per_pixel(int format);                   // 0x5024a0
int  bm_get_total_bytes(int bm);                      // 0x5024c0 (no caller)
int  bm_get_type(int bm);                             // 0x5024e0 (no caller)
// Returns the frame count (1 for a single bitmap); *anim_secs (may be NULL) gets the length of the whole
// animation in seconds (num_frames / fps), 1.0 for a single bitmap.
int  bm_get_num_frames(int bm, float *anim_secs);     // 0x502500
int  bm_get_current_frame(int bm);                    // 0x502560 index of the frame for the current time
int  bm_find(const char *name);                       // 0x5025d0, -1 if none
int  bm_hash_name(const char *name);                  // 0x502650
// True for the missing-file placeholders of bm_create_missing (type 3 entries whose name is not
// "USERBMAP"); for handle -1, whether a placeholder was ever made. No caller.
bool bm_is_missing_placeholder(int bm);                       // 0x502690
int  bm_load(const char *name, int path_id, int unk); // 0x5026e0 (the third argument is -1 at every pilot call; unused)
int  bm_load_internal(const char *name, int path_id, int force_new);   // 0x502720
// Returns the type (0 = failure); (name, &w, &h, &format, &num_levels, &num_frames, &fps, &size, path_id).
int  bm_read_header(const char *name, int *w, int *h, int *format, int *num_levels, int *num_frames,
                    int *fps, int *size, int path_id);   // 0x502a00
int  bm_get_type_from_ext(const char *name);          // 0x502e30
bitmap_entry *bm_alloc_entry();                       // 0x502eb0
bitmap_entry *bm_alloc_entries(int count);            // 0x502ee0 NULL when no run of count free entries
void bm_prefer_vbm(char *name, int *handle, int path_id);   // 0x502f70
int  bm_create_missing(const char *name);             // 0x5030a0
int  bm_load_animation_path(const char *name, int *num_frames, int *fps, int path_id);   // 0x503110 (no caller)
int  bm_reload(int bm);                               // 0x503190
int  bm_replace_file(int bm, const char *name, int path_id);   // 0x5031c0 -1 failure (decomp30, character_replace_texture)
bool bm_is_valid_size(int w, int h);                  // 0x5033c0
int  bm_load_animation(const char *name, int *num_frames, int *fps);   // 0x503480 first frame's handle (no caller)
void bm_get_dimensions(int bm, int *w, int *h);       // 0x503500
void bm_get_mipmap_info(int bm, int *w, int *h, int *num_pixels, int *num_levels);   // 0x503550
int  bm_get_format(int bm);                           // 0x5035c0
bool bm_has_alpha(int bm);                            // 0x5035e0 format 4444, 1555 or 8888 (decomp28)
// (bm, &data, &palette) without locking; returns the format (unused by s3d.cpp; decomp28).
int  bm_get_data(int bm, void **data, void **palette);   // 0x503610
// bm_lock returns the format (0 = failure), the pixel data through *data and the palette through *palette
// (D1100).
int  bm_lock(int bm, ubyte **data, ubyte **palette);  // 0x503650
bool bm_lock_tga(int index);                          // 0x503710
void bm_lock_vbm(int index);                          // 0x5038f0
bool bm_lock_pcx(int index);                          // 0x503aa0
bool bm_lock_m2v(int index);                          // 0x503b20
void bm_unlock(int bm);                               // 0x503b50 bm_free_data(bm, 0)
const char *bm_get_filename(int bm);                  // 0x503b60 the header's name for a frame (no handle test)
int  bm_get_total_size(int bm, int bytes_per_pixel);  // 0x503ba0 -1 for a bad handle (D1102; no caller)
void bm_read_pcx_header_of(int bm, ubyte *palette);   // 0x503c00 (no caller)
void bm_release(int bm);                              // 0x503c80 (return unused)
// (format, w, h, data, palette, a6); gr_font_create passes (4, w, h, bits, 0, -1). a6 is unused.
int  bm_create(int format, int w, int h, void *data, void *palette, int a6);   // 0x503cf0
void *bm_malloc(int size);                            // 0x503d90 (no caller)
void bm_set_preload_on_load(int on);                  // 0x503da0

int  progress_update(int check_keys);                 // 0x503e80 (called with 0; return type guessed)

#endif
