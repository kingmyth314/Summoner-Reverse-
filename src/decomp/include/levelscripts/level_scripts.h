// levelscripts/level_scripts.h - level script hooks, the level tables, shopkeepers and quests
// (levelscripts\scripts\level_scripts_common.cpp path references 0x4caff8-0x4cbe8d; levelscripts\
// level_scripts.cpp 0x4ceec0-0x4d6651, decomp27: the savegame object, events.tbl, the level table and the
// level-script helpers). One header for these units; file name from strings.tbl. Not original source.
// Layouts: symbols/types.h level_script_info, shop_item, shopkeeper, quest_info, script_object.
#ifndef LEVELSCRIPTS_LEVEL_SCRIPTS_H
#define LEVELSCRIPTS_LEVEL_SCRIPTS_H

#include "vsdk/vsdk.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"

struct dialogue_action;                              // levelscripts/conversation.h

// One level's script hooks in level_script_table (0x59f128, 51 records). The flag / int / float name
// hash lists and their counts are filled by events_parse_table, automap_layers by
// level_parse_automap_table; the table itself holds only the first nine fields (D703).
struct level_script_info {
    const char *name;                    // +0x00 level file name ("masad", "sewer", ...)
    void (*init)();                      // +0x04
    void (*frame)();                     // +0x08
    void (*leave)();                     // +0x0C
    char (*use)(const char *name);       // +0x10
    void *unused14;                      // +0x14 copied to level_script_unk14_fn, never called (IonaExt
                                         //        holds ls_test_use)
    void (*action)(dialogue_action *action);   // +0x18 conversation_execute_action kind 0 (D703)
    int load_steps;                      // +0x1C loading-screen progress total
    const char *music;                   // +0x20
    int num_flags;                       // +0x24 savegame flag bits
    unsigned int *flag_hashes;           // +0x28 (written by events_parse_table: not const, D703)
    int automap_layers;                  // +0x2C
    int num_ints;                        // +0x30
    unsigned int *int_hashes;            // +0x34
    int num_floats;                      // +0x38
    unsigned int *float_hashes;          // +0x3C
};
LAYOUT_CHECK(level_script_info, sizeof(level_script_info) == 0x40);
#define NUM_LEVEL_SCRIPTS 51
extern level_script_info level_script_table[NUM_LEVEL_SCRIPTS];   // 0x59f128
int name_hash_nocase(const char *name);               // 0x4d5f50 (savegame name hashes; Crc32 of the lower case)
bool cutscene_script_is_active();                    // 0x4ed770 (ai_do_frame skips ai_groups_frame; unit not known, decomp39)
extern bool level_is_town;                           // 0x60a40c set by level_script_parse_table ("#Town")
extern int cur_level_index;                          // 0x59f120 (4 = world map)
int  level_script_find(const char *level_name);      // 0x4d6300, -1 = none
// Script objects (level_scripts_common.cpp code; the 200 records are in level_scripts.cpp's .bss).
// 0x550 bytes (types.h script_object). One action (0x18 bytes, types.h script_action; decomp41, D2101):
// level_script_restart and script_object_receive_go read the type and write arg0c.
#define SCRIPT_ACTION_WAIT_GO   0x0b     // arg0c set by script_object_receive_go, cleared by the restart
#define SCRIPT_ACTION_LOOP      0x0d     // makes level_script_restart set end_mode 1
#define SCRIPT_ACTION_GOTO      0x0f     // script_action_goto (script_object_goto_label builds one)
struct script_action {
    int type;                            // +0x00
    const char *name;                    // +0x04 navpoint / file name argument
    int _unk08;                          // +0x08
    unsigned int arg0c;                  // +0x0C (the goto label; the "go" flag of type 0xb)
    unsigned int arg10;                  // +0x10
    ubyte flag14;                        // +0x14
    ubyte _pad15[3];
};
LAYOUT_CHECK(script_action, sizeof(script_action) == 0x18);
struct script_object {
    int end_mode;                        // +0x000 -1 = free (level_script_entries_frame)
    script_action actions[54];           // +0x004
    void *path;                          // +0x514
    GameTimer timer;                     // +0x518 (init_script_object_timers constructs the 200)
    int num_actions;                     // +0x51C
    int cur_action;                      // +0x520
    int handle;                          // +0x524 the scripted entity
    ubyte suspended;                     // +0x528
    ubyte hidden;                        // +0x529
    ubyte _pad52a[2];
    int trigger_level;                   // +0x52C
    char trigger[0x20];                  // +0x530
};
LAYOUT_CHECK(script_object, sizeof(script_object) == 0x550);
#define MAX_SCRIPT_OBJECTS 200
extern script_object script_objects[MAX_SCRIPT_OBJECTS];   // 0x24f6230
extern int script_object_count;                      // 0x25443a0
int  script_object_find(const char *name);           // 0x4cddf0 -1 = none
void script_object_resume(script_object *so);        // 0x4cddc0 (level_scripts_common.cpp)
bool script_object_is_type5(int obj);                // 0x4ce1e0 character type (+0xc) == 5
bool script_npc_near_player(const char *name, float dist);   // 0x4ce540
// level_scripts_common.cpp helpers the level script units call (decomp34, D1406). Argument lists from the
// call sites in level_script_khosani_lab.cpp / level_script_worldmap.cpp and the callees' prologues.
void level_find_player_starts();                     // 0x4caf40 "$player%d-%02d" -> level_player_starts
void script_play_cutscene(const char *name, void (*done)());   // 0x4ccf00 (return unused)
// A level trigger (name, script variant; NULL script = any): set_exit clears trigger +0x54 bit 2 (a live
// level exit), set_script_use sets it (level_check_triggers calls level_script_use instead).
void level_trigger_set_exit(const char *name, const char *script);         // 0x4ccf90
void level_trigger_set_script_use(const char *name, const char *script);   // 0x4ccff0
void script_object_say(int index, const char *text, float time);           // 0x4cdef0 (return unused)
void script_npc_set_disabled(const char *name, bool disabled);             // 0x4ce1c0
void script_npc_send_go(const char *name);           // 0x4ce320 (return unused)
bool script_player_near_navpoint(const char *navpoint, float dist);   // 0x4ce4d0 cur_character within dist
bool script_npc_is_dead(const char *name);           // 0x4ce600 false when not found
void script_party_gain_xp(int xp);                   // 0x4ceb00 L570 "party gained %d XP"
char level_script_use(const char *name);             // 0x4d5e20 the level's use handler, 0 without (D704)
int  level_script_find_index(int level, const char *script_name);   // 0x4d6340, -1 = none
void level_format_display_name(char *out, int level);// 0x4d63b0

