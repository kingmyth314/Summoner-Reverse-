// bmpman.cpp - Sum.exe v1.40, code 0x501b20-0x503db5 (vsdk\bmpman\bmpman.cpp), the bitmap manager.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Data of this unit: .data 0x5a5760-0x5a5c20 (Load_mipmaps, then the string literals in function order),
// .bss 0x257f0b0-0x25c6897 (the gaps at 0x2591c74, 0x25c2ffc, 0x25c5c64 and 0x25c5c7c have no reference:
// 8-byte alignment of the following object). Three static initialisers (table 0x5887d4-0x5887dc): the two
// console commands and the name pool.
//
// Handles are entry indices (bm_handle_to_index is the identity). An animation takes a header entry
// (anim_type 1, num_frames = frame count) followed by its frames (anim_type 2, num_frames = frame number),
// so "e - e->num_frames" is a frame's header. Most getters go through bm_get_current_frame, which turns an
// animation header into the frame for the current time.
//
// Out-of-line header inline emitted at the end of this object (module "-" in sum.symbols, no source here):
// imin at 0x503dc0 (bm_read_header and File::read call it).

#include <math.h>
#include "vsdk/os/debug.h"
#include "vsdk/os/console.h"
#include "vsdk/os/stringpool.h"
#include "vsdk/os/frametime.h"
#include "vsdk/math/vector.h"
#include "vsdk/vfile/file.h"
#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/bmpman/targautils.h"

// Original __FILE__ of this unit (0x5a58f0 and its copies; string pooling is off).
#define BMPMAN_CPP "D:\\projects\\Summoner\\pccode\\vsdk\\bmpman\\bmpman.cpp"

// ---- .data -----------------------------------------------------------------------------------------------
bool Load_mipmaps = true;                              // 0x5a5760

// ---- .bss, in address order ------------------------------------------------------------------------------
// Whether VC6 keeps .bss in definition order is the open question of frametime.cpp; the definitions are
// in address order. The command objects and the name pool are defined below with their initialisers.
bitmap_entry *bm_hash_table[BM_HASH_SIZE];             // 0x257f0b0
char bm_name_pool[BM_NAME_POOL_SIZE];                  // 0x2581c74
bitmap_entry bm_bitmaps[MAX_BITMAPS];                  // 0x2591c78
ubyte *bm_lock_format_unused;                          // 0x25c2ff8
bitmap_entry bm_used_list;                             // 0x25c3000
//  ccmd_load_mipmaps                                     0x25c3048
bitmap_entry *bm_hash_table_mark[BM_HASH_SIZE];        // 0x25c3054
bitmap_entry bm_free_list;                             // 0x25c5c18
int bm_mark_name_pool_length;                          // 0x25c5c60
//  bm_name_string_pool                                   0x25c5c68
//  ccmd_change_texture_res                               0x25c5c80
ubyte bm_missing_bitmap_pixels[32 * 32 * 3];           // 0x25c5c8c
bool bm_missing_created;                               // 0x25c688c
bool bm_reduce_textures;                               // 0x25c688d
bool bm_inited;                                        // 0x25c688e
int bm_mark_active;                                    // 0x25c6890
int bm_preload_on_load;                                // 0x25c6894

// ---- console command "load_mipmaps" ----------------------------------------------------------------------
// 0x501b20 init_ccmd_load_mipmaps_thunk (jmp, initialiser table 0x5887d4): compiler-generated.
// 0x501b30 init_ccmd_load_mipmaps: the dynamic initialiser of this definition. Confidence: exact.
console_command ccmd_load_mipmaps("load_mipmaps", "Toggles Load_mipmaps", cmd_load_mipmaps);   // 0x25c3048

// 0x501b50  Confidence: exact. The console's bool command macro (as cmd_Super_power in combat.cpp); the
// usage text names the command first and the variable second, the status line the command.
void cmd_load_mipmaps()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Load_mipmaps = true;
        else if (arg_type & ARG_FALSE)
            Load_mipmaps = false;
        else if (arg_type & ARG_NONE)
            Load_mipmaps ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it.\n",
                "load_mipmaps", "Load_mipmaps");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "load_mipmaps", Load_mipmaps ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// ---- the name pool ---------------------------------------------------------------------------------------
// 0x501c10 init_bm_name_pool (call 0x501c20, jmp 0x501c40; initialiser table 0x5887d8): compiler-generated.
// 0x501c20 bm_init_name_pool: StringPool(bm_name_pool, 0x10000, false). Confidence: exact.
// 0x501c40 bm_name_pool_register_dtor: atexit(0x501c50). Confidence: exact.
// 0x501c50 bm_name_pool_dtor: mov ecx, this; jmp StringPool::~StringPool. Confidence: exact.
StringPool bm_name_string_pool(bm_name_pool, BM_NAME_POOL_SIZE, false);   // 0x25c5c68

// ---- console command "change_texture_res" ----------------------------------------------------------------
// 0x501c60 init_ccmd_change_texture_res_thunk (jmp, initialiser table 0x5887dc): compiler-generated.
// 0x501c70 init_ccmd_change_texture_res: the dynamic initialiser of this definition. Confidence: exact.
console_command ccmd_change_texture_res("change_texture_res",
                                        "Toggle the texture resolution between high and low resolutions.",
                                        cmd_change_texture_res);   // 0x25c5c80

