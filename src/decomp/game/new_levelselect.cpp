// new_levelselect.cpp - Sum.exe v1.40, code 0x41bea0-0x41d72b (screen 18, multiplayer lobby: chat, players,
// level choice)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .rdata 0x57fffc-0x5800f7 (layout tables), .data 0x58d980-0x58dd47 (button table, scrollbar names),
// .bss 0x5c1ad0-0x5c2f97. "|" (0x588c10) and the ScrollBarMid names are pooled copies in earlier units' .data.
//
// Two scrolled lists share the per-list arrays: 0 the chat lines (mp_levelselect_chat_lines, newest
// last), 1 the levels the host may pick (mp_levelselect_levels, level_table indices). The player box has
// no scrollbar. Only the host (game_flags bit 1 and mp_local_player flag bit 0) can pick a level, kick a
// player (button 11) or start (button 13).

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
#include "vsdk/os/plainstring.h"
#include "vsdk/vfile/file.h"
#include "levelscripts/level_scripts.h"
#include "summoner/multi/multi.h"
#include "summoner/interface/new_levelselect.h"

#define LEVELSELECT_NUM_BUTTONS 15
#define LEVELSELECT_CHAT_LINES  64

// A chat line (0x44 bytes). type: 0 first line of a player message (name up to the first space in
// blue), 1 its continuation lines, 2 / 3 the same for the second kind of message (name in the highlight
// colour), 4 a system line (magenta).
struct mp_chat_line {
    int type;                                                     // +0x00
    char text[0x40];                                              // +0x04
};

// ---- .rdata --------------------------------------------------------------------------------
// Per resolution_index (640, 800, 1024) and list (0 chat, 1 levels).
const int mp_levelselect_list_pos[3][2][2] = {                                // 0x57fffc x, y of the first row
    { { 191, 14 }, { 17, 252 } },
    { { 242, 17 }, { 23, 314 } },
    { { 310, 23 }, { 28, 403 } },
};
const int mp_levelselect_list_width[3][2]   = { { 420, 360 }, { 505, 360 }, { 663, 360 } };       // 0x58002c
const int mp_levelselect_list_page[3][2]    = { { 11, 13 }, { 11, 13 }, { 11, 13 } };             // 0x580044 rows
const int mp_levelselect_track_pos[3][2][2] = {                               // 0x58005c scrollbar track top
    { { 623, 51 }, { 389, 287 } },
    { { 780, 66 }, { 487, 362 } },
    { { 999, 80 }, { 623, 460 } },
};
const int mp_levelselect_track_h[3][2]      = { { 96, 122 }, { 116, 145 }, { 155, 170 } };        // 0x58008c
const int mp_levelselect_chat_box_pos[3][2]  = { { 191, 201 }, { 240, 251 }, { 308, 324 } };      // 0x5800a4
const int mp_levelselect_chat_box_size[3][2] = { { 433, 14 }, { 541, 18 }, { 693, 22 } };         // 0x5800bc
const int mp_levelselect_players_pos[3][2]   = { { 432, 254 }, { 542, 321 }, { 698, 414 } };      // 0x5800d4
const int mp_levelselect_players_width[3]    = { 184, 230, 294 };                                 // 0x5800ec

// ---- .data ---------------------------------------------------------------------------------
// 0x58d980  15 x 0x40. flags: 1 HiLite hot spot acting on release through the Press entry after it,
// 2 hot spot acting on press with auto-repeat (the four scroll arrows: 3/5 chat, 7/9 levels).
ui_button_def mp_levelselect_button_defs[LEVELSELECT_NUM_BUTTONS] = {
    { "PXO-LevelSelect",       { { 0, 0 },     { 0, 0 },     { 0, 0 } },     0 },
    { "LevelSelectBackHiLite", { { 21, 7 },    { 27, 8 },    { 26, 12 } },   1 },   // back
    { "LevelSelectBackPress",  { { 21, 7 },    { 27, 8 },    { 26, 12 } },   0 },
    { "Icon-ArrwUPHilite",     { { 621, 20 },  { 776, 25 },  { 992, 28 } },  2 },   // chat
    { "Icon-ArrwUPPress",      { { 621, 20 },  { 776, 25 },  { 992, 28 } },  0 },
    { "Icon-ArrwDNHilite",     { { 621, 159 }, { 776, 199 }, { 992, 250 } }, 2 },
    { "Icon-ArrwDNPress",      { { 621, 159 }, { 776, 199 }, { 992, 250 } }, 0 },
    { "Icon-ArrwUPHilite",     { { 387, 256 }, { 484, 320 }, { 617, 406 } }, 2 },   // levels
    { "Icon-ArrwUPPress",      { { 387, 256 }, { 484, 320 }, { 617, 406 } }, 0 },
    { "Icon-ArrwDNHilite",     { { 387, 422 }, { 484, 528 }, { 617, 671 } }, 2 },
    { "Icon-ArrwDNPress",      { { 387, 422 }, { 484, 528 }, { 617, 671 } }, 0 },
    { "LevelSelectKickHiLite", { { 440, 372 }, { 550, 465 }, { 704, 595 } }, 1 },   // kick
    { "LevelSelectKickPress",  { { 440, 372 }, { 550, 465 }, { 704, 595 } }, 0 },
    { "CreateBeginHiLite",     { { 475, 413 }, { 594, 516 }, { 814, 671 } }, 1 },   // start
    { "CreateBeginPress",      { { 475, 413 }, { 594, 516 }, { 814, 671 } }, 0 },
};
// 0x58dd40. The strings are pooled copies at 0x588b8c / 0x588b74 (see new_journal.cpp).
char *mp_levelselect_scrollbar_names[2] = { "ScrollBarMid640.tga", "ScrollBarMid1024.tga" };