// level_scripts.cpp helpers the conversation code calls (decomp20).
void script_object_suspend(int index);               // 0x4d58f0
void script_object_resume_index(int index);          // 0x4d5960
void conversation_execute_action(dialogue_action *action);   // 0x4d5990 (return value never used)
int  quest_find_by_name(const char *name);           // 0x4d5ff0, -1 = none
void quest_journal_set_entry(int quest, int value);  // 0x4d6040 (return value never used)

// The level table (0x10 bytes, level_table 0x59fde8).
struct level_table_entry {
    int id;                                          // +0x0 level_script_table index
    char **display_name;                             // +0x4 localize_strings slot
    unsigned int mp_flags;                           // +0x8 bit 0: listed on the level select screen
    int unk0c;                                       // +0xc
};
extern level_table_entry level_table[26];            // 0x59fde8
// The level_table index of a level id, or -1 (the "endgame" string after the table is only its end).
int  level_table_find_id(int level);                 // 0x4d6390 (formerly level_is_endgame)
char *level_table_display_name(int level);           // 0x4d64c0 NULL when unknown
void script_filenames_parse_table();                 // 0x4d6100 script_filenames.tbl (GameInit)

// {level file name, localized display name slot} (type name ours).
struct level_display_name {
    const char *name;
    char **text;
};
extern level_display_name level_display_names[51];   // 0x594440-0x5945d7 (D603)

// ---- shops (level_parse_shopkeeper 0x4cc4d0) ---------------------------------------------------------
struct shop_item {                   // 8 bytes, level_shop_items 0x2491ac0
    int item_type;                   // +0x0 item_types index
    int price;                       // +0x4
};
struct shopkeeper {                  // 0x14 bytes, level_shopkeepers 0x2490ca0
    const char *name;                // +0x00 the character's name
    int num_sell;                    // +0x04 +Sell List: entries
    shop_item *sell;                 // +0x08
    int num_buy;                     // +0x0C +Buy List: entries
    shop_item *buy;                  // +0x10
};
shopkeeper *level_shopkeeper_find(const char *name); // 0x4ce950

// ---- quests (events_parse_table 0x4d39c0) --------------------------------------------------------------
struct quest_info {                  // 0x6c bytes at 0x253ef40
    const char *name;                // +0x00 $Quest: name
    ubyte side_quest;                // +0x04 1 = side quest
    ubyte _pad05[3];
    struct {                         // +0x08 one per +Stage: / +Invalidate: line
        const char *text;            //   +Stage: text (0 for an +Invalidate: entry)
        unsigned int flag_hash;      //   +Flag: / +Invalidate: name hash, 0 none
    } steps[12];
    ubyte num_steps;                 // +0x68
    signed char invalidate_step;     // +0x69 -1 none
    ubyte _pad6a[2];
};
LAYOUT_CHECK(quest_info, sizeof(quest_info) == 0x6c);
#define MAX_QUESTS 200
extern quest_info quest_table[MAX_QUESTS];           // 0x253ef40
extern int quest_count;                              // 0x25443a4
int quest_get_current_step(int quest);               // 0x4d6080, -1 = not active

