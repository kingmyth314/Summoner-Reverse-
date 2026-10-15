// new_inventory.cpp - Sum.exe v1.40, code 0x414c80-0x418572 (screen 1, the inventory: party item list with
// category jumps, the paper-doll equipment slots, the 3D views of the character and of the equipped items,
// the item action popup (info / identify / use / destroy / cancel), drag and drop between list and slots).
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .rdata 0x57f040-0x57fdbb (layout tables, the per-item view offset table), .data 0x58bfcc-0x58cbd7
// (show_ui strings, button table, scrollbar names, "inv_cameras.tbl"), .bss 0x5bd970-0x5c146b. The console
// command object ccmd_show_ui (0x5bd780) lies in the interface unit's .bss but is constructed by this unit's
// first initialiser, so it is defined here (see NOTES). status_character (0x5bd810) is extern.
//
// The list shows the party inventory (open mode 0) or only its unidentified items (open mode 1, the
// inventory_init byte). Paper-doll hot spots are the buttons 18-27; inventory_equip_index_for_button turns a
// hot-spot button into the equipment index. Mouse modes: 0 idle, 1 button held, 2 dragging the thumb, 3 action
// popup, 4 dragging a list item, 5 dragging an equipped item.

#include <stdio.h>
#include <string.h>
#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/key.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/frametime.h"
#include "vsdk/os/console.h"
#include "vsdk/localize/localize.h"
#include "vsdk/ca/character.h"
#include "vsdk/ca/character_instance.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "engine/gamesound/gamesound.h"
#include "engine/ai/ai.h"
#include "engine/objects/living_entity.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/item_paging.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/spells/spells.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"
#include "levelscripts/level_scripts.h"
#include "summoner/interface/new_buysell.h"
#include "summoner/interface/new_skill.h"
#include "summoner/interface/new_inventory.h"

#define INVENTORY_NUM_BUTTONS 45
#define INVENTORY_PAGE_SLOT   0x12    // item paging slot of the drag preview (ITEM_PAGE_SLOT_POPUP)
#define SKILL_IDENTIFY        0x14
#define NUM_VIEW_OFFSETS      35

// A reference to a localize_strings slot (see new_help.cpp: binding the reference is what makes the table a
// dynamic initialiser).
struct inventory_str_ref {
    char *&s;
};

// One entry of the per-item model offset table (0x58 bytes, *new* name).
struct inventory_view_offset {
    char name[0x40];          // +0x00 ItemType name
    int  offset[3][2];        // +0x40 per resolution_index (the 1024 pair is 0, 0 in every entry)
};

// ---- .rdata --------------------------------------------------------------------------------
// Per resolution_index (640, 800, 1024). All names here are *new* (sum.symbols has none).
const int inventory_hvyarmr_size[3][2]   = { { 108, 153 }, { 135, 191 }, { 173, 245 } };  // 0x57f040 hot spot 27 w, h
// 0x57f058: paper-doll model offset per equipment index (inventory_model_view_begin, preview_view_begin).
const int inventory_model_view_offset[3][9][2] = {
    { { -7, -11 },  { -1, 11 }, { 39, 11 }, { 0, 8 },  { 3, 5 }, { 2, -5 }, { 0, 3 }, { 1, -5 }, { 2, 0 } },
    { { -9, -14 },  { -1, 14 }, { 49, 14 }, { 0, 10 }, { 4, 6 }, { 3, -6 }, { 0, 4 }, { 1, -6 }, { 3, 0 } },
    { { -11, -18 }, { -2, 18 }, { 62, 18 }, { 0, 13 }, { 5, 8 }, { 3, -8 }, { 0, 5 }, { 2, -8 }, { 3, 0 } },
};
const int inventory_heavy_armor_offset[3][2] = { { 0, 0 }, { 0, 0 }, { -15, 0 } };         // 0x57f130 ItemType slot 8
const inventory_view_offset inventory_view_offsets[NUM_VIEW_OFFSETS] = {                    // 0x57f148
    { "Nodachi",             { { 1, -5 },  { 1, -6 },  { 0, 0 } } },
    { "Thunder Nagitana",    { { 1, -8 },  { 1, -10 }, { 0, 0 } } },
    { "Leather Gauntlets",   { { 0, -6 },  { 0, -8 },  { 0, 0 } } },
    { "Summoner Gauntlets",  { { -1, 2 },  { -1, 3 },  { 0, 0 } } },
    { "Warrior's Gauntlets", { { -5, 5 },  { -6, 6 },  { 0, 0 } } },
    { "Bone Jerkin",         { { -1, 13 }, { -1, 16 }, { 0, 0 } } },
    { "Green Dragon Scale",  { { 4, 4 },   { 5, 5 },   { 0, 0 } } },
    { "Red Dragon Scale",    { { 4, 4 },   { 5, 5 },   { 0, 0 } } },
    { "Gray Dragon Scale",   { { 4, 4 },   { 5, 5 },   { 0, 0 } } },
    { "Blue Dragon Scale",   { { 4, 4 },   { 5, 5 },   { 0, 0 } } },
    { "Summoner's Torque",   { { 3, 2 },   { 4, 3 },   { 0, 0 } } },
    { "Torque of Haste",     { { 4, 2 },   { 5, 3 },   { 0, 0 } } },
    { "Medallion of Vigor",  { { 5, 2 },   { 6, 3 },   { 0, 0 } } },
    { "Thermal Medallion",   { { 5, 2 },   { 6, 3 },   { 0, 0 } } },
    { "Berserker's Torque",  { { 5, 2 },   { 6, 3 },   { 0, 0 } } },
    { "Collar of Health",    { { 5, 2 },   { 6, 3 },   { 0, 0 } } },
    { "Shadow Medallion",    { { 5, 2 },   { 6, 3 },   { 0, 0 } } },
    { "Anti-Magus Torque",   { { 5, 2 },   { 6, 3 },   { 0, 0 } } },
    { "Elemental Necklace",  { { 7, 4 },   { 9, 5 },   { 0, 0 } } },
    { "Medallion of Night",  { { 6, 3 },   { 8, 4 },   { 0, 0 } } },
    { "Collar of Precision", { { 7, 5 },   { 9, 6 },   { 0, 0 } } },
    { "Torque of Bashing",   { { 7, 5 },   { 9, 6 },   { 0, 0 } } },
    { "Farseeing Necklace",  { { 7, 5 },   { 9, 6 },   { 0, 0 } } },
    { "Collar of Dexterity", { { 7, 5 },   { 9, 6 },   { 0, 0 } } },
    { "Torque of Celerity",  { { 7, 5 },   { 9, 6 },   { 0, 0 } } },
    { "Bacite Medallion",    { { 7, 5 },   { 9, 6 },   { 0, 0 } } },
    { "Necklace of Defense", { { 7, 5 },   { 9, 6 },   { 0, 0 } } },
    { "Archer's Medallion",  { { 7, 5 },   { 9, 6 },   { 0, 0 } } },
    { "Barbarian's Torque",  { { 7, 5 },   { 9, 6 },   { 0, 0 } } },
    { "Fighter's Torque",    { { 7, 5 },   { 9, 6 },   { 0, 0 } } },
    { "Thief's Torque",      { { 7, 5 },   { 9, 6 },   { 0, 0 } } },
    { "Warding Necklace",    { { 7, 5 },   { 9, 6 },   { 0, 0 } } },
    { "Necklace of Anshu",   { { 6, 2 },   { 8, 3 },   { 0, 0 } } },
    { "Medallion of Power",  { { 6, 4 },   { 8, 5 },   { 0, 0 } } },
    { "Swordsman's Torque",  { { 6, 4 },   { 8, 5 },   { 0, 0 } } },
};
const int inventory_list_pos[3][2]        = { { 342, 85 }, { 428, 103 }, { 547, 136 } };   // 0x57fd50 first row
const int inventory_list_width[3]         = { 258, 323, 413 };                             // 0x57fd68 count right edge
const int inventory_page_size[3]          = { 17, 17, 17 };                                // 0x57fd74 visible rows
const int inventory_scrollbar_track[3][2] = { { 612, 123 }, { 765, 154 }, { 981, 197 } };  // 0x57fd80
const int inventory_scrollbar_track_h[3]  = { 183, 229, 293 };                             // 0x57fd98
const int inventory_gold_pos[3][2]        = { { 476, 427 }, { 595, 534 }, { 762, 683 } };  // 0x57fda4 party gold
// 0x57e6b0 (0.5f) and 0x57eb84 (0.0f) are pooled __real literals of other units.

