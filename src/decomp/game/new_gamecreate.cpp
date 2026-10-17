// new_gamecreate.cpp - Sum.exe v1.40, code 0x4099b0-0x40a65f (screen 17, multiplayer game creation)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .rdata 0x57ec20-0x57ecc7 (layout tables), .data 0x58a1e0-0x58a79f (button table),
// .bss 0x5bc490-0x5bc708. "%d" (0x589820) and "|" (0x588c10) are pooled copies in earlier units' .data.

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
#include "summoner/multi/multi.h"
#include "summoner/interface/new_charselect.h"
#include "summoner/interface/new_gamecreate.h"

#define GAMECREATE_NUM_BUTTONS 23

// ---- .rdata --------------------------------------------------------------------------------
// Per resolution_index (640, 800, 1024) and text field (0 game name, 1 password).
const int gamecreate_field_pos[3][2][2]  = { { { 49,  63 }, {  68, 333 } },    // 0x57ec20 x, y
                                             { { 61,  79 }, {  85, 416 } },
                                             { { 78, 101 }, { 109, 533 } } };
const int gamecreate_field_size[3][2][2] = { { { 240, 15 }, { 198, 15 } },     // 0x57ec50 w, h (clip rect)
                                             { { 300, 19 }, { 248, 19 } },
                                             { { 384, 24 }, { 317, 24 } } };
const int gamecreate_players_pos[3][2]      = { { 170, 149 }, { 213, 186 }, { 272, 238 } };  // 0x57ec80 centre
const int gamecreate_levels_above_pos[3][2] = { { 105, 446 }, { 131, 558 }, { 168, 714 } };  // 0x57ec98 centre
const int gamecreate_levels_below_pos[3][2] = { { 221, 446 }, { 276, 558 }, { 354, 714 } };  // 0x57ecb0 centre

// ---- .data ---------------------------------------------------------------------------------
// 0x58a1e0  23 x 0x40. flags: 1 HiLite hot spot (the action is the Press entry after it, on release),
// 2 hot spot that is its own action (the three difficulty check boxes). Entries 5/7/9 are the ticks
// that gamecreate_set_difficulty shows; 8 and 10 are disabled by gamecreate_init unless the profile
// has unlocked difficulty 1 / 2; 21 (Begin) is disabled while the game name is empty.
ui_button_def gamecreate_button_defs[GAMECREATE_NUM_BUTTONS] = {
    { "PXO-CreateGame",     { { 0, 0 },     { 0, 0 },     { 0, 0 } },     0 },
    { "Icon-ArrwLFHilite",  { { 123, 142 }, { 154, 178 }, { 194, 224 } }, 1 },   // players -
    { "Icon-ArrwLFPress",   { { 123, 142 }, { 154, 178 }, { 194, 224 } }, 0 },
    { "Icon-ArrwRTHilite",  { { 192, 142 }, { 240, 178 }, { 308, 224 } }, 1 },   // players +
    { "Icon-ArrwRTPress",   { { 192, 142 }, { 240, 178 }, { 308, 224 } }, 0 },
    { "checkbox",           { { 96, 240 },  { 120, 300 }, { 158, 388 } }, 0 },   // difficulty 0 tick
    { "checkbox-hilite",    { { 96, 240 },  { 120, 300 }, { 158, 388 } }, 2 },
    { "checkbox",           { { 185, 240 }, { 231, 300 }, { 301, 388 } }, 0 },   // difficulty 1 tick
    { "checkbox-hilite",    { { 185, 240 }, { 231, 300 }, { 301, 388 } }, 2 },
    { "checkbox",           { { 260, 240 }, { 325, 300 }, { 421, 388 } }, 0 },   // difficulty 2 tick
    { "checkbox-hilite",    { { 260, 240 }, { 325, 300 }, { 421, 388 } }, 2 },
    { "Icon-ArrwLFHilite",  { { 58, 440 },  { 73, 550 },  { 89, 702 } },  1 },   // levels above -
    { "Icon-ArrwLFPress",   { { 58, 440 },  { 73, 550 },  { 89, 702 } },  0 },
    { "Icon-ArrwRTHilite",  { { 127, 440 }, { 159, 550 }, { 203, 702 } }, 1 },   // levels above +
    { "Icon-ArrwRTPress",   { { 127, 440 }, { 159, 550 }, { 203, 702 } }, 0 },
    { "Icon-ArrwLFHilite",  { { 174, 440 }, { 218, 550 }, { 275, 702 } }, 1 },   // levels below -
    { "Icon-ArrwLFPress",   { { 174, 440 }, { 218, 550 }, { 275, 702 } }, 0 },
    { "Icon-ArrwRTHilite",  { { 243, 440 }, { 304, 550 }, { 389, 702 } }, 1 },   // levels below +
    { "Icon-ArrwRTPress",   { { 243, 440 }, { 304, 550 }, { 389, 702 } }, 0 },
    { "CreateCancelHiLite", { { 297, 413 }, { 371, 516 }, { 491, 671 } }, 1 },
    { "CreateCancelPress",  { { 297, 413 }, { 371, 516 }, { 491, 671 } }, 0 },
    { "CreateBeginHiLite",  { { 475, 413 }, { 594, 516 }, { 814, 671 } }, 1 },
    { "CreateBeginPress",   { { 475, 413 }, { 594, 516 }, { 814, 671 } }, 0 },
};

