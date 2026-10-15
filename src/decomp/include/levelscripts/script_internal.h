// levelscripts/script_internal.h - the script object parser's navpoint lookup (sum.symbols module
// script_internal.cpp, 0x4dc600; directory guessed, decomp34). Not original source.
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

// (name, out position, out orientation): _stricmp over the level navpoints (0x24e4950, count 0x2539f2c)
// and the script navpoints (0x256b750, count 0x257e96c); false when not found (xor al, al at 0x4dc6db).
bool navpoint_get_position(const char *name, vector3 *pos, matrix *orient);   // 0x4dc600

// decomp41 (level_scripts_common.cpp). Argument lists from its call sites and the callees' prologues.
// The same two searches, the record: the level navpoint, else the script (cutscene) navpoint, else NULL.
navpoint *navpoint_find(const char *name);           // 0x4dc570
void script_parse_navpoints(TextFile *tf);           // 0x4db700 one $Name: record of #Navpoints
void script_parse_objects(TextFile *tf, int flag);   // 0x4dba30 one $Name: record of #Objects (the level passes 1)
// (character, action, its object): action 15, sets the object's cur_action to the label (+0x0c).
void script_action_goto(Character *c, script_action *a, script_object *so);   // 0x4dca10 (return unused)
void script_entry_start(script_object *so, Character *c);       // 0x4dd0b0 (return unused)
void script_entry_advance(script_object *so, Character *c);     // 0x4dd720 (return unused)
bool script_entry_check_done(script_object *so, Character *c);  // 0x4dd780 the current action completed
int  script_build_avoid_spline(Character *c, Character *target);// 0x4ddc60 avoid spline index, -1 failed
// (out, base, suffix): base cut to 32 - strlen(suffix) characters, then suffix (unit not known, after
// conversationctl.cpp).
void str_copy_append_32(char *out, const char *base, const char *suffix);   // 0x4db250
// The script editor's unsaved-changes flags (cleared by level_script_reparse; unit not known).
extern bool script_ambients_dirty;                   // 0x255b4d8
extern bool script_level_items_dirty;                // 0x2563d88

#endif
