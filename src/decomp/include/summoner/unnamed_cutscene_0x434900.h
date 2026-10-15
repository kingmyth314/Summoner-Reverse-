// summoner/unnamed_cutscene_0x434900.h - the cutscene file (code 0x434900-0x437659, .data 0x59399c-0x593c31,
// .bss 0x5fccc0-0x609f3b; source game/unnamed_cutscene_0x434900.cpp, decomp52). THE FILE NAME IS A
// PLACEHOLDER (no path string, no strings.tbl block, sum.symbols module "-"); the directory is ours too.
// Not original source. Layouts: symbols/types.h cutscene_entry (0x140) and the records cutscene_parse_table
// allocates from cutscene_alloc_pool (names ours); docs/sum-exe/cutscenes.md.
// Included by summoner/summoner.h and levelscripts/level_scripts.h; includes neither.
#ifndef SUMMONER_UNNAMED_CUTSCENE_0X434900_H
#define SUMMONER_UNNAMED_CUTSCENE_0X434900_H

#include "vsdk/vsdk.h"
#include "vsdk/os/norlsallocpool.h"
#include "vsdk/os/console.h"
#include "engine/camera/camera_sequence.h"

class TextFile;

#define MAX_CUTSCENES            85   // cutscene_entries (init_cutscene_entries_6034c8 loop count 0x55)
#define MAX_CUTSCENE_CAMERAS     4    // the parser does not check these limits
#define MAX_CUTSCENE_EFFECTS     2
#define MAX_CUTSCENE_SHOT_EFFECTS 10
#define MAX_CUTSCENE_MINITRACKS  4
#define MAX_CUTSCENE_FOGS        16
#define MAX_CUTSCENE_VISSETS     16   // cutscene_parse_visset checks this one (and 16 names per set)
#define MAX_CUTSCENE_FADES       16
#define MAX_CUTSCENE_SEQUENCES   50   // the camera shots of the cutscene object
#define MAX_CUTSCENE_SAVED_ACTORS 175 // ends at the pool buffer 0x5fd260; cutscene_play does not check it
#define CUTSCENE_POOL_SIZE       0x4c00

#define CUTSCENE_FLAG_NOABORT        1  // +NoAbort: Esc does not end it (interface_frame)
#define CUTSCENE_FLAG_HIDEUNSCRIPTED 2  // +HideUnscripted

// "$Animation:" / "$Character Animation:" without a shot number (8 bytes): started at load time.
struct cutscene_effect {
    char *name;                       // +0x0 .vfx
    char *character;                  // +0x4 actor name, NULL for $Animation:
};
// The same with "shot [length]" (0x10 bytes): started by cutscene_events_frame at the shot.
struct cutscene_shot_effect {
    char *name;                       // +0x0
    short shot;                       // +0x4
    char *character;                  // +0x8
    short length;                     // +0xC shots it is drawn for, 0 = until the end (at least 1 when given)
};
struct cutscene_minitrack {           // "+Minitrack: file shot" (8 bytes)
    char *name;                       // +0x0
    short shot;                       // +0x4
};
struct cutscene_fog {                 // "$Fog: near far shot" (0xc bytes)
    float near_dist;                  // +0x0
    float far_dist;                   // +0x4
    short shot;                       // +0x8
};
struct cutscene_visset {              // "$VisSet: shot "$vis.." .." (0x44 bytes)
    ushort shot;                      // +0x00 must grow from set to set
    ushort count;                     // +0x02
    char *names[16];                  // +0x04
};
struct cutscene_fade {                // "$Fade Out:" / "$Fade In: time length shot" (0x10 bytes)
    int fade_out;                     // +0x0 1 Fade Out, 0 Fade In
    float time;                       // +0x4 seconds after the shot starts
    float length;                     // +0x8 seconds
    short shot;                       // +0xC
};

// A cutscene.tbl entry (0x140 bytes). +4 and +0xc are not set by the constructor.
class cutscene_entry {
public:
    cutscene_entry();                 // 0x4349b0 cutscene_entry__ctor

