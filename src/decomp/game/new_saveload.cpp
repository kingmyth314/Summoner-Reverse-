// new_saveload.cpp - Sum.exe v1.40, code 0x425410-0x42723b (screen 8, save / load game)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .rdata 0x580604-0x58070b (layout tables), .data 0x590068-0x59066f (button table, scrollbar names,
// saveload_thumbnail_bm, string literals), .bss 0x5c5968-0x5f4a77.
// Pooled literals of other units: "*.sav" 0x58bcac, ".sav" 0x58bc98, "|" 0x588c10, "#01234567890" 0x588ba8,
// "ScrollBarMid640.tga" 0x588b8c / "ScrollBarMid1024.tga" 0x588b74 (see NOTES).
//
// saveload_save_mode (0x5f4a46, sum.symbols name) is TRUE IN SAVE MODE: it shows the SaveLoad-SG / -SF /
// -Save bitmaps (entries 1, 3, 13), adds the "<New Savegame>" row and makes the action button save.

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/key.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/localize/localize.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "engine/gamesound/gamesound.h"
#include "vsdk/vfile/file.h"
#include "vsdk/os/plainstring.h"
#include "engine/camera/camera.h"
#include "engine/rendering/levelrender.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"
#include "summoner/interface/new_saveload.h"

#define SAVELOAD_NUM_BUTTONS 23
#define SAVELOAD_MAX_SAVES   32
#define SAVELOAD_THUMB_W     201
#define SAVELOAD_THUMB_H     151

// One row of the list (0x130 bytes). Type and field names (*new*).
struct saveload_entry {
    char filename[0x20];   // +0x00 "summoner3.sav"
    int level;             // +0x20 level_script_table index, -1 = the "<New Savegame>" row
    int script;            // +0x24 conversation slot of the level (level_script_find_index)
    float level_time;      // +0x28 header +0xac, seconds in the level
    time_t time;           // +0x2c file time (file_find_node.time; time() for the new row)
    char name[0x100];      // +0x30 description typed by the player
};

// ---- .rdata --------------------------------------------------------------------------------
// Per resolution_index (640, 800, 1024). Names (*new*).
const int saveload_list_pos[3][2]      = { { 27, 103 },  { 36, 128 },  { 43, 165 } };    // 0x580604 first row x, y
const int saveload_list_w[3]           = { 300, 375, 480 };                               // 0x58061c row width
const int saveload_list_page[3]        = { 9, 9, 9 };                                     // 0x580628 visible rows
const int saveload_level_name_w[3]     = { 244, 305, 390 };                               // 0x580634 ellipsis width
const int saveload_track_pos[3][2]     = { { 344, 140 }, { 431, 175 }, { 552, 224 } };   // 0x580640 scrollbar track
const int saveload_track_h[3]          = { 62, 78, 99 };                                  // 0x580658
const int saveload_time_pos[3][2]      = { { 100, 302 }, { 125, 378 }, { 160, 483 } };   // 0x580664 centre x, y
const int saveload_date_pos[3][2]      = { { 100, 371 }, { 125, 464 }, { 160, 594 } };   // 0x58067c centre x, y
const int saveload_name_pos[3][2]      = { { 201, 312 }, { 251, 390 }, { 322, 499 } };   // 0x580694 description box
const int saveload_name_size[3][2]     = { { 409, 132 }, { 511, 165 }, { 654, 211 } };   // 0x5806ac w, h
const int saveload_picture_pos[3][2]   = { { 385, 64 },  { 481, 80 },  { 617, 102 } };   // 0x5806c4 thumbnail x, y
const int saveload_picture_size[3][2]  = { { 201, 151 }, { 252, 189 }, { 322, 241 } };   // 0x5806dc thumbnail w, h
// 0x5806f4: nothing references this table; it lies between saveload_picture_size and the next unit's
// float 0x58070c. Kept as an unused table of this unit (unsure); it must have external linkage, since VC6
// drops an unreferenced internal const.
extern const int saveload_unused_pos[3][2] = { { 50, 450 },  { 63, 563 },  { 80, 720 } };    // 0x5806f4

