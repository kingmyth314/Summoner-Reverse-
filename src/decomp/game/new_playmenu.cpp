// new_playmenu.cpp - Sum.exe v1.40, code 0x421590-0x421dd3 (screen 9, the in-game play menu)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .data 0x58f4b8-0x58f6f8 (button table), .bss 0x5c3b78-0x5c3c5c.

#include <stdio.h>
#include <string.h>
#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/key.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/localize/localize.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"
#include "engine/objects/living_entity.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "summoner/interface/new_playmenu.h"
#include "summoner/interface/new_mainmenu.h"

#ifdef __GNUC__
// playmenu_open_save passes a localized string as the sprintf format, as the binary does.
#pragma GCC diagnostic ignored "-Wformat-security"
#endif

#define PLAYMENU_NUM_BUTTONS 9

// ---- .data ---------------------------------------------------------------------------------
// 0x58f4b8  9 x 0x40. flags: 1 "Game" heading, 2 menu entries, 4 close button hot spot (its pressed
// image is the next entry, crossbox2).
ui_button_def playmenu_button_table[PLAYMENU_NUM_BUTTONS] = {
    { "MenuPopup-bkg",     { { 240, 136 }, { 300, 170 }, { 384, 218 } }, 0 },
    { "MenuPopup-Game",    { { 280, 212 }, { 350, 265 }, { 444, 342 } }, 1 },
    { "MenuPopup-New",     { { 296, 227 }, { 370, 284 }, { 473, 366 } }, 2 },
    { "MenuPopup-Save",    { { 296, 239 }, { 370, 299 }, { 473, 387 } }, 2 },
    { "MenuPopup-Load",    { { 296, 252 }, { 370, 315 }, { 473, 405 } }, 2 },
    { "MenuPopup-Options", { { 280, 269 }, { 350, 336 }, { 444, 429 } }, 2 },
    { "MenuPopup-Quit",    { { 280, 289 }, { 350, 361 }, { 444, 464 } }, 2 },
    { "crossbox-hilite2",  { { 352, 204 }, { 440, 255 }, { 573, 327 } }, 4 },
    { "crossbox2",         { { 352, 204 }, { 440, 255 }, { 573, 327 } }, 0 },
};

// ---- .bss ----------------------------------------------------------------------------------
int playmenu_mouse_mode;                                     // 0x5c3b78  0 idle, 1 button held
int playmenu_pressed_button;                                 // 0x5c3b7c
ui_button_state playmenu_buttons[PLAYMENU_NUM_BUTTONS];      // 0x5c3b80
unsigned int playmenu_flags;                                 // 0x5c3c58  bit 0: playmenu_init paused the game

// 0x421590  Confidence: exact.
void playmenu_load_bitmaps()
{
    float scale = ui_get_scale();
    int i;

    for (i = 0; i < PLAYMENU_NUM_BUTTONS; i++) {
        playmenu_buttons[i].flags = 0;
        playmenu_buttons[i].bitmap = -1;
        playmenu_buttons[i].x = 0;
        playmenu_buttons[i].y = 0;
        playmenu_buttons[i].w = 0;
        playmenu_buttons[i].h = 0;
        progress_update(0);

        if (strlen(playmenu_button_table[i].name)) {
            char filename[128];
            strcpy(filename, playmenu_button_table[i].name);
            strcat(filename, ui_bitmap_suffix_tga[ui_hires]);
            playmenu_buttons[i].bitmap = bm_load(filename, path_interface, -1);
        } else {
            playmenu_buttons[i].bitmap = -1;
        }

        playmenu_buttons[i].x = playmenu_button_table[i].pos[resolution_index][0];
        playmenu_buttons[i].y = playmenu_button_table[i].pos[resolution_index][1];

        if (playmenu_buttons[i].bitmap >= 0) {
            bm_get_dimensions(playmenu_buttons[i].bitmap, &playmenu_buttons[i].w, &playmenu_buttons[i].h);
            playmenu_buttons[i].w = (int)(playmenu_buttons[i].w * scale + 0.5f);
            playmenu_buttons[i].h = (int)(playmenu_buttons[i].h * scale + 0.5f);
        } else {
            playmenu_buttons[i].w = 0;
            playmenu_buttons[i].h = 0;
        }

        if (i == 0)
            playmenu_buttons[i].flags |= 1;
    }
}

