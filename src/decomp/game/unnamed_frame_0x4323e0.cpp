// unnamed_frame_0x4323e0.cpp - Sum.exe v1.40, code 0x4323e0-0x433331: the end of every frame (FrameEnd: the
// status message, the FPS counter, the gr_info statistics, the mouse cursor, the screenshot, the flip),
// pause, the input poll (PollInputEvents), screenshots, the cursor bitmaps, the level-script input lock and
// the Gamewide_* / targa display console commands.
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl section, sum.symbols module "-"; docs/sum-exe/source-map.md "Hand cuts inside the unnamed
// units"). Rename the file when the name turns up.
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit range: static_initializers entry 100 (0x4323e0) to cmd_gr_info (0x433280, entries 102-105 are the
// four commands at the end; sum.symbols tagged 0x432f80-0x433280 player.cpp until 2026-10-15). player.cpp
// starts with entry 106 at 0x433340.
// Data: .data 0x5931b0-0x5934cb: four initialised objects (0x5931b0-0x5931bf), then the literal pool
// ("show_framerate" 0x5931c0 .. "Gamewide_show_gr_info" 0x5934b4; within a function in reverse order of
// use). The net_stats command's strings (0x593448-0x593489) are in this pool and its variable is this
// unit's first .data object, so the four commands belong here; player.cpp's first object
// (hero_model_names, 0x5934d0) follows the pool. Pooled with earlier units (/Gf): the console strings
// (0x588c34-0x588c50).
// .bss 0x5fbf20-0x5fc029 (not in definition order, as in the other units): the five command objects, the
// cursor bitmaps, the status message and its timer, game_flags, game_mouse_x / y, the byte flags
// 0x5fc014-0x5fc022 and 0x5fc028 / 0x5fc029, game_pause_count.

#include <string.h>
#include <stdio.h>
#include "vsdk/os/console.h"
#include "vsdk/os/key.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/os.h"
#include "vsdk/os/timer.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/frametime.h"
#include "vsdk/vfile/file.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/gr_font.h"
#include "vsdk/snd/snd.h"
#include "vsdk/ui/ui.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"
#include "summoner/multi/multiendgame.h"
#include "summoner/interface/interface.h"

#define STATE_QUIT          3
#define STATE_GAME          4
#define MP_TYPE_PLAYING     10     // mp_player::type of a player in the running game (game_pause)
#define SCREENSHOT_FORMAT_TGA 0    // gr_screenshot's second argument
#define SCREENSHOT_FORMAT_JPG 1

// ---- .data (0x5931b0) ----------------------------------------------------------------------------------
bool Gamewide_show_net_stats = true;  // 0x5931b0 only cmd_net_stats uses it
int  cursor_type = -1;                // 0x5931b4
int  frame_last_mouse_x = 999;        // 0x5931b8 written by FrameEnd, no reader (name ours)
int  frame_last_mouse_y = 999;        // 0x5931bc

// ---- .bss ----------------------------------------------------------------------------------------------
int  game_flags;                      // 0x5fbfd8
int  game_mouse_x;                    // 0x5fbfdc filled by GameTick (mouse_get_pos)
int  game_mouse_y;                    // 0x5fbfe0
int  cursor_url_bm;                   // 0x5fbf2c
int  cursor_spot_walkto_bm;           // 0x5fbf3c
int  cursor_spot_target_bm;           // 0x5fbf40
int  hud_loading_bm;                  // 0x5fbf44
int  cursor_spot_invalid_bm;          // 0x5fbf4c
int  cursor_target_bm;                // 0x5fbf50
char game_status_message[128];        // 0x5fbf54
int  cursor_walk_bm;                  // 0x5fbfd4
bool game_debug_5fc014;               // 0x5fc014 no writer (ls_catacombs_frame reads it)
int  cursor_hover_bm;                 // 0x5fc018
bool cursor_visible;                  // 0x5fc01c
bool frame_show_loading_icon;         // 0x5fc01d
bool frame_debug_flag_5fc01e;         // 0x5fc01e
bool Gamewide_show_framerate;         // 0x5fc01f
bool Gamewide_show_gr_info;           // 0x5fc020
bool Gamewide_show_hud_info;          // 0x5fc021
bool Print_screen_as_targa;           // 0x5fc022
int  game_pause_count;                // 0x5fc024
// PollInputEvents: the debug key (`) has been pressed, and since then another key or a mouse button. Only
// written (names ours).
bool input_debug_key_used;            // 0x5fc028
bool input_debug_key_seen;            // 0x5fc029

// ---- code ----------------------------------------------------------------------------------------------

// 0x4323e0  init_ccmd_show_framerate_thunk (static_initializers entry 100, jmp). Confidence: exact (compiler-generated).
// 0x4323f0  init_ccmd_show_framerate: console_command::console_command. Confidence: exact (compiler-generated).
console_command ccmd_show_framerate("show_framerate", "Toggles Gamewide_show_framerate", cmd_show_framerate);   // 0x5fbfe8

// 0x432410  Confidence: exact (the console's bool command macro).
void cmd_show_framerate()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Gamewide_show_framerate = true;
        else if (arg_type & ARG_FALSE)
            Gamewide_show_framerate = false;
        else if (arg_type & ARG_NONE)
            Gamewide_show_framerate ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "show_framerate", "Gamewide_show_framerate");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "show_framerate", Gamewide_show_framerate ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x4324d0  init_realtimetimer_5fbf48_thunk (static_initializers entry 101, jmp). Confidence: exact (compiler-generated).