// 0x501c90  Confidence: exact. No calling_a_command_function test: help and status toggle it too.
void cmd_change_texture_res()
{
    char buf[256];

    bm_set_reduce_textures(!bm_reduce_textures);
    sprintf(buf, "Low resolution textures are now %s!\n", bm_reduce_textures ? "enabled" : "disabled");
    print_to_console(buf, 0);
}

// ---- functions -------------------------------------------------------------------------------------------

// 0x501ce0  Confidence: exact (VC6 counts the loop down).
int bm_calc_num_pixels(int w, int h, int num_levels)
{
    int total = 0;
    int i;

    for (i = 0; i < num_levels; i++) {
        total += w * h;
        w /= 2;
        h /= 2;
    }
    return total;
}

// 0x501d20  Confidence: likely (the format mapping may be a switch; the version int is read and dropped).
// "VBM " signature, version, width, height, format (0 1555, 1 4444, other 565), fps, frame count and
// the number of extra mip levels. Without Load_mipmaps only the first level is used.
void bm_read_vbm_header(const char *name, int *w, int *h, int *format, int *num_frames, int *num_levels,
                        int *fps, int path_id)
{
    char signature[4];
    File f;
    char error[256];
    int vbm_format;

    if (f.open(name, 1, path_id) < 0) {
        sprintf(error, "Unable to open %s.  Maybe file doesn't exist.", name);
        VSDK_ERROR(BMPMAN_CPP, 0x3f9, error);
    }
    f.read(signature, 4, 0, 0);
    f.read_int(0, 0);
    *w = f.read_int(0, 0);
    *h = f.read_int(0, 0);
    vbm_format = f.read_int(0, 0);
    if (vbm_format == 0)
        *format = BM_FORMAT_1555;
    else if (vbm_format == 1)
        *format = BM_FORMAT_4444;
    else
        *format = BM_FORMAT_565;
    *fps = f.read_int(0, 0);
    *num_frames = f.read_int(0, 0);
    *num_levels = f.read_int(0, 0) + 1;
    if (!Load_mipmaps)
        *num_levels = 1;
    f.close();
}

// 0x501eb0  Confidence: likely (the call to 0x503db0, a thiscall stub that jumps to
// StringPool::GetLength, is written as a direct GetLength call; see 0x503db0 below).
void bm_set_mark()
{
    bitmap_entry *e;

    if (bm_mark_active)
        VSDK_ERROR(BMPMAN_CPP, 0x51d, "bm_set_mark within bm_set_mark not allowed!");
    bm_mark_active = 1;
    bm_mark_name_pool_length = bm_name_string_pool.GetLength();
    for (e = bm_used_list.next; e != &bm_used_list; e = e->next)
        e->mark_count++;
    memcpy(bm_hash_table_mark, bm_hash_table, sizeof(bm_hash_table));
}

// 0x501f20  Confidence: likely (the first loop reloads bm_used_list.next after each release, which the
// second loop then starts from). Frees the bitmaps loaded since bm_set_mark, restores the hash table and
// the name pool length, and re-adds the names that lay past the restored length (bm_replace_file renames).
void bm_release_from_mark()
{
    char name[256];
    bitmap_entry *e, *next;

    if (!bm_mark_active)
        VSDK_ERROR(BMPMAN_CPP, 0x52f, "bm_release_from_mark without bm_set_mark not allowed!");
    bm_mark_active = 0;
    bm_name_string_pool.SetLength(bm_mark_name_pool_length);
    for (e = bm_used_list.next; e != &bm_used_list; e = next) {
        next = e->next;
        if (e->mark_count > 0)
            e->mark_count--;
        else
            bm_release(e - bm_bitmaps);
    }
    memcpy(bm_hash_table, bm_hash_table_mark, sizeof(bm_hash_table));
    for (e = bm_used_list.next; e != &bm_used_list; e = e->next) {
        if (e->name - bm_name_pool >= bm_mark_name_pool_length) {
            strcpy(name, e->name);
            e->name = bm_name_string_pool.Add(name);
            bm_hash_insert(e);
        }
    }
}

// 0x502040  Confidence: exact. Linear probing from name_checksum.
void bm_hash_insert(bitmap_entry *e)
{
    int i = e->name_checksum;

    while (bm_hash_table[i % BM_HASH_SIZE] != NULL)
        i++;
    bm_hash_table[i % BM_HASH_SIZE] = e;
}

// 0x502090  Confidence: exact (the loop is rotated; StringPool::Reset is a tail jump).
void bm_close()
{
    while (bm_used_list.next != &bm_used_list) {
        int bm = bm_used_list.next - bm_bitmaps;
        bm_free_data(bm, true);
        bm_release(bm);
    }
    bm_inited = false;
    bm_name_string_pool.Reset();
}

// 0x5020f0  Confidence: likely (block order: VC6 places the single-bitmap branch first). Single bitmaps
// are freed at once (user bitmaps never); an animation's shared block is freed through its header when
// the lock count runs out (or with force), and the frames' pointers are cleared.
void bm_free_data(int bm, bool force)
{
    bitmap_entry *e = &bm_bitmaps[bm_handle_to_index(bm)];
    int i;

    if (e->anim_type == BM_ANIM_HEADER || e->anim_type == BM_ANIM_FRAME) {
        if (e->anim_type == BM_ANIM_FRAME)
            e -= e->num_frames;
        if (!force) {
            if (--e->lock_count != 0)
                return;
        }
        if (e->data != NULL) {
            delete[] (ubyte *)e->data;
            e->data = NULL;
        }
        if (e->palette != NULL) {
            delete[] (ubyte *)e->palette;
            e->palette = NULL;
        }
        for (i = 0; i < e->num_frames; i++) {
            e[i + 1].data = NULL;
            e[i + 1].palette = NULL;
        }
    } else {
        if (e->type == BM_TYPE_USER)
            return;
        if (e->data != NULL) {
            delete[] (ubyte *)e->data;
            e->data = NULL;
        }
        if (e->palette != NULL) {
            delete[] (ubyte *)e->palette;
            e->palette = NULL;
        }
    }
}

