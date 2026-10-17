// new_mainmenu.cpp - Sum.exe v1.40, code 0x41d740-0x41ddde (screen 14, the main menu)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact / likely / unsure (see new_playmenu.cpp header).
// Data: .data 0x58dd48-0x58e09e (button table, "eaxlogo.bik", "vlogo.bik"), .bss 0x5c2f98-0x5c30d8.
// The elevator code that follows (0x41dde0-0x41e37f) is not included; see NOTES.md.

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
#include "vsdk/os/movie.h"
#include "vsdk/net/net.h"
#include "vsdk/snd/snd.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/conversation.h"
#include "summoner/interface/new_mainmenu.h"
#include "summoner/interface/new_playmenu.h"
#include "summoner/interface/new_hud.h"         // hud_key_help

#define MAINMENU_NUM_BUTTONS 13

// ---- .data ---------------------------------------------------------------------------------
// 0x58dd48  13 x 0x40: the background, then a HiLite / Press pair per entry. Table flag 1 marks the
// HiLite images, which are the hot spots; the Press image is the next entry.
ui_button_def mainmenu_button_table[MAINMENU_NUM_BUTTONS] = {
    { "MainMenu-bkgrnd",    { { 0, 0 },     { 0, 0 },     { 0, 0 } },     0 },
    { "MainStartNewHiLite", { { 347, 180 }, { 434, 225 }, { 573, 290 } }, 1 },
    { "MainStartNewPress",  { { 347, 180 }, { 434, 225 }, { 573, 290 } }, 0 },
    { "MainContGameHiLite", { { 347, 228 }, { 434, 285 }, { 573, 367 } }, 1 },
    { "MainContGamePress",  { { 347, 228 }, { 434, 285 }, { 573, 367 } }, 0 },
    { "MainMultiHiLite",    { { 391, 276 }, { 489, 345 }, { 645, 444 } }, 1 },
    { "MainMultiPress",     { { 391, 276 }, { 489, 345 }, { 645, 444 } }, 0 },
    { "MainOptionsHiLite",  { { 447, 324 }, { 559, 405 }, { 734, 521 } }, 1 },
    { "MainOptionsPress",   { { 447, 324 }, { 559, 405 }, { 734, 521 } }, 0 },
    { "MainCreditsHiLite",  { { 447, 372 }, { 559, 465 }, { 734, 598 } }, 1 },
    { "MainCreditsPress",   { { 447, 372 }, { 559, 465 }, { 734, 598 } }, 0 },
    { "MainQuitHiLite",     { { 413, 420 }, { 516, 525 }, { 683, 675 } }, 1 },
    { "MainQuitPress",      { { 413, 420 }, { 516, 525 }, { 683, 675 } }, 0 },
};

// ---- .bss ----------------------------------------------------------------------------------
ui_button_state mainmenu_buttons[MAINMENU_NUM_BUTTONS];      // 0x5c2f98
int mainmenu_mouse_mode;                                     // 0x5c30d0  0 idle, 1 button held, 2 message box open
int mainmenu_pressed_button;                                 // 0x5c30d4

// 0x41d740  Confidence: exact. Unlike the play menu: no progress_update, names that already have an
// extension are loaded as they are and not scaled, bm_get_dimensions is called without a handle test,
// and an empty name leaves the whole record (position too) at its cleared values.
void mainmenu_load_bitmaps()
{
    float scale = ui_get_scale();
    char filename[128];
    bool scaled;

    for (int i = 0; i < MAINMENU_NUM_BUTTONS; i++) {
        ui_button_state *b = &mainmenu_buttons[i];
        ui_button_def *d = &mainmenu_button_table[i];
        b->flags = 0;
        b->bitmap = -1;
        b->x = 0;
        b->y = 0;
        b->w = 0;
        b->h = 0;

        if (strlen(d->name)) {
            strcpy(filename, d->name);
            if (!strchr(filename, '.')) {
                scaled = true;
                strcat(filename, ui_bitmap_suffix_tga[ui_hires]);
            } else {
                scaled = false;
            }
            b->bitmap = bm_load(filename, path_interface, -1);
            b->x = d->pos[resolution_index][0];
            b->y = d->pos[resolution_index][1];
            bm_get_dimensions(b->bitmap, &b->w, &b->h);
            if (scaled) {
                b->w = (int)(b->w * scale + 0.5f);
                b->h = (int)(b->h * scale + 0.5f);
            }
            if (i == 0)
                b->flags |= 1;
        }
    }
}

// 0x41d8b0  Confidence: exact. "Title" is the pooled copy at 0x58a144 (new_credits.cpp's .data).
void mainmenu_init()
{
    mainmenu_mouse_mode = 0;
    mainmenu_pressed_button = -1;

    if (play_intro_movies) {
        movie_play("vlogo.bik");
        if (snd_is_eax_enabled())
            movie_play("eaxlogo.bik");
        play_intro_movies = false;
    }
    music_play(music_find_by_name("Title"), 1);
}

// 0x41d910  Confidence: exact. Case 0 returns the click handler's result directly (the play menu stores
// it back into key instead).
int mainmenu_frame(int key)
{
    int i;

    for (i = 0; i < MAINMENU_NUM_BUTTONS; i++)
        mainmenu_buttons[i].flags &= ~2;

    switch (mainmenu_mouse_mode) {
    case 0:
        return mainmenu_handle_click(key);
    case 1:
        mainmenu_update_buttons();
        return key;
    case 2:
        empty_frame_hook_40acc0();
        break;
    }
    return key;
}

