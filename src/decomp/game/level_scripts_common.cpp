// level_scripts_common.cpp - Sum.exe v1.40, code 0x4ca700-0x4ceebf (levelscripts\Scripts\level_scripts_common.cpp,
// path string 0x59e22c; docs/sum-exe/level-scripts.md, source-map.md)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// The unit holds the level side of the level scripts: the console toggles dont_load_fx / do_frame_callback /
// No_cutscenes and eax_save_props, the level triggers (level exits and script "use" triggers), the player
// starts, level restart / reload / reparse, the level tables (level_items.tbl, <level>.tbl with effects,
// ambient sounds, triggers, shopkeepers, automap layers and fog, <script>_script.tbl with navpoints, script
// objects, doors and clicks), the per-frame hooks, the cutscene hook, the ambient emitters, the level
// container ("lc") entries and the script helpers the ls_* units call (script objects by index and by name,
// party items and XP, guards, the avoid splines). Static initialiser table entries 338-344 (0x58854c-0x588564)
// are this unit's.
// Data: .data 0x59e0e8-0x59e793 (the two trigger keyword tables, Do_frame_callback, the literals);
// .bss 0x248ce40-0x2493a17. Pooled literals of other units: "#End" 0x58bd08, "$Name:" 0x58bd00,
// "$Script:" 0x593a4c, "$Trigger:" 0x59838c, "#Town" 0x593d6c, "#Objects" 0x593d60, "+Locked:" 0x59a428,
// "$layer" 0x59c10c, "+Distance:" 0x59bb30, ".tbl" 0x594c20, "generic" 0x5963f0, "Joseph" 0x593888, the
// four ring names, the bool command texts (0x588c34..0x588c50), "" 0x5bd0d0.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vsdk/os/console.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/stringpool.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/frametime.h"
#include "vsdk/parse/parse.h"
#include "vsdk/vfile/file.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/snd/snd.h"
#include "vsdk/localize/localize.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/ca/character_instance.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/ai/ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/ai/boss_ai.h"
#include "engine/effects/fx.h"
#include "engine/rendering/levelrender.h"
#include "engine/s3d/s3d.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/interface/interface.h"
#include "summoner/multi/multi.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"
#include "levelscripts/script_internal.h"

#define LEVEL_SCRIPTS_COMMON_FILE "D:\\projects\\Summoner\\pccode\\levelscripts\\Scripts\\level_scripts_common.cpp"

// ---- .data (0x59e0e8-) -----------------------------------------------------------------------------
const char *level_trigger_type_names[2] = { "spline", "location" };          // 0x59e0e8 (name ours)
const char *level_trigger_id_names[3] = { "load level", "boss", "sound" };   // 0x59e0f0 (name ours)
bool Do_frame_callback = true;                                               // 0x59e0fc

// ---- .bss (0x248ce40-0x2493a17) --------------------------------------------------------------------
// The objects with dynamic initialisers are defined below at their initialisers' place (entries 338-344,
// definition order = initialiser order): level_ambients (entry 338, guessed), level_music_stop_timer
// (0x24902a8), level_exit_return_pos (entry 340, guessed), ccmd_dont_load_fx (0x2490c80),
// ccmd_do_frame_callback (0x2490c68), ccmd_eax_save_props (0x24902b8), ccmd_No_cutscenes (0x2490c90).
lc_entry lc_entries[50];                             // 0x248ce40
float level_fog_near;                                // 0x2490298
void (*level_script_init_fn)();                      // 0x249029c
int level_shopkeeper_count;                          // 0x24902a0
const char *level_pending_exit_name;                 // 0x24902a4
void (*level_script_frame_fn)();                     // 0x24902ac
void (*cutscene_done_callback)();                    // 0x24902b0
int level_music;                                     // 0x24902b4
char *level_exit_trigger;                            // 0x24902c4
level_effect level_effects[MAX_LEVEL_EFFECTS];       // 0x24902c8
float level_fog_far;                                 // 0x2490c58
void (*level_script_action_fn)(dialogue_action *action);   // 0x2490c5c
int level_exit_trigger_index;                        // 0x2490c60
int level_trigger_count;                             // 0x2490c74
int level_unk_2490c78;                               // 0x2490c78 (name ours)
shopkeeper level_shopkeepers[MAX_LEVEL_SHOPKEEPERS]; // 0x2490ca0
int level_effect_count;                              // 0x2490dcc
level_trigger level_triggers[MAX_LEVEL_TRIGGERS];    // 0x2490dd0
unsigned int level_exit_block_flags;                 // 0x2491a50
void (*level_script_leave_fn)();                     // 0x2491a54
int level_ambient_count;                             // 0x2491a58
bool game_frozen;                                    // 0x2491a5c
char (*level_script_use_fn)(const char *name);       // 0x2491a60
void *level_script_unk14_fn;                         // 0x2491a64
int lc_entry_count;                                  // 0x2491a68
script_avoid_spline script_avoid_splines[MAX_SCRIPT_AVOID_SPLINES];   // 0x2491a70
shop_item level_shop_items[MAX_LEVEL_SHOP_ITEMS];    // 0x2491ac0
int level_shop_item_count;                           // 0x2493a00
void (*cutscene_saved_frame_fn)();                   // 0x2493a04
bool level_script_restarting;                        // 0x2493a14
bool Dont_load_fx;                                   // 0x2493a15
bool No_cutscenes;                                   // 0x2493a16

// 0x4ca700 static_init_empty_4ca710 (jmp 0x4ca710). Confidence: unsure.
// 0x4ca710 init_empty_4ca710 (ret). Confidence: unsure. An empty dynamic initialiser: an object whose
// constructor is inline and empty (vector3 members). Which object is a guess: level_ambients (its records
// hold a vector3); see NOTES.
level_ambient level_ambients[0x80];                  // 0x2490458

// 0x4ca720 static_init_level_script_timer (jmp 0x4ca730). Confidence: exact.
// 0x4ca730 init_level_script_timer (mov ecx, 0x24902a8; jmp GameTimer__GameTimer). Confidence: exact.
GameTimer level_music_stop_timer;                    // 0x24902a8

// 0x4ca740 static_init_empty_4ca750 (jmp 0x4ca750). Confidence: unsure.
// 0x4ca750 init_empty_4ca750 (ret). Confidence: unsure. The second empty initialiser; guessed to be the
// vector3 level_exit_return_pos (see NOTES).
vector3 level_exit_return_pos;                       // 0x2493a08

// 0x4ca760 static_init_ccmd_dont_load_fx (jmp). Confidence: exact.
// 0x4ca770 init_ccmd_dont_load_fx. Confidence: exact.
console_command ccmd_dont_load_fx("dont_load_fx", "Toggles Dont_load_fx", cmd_dont_load_fx);   // 0x2490c80

