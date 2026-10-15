// stubs_player_effectmem.h - declarations of the Sum.exe v1.40 engine and game functions, classes and
// globals that player.cpp and vsdk\geom\effect_mem.cpp use but do not define. Not original source: only
// what these two units need. Names and addresses from symbols/sum.symbols; argument lists read from the
// call sites in build/sum.asm. Layouts give only the fields these units touch (offsets in comments,
// checked against symbols/types.h: Character 0x788, char_info 0x3d0). "(type guessed)" marks return
// or parameter types the call sites do not show.
//
// Why the other include/ headers are not used: stubs_sound_items_registry.h (DebugError, Mempool,
// StringPool, File) also defines struct Character and struct Player with too few fields and no
// methods, stubs_menus_dialogue.h includes symbols/types.h, whose C structs (Character, Player) cannot
// carry the thiscall methods and GameTimer members these units need. Including either would redefine
// Character / Player. timestamp.h (GameTimer) is used as is.
#ifndef STUBS_PLAYER_EFFECTMEM_H
#define STUBS_PLAYER_EFFECTMEM_H

#include "timestamp.h"                // GameTimer (src/decomp/vsdk)

typedef unsigned char ubyte;
typedef unsigned short ushort;

// Compile-time layout check (VC6 has no static_assert).
#define LAYOUT_CHECK(name, cond) typedef char layout_check_##name[(cond) ? 1 : -1]

// ---- debug (0x531cb0) -------------------------------------------------------------------------
// Not noreturn: every call site loops back to the call, so the error macro is a loop.
void DebugError(const char *file, int line, const char *msg);
#define VSDK_ERROR(file, line, msg) for (;;) DebugError(file, line, msg)

// ---- localisation ------------------------------------------------------------------------------
extern char *localize_strings[];      // 0x25cd4dc
#ifndef XSTR
#define XSTR(text, id) (localize_strings[id])
#endif

// ---- vsdk\math ----------------------------------------------------------------------------------
// Both default constructors exist out of line (vector constructor iterator and the fx record
// constructors call them), so they are declared, not defined, here.
class vector3 {
public:
    vector3();                        // 0x505530 vector3__vector3_default (returns this)
    float x, y, z;
};
class matrix {
public:
    matrix();                         // 0x50f660 matrix__matrix_default (three vector3 ctors)
    vector3 rvec, uvec, fvec;
};
LAYOUT_CHECK(vector3, sizeof(vector3) == 0xc);
LAYOUT_CHECK(matrix, sizeof(matrix) == 0x24);

// ---- vsdk\os\console.cpp ----------------------------------------------------------------------
// The objects are 0x10 apart in .bss (ccmd_show_framerate 0x5fbfe8, ccmd_targa 0x5fbff8,
// ccmd_max_skills 0x5fc1b8 .. player_free_list 0x5fc1c8), so 0x10 bytes; symbols/types.h lists 0xc.
class console_command {
public:
    console_command(const char *name, const char *help, void (*func)()); // 0x50a180
private:
    char _data[0x10];
};
#define ARG_NONE  0x01
#define ARG_TRUE  0x40
#define ARG_FALSE 0x80
extern int calling_a_command_function;   // 0x25db218
extern int doing_help_for_comand;        // 0x25db2e0 (spelling as in Summoner 2)
extern int checking_status_for_command;  // 0x25dadf0
extern unsigned int arg_type;            // 0x25dae04
void process_command(int arg_types);     // 0x50a5b0 (return value never used; type guessed)
void print_to_console(const char *text, int flags); // 0x50a5a0 (empty in this build)
extern char string_buffer[];             // 0x257ed80

// ---- vsdk\os\memory.cpp -----------------------------------------------------------------------
// In effect_mem.cpp the constructor (0x51dff0) and getPoolSpaceLeft (0x51e010) are emitted out of
// line at the end of the unit and called, so they are declared here without bodies. The other
// header (stubs_sound_items_registry.h) gives an inline constructor body; see NOTES.md.
class Mempool {                       // 0x1c bytes
public:
    Mempool(char *dest, int size, const char *name);    // 0x51dff0 (calls Copy, returns this)
    void Copy(char *dest, int size, const char *name);  // 0x512770
    unsigned int getPoolSpaceLeft();                    // 0x51e010 size - offset (compared JB)
    char *destination;                // +0x00
    unsigned int size;                // +0x04
    unsigned int offset;              // +0x08
    char name[0x10];                  // +0x0c ("Going into pool %s")
};
LAYOUT_CHECK(Mempool, sizeof(Mempool) == 0x1c);
void set_current_mempool(Mempool *pool);   // 0x5127f0
Mempool *get_current_mempool();            // 0x512800
extern Mempool level_mempool;              // 0x22e43c0
extern Mempool summon_mempool;             // 0x7592c0