// 0x5021d0  Confidence: exact.
int bm_handle_to_index(int bm)
{
    return bm;
}

// 0x5021e0  Confidence: exact.
void bm_set_reduce_textures(bool reduce)
{
    bitmap_entry *e;

    if (bm_reduce_textures == reduce)
        return;
    bm_reduce_textures = reduce;
    for (e = bm_used_list.next; e != &bm_used_list; e = e->next)
        bm_set_reduced(e - bm_bitmaps, reduce);
}

// 0x502240  Confidence: exact. Only unlocked single bitmaps with mip levels; total_bytes is not updated.
void bm_set_reduced(int bm, bool reduce)
{
    bitmap_entry *e = &bm_bitmaps[bm_handle_to_index(bm)];

    if (reduce) {
        if (e->anim_type != BM_ANIM_NONE || e->reduced || e->num_levels <= 1 || e->num_frames != 1
            || e->lock_count != 0)
            return;
        e->reduced = 1;
        e->width >>= 1;
        e->height >>= 1;
        e->num_levels--;
    } else {
        if (!e->reduced || e->lock_count != 0)
            return;
        e->reduced = 0;
        e->width <<= 1;
        e->height <<= 1;
        e->num_levels++;
    }
    e->num_pixels = bm_calc_num_pixels(e->width, e->height, e->num_levels);
    gr_mark_texture_dirty(e - bm_bitmaps);
}

// 0x502300  Confidence: exact (a dead lea of the entry address stays in the code).
bool bm_is_reduced(int bm)
{
    return bm_bitmaps[bm_get_current_frame(bm)].reduced;
}

// 0x502320  Confidence: likely (the list macros: VC6 keeps bm_used_list.prev in a register through the
// append loop and drops list_init's dead store). Every entry is appended to the used list and then moved
// to the free list by bm_free_entry. The missing-bitmap pixels: 888 pink-grey with a grey line every
// eight pixels.
void bm_init()
{
    ubyte *p;
    int i, x, y;

    if (bm_inited == true)
        return;
    list_init(&bm_free_list);
    list_init(&bm_used_list);
    for (i = 0; i < MAX_BITMAPS; i++)
        list_append(&bm_used_list, &bm_bitmaps[i]);
    memset(bm_hash_table, 0, sizeof(bm_hash_table));
    for (i = 0; i < MAX_BITMAPS; i++)
        bm_free_entry(i);

    p = bm_missing_bitmap_pixels;
    for (y = 0; y < 32; y++) {
        for (x = 0; x < 32; x++) {
            if ((x & 7) && (y & 7)) {
                p[0] = 0x8e;
                p[1] = 0x79;
                p[2] = 0x79;
            } else {
                p[0] = 0x40;
                p[1] = 0x40;
                p[2] = 0x40;
            }
            p += 3;
        }
    }
    bm_inited = true;
}

// 0x5023e0  Confidence: likely (list_remove's NULL stores are dead before list_append; field order).
// anim_type, num_frames, frames_per_ms, path_id and reduced are left as they were.
void bm_free_entry(int index)
{
    bitmap_entry *e = &bm_bitmaps[index];

    list_remove(e);
    list_append(&bm_free_list, e);
    e->name = NULL;
    e->name_checksum = -1;
    e->type = BM_TYPE_NONE;
    e->width = 0;
    e->height = 0;
    e->num_levels = 0;
    e->num_pixels = 0;
    e->format = BM_FORMAT_NONE;
    e->data = NULL;
    e->palette = NULL;
    e->lock_count = 0;
    e->total_bytes = 0;
    e->mark_count = 0;
}

// 0x502450  Confidence: likely (case order; jump table 0x50247c for formats 1-8).
int bm_bytes_per_pixel(int format)
{
    switch (format) {
    case BM_FORMAT_565:
    case BM_FORMAT_4444:
    case BM_FORMAT_1555:
    case BM_FORMAT_88:
        return 2;
    case BM_FORMAT_888:
        return 3;
    case BM_FORMAT_8888:
        return 4;
    default:
        return 1;
    }
}

// 0x5024a0  Confidence: exact.
int bm_bits_per_pixel(int format)
{
    return bm_bytes_per_pixel(format) * 8;
}

// 0x5024c0  Confidence: likely (the failure return is `or eax, eax` with eax already -1, not the usual
// `or eax, -1`: the source may return the index itself).
int bm_get_total_bytes(int bm)
{
    int i = bm_get_current_frame(bm);

    if (i == -1)
        return -1;
    return bm_bitmaps[i].total_bytes;
}

// 0x5024e0  Confidence: exact.
int bm_get_type(int bm)
{
    int i = bm_get_current_frame(bm);

    if (i == -1)
        return BM_TYPE_NONE;
    return bm_bitmaps[i].type;
}

