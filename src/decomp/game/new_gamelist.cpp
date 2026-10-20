// new_gamelist.cpp - Sum.exe v1.40, code 0x40a660-0x40bf2b (screen 15, the multiplayer game list)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .rdata 0x57ecc8-0x57edcf (layout tables), .data 0x58a7a0-0x58ac47 (button table, scrollbar names,
// strings), .bss 0x5bc710-0x5bc918. The ScrollBarMid names (0x588b8c / 0x588b74) and "|" (0x588c10) are
// pooled copies in earlier units' .data.

#include <stdio.h>
#include <string.h>
#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/key.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/localize/localize.h"
#include "vsdk/net/net.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "engine/gamesound/gamesound.h"
#include "levelscripts/level_scripts.h"
#include "summoner/multi/multi.h"
#include "summoner/interface/new_charselect.h"
#include "summoner/interface/new_gamelist.h"

#define GAMELIST_NUM_BUTTONS 18

// ---- .rdata (per resolution_index: 640, 800, 1024; names ours) -------------------------------------
const int gamelist_header_pos[3][2]        = { { 16, 80 }, { 20, 97 }, { 26, 128 } };     // 0x57ecc8 column titles
const int gamelist_list_pos[3][2]          = { { 16, 100 }, { 20, 122 }, { 26, 160 } };   // 0x57ece0 first row
const int gamelist_list_width[3]           = { 587, 734, 939 };                           // 0x57ecf8
const int gamelist_page_rows[3]            = { 20, 20, 20 };                              // 0x57ed04
const int gamelist_col_name[3]             = { 0, 0, 0 };                                 // 0x57ed10 column x
const int gamelist_col_level[3]            = { 168, 209, 269 };                           // 0x57ed1c
const int gamelist_col_players[3]          = { 330, 411, 528 };                           // 0x57ed28
const int gamelist_col_difficulty[3]       = { 380, 473, 608 };                           // 0x57ed34
const int gamelist_col_exp[3]              = { 435, 542, 696 };                           // 0x57ed40
const int gamelist_col_server[3]           = { 491, 627, 800 };                           // 0x57ed4c ("Svr" title)
const int gamelist_col_server_centre[3]    = { 508, 646, 826 };                           // 0x57ed58 (connection text)
const int gamelist_col_ping[3]             = { 544, 691, 882 };                           // 0x57ed64
const int gamelist_col_gap[3]              = { 5, 6, 8 };                                 // 0x57ed70 truncation margin
const int gamelist_track_pos[3][2]         = { { 616, 117 }, { 770, 146 }, { 986, 187 } };  // 0x57ed7c scrollbar track
const int gamelist_track_h[3]              = { 247, 309, 395 };                           // 0x57ed94
const int gamelist_password_pos[3][2]      = { { 186, 232 }, { 232, 290 }, { 298, 371 } };  // 0x57eda0 clip rect
const int gamelist_password_size[3][2]     = { { 269, 27 }, { 269, 32 }, { 269, 43 } };     // 0x57edb8

// ---- .data ---------------------------------------------------------------------------------------
// 0x58a7a0  18 x 0x40 (sum.symbols gamelist_button_def0_flags = 0x58a7dc, the flags of entry 0). flags:
// 1 HiLite that acts through the Press entry after it on release, 2 acts (Press entry) on press and
// auto-repeats, 4 belongs to the password popup (reacts and is drawn only while it is open), 8 has a
// tooltip (Refresh). Entries 0 (background) and 13 (popup) are always drawn; 7 (Join) is disabled while
// no game is selected, 1 / 3 (arrows) while the list fits on one page.
ui_button_def gamelist_button_defs[GAMELIST_NUM_BUTTONS] = {
    { "PXO-GameList",          { { 0, 0 },     { 0, 0 },     { 0, 0 } },     0 },
    { "Icon-ArrwUPHilite",     { { 614, 85 },  { 768, 103 }, { 981, 133 } }, 2 },   // scroll up
    { "Icon-ArrwUPPress",      { { 614, 85 },  { 768, 103 }, { 981, 133 } }, 0 },
    { "Icon-ArrwDNHilite",     { { 614, 376 }, { 768, 470 }, { 981, 598 } }, 2 },   // scroll down
    { "Icon-ArrwDNPress",      { { 614, 376 }, { 768, 470 }, { 981, 598 } }, 0 },
    { "GameListRefreshHiLite", { { 19, 409 },  { 24, 511 },  { 27, 655 } },  9 },   // refresh
    { "GameListRefreshPress",  { { 19, 409 },  { 24, 511 },  { 27, 655 } },  0 },
    { "GameListJoinHiLite",    { { 412, 409 }, { 515, 511 }, { 668, 655 } }, 1 },   // join
    { "GameListJoinPress",     { { 412, 409 }, { 515, 511 }, { 668, 655 } }, 0 },
    { "GameListCreateHiLite",  { { 477, 409 }, { 596, 511 }, { 753, 662 } }, 1 },   // create
    { "GameListCreatePress",   { { 477, 409 }, { 596, 511 }, { 753, 662 } }, 0 },
    { "GameListChatHiLite",    { { 557, 409 }, { 696, 511 }, { 903, 655 } }, 1 },   // back (chat / menu)
    { "GameListChatPress",     { { 557, 409 }, { 696, 511 }, { 903, 655 } }, 0 },
    { "PXO-PasswordPopup",     { { 170, 174 }, { 213, 218 }, { 272, 278 } }, 4 },
    { "Icon-CheckHilite",      { { 427, 264 }, { 534, 330 }, { 683, 422 } }, 5 },   // popup: join
    { "Icon-CheckPress",       { { 427, 264 }, { 534, 330 }, { 683, 422 } }, 4 },
    { "Icon-GrnClseHilite",    { { 427, 195 }, { 534, 244 }, { 683, 312 } }, 5 },   // popup: close
    { "Icon-GrnClsePress",     { { 427, 195 }, { 534, 244 }, { 683, 312 } }, 4 },
};
// 0x58ac20. The strings are pooled copies at 0x588b8c / 0x588b74 (see new_journal.cpp).
char *gamelist_scrollbar_names[2] = { "ScrollBarMid640.tga", "ScrollBarMid1024.tga" };
// then the literals "%-4d" (0x58ac28), "9999" (0x58ac30), "%d-%d" (0x58ac38), "%d/%d" (0x58ac40).