// 0x4324e0  init_realtimetimer_5fbf48: RealtimeTimer() (0x5019e0). Confidence: exact (compiler-generated).
// How long FrameEnd shows game_status_message. Nothing sets it in v1.40 (name ours).
RealtimeTimer game_status_message_timer;   // 0x5fbf48

// screenshot_save passes its message to a bare ret (0x4f13e0, empty_function; D3102). No address of its
// own: identical-code folding keeps one bare ret. Name ours. Confidence: unsure (whether and where the
// source defined it; a compiled-out debug print is likely).
void screenshot_debug_hook(const char *message)
{
}

// 0x4324f0  Confidence: likely (the local array initialisers and the declaration order). Print Screen:
// screenNNN.tga (targa on) or .jpg in the install directory, NNN one above the last file found. The
// message goes to the empty debug hook, so "Screenshot written to" is never shown (D3104). Game bugs: the
// file list is never freed (one leak per screenshot), and the number is read from the last file the
// directory scan returns, not the highest one (only the same on file systems that return names sorted).
void screenshot_save()
{
    int n;
    char tga_pattern[] = "screen*.tga";
    char jpg_pattern[] = "screen*.jpg";
    file_find_node list;              // a whole node as the head (0x38 bytes on the stack)
    char path[260];
    char message[256];
    char name[256];

    file_find_files(&list, path_root, Print_screen_as_targa ? tga_pattern : jpg_pattern, 1, 0);
    if (list.next == &list) {
        printf("EMPTY\n");
        n = 0;
    } else {
        printf("NOT_EMPTY\n");
        sscanf(list.prev->name, "screen%d", &n);
        n++;
    }
    sprintf(name, "screen%03d", n);
    file_make_path(path_root, name, path);
    if (Print_screen_as_targa) {
        sprintf(message, "%s %s.tga\n", "Screenshot written to", path);
        gr_screenshot(path, SCREENSHOT_FORMAT_TGA, path_root);
    } else {
        sprintf(message, "%s %s.jpg\n", "Screenshot written to", path);
        gr_screenshot(path, SCREENSHOT_FORMAT_JPG, path_root);
    }
    screenshot_debug_hook(message);
}

// 0x432660  Confidence: exact. a and b go to snd_pause_all (only b is used there). In a multiplayer game
// the host broadcasts the session state (which carries the pause flag).
void game_pause(int a, int b)
{
    if (game_is_paused())
        return;
    game_pause_count_inc();
    increase_animation_pause_counter();
    snd_pause_all(a, b);
    game_flags |= GF_PAUSED;
    if ((game_flags & GF_MULTIPLAYER) && mp_local_player && (mp_local_player->flags & 1)
        && mp_local_player->type == MP_TYPE_PLAYING)
        multi_set_netgame_state(true);
}

// 0x4326c0  Confidence: exact.
void game_unpause()
{
    if (!game_is_paused())
        return;
    game_pause_count_dec();
    decrease_animation_pause_counter();
    snd_unpause_all();
    game_flags &= ~GF_PAUSED;
    if ((game_flags & GF_MULTIPLAYER) && mp_local_player && (mp_local_player->flags & 1)
        && mp_local_player->type == MP_TYPE_PLAYING)
        multi_set_netgame_state(true);
}

