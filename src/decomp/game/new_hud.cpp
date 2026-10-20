// new_hud.cpp - Sum.exe v1.40, code 0x40cb60-0x41075a (screen 0, the in-game HUD: the bar with the party
// portraits, health/AP bars, panel icons and the six hot buttons; the keyboard map hud_handle_keys and the
// hud_key_* actions; the level-exit, solo-mode and quit prompts; interact-nearest (X / middle mouse); quick
// save / load; the camera-mode overlay; the death and game-over handling; the multiplayer loading panel and
// the icon dragged from the skills/spells panels onto a hot button).
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .rdata 0x57ee00-0x57ef0f (layout tables and two const name arrays), .data 0x58ac88-0x58bcbb (button
// table, species table, bitmap names, strings), .bss 0x5bca58-0x5bd0cf. hud_target_mode (0x58bdb8),
// lc_entries / lc_entry_count (0x248ce40 / 0x2491a68) lie outside these ranges and are extern (see NOTES).
//
// hud_mouse_mode (0x5bd0b4) selects what hud_frame runs: 0 keys and hover (hud_handle_keys), 1 a panel button
// held, 2 a portrait right-clicked, 3 a hot button held, 4 an icon dragged from the skills/spells panel,
// 5 waiting for the death animation, 6 the "You have died." popup.

#include <string.h>
#include <stdio.h>
#include <math.h>
#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/key.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/frametime.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/vfile/file.h"
#include "vsdk/localize/localize.h"
#include "vsdk/ca/character_instance.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "engine/gamesound/gamesound.h"
#include "engine/ai/ai.h"
#include "engine/camera/camera.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/objects/living_entity.h"
#include "engine/objects/objects.h"
#include "engine/rendering/levelrender.h"
#include "engine/spells/spells.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"
#include "summoner/interface/new_playmenu.h"
#include "summoner/interface/new_saveload.h"
#include "summoner/interface/new_skill.h"
#include "summoner/interface/new_spell.h"
#include "summoner/interface/new_tutorial.h"
#include "summoner/interface/new_hud.h"

#define HUD_NUM_BUTTONS   62
#define HUD_NUM_PORTRAITS 5
#define HUD_NUM_HOT       6

// hud_buttons entries the code addresses directly (indices of hud_button_defs).
#define HUD_BTN_PORTRAIT_BKG    1   // 1-5 "Main-HUD-PortraitBkg" (tooltips; health bars hang off them)
#define HUD_BTN_PORTRAIT        6   // 6-10 portrait bitmaps (no name: hud_update_portraits loads them)
#define HUD_BTN_PORT_SELECTED  11   // 11-15
#define HUD_BTN_LEVELUP        16   // 16-19 (only four)
#define HUD_BTN_PORT_HILITE    20   // 20, 22, .. 28 HiLite, the Press entry follows
#define HUD_BTN_PAUSE          30   // "Main-HUD-PauseA": the animated pause icons are drawn on it
#define HUD_BTN_PAUSE_HILITE   31
#define HUD_BTN_SOLO_ACTIVE    37
#define HUD_BTN_HOTBAR         40   // "Main-HUD-HotBtnBkg" (sum.symbols hud_hot_buttons_x/_y/_w/_h = its x, y, w, h)
#define HUD_BTN_NONCOMBAT      41   // 41-43 non-combat icon (A, B hilite, C)
#define HUD_BTN_COMBAT         44   // 44-46 combat icon

// One portrait record (0x10 bytes). The state picks the portrait colour (hud_portrait_get_color): 0 normal,
// 1 hold (solo mode), 2 hit flash (250 ms), 3 fighting, 4 HP at or below 20%, 5 dead.
struct hud_portrait_slot {
    Character *character;             // +0x00
    int state;                        // +0x04
    GameTimer timer;                  // +0x08
    int species;                      // +0x0c char_info species 0..3, 4 for the rest; selects the hot button set
};

// hud_drag_flags (0x5bd0a8, sum.symbols: int). The code reads and writes it as bit fields: hud_drag_icon_begin
// merges all of them into one store, hud_hot_buttons_drag tests (flags & 0xe) == 0xc and shifts by 5.
struct hud_drag_state {
    unsigned int active : 1;          // bit 0 an icon is dragged (cleared while the mouse is over its source)
    unsigned int slot : 3;            // bits 1-3 source hot button, 6 = none (only hud_drag_icon_begin writes it)
    unsigned int is_skill : 1;        // bit 4 skill (skills_icon_bms) or spell (spells_icon_bms)
    unsigned int id : 27;             // bits 5- skill or spell id
};

// ---- .rdata (0x57ee00) ---------------------------------------------------------------------------------
// Per resolution_index (640, 800, 1024); all names *new*.
const int hud_hot_button_size[3][2]    = { { 32, 32 }, { 40, 40 }, { 32, 32 } };        // 0x57ee00 w, h
const int hud_camera_mode_pos[3][2]    = { { 570, 5 }, { 713, 6 }, { 912, 8 } };        // 0x57ee18 overlay x, y
const int hud_camera_icon_width[3]     = { 18, 23, 29 };                                // 0x57ee30
const char hud_portrait_prefix[]       = "hud-charportrait";                            // 0x57ee3c
const int hud_hp_bar_pos[3][2]         = { { 6, 50 }, { 7, 61 }, { 9, 80 } };           // 0x57ee50 from the portrait bkg
const int hud_bar_size[3][2]           = { { 42, 3 }, { 54, 5 }, { 70, 6 } };           // 0x57ee68 full length, height
const char hud_pause_name[]            = "pause.vbm";                                   // 0x57ee80
const int hud_pause_icon_pos[3][2]     = { { 11, 17 }, { 14, 21 }, { 16, 28 } };        // 0x57ee8c from the pause button
const int hud_flash_paused_pos[3][2]   = { { 11, 3 }, { 14, 4 }, { 16, 6 } };           // 0x57eea4
const int hud_hot_button_origin[3][2]  = { { 514, 400 }, { 643, 500 }, { 887, 680 } };  // 0x57eebc slot 0
const int hud_hot_button_spacing[3][2] = { { 40, 40 }, { 50, 50 }, { 43, 43 } };        // 0x57eed4 dx, dy (second row)
const int mp_loading_panel_width[3]    = { 300, 375, 480 };                             // 0x57eeec
const int mp_loading_name_width[3]     = { 150, 188, 240 };                             // 0x57eef8
const int hud_death_popup_y[3]         = { 140, 175, 224 };                             // 0x57ef04
// 0x57ef10-0x57ef34 are the float literals 0.2, 15.0, 5.0, 3.0, 204.0, 0.001, 1000.0, 128.0, 1.02 and 0.002
// (__real COMDATs, in order of first use in this file); 0.5 (0x57e6b0), 1.0 (0x57e990), 255.0 (0x57eb80) and
// 0.0 (0x57eb84) come from earlier units.

// ---- .data (0x58ac88) ----------------------------------------------------------------------------------
// 62 x 0x40 (*new* name). flags: 1 reacts to the left button (hud_mouse_mode 1, the action is the next
// entry), 2 to the right button (mode 2, portraits), 4 drawn in the portrait colour, 8 pulsing alpha (level
// up), 0x10 hidden in multiplayer, 0x20 hidden on level 4. Names with a '.' are loaded as is and not scaled.
ui_button_def hud_button_defs[HUD_NUM_BUTTONS] = {
    { "Main-HUD-MPH",            { { 0, 416 }, { 0, 520 }, { 0, 664 } }, 0 },                        // 0
    { "Main-HUD-PortraitBkg",    { { 50, 416 }, { 63, 520 }, { 80, 664 } }, 0 },                     // 1
    { "Main-HUD-PortraitBkg",    { { 101, 416 }, { 127, 520 }, { 162, 664 } }, 0 },                  // 2
    { "Main-HUD-PortraitBkg",    { { 152, 416 }, { 191, 520 }, { 244, 664 } }, 0 },                  // 3
    { "Main-HUD-PortraitBkg",    { { 203, 416 }, { 255, 520 }, { 326, 664 } }, 0 },                  // 4
    { "Main-HUD-PortraitBkg",    { { 254, 416 }, { 319, 520 }, { 408, 664 } }, 0 },                  // 5
    { "",                        { { 55, 422 }, { 69, 528 }, { 88, 675 } }, 4 },                     // 6
    { "",                        { { 106, 422 }, { 133, 528 }, { 170, 675 } }, 4 },                  // 7
    { "",                        { { 157, 422 }, { 196, 528 }, { 252, 675 } }, 4 },                  // 8
    { "",                        { { 208, 422 }, { 260, 528 }, { 334, 675 } }, 4 },                  // 9
    { "",                        { { 259, 422 }, { 324, 528 }, { 416, 675 } }, 4 },                  // 10
    { "Main-HUD-PortSelected",   { { 54, 421 }, { 68, 526 }, { 86, 674 } }, 0 },                     // 11
    { "Main-HUD-PortSelected",   { { 105, 421 }, { 131, 526 }, { 168, 674 } }, 0 },                  // 12
    { "Main-HUD-PortSelected",   { { 156, 421 }, { 195, 526 }, { 250, 674 } }, 0 },                  // 13
    { "Main-HUD-PortSelected",   { { 207, 421 }, { 259, 526 }, { 332, 674 } }, 0 },                  // 14
    { "Main-HUD-PortSelected",   { { 258, 421 }, { 323, 526 }, { 414, 674 } }, 0 },                  // 15
    { "Main-HUD-levelupicon",    { { 84, 423 }, { 105, 529 }, { 143, 678 } }, 8 },                   // 16
    { "Main-HUD-levelupicon",    { { 135, 423 }, { 169, 529 }, { 225, 678 } }, 8 },                  // 17
    { "Main-HUD-levelupicon",    { { 186, 423 }, { 233, 529 }, { 307, 678 } }, 8 },                  // 18
    { "Main-HUD-levelupicon",    { { 237, 423 }, { 296, 529 }, { 389, 678 } }, 8 },                  // 19
    { "Main-HUD-PortHiLite",     { { 55, 422 }, { 69, 528 }, { 88, 675 } }, 3 },                     // 20
    { "Main-HUD-PortPress",      { { 55, 422 }, { 69, 528 }, { 88, 675 } }, 0 },                     // 21
    { "Main-HUD-PortHiLite",     { { 106, 422 }, { 133, 528 }, { 170, 675 } }, 3 },                  // 22
    { "Main-HUD-PortPress",      { { 106, 422 }, { 133, 528 }, { 170, 675 } }, 0 },                  // 23
    { "Main-HUD-PortHiLite",     { { 157, 422 }, { 196, 528 }, { 252, 675 } }, 3 },                  // 24
    { "Main-HUD-PortPress",      { { 157, 422 }, { 196, 528 }, { 252, 675 } }, 0 },                  // 25
    { "Main-HUD-PortHiLite",     { { 208, 422 }, { 260, 528 }, { 334, 675 } }, 3 },                  // 26
    { "Main-HUD-PortPress",      { { 208, 422 }, { 260, 528 }, { 334, 675 } }, 0 },                  // 27
    { "Main-HUD-PortHiLite",     { { 259, 422 }, { 324, 528 }, { 416, 675 } }, 3 },                  // 28
    { "Main-HUD-PortPress",      { { 259, 422 }, { 324, 528 }, { 416, 675 } }, 0 },                  // 29
    { "Main-HUD-PauseA",         { { 305, 416 }, { 381, 520 }, { 490, 664 } }, 0 },                  // 30
    { "Main-HUD-PauseHiLite",    { { 316, 433 }, { 395, 541 }, { 506, 692 } }, 1 },                  // 31
    { "Main-HUD-PausePress",     { { 316, 433 }, { 395, 541 }, { 506, 692 } }, 0 },                  // 32
    { "Main-HUD-BttnHiLite",     { { 8, 419 }, { 10, 524 }, { 13, 670 } }, 1 },                      // 33
    { "Main-HUD-BttnPress",      { { 8, 419 }, { 10, 524 }, { 13, 670 } }, 0 },                      // 34
    { "Main-HUD-BttnHiLite",     { { 8, 439 }, { 10, 549 }, { 13, 702 } }, 1 },                      // 35
    { "Main-HUD-BttnPress",      { { 8, 439 }, { 10, 549 }, { 13, 702 } }, 0 },                      // 36
    { "SoloActive",              { { 8, 459 }, { 10, 574 }, { 13, 734 } }, 0 },                      // 37
    { "Main-HUD-BttnHiLite",     { { 8, 459 }, { 10, 574 }, { 13, 734 } }, 33 },                     // 38
    { "Main-HUD-BttnPress",      { { 8, 459 }, { 10, 574 }, { 13, 734 } }, 0 },                      // 39
    { "Main-HUD-HotBtnBkg",      { { 360, 388 }, { 450, 485 }, { 713, 664 } }, 0 },                  // 40
    { "HUD-NonCmbtIconA.tga",    { { 371, 400 }, { 464, 500 }, { 728, 680 } }, 48 },                 // 41
    { "HUD-NonCmbtIconB.tga",    { { 371, 400 }, { 464, 500 }, { 728, 680 } }, 49 },                 // 42
    { "HUD-NonCmbtIconC.tga",    { { 371, 400 }, { 464, 500 }, { 728, 680 } }, 0 },                  // 43
    { "HUD-CmbtIconA.tga",       { { 371, 400 }, { 464, 500 }, { 728, 680 } }, 48 },                 // 44
    { "HUD-CmbtIconB.tga",       { { 371, 400 }, { 464, 500 }, { 728, 680 } }, 49 },                 // 45
    { "HUD-CmbtIconC.tga",       { { 371, 400 }, { 464, 500 }, { 728, 680 } }, 0 },                  // 46
    { "HUD-CharIconA.tga",       { { 411, 400 }, { 514, 500 }, { 771, 680 } }, 0 },                  // 47
    { "HUD-CharIconB.tga",       { { 411, 400 }, { 514, 500 }, { 771, 680 } }, 1 },                  // 48
    { "HUD-CharIconC.tga",       { { 411, 400 }, { 514, 500 }, { 771, 680 } }, 0 },                  // 49
    { "Skillicon-Spells2C.tga",  { { 451, 400 }, { 564, 500 }, { 815, 680 } }, 32 },                 // 50
    { "Skillicon-Spells2A.tga",  { { 451, 400 }, { 564, 500 }, { 815, 680 } }, 33 },                 // 51
    { "Skillicon-Spells2B.tga",  { { 451, 400 }, { 564, 500 }, { 815, 680 } }, 0 },                  // 52
    { "HUD-JrnIconA.tga",        { { 371, 440 }, { 464, 550 }, { 728, 723 } }, 16 },                 // 53
    { "HUD-JrnIconB.tga",        { { 371, 440 }, { 464, 550 }, { 728, 723 } }, 17 },                 // 54
    { "HUD-JrnIconC.tga",        { { 371, 440 }, { 464, 550 }, { 728, 723 } }, 0 },                  // 55
    { "HUD-InvIconA.tga",        { { 411, 440 }, { 514, 550 }, { 771, 723 } }, 0 },                  // 56
    { "HUD-InvIconB.tga",        { { 411, 440 }, { 514, 550 }, { 771, 723 } }, 1 },                  // 57
    { "HUD-InvIconC.tga",        { { 411, 440 }, { 514, 550 }, { 771, 723 } }, 0 },                  // 58
    { "Skillicon-SkillsC.tga",   { { 451, 440 }, { 564, 550 }, { 815, 723 } }, 32 },                 // 59
    { "Skillicon-SkillsA.tga",   { { 451, 440 }, { 564, 550 }, { 815, 723 } }, 33 },                 // 60
    { "Skillicon-SkillsB.tga",   { { 451, 440 }, { 564, 550 }, { 815, 723 } }, 0 },                  // 61
};
// char_info species of the 16 portrait bitmaps "hud-charportrait01" .. "16" (*new* name).
int hud_portrait_species[16] = {                                                        // 0x58bc08
    0, 1, 2, 3, 4, 0x90, 0xb0, 0xa4, 0xc0, 0xcb, 0xca, 0xcc, 0xb9, 9, 0xcd, 0xce,
};
// [ui_hires]; the literals follow at 0x58bc50 / 0x58bc64 in reverse order.
char *hud_flash_paused_names[2] = { "FlashPaused640.vbm", "FlashPaused1024.vbm" };      // 0x58bc48
// Then the literals "%s%02d-%s" (0x58bc78), "unknown", "quicksave%s", ".sav", "Quick save", "*.sav" and the
// one-glyph strings "\xb9" (0x58bcb4, camera-mode icon) and "\xba" (0x58bcb8, camera-lock icon) of the HUD font.

// ---- .bss (0x5bca58) -----------------------------------------------------------------------------------
// Defined in address order. The static initialisers run in the order 0x5bd0cc, 0x5bd094, 0x5bd040 (the
// portrait timers), 0x5bd088, 0x5bd090 (static_initializers entries 20-24), which is not the address order
// (see NOTES).
ui_button_state hud_buttons[HUD_NUM_BUTTONS];                 // 0x5bca58
int hud_hot_button_used;                                      // 0x5bd028 slot highlighted as used, -1 none
int hud_pause_bm;                                             // 0x5bd02c
int hud_shown_camera_mode;                                    // 0x5bd030 camera mode shown by the overlay
int hud_hover_button;                                         // 0x5bd034
hud_portrait_slot hud_portrait_slots[HUD_NUM_PORTRAITS];      // 0x5bd038 (init_gametimer_array_5bd040)
GameTimer hud_hot_button_timer;                               // 0x5bd088 (init_gametimer_5bd088)
int hud_flash_paused_bm;                                      // 0x5bd08c
GameTimer hud_death_timer;                                    // 0x5bd090 (init_gametimer_5bd090)
GameTimer hud_camera_forced_timer;                            // 0x5bd094 (init_gametimer_5bd094)
int hud_drag_icon_x;                                          // 0x5bd098
int hud_drag_icon_y;                                          // 0x5bd09c
int hud_drag_icon_w;                                          // 0x5bd0a0
int hud_drag_icon_h;                                          // 0x5bd0a4
hud_drag_state hud_drag_flags;                                // 0x5bd0a8
int hud_hot_button_hover;                                     // 0x5bd0ac
int hud_pause_frames;                                         // 0x5bd0b0
int hud_mouse_mode;                                           // 0x5bd0b4
int hud_flash_paused_frames;                                  // 0x5bd0b8
int hud_portrait_count;                                       // 0x5bd0bc
int hud_selected_portrait;                                    // 0x5bd0c0
float hud_flash_paused_anim_time;                             // 0x5bd0c4
float hud_pause_anim_time;                                    // 0x5bd0c8
GameTimer hud_camera_mode_timer;                              // 0x5bd0cc (init_gametimer_5bd0cc)

// The dynamic initialisers, compiler-generated from the definitions above (no destructors, so no atexit
// helpers):
// 0x40cb60 init_gametimer_5bd0cc_thunk (jmp, static_initializers entry 20). Confidence: exact.
// 0x40cb70 init_gametimer_5bd0cc (hud_camera_mode_timer: mov ecx, 0x5bd0cc / jmp GameTimer::GameTimer). Confidence: exact.
// 0x40cb80 init_gametimer_5bd094_thunk (jmp, entry 21). Confidence: exact.
// 0x40cb90 init_gametimer_5bd094 (hud_camera_forced_timer). Confidence: exact.
// 0x40cba0 init_gametimer_array_5bd040_thunk (jmp, entry 22). Confidence: exact.
// 0x40cbb0 init_gametimer_array_5bd040: the inline implicit constructor of hud_portrait_slot run over the
//          five records (GameTimer() on +8, stride 0x10). Confidence: exact.
// 0x40cbd0 init_gametimer_5bd088_thunk (jmp, entry 23). Confidence: exact.
// 0x40cbe0 init_gametimer_5bd088 (hud_hot_button_timer). Confidence: exact.
// 0x40cbf0 init_gametimer_5bd090_thunk (jmp, entry 24). Confidence: exact.
// 0x40cc00 init_gametimer_5bd090 (hud_death_timer). Confidence: exact.

// The member whose hot buttons are shown (the selected portrait's species). Inline: every use loads
// hud_selected_portrait and reads +0xc of its record. A macro, not an inline function (D7501): build 8966
// evaluates an argument that is a call before the others, and the binary loads the slot argument first.
#define hud_member() (hud_portrait_slots[hud_selected_portrait].species)

// 0x40cc10  Confidence: exact (VC6 8966 byte match with the button and its definition through pointers b and d,
// D15201; status_load_bitmaps shape plus the '.' test; filename is 0x80 bytes, the
// binary's frame 0x8c, D11901; the flag switch is the jump table 0x40cea4 / byte table 0x40ceac over i <= 59).
void hud_load_bitmaps()
{
    float scale = ui_get_scale();
    int i;

    for (i = 0; i < HUD_NUM_BUTTONS; i++) {
        ui_button_state *b = &hud_buttons[i];
        ui_button_def *d = &hud_button_defs[i];
        bool scaled;

        b->flags = 0;
        b->bitmap = -1;
        b->x = 0;
        b->y = 0;
        b->w = 0;
        b->h = 0;
        progress_update(0);

        if (strlen(d->name)) {
            scaled = strchr(d->name, '.') == NULL;
            if (scaled) {
                char filename[128];

                strcpy(filename, d->name);
                strcat(filename, ui_bitmap_suffix_tga[ui_hires]);
                b->bitmap = bm_load(filename, path_interface, -1);
            } else {
                b->bitmap = bm_load(d->name, path_interface, -1);
            }
        } else {
            scaled = false;
            b->bitmap = -1;
        }
        b->x = d->pos[resolution_index][0];
        b->y = d->pos[resolution_index][1];
        if (b->bitmap >= 0) {
            bm_get_dimensions(b->bitmap, &b->w, &b->h);
            if (scaled) {
                b->w = (int)(b->w * scale + 0.5f);
                b->h = (int)(b->h * scale + 0.5f);
            }
        } else {
            b->w = 0;
            b->h = 0;
        }

        // Always drawn: the bar, the portrait backgrounds, the pause and hot button backgrounds and the
        // idle (C) state of the panel icons.
        switch (i) {
        case 0: case 1: case 2: case 3: case 4: case 5:
        case 30: case 40: case 47: case 50: case 53: case 56: case 59:
            b->flags |= 1;
            break;
        }
    }

    progress_update(0);
    hud_pause_bm = bm_load(hud_pause_name, path_interface, -1);
    if (hud_pause_bm >= 0)
        hud_pause_frames = bm_get_num_frames(hud_pause_bm, 0);
    else
        hud_pause_frames = 0;
    progress_update(0);
    hud_flash_paused_bm = bm_load(hud_flash_paused_names[ui_hires], path_interface, -1);
    if (hud_flash_paused_bm >= 0)
        hud_flash_paused_frames = bm_get_num_frames(hud_flash_paused_bm, 0);
    else
        hud_flash_paused_frames = 0;

    hud_shown_camera_mode = -1;
    for (i = 0; i < HUD_NUM_PORTRAITS; i++) {
        hud_portrait_slots[i].character = 0;
        hud_portrait_slots[i].state = 0;
        hud_portrait_slots[i].timer.invalidate();
        hud_portrait_slots[i].species = -1;
    }
    hud_portrait_count = 0;
    hud_selected_portrait = -1;
    hud_pause_anim_time = 0;
    hud_death_timer.invalidate();
}

// 0x40cef0  Confidence: exact.
void hud_init()
{
    hud_drag_icon_end();
    hud_camera_mode_timer.invalidate();
    hud_camera_forced_timer.invalidate();
    hud_mouse_mode = 0;
    hud_drag_flags.active = 0;
    hud_hot_button_used = -1;
    hud_hot_button_timer.invalidate();
    statscreen_refresh_anim(0);
}

// folded (0x4f13e0, empty_function): the screen's leave function is empty; the linker folded it into
// empty_function (identical-code folding). interface_set_screen calls each by name (D7903); name ours.
void hud_leave()
{
}

// 0x40cf40  Confidence: exact (the hot button timer test reads "valid || elapsed": the highlight set by
// hud_handle_keys is cleared on the next frame, not after 100 ms; reproduced as is). In a multiplayer game
// that is still loading only the host's F key does something: it drops every player still at state 4.
int hud_frame(int key)
{
    int i;

    for (i = 0; i < HUD_NUM_BUTTONS; i++)
        hud_buttons[i].flags &= ~2;
    if (hud_hot_button_timer.valid() || hud_hot_button_timer.elapsed()) {
        hud_hot_button_timer.invalidate();
        hud_hot_button_used = -1;
    }
    world_hover_portrait_character = 0;
    world_pick_portrait_clicked = false;
    hud_update_portraits();
    hud_update_anim_time();
    hud_update_combat_buttons();

    if (tutorial_frame(key))
        return key;
    hud_confirm_level_exit();
    i = interface_get_screen();
    if (i == 4 || i == 9 || i == 13)
        return key;

    if (game_flags & 2) {
        if (mp_netgame_state != 5) {
            Player *p, *next;

            if (!mp_local_player || !(mp_local_player->flags & 1) || key != KEY_F)
                return key;
            p = party_list.next;
            next = p->next;
            while (p != &party_list) {
                mp_player *owner = (mp_player *)p->mp_owner;

                if (owner != mp_local_player && MP_PLAYER_STATE(owner) == 4)
                    mp_remove_player(owner, 2);
                p = next;
                next = next->next;
            }
            return 0;
        }
        if (mp_trade_state == 1)
            return key;
    }

    if (ui_message_box_is_active())
        return key;

    switch (hud_mouse_mode) {
    case 0:
        key = hud_handle_keys(key);
        break;
    case 1:
        hud_update_buttons();
        break;
    case 2:
        hud_portraits_handle_click();
        break;
    case 3:
        hud_hot_buttons_handle_click();
        break;
    case 4:
        hud_hot_buttons_drag();
        break;
    case 5:
        key = hud_death_timer_frame(key);
        break;
    case 6:
        key = hud_confirm_quit(key);
        break;
    }

    // An item being used on a target (hud_target_mode 0): the hovered portrait is the target.
    if (hud_target_mode == 0) {
        switch (hud_hover_button) {
        case HUD_BTN_PORT_HILITE:
            world_hover_portrait_character = hud_portrait_slots[0].character;
            break;
        case HUD_BTN_PORT_HILITE + 2:
            world_hover_portrait_character = hud_portrait_slots[1].character;
            break;
        case HUD_BTN_PORT_HILITE + 4:
            world_hover_portrait_character = hud_portrait_slots[2].character;
            break;
        case HUD_BTN_PORT_HILITE + 6:
            world_hover_portrait_character = hud_portrait_slots[3].character;
            break;
        case HUD_BTN_PORT_HILITE + 8:
            world_hover_portrait_character = hud_portrait_slots[4].character;
            break;
        }
    }
    return key;
}

// 0x40d160  Confidence: exact (VC6 8966 byte match with the highlight button and the clearing loop buttons
// through pointers, D15201; p lives in a stack slot; the "not found" end of the species search jumps
// straight to the -1 store, read as a guarded i < 16 test). Portrait states: see hud_portrait_slot.
void hud_update_portraits()
{
    float scale = ui_get_scale();
    Player *p;
    int i;

    hud_portrait_count = 0;
    for (p = party_list.next; p != &party_list; p = p->next) {
        Character *c = living_from_handle(p->handle);

        if (!c)
            continue;
        if (c == cur_character)
            hud_selected_portrait = hud_portrait_count;
        if (c->flags & 0x100) {
            if (AI_HM(c)->is_mode(6, -1))
                continue;
            if (!(c->obj_flags & 1))
                continue;
        }

        if (hud_portrait_slots[hud_portrait_count].character != c) {
            ui_button_state *b = &hud_buttons[HUD_BTN_PORTRAIT + hud_portrait_count];
            int species = c->info->species;

            for (i = 0; i < 16; i++)
                if (hud_portrait_species[i] == species)
                    break;
            if (i < 16) {
                char filename[64];

                sprintf(filename, "%s%02d-%s", hud_portrait_prefix, i + 1, ui_bitmap_suffix_tga[ui_hires]);
                b->bitmap = bm_load(filename, path_interface, -1);
            } else {
                b->bitmap = -1;
            }
            if (b->bitmap >= 0) {
                bm_get_dimensions(b->bitmap, &b->w, &b->h);
                b->w = (int)(b->w * scale + 0.5f);
                b->h = (int)(b->h * scale + 0.5f);
            } else {
                b->w = 0;
                b->h = 0;
            }
            b->flags |= 1;
            hud_portrait_slots[hud_portrait_count].character = c;
            species = c->info->species;
            if (species >= 0 && species <= 3)
                hud_portrait_slots[hud_portrait_count].species = species;
            else
                hud_portrait_slots[hud_portrait_count].species = 4;
        }

        {
            ui_button_state *h = &hud_buttons[HUD_BTN_PORT_HILITE + 2 * hud_portrait_count];

            h->flags &= ~4;
        }
        if (c == cur_character)
            hud_buttons[HUD_BTN_PORT_SELECTED + hud_portrait_count].flags |= 2;
        else
            hud_buttons[HUD_BTN_PORT_SELECTED + hud_portrait_count].flags &= ~2;

        if (hud_portrait_count < 4 && ((game_flags & 1) || character_is_locally_controlled(c))) {
            if (!(c->flags & 0x100) && p->skill_points > 0)
                hud_buttons[HUD_BTN_LEVELUP + hud_portrait_count].flags |= 2;
            else
                hud_buttons[HUD_BTN_LEVELUP + hud_portrait_count].flags &= ~2;
        }

        if (c->hp <= 0.0f)
            hud_portrait_set_state(hud_portrait_count, 5);
        else if (game_flags & 0x100)
            hud_portrait_set_state(hud_portrait_count, 0);
        else if (c->flags & 0x4000)
            hud_portrait_set_state(hud_portrait_count, 2);
        else if (c->hp / CHAR_M(c)->get_max_stat(0) <= 0.2f)
            hud_portrait_set_state(hud_portrait_count, 4);
        else if (!(c->obj_flags & 2) && AI_HM(c)->get_mode() == 3 && !AI_HM(c)->is_combat_e_or_sneak_attack())
            hud_portrait_set_state(hud_portrait_count, 3);
        else if (p->flags & 4)
            hud_portrait_set_state(hud_portrait_count, 1);
        else
            hud_portrait_set_state(hud_portrait_count, 0);
        hud_portrait_count++;
    }

    for (i = hud_portrait_count; i < HUD_NUM_PORTRAITS; i++) {
        ui_button_state *b = &hud_buttons[HUD_BTN_PORTRAIT + i];
        ui_button_state *h = &hud_buttons[HUD_BTN_PORT_HILITE + 2 * i];

        b->flags &= ~1;
        h->flags |= 4;
        hud_portrait_slots[i].character = 0;
    }
}

// 0x40d4d0  Confidence: exact (VC6 byte match) (the default case jumps straight to the timer test when the state is 2,
// read as VC6 threading the "!= 2" test into the common "== 2" test).
void hud_portrait_set_state(int slot, int state)
{
    hud_portrait_slot *s = &hud_portrait_slots[slot];

    switch (state) {
    case 2:
        s->timer.SetTimeout(250);
        s->state = 2;
        break;
    case 5:
        s->state = 5;
        break;
    default:
        if (s->state != 2)
            s->state = state;
        break;
    }
    if (s->state == 2 && (!s->timer.valid() || s->timer.elapsed()))
        s->state = 0;
}

// 0x40d540  Confidence: exact (game_is_paused is called before the frame time is loaded; the product is
// one expression shared by both branches).
void hud_update_anim_time()
{
    if (game_is_paused()) {
        hud_flash_paused_anim_time += reported_frametime * 15.0f;
        if (hud_flash_paused_anim_time >= hud_flash_paused_frames)
            hud_flash_paused_anim_time -= hud_flash_paused_frames;
    } else {
        hud_pause_anim_time += reported_frametime * 15.0f;
        if (hud_pause_anim_time >= hud_pause_frames)
            hud_pause_anim_time -= hud_pause_frames;
        hud_flash_paused_anim_time = 0;
    }
}

// 0x40d5d0  Confidence: exact.
void hud_update_combat_buttons()
{
    if (combat_mode) {
        hud_buttons[HUD_BTN_COMBAT].flags |= 1;
        hud_buttons[HUD_BTN_COMBAT + 1].flags &= ~4;
        hud_buttons[HUD_BTN_NONCOMBAT].flags &= ~1;
        hud_buttons[HUD_BTN_NONCOMBAT + 1].flags |= 4;
    } else {
        hud_buttons[HUD_BTN_NONCOMBAT].flags |= 1;
        hud_buttons[HUD_BTN_NONCOMBAT + 1].flags &= ~4;
        hud_buttons[HUD_BTN_COMBAT].flags &= ~1;
        hud_buttons[HUD_BTN_COMBAT + 1].flags |= 4;
    }
}

// 0x40d640  Confidence: exact (VC6 8966 byte match, m-1020: the multiplayer format string read into a local first,
// D6205 / D11908) (frame: buttons 0x0, text 0x8, name 0x88). On the world map (level 4) the
// prompt names the level; elsewhere "Exit to %s?" or "Exit level?" (multiplayer: through the endgame
// table).
void hud_confirm_level_exit()
{
    char *buttons[2];
    char text[128];
    char name[256];
    int level;

    if (game_flags & 0x40000)
        return;
    if (!level_exit_trigger)
        return;
    if (game_frozen)
        return;
    if ((game_flags & 2) && mp_local_player && !(mp_local_player->flags & 1))
        return;
    if (ui_message_box_is_active())
        return;

    if (level_exit_block_flags & 2) {
        buttons[0] = XSTR("Ok", 213);
        ui_message_box_show(XSTR("Cannot Leave", 647), XSTR("Hostile monsters nearby", 648), -1, -1, 1, buttons,
                            hud_level_exit_step_back_thunk, 1);
        return;
    }
    if (level_exit_block_flags & 1) {
        buttons[0] = XSTR("Ok", 213);
        ui_message_box_show(XSTR("Cannot Leave", 647), XSTR("Party members too far away", 649), -1, -1, 1,
                            buttons, hud_level_exit_step_back_thunk, 1);
        return;
    }

    level = level_script_find(level_exit_trigger);
    if (cur_level_index == 4) {
        if (level < 51) {
            level_format_display_name(name, level);
            sprintf(text, XSTR("Enter %s?", 650), name);
        } else {
            sprintf(text, XSTR("Enter location?", 651));
        }
    } else if (game_flags & 2) {
        int endgame = level_table_find_id(level);

        if (endgame == -1)
            sprintf(text, XSTR("Exit level?", 652));
        else {
            char *fmt = XSTR("Exit to %s?", 653);

            sprintf(text, fmt, level_table_display_name(endgame));
        }
    } else if (level >= 0 && level < 51) {
        level_format_display_name(name, level);
        sprintf(text, XSTR("Exit to %s?", 653), name);
    } else {
        sprintf(text, XSTR("Exit level?", 652));
    }

    buttons[0] = XSTR("Yes", 654);
    buttons[1] = XSTR("No", 655);
    ui_message_box_show(XSTR("New Area", 656), text, -1, -1, 2, buttons, hud_confirm_level_exit_cb, 1);
}

// 0x40d840  Confidence: exact (a tail jump). The OK callback of the two "Cannot Leave" boxes.
void hud_level_exit_step_back_thunk(int button)
{
    hud_level_exit_step_back();
}

// 0x40d850  Confidence: exact (each statement reloads cur_character). Puts the leader back where
// level_check_triggers stored it and stops it.
void hud_level_exit_step_back()
{
    level_exit_trigger = 0;
    cur_character->last_pos = level_exit_return_pos;
    cur_character->pos = level_exit_return_pos;
    CHAR_HM(cur_character)->set_6ac(0);
    CHAR_HM(cur_character)->set_locomotion_speed(0);
    CHARINST_HM(cur_character)->clear_action_anims();
    CHARINST_HM(cur_character)->clear_state_anims();
    CHAR_HM(cur_character)->update_locomotion_anim();
    AI_HM(cur_character)->goals_clear();
    AI_HM(cur_character)->go_idle();
}

// 0x40d910  Confidence: exact. "Yes" freezes the game until the level change; "No" steps back.
void hud_confirm_level_exit_cb(int button)
{
    if (button == 0)
        game_frozen = true;
    else
        hud_level_exit_step_back();
}

// 0x40d930  Confidence: likely (one switch on the key: the jump table 0x40e02c with the byte table 0x40e064
// for 0x01-0x39, the jump table 0x40e0a0 for 0x3c-0x45, compares for 0x3b, 0x58, 0x100f, 0x401f, 0x4026;
// VC6 put id of the number-key case in the dead stack slot of key; the cases are in the binary's body order,
// D15205, so it matches up to the Esc case, where the binary keeps the shared 0x23 sound tail and 8966 the
// quicksave one). The mouse part (hover, clicks starting
// hud_mouse_mode 1-3, tooltips) is skipped while the left button is held (player_input bit 0x10).
// Keys: docs/sum-exe/controls.md. Q, Enter, F3, F4, Alt+S and Alt+L need single player (game_flags 1).
int hud_handle_keys(int key)
{
    int i;

    hud_hover_button = -1;
    hud_hot_button_hover = -1;
    if (game_flags & 0x1000)
        hud_buttons[HUD_BTN_SOLO_ACTIVE].flags |= 2;
    if (hud_drag_flags.active) {
        hud_mouse_mode = 4;
        return key;
    }

    if (!(player_input[0] & 0x10)) {
        for (i = 0; i < HUD_NUM_BUTTONS; i++) {
            ui_button_def *def = &hud_button_defs[i];
            ui_button_state *b = &hud_buttons[i];

            if (cur_level_index == 4 && (def->flags & 0x20))
                continue;
            if ((game_flags & 2) && (def->flags & 0x10))
                continue;
            if (b->flags & 4)
                continue;
            if (!(def->flags & 3))
                continue;
            if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h)) {
                b->flags |= 2;
                hud_hover_button = i;
                if ((def->flags & 1) && mouse_was_pressed(0)) {
                    hud_mouse_mode = 1;
                    return key;
                }
                if ((def->flags & 2) && mouse_was_pressed(1)) {
                    hud_mouse_mode = 2;
                    return key;
                }
                break;
            }
        }

        if (point_in_rect(game_mouse_x, game_mouse_y, hud_buttons[HUD_BTN_HOTBAR].x, hud_buttons[HUD_BTN_HOTBAR].y,
                          hud_buttons[HUD_BTN_HOTBAR].w, hud_buttons[HUD_BTN_HOTBAR].h)) {
            int slot;

            for (slot = 0; slot < HUD_NUM_HOT; slot++) {
                int kind, id;

                if (SAVEGAME_HM->hot_button_get(&kind, &id, hud_member(), slot)) {
                    int x, y;

                    if (slot < 3) {
                        x = hud_hot_button_origin[resolution_index][0] + hud_hot_button_spacing[resolution_index][0] * slot;
                        y = hud_hot_button_origin[resolution_index][1];
                    } else {
                        x = hud_hot_button_origin[resolution_index][0] + hud_hot_button_spacing[resolution_index][0] * (slot - 3);
                        y = hud_hot_button_origin[resolution_index][1] + hud_hot_button_spacing[resolution_index][1];
                    }
                    int w = hud_hot_button_size[resolution_index][0];
                    int h = hud_hot_button_size[resolution_index][1];
                    if (point_in_rect(game_mouse_x, game_mouse_y, x, y, w, h)) {
                        hud_hot_button_hover = slot;
                        if (mouse_was_pressed(0)) {
                            hud_mouse_mode = 3;
                            return key;
                        }
                        break;
                    }
                }
            }
        }

        if (interface_input_ready()) {
            if (hud_hot_button_hover >= 0) {
                hud_draw_tooltips(hud_hot_button_hover, true);
            } else {
                if (hud_hover_button >= 0)
                    hud_draw_tooltips(hud_hover_button, false);
                for (i = 0; i < HUD_NUM_BUTTONS && interface_input_ready(); i++) {
                    ui_button_state *b = &hud_buttons[i];

                    if ((b->flags & 3) && point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
                        hud_draw_tooltips(i, false);
                }
            }
        }
    }

    if (mouse_was_pressed(2)) {
        hud_interact_nearest();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        return key;
    }

    switch (key) {
    case KEY_1: case KEY_1 + 1: case KEY_1 + 2: case KEY_1 + 3: case KEY_1 + 4: case KEY_6: {
        int slot, kind, id;

        if (cur_level_index == 4)
            break;
        slot = key - KEY_1;
        if (!SAVEGAME_HM->hot_button_get(&kind, &id, hud_member(), slot))
            return 0;
        if (kind) {
            if (!skill_is_usable(id, 0))
                return 0;
            if (!skill_is_available(id, 0))
                return 0;
        } else {
            if (!character_can_cast_now(0))
                return 0;
            if (!spell_is_usable(id, 0, 0))
                return 0;
        }
        hud_use_hot_button(slot);
        hud_hot_button_used = slot;
        hud_hot_button_timer.SetTimeout(100);
        return 0;
    }

    case KEY_C:
        if (cur_level_index == 4)
            break;
        hud_key_spells();
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;

    case KEY_H:
        if (cur_level_index == 4)
            break;
        hud_toggle_solo_mode();
        game_play_sound(0x2a, 1.0f, 0, 0, 0);
        return 0;

    case KEY_I:
        hud_key_inventory();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        return 0;

    case KEY_M:
        hud_key_automap();
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;

    case KEY_Q:
        if (!(game_flags & 1))
            break;
        hud_key_journal();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        return 0;

    case KEY_R:
        hud_key_status();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        return 0;

    case KEY_S:
        if (cur_level_index == 4)
            break;
        hud_key_skills();
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;

    case KEY_X:
        hud_interact_nearest();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;

    case KEY_TAB:
        if (cur_level_index == 4)
            break;
        party_cycle(1);
        return 0;

    case KEY_ENTER:
        if (!(game_flags & 1))
            break;
        if (cur_level_index == 4)
            break;
        hud_toggle_combat_mode();
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;

    case KEY_ESC:
        if (hud_target_mode != -1) {
            hud_target_mode = -1;
            return 0;
        }
        hud_key_playmenu();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        return 0;

    case KEY_F1:
        hud_key_help();
        return 0;

    case KEY_SPACE:
    case KEY_NUMLOCK:
        hud_key_pause();
        game_play_sound(0x26, 1.0f, 0, 0, 0);
        return 0;

    case KEY_F2:
        interface_set_screen(12, 0);
        return 0;

    case KEY_F3:
        if (!(game_flags & 1))
            break;
        hud_key_save();
        return 0;

    case KEY_F4:
        if (!(game_flags & 1))
            break;
        open_saveload_screen();
        return 0;

    case KEY_F5:
        hud_select_party_member(0);
        return 0;

    case KEY_F6:
        hud_select_party_member(1);
        return 0;

    case KEY_F7:
        hud_select_party_member(2);
        return 0;

    case KEY_F8:
        hud_select_party_member(3);
        return 0;

    case KEY_F9:
        hud_select_party_member(4);
        return 0;

    case KEY_F12:
        hud_key_camera();
        return 0;

    case KEY_L | KEY_CTRLED:
        if (!(game_flags & 1))
            break;
        quickload();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        return 0;

    case KEY_S | KEY_CTRLED:
        if (!(game_flags & 1))
            break;
        quicksave();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        return 0;

    case KEY_TAB | KEY_SHIFTED:
        if (cur_level_index == 4)
            break;
        party_cycle(0);
        return 0;
    }
    return key;
}

