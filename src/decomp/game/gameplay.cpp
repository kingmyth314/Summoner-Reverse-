// gameplay.cpp - Sum.exe v1.40, code 0x437660-0x43a32f (the cheat console commands, level load and
// unload, game state 4 (state_game_enter / state_game_frame / state_game_leave), party control, the
// loading screen and the help screen of state 5). Written from the Sum.exe disassembly (tools/asm.mjs fn;
// the jump tables and the .data / .rdata values read from Summoner/Sum.exe). Target: VC++ 6.0
// (cl 12.00.8966), /O2 like the other optimised units (no frame pointer, 0x90 padding, argument slots
// reused for locals). Confidence per function: exact = expect identical code, likely = same semantics,
// small layout risk, unsure = the source shape is a guess.
//
// Unit: strings.tbl id 622 ("// gameplay.cpp", "%s has joined the party!\n") is read by party_member_join
// (0x439d11). The unoptimised cutscene file ends with cmd_stop_cutscene (0x437650); gametarget.cpp starts
// with init_gametimer_60ab70_thunk (0x43a330). The static initialisers are entries 117-129 of the table.
//
// Data: .rdata 0x580df4-0x580e07 (the loading bar factors, the 0.75 party spacing; 25.0 at 0x580e08 has
// no reader in the unit). .data 0x593c34-0x5943ef: the loading bar tables, loading_screen_bm, four
// unread ints, then the literals in order of use, with world_mouse_target_handle (0x593f50) between
// level_load_layers' and game_render_frame's literals and the help tables (0x5940ac-0x59412f) after the
// console texts. .bss 0x609f40-0x60ab5f (game_camera 0x60a018, the level names and layer maps, the
// help window 0x60a670, state_loading_active 0x60ab5c). The .bss addresses are not in definition order
// (as in summoner.cpp); the definitions follow the initialiser order where there is one.
//
// Literals pooled with other units (/Gf): "Joseph" 0x593888, "Rosalind" 0x593840, "Jekhar" 0x593870,
// "Flece" 0x593858 (player.cpp), "masad" 0x5930c0, ".plr" 0x58a0e0, "got_ring_of_darkness" 0x5917f0,
// " " 0x58e0cc, the console macro texts 0x588c34-0x588c50 ("TRUE", "FALSE", "%s is %s\n", usage).
//
// Profiling: state_game_frame opens an EH frame whose only unwind actions are profile_label scopes
// (vsdk/os/profile.h; states 0-7, record 0x57c140), as in ai.cpp / pathfinding.cpp; the label texts are
// not in the binary, the names used here are ours (unsure).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vsdk/os/console.h"
#include "vsdk/os/timer.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/frametime.h"
#include "vsdk/os/memory.h"
#include "vsdk/os/profile.h"
#include "vsdk/os/key.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/vfile/file.h"
#include "vsdk/vfile/packfile/file_packfile.h"
#include "vsdk/parse/parse.h"
#include "vsdk/localize/localize.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/gr_font.h"
#include "vsdk/gr/directx/gr_direct3d.h"
#include "vsdk/snd/snd.h"
#include "vsdk/geom/vfx.h"
#include "vsdk/geom/effect_mem.h"
#include "vsdk/ca/character.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/ui/window.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/ai/ai.h"
#include "engine/ai/boss_ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/camera/camera.h"
#include "engine/effects/fx.h"
#include "engine/gamesound/gamesound.h"
#include "engine/rendering/levelrender.h"
#include "engine/s3d/s3d.h"
#include "engine/spells/spells.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/conversation.h"
#include "levelscripts/savegame.h"
#include "summoner/summoner.h"
#include "summoner/gameplay.h"
#include "summoner/gametarget.h"
#include "summoner/automap.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"
#include "summoner/multi/multisave.h"
#include "summoner/interface/interface.h"
#include "summoner/interface/new_hud.h"

#define OBJ_TYPE_PARTY 7              // obj +0xc of a party member (objects.h)
#define AI_DIRECT_PRIORITY 999999     // the goal priority of direct orders (ai.cpp)
#define LEVEL_INDEX_WORLDMAP 4        // level_script_table / cur_level_index of worldmap1

// ---- .data (0x593c34-0x593c77) ------------------------------------------------------------------------
// The loading bar per resolution_index (640, 800, 1024): {w, h}, then {x, y}.
int loading_bar_size[3][2] = {                                // 0x593c34
    { 320, 28 }, { 400, 35 }, { 514, 45 },
};
int loading_bar_pos[3][2] = {                                 // 0x593c4c
    { 160, 345 }, { 200, 431 }, { 256, 554 },
};
int loading_screen_bm = -1;                                   // 0x593c64
int gameplay_data_593c68[4] = { 1, 1, 1, 1 };                 // 0x593c68 no reader (name ours)

// ---- .bss without initialisers -------------------------------------------------------------------------
int  loading_screen_start_time;                               // 0x609f4c
bool state_game_level_ready;                                  // 0x609f58 (see state_game_enter)
int  level_load_num_steps;                                    // 0x609f5c
int  level_load_start_time;                                   // 0x609f88
bool loading_screen_active;                                   // 0x609fbc
bool Full_party;                                              // 0x609fe0
bool Dump_sound_usage;                                        // 0x609fe1
bool Give_all_items;                                          // 0x609fec
// level_load_files' phase times (timer_get(1000)), never read. Names ours.
int  level_load_time_609fe8;                                  // 0x609fe8 only zeroed
int  level_load_time_s3d;                                     // 0x609f7c
int  level_load_time_s3d_done;                                // 0x609f80
int  level_load_time_render;                                  // 0x609f6c
int  level_load_time_render_done;                             // 0x609fa4
int  level_load_time_surfaces;                                // 0x609f48
int  level_load_time_surfaces_done;                           // 0x609f40
int  level_load_time_pathfinding;                             // 0x609f50
int  level_load_time_pathfinding_done;                        // 0x609f54
int  level_load_time_script;                                  // 0x609fa8
int  level_load_time_script_done;                             // 0x609f84
int  level_load_time_page_in;                                 // 0x609fa0
int  level_load_time_page_in_done;                            // 0x609f9c
int  level_load_time_end;                                     // 0x609f44
char level_filename[32];                                      // 0x60a2c8
char level_script_name[32];                                   // 0x60a2e8
char level_prev_filename[32];                                 // 0x60a308
float level_session_time;                                     // 0x60a328
navpoint *level_player_starts[4];                             // 0x60a32c
int  level_entry_point;                                       // 0x60a33c
int  level_num_layers;                                        // 0x60a340
int  level_layer_maps[10];                                    // 0x60a344
int  level_layer_automap_index[40];                           // 0x60a36c
bool level_is_town;                                           // 0x60a40c (levelscripts/level_scripts.h)
int  gameplay_no_render_60a630;                               // 0x60a630 never written
float level_time;                                             // 0x60a654
bool move_target_decal_visible;                               // 0x60a664
bool Render_3d_obstacles;                                     // 0x60a665
int  state_loading_next_state;                                // 0x60a668
int  loading_help_page;                                       // 0x60a66c
int  state_loading_active;                                    // 0x60ab5c