// ---- level containers and exits (level_scripts_common.cpp .bss 0x248ce40-0x2493a08) ------------------
// lc_entries: 50 records of 0x10c bytes (lc_entry_add). sum.symbols tags them new_hud.cpp, but the
// address is in the level scripts' .bss (see game/NOTES-hud.md). Type name ours.
struct lobj;
struct lc_entry {
    lobj *object;                    // +0x000
    char name[0x100];                // +0x004
    ubyte flags;                     // +0x104 bit 0 used (lc_entry_mark_used; savegame_read_two_stage
                                     //        sets it from the saved bit 0, "disabled"). A byte: every
                                     //        access is a byte op (D2100)
    ubyte _pad105[3];
    int gold;                        // +0x108 +Give:
};
LAYOUT_CHECK(lc_entry, sizeof(lc_entry) == 0x10c);
extern lc_entry lc_entries[50];                      // 0x248ce40
extern int lc_entry_count;                           // 0x2491a68
int  lc_find_by_hash(unsigned int hash);             // 0x4cd570 (level_script_name + name), -1 = none
void lc_entry_mark_used(int index);                  // 0x4cd4e0 (decomp35)
int  lc_find_by_name(const char *name);              // 0x4cd520 -1 = none
// level_check_triggers' state. sum.symbols types level_exit_trigger void* (the trigger record); new_hud.cpp
// passes it to level_script_find, so it is the record's leading name here.
extern char *level_exit_trigger;                     // 0x24902c4
extern unsigned int level_exit_block_flags;          // 0x2491a50 (1 party too far, 2 hostiles near)
extern bool game_frozen;                             // 0x2491a5c (GameTick zeroes reported_frametime)
extern vector3 level_exit_return_pos;                // 0x2493a08

void goto_credits();                                 // 0x4ef6a0 (level_script_templeint2.cpp range)

// ---- level_scripts_common.cpp state read by level_scripts.cpp (decomp27) -------------------------------
// The current level's hooks, copied from level_script_table by level_script_select.
extern void (*level_script_init_fn)();               // 0x249029c
extern void (*level_script_frame_fn)();              // 0x24902ac
extern void (*level_script_leave_fn)();              // 0x2491a54
extern char (*level_script_use_fn)(const char *name);// 0x2491a60
extern void *level_script_unk14_fn;                  // 0x2491a64 (+0x14; name ours, never called)
extern void (*level_script_action_fn)(dialogue_action *action);   // 0x2490c5c
extern int level_music;                              // 0x24902b4
// World effects of the level (0x28 bytes; level_create_world_effects). Type name ours.
struct level_effect {
    char name[0x20];                 // +0x00 effect file
    int handle;                      // +0x20 fx_create_world result
    bool flag;                       // +0x24 passed to fx_create_world
    ubyte _pad25[3];
};
LAYOUT_CHECK(level_effect, sizeof(level_effect) == 0x28);
#define MAX_LEVEL_EFFECTS 10
extern level_effect level_effects[MAX_LEVEL_EFFECTS];// 0x24902c8 (to 0x2490458)
extern int level_effect_count;                       // 0x2490dcc
// Ambient sound emitters (0x10 bytes {emitter, vector3 pos}; level_ambient_add). Type name ours.
struct level_ambient {
    int emitter;                     // +0x0
    vector3 pos;                     // +0x4
};
extern level_ambient level_ambients[0x80];           // 0x2490458
extern int level_ambient_count;                      // 0x2491a58
void level_script_reparse();                         // 0x4cb3a0
void party_set_level(int level);                     // 0x4cec00
void script_party_acquire_item(const char *name, int count);   // 0x4ceab0
int  party_count_item_by_name(const char *name);     // 0x4ce9a0

