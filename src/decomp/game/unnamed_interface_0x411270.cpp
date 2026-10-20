// unnamed_interface_0x411270.cpp - Sum.exe v1.40, code 0x411270-0x414d20: the interface manager (the screen
// switch interface_set_screen with every screen's init / leave / frame / draw dispatch, the 3D view clip,
// the status screen's character model copy, party cycling, the panel frame and text box helpers, the
// choice popup, the message box, the tooltip, the HUD message lines, the UI model lights, the cutscene
// loading bar, the pulse colours and the show_ui console command).
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl section, sum.symbols module "-"; docs/sum-exe/source-map.md "Hand cuts inside the unnamed
// units"). Rename the file when the name turns up.
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .rdata 0x57ef40-0x57f03f (layout tables, then the float literals 0x57f00c-0x57f038), .data
// 0x58bda4-0x58c06f (ui_bitmap_suffix_tga at 0x58bda4 is the first object: its strings are in this unit's
// literal pool), .bss 0x5bd2e0-0x5bd96f. See NOTES.md for the placement evidence.

#include <string.h>
#include <stdio.h>
#include <math.h>
#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/key.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/frametime.h"
#include "vsdk/os/console.h"
#include "vsdk/ca/character.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/geom/vfx.h"
#include "vsdk/geom/effect_mem.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "engine/gamesound/gamesound.h"
#include "engine/camera/camera.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/objects/living_entity.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/rendering/levelrender.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"
#include "levelscripts/level_scripts.h"
#include "summoner/interface/new_assess.h"
#include "summoner/interface/new_automap.h"
#include "summoner/interface/new_buysell.h"
#include "summoner/interface/new_charselect.h"
#include "summoner/interface/new_credits.h"
#include "summoner/interface/new_dialogue.h"
#include "summoner/interface/new_gamecreate.h"
#include "summoner/interface/new_gamelist.h"
#include "summoner/interface/new_help.h"
#include "summoner/interface/new_hud.h"
#include "summoner/interface/new_inventory.h"
#include "summoner/interface/new_itemtrade.h"
#include "summoner/interface/new_journal.h"
#include "summoner/interface/new_levelselect.h"
#include "summoner/interface/new_mainmenu.h"
#include "summoner/interface/new_multi.h"
#include "summoner/interface/new_options.h"
#include "summoner/interface/new_pickup.h"
#include "summoner/interface/new_playmenu.h"
#include "summoner/interface/new_pxochat.h"
#include "summoner/interface/new_saveload.h"
#include "summoner/interface/new_skill.h"
#include "summoner/interface/new_spell.h"
#include "summoner/interface/new_status.h"
#include "summoner/interface/new_tutorial.h"

#define NUM_SCREENS            23
#define UI_TOOLTIP_TEXT_LEN    64
#define UI_CHOICE_MAX          16
#define UI_MESSAGE_BOX_BUTTONS 3
#define UI_MAX_MODEL_LIGHTS    10
#define LOCO_INVENTORY         3       // locomotion_anim_names index the inventory model plays
#define LOCO_STATSCREEN        17      // "statscreen" (Character::anim_statscreen)
#define SCREEN_HUD             0
#define SCREEN_DIALOGUE        13
#define SCREEN_MAIN_MENU       14
#define SCREEN_ITEMTRADE       19

// ---- .rdata (0x57ef40) ---------------------------------------------------------------------------------
// Per resolution_index (640, 800, 1024); names ours.
const int ui_frame_border[3]               = { 2, 2, 2 };                                   // 0x57ef40
const int ui_frame_inset_x[3]              = { 8, 8, 8 };                                   // 0x57ef4c shadow offset
const int ui_frame_inset_y[3]              = { 5, 5, 5 };                                   // 0x57ef58
const int ui_choice_popup_margin[3]        = { 5, 5, 5 };                                   // 0x57ef64
const int ui_choice_popup_margin_y[3]      = { 3, 3, 3 };                                   // 0x57ef70
const int ui_choice_popup_title_gap[3]     = { 5, 5, 5 };                                   // 0x57ef7c
const int ui_message_box_size[3][2]        = { { 293, 165 }, { 366, 206 }, { 469, 264 } };  // 0x57ef88
const int ui_message_box_title_pos[3][2]   = { { 8, 6 }, { 10, 8 }, { 13, 11 } };           // 0x57efa0
const int ui_message_box_text_pos[3][2]    = { { 8, 45 }, { 10, 56 }, { 13, 72 } };         // 0x57efb8
const int ui_message_box_text_size[3][2]   = { { 280, 101 }, { 350, 126 }, { 448, 162 } };  // 0x57efd0
const int hud_message_y[3]                 = { 10, 13, 16 };                                // 0x57efe8
const int hud_chain_message_pos[3][2]      = { { 143, 163 }, { 179, 204 }, { 229, 261 } };  // 0x57eff4
// 0x57f00c-0x57f038 are the float literals 0.875, 0.0015625, 1024.0, 800.0, 640.0, 0.00025, 9.424778,
// 3.1415927, 1/256, 1/255, 360.0 and 90.0 (__real COMDATs in order of first use); 0.5 (0x57e6b0), 1.0
// (0x57e990) and 0.0 (0x57eb84) come from earlier units. 0x57f03c is a zero dword (padding).

// ---- .data (0x58bda4) ----------------------------------------------------------------------------------
// The literal pool after the data (0x58bea8-0x58c06f) holds the bitmap names in reverse order of the
// tables, then the function literals.
char *ui_bitmap_suffix_tga[2]        = { "640.tga", "1024.tga" };                    // 0x58bda4
char *ui_bitmap_suffix[2]            = { "640", "1024" };                            // 0x58bdac
int   cursoricon_chain_bm            = -1;                                           // 0x58bdb4
int   hud_target_mode                = -1;                                           // 0x58bdb8
float ui_pick_override_x             = 1.0f;                                         // 0x58bdbc
float ui_pick_override_y             = 1.0f;                                         // 0x58bdc0
float interface_pulse_time           = 1.0f;                                         // 0x58bdc4
void (*screen_refresh_table[NUM_SCREENS])() = {                                      // 0x58bdc8
    NULL, inventory_refresh_for_character, status_refresh_for_character, NULL, NULL,
    spells_refresh_for_character, skills_refresh_for_character,
};
float ui_scale                       = 1.0f;                                         // 0x58be24
float ui_scale_640                   = 1.0f;                                         // 0x58be28
char *message_popup_bkgrnd_names[2]  = { "MessagePopupBkgrnd2640.tga",
                                         "MessagePopupBckgrnd21024.tga" };           // 0x58be2c (sic, the file exists)
int   cursorspot_focus_bm            = -1;                                           // 0x58be34
int   cursorspot_focus_large_bm      = -1;                                           // 0x58be38
int   ui_tooltip_offset_y[3]         = { -5, -6, -8 };                               // 0x58be3c (name ours)
int   ui_tooltip_pad_left            = 1;                                            // 0x58be48
int   ui_tooltip_pad_right           = 0;                                            // 0x58be4c
int   ui_tooltip_pad_top             = 1;                                            // 0x58be50
int   ui_tooltip_pad_bottom          = 1;                                            // 0x58be54
char *cutscene_loading_names[2]      = { "LoadingPopup640.tga", "LoadingPopup1024.tga" };   // 0x58be58 (name ours)
int   cutscene_loading_bg_pos[3][2]  = { { 145, 190 }, { 181, 238 }, { 232, 304 } }; // 0x58be60 (names ours)
int   cutscene_loading_bar_pos[3][2] = { { 160, 248 }, { 199, 310 }, { 254, 395 } }; // 0x58be78
int   cutscene_loading_bar_size[3][2] = { { 320, 28 }, { 400, 36 }, { 516, 45 } };   // 0x58be90