// ---- .data ---------------------------------------------------------------------------------
// "show_ui" (0x58bfcc), "Toggles the visibility state of the HUD" (0x58bfd4) and the four cmd_show_ui texts
// (0x58bffc-0x58c054) are this unit's literals.
//
// 0x58c080  45 x 0x40 (*new* name). flags: 1 paper-doll hot spot (double click: info, drag: unequip, right
// click: action popup), 2 acting on release through the Press entry after it, 4 acting on press with
// auto-repeat (the scroll arrows), 8 shows a tooltip on hover (inventory_hover_tooltips), 0x10 still usable
// in open mode 1. Action id = HiLite index + 1 (inventory_button_action).
ui_button_def inventory_button_defs[INVENTORY_NUM_BUTTONS] = {
    { "Inv-Bkgrnd",          { { 0, 0 },     { 0, 0 },     { 0, 0 } },     0 },
    { "Inv-Name-Joseph",     { { 113, 48 },  { 141, 60 },  { 176, 76 } },  0 },   // 1-4 species name tabs
    { "Inv-Name-Jekhar",     { { 113, 48 },  { 141, 60 },  { 176, 76 } },  0 },
    { "Inv-Name-Flece",      { { 113, 48 },  { 141, 60 },  { 176, 76 } },  0 },
    { "Inv-Name-Rosalind",   { { 113, 48 },  { 141, 60 },  { 176, 76 } },  0 },
    { "Icon-ArrwLFHilite",   { { 21, 23 },   { 26, 29 },   { 33, 35 } },   18 },  // 6 shop / journal / status
    { "Icon-ArrwLFPress",    { { 21, 23 },   { 26, 29 },   { 33, 35 } },   0 },
    { "Icon-ArrwRTHilite",   { { 295, 23 },  { 369, 29 },  { 470, 35 } },  18 },  // 8 status
    { "Icon-ArrwRTPress",    { { 295, 23 },  { 369, 29 },  { 470, 35 } },  0 },
    { "Icon-ArrwLFHilite",   { { 21, 61 },   { 26, 76 },   { 33, 95 } },   18 },  // 10 previous member
    { "Icon-ArrwLFPress",    { { 21, 61 },   { 26, 76 },   { 33, 95 } },   0 },
    { "Icon-ArrwRTHilite",   { { 295, 61 },  { 369, 76 },  { 470, 95 } },  18 },  // 12 next member
    { "Icon-ArrwRTPress",    { { 295, 61 },  { 369, 76 },  { 470, 95 } },  0 },
    { "Icon-ArrwUPHilite",   { { 610, 91 },  { 763, 114 }, { 975, 146 } }, 4 },   // 14 scroll up
    { "Icon-ArrwUPPress",    { { 610, 91 },  { 763, 114 }, { 975, 146 } }, 0 },
    { "Icon-ArrwDNHilite",   { { 610, 314 }, { 763, 393 }, { 975, 500 } }, 4 },   // 16 scroll down
    { "Icon-ArrwDNPress",    { { 610, 314 }, { 763, 393 }, { 975, 500 } }, 0 },
    { "Wndw-HvyArmr",        { { 18, 83 },   { 23, 104 },  { 15, 136 } },  0 },   // heavy armour picture
    { "Wndw-ArmrHilite",     { { 23, 88 },   { 29, 110 },  { 37, 141 } },  25 },  // 18-27 paper-doll hot spots
    { "Wndw-PntsHilite",     { { 23, 193 },  { 29, 241 },  { 37, 310 } },  25 },
    { "Wndw-BtsHilite",      { { 23, 298 },  { 29, 373 },  { 37, 478 } },  25 },
    { "Wndw-Rng1Hilite",     { { 23, 369 },  { 29, 461 },  { 37, 591 } },  25 },
    { "Wndw-Rng2Hilite",     { { 63, 369 },  { 79, 461 },  { 101, 591 } }, 25 },
    { "Wndw-NckHilite",      { { 191, 88 },  { 239, 110 }, { 305, 141 } }, 25 },
    { "Wndw-WpnHilite",      { { 253, 88 },  { 316, 110 }, { 405, 142 } }, 25 },
    { "Wndw-ShldHilite",     { { 253, 230 }, { 316, 288 }, { 405, 369 } }, 25 },
    { "Wndw-GntHilite",      { { 253, 336 }, { 316, 420 }, { 405, 539 } }, 25 },
    { "Wndw-HvyArmrHilite",  { { 18, 83 },   { 23, 104 },  { 15, 136 } },  25 },
    { "Icon-WpnActive",      { { 379, 349 }, { 474, 436 }, { 609, 568 } }, 0 },   // 28.. category icons
    { "Icon-WpnHilite",      { { 379, 349 }, { 474, 436 }, { 609, 568 } }, 10 },  // 30 weapons
    { "Icon-WpnPress",       { { 379, 349 }, { 474, 436 }, { 609, 568 } }, 0 },
    { "Icon-ArmrActive",     { { 420, 349 }, { 525, 436 }, { 674, 568 } }, 0 },
    { "Icon-ArmrHilite",     { { 420, 349 }, { 525, 436 }, { 674, 568 } }, 10 },  // 33 armour
    { "Icon-ArmrPress",      { { 420, 349 }, { 525, 436 }, { 674, 568 } }, 0 },
    { "Icon-MgkActive",      { { 461, 349 }, { 576, 436 }, { 739, 568 } }, 0 },
    { "Icon-MgkHilite",      { { 461, 349 }, { 576, 436 }, { 739, 568 } }, 10 },  // 36 magic items
    { "Icon-MgkPress",       { { 461, 349 }, { 576, 436 }, { 739, 568 } }, 0 },
    { "Icon-JwlryActive",    { { 501, 349 }, { 626, 436 }, { 804, 568 } }, 0 },
    { "Icon-JwlryHilite",    { { 501, 349 }, { 626, 436 }, { 804, 568 } }, 10 },  // 39 jewelry
    { "Icon-JwlryPress",     { { 501, 349 }, { 626, 436 }, { 804, 568 } }, 0 },
    { "Icon-MiscActive",     { { 542, 349 }, { 678, 436 }, { 869, 568 } }, 0 },
    { "Icon-MiscHilite",     { { 542, 349 }, { 678, 436 }, { 869, 568 } }, 10 },  // 42 miscellaneous
    { "Icon-MiscPress",      { { 542, 349 }, { 678, 436 }, { 869, 568 } }, 0 },
    { "Icon-CloseHilite",    { { 599, 19 },  { 749, 24 },  { 959, 31 } },  2 },   // 44 close
    { "Icon-ClosePress",     { { 599, 19 },  { 749, 24 },  { 959, 31 } },  0 },
};
// 0x58cbc0 (*new*). The strings are the pooled copies at 0x588b8c / 0x588b74 (new_assess.cpp's .data).
char *inventory_scrollbar_names[2] = { "ScrollBarMid640.tga", "ScrollBarMid1024.tga" };
// "inv_cameras.tbl" (0x58cbc8) is this unit's literal; "%d" (0x589820) and "" (0x5bd0d0) are pooled copies
// in other units.

// ---- .bss ----------------------------------------------------------------------------------
int inventory_scroll_pos;                                     // 0x5bd970
int inventory_pressed_button;                                 // 0x5bd974 pressed button / hot spot, -1
int inventory_scrollbar_w;                                    // 0x5bd978 thumb w
int inventory_scrollbar_h;                                    // 0x5bd97c thumb h
new_icam_table inventory_cameras;                             // 0x5bd980 (init_empty_414d40)
int inventory_item_actions[5];                                // 0x5c0fa8 action ids of the popup
int inventory_drag_start_mouse_y;                             // 0x5c0fbc holds the mouse y at drag start (see NOTES)
int inventory_row_count;                                      // 0x5c0fc0
bool inventory_selection_is_list_row;                         // 0x5c0fc4 popup / info opened from a list row
int inventory_saved_view_x;                                   // 0x5c0fc8
int inventory_saved_view_y;                                   // 0x5c0fcc
char *inventory_action_menu_strings[5];                       // 0x5c0fd0
int inventory_mouse_mode;                                     // 0x5c0fe4 0..5, see the head comment
int inventory_item_action_count;                              // 0x5c0fe8
int inventory_scrollbar_bm;                                   // 0x5c0fec
int inventory_scrollbar_bm_w;                                 // 0x5c0ff0
int inventory_scrollbar_bm_h;                                 // 0x5c0ff4
int inventory_selected_row;                                   // 0x5c0ff8 visible row, -1
RealtimeTimer inventory_repeat_timer;                         // 0x5c0ffc
int inventory_drag_start_thumb_y;                             // 0x5c1000 holds the thumb y at drag start (see NOTES)
bool inventory_item_action_enabled[5];                        // 0x5c1004
bool inventory_scrollbar_visible;                             // 0x5c1009
int inventory_paused_game;                                    // 0x5c100c 1 unpause, 2 refresh statscreen, 4 clear shop
int inventory_scrollbar_x;                                    // 0x5c1010 thumb x
int inventory_scrollbar_y;                                    // 0x5c1014 thumb y
bool inventory_open_mode;                                     // 0x5c1018 (0x5c1019-0x5c101f unused)
ui_button_state inventory_buttons[INVENTORY_NUM_BUTTONS];     // 0x5c1020
// 0x5c1458 (5 x 4 bytes; 0x5c146c-0x5c146f unused), built by init_strptrs_5c1458 (0x414d60, thunk 0x414d50).
// Indexed by the action id. Confidence: likely (as new_help.cpp).
inventory_str_ref inventory_action_strings[5] = {
    { XSTR("Info", 692) }, { XSTR("Identify", 693) }, { XSTR("Use", 694) }, { XSTR("Destroy", 695) },
    { XSTR("Cancel", 696) },
};

// The show_ui command object. Its address (0x5bd780) is inside the interface unit's .bss. 0x414c80
// init_ccmd_show_ui_thunk (static_initializers entry 49) / 0x414c90 init_ccmd_show_ui. Confidence: exact.
// Placement: most likely wrong. symbols/sum.symbols no longer tags 0x414c80-0x414cb0 new_inventory.cpp. The
// object sits between ui_choice_popup_flags (0x5bd77c) and the interface manager's other globals, and its .data
// follows that unit's, so it belongs at the end of the interface manager file (0x411270-, name unknown). It
// stays here until that file is written.
console_command_M ccmd_show_ui("show_ui", "Toggles the visibility state of the HUD", cmd_show_ui);

// Static initialisers in definition order (entries 49-52): 0x414c80/0x414c90 ccmd_show_ui (above);
// 0x414d30/0x414d40 init_empty_414d40 (a bare ret), read as the initialiser of inventory_cameras with the
// inline empty new_icam_table constructor (as status_cameras, NOTES-status.md Q1); 0x414d50/0x414d60
// inventory_action_strings; 0x414da0/0x414db0 "mov ecx, 0x5c0ffc / jmp RealtimeTimer__RealtimeTimer" for
// inventory_repeat_timer. Confidence: exact (compiler-generated) except the cameras pairing (unsure).
// The source order of the definitions is therefore ccmd_show_ui, inventory_cameras, the action strings,
// the timer; the .bss addresses do not follow it (same open question as the other units).

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
    if (checking_status_for_command)
        print_to_console(ui_input_locked ? "HUD is currently hidden" : "HUD is currently visible", 0);
    if (doing_help_for_comand)
        print_to_console("Toggles the visibility of the HUD.\n", 0);
}

// 0x414dc0  Confidence: exact.
void inv_cameras_parse_table()
{
    new_icam_load(&inventory_cameras, "inv_cameras.tbl");
}

