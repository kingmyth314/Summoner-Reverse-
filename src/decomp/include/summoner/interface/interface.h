// summoner/interface/interface.h - what the hand-made interface screens (new_*.cpp) share: the screen
// switch, the button records, scaling, message boxes, choice popups, tooltips, the 3D model views, the
// interface colours and the helpers of the HUD / inventory / status units that other screens call.
// Units: the interface manager 0x411270-0x414cb0 (decomp48: game/unnamed_interface_0x411270.cpp, a
// placeholder name - the original file name is not known), new_inventory.cpp (item info popup
// 0x418920-0x419160), new_multichat.cpp (new_multichat.h), new_int_multi_update.cpp area (new_icam
// 0x410760), and the world map overlay (unit not known).
// One header for now; directory and file name ours. Not original source.
#ifndef SUMMONER_INTERFACE_INTERFACE_H
#define SUMMONER_INTERFACE_INTERFACE_H

#include "vsdk/vsdk.h"
#include "vsdk/gr/gr.h"
#include "vsdk/math/matrix.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/console.h"
#include "summoner/summoner.h"

class Character;
class character_instance;
class vfx_instance;

// ---- button records ---------------------------------------------------------------------------------
// One entry of a screen's static button table in .data (0x40 bytes). Two spellings of one layout: the
// position as int[3][2] (most screens) or as {x, y}[3] (assess, pickup); kept apart because the code
// indexes them differently (DECISIONS.md D17).
struct ui_button_def {
    char name[0x24];      // +0x00 bitmap base name; "640.tga"/"1024.tga" is appended unless it has a '.'
    int  pos[3][2];       // +0x24 x, y for each resolution_index (640, 800, 1024)
    int  flags;           // +0x3c screen-specific: which entries react to the mouse (1, 2 or 4)
};
LAYOUT_CHECK(ui_button_def, sizeof(ui_button_def) == 0x40);
struct screen_button_def {
    char name[0x24];                  // +0x00
    struct { int x, y; } pos[3];      // +0x24
    int  flags;                       // +0x3c 1 = fires on release, 4 = fires on press and auto-repeats
};
LAYOUT_CHECK(screen_button_def, sizeof(screen_button_def) == 0x40);

// Runtime state of one button (0x18 bytes), filled by *_load_bitmaps.
struct ui_button_state {
    int flags;            // +0x00 1 always drawn, 2 highlighted / pressed this frame, 4 disabled
    int bitmap;           // +0x04 bm handle, -1 none
    int x, y;             // +0x08 / +0x0c
    int w, h;             // +0x10 / +0x14 bitmap size times ui_get_scale()
};
LAYOUT_CHECK(ui_button_state, sizeof(ui_button_state) == 0x18);
typedef ui_button_state screen_button;   // new_assess.cpp / new_pickup.cpp's name

// ---- screens ----------------------------------------------------------------------------------------
// The second argument is a pointer: the callee null-tests it and reads a byte / dword through it
// (0x41281a-0x412997); the call sites pass 0 or an address (DECISIONS.md D6).
void interface_set_screen(int id, void *arg);                       // 0x412620
void interface_refresh_cur_screen();                                // 0x412db0
int  interface_get_screen();                                        // 0x412610
void interface_init_global();                                       // 0x4116c0 GameInit: screen tables and bitmaps
void interface_reset_state();                                       // 0x4118c0 game_cleanup
void interface_close_screen();                                      // 0x416a10 interface_set_screen(0, 0)
bool interface_input_ready();                                       // 0x4148e0
extern Character *status_character;                                 // 0x5bd810 (the interface manager's .bss; decomp48)
extern bool  ui_input_locked;                                       // 0x5bd962 (the HUD is hidden)
void  ui_input_lock();                                              // 0x4146a0 ui_input_locked = 1 (decomp35)
bool  interface_mouse_over_panel();                                 // 0x412af0 the open screen's panel is under the mouse
// The script editor's pick overrides (decomp35, world_pick_ground_point): the decal scale and the point
// the decal is turned towards. Both return false when not set.
bool  ui_get_pick_screen_override(float *x, float *y);              // 0x414580
bool  ui_get_pick_world_override(vector3 *pos);                     // 0x4145b0
extern float interface_pulse_time;                                  // 0x58bdc4 (1.0 + frame time, for blinking)
// Returns 1 when the leader changed (setne al at 0x413453; mov eax, 1 in multiplayer: int, D5900; the callers
// ignore it, D2800). In multiplayer it always steps forward.
int   party_cycle(int forward);                                      // 0x413360 Tab (1) / Shift+Tab (0)