// ---- .bss ----------------------------------------------------------------------------------------
bool gamelist_password_input_active;                          // 0x5bc710 password popup open
RealtimeTimer gamelist_repeat_timer;                          // 0x5bc714 button auto-repeat
RealtimeTimer gamelist_cursor_timer;                          // 0x5bc718 500 ms cursor blink
int gamelist_scroll_pos;                                      // 0x5bc71c first visible game
int gamelist_drag_start_thumb;                                // 0x5bc720
bool gamelist_scrollbar_visible;                              // 0x5bc724
char gamelist_password[16];                                   // 0x5bc728
int gamelist_scrollbar_w;                                     // 0x5bc738 thumb w, h
int gamelist_scrollbar_h;                                     // 0x5bc73c
int gamelist_scrollbar_bm_w;                                  // 0x5bc740
int gamelist_scrollbar_bm_h;                                  // 0x5bc744
int gamelist_scrollbar_x;                                     // 0x5bc748 thumb x, y
int gamelist_scrollbar_y;                                     // 0x5bc74c
int gamelist_mouse_mode;                                      // 0x5bc750 0 idle, 1 button held, 2 dragging,
                                                              //          3 message box of the Create button
int gamelist_scrollbar_bm;                                    // 0x5bc754
int gamelist_drag_start_y;                                    // 0x5bc758
int gamelist_hover_row;                                       // 0x5bc75c
ui_button_state gamelist_buttons[GAMELIST_NUM_BUTTONS];       // 0x5bc760
mp_gamelist_entry *gamelist_selected_game;                    // 0x5bc910
int gamelist_pressed_button;                                  // 0x5bc914
bool gamelist_cursor_on;                                      // 0x5bc918

// The compiler's dynamic initialiser pairs of the two timers (no source of their own; the definition order
// above gives the entry order):
// 0x40a660 init_realtimetimer_5bc714_thunk ("jmp 0x40a670", static_initializers entry 13). Confidence: exact.
// 0x40a670 init_realtimetimer_5bc714 (gamelist_repeat_timer: mov ecx, 0x5bc714 / jmp RealtimeTimer::RealtimeTimer). Confidence: exact.
// 0x40a680 init_realtimetimer_5bc718_thunk (jmp, entry 14). Confidence: exact.
// 0x40a690 init_realtimetimer_5bc718 (gamelist_cursor_timer). Confidence: exact.

// 0x40a6a0  Confidence: exact. Same loader as gamecreate_load_bitmaps; entries 0 (background) and 13
// (password popup) are marked always drawn. progress_update(0) after the buttons; the scrollbar tile is
// scaled without rounding (as mp_charcreate_load_bitmaps).
void gamelist_load_bitmaps()
{
    float scale = ui_get_scale();
    char filename[128];

    for (int i = 0; i < GAMELIST_NUM_BUTTONS; i++) {
        ui_button_state *b = &gamelist_buttons[i];
        ui_button_def *d = &gamelist_button_defs[i];
        b->flags = 0;
        b->bitmap = -1;
        b->x = 0;
        b->y = 0;
        b->w = 0;
        b->h = 0;

        if (strlen(d->name)) {
            strcpy(filename, d->name);
            strcat(filename, ui_bitmap_suffix_tga[ui_hires]);
            b->bitmap = bm_load(filename, path_interface, -1);
            b->x = d->pos[resolution_index][0];
            b->y = d->pos[resolution_index][1];
            bm_get_dimensions(b->bitmap, &b->w, &b->h);
            b->w = (int)(b->w * scale + 0.5f);
            b->h = (int)(b->h * scale + 0.5f);
            if (i == 0 || i == 13)
                b->flags |= 1;
        }
    }

    progress_update(0);
    gamelist_scrollbar_bm = bm_load(gamelist_scrollbar_names[ui_hires], path_interface, -1);
    bm_get_dimensions(gamelist_scrollbar_bm, &gamelist_scrollbar_bm_w, &gamelist_scrollbar_bm_h);
    gamelist_scrollbar_bm_w = (int)(gamelist_scrollbar_bm_w * scale);
    gamelist_scrollbar_bm_h = (int)(gamelist_scrollbar_bm_h * scale);
}