// ---- .data ---------------------------------------------------------------------------------
// 0x590068  23 x 0x40. flags: 2 hot spot that acts through the entry after it on release, 4 scroll
// arrows (act on press, repeat). 1/3/13 are the save bitmaps, 2/4/14 the load bitmaps
// (saveload_update_mode_buttons); 5/7 switch between save and load; 15 Save/Load, 17 Cancel,
// 19 Delete, 21 close.
ui_button_def saveload_button_defs[SAVELOAD_NUM_BUTTONS] = {
    { "SaveLoad-Bkgrnd",        { { 0, 0 },     { 0, 0 },     { 0, 0 } },     0 },
    { "SaveLoad-SG",            { { 48, 10 },   { 60, 13 },   { 113, 18 } },  0 },
    { "SaveLoad-LG",            { { 48, 10 },   { 60, 13 },   { 113, 18 } },  0 },
    { "SaveLoad-SF",            { { 29, 55 },   { 36, 69 },   { 61, 91 } },   0 },
    { "SaveLoad-LF",            { { 29, 55 },   { 36, 69 },   { 61, 91 } },   0 },
    { "Icon-ArrwLFHilite",      { { 21, 23 },   { 26, 29 },   { 31, 35 } },   2 },
    { "Icon-ArrwLFPress",       { { 21, 23 },   { 26, 29 },   { 31, 35 } },   0 },
    { "Icon-ArrwRTHilite",      { { 316, 23 },  { 395, 29 },  { 504, 35 } },  2 },
    { "Icon-ArrwRTPress",       { { 316, 23 },  { 395, 29 },  { 504, 35 } },  0 },
    { "Icon-ArrwUPHilite",      { { 342, 108 }, { 428, 135 }, { 546, 170 } }, 4 },
    { "Icon-ArrwUPPress",       { { 342, 108 }, { 428, 135 }, { 546, 170 } }, 0 },
    { "Icon-ArrwDNHilite",      { { 342, 215 }, { 428, 269 }, { 546, 341 } }, 4 },
    { "Icon-ArrwDNPress",       { { 342, 215 }, { 428, 269 }, { 546, 341 } }, 0 },
    { "SaveLoad-Save",          { { 389, 229 }, { 486, 286 }, { 623, 366 } }, 0 },
    { "SaveLoad-Load",          { { 389, 229 }, { 486, 286 }, { 623, 366 } }, 0 },
    { "SaveLoad-SL-HiLite",     { { 389, 229 }, { 486, 286 }, { 623, 366 } }, 2 },
    { "SaveLoad-SL-Press",      { { 389, 229 }, { 486, 286 }, { 623, 366 } }, 0 },
    { "CharCreateCancelHiLite", { { 485, 229 }, { 606, 286 }, { 777, 366 } }, 2 },
    { "CharCreateCancelPress",  { { 485, 229 }, { 606, 286 }, { 777, 366 } }, 0 },
    { "CharCreateCancelHiLite", { { 52, 420 },  { 65, 525 },  { 83, 672 } },  2 },
    { "CharCreateCancelPress",  { { 52, 420 },  { 65, 525 },  { 83, 672 } },  0 },
    { "Icon-GrnClseHilite",     { { 599, 18 },  { 749, 23 },  { 959, 32 } },  2 },
    { "Icon-GrnClsePress",      { { 599, 18 },  { 749, 23 },  { 959, 32 } },  0 },
};
char *saveload_scrollbar_names[2] = { "ScrollBarMid640.tga", "ScrollBarMid1024.tga" };   // 0x590628 (*new*)
int saveload_thumbnail_bm = -1;                                                         // 0x590630

// ---- .bss ----------------------------------------------------------------------------------
// The three objects with dynamic initialisers are defined first, in the order of their initialisers
// (static_initializers entries 77 month table, 78 cursor timer, 79 button timer), which is the reverse of
// their addresses; the rest follows the addresses. See NOTES.

// A month name, bound to its localize_strings slot (as the help tables of new_help.cpp), which is what
// makes VC6 emit init_strptrs_5f4a48 (0x425420, thunk 0x425410). Type name (*new*). Confidence: likely.
struct saveload_month_name {
    char *&text;
#ifdef _MSC_VER
    saveload_month_name(char *&r) : text(r) {}   // VC6 rejects { XSTR(..) } for a reference member otherwise (D4902)
#endif
};
saveload_month_name saveload_month_names[12] = {                          // 0x5f4a48
    { XSTR("Jan.", 702) }, { XSTR("Feb.", 703) }, { XSTR("Mar.", 704) }, { XSTR("Apr.", 705) },
    { XSTR("May", 706) },  { XSTR("Jun.", 707) }, { XSTR("Jul.", 708) }, { XSTR("Aug.", 709) },
    { XSTR("Sep.", 710) }, { XSTR("Oct.", 711) }, { XSTR("Nov.", 712) }, { XSTR("Dec.", 713) },
};
// init_realtimetimer_5e3614 (0x4254b0, thunk 0x4254a0) and init_realtimetimer_5e35f8 (0x4254d0, thunk
// 0x4254c0): "mov ecx, timer / jmp RealtimeTimer__RealtimeTimer". Confidence: exact.
RealtimeTimer saveload_cursor_timer;                       // 0x5e3614  500 ms text cursor blink
RealtimeTimer saveload_button_timer;                       // 0x5e35f8  500 ms delay, 100 ms repeat

bool saveload_cursor_visible;                              // 0x5c5968
int saveload_num_saves;                                    // 0x5c596c
ui_button_state saveload_buttons[SAVELOAD_NUM_BUTTONS];    // 0x5c5970
int saveload_drag_start_thumb_y;                           // 0x5c5b98
int saveload_scroll_pos;                                   // 0x5c5b9c  first visible row
unsigned char saveload_thumbnail_buf[SAVELOAD_THUMB_W * SAVELOAD_THUMB_H * 4];   // 0x5c5ba0 back buffer copy
                                                           //  (up to 32 bits per pixel: 0x1da3c bytes)
int saveload_pressed_button;                               // 0x5e35dc
int saveload_thumb_bm_w;                                   // 0x5e35e0  scrollbar tile size, scaled
int saveload_thumb_bm_h;                                   // 0x5e35e4
int saveload_scrollbar_bm;                                 // 0x5e35e8
int saveload_mouse_mode;                                   // 0x5e35ec  0 idle, 1 button held, 2 row
                                                           //           pressed, 3 dragging the thumb