// ---- the interface manager's own functions (decomp48) ---------------------------------------------
// Screen ids (interface.md): 0 HUD, 1 inventory, 2 status, 3 automap, 4 pickup, 5 spells, 6 skills,
// 7 buy/sell, 8 save/load, 9 play menu, 10 journal, 11 PXO chat, 12 options, 13 dialogue, 14 main menu,
// 15 game list, 16 character select, 17 game create, 18 level select, 19 item trade, 20 assess,
// 21 credits, 22 help.
int  interface_frame(int key);                                      // 0x4119a0 the key, 0 when used
void ui_tooltip_hide_on_mouse_activity(int key);                    // 0x411be0
void ui_tooltip_draw();                                             // 0x411ea0
void hud_draw_message();                                            // 0x411f80
void hud_draw_chain_message();                                      // 0x4120d0 (nothing while a tutorial runs)
void interface_pulse_update(float frametime);                       // 0x4121d0
void ui_load_model_lights();                                        // 0x4123c0 "inv-lights1.vfx"
void ui_input_unlock();                                             // 0x4146b0
void interface_start_input_delay();                                 // 0x4148c0 600 ms, hides the tooltip
// The loading screen of the cutscene loader (cutscene_load_data / cutscene_loading_finish callers).
void cutscene_draw_loading();                                       // 0x414900 one step, then gr_flip
void cutscene_begin_loading(int total);                             // 0x4149d0
void cutscene_loading_finish();                                     // 0x414a10
// Per-screen refresh calls, indexed by screen id (1 inventory, 2 status, 5 spells, 6 skills).
extern void (*screen_refresh_table[23])();                          // 0x58bdc8
extern int   interface_cur_screen;                                  // 0x5bd814 0 = HUD
extern float interface_view_fov;                                    // 0x5bd818 gr_get_view_fov at GameInit
extern RealtimeTimer interface_input_delay_timer;                   // 0x5bd950 (sum.symbols: int)
extern GameTimer hud_message_timer;                                 // 0x5bd80c 4000 ms
extern GameTimer hud_chain_message_timer;                           // 0x5bd81c 4000 ms
extern char  hud_message_text[68];                                  // 0x5bd32c (63 characters)
extern char  hud_chain_message_text[16];                            // 0x5bd304 (15 characters)
extern vfx_instance *ui_model_lights_vfx;                           // 0x5bd968
extern int   cursoricon_chain_bm;                                   // 0x58bdb4 (levelrender_render_scene)
extern int   cursoricon_incidental_bm;                              // 0x5bd934
extern int   cursoricon_quest_bm;                                   // 0x5bd7b4
extern int   cursorspot_focus_bm;                                   // 0x58be34
extern int   cursorspot_focus_large_bm;                             // 0x58be38
extern int   message_popup_bkgrnd_bm;                               // 0x5bd95c
extern int   cutscene_loading_bm;                                   // 0x5bd94c
// The show_ui console command (moved here from new_inventory.cpp, decomp48).
extern console_command ccmd_show_ui;                                // 0x5bd780
void cmd_show_ui();                                                 // 0x414cb0

// ---- world picking overrides (set by the inventory's use-item and by skills / spells) -----------------
extern float ui_pick_override_x;                                    // 0x58bdbc
extern float ui_pick_override_y;                                    // 0x58bdc0
extern bool  ui_pick_screen_override_on;                            // 0x5bd960
extern bool  ui_pick_world_override_on;                             // 0x5bd961
extern vector3 ui_pick_world_override_pos;                          // 0x5bd2f8
extern int   ui_pending_use_skill;                                  // 0x5bd8ec
extern bool  ui_using_list_item;                                    // 0x5bd7b0
extern bool  ui_using_equipped_item;                                // 0x5bd50c
extern int   ui_using_equipped_slot;                                // 0x5bd93c
struct party_inventory_entry;
extern party_inventory_entry *ui_using_list_item_entry;             // 0x5bd514 (sum.symbols types it int)