// 0x40e0d0  Confidence: exact (switch through the jump table 0x40e1d8; frame: buttons 0x0, the screen
// argument 0x4, text 0x8, name 0x48). F3: the save/load screen in save mode, or why saving is not allowed.
void hud_key_save()
{
    char *buttons[1];
    bool save_mode[4];
    char text[64];
    char name[128];

    buttons[0] = XSTR("OK", 231);
    switch (savegame_location_allowed()) {
    case 0:
        save_mode[0] = true;
        interface_set_screen(8, save_mode);
        return;
    case 1:
        strcpy(text, XSTR("Cannot save in combat.", 657));
        break;
    case 2:
        level_format_display_name(name, cur_level_index);
        if (!_stricmp(name, "unknown"))
            sprintf(text, XSTR("Cannot save here.", 658));
        else
            sprintf(text, XSTR("Cannot save in %s.", 659), name);
        break;
    case 3:
        strcpy(text, XSTR("Cannot save during summon.", 660));
        break;
    default:
        strcpy(text, XSTR("Cannot save now.", 661));
        break;
    }
    ui_message_box_show(XSTR("Warning", 645), text, -1, -1, 1, buttons, 0, 1);
}

// 0x40e1f0  Confidence: exact. F5-F9 and the portraits; not for dead (0x8a) members, and in multiplayer
// only for the local player's own characters.
void hud_select_party_member(int n)
{
    Character *c;
    Player *p;

    if (n < 0 || n >= hud_portrait_count)
        return;
    c = hud_portrait_slots[n].character;
    if (c->flags & 0x8a)
        return;
    p = player_from_handle(c->handle);
    if (!p)
        return;
    if ((game_flags & 2) && (mp_player *)p->mp_owner != mp_local_player)
        return;
    party_select_member(p);
}