// ---- .bss ----------------------------------------------------------------------------------
int gamecreate_max_players;                                   // 0x5bc490  1..4
bool gamecreate_field_active[2];                              // 0x5bc494  text field has the focus
ui_button_state gamecreate_buttons[GAMECREATE_NUM_BUTTONS];   // 0x5bc498
int gamecreate_mouse_mode;                                    // 0x5bc6c0  0 idle, 1 button held
int gamecreate_levels_above;                                       // 0x5bc6c4  levels above the host, 0..99
RealtimeTimer gamecreate_cursor_timer;                        // 0x5bc6c8  500 ms cursor blink
char gamecreate_password[16];                                 // 0x5bc6cc
int gamecreate_pressed_button;                                // 0x5bc6dc
int gamecreate_difficulty;                                    // 0x5bc6e0  0..2
int gamecreate_levels_below;                                       // 0x5bc6e4  levels below the host, 0..99
char gamecreate_game_name[32];                                // 0x5bc6e8
bool gamecreate_cursor_on;                                    // 0x5bc708

// The compiler's dynamic initialiser pair for gamecreate_cursor_timer (no source of their own):
// 0x4099b0 init_realtimetimer_5bc6c8_thunk ("jmp 0x4099c0", static_initializers entry 12). Confidence: exact.
// 0x4099c0 init_realtimetimer_5bc6c8 (mov ecx, 0x5bc6c8 / jmp RealtimeTimer::RealtimeTimer). Confidence: exact.