// ---- scaling and drawing helpers ----------------------------------------------------------------------
extern int   ui_hires;                                              // 0x5bd318, 0 = 640 layout, 1 = 1024
extern char *ui_bitmap_suffix_tga[2];                               // 0x58bda4 {"640.tga", "1024.tga"}
extern char *ui_bitmap_suffix[2];                                   // 0x58bdac {"640", "1024"} (skills / spells '#' names)
bool  point_in_rect(int px, int py, int x, int y, int w, int h);    // 0x413460
float ui_get_scale();                                               // 0x4134a0
// The mode goes on to gr_bitmap_stretched unchanged, so it is a gr::Mode (D2802; the callers pass
// gr_bitmap_wrap_mode).
void  ui_bitmap_scaled(int bm, int x, int y, gr::Mode mode);        // 0x4144f0
extern float ui_scale;                                              // 0x58be24 screen width / (640 or 1024)
extern float ui_scale_640;                                          // 0x58be28 screen width / 640 (never read; name ours)
void  ui_apply_pulse_rgb(float *r, float *g, float *b, float offset);   // 0x414bb0 (offset 0 in the pilots)
void  options_draw_version();                                       // 0x4146c0
void  ui_screen_stop_player();                                      // 0x41e380
void  ui_clear_pick_overrides();                                    // 0x414560
void  ui_draw_panel_frame(int x, int y, int w, int h);              // 0x4134b0 three nested gr_rect frames
void  ui_draw_text_box(int x, int y, int w, int h);                 // 0x4135a0 filled box with a border
// ui_draw_panel_frame's per-resolution sizes (.rdata of the interface core; names ours).
extern const int ui_frame_border[3];                                // 0x57ef40 {2, 2, 2} frame lines
extern const int ui_frame_inset_x[3];                               // 0x57ef4c {8, 8, 8}
extern const int ui_frame_inset_y[3];                               // 0x57ef58 {5, 5, 5}
void  color_pulse_scale(float *r, float *g, float *b, float bias);  // 0x414ae0
extern Color ui_color_text_box_bg;                                  // 0x5bd938 (0, 0, 0, 196)
extern float ui_pulse_scale_phase;                                  // 0x5bd8f4 0..1 (color_pulse_scale)
extern float ui_pulse_rgb_phase;                                    // 0x5bd7a4 0..1, three times as fast (ui_apply_pulse_rgb)
// The interface manager's other .rdata layout tables, per resolution_index (decomp48; names ours).
extern const int ui_choice_popup_title_gap[3];                      // 0x57ef7c {5, 5, 5} below the title
extern const int ui_message_box_size[3][2];                         // 0x57ef88 w, h of MessagePopupBkgrnd2
extern const int ui_message_box_title_pos[3][2];                    // 0x57efa0 from the box corner
extern const int ui_message_box_text_pos[3][2];                     // 0x57efb8 (only y is read: the text is centred)
extern const int ui_message_box_text_size[3][2];                    // 0x57efd0 wrap width, height (the buttons sit at its bottom)
extern const int hud_message_y[3];                                  // 0x57efe8 {10, 13, 16}
extern const int hud_chain_message_pos[3][2];                       // 0x57eff4

// ---- message box, choice popup, tooltip ---------------------------------------------------------------
// a = flags (bit 2 stores b), b = an int array of entry values, c = a bool array of enabled entries
// (new_skill.cpp, new_inventory.cpp pass the arrays cast to int; D103).
void ui_choice_popup_show(const char *title, int x, int y, int num, char **items,
                          int a, int b, int c);                     // 0x4136a0