// 0x4ca790  Confidence: exact. The console's bool command (as cmd_Preparse_scripts in level_scripts.cpp);
// the help names the command, then the variable.
void cmd_dont_load_fx()
{
    if (calling_a_command_function) {
        process_command(0xc1);
        if (arg_type & ARG_TRUE)
            Dont_load_fx = true;
        else if (arg_type & ARG_FALSE)
            Dont_load_fx = false;
        else if (arg_type & ARG_NONE)
            Dont_load_fx ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it.\n",
                "dont_load_fx", "Dont_load_fx");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "dont_load_fx", Dont_load_fx ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x4ca850 static_init_ccmd_do_frame_callback (jmp). Confidence: exact.
// 0x4ca860 init_ccmd_do_frame_callback. Confidence: exact.
console_command ccmd_do_frame_callback("do_frame_callback", "Toggles Do_frame_callback",
                                       cmd_do_frame_callback);   // 0x2490c68

// 0x4ca880  Confidence: exact.
void cmd_do_frame_callback()
{
    if (calling_a_command_function) {
        process_command(0xc1);
        if (arg_type & ARG_TRUE)
            Do_frame_callback = true;
        else if (arg_type & ARG_FALSE)
            Do_frame_callback = false;
        else if (arg_type & ARG_NONE)
            Do_frame_callback ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it.\n",
                "do_frame_callback", "Do_frame_callback");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "do_frame_callback", Do_frame_callback ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x4ca940 static_init_ccmd_eax_save_props (jmp). Confidence: exact.
// 0x4ca950 init_ccmd_eax_save_props. Confidence: exact.
console_command ccmd_eax_save_props("eax_save_props", "Saves the EAX properties to disk",
                                    cmd_eax_save_props);   // 0x24902b8

// 0x4ca970  Confidence: exact (VC6 byte match) (the order of the three buffers in the frame: message at +0, name +0x40,
// path +0x80).
void cmd_eax_save_props()
{
    char msg[64];
    char name[64];
    char path[256];

    if (calling_a_command_function) {
        sprintf(name, "%s.eax", level_filename);
        file_make_path(path_internal_copy, name, path);
        if (eax_save_props(path))
            sprintf(msg, "EAX props for %s saved\n", level_filename);
        else
            sprintf(msg, "Failed to save EAX props for %s\n", level_filename);
        print_to_console(msg, 0);
    }
}

// 0x4caa00  Confidence: likely (the vector temporaries; the radius test loads the radius once more after
// distance_squared, as `r * r` against a copy). Runs while no exit is pending: a crossed spline trigger
// or an entered radius trigger either calls the level's use handler (LEVEL_TRIGGER_SCRIPT_USE) or becomes
// the pending level exit. Then the exit is blocked (level_exit_block_flags) when, outside towns, the world
// map and the "rand" levels, a party member is more than 10 units away or a hostile in combat with a party
// member is within 20 units of it.
void level_check_triggers()
{
    vector3 prev, cur, dir;
    int i;

    if (US_open_demo || level_exit_trigger || cur_character->ai.is_mode(3, -1))
        return;
    for (i = 0; i < level_trigger_count; i++) {
        if (level_triggers[i].id != 0)
            continue;
        if (level_triggers[i].plane != -1 && level_triggers[i].plane != cur_character->layer)
            continue;
        if ((game_flags & GF_MULTIPLAYER) && !(level_triggers[i].flags & LEVEL_TRIGGER_MULTI))
            continue;
        if ((game_flags & GF_SINGLE_PLAYER) && (level_triggers[i].flags & LEVEL_TRIGGER_MULTI))
            continue;
        if (level_triggers[i].type == 0) {
            prev = cur_character->last_pos;
            cur = cur_character->pos;
            dir = cur - prev;
            dir.normalize();
            dir *= 0.3f;
            prev -= dir;
            cur += dir;
            vector3 *points = level_triggers[i].path->points;
            if (segments_intersect_xz(&prev, &cur, &points[0], &points[1]) == true
                && level_trigger_point_left_of_line(level_triggers[i].path, &prev, &cur)) {
                if (level_triggers[i].flags & LEVEL_TRIGGER_SCRIPT_USE)
                    level_script_use(level_triggers[i].ref_name);
                else {
                    level_exit_return_pos = prev;
                    break;
                }
            }
        } else if (level_triggers[i].type == 1) {
            if (distance_squared(&level_triggers[i].nav->pos, &cur_character->pos)
                <= level_triggers[i].radius * level_triggers[i].radius) {
                dir = cur_character->pos - level_triggers[i].nav->pos;
                dir.normalize();
                dir *= level_triggers[i].radius + 0.3f;
                if (level_triggers[i].flags & LEVEL_TRIGGER_SCRIPT_USE)
                    level_script_use(level_triggers[i].ref_name);
                else {
                    level_exit_return_pos = level_triggers[i].nav->pos + dir;
                    break;
                }
            }
        }
    }
    if (i == level_trigger_count)
        return;

    level_exit_block_flags = 0;
    level_exit_trigger = level_triggers[i].name;
    level_exit_trigger_index = level_triggers[i].index;
    level_pending_exit_name = level_triggers[i].script;
    if (!level_is_town && cur_level_index != 4 && _strnicmp(level_filename, "rand-", 4) != 0) {
        Player *p;
        Character *c;

        for (p = party_list.next; p != &party_list; p = p->next) {
            c = living_from_handle(p->handle);
            if (!(c->flags & CF_SUMMON) && !(c->flags & CF_DEAD) && c != cur_character) {
                vector3 d = c->pos - cur_character->pos;
                if (d.len_sq() > 100.0f) {
                    level_exit_block_flags |= 1;
                    break;
                }
            }
        }
        for (c = living_list.live_next; c != &living_list; c = c->live_next) {
            if (c->type == 5 || c->type == 9) {
                if (c->ai.is_mode(3, -1)) {
                    for (p = party_list.next; p != &party_list; p = p->next) {
                        if (p->handle == c->ai.target) {
                            Character *pc = living_from_handle(p->handle);
                            vector3 d = pc->pos - c->pos;
                            if (d.len_sq() <= 400.0f) {
                                level_exit_block_flags |= 2;
                                break;
                            }
                        }
                    }
                }
                if (level_exit_block_flags & 2)
                    break;
            }
        }
    }
    game_frozen = false;
}

// 0x4caed0  Confidence: exact. The previous position is passed but not read.
int level_trigger_point_left_of_line(s3d_named_path *path, const vector3 *prev, const vector3 *cur)
{
    vector2 a, b, p;
    seg2d seg;

    a.from_xz(&path->points[0]);
    b.from_xz(&path->points[1]);
    seg.a = &a;
    seg.b = &b;
    p.from_xz(cur);
    return seg2d_point_left_of(&p, &seg, 0) != 0;
}

// 0x4caf40  Confidence: exact. The "$player<entry>-NN" markers, else the plain "$playerNN" ones. The
// final check reads level_player_starts[4] (the loop counter after the loop), which is level_entry_point
// (>= 1), so the error never fires (game bug, NOTES).
void level_find_player_starts()
{
    char name[64];
    int i;

    if (level_entry_point == -1)
        level_entry_point = 1;
    for (i = 0; i < 4; i++) {
        sprintf(name, "$player%d-%02d", level_entry_point, i + 1);
        level_player_starts[i] = navpoint_find(name);
    }
    if (level_player_starts[0])
        return;
    for (i = 0; i < 4; i++) {
        sprintf(name, "$player%02d", i + 1);
        level_player_starts[i] = navpoint_find(name);
    }
    if (!level_player_starts[i]) {
        sprintf(string_buffer, "Unable to locate player starts for level %s\n\nFind Allender.", level_filename);
        VSDK_ERROR(LEVEL_SCRIPTS_COMMON_FILE, 1462, string_buffer);
    }
}

// 0x4cb010  Confidence: exact.
void script_place_party_at_starts()
{
    Player *p;
    int slot = 0;

    level_find_player_starts();
    for (p = party_list.next; p != &party_list; p = p->next, slot++) {
        Character *c = living_from_handle(p->handle);
        c->place_at_party_start(slot);
    }
    camera_reset_to_player(0);
}

// 0x4cb060  Confidence: likely (the door bit expression; the item and avoid spline loops). The level's
// leave and init hooks run again, every character with a start navpoint goes back to it with its script
// restarted, the doors close, the containers and the level items come back, and the avoid splines are
// freed (their slots are not cleared; level_script_leave does the same).
void level_script_restart()
{
    Character *c;
    obj *o, *next;
    vector3 pos;
    matrix orient;
    int i, j;

    level_script_restarting = true;
    if (level_script_leave_fn)
        level_script_leave_fn();
    if (level_script_init_fn)
        level_script_init_fn();
    level_script_restarting = false;
    level_session_time = 0.0f;

    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->info->species == 0x59 && character_is_serpent_part(c))
            continue;
        navpoint *np = (navpoint *)c->start_navpoint;
        if (!np || np->_unk28 != 0)
            continue;
        c->pos = np->pos;
        c->orient.set_from_angles(0.0f, 0.0f, ((navpoint *)c->start_navpoint)->heading);
        c->layer = ((navpoint *)c->start_navpoint)->layer;
        c->ai.reset();
        c->flags &= ~0x1001;
        c->prop_tag = c->default_prop_tag;
        c->instance->clear_action_anims();
        c->instance->clear_state_anims();
        c->update_locomotion_anim();
        c->instance->process(0.0f, false, 0);
        if (c->script_object >= 0) {
            script_object *so = &script_objects[c->script_object];
            so->end_mode = 0;
            for (j = 0; j < so->num_actions; j++) {
                if (so->actions[j].type == SCRIPT_ACTION_WAIT_GO)
                    so->actions[j].arg0c = 0;
                else if (so->actions[j].type == SCRIPT_ACTION_LOOP)
                    so->end_mode = 1;
            }
            so->cur_action = -1;
            so->timer.invalidate();
        }
    }

    for (i = 0; i < num_doors; i++) {
        lobj *l = &lobjs[doors[i].lobj_index];
        l->pos = doors[i].closed_pos;
        doors[i].closed_orient.to_matrix_b(&l->orient);
        savegame_state.two_stage_set(false, (doors[i].state >> 3) & 1, l);
    }
    for (i = 0; i < lc_entry_count; i++)
        if (lc_entries[i].object)
            savegame_state.two_stage_set(false, false, lc_entries[i].object);

    for (o = obj_active_list.next, next = o->next; o != &obj_active_list; o = next, next = next->next)
        if (o->type == OBJ_TYPE_ITEM && ((item_obj *)o)->level_item >= 0)
            o->deinit();
    containers_free_all();
    containers_reset();

    for (i = 0; i < level_item_count; i++) {
        if (navpoint_get_position(level_items[i].navpoint->name, &pos, &orient))
            level_item_spawn(item_create_by_name(level_items[i].type->name), &pos, &orient,
                             level_items[i].count, i, -1);
    }
    for (i = 0; i < MAX_SCRIPT_AVOID_SPLINES; i++)
        if (script_avoid_splines[i].key != -1)
            avoid_spline_free(script_avoid_splines[i].spline);
}

// 0x4cb350  Confidence: exact. Every object but the party members (type 7) is deinitialised, then the
// level is reparsed (script editor).
void level_script_reload()
{
    obj *o, *next;

    for (o = obj_active_list.next, next = o->next; o != &obj_active_list; o = next, next = next->next)
        if (o->type != 7)
            o->deinit();
    level_script_restarting = true;
    level_script_leave();
    level_script_reparse();
    level_script_restarting = false;
    level_session_time = 0.0f;
}

// 0x4cb3a0  Confidence: exact.
void level_script_reparse()
{
    script_ambients_dirty = false;
    script_level_items_dirty = false;
    level_music_stop_timer.SetTimeout(900);
    level_exit_trigger = NULL;
    level_pending_exit_name = NULL;
    game_frozen = false;
    if (!level_script_restarting) {
        level_ambient_count = 0;
        level_music = -1;
        level_effect_count = 0;
        script_object_count = 0;
        level_trigger_count = 0;
        level_shopkeeper_count = 0;
        level_shop_item_count = 0;
        level_unk_2490c78 = 0;
        lc_entry_count = 0;
    }
    level_script_parse_table();
    level_items_parse_table();
    level_parse_table();
    level_find_player_starts();
    script_avoid_splines_reset();
}