// 0x4216f0  Confidence: likely. Both branches of the multiplayer test store playmenu_mouse_mode first;
// written once after the if/else here (the stores may also have been inside each branch). The order of
// the three button stores (3, 4, 2 in the binary) is scheduling.
void playmenu_init()
{
    playmenu_flags = 0;

    if (game_flags & 1) {
        if (!game_is_paused()) {
            playmenu_flags |= 1;
            game_pause(0, 0);
        }
    } else {
        ui_screen_stop_player();
    }

    // New, Save and Load are disabled in multiplayer.
    if (game_flags & 2) {
        playmenu_buttons[2].flags |= 4;
        playmenu_buttons[3].flags |= 4;
        playmenu_buttons[4].flags |= 4;
    } else {
        playmenu_buttons[2].flags &= ~4;
        playmenu_buttons[3].flags &= ~4;
        playmenu_buttons[4].flags &= ~4;
    }
    playmenu_mouse_mode = 0;
}

// 0x4217b0  Confidence: exact (tail call).
void playmenu_leave()
{
    if (playmenu_flags & 1)
        game_unpause();
}

// 0x4217c0  Confidence: exact. Key 1 (Esc) is handled in playmenu_handle_click.
int playmenu_frame(int key)
{
    int i;

    for (i = 0; i < PLAYMENU_NUM_BUTTONS; i++)
        playmenu_buttons[i].flags &= ~2;

    if (!(player_input[0] & 0x10) && !ui_message_box_is_active()) {
        switch (playmenu_mouse_mode) {
        case 1:
            playmenu_update_buttons();
            break;
        case 0:
            key = playmenu_handle_click(key);
            break;
        }
    }
    return key;
}

// 0x421820  Confidence: exact.
int playmenu_handle_click(int key)
{
    int i;

    playmenu_buttons[0].flags |= 2;
    playmenu_pressed_button = -1;

    for (i = 0; i < PLAYMENU_NUM_BUTTONS; i++) {
        if (!(playmenu_buttons[i].flags & 4) && (playmenu_button_table[i].flags & 7)
            && point_in_rect(game_mouse_x, game_mouse_y, playmenu_buttons[i].x, playmenu_buttons[i].y,
                             playmenu_buttons[i].w, playmenu_buttons[i].h)) {
            playmenu_buttons[i].flags |= 2;
            playmenu_pressed_button = i;
            if (i >= 2 && i <= 4)           // New / Save / Load also light the "Game" heading
                playmenu_buttons[1].flags |= 2;
            if ((playmenu_button_table[i].flags & 6) && mouse_was_pressed(0)) {
                playmenu_mouse_mode = 1;
                return key;
            }
            break;
        }
    }

    if (key == KEY_ESC) {
        interface_close_screen();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        return 0;
    }
    return key;
}

// 0x421900  Confidence: exact. Entries with table flag 4 (the close hot spot) test and light the next
// entry's state instead of their own.
void playmenu_update_buttons()
{
    if (playmenu_pressed_button >= 0 && playmenu_pressed_button < PLAYMENU_NUM_BUTTONS) {
        ui_button_state *b;
        if (playmenu_button_table[playmenu_pressed_button].flags & 4)
            b = &playmenu_buttons[playmenu_pressed_button + 1];
        else
            b = &playmenu_buttons[playmenu_pressed_button];

        if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h)) {
            b->flags |= 2;
            if (playmenu_pressed_button >= 2 && playmenu_pressed_button <= 4)
                playmenu_buttons[1].flags |= 2;
        }
        if (mouse_is_down(0))
            return;
        if (b->flags & 2)
            playmenu_button_action(playmenu_pressed_button);
    }
    playmenu_mouse_mode = 0;
}

// 0x4219c0  Confidence: exact (jump table 0x421a7c for buttons 2..7).
void playmenu_button_action(int button)
{
    switch (button) {
    case 2:
        playmenu_confirm_new_game();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 3:
        playmenu_open_save();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 4:
        open_saveload_screen();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 5:
        ui_open_options();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 6:
        playmenu_confirm_quit();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 7:
        interface_close_screen();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    }
}