// ---- level_scripts_common.cpp (0x4ca700-0x4ceebf, decomp41) ---------------------------------------------
// A level trigger (0x64 bytes, level_triggers 0x2490dd0, 32; level_parse_trigger). Type and field names
// ours. sum.symbols calls +0x50 level_exit_trigger_index when copied; it is the +Index: value (the entry
// point of the next level, D2104).
#define LEVEL_TRIGGER_ON         0x1     // set by level_parse_trigger; radius triggers trip only while set
#define LEVEL_TRIGGER_SCRIPT_USE 0x2     // level_script_use(ref_name) instead of a level exit
#define LEVEL_TRIGGER_MULTI      0x4     // +Multi: multiplayer only (single player skips it)
#define LEVEL_TRIGGER_TRIPPED    0x80000000   // level_trigger_trip: an +ON radius trigger fired once
struct s3d_named_path;
struct navpoint;
struct level_trigger {
    char name[0x20];                 // +0x00 $Trigger: the level file it leads to
    const char *script;              // +0x20 the script name of +Script: (conversation_save_table), "" none
    char ref_name[0x20];             // +0x24 +RefName: (default the name), level_script_use argument
    int _unk44;                      // +0x44
    int id;                          // +0x48 +Id: 0 load level, 1 boss, 2 sound (level_trigger_id_names)
    int type;                        // +0x4c +Type: 0 spline (a named path), 1 location (a radius)
    int index;                       // +0x50 +Index: (0) -> level_entry_point of the next level
    unsigned int flags;              // +0x54 LEVEL_TRIGGER_*
    int plane;                       // +0x58 +Plane: layer, -1 any
    union {                          // +0x5c
        s3d_named_path *path;        //   type 0: level_trigger_record_find (+Spline name:)
        navpoint *nav;               //   type 1: navpoint_find (+Location:)
    };
    float radius;                    // +0x60 +Radius:
};
LAYOUT_CHECK(level_trigger, sizeof(level_trigger) == 0x64);
#define MAX_LEVEL_TRIGGERS 32
extern level_trigger level_triggers[MAX_LEVEL_TRIGGERS];   // 0x2490dd0
extern int level_trigger_count;                      // 0x2490c74
extern const char *level_trigger_type_names[2];      // 0x59e0e8 "spline", "location" (name ours)
extern const char *level_trigger_id_names[3];        // 0x59e0f0 "load level", "boss", "sound" (name ours)
extern const char *level_pending_exit_name;          // 0x24902a4 trigger +0x20 (the next script name)
extern int level_exit_trigger_index;                 // 0x2490c60 trigger +0x50 (+Index:)
extern int level_unk_2490c78;                        // 0x2490c78 zeroed by level_script_reparse, never read (name ours)
// Shops.
#define MAX_LEVEL_SHOPKEEPERS 15
#define MAX_LEVEL_SHOP_ITEMS 1000
extern shopkeeper level_shopkeepers[MAX_LEVEL_SHOPKEEPERS];  // 0x2490ca0
extern int level_shopkeeper_count;                   // 0x24902a0
extern shop_item level_shop_items[MAX_LEVEL_SHOP_ITEMS];     // 0x2491ac0
extern int level_shop_item_count;                    // 0x2493a00
// The fog distances of "+Fog Near:" / "+Fog Far:".
extern float level_fog_near;                         // 0x2490298
extern float level_fog_far;                          // 0x2490c58
// Music restart timer, cutscene hook.
extern GameTimer level_music_stop_timer;             // 0x24902a8 (900 ms after level_script_reparse)
extern void (*cutscene_done_callback)();             // 0x24902b0
extern void (*cutscene_saved_frame_fn)();            // 0x2493a04
// Avoid splines of the scripts: {character handle, spline} pairs, key -1 = free. Type name ours.
struct script_avoid_spline {
    int key;                         // +0x0 handle of the character the spline belongs to
    int spline;                      // +0x4 avoid_spline index (script_build_avoid_spline)
};
#define MAX_SCRIPT_AVOID_SPLINES 10
extern script_avoid_spline script_avoid_splines[MAX_SCRIPT_AVOID_SPLINES];   // 0x2491a70
// Console toggles and state flags.
extern bool level_script_restarting;                 // 0x2493a14
extern bool Dont_load_fx;                            // 0x2493a15 console "dont_load_fx"
extern bool No_cutscenes;                            // 0x2493a16 console "No_cutscenes"
extern bool Do_frame_callback;                       // 0x59e0fc console "do_frame_callback" (1)

