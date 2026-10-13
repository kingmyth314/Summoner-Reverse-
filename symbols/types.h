// Structure layouts for Sum.exe v1.40, referenced by symbols/sum.symbols.
// Field offsets are observed in Sum.exe unless noted. Reference names come from the projects listed
// in docs/reference/engine-references.md (layouts only; no code is copied).
#pragma once
#include <stdint.h>

// gr::Screen at 0x25d8d30 (zeroed by gr__init, 0x2a dwords). Layout matches Dash Faction's
// rf::gr::Screen for every field Sum.exe has been seen to use.
typedef struct gr__Screen {
    int32_t signature;      // +0x00
    int32_t max_w;          // +0x04 screen width
    int32_t max_h;          // +0x08 screen height
    int32_t mode;           // +0x0C renderer: 0x66 Direct3D, 0x67 Glide, 0x68 OpenGL
    int32_t window_mode;    // +0x10 200 windowed, 201 fullscreen, 202 desktop size
    int32_t dual_monitors;  // +0x14 (bool in DF; padded)
    float   aspect;         // +0x18 (width / height) * 0.75
    int32_t rowsize;        // +0x1C
    int32_t bits_per_pixel; // +0x20
    int32_t bytes_per_pixel;// +0x24
    float   far_zvalue;     // +0x28
    int32_t offset_x;       // +0x2C clip origin x
    int32_t offset_y;       // +0x30 clip origin y
    int32_t clip_width;     // +0x34
    int32_t clip_height;    // +0x38
    int32_t max_bitmap_w;   // +0x3C from GL_MAX_TEXTURE_SIZE (OpenGL) / device caps (D3D)
    int32_t max_bitmap_h;   // +0x40
    int32_t clip_left;      // +0x44
    int32_t clip_right;     // +0x48
    int32_t clip_top;       // +0x4C
    int32_t clip_bottom;    // +0x50
    // +0x54.. colour, textures, fog, tint (Dash Faction layout; not yet verified in Sum.exe)
} gr__Screen;

// A developer console command (Summoner 2: console_command). On PC the constructor takes
// (name, description, callback); there is no type field at +0xC as in Summoner 2's version.
typedef struct console_command {
    const char* name;        // +0x0
    const char* description; // +0x4
    void (*callback)(void);  // +0x8
} console_command;

// Text-table parser (Summoner 2: TextFile, vsdk\parse\parse.cpp). Partial layout observed in the
// TextFile__* methods; the gap holds the file name and other state (not yet mapped).
typedef struct TextFile {
    char* buffer;            // +0x00 file contents (freed by Close unless a static buffer is used)
    char* cursor;            // +0x04 current parse position
    uint8_t flag08;          // +0x08 zeroed by the constructor
    uint8_t _unmapped[0x3f]; // +0x09
    int32_t line;            // +0x48 current line number (Rewind sets 1)
    int32_t size;            // +0x4C buffer length (SkipTo bounds check)
} TextFile;

// .vpp packfile records (vsdk\vfile\packfile\file_packfile.cpp). PC layouts differ from Summoner 2's:
// observed in packfile_find_packfile / packfile_find_in_packfile / packfile_fopen / fseek.
typedef struct Packfile Packfile;
typedef struct PackfileEntry {
    uint32_t hash;           // +0x00 packfile_hash(name)
    const char* name;        // +0x04
    int32_t sector;          // +0x08 offset in 2048-byte sectors from the packfile start
    int32_t size;            // +0x0C bytes
    Packfile* packfile;      // +0x10
    void* hdd_file;          // +0x14 FILE* when opened from the hard drive
    void* cd_file;           // +0x18 FILE* when the packfile is on the CD (Packfile.flags & 1)
} PackfileEntry;             // 0x1C bytes
struct Packfile {
    char name[0x20];         // +0x00 e.g. "summoner.vpp"
    uint8_t flags;           // +0x20 bit 0: on CD
    uint8_t _pad[7];
    int32_t num_files;       // +0x28
    PackfileEntry* entries;  // +0x2C
    int32_t directory_size;  // +0x30
};                           // 0x34 bytes
typedef struct PackfileLookup {
    uint32_t hash;           // -1 = empty slot
    PackfileEntry* entry;
} PackfileLookup;            // table of 0x50e9 entries at 0x2d68780

// Bump allocator (vsdk memory.cpp). PC layout from Mempool__Copy / allocate_in_mempool; Summoner 2's
// version also has an alignment byte at +0xC, which PC replaces with the name (alignment fixed at 16).
typedef struct Mempool {
    uint8_t* destination;    // +0x00
    uint32_t size;           // +0x04
    uint32_t offset;         // +0x08
    char name[1];            // +0x0C length not yet known
} Mempool;

