// new_charselect.cpp - Sum.exe v1.40, code 0x4063b0-0x408582 (screen 16, multiplayer character select / create)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .rdata 0x57ea20-0x57eb33 (layout tables), .data 0x589828-0x58a0e7 (button table, scrollbar names,
// name box origin, strings), .bss 0x5b6808-0x5bc3f7. "|" (0x588c10), "\n" (0x588ba4) and the ScrollBarMid
// names (0x588b8c / 0x588b74) are pooled copies in earlier units' .data.
//
// Three scrolled lists share the per-list arrays: 0 the character list (mp_char_profiles), 1 the
// description the player types for a new character (mp_charselect_text_input, wrapped), 2 the character
// info panel (mp_charselect_info_text, opened by the Profile button). The three focus flags:
// mp_charselect_name_focus = the name box has the focus, mp_charselect_desc_focus = the description box
// has the focus, mp_charselect_info_open = the info panel is open (button table flag 8 = entry belongs to
// the panel).

#include <ctype.h>
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
#include "vsdk/vfile/file.h"
#include "vsdk/os/registry.h"
#include "vsdk/os/plainstring.h"
#include "engine/characterinfo/characterinfo.h"
#include "summoner/multi/multi.h"
#include "summoner/interface/new_charselect.h"

#define CHARSELECT_NUM_BUTTONS 34
#define CHARSELECT_MAX_CHARS   64

// ---- .rdata --------------------------------------------------------------------------------
// Per resolution_index (640, 800, 1024) and list (0 characters, 1 description input, 2 info panel).
const int mp_charselect_list_pos[3][3][2] = {                                // 0x57ea20 x, y of the first row
    { { 45, 43 }, { 352, 279 }, { 190, 185 } },
    { { 56, 54 }, { 440, 349 }, { 238, 231 } },
    { { 72, 69 }, { 563, 446 }, { 304, 295 } },
};
const int mp_charselect_list_width[3][3]   = { { 207, 227, 239 }, { 259, 284, 299 }, { 331, 363, 382 } };  // 0x57ea68
const int mp_charselect_list_page[3][3]    = { { 23, 6, 10 }, { 23, 6, 10 }, { 23, 6, 10 } };         // 0x57ea8c rows
const int mp_charselect_track_pos[3][3][2] = {                               // 0x57eab0 scrollbar track top
    { { 265, 77 }, { 592, 316 }, { 445, 209 } },
    { { 331, 96 }, { 740, 395 }, { 556, 261 } },
    { { 424, 123 }, { 949, 506 }, { 713, 338 } },
};
const int mp_charselect_track_h[3][3]      = { { 272, 17, 100 }, { 340, 21, 125 }, { 435, 27, 156 } };  // 0x57eaf8
const int mp_charselect_name_box_size[3][2] = { { 240, 14 }, { 300, 18 }, { 384, 22 } };            // 0x57eb1c w, h

// ---- .data ---------------------------------------------------------------------------------
// 0x589828  34 x 0x40 (sum.symbols' mp_charselect_button_defs, 0x589864, is the flags field of entry 0).
// flags: 1 HiLite hot spot acting on release through the Press entry after it, 2 hot spot acting on
// press with auto-repeat through the entry after it, 4 hot spot that acts itself on release (the four
// class check boxes), 8 belongs to the info panel (drawn and hit-tested only while it is open).
ui_button_def mp_charselect_button_defs[CHARSELECT_NUM_BUTTONS] = {
    { "PXO-CharCreate",          { { 0, 0 },     { 0, 0 },     { 0, 0 } },     0 },
    { "checkbox",                { { 518, 153 }, { 648, 191 }, { 828, 249 } }, 0 },   // class 0 tick
    { "checkbox-hilite",         { { 518, 153 }, { 648, 191 }, { 828, 249 } }, 4 },
    { "checkbox",                { { 518, 174 }, { 648, 218 }, { 828, 283 } }, 0 },   // class 1 tick
    { "checkbox-hilite",         { { 518, 174 }, { 648, 218 }, { 828, 283 } }, 4 },
    { "checkbox",                { { 518, 195 }, { 648, 244 }, { 828, 318 } }, 0 },   // class 2 tick
    { "checkbox-hilite",         { { 518, 195 }, { 648, 244 }, { 828, 318 } }, 4 },
    { "checkbox",                { { 518, 216 }, { 648, 270 }, { 828, 351 } }, 0 },   // class 3 tick
    { "checkbox-hilite",         { { 518, 216 }, { 648, 270 }, { 828, 351 } }, 4 },
    { "CharCreateDeleteHiLite",  { { 32, 394 },  { 40, 493 },  { 48, 638 } },  1 },   // delete
    { "CharCreateDeletePress",   { { 32, 394 },  { 40, 493 },  { 48, 638 } },  0 },
    { "CharCreateProfileHiLite", { { 116, 394 }, { 145, 493 }, { 177, 638 } }, 1 },   // info panel
    { "CharCreateProfilePress",  { { 116, 394 }, { 145, 493 }, { 177, 638 } }, 0 },
    { "CharCreateSelectHiLite",  { { 204, 394 }, { 255, 493 }, { 322, 638 } }, 1 },   // accept
    { "CharCreateSelectPress",   { { 204, 394 }, { 255, 493 }, { 322, 638 } }, 0 },
    { "GameListCreateHiLite",    { { 353, 394 }, { 441, 493 }, { 554, 638 } }, 1 },   // create
    { "GameListCreatePress",     { { 353, 394 }, { 441, 493 }, { 554, 638 } }, 0 },
    { "CharCreateCancelHiLite",  { { 511, 420 }, { 639, 525 }, { 820, 673 } }, 1 },   // back
    { "CharCreateCancelPress",   { { 511, 420 }, { 639, 525 }, { 820, 673 } }, 0 },
    { "Profile-bkgrnd",          { { 170, 122 }, { 213, 153 }, { 272, 194 } }, 8 },   // info panel
    { "Icon-GrnClseHilite",      { { 427, 143 }, { 534, 179 }, { 683, 229 } }, 9 },   // close panel
    { "Icon-GrnClsePress",       { { 427, 143 }, { 534, 179 }, { 683, 229 } }, 8 },
    { "Icon-ArrwUPHilite",       { { 263, 45 },  { 329, 56 },  { 418, 69 } },  2 },   // list 0
    { "Icon-ArrwUPPress",        { { 263, 45 },  { 329, 56 },  { 418, 69 } },  0 },
    { "Icon-ArrwDNHilite",       { { 263, 361 }, { 329, 451 }, { 418, 574 } }, 2 },
    { "Icon-ArrwDNPress",        { { 263, 361 }, { 329, 451 }, { 418, 574 } }, 0 },
    { "Icon-ArrwUPHilite",       { { 590, 283 }, { 738, 354 }, { 942, 450 } }, 2 },   // list 1
    { "Icon-ArrwUPPress",        { { 590, 283 }, { 738, 354 }, { 942, 450 } }, 0 },
    { "Icon-ArrwDNHilite",       { { 590, 345 }, { 738, 431 }, { 942, 548 } }, 2 },
    { "Icon-ArrwDNPress",        { { 590, 345 }, { 738, 431 }, { 942, 548 } }, 0 },
    { "Icon-ArrwUPHilite",       { { 441, 178 }, { 551, 223 }, { 706, 286 } }, 10 },  // list 2 (panel)
    { "Icon-ArrwUPPress",        { { 441, 178 }, { 551, 223 }, { 706, 286 } }, 8 },
    { "Icon-ArrwDNHilite",       { { 441, 322 }, { 551, 403 }, { 706, 509 } }, 10 },
    { "Icon-ArrwDNPress",        { { 441, 322 }, { 551, 403 }, { 706, 509 } }, 8 },
};
// 0x58a0a8. The strings are pooled copies at 0x588b8c / 0x588b74 (see new_journal.cpp).
char *mp_charselect_scrollbar_names[2] = { "ScrollBarMid640.tga", "ScrollBarMid1024.tga" };
// 0x58a0b0 x, y of the name box per resolution (sum.symbols: mp_charselect_name_box_pos, y at mp_charselect_name_box_y). Not const,
// so it is in .data, unlike its size table.
int mp_charselect_name_box_pos[3][2] = { { 352, 89 }, { 440, 111 }, { 563, 142 } };
// then "Multiplayer Char" (0x58a0c8), "*%s" (0x58a0dc), ".plr" (0x58a0e0).