// 0x40a850  Confidence: exact (store order: VC6 keeps the source order for these globals).
void gamelist_init()
{
    gamelist_scroll_pos = 0;
    gamelist_password_input_active = false;
    gamelist_password[0] = 0;
    gamelist_mouse_mode = 0;
    gamelist_pressed_button = -1;
    gamelist_hover_row = -1;
    gamelist_selected_game = 0;
}

// folded (0x4f13e0, empty_function): the screen's leave function is empty; the linker folded it into
// empty_function (identical-code folding). interface_set_screen calls each by name (D7903); name ours.
void gamelist_leave()
{
}

// folded (0x4063a0, ui_return_true): the screen has no panel of its own, so its mouse_in_panel returns true;
// the linker folded the identical copies into one (identical-code folding). interface_mouse_over_panel
// calls each by name (D7903); name ours.
bool gamelist_mouse_in_panel(int x, int y)
{
    return true;
}

// 0x40a880  Confidence: exact (the join-result switch: jump table 0x40a94c over 0..4, cases 0 and 2 empty,
// 3 = default; case 4 with the popup already open pushes the constant 4 into the default's call, read as
// show_join_error(result) folded by VC6). While the join request is pending (1) the screen ignores input.
int gamelist_frame(int key)
{
    int result;
    int i;

    for (i = 0; i < GAMELIST_NUM_BUTTONS; i++)
        gamelist_buttons[i].flags &= ~2;

    gamelist_update_join_button();
    gamelist_update_scrollbar();

    if (gamelist_cursor_timer.elapsed()) {
        gamelist_cursor_timer.SetTimeout(500);
        gamelist_cursor_on = !gamelist_cursor_on;
    }

    result = mp_join_get_result();
    switch (result) {
    case 0:
    case 2:
        break;
    case 1:
        return key;
    case 4:
        if (gamelist_password_input_active)
            gamelist_show_join_error(result);
        else
            gamelist_password_input_begin();
        mp_join_clear_result();
        break;
    default:
        gamelist_show_join_error(result);
        mp_join_clear_result();
        break;
    }

    if (ui_message_box_is_active())
        return key;

    switch (gamelist_mouse_mode) {
    case 0:
        key = gamelist_handle_click(key);
        break;
    case 1:
        gamelist_update_buttons();
        break;
    case 2:
        gamelist_drag_scrollbar();
        break;
    case 3:
        empty_frame_hook_40acc0();
        break;
    }
    return key;
}

// 0x40a970  Confidence: exact (the first game becomes the selection when there is none; VC6 threads the
// "list empty" test straight to the disable branch). Join (entry 7) needs a selected game.
void gamelist_update_join_button()
{
    if (!gamelist_selected_game && mp_gamelist_used.next != &mp_gamelist_used)
        gamelist_selected_game = mp_gamelist_used.next;

    if (gamelist_selected_game)
        gamelist_buttons[7].flags &= ~4;
    else
        gamelist_buttons[7].flags |= 4;
}

// 0x40a9b0  Confidence: exact (jump table 0x40ab50 over code - 3, 0..10). The first six texts are copied
// with strcpy, the last five with sprintf(buf, text) (no arguments; kept as in the binary). Other codes
// show nothing.
void gamelist_show_join_error(int code)
{
    char *ok = XSTR("Ok", 213);
    char buf[256];

    switch (code) {
    case 3:
        strcpy(buf, XSTR("Game is full.", 788));
        break;
    case 4:
        strcpy(buf, XSTR("Incorrect password.", 789));
        break;
    case 5:
        strcpy(buf, XSTR("Game has already started.", 790));
        break;
    case 6:
        strcpy(buf, XSTR("An error occurred on the server.", 791));
        break;
    case 7:
        strcpy(buf, XSTR("Another player is joining -- try again in a few moments.", 792));
        break;
    case 8:
        strcpy(buf, XSTR("Your data does not match the server's data.", 793));
        break;
    case 9:
        sprintf(buf, XSTR("Your level is too low to join this game.", 794));
        break;
    case 10:
        sprintf(buf, XSTR("Your level is too high to join this game.", 795));
        break;
    case 11:
        sprintf(buf, XSTR("You cannot join games of that difficulty level yet.", 796));
        break;
    case 12:
        sprintf(buf, XSTR("You have not unlocked that level yet.", 797));
        break;
    case 13:
        sprintf(buf, XSTR("No response from server.", 798));
        break;
    default:
        return;
    }

    ui_message_box_show(XSTR("Join failure", 799), buf, -1, -1, 1, &ok, 0, 0);
}