// Command-line parameter (vsdkoscmdline.cpp). Same layout as Dash Faction's rf::CmdLineParam.
typedef struct CmdLineParam {
    struct CmdLineParam* next; // +0x00
    struct CmdLineParam* prev; // +0x04
    const char* name;        // +0x08 e.g. "-glide"
    const char* help;        // +0x0C
    char* arg;               // +0x10 heap copy of the following argument
    uint8_t name_found;      // +0x14
    uint8_t has_argument;    // +0x15
} CmdLineParam;
typedef struct CmdArg {
    char* arg;               // +0x0
    uint8_t is_done;         // +0x4
} CmdArg;                    // cmdline_args[50] at 0x2dc6ea8

// Bitmap manager entry (vsdk\bmpman\bmpman.cpp), bm_bitmaps[0xaf0] at 0x2591c78; handle = index.
// Smaller than Red Faction's 0x6C-byte BitmapEntry (Dash Faction): the name is a pointer into
// bm_name_pool instead of char[32]. Offsets observed in bm_load_internal, bm_create, bm_lock_*.
typedef struct bitmap_entry {
    const char* name;        // +0x00 in bm_name_pool
    int32_t name_checksum;   // +0x04 bm_hash_name(name); -1 for user bitmaps
    uint16_t width;          // +0x08
    uint16_t height;         // +0x0A
    int32_t num_pixels;      // +0x0C all mip levels (bm_calc_num_pixels)
    int32_t type;            // +0x10 0 none, 1 pcx, 2 tga, 3 user, 4 vbm, 5 m2v
    int32_t anim_type;       // +0x14 0 single, 1 animation header, 2 frame
    int32_t format;          // +0x18 same numbering as Dash Faction bm::Format
    uint8_t num_levels;      // +0x1C mip levels
    uint8_t num_frames;      // +0x1D header: frame count; frame: frame number (1-based)
    uint8_t _pad1e[2];
    void* data;              // +0x20 locked pixels
    float frames_per_ms;     // +0x24
    void* palette;           // +0x28 0x300 bytes for 8-bit paletted
    struct bitmap_entry* next; // +0x2C used/free list
    struct bitmap_entry* prev; // +0x30
    uint8_t lock_count;      // +0x34
    uint8_t _pad35[3];
    int32_t total_bytes;     // +0x38
    int32_t path_id;         // +0x3C File::open path id (9999999 = default)
    uint8_t reduced;         // +0x40 loaded at half size (bm_reduce_textures)
    uint8_t _pad41[3];
    int32_t mark_count;      // +0x44 bm_set_mark nesting
} bitmap_entry;              // 0x48 bytes

// Bitmap font (vsdk\gr\gr_font.cpp), gr_fonts[6] at 0x25d9238. Same 0x6C-byte layout as Dash
// Faction's rf::gr::Font; offsets observed in gr_font_create, gr_font_get_char, gr_string.
typedef struct FontCharInfo {
    int32_t spacing;         // +0x0
    int32_t width;           // +0x4
    int32_t pixel_data_offset; // +0x8
    int16_t kerning_entry;   // +0xC first kern pair for this char, -1 none
    int16_t user_data;       // +0xE
} FontCharInfo;
typedef struct Font {
    char filename[32];       // +0x00
    int32_t signature;       // +0x20 0 = free slot
    int32_t format;          // +0x24 0xF 4-bit alpha, 0x0F0F0F0F 16-bit, -0x10 8-bit paletted
    int32_t num_chars;       // +0x28
    int32_t first_ascii;     // +0x2C
    int32_t default_spacing; // +0x30
    int32_t height;          // +0x34
    int32_t height2;         // +0x38
    int32_t num_kern_pairs;  // +0x3C
    int32_t pixel_data_size; // +0x40
    uint8_t* kern_data;      // +0x44 3-byte {ch0, ch1, offset}
    FontCharInfo* chars;     // +0x48
    uint8_t* pixel_data;     // +0x4C
    int32_t* palette;        // +0x50
    int32_t bm_handle;       // +0x54 4444 texture made with bm_create
    int32_t bm_width;        // +0x58 64, 128 or 256
    int32_t bm_height;       // +0x5C
    int32_t* char_x_coords;  // +0x60
    int32_t* char_y_coords;  // +0x64
    uint16_t* bm_bits;       // +0x68
} Font;                      // 0x6C bytes

// One entry of gameseq_state_table (0x588a70): handlers for a game state.
typedef struct GameStateHandlers {
    void (*enter)(void);    // +0x0 called after the state becomes current
    void (*frame)(void);    // +0x4 called every GameTick while current
    void (*leave)(void);    // +0x8 called before switching away
} GameStateHandlers;