// 0x40e250  Confidence: exact.
void hud_key_playmenu()
{
    interface_set_screen(9, 0);
}

// 0x40e260  Confidence: exact.
void hud_key_help()
{
    interface_set_screen(22, 0);
}

// 0x40e270  Confidence: exact (one request record for both branches). A multiplayer client asks the host.
void hud_key_pause()
{
    ai_service_req req;

    if (game_is_paused()) {
        if ((game_flags & 2) && mp_local_player && !(mp_local_player->flags & 1)) {
            req.type = 1;
            ai_service_request(&req);
            return;
        }
        game_unpause();
    } else {
        if ((game_flags & 2) && mp_local_player && !(mp_local_player->flags & 1)) {
            req.type = 0;
            ai_service_request(&req);
            return;
        }
        game_pause(0, 1);
    }
}

// 0x40e300  Confidence: exact.
void hud_toggle_solo_mode()
{
    char *buttons[2];

    if (config_solo_confirm) {
        buttons[0] = XSTR("Yes", 654);
        buttons[1] = XSTR("No", 655);
        if (game_flags & 0x1000)
            ui_message_box_show(XSTR("Solo mode", 662), XSTR("Leave solo mode?", 663), -1, -1, 2, buttons,
                                hud_key_h_cb, 1);
        else
            ui_message_box_show(XSTR("Solo mode", 662), XSTR("Enter solo mode?", 664), -1, -1, 2, buttons,
                                hud_key_h_cb, 1);
    } else {
        party_toggle_hold();
    }
}