// ---- .bss (0x5bd2e0-0x5bd96f) ----------------------------------------------------------------------------
// The objects with constructors, in the order of their static initialisers (static_initializers entries
// 25-48; entry 49 is ccmd_show_ui at the end of the file). Each initialiser is a thunk (jmp) and a body.
// 0x411270  init_color_5bd2ec_thunk (entry 25, jmp). Confidence: exact (compiler-generated).
// 0x411280  init_color_5bd2ec: ui_color_text's constructor call. Confidence: exact (compiler-generated).
Color ui_color_text(180, 180, 180, 255);                // 0x5bd2ec
// 0x4112a0  init_color_5bd808_thunk (entry 26). Confidence: exact (compiler-generated).
// 0x4112b0  init_color_5bd808. Confidence: exact (compiler-generated).
Color ui_color_highlight(255, 255, 158, 255);           // 0x5bd808
// 0x4112d0  init_color_5bd794_thunk (entry 27). Confidence: exact (compiler-generated).
// 0x4112e0  init_color_5bd794. Confidence: exact (compiler-generated).
Color ui_color_disabled(90, 90, 90, 255);               // 0x5bd794
// 0x411300  init_color_5bd324_thunk (entry 28). Confidence: exact (compiler-generated).
// 0x411310  init_color_5bd324. Confidence: exact (compiler-generated).
Color ui_color_white(255, 255, 255, 255);               // 0x5bd324
// 0x411330  init_color_5bd7a8_thunk (entry 29). Confidence: exact (compiler-generated).
// 0x411340  init_color_5bd7a8. Confidence: exact (compiler-generated).
Color ui_color_chain_message(211, 106, 106, 255);       // 0x5bd7a8
// 0x411360  init_color_5bd954_thunk (entry 30). Confidence: exact (compiler-generated).
// 0x411370  init_color_5bd954. Confidence: exact (compiler-generated).
Color ui_color_red(191, 0, 0, 255);                     // 0x5bd954
// 0x411390  init_color_5bd924_thunk (entry 31). Confidence: exact (compiler-generated).
// 0x4113a0  init_color_5bd924. Confidence: exact (compiler-generated).
Color ui_color_dark_red(110, 0, 0, 255);                // 0x5bd924
// 0x4113c0  init_color_5bd8f0_thunk (entry 32). Confidence: exact (compiler-generated).
// 0x4113d0  init_color_5bd8f0. Confidence: exact (compiler-generated).
Color ui_color_dark_green(8, 100, 0, 255);              // 0x5bd8f0
// 0x4113f0  init_color_5bd8f8_thunk (entry 33). Confidence: exact (compiler-generated).
// 0x411400  init_color_5bd8f8. Confidence: exact (compiler-generated).
Color ui_color_green(11, 226, 11, 255);                 // 0x5bd8f8
// 0x411420  init_color_5bd328_thunk (entry 34). Confidence: exact (compiler-generated).
// 0x411430  init_color_5bd328. Confidence: exact (compiler-generated).
Color ui_color_blue(110, 138, 177, 255);                // 0x5bd328
// 0x411450  init_color_5bd790_thunk (entry 35). Confidence: exact (compiler-generated).
// 0x411460  init_color_5bd790. Confidence: exact (compiler-generated).
Color ui_color_dark_blue(10, 23, 115, 255);             // 0x5bd790
// 0x411480  init_color_5bd7ac_thunk (entry 36). Confidence: exact (compiler-generated).
// 0x411490  init_color_5bd7ac. Confidence: exact (compiler-generated).
Color ui_color_loading_bar(36, 121, 28, 255);           // 0x5bd7ac
// 0x4114b0  init_color_5bd928_thunk (entry 37). Confidence: exact (compiler-generated).
// 0x4114c0  init_color_5bd928. Confidence: exact (compiler-generated).
Color ui_color_magenta(190, 80, 190, 255);              // 0x5bd928
// 0x4114e0  init_color_5bd7a0_thunk (entry 38). Confidence: exact (compiler-generated).
// 0x4114f0  init_color_5bd7a0. Confidence: exact (compiler-generated).
Color ui_color_gold(212, 165, 51, 255);                 // 0x5bd7a0
// 0x411510  init_color_5bd938_thunk (entry 39). Confidence: exact (compiler-generated).
// 0x411520  init_color_5bd938. Confidence: exact (compiler-generated).
Color ui_color_text_box_bg(0, 0, 0, 196);               // 0x5bd938
// 0x411540  init_color_5bd314_thunk (entry 40). Confidence: exact (compiler-generated).
// 0x411550  init_color_5bd314. Confidence: exact (compiler-generated).
Color ui_color_black(0, 0, 0, 255);                     // 0x5bd314 (no reader found)
// 0x411570  init_empty_411580_thunk (entry 41). Confidence: exact (compiler-generated).
// 0x411580  init_empty_411580: a bare ret, read as the dynamic initialiser of the one vector3 of the unit
// (the inline empty vector3() constructor, as in ai.cpp). Confidence: unsure (which object it belongs to).
vector3 ui_pick_world_override_pos;                     // 0x5bd2f8
// 0x411590  sinit_character_instance_5bd820 (entry 42): constructs, then registers the destructor.
// Confidence: exact (compiler-generated).
// 0x4115a0  init_character_instance_5bd820: character_instance() (0x5196b0). Confidence: exact (compiler-generated).
// 0x4115b0  register_dtor_character_instance_5bd820: atexit(0x4115c0). Confidence: exact (compiler-generated).
// 0x4115c0  dtor_character_instance_5bd820: ~character_instance() (0x519840). Confidence: exact (compiler-generated).
character_instance ui_character_instance;               // 0x5bd820 the status / inventory screens' model copy
// 0x4115d0  init_color_5bd944_thunk (entry 43). Confidence: exact (compiler-generated).
// 0x4115e0  init_color_5bd944. Confidence: exact (compiler-generated).
Color ui_color_panel_shadow(0, 0, 0, 50);               // 0x5bd944
// 0x411600  init_color_5bd7b8_thunk (entry 44). Confidence: exact (compiler-generated).
// 0x411610  init_color_5bd7b8. Confidence: exact (compiler-generated).
Color ui_color_panel_frame(110, 110, 110, 255);         // 0x5bd7b8
// 0x411630  init_color_5bd7fc_thunk (entry 45). Confidence: exact (compiler-generated).
// 0x411640  init_color_5bd7fc. Confidence: exact (compiler-generated).
Color ui_color_panel_inner(0, 0, 0, 255);               // 0x5bd7fc
// 0x411660  init_realtimetimer_5bd950_thunk (entry 46). Confidence: exact (compiler-generated).
// 0x411670  init_realtimetimer_5bd950. Confidence: exact (compiler-generated).
RealtimeTimer interface_input_delay_timer;              // 0x5bd950 600 ms after a screen opens / the mouse moves
// 0x411680  init_gametimer_5bd80c_thunk (entry 47). Confidence: exact (compiler-generated).
// 0x411690  init_gametimer_5bd80c. Confidence: exact (compiler-generated).
GameTimer hud_message_timer;                            // 0x5bd80c
// 0x4116a0  init_gametimer_5bd81c_thunk (entry 48). Confidence: exact (compiler-generated).
// 0x4116b0  init_gametimer_5bd81c. Confidence: exact (compiler-generated).
GameTimer hud_chain_message_timer;                      // 0x5bd81c

// The rest of the .bss, by address (the order of the definitions in the original is not known).
int   ui_tooltip_rect_w;                                // 0x5bd2e0 the hover rectangle of the tooltip's owner
int   ui_tooltip_rect_h;                                // 0x5bd2e4
float ui_3d_saved_ambient_b;                            // 0x5bd2e8
float ui_3d_saved_ambient_r;                            // 0x5bd2f0
float ui_3d_saved_ambient_g;                            // 0x5bd2f4
char  hud_chain_message_text[16];                       // 0x5bd304
int   ui_hires;                                         // 0x5bd318
int   ui_tooltip_rect_x;                                // 0x5bd31c
int   ui_tooltip_rect_y;                                // 0x5bd320
char  hud_message_text[68];                             // 0x5bd32c
// The message box's state is one object too (D8603): ui_message_box_show re-reads the button count after
// each label copy (0x413e8b), as the choice popup. ui_message_box_flags (0x5bd504, declared in interface.h)
// would be its next field; it stays a separate global here. Struct and field names ours.
struct ui_message_box_state {
    char  title[0x21];                                  // 0x5bd370
    char  text[0x100];                                  // 0x5bd391
    int   x;                                            // 0x5bd494
    int   y;                                            // 0x5bd498
    int   button_count;                                 // 0x5bd49c
    char  button_labels[UI_MESSAGE_BOX_BUTTONS][32];    // 0x5bd4a0
    int   hover_button;                                 // 0x5bd500
};
ui_message_box_state ui_message_box;                    // 0x5bd370
#define ui_message_box_title         ui_message_box.title
#define ui_message_box_text          ui_message_box.text
#define ui_message_box_x             ui_message_box.x
#define ui_message_box_y             ui_message_box.y
#define ui_message_box_button_count  ui_message_box.button_count
#define ui_message_box_button_labels ui_message_box.button_labels
#define ui_message_box_hover_button  ui_message_box.hover_button
unsigned int ui_message_box_flags;                      // 0x5bd504
void (*ui_message_box_callback)(int);                   // 0x5bd508
bool  ui_using_equipped_item;                           // 0x5bd50c
int   cutscene_loading_total;                           // 0x5bd510
party_inventory_entry *ui_using_list_item_entry;        // 0x5bd514
// The choice popup's state is one object (D8603): ui_choice_popup_show re-reads the count after each store
// into the choice table (0x413706-0x413717), which 8966 does for fields of one object only (as D7201).
// Struct and field names ours; the old names are macros.
struct ui_choice_popup_state {
    char  title[0x201];                                 // 0x5bd518 (strncpy 0x200 and the terminator at 0x5bd718)
    int   x;                                            // 0x5bd71c
    int   y;                                            // 0x5bd720
    int   w;                                            // 0x5bd724
    int   h;                                            // 0x5bd728
    int   count;                                        // 0x5bd72c
    char *choices[UI_CHOICE_MAX];                       // 0x5bd730
    int  *values;                                       // 0x5bd770
    bool *enabled;                                      // 0x5bd774
    int   hover;                                        // 0x5bd778
};
LAYOUT_CHECK(ui_choice_popup_state, sizeof(ui_choice_popup_state) == 0x264);
ui_choice_popup_state ui_choice_popup;                  // 0x5bd518
#define ui_choice_popup_title   ui_choice_popup.title
#define ui_choice_popup_x       ui_choice_popup.x
#define ui_choice_popup_y       ui_choice_popup.y
#define ui_choice_popup_w       ui_choice_popup.w
#define ui_choice_popup_h       ui_choice_popup.h
#define ui_choice_popup_count   ui_choice_popup.count
#define ui_choice_popup_choices ui_choice_popup.choices
#define ui_choice_popup_values  ui_choice_popup.values
#define ui_choice_popup_enabled ui_choice_popup.enabled
#define ui_choice_popup_hover   ui_choice_popup.hover
unsigned int ui_choice_popup_flags;                     // 0x5bd77c
int   ui_choice_popup_mouse_mode;                       // 0x5bd78c (after ccmd_show_ui 0x5bd780)
int   ui_tooltip_x;                                     // 0x5bd798
int   ui_tooltip_y;                                     // 0x5bd79c
float ui_pulse_rgb_phase;                               // 0x5bd7a4
bool  ui_using_list_item;                               // 0x5bd7b0
int   cursoricon_quest_bm;                              // 0x5bd7b4
char  ui_tooltip_text[UI_TOOLTIP_TEXT_LEN];             // 0x5bd7bc
int   ui_tooltip_text_w;                                // 0x5bd800
int   ui_tooltip_text_h;                                // 0x5bd804
Character *status_character;                            // 0x5bd810
int   interface_cur_screen;                             // 0x5bd814
float interface_view_fov;                               // 0x5bd818
int   ui_pending_use_skill;                             // 0x5bd8ec
float ui_pulse_scale_phase;                             // 0x5bd8f4
int   ui_3d_lights[UI_MAX_MODEL_LIGHTS];                // 0x5bd8fc
int   cutscene_loading_progress;                        // 0x5bd92c
int   ui_message_box_mouse_mode;                        // 0x5bd930
int   cursoricon_incidental_bm;                         // 0x5bd934
int   ui_using_equipped_slot;                           // 0x5bd93c
int   ui_3d_saved_max_lights;                           // 0x5bd940
bool  ui_tooltip_visible;                               // 0x5bd948
int   cutscene_loading_bm;                              // 0x5bd94c
character_instance *statscreen_saved_instance;          // 0x5bd958
int   message_popup_bkgrnd_bm;                          // 0x5bd95c
bool  ui_pick_screen_override_on;                       // 0x5bd960
bool  ui_pick_world_override_on;                        // 0x5bd961
bool  ui_input_locked;                                  // 0x5bd962
int   ui_3d_light_count;                                // 0x5bd964
vfx_instance *ui_model_lights_vfx;                      // 0x5bd968
float leader_marker_phase;                              // 0x5bd96c