// ---- vsdk\os\stringpool.cpp -------------------------------------------------------------------
class StringPool {                    // 0x14 bytes
public:
    StringPool();                     // 0x544dc0 (zeroes the five fields)
    ~StringPool();                    // 0x544e20
    void Init(char *pool, int max_length, bool allocated); // 0x544b80
    const char *AddUnique(const char *s);                  // 0x544cd0
private:
    char *pool;
    bool allocated;
    int max_length;
    int length;
    int count;
};
LAYOUT_CHECK(StringPool, sizeof(StringPool) == 0x14);

// ---- vsdk\vfile ---------------------------------------------------------------------------------
#define PATH_ANY 9999999              // 0x98967f, search every path
class File {                          // 0x114 bytes (stack frames of effect_base_read / effects_load_bases)
public:
    File();                                            // 0x5140d0
    ~File();                                           // 0x5140f0
    int open(const char *name, int mode, int path_id); // 0x514630 (0 = success)
    void close();                                      // 0x514740
    bool eof();                                        // 0x514a10
    int read(void *buf, int size, int min_version, int unused); // 0x525fe0 (bytes read)
    int read_line(char *buf, int size, int unused);    // 0x525f60 (0 at end of file)
private:
    char _data[0x114];
};

// ---- vsdk\bmpman --------------------------------------------------------------------------------
void bm_set_preload_on_load(int on);  // 0x503da0

// ---- vsdk\geom (other units) --------------------------------------------------------------------
// The loaded effect base ("vfx base") an effect id resolves to. Only +0x1c is written here.
class vfx_base {
public:
    void load_bitmaps(int flag);              // 0x536e50 vfx_base__load_bitmaps
    void relocate_node(char *a, char *b);     // 0x536bc0 vfx_node_relocate (argument meaning unsure)
    char _unk00[0x1c];
    int category;                             // +0x1c copied from the base record's category
};
void ptr_relocate(void *p, char *base);       // 0x564d50 (*p += base unless NULL)

// ---- effect streamer (game, 0x4404a0..0x440b20) --------------------------------------------------
int  effect_stream_queue_add(const char *name, Mempool *pool, int arg); // 0x4404a0 (0 = refused)
int  effect_stream_queue_contains(const char *name);                    // 0x440510
void effect_stream_start();                                             // 0x440560
int  effect_stream_update();                                            // 0x4408a0 (1 = still reading)
void effect_stream_reset();                                             // 0x440b20

// ---- objects (objects.cpp, living_entity.cpp) ----------------------------------------------------
class Character;

class ai_group {                      // 0x54 bytes (types.h ai_group)
public:
    char _unk00[0x48];
    int leader;                       // +0x48 leader's object handle
    char _unk4c[0x08];
};

class ai_state {                      // Character +0x324, 0x234 bytes
public:
    void reset();                     // 0x472450 ai_reset
    void goals_remove_type1();        // 0x475e00 ai_goals_remove_type1
    bool is_disabled_any();           // 0x475000 ai_is_disabled_any
    bool is_casting();                // 0x4731d0 ai_is_casting
    void go_idle();                   // 0x4738c0 ai_go_idle
    char _unk000[0x50];
    unsigned int flags;               // +0x050 (Character +0x374) bit 0: player controlled, AI off
    char _unk054[0x1a8 - 0x54];
    ai_group *group;                  // +0x1a8 (Character +0x4cc)
    char _unk1ac[0x234 - 0x1ac];
};
#define AI_FLAG_PLAYER_CONTROL 0x1

// Object base: one virtual function (living_entity__vtable 0x581914 has one slot), then the active
// object list links. obj_active_list (0x970770) is a bare 12-byte head of this kind.
class obj {
public:
    virtual void vfunc0();            // 0x44edf0 living_entity__vfunc0: frees AI, equipment, lights...
    obj *next;                        // +0x004 obj_active_list
    obj *prev;                        // +0x008
};
LAYOUT_CHECK(obj, sizeof(obj) == 0xc);

struct char_info {                    // 0x3d0 bytes, char_infos 0x6d7e50
    char _unk000[0x10];
    int species;                      // +0x010 0 Joseph, 1 Jekhar, 2 Flece, 3 Rosalind
    char _unk014[0x3d0 - 0x14];
};
LAYOUT_CHECK(char_info, sizeof(char_info) == 0x3d0);
extern char_info char_infos[256];     // 0x6d7e50

#define SPECIES_JOSEPH   0
#define SPECIES_JEKHAR   1
#define SPECIES_FLECE    2
#define SPECIES_ROSALIND 3