// 0x432710  Confidence: exact (VC6 byte match, cl 8966; was likely: the debug key block: the second `if` on the
// flag is folded into the store's
// branch). Waits while the window is in the background (timers paused), then takes one key: Print Screen
// requests a screenshot, Shift+Esc quits (in multiplayer: leaves the game unless paused), and the debug
// Backspace (KEY_DEBUGGED is 0x8000 here but key_mark sets 0x800, so it never matches; D3103) flushes the
// keys. Those return 0; any other key is returned.
int PollInputEvents()
{
    int key;

    if (!os_foreground()) {
        game_pause_count_inc();
        while (!os_foreground()) {
            os_poll();
            timer_wait(100);
        }
        gr_set_alpha_test(true);
        game_pause_count_dec();
    }

    key = key_inkey();
    if (gameplay_no_render_60a630 != 1) {
        if (key_get_press_count(KEY_LAPOSTRO) > 0)
            input_debug_key_seen = true;
        if (input_debug_key_seen) {
            if ((key && (key & ~KEY_DEBUGGED) != KEY_LAPOSTRO) || mouse_was_pressed(0) || mouse_was_pressed(1)
                || mouse_was_pressed(2))
                input_debug_key_used = true;
        }
    }
    if (!key)
        return 0;

    switch (key) {
    case KEY_SYSRQ:
        game_flags |= GF_SCREENSHOT;
        key = 0;
        break;
    case KEY_SHIFTED | KEY_ESC:
        if (game_flags & GF_MULTIPLAYER) {
            if (!game_is_paused()) {
                if ((game_flags & GF_MULTIPLAYER) && mp_local_player && (mp_local_player->flags & 1))
                    mp_player_disconnected(2, true);
                else
                    mp_player_disconnected(0, true);
            }
        } else {
            gameseq_set_state(STATE_QUIT);
        }
        key = 0;
        break;
    case KEY_DEBUGGED | KEY_BACKSPACE:
        key_flush();
        key = 0;
        break;
    }
    return key;
}

// 0x432860  Confidence: exact. state7_frame, state_loading_frame (gameplay.cpp).
int ui_poll_and_process(void (*key_handler)(int key), bool process_ui)
{
    int key;

    do {
        key = PollInputEvents();
        if (process_ui)
            key = ui_process(key, 1);
        if (!key_handler)
            break;
        key_handler(key);
    } while (key);
    return key;
}

// 0x4328a0  Confidence: exact.
void input_flush()
{
    key_flush();
    mouse_flush();
}