// 0x41d960  Confidence: exact. Key switch: byte table 0x41da78 over key-1 (0..0x3b) into the jump
// table 0x41da5c. N/S and L share the click sound tail (cross-jumping), so they are written separately.
int mainmenu_handle_click(int key)
{
    int i;

    mainmenu_pressed_button = -1;

    for (i = 0; i < MAINMENU_NUM_BUTTONS; i++) {
        ui_button_state *b = &mainmenu_buttons[i];
        ui_button_def *d = &mainmenu_button_table[i];
        if (!(b->flags & 4) && (d->flags & 1) && point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h)) {
            b->flags |= 2;
            mainmenu_pressed_button = i;
            if (mouse_was_pressed(0)) {
                mainmenu_mouse_mode = 1;
                return key;
            }
            break;
        }
    }

    switch (key) {
    case KEY_ESC:
    case KEY_Q:
        mainmenu_quit();
        return 0;
    case KEY_S:
    case KEY_N:
        mainmenu_start_new_game();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case KEY_L:
        open_saveload_screen();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case KEY_M:
        mainmenu_check_network();
        return 0;
    case KEY_F1:
        hud_key_help();
        return 0;
    case KEY_F2:
        ui_open_options();
        return 0;
    }
    return key;
}

// 0x41dac0  Confidence: exact. Starts level_script_table[3] (0x59f1e8). The second argument is the
// dword at 0x2538a10, which sum.symbols places inside conversation_save_table (entry 84); its real
// name and type are unknown.
void mainmenu_start_new_game()
{
    game_change_level(level_script_table[3].name, conversation_save_table[84]);
}

// 0x41dae0  Confidence: exact (the three message boxes share one call; the mouse mode store after it is
// common to all three, the winsock path returns before it).
void mainmenu_check_network()
{
    char *buttons[2];

    if ((nw_network_status == 1 || nw_network_status == 2) && !nw_networking_ok) {
        buttons[0] = localize_strings[213];                          // "Ok"
        buttons[1] = localize_strings[696];                          // "Cancel"
        ui_message_box_show(localize_strings[773],                   // "Network"
                            localize_strings[948],                   // "Internet connection not detected. ..."
                            -1, -1, 2, buttons, mainmenu_check_network_cb2, 0);
    } else if (nw_network_status == 0) {
        buttons[0] = localize_strings[213];
        ui_message_box_show(localize_strings[773],
                            localize_strings[774],                   // "Connection speed undefined. ..."
                            -1, -1, 1, buttons, mainmenu_check_network_cb, 2);
    } else if (nw_winsock_started) {
        gameseq_set_state(10);
        return;
    } else {
        buttons[0] = localize_strings[213];
        ui_message_box_show(localize_strings[773],
                            localize_strings[775],                   // "No network connection detected. ..."
                            -1, -1, 1, buttons, mainmenu_check_network_cb, 2);
    }
    mainmenu_mouse_mode = 2;
}

// 0x41dbc0  Confidence: exact.
void mainmenu_check_network_cb(int)
{
    mainmenu_mouse_mode = 0;
}

// 0x41dbd0  Confidence: exact. Button 1 (Cancel) returns to the menu, Ok goes on to state 10.
void mainmenu_check_network_cb2(int button)
{
    if (button == 1)
        mainmenu_mouse_mode = 0;
    else
        gameseq_set_state(10);
}

// 0x41dbf0  Confidence: exact (untagged; between mainmenu_check_network_cb2 and mainmenu_quit).
void ui_open_options()
{
    interface_set_screen(12, 0);
}

// 0x41dc00  Confidence: exact.
void mainmenu_quit()
{
    gameseq_set_state(3);
}

// 0x41dc10  Confidence: exact. Tests and acts on the Press entry after the pressed HiLite entry. The two
// "mode = 0" exits are one store in the binary; written as the early return plus the mode-1 test.
void mainmenu_update_buttons()
{
    int button = mainmenu_pressed_button + 1;

    if (mainmenu_pressed_button >= 0 && mainmenu_pressed_button < MAINMENU_NUM_BUTTONS) {
        ui_button_state *b = &mainmenu_buttons[button];
        if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
            b->flags |= 2;
        if (mouse_is_down(0))
            return;
        if (b->flags & 2)
            mainmenu_button_action(button);
        if (mainmenu_mouse_mode != 1)       // mainmenu_check_network may have set 2
            return;
    }
    mainmenu_mouse_mode = 0;
}

// 0x41dca0  Confidence: exact (jump table 0x41dd5c over button-2, 0..10; the odd entries are HiLites).
void mainmenu_button_action(int button)
{
    switch (button) {
    case 2:
        mainmenu_start_new_game();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 4:
        open_saveload_screen();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 6:
        mainmenu_check_network();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 8:
        ui_open_options();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 10:
        goto_credits();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 12:
        mainmenu_quit();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    }
}

// 0x41dd90  Confidence: exact (untagged; screen 14 draw from interface_draw).
void mainmenu_draw()
{
    int i;

    gr_set_color(255, 255, 255, 255);
    for (i = 0; i < MAINMENU_NUM_BUTTONS; i++) {
        ui_button_state *b = &mainmenu_buttons[i];
        if (b->flags & 3)
            ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
    }
}