int saveload_hover_row;                                    // 0x5e35f0
bool saveload_scrollbar_visible;                           // 0x5e35f4
// 0x5e35f8 saveload_button_timer (above)
int saveload_drag_start_mouse_y;                           // 0x5e35fc
unsigned int saveload_flags;                               // 0x5e3600  1 paused the game, 2 opened from the main menu
int saveload_thumb_x;                                      // 0x5e3604  scrollbar thumb
int saveload_thumb_y;                                      // 0x5e3608
int saveload_selected_row;                                 // 0x5e360c  visible row, -1 none
bool saveload_editing_name;                                // 0x5e3610
// 0x5e3614 saveload_cursor_timer (above)
bool saveload_list_pressed;                                // 0x5e3618
// 0x5e361c: 4 bytes nothing reads or writes
saveload_entry saveload_entries[SAVELOAD_MAX_SAVES];       // 0x5e3620
char saveload_name_input[256];                             // 0x5e5c20
int saveload_thumb_w;                                      // 0x5e5d20
int saveload_thumb_h;                                      // 0x5e5d24
unsigned short saveload_thumbnail_pixels[SAVELOAD_THUMB_W * SAVELOAD_THUMB_H]; // 0x5e5d28 RGB565, bm_create data
bool saveload_save_mode;                                   // 0x5f4a46  true = SAVE mode (see the top)
// 0x5f4a48 saveload_month_names (above)

// 0x4254e0  Confidence: likely (the scrollbar tile is scaled without the 0.5 rounding, as in the journal).
void saveload_load_bitmaps()
{
    float scale = ui_get_scale();
    int i;

    for (i = 0; i < SAVELOAD_NUM_BUTTONS; i++) {
        saveload_buttons[i].flags = 0;
        saveload_buttons[i].bitmap = -1;
        saveload_buttons[i].x = 0;
        saveload_buttons[i].y = 0;
        saveload_buttons[i].w = 0;
        saveload_buttons[i].h = 0;

        if (strlen(saveload_button_defs[i].name)) {
            char filename[128];

            strcpy(filename, saveload_button_defs[i].name);
            strcat(filename, ui_bitmap_suffix_tga[ui_hires]);
            saveload_buttons[i].bitmap = bm_load(filename, path_interface, -1);
            saveload_buttons[i].x = saveload_button_defs[i].pos[resolution_index][0];
            saveload_buttons[i].y = saveload_button_defs[i].pos[resolution_index][1];
            bm_get_dimensions(saveload_buttons[i].bitmap, &saveload_buttons[i].w, &saveload_buttons[i].h);
            saveload_buttons[i].w = (int)(saveload_buttons[i].w * scale + 0.5f);
            saveload_buttons[i].h = (int)(saveload_buttons[i].h * scale + 0.5f);
            if (i == 0)
                saveload_buttons[i].flags |= 1;
        }
    }

    saveload_scrollbar_bm = bm_load(saveload_scrollbar_names[ui_hires], path_interface, -1);
    bm_get_dimensions(saveload_scrollbar_bm, &saveload_thumb_bm_w, &saveload_thumb_bm_h);
    saveload_thumb_bm_w = (int)(saveload_thumb_bm_w * scale);
    saveload_thumb_bm_h = (int)(saveload_thumb_bm_h * scale);
    saveload_thumbnail_bm = bm_create(3, SAVELOAD_THUMB_W, SAVELOAD_THUMB_H, saveload_thumbnail_pixels, 0, -1);
}

// 0x4256a0  Confidence: likely (order of the five resets). interface_set_screen(8, &byte) passes the byte
// and prev != 14. Opened for saving from the main menu, nothing is reset or rescanned.
void saveload_init(bool save_mode, bool in_game)
{
    if (in_game)
        saveload_capture_thumbnail();

    saveload_flags = 0;
    if (in_game) {
        if (!game_is_paused()) {
            saveload_flags |= 1;
            game_pause(0, 0);
        }
    } else {
        saveload_flags = 2;
    }

    if (!save_mode || in_game) {
        saveload_editing_name = false;
        saveload_scroll_pos = 0;
        saveload_mouse_mode = 0;
        saveload_pressed_button = -1;
        saveload_hover_row = -1;
        saveload_refresh(save_mode);
        saveload_update_save_button();
    }
}

// 0x425730  Confidence: exact. The save / load switch arrows (5, 7) are disabled from the main menu and
// wherever saving is not allowed.
void saveload_update_save_button()
{
    if (!(saveload_flags & 2) && savegame_location_allowed() == 0) {
        saveload_buttons[5].flags &= ~4;
        saveload_buttons[7].flags &= ~4;
    } else {
        saveload_buttons[5].flags |= 4;
        saveload_buttons[7].flags |= 4;
    }
}

// 0x425790  Confidence: exact. The save list is refused from the main menu.
void saveload_refresh(bool save_mode)
{
    if (!(saveload_flags & 2) || !save_mode) {
        saveload_save_mode = save_mode;
        saveload_scroll_pos = 0;
        saveload_hover_row = -1;
        saveload_update_mode_buttons();
        saveload_scan_saves();
        saveload_update_scrollbar();
        if (!saveload_save_mode && saveload_num_saves > 0)
            saveload_load_thumbnail(0);
        else
            saveload_load_thumbnail(-1);
    }
}

// 0x4257f0  Confidence: exact.
void saveload_update_mode_buttons()
{
    if (saveload_save_mode) {
        saveload_buttons[1].flags |= 1;
        saveload_buttons[3].flags |= 1;
        saveload_buttons[13].flags |= 1;
        saveload_buttons[2].flags &= ~1;
        saveload_buttons[4].flags &= ~1;
        saveload_buttons[14].flags &= ~1;
    } else {
        saveload_buttons[2].flags |= 1;
        saveload_buttons[4].flags |= 1;
        saveload_buttons[14].flags |= 1;
        saveload_buttons[1].flags &= ~1;
        saveload_buttons[3].flags &= ~1;
        saveload_buttons[13].flags &= ~1;
    }
}