// 0x502500  Confidence: exact. *anim_secs = num_frames / frames_per_ms * 0.001, the length of the whole
// animation (not the time per frame).
int bm_get_num_frames(int bm, float *anim_secs)
{
    bitmap_entry *e = &bm_bitmaps[bm_handle_to_index(bm)];

    if (e->anim_type == BM_ANIM_HEADER) {
        if (anim_secs != NULL)
            *anim_secs = e->num_frames / e->frames_per_ms * 0.001f;
        return e->num_frames;
    }
    if (anim_secs != NULL)
        *anim_secs = 1.0f;
    return 1;
}

// 0x502560  Confidence: exact (milliseconds_unpaused is converted as unsigned: fild qword with a zero high
// dword, D1103).
int bm_get_current_frame(int bm)
{
    int i = bm_handle_to_index(bm);
    bitmap_entry *e = &bm_bitmaps[i];

    if (e->anim_type == BM_ANIM_HEADER)
        return (int)((unsigned int)milliseconds_unpaused * e->frames_per_ms) % e->num_frames + i + 1;
    return i;
}

// 0x5025d0  Confidence: exact.
int bm_find(const char *name)
{
    int hash = bm_hash_name(name);
    int i = hash;
    bitmap_entry *e;

    while ((e = bm_hash_table[i % BM_HASH_SIZE]) != NULL) {
        if (e->name_checksum == hash && _stricmp(e->name, name) == 0)
            return e - bm_bitmaps;
        i++;
    }
    return -1;
}

// 0x502650  Confidence: exact (rol 6 from the shift pair, abs inline). The lower-case character is
// sign-extended from a char.
int bm_hash_name(const char *name)
{
    unsigned int hash;
    char c;

    if (name == NULL)
        return -1;
    hash = 0;
    while (*name) {
        c = tolower(*name);
        hash = ((hash << 6) | (hash >> 26)) ^ c;
        name++;
    }
    return abs((int)hash);
}

// 0x502690  Confidence: exact. Despite the name, true only for bm_create_missing's placeholders: user
// bitmaps whose name is not "USERBMAP".
bool bm_is_missing_placeholder(int bm)
{
    bitmap_entry *e;

    if (bm == -1)
        return bm_missing_created;
    e = &bm_bitmaps[bm_get_current_frame(bm)];
    if (e->type == BM_TYPE_USER && _stricmp(e->name, "USERBMAP") != 0)
        return true;
    return false;
}

// 0x5026e0  Confidence: exact. The third argument is not read.
int bm_load(const char *name, int path_id, int unk)
{
    int bm = bm_load_internal(name, path_id, 0);

    if (bm_preload_on_load && bm >= 0)
        gr_page_in(bm, 0);
    return bm;
}

// 0x502720  Confidence: likely (store order in the entry set-up). The header is read from any path
// (PATH_ANY), the entry keeps path_id for bm_lock. With force_new the name is not hashed, so the copy is
// never found by name (bm_reload). A missing file gives bm_create_missing's placeholder.
int bm_load_internal(const char *name, int path_id, int force_new)
{
    int num_levels, w, h, num_frames, format;
    int bm;
    int size, fps;
    int index;
    char filename[256];
    int type, num_pixels, i;
    bool reduced;
    bitmap_entry *e, *frame;

    if (!force_new) {
        bm = bm_find(name);
        if (bm >= 0)
            return bm;
    } else {
        bm = -1;
    }
    if (path_id == -1)
        path_id = PATH_ANY;
    strcpy(filename, name);
    if (!force_new) {
        bm_prefer_vbm(filename, &bm, path_id);
        if (bm >= 0)
            return bm;
    }
    type = bm_read_header(filename, &w, &h, &format, &num_levels, &num_frames, &fps, &size, PATH_ANY);
    if (type == BM_TYPE_NONE)
        return bm_create_missing(name);

    reduced = false;
    if (bm_reduce_textures && num_levels > 1 && num_frames == 1) {
        reduced = true;
        num_levels--;
        w >>= 1;
        h >>= 1;
    }
    num_pixels = bm_calc_num_pixels(w, h, num_levels);
    if (num_frames == 1) {
        e = bm_alloc_entry();
        e->anim_type = BM_ANIM_NONE;
    } else {
        e = bm_alloc_entries(num_frames + 1);
        e->anim_type = BM_ANIM_HEADER;
    }
    e->type = type;
    index = e - bm_bitmaps;
    e->format = format;
    e->name = bm_name_string_pool.Add(filename);
    if (!force_new) {
        e->name_checksum = bm_hash_name(filename);
        bm_hash_insert(e);
    }
    e->num_frames = num_frames;
    e->frames_per_ms = fps * 0.001f;
    e->width = w;
    e->height = h;
    e->data = NULL;
    e->palette = NULL;
    e->num_levels = num_levels;
    e->num_pixels = num_pixels;
    e->path_id = path_id;
    e->reduced = reduced;
    if (e->type == BM_TYPE_M2V)
        e->total_bytes = size;
    else
        e->total_bytes = bm_bytes_per_pixel(format) * num_pixels;

    if (num_frames > 1) {
        for (i = 0; i < num_frames; i++) {
            frame = e + 1 + i;
            frame->name = NULL;
            frame->name_checksum = -1;
            frame->type = type;
            frame->anim_type = BM_ANIM_FRAME;
            frame->format = format;
            frame->width = w;
            frame->height = h;
            frame->num_levels = num_levels;
            frame->num_pixels = num_pixels;
            frame->num_frames = i + 1;
            frame->data = NULL;
            frame->palette = NULL;
            frame->path_id = path_id;
            frame->reduced = 0;
            if (frame->type == BM_TYPE_M2V)
                frame->total_bytes = size;
            else
                frame->total_bytes = bm_bytes_per_pixel(format) * num_pixels;
        }
    }
    return index;
}

