// new_help.cpp - screen 22, the four-page controls help (Sum.exe v1.40, 0x40bf30-0x40cb55).
// Written from the Sum.exe disassembly only (tools/asm.mjs). Target: VC++ 6.0 SP (cl 12.00.8966), /O2.
//
// The unit starts at 0x40bf30 with five dynamic initialisers (init_strptrs_*): the five
// key/description tables below are built at start-up because their entries refer to
// localize_strings slots. The four help_draw_page_* functions (0x40c2b0-0x40c7c6) are untagged
// in sum.symbols but are only reachable through help_page_draw_table and use this unit's tables,
// so they belong here.
//
// Confidence per function: exact / likely / unsure (see new_credits.cpp for the meaning).

#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/key.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/localize/localize.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "engine/gamesound/gamesound.h"
#include "vsdk/os/frametime.h"
#include "summoner/interface/new_help.h"

// ---------------------------------------------------------------- .rdata (0x57edd0-0x57edff)
// Per resolution_index: the help panel rectangle.
struct help_point { int x, y; };
struct help_rect_size { int w, h; };
const help_point help_origin[3] = { { 35, 85 }, { 44, 106 }, { 56, 136 } };      // 0x57edd0
const help_rect_size help_size[3] = { { 570, 332 }, { 713, 415 }, { 912, 531 } }; // 0x57ede8 (w, h)

// ---------------------------------------------------------------- .bss (0x5bc91c-0x5bca57)
int help_paused_game;   // 0x5bc91c bit 0: help_init paused the game; bit 1: opened from the main menu
int help_page;          // 0x5bc920 0..3
int help_bkgrnd_bm;     // 0x5bc924

// A reference to a localize_strings slot, so the tables follow a language change; binding the
// references is what makes VC6 emit the init_strptrs_* dynamic initialisers (one per table, in
// definition order = static_initializers entries 15-19). Each table row is a pair of them,
// [0] the key name and [1] its description (D6500): VC6 cannot brace-initialise a struct with two
// reference members, or one whose members have constructors, but it takes this two-dimensional
// array of the one-reference struct and gives exactly the binary's stores and drawing code.
struct help_string {
    char *&str;
#ifdef _MSC_VER
    help_string(char *&r) : str(r) {}   // as saveload_month_name (D4902)
#endif
};

// 0x40bf30 init_strptrs_5bc928_thunk (jmp, static_initializers entry 15). Confidence: exact.
// 0x40bf40 init_strptrs_5bc928: the 16 slot stores of the table below. Confidence: exact (VC6: the same stores in the same order, D6500).
// 0x5bc928:
help_string help_mouse_lines[][2] = {
    { { XSTR("Left click and release", 835) },   { XSTR("Run to point", 836) } },
    { { XSTR("Left click and hold", 837) },      { XSTR("Move towards mouse cursor", 838) } },
    { { XSTR("Left click on enemy", 839) },      { XSTR("Attack enemy", 840) } },
    { { XSTR("Left click on NPC", 841) },        { XSTR("Talk to NPC", 842) } },
    { { XSTR("Left click on gold/items", 843) }, { XSTR("Pick up gold/items", 844) } },
    { { XSTR("Middle click", 845) },             { XSTR("Use nearby object", 846) } },
    { { XSTR("Right click (in combat)", 847) },  { XSTR("Perform chain attack", 848) } },
    { { XSTR("Right click on portrait", 849) },  { XSTR("Status screen", 850) } },
};

// 0x40bff0 init_strptrs_5bc968_thunk (jmp, static_initializers entry 16). Confidence: exact.
// 0x40c000 init_strptrs_5bc968: the 6 slot stores of the table below. Confidence: exact (VC6: the same stores in the same order, D6500).
// 0x5bc968:
help_string help_camera_lines[][2] = {
    { { XSTR("Mouse at edge", 851) }, { XSTR("Rotates camera", 852) } },
    { { XSTR("Mouse wheel", 853) },   { XSTR("Zoom camera", 854) } },
    { { XSTR("Arrow keys", 855) },    { XSTR("Rotate/Zoom camera", 856) } },
};