// 0x4116c0  Confidence: likely (the two resolution switches: the first is lowered as range tests with the
// 0 / 1 cases and the default on one block). GameInit: the UI scale, the 3D view, the screens' one-time
// set-up in file order (the empty bodies are folded into empty_function) and the cursor / popup bitmaps.
void interface_init_global()
{
    float base_w;
    float screen_w;
    float h_fov;

    switch (resolution_index) {
    case 2:
        ui_hires = 1;
        base_w = 1024.0f;
        break;
    case 0:
    case 1:
        ui_hires = 0;
        base_w = 640.0f;
        break;
    default:
        ui_hires = 0;
        base_w = 640.0f;
        break;
    }
    screen_w = 640.0f;
    switch (resolution_index) {
    case 1:
        screen_w = 800.0f;
        break;
    case 2:
        screen_w = 1024.0f;
        break;
    }
    ui_scale = screen_w / base_w;
    status_character = NULL;
    interface_cur_screen = 0;
    ui_scale_640 = screen_w * 0.0015625f;
    gr::set_clip(0, 0, gr_screen_width(), (int)(gr_screen_height() * 0.875f + 0.5f));
    gr_setup_3d(&camera_view_orient, &camera_view_pos, camera_fov, 1, 1);
    gr_get_view_fov(&h_fov, &interface_view_fov);
    ui_choice_popup_flags = 0;
    ui_message_box_flags = 0;
    empty_function();                   // new_assess.cpp (empty)
    empty_function();                   // new_automap.cpp (empty)
    bs_cameras_parse_table();           // new_buysell.cpp
    mp_charcreate_load_bitmaps();       // new_charselect.cpp
    empty_function();                   // new_credits.cpp (empty)
    empty_function();                   // new_dialogue.cpp (empty)
    gamecreate_load_bitmaps();
    gamelist_load_bitmaps();
    empty_function();                   // new_hud.cpp (empty)
    inv_cameras_parse_table();          // new_inventory.cpp
    empty_function();                   // new_itemtrade.cpp (empty)
    empty_function();                   // new_journal.cpp (empty)
    mp_levelselect_load_bitmaps();
    mainmenu_load_bitmaps();
    options_load_bitmaps();
    empty_function();                   // new_pickup.cpp (empty)
    empty_function();                   // new_playmenu.cpp (empty)
    pxochat_load_bitmaps();
    saveload_load_bitmaps();
    empty_function();                   // new_skill.cpp (empty)
    empty_function();                   // new_spell.cpp (empty)
    status_cameras_parse_table();       // new_status.cpp
    empty_function();                   // new_tutorial.cpp (empty)
    cursoricon_chain_bm = bm_load("cursoricon-chain.tga", -1, -1);
    cursoricon_incidental_bm = bm_load("cursoricon-incidental.vbm", -1, -1);
    cursoricon_quest_bm = bm_load("cursoricon-quest.vbm", -1, -1);
    cursorspot_focus_bm = bm_load("cursorspot-focus.tga", -1, -1);
    cursorspot_focus_large_bm = bm_load("cursorspot-focus-large.tga", -1, -1);
    message_popup_bkgrnd_bm = bm_load(message_popup_bkgrnd_names[ui_hires], -1, -1);
    cutscene_loading_bm = bm_load(cutscene_loading_names[ui_hires], -1, -1);
}

// 0x4118c0  Confidence: exact (memset is the inline rep stosd of 0x33 dwords). game_cleanup.
void interface_reset_state()
{
    memset((void *)&ui_character_instance, 0, sizeof(ui_character_instance));
    party_inventory_clear();
    game_unpause();
}

// 0x4118e0  Confidence: exact. state_game_enter: back to the HUD and every screen's bitmaps (file order).
void interface_load_all()
{
    status_character = NULL;
    interface_set_screen(0, 0);
    assess_load_bitmaps();
    automap_load_bitmaps();
    buysell_load_bitmaps();
    empty_function();                   // new_charselect.cpp (empty)
    empty_function();                   // new_credits.cpp (empty)
    dialogue_load_bitmaps();
    empty_function();                   // new_gamecreate.cpp (empty)
    empty_function();                   // new_gamelist.cpp (empty)
    hud_load_bitmaps();
    inventory_load_bitmaps();
    item_info_popup_load_bitmaps();
    itemtrade_load_bitmaps();
    journal_load_bitmaps();
    empty_function();                   // new_levelselect.cpp (empty)
    empty_function();                   // new_mainmenu.cpp (empty)
    chat_reset();                       // new_multichat.cpp
    empty_function();                   // new_options.cpp (empty)
    pickup_load_bitmaps();
    playmenu_load_bitmaps();
    empty_function();                   // new_pxochat.cpp (empty)
    empty_function();                   // new_saveload.cpp (empty)
    skills_load_bitmaps();
    spells_load_bitmaps();
    status_load_bitmaps();
    help_reset_page();
    tutorial_reset();
    worldmap_load_overlay();
    hud_message_timer.invalidate();
    hud_chain_message_timer.invalidate();
    hud_target_mode = -1;
}

// 0x4119a0  Confidence: exact. The frame input of the open screen; the HUD runs after every screen that is
// not full-screen. During a cutscene only Esc is handled (it ends the cutscene unless +NoAbort).
int interface_frame(int key)
{
    if (hud_message_timer.elapsed())
        hud_message_timer.invalidate();
    ui_tooltip_hide_on_mouse_activity(key);
    if (game_flags & GF_CUTSCENE) {
        if (key == KEY_ESC && !(cutscene_data->flags & 1)) {
            cutscene_cleanup();
            return 0;
        }
        return key;
    }
    if (ui_model_lights_vfx)
        ui_model_lights_vfx->id_bits &= ~4;
    if (ui_message_box_frame(key))
        key = 0;
    if (cur_level_index == 4 && interface_cur_screen == SCREEN_HUD)
        key = worldmap_frame(key);
    if ((game_flags & GF_MULTIPLAYER) && gameseq_get_state() == 4) {
        elevator_popup_frame();
        if (interface_cur_screen != SCREEN_ITEMTRADE || !itemtrade_gold_input_active)
            key = chat_input_frame(key);
        if (hud_target_mode == 1 && ui_pending_use_skill >= 0x26 && ui_pending_use_skill <= 0x35 && cur_summon)
            hud_target_mode = -1;
    }
    switch (interface_cur_screen) {
    case 1:  key = inventory_frame(key); break;
    case 2:  key = status_frame(key); break;
    case 3:  key = automap_frame(key); break;
    case 4:  key = pickup_frame(key); break;
    case 5:  key = spells_frame(key); break;
    case 6:  key = skills_frame(key); break;
    case 7:  key = buysell_frame(key); break;
    case 8:  key = saveload_frame(key); break;
    case 9:  key = playmenu_frame(key); break;
    case 10: key = journal_frame(key); break;
    case 11: key = pxochat_frame(key); break;
    case 12: key = options_frame(key); break;
    case 13: key = dialogue_frame(key); break;
    case 14: key = mainmenu_frame(key); break;
    case 15: key = gamelist_frame(key); break;
    case 16: key = mp_charselect_frame(key); break;
    case 17: key = gamecreate_frame(key); break;
    case 18: key = mp_levelselect_frame(key); break;
    case 19: key = itemtrade_frame(key); break;
    case 20: key = assess_frame(key); break;
    case 21: key = credits_frame(key); break;
    case 22: key = help_frame(key); break;
    }
    if (!interface_screen_is_fullscreen())
        key = hud_frame(key);
    return key;
}

// 0x411be0  Confidence: exact (VC6 byte match) (the order of the tests inside the two conditions). While the tooltip is
// shown, only the wheel, a button, a key or leaving the owner's rectangle hides it; while it is hidden any
// mouse activity restarts the 600 ms delay.
void ui_tooltip_hide_on_mouse_activity(int key)
{
    int dx, dy, dz;

    mouse_get_delta(&dx, &dy, &dz);
    if (ui_tooltip_visible) {
        if (!dz && !mouse_is_down(0) && !mouse_is_down(1) && !key
            && point_in_rect(game_mouse_x, game_mouse_y, ui_tooltip_rect_x, ui_tooltip_rect_y,
                             ui_tooltip_rect_w, ui_tooltip_rect_h))
            return;
    } else if (!dx && !dy && !dz && !mouse_is_down(0) && !mouse_is_down(1) && !key)
        return;
    ui_tooltip_visible = false;
    interface_input_delay_timer.SetTimeout(600);
}

// 0x411cd0  Confidence: exact. Nothing during a cutscene or in the script editor (game_flags 0x80).
void interface_draw()
{
    bool fullscreen;

    interface_pulse_update(reported_frametime);
    if (game_flags & (GF_CUTSCENE | 0x80))
        return;
    fullscreen = interface_screen_is_fullscreen();
    if (!ui_input_locked) {
        if (cur_level_index == 4 && interface_cur_screen == SCREEN_HUD)
            worldmap_draw_locations();
        if (!fullscreen)
            hud_draw();
    }
    switch (interface_cur_screen) {
    case 1:  inventory_draw(); break;
    case 2:  status_draw(); break;
    case 3:  automap_draw(); break;
    case 4:  pickup_draw(); break;
    case 5:  spells_draw(); break;
    case 6:  skills_draw(); break;
    case 7:  buysell_draw(); break;
    case 8:  saveload_draw(); break;
    case 9:  playmenu_draw(); break;
    case 10: journal_draw(); break;
    case 11: pxochat_draw(); break;
    case 12: options_draw(); break;
    case 13: dialogue_draw(); break;
    case 14: mainmenu_draw(); break;
    case 15: gamelist_draw(); break;
    case 16: mp_charselect_draw(); break;
    case 17: gamecreate_draw(); break;
    case 18: mp_levelselect_draw(); break;
    case 19: itemtrade_draw(); break;
    case 20: assess_draw(); break;
    case 21: credits_draw(); break;
    case 22: help_draw(); break;
    }
    ui_message_box_draw();
    if (!ui_input_locked && !fullscreen)
        hud_drag_icon_draw();
    ui_tooltip_draw();
    if (!fullscreen) {
        if (interface_cur_screen != SCREEN_DIALOGUE && hud_message_timer.valid())
            hud_draw_message();
        if (hud_chain_message_timer.valid())
            hud_draw_chain_message();
    }
    if ((game_flags & GF_MULTIPLAYER) && gameseq_get_state() == 4)
        chat_draw();
}

