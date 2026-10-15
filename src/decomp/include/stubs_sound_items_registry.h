// engine_stubs.h - declarations of the Sum.exe v1.40 engine functions, classes and globals that
// registry.cpp, gamesound.cpp and item_paging.cpp call or read. Not original source: only enough
// for the three units to compile. Signatures are read from the call sites in the disassembly;
// layouts give only the fields these units touch (offsets in comments). Names from symbols/sum.symbols.
#ifndef STUBS_SOUND_ITEMS_REGISTRY_H
#define STUBS_SOUND_ITEMS_REGISTRY_H

typedef unsigned char ubyte;

// ---- debug (0x531cb0) -------------------------------------------------------------------------
// MessageBox + exit(1) on PC, but not declared noreturn: every call site in Sum.exe is followed by
// a jump back to the call (the error macro is a loop), so callers here write
//     for (;;) DebugError(file, line, msg);
void DebugError(const char *file, int line, const char *msg);

// ---- vsdk\math --------------------------------------------------------------------------------
struct vector3 { float x, y, z; };

// ---- String (0x556ea0 String::operator=(const char*)) ------------------------------------------
class String {
public:
    String &operator=(const char *s); // NULL -> empty
};

// ---- vsdk\vfile -------------------------------------------------------------------------------
class File {                          // 0x114 bytes
public:
    File();                           // 0x5140d0
    ~File();                          // 0x5140f0
    bool find(const char *name, int path_id); // 0x5141c0 (__thiscall, returns AL)
private:
    char _data[0x114];
};
void file_strip_ext(char *out, const char *name); // 0x513100
extern int path_tables;               // 0x60ac5c  file_add_path id of data\tables

#define PATH_ANY 9999999              // 0x98967f, the "search every path" id

// ---- vsdk\parse\parse.cpp ---------------------------------------------------------------------
class TextFile {                      // 0x50 bytes (stack layout of the callers)
public:
    TextFile();                       // 0x515960
    ~TextFile();                      // 0x5159d0 (calls Close)
    int Open(const char *name, int path_id);  // 0x515d20 (result never used here)
    void Close();                     // 0x515e90
    bool Peek(const char *s);         // 0x5160d0
    bool Accept(const char *s);       // 0x516110
    void Expect(const char *s);       // 0x5161e0
    int ParseInt();                   // 0x516290
    void ParseString(char *buf, int size, char open, char close); // 0x5166b0
    int ParseTypeString(const char **types, int count);          // 0x516bd0
private:
    char _data[0x50];
};

// ---- vsdk\os\stringpool.cpp -------------------------------------------------------------------
class StringPool {                    // 0x14 bytes (0x941740..0x941754)
public:
    StringPool(char *pool, int max_length, bool allocated); // 0x544de0 (out of line here)
    ~StringPool();                    // 0x544e20 (Release)
    void Reset();                     // 0x544bd0
    const char *AddUnique(const char *s); // 0x544cd0
private:
    char *pool;                       // +0x00
    bool allocated;                   // +0x04
    int max_length;                   // +0x08
    int length;                       // +0x0C
    int count;                        // +0x10
};

// ---- vsdk memory.cpp ----------------------------------------------------------------------------
// Mempool::Copy (0x512770) is a plain __thiscall member returning void (EAX is not `this`).
// The inline constructor is an assumption to explain init_mempools_ip_general (see NOTES.md).
class Mempool {                       // 0x1C bytes
public:
    Mempool(char *dest, int size, const char *name) { Copy(dest, size, name); }
    void Copy(char *dest, int size, const char *name);
    char *destination;                // +0x00
    int size;                         // +0x04
    int offset;                       // +0x08
    char name[0x10];                  // +0x0C
};
void set_current_mempool(Mempool *pool); // 0x5127f0