// 0x40e380  Confidence: exact.
void hud_key_h_cb(int button)
{
    if (button == 0)
        party_toggle_hold();
}

// 0x40e390  Confidence: exact.
void hud_key_automap()
{
    if (interface_get_screen() != 3)
        interface_set_screen(3, 0);
    else
        interface_set_screen(0, 0);
}

// 0x40e3c0  Confidence: exact.
void hud_toggle_combat_mode()
{
    combat_mode_set(!combat_mode);
}

// 0x40e3e0  Confidence: exact.
void hud_key_status()
{
    interface_set_screen(2, 0);
}

// 0x40e3f0  Confidence: exact.
void hud_key_spells()
{
    if (interface_get_screen() != 5)
        interface_set_screen(5, 0);
    else
        interface_set_screen(0, 0);
}

// 0x40e420  Confidence: exact.
void hud_key_journal()
{
    interface_set_screen(10, 0);
}

// 0x40e430  Confidence: exact. The argument false: plain inventory (hud_use_hot_button passes true for
// Appraise). The flag is the first byte of a 4-byte block at the bottom of the frame (D7501: a lone bool
// would sit in the top byte of its slot).
void hud_key_inventory()
{
    bool identify[4];

    identify[0] = false;
    interface_set_screen(1, identify);
}