// 0x421aa0  Confidence: exact.
void playmenu_confirm_new_game()
{
    char *buttons[2];
    buttons[0] = localize_strings[654];          // "Yes"
    buttons[1] = localize_strings[655];          // "No"
    ui_message_box_show(localize_strings[756],   // "New game"
                        localize_strings[682],   // "Are you sure?"
                        -1, -1, 2, buttons, mainmenu_confirm_new_game_cb, 0);
}

// 0x421ae0  Confidence: exact.
void mainmenu_confirm_new_game_cb(int button)
{
    if (button == 0) {
        mainmenu_start_new_game();
        game_flags |= 0x8000;
    }
}

// 0x421b00  Confidence: likely (jump table 0x421c68 for 0..3). The button array store sits before
// level_format_display_name in case 2, so it is written first in every case.
void playmenu_open_save()
{
    char *buttons[1];
    char level_name[128];
    char text[128];
    bool save;

    switch (savegame_location_allowed()) {
    case 0:
        save = true;
        interface_set_screen(8, &save);
        break;
    case 1:
        buttons[0] = localize_strings[231];                          // "OK"
        ui_message_box_show(localize_strings[645],                   // "Warning"
                            localize_strings[657],                   // "Cannot save in combat."
                            -1, -1, 1, buttons, NULL, 0);
        break;
    case 2:
        buttons[0] = localize_strings[231];
        level_format_display_name(level_name, cur_level_index);
        if (!_stricmp(level_name, "unknown"))
            sprintf(text, localize_strings[658]);                    // "Cannot save here."
        else
            sprintf(text, localize_strings[659], level_name);        // "Cannot save in %s."
        ui_message_box_show(localize_strings[645], text, -1, -1, 1, buttons, NULL, 0);
        break;
    case 3:
        buttons[0] = localize_strings[231];
        ui_message_box_show(localize_strings[645],
                            localize_strings[660],                   // "Cannot save during summon."
                            -1, -1, 1, buttons, NULL, 0);
        break;
    default:
        buttons[0] = localize_strings[231];
        ui_message_box_show(localize_strings[645],
                            localize_strings[661],                   // "Cannot save now."
                            -1, -1, 1, buttons, NULL, 0);
        break;
    }
}

// 0x421c80  Confidence: exact (VC6 byte match, match14; was exact). The screen argument is a pointer to a "save" byte (0 = load).
void open_saveload_screen()
{
    bool save[4];                  // only save[0] is written; the byte sits at the bottom of its dword slot (D7200)
    save[0] = false;
    interface_set_screen(8, save);
}

// 0x421ca0  Confidence: exact.
void playmenu_confirm_quit()
{
    char *buttons[2];
    buttons[0] = localize_strings[654];          // "Yes"
    buttons[1] = localize_strings[655];          // "No"
    ui_message_box_show(localize_strings[681],   // "Quit game"
                        localize_strings[682],   // "Are you sure?"
                        -1, -1, 2, buttons, mainmenu_confirm_quit_cb, 0);
}

// 0x421ce0  Confidence: exact (tail call).
void mainmenu_confirm_quit_cb(int button)
{
    if (button == 0)
        game_abandon_to_menu();
}

// 0x421cf0  Confidence: exact (VC6 byte match, match14; was exact).
void playmenu_draw()
{
    int i;

    gr_set_color(255, 255, 255, 255);
    for (i = 0; i < PLAYMENU_NUM_BUTTONS; i++) {
        ui_button_state *b = &playmenu_buttons[i];
        if (b->flags & 3)
            ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
    }
}

// 0x421d40  Confidence: exact.
bool playmenu_mouse_in_panel(int x, int y)
{
    return point_in_rect(x, y, playmenu_buttons[0].x, playmenu_buttons[0].y,
                         playmenu_buttons[0].w, playmenu_buttons[0].h);
}

// 0x421d70  Confidence: likely (untagged in sum.symbols; placed here because it sits right after
// playmenu_mouse_in_panel and its only caller is mainmenu_confirm_quit_cb). A dead party leader pays the
// XP penalty; in multiplayer the local player leaves (reason 2 when hosting), else back to state 1.
void game_abandon_to_menu()
{
    if (cur_character->flags & 0x80)
        character_xp_death_penalty(living_from_handle(mp_local_player->object->handle));

    if (game_flags & 2) {
        if (mp_local_player && (mp_local_player->flags & 1))
            mp_player_disconnected(2, 0);
        else
            mp_player_disconnected(0, 0);
    } else {
        gameseq_set_state(1);
    }
}