// 0x40ab80  Confidence: exact (the page size lives in the spare argument slot for fild; the order of the
// four stores of the count <= 0 branch). Same as mp_charselect_update_scrollbar for one list; entries 1 / 3
// are the arrows.
void gamelist_update_scrollbar()
{
    int count = mp_gamelist_count;
    int page = gamelist_page_rows[resolution_index];

    if (count <= page) {
        gamelist_scrollbar_visible = false;
        gamelist_buttons[1].flags |= 4;
        gamelist_buttons[3].flags |= 4;
        return;
    }

    gamelist_scrollbar_visible = true;
    gamelist_buttons[1].flags &= ~4;
    gamelist_buttons[3].flags &= ~4;

    if (count <= 0) {
        gamelist_scrollbar_x = gamelist_track_pos[resolution_index][0];
        gamelist_scrollbar_y = gamelist_track_pos[resolution_index][1];
        gamelist_scrollbar_w = gamelist_scrollbar_bm_w;
        gamelist_scrollbar_h = gamelist_track_h[resolution_index];
    } else {
        gamelist_scrollbar_x = gamelist_track_pos[resolution_index][0];
        gamelist_scrollbar_y = (int)((float)gamelist_scroll_pos / mp_gamelist_count
                                     * gamelist_track_h[resolution_index] + 0.5f)
                               + gamelist_track_pos[resolution_index][1];
        gamelist_scrollbar_w = gamelist_scrollbar_bm_w;
        gamelist_scrollbar_h = (int)((float)page / mp_gamelist_count * gamelist_track_h[resolution_index] + 0.5f);
    }
}

// 0x40ac90  Confidence: exact. Opens the password popup (join result 4).
void gamelist_password_input_begin()
{
    gamelist_password_input_active = true;
    gamelist_password[0] = 0;
    gamelist_cursor_timer.SetTimeout(500);
    gamelist_cursor_on = true;
}

// 0x40acc0  Confidence: exact (a bare ret). The frame hook of mouse mode 3; after identical-code folding
// it is the RET 12 other call sites reach (declared in summoner.h, DECISIONS.md D57).
void empty_frame_hook_40acc0()
{
}

// 0x40acd0  Confidence: exact (VC6 8966 byte match, w10: the list rectangles through locals, the row offset
// row * font_h, the drag thumb stored before the drag start). The password block: every path in it returns; the tooltip
// test sits between the hit test and the disabled test, a disabled entry lets the loop go on. While the
// popup is open only its entries (flag 4) react and the keys go to the password (at most 15 characters);
// Enter joins, Esc closes the popup.
int gamelist_handle_click(int key)
{
    int rows;
    int font_h;
    int i;
    int dx, dy, dz;

    gamelist_pressed_button = -1;
    gamelist_hover_row = -1;

    for (i = 0; i < GAMELIST_NUM_BUTTONS; i++) {
        ui_button_def *d = &gamelist_button_defs[i];
        ui_button_state *b = &gamelist_buttons[i];
        if (gamelist_password_input_active == (((unsigned)d->flags >> 2) & 1)
            && (d->flags & 0xb)
            && point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h)) {
            if ((d->flags & 8) && interface_input_ready())
                gamelist_refresh_tooltip(i);
            if (!(b->flags & 4)) {
                b->flags |= 2;
                gamelist_pressed_button = i;
                if (d->flags & 1) {
                    if (mouse_was_pressed(0)) {
                        gamelist_mouse_mode = 1;
                        gamelist_repeat_timer.invalidate();
                        return key;
                    }
                } else if (d->flags & 2) {
                    if (mouse_was_pressed(0)) {
                        gamelist_mouse_mode = 1;
                        gamelist_button_action(gamelist_pressed_button + 1);
                        gamelist_repeat_timer.SetTimeout(500);
                        return key;
                    }
                }
                break;
            }
        }
    }

    if (gamelist_password_input_active) {
        if (key) {
            int c = key_to_ascii(key);
            if (c != 0xff) {
                int len = strlen(gamelist_password);
                if (len < 15) {
                    gamelist_password[len] = (char)c;
                    gamelist_password[len + 1] = 0;
                    gamelist_cursor_timer.SetTimeout(500);
                    gamelist_cursor_on = true;
                    return 0;
                }
            } else {
                switch (key) {
                case KEY_ESC:
                    gamelist_password_input_cancel();
                    return 0;
                case KEY_BACKSPACE: {
                    int len = strlen(gamelist_password);
                    if (len > 0) {
                        gamelist_password[len - 1] = 0;
                        gamelist_cursor_timer.SetTimeout(500);
                        gamelist_cursor_on = true;
                        return 0;
                    }
                    break;
                }
                case KEY_ENTER:
                    gamelist_join_selected();
                    return 0;
                }
            }
        }
        return key;
    }

    if (point_in_rect(game_mouse_x, game_mouse_y, gamelist_scrollbar_x, gamelist_scrollbar_y, gamelist_scrollbar_w,
                      gamelist_scrollbar_h)
        && mouse_was_pressed(0)) {
        gamelist_mouse_mode = 2;
        gamelist_drag_start_thumb = gamelist_scrollbar_y;
        gamelist_drag_start_y = game_mouse_y;
        return key;
    }

    rows = mp_gamelist_count;
    if (rows >= gamelist_page_rows[resolution_index])
        rows = gamelist_page_rows[resolution_index];
    font_h = gr_get_font_height(-1);

    int lx = gamelist_list_pos[resolution_index][0];
    int ly = gamelist_list_pos[resolution_index][1];
    int lw = gamelist_list_width[resolution_index];
    if (point_in_rect(game_mouse_x, game_mouse_y, lx, ly, lw, font_h * rows)) {
        int row;

        for (row = 0; row < rows; row++) {
            int rx = gamelist_list_pos[resolution_index][0];
            int ry = gamelist_list_pos[resolution_index][1] + row * font_h;
            int rw = gamelist_list_width[resolution_index];
            if (point_in_rect(game_mouse_x, game_mouse_y, rx, ry, rw, font_h)) {
                mp_gamelist_entry *game = mp_server_list_get(gamelist_scroll_pos + row);
                if (game->status == 1) {
                    gamelist_hover_row = row;
                    if (mouse_was_double_clicked(0)) {
                        gamelist_selected_game = game;
                        gamelist_join_selected();
                        return key;
                    }
                    if (mouse_was_pressed(0)) {
                        gamelist_selected_game = game;
                        return key;
                    }
                }
                break;
            }
        }
    }

    switch (key) {
    case KEY_ESC:
        gamelist_back();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        return 0;
    }

    mouse_get_delta(&dx, &dy, &dz);
    if (dz > 0)
        gamelist_page_up();
    else if (dz < 0)
        gamelist_page_down();
    return key;
}