// 0x4cb430  Confidence: likely (one shared text buffer; the two messages are only formatted, never shown).
// The "#Level:" section of the current level: every record is kept; the ones of the current script whose
// savegame entry is still there are placed at their navpoint (hidden while their +Event: flag is clear).
void level_items_parse_table()
{
    TextFile tf;
    vector3 pos;
    matrix orient;
    char buf[256];

    if (cur_level_index == -1)
        return;
    if (tf.Open("level_items.tbl", path_tables)) {
        sprintf(buf, "\"%s\" not found.\n", "level_items.tbl");
        return;
    }
    tf.Expect("#Level Items");
    sprintf(buf, "#Level: \"%s\"", level_script_table[cur_level_index].name);
    if (!tf.SkipPast(buf, "#End")) {
        sprintf(buf, "No level item entry for %s in \"%s\"\n", level_script_table[cur_level_index].name,
                "level_items.tbl");
        return;
    }
    level_item_count = 0;
    while (tf.Accept("$Name:")) {
        level_item_rec *li = &level_items[level_item_count];
        int type;

        tf.ParseString(li->name, 0x20, '"', '"');
        tf.Expect("$Script:");
        tf.ParseString(li->script, 0x20, '"', '"');
        tf.Expect("$Item:");
        tf.ParseString(buf, 0x100, '"', '"');
        type = item_type_find(buf);
        if (type >= 0)
            li->type = &item_types[type];
        tf.Expect("$Navpoint:");
        tf.ParseString(buf, 0x100, '"', '"');
        if (!_stricmp(li->script, level_script_name))
            li->navpoint = navpoint_find(buf);
        if (tf.Accept("+Count:"))
            li->count = tf.ParseInt();
        else
            li->count = 1;
        if (tf.Accept("+Event:")) {
            tf.ParseString(buf, 0x100, '"', '"');
            li->flag_level = level_script_find(buf);
            tf.ParseString(li->flag, 0x20, '"', '"');
        } else {
            li->flag_level = -1;
            li->flag[0] = '\0';
        }
        if (!_stricmp(li->script, level_script_name) && savegame_state.level_item_has(li->name)) {
            li->navpoint->level_item = level_item_count;
            if (navpoint_get_position(li->navpoint->name, &pos, &orient)) {
                container *it = level_item_spawn(item_create_by_name(li->type->name), &pos, &orient, li->count,
                                                 level_item_count, li->navpoint->layer);
                if (li->flag_level != -1 && !savegame_state.get_flag(li->flag_level, li->flag))
                    it->obj_flags &= ~1;
                savegame_state.level_item_add(li->name);
            }
        }
        level_item_count++;
    }
}

// 0x4cb7b0  Confidence: likely (the buffers of the automap and fog parts; the "+Distance:" loop is a
// do-while here, the binary repeats its first statements at the loop end). <level>.tbl when it exists and
// the level is not restarting: #Effects, #Ambient Sounds, #Triggers, #Shopkeepers, #Automap, #Fog; then the
// EAX properties of the level (<level>.eax in data\internal) and the world effects.
void level_parse_table()
{
    File f;
    TextFile tf;
    char filename[256];
    char fxname[0x20];
    int i;

    level_num_layers = 0;
    memset(level_layer_maps, -1, sizeof(level_layer_maps));
    memset(level_layer_automap_index, -1, sizeof(level_layer_automap_index));
    if (!level_script_restarting) {
        strcpy(filename, level_filename);
        strcat(filename, ".tbl");
        if (f.find(filename, PATH_ANY)) {
            tf.Open(filename, path_tables_levels);
            tf.Expect("#Effects");
            while (tf.Accept("$Level_fx:")) {
                char script[0x80];
                int use;

                tf.ParseString(fxname, 0x20, '"', '"');
                strcpy(level_effects[level_effect_count].name, fxname);
                level_effects[level_effect_count].handle = -1;
                level_effects[level_effect_count].flag = tf.Accept("once");
                if (tf.Peek("+OnlyScript:")) {
                    use = 0;
                    while (tf.Accept("+OnlyScript:")) {
                        tf.ParseString(script, 0x80, '"', '"');
                        if (!_stricmp(script, level_script_name))
                            use = 1;
                    }
                } else
                    use = 1;
                if (!Dont_load_fx && use)
                    level_effect_count++;
            }
            if (tf.Accept("$Vif_count:"))
                tf.ParseInt();
            tf.Expect("#Ambient Sounds");
            level_parse_ambient(&tf);
            tf.Expect("#Triggers");
            level_parse_trigger(&tf);
            tf.Expect("#Shopkeepers");
            level_parse_shopkeeper(&tf);
            tf.Expect("#Automap");
            while (!tf.Peek("#Fog")) {
                if (tf.Accept("$AutomapPSX:")) {
                    char psx_layer[0x80], psx_bitmap[0x80];

                    tf.ParseString(psx_layer, 0x7f, '"', '"');
                    if (_strnicmp(psx_layer, "$layer", 6))
                        VSDK_ERROR(LEVEL_SCRIPTS_COMMON_FILE, 882, "Layer name is invalid.");
                    if (atoi(psx_layer + 6) == 0)
                        VSDK_ERROR(LEVEL_SCRIPTS_COMMON_FILE, 886, "Layer name is invalid.");
                    tf.ParseInt();
                    tf.ParseString(psx_bitmap, 0x7f, '"', '"');
                } else if (tf.Accept("$AutomapPC:")) {
                    char pc_layer[0x80], pc_bitmap[0x80];
                    int layer, map;

                    tf.ParseString(pc_layer, 0x7f, '"', '"');
                    if (_strnicmp(pc_layer, "$layer", 6))
                        VSDK_ERROR(LEVEL_SCRIPTS_COMMON_FILE, 917, "Layer name is invalid.");
                    layer = atoi(pc_layer + 6);
                    if (layer == 0)
                        VSDK_ERROR(LEVEL_SCRIPTS_COMMON_FILE, 921, "Layer name is invalid.");
                    layer--;
                    map = tf.ParseInt();
                    tf.ParseString(pc_bitmap, 0x7f, '"', '"');
                    level_layer_maps[map - 1] = bm_load(pc_bitmap, -1, -1);
                    level_layer_automap_index[layer] = map - 1;
                    if (map > level_num_layers)
                        level_num_layers = map;
                } else
                    break;
            }
            for (i = 0; i < level_num_layers; i++)
                if (level_layer_maps[i] < 0)
                    VSDK_ERROR(LEVEL_SCRIPTS_COMMON_FILE, 972, "Specified automap bitmap not found!");

            int r, g, b;
            tf.Expect("#Fog");
            if (tf.Accept("+Color:")) {
                r = tf.ParseInt();
                g = tf.ParseInt();
                b = tf.ParseInt();
                if (r < 0)
                    r = 0;
                else if (r > 255)
                    r = 255;
                if (g < 0)
                    g = 0;
                else if (g > 255)
                    g = 255;
                if (b < 0)
                    b = 0;
                else if (b > 255)
                    b = 255;
            } else {
                b = 0;
                g = 0;
                r = 0;
            }
            fog_set_color(r, g, b);
            if (tf.Accept("+Distance:")) {
                do {
                    char fog_layer[0x100];
                    float far_dist, near_dist;
                    int layer, lr, lg, lb;

                    tf.ParseString(fog_layer, 0x100, '"', '"');
                    far_dist = tf.ParseFloat();
                    near_dist = tf.ParseFloat();
                    if (_strnicmp(fog_layer, "$layer", 6))
                        VSDK_ERROR(LEVEL_SCRIPTS_COMMON_FILE, 1013, "Layer name is invalid.");
                    layer = atoi(fog_layer + 6);
                    if (!layer)
                        VSDK_ERROR(LEVEL_SCRIPTS_COMMON_FILE, 1019, "Layer name is invalid.");
                    layer--;
                    if (tf.Accept("+Color:")) {
                        lr = tf.ParseInt();
                        lg = tf.ParseInt();
                        lb = tf.ParseInt();
                    } else {
                        lr = r;
                        lg = g;
                        lb = b;
                    }
                    fog_set_layer(layer, far_dist, near_dist, lr, lg, lb);
                    if (tf.Accept("NoSnow"))
                        fog_layer_set_snow(layer, false);
                } while (tf.Accept("+Distance:"));
            } else {
                bool set;

                level_fog_near = 0.0f;
                level_fog_far = 0.0f;
                set = false;
                if (tf.Accept("+Fog Near:")) {
                    level_fog_near = tf.ParseFloat();
                    set = true;
                }
                if (tf.Accept("+Fog Far:")) {
                    level_fog_far = tf.ParseFloat();
                    set = true;
                }
                if (set) {
                    fog_set_level_default(level_fog_far, level_fog_near);
                    fog_restore_level_default();
                }
            }
            if (tf.Accept("+CharFog:")) {
                float near_dist = tf.ParseFloat();
                float far_dist = tf.ParseFloat();
                char_fog_set(near_dist, far_dist);
            } else
                char_fog_disable();
            tf.Expect("#End");
            tf.Close();
        }
    }
    {
        char eax_name[0x40];
        char eax_path[0x180];

        snd_set_eax_environment("generic");
        sprintf(eax_name, "%s.eax", level_filename);
        file_make_path(path_internal_copy, eax_name, eax_path);
        eax_load_props(eax_path);
    }
    level_create_world_effects();
}