// Character +0x320 flags
#define CF_SUMMONLIKE    0x2          // cleared by party_add_member (party_cycle skips it)
#define CF_HIDDEN        0x8
#define CF_DEAD          0x80
#define CF_LEFT_PARTY    0x800000

#define MAX_SKILLS 0x54               // 84 skill ids

class Character : public obj {        // 0x788 bytes
public:
    void set_name(const char *name);  // 0x44edb0 character_set_name
    char *get_name();                 // 0x44ed90 character_get_name
    void set_team(int team);          // 0x454610 character_set_team
    void resurrect();                 // 0x454070 character_resurrect
    void clear_all_effects();         // 0x455a90 character_clear_all_effects
    int get_max_stat(int which);      // 0x462840 character_get_max_stat (0 HP, 1 AP)

    int type;                         // +0x00c 7 = party member
    char _unk010[0x80 - 0x10];
    int handle;                       // +0x080
    char _unk084[0x318 - 0x84];
    Character *live_next;             // +0x318 living_list
    Character *live_prev;             // +0x31c
    unsigned int flags;               // +0x320 CF_*
    ai_state ai;                      // +0x324
    char _unk558[0x5f8 - 0x558];
    float hp;                         // +0x5f8
    char _unk5fc[0x5fe - 0x5fc];
    short ap;                         // +0x5fe
    short level;                      // +0x600
    char _unk602[0x614 - 0x602];
    ubyte skill[0x60];                // +0x614 by skill id, value * 0.01
    char _unk674[0x71c - 0x674];
    char_info *info;                  // +0x71c
    char _unk720[0x788 - 0x720];
};
LAYOUT_CHECK(Character, sizeof(Character) == 0x788);

extern obj obj_active_list;           // 0x970770 head (next +4 = 0x970774, prev +8 = 0x970778)
extern Character living_list;         // 0x8eca60 sentinel (live_next +0x318, live_prev 0x8ecd7c)

obj *obj_from_handle(int handle);                     // 0x46b130
Character *living_from_handle(int handle);            // 0x46b1a0
Character *living_entity_create(int type, int char_info, const vector3 *pos, const matrix *orient,
                                int a, int b, int c, int d, int e);   // 0x461b80 (last five: 1, 0, 0, 0, 0)
void ai_group_create(Character *c);                   // 0x492ba0 (return value unused; type guessed)
void ai_group_add_member(ai_group *group, Character *c); // 0x492c50
void ai_group_remove_member(Character *c);            // 0x492ca0

// ---- skills (characterinfo.cpp) ---------------------------------------------------------------
struct skill_info {                   // 0x34 bytes, skill_table 0x8b3f88
    char _unk00[0x08];
    int recovery;                     // +0x08 seconds (player_start_skill_cooldown multiplies by 1000)
    char _unk0c[0x34 - 0x0c];
};
LAYOUT_CHECK(skill_info, sizeof(skill_info) == 0x34);
extern skill_info skill_table[MAX_SKILLS]; // 0x8b3f88
bool skill_is_summon(int skill);           // 0x446e70
extern bool All_summons;                   // 0x8b50b0
float xp_level_skill_cap(int level);       // 0x489bf0

// ---- multiplayer ------------------------------------------------------------------------------
struct mp_player {
    mp_player *next;                  // +0x00 mp_player_list
    char _unk04[0x28 - 0x04];
    void *object;                     // +0x28 cleared by party_remove_all
};
extern mp_player mp_player_list;      // 0x651418 head
extern mp_player *mp_local_player;    // 0x651598

// ---- savegame, camera, interface, hud ----------------------------------------------------------
class savegame {
public:
    int get_flag(int level, const char *name);   // 0x4cfca0 savegame_get_flag
};
extern savegame savegame_state;       // 0x2493a38

class camera {
public:
    void set_target(Character *c);    // 0x478ff0 camera_set_target
};
extern camera game_camera;            // 0x60a018

void interface_refresh_cur_screen();  // 0x412db0
void ui_clear_pick_overrides();       // 0x414560
void world_pick_clear_filter();       // 0x43a390
void levelrender_clear_draw_lists();  // 0x486b00
void hud_show_message(const char *text); // 0x414a20
extern int hud_target_mode;           // 0x58bdb8 (-1 none)
int game_play_sound(int handle, float volume, float pan, int unused, int group); // 0x468d90

extern int game_flags;                // 0x5fbfd8
#define GF_SINGLE_PLAYER 0x1
#define GF_MULTIPLAYER   0x2
#define GF_LEADER_SOUND  0x200        // party_set_leader plays sound 0x28 when set (meaning unsure)
#define GF_SOLO_MODE     0x1000
extern int cur_level_index;           // 0x59f120 (4 = world map)

#endif