class TextFile;
class Character;
class obj;
void cmd_dont_load_fx();                             // 0x4ca790
void cmd_do_frame_callback();                        // 0x4ca880
void cmd_eax_save_props();                           // 0x4ca970 saves <level>.eax
void cmd_No_cutscenes();                             // 0x4cce40
void level_check_triggers();                         // 0x4caa00 (level_script_frame)
// (trigger, previous position, position): the position on the left of the trigger's path (x / z); the
// previous position is not read.
int  level_trigger_point_left_of_line(level_trigger *t, const vector3 *prev, const vector3 *cur);   // 0x4caed0
void script_place_party_at_starts();                 // 0x4cb010
void level_script_restart();                         // 0x4cb060
void level_script_reload();                          // 0x4cb350 (script editor)
void level_items_parse_table();                      // 0x4cb430 level_items.tbl
void level_parse_table();                            // 0x4cb7b0 <level>.tbl
void level_parse_ambient(TextFile *tf);              // 0x4cc010 #Ambient Sounds
void level_parse_trigger(TextFile *tf);              // 0x4cc190 #Triggers
void level_parse_shopkeeper(TextFile *tf);           // 0x4cc4d0 #Shopkeepers
// state_game_enter passes !loaded: set the monster levels from the party (single player) or the host
// level (multiplayer) only on a fresh entry (D2105).
void level_script_init(bool scale_levels);           // 0x4cc7e0
void level_script_frame();                           // 0x4ccab0 (state_game_frame)
void level_ambient_sounds_frame();                   // 0x4ccb40
void level_script_entries_frame();                   // 0x4ccba0
bool script_character_has_pending_goal(Character *c);// 0x4ccc50
void level_script_leave();                           // 0x4ccc70 (state_game_leave)
void level_apply_pending_change();                   // 0x4ccd10
void script_cutscene_done();                         // 0x4ccdf0 the frame hook while a cutscene plays
void level_trigger_set_exit_by_refname(const char *ref_name);   // 0x4cd060 clears LEVEL_TRIGGER_SCRIPT_USE
void level_trigger_set_script_use_by_refname(const char *ref_name);    // 0x4cd0c0 sets it
// (object, out id, out index): the first non-script trigger of the object's move (-1, -1 when none);
// "Tripped trigger:  %s" goes only to string_buffer. ls_catacombs_frame passes cur_character.
void level_trigger_trip(obj *o, int *out_id, int *out_index);  // 0x4cd120
void level_ambient_add(int sound, const vector3 *pos, int delay_ms);   // 0x4cd250
void level_ambient_remove(int index);                // 0x4cd2c0 (compacts the table)
int  level_ambient_find_at(const vector3 *pos);      // 0x4cd330 exact compare, -1 none
// lc entries are keyed by their level object (sum.symbols says hash; D2106).
void lc_entry_add(lobj *object);                     // 0x4cd390
void lc_entry_clear(lobj *object);                   // 0x4cd470
bool lc_entry_exists(lobj *object);                  // 0x4cd4b0
int  level_script_parse_table();                     // 0x4cd5f0 <script>_script.tbl, TextFile::Open's error
Character *script_object_find_character(const char *name);   // 0x4cde70 NULL when not found
void script_party_member_say(const char *name, const char *text, float time);   // 0x4cdf30
void script_party_remove_item(const char *name);     // 0x4cdfa0 one off the stack
void script_eleh_strip_party_gear(bool strip);       // 0x4cdff0
void script_object_set_disabled(int index, bool disabled);   // 0x4ce140
void script_object_receive_go(int index);            // 0x4ce220
void script_object_goto_label(int index, int label); // 0x4ce340
void script_npc_goto_label(const char *name, int label);     // 0x4ce3a0
bool script_check_navpoint(int index, const char *navpoint, float dist);   // 0x4ce3c0
bool script_object_near_player(int index, float dist);       // 0x4ce460
bool script_object_is_dead(int index);               // 0x4ce570 true without an object
void script_object_drop_loot(int index);             // 0x4ce5d0
void script_npc_drop_loot(const char *name);         // 0x4ce620
void script_object_talk(int index);                  // 0x4ce640
void script_npc_talk(const char *name);              // 0x4ce6d0
// (guard, minimum cosine of the view cone): cur_character in range, in the cone and not hidden from it.
bool script_guard_sees_player(Character *guard, float cone);   // 0x4ce6f0
bool script_player_has_equipped(const char *item);   // 0x4cea70
// (item, speaker, topic): the topic is available when the party lacks the item (_inv: when it has it).
void script_topic_toggle_by_item(const char *item, const char *speaker, const char *topic);       // 0x4ceb80
void script_topic_toggle_by_item_inv(const char *item, const char *speaker, const char *topic);   // 0x4cebc0
int  script_avoid_spline_find_free();                // 0x4ced90 -1 none
void script_avoid_splines_reset();                   // 0x4cedb0
bool script_avoid_spline_in_use(int key);            // 0x4cedd0
void script_alloc_avoid_spline(int handle, int target);      // 0x4cedf0 (target -1 = cur_character)
void script_free_avoid_spline(int handle);           // 0x4cee90