// 0x4328b0  Confidence: exact (D10802: the first message's w / h at function scope, the frame rate's own w / h and a
// 28-byte buffer in its block, the mouse position in a block of its own: the binary's 0x2c frame and slots).
// GameTick, after the state's frame.
void FrameEnd()
{
    int state = gameseq_get_state();
    int w, h;

    if (game_status_message_timer.valid() && !game_status_message_timer.elapsed()) {
        if (_strnicmp(game_status_message, "Screenshot written to", strlen("Screenshot written to") - 1)
            || !(game_flags & GF_SCREENSHOT)) {
            gr_get_string_size(&w, &h, game_status_message, -1, font_small);
            gr_set_color(0, 0, 0, 192);
            gr_rect(8, 18, w + 2, h + 2, gr_rect_mode, 1.0f);
            gr_set_color(255, 255, 255, 128);
            gr_string(10, 20, game_status_message, font_small, gr_string_mode);
        }
    }
    gr::reset_clip();

    // The frame rate, only on the HUD (no screen open).
    if (Gamewide_show_framerate && !interface_get_screen()) {
        char buf[28];
        int w, h;
        sprintf(buf, "FPS=%3.1f", fps);
        gr_get_string_size(&w, &h, buf, -1, font_small);
        gr_set_color(0, 0, 0, 192);
        gr_rect(gr_screen_width() - 72, gr_screen_height() - 12, w + 4, h + 4, gr_rect_mode, 1.0f);
        gr_set_color(0x60, 0x60, 0x60, 255);
        gr_string(gr_screen_width() - 70, gr_screen_height() - 10, buf, font_small, gr_string_mode);
    }

    // Never shown: nothing adds to gr_info_total_time (D3104).
    if (Gamewide_show_gr_info && gr_info_total_time) {
        int y;
        sprintf(string_buffer, "PointRotation: %d%%", gr_info_point_rotation_time * 100 / gr_info_total_time);
        gr_string(0, 0, string_buffer, font_small, gr_string_mode);
        y = gr_get_font_height(font_small) + 1;
        sprintf(string_buffer, "Backface/FrustumCulling: %d%%", gr_info_backface_time * 100 / gr_info_total_time);
        gr_string(0, y, string_buffer, font_small, gr_string_mode);
        y += gr_get_font_height(font_small) + 1;
        sprintf(string_buffer, "Z-SortingFaces: %d%%", gr_info_zsort_time * 100 / gr_info_total_time);
        gr_string(0, y, string_buffer, font_small, gr_string_mode);
        y += gr_get_font_height(font_small) + 1;
        sprintf(string_buffer, "SmoothVertexLighting: %d%%", gr_info_smooth_lighting_time * 100 / gr_info_total_time);
        gr_string(0, y, string_buffer, font_small, gr_string_mode);
        y += gr_get_font_height(font_small) + 1;
        sprintf(string_buffer, "PolygonRendering: %d%%", gr_info_polygon_time * 100 / gr_info_total_time);
        gr_string(0, y, string_buffer, font_small, gr_string_mode);
        y += gr_get_font_height(font_small) + 1;
        sprintf(string_buffer, "EverythingElse: %d%%", gr_info_everything_else_time * 100 / gr_info_total_time);
        gr_string(0, y, string_buffer, font_small, gr_string_mode);
        y += gr_get_font_height(font_small) + 1;   // the call stays, the sum is dead
        gr_info_total_time = 0;
        gr_info_everything_else_time = 0;
        gr_info_polygon_time = 0;
        gr_info_smooth_lighting_time = 0;
        gr_info_zsort_time = 0;
        gr_info_backface_time = 0;
        gr_info_point_rotation_time = 0;
    }

    {
        int mouse_x, mouse_y;
        mouse_get_pos(&mouse_x, &mouse_y, &game_mouse_z);
        if (cursor_visible)
            cursor_draw(mouse_x, mouse_y);
        if ((game_flags & GF_MULTIPLAYER) && frame_debug_flag_5fc01e)
            debug_noop_frame_end(0, 10, 10);
        if (state == STATE_GAME && frame_show_loading_icon)
            draw_loading_icon();
        frame_last_mouse_x = mouse_x;
        frame_last_mouse_y = mouse_y;
        if (game_flags & GF_SCREENSHOT) {
            screenshot_save();
            game_flags &= ~GF_SCREENSHOT;
        }
        if (!(game_flags & GF_NO_FLIP))
            gr_flip();
        game_flags &= ~GF_NO_FLIP;
        snd_do_frame();
    }
}

// 0x432d60  Confidence: exact. FrameEnd, state 4 during a cutscene load.
void draw_loading_icon()
{
    gr_bitmap(hud_loading_bm, 291, 108, gr_mode_ui_bitmap);
}

// 0x432d80  Confidence: exact.
void game_pause_count_dec()
{
    DecrementPauseCounter();
    game_pause_count--;
}

// 0x432d90  Confidence: exact.
void game_pause_count_inc()
{
    IncrementPauseCounter();
    game_pause_count++;
}

// 0x432da0  Confidence: likely (written as the shift the code does; `& GF_PAUSED` with a bool conversion
// would be a test / setne).
bool game_is_paused()
{
    return (game_flags >> 2) & 1;
}

// 0x432db0  Confidence: exact. GameInit.
void gameplay_load_cursors()
{
    cursor_walk_bm = bm_load("cursor-walk.vbm", -1, -1);
    cursor_target_bm = bm_load("cursor-target.vbm", -1, -1);
    cursor_spot_walkto_bm = bm_load("cursorspot-walkto.vbm", -1, -1);
    cursor_spot_target_bm = bm_load("cursorspot-target.vbm", -1, -1);
    cursor_spot_invalid_bm = bm_load("cursorspot-invalid.vbm", -1, -1);
    hud_loading_bm = bm_load("hud-loading.tga", -1, -1);
    cursor_url_bm = bm_load("cursor-url.vbm", -1, -1);
    cursor_hover_bm = bm_load("cursor-hover.vbm", -1, -1);
}

// 0x432e50  Confidence: exact.
void cursor_set_type(int bm)
{
    cursor_type = bm;
}