// ---- .bss ----------------------------------------------------------------------------------
int mp_levelselect_pressed_button;                                // 0x5c1ad0
int mp_levelselect_hover_player;                                  // 0x5c1ad4 player row under the mouse
int mp_levelselect_list_count[2];                                 // 0x5c1ad8 [1] is sum.symbols'
                                                                  // mp_levelselect_num_levels (0x5c1adc)
bool mp_levelselect_cursor_blink;                                 // 0x5c1ae0
int mp_levelselect_drag_list;                                     // 0x5c1ae4 list under the mouse / dragged
ui_button_state mp_levelselect_buttons[LEVELSELECT_NUM_BUTTONS];  // 0x5c1ae8
int mp_levelselect_hover_level;                                   // 0x5c1c50
bool mp_levelselect_chat_active;                                  // 0x5c1c54 chat box has the focus
int mp_levelselect_drag_start_mouse_y;                            // 0x5c1c58
int mp_levelselect_thumb_w;                                       // 0x5c1c5c scrollbar tile size
int mp_levelselect_thumb_h;                                       // 0x5c1c60
int mp_levelselect_selected_row[2];                               // 0x5c1c64 per list; sum.symbols names
                                                                  // element [1] (0x5c1c68)
RealtimeTimer mp_levelselect_blink_timer;                         // 0x5c1c6c 500 ms cursor blink
int mp_levelselect_levels[51];                                    // 0x5c1c70 (26 used: level_table size)
char mp_levelselect_selected_player[32];                          // 0x5c1d3c
RealtimeTimer mp_levelselect_button_timer;                        // 0x5c1d5c button auto-repeat
mp_chat_line mp_levelselect_chat_lines[LEVELSELECT_CHAT_LINES];   // 0x5c1d60
char mp_levelselect_chat_input[256];                              // 0x5c2e60
int mp_levelselect_scrollbar_bm;                                  // 0x5c2f60
int mp_levelselect_drag_start_thumb_y;                            // 0x5c2f64
bool mp_levelselect_scrollbar_visible[2];                         // 0x5c2f68
int mp_levelselect_mouse_mode;                                    // 0x5c2f6c 0 idle, 1 button held, 2 dragging
int mp_levelselect_scroll_pos[2];                                 // 0x5c2f70
int mp_levelselect_thumb_pos[2][2];                               // 0x5c2f78 thumb x, y
int mp_levelselect_thumb_size[2][2];                              // 0x5c2f88 thumb w, h

// init_realtimetimer_5c1c6c_thunk (0x41bea0, "jmp 0x41beb0") and init_realtimetimer_5c1c6c (0x41beb0,
// "mov ecx, 0x5c1c6c / jmp RealtimeTimer__RealtimeTimer") are the compiler's dynamic initialiser pair for
// mp_levelselect_blink_timer (static_initializers entry 57); init_realtimetimer_5c1d5c_thunk (0x41bec0) and
// init_realtimetimer_5c1d5c (0x41bed0) the pair for mp_levelselect_button_timer (entry 58). Confidence:
// exact (no source of their own).
// 0x41bea0 init_realtimetimer_5c1c6c_thunk (jmp, entry 57). Confidence: exact.
// 0x41beb0 init_realtimetimer_5c1c6c (mp_levelselect_blink_timer). Confidence: exact.
// 0x41bec0 init_realtimetimer_5c1d5c_thunk (jmp, entry 58). Confidence: exact.
// 0x41bed0 init_realtimetimer_5c1d5c (mp_levelselect_button_timer). Confidence: exact.

// 0x41bee0  Confidence: exact. Same loader as gamecreate_load_bitmaps (index loop, entry 0 always drawn);
// the scrollbar tile is scaled without rounding.
void mp_levelselect_load_bitmaps()
{
    float scale = ui_get_scale();
    int i;

    for (i = 0; i < LEVELSELECT_NUM_BUTTONS; i++) {
        ui_button_state *b = &mp_levelselect_buttons[i];
        ui_button_def *d = &mp_levelselect_button_defs[i];
        b->flags = 0;
        b->bitmap = -1;
        b->x = 0;
        b->y = 0;
        b->w = 0;
        b->h = 0;

        if (strlen(d->name)) {
            char filename[128];

            strcpy(filename, d->name);
            strcat(filename, ui_bitmap_suffix_tga[ui_hires]);
            b->bitmap = bm_load(filename, path_interface, -1);
            b->x = d->pos[resolution_index][0];
            b->y = d->pos[resolution_index][1];
            bm_get_dimensions(b->bitmap, &b->w, &b->h);
            b->w = (int)(b->w * scale + 0.5f);
            b->h = (int)(b->h * scale + 0.5f);
            if (i == 0)
                b->flags |= 1;
        }
    }

    mp_levelselect_scrollbar_bm = bm_load(mp_levelselect_scrollbar_names[ui_hires], path_interface, -1);
    bm_get_dimensions(mp_levelselect_scrollbar_bm, &mp_levelselect_thumb_w, &mp_levelselect_thumb_h);
    mp_levelselect_thumb_w = (int)(mp_levelselect_thumb_w * scale);
    mp_levelselect_thumb_h = (int)(mp_levelselect_thumb_h * scale);
}