// 0x40b0b0  Confidence: exact (loop shape). The index-th received game, NULL past the end.
mp_gamelist_entry *mp_server_list_get(int index)
{
    mp_gamelist_entry *game;

    for (game = mp_gamelist_used.next; game != &mp_gamelist_used; game = game->next) {
        if (!index)
            return game;
        index--;
    }
    return 0;
}

// 0x40b0e0  Confidence: exact. Only the Refresh button (entry 5) has a tooltip.
void gamelist_refresh_tooltip(int button)
{
    if (button == 5)
        ui_tooltip_set(gamelist_buttons[5].x, gamelist_buttons[5].y, gamelist_buttons[5].w, gamelist_buttons[5].h,
                       XSTR("Refresh list", 800));
}

// 0x40b120  Confidence: exact (VC6 byte match, m-1020 w11, D12002: the global is decremented, then clamped with
// `&= (x <= 0) - 1`).
void gamelist_page_up()
{
    if (gamelist_scroll_pos > 0) {
        gamelist_scroll_pos -= gamelist_page_rows[resolution_index];
        gamelist_scroll_pos &= (gamelist_scroll_pos <= 0) - 1;
        gamelist_update_scrollbar();
    }
}

// 0x40b150  Confidence: exact (as gamelist_page_up).
void gamelist_page_down()
{
    int pos = gamelist_scroll_pos;
    int page = gamelist_page_rows[resolution_index];
    int last = mp_gamelist_count - page;

    if (pos < last) {
        pos += page;
        if (pos < last)
            last = pos;
        gamelist_scroll_pos = last;
        gamelist_update_scrollbar();
    }
}

// 0x40b180  Confidence: exact. Sends the join request with the character record from its name on
// (profile + 0x28) and the typed password.
void gamelist_join_selected()
{
    mp_char_profile *profile = mp_charselect_get_selected_profile(0);

    if (profile)
        mp_send_jn_rqst(gamelist_selected_game, profile->name, gamelist_password, 1);
}

// 0x40b1b0  Confidence: exact. Back to the PXO chat (state 11) with bit 0 of the PXO config, else to
// state 1.
void gamelist_back()
{
    if (pxo_cfg_flags & 1)
        gameseq_set_state(11);
    else
        gameseq_set_state(1);
}

// 0x40b1d0  Confidence: exact.
void gamelist_password_input_cancel()
{
    gamelist_password_input_active = false;
    gamelist_password[0] = 0;
}