// 0x40e450  Confidence: exact.
void hud_key_skills()
{
    if (interface_get_screen() != 6)
        interface_set_screen(6, 0);
    else
        interface_set_screen(0, 0);
}

// 0x40e480  Confidence: likely (the distance is an inline vector length; the FPU order z, y, x of the squares
// and the stack slots are VC6's). Nearest of: active item (type 3, unless item_obj_is_in_view says so) or
// type 11 objects within 3 units (and 3 in height); in single player also doors in use range and
// containers (lc_entries, not hidden, not empty gold) within 5 units. An object wins over a door or
// container only when strictly closer.
void hud_interact_nearest()
{
    float best_door_dist;
    void *best_door;
    float d;
    float best_obj_dist;
    Character *best_obj;
    vector3 maxs;
    vector3 mins;
    Character *obj;
    int i;

    best_obj = 0;
    best_obj_dist = 999999.0f;
    best_door = 0;
    best_door_dist = 999999.0f;
    for (obj = (Character *)obj_active_list.next; obj != (Character *)&obj_active_list; obj = (Character *)obj->next) {
        float dx, dy, dz;

        if (!(obj->obj_flags & 2))
            continue;
        if (obj->type == 3) {
            if (((item_obj *)obj)->is_in_view())
                continue;
        } else if (obj->type != 11) {
            continue;
        }
        dx = cur_character->pos.x - obj->pos.x;
        dy = cur_character->pos.y - obj->pos.y;
        dz = cur_character->pos.z - obj->pos.z;
        d = (float)sqrt(dx * dx + dy * dy + dz * dz);
        if (d <= 3.0f && d <= best_obj_dist && fabsf(cur_character->pos.y - obj->pos.y) <= 3.0f) {
            best_obj = obj;
            best_obj_dist = d;
        }
    }

    if (game_flags & 1) {
        for (i = 0; i < num_lobjs; i++) {
            lobj *l = &lobjs[i];

            if (l->flags & 0x100) {
                lobj_get_bbox(i, &mins, &maxs);
                if (!gr_cull_bounding_box(&mins, &maxs) && door_in_use_range(cur_character, l)) {
                    float dx = l->centre.x - cur_character->pos.x;
                    float dy = l->centre.y - cur_character->pos.y;
                    float dz = l->centre.z - cur_character->pos.z;
                    float dist = (float)sqrt(dx * dx + dy * dy + dz * dz);

                    if (dist <= best_door_dist) {
                        best_door_dist = dist;
                        best_door = l;
                    }
                }
            }
        }
        for (i = 0; i < lc_entry_count; i++) {
            lobj *l = lc_entries[i].object;

            if (l->flags & 0x80)
                continue;
            if ((lc_entries[i].flags & 1) && lc_entries[i].gold <= 0)
                continue;
            if (!gr_cull_bounding_box(&l->bbox_min, &l->bbox_max)) {
                float dx = l->pos.x - cur_character->pos.x;
                float dy = l->pos.y - cur_character->pos.y;
                float dz = l->pos.z - cur_character->pos.z;
                float dist = (float)sqrt(dx * dx + dy * dy + dz * dz);

                if (dist <= 5.0f && dist <= best_door_dist) {
                    best_door_dist = dist;
                    best_door = l;
                }
            }
        }
    }

    if (best_obj) {
        if (!best_door || best_obj_dist < best_door_dist) {
            world_hover_object = best_obj;
            world_interact_kind = (best_obj->type != 3) + 1;
            return;
        }
        world_interact_object = best_door;
        world_interact_kind = (((lobj *)best_door)->flags & 0x100) ? 3 : 4;
    } else if (best_door) {
        world_interact_object = best_door;
        world_interact_kind = (((lobj *)best_door)->flags & 0x100) ? 3 : 4;
    }
}

// 0x40e740  Confidence: exact (frame: two dwords 0x0/0x4, filename 0x8, level file 0x28, level name 0x68,
// description 0xa8).
void quickload()
{
    float b;                 // the level time (savegame::read_info writes a float, D52)
    int a;
    char filename[32];
    char level_file[64];
    char level_name[64];
    char description[256];

    sprintf(filename, "quicksave%s", ".sav");
    if (SAVEGAME_HM->read_info(filename, level_file, level_name, &a, &b, description, 0)) {
        SAVEGAME_HM->set_pending_load(filename, 0);
        game_change_level(level_file, level_name);
    }
}

// 0x40e7c0  Confidence: exact (frame: buttons 0x0, the list head a whole 0x38-byte node at 0x4, filename[32]
// at 0x3c, message[64] at 0x5c, D10306; the case bodies share the message box call). At most 32 savegames:
// the files are only counted, then freed.
void quicksave()
{
    char *buttons[1];
    file_entry files;
    char filename[32];
    char message[64];
    file_entry *e;
    int count;

    buttons[0] = XSTR("Ok", 213);
    switch (savegame_location_allowed()) {
    case 1:
        ui_message_box_show(XSTR("Quick save", 665), XSTR("Cannot save in combat.", 657), -1, -1, 1, buttons, 0, 1);
        return;
    case 2:
        ui_message_box_show(XSTR("Quick save", 665), XSTR("Cannot save in this area.", 666), -1, -1, 1, buttons,
                            0, 1);
        return;
    case 3:
        ui_message_box_show(XSTR("Quick save", 665), XSTR("Cannot save while summon is emerging.", 667), -1, -1,
                            1, buttons, 0, 1);
        return;
    }

    file_find_files(&files, path_savegame, "*.sav", 0, 0);
    count = 0;
    for (e = files.next; e != (file_entry *)&files; e = e->next)
        count++;
    while (files.next != (file_entry *)&files) {
        files.prev = files.next;
        files.next = files.prev->next;
        delete files.prev;
    }
    files.next = (file_entry *)&files;
    files.prev = (file_entry *)&files;
    if (count >= 32) {
        ui_message_box_show(XSTR("Quick save", 665), XSTR("No more savegame slots.", 668), -1, -1, 1, buttons, 0, 1);
        return;
    }

    sprintf(filename, "quicksave%s", ".sav");
    saveload_capture_thumbnail();
    conversation_db.save_state();
    if (SAVEGAME_HM->write(filename, 0, "Quick save")) {
        sprintf(message, XSTR("Quick save successful.", 669));
        hud_show_message(message);
        return;
    }
    sprintf(message, XSTR("Error during quick save.", 670));
    if (game_flags & 1)
        game_pause(0, 0);
    ui_message_box_show(XSTR("Quick save", 665), message, -1, -1, 1, buttons, 0, 1);
}

// 0x40e980  Confidence: exact. F12; the camera-lock icon shows for 3 s when the high view is forced (mode 3)
// and not suppressed by the detail level.
void hud_key_camera()
{
    CAMERA_HM->cycle_mode();
    if (game_camera.mode == 3 && !game_camera.high_view_suppressed) {
        hud_camera_forced_timer.SetTimeout(3000);
        game_play_sound(0x27, 1.0f, 0, 0, 0);
    }
}