// 0x41c080  Confidence: exact. Start (13) is for the host only. Clearing the chat count drops the chat
// history of the previous game.
void mp_levelselect_init()
{
    int i;

    if ((game_flags & 2) && mp_local_player && (mp_local_player->flags & 1))
        mp_levelselect_buttons[13].flags &= ~4;
    else
        mp_levelselect_buttons[13].flags |= 4;

    for (i = 0; i < 2; i++) {
        if (i == 1)
            mp_levelselect_build_list();
        else
            mp_levelselect_list_count[i] = 0;
        mp_levelselect_scroll_pos[i] = 0;
        mp_levelselect_selected_row[i] = -1;
        mp_levelselect_update_scrollbar(i);
    }

    mp_levelselect_pressed_button = -1;
    mp_levelselect_drag_list = -1;
    mp_levelselect_hover_player = -1;
    mp_levelselect_chat_input[0] = 0;
    mp_levelselect_mouse_mode = 0;
    mp_levelselect_selected_player[0] = 0;
}

// folded (0x4f13e0, empty_function): the screen's leave function is empty; the linker folded it into
// empty_function (identical-code folding). interface_set_screen calls each by name (D7903); name ours.
void levelselect_leave()
{
}

// folded (0x4063a0, ui_return_true): the screen has no panel of its own, so its mouse_in_panel returns true;
// the linker folded the identical copies into one (identical-code folding). interface_mouse_over_panel
// calls each by name (D7903); name ours.
bool levelselect_mouse_in_panel(int x, int y)
{
    return true;
}

// 0x41c110  Confidence: exact (VC6 byte match, match9; was likely: same template as mp_charselect_update_scrollbar).
// Buttons 3 + 4 * list /
// 5 + 4 * list are the list's arrows.
void mp_levelselect_update_scrollbar(int list)
{
    if (list == -1)
        return;

    int count = mp_levelselect_list_count[list];
    int page = mp_levelselect_list_page[resolution_index][list];

    if (count <= page) {
        mp_levelselect_scrollbar_visible[list] = false;
        mp_levelselect_buttons[3 + list * 4].flags |= 4;
        mp_levelselect_buttons[5 + list * 4].flags |= 4;
        return;
    }

    mp_levelselect_scrollbar_visible[list] = true;
    mp_levelselect_buttons[3 + list * 4].flags &= ~4;
    mp_levelselect_buttons[5 + list * 4].flags &= ~4;

    if (count <= 0) {
        mp_levelselect_thumb_pos[list][0] = mp_levelselect_track_pos[resolution_index][list][0];
        mp_levelselect_thumb_pos[list][1] = mp_levelselect_track_pos[resolution_index][list][1];
        mp_levelselect_thumb_size[list][0] = mp_levelselect_thumb_w;
        mp_levelselect_thumb_size[list][1] = mp_levelselect_track_h[resolution_index][list];
    } else {
        mp_levelselect_thumb_pos[list][0] = mp_levelselect_track_pos[resolution_index][list][0];
        mp_levelselect_thumb_pos[list][1] = (int)(mp_levelselect_track_h[resolution_index][list]
                                                  * ((float)mp_levelselect_scroll_pos[list] / count) + 0.5f)
                                            + mp_levelselect_track_pos[resolution_index][list][1];
        mp_levelselect_thumb_size[list][0] = mp_levelselect_thumb_w;
        mp_levelselect_thumb_size[list][1] = (int)(mp_levelselect_track_h[resolution_index][list]
                                                   * ((float)page / count) + 0.5f);
    }
}

// 0x41c250  Confidence: exact (VC6 byte match, match9; was likely: the counter is a register; the first store of 0 is
// kept as a separate
// statement). The levels with bit 0 of their mp flags, or every level with All_multi_levels.
void mp_levelselect_build_list()
{
    int i;

    mp_levelselect_list_count[1] = 0;
    for (i = 0; i < 26; i++) {
        if (All_multi_levels || (level_table[i].mp_flags & 1))
            mp_levelselect_levels[mp_levelselect_list_count[1]++] = i;
    }
    mp_levelselect_update_scrollbar(1);
}

// 0x41c2a0  Confidence: exact.
int mp_levelselect_frame(int key)
{
    int i;

    for (i = 0; i < LEVELSELECT_NUM_BUTTONS; i++)
        mp_levelselect_buttons[i].flags &= ~2;

    mp_levelselect_sync_selection();
    mp_levelselect_update_kick_button();

    if (mp_levelselect_blink_timer.elapsed()) {
        mp_levelselect_blink_timer.SetTimeout(500);
        mp_levelselect_cursor_blink = !mp_levelselect_cursor_blink;
    }

    if (ui_message_box_is_active())
        return key;

    switch (mp_levelselect_mouse_mode) {
    case 0:
        key = mp_levelselect_handle_click(key);
        break;
    case 1:
        mp_levelselect_update_buttons();
        break;
    case 2:
        mp_levelselect_drag_scrollbar();
        break;
    }
    return key;
}