// 0x40b1e0  Confidence: exact (byte table 0x40b32c over button - 2, 0..15, into the jump table 0x40b30c;
// 8 and 15 share the join case; the Create case reuses the argument slot for "ok"; the back / close cases
// share the sound call). Create warns modem players first (nw_network_status 1 below 56k, 2 56k): the
// message box calls gamelist_create_game_cb, mouse mode 3 until then.
void gamelist_button_action(int button)
{
    switch (button) {
    case 2:
        gamelist_scroll_up();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 4:
        gamelist_scroll_down();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 6:
        gamelist_refresh();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 8:
    case 15:
        gamelist_join_selected();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 10: {
        char *ok;

        game_play_sound(0x23, 1.0f, 0, 0, 0);
        ok = XSTR("Ok", 213);
        if (nw_network_status == 1) {
            ui_message_box_show(XSTR("Network", 773),
                                XSTR("If you host a game on a modem slower than 56K, we recommend that you host no "
                                     "more than 1 other player.", 801),
                                -1, -1, 1, &ok, gamelist_create_game_cb, 2);
            gamelist_mouse_mode = 3;
        } else if (nw_network_status == 2) {
            ui_message_box_show(XSTR("Network", 773),
                                XSTR("If you host a game on a 56K modem, we recommend that you host no more than 2 "
                                     "other players.", 802),
                                -1, -1, 1, &ok, gamelist_create_game_cb, 2);
            gamelist_mouse_mode = 3;
        } else {
            gamelist_create_game();
        }
        break;
    }
    case 12:
        gamelist_back();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    case 17:
        gamelist_password_input_cancel();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    }
}

// 0x40b340  Confidence: exact.
void gamelist_scroll_up()
{
    if (gamelist_scroll_pos > 0) {
        gamelist_scroll_pos--;
        gamelist_update_scrollbar();
    }
}

// 0x40b360  Confidence: exact.
void gamelist_scroll_down()
{
    if (gamelist_scroll_pos < mp_gamelist_count - gamelist_page_rows[resolution_index]) {
        gamelist_scroll_pos++;
        gamelist_update_scrollbar();
    }
}

// 0x40b390  Confidence: exact.
void gamelist_refresh()
{
    gamelist_selected_game = 0;
    mp_gamelist_request_refresh();
    gamelist_scroll_pos = 0;
}

// 0x40b3b0  Confidence: exact. On to the game creation (state 13).
void gamelist_create_game()
{
    gameseq_set_state(13);
}

// 0x40b3c0  Confidence: exact (the argument is not read).
void gamelist_create_game_cb(int button)
{
    gamelist_create_game();
    gamelist_mouse_mode = 0;
}

// 0x40b3d0  Confidence: exact. The entry after the pressed one acts: on release for the HiLite entries
// (the repeat timer was invalidated on press), every 100 ms while held for the arrows.
void gamelist_update_buttons()
{
    int pressed = gamelist_pressed_button;
    int button = pressed + 1;

    if (pressed >= 0 && pressed < GAMELIST_NUM_BUTTONS) {
        ui_button_state *b = &gamelist_buttons[button];
        if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
            b->flags |= 2;

        if (!mouse_is_down(0)) {
            if ((b->flags & 2) && !gamelist_repeat_timer.valid())
                gamelist_button_action(button);
        } else {
            if (gamelist_repeat_timer.valid() && gamelist_repeat_timer.elapsed() && (b->flags & 2)) {
                gamelist_button_action(button);
                gamelist_repeat_timer.SetTimeout(100);
            }
            return;
        }
    }
    gamelist_mouse_mode = 0;
}

// 0x40b4a0  Confidence: exact (as mp_charselect_drag_scrollbar, but the position is clamped in a
// register and stored once).
void gamelist_drag_scrollbar()
{
    int pos = (int)((float)(gamelist_drag_start_thumb
                            + (game_mouse_y - gamelist_track_pos[resolution_index][1] - gamelist_drag_start_y))
                    / gamelist_track_h[resolution_index] * mp_gamelist_count + 0.5f);
    int last;

    pos = pos > 0 ? pos : 0;
    last = mp_gamelist_count - gamelist_page_rows[resolution_index];
    if (pos < last)
        last = pos;
    gamelist_scroll_pos = last;
    gamelist_update_scrollbar();

    if (!mouse_is_down(0))
        gamelist_mouse_mode = 0;
}

// 0x40b530  Confidence: exact. The popup entries (flag 4) are drawn only while the popup is open; the
// "Joining game..." box while the join request is pending.
void gamelist_draw()
{
    int i;

    gr_set_color(255, 255, 255, 255);
    for (i = 0; i < GAMELIST_NUM_BUTTONS; i++) {
        ui_button_state *b = &gamelist_buttons[i];
        ui_button_def *d = &gamelist_button_defs[i];
        if ((gamelist_password_input_active || !(d->flags & 4)) && (b->flags & 3))
            ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
    }
    mp_server_list_draw();
    gamelist_draw_scrollbar();
    if (gamelist_password_input_active)
        gamelist_draw_password_input();
    if (mp_join_get_result() == 1)
        mp_draw_join_help();
}