// ---- .bss ----------------------------------------------------------------------------------
bool mp_charselect_scrollbar_visible[3];                          // 0x5b6808
int mp_charselect_scrollbar_size[3][2];                           // 0x5b680c thumb w, h
int mp_charselect_selected[3];                                    // 0x5b6824 [0] selected character; [1] / [2]
                                                                  // only reset (sum.symbols types it int)
bool mp_charselect_name_focus;                                    // 0x5b6830 name box has the focus
int mp_charselect_drag_start_thumb;                               // 0x5b6834
bool mp_charselect_info_open;                                     // 0x5b6838 info panel open
char mp_charselect_text_input[260];                               // 0x5b683c description (max 255 + nul; the
                                                                  // 4 bytes up to 0x5b6940 are never used)
ui_button_state mp_charselect_buttons[CHARSELECT_NUM_BUTTONS];   // 0x5b6940
RealtimeTimer mp_charselect_repeat_timer;                         // 0x5b6c70
char mp_charselect_info_text[516];                                // 0x5b6c74 (size up to 0x5b6e78 assumed)
mp_char_record mp_char_profiles[CHARSELECT_MAX_CHARS];            // 0x5b6e78
RealtimeTimer mp_charselect_cursor_timer;                         // 0x5bc378 500 ms cursor blink
int mp_charselect_pressed_button;                                 // 0x5bc37c (unnamed in sum.symbols)
int mp_charselect_row_counts[3];                                  // 0x5bc380
int mp_charselect_scroll_pos[3];                                  // 0x5bc38c
int mp_charselect_scrollbar_bm_w;                                 // 0x5bc398
int mp_charselect_scrollbar_bm_h;                                 // 0x5bc39c
int mp_charselect_scrollbar_pos[3][2];                            // 0x5bc3a0 thumb x, y
char mp_charselect_name[32];                                      // 0x5bc3b8
int mp_charselect_scrollbar_bm;                                   // 0x5bc3d8
int mp_charselect_species;                                        // 0x5bc3dc char_infos index, -1 none
int mp_charselect_drag_start_y;                                   // 0x5bc3e0
bool mp_charselect_desc_focus;                                    // 0x5bc3e4 description box has the focus
int mp_charselect_drag_scrollbar_index;                           // 0x5bc3e8 list under the mouse / dragged
int mp_charselect_mouse_mode;                                     // 0x5bc3ec 0 idle, 1 button held, 2 dragging
int mp_charselect_hover_row;                                      // 0x5bc3f0
bool mp_charselect_cursor_on;                                     // 0x5bc3f4

// The compiler's dynamic initialiser pairs of the two timers (no source of their own; the definition order
// above gives the entry order):
// 0x4063b0 init_realtimetimer_5b6c70_thunk ("jmp 0x4063c0", static_initializers entry 9). Confidence: exact.
// 0x4063c0 init_realtimetimer_5b6c70 (mp_charselect_repeat_timer: mov ecx, 0x5b6c70 / jmp RealtimeTimer::RealtimeTimer). Confidence: exact.
// 0x4063d0 init_realtimetimer_5bc378_thunk (jmp, entry 10). Confidence: exact.
// 0x4063e0 init_realtimetimer_5bc378 (mp_charselect_cursor_timer). Confidence: exact.

// 0x4063f0  Confidence: exact. Same loader as gamecreate_load_bitmaps; entries 0 (background) and 19 (info
// panel background) are marked always drawn. The scrollbar tile is scaled without rounding.
void mp_charcreate_load_bitmaps()
{
    float scale = ui_get_scale();
    int i;

    for (i = 0; i < CHARSELECT_NUM_BUTTONS; i++) {
        ui_button_state *b = &mp_charselect_buttons[i];
        ui_button_def *d = &mp_charselect_button_defs[i];

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
            bm_get_dimensions(b->bitmap, &b->w,
                              &b->h);
            b->w = (int)(b->w * scale + 0.5f);
            b->h = (int)(b->h * scale + 0.5f);
            if (i == 0 || i == 19)
                b->flags |= 1;
        }
    }

    mp_charselect_scrollbar_bm = bm_load(mp_charselect_scrollbar_names[ui_hires], path_interface, -1);
    bm_get_dimensions(mp_charselect_scrollbar_bm, &mp_charselect_scrollbar_bm_w, &mp_charselect_scrollbar_bm_h);
    mp_charselect_scrollbar_bm_w = (int)(mp_charselect_scrollbar_bm_w * scale);
    mp_charselect_scrollbar_bm_h = (int)(mp_charselect_scrollbar_bm_h * scale);
}