// 0x411ea0  Confidence: exact. Drawn over the whole screen, then the caller's clip is restored.
void ui_tooltip_draw()
{
    int clip_x, clip_y, clip_w, clip_h;

    if (!ui_tooltip_visible)
        return;
    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    gr::set_clip(0, 0, gr_screen_width(), gr_screen_height());
    ui_draw_text_box(ui_tooltip_x, ui_tooltip_y, ui_tooltip_text_w + ui_tooltip_pad_left + ui_tooltip_pad_right + 2,
                     ui_tooltip_text_h + ui_tooltip_pad_top + ui_tooltip_pad_bottom + 2);
    gr_set_color_ptr(&ui_color_text);
    gr_string(ui_tooltip_x + ui_tooltip_pad_left + 1, ui_tooltip_y + ui_tooltip_pad_top + 1, ui_tooltip_text, -1,
              gr_string_mode);
    gr::set_clip(clip_x, clip_y, clip_w, clip_h);
}

// 0x411f80  Confidence: exact (VC6 byte match) (one Color local or two: the binary reuses one stack slot). The message
// line across the top of the 3D view, fading over its last second (4000 ms timer, alpha = remaining / 4000).
void hud_draw_message()
{
    int clip_x, clip_y, clip_w, clip_h;
    int w, h;
    float alpha;
    Color c;

    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    gr_get_string_size(&w, &h, hud_message_text, -1, -1);
    alpha = hud_message_timer.GetRemainingTime() * 0.00025f;
    if (alpha >= 1.0f)
        alpha = 1.0f;
    else if (alpha <= 0.0f)
        alpha = 0.0f;
    c = ui_color_text_box_bg;
    c.a = (ubyte)(c.a * alpha + 0.5f);
    gr_set_color_ptr(&c);
    gr_rect(0, hud_message_y[resolution_index], clip_w, h, gr_rect_mode, 1.0f);
    c = ui_color_text;
    c.a = (ubyte)(c.a * alpha + 0.5f);
    gr_set_color_ptr(&c);
    gr_string((clip_w - w) / 2, hud_message_y[resolution_index], hud_message_text, -1, gr_string_mode);
}

// 0x4120d0  Confidence: exact (VC6 byte match) (the x clamp read as `if (x <= 0) x = 0`, jg). Not drawn at all while a
// tutorial page is shown.
void hud_draw_chain_message()
{
    int clip_x, clip_y, clip_w, clip_h;
    int x, y;
    float alpha;
    Color c;

    if (tutorial_is_active())
        return;
    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    x = (clip_w - gr_screen_width()) / 2 + hud_chain_message_pos[resolution_index][0];
    if (x <= 0)
        x = 0;
    y = hud_chain_message_pos[resolution_index][1];
    alpha = hud_chain_message_timer.GetRemainingTime() * 0.00025f;
    if (alpha >= 1.0f)
        alpha = 1.0f;
    else if (alpha <= 0.0f)
        alpha = 0.0f;
    c = ui_color_chain_message;
    c.a = (ubyte)(c.a * alpha + 0.5f);
    gr_set_color_ptr(&c);
    gr_string(x, y, hud_chain_message_text, -1, gr_string_mode);
}

// 0x4121d0  Confidence: exact (fsin inline). Two pulses in 0..1: one per 2 s and one three times as fast.
void interface_pulse_update(float frametime)
{
    interface_pulse_time += frametime;
    ui_pulse_scale_phase = (sinf(interface_pulse_time * 3.1415927f) + 1.0f) * 0.5f;
    ui_pulse_rgb_phase = (sinf(interface_pulse_time * 9.424778f) + 1.0f) * 0.5f;
}

// 0x412220  Confidence: exact (every screen 0..22 has a case, the whole-screen ones beside default: the jump
// table has six entries, the whole-screen block second; it is cross-jumped with default and the input-locked
// branch). The 3D
// view: letterboxed to 2:1 during a cutscene; minus the HUD bar on the HUD-type screens; minus the side
// panel and the HUD bar for the automap, spells, skills and item trade; the whole screen otherwise.
void interface_set_view_clip()
{
    int x, y, w, h;

    gr::reset_clip();
    if (game_flags & GF_CUTSCENE) {
        w = gr_screen_width();
        x = 0;
        h = w / 2;
        y = (gr_screen_height() - h) / 2;
    } else if (ui_input_locked) {
        x = 0;
        y = 0;
        w = gr_screen_width();
        h = gr_screen_height();
    } else {
        switch (interface_cur_screen) {
        case 0:
        case 4:
        case 9:
        case 13:
        case 20:
            x = 0;
            y = 0;
            w = gr_screen_width();
            h = gr_screen_height() - hud_get_panel_height();
            break;
        case 3:
            x = automap_get_panel_width();
            y = 0;
            w = gr_screen_width() - x;
            h = gr_screen_height() - hud_get_panel_height();
            break;
        case 5:
            x = spells_get_panel_width();
            y = 0;
            w = gr_screen_width() - x;
            h = gr_screen_height() - hud_get_panel_height();
            break;
        case 6:
            x = skills_get_panel_width();
            y = 0;
            w = gr_screen_width() - x;
            h = gr_screen_height() - hud_get_panel_height();
            break;
        case 19:
            x = itemtrade_get_panel_width();
            y = 0;
            w = gr_screen_width() - x;
            h = gr_screen_height() - hud_get_panel_height();
            break;
        case 1:
        case 2:
        case 7:
        case 8:
        case 10:
        case 11:
        case 12:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 21:
        case 22:
            x = 0;
            y = 0;
            w = gr_screen_width();
            h = gr_screen_height();
            break;
        default:
            x = 0;
            y = 0;
            w = gr_screen_width();
            h = gr_screen_height();
            break;
        }
    }
    gr::set_clip(x, y, w, h);
    if (!(game_flags & GF_CUTSCENE))
        camera_fov = gr_fov_to_vertical(interface_view_fov);
}

// 0x4123c0  Confidence: exact.
void ui_load_model_lights()
{
    if (ui_model_lights_vfx)
        vfx_instance_free(ui_model_lights_vfx);
    ui_model_lights_vfx = vfx_load("inv-lights1.vfx", 1);
}

// 0x4123f0  Confidence: exact. level_load (the level memory that held the instance is gone).
void ui_model_lights_reset()
{
    ui_model_lights_vfx = NULL;
}

// 0x412400  Confidence: likely (the loop as if / do-while with the track step before the wrap test, D11910; the light position is built through the inline
// vector3 operator+ and copy constructor, which leaves the temporary and the copy on the stack). The
// UI model view: ambient 0.5, up to 10 lights per object, the world lights off and one point light per
// light of "inv-lights1.vfx", placed relative to pos.
void ui_3d_begin(vector3 *pos)
{
    vfx_base *base;
    vfx_light_desc *light;
    int track;

    ui_3d_saved_max_lights = lights_max_per_object;
    ui_3d_saved_ambient_r = light_ambient_r;
    ui_3d_saved_ambient_g = light_ambient_g;
    ui_3d_saved_ambient_b = light_ambient_b;
    light_ambient_r = light_ambient_g = light_ambient_b = 0.5f;
    lights_set_max_per_object(UI_MAX_MODEL_LIGHTS);
    if (!ui_model_lights_vfx) {
        ui_load_model_lights();
        if (!ui_model_lights_vfx)
            return;
    }
    light_set_all_enabled(false);
    base = effect_get_base(ui_model_lights_vfx->id_bits >> 9);
    light = ui_model_lights_vfx->lights;
    if (light) {
        track = 0;
        do {
            vector3 p;
            p = light->pos + *pos;
            light->sample(base->light_tracks + track, reported_frametime);
            ui_3d_lights[ui_3d_light_count++] = light_create_point(&p, 0.0f, light->range * (1.0f / 256.0f), 0.5f,
                                                                   light->r * (1.0f / 255.0f),
                                                                   light->g * (1.0f / 255.0f),
                                                                   light->b * (1.0f / 255.0f), 0);
            track += 0x70;
            light = light->next;
            if (light == ui_model_lights_vfx->lights)
                break;
        } while (light);
    }
}

// 0x4125a0  Confidence: exact.
void ui_3d_end()
{
    int i;

    for (i = 0; i < ui_3d_light_count; i++)
        light_delete(ui_3d_lights[i]);
    ui_3d_light_count = 0;
    light_set_all_enabled(true);
    lights_set_max_per_object(ui_3d_saved_max_lights);
    light_ambient_r = ui_3d_saved_ambient_r;
    light_ambient_g = ui_3d_saved_ambient_g;
    light_ambient_b = ui_3d_saved_ambient_b;
}

// 0x412610  Confidence: exact.
int interface_get_screen()
{
    return interface_cur_screen;
}

// The empty leave functions of these screens, each in its own unit, folded by the linker into empty_function
// (0x4f13e0). Named apart: calls to one function in several cases are merged by VC6 (D7903). Not yet defined
// in their units.
void hud_leave();
void automap_leave();
void spells_leave();
void mainmenu_leave();
void gamelist_leave();
void charselect_leave();
void gamecreate_leave();
void levelselect_leave();