    char *name;                                   // +0x000 $Cutscene:
    ushort flags;                                 // +0x004 CUTSCENE_FLAG_*
    char *level;                                  // +0x008 $Level: ("any", "Placeholder")
    char *script;                                 // +0x00C $Script: (defaults to $Level)
    short num_cameras;                            // +0x010
    char *cameras[MAX_CUTSCENE_CAMERAS];          // +0x014 .csc files
    short num_effects;                            // +0x024
    cutscene_effect *effects[MAX_CUTSCENE_EFFECTS];        // +0x028
    short num_shot_effects;                       // +0x030
    cutscene_shot_effect *shot_effects[MAX_CUTSCENE_SHOT_EFFECTS];   // +0x034
    char *soundtrack;                             // +0x05C $Soundtrack:
    short num_minitracks;                         // +0x060
    cutscene_minitrack *minitracks[MAX_CUTSCENE_MINITRACKS];  // +0x064
    short num_fogs;                               // +0x074
    cutscene_fog *fogs[MAX_CUTSCENE_FOGS];        // +0x078
    short num_vissets;                            // +0x0B8
    cutscene_visset *vissets[MAX_CUTSCENE_VISSETS];          // +0x0BC
    short num_fades;                              // +0x0FC
    cutscene_fade *fades[MAX_CUTSCENE_FADES];     // +0x100
};
LAYOUT_CHECK(cutscene_entry, sizeof(cutscene_entry) == 0x140);

// The cutscene player (one static object, 0x1648 bytes at 0x601e68; class and member names ours, the
// sum.symbols names of the fields are the macros below). The thiscall members are its methods; the free
// functions of the file use the object by its address.
class cutscene {
public:
    cutscene();                       // 0x4350b0 cutscene__ctor
    ~cutscene();                      // 0x4351c0 cutscene__dtor
    bool load_files(cutscene_entry *e);       // 0x434b40 cutscene_load_files
    bool bind_actors(cutscene_entry *e);      // 0x434e20 cutscene_bind_actors
    bool load_sounds(cutscene_entry *e);      // 0x435000 cutscene_load_sounds
    void unload();                    // 0x4351e0 cutscene_unload
    int  set_data_path(const char *name);     // 0x435320 cutscene_set_data_path (the path id, -1)
    void clear_data_path();           // 0x4353b0 cutscene_clear_data_path
    bool load_data(cutscene_entry *e);        // 0x4353e0 cutscene_load_data
    void get_fog_distances(float *near_out, float *far_out);   // 0x4354a0 cutscene_get_fog_distances
    void apply_vis_overrides();       // 0x435520 cutscene_apply_vis_overrides

    cutscene_entry *data;             // +0x0000 the playing entry, NULL when none
    int path_id;                      // +0x0004 data\cutscenes\<name> path id, -1
    float time_offset;                // +0x0008 start of the first shot (0), -1 when unloaded
    float end_time;                   // +0x000C end of the last shot of the last camera file
    unsigned int start_time;          // +0x0010 timeGetTime at shot 0 (unsigned: the G3 conversion)
    short num_sequences;              // +0x0014
    cutscene_camera_sequence sequences[MAX_CUTSCENE_SEQUENCES];   // +0x0018
    short cur_sequence;               // +0x15F8 -1 before shot 0
    short cur_fade_event;             // +0x15FA next $Fade
    bool sequence_started;            // +0x15FC a shot started this frame
    int effect_handles[MAX_CUTSCENE_EFFECTS];          // +0x1600 cutscene_effect handles (bind_actors)
    int fx_handles[MAX_CUTSCENE_SHOT_EFFECTS];         // +0x1608 cutscene_shot_effect handles
    int soundtrack_sound;             // +0x1630 snd_get_handle of $Soundtrack
    int minitrack_sounds[MAX_CUTSCENE_MINITRACKS];     // +0x1634 only ever set to -1 (load_sounds)
    int vis_index;                    // +0x1644 last applied $VisSet, -1
};
LAYOUT_CHECK(cutscene, sizeof(cutscene) == 0x1648);

// One saved actor (8 bytes): the name hash and four flags, restored by cutscene_cleanup (names ours).
struct cutscene_saved_actor {
    int name_hash;                    // +0 name_hash_nocase(character_get_name)
    unsigned int summonlike : 1;      // +4 bit 0: Character +0x320 bit 0x2
    unsigned int flag_20 : 1;         //    bit 1: +0x320 bit 0x20
    unsigned int alt_texture : 1;     //    bit 2: +0x320 bit 0x2000000 (armour alt texture)
    unsigned int active : 1;          //    bit 3: obj_flags bit 0
};