// ---- level_scripts.cpp (0x4ceec0-0x4d6651, decomp27) ---------------------------------------------------
// A level item record (0x70 bytes; level_items_parse_table 0x4cb430). Type and field names ours.
struct ItemType;
struct level_item_rec {
    char name[0x20];                 // +0x00 $Name: (name_hash_nocase in the level item list)
    char script[0x20];               // +0x20 $Script: the level
    ItemType *type;                  // +0x40 $Item:
    int count;                       // +0x44 +Count: (1)
    struct navpoint *navpoint;       // +0x48 $Navpoint: navpoint_find (a record pointer, D2102)
    int flag_level;                  // +0x4C +Event: level, -1 none
    char flag[0x20];                 // +0x50 +Event: flag name
};
LAYOUT_CHECK(level_item_rec, sizeof(level_item_rec) == 0x70);
#define MAX_LEVEL_ITEMS 32
extern level_item_rec level_items[MAX_LEVEL_ITEMS];  // 0x24e3b50 (sum.symbols: items.cpp, D705)
extern int level_item_count;                         // 0x2539f10
// A navpoint (0x30 bytes; level_add_navpoint). sum.symbols calls +0x10 a handle; the code stores the
// heading there. Type and field names ours.
struct navpoint {
    char name[0x10];                 // +0x00
    float heading;                   // +0x10 matrix3::extract_angles h of the level-file navpoint
    vector3 pos;                     // +0x14 dropped onto the geometry below
    int layer;                       // +0x20 lobj_get_layer_n of the floor, 0 when invalid
    int _unk24;                      // +0x24 zeroed
    int _unk28;                      // +0x28 level_script_restart only moves characters whose start
                                     //        navpoint has 0 here (decomp41)
    int level_item;                  // +0x2c level_items index placed here (level_items_parse_table; D2102)
};
LAYOUT_CHECK(navpoint, sizeof(navpoint) == 0x30);
#define MAX_NAVPOINTS 750
extern navpoint navpoints[MAX_NAVPOINTS];            // 0x24e4950
extern navpoint *level_player_starts[4];             // 0x60a32c (level_find_player_starts; decomp37)
extern int navpoint_count;                           // 0x2539f2c
// conversation_save_table (levelscripts/conversation.h) as records: one per (level, slot).
struct conversation_save_info {
    const char *name;                // +0x0 +Script: (script_filenames.tbl), NULL = none
    int num_speakers;                // +0x4 +Dialogue: first int
    int num_topics;                  // +0x8 +Dialogue: second int
    int level_cap;                   // +0xC +Levelcap: (100)
};
#define CONVERSATION_SAVE_INFO(level, slot) \
    ((conversation_save_info *)&conversation_save_table[((level) * 7 + (slot)) * 4])

extern bool Preparse_scripts;                        // 0x24e3b48 console toggle (not read here)
extern bool flece_joined;                            // 0x24e3b40
extern bool jekhar_joined;                           // 0x24e3b41
extern bool rosalind_joined;                         // 0x24e3b42
extern bool level_script_event_active;               // 0x24e3b43 saving is refused while set
extern unsigned int Script_filenames_tbl_checksum;   // 0x25443ac header +0x1e4, not checked on load
extern unsigned int events_tbl_checksum;             // 0x24e3b4c (name ours; never read)
extern int events_int_pool[0x1000];                  // 0x24dfb3c the hash lists of level_script_table
extern int events_int_pool_offset;                   // 0x24e3b44 bytes used
class StringPool;
extern StringPool events_string_pool;                // 0x24f6218 events.tbl quest names and stage texts
extern StringPool script_filenames_string_pool;      // 0x24ed600 script_filenames.tbl names
extern StringPool level_script_stringpool;           // 0x2539f18 (script_editor_store_string)

void cmd_Preparse_scripts();                         // 0x4cef30
void events_parse_table();                           // 0x4d39c0
void level_parse_automap_table();                    // 0x4d4020
void level_script_event_flag_clear();                // 0x4d4150
void game_stage_set(int stage);                      // 0x4d42e0 (0..22)
int  game_stage_get();                               // 0x4d47b0
void cmd_gamestage();                                // 0x4d4bf0
void cmd_savegame();                                 // 0x4d4c80
void cmd_loadgame();                                 // 0x4d4ea0
void level_create_world_effects();                   // 0x4d54e0
void level_script_select();                          // 0x4d5570
int  level_script_load_steps(const char *level_file);// 0x4d56d0 (10000 when not found)
void navpoints_reset();                              // 0x4d58e0
void cmd_door_open();                                // 0x4d5e70
int  level_script_flag_hash_known(unsigned int hash);// 0x4d5ed0
int  level_script_int_hash_known(unsigned int hash); // 0x4d5f10
void level_ambient_pause_all();                      // 0x4d64f0
void level_ambient_unpause_all();                    // 0x4d6520
void cmd_door_close();                               // 0x4d6580
void cmd_door_toggle();                              // 0x4d6600