void cmd_full_party();
void cmd_dump_sound_usage();
void cmd_add_xp();

// 0x437660 init_screen_rects_609fc0_thunk (jmp, static initialiser entry 117). Confidence: exact.
// 0x437670 init_screen_rects_609fc0: the dynamic initialiser of the array below. Confidence: likely (the
//          bar's four corners at 640 x 480 are (160, 345), (480, 345), (480, 373), (160, 373); the
//          constants are 0.25, 23/32, -0.5 and -7/120 (0x580df4-0x580e00), so the width and height were
//          written as negative factors; the evaluation order differs between [2] and [5]).
int loading_bar_corners[8] = {                                // 0x609fc0 never read
    (int)(gr_screen_width() * 0.25f),
    (int)(gr_screen_height() * 0.71875f),
    (int)(gr_screen_width() * 0.25f) - (int)(gr_screen_width() * -0.5f),
    (int)(gr_screen_height() * 0.71875f),
    (int)(gr_screen_width() * 0.25f) - (int)(gr_screen_width() * -0.5f),
    (int)(gr_screen_height() * 0.71875f) - (int)(gr_screen_height() * -0.058333334f),
    (int)(gr_screen_width() * 0.25f),
    (int)(gr_screen_height() * 0.71875f) - (int)(gr_screen_height() * -0.058333334f),
};

// 0x4377d0 init_ccmd_full_party_thunk (jmp, entry 118). Confidence: exact.
// 0x4377e0 init_ccmd_full_party (console_command constructor). Confidence: exact.
console_command ccmd_full_party("full_party", "Toggles Full_party", cmd_full_party);   // 0x609f70

// 0x437800  Confidence: exact (the console's bool command macro).
void cmd_full_party()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Full_party = true;
        else if (arg_type & ARG_FALSE)
            Full_party = false;
        else if (arg_type & ARG_NONE)
            Full_party ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it.\n",
                "full_party", "Full_party");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "full_party", Full_party ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x4378c0 init_ccmd_dump_sound_usage_thunk (jmp, entry 119). Confidence: exact.
// 0x4378d0 init_ccmd_dump_sound_usage (console_command constructor). Confidence: exact.
console_command ccmd_dump_sound_usage("dump_sound_usage", "Toggles Dump_sound_usage", cmd_dump_sound_usage);   // 0x609fb0

// 0x4378f0  Confidence: exact (the console's bool command macro). Nothing reads Dump_sound_usage.
void cmd_dump_sound_usage()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Dump_sound_usage = true;
        else if (arg_type & ARG_FALSE)
            Dump_sound_usage = false;
        else if (arg_type & ARG_NONE)
            Dump_sound_usage ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it.\n",
                "dump_sound_usage", "Dump_sound_usage");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "dump_sound_usage", Dump_sound_usage ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x4379b0 init_ccmd_add_xp_thunk (jmp, entry 120). Confidence: exact.
// 0x4379c0 init_ccmd_add_xp (console_command constructor). Confidence: exact.
console_command ccmd_add_xp("add_xp", "Adds experience points to currently selected character", cmd_add_xp);   // 0x609f90

// 0x4379e0  Confidence: exact.
void cmd_add_xp()
{
    if (calling_a_command_function) {
        process_command(ARG_INT);
        if (cur_character)
            cur_character->add_xp(command_argument_int, false, false);
        else
            print_to_console("Can't add xp!", 0);
    }
    if (doing_help_for_comand)
        print_to_console("Usage: add_xp <xp to add>", 0);
}

// 0x437a40  Confidence: likely (the local layout: TextFile at esp+0, file name +0x1d0, the seen table
// +0xd8, the name +0x58). Reads "<script>_script.tbl": "#Town" sets *is_town (never in multiplayer), then
// each "$Character:" under "#Objects" counts once (num_characters), and once more in num_models when the
// record has no alt_parent (a model of its own). Callers level_load (into level_is_town) and
// party_level_reset (a local).
void level_script_read_town_characters(const char *script_name, bool *is_town, int *num_characters,
                                       int *num_models)
{
    TextFile tf;
    char filename[256];
    char name[128];
    ubyte seen[256];

    sprintf(filename, "%s_script.tbl", script_name);
    if (tf.Open(filename, path_scripts)) {
        *is_town = false;
        *num_characters = 0;
        return;
    }
    *is_town = tf.Accept("#Town");
    if (game_flags & GF_MULTIPLAYER)
        *is_town = false;
    memset(seen, 0, sizeof(seen));
    *num_characters = 0;
    *num_models = 0;
    if (tf.SkipTo("#Objects", NULL) == true) {
        while (tf.SkipTo("$Character:", NULL)) {
            tf.Expect("$Character:");
            tf.ParseString(name, 0x7f, '"', '"');
            int index = charinfo_find_by_name(name);
            if (index >= 0 && !seen[index]) {
                (*num_characters)++;
                seen[index] = 1;
                if (char_infos[index].alt_parent == NULL)
                    (*num_models)++;
            }
        }
    }
    tf.Rewind();
    tf.Close();
}

// 0x437c20  Confidence: exact. Both arguments are unused (level_load passes the model count and 0).
void loading_screen_load_bitmap(int num_models, int unused)
{
    loading_screen_active = true;
    gr_tcache_flush();
    if (resolution_index == 2)
        loading_screen_bm = bm_load("Loading-Bkgrnd1024.tga", -1, -1);
    else
        loading_screen_bm = bm_load("Loading-Bkgrnd640.tga", -1, -1);
    gr_page_in(loading_screen_bm, 0);
    progress_set_callback(LoadingScreenDraw, level_load_num_steps, 0);
    loading_screen_start_time = timer_get(1000);
}

// 0x437c80  Confidence: exact. The progress callback: the background and the green bar, then a whole
// frame (flip, frame time, sound, and the network in a multiplayer game).
void LoadingScreenDraw(int step, int flags)
{
    if (step < 0 || flags != 0)
        return;
    gr_clear();
    if (loading_screen_bm != -1) {
        gr_set_color(255, 255, 255, 255);
        ui_bitmap_scaled(loading_screen_bm, 0, 0, gr_bitmap_wrap_mode);
    }
    float done = (float)step / level_load_num_steps;
    if (done >= 1.0f)
        done = 1.0f;
    gr_set_color_ptr(&ui_color_loading_bar);
    gr_rect(loading_bar_pos[resolution_index][0], loading_bar_pos[resolution_index][1],
            (int)(loading_bar_size[resolution_index][0] * done), loading_bar_size[resolution_index][1],
            gr_rect_mode, 1.0f);
    gr_flip();
    FrameDone();
    snd_do_frame();
    if (game_flags & GF_MULTIPLAYER)
        multi_do_frame();
}

// 0x437d70  Confidence: exact. The three reports are only formatted into string_buffer (never printed).
// The loading screen bitmap is not released here: it was loaded after bm_set_mark, so the next
// level_unload's bm_release_from_mark frees it.
void level_load_report()
{
    LoadingScreenDraw(level_load_num_steps, 0);
    int ms = timer_get(1000) - loading_screen_start_time;
    int real_count = progress_set_callback(NULL, -1, 0);
    sprintf(string_buffer, "========== ENDING LOAD OF %s =======\n", level_filename);
    sprintf(string_buffer, "Real count = %d Estimated count = %d\n", real_count, level_load_num_steps);
    sprintf(string_buffer, "Time took: %d\n", ms);
    loading_screen_active = false;
    loading_screen_bm = -1;
}