// 0x4099d0  Confidence: exact. No progress_update and no '.' test: the suffix is always appended,
// bm_get_dimensions is called without a handle test, an empty name leaves the record cleared.
void gamecreate_load_bitmaps()
{
    float scale = ui_get_scale();
    int i;

    for (i = 0; i < GAMECREATE_NUM_BUTTONS; i++) {
        ui_button_state *b = &gamecreate_buttons[i];
        ui_button_def *d = &gamecreate_button_defs[i];
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
}

// 0x409b10  Confidence: exact (VC6 byte match, match22; (int)strlen for the signed compare; was likely). The word store "mov word [0x5bc494], 0" is read as an inlined
// memset of the two flags (two byte stores would not be merged by VC6). L"'s Game" is localize_strings[819].
void gamecreate_init()
{
    char name[1024];
    mp_char_profile *profile = mp_charselect_get_selected_profile(0);

    if (profile->flags & 0x8000000)
        gamecreate_buttons[10].flags &= ~4;
    else
        gamecreate_buttons[10].flags |= 4;
    if (profile->flags & 0x4000000)
        gamecreate_buttons[8].flags &= ~4;
    else
        gamecreate_buttons[8].flags |= 4;

    memset(gamecreate_field_active, 0, sizeof(gamecreate_field_active));

    strcpy(name, MP_PLAYER_NAME(mp_local_player));
    strcat(name, localize_strings[819]);
    if ((int)strlen(name) <= 31) {
        strcpy(gamecreate_game_name, name);
    } else {
        strncpy(gamecreate_game_name, name, 31);
        gamecreate_game_name[31] = 0;
    }

    gamecreate_password[0] = 0;
    gamecreate_max_players = 4;
    gamecreate_set_difficulty(0);
    gamecreate_levels_below = 4;
    gamecreate_levels_above = 4;
    gamecreate_mouse_mode = 0;
    gamecreate_pressed_button = -1;
}

// 0x409c60  Confidence: exact (VC6 byte match, cl 8966; was likely: where the store of gamecreate_difficulty sits
// among the three clears).
// Bit 0 of the tick entries 5 / 7 / 9 shows which difficulty is chosen.
void gamecreate_set_difficulty(int difficulty)
{
    gamecreate_difficulty = difficulty;
    gamecreate_buttons[5].flags &= ~1;
    gamecreate_buttons[7].flags &= ~1;
    gamecreate_buttons[9].flags &= ~1;

    switch (difficulty) {
    case 0:
        gamecreate_buttons[5].flags |= 1;
        break;
    case 1:
        gamecreate_buttons[7].flags |= 1;
        break;
    case 2:
        gamecreate_buttons[9].flags |= 1;
        break;
    }
}

// 0x409cd0  Confidence: likely (marked exact earlier; differs under cl 12.00.8966, match31). A click outside a focused text field takes the focus away.
int gamecreate_frame(int key)
{
    int i;

    for (i = 0; i < GAMECREATE_NUM_BUTTONS; i++)
        gamecreate_buttons[i].flags &= ~2;

    gamecreate_update_create_button();

    if (gamecreate_cursor_timer.elapsed()) {
        gamecreate_cursor_timer.SetTimeout(500);
        gamecreate_cursor_on = !gamecreate_cursor_on;
    }

    switch (gamecreate_mouse_mode) {
    case 0:
        key = gamecreate_handle_click(key);
        break;
    case 1:
        gamecreate_update_buttons();
        break;
    }

    if (mouse_was_pressed(0)) {
        for (i = 0; i < 2; i++) {
            if (gamecreate_field_active[i])
                gamecreate_field_active[i] = point_in_rect(game_mouse_x, game_mouse_y,
                                                           gamecreate_field_pos[resolution_index][i][0],
                                                           gamecreate_field_pos[resolution_index][i][1],
                                                           gamecreate_field_size[resolution_index][i][0],
                                                           gamecreate_field_size[resolution_index][i][1]);
        }
    }
    return key;
}

// 0x409db0  Confidence: exact. Begin (entry 21) is disabled while there is no game name.
void gamecreate_update_create_button()
{
    if (strlen(gamecreate_game_name))
        gamecreate_buttons[21].flags &= ~4;
    else
        gamecreate_buttons[21].flags |= 4;
}

// 0x409de0  Confidence: unsure (the text input part: the binary tests field_active[i] again after the
// key handling and returns 0 when it is clear, which no path can reach; written as the guard below. The
// default case of the field switch zeroes key, so the buffer pointer is never used uninitialised).
// The backspace and typing paths share their timer tail in the binary (cross-jumping).
int gamecreate_handle_click(int key)
{
    int i;

    gamecreate_pressed_button = -1;

    for (i = 0; i < GAMECREATE_NUM_BUTTONS; i++) {
        ui_button_state *b = &gamecreate_buttons[i];
        ui_button_def *d = &gamecreate_button_defs[i];
        if (!(b->flags & 4) && (d->flags & 3)
            && point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h)) {
            b->flags |= 2;
            gamecreate_pressed_button = i;
            if (mouse_was_pressed(0)) {
                gamecreate_mouse_mode = 1;
                return key;
            }
            break;
        }
    }

    for (i = 0; i < 2; i++) {
        if (!gamecreate_field_active[i])
            continue;
        char *text;
        int max_len;

        switch (i) {
        case 0:
            text = gamecreate_game_name;
            max_len = 31;
            break;
        case 1:
            text = gamecreate_password;
            max_len = 15;
            break;
        default:
            key = 0;
            break;
        }

        if (key) {
            int c = key_to_ascii(key);
            if (c != 0xff) {
                int len = strlen(text);
                if (len < max_len) {
                    text[len] = (char)c;
                    text[len + 1] = 0;
                    gamecreate_cursor_timer.SetTimeout(500);
                    gamecreate_cursor_on = true;
                }
            } else {
                switch (key) {
                case KEY_ESC:
                    text[0] = 0;
                    gamecreate_field_active[i] = false;
                    return 0;
                case KEY_BACKSPACE: {
                    int len = strlen(text);
                    if (len > 0) {
                        text[len - 1] = 0;
                        gamecreate_cursor_timer.SetTimeout(500);
                        gamecreate_cursor_on = true;
                    }
                    break;
                }
                case KEY_ENTER:
                    gamecreate_field_active[i] = false;
                    return 0;
                }
            }
        }

        if (!gamecreate_field_active[i])
            return 0;
        break;
    }

    if (mouse_was_pressed(0)) {
        for (i = 0; i < 2; i++) {
            if (!gamecreate_field_active[i]
                && point_in_rect(game_mouse_x, game_mouse_y,
                                 gamecreate_field_pos[resolution_index][i][0],
                                 gamecreate_field_pos[resolution_index][i][1],
                                 gamecreate_field_size[resolution_index][i][0],
                                 gamecreate_field_size[resolution_index][i][1])) {
                gamecreate_field_active[i] = true;
                gamecreate_cursor_timer.SetTimeout(500);
                gamecreate_cursor_on = true;
                return key;
            }
        }
    }

    if (key != KEY_ESC)
        return key;
    gamecreate_cancel();
    game_play_sound(0x24, 1.0f, 0, 0, 0);
    return 0;
}