// ---- the level script hooks in level_script_table (one unit per level, level_script_<name>.cpp) ---------
// Names from sum.symbols; several records share a folded copy (ls_test_use, empty_function).
void ls_sewer_init();                                       // 0x4ed7e0
void ls_sewerboss_init();                                   // 0x4edc30
void ls_sewerboss_leave();                                  // 0x4edd30
void ls_sewer_frame();                                      // 0x4edd50
void ls_sewerboss_frame();                                  // 0x4ee0e0
char ls_sewer_use(const char *name);                        // 0x4ee4f0
void ls_sewer_action(dialogue_action *action);              // 0x4ee580
void ls_lenele2d_init();                                    // 0x4ee730
void ls_lenele2d_frame();                                   // 0x4ee760
void ls_lenele2d_action(dialogue_action *action);           // 0x4ee7d0
char ls_lenele2d_use(const char *name);                     // 0x4ee800
void ls_rand_iceland01_init();                              // 0x4ee840
void ls_rand_grasslandnite01_init();                        // 0x4ee860
void ls_rand_desertnite01_init();                           // 0x4ee880
void ls_rand_orenianite01_init();                           // 0x4ee8a0
void ls_rand_orenia01_init();                               // 0x4ee8f0
void ls_rand_grassland01_init();                            // 0x4ee940
void ls_rand_grassland01_action(dialogue_action *action);   // 0x4eed80
void ls_rand_hillsnite01_init();                            // 0x4eee20
void ls_ikaemosbottomint2_init();                           // 0x4eee60
void ls_ikaemosbottomint2_frame();                          // 0x4eeef0
void ls_ikaemosext2_init();                                 // 0x4ef120
void ls_ikaemosext2_frame();                                // 0x4ef140
void ls_templeint2_init();                                  // 0x4ef190
void ls_templeint2_frame();                                 // 0x4ef1e0
char ls_templeint2_use(const char *name);                   // 0x4ef770
void ls_lenele1ab_init();                                   // 0x4ef850
void ls_lenele1aa_init();                                   // 0x4efa30
void ls_lenele1aa_frame();                                  // 0x4efdb0
char ls_lenele1aa_use(const char *name);                    // 0x4efef0
void ls_lenele1aa_action(dialogue_action *action);          // 0x4eff30
void ls_jadetemple_init();                                  // 0x4effe0
void ls_jadetemple_frame();                                 // 0x4f0110
void ls_jadetemple_action(dialogue_action *action);         // 0x4f0a40
void ls_lenele3d_init();                                    // 0x4f0da0
void ls_lenele3d_frame();                                   // 0x4f0eb0
void ls_ionaext02_leave();                                  // 0x4f0f00
char ls_lenele3d_use(const char *name);                     // 0x4f0f10
void ls_lenele3a_init();                                    // 0x4f0f70
void ls_lenele3a_leave();                                   // 0x4f1000
char ls_lenele3a_use(const char *name);                     // 0x4f1030
void ls_lenele2ab_init();                                   // 0x4f1070
void ls_lenele2ab_frame();                                  // 0x4f10a0
void ls_lenele2ab_action(dialogue_action *action);          // 0x4f1220
void ls_lenele2aa_init();                                   // 0x4f12c0
void ls_lenele2aa_frame();                                  // 0x4f1310
void ls_lenele2aa_action(dialogue_action *action);          // 0x4f13f0
char ls_lenele2aa_use(const char *name);                    // 0x4f1420
void ls_eleh_init();                                        // 0x4f1490
void ls_eleh_frame();                                       // 0x4f1560
void ls_eleh_action(dialogue_action *action);               // 0x4f1890
char ls_eleh_use(const char *name);                         // 0x4f1910
void ls_wolong2_init();                                     // 0x4f2060
void ls_wolong2_frame();                                    // 0x4f2140
void ls_wolong2_action(dialogue_action *action);            // 0x4f21e0
char ls_wolong2_use(const char *name);                      // 0x4f21f0
void ls_khosanilab2_init();                                 // 0x4f2250
void ls_khosanilab2_frame();                                // 0x4f22a0
void ls_lpalaceint02_init();                                // 0x4f2450
void ls_lpalaceint02_frame();                               // 0x4f2550
void ls_lpalaceint02_leave();                               // 0x4f27c0
char ls_lpalaceint02_use(const char *name);                 // 0x4f27f0
void ls_tancredhouse_init();                                // 0x4f2830
void ls_tancredhouse_action(dialogue_action *action);       // 0x4f2930
char ls_test_use(const char *name);                         // 0x4f29c0
void ls_lpalaceint_init();                                  // 0x4f2ac0
void ls_lpalaceint_frame();                                 // 0x4f2ce0
void ls_lpalaceint_action(dialogue_action *action);         // 0x4f3890
char ls_lpalaceint_use(const char *name);                   // 0x4f3a40
void ls_khosanistrng_init();                                // 0x4f3a80
void ls_khosanistrng_frame();                               // 0x4f3ee0
void ls_khosanistrng_action(dialogue_action *action);       // 0x4f4210
void ls_ionaext02_init();                                   // 0x4f4570
void ls_ionaext02_frame();                                  // 0x4f4650
void ls_rand_desert_init();                                 // 0x4f4910
void ls_liangshan_init();                                   // 0x4f4930
void ls_liangshan_frame();                                  // 0x4f4a20
void ls_liangshan_action(dialogue_action *action);          // 0x4f54f0
char ls_liangshan_use(const char *name);                    // 0x4f5630
void ls_rand_forestnite1_init();                            // 0x4f5670
void ls_rand_forestnite1_frame();                           // 0x4f56e0
void ls_rand_forest01_init();                               // 0x4f5910
void ls_rand_forest01_frame();                              // 0x4f5b90
void ls_rand_forest01_action(dialogue_action *action);      // 0x4f5e70
void ls_templeint_init();                                   // 0x4f5e90
void ls_templeint_frame();                                  // 0x4f5f80
void ls_templeint_action(dialogue_action *action);          // 0x4f64f0
char ls_templeint_use(const char *name);                    // 0x4f65e0
void ls_wolongcaverns_init();                               // 0x4f66c0
void ls_wolongcaverns_frame();                              // 0x4f68f0
void ls_wolongcaverns_action(dialogue_action *action);      // 0x4f6f80
char ls_wolongcaverns_use(const char *name);                // 0x4f6ff0
void ls_ionaext_init();                                     // 0x4f7220
void ls_ionaext_frame();                                    // 0x4f7270
void ls_ionaext_action(dialogue_action *action);            // 0x4f7760
void ls_khosanilab_init();                                  // 0x4f8920
void ls_khosanilab_frame();                                 // 0x4f8a30
void ls_khosanilab_leave();                                 // 0x4f9580
char ls_khosanilab_use(const char *name);                   // 0x4f95a0
void ls_ikaemosext_init();                                  // 0x4f9e50
void ls_ikaemosext_frame();                                 // 0x4f9e70
void ls_ikaemostopint_init();                               // 0x4f9ec0
void ls_ikaemostopint_frame();                              // 0x4f9f50
char ls_ikaemostopint_use(const char *name);                // 0x4fa230
void ls_ikaemosbottomint_init();                            // 0x4fa400
void ls_rand_hills01_init();                                // 0x4fa420
void ls_rand_hills01_frame();                               // 0x4fa4a0
void ls_rand_hills01_action(dialogue_action *action);       // 0x4fa780
void ls_lenele1e_init();                                    // 0x4fa7c0
char ls_lenele1e_use(const char *name);                     // 0x4fa850
void ls_lenele1d_init();                                    // 0x4fa890
void ls_lenele1d_frame();                                   // 0x4fa970
char ls_lenele1d_use(const char *name);                     // 0x4fa9d0
void ls_lenele1c_init();                                    // 0x4faa60
void ls_lenele1c_frame();                                   // 0x4fab80
char ls_lenele1c_use(const char *name);                     // 0x4fad50
void ls_lenele1b_init();                                    // 0x4fad90
void ls_lenele1b_frame();                                   // 0x4faf90
char ls_lenele1b_use(const char *name);                     // 0x4fb0c0
void ls_lenele1b_action(dialogue_action *action);           // 0x4fb160
void ls_worldmap1_init();                                   // 0x4fb510
void ls_worldmap1_frame();                                  // 0x4fbd80
void ls_worldmap1_leave();                                  // 0x4fd560
void ls_masad_init();                                       // 0x4fd750
void ls_masad_frame();                                      // 0x4fd980
void ls_masad_leave();                                      // 0x4fdc70
char ls_masad_use(const char *name);                        // 0x4fdc90
void ls_masad_action(dialogue_action *action);              // 0x4fdd60
void ls_test_init();                                        // 0x4fdde0
void ls_test_frame();                                       // 0x4fde20
void ls_test_leave();                                       // 0x4fde60
void ls_catacombs_init();                                   // 0x4fde80
void ls_catacombs_frame();                                  // 0x4fe1c0
char ls_catacombs_use(const char *name);                    // 0x4fe5e0
void ls_wolong_init();                                      // 0x4fe8f0
void ls_wolong_frame();                                     // 0x4fe9a0
char ls_wolong_use(const char *name);                       // 0x4fea40
void ls_wolong_action(dialogue_action *action);             // 0x4fea80
// A level-file navpoint ("$npc", "$hostile", "$player") into the level marker records (level_scripts.cpp;
// decomp28: level_load_s3d passes the s3d record).
struct s3d_navpoint;
bool level_add_navpoint(s3d_navpoint *np);           // 0x4d5720 always true (mov al, 1; D710)

// ---- the level hooks gameplay.cpp calls (decomp42) ---------------------------------------------------
void cutscene_script_entries_frame();                 // 0x4ed680

#endif