// 0x414de0  Confidence: likely (journal_load_bitmaps shape; the i == 0 test is skipped from the i == 27
// branch, read as jump threading of a test after the if / else). Hot spot 27 (the heavy armour outline) takes
// its size from inventory_hvyarmr_size, unscaled, instead of from its bitmap.
void inventory_load_bitmaps()
{
    float scale = ui_get_scale();
    int i;

    for (i = 0; i < INVENTORY_NUM_BUTTONS; i++) {
        inventory_buttons[i].flags = 0;
        inventory_buttons[i].bitmap = -1;
        inventory_buttons[i].x = 0;
        inventory_buttons[i].y = 0;
        inventory_buttons[i].w = 0;
        inventory_buttons[i].h = 0;
        progress_update(0);

        if (strlen(inventory_button_defs[i].name)) {
            char filename[128];

            strcpy(filename, inventory_button_defs[i].name);
            strcat(filename, ui_bitmap_suffix_tga[ui_hires]);
            inventory_buttons[i].bitmap = bm_load(filename, path_interface, -1);
            inventory_buttons[i].x = inventory_button_defs[i].pos[resolution_index][0];
            inventory_buttons[i].y = inventory_button_defs[i].pos[resolution_index][1];
            if (i == 27) {
                inventory_buttons[i].w = inventory_hvyarmr_size[resolution_index][0];
                inventory_buttons[i].h = inventory_hvyarmr_size[resolution_index][1];
            } else {
                bm_get_dimensions(inventory_buttons[i].bitmap, &inventory_buttons[i].w, &inventory_buttons[i].h);
                inventory_buttons[i].w = (int)(inventory_buttons[i].w * scale + 0.5f);
                inventory_buttons[i].h = (int)(inventory_buttons[i].h * scale + 0.5f);
            }
            if (i == 0)
                inventory_buttons[i].flags |= 1;
        }
    }

    inventory_scrollbar_bm = bm_load(inventory_scrollbar_names[ui_hires], path_interface, -1);
    bm_get_dimensions(inventory_scrollbar_bm, &inventory_scrollbar_bm_w, &inventory_scrollbar_bm_h);
    inventory_scrollbar_bm_w = (int)(inventory_scrollbar_bm_w * scale);
    inventory_scrollbar_bm_h = (int)(inventory_scrollbar_bm_h * scale);
}

// 0x414fc0  Confidence: likely (as status_init / buysell_init: the found test after the loop survives as a
// dead compare). Without a character the first usable party member (type 7, no flag 0x108) is shown.
void inventory_init(bool open_mode)
{
    inventory_paused_game = 6;
    if (game_flags & 1) {
        if (!game_is_paused()) {
            inventory_paused_game |= 1;
            game_pause(0, 0);
        }
    } else {
        ui_screen_stop_player();
    }

    inventory_scroll_pos = 0;
    inventory_pressed_button = -1;
    inventory_selected_row = -1;
    inventory_mouse_mode = 0;
    inventory_open_mode = open_mode;

    if (status_character) {
        statscreen_refresh_anim(status_character);
    } else {
        Character *c;

        for (c = living_list.live_next; c != &living_list; c = c->live_next) {
            if (c->type == 7 && !(c->flags & 0x108)) {
                statscreen_refresh_anim(c);
                break;
            }
        }
        if (c == &living_list) {
            interface_set_screen(0, 0);
            return;
        }
    }

    inventory_update_species_tab();
    inventory_page_in_equipped();
    inventory_update_trade_buttons();
    inventory_update_equip_slots();
    inventory_count_rows();
    inventory_update_category_buttons();
    inventory_update_scrollbar();
}

// 0x4150c0  Confidence: likely (the loop runs on a pointer into equip_page_slots; the index tests become
// pointer compares). Weapon and shield go to the two paging slots of the hero, the rest to fixed slots.
void inventory_page_in_equipped()
{
    Character *c = status_character;
    int slot1, slot2;

    if (!c)
        return;
    item_paging_find_player_slot(c, &slot1, &slot2);
    if (c->equipment[5])
        item_page_in(INV_EQUIP(c, 5)->type - item_types, slot1, 0);
    if (c->equipment[6])
        item_page_in(INV_EQUIP(c, 6)->type - item_types, slot2, 0);
    for (int i = 0; i < 9; i++) {
        if (i != 5 && i != 6 && c->equipment[i])
            item_page_in(INV_EQUIP(c, i)->type - item_types, equip_page_slots[i], 0);
    }
}

// 0x415180  Confidence: exact (jump table 0x415210). Same as status_update_species_tab.
void inventory_update_species_tab()
{
    inventory_buttons[1].flags &= ~1;
    inventory_buttons[2].flags &= ~1;
    inventory_buttons[3].flags &= ~1;
    inventory_buttons[4].flags &= ~1;

    if (status_character) {
        switch (status_character->info->species) {
        case 0:
            inventory_buttons[1].flags |= 1;
            break;
        case 1:
            inventory_buttons[2].flags |= 1;
            break;
        case 2:
            inventory_buttons[3].flags |= 1;
            break;
        case 3:
            inventory_buttons[4].flags |= 1;
            break;
        }
    }
}

// 0x415220  Confidence: likely (a switch on the loop index over all 45 records, jump table 0x4153a8 / index
// bytes 0x4153c0 for i - 28, as buysell_update_category_buttons). Despite the name it only updates the
// category icons: a category with no entry hides its "Active" icon and disables its hot spot.
void inventory_update_category_buttons()
{
    for (int i = 0; i < INVENTORY_NUM_BUTTONS; i++) {
        switch (i) {
        case 28:
            if (inventory_category_first_index(0) >= 0) {
                inventory_buttons[28].flags |= 1;
                inventory_buttons[29].flags &= ~4;
            } else {
                inventory_buttons[28].flags &= ~1;
                inventory_buttons[29].flags |= 4;
            }
            break;
        case 31:
            if (inventory_category_first_index(1) >= 0) {
                inventory_buttons[31].flags |= 1;
                inventory_buttons[32].flags &= ~4;
            } else {
                inventory_buttons[31].flags &= ~1;
                inventory_buttons[32].flags |= 4;
            }
            break;
        case 34:
            if (inventory_category_first_index(2) >= 0) {
                inventory_buttons[34].flags |= 1;
                inventory_buttons[35].flags &= ~4;
            } else {
                inventory_buttons[34].flags &= ~1;
                inventory_buttons[35].flags |= 4;
            }
            break;
        case 37:
            if (inventory_category_first_index(3) >= 0) {
                inventory_buttons[37].flags |= 1;
                inventory_buttons[38].flags &= ~4;
            } else {
                inventory_buttons[37].flags &= ~1;
                inventory_buttons[38].flags |= 4;
            }
            break;
        case 40:
            if (inventory_category_first_index(4) >= 0) {
                inventory_buttons[40].flags |= 1;
                inventory_buttons[41].flags &= ~4;
            } else {
                inventory_buttons[40].flags &= ~1;
                inventory_buttons[41].flags |= 4;
            }
            break;
        }
    }
}

// 0x4153d0  Confidence: likely (loop shape). First party inventory index of the category, -1. In open mode
// only unidentified items count, but the index returned is still a party inventory index, not a row of the
// unidentified-only list (so the category jumps can scroll to the wrong row there; behaviour of the binary).
int inventory_category_first_index(int category)
{
    if (inventory_open_mode) {
        int i = -1;

        while (i < party_inventory_count - 1) {
            int n = party_inventory_find_first_in_category(category, &party_inventory[i + 1],
                                                           party_inventory_count - i - 1);
            if (n < 0)
                break;
            i += n + 1;
            if (!item_is_identified(party_inventory[i].item))
                return i;
        }
        return -1;
    }
    return party_inventory_find_first_in_category(category, party_inventory, party_inventory_count);
}

// 0x415470  Confidence: likely (as buysell_update_scrollbar: the second test is on the row count, always
// > 0 there). Thumb x / y / w / h from the track and page / rows.
void inventory_update_scrollbar()
{
    int page = inventory_page_size[resolution_index];

    if (inventory_row_count <= page) {
        inventory_scrollbar_visible = 0;
        inventory_buttons[13].flags |= 4;
        inventory_buttons[15].flags |= 4;
        return;
    }

    inventory_buttons[13].flags &= ~4;
    inventory_scrollbar_visible = 1;
    inventory_buttons[15].flags &= ~4;
    if (inventory_row_count <= 0) {
        inventory_scrollbar_x = inventory_scrollbar_track[resolution_index][0];
        inventory_scrollbar_y = inventory_scrollbar_track[resolution_index][1];
        inventory_scrollbar_w = inventory_scrollbar_bm_w;
        inventory_scrollbar_h = inventory_scrollbar_track_h[resolution_index];
    } else {
        inventory_scrollbar_x = inventory_scrollbar_track[resolution_index][0];
        inventory_scrollbar_y = (int)((float)inventory_scroll_pos / (float)inventory_row_count
                                      * (float)inventory_scrollbar_track_h[resolution_index] + 0.5f)
                              + inventory_scrollbar_track[resolution_index][1];
        inventory_scrollbar_w = inventory_scrollbar_bm_w;
        inventory_scrollbar_h = (int)((float)page / (float)inventory_row_count
                                      * (float)inventory_scrollbar_track_h[resolution_index] + 0.5f);
    }
}

// 0x415580  Confidence: exact (same code as status_update_member_buttons). Despite the name: the previous /
// next member arrows (9, 11), enabled with more than one member (the summon not counted) in single player.
void inventory_update_trade_buttons()
{
    int count = party_count_active();

    if (summon_player)
        count--;
    if (count > 1 && !(game_flags & 2)) {
        inventory_buttons[9].flags &= ~4;
        inventory_buttons[11].flags &= ~4;
    } else {
        inventory_buttons[9].flags |= 4;
        inventory_buttons[11].flags |= 4;
    }
}

// 0x4155f0  Confidence: likely (the store order with no character follows the equipment order; written as
// one statement per hot spot). An empty slot disables its hot spot. Heavy armour (ItemType slot 8 in
// equipment 7) covers both armour and pants: it shows picture 17 and uses hot spot 27 instead of 18 / 19.
void inventory_update_equip_slots()
{
    Character *c = status_character;

    if (!c) {
        inventory_buttons[23].flags |= 4;
        inventory_buttons[21].flags |= 4;
        inventory_buttons[22].flags |= 4;
        inventory_buttons[26].flags |= 4;
        inventory_buttons[20].flags |= 4;
        inventory_buttons[24].flags |= 4;
        inventory_buttons[25].flags |= 4;
        inventory_buttons[18].flags |= 4;
        inventory_buttons[19].flags |= 4;
        inventory_buttons[27].flags |= 4;
        inventory_buttons[17].flags &= ~1;
        return;
    }

    if (c->equipment[0])
        inventory_buttons[23].flags &= ~4;
    else
        inventory_buttons[23].flags |= 4;
    if (c->equipment[1])
        inventory_buttons[21].flags &= ~4;
    else
        inventory_buttons[21].flags |= 4;
    if (c->equipment[2])
        inventory_buttons[22].flags &= ~4;
    else
        inventory_buttons[22].flags |= 4;
    if (c->equipment[3])
        inventory_buttons[26].flags &= ~4;
    else
        inventory_buttons[26].flags |= 4;
    if (c->equipment[4])
        inventory_buttons[20].flags &= ~4;
    else
        inventory_buttons[20].flags |= 4;
    if (c->equipment[5])
        inventory_buttons[24].flags &= ~4;
    else
        inventory_buttons[24].flags |= 4;
    if (c->equipment[6])
        inventory_buttons[25].flags &= ~4;
    else
        inventory_buttons[25].flags |= 4;

    if (c->equipment[7]) {
        if (INV_EQUIP(c, 7)->type->slot == 8) {
            inventory_buttons[18].flags |= 4;
            inventory_buttons[19].flags |= 4;
            inventory_buttons[27].flags &= ~4;
            inventory_buttons[17].flags |= 1;
            return;
        }
        inventory_buttons[18].flags &= ~4;
        inventory_buttons[27].flags |= 4;
        inventory_buttons[17].flags &= ~1;
    } else {
        inventory_buttons[18].flags |= 4;
        inventory_buttons[27].flags |= 4;
        inventory_buttons[17].flags &= ~1;
    }
    if (c->equipment[8])
        inventory_buttons[19].flags &= ~4;
    else
        inventory_buttons[19].flags |= 4;
}