int  ui_choice_popup_frame(int key);                                // 0x4138b0: choice, -1 none yet, -2 cancelled
void ui_choice_popup_draw();                                        // 0x413c70
bool ui_choice_popup_is_active();                                   // 0x413e20
void ClearFrameFlag5bd77c();                                        // 0x4138a0 (ui_choice_popup_flags)
void ClearFrameFlag5bd504();                                        // 0x414020 (the message box's flag)
extern unsigned int ui_choice_popup_flags;                          // 0x5bd77c bit 31 shown
extern const int ui_choice_popup_margin[3];                         // 0x57ef64 {5, 5, 5} per resolution_index (name ours)
extern const int ui_choice_popup_margin_y[3];                       // 0x57ef70 {3, 3, 3} (sum.symbols module new_hud.cpp)
void ui_message_box_show(const char *title, const char *text, int x, int y, int num_buttons,
                         char **buttons, void (*callback)(int), int flags);   // 0x413e40 (last argument: 0 or 2)
bool ui_message_box_is_active();                                    // 0x4144d0
void ui_tooltip_set(int x, int y, int w, int h, const char *text);  // 0x414750 (sum.symbols says (x, y, text))
// The popups' state machines (decomp48). The choice popup: mouse mode 0 hover / press, 1 held, 2 an entry
// dragged out (flag 4: hud_drag_icon_begin with the entry's value); flags bit 0 centred title, 1 centred
// entries, 2 values given, 3 no cancel by Esc / click outside, 30 pressed entry under the mouse, 31 shown.
int  ui_choice_popup_handle_click(int key);                         // 0x413900
int  ui_choice_popup_hit_test();                                    // 0x413970 entry under the mouse, -1
int  ui_choice_popup_update_buttons();                              // 0x413a90
int  ui_choice_popup_wait_release();                                // 0x413c50
// The message box: flags bit 0 pauses the game (single player), 1 no cancel by Esc, 30 / 31 as above.
// frame returns true while the box is open (interface_frame then drops the key).
bool ui_message_box_frame(int key);                                 // 0x414050
int  ui_message_box_handle_click(int key);                          // 0x4140a0
int  ui_message_box_hit_test();                                     // 0x414110 button under the mouse, -1
int  ui_message_box_update_buttons();                               // 0x414200
void ui_message_box_draw();                                         // 0x414270
extern unsigned int ui_message_box_flags;                           // 0x5bd504
extern int   ui_choice_popup_mouse_mode;                            // 0x5bd78c
extern int   ui_message_box_mouse_mode;                             // 0x5bd930
extern bool  ui_tooltip_visible;                                    // 0x5bd948

// ---- HUD (the new_hud.cpp functions are in summoner/interface/new_hud.h) ---------------------------
void hud_show_message(const char *text);                            // 0x414a20
// The chain attack line (15 characters, 4000 ms; only caller chain_attack_select). decomp35.
void hud_show_chain_message(const char *text);                      // 0x414a80
extern int hud_target_mode;                                         // 0x58bdb8 (-1 none, 0, 2; in this unit's .data)

// ---- 3D model views ---------------------------------------------------------------------------------
// The model's position: the lights of "inv-lights1.vfx" are placed relative to it (decomp48).
void ui_3d_begin(vector3 *pos);                                     // 0x412400
void ui_3d_end();                                                   // 0x4125a0

// new_icam: a camera table (status_cameras.tbl, bs_cameras.tbl, ...; 0x3628 bytes). new_status.cpp keeps
// its table as this class (its inline empty constructor explains the empty initialiser 0x42b1d0),
// new_buysell.cpp as a byte array, so the functions take void* (DECISIONS.md D18).
class new_icam_table {
public:
    new_icam_table() {}
    ubyte data[0x3628];
};
void new_icam_load(void *table, const char *filename);                          // 0x410760 (return unused)
void new_icam_get(void *table, const char *name, matrix *orient, vector3 *pos); // 0x410b70
int  new_icam_find(void *table, const char *name);                              // 0x410af0 entry or -1 (decomp73)
// The three are game/unnamed_icam_0x410760.cpp (file name a placeholder; decomp73).