// 0x437e00  Confidence: exact.
void level_unload(bool leaving_game)
{
    levelrender_level_unload();
    pathfinding_free_graph();
    party_level_reset(leaving_game);
    level_free_s3d(&level_s3d);
    effects_release_level_memory();
    printf("Releasing from mark!\n");
    bm_release_from_mark();
    gr_tcache_flush();
    snd_level_release();
}

// 0x437e50  Confidence: likely (living_from_handle is called twice for a party object: the second call
// is written as its own variable). Leaving the game: every party record and every object goes. A level
// change: the objects go except the party members, whose effects are cleared and AI reset; a summon
// (CF_SUMMON) goes too, after giving back the borrowed HP. In a town (or the world map) the party is
// then revived and healed.
void party_level_reset(bool leaving_game)
{
    if (leaving_game == true) {
        party_remove_all();
        flece_joined = false;
        jekhar_joined = false;
        rosalind_joined = false;
        while (obj_active_list.next != &obj_active_list)
            obj_active_list.next->deinit();
        cur_character = NULL;
        cur_summon = NULL;
        summon_player = NULL;
        return;
    }

    obj *o, *next;
    for (o = obj_active_list.next, next = o->next; o != &obj_active_list; o = next, next = next->next) {
        Character *c = living_from_handle(o->handle);
        if (o->type == OBJ_TYPE_PARTY) {
            Character *member = living_from_handle(o->handle);
            member->clear_all_effects();
            if (!(member->flags & CF_SUMMON)) {
                member->ai.reset_party_member();
                continue;
            }
        }
        if (c != NULL && (c->flags & CF_SUMMON)) {
            if ((game_flags & GF_MULTIPLAYER) && c == cur_character)
                party_set_leader(mp_local_player->object);
            c->summon_return_lent_max_hp();
        }
        o->deinit();
    }

    bool is_town;
    int num_characters, num_models;
    level_script_read_town_characters(level_script_name, &is_town, &num_characters, &num_models);
    if (!is_town && !_stricmp(level_filename, level_script_table[LEVEL_INDEX_WORLDMAP].name))
        is_town = true;
    if (is_town) {
        for (Player *p = party_list.next; p != &party_list; p = p->next) {
            Character *c = living_from_handle(p->handle);
            if (c->flags & CF_DEAD)
                c->resurrect();
            c->hp = (float)c->get_max_stat(0);
            c->ap = (short)c->get_max_stat(1);
            c->flags &= ~(CF_SUMMONLIKE | 0x400);
            c->clear_all_effects();
        }
    }
    cur_summon = NULL;
    summon_player = NULL;
}

void cmd_Give_all_items();

// 0x438040 init_ccmd_Give_all_items_thunk (jmp, entry 121). Confidence: exact.
// 0x438050 init_ccmd_Give_all_items (console_command constructor). Confidence: exact.
console_command ccmd_Give_all_items("Give_all_items", "Toggles Give_all_items", cmd_Give_all_items);   // 0x609f60