// 0x40a020  Confidence: exact. Back to the game list (state 12).
void gamecreate_cancel()
{
    gameseq_set_state(12);
}

// 0x40a030  Confidence: exact. A flag-2 entry is its own action, otherwise the Press entry after the
// HiLite acts. The table flags are read before the range test, as in the binary.
void gamecreate_update_buttons()
{
    int pressed = gamecreate_pressed_button;
    int button = pressed;

    if (!(gamecreate_button_defs[pressed].flags & 2))
        button = pressed + 1;

    if (pressed >= 0 && pressed < GAMECREATE_NUM_BUTTONS) {
        ui_button_state *b = &gamecreate_buttons[button];
        if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
            b->flags |= 2;

        if (mouse_is_down(0))
            return;
        if (b->flags & 2)
            gamecreate_button_action(button);
    }
    gamecreate_mouse_mode = 0;
}

// 0x40a0c0  Confidence: exact (byte table 0x40a20c over button-2, 0..20, into the jump table 0x40a1dc;
// the three difficulty cases share the sound call: push 0/1/2, jmp).
void gamecreate_button_action(int button)
{
    switch (button) {
    case 2:
        gamecreate_players_dec();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 4:
        gamecreate_players_inc();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 6:
        gamecreate_set_difficulty(0);
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 8:
        gamecreate_set_difficulty(1);
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 10:
        gamecreate_set_difficulty(2);
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 12:
        gamecreate_levels_above_dec();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 14:
        gamecreate_levels_above_inc();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 16:
        gamecreate_levels_below_dec();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 18:
        gamecreate_levels_below_inc();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 20:
        gamecreate_cancel();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    case 22:
        gamecreate_create_game();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    }
}

// 0x40a230  Confidence: exact.
void gamecreate_players_dec()
{
    if (gamecreate_max_players > 1)
        gamecreate_max_players--;
}

// 0x40a250  Confidence: exact.
void gamecreate_players_inc()
{
    if (gamecreate_max_players < 4)
        gamecreate_max_players++;
}

// 0x40a270  Confidence: exact (minimum 0).
void gamecreate_levels_above_dec()
{
    if (gamecreate_levels_above > 0)
        gamecreate_levels_above--;
}

