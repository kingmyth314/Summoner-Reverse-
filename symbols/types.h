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

// One entry of gameseq_state_table (0x588a70): handlers for a game state.
typedef struct GameStateHandlers {
    void (*enter)(void);    // +0x0 called after the state becomes current
    void (*frame)(void);    // +0x4 called every GameTick while current
    void (*leave)(void);    // +0x8 called before switching away
} GameStateHandlers;
