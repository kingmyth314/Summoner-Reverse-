// levelscripts/script_internal.h - levelscripts\script_internal.cpp (0x4db340-0x4de0a3, assert path
// 0x5a0670; written in decomp44): the navpoint and #Objects parsers of the level and cutscene scripts,
// the script actions, and the script entry state machine. Not original source.
#ifndef LEVELSCRIPTS_SCRIPT_INTERNAL_H
#define LEVELSCRIPTS_SCRIPT_INTERNAL_H

#include "vsdk/vsdk.h"

class vector3;
class matrix;
class TextFile;
class Character;
struct navpoint;
struct script_action;
struct script_object;
struct item_info;

// Action types: the script_action_names index (+Action: of #Objects). SCRIPT_ACTION_WAIT_GO (0x0b),
// SCRIPT_ACTION_LOOP (0x0d, the "die" action) and SCRIPT_ACTION_GOTO (0x0f) are in level_scripts.h.
#define SCRIPT_ACTION_WALK_TO          0x00
#define SCRIPT_ACTION_RUN_TO           0x01
#define SCRIPT_ACTION_MOVE_THROUGH     0x02   // appends to the path of the walk that follows
#define SCRIPT_ACTION_WAIT_AT          0x03
#define SCRIPT_ACTION_WANDER           0x04
#define SCRIPT_ACTION_WANDER_FOR       0x05
#define SCRIPT_ACTION_DO_STATE         0x06
#define SCRIPT_ACTION_PLAY_ACTION      0x07
#define SCRIPT_ACTION_PLAY_MVF         0x08
#define SCRIPT_ACTION_PLAY_EFFECT      0x09
#define SCRIPT_ACTION_TURN_HOSTILE     0x0a
#define SCRIPT_ACTION_SAY              0x0c
#define SCRIPT_ACTION_DIE              0x0d   // = SCRIPT_ACTION_LOOP
#define SCRIPT_ACTION_LABEL            0x0e
#define SCRIPT_ACTION_NO_BLOCK         0x10
#define SCRIPT_ACTION_TRIGGER          0x11
#define SCRIPT_ACTION_TRIGGER_NAV      0x12
#define SCRIPT_ACTION_TRIGGER_VISIT    0x13
#define SCRIPT_ACTION_TELEPORT_TO      0x14
#define SCRIPT_ACTION_PASS_GO          0x15
#define SCRIPT_ACTION_SHOW_HIDE        0x16
#define SCRIPT_ACTION_SYNCH            0x17
#define SCRIPT_ACTION_SYNCH_TELEPORT   0x18
#define SCRIPT_ACTION_FOLLOW           0x19
#define SCRIPT_ACTION_COMBAT_MODE      0x1a
#define SCRIPT_ACTION_MOUNT_ANIM       0x1b
#define SCRIPT_ACTION_DISMOUNT_ANIM    0x1c
#define SCRIPT_ACTION_REPLACE_WEAPON   0x1d
#define SCRIPT_ACTION_GUARD_SPLINE     0x1e
#define SCRIPT_ACTION_REPLACE_ARMOR    0x1f
#define SCRIPT_ACTION_ONE_HAND         0x20
#define NUM_SCRIPT_ACTIONS             0x21

// Navpoint types (navpoint::type; script_navpoint_type_names).
#define NAVPOINT_TYPE_LEVEL      0
#define NAVPOINT_TYPE_CUTSCENE   1
#define NAVPOINT_TYPE_CAMERA     2
#define NAVPOINT_TYPE_AMBIENT    3

extern const char *script_action_names[NUM_SCRIPT_ACTIONS];   // 0x5a045c "walk to" ... "one hand"
extern const char *script_navpoint_type_names[4];             // 0x5a04e0 "level", "cutscene", "camera", "ambient sound"

// The items the cutscene replace weapon / armor actions took off (restored by cutscene_script_rewind;
// 0xc bytes, 16 records). Type name ours; sum.symbols types the array int[48].
struct cutscene_saved_item {
    Character *c;                     // +0x0
    item_info *item;                  // +0x4 NULL: the slot was empty (restore by unequipping)
    int slot;                         // +0x8 equipment slot (5 weapon, 6 / 9 the other hands, 7 / 8 armour)
};
#define MAX_CUTSCENE_SAVED_ITEMS 16
extern cutscene_saved_item cutscene_saved_equipment[MAX_CUTSCENE_SAVED_ITEMS];   // 0x2546760
extern int cutscene_saved_equipment_count;                    // 0x2546820