// 0x41c330  Confidence: exact. The level row shows the netgame level when every player may play it.
void mp_levelselect_sync_selection()
{
    int i;

    mp_levelselect_selected_row[1] = -1;
    if (mp_level_available_to_all(mp_netgame_level, mp_game_difficulty)) {
        for (i = 0; i < mp_levelselect_list_count[1]; i++) {
            if (mp_levelselect_levels[i] == mp_netgame_level) {
                mp_levelselect_selected_row[1] = i;
                break;
            }
        }
    }
}

// 0x41c380  Confidence: exact (VC6 byte match, match24; was likely) (the bool local for "a name is selected"). Forgets a selected player who
// has left; Kick (11) is for the host, with another player selected.
void mp_levelselect_update_kick_button()
{
    mp_player *p = 0;
    bool selected = strlen(mp_levelselect_selected_player) != 0;

    if (selected) {
        for (p = MP_PLAYER_NEXT(&mp_player_list); p != &mp_player_list; p = MP_PLAYER_NEXT(p)) {
            if (MP_PLAYER_TYPE(p) == 3 && !strcmp(MP_PLAYER_NAME(p), mp_levelselect_selected_player))
                break;
        }
        if (p == &mp_player_list) {
            mp_levelselect_selected_player[0] = 0;
            selected = false;
        }
    }

    if ((game_flags & 2) && mp_local_player && (mp_local_player->flags & 1) && selected && p != mp_local_player)
        mp_levelselect_buttons[11].flags &= ~4;
    else
        mp_levelselect_buttons[11].flags |= 4;
}