// 0x40a280  Confidence: exact.
void gamecreate_levels_above_inc()
{
    if (gamecreate_levels_above < 99)
        gamecreate_levels_above++;
}

// 0x40a2a0  Confidence: exact (minimum 0, not 1 as the sum.symbols comment says).
void gamecreate_levels_below_dec()
{
    if (gamecreate_levels_below > 0)
        gamecreate_levels_below--;
}

// 0x40a2b0  Confidence: exact.
void gamecreate_levels_below_inc()
{
    if (gamecreate_levels_below < 99)
        gamecreate_levels_below++;
}

// 0x40a2d0  Confidence: exact. On to the level select / lobby (state 14).
void gamecreate_create_game()
{
    mp_host_game(gamecreate_game_name, gamecreate_password, gamecreate_max_players, gamecreate_difficulty,
                 gamecreate_levels_below, gamecreate_levels_above);
    gameseq_set_state(14);
}

// 0x40a310  Confidence: exact.
void gamecreate_draw()
{
    int i;

    gr_set_color(255, 255, 255, 255);
    for (i = 0; i < GAMECREATE_NUM_BUTTONS; i++) {
        ui_button_state *b = &gamecreate_buttons[i];
        if (b->flags & 3)
            ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
    }
    for (i = 0; i < 2; i++)
        gamecreate_draw_text_field(i);
    gamecreate_draw_player_count();
    gamecreate_draw_limits();
}

// 0x40a380  Confidence: exact (the "min(0, w - text w)" spelling; setg/dec/and). The text is drawn
// right-aligned once it is wider than the field, with "|" appended while the field has the focus; the
// cursor blink chooses between the copy with "|" and the bare buffer.
void gamecreate_draw_text_field(int field)
{
    char *text;
    int clip_x, clip_y, clip_w, clip_h;
    char buf[256];
    int w, h;
    int x;

    switch (field) {
    case 0:
        text = gamecreate_game_name;
        break;
    case 1:
        text = gamecreate_password;
        break;
    default:
        return;
    }

    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    gr__set_clip(gamecreate_field_pos[resolution_index][field][0], gamecreate_field_pos[resolution_index][field][1],
                 gamecreate_field_size[resolution_index][field][0], gamecreate_field_size[resolution_index][field][1]);

    strcpy(buf, text);
    if (gamecreate_field_active[field])
        strcat(buf, "|");
    gr_get_string_size(&w, &h, buf, -1, -1);

    x = gamecreate_field_size[resolution_index][field][0] - w;
    x = x > 0 ? 0 : x;
    gr_set_color_ptr(&ui_color_text);
    if (gamecreate_cursor_on)
        gr_string(x, 0, buf, -1, gr_string_mode);
    else
        gr_string(x, 0, text, -1, gr_string_mode);

    gr__set_clip(clip_x, clip_y, clip_w, clip_h);
}

// 0x40a4f0  Confidence: likely. Both coordinates are centred with the string HEIGHT (the second
// gr_get_string_size output, [esp+4]); the width ([esp+8]) is never read. Kept as the binary does it.
void gamecreate_draw_player_count()
{
    char buf[4];
    int w, h;

    gr_set_color_ptr(&ui_color_text);
    sprintf(buf, "%d", gamecreate_max_players);
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_string(gamecreate_players_pos[resolution_index][0] - h / 2,
              gamecreate_players_pos[resolution_index][1] - h / 2, buf, -1, gr_string_mode);
}

// 0x40a570  Confidence: likely (same centring by the height as gamecreate_draw_player_count).
void gamecreate_draw_limits()
{
    char buf[4];
    int w, h;

    gr_set_color_ptr(&ui_color_text);
    sprintf(buf, "%d", gamecreate_levels_above);
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_string(gamecreate_levels_above_pos[resolution_index][0] - h / 2,
              gamecreate_levels_above_pos[resolution_index][1] - h / 2, buf, -1, gr_string_mode);

    sprintf(buf, "%d", gamecreate_levels_below);
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_string(gamecreate_levels_below_pos[resolution_index][0] - h / 2,
              gamecreate_levels_below_pos[resolution_index][1] - h / 2, buf, -1, gr_string_mode);
}