// 0x4cc010  Confidence: exact (VC6 byte match) (the order of the three buffers). "$Ambient:" name sound p1 p2 p3 [delay]:
// the rest of the line is the repeat delay in seconds (none or "%f"); the position is a script navpoint
// or a level-file marker.
void level_parse_ambient(TextFile *tf)
{
    char name[0x80];
    char sound[0x80];
    char line[0x80];
    vector3 pos;
    matrix orient;
    float p1, p2, p3, delay;
    int handle;

    while (tf->Accept("$Ambient:")) {
        tf->ParseString(name, 0x80, '"', '"');
        tf->ParseString(sound, 0x80, '"', '"');
        p1 = tf->ParseFloat();
        p2 = tf->ParseFloat();
        p3 = tf->ParseFloat();
        delay = 0.0f;
        if (!tf->ReadLine(line, 0x80))
            continue;
        if (strlen(line) != 0 && sscanf(line, "%f", &delay) != 1)
            continue;
        if (navpoint_get_position(name, &pos, &orient) == true || level_marker_find(name, &pos, &orient) == true) {
            handle = snd_get_handle(sound, p1, p2, p3, 0, false);
            if (handle != -1)
                level_ambient_add(handle, &pos, (int)(delay * 1000.0f));
        } else
            sprintf(string_buffer, "Ambient sound navpoint not found: %s\n", name);
    }
}

// 0x4cc190  Confidence: exact (VC6 byte match) (the running record pointer; the "default" case leaves the slot counted).
// "$Trigger:" name [+Multi] +Id: (load level | boss | sound); load level: [+Script:] [+RefName:];
// [+Index:] +Type: (spline: [+Spline name:] | location: +Location: +Radius:) [+Plane:]. A trigger whose path
// or navpoint is missing is dropped (the count goes back, the record is reused).
void level_parse_trigger(TextFile *tf)
{
    char buf[0x100];
    level_trigger *t = level_triggers;
    int level, slot;

    while (tf->Accept("$Trigger:")) {
        level_trigger_count++;
        t->flags = LEVEL_TRIGGER_ON;
        tf->ParseString(t->name, 0x20, '"', '"');
        if (tf->Accept("+Multi"))
            t->flags |= LEVEL_TRIGGER_MULTI;
        tf->Expect("+Id:");
        t->id = tf->ParseTypeString(level_trigger_id_names, 3);
        if (t->id == 0) {
            if (tf->Accept("+Script:"))
                tf->ParseString(buf, 0x100, '"', '"');
            else {
                strcpy(buf, t->name);
                if (t->flags & LEVEL_TRIGGER_MULTI)
                    strcat(buf, "_m1");
            }
            if (_stricmp(t->name, "level select") != 0
                && (slot = level_script_find_index(level = level_script_find(t->name), buf)) >= 0)
                t->script = CONVERSATION_SAVE_INFO(level, slot)->name;
            else
                t->script = "";
            if (tf->Accept("+RefName:"))
                tf->ParseString(t->ref_name, 0x20, '"', '"');
            else
                strcpy(t->ref_name, t->name);
        }
        t->index = 0;
        if (tf->Accept("+Index:"))
            t->index = tf->ParseInt();
        tf->Expect("+Type:");
        t->type = tf->ParseTypeString(level_trigger_type_names, 2);
        t->plane = -1;
        if (tf->Accept("+Plane:") == true)
            t->plane = tf->ParseInt();
        switch (t->type) {
        case 0:
            if (tf->Accept("+Spline name:"))
                tf->ParseString(buf, 0x20, '"', '"');
            else
                strcpy(buf, t->name);
            t->path = level_trigger_record_find(buf);
            if (!t->path) {
                sprintf(string_buffer, "Couldn't find level trigger: %s\n", t->name);
                level_trigger_count--;
                break;
            }
            t++;
            break;
        case 1:
            tf->Expect("+Location:");
            tf->ParseString(buf, 0x20, '"', '"');
            t->nav = navpoint_find(buf);
            if (!t->nav) {
                sprintf(string_buffer, "Couldn't find level trigger: %s\n", t->name);
                tf->Expect("+Radius:");
                t->radius = tf->ParseFloat();
                level_trigger_count--;
            } else {
                tf->Expect("+Radius:");
                t->radius = tf->ParseFloat();
                t++;
            }
            break;
        }
    }
}

// 0x4cc4d0  Confidence: likely (the duplicate scans and the price rounding). "$Shopkeeper:" character
// then any number of "+Sell List:" / "+Buy List:" blocks of item names up to "$End". A sell entry costs the
// item's value; the shop pays 70 % of it for a buy entry (10 %, at least 1, in multiplayer). Entries of the
// same item are kept once; both lists are sorted.
void level_parse_shopkeeper(TextFile *tf)
{
    char name[0x80];
    char item[0x100];
    shopkeeper *s;
    Character *c;
    int obj, type, price, i;
    bool sell;

    while (tf->Accept("$Shopkeeper:")) {
        s = &level_shopkeepers[level_shopkeeper_count++];
        tf->ParseString(name, 0x80, '"', '"');
        obj = script_object_find(name);
        if (obj == -1) {
            for (c = living_list.live_next; c != &living_list; c = c->live_next)
                if (!_stricmp(c->get_name(), name))
                    break;
            if (c != &living_list)
                s->name = c->get_name();
            else {
                sprintf(string_buffer, "Unable to locate character %s for shopkeeper support", name);
                s->name = "";
            }
        } else
            s->name = living_from_handle(script_objects[obj].handle)->get_name();
        s->num_sell = 0;
        s->num_buy = 0;
        s->sell = NULL;
        s->buy = NULL;
        while (tf->Peek("+Sell List:") || tf->Peek("+Buy List:")) {
            if (tf->Peek("+Sell List:")) {
                tf->Expect("+Sell List:");
                sell = true;
                s->sell = &level_shop_items[level_shop_item_count];
            } else {
                tf->Expect("+Buy List:");
                sell = false;
                s->buy = &level_shop_items[level_shop_item_count];
            }
            while (!tf->Peek("$End")) {
                tf->ParseString(item, 0x100, '"', '"');
                type = item_type_find(item);
                if (type == -1) {
                    sprintf(string_buffer, "Unable to locate item %s in any table file.\n", item);
                    continue;
                }
                price = item_types[type].value;
                if (!item_types[type].name)
                    continue;
                if (sell) {
                    for (i = 0; i < s->num_sell; i++)
                        if (s->sell[i].item_type == type)
                            break;
                    if (i < s->num_sell)
                        continue;
                } else {
                    for (i = 0; i < s->num_buy; i++)
                        if (s->buy[i].item_type == type)
                            break;
                    if (i < s->num_buy)
                        continue;
                    if (game_flags & GF_MULTIPLAYER) {
                        price = (int)(price * 0.1f);
                        if (price < 1)
                            price = 1;
                    } else
                        price = (int)(price * 0.7f);
                }
                level_shop_items[level_shop_item_count].item_type = type;
                level_shop_items[level_shop_item_count].price = price;
                level_shop_item_count++;
                if (sell)
                    s->num_sell++;
                else
                    s->num_buy++;
            }
            tf->Expect("$End");
        }
        if (s->sell)
            shop_inventory_sort(s->sell, s->num_sell);
        if (s->buy)
            shop_inventory_sort(s->buy, s->num_buy);
    }
}

// 0x4cc7e0  Confidence: exact (VC6 byte match) (the clamps as max / min). The level's init hook, the lc entries' "used"
// bit from the savegame; on a fresh entry the monsters' levels: single player 0.8-1.2 times the current
// character's level, capped by the script's +Levelcap: (characters above the player keep theirs);
// multiplayer the host level times the session scale within a difficulty band. In multiplayer four boss
// levels start with their traps off or their boss engaged.
void level_script_init(bool scale_levels)
{
    Character *c;
    int i, level, slot;

    if (level_script_init_fn)
        level_script_init_fn();
    for (i = 0; i < lc_entry_count; i++) {
        bool open, locked;

        savegame_state.two_stage_get(&open, &locked, lc_entries[i].object);
        if (open)
            lc_entries[i].flags |= 1;
        else
            lc_entries[i].flags &= ~1;
    }
    level = level_script_find(level_filename);
    slot = level_script_find_index(level, level_script_name);
    if (scale_levels) {
        if (game_flags & GF_SINGLE_PLAYER) {
            int cap = CONVERSATION_SAVE_INFO(level, slot)->level_cap;
            float target = frand_range(0.8f, 1.2f) * cur_character->level;

            for (c = living_list.live_next; c != &living_list; c = c->live_next) {
                if (cur_character) {
                    if (c->type != 7 && cur_character->level >= c->level) {
                        c->level = (short)(int)(target + 0.5f);
                        c->level = max((short)1, c->level);
                        c->level = min((short)cap, c->level);
                        c->scale_stats_to_level();
                    }
                } else if (c->type != 7) {
                    c->level = (short)c->info->level;
                    c->scale_stats_to_level();
                }
            }
        } else {
            int lo, hi;
            float target;

            switch (mp_game_difficulty) {
            case 0:
                lo = 1;
                hi = CONVERSATION_SAVE_INFO(level, slot)->level_cap;
                break;
            case 1:
                lo = 25;
                hi = CONVERSATION_SAVE_INFO(level, slot)->level_cap + lo;
                break;
            case 2:
                lo = 50;
                hi = CONVERSATION_SAVE_INFO(level, slot)->level_cap + lo;
                break;
            default:
                lo = 1;
                hi = CONVERSATION_SAVE_INFO(level, slot)->level_cap;
                break;
            }
            target = mp_netgame_host_level * mp_netgame_level_scale;
            for (c = living_list.live_next; c != &living_list; c = c->live_next) {
                if (c->type != 7) {
                    c->level = (short)(int)(target + 0.5f);
                    c->level = max((short)lo, c->level);
                    c->level = min((short)hi, c->level);
                    c->scale_stats_to_level();
                }
            }
        }
    }
    if (game_flags & GF_MULTIPLAYER) {
        if (mp_netgame_level == 16) {
            fx_disable_by_effect_name("wc-trap1.vfx");
            fx_disable_by_effect_name("wc-trap2.vfx");
            fx_disable_by_effect_name("wc-ptrap.vfx");
        } else if (mp_netgame_level == 0x15)
            titus_combat_enabled = true;
        else if (mp_netgame_level == 0x13)
            pyrul_combat_enabled = true;
        else if (mp_netgame_level == 0x19)
            machival_combat_enabled = true;
    }
}