// 0x406590  Confidence: likely (list 0 stores scroll 0 / selection -1 twice, before and after the strlen
// test; written out twice. The clamp is "> 0 ? s : 0" here and "< 0 ? 0 : s" in mp_char_create: setle vs
// setl). Selects the character saved under "Multiplayer Char" and scrolls it into view.
void mp_charselect_init()
{
    char saved[256];
    int i;

    mp_session_reset();
    if (os_registry_read_string_nodefault(0, "Multiplayer Char", saved))
        saved[0] = 0;

    for (i = 0; i < 3; i++) {
        if (i == 0) {
            mp_char_scan_files();
            mp_charselect_scroll_pos[0] = 0;
            mp_charselect_selected[0] = -1;
            if (strlen(saved)) {
                int j;

                mp_charselect_scroll_pos[0] = 0;
                mp_charselect_selected[0] = -1;
                for (j = 0; j < mp_charselect_row_counts[0]; j++) {
                    if (!strcmp(mp_char_profiles[j].name, saved)) {
                        int first = mp_charselect_row_counts[0] - mp_charselect_list_page[resolution_index][0];
                        mp_charselect_selected[0] = j;
                        if (j < first)
                            first = j;
                        mp_charselect_scroll_pos[0] = first > 0 ? first : 0;
                        break;
                    }
                }
            }
        } else {
            mp_charselect_row_counts[i] = 0;
            mp_charselect_scroll_pos[i] = 0;
            mp_charselect_selected[i] = -1;
        }
        mp_charselect_update_scrollbar(i);
    }

    mp_charselect_name[0] = 0;
    mp_charselect_name_focus = false;
    mp_charselect_select_species(-1);
    mp_charselect_mouse_mode = 0;
    mp_charselect_pressed_button = -1;
    mp_charselect_text_input[0] = 0;
    mp_charselect_desc_focus = false;
    mp_charselect_info_open = false;
}

// 0x406710  Confidence: exact (VC6 byte match) (the list free loop: the head's prev field is the temporary, read as the
// three statements below; a list macro or inline in the original is likely). At most 64 records; a file
// whose header cannot be read is skipped (its slot is reused). A directory failure leaves the selection.
void mp_char_scan_files()
{
    char pattern[16];
    file_find_entry list;
    file_find_entry *e;

    mp_charselect_row_counts[0] = 0;
    if (file_make_path_dirs(path_players))
        return;

    sprintf(pattern, "*%s", ".plr");
    file_find_files(&list, path_players, pattern, 5, 0);

    for (e = list.next; e != &list; e = e->next) {
        mp_char_record *rec;

        if (mp_charselect_row_counts[0] >= CHARSELECT_MAX_CHARS)
            break;
        rec = &mp_char_profiles[mp_charselect_row_counts[0]];
        strcpy(rec->filename, e->name);
        if (!mp_player_file_read_summary(rec->name, rec->filename))
            mp_charselect_row_counts[0]++;
    }

    while (list.next != &list) {
        list.prev = list.next;
        list.next = list.next->next;
        delete list.prev;
    }

    mp_charselect_selected[0] = -1;
}

// 0x406820  Confidence: exact (VC6 byte match) (count and page live in the argument and spare slots for fild; the order
// of the four stores of the count <= 0 branch). Buttons 22 + 4 * list / 24 + 4 * list are the list's
// up / down arrows.
void mp_charselect_update_scrollbar(int list)
{
    if (list == -1)
        return;

    int page = mp_charselect_list_page[resolution_index][list];
    int count = mp_charselect_row_counts[list];

    if (count <= page) {
        mp_charselect_scrollbar_visible[list] = false;
        mp_charselect_buttons[22 + list * 4].flags |= 4;
        mp_charselect_buttons[24 + list * 4].flags |= 4;
        return;
    }

    mp_charselect_scrollbar_visible[list] = true;
    mp_charselect_buttons[22 + list * 4].flags &= ~4;
    mp_charselect_buttons[24 + list * 4].flags &= ~4;

    if (count <= 0) {
        mp_charselect_scrollbar_pos[list][0] = mp_charselect_track_pos[resolution_index][list][0];
        mp_charselect_scrollbar_pos[list][1] = mp_charselect_track_pos[resolution_index][list][1];
        mp_charselect_scrollbar_size[list][0] = mp_charselect_scrollbar_bm_w;
        mp_charselect_scrollbar_size[list][1] = mp_charselect_track_h[resolution_index][list];
    } else {
        mp_charselect_scrollbar_pos[list][0] = mp_charselect_track_pos[resolution_index][list][0];
        mp_charselect_scrollbar_pos[list][1] = (int)(mp_charselect_track_h[resolution_index][list]
                                                     * ((float)mp_charselect_scroll_pos[list] / count) + 0.5f)
                                               + mp_charselect_track_pos[resolution_index][list][1];
        mp_charselect_scrollbar_size[list][0] = mp_charselect_scrollbar_bm_w;
        mp_charselect_scrollbar_size[list][1] = (int)(mp_charselect_track_h[resolution_index][list]
                                                      * ((float)page / count) + 0.5f);
    }
}

// 0x406970  Confidence: exact (VC6 byte match) (where the store of mp_charselect_species sits among the clears). Bit 0 of
// the tick entries 1 / 3 / 5 / 7 shows the class; the tick is indexed by the char_infos index, which
// assumes the first four records are the four classes in species order.
void mp_charselect_select_species(int species)
{
    mp_charselect_species = charinfo_find_by_species(species);
    mp_charselect_buttons[1].flags &= ~1;
    mp_charselect_buttons[3].flags &= ~1;
    mp_charselect_buttons[5].flags &= ~1;
    mp_charselect_buttons[7].flags &= ~1;
    if (mp_charselect_species >= 0)
        mp_charselect_buttons[1 + mp_charselect_species * 2].flags |= 1;
}