// ---- status screen helpers (interface core) -----------------------------------------------------------
void statscreen_refresh_anim(Character *c);                         // 0x412e50 (0 from most callers)
// skip_summon: the argument goes on unchanged to party_find_*_member, which reads it as a byte (D2801).
void statscreen_show_next_member(bool skip_summon);                 // 0x413320 (1 here)
void statscreen_show_prev_member(bool skip_summon);                 // 0x413340 (1 here)
void statscreen_set_reduced_textures(int reduced);                  // 0x412dd0 (bm_set_reduced on the model's textures)
void statscreen_clone_instance(int loco_anim);                      // 0x412f20 -1: no animation set
void statscreen_restore_instance();                                 // 0x4131c0
// The next / previous party member on this level after status_character (skipping hidden, dead and
// summon-like members, and the summon too when skip_summon is set); status_character when none.
Character *party_find_next_member(bool skip_summon);                // 0x413200
Character *party_find_prev_member(bool skip_summon);                // 0x413290
extern character_instance ui_character_instance;                    // 0x5bd820 the status screen's copy
extern character_instance *statscreen_saved_instance;               // 0x5bd958 (sum.symbols: void*)

// ---- item info popup (new_inventory.cpp, after the pilot's range) ---------------------------------------
struct item_info;
void item_info_popup_load_bitmaps();                                // 0x4185a0 (interface_load_all; decomp48)
void item_info_popup_open_item(item_info *item);                    // 0x418760
void item_info_popup_open_type(int type);                           // 0x418920
int  item_info_popup_frame(int key);                                // 0x418960
void item_info_popup_render();                                      // 0x418e40
bool item_info_popup_is_active();                                   // 0x419160

// ---- chat (new_multichat.cpp) -----------------------------------------------------------------------
// The unit's functions and globals (chat_input_cancel 0x41ed50, ...) are in new_multichat.h (decomp44).
#include "summoner/interface/new_multichat.h"

// ---- the world map overlay (0x42f320-0x42ffdf, after new_tutorial.cpp; unit not known; decomp48) -----
void worldmap_load_overlay();                                       // 0x42f320 (interface_load_all; level 4 only)
int  worldmap_frame(int key);                                       // 0x42f500 (interface_frame on the HUD of level 4)
void worldmap_draw_locations();                                     // 0x42fa80 (interface_draw, the same case)

// ---- interface colours (static Color objects of the interface core) -----------------------------------
extern Color ui_color_text;                                         // 0x5bd2ec
extern Color ui_color_white;                                        // 0x5bd324
extern Color ui_color_blue;                                         // 0x5bd328 (110, 138, 177)
extern Color ui_color_dark_blue;                                    // 0x5bd790
extern Color ui_color_disabled;                                     // 0x5bd794 (90, 90, 90)
extern Color ui_color_gold;                                         // 0x5bd7a0
extern Color ui_color_highlight;                                    // 0x5bd808
extern Color ui_color_dark_green;                                   // 0x5bd8f0 (8, 100, 0, 255)
extern Color ui_color_green;                                        // 0x5bd8f8
extern Color ui_color_dark_red;                                     // 0x5bd924
extern Color ui_color_magenta;                                      // 0x5bd928 (190, 80, 190)
extern Color ui_color_red;                                          // 0x5bd954
// decomp48: the rest of the static colours (construction order: interface manager .cpp).
extern Color ui_color_chain_message;                                // 0x5bd7a8 (211, 106, 106)
extern Color ui_color_black;                                        // 0x5bd314 (0, 0, 0, 255; no reader found; name ours)
extern Color ui_color_panel_shadow;                                 // 0x5bd944 (0, 0, 0, 50)
extern Color ui_color_panel_frame;                                  // 0x5bd7b8 (110, 110, 110)
extern Color ui_color_panel_inner;                                  // 0x5bd7fc (0, 0, 0)

// ---- interface calls of gameplay.cpp (decomp42) -------------------------------------------------------
void interface_load_all();                                           // 0x4118e0 (state_game_enter)
void interface_draw();                                               // 0x411cd0 (game_render_frame)
void interface_set_view_clip();                                      // 0x412220 (state_game_frame)
void ui_model_lights_reset();                                        // 0x4123f0 (level_load)
bool interface_screen_is_fullscreen();                               // 0x412ab0 (game_render_frame)
void game_draw_leader_marker();                                      // 0x4145e0 (state_game_frame)
extern Color ui_color_loading_bar;                                   // 0x5bd7ac (36, 121, 28, 255)

#endif