// 0x412620  Confidence: exact (the two `interface_set_screen(0, 0); return;` are compiled as jumps back
// to the top with id = arg = 0, VC6's self tail call; each case of the second switch ends in its own copy
// of the interface_start_input_delay tail). Leaves the current screen, picks the character of the
// inventory / status / buy-sell / journal screens when the current screen is not full-screen, then
// enters the new one. The leave functions of screens 0, 3, 5 and 14-18 are empty (folded into empty_function).
void interface_set_screen(int id, void *arg)
{
    int prev = interface_cur_screen;
    Character *c;

    switch (interface_cur_screen) {
    case 0:  hud_leave(); break;   // empty, folded
    case 1:  inventory_leave(); break;
    case 2:  status_leave(); break;
    case 3:  automap_leave(); break;   // empty, folded
    case 4:  pickup_leave(); break;
    case 5:  spells_leave(); break;   // empty, folded
    case 6:  skills_leave(); break;
    case 7:  buysell_leave(); break;
    case 8:  saveload_leave(); break;
    case 9:  playmenu_leave(); break;
    case 10: journal_leave(); break;
    case 11: pxochat_leave_thunk(); break;
    case 12: options_leave(); break;
    case 13: dialogue_leave(); break;
    case 14: mainmenu_leave(); break;   // empty, folded
    case 15: gamelist_leave(); break;   // empty, folded
    case 16: charselect_leave(); break;   // empty, folded
    case 17: gamecreate_leave(); break;   // empty, folded
    case 18: levelselect_leave(); break;   // empty, folded
    case 19: itemtrade_leave(); break;
    case 20: assess_leave(); break;
    case 21: credits_leave(); break;
    case 22: help_leave(); break;
    }
    if (interface_screen_is_fullscreen()) {
        if (id == 0)
            mouse_flush();
    } else {
        switch (id) {
        case 1:
        case 2:
        case 7:
        case 10:
            if (status_character)
                break;
            if (cur_character && cur_character->is_main_hero()) {
                statscreen_refresh_anim(cur_character);
            } else if (game_flags & GF_MULTIPLAYER) {
                c = living_from_handle(mp_local_player->object->handle);
                if (!c || (c->flags & (CF_HIDDEN | CF_SUMMON))) {
                    interface_set_screen(0, 0);
                    return;
                }
                statscreen_refresh_anim(c);
            } else {
                for (c = living_list.live_next; c != &living_list; c = c->live_next) {
                    if (c->type == 7 && (cur_level_index == 4 || !(c->flags & CF_HIDDEN)) && !(c->flags & CF_SUMMON)) {
                        statscreen_refresh_anim(c);
                        break;
                    }
                }
                if (c == &living_list) {
                    interface_set_screen(0, 0);
                    return;
                }
            }
            break;
        }
    }
    interface_cur_screen = id;
    switch (id) {
    case 0:
        hud_init();
        break;
    case 1:
        if (arg)
            inventory_init(*(bool *)arg);
        break;
    case 2:
        status_init();
        break;
    case 3:
        overhead_label_slots_clear();
        automap_init();
        break;
    case 4:
        if (arg)
            pickup_init(*(container **)arg);
        break;
    case 5:
        overhead_label_slots_clear();
        spells_init();
        break;
    case 6:
        overhead_label_slots_clear();
        skills_init();
        break;
    case 7:
        buysell_init();
        break;
    case 8:
        if (arg)
            saveload_init(*(bool *)arg, prev != SCREEN_MAIN_MENU);
        break;
    case 9:
        playmenu_init();
        break;
    case 10:
        journal_init();
        break;
    case 11:
        pxochat_init();
        break;
    case 12:
        options_init(prev != SCREEN_MAIN_MENU);
        break;
    case 13:
        if (arg)
            dialogue_init(*(const char **)arg);
        break;
    case 14:
        mainmenu_init();
        break;
    case 15:
        gamelist_init();
        break;
    case 16:
        mp_charselect_init();
        break;
    case 17:
        gamecreate_init();
        break;
    case 18:
        mp_levelselect_init();
        break;
    case 19:
        itemtrade_init();
        break;
    case 20:
        if (arg)
            assess_init(*(Character **)arg);
        break;
    case 21:
        credits_init();
        break;
    case 22:
        help_init(prev != SCREEN_MAIN_MENU);
        break;
    }
    interface_start_input_delay();
}

// 0x412ab0  Confidence: exact (which of the true cases are explicit labels cannot be told from the byte
// table 0x412ad8: screens 0, 3-6, 9, 13, 19 and 20 show the 3D view, the rest cover it).
bool interface_screen_is_fullscreen()
{
    switch (interface_cur_screen) {
    case 0:
    case 3:
    case 4:
    case 5:
    case 6:
    case 9:
    case 13:
    case 19:
    case 20:
        return false;
    default:
        return true;
    }
}

// The screens without a panel of their own have a mouse_in_panel each that returns true; the linker folds
// them into one copy, 0x4063a0 (identical-code folding; ui_return_true in new_buysell.cpp, the buy / sell
// screen's). The calls name them apart: with one callee VC6 merges the call sites into one (D7903).
// Their units do not define them yet.
bool inventory_mouse_in_panel(int x, int y);
bool status_mouse_in_panel(int x, int y);
bool saveload_mouse_in_panel(int x, int y);
bool journal_mouse_in_panel(int x, int y);
bool pxochat_mouse_in_panel(int x, int y);
bool options_mouse_in_panel(int x, int y);
bool mainmenu_mouse_in_panel(int x, int y);
bool gamelist_mouse_in_panel(int x, int y);
bool charselect_mouse_in_panel(int x, int y);
bool gamecreate_mouse_in_panel(int x, int y);
bool levelselect_mouse_in_panel(int x, int y);
bool credits_mouse_in_panel(int x, int y);
bool help_mouse_in_panel(int x, int y);

// 0x412af0  Confidence: exact. The screens without a panel test of their own call a mouse_in_panel that
// returns true (identical-code folded into ui_return_true, new_buysell.cpp); the result is normalised with
// `? true : false` (`mov al, 1` on the true path).
bool interface_mouse_over_panel()
{
    bool over = false;

    switch (interface_cur_screen) {
    case 1:  over = inventory_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    case 2:  over = status_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    case 3:  over = automap_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    case 4:  over = pickup_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    case 5:  over = spells_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    case 6:  over = skills_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    case 7:  over = ui_return_true(game_mouse_x, game_mouse_y); break;
    case 8:  over = saveload_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    case 9:  over = playmenu_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    case 10: over = journal_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    case 11: over = pxochat_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    case 12: over = options_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    case 13: over = dialogue_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    case 14: over = mainmenu_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    case 15: over = gamelist_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    case 16: over = charselect_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    case 17: over = gamecreate_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    case 18: over = levelselect_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    case 19: over = itemtrade_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    case 20: over = assess_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    case 21: over = credits_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    case 22: over = help_mouse_in_panel(game_mouse_x, game_mouse_y); break;
    }
    if (tutorial_is_active())
        over |= tutorial_mouse_in_panel(game_mouse_x, game_mouse_y);
    if (!ui_input_locked && !interface_screen_is_fullscreen())
        return (over || hud_mouse_in_panel(game_mouse_x, game_mouse_y)) ? true : false;
    return over;
}

// 0x412db0  Confidence: exact (tail jump).
void interface_refresh_cur_screen()
{
    void (*refresh)() = screen_refresh_table[interface_cur_screen];

    if (refresh)
        refresh();
}

// 0x412dd0  Confidence: exact (the loop is strength-reduced to a pointer and a down counter). The model's
// textures (the multiplayer appearance's when the record is a multiplayer copy).
void statscreen_set_reduced_textures(int reduced)
{
    int *textures;
    int n, i;

    if (status_character->info->flags & 0x10000)
        textures = (int *)charinfo_get_multi_appearance(status_character->info);
    else
        textures = status_character->instance->character->mesh->vim->textures;
    n = status_character->instance->character->mesh->vim->num_textures;
    for (i = 0; i < n; i++)
        bm_set_reduced(textures[i], reduced != 0);
}

// 0x412e50  Confidence: exact. Puts the model of c on the inventory / status screen: the old character
// gets its own instance back, the new one a copy playing the screen's locomotion animation.
void statscreen_refresh_anim(Character *c)
{
    int anim;

    switch (interface_cur_screen) {
    case 1:
        anim = LOCO_INVENTORY;
        break;
    case 2:
        anim = LOCO_STATSCREEN;
        break;
    default:
        anim = -1;
        break;
    }
    if (status_character && status_character->anim_statscreen != -1)
        statscreen_unload_anim(status_character);
    statscreen_restore_instance();
    if (status_character && !Summoner_options.texture_quality)
        statscreen_set_reduced_textures(1);
    status_character = c;
    if (status_character && !Summoner_options.texture_quality)
        statscreen_set_reduced_textures(0);
    if (status_character) {
        if (anim == LOCO_STATSCREEN)
            statscreen_load_anim(status_character,
                                 status_character->equipment[7] && status_character->equipment[7]->type->slot == 8);
        statscreen_clone_instance(anim);
    }
    interface_refresh_cur_screen();
}

// 0x412f20  Confidence: exact (VC6 byte match with the three tint members, D8604) (the copy is character_instance's implicit operator=, expanded member by
// member; its vector3 members come out three ways: plain dwords, dwords plus the inline operator='s
// return copy through vector3__vector3_copy, and the out-of-line COMDAT below).
void statscreen_clone_instance(int loco_anim)
{
    if (!status_character)
        return;
    ui_character_instance = *status_character->instance;
    if (loco_anim >= 0) {
        ui_character_instance.clear_action_anims();
        ui_character_instance.clear_state_anims();
        ui_character_instance.set_state_weight(status_character->loco_anims[loco_anim], 1.0f);
        ui_character_instance.process(1.0f, false, 0);
    }
    statscreen_saved_instance = status_character->instance;
    status_character->instance = &ui_character_instance;
}

// 0x413190  vector3::operator= (vector3__assign): the COMDAT copy of the inline member (vsdk/math/vector.h),
// emitted here for statscreen_clone_instance. Confidence: exact (compiler-generated).

// 0x4131c0  Confidence: exact.
void statscreen_restore_instance()
{
    if (!status_character)
        return;
    status_character->instance = statscreen_saved_instance;
    status_character->update_locomotion_anim();
    status_character->instance->process(0.0f, false, 0);
}

// 0x413200  Confidence: exact (VC6 8966 byte match, m-1020: the null test and the handle read through the
// global, D11900).
Character *party_find_next_member(bool skip_summon)
{
    Character *c = status_character;
    Player *start, *p;
    unsigned int skip;

    if (!status_character)
        return NULL;
    p = player_from_handle(status_character->handle);
    if (!p)
        return c;
    start = p;
    skip = CF_SUMMONLIKE | CF_DEAD;
    if (cur_level_index != 4)
        skip |= CF_HIDDEN;
    if (skip_summon)
        skip |= CF_SUMMON;
    do {
        p = p == party_list.prev ? party_list.next : p->next;
        c = living_from_handle(p->handle);
    } while (p != start && (!c || (c->flags & skip)));
    return c;
}

// 0x413290  Confidence: exact (VC6 8966 byte match, m-1020: written as party_find_next_member, the skip
// mask built by two ifs, D11900).
Character *party_find_prev_member(bool skip_summon)
{
    Character *c = status_character;
    Player *start, *p;
    unsigned int skip;

    if (!status_character)
        return NULL;
    p = player_from_handle(status_character->handle);
    if (!p)
        return c;
    start = p;
    skip = CF_SUMMONLIKE | CF_DEAD;
    if (cur_level_index != 4)
        skip |= CF_HIDDEN;
    if (skip_summon)
        skip |= CF_SUMMON;
    do {
        p = p == party_list.next ? party_list.prev : p->prev;
        c = living_from_handle(p->handle);
    } while (p != start && (!c || (c->flags & skip)));
    return c;
}

// 0x413320  Confidence: exact.
void statscreen_show_next_member(bool skip_summon)
{
    statscreen_refresh_anim(party_find_next_member(skip_summon));
}

// 0x413340  Confidence: exact.
void statscreen_show_prev_member(bool skip_summon)
{
    statscreen_refresh_anim(party_find_prev_member(skip_summon));
}