// 0x40b5c0  Confidence: exact (VC6 8966 byte match, w10: the level and players tests are positive, the text
// colour first). Each column's colour test is written out as in the binary, which repeats it per column. A closed game is red; the hovered or selected game uses the highlight
// colour; otherwise a column is red when it keeps the selected character out (level not unlocked, game
// full, difficulty locked, character level out of range).
void mp_server_list_draw()
{
    mp_char_profile *profile = mp_charselect_get_selected_profile(0);
    int rows = mp_gamelist_count;
    mp_gamelist_entry *game;
    int font_h;
    int x, y;
    int row;
    char buf[256];

    if (rows >= gamelist_page_rows[resolution_index])
        rows = gamelist_page_rows[resolution_index];
    game = mp_gamelist_used.next;
    font_h = gr_get_font_height(-1);

    x = gamelist_header_pos[resolution_index][0];
    y = gamelist_header_pos[resolution_index][1];
    gr_set_color_ptr(&ui_color_blue);
    gr_string(x + gamelist_col_name[resolution_index], y, XSTR("Name", 803), -1, gr_string_mode);
    gr_string(x + gamelist_col_level[resolution_index], y, XSTR("Level", 804), -1, gr_string_mode);
    gr_string(x + gamelist_col_players[resolution_index], y, XSTR("Plyr", 805), -1, gr_string_mode);
    gr_string(x + gamelist_col_difficulty[resolution_index], y, XSTR("Diff", 806), -1, gr_string_mode);
    gr_string(x + gamelist_col_exp[resolution_index], y, XSTR("Exp", 807), -1, gr_string_mode);
    gr_string(x + gamelist_col_server[resolution_index], y, XSTR("Svr", 808), -1, gr_string_mode);
    gr_string(x + gamelist_col_ping[resolution_index], y, XSTR("Ping", 809), -1, gr_string_mode);

    x = gamelist_list_pos[resolution_index][0];
    y = gamelist_list_pos[resolution_index][1];
    if (mp_gamelist_count == 0) {
        gr_set_color_ptr(&ui_color_red);
        gr_string(x, y, XSTR("<No servers found>", 810), -1, gr_string_mode);
        return;
    }

    for (row = -gamelist_scroll_pos; row < rows && game != &mp_gamelist_used; row++, game = game->next) {
        char *name;
        int w, h;

        if (row < 0)
            continue;

        // name
        if (game->status != 1)
            gr_set_color_ptr(&ui_color_red);
        else if (row == gamelist_hover_row || game == gamelist_selected_game)
            gr_set_color_ptr(&ui_color_highlight);
        else
            gr_set_color_ptr(&ui_color_text);
        gr_string_truncate_ellipsis(buf, game->name, gamelist_col_level[resolution_index]
                                    - gamelist_col_gap[resolution_index] - gamelist_col_name[resolution_index], -1);
        gr_string(x + gamelist_col_name[resolution_index], y, buf, -1, gr_string_mode);

        // level
        if (game->status != 1)
            gr_set_color_ptr(&ui_color_red);
        else if (row == gamelist_hover_row || game == gamelist_selected_game)
            gr_set_color_ptr(&ui_color_highlight);
        else if (mp_player_level_available(profile->flags, game->level, game->difficulty))
            gr_set_color_ptr(&ui_color_text);
        else
            gr_set_color_ptr(&ui_color_red);
        name = level_table_display_name(game->level);
        if (name) {
            gr_string_truncate_ellipsis(buf, name, gamelist_col_players[resolution_index]
                                        - gamelist_col_gap[resolution_index] - gamelist_col_level[resolution_index], -1);
            gr_string(x + gamelist_col_level[resolution_index], y, buf, -1, gr_string_mode);
        } else {
            gr_string(x + gamelist_col_level[resolution_index], y, XSTR("Unknown", 352), -1, gr_string_mode);
        }

        // players
        if (game->status != 1)
            gr_set_color_ptr(&ui_color_red);
        else if (row == gamelist_hover_row || game == gamelist_selected_game)
            gr_set_color_ptr(&ui_color_highlight);
        else if (game->num_players < game->max_players)
            gr_set_color_ptr(&ui_color_text);
        else
            gr_set_color_ptr(&ui_color_red);
        sprintf(buf, "%d/%d", game->num_players, game->max_players);
        gr_string(x + gamelist_col_players[resolution_index], y, buf, -1, gr_string_mode);

        // difficulty (bits 0x4000000 / 0x8000000 of the profile unlock 1 / 2)
        if (game->status != 1)
            gr_set_color_ptr(&ui_color_red);
        else if (row == gamelist_hover_row || game == gamelist_selected_game)
            gr_set_color_ptr(&ui_color_highlight);
        else if (game->difficulty && !(profile->flags & (1 << (game->difficulty + 25))))
            gr_set_color_ptr(&ui_color_red);
        else
            gr_set_color_ptr(&ui_color_text);
        switch (game->difficulty) {
        case 0:
            gr_string(x + gamelist_col_difficulty[resolution_index], y, XSTR("Easy", 811), -1, gr_string_mode);
            break;
        case 1:
            gr_string(x + gamelist_col_difficulty[resolution_index], y, XSTR("Med", 812), -1, gr_string_mode);
            break;
        case 2:
            gr_string(x + gamelist_col_difficulty[resolution_index], y, XSTR("Hard", 813), -1, gr_string_mode);
            break;
        }

        // character level range
        if (game->status != 1)
            gr_set_color_ptr(&ui_color_red);
        else if (row == gamelist_hover_row || game == gamelist_selected_game)
            gr_set_color_ptr(&ui_color_highlight);
        else if (profile->level < game->min_level || profile->level > game->max_level)
            gr_set_color_ptr(&ui_color_red);
        else
            gr_set_color_ptr(&ui_color_text);
        sprintf(buf, "%d-%d", game->min_level, game->max_level);
        gr_string(x + gamelist_col_exp[resolution_index], y, buf, -1, gr_string_mode);

        // connection, centred
        if (game->status != 1)
            gr_set_color_ptr(&ui_color_red);
        else if (row == gamelist_hover_row || game == gamelist_selected_game)
            gr_set_color_ptr(&ui_color_highlight);
        else
            gr_set_color_ptr(&ui_color_text);
        switch (game->connection) {
        case 1:
            strcpy(buf, XSTR("<56k", 814));
            break;
        case 2:
            strcpy(buf, XSTR("56k", 815));
            break;
        case 3:
            strcpy(buf, XSTR("ISDN+", 816));
            break;
        default:
            strcpy(buf, XSTR("unkwn", 817));
            break;
        }
        gr_get_string_size(&w, &h, buf, -1, -1);
        gr_string(x + gamelist_col_server_centre[resolution_index] - w / 2, y, buf, -1, gr_string_mode);

        // ping
        if (game->status != 1)
            gr_set_color_ptr(&ui_color_red);
        else if (row == gamelist_hover_row || game == gamelist_selected_game)
            gr_set_color_ptr(&ui_color_highlight);
        else
            gr_set_color_ptr(&ui_color_text);
        if (game->ping > 9999)
            strcpy(buf, "9999");
        else
            sprintf(buf, "%-4d", game->ping);
        gr_string(x + gamelist_col_ping[resolution_index], y, buf, -1, gr_string_mode);

        y += font_h;
    }
}