// ---- .bss / .data ---------------------------------------------------------------------------------
extern cutscene the_cutscene;                         // 0x601e68 (object name ours)
extern cutscene_entry cutscene_entries[MAX_CUTSCENES]; // 0x6034c8
extern NoRlsAllocPool cutscene_alloc_pool;            // 0x5fccc0
extern char cutscene_pool_buffer[CUTSCENE_POOL_SIZE]; // 0x5fd260 (name ours)
extern cutscene_saved_actor cutscene_saved_actors[MAX_CUTSCENE_SAVED_ACTORS];   // 0x5fcce8
extern int cutscene_num_saved_actors;                 // 0x6034b0
extern float cutscene_saved_fog_multiplier;           // 0x601e60
extern float cutscene_saved_fog_far;                  // 0x609f08 holds the NEAR distance (name swapped, D3204)
extern float cutscene_saved_fog_near;                 // 0x609f1c holds the FAR distance (name swapped, D3204)
extern float cutscene_last_time;                      // 0x609f2c
extern int cutscene_num_entries;                      // 0x609f30
extern bool cutscene_playing;                         // 0x609f34 set for "Placeholder", never read
extern char *cutscene_pending_name;                   // 0x609f38
extern int cutscene_saved_music;                      // 0x59399c (.data -1)
extern int cutscene_stream_sound;                     // 0x5939a0 (.data -1)
extern console_command ccmd_play_cutscene;            // 0x6034b8
extern console_command ccmd_play_and_load_cutscene;   // 0x609f10
extern console_command ccmd_stop_cutscene;            // 0x609f20

// The fields under their sum.symbols names (other units read them; D3200, as D2205 for game_camera).
#define cutscene_data              (the_cutscene.data)               // 0x601e68
#define cutscene_path_id           (the_cutscene.path_id)            // 0x601e6c
#define cutscene_time_offset       (the_cutscene.time_offset)        // 0x601e70
#define sequence_start_time        (the_cutscene.start_time)         // 0x601e78
#define cutscene_num_sequences     (the_cutscene.num_sequences)      // 0x601e7c
#define cutscene_sequences         (the_cutscene.sequences)          // 0x601e80
#define cutscene_cur_sequence      (the_cutscene.cur_sequence)       // 0x603460
#define cutscene_cur_fade_event    (the_cutscene.cur_fade_event)     // 0x603462
#define cutscene_sequence_started  (the_cutscene.sequence_started)   // 0x603464
#define cutscene_fx_handles        (the_cutscene.fx_handles)         // 0x603470

// ---- functions ------------------------------------------------------------------------------------
void cutscene_parse_table();          // 0x4355e0 cutscene.tbl (GameInit)
int  cutscene_parse_visset(cutscene_entry *e, TextFile *tf);   // 0x435ff0 0, or -1 (no more sets / error)
void cutscene_play(const char *name); // 0x4361d0
void cutscene_start_special_cases(const char *name);   // 0x4365e0
void cutscene_cleanup();              // 0x436620 ends the running cutscene (interface_frame on Esc; decomp48)
void cutscene_end_special_cases(const char *name);     // 0x436a60
void cutscene_sequence_update();      // 0x436a90 (state_game_frame while GF_CUTSCENE)
void cutscene_events_frame();         // 0x436c00
void cutscene_shutdown_stub();        // 0x437120 empty (push ebp / pop ebp / ret; game_cleanup)
void cutscene_play_escape();          // 0x437130 (state_game_enter with GF_ESCAPE_CUTSCENE)
void cutscene_change_level_for(const char *name);      // 0x437400
void cmd_play_cutscene();             // 0x4374d0
void cmd_play_and_load_cutscene();    // 0x437590
void cmd_stop_cutscene();             // 0x437650

// ---- other units' data the file uses (units not known) ----------------------------------------------
extern bool Snow_enabled;             // 0x59c5e8 console "snow" (the snow unit 0x4946e0..)

#endif