// 0x40e9d0  Confidence: exact.
// Appraise opens the inventory in identify mode, Pick Lock waits for a target (hud_target_mode 2), Banish
// Summon acts on cur_summon; spells go through character_cast_spell.
void hud_use_hot_button(int slot)
{
    int kind, id;

    if (slot < 0 || slot >= HUD_NUM_HOT)
        return;
    if (!SAVEGAME_HM->hot_button_get(&kind, &id, hud_member(), slot)) {
        hud_mouse_mode = 0;
        return;
    }
    if (interface_get_screen() != 0)
        interface_set_screen(0, 0);
    hud_target_mode = -1;

    if (kind) {
        switch (id) {
        default:
            if (character_use_skill(id, 0, 0))
                game_play_sound(0x23, 1.0f, 0, 0, 0);
            else
                game_play_sound(0x24, 1.0f, 0, 0, 0);
            return;
        case SKILL_BANISH_SUMMON:
            if (character_use_skill(SKILL_BANISH_SUMMON, cur_summon, 0))
                game_play_sound(0x23, 1.0f, 0, 0, 0);
            else
                game_play_sound(0x24, 1.0f, 0, 0, 0);
            return;
        case SKILL_APPRAISE: {
            bool identify[4];

            identify[0] = true;
            interface_set_screen(1, identify);
            game_play_sound(0x23, 1.0f, 0, 0, 0);
            return;
        }
        case SKILL_PICK_LOCK:
            hud_target_mode = 2;
            game_play_sound(0x27, 1.0f, 0, 0, 0);
            return;
        }
    }
    character_cast_spell(id, 0);
    game_play_sound(0x23, 1.0f, 0, 0, 0);
}

// 0x40eb30  Confidence: exact (VC6 8966 byte match, m-1020: kind and id local to the hot-button block, D11901) (switch 0x40edbc / byte table 0x40ede0 on n - 1; frame: kind 0x0, w 0x4,
// h 0x8, id 0xc, text 0x10, name 0x110). n is a hot button slot when hot_button, else a hud_buttons index:
// 1-5 the portraits (name, HP, AP), 41-61 the panel icons.
void hud_draw_tooltips(int n, bool hot_button)
{
    int w, h;
    char text[256];
    char name[64];
    int x, y;

    if (hot_button) {
        int kind;
        int id;

        if (!SAVEGAME_HM->hot_button_get(&kind, &id, hud_member(), n))
            return;
        if (n < 3) {
            x = hud_hot_button_origin[resolution_index][0] + hud_hot_button_spacing[resolution_index][0] * n;
            y = hud_hot_button_origin[resolution_index][1];
        } else {
            x = hud_hot_button_origin[resolution_index][0] + hud_hot_button_spacing[resolution_index][0] * (n - 3);
            y = hud_hot_button_origin[resolution_index][1] + hud_hot_button_spacing[resolution_index][1];
        }
        w = hud_hot_button_size[resolution_index][0];
        h = hud_hot_button_size[resolution_index][1];
        if (kind)
            strcpy(text, localize_string_or_invalid(skill_table[id].name));
        else
            strcpy(text, localize_string_or_invalid(spell_display_names[id]));
    } else {
        switch (n) {
        case 1: case 2: case 3: case 4: case 5: {
            Character *c = hud_portrait_slots[n - 1].character;
            int hp, max_hp, ap, max_ap;

            if (!c)
                return;
            CHAR_HM(c)->get_display_name(name);
            hp = (int)c->hp;
            max_hp = CHAR_M(c)->get_max_stat(0);
            ap = c->ap;
            max_ap = CHAR_M(c)->get_max_stat(1);
            sprintf(text, XSTR("%s\nHP: %d/%d\nAP: %d/%d", 955), name, hp, max_hp, ap, max_ap);
            break;
        }
        case 41: case 42: case 43:
            strcpy(text, XSTR("Enter combat mode", 671));
            break;
        case 44: case 45: case 46:
            strcpy(text, XSTR("Leave combat mode", 672));
            break;
        case 47: case 48: case 49:
            strcpy(text, XSTR("Character status", 673));
            break;
        case 50: case 51: case 52:
            strcpy(text, XSTR("Spells", 674));
            break;
        case 53: case 54: case 55:
            strcpy(text, XSTR("Quest journal", 675));
            break;
        case 56: case 57: case 58:
            strcpy(text, XSTR("Inventory", 676));
            break;
        case 59: case 60: case 61:
            strcpy(text, XSTR("Skills", 677));
            break;
        default:
            return;
        }
        x = hud_buttons[n].x;
        y = hud_buttons[n].y;
        w = hud_buttons[n].w;
        h = hud_buttons[n].h;
    }
    ui_tooltip_set(x, y, w, h, text);
}

// 0x40ee20  Confidence: exact. Mode 1: the left button is held on a HiLite entry; on release over its Press
// entry (the next one) the action runs.
void hud_update_buttons()
{
    int button = hud_hover_button + 1;

    if (hud_hover_button >= 0 && hud_hover_button < HUD_NUM_BUTTONS) {
        ui_button_state *b = &hud_buttons[button];

        if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
            b->flags |= 2;
        if (mouse_is_down(0))
            return;
        if (b->flags & 2)
            hud_button_action(button);
        if (hud_mouse_mode != 1)
            return;
    }
    hud_mouse_mode = 0;
}

// 0x40eeb0  Confidence: exact (VC6 byte match, match27; was likely: switch 0x40f0a8 / byte table 0x40f0e8 on
// button - 21, 0..40; the bodies ending in sound 0x27 are tail-merged; case 32 laid out after 39 as in the
// binary). LOGIC FIX (D8600): the panel buttons are the Press entries 43 / 46 / 49 / 52 / 55 / 58 / 61 (the
// byte table), not 42 ... 60 (Hilite + 0), which never ran. With an item waiting for a target
// (hud_target_mode 0) a portrait click picks that portrait instead.
void hud_button_action(int button)
{
    switch (button) {
    case 21:
        if (hud_target_mode == 0) {
            world_pick_portrait_clicked = true;
            return;
        }
        hud_select_party_member(0);
        break;
    case 23:
        if (hud_target_mode == 0) {
            world_pick_portrait_clicked = true;
            return;
        }
        hud_select_party_member(1);
        break;
    case 25:
        if (hud_target_mode == 0) {
            world_pick_portrait_clicked = true;
            return;
        }
        hud_select_party_member(2);
        break;
    case 27:
        if (hud_target_mode == 0) {
            world_pick_portrait_clicked = true;
            return;
        }
        hud_select_party_member(3);
        break;
    case 29:
        if (hud_target_mode == 0) {
            world_pick_portrait_clicked = true;
            return;
        }
        hud_select_party_member(4);
        break;
    case 34:
        hud_key_playmenu();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 36:
        hud_key_automap();
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        break;
    case 39:
        hud_toggle_solo_mode();
        game_play_sound(0x2a, 1.0f, 0, 0, 0);
        break;
    case 32:
        hud_key_pause();
        game_play_sound(0x26, 1.0f, 0, 0, 0);
        break;
    case 43:
    case 46:
        hud_toggle_combat_mode();
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        break;
    case 49:
        hud_key_status();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 52:
        hud_key_spells();
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        break;
    case 55:
        hud_key_journal();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 58:
        hud_key_inventory();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 61:
        hud_key_skills();
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        break;
    }
    hud_target_mode = -1;
}

// 0x40f120  Confidence: exact. Mode 2: as hud_update_buttons with the right button.
void hud_portraits_handle_click()
{
    int button = hud_hover_button + 1;

    if (hud_hover_button >= 0 && hud_hover_button < HUD_NUM_BUTTONS) {
        ui_button_state *b = &hud_buttons[button];

        if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
            b->flags |= 2;
        if (mouse_is_down(1))
            return;
        if (b->flags & 2)
            hud_portrait_action(button);
        if (hud_mouse_mode != 2)
            return;
    }
    hud_mouse_mode = 0;
}

// 0x40f1b0  Confidence: exact (VC6 byte match, match27; was likely: jump table 0x40f200; each case has the
// whole select / status / sound sequence, which 8966 tail-merges down to the pushed slot). Right click on a
// portrait: select the member and open its Character screen.
void hud_portrait_action(int button)
{
    hud_target_mode = -1;
    switch (button) {
    case 21:
        hud_select_party_member(0);
        hud_key_status();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 23:
        hud_select_party_member(1);
        hud_key_status();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 25:
        hud_select_party_member(2);
        hud_key_status();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 27:
        hud_select_party_member(3);
        hud_key_status();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 29:
        hud_select_party_member(4);
        hud_key_status();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    }
}

// 0x40f230  Confidence: exact. Mode 3: a hot button is held; on release over it, it is used.
void hud_hot_buttons_handle_click()
{
    int id, kind;

    hud_hot_button_used = -1;
    if (SAVEGAME_HM->hot_button_get(&kind, &id, hud_member(), hud_hot_button_hover)) {
        int x, y;

        if (hud_hot_button_hover < 3) {
            x = hud_hot_button_origin[resolution_index][0] + hud_hot_button_spacing[resolution_index][0] * hud_hot_button_hover;
            y = hud_hot_button_origin[resolution_index][1];
        } else {
            x = hud_hot_button_origin[resolution_index][0] + hud_hot_button_spacing[resolution_index][0] * (hud_hot_button_hover - 3);
            y = hud_hot_button_origin[resolution_index][1] + hud_hot_button_spacing[resolution_index][1];
        }
        int w = hud_hot_button_size[resolution_index][0];
        int h = hud_hot_button_size[resolution_index][1];
        if (point_in_rect(game_mouse_x, game_mouse_y, x, y, w, h))
            hud_hot_button_used = hud_hot_button_hover;
        if (mouse_is_down(0))
            return;
        if (hud_hot_button_used >= 0) {
            hud_use_hot_button(hud_hot_button_hover);
            hud_hot_button_used = -1;
        }
    }
    hud_mouse_mode = 0;
}

// 0x40f340  Confidence: likely (frame: id2 0x0, kind2 0x4, id1 0x8, kind1 0xc). Mode 4: a skill or spell
// icon dragged from the panel (hud_drag_icon_begin). Dropped on a slot it is stored there; the swap branch
// for a drag that starts on a hot button is never reached, since only hud_drag_icon_begin writes the source
// slot and it writes 6 (none).
void hud_hot_buttons_drag()
{
    int id2, kind2, id1, kind1;

    hud_hot_button_hover = -1;
    if (point_in_rect(game_mouse_x, game_mouse_y, hud_drag_icon_x, hud_drag_icon_y, hud_drag_icon_w, hud_drag_icon_h))
        hud_drag_flags.active = 0;
    else
        hud_drag_flags.active = 1;

    if (point_in_rect(game_mouse_x, game_mouse_y, hud_buttons[HUD_BTN_HOTBAR].x, hud_buttons[HUD_BTN_HOTBAR].y,
                      hud_buttons[HUD_BTN_HOTBAR].w, hud_buttons[HUD_BTN_HOTBAR].h)) {
        int slot;

        for (slot = 0; slot < HUD_NUM_HOT; slot++) {
            int x, y;

            if (slot < 3) {
                x = hud_hot_button_origin[resolution_index][0] + hud_hot_button_spacing[resolution_index][0] * slot;
                y = hud_hot_button_origin[resolution_index][1];
            } else {
                x = hud_hot_button_origin[resolution_index][0] + hud_hot_button_spacing[resolution_index][0] * (slot - 3);
                y = hud_hot_button_origin[resolution_index][1] + hud_hot_button_spacing[resolution_index][1];
            }
            int w = hud_hot_button_size[resolution_index][0];
            int h = hud_hot_button_size[resolution_index][1];
            if (point_in_rect(game_mouse_x, game_mouse_y, x, y, w, h)) {
                hud_hot_button_hover = slot;
                break;
            }
        }
    }

    if (mouse_is_down(0))
        return;

    if (hud_drag_flags.active) {
        int src = hud_drag_flags.slot == 6 ? -1 : hud_drag_flags.slot;
        int member = hud_member();
        int dst = hud_hot_button_hover;

        if (dst >= 0) {
            if (src >= 0) {
                bool had_dst = SAVEGAME_HM->hot_button_get(&kind1, &id1, member, dst);

                if (SAVEGAME_HM->hot_button_get(&kind2, &id2, member, src))
                    SAVEGAME_HM->hot_button_set(kind2, id2, member, dst);
                else
                    SAVEGAME_HM->hot_button_clear(member, dst);
                if (had_dst)
                    SAVEGAME_HM->hot_button_set(kind1, id1, member, src);
                else
                    SAVEGAME_HM->hot_button_clear(member, src);
            } else {
                SAVEGAME_HM->hot_button_set(hud_drag_flags.is_skill, hud_drag_flags.id, member, dst);
            }
        } else if (src >= 0) {
            SAVEGAME_HM->hot_button_clear(member, src);
        }
    }
    hud_drag_icon_end();
    hud_mouse_mode = 0;
}