// 0x4258c0  Confidence: likely (the list is freed by hand: no destructor, since the function has no EH
// frame; the 0xa8 header field goes to a dead local; level_time and now share one stack slot, so they
// are declared in different blocks). Saves whose level or conversation slot no longer exists are skipped.
void saveload_scan_saves()
{
    file_find_list list;
    file_find_node *node;

    file_find_files(&list, path_savegame, "*.sav", 6, 0);
    saveload_num_saves = 0;

    for (node = list.next; node != (file_find_node *)&list; node = node->next) {
        if (saveload_num_saves >= SAVELOAD_MAX_SAVES)
            break;

        char level_name[0x80];
        char script_name[0x20];
        char description[0x100];
        int unk_a8;
        float level_time;

        if (savegame_state.read_info(node->name, level_name, script_name, &unk_a8, &level_time, description, 0)) {
            int level = level_script_find(level_name);
            if (level != -1) {
                int script = level_script_find_index(level, script_name);
                if (script >= 0) {
                    saveload_entry *e = &saveload_entries[saveload_num_saves];

                    strcpy(e->filename, node->name);
                    e->level = level;
                    e->script = script;
                    e->level_time = level_time;
                    strcpy(e->name, description);
                    e->time = node->time;
                    saveload_num_saves++;
                }
            }
        }
    }

    while (list.next != (file_find_node *)&list) {
        list.prev = list.next;
        list.next = list.next->next;
        delete list.prev;
    }
    list.next = list.prev = (file_find_node *)&list;

    if (saveload_save_mode && saveload_num_saves < SAVELOAD_MAX_SAVES) {
        time_t now;

        time(&now);
        memmove(&saveload_entries[1], &saveload_entries[0], saveload_num_saves * sizeof(saveload_entry));
        saveload_entries[0].level = -1;
        saveload_entries[0].script = -1;
        saveload_entries[0].level_time = 0.0f;
        saveload_entries[0].time = now;
        strcpy(saveload_entries[0].name, XSTR("No description", 237));
        saveload_num_saves++;
    }
}

// 0x425b00  Confidence: likely (as journal_update_list_scrollbar: the count > 0 guard can never fail).
void saveload_update_scrollbar()
{
    int page = saveload_list_page[resolution_index];

    if (saveload_num_saves <= page) {
        saveload_scrollbar_visible = false;
        saveload_buttons[9].flags |= 4;
        saveload_buttons[11].flags |= 4;
        return;
    }

    saveload_buttons[9].flags &= ~4;
    saveload_scrollbar_visible = true;
    saveload_buttons[11].flags &= ~4;

    if (saveload_num_saves > 0) {
        saveload_thumb_x = saveload_track_pos[resolution_index][0];
        saveload_thumb_y = (int)((float)saveload_scroll_pos / saveload_num_saves * saveload_track_h[resolution_index]
                                 + 0.5f)
                         + saveload_track_pos[resolution_index][1];
        saveload_thumb_w = saveload_thumb_bm_w;
        saveload_thumb_h = (int)((float)page / saveload_num_saves * saveload_track_h[resolution_index] + 0.5f);
    } else {
        saveload_thumb_x = saveload_track_pos[resolution_index][0];
        saveload_thumb_y = saveload_track_pos[resolution_index][1];
        saveload_thumb_w = saveload_thumb_bm_w;
        saveload_thumb_h = saveload_track_h[resolution_index];
    }
}

// 0x425c10  Confidence: exact. In save mode row 0 keeps the captured picture (the row index, not the list
// index, is tested, so this is also true for the first visible row after scrolling).
void saveload_load_thumbnail(int row)
{
    saveload_selected_row = row;
    if (row >= 0) {
        saveload_entry *e = &saveload_entries[saveload_scroll_pos + row];

        strcpy(saveload_name_input, e->name);
        if (!saveload_save_mode || row > 0)
            savegame_state.read_thumbnail_file(e->filename, 0);
    }
}

// 0x425c80  Confidence: exact.
void saveload_leave()
{
    if (saveload_flags & 1)
        game_unpause();
}

// 0x425c90  Confidence: exact (jump table 0x425d50). A click outside the description box ends the editing.
int saveload_frame(int key)
{
    int i;

    for (i = 0; i < SAVELOAD_NUM_BUTTONS; i++)
        saveload_buttons[i].flags &= ~2;

    saveload_update_select_button();
    saveload_update_delete_button();

    if (ui_message_box_is_active())
        return key;

    switch (saveload_mouse_mode) {
    case 0:
        key = saveload_handle_input(key);
        break;
    case 1:
        saveload_update_buttons();
        break;
    case 2:
        saveload_hover_thumbnail();
        break;
    case 3:
        saveload_drag_scrollbar();
        break;
    }

    if (mouse_was_pressed(0) && saveload_editing_name)
        saveload_editing_name = point_in_rect(game_mouse_x, game_mouse_y,
                                              saveload_name_pos[resolution_index][0],
                                              saveload_name_pos[resolution_index][1],
                                              saveload_name_size[resolution_index][0],
                                              saveload_name_size[resolution_index][1]);
    return key;
}

// 0x425d60  Confidence: exact. Save / Load (15) needs a selected row.
void saveload_update_select_button()
{
    if (saveload_selected_row >= 0)
        saveload_buttons[15].flags &= ~4;
    else
        saveload_buttons[15].flags |= 4;
}

// 0x425d80  Confidence: likely (two copies of the disable store: written as two statements). Delete (19)
// needs a row, and not the "<New Savegame>" row.
void saveload_update_delete_button()
{
    int index = saveload_scroll_pos + saveload_selected_row;

    if (saveload_save_mode) {
        if (index < 0 || (index == 0 && saveload_entries[0].level == -1)) {
            saveload_buttons[19].flags |= 4;
            return;
        }
    } else if (index < 0) {
        saveload_buttons[19].flags |= 4;
        return;
    }
    saveload_buttons[19].flags &= ~4;
}