// 0x413360  Confidence: exact (VC6 byte match) (the multiplayer loop's condition order). Selects the next (or previous)
// party member that is not summon-like, hidden or dead; in multiplayer the next one this player owns
// (always forward). True when the leader changed.
int party_cycle(int forward)
{
    Player *start = party_leader;
    Player *p = party_leader;
    Character *c;

    if (game_flags & GF_MULTIPLAYER) {
        do {
            p = p->next;
            if (p == &party_list)
                p = party_list.next;
            c = living_from_handle(p->handle);
        } while (!c || (p != start && ((c->flags & (CF_SUMMONLIKE | CF_HIDDEN | CF_DEAD)) || p->mp_owner != mp_local_player)));
        if (p != party_leader) {
            party_select_member(p);
            return 1;
        }
        return 0;
    }
    if (forward) {
        do {
            p = p->next;
            if (p == &party_list)
                p = party_list.next;
            c = living_from_handle(p->handle);
        } while (p != party_leader && (!c || (c->flags & (CF_SUMMONLIKE | CF_HIDDEN | CF_DEAD))));
    } else {
        do {
            if (p == party_list.next)
                p = &party_list;
            p = p->prev;
            c = living_from_handle(p->handle);
        } while (p != party_leader && (!c || (c->flags & (CF_SUMMONLIKE | CF_HIDDEN | CF_DEAD))));
    }
    party_select_member(p);
    return start != party_leader;
}

// 0x413460  Confidence: exact.
bool point_in_rect(int px, int py, int x, int y, int w, int h)
{
    if (px >= x && px < x + w && py >= y && py < y + h)
        return true;
    return false;
}

// 0x4134a0  Confidence: exact.
float ui_get_scale()
{
    return ui_scale;
}

// 0x4134b0  Confidence: exact (VC6 8966 byte match, m-1020: the border step's offset in a block local,
// D11900). A shadow rectangle offset by the inset, a border of ui_frame_border + 1
// rectangles cut at the corners, and the inner panel.
void ui_draw_panel_frame(int x, int y, int w, int h)
{
    int i;

    gr_set_color_ptr(&ui_color_panel_shadow);
    gr_rect(x + ui_frame_inset_x[resolution_index], y + ui_frame_inset_y[resolution_index], w, h, gr_rect_mode, 1.0f);
    gr_set_color_ptr(&ui_color_panel_frame);
    for (i = 0; i <= ui_frame_border[resolution_index]; i++) {
        int d = ui_frame_border[resolution_index] - i;
        gr_rect(x + i, y + d, w - 2 * i, h - d * 2, gr_rect_mode, 1.0f);
    }
    gr_set_color_ptr(&ui_color_panel_inner);
    gr_rect(x + ui_frame_border[resolution_index], y + ui_frame_border[resolution_index],
            w - 2 * ui_frame_border[resolution_index], h - 2 * ui_frame_border[resolution_index], gr_rect_mode, 1.0f);
}

// 0x4135a0  Confidence: exact (VC6 byte match, w6 pass: the corners are four vector2 points declared
// top-left, top-right, bottom-left, bottom-right, D10501). A filled box with a one-pixel border.
void ui_draw_text_box(int x, int y, int w, int h)
{
    vector2 a((float)x, (float)y);
    vector2 d((float)(x + w), (float)y);
    vector2 b((float)x, (float)(y + h));
    vector2 c((float)(x + w), (float)(y + h));

    gr_set_color_ptr(&ui_color_text_box_bg);
    gr_rect(x, y, w, h, gr_rect_mode, 1.0f);
    gr_set_color_ptr(&ui_color_text);
    gr_line(a.x, a.y, b.x, b.y, gr_line_mode);
    gr_line(b.x, b.y, c.x, c.y, gr_line_mode);
    gr_line(c.x, c.y, d.x, d.y, gr_line_mode);
    gr_line(d.x, d.y, a.x, a.y, gr_line_mode);
}

// 0x4136a0  Confidence: likely (the clamps written with a temporary: one gr_screen_width call each).
// Signature as declared (D103): a = flags, b = the int array of values (stored only with flag 4), c = the
// bool array of enabled entries (NULL: all).
void ui_choice_popup_show(const char *title, int x, int y, int num, char **items, int a, int b, int c)
{
    int i, w, h, lim;

    if (num <= 0 || !items)
        return;
    if (title) {
        strncpy(ui_choice_popup_title, title, 0x200);
        ui_choice_popup_title[0x200] = 0;
    } else
        ui_choice_popup_title[0] = 0;
    if (num >= UI_CHOICE_MAX)
        num = UI_CHOICE_MAX;
    ui_choice_popup_count = num;
    for (i = 0; i < ui_choice_popup_count; i++)
        ui_choice_popup_choices[i] = items[i];
    if (a & 4)
        ui_choice_popup_values = (int *)b;
    ui_choice_popup_enabled = (bool *)c;
    ui_choice_popup_w = 0;
    ui_choice_popup_h = 0;
    if (strlen(ui_choice_popup_title))
        gr_get_string_size(&ui_choice_popup_w, &ui_choice_popup_h, ui_choice_popup_title, -1, -1);
    for (i = 0; i < ui_choice_popup_count; i++) {
        gr_get_string_size(&w, &h, ui_choice_popup_choices[i], -1, -1);
        ui_choice_popup_w = max(ui_choice_popup_w, w);
        ui_choice_popup_h += h;
    }
    if (!ui_choice_popup_w)
        return;
    ui_choice_popup_w += 2 * ui_choice_popup_margin[resolution_index];
    ui_choice_popup_h += 2 * ui_choice_popup_margin_y[resolution_index];
    if (title)
        ui_choice_popup_h += ui_choice_popup_title_gap[resolution_index];
    if (x >= 0) {
        lim = gr_screen_width() - ui_choice_popup_w;
        ui_choice_popup_x = x < lim ? x : lim;
    } else
        ui_choice_popup_x = (gr_screen_width() - ui_choice_popup_w) / 2;
    if (y >= 0) {
        lim = gr_screen_height() - ui_choice_popup_h;
        ui_choice_popup_y = y < lim ? y : lim;
    } else
        ui_choice_popup_y = (gr_screen_height() - ui_choice_popup_h) / 2;
    ui_choice_popup_mouse_mode = 0;
    ui_choice_popup_flags = a | 0x80000000;
}

// 0x4138a0  Confidence: exact.
void ClearFrameFlag5bd77c()
{
    ui_choice_popup_flags = 0;
}

// 0x4138b0  Confidence: exact (tail jumps).
int ui_choice_popup_frame(int key)
{
    if (!(ui_choice_popup_flags & 0x80000000)) {
        ClearFrameFlag5bd77c();
        return -2;
    }
    switch (ui_choice_popup_mouse_mode) {
    case 0:
        return ui_choice_popup_handle_click(key);
    case 1:
        return ui_choice_popup_update_buttons();
    case 2:
        return ui_choice_popup_wait_release();
    default:
        ClearFrameFlag5bd77c();
        return -2;
    }
}

// 0x413900  Confidence: exact. Esc cancels (sound 0x24) unless flag 8.
int ui_choice_popup_handle_click(int key)
{
    ui_choice_popup_hover = ui_choice_popup_hit_test();
    ui_choice_popup_flags |= 0x40000000;
    if (mouse_was_pressed(0)) {
        ui_choice_popup_mouse_mode = 1;
        return -1;
    }
    if (!(ui_choice_popup_flags & 8) && key == KEY_ESC) {
        ClearFrameFlag5bd77c();
        game_play_sound(0x24, 1.0f, 0.0f, 0, 0);
        return -2;
    }
    return -1;
}

// 0x413970  Confidence: likely (the title branch: the common `+ y` is tail-merged). The entry rows start
// below the title without the title gap that ui_choice_popup_draw adds (NOTES: game bug).
int ui_choice_popup_hit_test()
{
    int w, h;
    int y, line_h, i, x, bw;

    if (strlen(ui_choice_popup_title)) {
        gr_get_string_size(&w, &h, ui_choice_popup_title, -1, -1);
        y = ui_choice_popup_margin_y[resolution_index] + ui_choice_popup_y + h;
    } else
        y = ui_choice_popup_margin_y[resolution_index] + ui_choice_popup_y;
    line_h = gr_get_font_height(-1);
    for (i = 0; i < ui_choice_popup_count; i++, y += line_h) {
        if (ui_choice_popup_enabled && !ui_choice_popup_enabled[i])
            continue;
        bw = ui_choice_popup_w - 2 * ui_choice_popup_margin[resolution_index];
        if (ui_choice_popup_flags & 2)
            x = (ui_choice_popup_w - bw) / 2 + ui_choice_popup_x;
        else
            x = ui_choice_popup_x + ui_choice_popup_margin[resolution_index];
        if (point_in_rect(game_mouse_x, game_mouse_y, x, y, bw, line_h))
            return i;
    }
    return -1;
}

// 0x413a90  Confidence: exact (VC6 byte match) (the drag rectangle's statement order). Mouse held: the pressed entry
// stays highlighted while the mouse is on it; leaving it with flag 4 drags the entry's icon to the hot
// buttons (value bit 1 skill / spell, bits 2- the id). On release: the entry (sound 0x23), a release
// inside the popup re-arms it, outside cancels (sound 0x24) unless flag 8.
int ui_choice_popup_update_buttons()
{
    int w, h;
    int i = ui_choice_popup_hit_test();

    if (ui_choice_popup_hover >= 0 && i == ui_choice_popup_hover)
        ui_choice_popup_flags |= 0x40000000;
    else {
        ui_choice_popup_flags &= ~0x40000000;
        if (ui_choice_popup_hover >= 0 && (ui_choice_popup_flags & 4)) {
            int line_h = gr_get_font_height(-1);
            int x = ui_choice_popup_margin[resolution_index] + ui_choice_popup_x;
            int y = ui_choice_popup_margin_y[resolution_index] + ui_choice_popup_hover * line_h + ui_choice_popup_y;
            unsigned int value;

            if (ui_choice_popup_flags & 2)
                gr_get_string_size(&w, &h, ui_choice_popup_choices[ui_choice_popup_hover], -1, -1);
            else {
                w = ui_choice_popup_w - 2 * ui_choice_popup_margin[resolution_index];
                h = line_h;
            }
            value = ui_choice_popup_values[ui_choice_popup_hover];
            hud_drag_icon_begin(x, y, w, h, (value >> 1) & 1, value >> 2);
            ui_choice_popup_mouse_mode = 2;
            return -1;
        }
    }
    if (mouse_is_down(0))
        return -1;
    if (ui_choice_popup_flags & 0x40000000) {
        ClearFrameFlag5bd77c();
        game_play_sound(0x23, 1.0f, 0.0f, 0, 0);
        return ui_choice_popup_hover;
    }
    if (point_in_rect(game_mouse_x, game_mouse_y, ui_choice_popup_x, ui_choice_popup_y, ui_choice_popup_w,
                      ui_choice_popup_h)) {
        ui_choice_popup_mouse_mode = 0;
        return -1;
    }
    if (ui_choice_popup_flags & 8)
        return -1;
    ClearFrameFlag5bd77c();
    game_play_sound(0x24, 1.0f, 0.0f, 0, 0);
    return -2;
}