// 0x415850  Confidence: exact. Despite the name: the row count of the list (all items, or the unidentified
// ones in open mode).
void inventory_count_rows()
{
    if (inventory_open_mode) {
        inventory_row_count = 0;
        for (int i = 0; i < party_inventory_count; i++) {
            if (!item_is_identified(party_inventory[i].item))
                inventory_row_count++;
        }
    } else {
        inventory_row_count = party_inventory_count;
    }
}

// 0x4158b0  Confidence: exact.
void inventory_leave()
{
    if (inventory_paused_game & 1)
        game_unpause();
    if (inventory_paused_game & 2)
        statscreen_refresh_anim(0);
    if (inventory_paused_game & 4)
        shop_set_shopkeeper_from_char(0);
}

// 0x4158f0  Confidence: exact (jump table 0x415984). Screen 1 per-frame input.
int inventory_frame(int key)
{
    for (ui_button_state *b = inventory_buttons; b < inventory_buttons + INVENTORY_NUM_BUTTONS; b++)
        b->flags &= ~2;

    if (ui_message_box_is_active())
        return key;
    if (item_info_popup_is_active())
        return item_info_popup_frame(key);

    switch (inventory_mouse_mode) {
    case 0:
        key = inventory_handle_click(key);
        break;
    case 1:
        inventory_update_buttons();
        break;
    case 2:
        inventory_drag_scrollbar();
        break;
    case 3:
        key = inventory_popup_result(key);
        break;
    case 4:
        inventory_drag_to_equip();
        break;
    case 5:
        inventory_drag_from_equip();
        break;
    }
    return key;
}

// 0x4159a0  Confidence: likely (the many return tails are VC6's; key jump table 0x415fd4, index bytes
// 0x415fe8 for key - 1 in 0..0x2f). Buttons and paper-doll hot spots, thumb, list rows, keys, mouse wheel.
// Hot spot: double click shows the item info, press starts dragging it (mode 5; not the weapon), right click
// opens the action popup. List row: double click shows the info of an identified item, press starts dragging
// an equippable one (mode 4), right click opens the action popup.
int inventory_handle_click(int key)
{
    int i;

    inventory_pressed_button = -1;
    inventory_selected_row = -1;

    for (i = 0; i < INVENTORY_NUM_BUTTONS; i++) {
        if ((inventory_button_defs[i].flags & 0xf)
            && point_in_rect(game_mouse_x, game_mouse_y, inventory_buttons[i].x, inventory_buttons[i].y,
                             inventory_buttons[i].w, inventory_buttons[i].h)) {
            if ((inventory_button_defs[i].flags & 8) && interface_input_ready())
                inventory_hover_tooltips(i);
            if (!(inventory_buttons[i].flags & 4)
                && (!inventory_open_mode || (inventory_button_defs[i].flags & 0x10))) {
                inventory_buttons[i].flags |= 2;
                inventory_pressed_button = i;
                if (inventory_button_defs[i].flags & 1) {
                    int slot = inventory_equip_index_for_button(i);

                    if (mouse_was_double_clicked(0)) {
                        if (slot != -1) {
                            if (!item_is_identified(INV_EQUIP(status_character, slot))) {
                                game_play_sound(0x24, 1.0f, 0, 0, 0);
                                return key;
                            }
                            inventory_selection_is_list_row = 0;
                            inventory_show_item_info();
                            game_play_sound(0x23, 1.0f, 0, 0, 0);
                            return key;
                        }
                    } else if (mouse_was_pressed(0)) {
                        if (slot != -1 && slot != 5) {
                            item_info *item = INV_EQUIP(status_character, slot);
                            inventory_mouse_mode = 5;
                            item_page_in(item->type - item_types, INVENTORY_PAGE_SLOT, 0);
                            return key;
                        }
                    } else if (mouse_was_pressed(1)) {
                        inventory_mouse_mode = 3;
                        item_info *item = INV_EQUIP(status_character, slot);
                        if (!item)
                            return key;
                        inventory_selection_is_list_row = 0;
                        inventory_update_item_buttons(item, inventory_selection_is_list_row);
                        for (int j = 0; j < inventory_item_action_count; j++)
                            inventory_action_menu_strings[j] = localize_string_or_invalid(
                                &inventory_action_strings[inventory_item_actions[j]].s);
                        ui_choice_popup_show(0, game_mouse_x, game_mouse_y, inventory_item_action_count,
                                             inventory_action_menu_strings, 0, 0,
                                             (int)inventory_item_action_enabled);
                        return key;
                    }
                } else if (inventory_button_defs[i].flags & 2) {
                    if (mouse_was_pressed(0)) {
                        inventory_mouse_mode = 1;
                        inventory_repeat_timer.invalidate();
                        return key;
                    }
                } else if (inventory_button_defs[i].flags & 4) {
                    if (mouse_was_pressed(0)) {
                        inventory_mouse_mode = 1;
                        inventory_button_action(inventory_pressed_button + 1);
                        inventory_repeat_timer.SetTimeout(500);
                        return key;
                    }
                }
                break;
            }
        }
    }

    if (point_in_rect(game_mouse_x, game_mouse_y, inventory_scrollbar_x, inventory_scrollbar_y,
                      inventory_scrollbar_w, inventory_scrollbar_h)
        && mouse_was_pressed(0)) {
        inventory_drag_start_thumb_y = inventory_scrollbar_y;
        inventory_mouse_mode = 2;
        inventory_drag_start_mouse_y = game_mouse_y;
        return key;
    }

    int rows = inventory_row_count;
    if (rows >= inventory_page_size[resolution_index])
        rows = inventory_page_size[resolution_index];
    int fh = gr_get_font_height(-1);
    if (point_in_rect(game_mouse_x, game_mouse_y, inventory_list_pos[resolution_index][0],
                      inventory_list_pos[resolution_index][1], inventory_list_width[resolution_index], fh * rows)) {
        for (i = 0; i < rows; i++) {
            if (point_in_rect(game_mouse_x, game_mouse_y, inventory_list_pos[resolution_index][0],
                              inventory_list_pos[resolution_index][1] + i * fh,
                              inventory_list_width[resolution_index], fh)) {
                inventory_selected_row = i;
                if (mouse_was_double_clicked(0)) {
                    party_inventory_entry *entry = inventory_get_list_item(inventory_scroll_pos + i);
                    if (!item_type_is_identified(entry->item->type)) {
                        game_play_sound(0x24, 1.0f, 0, 0, 0);
                        return key;
                    }
                    inventory_selection_is_list_row = 1;
                    inventory_show_item_info();
                    game_play_sound(0x23, 1.0f, 0, 0, 0);
                    return key;
                }
                if (mouse_was_pressed(0)) {
                    party_inventory_entry *entry = inventory_get_list_item(inventory_scroll_pos + i);
                    if (item_type_is_identified(entry->item->type) && entry->item->type->slot != -1) {
                        inventory_mouse_mode = 4;
                        item_page_in(entry->item->type - item_types, INVENTORY_PAGE_SLOT, 0);
                    }
                    return key;
                }
                if (mouse_was_pressed(1)) {
                    inventory_mouse_mode = 3;
                    inventory_selection_is_list_row = 1;
                    party_inventory_entry *entry = inventory_get_list_item(inventory_scroll_pos + i);
                    inventory_update_item_buttons(entry->item, inventory_selection_is_list_row);
                    for (int j = 0; j < inventory_item_action_count; j++)
                        inventory_action_menu_strings[j] = localize_string_or_invalid(
                            &inventory_action_strings[inventory_item_actions[j]].s);
                    ui_choice_popup_show(0, game_mouse_x, game_mouse_y, inventory_item_action_count,
                                         inventory_action_menu_strings, 0, 0, (int)inventory_item_action_enabled);
                    return key;
                }
                break;
            }
        }
    }

    switch (key) {
    case KEY_ESC:
    case KEY_I:
        interface_set_screen(0, 0);
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        return 0;
    case KEY_Q:
        if (!(game_flags & 1))
            break;
        inventory_paused_game &= ~6;
        interface_set_screen(10, 0);
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;
    case KEY_R:
        inventory_paused_game &= ~6;
        interface_set_screen(2, 0);
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;
    case KEY_B:
        if (!ui_has_owner_character())
            break;
        inventory_paused_game &= ~6;
        interface_set_screen(7, 0);
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;
    }

    int dx, dy, dz;
    mouse_get_delta(&dx, &dy, &dz);
    if (dz > 0)
        inventory_page_up();
    else if (dz < 0)
        inventory_page_down();
    return key;
}

// 0x416020  Confidence: exact. Open mode counts only unidentified items.
party_inventory_entry *inventory_get_list_item(int row)
{
    if (inventory_open_mode) {
        for (int i = 0; i < party_inventory_count; i++) {
            if (!item_is_identified(party_inventory[i].item)) {
                if (row == 0)
                    return &party_inventory[i];
                row--;
            }
        }
        return 0;
    }
    if (row < 0 || row >= party_inventory_count)
        return 0;
    return &party_inventory[row];
}

// 0x416090  Confidence: exact (jump table 0x4160dc; case 27 shares case 18's body). Despite the name: the
// paper-doll hot-spot button (18-27) to the equipment index, -1.
int inventory_equip_index_for_button(int button)
{
    switch (button) {
    case 18:                          // Wndw-ArmrHilite
    case 27:                          // Wndw-HvyArmrHilite
        return 7;
    case 19:                          // Wndw-PntsHilite
        return 8;
    case 20:                          // Wndw-BtsHilite
        return 4;
    case 21:                          // Wndw-Rng1Hilite
        return 1;
    case 22:                          // Wndw-Rng2Hilite
        return 2;
    case 23:                          // Wndw-NckHilite
        return 0;
    case 24:                          // Wndw-WpnHilite
        return 5;
    case 25:                          // Wndw-ShldHilite
        return 6;
    case 26:                          // Wndw-GntHilite
        return 3;
    }
    return -1;
}

