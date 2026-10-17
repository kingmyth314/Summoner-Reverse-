// unnamed_cutscene_script_0x4ed050.cpp - Sum.exe v1.40, code 0x4ed050-0x4ed7be. Written from the Sum.exe
// disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact
// (control flow, calls, constants and globals match), likely (one detail of statement order or expression form
// is a guess), unsure (the shape itself is a guess).
//
// The cutscene scripts: <cutscene>_cscript.tbl (its navpoints and script objects), their per-frame driver,
// and the restore of the actors when a cutscene ends or the script editor rewinds it.
// FILE NAME NOT KNOWN (D5306): no path string, no strings.tbl block, no reference name; a placeholder. The
// unit is cut by its static initialiser run (table entries 474-476, 0x58876c-0x588774) after the script
// editor's last COMDAT and before level_script_sewers.cpp; its .bss is 0x256b750-0x257e997 (source-map.md).
// The three initialisers follow the declaration order of level_scripts.cpp's script objects, empty
// initialiser and string pool (0x4d4ff0-0x4d5080), the same three objects for the level scripts.
//
// Header: the functions and data are declared in levelscripts/level_scripts.h (cutscene_script_table in
// summoner/summoner.h).

#include <stdio.h>
#include <string.h>
#include "vsdk/os/stringpool.h"
#include "vsdk/os/frametime.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/vfile/file.h"
#include "vsdk/parse/parse.h"
#include "vsdk/math/matrix.h"
#include "vsdk/ca/character_instance.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/living_entity.h"
#include "engine/camera/camera.h"
#include "summoner/summoner.h"
#include "summoner/gameplay.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/script_internal.h"

// ---- .bss (0x256b750-0x257e997) ------------------------------------------------------------------------
// 0x4ed050  Confidence: exact. static_init_cutscene_object_timers: "jmp 0x4ed060" (initialiser entry 474).
// 0x4ed060  Confidence: exact. init_cutscene_object_timers: the implicit constructor of the 32 records (one
// GameTimer each, at +0x518; the loop starts at 0x2574480 = record 0 + 0x518).
script_object cutscene_script_objects[MAX_CUTSCENE_SCRIPT_OBJECTS];   // 0x2573f68
// 0x4ed080  Confidence: likely. static_init_empty_4ed090: "jmp 0x4ed090" (initialiser entry 475).
// 0x4ed090  Confidence: likely. init_empty_4ed090: a bare RET, the dynamic initialiser of an array whose
// element constructor is inline and empty: the navpoint table (navpoint holds a vector3, whose default
// constructor is `vector3() {}`). Which object it is cannot be seen in the code (D5306).
navpoint cutscene_navpoints[MAX_CUTSCENE_NAVPOINTS];  // 0x256b750 (0x200 records, up to 0x2571750)
// 0x4ed0a0  Confidence: exact. static_init_stringpool_2571750: calls 0x4ed0b0, then "jmp 0x4ed0d0".
// 0x4ed0b0  Confidence: exact. init_stringpool_2571750: StringPool(buffer, 0x2800, false).
// 0x4ed0d0  Confidence: exact. register_atexit_stringpool_2571750: atexit(0x4ed0e0).
// 0x4ed0e0  Confidence: exact. atexit_dtor_stringpool_2571750: "mov ecx, 0x2571750 / jmp ~StringPool".
char cutscene_script_string_buffer[0x2800];           // 0x2571764 (name ours)
StringPool cutscene_script_stringpool(cutscene_script_string_buffer, 0x2800, false);   // 0x2571750
int cutscene_script_object_count;                     // 0x257e968
int cutscene_navpoint_count;                          // 0x257e96c
float cutscene_time;                                  // 0x257e970
char cutscene_script_name[0x24];                      // 0x257e974 "" when no script is loaded

// 0x4ed0f0  Confidence: likely (the unused File local is from the code: it is constructed first and
// destroyed last, with its own EH state 0; the debug sprintf's buffer is never printed). (cutscene name, with
// or without "_cscript.tbl") Loads the cutscene script of the current level: the old script is unloaded,
// "$Level:" must name level_filename, then #Navpoints (the "$Num navpoints:" count is read and ignored; the
// records are counted until #Objects, with no check against the 0x200 slots, see NOTES) and the #Objects
// records (script_parse_objects with level = false). False when there is no name, no cutscene file, the
// table cannot be opened or belongs to another level.
bool cutscene_script_load(const char *name)
{
    File unused;
    TextFile tf;
    char filename[0x24];
    char level[0x100];
    char text[0x100];

    if (!name || cutscene_path_id < 0)
        return false;
    strcpy(filename, name);
    if (!strstr(filename, "_cscript.tbl"))
        strcat(filename, "_cscript.tbl");
    cutscene_script_unload();
    if (tf.Open(filename, path_scripts) != 0)
        return false;
    tf.Expect("$Level: ");
    tf.ParseString(level, 0x100, '"', '"');
    if (_stricmp(level, level_filename))
        return false;
    tf.Expect("#Navpoints");
    if (tf.Accept("$Num navpoints:"))
        tf.ParseInt();
    cutscene_navpoint_count = 0;
    while (!tf.Accept("#Objects"))
        script_parse_navpoint(&cutscene_navpoints[cutscene_navpoint_count++], &tf);
    while (tf.Peek("$Name:"))
        script_parse_objects(&tf, false);
    tf.Expect("#end");
    tf.Close();
    strcpy(cutscene_script_name, filename);
    sprintf(text, "Cutscene script string pool usage: %d bytes\n", cutscene_script_stringpool.GetLength());
    return true;
}