// 0x40c040 init_strptrs_5bc980_thunk (jmp, static_initializers entry 17). Confidence: exact.
// 0x40c050 init_strptrs_5bc980: the 26 slot stores of the table below. Confidence: exact (VC6: the same stores in the same order, D6500).
// 0x5bc980:
help_string help_keyboard1_lines[][2] = {
    { { XSTR("C", 857) },              { XSTR("Cast spell", 858) } },
    { { XSTR("H", 859) },              { XSTR("Hold position (solo mode)", 860) } },
    { { XSTR("I", 861) },              { XSTR("Inventory screen", 862) } },
    { { XSTR("M", 863) },              { XSTR("Automap", 864) } },
    { { XSTR("Q", 865) },              { XSTR("Quest journal", 675) } },
    { { XSTR("R", 866) },              { XSTR("Status screen", 850) } },
    { { XSTR("S", 867) },              { XSTR("Use skill", 868) } },
    { { XSTR("X", 869) },              { XSTR("Use nearby object", 846) } },
    { { XSTR("1-6", 870) },            { XSTR("Hot buttons", 871) } },
    { { XSTR("TAB", 872) },            { XSTR("Switch to next party member", 873) } },
    { { XSTR("SHIFT-TAB", 874) },      { XSTR("Switch to previous party member", 875) } },
    { { XSTR("ENTER", 876) },          { XSTR("Toggle combat mode", 877) } },
    { { XSTR("SPACE or PAUSE", 878) }, { XSTR("Pause/unpause game", 879) } },
};

// 0x40c160 init_strptrs_5bc9e8_thunk (jmp, static_initializers entry 18). Confidence: exact.
// 0x40c170 init_strptrs_5bc9e8: the 20 slot stores of the table below. Confidence: exact (VC6: the same stores in the same order, D6500).
// 0x5bc9e8:
help_string help_keyboard2_lines[][2] = {
    { { XSTR("ESC", 880) },      { XSTR("Game menu", 881) } },
    { { XSTR("F1", 882) },       { XSTR("Help screen", 883) } },
    { { XSTR("F2", 884) },       { XSTR("Options", 885) } },
    { { XSTR("F3", 886) },       { XSTR("Save game menu", 887) } },
    { { XSTR("F4", 888) },       { XSTR("Load game menu", 889) } },
    { { XSTR("F5-F9", 890) },    { XSTR("Switch to party member", 891) } },
    { { XSTR("F12", 892) },      { XSTR("Change camera mode", 893) } },
    { { XSTR("CTRL-L", 894) },   { XSTR("Quick load", 895) } },
    { { XSTR("CTRL-S", 896) },   { XSTR("Quick save", 665) } },
    { { XSTR("PrntScrn", 897) }, { XSTR("Take screen shot", 898) } },
};

// 0x40c240 init_strptrs_5bca38_thunk (jmp, static_initializers entry 19). Confidence: exact.
// 0x40c250 init_strptrs_5bca38: the 8 slot stores of the table below. Confidence: exact (VC6: the same stores in the same order, D6500).
// 0x5bca38:
help_string help_automap_lines[][2] = {
    { { XSTR("Left Click", 899) },           { XSTR("Select label / edit label text", 900) } },
    { { XSTR("Right click and drag", 901) }, { XSTR("Reposition map", 902) } },
    { { XSTR("CTRL-Left Click", 903) },      { XSTR("Place user label", 904) } },
    { { XSTR("DEL", 905) },                  { XSTR("Delete selected label", 906) } },
};

#define HELP_NUM_LINES(t) ((int)(sizeof(t) / sizeof(t[0])))