// 0x413c50  Confidence: exact. After a drag: waits for the button to come up.
int ui_choice_popup_wait_release()
{
    if (!mouse_is_down(0))
        ui_choice_popup_mouse_mode = 0;
    return -1;
}

// 0x413c70  Confidence: likely (marked exact; differs under VC6 8966, match27).
void ui_choice_popup_draw()
{
    int w, h;
    int x, y, i;

    if (!(ui_choice_popup_flags & 0x80000000))
        return;
    ui_draw_panel_frame(ui_choice_popup_x, ui_choice_popup_y, ui_choice_popup_w, ui_choice_popup_h);
    y = ui_choice_popup_margin_y[resolution_index] + ui_choice_popup_y;
    gr_set_color_ptr(&ui_color_text);
    if (strlen(ui_choice_popup_title)) {
        gr_get_string_size(&w, &h, ui_choice_popup_title, -1, -1);
        x = (ui_choice_popup_flags & 1) ? (ui_choice_popup_w - w) / 2 : ui_choice_popup_margin[resolution_index];
        gr_string(x + ui_choice_popup_x, y, ui_choice_popup_title, -1, gr_string_mode);
        y += ui_choice_popup_title_gap[resolution_index] + h;
    }
    for (i = 0; i < ui_choice_popup_count; i++) {
        gr_get_string_size(&w, &h, ui_choice_popup_choices[i], -1, -1);
        x = ((ui_choice_popup_flags & 2) ? (ui_choice_popup_w - w) / 2 : ui_choice_popup_margin[resolution_index])
            + ui_choice_popup_x;
        if (i == ui_choice_popup_hover && (ui_choice_popup_flags & 0x40000000))
            gr_set_color_ptr(&ui_color_highlight);
        else if (ui_choice_popup_enabled && !ui_choice_popup_enabled[i])
            gr_set_color_ptr(&ui_color_disabled);
        else
            gr_set_color_ptr(&ui_color_text);
        gr_string(x, y, ui_choice_popup_choices[i], -1, gr_string_mode);
        y += h;
    }
}

// 0x413e20  Confidence: exact (neg / sbb / neg: the masked flag goes through an int).
bool ui_choice_popup_is_active()
{
    int active = ui_choice_popup_flags & 0x80000000;
    return active != 0;
}

// 0x413e40  Confidence: likely (the clamps as in ui_choice_popup_show). Flag 1 pauses a single-player
// game. The side panels (automap, spells, skills, item trade) are closed; sound 0x27 once the level runs
// (game_flags 0x200, GF_LEADER_SOUND).
void ui_message_box_show(const char *title, const char *text, int x, int y, int num_buttons, char **buttons,
                         void (*callback)(int), int flags)
{
    int i, lim;

    if (title) {
        strncpy(ui_message_box_title, title, 0x20);
        ui_message_box_title[0x20] = 0;
    } else
        ui_message_box_title[0] = 0;
    if (text) {
        strncpy(ui_message_box_text, text, 0xff);
        ui_message_box_text[0xff] = 0;
    } else
        ui_message_box_text[0] = 0;
    if (num_buttons >= UI_MESSAGE_BOX_BUTTONS)
        num_buttons = UI_MESSAGE_BOX_BUTTONS;
    ui_message_box_button_count = num_buttons;
    for (i = 0; i < ui_message_box_button_count; i++) {
        strncpy(ui_message_box_button_labels[i], buttons[i], 0x1f);
        ui_message_box_button_labels[i][0x1f] = 0;
    }
    if (x >= 0) {
        lim = gr_screen_width() - ui_message_box_size[resolution_index][0];
        ui_message_box_x = x < lim ? x : lim;
    } else
        ui_message_box_x = (gr_screen_width() - ui_message_box_size[resolution_index][0]) / 2;
    if (y >= 0) {
        lim = gr_screen_height() - ui_message_box_size[resolution_index][1];
        ui_message_box_y = y < lim ? y : lim;
    } else
        ui_message_box_y = (gr_screen_height() - ui_message_box_size[resolution_index][1]) / 2;
    ui_message_box_flags = flags | 0x80000000;
    ui_message_box_callback = callback;
    ui_message_box_mouse_mode = 0;
    if ((ui_message_box_flags & 1) && (game_flags & GF_SINGLE_PLAYER))
        game_pause(0, 0);
    if ((game_flags & GF_MULTIPLAYER) && gameseq_get_state() == 4)
        ui_screen_stop_player();
    switch (interface_cur_screen) {
    case 3:
    case 5:
    case 6:
    case 19:
        interface_set_screen(0, 0);
        break;
    }
    if (game_flags & GF_LEADER_SOUND)
        game_play_sound(0x27, 1.0f, 0.0f, 0, 0);
}

// 0x414020  Confidence: exact.
void ClearFrameFlag5bd504()
{
    if ((ui_message_box_flags & 1) && (game_flags & GF_SINGLE_PLAYER))
        game_unpause();
    ui_message_box_flags = 0;
}

// 0x414050  Confidence: exact. The callback gets the button index (Esc's -2 and "nothing yet" are not
// passed on).
bool ui_message_box_frame(int key)
{
    int choice;

    if (!ui_message_box_is_active())
        return false;
    switch (ui_message_box_mouse_mode) {
    case 0:
        choice = ui_message_box_handle_click(key);
        break;
    case 1:
        choice = ui_message_box_update_buttons();
        break;
    default:
        choice = -1;
        break;
    }
    if (ui_message_box_callback && choice >= 0)
        ui_message_box_callback(choice);
    return true;
}

// 0x4140a0  Confidence: exact. Esc closes (sound 0x23, as a button) unless flag 2.
int ui_message_box_handle_click(int key)
{
    ui_message_box_hover_button = ui_message_box_hit_test();
    ui_message_box_flags |= 0x40000000;
    if (mouse_was_pressed(0)) {
        ui_message_box_mouse_mode = 1;
        return -1;
    }
    if (!(ui_message_box_flags & 2) && key == KEY_ESC) {
        ClearFrameFlag5bd504();
        game_play_sound(0x23, 1.0f, 0.0f, 0, 0);
        return -2;
    }
    return -1;
}

// 0x414110  Confidence: exact (VC6 byte match, w6 pass; was likely) (marked exact; differs under VC6 8966, match27). The buttons are spread evenly over the box width, on the last text line.
int ui_message_box_hit_test()
{
    int w, h;
    int y = ui_message_box_text_pos[resolution_index][1] + ui_message_box_text_size[resolution_index][1]
            + ui_message_box_y - gr_get_font_height(-1);
    int i, x;

    for (i = 0; i < ui_message_box_button_count; i++) {
        gr_get_string_size(&w, &h, ui_message_box_button_labels[i], -1, -1);
        x = (int)((float)(i + 1) / (ui_message_box_button_count + 1) * ui_message_box_size[resolution_index][0]
                  + 0.5f) + ui_message_box_x;
        if (point_in_rect(game_mouse_x, game_mouse_y, x - w / 2, y, w, h))
            return i;
    }
    return -1;
}

// 0x414200  Confidence: exact.
int ui_message_box_update_buttons()
{
    int i;

    ui_message_box_flags &= ~0x40000000;
    i = ui_message_box_hit_test();
    if (ui_message_box_hover_button >= 0 && i == ui_message_box_hover_button)
        ui_message_box_flags |= 0x40000000;
    if (!mouse_is_down(0)) {
        if (ui_message_box_flags & 0x40000000) {
            ClearFrameFlag5bd504();
            game_play_sound(0x23, 1.0f, 0.0f, 0, 0);
            return ui_message_box_hover_button;
        }
        ui_message_box_mouse_mode = 0;
    }
    return -1;
}

// 0x414270  Confidence: likely (the line buffer's size: 64 bytes between the size locals and the split
// arrays). The background bitmap, the title, the text wrapped to the text width (16 lines at most, each
// centred) and the buttons.
void ui_message_box_draw()
{
    int w, h;
    int line_h;
    char line[64];
    int len[16];
    int offset[16];
    int n, i, cx, x, y;

    if (!ui_message_box_is_active())
        return;
    gr_set_color(255, 255, 255, 255);
    ui_bitmap_scaled(message_popup_bkgrnd_bm, ui_message_box_x, ui_message_box_y, gr_bitmap_wrap_mode);
    gr_set_color_ptr(&ui_color_text);
    gr_string(ui_message_box_title_pos[resolution_index][0] + ui_message_box_x,
              ui_message_box_title_pos[resolution_index][1] + ui_message_box_y, ui_message_box_title, -1,
              gr_string_mode);
    cx = ui_message_box_size[resolution_index][0] / 2 + ui_message_box_x;
    y = ui_message_box_text_pos[resolution_index][1] + ui_message_box_y;
    line_h = gr_get_font_height(-1);
    n = gr_split_str(len, offset, ui_message_box_text, ui_message_box_text_size[resolution_index][0], 16, 0, -1);
    for (i = 0; i < n; i++) {
        strncpy(line, ui_message_box_text + offset[i], len[i]);
        line[len[i]] = 0;
        gr_get_string_size(&w, &h, line, -1, -1);
        gr_string(cx - w / 2, y, line, -1, gr_string_mode);
        y += line_h;
    }
    y = ui_message_box_text_size[resolution_index][1] + ui_message_box_text_pos[resolution_index][1] - line_h
        + ui_message_box_y;
    for (i = 0; i < ui_message_box_button_count; i++) {
        gr_get_string_size(&w, &h, ui_message_box_button_labels[i], -1, -1);
        x = (int)((float)(i + 1) / (ui_message_box_button_count + 1) * ui_message_box_size[resolution_index][0]
                  + 0.5f) + ui_message_box_x;
        if (i == ui_message_box_hover_button && (ui_message_box_flags & 0x40000000))
            gr_set_color_ptr(&ui_color_highlight);
        else
            gr_set_color_ptr(&ui_color_text);
        gr_string(x - w / 2, y, ui_message_box_button_labels[i], -1, gr_string_mode);
    }
}