// 0x438070  Confidence: exact (the console's bool command macro). Nothing reads Give_all_items.
void cmd_Give_all_items()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Give_all_items = true;
        else if (arg_type & ARG_FALSE)
            Give_all_items = false;
        else if (arg_type & ARG_NONE)
            Give_all_items ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it.\n",
                "Give_all_items", "Give_all_items");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "Give_all_items", Give_all_items ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x438130  Confidence: likely (the switch: jump table 0x438554 / index table 0x438560; the two failure
// tails are one block in the binary, written twice here). State 4 enter. Coming from state 2 or 14 (a
// new game) the AI groups, the party inventory and level_time are reset and the camera snaps; coming back
// from state 5 (help) or 6 nothing is done. state_game_level_ready is set only on the failure paths
// (level_load failed or the pending savegame did not load), so state_game_leave saves the conversation
// state and pauses only when the level did load: the sum.symbols name has the meaning inverted.
void state_game_enter()
{
    bool new_game;
    bool loaded_save;

    state_game_level_ready = false;
    switch (gameseq_get_prev_state()) {
    case 2:
    case 14:
        new_game = true;
        ai_groups_reset();
        party_inventory_clear();
        level_time = 0.0f;
        break;
    case 5:
    case 6:
        return;
    default:
        new_game = false;
        break;
    }
    cursor_visible = false;
    cursor_set_type(cursor_walk_bm);
    move_target_set_bitmap(cursor_spot_walkto_bm);
    vfile_raise_access_level();
    living_entity_level_start();
    containers_reset();
    fx_level_enter();
    spells_reset_runtime();
    ai_groups_restart_timers();
    boss_rider_sound_timer_reset();
    if (!level_load()) {
        if (game_flags & GF_MULTIPLAYER)
            mp_player_disconnected(4, 0);
        else
            gameseq_set_state(1);
        state_game_level_ready = true;
        vfile_lower_access_level();
        input_flush();
        return;
    }
    automap_find_map_corners();
    level_session_time = 0.0f;
    combat_mode = false;
    loaded_save = false;
    if (savegame_state.pending_load_length()) {
        if (!savegame_state.load_pending()) {
            gameseq_set_state(1);
            state_game_level_ready = true;
            vfile_lower_access_level();
            input_flush();
            return;
        }
        loaded_save = true;
    } else {
        savegame_state.read_automap_labels();
    }
    level_script_init(!loaded_save);
    if (!loaded_save) {
        if (game_flags & GF_MULTIPLAYER) {
            for (mp_player *p = mp_player_list.next; p != &mp_player_list; p = p->next)
                living_from_handle(p->object->handle)->place_at_party_start(p->id);
        } else {
            int slot = 0;
            for (Player *p = party_list.next; p != &party_list; p = p->next)
                living_from_handle(p->handle)->place_at_party_start(slot++);
        }
        if ((game_flags & GF_SINGLE_PLAYER) && party_inventory_count <= 0) {
            party_inventory_add_by_name("Health Tonic", 3, -1, 1);
            if (cheat_gold_5fbf19)
                party_gold = 9999900;
        }
    }
    if (US_open_demo) {
        party_gold = 100;
        party_inventory_add_item(item_create_by_name("katana"), 1, 1);
        party_inventory_add_item(item_create_by_name("wakasashi"), 1, 1);
        item_info *item = item_create_by_name("icy dagger");
        party_inventory_add_item(item, 1, 1);
        item->type->flags &= ~ITF_NOT_IDENTIFIED;
        item = item_create_by_name("sword of spirits");
        party_inventory_add_item(item, 1, 1);
        item->type->flags &= ~ITF_NOT_IDENTIFIED;
        party_inventory_add_item(item_create_by_name("heavy boots"), 1, 1);
        party_inventory_add_item(item_create_by_name("steel gauntlets"), 1, 1);
        party_inventory_add_item(item_create_by_name("studded boots"), 1, 1);
        party_inventory_add_item(item_create_by_name("studded leather"), 1, 1);
        party_inventory_add_item(item_create_by_name("studded pants"), 1, 1);
        party_inventory_add_item(item_create_by_name("Ring of Fire"), 1, 1);
        party_inventory_add_item(item_create_by_name("Ring of Darkness"), 1, 1);
        savegame_state.set_flag(0x18, "got_ring_of_darkness", true);
        savegame_state.set_int(4, "fire_ring_xp", 80000);
    }
    conversation_active.reset_state();
    if (cur_level_index != -1)
        conversation_active.load_for_level(level_filename, level_script_name);
    level_script_event_flag_clear();
    items_level_load();
    if (!loaded_save && cur_character)
        camera_reset_to_player(new_game);
    game_pause_count_dec();
    if (game_flags & GF_MULTIPLAYER)
        mp_level_start();
    if (game_flags & GF_ESCAPE_CUTSCENE)
        cutscene_play_escape();
    party_leader->make_leader();
    vfile_lower_access_level();
    input_flush();
    game_flags = (game_flags & ~GF_PARTY_DEAD) | GF_LEADER_SOUND;
    if (!loaded_save && cur_level_index != -1)
        savegame_state.inc_visit_count(cur_level_index);
    interface_load_all();
    level_load_report();
    if (game_flags & GF_MULTIPLAYER) {
        mp_set_local_status(5);
        if ((game_flags & GF_MULTIPLAYER) && mp_local_player && !(mp_local_player->flags & 1))
            multi_process_client_status(mp_local_player, 1);
    }
}

// 0x438570  Confidence: likely (the eight counter stores are in the binary's order; the comparison
// `== true` gives the cmp al, 1 / sete of 0x438683). Adds the level packfiles when they are not on the
// hard disk (asking for disc 2 when levelm.vpp is missing), sets the bitmap mark, starts the loading
// screen and loads the level.
bool level_load()
{
    File f;
    int num_characters, num_models;

    level_mem_2693020 = 0;
    level_s3d_mem = 0;
    level_mem_269301c = 0;
    level_mem_2693008 = 0;
    level_mem_2693018 = 0;
    level_mem_2693010 = 0;
    level_mem_2693014 = 0;
    level_texture_mem = 0;
    if (!cd_has_level_data()) {
        if (!f.find("levelm.vpp", path_root))
            cd_prompt_disc2();
        packfile_add("levelm.vpp");
        packfile_add("levele.vpp");
        packfile_add("levelt.vpp");
    }
    bm_set_mark();
    vif_reset_all();
    snd_level_page_in();
    world_pick_reset();
    ui_model_lights_reset();
    level_script_read_town_characters(level_script_name, &level_is_town, &num_characters, &num_models);
    level_load_num_steps = level_script_load_steps(level_filename);
    level_load_start_time = timer_get(1000);
    loading_screen_load_bitmap(num_models, 0);
    localize_level_init(level_script_name);
    bool ok = (level_load_files() == true);
    return ok;
}

// 0x4386c0  Confidence: exact. The level file, the vis sets and layers, the render data, the surfaces,
// the pathfinding graph, the party, the level script, the doors, the camera and the character page-in;
// progress_update returns nonzero when the load is to stop. The phase times are written, never read.
bool level_load_files()
{
    char filename[64];

    level_load_start_time = timer_get(1000);
    navpoints_reset();
    level_load_time_609fe8 = 0;
    file_change_ext(filename, level_filename, ".s3d");
    level_load_time_s3d = timer_get(1000);
    if (level_load_s3d(filename, &level_s3d, 0x2d000))
        return false;
    if (level_vis_load(level_filename))
        return false;
    level_load_layers();
    level_load_time_s3d_done = timer_get(1000);
    level_load_time_render = timer_get(1000);
    levelrender_level_load();
    game_camera.num_saved_states = 0;
    level_load_time_render_done = timer_get(1000);
    level_load_time_surfaces = timer_get(1000);
    surfaces_parse_table(level_filename);
    level_load_time_surfaces_done = timer_get(1000);
    level_load_time_pathfinding = timer_get(1000);
    if (pathfinding_load_graph(level_filename))
        return false;
    level_load_time_pathfinding_done = timer_get(1000);
    if (progress_update(0))
        return false;
    characterinfo_alloc_level_buffers();
    level_spawn_party();
    level_preload_effects();
    level_load_time_script = timer_get(1000);
    level_script_select();
    level_load_time_script_done = timer_get(1000);
    if (progress_update(0))
        return false;
    level_restore_doors();
    camera_level_reset();
    level_load_time_page_in = timer_get(1000);
    level_page_in_characters();
    level_load_time_page_in_done = timer_get(1000);
    level_load_time_end = timer_get(1000);
    return true;
}

// 0x438880  Confidence: likely (the second get_layer_n result is not used: written as a bare call).
// level_layer_ground[i] = the vis set "$layer<i+1>" (D2202); the level objects of each set get flag 0x40
// and their second layer byte set to the set's index. A level without layer sets puts every object on
// layer 0.
void level_load_layers()
{
    char name[48];
    int i, j;

    for (i = 0; i < 20; i++) {
        level_layer_ground[i] = NULL;
        sprintf(name, "$layer%02d", i + 1);
        for (j = 0; j < level_num_vis_records; j++) {
            if (!_stricmp(name, level_vis_records[j].name)) {
                level_layer_ground[i] = &level_vis_records[j];
                break;
            }
        }
    }
    bool found = false;
    for (i = 0; i < 20; i++) {
        layer_lobj_list *set = level_layer_ground[i];
        if (set == NULL)
            continue;
        found = true;
        int count = set->count;
        for (j = 0; j < count; j++) {
            lobj *l = &lobjs[set->indices[j]];
            l->flags |= 0x40;
            int layer = l->get_layer_n(0);
            l->get_layer_n(1);
            if (i != layer)
                l->set_layer_n(1, i);
        }
    }
    if (!found) {
        for (i = 0; i < num_lobjs; i++) {
            lobjs[i].flags |= 0x40;
            lobjs[i].set_layer_n(0, 0);
        }
    }
}

// 0x4389f0  Confidence: likely (two_stage_get's results are not used). Every door (lobj flag 0x100)
// finds its floor point; in a multiplayer game the doors are also opened at once and stored as
// "open, locked" in the two-stage table. Single player restores nothing here.
void level_restore_doors()
{
    for (int i = 0; i < num_lobjs; i++) {
        lobj *l = &lobjs[i];
        if (!(l->flags & 0x100))
            continue;
        door_find_floor_point(l);
        if (game_flags & GF_MULTIPLAYER) {
            bool open, locked;
            door_set_open(&lobjs[i], -1.0f, 1, 0);
            savegame_state.two_stage_get(&open, &locked, &lobjs[i]);
            savegame_state.two_stage_set(true, true, &lobjs[i]);
        }
    }
}

// 0x438a90  Confidence: likely (the loop head). Single player, new game (previous state 2, no load
// pending): Joseph joins as leader at level 1; with Full_party (except in masad, which clears it) Rosalind,
// Jekhar and Flece join too. Multiplayer, new game: each player's character joins (the local one is the
// leader and reads its .plr); later levels only take the existing characters. Then every character gets
// CF_COMBAT_READY and prop tag 0, and combat_mode is set.
void level_spawn_party()
{
    char plr_name[64];
    int prev = gameseq_get_prev_state();

    if (game_flags & GF_SINGLE_PLAYER) {
        if (prev == 2 && !savegame_state.pending_load_length()) {
            party_member_join("Joseph", 1, true, false, -1);
            if (!_stricmp(level_filename, "masad")) {
                Full_party = false;
                return;
            }
            if (Full_party) {
                party_member_join("Rosalind", 1, false, false, -1);
                party_member_join("Jekhar", 1, false, false, -1);
                party_member_join("Flece", 1, false, false, -1);
            }
        }
        return;
    }
    for (mp_player *mp = mp_player_list.next; mp != &mp_player_list; mp = mp->next) {
        Character *c;
        Player *p;
        if (prev == 2) {
            c = party_member_join(mp->name, 1, mp == mp_local_player, false, mp->char_info);
            if (c == NULL)
                return;
            p = player_from_handle(c->handle);
            if (p == NULL)
                return;
            p->mp_owner = mp;
            mp->object = p;
            if (mp == mp_local_player) {
                file_change_ext(plr_name, mp->name, ".plr");
                mp_player_file_load(c, plr_name);
                mp_local_player->flags |= 4;
                if (Max_skills) {
                    for (int i = 0; i < p->num_progression; i++) {
                        c->skill[p->progression[i].skill] = 100;
                        p->progression[i].level = 1;
                    }
                }
            }
        } else {
            p = mp->object;
            c = living_from_handle(p->handle);
            if (p == NULL || c == NULL)
                return;
        }
        c->prop_tag = 0;
        c->flags |= CF_COMBAT_READY;
    }
    combat_mode = true;
}

// 0x438c80  Confidence: likely (the unpause test is two branches in the binary). State 4 leave. Going to
// state 5 (help) or 6 keeps the level. Otherwise the level is unloaded; a next state other than 4 (a
// level change) ends the game: the party records and the inventory count go as well.
void state_game_leave()
{
    handle_slots_reset();
    cursor_visible = false;
    frame_debug_flag_5fc01e = false;
    int next = gameseq_get_next_state();
    if (next == 5 || next == 6)
        return;
    containers_free_all();
    items_level_unload();
    if (!state_game_level_ready)
        conversation_db.save_state();
    snd_stop_all();
    level_script_leave();
    bool leaving_game = next != 4;
    level_unload(leaving_game);
    if (leaving_game)
        party_inventory_zero_count();
    characterinfo_level_unload(leaving_game);
    camera_leave_level();
    if (game_flags & GF_MULTIPLAYER) {
        if (next != 4 && game_is_paused())
            game_unpause();
    } else if (game_is_paused()) {
        game_unpause();
    }
    if (!state_game_level_ready)
        game_pause_count_inc();
    fx_level_leave();
    game_flags &= ~(GF_PARTY_DEAD | GF_LEADER_SOUND | GF_CONVERSATION);
    if (game_flags & GF_SOLO_MODE)
        party_toggle_hold();
}

// 0x438d60 sinit_game_camera (call init, jmp register; static initialiser entry 122). Confidence: exact.
// 0x438d70 init_game_camera (mov ecx, 0x60a018; jmp camera::camera). Confidence: exact.
// 0x438d80 register_dtor_game_camera (atexit(0x438d90)). Confidence: exact.
// 0x438d90 dtor_game_camera (mov ecx, 0x60a018; jmp camera::~camera). Confidence: exact.
camera game_camera;                                           // 0x60a018
// 0x438da0 init_empty_438db0_thunk (jmp, entry 123). Confidence: unsure (which object).
// 0x438db0 init_empty_438db0. Confidence: unsure (which object). A bare ret, the empty inline vector3()
//          of the object below (the only class-type object of the unit's .bss without a constructor
//          call; gametarget.cpp's world_click_move writes it; D2208).
vector3 world_last_click_pos;                                 // 0x609ff8 (gametarget.h, name ours)
// .data 0x593f50: between level_load_layers' "$layer%02d" and game_render_frame's "Fog_near" literal.
int world_mouse_target_handle = -1;                           // 0x593f50 (gametarget.h)

// 0x438dc0  Confidence: likely (the cursor_visible expression). The move target decal, the 3D scene
// (unless a full-screen interface screen is up), the obstacle debug view, the interface (with the depth
// test inverted on a full-screen screen), the script editor. The fog text below can never show: the
// timer is a function-local static that only this block starts, and only while it is already valid.
// It prints fog_get_far_current (0x486bf0, which returns fog_near_distance) as Fog_near.
void game_render_frame()
{
    char text[256];

    if (move_target_decal_visible && !(game_flags & GF_CUTSCENE))
        move_target_draw_decal();
    if (!interface_screen_is_fullscreen())
        levelrender_render_scene();
    if (Render_3d_obstacles)
        obstacles_debug_render();
    cursor_visible = !(game_flags & GF_CUTSCENE);
    if (interface_screen_is_fullscreen())
        gr_d3d_invert_depth_test();
    interface_draw();
    if (interface_screen_is_fullscreen())
        gr_d3d_restore_depth_test();
    script_editor_frame();
    empty_frame_hook_40acc0();

    static GameTimer fog_text_timer;                          // 0x60a2c0 game_render_frame_timer (guard 0x609ff0 bit 0)
    if (fog_text_timer.valid() && fog_text_timer.GetElapsedTime() < 10000) {
        if (levelrender_get_flag_22fb6f7())
            fog_text_timer.SetTimeout(0);
        sprintf(text, "Fog_near = %.2f, Fog_far = %.2f", fog_get_far_current(), fog_get_near_current());
        gr_set_color(250, 190, 190, 255);
        gr_string(10, 50, text, font_small, gr_string_mode);
    } else {
        fog_text_timer.invalidate();
    }
}

// 0x438f20 game_render_frame_static_dtor: the atexit handler of fog_text_timer (a bare ret; VC6
//          registers one for a function-local static of class type). Confidence: exact.

// 0x438f30  Confidence: likely (the profile_label scopes and their texts are ours; the fx_frame frame
// time of the paused and cutscene branches is two calls in the binary). State 4 frame.
void state_game_frame()
{
    if (d3d_use_vertex_buffer == false) {
        d3d_vertex_buffer_pos = 0;
        levelrender_d3d_vertex_buffer_pos = 0;
    }
    profile_label frame_label("state_game_frame");
    if (!game_is_paused()) {
        level_session_time += reported_frametime;
        level_time += reported_frametime;
    }
    if ((game_flags & GF_SINGLE_PLAYER) || mp_netgame_state >= 5)
        party_check_all_dead();
    {
        profile_label label("player_input_frame");
        player_input_frame();
    }
    empty_function();
    if (game_flags & GF_CUTSCENE)
        cutscene_sequence_update();
    else
        automap_reveal_around_party();
    level_set_current_layer(cur_character->layer);

    static vector3 listener_velocity(0.0f, 0.0f, 0.0f);       // 0x60a008 (guard 0x60a41c bit 0)
    snd_change_listener(&camera_view_pos, &listener_velocity, &camera_view_orient);
    {
        profile_label label("camera");
        gr::reset_clip();
        if (!(game_flags & GF_NO_CLEAR))
            gr_clear();
        if (!game_is_paused()) {
            vector3 shake_pos;
            quaternion shake_rot;
            matrix shake;
            vfx_get_camera(&shake_pos, &shake_rot);
            shake_rot.to_matrix_b(&shake);
            camera_view_pos += shake_pos;
            camera_view_orient = shake * camera_view_orient;
        }
        shadow_list_clear();
        interface_set_view_clip();
        gr_setup_3d(&camera_view_orient, &camera_view_pos, camera_fov, 1, 1);
    }
    float frametime;
    if (game_is_paused()) {
        if (cutscene_active_count) {
            frametime = cutscene_frametime;
            fx_frame(frametime, true);
        } else {
            frametime = reported_frametime;
            fx_frame(frametime, true);
        }
    } else if (game_flags & GF_CUTSCENE) {
        level_script_frame();
        cutscene_events_frame();
        objects_do_frame();
        if (cutscene_active_count) {
            frametime = cutscene_frametime;
            fx_frame(frametime, false);
        } else {
            frametime = reported_frametime;
            fx_frame(frametime, false);
        }
    } else {
        {
            profile_label label("level_script_frame");
            cutscene_script_entries_frame();
            level_script_frame();
        }
        {
            profile_label label("objects_do_frame");
            objects_do_frame();
        }
        {
            profile_label label("fx_frame");
            if (cutscene_active_count)
                frametime = cutscene_frametime;
            else
                frametime = reported_frametime;
            fx_frame(frametime, false);
        }
        if (game_flags & GF_CONVERSATION) {
            if (conversation_active.frame())
                game_flags &= ~GF_CONVERSATION;
        }
        mp_game_frame();
    }
    {
        profile_label label("world_mouse_frame");
        gameplay_update_cursor();
        hover_tint_set_object(NULL, 0.0f, 0.0f, 0.0f);
        hover_tint_set_character(NULL, 0.0f, 0.0f, 0.0f);
        if (!(game_flags & (GF_CUTSCENE | 0x80))
            && (!(game_flags & GF_MULTIPLAYER) || mp_netgame_state == 5 || mp_netgame_state == 6)) {
            switch (interface_get_screen()) {
            case 4:
            case 9:
            case 13:
            case 20:
                break;
            default:
                world_mouse_frame(player_input, &camera_view_pos);
                break;
            }
        }
    }
    if (!(game_flags & GF_CUTSCENE) && (!(game_flags & GF_MULTIPLAYER) || mp_netgame_state >= 5))
        game_draw_leader_marker();
    if (!(game_flags & GF_NO_RENDER)) {
        profile_label label("game_render_frame");
        if (gameplay_no_render_60a630 == 1)
            frame_sleep_when_inactive(0, 1);
        else
            game_render_frame();
    }
    level_apply_pending_change();
}

// 0x439370  Confidence: exact (jump table 0x43942c / index table 0x439434: screens 0, 3, 5, 6 and 19).
void gameplay_update_cursor()
{
    if (interface_mouse_over_panel()) {
        switch (interface_get_screen()) {
        case 0:
        case 3:
        case 5:
        case 6:
        case 19:
            if (hud_target_mode == 0 && world_hover_portrait_character) {
                cursor_set_type(cursor_target_bm);
                return;
            }
            cursor_set_type(cursor_walk_bm);
            return;
        default:
            cursor_set_type(cursor_walk_bm);
            return;
        }
    }
    if (cur_level_index == LEVEL_INDEX_WORLDMAP) {
        cursor_set_type(cursor_walk_bm);
        return;
    }
    if (hud_target_mode >= 0 && hud_target_mode <= 1) {
        cursor_set_type(cursor_target_bm);
        return;
    }
    int bm = cursor_walk_bm;
    if (hover_tint_active())
        bm = cursor_hover_bm;
    cursor_set_type(bm);
}

// 0x439450  Confidence: exact. A dead (or missing) leader: in single player the first living member in
// party_list order takes over (with a fade-in), and with nobody alive the death screen opens; in a
// multiplayer game hud_party_dead.
void party_check_all_dead()
{
    int leader_dead = 0;
    Character *c = living_from_handle(party_leader->handle);
    if (c == NULL || (c->flags & CF_DEAD))
        leader_dead = 1;
    Player *alive = NULL;
    for (Player *p = party_list.next; p != &party_list; p = p->next) {
        c = living_from_handle(p->handle);
        if (alive == NULL && c != NULL && !(c->flags & CF_DEAD))
            alive = p;
    }
    if (!leader_dead)
        return;
    if (game_flags & GF_SINGLE_PLAYER) {
        if (alive) {
            gr_fade_in(0);
            party_select_member(alive);
            return;
        }
        game_flags |= GF_PARTY_DEAD;
        if (game_is_paused())
            game_unpause();
        hud_mode_set_5();
        return;
    }
    hud_party_dead();
}

// 0x439500 state_game_frame_static_dtor: the atexit handler of listener_velocity (a bare ret).
//          Confidence: exact.

// 0x439510  Confidence: exact. snap: the default view (yaw 0, zoom 1.1) first.
void camera_reset_to_player(bool snap)
{
    if (snap) {
        game_camera.yaw = 0.0f;
        game_camera.zoom = 1.1f;
        game_camera.reset_default();
    }
    level_set_current_layer(cur_character->layer);
    game_camera.set_target(cur_character);
    game_camera.resume_follow();
    game_camera.sync_yaw_to_target();
    game_camera.snap_to_target(1, 1);
}

// 0x439580  Confidence: exact. Never in a town. Single player: every party member gets the goal; a
// network game: only the own character (and without ai_process_goals).
void combat_mode_set(bool on)
{
    if (level_is_town) {
        combat_mode = false;
        return;
    }
    if (combat_mode == on)
        return;
    combat_mode = on;
    if (game_flags & GF_SINGLE_PLAYER) {
        for (Player *p = party_list.next; p != &party_list; p = p->next) {
            Character *c = living_from_handle(p->handle);
            if (c) {
                c->ai.set_combat_mode_goal(on, AI_DIRECT_PRIORITY);
                c->ai.process_goals();
            }
        }
    } else {
        cur_character->ai.set_combat_mode_goal(on, AI_DIRECT_PRIORITY);
    }
}

// 0x439620  Confidence: exact. Solo mode: every member but the leader holds (and goes idle unless in AI
// mode 6); leaving solo mode releases them all. Returns the new solo mode bit (0 in a town).
int party_toggle_hold()
{
    if (level_is_town) {
        game_flags &= ~GF_SOLO_MODE;
        return 0;
    }
    game_flags ^= GF_SOLO_MODE;
    for (Player *p = party_list.next; p != &party_list; p = p->next) {
        if ((game_flags & GF_SOLO_MODE) && !p->is_leader()) {
            p->flags |= PF_HOLD;
            Character *c = living_from_handle(p->handle);
            if (c && !c->ai.is_mode(6, -1))
                c->ai.go_idle();
        } else {
            p->flags &= ~PF_HOLD;
        }
    }
    return game_flags & GF_SOLO_MODE;
}

// 0x4396d0  Confidence: exact. A dead or hidden member cannot be selected (sound 0x24).
void party_select_member(Player *p)
{
    Character *c = living_from_handle(p->handle);
    if (c && !(c->flags & (CF_DEAD | CF_HIDDEN))) {
        for (Player *q = party_list.next; q != &party_list; q = q->next)
            q->drop_leader();
        party_set_leader(p);
        game_play_sound(0x23, 1.0f, 0.0f, 0, 0);
        return;
    }
    game_play_sound(0x24, 1.0f, 0.0f, 0, 0);
}

// 0x439750  Confidence: exact. The others line up 0.75 apart along x (single player only).
void party_teleport_to(vector3 *pos, int layer)
{
    vector3 p = *pos;
    cur_character->pos = p;
    cur_character->layer = layer;
    if (game_flags & GF_SINGLE_PLAYER) {
        for (Player *pl = party_list.next; pl != &party_list; pl = pl->next) {
            Character *c = living_from_handle(pl->handle);
            if (c && c != cur_character) {
                p.x += 0.75f;
                c->pos = p;
                c->layer = layer;
            }
        }
    }
    camera_reset_to_player(false);
}

// 0x439800  Confidence: exact.
void items_reset_for_new_game()
{
    savegame_build_layout();
    item_types_reset_identified();
}

void cmd_render_3d_obstacles();

// 0x439810 init_ccmd_render_3d_obstacles_thunk (jmp, entry 124). Confidence: exact.
// 0x439820 init_ccmd_render_3d_obstacles (console_command constructor). Confidence: exact.
console_command ccmd_render_3d_obstacles("render_3d_obstacles", "Toggles Render_3d_obstacles",
                                         cmd_render_3d_obstacles);   // 0x60a648

// 0x439840  Confidence: exact (the console's bool command macro).
void cmd_render_3d_obstacles()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Render_3d_obstacles = true;
        else if (arg_type & ARG_FALSE)
            Render_3d_obstacles = false;
        else if (arg_type & ARG_NONE)
            Render_3d_obstacles ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it.\n",
                "render_3d_obstacles", "Render_3d_obstacles");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "render_3d_obstacles", Render_3d_obstacles ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