// 0x4069e0  Confidence: exact. A click outside a focused text box takes the focus away.
int mp_charselect_frame(int key)
{
    int i;

    if (ui_message_box_is_active())
        return key;

    for (i = 0; i < CHARSELECT_NUM_BUTTONS; i++)
        mp_charselect_buttons[i].flags &= ~2;

    mp_charselect_update_button_states();
    mp_charselect_update_create_button();

    if (mp_charselect_cursor_timer.elapsed()) {
        mp_charselect_cursor_timer.SetTimeout(500);
        mp_charselect_cursor_on = !mp_charselect_cursor_on;
    }

    if (ui_message_box_is_active())
        return key;

    switch (mp_charselect_mouse_mode) {
    case 0:
        key = mp_charselect_handle_click(key);
        break;
    case 1:
        mp_charselect_update_buttons();
        break;
    case 2:
        mp_charselect_drag_scrollbar();
        break;
    }

    if (mouse_was_pressed(0)) {
        if (mp_charselect_name_focus) {
            int x = mp_charselect_name_box_pos[resolution_index][0];
            int y = mp_charselect_name_box_pos[resolution_index][1];
            int w = mp_charselect_name_box_size[resolution_index][0];
            int h = mp_charselect_name_box_size[resolution_index][1];
            mp_charselect_name_focus = point_in_rect(game_mouse_x, game_mouse_y, x, y, w, h);
        }
        if (mp_charselect_desc_focus) {
            int x = mp_charselect_list_pos[resolution_index][1][0];
            int y = mp_charselect_list_pos[resolution_index][1][1];
            int w = mp_charselect_list_width[resolution_index][1];
            int h = gr_get_font_height(-1) * mp_charselect_list_page[resolution_index][1];
            mp_charselect_desc_focus = point_in_rect(game_mouse_x, game_mouse_y, x, y, w, h);
        }
    }
    return key;
}

// 0x406b40  Confidence: exact. Delete (9), Profile (11) and Select (13) need a selected character.
void mp_charselect_update_button_states()
{
    if (mp_charselect_selected[0] >= 0) {
        mp_charselect_buttons[9].flags &= ~4;
        mp_charselect_buttons[11].flags &= ~4;
        mp_charselect_buttons[13].flags &= ~4;
    } else {
        mp_charselect_buttons[9].flags |= 4;
        mp_charselect_buttons[11].flags |= 4;
        mp_charselect_buttons[13].flags |= 4;
    }
}

// 0x406bb0  Confidence: exact. Create (15) needs a name and a class.
void mp_charselect_update_create_button()
{
    if (strlen(mp_charselect_name) && mp_charselect_species >= 0)
        mp_charselect_buttons[15].flags &= ~4;
    else
        mp_charselect_buttons[15].flags |= 4;
}