// 0x4ed390  Confidence: exact. The saved equipment of the replace weapon / armor actions is put back (what
// the actor wore instead is freed); the actors that exist only for the cutscene (start navpoint of type
// cutscene, not a party member) are deleted; every other living entity runs its cutscene script object to
// the end, leaves it, gets its AI reset (not type 9 and not flag 0x800) and loses flag 0x1000 (not with
// 0x800) and its action animations. Then the tables are emptied, the name cleared and the camera put back
// on cur_character.
void cutscene_script_unload()
{
    item_info *old1;
    item_info *old2;
    Character *c;
    Character *next;
    int i;

    for (i = 0; i < cutscene_saved_equipment_count; i++) {
        cutscene_saved_item *s = &cutscene_saved_equipment[i];
        if (s->item) {
            character_equip_item(&old1, &old2, s->c, s->slot, s->item);
            if (old1)
                item_free(old1);
            if (old2)
                item_free(old2);
        } else {
            old1 = character_unequip_slot(s->c, s->slot, 1);
            if (old1)
                item_free(old1);
        }
    }
    cutscene_saved_equipment_count = 0;
    for (c = living_list.live_next, next = c->live_next; c != &living_list; c = next, next = next->live_next) {
        if (c->start_navpoint && ((navpoint *)c->start_navpoint)->type == NAVPOINT_TYPE_CUTSCENE
            && c->type != 7) {
            c->deinit();
            continue;
        }
        if (c->cutscene_object != -1) {
            script_object *so = &cutscene_script_objects[c->cutscene_object];
            while (so->cur_action < so->num_actions)
                script_entry_advance(so, c);
        }
        c->cutscene_object = -1;
        if (c->type != 9 && !(c->flags & 0x800))
            c->ai.reset();
        if (!(c->flags & 0x800))
            c->flags &= ~0x1000;
        c->instance->clear_action_anims();
    }
    cutscene_script_object_count = 0;
    cutscene_navpoint_count = 0;
    cutscene_script_stringpool.Reset();
    cutscene_script_name[0] = '\0';
    cutscene_time = 0.0f;
    game_camera.set_target(cur_character);
    game_camera.snap_to_target(1, 1);
}

// 0x4ed550  Confidence: exact. The script editor's "< Time": the clock to 0, the saved equipment put back,
// the cutscene actors back on their start navpoints (position, heading, layer; AI reset, flag 0x1000
// cleared) and every cutscene script object to its start (cur_action -1).
void cutscene_script_rewind_actors()
{
    item_info *old1;
    item_info *old2;
    Character *c;
    int i;

    cutscene_time = 0.0f;
    for (i = 0; i < cutscene_saved_equipment_count; i++) {
        cutscene_saved_item *s = &cutscene_saved_equipment[i];
        if (s->item) {
            character_equip_item(&old1, &old2, s->c, s->slot, s->item);
            if (old1)
                item_free(old1);
            if (old2)
                item_free(old2);
        } else {
            old1 = character_unequip_slot(s->c, s->slot, 1);
            if (old1)
                item_free(old1);
        }
    }
    cutscene_saved_equipment_count = 0;
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->start_navpoint && ((navpoint *)c->start_navpoint)->type == NAVPOINT_TYPE_CUTSCENE) {
            c->pos = ((navpoint *)c->start_navpoint)->pos;
            c->orient.set_from_angles(0.0f, 0.0f, ((navpoint *)c->start_navpoint)->heading);
            c->layer = ((navpoint *)c->start_navpoint)->layer;
            c->ai.reset();
            c->flags &= ~0x1000;
        }
        if (c->cutscene_object >= 0)
            cutscene_script_objects[c->cutscene_object].cur_action = -1;
    }
}

// 0x4ed680  Confidence: exact. Advances the cutscene clock and each live, not suspended cutscene script
// object whose character exists (the object is freed when it does not): while its timer runs it waits,
// unless the next action is a synch / synch-teleport; else after a sequence start it jumps to the sequence,
// or advances when the current action is done.
void cutscene_script_entries_frame()
{
    int i;

    cutscene_time += reported_frametime;
    for (i = 0; i < cutscene_script_object_count; i++) {
        script_object *so = &cutscene_script_objects[i];
        Character *c;

        if (so->end_mode == -1 || so->suspended)
            continue;
        c = living_from_handle(so->handle);
        if (!c) {
            so->end_mode = -1;
            continue;
        }
        if (so->timer.valid() == true && !so->timer.elapsed() && so->cur_action + 1 < so->num_actions) {
            script_action *next = &so->actions[so->cur_action + 1];
            if (next->type != SCRIPT_ACTION_SYNCH && next->type != SCRIPT_ACTION_SYNCH_TELEPORT)
                continue;
        }
        if (cutscene_sequence_started)
            script_entry_jump_to_sequence(so, c);
        else if (script_entry_check_done(so, c))
            script_entry_advance(so, c);
    }
}

// 0x4ed760  Confidence: exact.
char *cutscene_script_table()
{
    return cutscene_script_name;
}

// 0x4ed770  Confidence: exact (the result is built in EAX, `mov eax, 1` / the strcmp result 0; the
// callers test AL). A cutscene runs or a cutscene script is loaded (the inline strcmp against "").
bool cutscene_script_is_active()
{
    return (game_flags & GF_CUTSCENE) || strcmp(cutscene_script_name, "");
}