// 0x502a00  Confidence: likely (jump tables 0x502de0 on type - 1 and 0x502df4 / byte table 0x502e08 on
// bpp - 8; the "Cannot determine" message uses the base-name buffer's stack slot, written here as that
// buffer). Fills the header fields, *size only for m2v. TGA mip levels are counted from the files
// "<base>-mip<n>.tga" while each has the halved size and the same depth.
int bm_read_header(const char *name, int *w, int *h, int *format, int *num_levels, int *num_frames,
                   int *fps, int *size, int path_id)
{
    int bpp, mip_bpp, mip_w, mip_h;
    int type, err, mw, mh;
    char error[256];
    char base_name[256];
    char mip_name[256];
    char *p;

    type = bm_get_type_from_ext(name);
    *num_levels = 1;
    *fps = 0;
    *num_frames = 1;
    *size = -1;
    switch (type) {
    case BM_TYPE_PCX:
        err = pcx_read_header(name, w, h, &bpp, NULL, path_id);
        if (err != 0) {
            sprintf(error, "Couldn't open %s because of error: %s\n", name, bm_image_error_string(err));
            sprintf(error, "Could not open file: %s", name);
            VSDK_ERROR(BMPMAN_CPP, 0x2a8, error);
        }
        *format = BM_FORMAT_8_PALETTED;
        return type;

    case BM_TYPE_TGA:
        err = tga_read_header(name, w, h, &bpp, NULL, NULL, path_id);
        if (err != 0) {
            sprintf(error, "Couldn't open %s because of error: %s\n", name, bm_image_error_string(err));
            return 0;
        }
        switch (bpp) {
        case 8:
            *format = BM_FORMAT_8_PALETTED;
            break;
        case 16:
            *format = BM_FORMAT_1555;
            break;
        case 24:
            *format = BM_FORMAT_888;
            break;
        case 32:
            *format = BM_FORMAT_8888;
            break;
        default:
            *format = BM_FORMAT_NONE;
            break;
        }
        if (Load_mipmaps) {
            strcpy(base_name, name);
            p = strchr(base_name, '.');
            if (p != NULL)
                *p = 0;
            mw = *w / 2;
            mh = *h / 2;
            while (imin(mw, mh) >= 1) {
                sprintf(mip_name, "%s-mip%d.tga", base_name, *num_levels);
                if (strlen(mip_name) > 40) {
                    sprintf(error, "Mipmap filename exceeds maximum length: '%s'\n", mip_name);
                    return type;
                }
                if (tga_read_header(mip_name, &mip_w, &mip_h, &mip_bpp, NULL, (void *)1, path_id) != 0)
                    break;
                if (mip_w != mw || mip_h != mh || mip_bpp != bpp)
                    break;
                (*num_levels)++;
                mw /= 2;
                mh /= 2;
            }
        }
        return type;

    case BM_TYPE_VBM:
        bm_read_vbm_header(name, w, h, format, num_frames, num_levels, fps, path_id);
        return type;

    case BM_TYPE_M2V:
        *size = m2v_read_header(name, w, h, 0);
        if (*size <= 0)
            return 0;
        *num_frames = 1;
        *num_levels = 1;
        *fps = 0;
        *format = BM_FORMAT_1555;
        return type;

    default:
        sprintf(base_name, "Cannot determine bitmap type in bm_read_header for file %s", name);
        VSDK_ERROR(BMPMAN_CPP, 0x340, base_name);
    }
}

// 0x502e30  Confidence: exact. 0 without an extension or for an unknown one.
int bm_get_type_from_ext(const char *name)
{
    const char *ext = strrchr(name, '.');

    if (ext == NULL)
        return BM_TYPE_NONE;
    ext++;
    if (_stricmp(ext, "pcx") == 0)
        return BM_TYPE_PCX;
    if (_stricmp(ext, "tga") == 0)
        return BM_TYPE_TGA;
    if (_stricmp(ext, "vbm") == 0)
        return BM_TYPE_VBM;
    if (_stricmp(ext, "m2v") == 0)
        return BM_TYPE_M2V;
    return BM_TYPE_NONE;
}

// 0x502eb0  Confidence: exact. Free list head to the front of the used list.
bitmap_entry *bm_alloc_entry()
{
    bitmap_entry *e = bm_free_list.next;

    list_remove(e);                     // its NULL stores are dead before list_insert
    list_insert(&bm_used_list, e);
    return e;
}

// 0x502ee0  Confidence: likely (loop shape of the search). The first run of count entries with type 0,
// moved to the end of the used list; NULL when there is none (bm_load_internal does not test it).
bitmap_entry *bm_alloc_entries(int count)
{
    int run = 0, start = 0;
    int i, j;

    for (i = 0; i < MAX_BITMAPS; i++) {
        if (bm_bitmaps[i].type == BM_TYPE_NONE) {
            if (run == 0)
                start = i;
            run++;
        } else {
            run = 0;
        }
        if (run == count) {
            for (j = start; j < start + count; j++) {
                list_remove(&bm_bitmaps[j]);
                list_append(&bm_used_list, &bm_bitmaps[j]);
            }
            return &bm_bitmaps[start];
        }
    }
    return NULL;
}