// 0x41c450  Confidence: likely (the thumb test breaks out of its loop when the mouse is over a thumb but
// not pressed; the level rows of the host only; ESC only while the chat box has no focus, so the ESC
// switch sits in the else branch). The player rows advance by one line for every listed player but
// mp_levelselect_draw_players only for type-3 players, so the hit rows can differ from the drawn ones.
int mp_levelselect_handle_click(int key)
{
    int list;
    int font_h;
    int i;

    mp_levelselect_pressed_button = -1;
    mp_levelselect_drag_list = -1;
    mp_levelselect_hover_level = -1;
    mp_levelselect_hover_player = -1;

    for (i = 0; i < LEVELSELECT_NUM_BUTTONS; i++) {
        ui_button_state *b = &mp_levelselect_buttons[i];
        ui_button_def *d = &mp_levelselect_button_defs[i];
        if (!(b->flags & 4) && (d->flags & 3)
            && point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y,
                             b->w, b->h)) {
            mp_levelselect_pressed_button = i;
            b->flags |= 2;
            if (d->flags & 1) {
                if (mouse_was_pressed(0)) {
                    mp_levelselect_mouse_mode = 1;
                    mp_levelselect_button_timer.invalidate();
                    return key;
                }
            } else if (d->flags & 2) {
                if (mouse_was_pressed(0)) {
                    mp_levelselect_mouse_mode = 1;
                    mp_levelselect_button_action(mp_levelselect_pressed_button + 1);
                    mp_levelselect_button_timer.SetTimeout(500);
                    return key;
                }
            }
            break;
        }
    }

    for (i = 0; i < 2; i++) {
        if (point_in_rect(game_mouse_x, game_mouse_y, mp_levelselect_thumb_pos[i][0], mp_levelselect_thumb_pos[i][1],
                          mp_levelselect_thumb_size[i][0], mp_levelselect_thumb_size[i][1])) {
            if (mouse_was_pressed(0)) {
                mp_levelselect_drag_list = i;
                mp_levelselect_mouse_mode = 2;
                mp_levelselect_drag_start_thumb_y = mp_levelselect_thumb_pos[i][1];
                mp_levelselect_drag_start_mouse_y = game_mouse_y;
                return key;
            }
            break;
        }
    }

    font_h = gr_get_font_height(-1);
    for (list = 0; list < 2; list++) {
        int rows = mp_levelselect_list_count[list];
        if (rows >= mp_levelselect_list_page[resolution_index][list])
            rows = mp_levelselect_list_page[resolution_index][list];

        if (point_in_rect(game_mouse_x, game_mouse_y, mp_levelselect_list_pos[resolution_index][list][0],
                          mp_levelselect_list_pos[resolution_index][list][1],
                          mp_levelselect_list_width[resolution_index][list], rows * font_h)) {
            mp_levelselect_drag_list = list;
            if (list == 1 && (game_flags & 2) && mp_local_player && (mp_local_player->flags & 1)) {
                int row, y;

                for (row = 0, y = 0; row < rows; row++, y += font_h) {
                    if (mp_level_available_to_all(mp_levelselect_levels[mp_levelselect_scroll_pos[1] + row],
                                                  mp_game_difficulty)
                        && point_in_rect(game_mouse_x, game_mouse_y, mp_levelselect_list_pos[resolution_index][1][0],
                                         mp_levelselect_list_pos[resolution_index][1][1] + y,
                                         mp_levelselect_list_width[resolution_index][1], font_h)) {
                        mp_levelselect_hover_level = row;
                        if (mouse_was_double_clicked(0)) {
                            mp_levelselect_set_level(mp_levelselect_levels[mp_levelselect_scroll_pos[1] + row]);
                            mp_levelselect_start_thunk();
                            game_play_sound(0x23, 1.0f, 0, 0, 0);
                            return key;
                        }
                        if (mouse_was_pressed(0)) {
                            mp_levelselect_set_level(mp_levelselect_levels[mp_levelselect_scroll_pos[1] + row]);
                            return key;
                        }
                        break;
                    }
                }
            }
        }
    }

    if (point_in_rect(game_mouse_x, game_mouse_y, mp_levelselect_players_pos[resolution_index][0],
                      mp_levelselect_players_pos[resolution_index][1], mp_levelselect_players_width[resolution_index],
                      font_h * 4)) {
        mp_player *p;
        int row, y;

        for (p = MP_PLAYER_NEXT(&mp_player_list), row = 0, y = 0; p != &mp_player_list;
             p = MP_PLAYER_NEXT(p), row++, y += font_h) {
            if (MP_PLAYER_TYPE(p) == 3
                && point_in_rect(game_mouse_x, game_mouse_y, mp_levelselect_players_pos[resolution_index][0],
                                 mp_levelselect_players_pos[resolution_index][1] + y,
                                 mp_levelselect_players_width[resolution_index], font_h)) {
                mp_levelselect_hover_player = row;
                if (mouse_was_pressed(0)) {
                    strcpy(mp_levelselect_selected_player, MP_PLAYER_NAME(p));
                    return key;
                }
                break;
            }
        }
    }

    if (mp_levelselect_chat_active) {
        if (key) {
            int c = key_to_ascii(key);
            if (c != 0xff) {
                int len = strlen(mp_levelselect_chat_input);
                if (len < 255) {
                    mp_levelselect_chat_input[len] = (char)c;
                    mp_levelselect_chat_input[len + 1] = 0;
                    mp_levelselect_blink_timer.SetTimeout(500);
                    mp_levelselect_cursor_blink = true;
                    return 0;
                }
            } else {
                switch (key) {
                case KEY_ESC:
                    mp_levelselect_chat_input[0] = 0;
                    mp_levelselect_chat_active = false;
                    return 0;
                case KEY_BACKSPACE: {
                    int len = strlen(mp_levelselect_chat_input);
                    if (len > 0) {
                        mp_levelselect_chat_input[len - 1] = 0;
                        mp_levelselect_blink_timer.SetTimeout(500);
                        mp_levelselect_cursor_blink = true;
                        return 0;
                    }
                    break;
                }
                case KEY_ENTER: {
                    char line[256];
                    trim_whitespace(line, mp_levelselect_chat_input, -1);
                    if (strlen(line))
                        chat_send_line(line);
                    mp_levelselect_chat_input[0] = 0;
                    return 0;
                }
                }
            }
        }
        if (!mp_levelselect_chat_active)
            return 0;
    } else {
        if (mouse_was_pressed(0)
            && point_in_rect(game_mouse_x, game_mouse_y, mp_levelselect_chat_box_pos[resolution_index][0],
                             mp_levelselect_chat_box_pos[resolution_index][1],
                             mp_levelselect_chat_box_size[resolution_index][0],
                             mp_levelselect_chat_box_size[resolution_index][1])) {
            mp_levelselect_chat_input[0] = 0;
            mp_levelselect_blink_timer.SetTimeout(500);
            mp_levelselect_cursor_blink = true;
            mp_levelselect_chat_active = true;
            return key;
        }
        switch (key) {
        case KEY_ESC:
            mp_levelselect_back();
            game_play_sound(0x24, 1.0f, 0, 0, 0);
            return 0;
        }
    }

    {
        int dx, dy, dz;
        mouse_get_delta(&dx, &dy, &dz);
        if (dz > 0)
            mp_levelselect_page_up(mp_levelselect_drag_list);
        else if (dz < 0)
            mp_levelselect_page_down(mp_levelselect_drag_list);
    }
    return key;
}

// 0x41cad0  Confidence: exact (VC6 byte match, match24; was likely) (two calls merged by the compiler, or one call with a ?: argument).
// Leaves the game (the host passes 2) and returns to state 11 or 12 like mp_charselect_save_choice.
void mp_levelselect_back()
{
    if ((game_flags & 2) && mp_local_player && (mp_local_player->flags & 1))
        mp_player_disconnected(2, 0);
    else
        mp_player_disconnected(0, 0);
    if (pxo_cfg_flags & 1)
        gameseq_set_state(11);
    else
        gameseq_set_state(12);
}

// 0x41cb20  Confidence: exact (VC6 byte match, match24; was likely) (two stores of the element, as journal_list_page_up).
void mp_levelselect_page_up(int list)
{
    if (list == -1)
        return;
    if (mp_levelselect_scroll_pos[list] > 0) {
        mp_levelselect_scroll_pos[list] -= mp_levelselect_list_page[resolution_index][list];
        mp_levelselect_scroll_pos[list] = mp_levelselect_scroll_pos[list] > 0 ? mp_levelselect_scroll_pos[list] : 0;
        mp_levelselect_update_scrollbar(list);
    }
}

