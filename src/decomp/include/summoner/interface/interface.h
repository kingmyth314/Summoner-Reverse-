// summoner/interface/interface.h - what the hand-made interface screens (new_*.cpp) share: the screen
// switch, the button records, scaling, message boxes, choice popups, tooltips, the 3D model views, the
// interface colours and the helpers of the HUD / inventory / status units that other screens call.
// Units: the interface core 0x412400-0x414bb0 (new_hud.cpp / new_inventory.cpp by string ids; the file
// that holds interface_set_screen is not known), new_inventory.cpp (item info popup 0x418920-0x419160),
// new_multichat.cpp (chat_input_cancel 0x41ed50), new_int_multi_update.cpp area (new_icam 0x410760).
// One header for now; directory and file name ours. Not original source.
#ifndef SUMMONER_INTERFACE_INTERFACE_H
#define SUMMONER_INTERFACE_INTERFACE_H

#include "vsdk/vsdk.h"
#include "vsdk/gr/gr.h"
#include "vsdk/math/matrix.h"
#include "summoner/summoner.h"

class Character;

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
extern Character *status_character;                                 // 0x5bd810 (new_status.cpp)
extern bool  ui_input_locked;                                       // 0x5bd962 (the HUD is hidden)
extern float interface_pulse_time;                                  // 0x58bdc4 (1.0 + frame time, for blinking)
void  party_cycle(int forward);                                     // 0x413360 Tab (1) / Shift+Tab (0)

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
void  ui_bitmap_scaled(int bm, int x, int y, gr_mode mode);         // 0x4144f0
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

// ---- HUD (the new_hud.cpp functions are in summoner/interface/new_hud.h) ---------------------------
void hud_show_message(const char *text);                            // 0x414a20
extern int hud_target_mode;                                         // 0x58bdb8 (-1 none, 0, 2; in this unit's .data)

// ---- 3D model views ---------------------------------------------------------------------------------
void ui_3d_begin(vector3 *pos);                                     // 0x412400 (argument type guessed)
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

// ---- status screen helpers (interface core) -----------------------------------------------------------
void statscreen_refresh_anim(Character *c);                         // 0x412e50 (0 from most callers)
void statscreen_show_next_member(int a);                            // 0x413320 (1 here)
void statscreen_show_prev_member(int a);                            // 0x413340 (1 here)

// ---- item info popup (new_inventory.cpp, after the pilot's range) ---------------------------------------
struct item_info;
void item_info_popup_open_item(item_info *item);                    // 0x418760
void item_info_popup_open_type(int type);                           // 0x418920
int  item_info_popup_frame(int key);                                // 0x418960
void item_info_popup_render();                                      // 0x418e40
bool item_info_popup_is_active();                                   // 0x419160

// ---- chat (new_multichat.cpp) -----------------------------------------------------------------------
void chat_input_cancel();                                           // 0x41ed50

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

#endif