// 0x4ccab0  Confidence: exact. The level music starts 900 ms after the parse; the triggers and the level's
// frame hook run while no cutscene (flag or script) plays.
void level_script_frame()
{
    level_ambient_sounds_frame();
    if (!(game_flags & GF_CUTSCENE) && level_music_stop_timer.elapsed()) {
        if (level_music >= 0)
            music_play(level_music, true);
        else
            music_stop(200);
        level_music_stop_timer.invalidate();
    }
    level_script_entries_frame();
    if (!(game_flags & GF_CUTSCENE) && strlen(cutscene_script_table()) == 0) {
        level_check_triggers();
        if (level_script_frame_fn && Do_frame_callback)
            level_script_frame_fn();
    }
}

// 0x4ccb40  Confidence: exact.
void level_ambient_sounds_frame()
{
    vector3 vel(0.0f, 0.0f, 0.0f);
    int i;

    for (i = 0; i < level_ambient_count; i++)
        sound_emitter_update_position(level_ambients[i].emitter, &level_ambients[i].pos, &vel, 1.75f);
}

// 0x4ccba0  Confidence: exact (VC6 byte match) (the two `== true` tests). Each live script object advances to its next
// action when its character is free (not suspended, not in a cutscene, no pending goal) and the current
// action has completed; an object whose character is gone is freed.
void level_script_entries_frame()
{
    int i;

    for (i = 0; i < script_object_count; i++) {
        script_object *so = &script_objects[i];
        Character *c;

        if (so->end_mode == -1 || so->suspended)
            continue;
        c = living_from_handle(so->handle);
        if (!c) {
            so->end_mode = -1;
            continue;
        }
        if (c->ai_suspended() || c->cutscene_object >= 0)
            continue;
        if (script_character_has_pending_goal(c) == true)
            continue;
        if (so->timer.valid() == true && !so->timer.elapsed())
            continue;
        if (script_entry_check_done(so, c))
            script_entry_advance(so, c);
    }
}

// 0x4ccc50  Confidence: exact.
bool script_character_has_pending_goal(Character *c)
{
    bool r = c->ai.has_goal_without_flag20() == true;
    return r;
}

// 0x4ccc70  Confidence: exact. Removing the ambients by index while the table compacts frees only every
// other emitter (game bug, NOTES).
void level_script_leave()
{
    int i;

    if (!level_script_restarting) {
        for (i = 0; i < level_effect_count; i++)
            if (level_effects[i].handle != -1 && !level_effects[i].flag)
                fx_stop(level_effects[i].handle);
        for (i = 0; i < level_ambient_count; i++)
            level_ambient_remove(i);
        level_effect_count = 0;
    } else
        fx_level_restart();
    for (i = 0; i < MAX_SCRIPT_AVOID_SPLINES; i++)
        if (script_avoid_splines[i].key != -1)
            avoid_spline_free(script_avoid_splines[i].spline);
    if (level_script_leave_fn)
        level_script_leave_fn();
}

// 0x4ccd10  Confidence: exact. Once the exit is confirmed (game_frozen): the trigger's level becomes the
// current one with its script and entry point; the multiplayer host announces it.
void level_apply_pending_change()
{
    if (level_exit_trigger && game_frozen) {
        strcpy(level_prev_filename, level_filename);
        strcpy(level_filename, level_exit_trigger);
        strcpy(level_script_name, level_pending_exit_name);
        level_entry_point = level_exit_trigger_index;
        if (MP_IS_HOST()) {
            mp_netgame_level = level_table_find_id(level_script_find(level_filename));
            mp_change_level(level_entry_point);
        } else
            gameseq_set_state(4);
    }
}

// 0x4ccdf0  Confidence: exact.
void script_cutscene_done()
{
    if (!(game_flags & GF_CUTSCENE)) {
        level_script_frame_fn = cutscene_saved_frame_fn;
        if (cutscene_done_callback)
            cutscene_done_callback();
    }
}

// 0x4cce10 static_init_ccmd_No_cutscenes (jmp). Confidence: exact.
// 0x4cce20 init_ccmd_No_cutscenes. Confidence: exact.
console_command ccmd_No_cutscenes("No_cutscenes", "Toggles No_cutscenes", cmd_No_cutscenes);   // 0x2490c90