// 0x41cb70  Confidence: exact (VC6 byte match, match24; was likely) (as journal_list_page_down).
void mp_levelselect_page_down(int list)
{
    if (list == -1)
        return;
    int last = mp_levelselect_list_count[list] - mp_levelselect_list_page[resolution_index][list];
    if (mp_levelselect_scroll_pos[list] < last) {
        mp_levelselect_scroll_pos[list] += mp_levelselect_list_page[resolution_index][list];
        mp_levelselect_scroll_pos[list] = mp_levelselect_scroll_pos[list] < last ? mp_levelselect_scroll_pos[list] : last;
        mp_levelselect_update_scrollbar(list);
    }
}

// 0x41cbc0  Confidence: exact ("jmp mp_levelselect_start_if_ready": a tail call).
void mp_levelselect_start_thunk()
{
    mp_levelselect_start_if_ready();
}

// 0x41cbd0  Confidence: exact (jump table 0x41cc54 over button-2, 0..12; the scroll cases share the call:
// push 0/1, jmp; kick and start share the sound).
void mp_levelselect_button_action(int button)
{
    switch (button) {
    case 2:
        mp_levelselect_back();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    case 4:
        mp_levelselect_scroll_up(0);
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 6:
        mp_levelselect_scroll_down(0);
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 8:
        mp_levelselect_scroll_up(1);
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 10:
        mp_levelselect_scroll_down(1);
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 12:
        mp_levelselect_kick_selected_player();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 14:
        mp_levelselect_start_thunk();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    }
}

// 0x41cc90  Confidence: exact.
void mp_levelselect_scroll_up(int list)
{
    if (list == -1)
        return;
    if (mp_levelselect_scroll_pos[list] > 0) {
        mp_levelselect_scroll_pos[list]--;
        mp_levelselect_update_scrollbar(list);
    }
}

// 0x41ccc0  Confidence: exact.
void mp_levelselect_scroll_down(int list)
{
    if (list == -1)
        return;
    if (mp_levelselect_scroll_pos[list] < mp_levelselect_list_count[list]
                                          - mp_levelselect_list_page[resolution_index][list]) {
        mp_levelselect_scroll_pos[list]++;
        mp_levelselect_update_scrollbar(list);
    }
}

// 0x41cd10  Confidence: exact. Kicks (reason 1) the listed player with the selected name.
void mp_levelselect_kick_selected_player()
{
    mp_player *p;

    for (p = MP_PLAYER_NEXT(&mp_player_list); p != &mp_player_list; p = MP_PLAYER_NEXT(p)) {
        if (MP_PLAYER_TYPE(p) == 3 && !strcmp(MP_PLAYER_NAME(p), mp_levelselect_selected_player)) {
            mp_remove_player(p, 1);
            return;
        }
    }
}

// 0x41cd80  Confidence: exact. Always the entry after the hot spot acts (no flag test, unlike
// mp_charselect_update_buttons); the range test comes first here.
void mp_levelselect_update_buttons()
{
    int button = mp_levelselect_pressed_button + 1;

    if (mp_levelselect_pressed_button >= 0 && mp_levelselect_pressed_button < LEVELSELECT_NUM_BUTTONS) {
        ui_button_state *b = &mp_levelselect_buttons[button];
        if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
            b->flags |= 2;

        if (!mouse_is_down(0)) {
            if ((b->flags & 2) && !mp_levelselect_button_timer.valid())
                mp_levelselect_button_action(button);
        } else {
            if (mp_levelselect_button_timer.valid() && mp_levelselect_button_timer.elapsed() && (b->flags & 2)) {
                mp_levelselect_button_action(button);
                mp_levelselect_button_timer.SetTimeout(100);
            }
            return;
        }
    }
    mp_levelselect_mouse_mode = 0;
}

// 0x41ce50  Confidence: likely (three stores of the element, as mp_charselect_drag_scrollbar).
void mp_levelselect_drag_scrollbar()
{
    int list = mp_levelselect_drag_list;
    int count = mp_levelselect_list_count[list];

    mp_levelselect_scroll_pos[list] = (int)((float)(game_mouse_y - mp_levelselect_track_pos[resolution_index][list][1]
                                                    - mp_levelselect_drag_start_mouse_y
                                                    + mp_levelselect_drag_start_thumb_y)
                                            / mp_levelselect_track_h[resolution_index][list] * count + 0.5f);
    mp_levelselect_scroll_pos[list] = mp_levelselect_scroll_pos[list] > 0 ? mp_levelselect_scroll_pos[list] : 0;
    count -= mp_levelselect_list_page[resolution_index][list];
    mp_levelselect_scroll_pos[list] = mp_levelselect_scroll_pos[list] < count ? mp_levelselect_scroll_pos[list] : count;
    mp_levelselect_update_scrollbar(list);

    if (!mouse_is_down(0))
        mp_levelselect_mouse_mode = 0;
}