// 0x416110  Confidence: exact (the open-mode test before the destroy action is jump-threaded from the first
// one). Actions: 0 info (identified) or 1 identify (enabled when someone can), 2 use (magic info), 3 destroy
// (list items only, not in open mode), 4 cancel.
void inventory_update_item_buttons(item_info *item, bool from_list)
{
    inventory_item_action_count = 0;
    if (item_is_identified(item)) {
        inventory_item_actions[inventory_item_action_count] = 0;
        inventory_item_action_enabled[inventory_item_action_count] = 1;
    } else {
        inventory_item_actions[inventory_item_action_count] = 1;
        inventory_item_action_enabled[inventory_item_action_count] = inventory_can_identify();
    }
    inventory_item_action_count++;
    if (!inventory_open_mode && item_has_magic_info(item)) {
        inventory_item_actions[inventory_item_action_count] = 2;
        inventory_item_action_enabled[inventory_item_action_count] = 1;
        inventory_item_action_count++;
    }
    if (!inventory_open_mode && from_list && item_can_destroy(item)) {
        inventory_item_actions[inventory_item_action_count] = 3;
        inventory_item_action_enabled[inventory_item_action_count] = 1;
        inventory_item_action_count++;
    }
    inventory_item_actions[inventory_item_action_count] = 4;
    inventory_item_action_enabled[inventory_item_action_count] = 1;
    inventory_item_action_count++;
}

// 0x416200  Confidence: exact. Characters with flags 0x82 (0x8a outside level 4) cannot identify. Open mode:
// only status_character; else anyone in the party.
bool inventory_can_identify()
{
    int mask = cur_level_index == 4 ? 0x82 : 0x8a;

    if (inventory_open_mode) {
        if (status_character && !(status_character->flags & mask))
            return skill_is_available(SKILL_IDENTIFY, status_character);
        return false;
    }
    for (Player *p = party_list.next; p != &party_list; p = p->next) {
        Character *c = living_from_handle(p->handle);
        if (c && !(c->flags & mask) && skill_is_available(SKILL_IDENTIFY, c))
            return true;
    }
    return false;
}

// 0x416290  Confidence: exact (jump table 0x416500 for button - 18). Hot spots show the equipped item's name,
// the category hot spots the category name; nothing is drawn here.
void inventory_hover_tooltips(int button)
{
    char text[256] = "";

    switch (button) {
    case 18:
        if (status_character->equipment[7] && INV_EQUIP(status_character, 7)->type->slot == 6)
            item_get_display_name(text, INV_EQUIP(status_character, 7)->type);
        break;
    case 19:
        if (status_character->equipment[8])
            item_get_display_name(text, INV_EQUIP(status_character, 8)->type);
        break;
    case 20:
        if (status_character->equipment[4])
            item_get_display_name(text, INV_EQUIP(status_character, 4)->type);
        break;
    case 21:
        if (status_character->equipment[1])
            item_get_display_name(text, INV_EQUIP(status_character, 1)->type);
        break;
    case 22:
        if (status_character->equipment[2])
            item_get_display_name(text, INV_EQUIP(status_character, 2)->type);
        break;
    case 23:
        if (status_character->equipment[0])
            item_get_display_name(text, INV_EQUIP(status_character, 0)->type);
        break;
    case 24:
        if (status_character->equipment[5])
            item_get_display_name(text, INV_EQUIP(status_character, 5)->type);
        break;
    case 25:
        if (status_character->equipment[6])
            item_get_display_name(text, INV_EQUIP(status_character, 6)->type);
        break;
    case 26:
        if (status_character->equipment[3])
            item_get_display_name(text, INV_EQUIP(status_character, 3)->type);
        break;
    case 27:
        if (status_character->equipment[7] && INV_EQUIP(status_character, 7)->type->slot == 8)
            item_get_display_name(text, INV_EQUIP(status_character, 7)->type);
        break;
    case 29:
        strcpy(text, localize_strings[640]);      // "Weapons"
        break;
    case 32:
        strcpy(text, localize_strings[641]);      // "Armor"
        break;
    case 35:
        strcpy(text, localize_strings[642]);      // "Magic items"
        break;
    case 38:
        strcpy(text, localize_strings[643]);      // "Jewelry"
        break;
    case 41:
        strcpy(text, localize_strings[644]);      // "Miscellaneous items"
        break;
    }
    if (strlen(text))
        ui_tooltip_set(inventory_buttons[button].x, inventory_buttons[button].y,
                       inventory_buttons[button].w, inventory_buttons[button].h, text);
}

// 0x416560  Confidence: exact. The selected list row's item, or the pressed hot spot's equipped item.
void inventory_show_item_info()
{
    item_info *item;

    if (inventory_selection_is_list_row) {
        party_inventory_entry *entry = inventory_get_list_item(inventory_scroll_pos + inventory_selected_row);
        if (!entry)
            return;
        item = entry->item;
    } else {
        int slot = inventory_equip_index_for_button(inventory_pressed_button);
        if (slot == -1)
            return;
        item = INV_EQUIP(status_character, slot);
    }
    if (item)
        item_info_popup_open_item(item);
}

// 0x4165c0  Confidence: exact.
void inventory_page_up()
{
    if (inventory_scroll_pos > 0) {
        inventory_scroll_pos = max(inventory_scroll_pos - inventory_page_size[resolution_index], 0);
        inventory_update_scrollbar();
    }
}

// 0x4165f0  Confidence: exact.
void inventory_page_down()
{
    int page = inventory_page_size[resolution_index];
    int limit = inventory_row_count - page;

    if (inventory_scroll_pos < limit) {
        inventory_scroll_pos = min(inventory_scroll_pos + page, limit);
        inventory_update_scrollbar();
    }
}

// 0x416620  Confidence: exact (jump table 0x416784, index bytes 0x4167b8 for id - 6). id = HiLite + 1.
void inventory_button_action(int id)
{
    switch (id) {
    case 6:
        inventory_goto_other_screen();
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        break;
    case 8:
        inventory_goto_status();
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        break;
    case 10:
        inventory_prev_member();
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 12:
        inventory_next_member();
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 14:
        inventory_scroll_up();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 16:
        inventory_scroll_down();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 30:
        inventory_goto_category0();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 33:
        inventory_goto_category1();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 36:
        inventory_goto_category2();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 39:
        inventory_goto_category3();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 42:
        inventory_goto_category4();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 44:
        interface_close_screen();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    }
}

// 0x4167e0  Confidence: exact (two has-owner calls, so the game_flags test is the outer one). The left arrow:
// the shop when a shopkeeper is set, else the journal in single player, the Character screen in multiplayer.
void inventory_goto_other_screen()
{
    inventory_paused_game &= ~6;
    if (game_flags & 1) {
        if (ui_has_owner_character())
            interface_set_screen(7, 0);
        else
            interface_set_screen(10, 0);
    } else {
        if (ui_has_owner_character())
            interface_set_screen(7, 0);
        else
            interface_set_screen(2, 0);
    }
}

// 0x416840  Confidence: exact.
void inventory_goto_status()
{
    inventory_paused_game &= ~6;
    interface_set_screen(2, 0);
}

// 0x416860  Confidence: exact.
void inventory_prev_member()
{
    statscreen_show_prev_member(1);
}

// 0x416870  Confidence: exact.
void inventory_next_member()
{
    statscreen_show_next_member(1);
}

// 0x416880  Confidence: exact.
void inventory_scroll_up()
{
    if (inventory_scroll_pos > 0) {
        inventory_scroll_pos--;
        inventory_update_scrollbar();
    }
}

// 0x4168a0  Confidence: exact.
void inventory_scroll_down()
{
    if (inventory_scroll_pos < inventory_row_count - inventory_page_size[resolution_index]) {
        inventory_scroll_pos++;
        inventory_update_scrollbar();
    }
}

// 0x4168d0 .. 0x4169d0: scroll to the first entry of a category (0 weapons .. 4 misc), kept a page from the
// end. Five copies of the same code. Confidence: exact (one call each, so the first index is a local).
void inventory_goto_category0()
{
    int first = inventory_category_first_index(0);
    int pos = min(first, inventory_row_count - inventory_page_size[resolution_index]);
    inventory_scroll_pos = max(pos, 0);
    inventory_update_scrollbar();
}

void inventory_goto_category1()
{
    int first = inventory_category_first_index(1);
    int pos = min(first, inventory_row_count - inventory_page_size[resolution_index]);
    inventory_scroll_pos = max(pos, 0);
    inventory_update_scrollbar();
}

void inventory_goto_category2()
{
    int first = inventory_category_first_index(2);
    int pos = min(first, inventory_row_count - inventory_page_size[resolution_index]);
    inventory_scroll_pos = max(pos, 0);
    inventory_update_scrollbar();
}

void inventory_goto_category3()
{
    int first = inventory_category_first_index(3);
    int pos = min(first, inventory_row_count - inventory_page_size[resolution_index]);
    inventory_scroll_pos = max(pos, 0);
    inventory_update_scrollbar();
}

void inventory_goto_category4()
{
    int first = inventory_category_first_index(4);
    int pos = min(first, inventory_row_count - inventory_page_size[resolution_index]);
    inventory_scroll_pos = max(pos, 0);
    inventory_update_scrollbar();
}

// 0x416a10  Confidence: exact. Shared by the close buttons of many screens.
void interface_close_screen()
{
    interface_set_screen(0, 0);
}

// 0x416a20  Confidence: exact (buysell_update_buttons template). Mouse mode 1: the action fires on release
// over the button unless the repeat timer runs, and repeats every 100 ms while held.
void inventory_update_buttons()
{
    int id = inventory_pressed_button + 1;

    if (inventory_pressed_button >= 0 && inventory_pressed_button < INVENTORY_NUM_BUTTONS) {
        ui_button_state *b = &inventory_buttons[id];

        if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
            b->flags |= 2;
        if (mouse_is_down(0)) {
            if (inventory_repeat_timer.valid() && inventory_repeat_timer.elapsed() && (b->flags & 2)) {
                inventory_button_action(id);
                inventory_repeat_timer.SetTimeout(100);
            }
            return;
        }
        if ((b->flags & 2) && !inventory_repeat_timer.valid())
            inventory_button_action(id);
    }
    inventory_mouse_mode = 0;
}