// ---- vsdk sound (snd_*) ---------------------------------------------------------------------------
// The three floats of snd_get_handle are the three table parameters of a sound (sounds.tbl); their
// meaning is not settled. group: 0 effects, 1 music. from_start: search from slot 0, not 8.
int snd_get_handle(const char *name, float p1, float p2, float p3, int group, bool from_start); // 0x5207d0
int snd_load(int handle, int streamed);                                    // 0x5209c0
int snd_play(int handle, float volume, float pan, int unused, int group); // 0x520d50
int snd_play_streamed(int handle, float volume, int arg);                  // 0x520ea0
int snd_play_streamed_looping(int handle, float volume, int arg);          // 0x520f20
int snd_play_3d(int handle, const vector3 *pos, float a, float b, int c);  // 0x521480 (trailing types unsure)
void snd_stop(int sig);                                                    // 0x5215d0
void snd_stop_fade(int sig, int ms);                                       // 0x521620
bool snd_is_playing(int sig);                                              // 0x521650
void snd_set_volume(int sig, float volume);                                // 0x521680
void snd_set_group_volume(int group, float volume);                        // 0x5218e0

// A sound reference in a table record (12 bytes).
struct sound_ref {
    const char *name;                 // +0x0
    int handle;                       // +0x4 -1 until loaded
    ubyte params[3];                  // +0x8 volume (0..255), two more table values
};

// ---- misc engine ----------------------------------------------------------------------------------
extern char string_buffer[];          // 0x257ed80
extern int game_flags;                // 0x5fbfd8  0x2 multiplayer, 0x20 / 0x200 see feedback_play
float frand();                        // 0x5426e0  rand() * (1/32768)
int species_is_hero(int species);     // 0x449a20
int bm_find(const char *name);        // 0x5025d0  -1 if none
void gr_page_in(int bm, int arg);     // 0x5069e0

// Level objects (0x70 bytes each); +0xC points at 8-byte face records whose first word is a
// 1-based index into level_texture_bms.
struct lobj_face { unsigned short texture; unsigned short _pad2; int _unk4; };
struct lobj { char _pad0[0xc]; lobj_face *faces; char _pad10[0x60]; };
extern lobj *lobjs;                   // 0xa5a2cc
extern int *level_texture_bms;        // 0xa5a320

// ---- vsdk\ca\skeleton.cpp (vim meshes) ------------------------------------------------------------
struct vim {
    char _pad0[0x10];
    int textures[6];                  // +0x10 bitmap handles
    int num_textures;                 // +0x28
};
vim *vim_load(const char *name, int arg);          // 0x543920
int vim_find_tag(vim *mesh, const char *name);     // 0x543c20  -1 if none

// ---- items.cpp ------------------------------------------------------------------------------------
struct ItemType {                     // 0x100 bytes
    const char *name;                 // +0x00
    int kind;                         // +0x04 0 weapon, 1 armour, 2 misc
    int material;                     // +0x08
    const char *model;                // +0x0C
    vim *mesh;                        // +0x10 paged-in mesh, NULL when paged out
    char _pad14[0x68];
    int drop_tag1;                    // +0x7C "$drop-ground01" (weapons)
    int drop_tag2;                    // +0x80 "$drop-ground02"
    char _pad84[0x7c];
};
extern ItemType item_types[];         // 0x8cbec0
extern int num_item_types;            // 0x8ec7c0

// ---- characters / players -------------------------------------------------------------------------
struct char_info { char _pad0[0x10]; int species; };      // +0x10 species
struct Character {
    char _pad0[0xc];
    int type;                         // +0x00C 7 = player party member
    char _pad10[0x70];
    int handle;                       // +0x080
    char _pad84[0x698];
    char_info *info;                  // +0x71C
};
struct Player {
    Player *next;                     // +0x0 circular list, head party_list
    Player *prev;                     // +0x4
    int char_info;                    // +0x8
    int handle;                       // +0xC character object handle
};
extern Player party_list;             // 0x5fc350

#endif