// 0x40f570  Confidence: exact. Mode 5: once the current character's death move has finished, a 3 s timer;
// after it a key returns to the main menu state (gameseq 1).
int hud_death_timer_frame(int key)
{
    if (cur_character && !CHAR_HM(cur_character)->get_current_move()) {
        if (!hud_death_timer.valid()) {
            hud_death_timer.SetTimeout(3000);
        } else if (hud_death_timer.elapsed() && key) {
            gameseq_set_state(1);
            return 0;
        }
    }
    return key;
}

// 0x40f5d0  Confidence: exact. Mode 6: the "You have died." popup (death_popup): Restart respawns the local
// multiplayer player, Quit asks again.
int hud_confirm_quit(int key)
{
    char *buttons[2];

    buttons[0] = XSTR("Yes", 654);
    buttons[1] = XSTR("No", 655);
    if (ui_choice_popup_is_active()) {
        switch (ui_choice_popup_frame(key)) {
        case 0:
            mp_respawn_and_notify(MP_PLAYER_ID(mp_local_player));
            break;
        case 1:
            ui_message_box_show(XSTR("Quit game", 681), XSTR("Are you sure?", 682), -1, -1, 2, buttons,
                                hud_confirm_quit_cb, 0);
            return key;
        }
    }
    return key;
}

// 0x40f650  Confidence: exact. Yes: leave the game (2 = the host ends it); No: back to the popup.
void hud_confirm_quit_cb(int button)
{
    if (button == 0) {
        if ((game_flags & 2) && mp_local_player && (mp_local_player->flags & 1))
            mp_player_disconnected(2, 0);
        else
            mp_player_disconnected(0, 0);
        return;
    }
    death_popup();
}

// 0x40f690  Confidence: exact.
void death_popup()
{
    char *items[2];

    items[0] = XSTR("Restart", 678);
    items[1] = XSTR("Quit", 679);
    ui_choice_popup_show(XSTR("You have died.", 680), -1, hud_death_popup_y[resolution_index], 2, items, 11, 0, 0);
}

// 0x40f6e0  Confidence: exact. Drawn by
// interface_draw with the full screen as clip. Entry 31 ("Main-HUD-PauseHiLite") first draws the animated
// pause icons, so they lie under the pause highlight.
void hud_draw()
{
    int clip_x, clip_y, clip_w, clip_h;
    int i;

    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    gr__set_clip(0, 0, gr_screen_width(), gr_screen_height());

    for (i = 0; i < HUD_NUM_BUTTONS; i++) {
        ui_button_def *def = &hud_button_defs[i];
        ui_button_state *b = &hud_buttons[i];

        if ((game_flags & 2) && (def->flags & 0x10))
            continue;
        if (cur_level_index == 4 && (def->flags & 0x20))
            continue;
        if (i == HUD_BTN_PAUSE_HILITE)
            hud_draw_animated_icons();
        if (!(b->flags & 3))
            continue;
        if (def->flags & 4) {
            Color c = hud_portrait_get_color(i - HUD_BTN_PORTRAIT);

            gr_set_color_ptr(&c);
        } else if (def->flags & 8) {
            float t = ((int)(interface_pulse_time * 1000.0f) % 1000) * 0.001f;
            gr_set_color(255, 255, 255, 255 - (int)(t * 204.0f));
        } else {
            gr_set_color(255, 255, 255, 255);
        }
        ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
    }

    hud_draw_camera_mode();
    hud_draw_camera_forced_icon();
    hud_draw_health_bars();
    if (game_flags & 2) {
        if (mp_local_player && !(mp_local_player->flags & 0x80000000))
            hud_draw_hot_buttons();
    } else if (cur_level_index != 4) {
        hud_draw_hot_buttons();
    }
    draw_game_over();
    mp_draw_loading_status();
    tutorial_draw();
    gr__set_clip(clip_x, clip_y, clip_w, clip_h);
}

// 0x40f880  Confidence: likely (switch 0x40fa70 on state - 1; the clamp of case 2 is the unsigned char
// "<= 0 / >= 255" pattern; in case 3 red is computed from the green channel, as in the code; VC6 keeps the
// green float in the dead slot of n). White when n is not a shown portrait or the state is 0.
Color hud_portrait_get_color(int n)
{
    Color_ctor c(255, 255, 255, 255);
    hud_portrait_slot *s;

    if (n < 0 || n >= hud_portrait_count)
        return c;
    s = &hud_portrait_slots[n];
    switch (s->state) {
    case 1:                                   // hold: light blue
        c.r = 128;
        c.g = 128;
        c.b = 255;
        c.a = 255;
        break;
    case 2: {                                 // hit: red fading back to white over 250 ms
        ubyte v;

        c.r = 255;
        v = (ubyte)(int)((250 - s->timer.GetRemainingTime()) * 1.02f + 0.5f);
        if (v <= 0)
            v = 0;
        else if (v >= 255)
            v = 255;
        c.g = v;
        c.b = v;
        c.a = 255;
        break;
    }
    case 3: {                                 // fighting: pulsing
        float r = 1.0f, g = 1.0f, b = 1.0f;

        color_pulse_scale(&r, &g, &b, 0.25f);
        c.r = (ubyte)(int)(g * 255.0f + 0.5f);
        c.g = (ubyte)(int)(g * 255.0f + 0.5f);
        c.b = (ubyte)(int)(b * 255.0f + 0.5f);
        c.a = 255;
        break;
    }
    case 4: {                                 // HP low: pulsing red (purple when holding)
        float r = 1.0f, g = 1.0f, b = 1.0f;
        Player *p;

        ui_apply_pulse_rgb(&r, &g, &b, 0.25f);
        p = player_from_handle(s->character->handle);
        if (p && (p->flags & 4)) {
            c.r = (ubyte)(int)(255.0f - r * 128.0f + 0.5f);
            c.g = (ubyte)(int)(g * 128.0f + 0.5f);
        } else {
            c.r = 255;
            c.g = (ubyte)(int)(g * 255.0f + 0.5f);
        }
        c.b = (ubyte)(int)(b * 255.0f + 0.5f);
        c.a = 255;
        break;
    }
    case 5:                                   // dead: grey
        c.r = 0x60;
        c.g = 0x60;
        c.b = 0x60;
        c.a = 255;
        break;
    }
    return c;
}

// 0x40fa90  Confidence: exact (VC6 byte match) (switch 0x40fc10, bodies in the order 2, 0, 1/3; frame: w 0x0, h 0x4,
// text 0x8). sum.symbols calls it the detail level; what it shows is the camera mode (game_camera +0, or
// +4 while gliding, mode 5): for 4 s after it changes, the "\xb9" icon and "auto" / "low" / "high".
void hud_draw_camera_mode()
{
    int w, h;
    char text[16];
    int mode;

    if (game_flags & 0x20)
        return;
    mode = game_camera.mode;
    if (mode == 5)
        mode = game_camera.prev_mode;
    if (mode != hud_shown_camera_mode) {
        hud_shown_camera_mode = mode;
        hud_camera_mode_timer.SetTimeout(4000);
    } else if (!hud_camera_mode_timer.valid()) {
        return;
    }

    switch (mode) {
    case 2:
        sprintf(text, XSTR("auto", 683));
        break;
    case 0:
        sprintf(text, XSTR("low", 684));
        break;
    case 1:
    case 3:
        sprintf(text, XSTR("high", 685));
        break;
    default:
        return;
    }

    gr_get_string_size(&w, &h, text, -1, -1);
    w += hud_camera_icon_width[resolution_index];
    gr_set_color_ptr(&ui_color_text_box_bg);
    gr_rect(hud_camera_mode_pos[resolution_index][0], hud_camera_mode_pos[resolution_index][1], w, h, gr_rect_mode,
            1.0f);
    gr_set_color_ptr(&ui_color_text);
    gr_string(hud_camera_mode_pos[resolution_index][0], hud_camera_mode_pos[resolution_index][1], "\xb9", -1,
              gr_string_mode);
    gr_string(hud_camera_mode_pos[resolution_index][0] + hud_camera_icon_width[resolution_index],
              hud_camera_mode_pos[resolution_index][1], text, -1, gr_string_mode);
    if (hud_camera_mode_timer.elapsed())
        hud_camera_mode_timer.invalidate();
}

// 0x40fc20  Confidence: exact (VC6 byte match) (frame: Color 0x0, w 0x4, h 0x8, temp 0xc). The camera-lock icon "\xba"
// right-aligned to the camera-mode overlay for 3 s after hud_key_camera, blinking every 500 ms.
void hud_draw_camera_forced_icon()
{
    Color c;
    int w, h;

    if (!hud_camera_forced_timer.valid())
        return;
    gr_get_string_size(&w, &h, "\xba", -1, -1);
    gr_set_color_ptr(&ui_color_text_box_bg);
    gr_rect(hud_camera_mode_pos[resolution_index][0] - w, hud_camera_mode_pos[resolution_index][1], w, h,
            gr_rect_mode, 1.0f);
    int t = (3000 - hud_camera_forced_timer.GetRemainingTime()) % 500;
    c.r = ui_color_text.r;
    c.g = ui_color_text.g;
    c.b = ui_color_text.b;
    c.a = (ubyte)(int)((1.0f - t * 0.002f) * 255.0f + 0.5f);
    gr_set_color_ptr(&c);
    gr_string(hud_camera_mode_pos[resolution_index][0] - w, hud_camera_mode_pos[resolution_index][1], "\xba", -1,
              gr_string_mode);
    if (hud_camera_forced_timer.elapsed())
        hud_camera_forced_timer.invalidate();
}

