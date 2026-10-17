// summoner/gameplay.h - gameplay.cpp (0x437660-0x43a32f: the console cheats, level load and unload, the
// game state 4 (state_game_enter / _frame / _leave), party control, the loading screen and the help
// state 5; strings.tbl id 622 "// gameplay.cpp"; decomp42). File name from strings.tbl and the module
// column of sum.symbols; the directory follows summoner.h, which includes this header (the declarations
// were in summoner.h before). Not original source.
#ifndef SUMMONER_GAMEPLAY_H
#define SUMMONER_GAMEPLAY_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "summoner/level_info.h"

class Character;
class Player;
struct navpoint;

// ---- .data 0x593c34-0x5943ef --------------------------------------------------------------------------
// The loading bar per resolution_index (LoadingScreenDraw): {w, h} and {x, y}. Names ours.
extern int loading_bar_size[3][2];             // 0x593c34 {320, 28}, {400, 35}, {514, 45}
extern int loading_bar_pos[3][2];              // 0x593c4c {160, 345}, {200, 431}, {256, 554}
extern int loading_screen_bm;                  // 0x593c64 -1 while no loading screen is up
extern int gameplay_data_593c68[4];            // 0x593c68 {1, 1, 1, 1}, never read (name ours)
// world_mouse_target_handle (0x593f50) is defined in gameplay.cpp; declared in summoner/gametarget.h.
// The help screen's text tables: {key, action} string pairs, each with its count of strings. Names ours.
extern char *help_mouse_controls[8];           // 0x5940ac
extern int   help_num_mouse_controls;          // 0x5940cc 8
extern char *help_keyboard_controls[6];        // 0x5940d0
extern int   help_num_keyboard_controls;       // 0x5940e8 6
extern char *help_debug_controls[12];          // 0x5940ec never drawn
extern int   help_num_debug_controls;          // 0x59411c 12
extern char *help_camera_controls[2];          // 0x594120
extern int   help_num_camera_controls;         // 0x594128 2
extern void (*help_page_draw_funcs[1])(int y); // 0x59412c (help_controls_window_draw)
#define NUM_HELP_PAGES 1                       // help_draw_page and state_help_key_handler (ours)

// ---- .bss 0x609f40-0x60ab5f ---------------------------------------------------------------------------
extern int  loading_screen_start_time;         // 0x609f4c
extern bool state_game_load_failed;            // 0x609f58 set only when level_load / the pending load FAILED (D2207)
extern int  level_load_num_steps;              // 0x609f5c
extern int  level_load_start_time;             // 0x609f88
extern bool loading_screen_active;             // 0x609fbc
extern int  loading_bar_corners[8];            // 0x609fc0 never read (init_screen_rects_609fc0; name ours)
extern bool Full_party;                        // 0x609fe0 console "full_party"
extern bool Dump_sound_usage;                  // 0x609fe1 console "dump_sound_usage" (never read)
extern bool Give_all_items;                    // 0x609fec console "Give_all_items" (never read)
// The current level's names and layer maps (set by game_change_level / level_parse_table).
// level_filename, level_script_name, level_prev_filename (0x60a2c8-0x60a327), level_session_time, the party
// starts, level_entry_point and the layer maps are members of level_info (summoner/level_info.h, D7201).
extern float level_time;                       // 0x60a654 unpaused seconds since state_game_enter (savegame header +0xac; decomp27)
// The party start markers level_find_player_starts fills ("$player%d-%02d", index level_entry_point);
// character_place_at_party_start reads them. sum.symbols types the array void*[4]; the records are
// navpoints (level_scripts.h; ls_worldmap1_init points all four at its own record; D1405).
// level_player_starts 0x60a32c, level_entry_point 0x60a33c (1 after game_change_level; 3 in
// ls_khosanilab_puzzle_use), level_num_layers 0x60a340, level_layer_maps 0x60a344, level_layer_automap_index
// 0x60a36c: level_info members (D7201).
extern int  gameplay_no_render_60a630;         // 0x60a630 never written: 1 would skip game_render_frame (name ours)
extern bool move_target_decal_visible;         // 0x60a664 player_read_mouse_input; game_render_frame draws the decal (name ours)
extern bool Render_3d_obstacles;               // 0x60a665 console "render_3d_obstacles"
extern int  state_help_return_state;          // 0x60a668 the state the help screen returns to
extern int  help_page;                 // 0x60a66c
extern int  state_help_active;              // 0x60ab5c

// ---- functions, in address order ----------------------------------------------------------------------
void cmd_full_party();                         // 0x437800
void cmd_dump_sound_usage();                   // 0x4378f0
void cmd_add_xp();                             // 0x4379e0
// (script name, out: #Town, distinct $Character: records, of those the ones without an alt_parent).
void level_script_read_town_characters(const char *script_name, bool *is_town, int *num_characters,
                                       int *num_models);                   // 0x437a40
void loading_screen_load_bitmap(int num_models, int unused);               // 0x437c20 both arguments unused
void LoadingScreenDraw(int step, int flags);   // 0x437c80 the progress callback (step < 0 or flags: nothing)
void level_load_report();                      // 0x437d70
void level_unload(bool leaving_game);          // 0x437e00 (state_game_leave: next state != 4)
void party_level_reset(bool leaving_game);     // 0x437e50
void cmd_Give_all_items();                     // 0x438070
void state_game_enter();                       // 0x438130 state 4 enter
bool level_load();                             // 0x438570
bool level_load_files();                       // 0x4386c0
void level_load_layers();                      // 0x438880
void level_restore_doors();                    // 0x4389f0
void level_spawn_party();                      // 0x438a90
void state_game_leave();                       // 0x438c80 state 4 leave
void game_render_frame();                      // 0x438dc0
void state_game_frame();                       // 0x438f30 state 4 frame
void gameplay_update_cursor();                 // 0x439370
void party_check_all_dead();                   // 0x439450
void camera_reset_to_player(bool snap);        // 0x439510 (bool: test al at 0x439514; D2204)
void combat_mode_set(bool on);                 // 0x439580 (bool: cmp al, bl at 0x43959e; D2200)
int  party_toggle_hold();                      // 0x439620 game_flags & GF_SOLO_MODE (D2201)
void party_select_member(Player *p);           // 0x4396d0
// (pos, layer): moves cur_character and the party there (world_mouse_frame's debug click, lobj flag 0x40).
void party_teleport_to(vector3 *pos, int layer);   // 0x439750
void items_reset_for_new_game();               // 0x439800 (state_title_enter)
void cmd_render_3d_obstacles();                // 0x439840
void cmd_character();                          // 0x439930
// (name, level, leader, place at the party start, char_infos index or -1 = by name); NULL when a load is
// pending, the name is unknown, or (single player) five members are already known. D2203.
Character *party_member_join(const char *name, int level, bool leader, bool place_at_start,
                             int char_index);  // 0x439ba0
void cmd_flece();                              // 0x439d80
void cmd_jekhar();                             // 0x439dd0
void cmd_rosalind();                           // 0x439e20
void help_controls_window_draw(int y);         // 0x439e80 help page 1
void help_draw_page();                         // 0x43a0a0
void state_help_key_handler(int key);       // 0x43a170
void state_help_enter();                    // 0x43a2b0 state 5 enter (the help screen)
void state_help_frame();                    // 0x43a2e0
void state_help_leave();                    // 0x43a300

#endif