// 0x416af0  Confidence: likely (one store to the scroll position, so the clamp is one expression). The
// stored start values: see NOTES (the two sum.symbols names are swapped).
void inventory_drag_scrollbar()
{
    int offset = game_mouse_y - inventory_scrollbar_track[resolution_index][1] - inventory_drag_start_mouse_y
               + inventory_drag_start_thumb_y;
    int pos = (int)((float)offset / inventory_scrollbar_track_h[resolution_index] * inventory_row_count + 0.5f);

    pos = max(pos, 0);
    inventory_scroll_pos = min(pos, inventory_row_count - inventory_page_size[resolution_index]);
    inventory_update_scrollbar();
    if (!mouse_is_down(0))
        inventory_mouse_mode = 0;
}

// 0x416b80  Confidence: likely (jump table 0x416bfc; the duplicated "mode = 0" tails are read as breaks).
// Mouse mode 3: -1 still open, -2 cancelled (the key is eaten).
int inventory_popup_result(int key)
{
    int choice = ui_choice_popup_frame(key);

    if (choice == -1)
        return key;
    if (choice == -2) {
        inventory_mouse_mode = 0;
        return 0;
    }
    if (choice >= 0) {
        switch (inventory_item_actions[choice]) {
        case 0:
            inventory_show_item_info();
            break;
        case 1:
            inventory_identify_item();
            break;
        case 2:
            inventory_use_selected_item();
            break;
        case 3:
            inventory_confirm_destroy();      // resets the mouse mode itself
            return key;
        }
    }
    inventory_mouse_mode = 0;
    return key;
}

// 0x416c10  Confidence: likely (the slot switch of inventory_equip_index_for_button is written out again here,
// jump table 0x416d98; the "flags & 4 || flags & 2" test is laid out differently in the two branches).
// Items whose spell needs a target (spell flags 2 character / 4 location) start the world pick and close the
// screen; others are used at once on the character (the screen closes when that succeeds).
void inventory_use_selected_item()
{
    if (inventory_selection_is_list_row) {
        party_inventory_entry *entry = inventory_get_list_item(inventory_scroll_pos + inventory_selected_row);
        if (!entry)
            return;
        item_charge_effect *charge = (item_charge_effect *)entry->item->type->charge_effect;
        if (!charge->self_only) {
            unsigned int flags = spells[charge->spell].flags;
            if ((flags & 4) || (flags & 2)) {
                inventory_use_list_item_begin(entry);
                interface_set_screen(0, 0);
                return;
            }
        }
        if (character_use_inventory_item(status_character, entry, status_character->handle, 0, -1))
            interface_set_screen(0, 0);
        return;
    }

    int slot;
    switch (inventory_pressed_button) {
    case 18:
    case 27:
        slot = 7;
        break;
    case 19:
        slot = 8;
        break;
    case 20:
        slot = 4;
        break;
    case 21:
        slot = 1;
        break;
    case 22:
        slot = 2;
        break;
    case 23:
        slot = 0;
        break;
    case 24:
        slot = 5;
        break;
    case 25:
        slot = 6;
        break;
    case 26:
        slot = 3;
        break;
    default:
        return;
    }

    Character *c = status_character;
    item_charge_effect *charge = (item_charge_effect *)INV_EQUIP(c, slot)->type->charge_effect;
    if (!charge->self_only) {
        unsigned int flags = spells[charge->spell].flags;
        if ((flags & 4) || (flags & 2)) {
            inventory_use_equipped_item_begin(slot);
            interface_set_screen(0, 0);
            return;
        }
    }
    if (character_use_equipped_item(c, slot, c->handle, 0, -1))
        interface_set_screen(0, 0);
}

// 0x416dc0  Confidence: likely (the charge effect is re-read after ui_clear_pick_overrides, so it is not a
// local). Starts targeting with a list item's spell: pick radius from the spell, spell 0x1d (centred on the
// character) also fixes the world pick at cur_character; flags 2 / 4 hand over to the HUD target modes 0 / 1,
// else the AI casts it and the world pick filters by the spell.
void inventory_use_list_item_begin(party_inventory_entry *entry)
{
    if (!entry->item->type->charge_effect)
        return;
    ui_clear_pick_overrides();

    int spell = ((item_charge_effect *)entry->item->type->charge_effect)->spell;
    if (spells[spell].target_radius != 0.0f) {
        ui_pick_screen_override_on = 1;
        ui_pick_override_x = ui_pick_override_y = spells[spell].target_radius;
        if (spell == 0x1d) {
            ui_pick_world_override_on = 1;
            ui_pick_world_override_pos = cur_character->pos;
            ui_pick_override_y = 1.0f;
        }
    }
    ui_using_list_item = 1;
    ui_using_list_item_entry = entry;
    if (spells[spell].flags & 2) {
        ui_pending_use_skill = spell;
        hud_target_mode = 0;
        return;
    }
    if (spells[spell].flags & 4) {
        ui_pending_use_skill = spell;
        hud_target_mode = 1;
        return;
    }
    AI_GOAL_M(cur_character)->add_cast_spell(spell, 999999);
    world_pick_set_skill_filter(spell);
}

// 0x416eb0  Confidence: likely (as inventory_use_list_item_begin). The same for status_character's
// equipped item in the slot.
void inventory_use_equipped_item_begin(int slot)
{
    item_info *item = INV_EQUIP(status_character, slot);

    if (!item->type->charge_effect)
        return;
    ui_clear_pick_overrides();

    int spell = ((item_charge_effect *)item->type->charge_effect)->spell;
    if (spells[spell].target_radius != 0.0f) {
        ui_pick_screen_override_on = 1;
        ui_pick_override_x = ui_pick_override_y = spells[spell].target_radius;
        if (spell == 0x1d) {
            ui_pick_world_override_on = 1;
            ui_pick_world_override_pos = cur_character->pos;
            ui_pick_override_y = 1.0f;
        }
    }
    ui_using_equipped_item = 1;
    ui_using_equipped_slot = slot;
    if (spells[spell].flags & 2) {
        ui_pending_use_skill = spell;
        hud_target_mode = 0;
        return;
    }
    if (spells[spell].flags & 4) {
        ui_pending_use_skill = spell;
        hud_target_mode = 1;
        return;
    }
    AI_GOAL_M(cur_character)->add_cast_spell(spell, 999999);
    world_pick_set_skill_filter(spell);
}

// 0x416fb0  Confidence: likely (the "== true" on GameTimer::elapsed is read from cmp al, 1). The party member
// with the best identify skill whose identify cooldown has run out tries it; success opens the item info
// (open mode drops the row), failure shows "Couldn't identify item.".
void inventory_identify_item()
{
    Character *best = 0;
    float best_skill = 0.0f;

    if (!inventory_selection_is_list_row)
        return;
    party_inventory_entry *entry = inventory_get_list_item(inventory_scroll_pos + inventory_selected_row);
    if (!entry)
        return;

    for (Player *p = party_list.next; p != &party_list; p = p->next) {
        Character *c = living_from_handle(p->handle);
        if (c && !(c->flags & 0x88)) {
            float skill = CHAR_M(c)->get_skill(SKILL_IDENTIFY);
            if (skill > best_skill && ((GameTimer *)&p->skill_cooldown[SKILL_IDENTIFY])->elapsed() == true) {
                best = c;
                best_skill = skill;
            }
        }
    }
    if (!best)
        return;
    if (!character_use_skill(SKILL_IDENTIFY, entry->item, best))
        return;

    if (item_is_identified(entry->item)) {
        item_info_popup_open_item(entry->item);
        if (inventory_open_mode) {
            inventory_count_rows();
            int pos = min(inventory_scroll_pos, inventory_row_count - inventory_page_size[resolution_index]);
            inventory_scroll_pos = max(pos, 0);
            inventory_update_category_buttons();
            inventory_update_scrollbar();
        }
    } else {
        char *buttons[1];
        buttons[0] = localize_strings[213];       // "Ok"
        ui_message_box_show(localize_strings[697], localize_strings[698], -1, -1, 1, buttons, 0, 0);
                                                  // "Failure", "Couldn't identify item."
    }
}

// 0x417110  Confidence: exact.
void inventory_confirm_destroy()
{
    char *buttons[2];

    buttons[0] = localize_strings[654];           // "Yes"
    buttons[1] = localize_strings[655];           // "No"
    ui_message_box_show(localize_strings[645], localize_strings[699], -1, -1, 2, buttons,
                        inventory_confirm_destroy_cb, 0);   // "Warning", "Destroy item?"
    inventory_mouse_mode = 0;
}

// 0x417160  Confidence: exact (the call is a tail jump).
void inventory_confirm_destroy_cb(int button)
{
    if (button == 0)
        inventory_destroy_selected_item();
}

// 0x417170  Confidence: likely (item_can_destroy is called twice on the same item; written as two tests).
// One unit of a stack goes; the last one removes the entry. The row is used as the party inventory index
// (right outside open mode, the only mode that offers destroy).
void inventory_destroy_selected_item()
{
    if (!inventory_selection_is_list_row)
        return;
    int index = inventory_selected_row + inventory_scroll_pos;
    party_inventory_entry *entry = inventory_get_list_item(index);
    if (!entry)
        return;
    if (!item_can_destroy(entry->item))
        return;
    if (!item_can_destroy(entry->item))
        return;

    if (entry->count > 1) {
        entry->count--;
    } else {
        party_inventory_remove(index, 1, 1);
        inventory_count_rows();
        int pos = min(inventory_scroll_pos, inventory_row_count - inventory_page_size[resolution_index]);
        inventory_scroll_pos = max(pos, 0);
        inventory_update_category_buttons();
        inventory_update_scrollbar();
    }
}