// 0x502f70  Confidence: likely (the memcpy / strcat intrinsics). For a .tga name, switches to name.vbm
// when that bitmap is loaded (*handle = its handle) or the file exists (*handle = -1).
void bm_prefer_vbm(char *name, int *handle, int path_id)
{
    *handle = -1;
    if (bm_get_type_from_ext(name) != BM_TYPE_TGA)
        return;

    char *ext = strrchr(name, '.');
    File f;
    char vbm_name[256];
    int bm;

    memcpy(vbm_name, name, ext - name);
    vbm_name[ext - name] = 0;
    strcat(vbm_name, ".vbm");
    bm = bm_find(vbm_name);
    if (bm >= 0 || f.find(vbm_name, path_id)) {
        *handle = bm;
        strcpy(name, vbm_name);
    }
}

// 0x5030a0  Confidence: exact. The flag is set but never stops the creation: every missing name gets its
// own 32x32 888 copy of the checker, registered under that name.
int bm_create_missing(const char *name)
{
    int bm;
    bitmap_entry *e;

    if (!bm_missing_created)
        bm_missing_created = true;
    bm = bm_create(BM_FORMAT_888, 32, 32, bm_missing_bitmap_pixels, NULL, -1);
    e = &bm_bitmaps[bm_get_current_frame(bm)];
    e->name = bm_name_string_pool.Add(name);
    e->name_checksum = bm_hash_name(name);
    bm_hash_insert(e);
    return bm;
}

// 0x503110  Confidence: exact. Returns the header's handle (bm_load_animation returns the first frame).
int bm_load_animation_path(const char *name, int *num_frames, int *fps, int path_id)
{
    int bm = bm_load_internal(name, path_id, 0);
    int i = bm_handle_to_index(bm);

    if (bm_bitmaps[i].anim_type == BM_ANIM_HEADER) {
        *num_frames = bm_bitmaps[i].num_frames;
        *fps = (int)ceil(bm_bitmaps[i].frames_per_ms * 1000.0f);
        return bm;
    }
    *num_frames = 1;
    *fps = 0;
    return bm;
}

// 0x503190  Confidence: exact. A second, unhashed entry for the same file (charinfo_create_character_instance).
int bm_reload(int bm)
{
    int i = bm_get_current_frame(bm);

    return bm_load_internal(bm_bitmaps[i].name, bm_bitmaps[i].path_id, 1);
}

// 0x5031c0  Confidence: likely (comparison order). Points an unlocked single bitmap at another file of the
// same size, levels and pixel count (m2v and unsupported files refused); 0 on success, -1 otherwise.
int bm_replace_file(int bm, const char *name, int path_id)
{
    char filename[256];
    int w, h, format, num_levels, num_frames, fps, size, handle;
    int type, num_pixels;
    bool reduced;
    bitmap_entry *e = &bm_bitmaps[bm_get_current_frame(bm)];

    if (e->lock_count != 0)
        return -1;
    if (e->num_frames > 1)
        return -1;
    if (path_id < 0)
        path_id = PATH_ANY;
    strcpy(filename, name);
    bm_prefer_vbm(filename, &handle, path_id);
    type = bm_read_header(filename, &w, &h, &format, &num_levels, &num_frames, &fps, &size, PATH_ANY);
    if (type == BM_TYPE_NONE || type == BM_TYPE_M2V)
        return -1;
    if (!bm_is_valid_size(w, h))
        return -1;
    reduced = false;
    if (e->reduced && num_levels > 1 && num_frames == 1) {
        num_levels--;
        reduced = true;
        h >>= 1;
        w >>= 1;
    }
    if (e->num_levels < num_levels)
        num_levels = e->num_levels;
    num_pixels = bm_calc_num_pixels(w, h, num_levels);
    if (e->num_frames != num_frames || e->width != w || e->height != h || e->num_levels != num_levels
        || e->num_pixels != num_pixels)
        return -1;
    e->type = type;
    e->name = bm_name_string_pool.Add(filename);
    e->name_checksum = bm_hash_name(filename);
    bm_hash_insert(e);
    e->frames_per_ms = fps * 0.001f;
    e->path_id = path_id;
    e->format = format;
    e->reduced = reduced;
    gr_mark_texture_dirty(bm);
    return 0;
}

// Texture sizes bm_is_valid_size accepts: n rounded up through 4, 8, .. 128, 256 (name ours; inline,
// so expanded twice in bm_is_valid_size).
static inline int bm_round_texture_size(int n)
{
    if (n <= 4)
        return 4;
    if (n <= 8)
        return 8;
    if (n <= 16)
        return 16;
    if (n <= 32)
        return 32;
    if (n <= 64)
        return 64;
    if (n <= 128)
        return 128;
    return 256;
}

// 0x5033c0  Confidence: likely (the rounding may be written out twice or as a macro). Both dimensions
// powers of two from 4 to 256.
bool bm_is_valid_size(int w, int h)
{
    int rw = bm_round_texture_size(w);
    int rh = bm_round_texture_size(h);

    return rw == w && rh == h;
}

// 0x503480  Confidence: exact. Returns the first frame's handle for an animation.
int bm_load_animation(const char *name, int *num_frames, int *fps)
{
    int bm = bm_load_internal(name, -1, 0);
    int i = bm_handle_to_index(bm);

    if (bm_bitmaps[i].anim_type != BM_ANIM_HEADER) {
        *num_frames = 1;
        *fps = 0;
        return bm;
    }
    *num_frames = bm_bitmaps[i].num_frames;
    *fps = (int)ceil(bm_bitmaps[i].frames_per_ms * 1000.0f);
    return bm + 1;
}