// 0x4144d0  Confidence: exact (as ui_choice_popup_is_active).
bool ui_message_box_is_active()
{
    int active = ui_message_box_flags & 0x80000000;
    return active != 0;
}

// 0x4144f0  Confidence: exact. The bitmap at its size times ui_scale (h is converted first: the arguments
// are evaluated right to left).
void ui_bitmap_scaled(int bm, int x, int y, gr::Mode mode)
{
    int w, h;

    if (bm < 0)
        return;
    bm_get_dimensions(bm, &w, &h);
    gr_bitmap_stretched(bm, x, y, (int)(w * ui_scale), (int)(h * ui_scale), 0, 0, w, h, mode);
}

// 0x414560  Confidence: exact.
void ui_clear_pick_overrides()
{
    ui_pick_screen_override_on = false;
    ui_pick_world_override_on = false;
    ui_using_list_item = false;
    ui_using_equipped_item = false;
}

// 0x414580  Confidence: exact.
bool ui_get_pick_screen_override(float *x, float *y)
{
    if (ui_pick_screen_override_on) {
        *x = ui_pick_override_x;
        *y = ui_pick_override_y;
        return true;
    }
    return false;
}

// 0x4145b0  Confidence: exact (the inline vector3 assignment, three dwords; the first through the FPU).
bool ui_get_pick_world_override(vector3 *pos)
{
    if (ui_pick_world_override_on) {
        *pos = ui_pick_world_override_pos;
        return true;
    }
    return false;
}

// 0x4145e0  Confidence: exact. The leader's ground ring, turning at 90 degrees a second.
void game_draw_leader_marker()
{
    if (!cur_character)
        return;
    leader_marker_phase -= reported_frametime * 90.0f;
    if (leader_marker_phase < 0.0f)
        leader_marker_phase += 360.0f;
    gr_set_color(255, 255, 255, 255);
    if (cur_character->info->size != 1)
        place_ground_decal(cursorspot_focus_large_bm, &cur_character->pos, leader_marker_phase, 1.5f, 1.5f);
    else
        place_ground_decal(cursorspot_focus_bm, &cur_character->pos, leader_marker_phase, 1.5f, 1.5f);
}

// 0x4146a0  Confidence: exact.
void ui_input_lock()
{
    ui_input_locked = true;
}

// 0x4146b0  Confidence: exact.
void ui_input_unlock()
{
    ui_input_locked = false;
}

// 0x4146c0  Confidence: exact (whether the version numbers were macros). "v1.40", bottom right, dark grey.
void options_draw_version()
{
    int w, h;
    char buf[256];

    sprintf(buf, "v%d.%02d", 1, 40);
    gr_set_color(64, 64, 64, 255);
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_string(gr_screen_width() - 10 - w, gr_screen_height() - 5 - gr_get_font_height(-1), buf, -1, gr_string_mode);
}

// 0x414750  Confidence: exact (VC6 8966 byte match, m-1020: the clamps at 0 as `x &= (x <= 0) - 1` on the
// stored global, D10600; pad_top before pad_bottom in the y sums, D11900) (the length test is in the binary
// twice, as in hud_show_message; the clamps to the screen use a temporary). The tooltip is centred above the
// mouse and kept on the screen; (x, y, w, h) is the owner's rectangle (ui_tooltip_hide_on_mouse_activity).
void ui_tooltip_set(int x, int y, int w, int h, const char *text)
{
    int lim;

    if (strlen(text) > UI_TOOLTIP_TEXT_LEN - 1)
        return;
    if (strlen(text) > UI_TOOLTIP_TEXT_LEN - 1)
        return;
    ui_tooltip_rect_x = x;
    ui_tooltip_rect_y = y;
    ui_tooltip_rect_w = w;
    ui_tooltip_rect_h = h;
    gr_get_string_size(&ui_tooltip_text_w, &ui_tooltip_text_h, text, -1, -1);
    ui_tooltip_x = game_mouse_x - ui_tooltip_text_w / 2 - ui_tooltip_pad_left - 1;
    ui_tooltip_x &= (ui_tooltip_x <= 0) - 1;
    lim = gr_screen_width() - ui_tooltip_pad_right - ui_tooltip_pad_left - ui_tooltip_text_w - 3;
    if (ui_tooltip_x < lim)
        lim = ui_tooltip_x;
    ui_tooltip_x = lim;
    ui_tooltip_y = ui_tooltip_offset_y[resolution_index] - ui_tooltip_pad_top - ui_tooltip_pad_bottom - ui_tooltip_text_h
                   + game_mouse_y - 2;
    ui_tooltip_y &= (ui_tooltip_y <= 0) - 1;
    lim = gr_screen_height() - ui_tooltip_pad_top - ui_tooltip_pad_bottom - ui_tooltip_text_h - 3;
    if (ui_tooltip_y < lim)
        lim = ui_tooltip_y;
    ui_tooltip_y = lim;
    strcpy(ui_tooltip_text, text);
    ui_tooltip_visible = true;
}

// 0x4148c0  Confidence: exact.
void interface_start_input_delay()
{
    ui_tooltip_visible = false;
    interface_input_delay_timer.SetTimeout(600);
}

// 0x4148e0  Confidence: exact.
bool interface_input_ready()
{
    return !ui_tooltip_visible && interface_input_delay_timer.elapsed();
}

// 0x414900  Confidence: exact. One step of the cutscene loader's bar, then the frame is shown.
void cutscene_draw_loading()
{
    float f;

    cutscene_loading_progress++;
    gr_set_color(255, 255, 255, 255);
    ui_bitmap_scaled(cutscene_loading_bm, cutscene_loading_bg_pos[resolution_index][0],
                     cutscene_loading_bg_pos[resolution_index][1], gr_bitmap_wrap_mode);
    gr_set_color_ptr(&ui_color_loading_bar);
    f = (float)cutscene_loading_progress / cutscene_loading_total;
    if (f >= 1.0f)
        f = 1.0f;
    gr_rect(cutscene_loading_bar_pos[resolution_index][0], cutscene_loading_bar_pos[resolution_index][1],
            (int)(cutscene_loading_bar_size[resolution_index][0] * f + 0.5f),
            cutscene_loading_bar_size[resolution_index][1], gr_rect_mode, 1.0f);
    gr_flip();
}

// 0x4149d0  Confidence: exact (the last gr_clear is a tail jump).
void cutscene_begin_loading(int total)
{
    cutscene_loading_progress = 0;
    cutscene_loading_total = total;
    gr_set_color(0, 0, 0, 255);
    gr::reset_clip();
    gr_clear();
    gr_flip();
    gr_clear();
}

// 0x414a10  Confidence: exact (tail jump).
void cutscene_loading_finish()
{
    cutscene_loading_progress = cutscene_loading_total;
    cutscene_draw_loading();
}

// 0x414a20  Confidence: exact (VC6 byte match) (the length test twice: three inline strlen, the third is strcpy's).
void hud_show_message(const char *text)
{
    if (strlen(text) > 63)
        return;
    if (strlen(text) > 63)
        return;
    strcpy(hud_message_text, text);
    hud_message_timer.SetTimeout(4000);
}

// 0x414a80  Confidence: exact (VC6 byte match) (as hud_show_message).
void hud_show_chain_message(const char *text)
{
    if (strlen(text) > 15)
        return;
    if (strlen(text) > 15)
        return;
    strcpy(hud_chain_message_text, text);
    hud_chain_message_timer.SetTimeout(4000);
}

// 0x414ae0  Confidence: exact (VC6 byte match; the value stays on the x87 stack and is stored after each step,
// and the last component stores in an if / else: probably a clamp macro). Each component times (pulse + bias), 0..1.
void color_pulse_scale(float *r, float *g, float *b, float bias)
{
    float v;

    v = *r * (ui_pulse_scale_phase + bias);
    *r = v;
    if (v >= 1.0f)
        v = 1.0f;
    *r = v;
    if (v <= 0.0f)
        v = 0.0f;
    *r = v;
    v = *g * (ui_pulse_scale_phase + bias);
    *g = v;
    if (v >= 1.0f)
        v = 1.0f;
    *g = v;
    if (v <= 0.0f)
        v = 0.0f;
    *g = v;
    v = *b * (ui_pulse_scale_phase + bias);
    *b = v;
    if (v >= 1.0f)
        v = 1.0f;
    *b = v;
    if (v > 0.0f)
        *b = v;
    else
        *b = 0.0f;
}

// 0x414bb0  Confidence: exact (VC6 byte match) (as color_pulse_scale, with the faster pulse).
void ui_apply_pulse_rgb(float *r, float *g, float *b, float offset)
{
    float v;

    v = *r * (ui_pulse_rgb_phase + offset);
    *r = v;
    if (v >= 1.0f)
        v = 1.0f;
    *r = v;
    if (v <= 0.0f)
        v = 0.0f;
    *r = v;
    v = *g * (ui_pulse_rgb_phase + offset);
    *g = v;
    if (v >= 1.0f)
        v = 1.0f;
    *g = v;
    if (v <= 0.0f)
        v = 0.0f;
    *g = v;
    v = *b * (ui_pulse_rgb_phase + offset);
    *b = v;
    if (v >= 1.0f)
        v = 1.0f;
    *b = v;
    if (v > 0.0f)
        *b = v;
    else
        *b = 0.0f;
}

// The show_ui command object (moved from new_inventory.cpp, decomp48). Its static initialiser lies here,
// after the last function and before cmd_show_ui, so the definition is placed at this point of the file.
// 0x414c80  init_ccmd_show_ui_thunk (static_initializers entry 49, jmp). Confidence: exact (compiler-generated).
// 0x414c90  init_ccmd_show_ui: console_command::console_command("show_ui", ..., cmd_show_ui). Confidence: exact
// (compiler-generated).
console_command_M ccmd_show_ui("show_ui", "Toggles the visibility state of the HUD", cmd_show_ui);

// 0x414cb0  Confidence: exact. The HUD toggle is refused while a screen is open.
void cmd_show_ui()
{
    if (calling_a_command_function) {
        if (interface_get_screen()) {
            print_to_console("Must close all non-HUD windows first", 0);
            return;
        }
        ui_input_locked = !ui_input_locked;
    }
    if (checking_status_for_command) {
        if (ui_input_locked)
            print_to_console("HUD is currently hidden", 0);
        else
            print_to_console("HUD is currently visible", 0);
    }
    if (doing_help_for_comand)
        print_to_console("Toggles the visibility of the HUD.\n", 0);
}