// 0x406bf0  Confidence: likely (the "if (!focus) return 0" after each key block, as in
// gamecreate_handle_click; the list loop skips list 2 with a test inside the loop; the row loop of list 1
// is skipped but the list still becomes the wheel target; the ESC case is a one-case switch). The typing
// paths share their timer tails in the binary. Names: letters, digits, '_' and '-', starting with a letter
// or '_', at most 31. The description: any character but a leading space, at most 255.
int mp_charselect_handle_click(int key)
{
    int rows;
    int font_h;
    int i;

    mp_charselect_pressed_button = -1;
    mp_charselect_drag_scrollbar_index = -1;
    mp_charselect_hover_row = -1;

    for (i = 0; i < CHARSELECT_NUM_BUTTONS; i++) {
        if (!(mp_charselect_buttons[i].flags & 4)
            && ((mp_charselect_button_defs[i].flags >> 3) & 1) == mp_charselect_info_open
            && (mp_charselect_button_defs[i].flags & 7)
            && point_in_rect(game_mouse_x, game_mouse_y, mp_charselect_buttons[i].x, mp_charselect_buttons[i].y,
                             mp_charselect_buttons[i].w, mp_charselect_buttons[i].h)) {
            mp_charselect_pressed_button = i;
            mp_charselect_buttons[i].flags |= 2;
            if (mp_charselect_button_defs[i].flags & 5) {
                if (mouse_was_pressed(0)) {
                    mp_charselect_mouse_mode = 1;
                    mp_charselect_repeat_timer.invalidate();
                    return key;
                }
            } else if (mp_charselect_button_defs[i].flags & 2) {
                if (mouse_was_pressed(0)) {
                    mp_charselect_mouse_mode = 1;
                    mp_charselect_button_action(mp_charselect_pressed_button + 1);
                    mp_charselect_repeat_timer.SetTimeout(500);
                    return key;
                }
            }
            break;
        }
    }

    // scrollbar thumbs: lists 0 and 1 while the info panel is closed, list 2 while it is open
    for (i = 0; i < 3; i++) {
        if (mp_charselect_info_open == (i == 2)
            && point_in_rect(game_mouse_x, game_mouse_y, mp_charselect_scrollbar_pos[i][0],
                             mp_charselect_scrollbar_pos[i][1], mp_charselect_scrollbar_size[i][0],
                             mp_charselect_scrollbar_size[i][1])
            && mouse_was_pressed(0)) {
            mp_charselect_mouse_mode = 2;
            mp_charselect_drag_scrollbar_index = i;
            mp_charselect_drag_start_y = game_mouse_y;
            mp_charselect_drag_start_thumb = mp_charselect_scrollbar_pos[i][1];
            return key;
        }
    }

    font_h = gr_get_font_height(-1);
    if (mp_charselect_info_open) {
        mp_charselect_drag_scrollbar_index = 2;
    } else {
        for (i = 0; i < 3; i++) {
            if (i == 2)
                continue;
            rows = mp_charselect_row_counts[i];
            if (rows >= mp_charselect_list_page[resolution_index][i])
                rows = mp_charselect_list_page[resolution_index][i];
            if (point_in_rect(game_mouse_x, game_mouse_y, mp_charselect_list_pos[resolution_index][i][0],
                              mp_charselect_list_pos[resolution_index][i][1],
                              mp_charselect_list_width[resolution_index][i], rows * font_h)) {
                mp_charselect_drag_scrollbar_index = i;
                if (i != 1) {
                    int row, y;

                    for (row = 0, y = 0; row < rows; row++, y += font_h) {
                        if (point_in_rect(game_mouse_x, game_mouse_y, mp_charselect_list_pos[resolution_index][i][0],
                                          mp_charselect_list_pos[resolution_index][i][1] + y,
                                          mp_charselect_list_width[resolution_index][i], font_h)) {
                            mp_charselect_hover_row = row;
                            if (mouse_was_double_clicked(0)) {
                                mp_charselect_selected[i] = mp_charselect_scroll_pos[i] + row;
                                mp_charselect_save_choice();
                                game_play_sound(0x23, 1.0f, 0, 0, 0);
                                return key;
                            }
                            if (mouse_was_pressed(0)) {
                                mp_charselect_selected[i] = mp_charselect_scroll_pos[i] + row;
                                return key;
                            }
                        }
                    }
                }
                break;
            }
        }
    }

    if (mp_charselect_name_focus) {
        if (key) {
            int c = key_to_ascii(key);
            if (c != 0xff) {
                int len = strlen(mp_charselect_name);
                if (len < 31) {
                    bool ok;
                    if (len > 0)
                        ok = isalpha(c) || isdigit(c) || c == '_' || c == '-';
                    else
                        ok = isalpha(c) || c == '_';
                    if (ok) {
                        mp_charselect_name[len] = (char)c;
                        mp_charselect_name[len + 1] = 0;
                        mp_charselect_cursor_timer.SetTimeout(500);
                        mp_charselect_cursor_on = true;
                        return 0;
                    }
                }
            } else {
                switch (key) {
                case KEY_ESC:
                    mp_charselect_name[0] = 0;
                    mp_charselect_name_focus = false;
                    return 0;
                case KEY_BACKSPACE: {
                    int len = strlen(mp_charselect_name);
                    if (len > 0) {
                        mp_charselect_name[len - 1] = 0;
                        mp_charselect_cursor_timer.SetTimeout(500);
                        mp_charselect_cursor_on = true;
                        return 0;
                    }
                    break;
                }
                case KEY_ENTER:
                    mp_charselect_name_focus = false;
                    return 0;
                }
            }
        }
        if (!mp_charselect_name_focus)
            return 0;
    } else if (mp_charselect_desc_focus) {
        if (key) {
            int c = key_to_ascii(key);
            if (c != 0xff) {
                int len = strlen(mp_charselect_text_input);
                if ((c != ' ' || len > 0) && len < 255) {
                    mp_charselect_text_input[len] = (char)c;
                    mp_charselect_text_input[len + 1] = 0;
                    mp_charselect_cursor_timer.SetTimeout(500);
                    mp_charselect_cursor_on = true;
                    mp_charselect_wrap_text_input();
                    return 0;
                }
            } else {
                switch (key) {
                case KEY_ESC:
                    mp_charselect_text_input[0] = 0;
                    mp_charselect_desc_focus = false;
                    mp_charselect_wrap_text_input();
                    return 0;
                case KEY_BACKSPACE: {
                    int len = strlen(mp_charselect_text_input);
                    if (len > 0) {
                        mp_charselect_text_input[len - 1] = 0;
                        mp_charselect_cursor_timer.SetTimeout(500);
                        mp_charselect_cursor_on = true;
                        mp_charselect_wrap_text_input();
                        return 0;
                    }
                    break;
                }
                case KEY_ENTER:
                    mp_charselect_desc_focus = false;
                    mp_charselect_wrap_text_input();
                    return 0;
                }
            }
        }
        if (!mp_charselect_desc_focus) {
            mp_charselect_wrap_text_input();
            return 0;
        }
    }

    if (!mp_charselect_info_open && mouse_was_pressed(0)) {
        if (!mp_charselect_name_focus
            && point_in_rect(game_mouse_x, game_mouse_y, mp_charselect_name_box_pos[resolution_index][0],
                             mp_charselect_name_box_pos[resolution_index][1],
                             mp_charselect_name_box_size[resolution_index][0],
                             mp_charselect_name_box_size[resolution_index][1])) {
            mp_charselect_name_focus = true;
            mp_charselect_cursor_timer.SetTimeout(500);
            mp_charselect_cursor_on = true;
            return key;
        }
        if (!mp_charselect_desc_focus
            && point_in_rect(game_mouse_x, game_mouse_y, mp_charselect_list_pos[resolution_index][1][0],
                             mp_charselect_list_pos[resolution_index][1][1],
                             mp_charselect_list_width[resolution_index][1],
                             gr_get_font_height(-1) * mp_charselect_list_page[resolution_index][1])) {
            mp_charselect_desc_focus = true;
            mp_charselect_wrap_text_input();
            mp_charselect_cursor_timer.SetTimeout(500);
            mp_charselect_cursor_on = true;
            return key;
        }
    }

    switch (key) {
    case KEY_ESC:
        if (mp_charselect_info_open)
            mp_charselect_close_info();
        else
            mp_charselect_back();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        return 0;
    }

    {
        int dx, dy, dz;
        mouse_get_delta(&dx, &dy, &dz);
        if (dz > 0)
            mp_charselect_page_up(mp_charselect_drag_scrollbar_index);
        else if (dz < 0)
            mp_charselect_page_down(mp_charselect_drag_scrollbar_index);
    }
    return key;
}

// 0x4072e0  Confidence: likely (the min / clamp spelling of the scroll position: the last page is shown).
// Wraps the description (with the "|" cursor while it has the focus) to count its lines.
void mp_charselect_wrap_text_input()
{
    char buf[260];
    int lengths[32];
    int starts[32];
    int first;

    strcpy(buf, mp_charselect_text_input);
    if (mp_charselect_desc_focus)
        strcat(buf, "|");
    mp_charselect_row_counts[1] = gr_split_str(lengths, starts, buf, mp_charselect_list_width[resolution_index][1],
                                               32, 0, -1);
    first = mp_charselect_row_counts[1] < mp_charselect_list_page[resolution_index][1]
            ? mp_charselect_row_counts[1] : mp_charselect_list_page[resolution_index][1];
    first = mp_charselect_row_counts[1] - first;
    mp_charselect_scroll_pos[1] = first > 0 ? first : 0;
    mp_charselect_update_scrollbar(1);
}

// 0x4073c0  Confidence: exact (VC6 byte match) (two stores of the element, as journal_list_page_up).
void mp_charselect_page_up(int list)
{
    if (list == -1)
        return;
    if (mp_charselect_scroll_pos[list] > 0) {
        mp_charselect_scroll_pos[list] -= mp_charselect_list_page[resolution_index][list];
        mp_charselect_scroll_pos[list] = mp_charselect_scroll_pos[list] > 0 ? mp_charselect_scroll_pos[list] : 0;
        mp_charselect_update_scrollbar(list);
    }
}