// 0x4cce40  Confidence: exact.
void cmd_No_cutscenes()
{
    if (calling_a_command_function) {
        process_command(0xc1);
        if (arg_type & ARG_TRUE)
            No_cutscenes = true;
        else if (arg_type & ARG_FALSE)
            No_cutscenes = false;
        else if (arg_type & ARG_NONE)
            No_cutscenes ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it.\n",
                "No_cutscenes", "No_cutscenes");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "No_cutscenes", No_cutscenes ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x4ccf00  Confidence: exact. Nothing happens without a living party member; with No_cutscenes or in
// multiplayer the continuation runs at once; else the level's frame hook is parked until the cutscene ends.
void script_play_cutscene(const char *name, void (*done)())
{
    Player *p;

    for (p = party_list.next; p != &party_list; p = p->next) {
        Character *c = living_from_handle(p->handle);
        if (c && !(c->flags & CF_SUMMON) && !(c->flags & CF_DEAD))
            break;
    }
    if (p == &party_list)
        return;
    if (!No_cutscenes && !(game_flags & GF_MULTIPLAYER)) {
        cutscene_saved_frame_fn = level_script_frame_fn;
        level_script_frame_fn = script_cutscene_done;
        cutscene_done_callback = done;
        cutscene_play(name);
    } else if (done)
        done();
}

// 0x4ccf90  Confidence: exact.
void level_trigger_set_exit(const char *name, const char *script)
{
    int i;

    for (i = 0; i < level_trigger_count; i++)
        if (!_stricmp(name, level_triggers[i].name) && (!script || !_stricmp(script, level_triggers[i].script)))
            level_triggers[i].flags &= ~LEVEL_TRIGGER_SCRIPT_USE;
}

// 0x4ccff0  Confidence: exact.
void level_trigger_set_script_use(const char *name, const char *script)
{
    bool found = false;
    int i;

    for (i = 0; i < level_trigger_count; i++)
        if (!_stricmp(name, level_triggers[i].name) && (!script || !_stricmp(script, level_triggers[i].script))) {
            found = true;
            level_triggers[i].flags |= LEVEL_TRIGGER_SCRIPT_USE;
        }
    if (!found)
        printf("Loadarea or table file trigger missing!\n");
}

// 0x4cd060  Confidence: exact.
void level_trigger_set_exit_by_refname(const char *ref_name)
{
    int i;

    for (i = 0; i < level_trigger_count; i++)
        if (!_stricmp(ref_name, level_triggers[i].ref_name)) {
            level_triggers[i].flags &= ~LEVEL_TRIGGER_SCRIPT_USE;
            return;
        }
    printf("Loadarea or table file trigger missing!\n");
}

// 0x4cd0c0  Confidence: exact.
void level_trigger_set_script_use_by_refname(const char *ref_name)
{
    int i;

    for (i = 0; i < level_trigger_count; i++)
        if (!_stricmp(ref_name, level_triggers[i].ref_name)) {
            level_triggers[i].flags |= LEVEL_TRIGGER_SCRIPT_USE;
            return;
        }
    printf("Loadarea or table file trigger missing!\n");
}

// 0x4cd120  Confidence: likely (the bool is kept in AL across the loop, so it is a local; the function has
// no result: the caller ignores EAX). The boss / sound triggers (id != 0) only: a radius trigger fires once
// (LEVEL_TRIGGER_TRIPPED), a spline trigger every time it is crossed.
void level_trigger_trip(obj *o, int *out_id, int *out_index)
{
    bool tripped = false;
    int i;

    for (i = 0; i < level_trigger_count; i++) {
        if (level_triggers[i].flags & LEVEL_TRIGGER_SCRIPT_USE)
            continue;
        if ((game_flags & GF_MULTIPLAYER) && !(level_triggers[i].flags & LEVEL_TRIGGER_MULTI))
            continue;
        if ((game_flags & GF_SINGLE_PLAYER) && (level_triggers[i].flags & LEVEL_TRIGGER_MULTI))
            continue;
        if (level_triggers[i].id == 0)
            continue;
        switch (level_triggers[i].type) {
        case 0:
            tripped = segments_intersect_xz(&o->last_pos, &o->pos, &level_triggers[i].path->points[0],
                                            &level_triggers[i].path->points[1]);
            break;
        case 1:
            if ((level_triggers[i].flags & LEVEL_TRIGGER_ON) && !(level_triggers[i].flags & LEVEL_TRIGGER_TRIPPED))
            {
                float d = distance_squared(&level_triggers[i].nav->pos, &o->pos);
                tripped = d <= level_triggers[i].radius * level_triggers[i].radius;
            }
            break;
        }
        if (tripped) {
            *out_id = level_triggers[i].id;
            *out_index = level_triggers[i].index;
            sprintf(string_buffer, "Tripped trigger:  %s\n", level_triggers[i].name);
            if (level_triggers[i].flags & LEVEL_TRIGGER_ON)
                level_triggers[i].flags |= LEVEL_TRIGGER_TRIPPED;
            return;
        }
    }
    *out_id = -1;
    *out_index = -1;
}

// 0x4cd250  Confidence: exact.
void level_ambient_add(int sound, const vector3 *pos, int delay_ms)
{
    if (level_ambient_count < 0x80) {
        level_ambients[level_ambient_count].emitter = sound_create_emitter(sound, 100, delay_ms);
        level_ambients[level_ambient_count].pos = *pos;
        if (level_ambients[level_ambient_count].emitter != -1)
            level_ambient_count++;
    }
}

// 0x4cd2c0  Confidence: exact.
void level_ambient_remove(int index)
{
    int i;

    if (level_ambients[index].emitter >= 0)
        sound_emitter_delete(level_ambients[index].emitter, 0);
    for (i = index; i < level_ambient_count - 1; i++)
        level_ambients[i] = level_ambients[i + 1];
    level_ambient_count--;
}

// 0x4cd330  Confidence: exact.
int level_ambient_find_at(const vector3 *pos)
{
    int i;

    for (i = 0; i < level_ambient_count; i++) {
        vector3 *a = &level_ambients[i].pos;
        if (a->x == pos->x && a->y == pos->y && a->z == pos->z)
            return i;
    }
    return -1;
}

// 0x4cd390  Confidence: likely (two copies of the record set-up: the reused slot and the appended one).
void lc_entry_add(lobj *object)
{
    int i;

    for (i = 0; i < lc_entry_count; i++)
        if (lc_entries[i].object == NULL) {
            lc_entries[i].object = object;
            strcpy(lc_entries[i].name, "");
            lc_entries[i].flags = 0;
            lc_entries[i].gold = 0;
            return;
        }
    lc_entries[lc_entry_count].object = object;
    strcpy(lc_entries[lc_entry_count].name, "");
    lc_entries[lc_entry_count].flags = 0;
    lc_entries[lc_entry_count++].gold = 0;
}

// 0x4cd470  Confidence: exact.
void lc_entry_clear(lobj *object)
{
    int i;

    for (i = 0; i < lc_entry_count; i++)
        if (lc_entries[i].object == object) {
            lc_entries[i].object = NULL;
            return;
        }
}

// 0x4cd4b0  Confidence: exact.
bool lc_entry_exists(lobj *object)
{
    int i;

    for (i = 0; i < lc_entry_count; i++)
        if (lc_entries[i].object == object)
            return true;
    return false;
}

// 0x4cd4e0  Confidence: exact.
void lc_entry_mark_used(int index)
{
    lc_entries[index].flags |= 1;
    if (lc_entries[index].object)
        savegame_state.two_stage_set(true, false, lc_entries[index].object);
}

// 0x4cd520  Confidence: exact.
int lc_find_by_name(const char *name)
{
    int i;

    for (i = 0; i < lc_entry_count; i++)
        if (lc_entries[i].object && !_stricmp(lc_entries[i].object->name, name))
            return i;
    return -1;
}

// 0x4cd570  Confidence: exact (VC6 byte match) (the buffer is 0x44 bytes in the frame, two strncpy / strncat of 0x40).
int lc_find_by_hash(unsigned int hash)
{
    char name[0x44];
    int i;

    for (i = 0; i < lc_entry_count; i++) {
        if (lc_entries[i].object == NULL)
            continue;
        strncpy(name, level_script_name, 0x40);
        strncat(name, lc_entries[i].object->name, 0x40);
        if ((unsigned int)name_hash_nocase(name) == hash)
            return i;
    }
    return -1;
}

// 0x4cd5f0  Confidence: likely (one name buffer shared by the file name and the door names; the File object
// is constructed and never used). <script>_script.tbl: #Town, #Navpoints, #Objects, #Doors (sounds, lock,
// key and the saved open state of each door), #Clicks (the lc entries); returns TextFile::Open's error.
int level_script_parse_table()
{
    File f;
    TextFile tf;
    char buf[256];
    char name[256];
    int rc, i;

    level_script_stringpool.Reset();
    str_copy_append_32(buf, level_script_name, "_script.tbl");
    rc = tf.Open(buf, path_scripts);
    if (rc)
        return rc;
    level_is_town = tf.Accept("#Town") != 0;
    if (game_flags & GF_MULTIPLAYER)
        level_is_town = false;
    tf.Expect("#Navpoints");
    if (tf.Accept("$Num navpoints:"))
        tf.ParseInt();
    while (!tf.Accept("#Objects"))
        script_parse_navpoints(&tf);
    script_object_count = 0;
    while (tf.Peek("$Name:"))
        script_parse_objects(&tf, 1);
    if (tf.Accept("#Doors")) {
        while (tf.Accept("$Door:")) {
            lobj *l = NULL;
            door_info *d;
            bool locked, open, saved_locked;
            float lock;
            char key[256];

            tf.ParseString(buf, 0x100, '"', '"');
            char open_sound[64] = "";
            float open_a = 0.0f, open_b = 0.0f, open_c = 0.1f;
            if (tf.Accept("+Open sound:")) {
                tf.ParseString(open_sound, 0x40, '"', '"');
                open_a = tf.ParseFloat();
                open_b = tf.ParseFloat();
                open_c = tf.ParseFloat();
                if (open_c == 0.1f)
                    open_c = 1.0f;
            }
            char close_sound[64] = "";
            float close_a = 0.0f, close_b = 0.0f, close_c = 0.1f;
            if (tf.Accept("+Close sound:")) {
                tf.ParseString(close_sound, 0x40, '"', '"');
                close_a = tf.ParseFloat();
                close_b = tf.ParseFloat();
                close_c = tf.ParseFloat();
                if (close_c == 0.1f)
                    close_c = 1.0f;
            }
            locked = tf.Accept("+Locked:");
            strcpy(key, "");
            lock = 0.0f;
            if (locked) {
                lock = tf.ParseFloat();
                if (tf.Accept("+Key:"))
                    tf.ParseString(key, 0x100, '"', '"');
            }
            for (i = 0; i < num_lobjs; i++) {
                l = &lobjs[i];
                if (!strcmp(buf, l->name))
                    break;
            }
            d = door_from_lobj(l);
            d->state = (d->state & ~8) | ((locked ? 1 : 0) << 3);
            if (locked && strlen(key))
                d->key = &item_types[item_type_find(key)];
            savegame_state.two_stage_get(&open, &saved_locked, l);
            if (open) {
                l->pos = d->open_pos;
                d->open_orient.to_matrix_b(&l->orient);
            } else {
                l->pos = d->closed_pos;
                d->closed_orient.to_matrix_b(&l->orient);
            }
            d->lock = lock;
            if (strlen(open_sound)) {
                d->open_sound.name = level_script_stringpool.AddUnique(open_sound);
                d->open_sound.params[1] = (ubyte)open_a;
                d->open_sound.params[0] = (ubyte)(open_b * 255.0f);
                d->open_sound.params[2] = (ubyte)open_c;
            } else {
                d->open_sound.name = NULL;
                d->open_sound.handle = -1;
            }
            if (strlen(close_sound)) {
                d->close_sound.name = level_script_stringpool.AddUnique(close_sound);
                d->close_sound.params[1] = (ubyte)close_a;
                d->close_sound.params[0] = (ubyte)(close_b * 255.0f);
                d->close_sound.params[2] = (ubyte)close_c;
            } else {
                d->close_sound.name = NULL;
                d->close_sound.handle = -1;
            }
        }
    }
    lc_entry_count = 0;
    if (tf.Accept("#Clicks")) {
        while (tf.Accept("$Name:")) {
            int index;

            tf.ParseString(name, 0x100, '"', '"');
            index = lobj_find_by_name(name);
            lc_entries[lc_entry_count].object = lobj_from_index(index < 0 ? 0 : index);
            lc_entries[lc_entry_count].flags = 0;
            if (tf.Accept("+Give:"))
                lc_entries[lc_entry_count].gold = tf.ParseInt();
            else
                lc_entries[lc_entry_count].gold = 0;
            if (tf.Accept("+Messagebox:")) {
                tf.ParseString(lc_entries[lc_entry_count].name, 0xff, '"', '"');
                localize_remap_charset(lc_entries[lc_entry_count].name, 1);
            } else
                strcpy(lc_entries[lc_entry_count].name, "");
            lc_entry_count++;
        }
    }
    tf.Expect("#End");
    tf.Close();
    for (i = 0; i < num_doors; i++) {
        if (doors[i].open_sound.name)
            sound_ref_load(&doors[i].open_sound, true);
        if (doors[i].close_sound.name)
            sound_ref_load(&doors[i].close_sound, true);
    }
    sprintf(buf, "Level script string pool usage: %d bytes\n", level_script_stringpool.GetLength());
    return 0;
}

// 0x4cddc0  Confidence: exact.
void script_object_resume(script_object *so)
{
    script_entry_start(so, living_from_handle(so->handle));
    so->suspended = 0;
}

// 0x4cddf0  Confidence: exact. A living or dead character of that name.
int script_object_find(const char *name)
{
    int i;

    for (i = 0; i < script_object_count; i++) {
        obj *o = obj_from_handle(script_objects[i].handle);
        if (o && (o->is_dead_type() || o->is_living()) && !_stricmp(((Character *)o)->get_name(), name))
            return i;
    }
    return -1;
}

// 0x4cde70  Confidence: exact.
Character *script_object_find_character(const char *name)
{
    int i;

    for (i = 0; i < script_object_count; i++) {
        obj *o = obj_from_handle(script_objects[i].handle);
        if (o && (o->is_dead_type() || o->is_living()) && !_stricmp(((Character *)o)->get_name(), name))
            return (Character *)o;
    }
    return NULL;
}

// 0x4cdef0  Confidence: exact.
void script_object_say(int index, const char *text, float time)
{
    if (index >= 0) {
        Character *c = living_from_handle(script_objects[index].handle);
        if (c)
            c->say(text, time);
    }
}

// 0x4cdf30  Confidence: exact. In solo mode (GF_SOLO_MODE) only the current character speaks.
void script_party_member_say(const char *name, const char *text, float time)
{
    Player *p;

    for (p = party_list.next; p != &party_list; p = p->next) {
        Character *c = (Character *)obj_from_handle(p->handle);
        if ((c == cur_character || !(game_flags & GF_SOLO_MODE)) && c && !_stricmp(c->get_name(), name))
            c->say(text, time);
    }
}

// 0x4cdfa0  Confidence: exact.
void script_party_remove_item(const char *name)
{
    int type = item_type_find(name);
    int slot = party_inventory_find_type(&item_types[type]);

    if (slot >= 0 && type >= 0) {
        if (party_inventory[slot].count > 1)
            party_inventory[slot].count--;
        else
            party_inventory_remove(slot, 1, 1);
    }
}

// 0x4cdff0  Confidence: exact. Eleh's prison: Joseph's armour shows the alternate texture; with strip set
// he gets a Short Sword and the party loses his weapon, the slot 6 / 1 / 2 items (to the inventory) and the
// four element rings.
void script_eleh_strip_party_gear(bool strip)
{
    Player *p;

    for (p = party_list.next; p != &party_list; p = p->next) {
        Character *c = (Character *)obj_from_handle(p->handle);
        item_info *it;

        if (!c || _stricmp(c->get_name(), "Joseph"))
            continue;
        c->set_armour_alt_texture(strip, false);
        if (!strip)
            continue;
        item_info *old1, *old2;
        character_equip_item(&old1, &old2, c, 5, item_create_by_name("Short Sword"));
        if (old1)
            party_inventory_add_item(old1, 1, 1);
        if (old2)
            party_inventory_add_item(old2, 1, 1);
        old1 = character_unequip_slot(c, 6, 1);
        if (old1)
            party_inventory_add_item(old1, 1, 1);
        old1 = character_unequip_slot(c, 1, 1);
        if (old1)
            party_inventory_add_item(old1, 1, 1);
        old1 = character_unequip_slot(c, 2, 1);
        if (old1)
            party_inventory_add_item(old1, 1, 1);
    }
    if (strip) {
        script_party_remove_item("Ring of Darkness");
        script_party_remove_item("Ring of Light");
        script_party_remove_item("Ring of Fire");
        script_party_remove_item("Ring of Stone");
    }
}

// 0x4ce140  Confidence: exact.
void script_object_set_disabled(int index, bool disabled)
{
    Character *c;

    if (index < 0)
        return;
    c = living_from_handle(script_objects[index].handle);
    if (!c)
        return;
    if (disabled) {
        c->set_flag_2_4(-1);
        c->set_ai_active(false);
        c->flags |= 0x20;
    } else {
        c->flags &= ~0x20;
        c->set_ai_active(true);
        c->clear_flag_2_4();
    }
}

// 0x4ce1c0  Confidence: exact.
void script_npc_set_disabled(const char *name, bool disabled)
{
    int index = script_object_find(name);

    if (index >= 0)
        script_object_set_disabled(index, disabled);
}

// 0x4ce1e0  Confidence: exact.
bool script_object_is_type5(int obj)
{
    Character *c;

    if (obj < 0)
        return false;
    c = living_from_handle(script_objects[obj].handle);
    if (!c)
        return false;
    return c->type == 5;
}

// 0x4ce220  Confidence: exact. Ends the object's "wait for go": the first waiting action from the current
// one on (4 / 0x19: the AI goal 0xd / 2 is removed and the script advances; 0xb: its flag is set).
void script_object_receive_go(int index)
{
    script_object *so;
    Character *c;
    int i;

    if (index < 0)
        return;
    so = &script_objects[index];
    c = living_from_handle(so->handle);
    if (!c)
        return;
    if (c->ai.flags & 0x40) {
        c->frametime = reported_frametime;
        c->ai.flags &= ~0x40;
    }
    for (i = so->cur_action; i < so->num_actions; i++) {
        script_action *a = &so->actions[i];

        if (a->type == 4 || a->type == 0x19) {
            Character *sc = living_from_handle(so->handle);
            if (sc) {
                sc->ai.goal_remove(sc->ai.goal_find(1, a->type == 0x19 ? 2 : 0xd, 0, 0, -1, 0, 0, 0x6c));
                script_entry_advance(so, sc);
                return;
            }
        } else if (a->type == SCRIPT_ACTION_WAIT_GO) {
            a->arg0c = 1;
            return;
        }
    }
}

// 0x4ce320  Confidence: exact.
void script_npc_send_go(const char *name)
{
    int index = script_object_find(name);

    if (index >= 0)
        script_object_receive_go(index);
}

// 0x4ce340  Confidence: exact. Only the type and the label of the action are set.
void script_object_goto_label(int index, int label)
{
    script_action a;
    Character *c;

    if (index < 0)
        return;
    a.type = SCRIPT_ACTION_GOTO;
    a.arg0c = label;
    c = living_from_handle(script_objects[index].handle);
    if (c)
        script_action_goto(c, &a, &script_objects[index]);
}

// 0x4ce3a0  Confidence: exact.
void script_npc_goto_label(const char *name, int label)
{
    int index = script_object_find(name);

    if (index >= 0)
        script_object_goto_label(index, label);
}

// 0x4ce3c0  Confidence: exact.
bool script_check_navpoint(int index, const char *navpoint, float dist)
{
    vector3 pos;
    matrix orient;
    Character *c;

    if (index < 0)
        return false;
    c = living_from_handle(script_objects[index].handle);
    if (!c)
        return false;
    if (navpoint_get_position(navpoint, &pos, &orient)) {
        if ((c->pos - pos).len() < dist)
            return true;
    } else
        printf("WARNING: Script trying to check navpoint that doesn't exist!\n");
    return false;
}

// 0x4ce460  Confidence: exact.
bool script_object_near_player(int index, float dist)
{
    Character *c;

    if (index < 0)
        return false;
    c = living_from_handle(script_objects[index].handle);
    if (!c || !cur_character)
        return false;
    if ((c->pos - cur_character->pos).len() < dist)
        return true;
    return false;
}

// 0x4ce4d0  Confidence: exact.
bool script_player_near_navpoint(const char *navpoint, float dist)
{
    vector3 pos;
    matrix orient;

    if (!cur_character)
        return false;
    if (!navpoint_get_position(navpoint, &pos, &orient))
        return false;
    if ((cur_character->pos - pos).len() < dist)
        return true;
    return false;
}

// 0x4ce540  Confidence: exact.
bool script_npc_near_player(const char *name, float dist)
{
    int index = script_object_find(name);

    if (index >= 0)
        return script_object_near_player(index, dist);
    return false;
}

// 0x4ce570  Confidence: exact.
bool script_object_is_dead(int index)
{
    Character *c;

    if (index < 0)
        return false;
    c = (Character *)obj_from_handle(script_objects[index].handle);
    if (!c)
        return true;
    if (c->flags & CF_DEAD)
        return true;
    if (c->type == 9 && c->hp <= 0.0f)
        return true;
    return false;
}

// 0x4ce5d0  Confidence: exact.
void script_object_drop_loot(int index)
{
    Character *c;

    if (index < 0)
        return;
    c = (Character *)obj_from_handle(script_objects[index].handle);
    if (c)
        c->monster_drop_loot();
}

// 0x4ce600  Confidence: exact.
bool script_npc_is_dead(const char *name)
{
    int index = script_object_find(name);

    if (index >= 0)
        return script_object_is_dead(index);
    return false;
}

// 0x4ce620  Confidence: exact.
void script_npc_drop_loot(const char *name)
{
    int index = script_object_find(name);

    if (index >= 0)
        script_object_drop_loot(index);
}

// 0x4ce640  Confidence: exact (VC6 byte match) (conversation_is_idle is called twice in a row; an expanded macro?). The
// name is the character's, else its char_info's; game_flags 0x100 is set for the conversation.
void script_object_talk(int index)
{
    Character *c;
    const char *name;

    if (!conversation_active.is_idle() || !conversation_active.is_idle())
        return;
    if (index < 0)
        return;
    c = living_from_handle(script_objects[index].handle);
    if (!c)
        return;
    name = c->get_name();
    if (!name || !*name)
        name = c->info->name;
    player_input[0] = 0;
    game_flags |= 0x100;
    conversation_begin(name, c);
}

// 0x4ce6d0  Confidence: exact.
void script_npc_talk(const char *name)
{
    int index = script_object_find(name);

    if (index >= 0)
        script_object_talk(index);
}

// 0x4ce6f0  Confidence: likely (the dot product's term order follows the binary; the ray set-up). Not while
// cur_character is in AI mode 9; beyond the guard's view range (scaled by the light level when
// cur_character has flag 0x400) never; beyond its detection range only inside the view cone and when the
// guard is not blind; status 8 (a positive value) hides; flag 0x400 or AI mode (3, 0x11) needs a narrower
// cone (0.71); finally a clear line of sight from the guard's mid height.
bool script_guard_sees_player(Character *guard, float cone)
{
    float dist, dot, view;
    vector3 dir;

    if (cur_character->ai.is_mode(9, -1) == true)
        return false;
    dist = distance(&guard->pos, &cur_character->pos);
    dir = cur_character->pos - guard->pos;
    dir.normalize();
    dot = dir.z * guard->orient.fvec.z + dir.y * guard->orient.fvec.y + dir.x * guard->orient.fvec.x;
    view = guard->info->fov_range;
    if (cur_character->flags & 0x400)
        view *= cur_character->light_level;
    if (dist > view)
        return false;
    if (dist > guard->info->detection_range) {
        if (dot < cone)
            return false;
        if (guard->has_status(STATUS_BLIND))
            return false;
    }
    if (cur_character->has_status(8)) {
        if (cur_character->status_value(8) > 0.0f)
            return false;
        if (dist > guard->info->detection_range)
            return false;
    }
    if ((cur_character->flags & 0x400) || cur_character->ai.is_mode(3, 0x11)) {
        if (dot < 0.71f)
            return false;
    }
    lobj_ray ray;
    lobj_hit hit;
    ray.start = guard->pos;
    ray.start.y += guard->size_height() * 0.5f;
    ray.dir = cur_character->pos - guard->pos;
    ray._unk18 = 0.0f;
    ray.flags = 0;
    ray.require_flags = 0;
    ray.exclude_flags = 0;
    lobj_raycast_all(&ray, &hit);
    return hit.num_hits <= 0;
}

// 0x4ce950  Confidence: exact.
shopkeeper *level_shopkeeper_find(const char *name)
{
    int i;

    for (i = 0; i < level_shopkeeper_count; i++)
        if (!_stricmp(level_shopkeepers[i].name, name))
            return &level_shopkeepers[i];
    return NULL;
}

// 0x4ce9a0  Confidence: exact. Identified items only; the equipment loop covers slots 0-8 (not 9).
int party_count_item_by_name(const char *name)
{
    int count = 0;
    int type, slot, i;
    Player *p;

    type = item_type_find(name);
    if (type < 0)
        return 0;
    slot = party_inventory_find_type(&item_types[type]);
    if (slot >= 0 && item_is_identified(party_inventory[slot].item))
        count = party_inventory[slot].count;
    for (p = party_list.next; p != &party_list; p = p->next) {
        Character *c = living_from_handle(p->handle);
        if (!c)
            continue;
        for (i = 0; i < 9; i++) {
            item_info *it = c->equipment[i];
            if (it && it->type - item_types == type && item_is_identified(it))
                count++;
        }
    }
    return count;
}

// 0x4cea70  Confidence: exact.
bool script_player_has_equipped(const char *item)
{
    int type = item_type_find(item);

    if (type < 0)
        return false;
    if (!cur_character)
        return false;
    return character_has_equipped_type(cur_character, &item_types[type]) ? true : false;
}

// 0x4ceab0  Confidence: exact.
void script_party_acquire_item(const char *name, int count)
{
    char text[256];

    party_inventory_add_by_name(name, count, -1, 1);
    const char *fmt = XSTR("Party acquired %s!", 569);
    sprintf(text, fmt, localize_item_name(name));
    hud_show_message(text);
}

// 0x4ceb00  Confidence: exact. Not the summon or hidden members.
void script_party_gain_xp(int xp)
{
    char text[256];
    Player *p;

    for (p = party_list.next; p != &party_list; p = p->next) {
        Character *c = living_from_handle(p->handle);
        if (!(c->flags & CF_SUMMON) && !(c->flags & CF_HIDDEN))
            c->add_xp(xp, 0, 0);
    }
    sprintf(text, XSTR("party gained %d XP", 570), xp);
    hud_show_message(text);
}

// 0x4ceb80  Confidence: exact.
void script_topic_toggle_by_item(const char *item, const char *speaker, const char *topic)
{
    if (party_count_item_by_name(item))
        conversation_db.set_topic_available(speaker, topic, false);
    else
        conversation_db.set_topic_available(speaker, topic, true);
}

// 0x4cebc0  Confidence: exact.
void script_topic_toggle_by_item_inv(const char *item, const char *speaker, const char *topic)
{
    if (party_count_item_by_name(item))
        conversation_db.set_topic_available(speaker, topic, true);
    else
        conversation_db.set_topic_available(speaker, topic, false);
}

// 0x4cec00  Confidence: likely (the skill loop). Every party member not above `level` restarts at level 1
// from its char_info stats and skills, with 10 skill points cap and none to spend, then gains the XP of
// `level`; its skill points then go round-robin over its progression table (+20 for Joseph, +10 for the
// others, up to 100) or, with Max_skills, every progression skill is set to 100. The round-robin loop never
// ends when points are left and no entry can take them (game bug, NOTES).
void party_set_level(int level)
{
    Player *p;
    int i, j;

    for (p = party_list.next; p != &party_list; p = p->next) {
        Character *c = (Character *)obj_from_handle(p->handle);
        char_info *info;

        if (c->flags & CF_SUMMON)
            continue;
        if (c->level > level)
            continue;
        info = c->info;
        c->level = 1;
        c->xp = 0;
        c->base_max_hp = info->max_hp;
        c->hp = info->hp;
        c->base_max_ap = info->max_ap;
        c->ap = info->ap;
        p->skill_points = 0;
        p->skill_cap = 10;
        for (i = 0; i < MAX_SKILLS; i++)
            c->skill[i] = c->info->skill[i];
        c->add_xp(xp_for_level(level), 1, 0);
        if (!Max_skills) {
            j = 0;
            while (p->skill_points > 0) {
                skill_progression *sp = &p->progression[j];
                if (c->level >= sp->level && c->skill[sp->skill] < 100) {
                    c->skill[sp->skill] += c->info->species == 0 ? 20 : 10;
                    p->skill_points--;
                }
                j = (j + 1) % p->num_progression;
            }
        } else {
            for (j = 0; j < p->num_progression; j++)
                c->skill[p->progression[j].skill] = 100;
        }
    }
}

// 0x4ced90  Confidence: exact.
int script_avoid_spline_find_free()
{
    int i;

    for (i = 0; i < MAX_SCRIPT_AVOID_SPLINES; i++)
        if (script_avoid_splines[i].key == -1)
            return i;
    return -1;
}

// 0x4cedb0  Confidence: exact.
void script_avoid_splines_reset()
{
    int i;

    for (i = 0; i < MAX_SCRIPT_AVOID_SPLINES; i++)
        script_avoid_splines[i].key = -1;
}

// 0x4cedd0  Confidence: exact.
bool script_avoid_spline_in_use(int key)
{
    int i;

    for (i = 0; i < MAX_SCRIPT_AVOID_SPLINES; i++)
        if (script_avoid_splines[i].key == key)
            return true;
    return false;
}

// 0x4cedf0  Confidence: exact. The in-use test is made twice with the same handle (the second was
// probably meant for the target), and the target check tests the handle, not the looked-up character
// (NOTES).
void script_alloc_avoid_spline(int handle, int target)
{
    int slot, spline;
    Character *c, *t;

    slot = script_avoid_spline_find_free();
    if (slot == -1) {
        printf("Ran out of level_script_avoid_spline slots\n");
        return;
    }
    c = living_from_handle(handle);
    if (!c)
        return;
    if (script_avoid_spline_in_use(handle))
        return;
    if (script_avoid_spline_in_use(handle))
        return;
    if (target == -1)
        target = cur_character->handle;
    t = living_from_handle(target);
    if (!target)
        return;
    spline = script_build_avoid_spline(c, t);
    if (spline != -1) {
        script_avoid_splines[slot].key = c->handle;
        script_avoid_splines[slot].spline = spline;
    }
}

// 0x4cee90  Confidence: exact.
void script_free_avoid_spline(int handle)
{
    int i;

    for (i = 0; i < MAX_SCRIPT_AVOID_SPLINES; i++)
        if (script_avoid_splines[i].key == handle) {
            avoid_spline_free(script_avoid_splines[i].spline);
            script_avoid_splines[i].key = -1;
        }
}