void cmd_character();

// 0x439900 init_ccmd_character_thunk (jmp, entry 125). Confidence: exact.
// 0x439910 init_ccmd_character (console_command constructor). Confidence: exact.
console_command ccmd_character("character", "Set the current main character", cmd_character);   // 0x60a420

// 0x439930  Confidence: likely (the copies of the position and orientation are expanded: a default
// matrix plus assignment). Replaces the leader's character with a new one of the named char_infos record
// at the same place; the words of the argument are joined with spaces. Without an argument the name is
// empty and name[strlen(name) - 1] writes the byte before the buffer.
void cmd_character()
{
    char name[128];

    if (calling_a_command_function) {
        process_command(ARG_STRING);
        name[0] = 0;
        while (!(arg_type & ARG_NONE)) {
            strcat(name, next_arg);
            strcat(name, " ");
            process_command(ARG_STRING | ARG_NONE);
        }
        name[strlen(name) - 1] = 0;
        int index = charinfo_find_by_name(name);
        if (index == -1) {
            sprintf(string_buffer, "Unable to find character %s in character table\n", name);
            print_to_console(string_buffer, 0);
            return;
        }
        Player *p;
        for (p = party_list.next; p != &party_list; p = p->next) {
            Character *c = living_from_handle(p->handle);
            if (c)
                ai_group_remove_member(c);
        }
        vector3 pos = cur_character->pos;
        matrix orient;
        orient = cur_character->orient;
        cur_character->deinit();
        party_leader->char_info = index;
        party_leader->create(&pos, &orient, NULL, true);
        cur_character->ai.reset();
        cur_character->find_ground();
        cur_character->update_layer();
        game_camera.set_target(cur_character);
        for (p = party_list.next; p != &party_list; p = p->next) {
            if (p != party_leader)
                ai_group_add_member(cur_character->ai.group, living_from_handle(p->handle));
        }
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "Main character: %s\n", cur_character->instance->character->name);
        print_to_console(string_buffer, 0);
    }
    if (doing_help_for_comand)
        print_to_console("Usage: character <string>\n", 0);
}