// 0x425de0  Confidence: likely (the shape of the text input part and the cursor blink expression; the
// typing and backspace paths jump straight to the SetTimeout, the others test the timer first).
int saveload_handle_input(int key)
{
    int i;

    saveload_pressed_button = -1;
    saveload_hover_row = -1;

    for (i = 0; i < SAVELOAD_NUM_BUTTONS; i++) {
        if (!(saveload_buttons[i].flags & 4) && (saveload_button_defs[i].flags & 7)
            && point_in_rect(game_mouse_x, game_mouse_y, saveload_buttons[i].x, saveload_buttons[i].y,
                             saveload_buttons[i].w, saveload_buttons[i].h)) {
            saveload_buttons[i].flags |= 2;
            saveload_pressed_button = i;
            if (saveload_button_defs[i].flags & 2) {
                if (mouse_was_pressed(0)) {
                    saveload_mouse_mode = 1;
                    saveload_button_timer.invalidate();
                    return key;
                }
            } else if (saveload_button_defs[i].flags & 4) {
                if (mouse_was_pressed(0)) {
                    saveload_mouse_mode = 1;
                    saveload_button_action(saveload_pressed_button + 1);
                    saveload_button_timer.SetTimeout(500);
                    return key;
                }
            }
            break;
        }
    }

    // The scrollbar thumb (no visibility test).
    if (point_in_rect(game_mouse_x, game_mouse_y, saveload_thumb_x, saveload_thumb_y, saveload_thumb_w,
                      saveload_thumb_h)
        && mouse_was_pressed(0)) {
        saveload_mouse_mode = 3;
        saveload_drag_start_thumb_y = saveload_thumb_y;
        saveload_drag_start_mouse_y = game_mouse_y;
        return key;
    }

    // The rows: a double click saves or loads at once.
    int rows = saveload_num_saves;
    if (rows >= saveload_list_page[resolution_index])
        rows = saveload_list_page[resolution_index];
    int font_h = gr_get_font_height(-1);

    if (point_in_rect(game_mouse_x, game_mouse_y, saveload_list_pos[resolution_index][0],
                      saveload_list_pos[resolution_index][1], saveload_list_w[resolution_index], font_h * rows)) {
        int y = 0;

        for (i = 0; i < rows; i++, y += font_h) {
            if (point_in_rect(game_mouse_x, game_mouse_y, saveload_list_pos[resolution_index][0],
                              saveload_list_pos[resolution_index][1] + y, saveload_list_w[resolution_index],
                              font_h)) {
                saveload_hover_row = i;
                saveload_list_pressed = true;
                if (mouse_was_double_clicked(0)) {
                    saveload_mouse_mode = 2;
                    saveload_save_or_load();
                    game_play_sound(0x23, 1.0f, 0, 0, 0);
                    return key;
                }
                if (mouse_was_pressed(0)) {
                    saveload_mouse_mode = 2;
                    return key;
                }
                break;
            }
        }
    }

    // The description box.
    if (saveload_editing_name) {
        bool changed = false;

        if (saveload_selected_row < 0)
            return 0;

        if (key) {
            int c = key_to_ascii(key);

            if (c != 0xff) {
                int len = strlen(saveload_name_input);
                if (len < 255) {
                    saveload_name_input[len] = (char)c;
                    saveload_name_input[len + 1] = 0;
                    changed = true;
                }
            } else {
                switch (key) {
                case KEY_ESC:
                    saveload_name_input[0] = 0;
                    // fall through
                case KEY_ENTER:
                    saveload_editing_name = false;
                    break;
                case KEY_BACKSPACE: {
                    int len = strlen(saveload_name_input);
                    if (len > 0) {
                        saveload_name_input[len - 1] = 0;
                        changed = true;
                    }
                    break;
                }
                }
            }
        }

        if (changed || saveload_cursor_timer.elapsed()) {
            saveload_cursor_timer.SetTimeout(500);
            saveload_cursor_visible = changed || !saveload_cursor_visible;
        }

        if (!saveload_editing_name)
            return 0;
        if (changed)
            return 0;
    } else if (saveload_save_mode && saveload_selected_row >= 0 && mouse_was_pressed(0)
               && point_in_rect(game_mouse_x, game_mouse_y, saveload_name_pos[resolution_index][0],
                                saveload_name_pos[resolution_index][1], saveload_name_size[resolution_index][0],
                                saveload_name_size[resolution_index][1])) {
        saveload_editing_name = true;
        saveload_cursor_timer.SetTimeout(500);
        saveload_cursor_visible = true;
        return key;
    }

    if (key == KEY_ESC) {
        saveload_close();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        return 0;
    }

    int dx, dy, dz;
    mouse_get_delta(&dx, &dy, &dz);
    if (dz > 0)
        saveload_page_up();
    else if (dz < 0)
        saveload_page_down();
    return key;
}

// 0x426260  Confidence: likely (setle / dec / and is VC6's "x > 0 ? x : 0").
void saveload_page_up()
{
    if (saveload_scroll_pos >= 0) {
        int pos = saveload_scroll_pos - saveload_list_page[resolution_index];
        saveload_scroll_pos = pos > 0 ? pos : 0;
        saveload_update_scrollbar();
    }
}