// 0x41cf00  Confidence: exact.
void mp_levelselect_draw()
{
    int i;

    gr_set_color(255, 255, 255, 255);
    for (i = 0; i < LEVELSELECT_NUM_BUTTONS; i++) {
        ui_button_state *b = &mp_levelselect_buttons[i];
        if (b->flags & 3)
            ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
    }
    mp_levelselect_draw_chat();
    mp_levelselect_draw_levels();
    for (i = 0; i < 2; i++)
        mp_levelselect_draw_scrollbar(i);
    mp_levelselect_draw_chat_input();
    mp_levelselect_draw_players();
}

// 0x41cf80  Confidence: exact (VC6 byte match, match24; was unsure) (the colour switch of the other line types: the binary tests 1 and 3
// explicitly and gives them and the default the text colour, 4 magenta; written as a switch with those
// cases. The two name buffers are separate stack slots, 0x14 and 0x54, so each branch declares its own).
// A first line is split after the first space: the name part (space included) in blue or the highlight
// colour, the rest in the text colour after it.
void mp_levelselect_draw_chat()
{
    int rows;
    int w;
    int i;
    int h;
    int font_h;
    int x, y;

    font_h = gr_get_font_height(-1);
    rows = mp_levelselect_list_count[0];
    if (rows >= mp_levelselect_list_page[resolution_index][0])
        rows = mp_levelselect_list_page[resolution_index][0];
    x = mp_levelselect_list_pos[resolution_index][0][0];
    y = mp_levelselect_list_pos[resolution_index][0][1];

    for (i = 0; i < rows; i++) {
        mp_chat_line *line = &mp_levelselect_chat_lines[mp_levelselect_scroll_pos[0] + i];

        if (line->type == 0) {
            char name[64];
            char *space = strchr(line->text, ' ');
            if (space) {
                int n = space - line->text + 1;   // the space is part of the name
                strncpy(name, line->text, n);
                name[n] = 0;
                gr_get_string_size(&w, &h, name, -1, -1);
                gr_set_color_ptr(&ui_color_blue);
                gr_string(x, y, name, -1, gr_string_mode);
                gr_set_color_ptr(&ui_color_text);
                gr_string(x + w, y, line->text + n, -1, gr_string_mode);
            } else {
                gr_set_color_ptr(&ui_color_blue);
                gr_string(x, y, line->text, -1, gr_string_mode);
            }
        } else if (line->type == 2) {
            char name[64];
            char *space = strchr(line->text, ' ');
            if (space) {
                int n = space - line->text + 1;   // the space is part of the name
                strncpy(name, line->text, n);
                name[n] = 0;
                gr_get_string_size(&w, &h, name, -1, -1);
                gr_set_color_ptr(&ui_color_highlight);
                gr_string(x, y, name, -1, gr_string_mode);
                gr_set_color_ptr(&ui_color_text);
                gr_string(x + w, y, line->text + n, -1, gr_string_mode);
            } else {
                gr_set_color_ptr(&ui_color_blue);
                gr_string(x, y, line->text, -1, gr_string_mode);
            }
        } else {
            switch (line->type) {
            case 1:
            case 3:
                gr_set_color_ptr(&ui_color_text);
                break;
            case 4:
                gr_set_color_ptr(&ui_color_magenta);
                break;
            default:
                gr_set_color_ptr(&ui_color_text);
                break;
            }
            gr_string(x, y, line->text, -1, gr_string_mode);
        }
        y += font_h;
    }
}

// 0x41d1d0  Confidence: exact (VC6 byte match, m-1020 w9; was likely) (the three-way colour and the "Unknown" fallback as if / else with
// merged calls). Levels some player may not play yet are greyed.
void mp_levelselect_draw_levels()
{
    int rows;
    int font_h;
    int x, y, i;

    font_h = gr_get_font_height(-1);
    rows = mp_levelselect_list_count[1] < mp_levelselect_list_page[resolution_index][1] ? mp_levelselect_list_count[1] : mp_levelselect_list_page[resolution_index][1];
    x = mp_levelselect_list_pos[resolution_index][1][0];
    y = mp_levelselect_list_pos[resolution_index][1][1];

    for (i = 0; i < rows; i++) {
        int row = mp_levelselect_scroll_pos[1] + i;
        char *name;

        if ((mp_levelselect_drag_list == 1 && i == mp_levelselect_hover_level) || row == mp_levelselect_selected_row[1])
            gr_set_color_ptr(&ui_color_highlight);
        else if (mp_level_available_to_all(mp_levelselect_levels[row], mp_game_difficulty))
            gr_set_color_ptr(&ui_color_text);
        else
            gr_set_color_ptr(&ui_color_disabled);

        name = level_table_display_name(mp_levelselect_levels[row]);
        if (name)
            gr_string(x, y, name, -1, gr_string_mode);
        else
            gr_string(x, y, XSTR("Unknown", 352), -1, gr_string_mode);
        y += font_h;
    }
}