// 0x432e60  Confidence: exact. FrameEnd: the cursor scaled with the resolution (1, 1.25, 1.6); the target
// cursor is centred on the mouse.
void cursor_draw(int x, int y)
{
    float scale;
    int w, h;

    gr_set_color(255, 255, 255, 255);
    switch (resolution_index) {
    case 2:
        scale = 1.6f;
        break;
    case 1:
        scale = 1.25f;
        break;
    case 0:
        scale = 1.0f;
        break;
    default:
        scale = 1.0f;
        break;
    }
    bm_get_dimensions(cursor_type, &w, &h);
    if (cursor_type == cursor_target_bm) {
        x += (int)(w * scale * -0.5f);
        y += (int)(h * scale * -0.5f);
    }
    gr_bitmap_stretched(cursor_type, x, y, (int)(w * scale), (int)(h * scale), 0, 0, w, h, gr_bitmap_wrap_mode);
}

// 0x432f50  Confidence: exact. Level-script cutscenes.
void script_disable_player_input()
{
    player_input[0] = 0;
    game_flags |= GF_NO_INPUT;
}

// 0x432f70  Confidence: exact.
void script_enable_player_input()
{
    game_flags &= ~GF_NO_INPUT;
}

// 0x432f80  init_ccmd_show_hud_info_thunk (static_initializers entry 102, jmp). Confidence: exact (compiler-generated).
// 0x432f90  init_ccmd_show_hud_info. Confidence: exact (compiler-generated).
console_command ccmd_show_hud_info("show_hud_info", "Toggles Gamewide_show_hud_info", cmd_show_hud_info);   // 0x5fbf20

// 0x432fb0  Confidence: exact (the console's bool command macro). Nothing reads Gamewide_show_hud_info.
void cmd_show_hud_info()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Gamewide_show_hud_info = true;
        else if (arg_type & ARG_FALSE)
            Gamewide_show_hud_info = false;
        else if (arg_type & ARG_NONE)
            Gamewide_show_hud_info ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "show_hud_info", "Gamewide_show_hud_info");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "show_hud_info", Gamewide_show_hud_info ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x433070  init_ccmd_targa_thunk (static_initializers entry 103, jmp). Confidence: exact (compiler-generated).
// 0x433080  init_ccmd_targa. Confidence: exact (compiler-generated).
console_command ccmd_targa("targa", "Toggles Print_screen_as_targa", cmd_targa);   // 0x5fbff8

// 0x4330a0  Confidence: exact (the console's bool command macro).
void cmd_targa()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Print_screen_as_targa = true;
        else if (arg_type & ARG_FALSE)
            Print_screen_as_targa = false;
        else if (arg_type & ARG_NONE)
            Print_screen_as_targa ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "targa", "Print_screen_as_targa");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "targa", Print_screen_as_targa ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x433160  init_ccmd_net_stats_thunk (static_initializers entry 104, jmp). Confidence: exact (compiler-generated).
// 0x433170  init_ccmd_net_stats. Confidence: exact (compiler-generated).
console_command ccmd_net_stats("net_stats", "Toggles Gamewide_show_net_stats", cmd_net_stats);   // 0x5fc008

// 0x433190  Confidence: exact (the console's bool command macro). Nothing reads Gamewide_show_net_stats.
void cmd_net_stats()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Gamewide_show_net_stats = true;
        else if (arg_type & ARG_FALSE)
            Gamewide_show_net_stats = false;
        else if (arg_type & ARG_NONE)
            Gamewide_show_net_stats ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "net_stats", "Gamewide_show_net_stats");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "net_stats", Gamewide_show_net_stats ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x433250  init_ccmd_gr_info_thunk (static_initializers entry 105, jmp). Confidence: exact (compiler-generated).
// 0x433260  init_ccmd_gr_info. Confidence: exact (compiler-generated).
console_command ccmd_gr_info("gr_info", "Toggles Gamewide_show_gr_info", cmd_gr_info);   // 0x5fbf30

// 0x433280  Confidence: exact (the console's bool command macro).
void cmd_gr_info()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Gamewide_show_gr_info = true;
        else if (arg_type & ARG_FALSE)
            Gamewide_show_gr_info = false;
        else if (arg_type & ARG_NONE)
            Gamewide_show_gr_info ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "gr_info", "Gamewide_show_gr_info");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "gr_info", Gamewide_show_gr_info ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}