// (char_info index, character type, start navpoint, level): creates the character at the navpoint, or
// moves the living entity that already has it as its start navpoint there; returns the handle.
int  script_object_spawn_at_navpoint(int char_info, int type, navpoint *np, int level);   // 0x4db340
// One $Name: record of a cutscene script's #Navpoints into np (cutscene_script_load); DebugError when it
// cannot be dropped onto the ground.
void script_parse_navpoint(navpoint *np, TextFile *tf);        // 0x4db420

// (name, out position, out orientation): _stricmp over the level navpoints (0x24e4950, count 0x2539f2c)
// and the script navpoints (0x256b750, count 0x257e96c); false when not found (xor al, al at 0x4dc6db).
bool navpoint_get_position(const char *name, vector3 *pos, matrix *orient);   // 0x4dc600

// decomp41 (level_scripts_common.cpp). Argument lists from its call sites and the callees' prologues.
// The same two searches, the record: the level navpoint, else the script (cutscene) navpoint, else NULL.
navpoint *navpoint_find(const char *name);           // 0x4dc570
void script_parse_navpoints(TextFile *tf);           // 0x4db700 one $Name: record of #Navpoints
// (tf, level): true for a level script (script_objects), false for a cutscene script
// (cutscene_script_objects); read as a byte (D2305).
void script_parse_objects(TextFile *tf, bool level); // 0x4dba30 one $Name: record of #Objects
void script_object_copy_from_template(Character *tmpl, Character *c);   // 0x4dc4d0
// (character, action, its object): action 15, sets the object's cur_action to the label (+0x0c);
// false when the label is missing (D2304: script_entry_start reads AL).
bool script_action_goto(Character *c, script_action *a, script_object *so);   // 0x4dca10
// decomp44: the other actions (true = started, false = retry in 5 s).
bool script_action_walk_to(Character *c, script_object *so);   // 0x4dc6e0 actions 0-2
bool script_action_wait_at(Character *c, script_object *so);   // 0x4dc860
bool script_action_do_state(Character *c, script_action *a);   // 0x4dc900
bool script_action_play_action(Character *c, script_action *a, bool hold);   // 0x4dc930 actions 7-8
bool script_action_die(Character *c, script_action *a);        // 0x4dc990
bool script_action_wander(Character *c, script_action *a, bool walk);        // 0x4dca90 actions 4-5
bool script_action_follow(script_object *so, Character *c, const char *leader, int seconds);   // 0x4dcb10
bool script_action_combat_mode(script_action *a, script_object *so, Character *c);   // 0x4dcbc0
bool script_action_replace_weapon(script_action *a, Character *c);   // 0x4dccb0
bool script_action_replace_armor(script_action *a, Character *c);    // 0x4dce60
void script_entry_start(script_object *so, Character *c);       // 0x4dd0b0 (return unused)
void script_entry_advance(script_object *so, Character *c);     // 0x4dd720 (return unused)
bool script_entry_check_done(script_object *so, Character *c);  // 0x4dd780 the current action completed
bool script_object_near_player_dist(Character *c, float dist);  // 0x4dda30 cur_character within dist
bool script_navpoint_near_player_dist(navpoint *np, float dist);// 0x4dda90
bool script_check_visit_count(int count);            // 0x4ddaf0 this level visited at least count times
void script_entry_jump_to_sequence(script_object *so, Character *c);   // 0x4ddb10 (cutscene sequences)
int  script_build_avoid_spline(Character *c, Character *target);// 0x4ddc60 avoid spline index, -1 failed
// (out, base, suffix): base cut to 32 - strlen(suffix) characters, then suffix (unit not known, after
// conversationctl.cpp; possibly the head of script_internal.cpp, decomp44 NOTES).
void str_copy_append_32(char *out, const char *base, const char *suffix);   // 0x4db250
// (name): true when name ends with "_cscript.tbl" (same unit question as str_copy_append_32; decomp55).
bool filename_has_cscript_suffix(const char *name);  // 0x4db2c0
// The script editor's unsaved-changes flags (cleared by level_script_reparse; unit not known).
extern bool script_ambients_dirty;                   // 0x255b4d8
extern bool script_level_items_dirty;                // 0x2563d88

#endif