// 0x439ba0  Confidence: exact. The index test after player_alloc_slot repeats the one before it (the
// slot is then not given back). In single player a known member (in party_list or player_list_other)
// is returned as it is, at any level.
Character *party_member_join(const char *name, int level, bool leader, bool place_at_start, int char_index)
{
    char text[256];
    int known = 0;

    if (savegame_state.pending_load_length())
        return NULL;
    if (char_index < 0) {
        char_index = charinfo_find_by_name(name);
        if (char_index < 0)
            return NULL;
    }
    if (game_flags & GF_SINGLE_PLAYER) {
        Player *p;
        for (p = party_list.next; p != &party_list; p = p->next, known++) {
            if (p->char_info == char_index)
                return living_from_handle(p->handle);
        }
        for (p = player_list_other.next; p != &player_list_other; p = p->next, known++) {
            if (p->char_info == char_index)
                return living_from_handle(p->handle);
        }
        if (known >= MAX_PLAYERS)
            return NULL;
    }
    Player *slot = player_alloc_slot();
    slot->char_info = char_index;
    if (char_index < 0)
        return NULL;
    Character *c = slot->create(&vector3_zero, &identity_matrix, name, leader);
    if (c == NULL)
        return NULL;
    if (leader == true)
        party_leader = slot;
    if (level > 1)
        c->add_xp(xp_for_level(level), true, false);
    c->ai.reset();
    if (place_at_start == true)
        c->place_at_party_start(known);
    party_sort_by_species();
    sprintf(text, XSTR("%s has joined the party!\n", 622), c->get_name());
    hud_show_message(text);
    return c;
}