// 0x40c2b0, help page 0. Confidence: exact (VC6 byte match) (fh kept in a stack slot, the loop step fh + 2 in ebp)
static void help_draw_page_mouse()
{
    int y = help_origin[resolution_index].y;
    int fh = gr_get_font_height(-1);
    int w, h;
    int i;

    gr_set_color_ptr(&ui_color_green);
    gr_string(0x8000, y, XSTR("Mouse Controls", 907), -1, gr_string_mode);
    y += fh * 3 / 2;
    gr_set_color_ptr(&ui_color_white);
    for (i = 0; i < HELP_NUM_LINES(help_mouse_lines); i++) {
        gr_get_string_size(&w, &h, localize_string_or_invalid(&help_mouse_lines[i][1].str), -1, -1);
        gr_string(help_origin[resolution_index].x, y,
                  localize_string_or_invalid(&help_mouse_lines[i][0].str), -1, gr_string_mode);
        gr_string(help_origin[resolution_index].x + help_size[resolution_index].w - w, y,
                  localize_string_or_invalid(&help_mouse_lines[i][1].str), -1, gr_string_mode);
        y += fh + 2;
    }

    y += fh * 2;
    gr_set_color_ptr(&ui_color_green);
    gr_string(0x8000, y, XSTR("Camera Controls", 908), -1, gr_string_mode);
    y += fh * 2;
    gr_set_color_ptr(&ui_color_white);
    for (i = 0; i < HELP_NUM_LINES(help_camera_lines); i++) {
        gr_get_string_size(&w, &h, localize_string_or_invalid(&help_camera_lines[i][1].str), -1, -1);
        gr_string(help_origin[resolution_index].x, y,
                  localize_string_or_invalid(&help_camera_lines[i][0].str), -1, gr_string_mode);
        gr_string(help_origin[resolution_index].x + help_size[resolution_index].w - w, y,
                  localize_string_or_invalid(&help_camera_lines[i][1].str), -1, gr_string_mode);
        y += fh + 2;
    }
}

// 0x40c4a0, help page 1. Confidence: exact (VC6 byte match)
static void help_draw_page_keyboard1()
{
    int y = help_origin[resolution_index].y;
    int fh = gr_get_font_height(-1);
    int w, h;

    gr_set_color_ptr(&ui_color_green);
    gr_string(0x8000, y, XSTR("Keyboard Controls", 909), -1, gr_string_mode);
    y += fh * 3 / 2;
    gr_set_color_ptr(&ui_color_white);
    for (int i = 0; i < HELP_NUM_LINES(help_keyboard1_lines); i++) {
        gr_get_string_size(&w, &h, localize_string_or_invalid(&help_keyboard1_lines[i][1].str), -1, -1);
        gr_string(help_origin[resolution_index].x, y,
                  localize_string_or_invalid(&help_keyboard1_lines[i][0].str), -1, gr_string_mode);
        gr_string(help_origin[resolution_index].x + help_size[resolution_index].w - w, y,
                  localize_string_or_invalid(&help_keyboard1_lines[i][1].str), -1, gr_string_mode);
        y += fh + 2;
    }
}

// 0x40c5b0, help page 2 (same title string as page 1). Confidence: exact (VC6 byte match)
static void help_draw_page_keyboard2()
{
    int y = help_origin[resolution_index].y;
    int fh = gr_get_font_height(-1);
    int w, h;

    gr_set_color_ptr(&ui_color_green);
    gr_string(0x8000, y, XSTR("Keyboard Controls", 909), -1, gr_string_mode);
    y += fh * 3 / 2;
    gr_set_color_ptr(&ui_color_white);
    for (int i = 0; i < HELP_NUM_LINES(help_keyboard2_lines); i++) {
        gr_get_string_size(&w, &h, localize_string_or_invalid(&help_keyboard2_lines[i][1].str), -1, -1);
        gr_string(help_origin[resolution_index].x, y,
                  localize_string_or_invalid(&help_keyboard2_lines[i][0].str), -1, gr_string_mode);
        gr_string(help_origin[resolution_index].x + help_size[resolution_index].w - w, y,
                  localize_string_or_invalid(&help_keyboard2_lines[i][1].str), -1, gr_string_mode);
        y += fh + 2;
    }
}

// 0x40c6c0, help page 3. Confidence: exact (VC6 byte match)
static void help_draw_page_automap()
{
    int y = help_origin[resolution_index].y;
    int fh = gr_get_font_height(-1);
    int w, h;

    gr_set_color_ptr(&ui_color_green);
    gr_string(0x8000, y, XSTR("Automap Controls", 910), -1, gr_string_mode);
    y += fh * 3 / 2;
    gr_set_color_ptr(&ui_color_white);
    for (int i = 0; i < HELP_NUM_LINES(help_automap_lines); i++) {
        gr_get_string_size(&w, &h, localize_string_or_invalid(&help_automap_lines[i][1].str), -1, -1);
        gr_string(help_origin[resolution_index].x, y,
                  localize_string_or_invalid(&help_automap_lines[i][0].str), -1, gr_string_mode);
        gr_string(help_origin[resolution_index].x + help_size[resolution_index].w - w, y,
                  localize_string_or_invalid(&help_automap_lines[i][1].str), -1, gr_string_mode);
        y += fh + 2;
    }
}