// 0x417220  Confidence: likely (local order; jump table 0x417478 on ItemType slot 0..9). Mouse mode 4: the
// dragged list item is equipped when it is released over one of its hot spots (rings: either ring slot).
// Items the character cannot use, and a shield while a two-handed weapon (slot 9) is held, have no target.
void inventory_drag_to_equip()
{
    int buttons[2] = { -1, -1 };
    int slots[2] = { -1, -1 };
    int n = 0;
    party_inventory_entry *entry = inventory_get_list_item(inventory_scroll_pos + inventory_selected_row);
    bool ok = true;
    int i;

    if (!character_meets_item_requirements(status_character, entry->item->type))
        ok = false;
    else if (entry->item->type->slot == 5 && status_character->equipment[5]
             && INV_EQUIP(status_character, 5)->type->slot == 9)
        ok = false;

    i = 0;
    if (ok) {
        switch (entry->item->type->slot) {
        case 0:
            slots[0] = 0;
            buttons[0] = 23;
            n = 1;
            break;
        case 1:
            n = 2;
            slots[0] = 1;
            buttons[0] = 21;
            slots[1] = 2;
            buttons[1] = 22;
            break;
        case 2:
            slots[0] = 3;
            buttons[0] = 26;
            n = 1;
            break;
        case 3:
            slots[0] = 4;
            buttons[0] = 20;
            n = 1;
            break;
        case 4:
        case 9:
            slots[0] = 5;
            buttons[0] = 24;
            n = 1;
            break;
        case 5:
            slots[0] = 6;
            buttons[0] = 25;
            n = 1;
            break;
        case 6:
        case 8:
            slots[0] = 7;
            if (status_character->equipment[7] && INV_EQUIP(status_character, 7)->type->slot == 8)
                buttons[0] = 27;
            else
                buttons[0] = 18;
            n = 1;
            break;
        case 7:
            slots[0] = 8;
            if (status_character->equipment[7] && INV_EQUIP(status_character, 7)->type->slot == 8)
                buttons[0] = 27;
            else
                buttons[0] = 19;
            n = 1;
            break;
        default:
            inventory_mouse_mode = 0;
            break;
        }
        for (i = 0; i < n; i++) {
            ui_button_state *b = &inventory_buttons[buttons[i]];
            if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h)) {
                b->flags |= 2;
                break;
            }
        }
    }

    if (!mouse_is_down(0)) {
        inventory_mouse_mode = 0;
        item_page_out_all(INVENTORY_PAGE_SLOT);
        if (i < n) {
            inventory_equip_item(slots[i], entry);
            game_play_sound(0x28, 1.0f, 0, 0, 0);
        }
    }
}

// 0x4174a0  Confidence: likely (VC6 reuses the two parameter slots for old1 / old2). A stack gives one new
// item; a single item moves from the list. What the slot held (and a conflicting slot) goes back to the list.
// A weapon (ItemType kind 0) reloads the statscreen animation.
void inventory_equip_item(int slot, party_inventory_entry *entry)
{
    item_info *old1, *old2;
    bool remove = false;

    if (slot == -1)
        return;
    if (items_same_type(INV_EQUIP(status_character, slot), entry->item))
        return;

    if (entry->count > 1) {
        character_equip_item(&old1, &old2, status_character, slot, item_create_by_name(entry->item->type->name));
        inventory_page_in_equipped();
        entry->count--;
    } else {
        character_equip_item(&old1, &old2, status_character, slot, entry->item);
        inventory_page_in_equipped();
        remove = true;
    }
    if (game_flags & 2)
        mp_send_equip_item(status_character, slot, entry->item->type - item_types);
    if (entry->item->type->kind == 0) {
        Character *c = status_character;
        statscreen_refresh_anim(0);
        statscreen_refresh_anim(c);
    }
    if (remove)
        party_inventory_remove(entry - party_inventory, 0, 1);
    if (status_character->prop_tag)
        status_character->prop_tag = status_character->default_prop_tag;
    if (old1)
        party_inventory_add_item(old1, 1, 1);
    if (old2)
        party_inventory_add_item(old2, 1, 1);

    inventory_update_equip_slots();
    inventory_count_rows();
    int pos = min(inventory_scroll_pos, inventory_row_count - inventory_page_size[resolution_index]);
    inventory_scroll_pos = max(pos, 0);
    inventory_update_category_buttons();
    inventory_update_scrollbar();
}

// 0x417630  Confidence: exact. Mouse mode 5: a ring dropped on the other ring slot swaps the rings; anything
// released outside its own hot spot is unequipped into the list.
void inventory_drag_from_equip()
{
    ui_button_state *b = &inventory_buttons[inventory_pressed_button];

    b->flags |= 2;
    if (inventory_pressed_button >= 21 && inventory_pressed_button <= 22) {
        ui_button_state *other = inventory_pressed_button == 21 ? &inventory_buttons[22] : &inventory_buttons[21];

        if (point_in_rect(game_mouse_x, game_mouse_y, other->x, other->y, other->w, other->h)) {
            other->flags |= 2;
            if (!mouse_is_down(0)) {
                inventory_mouse_mode = 0;
                inventory_swap_rings();
                game_play_sound(0x28, 1.0f, 0, 0, 0);
            }
            return;
        }
    }
    if (!mouse_is_down(0)) {
        inventory_mouse_mode = 0;
        if (!point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h)) {
            int slot = inventory_equip_index_for_button(inventory_pressed_button);
            if (slot != -1)
                inventory_unequip_item(slot);
        }
    }
}

// 0x417740  Confidence: exact.
void inventory_unequip_item(int slot)
{
    party_inventory_add_item(character_unequip_slot(status_character, slot, 1), 1, 1);
    inventory_update_equip_slots();
    inventory_count_rows();
    inventory_update_category_buttons();
    inventory_update_scrollbar();
    if (game_flags & 2)
        mp_send_unequip_item(status_character, slot);
}

// 0x417790  Confidence: likely (the final "ring1 || ring2" test is jump-threaded into the ring 1 branch).
// Despite the name: swaps the two rings (equipment 1 and 2); anything character_equip_item hands back is freed.
void inventory_swap_rings()
{
    item_info *old1, *old2;
    item_info *ring1 = character_unequip_slot(status_character, 1, 1);
    item_info *ring2 = character_unequip_slot(status_character, 2, 1);

    if (ring2) {
        character_equip_item(&old1, &old2, status_character, 1, ring2);
        if (old1)
            item_free(old1);
        if (old2)
            item_free(old2);
    }
    if (ring1) {
        character_equip_item(&old1, &old2, status_character, 2, ring1);
        if (old1)
            item_free(old1);
        if (old2)
            item_free(old2);
    }
    if (ring1 || ring2)
        inventory_update_equip_slots();
}

// 0x417860  Confidence: exact. Screen 1 draw: buttons, list, scrollbar, party gold, equipped item models,
// character model, then the popup or the drag preview of the mouse mode, the item info popup.
void inventory_draw()
{
    char gold[16];

    gr_set_color(255, 255, 255, 255);
    for (ui_button_state *b = inventory_buttons; b < inventory_buttons + INVENTORY_NUM_BUTTONS; b++) {
        if (b->flags & 3)
            ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
    }
    inventory_draw_list_rows();
    inventory_draw_scrollbar();

    sprintf(gold, "%d", party_gold);
    gr_set_color_ptr(&ui_color_gold);
    gr_string(inventory_gold_pos[resolution_index][0], inventory_gold_pos[resolution_index][1], gold, -1,
              gr_string_mode);
    inventory_draw_item_model();
    inventory_draw_character();

    switch (inventory_mouse_mode) {
    case 3:
        ui_choice_popup_draw();
        break;
    case 4:
        inventory_draw_drag_item();
        break;
    case 5:
        inventory_draw_equipped_hover();
        break;
    }
    if (item_info_popup_is_active())
        item_info_popup_render();
}

// 0x417950  Confidence: likely (local layout: one 64-byte buffer for the name and the count, as in
// buysell_draw_item_list). Despite the name: all visible rows, name left and stack count right-aligned.
void inventory_draw_list_rows()
{
    if (inventory_row_count <= 0)
        return;

    int x = inventory_list_pos[resolution_index][0];
    int y = inventory_list_pos[resolution_index][1];
    int rows = inventory_row_count;
    if (rows >= inventory_page_size[resolution_index])
        rows = inventory_page_size[resolution_index];
    int fh = gr_get_font_height(-1);

    for (int i = 0; i < rows; i++) {
        party_inventory_entry *entry = inventory_get_list_item(inventory_scroll_pos + i);
        char buf[64];
        int w, h;

        inventory_set_row_color(i);
        item_get_display_name(buf, entry->item->type);
        gr_string(x, y, buf, -1, gr_string_mode);

        sprintf(buf, "%d", entry->count);
        gr_get_string_size(&w, &h, buf, -1, -1);
        gr_string(inventory_list_width[resolution_index] - w + x, y, buf, -1, gr_string_mode);
        y += fh;
    }
}

// 0x417a60  Confidence: likely (the category test is read as a small switch: < 0, <= 1, == 3). Selected row
// highlighted, unidentified blue, weapons / armour / jewelry the character cannot equip red, else text.
void inventory_set_row_color(int row)
{
    int index = inventory_scroll_pos + row;

    if (row == inventory_selected_row) {
        gr_set_color_ptr(&ui_color_highlight);
        return;
    }
    party_inventory_entry *entry = inventory_get_list_item(index);
    if (!item_is_identified(entry->item)) {
        gr_set_color_ptr(&ui_color_blue);
        return;
    }
    switch (item_type_shop_category(entry->item->type - item_types)) {
    case 0:
    case 1:
    case 3:
        if (!character_can_equip_item(status_character, entry->item)) {
            gr_set_color_ptr(&ui_color_red);
            return;
        }
        break;
    }
    gr_set_color_ptr(&ui_color_text);
}

// 0x417b10  Confidence: exact (same as buysell / journal: whole tiles, then one tile flush with the end).
void inventory_draw_scrollbar()
{
    if (!inventory_scrollbar_visible)
        return;
    gr_set_color(255, 255, 255, 255);
    int n = inventory_scrollbar_h / inventory_scrollbar_bm_h;
    for (int i = 0; i < n; i++)
        ui_bitmap_scaled(inventory_scrollbar_bm, inventory_scrollbar_x,
                         inventory_scrollbar_y + inventory_scrollbar_bm_h * i, gr_bitmap_wrap_mode);
    ui_bitmap_scaled(inventory_scrollbar_bm, inventory_scrollbar_x,
                     inventory_scrollbar_y - inventory_scrollbar_bm_h + inventory_scrollbar_h, gr_bitmap_wrap_mode);
}

// 0x417bc0  Confidence: likely (status_character->equipment[i] is re-read for the view offset, so the item
// is not a local; the mesh is). Each equipped item with a paged mesh in its paper-doll slot.
void inventory_draw_item_model()
{
    vector3 pos;
    matrix3 orient;

    for (int i = 0; i < 9; i++) {
        if (status_character->equipment[i]) {
            ItemType *type = INV_EQUIP(status_character, i)->type;
            void *mesh = type->mesh;
            if (mesh) {
                new_icam_get(&inventory_cameras, type->name, &orient, &pos);
                inventory_model_view_begin(i);
                inventory_item_view_offset(INV_EQUIP(status_character, i)->type);
                ui_3d_begin(&pos);
                ui_render_item_mesh(mesh, &pos, &orient);
                ui_3d_end();
                inventory_model_view_end();
            }
        }
    }
}