// 0x426290  Confidence: exact.
void saveload_page_down()
{
    int page = saveload_list_page[resolution_index];
    int max = saveload_num_saves - page;

    if (saveload_scroll_pos < max) {
        int pos = saveload_scroll_pos + page;
        saveload_scroll_pos = pos < max ? pos : max;
        saveload_update_scrollbar();
    }
}

// 0x4262c0  Confidence: exact. Back to the main menu (14) when opened from there, else to the game (0).
void saveload_close()
{
    if (saveload_flags & 2)
        interface_set_screen(14, 0);
    else
        interface_set_screen(0, 0);
}

// 0x4262f0  Confidence: exact (saveload_load_selected is a tail jump).
void saveload_save_or_load()
{
    if (saveload_save_mode) {
        if (!saveload_try_save()) {
            hud_show_message(XSTR("Game Saved", 714));
            saveload_close();
        }
    } else {
        saveload_load_selected();
    }
}

// 0x426320  Confidence: likely (the 1..3 range test as one if). Returns 0 when saved now. The overwrite
// test reads saveload_entries[saveload_selected_row] without saveload_scroll_pos (a bug in the original:
// after scrolling it checks the wrong row); the second selected_row >= 0 test is in the binary too.
int saveload_try_save()
{
    int reason;

    if (!saveload_save_mode || saveload_selected_row < 0)
        return 1;

    reason = savegame_location_allowed();
    if (reason > 0 && reason <= 3)
        return 1;

    if (saveload_selected_row >= 0 && saveload_entries[saveload_selected_row].level != -1) {
        saveload_confirm_overwrite();
        return 1;
    }
    return saveload_save_to_slot();
}

// 0x426370  Confidence: likely (declaration order of the two locals). The new row gets the first unused
// summoner<n>.sav. Returns 0 when savegame_write succeeded.
int saveload_save_to_slot()
{
    File f;
    char filename[64];
    int index = saveload_scroll_pos + saveload_selected_row;

    if (index == 0 && saveload_entries[0].level == -1) {
        int i = 0;
        do {
            sprintf(filename, "summoner%d%s", i++, ".sav");
        } while (f.find(filename, path_savegame));
    } else {
        strcpy(filename, saveload_entries[index].filename);
    }

    conversation_db.save_state();
    return !savegame_state.write(filename, 0, saveload_name_input);
}

// 0x426480  Confidence: exact. The level starts; savegame_load_pending reads the file once it is loaded.
int saveload_load_selected()
{
    if (saveload_save_mode)
        return 1;
    if (saveload_selected_row < 0)
        return 1;

    saveload_entry *e = &saveload_entries[saveload_scroll_pos + saveload_selected_row];
    savegame_state.set_pending_load(e->filename, 0);
    game_change_level(level_script_table[e->level].name, conversation_save_table[(e->level * 7 + e->script) * 4]);
    return 0;
}

// 0x426500  Confidence: exact.
void saveload_confirm_overwrite()
{
    char *buttons[2] = { XSTR("Yes", 654), XSTR("No", 655) };

    ui_message_box_show(XSTR("Warning", 645), XSTR("Overwrite existing savegame?", 718), -1, -1, 2, buttons,
                        saveload_confirm_overwrite_cb, 0);
}

// 0x426540  Confidence: exact. Button 0 = Yes.
void saveload_confirm_overwrite_cb(int button)
{
    if (button == 0) {
        if (saveload_save_to_slot() == 0) {
            hud_show_message(XSTR("Game Saved", 714));
            saveload_close();
        }
    }
}

// 0x426570  Confidence: exact (byte table 0x42665c over button-6, 0..16, into 0x426640; the code order
// puts Delete before Cancel / close).
void saveload_button_action(int button)
{
    switch (button) {
    case 6:
    case 8:
        saveload_refresh(!saveload_save_mode);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 10:
        saveload_scroll_up();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 12:
        saveload_scroll_down();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 16:
        saveload_save_or_load();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 20:
        saveload_confirm_delete();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 18:
    case 22:
        saveload_close();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    }
}

// 0x426670  Confidence: exact.
void saveload_scroll_up()
{
    if (saveload_scroll_pos > 0) {
        saveload_scroll_pos--;
        saveload_update_scrollbar();
    }
}

// 0x426690  Confidence: exact.
void saveload_scroll_down()
{
    if (saveload_scroll_pos < saveload_num_saves - saveload_list_page[resolution_index]) {
        saveload_scroll_pos++;
        saveload_update_scrollbar();
    }
}

// 0x4266c0  Confidence: exact.
void saveload_confirm_delete()
{
    char *buttons[2] = { XSTR("Yes", 654), XSTR("No", 655) };

    ui_message_box_show(XSTR("Confirm delete", 717), XSTR("Are you sure?", 682), -1, -1, 2, buttons,
                        saveload_delete_save, 0);
}

// 0x426700  Confidence: exact. Message box callback; button 0 = Yes.
void saveload_delete_save(int button)
{
    File f;
    char *buttons[1] = { XSTR("Ok", 213) };

    if (button == 0) {
        if (f.Delete(saveload_entries[saveload_scroll_pos + saveload_selected_row].filename, path_savegame))
            ui_message_box_show(XSTR("Error", 715), XSTR("Unable to delete", 716), -1, -1, 1, buttons, 0, 0);
        else
            saveload_refresh(saveload_save_mode);
    }
}