// 0x407410  Confidence: likely (as journal_list_page_down).
void mp_charselect_page_down(int list)
{
    if (list == -1)
        return;
    int last = mp_charselect_row_counts[list] - mp_charselect_list_page[resolution_index][list];
    if (mp_charselect_scroll_pos[list] < last) {
        mp_charselect_scroll_pos[list] += mp_charselect_list_page[resolution_index][list];
        mp_charselect_scroll_pos[list] = mp_charselect_scroll_pos[list] < last ? mp_charselect_scroll_pos[list] : last;
        mp_charselect_update_scrollbar(list);
    }
}

// 0x407460  Confidence: exact. Remembers the character and goes on to the game list (state 12) or, with
// bit 0 of the PXO config, to state 11.
void mp_charselect_save_choice()
{
    if (mp_charselect_selected[0] < 0)
        return;
    os_registry_write_string(0, "Multiplayer Char", mp_char_profiles[mp_charselect_selected[0]].name);
    if (pxo_cfg_flags & 1)
        gameseq_set_state(11);
    else
        gameseq_set_state(12);
}

// 0x4074b0  Confidence: exact. Back to state 1.
void mp_charselect_back()
{
    mp_charselect_selected[0] = -1;
    gameseq_set_state(1);
}

// 0x4074d0  Confidence: exact. Closes the info panel.
void mp_charselect_close_info()
{
    mp_charselect_info_open = false;
}

// 0x4074e0  Confidence: exact (byte table 0x407630 over button-2, 0..31, into the jump table 0x4075f0;
// the class cases share the call: push 0..3, jmp). Button 31 (up arrow of the info panel) scrolls DOWN like
// 33: both byte-table entries point at the same case, so the original has scroll_down(2) twice.
void mp_charselect_button_action(int button)
{
    switch (button) {
    case 2:
        mp_charselect_select_species(0);
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 4:
        mp_charselect_select_species(1);
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 6:
        mp_charselect_select_species(2);
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 8:
        mp_charselect_select_species(3);
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 10:
        mp_char_confirm_delete();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 12:
        mp_char_format_info();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 14:
        mp_charselect_save_choice();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 16:
        mp_char_create();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 18:
        mp_charselect_back();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    case 21:
        mp_charselect_close_info();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    case 23:
        mp_charselect_scroll_up(0);
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 25:
        mp_charselect_scroll_down(0);
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 27:
        mp_charselect_scroll_up(1);
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 29:
        mp_charselect_scroll_down(1);
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 31:
    case 33:
        mp_charselect_scroll_down(2);
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    }
}

// 0x407650  Confidence: exact.
void mp_charselect_scroll_up(int list)
{
    if (list == -1)
        return;
    if (mp_charselect_scroll_pos[list] > 0) {
        mp_charselect_scroll_pos[list]--;
        mp_charselect_update_scrollbar(list);
    }
}

// 0x407680  Confidence: exact.
void mp_charselect_scroll_down(int list)
{
    if (list == -1)
        return;
    if (mp_charselect_scroll_pos[list] < mp_charselect_row_counts[list]
                                         - mp_charselect_list_page[resolution_index][list]) {
        mp_charselect_scroll_pos[list]++;
        mp_charselect_update_scrollbar(list);
    }
}

// 0x4076d0  Confidence: likely (the name is tested with strlen twice in the binary, written twice; the
// search loop breaks and is tested after, unlike mp_charselect_init). Writes the .plr file, rescans and
// selects the new character, then clears the name, class and description.
void mp_char_create()
{
    char *ok = XSTR("Ok", 213);
    int i;

    if (!strlen(mp_charselect_name))
        return;
    if (!strlen(mp_charselect_name))
        return;
    if (mp_charselect_species < 0)
        return;

    if (mp_charselect_row_counts[0] >= CHARSELECT_MAX_CHARS) {
        ui_message_box_show(XSTR("Error", 715), XSTR("Maximum number of multiplayer characters reached.", 776),
                            -1, -1, 1, &ok, 0, 0);
        return;
    }

    switch (mp_player_file_create(mp_charselect_name, mp_charselect_species, mp_charselect_text_input)) {
    case 0:
        mp_char_scan_files();
        mp_charselect_scroll_pos[0] = 0;
        for (i = 0; i < mp_charselect_row_counts[0]; i++) {
            if (!strcmp(mp_char_profiles[i].name, mp_charselect_name))
                break;
        }
        if (i < mp_charselect_row_counts[0]) {
            int first = mp_charselect_row_counts[0] - mp_charselect_list_page[resolution_index][0];
            mp_charselect_selected[0] = i;
            if (i < first)
                first = i;
            mp_charselect_scroll_pos[0] = first < 0 ? 0 : first;
        }
        mp_charselect_update_scrollbar(0);
        mp_charselect_name[0] = 0;
        mp_charselect_select_species(-1);
        mp_charselect_text_input[0] = 0;
        mp_charselect_scroll_pos[1] = 0;
        mp_charselect_row_counts[1] = 0;
        mp_charselect_update_scrollbar(1);
        break;
    case 1:
        ui_message_box_show(XSTR("Error", 715), XSTR("Player already exists.", 777), -1, -1, 1, &ok, 0, 0);
        break;
    case 2:
        ui_message_box_show(XSTR("Error", 715), XSTR("Unable to create player directory.", 778), -1, -1, 1, &ok,
                            0, 0);
        break;
    case 3:
        ui_message_box_show(XSTR("Error", 715), XSTR("Unable to open file for writing.", 779), -1, -1, 1, &ok,
                            0, 0);
        break;
    }
}

// 0x4078b0  Confidence: exact (sum.symbols module "-", but it lies inside this unit and is called only from
// it). L654 / L655 are the button labels (Yes / No).
void mp_char_confirm_delete()
{
    char *buttons[2];

    buttons[0] = XSTR("Yes", 654);
    buttons[1] = XSTR("No", 655);
    ui_message_box_show(XSTR("Warning", 645), XSTR("Permanently delete character?", 780), -1, -1, 2, buttons,
                        mp_char_confirm_delete_cb, 0);
}

// 0x4078f0  Confidence: exact (tail jump). Button 0 confirms.
void mp_char_confirm_delete_cb(int button)
{
    if (button == 0)
        mp_char_delete_file();
}