// 0x417c50  Confidence: likely (local order, as buysell_render_item_mesh, which also tests the mesh).
// Viewer at (0, 0, -3) with an identity orientation, fov 60. Also used by the item info popup.
void ui_render_item_mesh(void *mesh, vector3 *pos, matrix3 *orient)
{
    vector3 view_pos;
    gr_light_desc light;
    matrix3 view_orient;

    view_pos.x = 0.0f;
    view_pos.y = 0.0f;
    view_pos.z = -3.0f;
    MATRIX3_M(&view_orient)->make_identity();
    gr_setup_3d(&view_orient, &view_pos, 60.0f, 1, 1);
    ca_render_set_mesh(mesh);
    light.f14 = 0.15f;
    ca_render_mesh(pos, orient, &light);
    ca_render_set_mesh(0);
}

// 0x417cd0  Confidence: exact (inline strcmp). Per-item correction of the model position, plus a fixed one
// for heavy armour (ItemType slot 8).
void inventory_item_view_offset(ItemType *type)
{
    for (int i = 0; i < NUM_VIEW_OFFSETS; i++) {
        if (!strcmp(inventory_view_offsets[i].name, type->name)) {
            gr_screen.offset_x += inventory_view_offsets[i].offset[resolution_index][0];
            gr_screen.offset_y += inventory_view_offsets[i].offset[resolution_index][1];
            break;
        }
    }
    if (type->slot == 8) {
        gr_screen.offset_x += inventory_heavy_armor_offset[resolution_index][0];
        gr_screen.offset_y += inventory_heavy_armor_offset[resolution_index][1];
    }
}

// 0x417da0  Confidence: exact. Shifts the clip origin by the paper-doll offset of the equipment index.
void inventory_model_view_begin(int slot)
{
    inventory_saved_view_x = gr_screen.offset_x;
    inventory_saved_view_y = gr_screen.offset_y;
    gr_screen.offset_x = inventory_saved_view_x + inventory_model_view_offset[resolution_index][slot][0];
    gr_screen.offset_y = inventory_saved_view_y + inventory_model_view_offset[resolution_index][slot][1];
}

// 0x417df0  Confidence: exact.
void inventory_model_view_end()
{
    gr_screen.offset_x = inventory_saved_view_x;
    gr_screen.offset_y = inventory_saved_view_y;
}

// 0x417e10  Confidence: likely (local order). The character (animated in single player), then the weapon at
// char_info prop bone [0] and the shield at bone [15] (else [1]); not in the servant disguise on level 0x18.
void inventory_draw_character()
{
    vector3 bone_pos;
    vector3 cam_pos;
    gr_light_desc light;
    matrix3 cam_orient;
    matrix3 bone_orient;

    if (game_flags & 1)
        ((character_instance *)status_character->instance)->process(reported_frametime, 0, 0);
    new_icam_get(&inventory_cameras, status_character->info->name, &cam_orient, &cam_pos);
    ui_3d_begin(&cam_pos);
    inventory_render_character(status_character, &cam_pos, &cam_orient);
    ui_3d_end();

    if (cur_level_index == 0x18 && character_wearing_servant_disguise(status_character))
        return;

    if (status_character->equipment[5]) {
        int bone = status_character->info->_unk168[0];
        if (bone >= 0) {
            CHARINST_M(status_character)->get_bone_world_transform(bone, &cam_orient, &cam_pos, &bone_orient,
                                                                   &bone_pos);
            ui_3d_begin(&bone_pos);
            ca_render_set_mesh(INV_EQUIP(status_character, 5)->type->mesh);
            ca_render_mesh(&bone_pos, &bone_orient, &light);
            ca_render_set_mesh(0);
            ui_3d_end();
        }
    }
    if (status_character->equipment[6]) {
        int bone = status_character->info->_unk168[15] >= 0 ? status_character->info->_unk168[15]
                                                            : status_character->info->_unk168[1];
        if (bone >= 0) {
            CHARINST_M(status_character)->get_bone_world_transform(bone, &cam_orient, &cam_pos, &bone_orient,
                                                                   &bone_pos);
            ui_3d_begin(&bone_pos);
            ca_render_set_mesh(INV_EQUIP(status_character, 6)->type->mesh);
            ca_render_mesh(&bone_pos, &bone_orient, &light);
            ca_render_set_mesh(0);
            ui_3d_end();
        }
    }
}

// 0x417fe0  Confidence: likely (local order; the appearance test reads status_character, the model prefix
// the parameter). Same set-up as status_draw_model, light +0x14 = 0.1.
void inventory_render_character(Character *c, vector3 *pos, matrix3 *orient)
{
    character_instance *inst = (character_instance *)c->instance;
    gr_light_desc light;
    vector3 view_pos;
    matrix3 view_orient;

    view_pos.x = 0.0f;
    view_pos.y = 0.0f;
    view_pos.z = -3.0f;
    MATRIX3_M(&view_orient)->make_identity();
    gr_setup_3d(&view_orient, &view_pos, 60.0f, 1, 1);
    if (status_character->info->flags & 0x10000)
        light.appearance = charinfo_get_multi_appearance(status_character->info);
    light.f14 = 0.1f;
    light.model_prefix = CHAR_M(c)->hero_model_prefix();
    inst->render(pos, orient, &light);
}

// 0x418090  Confidence: exact. Mouse mode 4: the dragged list item's model follows the mouse once it leaves
// the item's name.
void inventory_draw_drag_item()
{
    party_inventory_entry *entry = inventory_get_list_item(inventory_scroll_pos + inventory_selected_row);

    if (!entry || !entry->item->type->mesh)
        return;

    int x = inventory_list_pos[resolution_index][0];
    int y = gr_get_font_height(-1) * inventory_selected_row + inventory_list_pos[resolution_index][1];
    int w, h;

    gr_get_string_size(&w, &h, localize_item_name(entry->item->type->name), -1, -1);
    if (!point_in_rect(game_mouse_x, game_mouse_y, x, y, w, h))
        inventory_draw_item_preview(entry->item);
}

// 0x418160  Confidence: exact. The item's model under the mouse, offset as in its paper-doll slot.
void inventory_draw_item_preview(item_info *item)
{
    vector3 pos;
    matrix3 orient;

    new_icam_get(&inventory_cameras, item->type->name, &orient, &pos);
    inventory_preview_view_begin(item->type->slot);
    inventory_item_view_offset(item->type);
    ui_3d_begin(&pos);
    ui_render_item_mesh(item->type->mesh, &pos, &orient);
    ui_3d_end();
    inventory_model_view_end();
}

// 0x4181d0  Confidence: likely (jump table 0x418408 on ItemType slot 0..9; cases 6 and 8 share their tail).
// Saves the clip origin and moves it so that the model drawn for the item's paper-doll hot spot lands on the
// mouse: minus the hot spot centre, plus the slot's model offset and the mouse position.
void inventory_preview_view_begin(int item_slot)
{
    int x = gr_screen.offset_x;
    int y = gr_screen.offset_y;
    int index;

    inventory_saved_view_x = x;
    inventory_saved_view_y = y;
    switch (item_slot) {
    case 0:
        x += -inventory_buttons[23].x - inventory_buttons[23].w / 2;
        y += -inventory_buttons[23].y - inventory_buttons[23].h / 2;
        index = 0;
        break;
    case 1:
        x += -inventory_buttons[21].x - inventory_buttons[21].w / 2;
        y += -inventory_buttons[21].y - inventory_buttons[21].h / 2;
        index = 1;
        break;
    case 2:
        x += -inventory_buttons[26].x - inventory_buttons[26].w / 2;
        y += -inventory_buttons[26].y - inventory_buttons[26].h / 2;
        index = 3;
        break;
    case 3:
        x += -inventory_buttons[20].x - inventory_buttons[20].w / 2;
        y += -inventory_buttons[20].y - inventory_buttons[20].h / 2;
        index = 4;
        break;
    case 4:
    case 9:
        x += -inventory_buttons[24].x - inventory_buttons[24].w / 2;
        y += -inventory_buttons[24].y - inventory_buttons[24].h / 2;
        index = 5;
        break;
    case 5:
        x += -inventory_buttons[25].x - inventory_buttons[25].w / 2;
        y += -inventory_buttons[25].y - inventory_buttons[25].h / 2;
        index = 6;
        break;
    case 6:
        x += -inventory_buttons[18].x - inventory_buttons[18].w / 2;
        y += -inventory_buttons[18].y - inventory_buttons[18].h / 2;
        index = 7;
        break;
    case 7:
        x += -inventory_buttons[19].x - inventory_buttons[19].w / 2;
        y += -inventory_buttons[19].y - inventory_buttons[19].h / 2;
        index = 8;
        break;
    case 8:
        x += -inventory_buttons[27].x - inventory_buttons[27].w / 2;
        y += -inventory_buttons[27].y - inventory_buttons[27].h / 2;
        index = 7;
        break;
    default:
        return;
    }
    gr_screen.offset_x = x + (inventory_model_view_offset[resolution_index][index][0] + game_mouse_x);
    gr_screen.offset_y = y + (inventory_model_view_offset[resolution_index][index][1] + game_mouse_y);
}

// 0x418430  Confidence: exact. Mouse mode 5: the dragged equipped item's model once the mouse leaves its
// hot spot.
void inventory_draw_equipped_hover()
{
    int slot = inventory_equip_index_for_button(inventory_pressed_button);

    if (slot == -1)
        return;
    item_info *item = INV_EQUIP(status_character, slot);
    if (!item || !item->type->mesh)
        return;
    ui_button_state *b = &inventory_buttons[inventory_pressed_button];
    if (!point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
        inventory_draw_item_preview(item);
}

// 0x4184b0  Confidence: exact (screen_refresh_table 0x58bdc8 [1]; the last call is a tail jump).
void inventory_refresh_for_character()
{
    inventory_update_species_tab();
    inventory_page_in_equipped();
    inventory_update_equip_slots();
}

// 0x4184c0  Confidence: exact. world_mouse_frame: a targeted list item used on an object (obj +0x80 handle).
void world_use_item_on_object(party_inventory_entry *entry, Character *obj)
{
    ui_clear_pick_overrides();
    hud_target_mode = -1;
    character_use_inventory_item(cur_character, entry, obj->handle, 0, -1);
    world_pick_clear_filter();
}

// 0x418500  Confidence: exact. The same through an equipped item.
void world_use_equipped_item_on_object(int slot, Character *obj)
{
    ui_clear_pick_overrides();
    hud_target_mode = -1;
    character_use_equipped_item(cur_character, slot, obj->handle, 0, -1);
    world_pick_clear_filter();
}

// 0x418540  Confidence: exact (hud_target_mode is stored between the pushes, as the compiler schedules it).
// A targeted list item used at a point.
void world_use_item_at_point(party_inventory_entry *entry, void *point, int arg)
{
    ui_clear_pick_overrides();
    hud_target_mode = -1;
    character_use_inventory_item(cur_character, entry, -1, point, arg);
    world_pick_clear_filter();
}