void cmd_flece();

// 0x439d50 init_ccmd_flece_thunk (jmp, entry 126). Confidence: exact.
// 0x439d60 init_ccmd_flece (console_command constructor). Confidence: exact.
console_command ccmd_flece("flece", "Add Flece to the game", cmd_flece);   // 0x60a410

// 0x439d80  Confidence: exact. No calling_a_command_function test: help and status join her too.
void cmd_flece()
{
    party_member_join("Flece", 1, false, true, -1);
}

void cmd_jekhar();

// 0x439da0 init_ccmd_jekhar_thunk (jmp, entry 127). Confidence: exact.
// 0x439db0 init_ccmd_jekhar (console_command constructor). Confidence: exact.
console_command ccmd_jekhar("jekhar", "Add Jekhar to the game", cmd_jekhar);   // 0x60a638

// 0x439dd0  Confidence: exact.
void cmd_jekhar()
{
    party_member_join("Jekhar", 1, false, true, -1);
}

void cmd_rosalind();

// 0x439df0 init_ccmd_rosalind_thunk (jmp, entry 128). Confidence: exact.
// 0x439e00 init_ccmd_rosalind (console_command constructor). Confidence: exact.
console_command ccmd_rosalind("rosalind", "Add Rosalind to the game", cmd_rosalind);   // 0x60a658