// 0x40bca0  Confidence: exact (as mp_charselect_draw_scrollbar: whole tiles, then one tile flush with the
// bottom).
void gamelist_draw_scrollbar()
{
    int tiles, i;

    if (!gamelist_scrollbar_visible)
        return;

    gr_set_color(255, 255, 255, 255);
    tiles = gamelist_scrollbar_h / gamelist_scrollbar_bm_h;
    for (i = 0; i < tiles; i++)
        ui_bitmap_scaled(gamelist_scrollbar_bm, gamelist_scrollbar_x, gamelist_scrollbar_bm_h * i + gamelist_scrollbar_y,
                         gr_bitmap_wrap_mode);
    ui_bitmap_scaled(gamelist_scrollbar_bm, gamelist_scrollbar_x,
                     gamelist_scrollbar_y - gamelist_scrollbar_bm_h + gamelist_scrollbar_h, gr_bitmap_wrap_mode);
}

// 0x40bd50  Confidence: exact (the min(0, w - text w) spelling, as gamecreate_draw_text_field; here the
// "|" is always appended and the blink picks the string as an argument). The buffer is 20 bytes on the
// stack.
void gamelist_draw_password_input()
{
    int clip_x, clip_y, clip_w, clip_h;
    char buf[20];
    int w, h;
    int x;

    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    gr__set_clip(gamelist_password_pos[resolution_index][0], gamelist_password_pos[resolution_index][1],
                 gamelist_password_size[resolution_index][0], gamelist_password_size[resolution_index][1]);

    strcpy(buf, gamelist_password);
    strcat(buf, "|");
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_set_color_ptr(&ui_color_text);

    x = gamelist_password_size[resolution_index][0] - w;
    x = x > 0 ? 0 : x;
    if (gamelist_cursor_on)
        gr_string(x, 0, buf, -1, gr_string_mode);
    else
        gr_string(x, 0, gamelist_password, -1, gr_string_mode);

    gr__set_clip(clip_x, clip_y, clip_w, clip_h);
}

// 0x40be80  Confidence: exact (the halving is signed: cdq / sub / sar). A centred box with
// "Joining game..." while the join request is pending.
void mp_draw_join_help()
{
    char *text = XSTR("Joining game...", 818);
    int w, h;
    int box_w, box_h;
    int x, y;

    gr_get_string_size(&w, &h, text, -1, -1);
    box_w = w + ui_choice_popup_margin[resolution_index] * 2;
    box_h = h + ui_choice_popup_margin_y[resolution_index] * 2;
    x = (gr_screen_width() - box_w) / 2;
    y = (gr_screen_height() - box_h) / 2;
    ui_draw_panel_frame(x, y, box_w, box_h);
    gr_set_color_ptr(&ui_color_text);
    gr_string(x + ui_choice_popup_margin[resolution_index], y + ui_choice_popup_margin_y[resolution_index], text, -1,
              gr_string_mode);
}