// 0x40fd40  Confidence: exact (the loop keeps i and the record pointer in stack slots; the AP product is
// fild / fimul, so the table value is converted to float first). HP (dark red) and AP (dark blue) bars
// under each portrait background.
void hud_draw_health_bars()
{
    int i;

    for (i = 0; i < hud_portrait_count; i++) {
        ui_button_state *b = &hud_buttons[HUD_BTN_PORTRAIT_BKG + i];
        Character *c = hud_portrait_slots[i].character;
        int x, y, w, h;

        x = hud_hp_bar_pos[resolution_index][0] + b->x;
        y = hud_hp_bar_pos[resolution_index][1] + b->y;
        w = (int)(hud_bar_size[resolution_index][0] * c->hp / CHAR_M(c)->get_max_stat(0) + 0.5f);
        h = hud_bar_size[resolution_index][1];
        if (w > 0) {
            gr_set_color_ptr(&ui_color_dark_red);
            gr_rect(x, y, w, h, gr_rect_mode, 1.0f);
        }

        x = hud_hp_bar_pos[resolution_index][0] + b->x;
        y = hud_bar_size[resolution_index][1] + hud_hp_bar_pos[resolution_index][1] + b->y;
        w = (int)((float)hud_bar_size[resolution_index][0] * c->ap / CHAR_M(c)->get_max_stat(1) + 0.5f);
        h = hud_bar_size[resolution_index][1];
        if (w > 0) {
            gr_set_color_ptr(&ui_color_dark_blue);
            gr_rect(x, y, w, h, gr_rect_mode, 1.0f);
        }
    }
}

// 0x40fed0  Confidence: exact (scale switch: bodies in the order 2, 1, default; frame: scale 0x0, h 0x4,
// w 0x8). The running pause animation (pause.vbm, frames after the first) on the pause button, stretched
// for 800 and 1024; while paused also the FlashPaused animation.
void hud_draw_animated_icons()
{
    float scale;
    int h, w;

    gr_set_color(255, 255, 255, 255);
    if (hud_pause_bm >= 0) {
        int bm = hud_pause_bm + (int)hud_pause_anim_time + 1;
        int x = hud_pause_icon_pos[resolution_index][0] + hud_buttons[HUD_BTN_PAUSE].x;
        int y = hud_pause_icon_pos[resolution_index][1] + hud_buttons[HUD_BTN_PAUSE].y;

        switch (resolution_index) {
        case 0:
            scale = 1.0f;
            break;
        case 1:
            scale = 1.25f;
            break;
        case 2:
            scale = 1.6f;
            break;
        default:
            scale = 1.0f;
            break;
        }
        bm_get_dimensions(bm, &w, &h);
        gr_bitmap_stretched(bm, x, y, (int)(w * scale + 0.5f), (int)(h * scale + 0.5f), 0, 0, w, h,
                            gr_bitmap_wrap_mode);
    }
    if (hud_flash_paused_bm >= 0 && game_is_paused())
        ui_bitmap_scaled(hud_flash_paused_bm + (int)hud_flash_paused_anim_time + 1,
                         hud_flash_paused_pos[resolution_index][0] + hud_buttons[HUD_BTN_PAUSE].x,
                         hud_flash_paused_pos[resolution_index][1] + hud_buttons[HUD_BTN_PAUSE].y,
                         gr_bitmap_wrap_mode);
}

// 0x410020  Confidence: exact (VC6 byte match, match27; was likely: the skill icon frame through a local as
// for spells, whose lookup 8966 copies into each branch; frame: id 0x0, kind 0x4). Icon frames: 0 normal,
// 1 hovered, 2 used, 3 not usable now.
void hud_draw_hot_buttons()
{
    int slot;

    if (hud_selected_portrait < 0)
        return;
    gr_set_color(255, 255, 255, 255);
    for (slot = 0; slot < HUD_NUM_HOT; slot++) {
        int id, kind;
        int x, y, bm;

        if (!SAVEGAME_HM->hot_button_get(&kind, &id, hud_member(), slot))
            continue;
        if (slot < 3) {
            x = hud_hot_button_origin[resolution_index][0] + hud_hot_button_spacing[resolution_index][0] * slot;
            y = hud_hot_button_origin[resolution_index][1];
        } else {
            x = hud_hot_button_origin[resolution_index][0] + hud_hot_button_spacing[resolution_index][0] * (slot - 3);
            y = hud_hot_button_origin[resolution_index][1] + hud_hot_button_spacing[resolution_index][1];
        }
        if (kind) {
            int frame;

            if (skill_is_usable(id, 0) && skill_is_available(id, 0)) {
                if (slot == hud_hot_button_used)
                    frame = 2;
                else if (slot == hud_hot_button_hover)
                    frame = hud_mouse_mode != 3;
                else
                    frame = 0;
            } else {
                frame = 3;
            }
            bm = skills_icon_bms[id][frame];
        } else {
            int frame;

            if (character_can_cast_now(0) && spell_is_usable(id, 0, 0)) {
                if (slot == hud_hot_button_used)
                    frame = 2;
                else
                    frame = slot == hud_hot_button_hover;
            } else {
                frame = 3;
            }
            bm = spells_icon_bms[id][frame];
        }
        ui_bitmap_scaled(bm, x, y, gr_bitmap_wrap_mode);
    }
}

// 0x4101e0  Confidence: exact (frame: w 0x0, h 0x4, text 0x8). Single player, after the death timer.
void draw_game_over()
{
    int w, h;
    char text[256];
    int screen_w, screen_h;

    if (!hud_death_timer.valid() || !hud_death_timer.elapsed())
        return;
    screen_w = gr_screen_width();
    screen_h = gr_screen_height();
    if (!(game_flags & 1))
        return;
    sprintf(text, XSTR("Game Over. Press any key to continue", 686));
    gr_get_string_size(&w, &h, text, -1, -1);
    gr_set_color(255, 255, 255, 255);
    gr_string((screen_w - w) / 2, screen_h / 3, text, -1, gr_string_mode);
}

// 0x4102a0  Confidence: exact (VC6 8966 byte match with the panel size held in w and h, which the string sizes
// reuse later, D15201; switch 0x4104fc on state - 6; frame: w 0x0, h 0x4, font height 0x8, name 0xc). While a
// multiplayer game loads: one line per player with its loading step, and for the host "'F' forces start".
void mp_draw_loading_status()
{
    int w, h;
    int font_h;
    char name[32];

    if (game_flags & 1)
        return;
    if (!mp_local_player)
        return;
    font_h = gr_get_font_height(-1);
    if (mp_netgame_state < 5 && MP_PLAYER_STATE(mp_local_player) > 4) {
        mp_player *local = mp_local_player;
        Player *p;
        int count = 0;
        int x, y;

        for (p = party_list.next; p != &party_list; p = p->next)
            count++;
        w = mp_loading_panel_width[resolution_index];
        h = count * font_h + 2 * ui_choice_popup_margin_y[resolution_index];
        if ((game_flags & 2) && local && (local->flags & 1))
            h = h + font_h + 5;
        x = (gr_screen_width() - w) / 2;
        y = (gr_screen_height() - h) / 3;
        ui_draw_panel_frame(x, y, w, h);
        y += ui_choice_popup_margin_y[resolution_index];
        gr_set_color_ptr(&ui_color_text);

        for (p = party_list.next; p != &party_list; p = p->next) {
            char *status;

            gr_string_truncate_ellipsis(name, MP_PLAYER_NAME(p->mp_owner), mp_loading_name_width[resolution_index], -1);
            gr_string(x + ui_choice_popup_margin[resolution_index], y, name, -1, gr_string_mode);
            switch (MP_PLAYER_STATE(p->mp_owner)) {
            case 6:
                status = XSTR("linking players", 687);
                break;
            case 7:
            case 8:
                status = XSTR("linking objects", 688);
                break;
            case 9:
                status = XSTR("ready", 689);
                break;
            default:
                status = XSTR("loading level", 690);
                break;
            }
            gr_get_string_size(&w, &h, status, -1, -1);
            gr_string(x + mp_loading_panel_width[resolution_index] - ui_choice_popup_margin[resolution_index] - w, y,
                      status, -1, gr_string_mode);
            y += font_h;
        }

        if ((game_flags & 2) && mp_local_player && (mp_local_player->flags & 1)) {
            char *s = XSTR("'F' forces start", 691);

            gr_get_string_size(&w, &h, s, -1, -1);
            y += 5;
            gr_string((mp_loading_panel_width[resolution_index] - w) / 2 + x, y, s, -1, gr_string_mode);
        }
    }
    if (hud_mouse_mode == 6)
        ui_choice_popup_draw();
}

// 0x410510  Confidence: exact. The height of the portrait background (the panel's visible height), which
// interface_set_view_clip takes off the 3D view.
int hud_get_panel_height()
{
    return hud_buttons[HUD_BTN_PORTRAIT_BKG].h;
}

// 0x410520  Confidence: exact.
bool hud_mouse_in_panel(int x, int y)
{
    int i;

    for (i = 0; i < HUD_NUM_BUTTONS; i++) {
        ui_button_state *b = &hud_buttons[i];
        if ((b->flags & 3) && point_in_rect(x, y, b->x, b->y, b->w, b->h))
            return true;
    }
    return false;
}

// 0x410570  Confidence: likely (the four bit fields come out as one merged store). Called by the skills and
// spells panels (and ui_choice_popup_update_buttons) when an icon is dragged; (x, y, w, h) is the source
// rectangle.
void hud_drag_icon_begin(int x, int y, int w, int h, int kind, int id)
{
    hud_drag_icon_y = y;
    hud_drag_icon_x = x;
    hud_drag_icon_h = h;
    hud_drag_icon_w = w;
    hud_drag_flags.active = 1;
    hud_drag_flags.slot = 6;
    hud_drag_flags.is_skill = kind != 0;
    hud_drag_flags.id = id;
}

// 0x4105d0  Confidence: exact.
void hud_drag_icon_end()
{
    hud_drag_flags.active = 0;
}

// 0x4105e0  Confidence: exact (frame: clip h/w/y/x 0x0-0xc). The dragged icon at half alpha, centred on the
// mouse.
void hud_drag_icon_draw()
{
    int clip_x, clip_y, clip_w, clip_h;
    int x, y, bm;

    if (!hud_drag_flags.active)
        return;
    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    gr__set_clip(0, 0, gr_screen_width(), gr_screen_height());
    x = game_mouse_x - hud_hot_button_size[resolution_index][0] / 2;
    y = game_mouse_y - hud_hot_button_size[resolution_index][1] / 2;
    if (hud_drag_flags.is_skill)
        bm = skills_icon_bms[hud_drag_flags.id][0];
    else
        bm = spells_icon_bms[hud_drag_flags.id][0];
    gr_set_color(255, 255, 255, 128);
    ui_bitmap_scaled(bm, x, y, gr_bitmap_wrap_mode);
    gr__set_clip(clip_x, clip_y, clip_w, clip_h);
}

// 0x4106d0  Confidence: exact. From party_check_all_dead: wait for the death animation (hud_death_timer_frame).
void hud_mode_set_5()
{
    hud_mouse_mode = 5;
}

// 0x4106e0  Confidence: exact (death_popup is a tail jump; the inline repeat of hud_drag_icon_end's store is
// in the code).
void hud_party_dead()
{
    int screen = interface_get_screen();

    if (screen != 0 && screen != 9)
        interface_set_screen(0, 0);
    if (hud_mouse_mode == 6)
        return;
    hud_mouse_mode = 6;
    hud_target_mode = -1;
    hud_drag_icon_end();
    hud_drag_flags.active = 0;
    death_popup();
}

// 0x410730  Confidence: exact (the second call is a tail jump).
void hud_dead_clear_popups()
{
    if (hud_mouse_mode == 6) {
        ClearFrameFlag5bd77c();
        ClearFrameFlag5bd504();
    }
}

// 0x410750  Confidence: exact.
void hud_mode_reset()
{
    hud_mouse_mode = 0;
}