// 0x439e20  Confidence: exact.
void cmd_rosalind()
{
    party_member_join("Rosalind", 1, false, true, -1);
}

// 0x439e40 sinit_ui_window_60a670 (call init, jmp register; entry 129). Confidence: exact.
// 0x439e50 init_ui_window_60a670 (mov ecx, 0x60a670; jmp ui_window::ui_window). Confidence: exact.
// 0x439e60 register_atexit_439e60 (atexit(0x439e70)). Confidence: exact.
// 0x439e70 dtor_ui_window_60a670 (mov ecx, 0x60a670; jmp ui_window::~ui_window). Confidence: exact.
ui_window gameplay_help_window;       // 0x60a670 constructed and destroyed only (name ours)

// ---- .data 0x5940ac-0x59412f: the help screen's tables ------------------------------------------------
// {key, action} pairs; each count is the number of strings. The strings follow the tables in reverse
// order (0x594130-0x59436c). The debug table is never drawn.
char *help_mouse_controls[8] = {                              // 0x5940ac
    "Right click and release", "Moves player to specified point",
    "Right click and hold", "Makes player follow mouse cursor",
    "Right click on enemy", "Invokes attack on that enemy",
    "Left click", "Casts a spell, after a spell has been selected",
};
int help_num_mouse_controls = sizeof(help_mouse_controls) / sizeof(help_mouse_controls[0]);   // 0x5940cc
char *help_keyboard_controls[6] = {                           // 0x5940d0
    "Tab", "Switches control between main characters",
    "Enter", "Toggles combat mode",
    "Space", "Pause/unpause the game",
};
int help_num_keyboard_controls = sizeof(help_keyboard_controls) / sizeof(help_keyboard_controls[0]);   // 0x5940e8
char *help_debug_controls[12] = {                             // 0x5940ec
    "<debug>-P", "Debug mode for pathfinding",
    "<debug>-S", "Camera Slew Mode",
    "<debug>-G", "Disable World Collisions",
    "<debug>-A", "Play action animation",
    "<debug>-N", "Select Next Action Animation",
    "<debug>-F", "Toggle Framerate counter",
};
int help_num_debug_controls = sizeof(help_debug_controls) / sizeof(help_debug_controls[0]);   // 0x59411c
char *help_camera_controls[2] = {                             // 0x594120
    "Arrow keys", "Move camera",
};
int help_num_camera_controls = sizeof(help_camera_controls) / sizeof(help_camera_controls[0]);   // 0x594128
void (*help_page_draw_funcs[NUM_HELP_PAGES])(int y) = {      // 0x59412c
    help_controls_window_draw,
};

// 0x439e80  Confidence: likely (the three loops' shape; the camera loop's y step h + 2 is computed once).
// Keys at x 10, actions right-aligned at 630 (640-wide coordinates at every resolution).
void help_controls_window_draw(int y)
{
    int w, h;
    int i;
    int line_h = gr_get_font_height(font_main);

    gr_string(GR_CENTER_X, y, "Mouse Controls", font_main, gr_string_mode);
    y += line_h * 3 / 2;
    for (i = 0; i < help_num_mouse_controls; i += 2) {
        gr_get_string_size(&w, &h, help_mouse_controls[i + 1], -1, font_main);
        gr_string(10, y, help_mouse_controls[i], font_main, gr_string_mode);
        gr_string(630 - w, y, help_mouse_controls[i + 1], font_main, gr_string_mode);
        y += line_h + 2;
    }
    y += line_h * 3;
    gr_string(GR_CENTER_X, y, "Keyboard Controls", font_main, gr_string_mode);
    y += line_h * 2;
    for (i = 0; i < help_num_keyboard_controls; i += 2) {
        gr_get_string_size(&w, &h, help_keyboard_controls[i + 1], -1, font_main);
        gr_string(10, y, help_keyboard_controls[i], font_main, gr_string_mode);
        gr_string(630 - w, y, help_keyboard_controls[i + 1], font_main, gr_string_mode);
        y += line_h + 2;
    }
    y += line_h * 3;
    gr_string(GR_CENTER_X, y, "Camera Controls", font_main, gr_string_mode);
    y += line_h * 2;
    for (i = 0; i < help_num_camera_controls; i += 2) {
        gr_get_string_size(&w, &h, help_camera_controls[i + 1], -1, font_main);
        gr_string(10, y, help_camera_controls[i], font_main, gr_string_mode);
        gr_string(630 - w, y, help_camera_controls[i + 1], font_main, gr_string_mode);
        y += line_h + 2;
    }
}

// 0x43a0a0  Confidence: exact. The hard-coded English help screen (no strings.tbl ids).
void help_draw_page()
{
    gr::reset_clip();
    gr_clear();
    int line_h = gr_get_font_height(font_main);
    gr_set_color(0, 255, 0, 128);
    gr_string(GR_CENTER_X, 20, "Summoner Help", font_bones_small, gr_string_mode);
    int y = gr_get_font_height(font_bones_small) + line_h + 20;
    sprintf(string_buffer, "Page %d of %d", loading_help_page + 1, NUM_HELP_PAGES);
    gr_string(GR_CENTER_X, y, string_buffer, font_main, gr_string_mode);
    help_page_draw_funcs[loading_help_page](76);
    gr_string(GR_CENTER_X, (160 - line_h) * 3, "ESC to exit this screen", font_main, gr_string_mode);
}

// 0x43a170  Confidence: exact (jump table 0x43a1d0 / index table 0x43a1e0 for keys 1-0xcd). Esc returns to
// the state the help screen came from; left / right page (one page: right wraps to 0).
void state_loading_key_handler(int key)
{
    switch (key) {
    case KEY_ESC:
        gameseq_set_state(state_loading_next_state);
        break;
    case KEY_PAD4:
    case KEY_LEFT:
        if (--loading_help_page < 0)
            loading_help_page = 0;
        break;
    case KEY_PAD6:
    case KEY_RIGHT:
        if (++loading_help_page == NUM_HELP_PAGES)
            loading_help_page = 0;
        break;
    }
}

// 0x43a2b0  Confidence: exact. State 5 enter: the help screen (not a loading screen). From the game
// (state 4) the game stays paused while it is up.
void state_loading_enter()
{
    loading_help_page = 0;
    state_loading_next_state = gameseq_get_prev_state();
    if (state_loading_next_state == 4)
        game_pause_count_inc();
    state_loading_active = 1;
    input_flush();
}

// 0x43a2e0  Confidence: exact.
void state_loading_frame()
{
    cursor_visible = false;
    help_draw_page();
    ui_poll_and_process(state_loading_key_handler, true);
}

// 0x43a300  Confidence: exact.
void state_loading_leave()
{
    state_loading_active = 0;
    gr_clear();
    gr_flip();
    gr_clear();
    gr_flip();
    if (state_loading_next_state == 4)
        game_pause_count_dec();
}