// 0x503500  Confidence: exact.
void bm_get_dimensions(int bm, int *w, int *h)
{
    int i = bm_get_current_frame(bm);

    if (i == -1) {
        *w = *h = 0;
        return;
    }
    *w = bm_bitmaps[i].width;
    *h = bm_bitmaps[i].height;
}

// 0x503550  Confidence: exact. Only w and h are cleared for an empty entry.
void bm_get_mipmap_info(int bm, int *w, int *h, int *num_pixels, int *num_levels)
{
    int i = bm_get_current_frame(bm);

    if (bm_bitmaps[i].type == BM_TYPE_NONE) {
        *w = *h = 0;
        return;
    }
    *w = bm_bitmaps[i].width;
    *h = bm_bitmaps[i].height;
    *num_pixels = bm_bitmaps[i].num_pixels;
    *num_levels = bm_bitmaps[i].num_levels;
}

// 0x5035c0  Confidence: exact.
int bm_get_format(int bm)
{
    return bm_bitmaps[bm_get_current_frame(bm)].format;
}

// 0x5035e0  Confidence: likely (the || result is built in EAX, the early false in AL).
bool bm_has_alpha(int bm)
{
    int format;

    if (bm < 0)
        return false;
    format = bm_get_format(bm);
    return format == BM_FORMAT_4444 || format == BM_FORMAT_8888 || format == BM_FORMAT_1555;
}

// 0x503610  Confidence: exact.
int bm_get_data(int bm, void **data, void **palette)
{
    bitmap_entry *e = &bm_bitmaps[bm_get_current_frame(bm)];

    *data = e->data;
    *palette = e->palette;
    return e->format;
}

// 0x503650  Confidence: likely (type 3 is tested before the switch: its jump table slot 0x5036f0 + 8 is
// the error). Loads the pixels every time (no test for data already present, except an animation's shared
// block in bm_lock_vbm); returns the format, 0 when a loader fails.
int bm_lock(int bm, ubyte **data, ubyte **palette)
{
    int i = bm_get_current_frame(bm);
    bitmap_entry *e = &bm_bitmaps[i];

    if (e->type != BM_TYPE_USER) {
        switch (e->type) {
        case BM_TYPE_PCX:
            if (!bm_lock_pcx(i))
                return 0;
            break;
        case BM_TYPE_TGA:
            if (!bm_lock_tga(i))
                return 0;
            break;
        case BM_TYPE_VBM:
            bm_lock_vbm(i);
            break;
        case BM_TYPE_M2V:
            if (!bm_lock_m2v(i))
                return 0;
            break;
        default:
            VSDK_ERROR(BMPMAN_CPP, 0x8e8, "Unsupported type in bm_lock");
        }
    }
    *data = (ubyte *)e->data;
    *palette = (ubyte *)e->palette;
    return e->format;
}

// 0x503710  Confidence: likely. One file without mip levels; otherwise every level from its own file
// (level 0 the name itself, level n "<base>-mip<n>.tga", one further when reduced), packed one after the
// other. The mip reads are not checked; the fourth tga_read_bitmap argument is a bool (pushed as a dword
// from a byte slot) that is false for the first level.
bool bm_lock_tga(int index)
{
    bitmap_entry *e = &bm_bitmaps[index];
    int i, w, h, bpp;
    bool loaded;
    ubyte *dst, *pal;
    char base_name[0x400];
    char filename[0x400];
    char *p;

    e->data = new ubyte[bm_bytes_per_pixel(e->format) * e->num_pixels];
    if (e->format == BM_FORMAT_8_PALETTED)
        e->palette = new ubyte[0x300];
    dst = (ubyte *)e->data;
    pal = (ubyte *)e->palette;
    if (e->num_levels <= 1 && !e->reduced) {
        if (tga_read_bitmap(e->name, dst, pal, 0, e->path_id) != 0)
            return false;
        return true;
    }
    loaded = false;
    bpp = bm_bytes_per_pixel(e->format);
    h = e->height;
    w = e->width;
    strcpy(base_name, e->name);
    p = strchr(base_name, '.');
    if (p != NULL)
        *p = 0;
    for (i = 0; i < e->num_levels; i++) {
        if (e->reduced)
            sprintf(filename, "%s-mip%d.tga", base_name, i + 1);
        else if (i == 0)
            strcpy(filename, e->name);
        else
            sprintf(filename, "%s-mip%d.tga", base_name, i);
        tga_read_bitmap(filename, dst, pal, loaded, e->path_id);
        dst += h * w * bpp;
        loaded = true;
        w /= 2;
        h /= 2;
    }
    return true;
}

// 0x5038f0  Confidence: likely. Pixels start at offset 0x20; a reduced bitmap skips the full-size level
// (4 * the halved size). Animations load the whole block once through the header and point each frame
// into it; the header's lock_count becomes the frame count. Called only for a frame or a single bitmap.
void bm_lock_vbm(int index)
{
    bitmap_entry *e = &bm_bitmaps[index];
    File f;
    int bpp = bm_bytes_per_pixel(e->format);
    bitmap_entry *hdr;
    int size, i;

    if (e->anim_type == BM_ANIM_NONE) {
        f.open(e->name, 1, e->path_id);
        f.seek(0x20, 0);
        e->data = new ubyte[e->num_pixels * bpp];
        if (e->reduced)
            f.seek(e->height * e->width * bpp * 4, 1);
        f.read(e->data, e->num_pixels * bpp, 0, 0);
        f.close();
    } else {
        hdr = e - e->num_frames;
        if (hdr->data == NULL) {
            f.open(hdr->name, 1, e->path_id);
            f.seek(0x20, 0);
            size = hdr->num_frames * bpp * hdr->num_pixels;
            hdr->data = new ubyte[size];
            f.read(hdr->data, size, 0, 0);
            f.close();
            for (i = 0; i < hdr->num_frames; i++) {
                bitmap_entry *frame = hdr + 1 + i;
                frame->data = (ubyte *)hdr->data + (frame->num_frames - 1) * frame->num_pixels * bpp;
            }
            hdr->lock_count = hdr->num_frames;
        }
    }
}