// 0x41d2d0  Confidence: exact (VC6 byte match, match24; was likely) (as mp_charselect_draw_scrollbar).
void mp_levelselect_draw_scrollbar(int list)
{
    int tiles, i;

    if (list == -1 || !mp_levelselect_scrollbar_visible[list])
        return;

    gr_set_color(255, 255, 255, 255);
    tiles = mp_levelselect_thumb_size[list][1] / mp_levelselect_thumb_h;
    for (i = 0; i < tiles; i++)
        ui_bitmap_scaled(mp_levelselect_scrollbar_bm, mp_levelselect_thumb_pos[list][0],
                         mp_levelselect_thumb_h * i + mp_levelselect_thumb_pos[list][1], gr_bitmap_wrap_mode);
    ui_bitmap_scaled(mp_levelselect_scrollbar_bm, mp_levelselect_thumb_pos[list][0],
                     mp_levelselect_thumb_pos[list][1] - mp_levelselect_thumb_h + mp_levelselect_thumb_size[list][1],
                     gr_bitmap_wrap_mode);
}

// 0x41d390  Confidence: exact (VC6 byte match, match24; was likely) (local order: clip y 0x00, w 0x04, clip x 0x08, clip w 0x0c, clip h 0x10,
// h 0x14, buf 0x18). Drawn only while the box has the focus; "|" is always appended, the blink chooses
// between the copy with it and the bare text.
void mp_levelselect_draw_chat_input()
{
    int clip_y;
    int w;
    int clip_x, clip_w, clip_h;
    int h;
    char buf[260];
    int x;

    if (!mp_levelselect_chat_active)
        return;

    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    gr__set_clip(mp_levelselect_chat_box_pos[resolution_index][0], mp_levelselect_chat_box_pos[resolution_index][1],
                 mp_levelselect_chat_box_size[resolution_index][0], mp_levelselect_chat_box_size[resolution_index][1]);

    strcpy(buf, mp_levelselect_chat_input);
    strcat(buf, "|");
    gr_get_string_size(&w, &h, buf, -1, -1);

    x = mp_levelselect_chat_box_size[resolution_index][0] - w;
    x = x > 0 ? 0 : x;
    gr_set_color_ptr(&ui_color_text);
    if (mp_levelselect_cursor_blink)
        gr_string(x, 0, buf, -1, gr_string_mode);
    else
        gr_string(x, 0, mp_levelselect_chat_input, -1, gr_string_mode);

    gr__set_clip(clip_x, clip_y, clip_w, clip_h);
}

// 0x41d4e0  Confidence: exact (VC6 byte match, match24; was likely) (local order: row 0x00, font_h 0x04, text 0x08). The type-3 players, the
// one under the mouse or selected in the highlight colour.
void mp_levelselect_draw_players()
{
    int row;
    int font_h;
    char text[64];
    int x, y;
    mp_player *p;

    x = mp_levelselect_players_pos[resolution_index][0];
    y = mp_levelselect_players_pos[resolution_index][1];
    font_h = gr_get_font_height(-1);
    row = 0;

    for (p = MP_PLAYER_NEXT(&mp_player_list); p != &mp_player_list; p = MP_PLAYER_NEXT(p), row++) {
        if (MP_PLAYER_TYPE(p) == 3) {
            if (row == mp_levelselect_hover_player || !strcmp(MP_PLAYER_NAME(p), mp_levelselect_selected_player))
                gr_set_color_ptr(&ui_color_highlight);
            else
                gr_set_color_ptr(&ui_color_text);
            gr_string_truncate_ellipsis(text, MP_PLAYER_NAME(p), mp_levelselect_players_width[resolution_index], -1);
            gr_string(x, y, text, -1, gr_string_mode);
            y += font_h;
        }
    }
}

// 0x41d5e0  Confidence: exact (VC6 byte match, m-1020 w9; was likely) (the memmove size, count * 0x44 - excess * 0x44 in the binary; the type
// switch inside the loop). Called by chat_history_pump. A message with a wrapped line longer than 63
// characters is dropped; the oldest lines make room; the view jumps to the newest lines.
void mp_levelselect_add_chat_line(const char *text, int type)
{
    int lengths[16];
    int starts[16];
    int count, excess, i;

    count = gr_split_str(lengths, starts, text, mp_levelselect_list_width[resolution_index][0], 16, 0, -1);
    for (i = 0; i < count; i++) {
        if (lengths[i] > 63)
            return;
    }

    excess = mp_levelselect_list_count[0] + count - LEVELSELECT_CHAT_LINES;
    if (excess > 0) {
        memmove(mp_levelselect_chat_lines, &mp_levelselect_chat_lines[excess],
                (mp_levelselect_list_count[0] - excess) * sizeof(mp_chat_line));
        mp_levelselect_list_count[0] -= excess;
    }

    for (i = 0; i < count; i++) {
        mp_chat_line *line = &mp_levelselect_chat_lines[mp_levelselect_list_count[0]];

        switch (type) {
        case 0:
        case 1:
            line->type = i != 0;
            break;
        case 2:
        case 3:
            line->type = (i != 0) + 2;
            break;
        case 4:
            line->type = 4;
            break;
        default:
            return;
        }
        strncpy(line->text, text + starts[i], lengths[i]);
        line->text[lengths[i]] = 0;
        mp_levelselect_list_count[0]++;
    }

    mp_levelselect_scroll_pos[0] = mp_levelselect_list_count[0] - mp_levelselect_list_page[resolution_index][0];
    mp_levelselect_scroll_pos[0] &= (mp_levelselect_scroll_pos[0] < 0) - 1;
    mp_levelselect_update_scrollbar(0);
}