// ---------------------------------------------------------------- .data (0x58ac48-0x58ac83)
// Strings stored in reverse order ("demohelp1024.tga" 0x58ac60, "DemoHelp640.tga" 0x58ac74).
char *help_bkgrnd_names[2] = { "DemoHelp640.tga", "demohelp1024.tga" };   // 0x58ac48
void (*help_page_draw_table[4])() = {                                      // 0x58ac50
    help_draw_page_mouse,
    help_draw_page_keyboard1,
    help_draw_page_keyboard2,
    help_draw_page_automap,
};

// 0x40c7d0, called from interface_load_all. Confidence: exact
void help_reset_page()
{
    help_page = 0;
}

// 0x40c7e0, screen 22 init; in_game is (previous screen != 14, the main menu). Confidence: exact
void help_init(bool in_game)
{
    help_paused_game = 0;
    if (in_game) {
        if (game_flags & 1) {
            if (!game_is_paused()) {
                help_paused_game |= 1;
                game_pause(0, 0);
            }
        } else {
            ui_screen_stop_player();
        }
    } else {
        help_paused_game = 2;
    }
    help_bkgrnd_bm = bm_load(help_bkgrnd_names[ui_hires], -1, -1);
}

// 0x40c850, screen 22 leave. Confidence: exact
void help_leave()
{
    if (help_paused_game & 1)
        game_unpause();
}

// 0x40c860, screen 22 per-frame input. Jump table 0x40c928 (4 targets), index bytes 0x40c938
// for key - 1 in 0..0xd0. Case bodies are laid out ESC, previous, next, which gives the source
// order; the ESC screen is an if / else of two calls (D6501). Confidence: exact (VC6 byte match)
int help_frame(int key)
{
    switch (key) {
    case KEY_ESC:
        if (help_paused_game & 2)
            interface_set_screen(14, 0);
        else
            interface_set_screen(0, 0);
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        return 0;

    case KEY_PAD8:
    case KEY_PAD4:
    case KEY_UP:
    case KEY_PAGEUP:
    case KEY_LEFT:
        help_prev_page();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        return 0;

    case KEY_ENTER:
    case KEY_SPACE:
    case KEY_PAD6:
    case KEY_PAD2:
    case KEY_RIGHT:
    case KEY_DOWN:
    case KEY_PAGEDOWN:
        help_next_page();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        return 0;

    default: {
        int dx, dy, dz;
        mouse_get_delta(&dx, &dy, &dz);
        if (dz < 0) {
            help_next_page();
            game_play_sound(0x25, 1.0f, 0, 0, 0);
        } else if (dz > 0) {
            help_prev_page();
            game_play_sound(0x25, 1.0f, 0, 0, 0);
        }
        break;
    }
    }
    return key;
}

// 0x40ca10. Confidence: exact
void help_prev_page()
{
    help_page--;
    if (help_page < 0)
        help_page = 3;
}

// 0x40ca30. Confidence: exact
void help_next_page()
{
    help_page++;
    if (help_page == 4)
        help_page = 0;
}

// 0x40ca50, screen 22 draw. Confidence: likely (the order of the two local buffers and the
// spelling of the y expression are guesses)
void help_draw()
{
    char page_text[32];
    char text[256];
    int fh = gr_get_font_height(-1);

    gr_set_color(255, 255, 255, 255);
    ui_bitmap_scaled(help_bkgrnd_bm, 0, 0, gr_bitmap_wrap_mode);
    if (help_page >= 0 && help_page < 4)
        help_page_draw_table[help_page]();

    int y = help_size[resolution_index].h - (fh * 2 + 2) + help_origin[resolution_index].y;
    gr_set_color_ptr(&ui_color_text);
    strcpy(text, XSTR("Press Left or Right arrow key to change pages", 911));
    gr_string(0x8000, y, text, -1, gr_string_mode);
    sprintf(page_text, XSTR("Page %d/%d", 912), help_page + 1, 4);
    gr_string(0x8000, y + fh + 2, page_text, -1, gr_string_mode);
}