// 0x4267e0  Confidence: exact. The scroll arrows act on press (saveload_handle_input) and then repeat
// every 100 ms; the other buttons act on release if the press timer was invalidated.
void saveload_update_buttons()
{
    int pressed = saveload_pressed_button;
    int button = pressed + 1;

    if (pressed >= 0 && pressed < SAVELOAD_NUM_BUTTONS) {
        if (point_in_rect(game_mouse_x, game_mouse_y, saveload_buttons[button].x, saveload_buttons[button].y,
                          saveload_buttons[button].w, saveload_buttons[button].h))
            saveload_buttons[button].flags |= 2;

        if (mouse_is_down(0)) {
            if (saveload_button_timer.valid() && saveload_button_timer.elapsed()
                && (saveload_buttons[button].flags & 2)) {
                saveload_button_action(button);
                saveload_button_timer.SetTimeout(100);
            }
            return;
        }
        if ((saveload_buttons[button].flags & 2) && !saveload_button_timer.valid())
            saveload_button_action(button);
    }
    saveload_mouse_mode = 0;
}

// 0x4268b0  Confidence: exact. Releasing the mouse on the pressed row selects it.
void saveload_hover_thumbnail()
{
    int font_h = gr_get_font_height(-1);

    saveload_list_pressed = point_in_rect(game_mouse_x, game_mouse_y, saveload_list_pos[resolution_index][0],
                                          saveload_list_pos[resolution_index][1] + saveload_hover_row * font_h,
                                          saveload_list_w[resolution_index], font_h);
    if (!mouse_is_down(0)) {
        if (saveload_list_pressed)
            saveload_load_thumbnail(saveload_hover_row);
        saveload_mouse_mode = 0;
    }
}

// 0x426930  Confidence: likely (the y difference goes through a stack int for the fild).
void saveload_drag_scrollbar()
{
    int y = game_mouse_y - saveload_track_pos[resolution_index][1] - saveload_drag_start_mouse_y
          + saveload_drag_start_thumb_y;
    int pos = (int)((float)y / saveload_track_h[resolution_index] * saveload_num_saves + 0.5f);
    int max;

    pos = pos > 0 ? pos : 0;
    max = saveload_num_saves - saveload_list_page[resolution_index];
    saveload_scroll_pos = pos < max ? pos : max;
    saveload_update_scrollbar();

    if (!mouse_is_down(0))
        saveload_mouse_mode = 0;
}

// 0x4269c0  Confidence: exact.
void saveload_draw()
{
    int i;

    gr_set_color(255, 255, 255, 255);
    for (i = 0; i < SAVELOAD_NUM_BUTTONS; i++) {
        if (saveload_buttons[i].flags & 3)
            ui_bitmap_scaled(saveload_buttons[i].bitmap, saveload_buttons[i].x, saveload_buttons[i].y,
                             gr_bitmap_wrap_mode);
    }
    saveload_draw_list();
    saveload_draw_scrollbar();
    saveload_draw_info();
}

// 0x426a20  Confidence: likely (the x of the time: list_w - w + x; the division by 3600 is a multiplication
// by the float 1/3600 in the binary, see NOTES). One buffer holds the level name and then the time.
void saveload_draw_list()
{
    char buf[256];
    int w, h;
    int i;
    int rows = saveload_num_saves;

    if (rows >= saveload_list_page[resolution_index])
        rows = saveload_list_page[resolution_index];

    int x = saveload_list_pos[resolution_index][0];
    int y = saveload_list_pos[resolution_index][1];
    int font_h = gr_get_font_height(-1);

    for (i = 0; i < rows; i++) {
        int index = saveload_scroll_pos + i;
        saveload_entry *e = &saveload_entries[index];

        if ((i == saveload_hover_row && saveload_list_pressed) || i == saveload_selected_row)
            gr_set_color_ptr(&ui_color_highlight);
        else
            gr_set_color_ptr(&ui_color_text);

        if (saveload_save_mode && index == 0 && saveload_entries[0].level == -1) {
            gr_string(x, y, XSTR("<New Savegame>", 719), -1, gr_string_mode);
        } else {
            int hours;

            saveload_format_level_name(buf, e->level);
            gr_string(x, y, buf, -1, gr_string_mode);
            hours = (int)(e->level_time / 3600.0f);
            sprintf(buf, "%02d:%02d", hours, (int)(e->level_time - hours * 3600) / 60);
            gr_get_string_size(&w, &h, buf, -1, -1);
            gr_string(saveload_list_w[resolution_index] - w + x, y, buf, -1, gr_string_mode);
        }
        y += font_h;
    }
}

// 0x426be0  Confidence: likely (layout of the found / not-found branches; the i < 51 test after the loop
// is in the binary). The localized name is cut at its first digit or '#' and shortened with "...".
void saveload_format_level_name(char *out, int level)
{
    char buf[256];
    int i;

    if (level < 0) {
        strcpy(out, XSTR("Unknown", 352));
        return;
    }

    for (i = 0; i < 51; i++) {
        if (!_stricmp(level_display_names[i].name, level_script_table[level].name))
            break;
    }

    if (i < 51) {
        int n = strcspn(localize_string_or_invalid(level_display_names[i].text), "#01234567890");
        if (n) {
            strncpy(buf, localize_string_or_invalid(level_display_names[i].text), n);
            buf[n] = 0;
        } else {
            strcpy(buf, localize_string_or_invalid(level_display_names[i].text));
        }
    } else {
        strcpy(buf, XSTR("Unknown", 352));
    }
    gr_string_truncate_ellipsis(out, buf, saveload_level_name_w[resolution_index], -1);
}