// 0x503aa0  Confidence: exact (no entry pointer: every field is indexed from bm_bitmaps).
bool bm_lock_pcx(int index)
{
    bm_bitmaps[index].data = new ubyte[bm_bytes_per_pixel(bm_bitmaps[index].format) * bm_bitmaps[index].num_pixels];
    if (bm_bitmaps[index].format == BM_FORMAT_8_PALETTED)
        bm_bitmaps[index].palette = new ubyte[0x300];
    return pcx_read_bitmap(bm_bitmaps[index].name, (ubyte *)bm_bitmaps[index].data,
                           (ubyte *)bm_bitmaps[index].palette, bm_bitmaps[index].path_id) == 0;
}

// 0x503b20  Confidence: exact. The raw file becomes the data (m2v_read_bitmap allocates it).
bool bm_lock_m2v(int index)
{
    return m2v_read_bitmap(bm_bitmaps[index].name, (ubyte **)&bm_bitmaps[index].data, 0) != 0;
}

// 0x503b50  Confidence: exact.
void bm_unlock(int bm)
{
    bm_free_data(bm, false);
}

// 0x503b60  Confidence: exact.
const char *bm_get_filename(int bm)
{
    bitmap_entry *e = &bm_bitmaps[bm_handle_to_index(bm)];

    if (e->anim_type == BM_ANIM_FRAME)
        e -= e->num_frames;
    return e->name;
}

// 0x503ba0  Confidence: likely (D1102: the product reads a second stack argument; the mip info's w, h and
// level count are unused, and num_pixels lands in the dead argument slot of bm).
int bm_get_total_size(int bm, int bytes_per_pixel)
{
    int num_frames, w, h, num_pixels, num_levels;

    if (bm_get_current_frame(bm) < 0)
        return -1;
    num_frames = bm_get_num_frames(bm, NULL);
    bm_get_mipmap_info(bm, &w, &h, &num_pixels, &num_levels);
    return num_pixels * num_frames * bytes_per_pixel;
}

// 0x503c00  Confidence: exact. Reads the PCX header (and so the palette) of the current frame's file
// from any path; failure is fatal.
void bm_read_pcx_header_of(int bm, ubyte *palette)
{
    int i = bm_get_current_frame(bm);
    int w, h, bpp;
    char error[256];

    if (pcx_read_header(bm_bitmaps[i].name, &w, &h, &bpp, palette, PATH_ANY) != 0) {
        sprintf(error, "Couldn't open %s.\n", bm_bitmaps[i].name);
        VSDK_ERROR(BMPMAN_CPP, 0x936, error);
    }
}

// 0x503c80  Confidence: likely (the two bm_free_entry tails are separate in the code). An animation
// header frees the frames' texture cache slots but only its own entry; the frames stay on the used list.
void bm_release(int bm)
{
    bitmap_entry *e = &bm_bitmaps[bm];
    int i;

    if (e->anim_type == BM_ANIM_HEADER) {
        for (i = 0; i < e->num_frames; i++)
            gr_tcache_free(bm + 1 + i);
    } else {
        gr_tcache_free(bm);
    }
    bm_free_entry(bm_handle_to_index(bm));
}

// 0x503cf0  Confidence: likely (VC6 passes the stored width / height and the constant level count to
// bm_calc_num_pixels). The caller's buffer becomes the data (never freed by bm_free_data); the sixth
// argument is not read.
int bm_create(int format, int w, int h, void *data, void *palette, int a6)
{
    bitmap_entry *e = bm_alloc_entry();

    e->name = "USERBMAP";
    e->name_checksum = -1;
    e->type = BM_TYPE_USER;
    e->anim_type = BM_ANIM_NONE;
    e->width = w;
    e->height = h;
    e->num_levels = 1;
    e->num_pixels = bm_calc_num_pixels(e->width, e->height, e->num_levels);
    e->format = format;
    e->data = data;
    e->palette = palette;
    e->total_bytes = bm_bytes_per_pixel(format) * w * h;
    e->reduced = 0;
    return e - bm_bitmaps;
}

// 0x503d90  Confidence: exact. No caller.
void *bm_malloc(int size)
{
    return operator new(size);
}

// 0x503da0  Confidence: exact.
void bm_set_preload_on_load(int on)
{
    bm_preload_on_load = on;
}

// 0x503db0  Confidence: unsure. bm_name_pool_get_length: `jmp StringPool::GetLength` with ECX from the
// caller (bm_set_mark), at the end of the object next to imin, so presumably an out-of-line copy of a
// header inline member that forwards to GetLength. Which inline it is is not known; bm_set_mark is
// written with GetLength directly (NOTES open question).

// 0x503dc0  Confidence: exact. imin: the out-of-line copy of a vsdk\math header inline that closes this
// object (callers bm_read_header and File::read; the linker keeps this one copy). Defined here,
// not inline in vector.h, so that the callers keep their out-of-line calls under /Ob1 (D5606).
int imin(int a, int b)
{
    return a < b ? a : b;
}