// 0x407900  Confidence: exact. File has a destructor, hence the EH frame (handler 0x57bd7b).
void mp_char_delete_file()
{
    File f;

    if (mp_charselect_selected[0] >= 0
        && f.find(mp_char_profiles[mp_charselect_selected[0]].filename, path_players)
        && !f.Delete(0, PATH_ANY)) {
        mp_charselect_selected[0] = -1;
        mp_char_scan_files();
        mp_charselect_update_scrollbar(0);
    }
}

// 0x4079c0  Confidence: exact (VC6 byte match) (the order of the three initial stores; whether the class lines were a
// switch on the species, as the jump table 0x407d20 suggests, or a table). Builds the info panel text:
// the wrapped "Name:" line, the class, "Level:", a blank line, then the wrapped description; the row count
// follows the lines.
void mp_char_format_info()
{
    int count;
    char line[128];
    int lengths[32];
    int starts[32];
    char buf[128];
    int i;

    mp_char_record *p = (mp_char_record *)mp_charselect_get_selected_profile(0);
    if (!p)
        return;

    mp_charselect_info_text[0] = 0;
    mp_charselect_scroll_pos[2] = 0;
    mp_charselect_row_counts[2] = 0;

    sprintf(buf, XSTR("Name: %s\n", 781), p->name);
    count = gr_split_str(lengths, starts, buf, mp_charselect_list_width[resolution_index][2], 32, 0, -1);
    for (i = 0; i < count; i++) {
        strncpy(line, buf + starts[i], lengths[i]);
        line[lengths[i]] = 0;
        strcat(mp_charselect_info_text, line);
        strcat(mp_charselect_info_text, "\n");
    }
    mp_charselect_row_counts[2] += count;

    switch (char_infos[p->char_info].species) {
    case 0:
        strcpy(line, XSTR("Class: Summoner\n", 782));
        break;
    case 1:
        strcpy(line, XSTR("Class: Fighter\n", 783));
        break;
    case 2:
        strcpy(line, XSTR("Class: Thief\n", 784));
        break;
    case 3:
        strcpy(line, XSTR("Class: Magic User\n", 785));
        break;
    default:
        strcpy(line, XSTR("Class: Unknown\n", 786));
        break;
    }
    strcat(mp_charselect_info_text, line);
    mp_charselect_row_counts[2]++;

    sprintf(line, XSTR("Level: %d\n", 787), p->level);
    strcat(mp_charselect_info_text, line);
    strcat(mp_charselect_info_text, "\n");
    mp_charselect_row_counts[2] += 2;

    count = gr_split_str(lengths, starts, p->description, mp_charselect_list_width[resolution_index][2], 32, 0, -1);
    for (i = 0; i < count; i++) {
        strncpy(line, p->description + starts[i], lengths[i]);
        line[lengths[i]] = 0;
        strcat(mp_charselect_info_text, line);
        strcat(mp_charselect_info_text, "\n");
    }
    mp_charselect_row_counts[2] += count;
    mp_charselect_update_scrollbar(2);

    mp_charselect_name_focus = false;
    mp_charselect_desc_focus = false;
    mp_charselect_info_open = true;
}

// 0x407d30  Confidence: exact. A flag-4 entry (class check box) is its own action, otherwise the entry
// after it acts; on release unless the repeat timer is running (flag-2 arrows: 500 ms, then 100 ms).
void mp_charselect_update_buttons()
{
    int pressed = mp_charselect_pressed_button;
    int button = pressed;

    if (!(mp_charselect_button_defs[pressed].flags & 4))
        button = pressed + 1;

    if (pressed >= 0 && pressed < CHARSELECT_NUM_BUTTONS) {
        ui_button_state *b = &mp_charselect_buttons[button];
        if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
            b->flags |= 2;

        if (!mouse_is_down(0)) {
            if ((b->flags & 2) && !mp_charselect_repeat_timer.valid())
                mp_charselect_button_action(button);
        } else {
            if (mp_charselect_repeat_timer.valid() && mp_charselect_repeat_timer.elapsed() && (b->flags & 2)) {
                mp_charselect_button_action(button);
                mp_charselect_repeat_timer.SetTimeout(100);
            }
            return;
        }
    }
    mp_charselect_mouse_mode = 0;
}

// 0x407e10  Confidence: likely (three stores of the element, as journal_drag_scrollbar).
void mp_charselect_drag_scrollbar()
{
    int list = mp_charselect_drag_scrollbar_index;
    int count = mp_charselect_row_counts[list];

    mp_charselect_scroll_pos[list] = (int)((float)(game_mouse_y - mp_charselect_track_pos[resolution_index][list][1]
                                                   - mp_charselect_drag_start_y + mp_charselect_drag_start_thumb)
                                           / mp_charselect_track_h[resolution_index][list] * count + 0.5f);
    mp_charselect_scroll_pos[list] = mp_charselect_scroll_pos[list] > 0 ? mp_charselect_scroll_pos[list] : 0;
    count -= mp_charselect_list_page[resolution_index][list];
    mp_charselect_scroll_pos[list] = mp_charselect_scroll_pos[list] < count ? mp_charselect_scroll_pos[list] : count;
    mp_charselect_update_scrollbar(list);

    if (!mouse_is_down(0))
        mp_charselect_mouse_mode = 0;
}

// 0x407ed0  Confidence: exact. The info panel entries (flag 8) and the panel are drawn on top.
void mp_charselect_draw()
{
    int i;

    gr_set_color(255, 255, 255, 255);
    for (i = 0; i < CHARSELECT_NUM_BUTTONS; i++) {
        ui_button_state *b = &mp_charselect_buttons[i];
        if (!(mp_charselect_button_defs[i].flags & 8) && (b->flags & 3))
            ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
    }
    mp_charselect_draw_char_list();
    for (i = 0; i < 3; i++) {
        if (i != 2)
            mp_charselect_draw_scrollbar(i);
    }
    mp_charselect_draw_name_input();
    mp_charselect_draw_text_input();

    if (mp_charselect_info_open) {
        gr_set_color(255, 255, 255, 255);
        for (i = 0; i < CHARSELECT_NUM_BUTTONS; i++) {
            ui_button_state *b = &mp_charselect_buttons[i];
            if ((mp_charselect_button_defs[i].flags & 8) && (b->flags & 3))
                ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
        }
        mp_charselect_draw_char_info();
        mp_charselect_draw_scrollbar(2);
    }
}