// 0x426d10  Confidence: likely (stack order of the locals; the 4 bytes between buf and line). File time
// as "h:mm am", date as "Mon. d, yyyy", then the description in up to 9 lines; while editing a "|" is
// appended and hidden on the off phase of the blink by cutting the last line one character short.
void saveload_draw_info()
{
    if (saveload_selected_row < 0)
        return;

    saveload_entry *e = &saveload_entries[saveload_scroll_pos + saveload_selected_row];
    struct tm *t = localtime(&e->time);
    char ampm[4];
    char buf[256];
    int lengths[9], starts[9];
    int w, h;
    int hour, x, y, font_h, n, i;

    gr_set_color_ptr(&ui_color_text);

    strcpy(ampm, t->tm_hour < 12 ? "am" : "pm");
    hour = t->tm_hour;
    if (hour == 0)
        hour = 12;
    else if (hour > 12)
        hour -= 12;
    sprintf(buf, "%d:%02d %s", hour, t->tm_min, ampm);
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_string(saveload_time_pos[resolution_index][0] - w / 2, saveload_time_pos[resolution_index][1], buf, -1,
              gr_string_mode);

    sprintf(buf, "%s %d, %d", localize_string_or_invalid(&saveload_month_names[t->tm_mon].text), t->tm_mday,
            t->tm_year + 1900);
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_string(saveload_date_pos[resolution_index][0] - w / 2, saveload_date_pos[resolution_index][1], buf, -1,
              gr_string_mode);

    x = saveload_name_pos[resolution_index][0];
    y = saveload_name_pos[resolution_index][1];
    font_h = gr_get_font_height(-1);
    strcpy(buf, saveload_name_input);
    if (saveload_editing_name)
        strcat(buf, "|");

    n = gr_split_str(lengths, starts, buf, saveload_name_size[resolution_index][0], 9, 0, -1);
    for (i = 0; i < n; i++) {
        char line[256];

        strncpy(line, buf + starts[i], lengths[i]);
        if (saveload_editing_name && !saveload_cursor_visible && i == n - 1)
            line[lengths[i] - 1] = 0;
        else
            line[lengths[i]] = 0;
        gr_string(x, y, line, -1, gr_string_mode);
        y += font_h;
    }

    saveload_draw_thumbnail();
}

// 0x426fd0  Confidence: likely (if / ternary spelling). The captured picture for row 0 in save mode,
// else the one savegame_read_thumbnail_file loaded.
void saveload_draw_thumbnail()
{
    int bm;

    if (saveload_save_mode && saveload_selected_row == 0)
        bm = saveload_thumbnail_bm;
    else
        bm = savegame_thumbnail_bm;

    gr_set_color(255, 255, 255, 255);
    gr_bitmap_stretched(bm, saveload_picture_pos[resolution_index][0], saveload_picture_pos[resolution_index][1],
                        saveload_picture_size[resolution_index][0], saveload_picture_size[resolution_index][1],
                        0, 0, SAVELOAD_THUMB_W, SAVELOAD_THUMB_H, gr_bitmap_wrap_mode);
}

// 0x427050  Confidence: likely (operand order of the last tile y, as journal_draw_list_scrollbar).
void saveload_draw_scrollbar()
{
    int tiles, i;

    if (!saveload_scrollbar_visible)
        return;

    gr_set_color(255, 255, 255, 255);
    tiles = saveload_thumb_h / saveload_thumb_bm_h;
    for (i = 0; i < tiles; i++)
        ui_bitmap_scaled(saveload_scrollbar_bm, saveload_thumb_x, saveload_thumb_bm_h * i + saveload_thumb_y,
                         gr_bitmap_wrap_mode);
    ui_bitmap_scaled(saveload_scrollbar_bm, saveload_thumb_x,
                     saveload_thumb_y - saveload_thumb_bm_h + saveload_thumb_h, gr_bitmap_wrap_mode);
}

// 0x427100  Confidence: likely (the 32-bit conversion written with unsigned short parts, for the
// movzx dx, dl; the row / column counters count down in the binary). The level is rendered into a
// 201x151 clip with game_flags 0x20000 set and read back; any format but 3 (565) and 7 (32-bit) gives
// a black picture.
void saveload_capture_thumbnail()
{
    int clip_x, clip_y, clip_w, clip_h;
    int format;

    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    gr__set_clip(0, 0, SAVELOAD_THUMB_W, SAVELOAD_THUMB_H);
    gr_setup_3d(&camera_view_orient, &camera_view_pos, camera_fov, 1, 1);
    gr_clear();
    game_flags |= 0x20000;
    levelrender_render_scene();
    game_flags &= ~0x20000;

    format = gr_read_backbuffer(0, 0, SAVELOAD_THUMB_W, SAVELOAD_THUMB_H, saveload_thumbnail_buf);
    if (format == 3) {
        memcpy(saveload_thumbnail_pixels, saveload_thumbnail_buf, sizeof(saveload_thumbnail_pixels));
    } else if (format == 7) {
        unsigned char *src = saveload_thumbnail_buf;
        unsigned short *dst = saveload_thumbnail_pixels;
        int x, y;

        for (y = 0; y < SAVELOAD_THUMB_H; y++) {
            for (x = 0; x < SAVELOAD_THUMB_W; x++) {
                unsigned short r = src[0] >> 3;
                unsigned short g = src[1] >> 2;
                unsigned short b = src[2] >> 3;

                *dst++ = (unsigned short)((((r << 6) | g) << 5) | b);
                src += 4;
            }
        }
    } else {
        memset(saveload_thumbnail_pixels, 0, sizeof(saveload_thumbnail_pixels));
    }

    gr_mark_texture_dirty(saveload_thumbnail_bm);
    gr__set_clip(clip_x, clip_y, clip_w, clip_h);
}