// 0x407fd0  Confidence: exact (VC6 byte match) (declaration order of the locals: font_h 0x10, x 0xc, text 0x18). The row
// under the mouse (while list 0 is the mouse target) and the selected row use the highlight colour.
void mp_charselect_draw_char_list()
{
    int x;
    int font_h;
    char text[64];
    int rows, y, i;

    font_h = gr_get_font_height(-1);
    rows = mp_charselect_row_counts[0];
    if (rows >= mp_charselect_list_page[resolution_index][0])
        rows = mp_charselect_list_page[resolution_index][0];
    x = mp_charselect_list_pos[resolution_index][0][0];
    y = mp_charselect_list_pos[resolution_index][0][1];

    for (i = 0; i < rows; i++) {
        int index = mp_charselect_scroll_pos[0] + i;
        char *name = mp_char_profiles[index].name;

        if ((mp_charselect_drag_scrollbar_index == 0 && i == mp_charselect_hover_row)
            || index == mp_charselect_selected[0])
            gr_set_color_ptr(&ui_color_highlight);
        else
            gr_set_color_ptr(&ui_color_text);
        gr_string_truncate_ellipsis(text, name, mp_charselect_list_width[resolution_index][0], -1);
        gr_string(x, y, text, -1, gr_string_mode);
        y += font_h;
    }
}

// 0x4080c0  Confidence: exact (VC6 byte match) (thumb y - tile h + thumb h for the last tile, as journal_draw_list_scrollbar).
void mp_charselect_draw_scrollbar(int list)
{
    int tiles, i;

    if (list == -1 || !mp_charselect_scrollbar_visible[list])
        return;

    gr_set_color(255, 255, 255, 255);
    tiles = mp_charselect_scrollbar_size[list][1] / mp_charselect_scrollbar_bm_h;
    for (i = 0; i < tiles; i++)
        ui_bitmap_scaled(mp_charselect_scrollbar_bm, mp_charselect_scrollbar_pos[list][0],
                         mp_charselect_scrollbar_bm_h * i + mp_charselect_scrollbar_pos[list][1], gr_bitmap_wrap_mode);
    ui_bitmap_scaled(mp_charselect_scrollbar_bm, mp_charselect_scrollbar_pos[list][0],
                     mp_charselect_scrollbar_pos[list][1] - mp_charselect_scrollbar_bm_h
                     + mp_charselect_scrollbar_size[list][1],
                     gr_bitmap_wrap_mode);
}

// 0x408180  Confidence: exact (VC6 byte match) (as gamecreate_draw_text_field; the buffer is 36 bytes on the stack, 33 or
// more in the source). Right-aligned once wider than the box; "|" while the box has the focus, shown when
// the cursor blink is on.
void mp_charselect_draw_name_input()
{
    int w;
    int clip_h, clip_w, clip_y, clip_x;
    int h;
    char buf[36];
    int x;

    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    gr__set_clip(mp_charselect_name_box_pos[resolution_index][0], mp_charselect_name_box_pos[resolution_index][1],
                 mp_charselect_name_box_size[resolution_index][0], mp_charselect_name_box_size[resolution_index][1]);

    strcpy(buf, mp_charselect_name);
    if (mp_charselect_name_focus)
        strcat(buf, "|");
    gr_get_string_size(&w, &h, buf, -1, -1);

    x = mp_charselect_name_box_size[resolution_index][0] - w;
    x = x > 0 ? 0 : x;
    gr_set_color_ptr(&ui_color_text);
    if (mp_charselect_cursor_on)
        gr_string(x, 0, buf, -1, gr_string_mode);
    else
        gr_string(x, 0, mp_charselect_name, -1, gr_string_mode);

    gr__set_clip(clip_x, clip_y, clip_w, clip_h);
}

// 0x4082d0  Confidence: exact (VC6 byte match) (local order). No colour is set: the text keeps the colour of the previous
// draw call. While the cursor blink is off the "|" is cut from the end of the last line.
void mp_charselect_draw_text_input()
{
    int x;
    int count;
    int font_h;
    int lengths[32];
    int starts[32];
    char buf[260];
    char text[260];
    int rows, y, i;

    strcpy(buf, mp_charselect_text_input);
    if (mp_charselect_desc_focus)
        strcat(buf, "|");
    count = gr_split_str(lengths, starts, buf, mp_charselect_list_width[resolution_index][1], 32, 0, -1);
    rows = count;
    if (count >= mp_charselect_list_page[resolution_index][1])
        rows = mp_charselect_list_page[resolution_index][1];
    font_h = gr_get_font_height(-1);
    x = mp_charselect_list_pos[resolution_index][1][0];
    y = mp_charselect_list_pos[resolution_index][1][1];

    for (i = 0; i < rows; i++) {
        int line = mp_charselect_scroll_pos[1] + i;

        if (mp_charselect_desc_focus && !mp_charselect_cursor_on && line == count - 1) {
            strncpy(text, buf + starts[line], lengths[line] - 1);
            text[lengths[line] - 1] = 0;
        } else {
            strncpy(text, buf + starts[line], lengths[line]);
            text[lengths[line]] = 0;
        }
        gr_string(x, y, text, -1, gr_string_mode);
        y += font_h;
    }
}

// 0x408490  Confidence: exact. The whole text is drawn once, clipped to the panel and shifted up by the
// scroll position.
void mp_charselect_draw_char_info()
{
    int clip_h, clip_w, clip_y, clip_x;
    int font_h = gr_get_font_height(-1);
    int y = -(mp_charselect_scroll_pos[2] * font_h);
    int h = mp_charselect_list_page[resolution_index][2] * font_h;

    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    gr__set_clip(mp_charselect_list_pos[resolution_index][2][0], mp_charselect_list_pos[resolution_index][2][1],
                 mp_charselect_list_width[resolution_index][2], h);
    gr_set_color_ptr(&ui_color_text);
    gr_string(0, y, mp_charselect_info_text, -1, gr_string_mode);
    gr__set_clip(clip_x, clip_y, clip_w, clip_h);
}

// 0x408540  Confidence: exact (VC6 byte match) (the parameter is a bool, "mov al, [esp+8]": D6513). With validate, the .plr header must still be readable.
mp_char_profile *mp_charselect_get_selected_profile(bool validate)
{
    mp_char_record *rec;

    if (mp_charselect_selected[0] < 0)
        return 0;
    rec = &mp_char_profiles[mp_charselect_selected[0]];
    if (validate)
        return (mp_char_profile *)(mp_player_file_read_summary(rec->name, rec->filename) ? 0 : rec);
    return (mp_char_profile *)rec;
}
