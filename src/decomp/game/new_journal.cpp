// new_journal.cpp - Sum.exe v1.40, code 0x41a940-0x41be94 (screen 10, the quest journal)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .rdata 0x57ff24-0x57fffb (layout tables), .data 0x58d4b8-0x58d97f (button table, scrollbar names),
// .bss 0x5c1888-0x5c1acb. quest_table / quest_count (0x253ef40 / 0x25443a4) are in another unit's .bss
// (events_parse_table fills them), so they are extern here although sum.symbols tags them new_journal.cpp.
//
// Two lists (0 main quests, 1 side quests: quest_info.side_quest) and a description pane. Scrollbar index
// 0 / 1 is a list, 3 the description (journal_drag_scrollbar_index). Description line 0 is the quest name,
// line 1 is blank, lines 2.. are the gr_split_str lines of the current step's text.

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
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "summoner/interface/new_buysell.h"
#include "summoner/interface/new_journal.h"

#define JOURNAL_NUM_BUTTONS 19

// ---- .rdata --------------------------------------------------------------------------------
// Per resolution_index (640, 800, 1024).
struct journal_list_origins {
    int pos[2][2];                                            // x, y of the first row of list 0 and list 1
};
const journal_list_origins journal_list_pos[3] = {            // 0x57ff24
    { { { 27, 95 }, { 353, 95 } } },
    { { { 34, 117 }, { 441, 117 } } },
    { { { 43, 152 }, { 565, 152 } } },
};
const int journal_list_width[3]      = { 254, 318, 406 };     // 0x57ff54 row width (hit test, ellipsis)
const int journal_list_page[3]       = { 10, 10, 10 };        // 0x57ff60 visible rows per list
const int journal_desc_pos[3][2]     = { { 30, 308 }, { 38, 383 }, { 48, 493 } };   // 0x57ff6c
const int journal_desc_width[3]      = { 575, 719, 920 };     // 0x57ff84 wrap width
const int journal_desc_page[3]       = { 10, 10, 10 };        // 0x57ff90 visible description lines
const int journal_list_track_pos[3][2][2] = {                 // 0x57ff9c scrollbar track top, per list
    { { 293, 130 }, { 617, 130 } },
    { { 366, 163 }, { 771, 163 } },
    { { 469, 208 }, { 987, 208 } },
};
const int journal_desc_track_pos[3][2] = { { 617, 342 }, { 771, 428 }, { 987, 547 } };  // 0x57ffcc
const int journal_list_track_h[3]    = { 81, 101, 130 };      // 0x57ffe4
const int journal_desc_track_h[3]    = { 82, 103, 131 };      // 0x57fff0

// ---- .data ---------------------------------------------------------------------------------
// 0x58d4b8  19 x 0x40. flags: 1 HiLite hot spot acting on release (the Press entry after it acts),
// 2 HiLite hot spot acting on press with auto-repeat (the six scroll arrows: 5/7 list 0, 9/11 list 1,
// 13/15 description).
ui_button_def journal_button_defs[JOURNAL_NUM_BUTTONS] = {
    { "QstJrn-Bkgrnd",      { { 0, 0 },     { 0, 0 },     { 0, 0 } },     0 },
    { "Icon-ArrwLFHilite",  { { 21, 23 },   { 26, 29 },   { 33, 35 } },   1 },   // shop / status
    { "Icon-ArrwLFPress",   { { 21, 23 },   { 26, 29 },   { 33, 35 } },   0 },
    { "Icon-ArrwRTHilite",  { { 379, 23 },  { 474, 29 },  { 604, 35 } },  1 },   // inventory
    { "Icon-ArrwRTPress",   { { 379, 23 },  { 474, 29 },  { 604, 35 } },  0 },
    { "Icon-ArrwUPHilite",  { { 291, 98 },  { 364, 123 }, { 464, 154 } }, 2 },
    { "Icon-ArrwUPPress",   { { 291, 98 },  { 364, 123 }, { 464, 154 } }, 0 },
    { "Icon-ArrwDNHilite",  { { 291, 224 }, { 364, 280 }, { 464, 354 } }, 2 },
    { "Icon-ArrwDNPress",   { { 291, 224 }, { 364, 280 }, { 464, 354 } }, 0 },
    { "Icon-ArrwUPHilite",  { { 615, 98 },  { 769, 123 }, { 982, 154 } }, 2 },
    { "Icon-ArrwUPPress",   { { 615, 98 },  { 769, 123 }, { 982, 154 } }, 0 },
    { "Icon-ArrwDNHilite",  { { 615, 224 }, { 769, 280 }, { 982, 354 } }, 2 },
    { "Icon-ArrwDNPress",   { { 615, 224 }, { 769, 280 }, { 982, 354 } }, 0 },
    { "Icon-ArrwUPHilite",  { { 616, 310 }, { 770, 388 }, { 982, 493 } }, 2 },
    { "Icon-ArrwUPPress",   { { 616, 310 }, { 770, 388 }, { 982, 493 } }, 0 },
    { "Icon-ArrwDNHilite",  { { 616, 437 }, { 770, 546 }, { 982, 693 } }, 2 },
    { "Icon-ArrwDNPress",   { { 616, 437 }, { 770, 546 }, { 982, 693 } }, 0 },
    { "Icon-GrnClseHilite", { { 599, 18 },  { 749, 23 },  { 959, 31 } },  1 },   // close
    { "Icon-GrnClsePress",  { { 599, 18 },  { 749, 23 },  { 959, 31 } },  0 },
};
// 0x58d978. The strings are pooled copies at 0x588b8c / 0x588b74 (1024 first, as VC6 orders the literals
// of one initialiser), outside this unit's .data.
char *journal_scrollbar_names[2] = { "ScrollBarMid640.tga", "ScrollBarMid1024.tga" };

// ---- .bss ----------------------------------------------------------------------------------
int journal_desc_line_count;                                  // 0x5c1888
int journal_desc_scrollbar_x;                                 // 0x5c188c thumb x
int journal_desc_scrollbar_y;                                 // 0x5c1890 thumb y
quest_info *journal_selected_quest;                           // 0x5c1894
int journal_drag_start_y;                                     // 0x5c1898
int journal_mouse_mode;                                       // 0x5c189c 0 idle, 1 button held, 2 dragging
int journal_drag_start_thumb;                                 // 0x5c18a0
quest_info *journal_hover_quest;                              // 0x5c18a4
int journal_list_scrollbar_size[2][2];                        // 0x5c18a8 thumb w, h per list
int journal_pressed_button;                                   // 0x5c18b8
RealtimeTimer journal_repeat_timer;                           // 0x5c18bc
ui_button_state journal_buttons[JOURNAL_NUM_BUTTONS];         // 0x5c18c0
int journal_scrollbar_bm;                                     // 0x5c1a88
int journal_desc_scrollbar_w;                                 // 0x5c1a8c thumb w
int journal_desc_scrollbar_h;                                 // 0x5c1a90 thumb h
bool journal_list_scrollbar_visible[2];                       // 0x5c1a94
bool journal_desc_scrollbar_visible;                          // 0x5c1a96 (unnamed in sum.symbols; set, never read)
int journal_drag_scrollbar_index;                             // 0x5c1a98 0/1 list, 3 description, -1 none
int journal_leave_flags;                                      // 0x5c1a9c 1 unpause, 2 refresh statscreen, 4 clear shopkeeper (on leave)
int journal_list_scroll_pos[2];                               // 0x5c1aa0
int journal_scrollbar_bm_w;                                   // 0x5c1aa8
int journal_scrollbar_bm_h;                                   // 0x5c1aac
int journal_thumb_pos[2][2];                                  // 0x5c1ab0 thumb x, y per list
int journal_list_count[2];                                    // 0x5c1ac0
int journal_desc_first_line;                                  // 0x5c1ac8

// init_realtimetimer_5c18bc_thunk (0x41a940, "jmp 0x41a950") and init_realtimetimer_5c18bc (0x41a950,
// "mov ecx, 0x5c18bc / jmp RealtimeTimer__RealtimeTimer") are the compiler's dynamic initialiser pair for
// journal_repeat_timer (static_initializers entry 56). Confidence: exact (no source of their own).
// 0x41a940 init_realtimetimer_5c18bc_thunk (jmp, entry 56). Confidence: exact.
// 0x41a950 init_realtimetimer_5c18bc (journal_repeat_timer). Confidence: exact.

// 0x41a960  Confidence: exact. Like the play menu (progress_update per entry) but the position and size
// are only set for named entries and bm_get_dimensions has no handle test. The scrollbar tile is scaled
// without the + 0.5 rounding.
void journal_load_bitmaps()
{
    float scale = ui_get_scale();
    int i;

    for (i = 0; i < JOURNAL_NUM_BUTTONS; i++) {
        ui_button_state *b = &journal_buttons[i];
        const ui_button_def *d = &journal_button_defs[i];

        b->flags = 0;
        b->bitmap = -1;
        b->x = 0;
        b->y = 0;
        b->w = 0;
        b->h = 0;
        progress_update(0);

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

    journal_scrollbar_bm = bm_load(journal_scrollbar_names[ui_hires], path_interface, -1);
    bm_get_dimensions(journal_scrollbar_bm, &journal_scrollbar_bm_w, &journal_scrollbar_bm_h);
    journal_scrollbar_bm_w = (int)(journal_scrollbar_bm_w * scale);
    journal_scrollbar_bm_h = (int)(journal_scrollbar_bm_h * scale);
}

// 0x41ab10  Confidence: exact (VC6 byte match, match22; the two lists cleared in a loop; was likely) (the two pairs of zero stores use two registers, read as two chained
// assignments).
void journal_init()
{
    int i;

    journal_leave_flags = 6;
    if (!game_is_paused() && (game_flags & 1)) {
        journal_leave_flags |= 1;
        game_pause(0, 0);
    }

    journal_mouse_mode = 0;
    journal_pressed_button = -1;
    journal_drag_scrollbar_index = -1;
    journal_hover_quest = 0;
    journal_select_quest(0);

    for (i = 0; i < 2; i++) {
        journal_list_scroll_pos[i] = 0;
        journal_list_count[i] = 0;
    }

    for (i = 0; i < quest_count; i++) {
        if (savegame_state.get_quest_bit(i)) {
            if (quest_table[i].side_quest)
                journal_list_count[1]++;
            else
                journal_list_count[0]++;
        }
    }

    for (i = 0; i < 2; i++)
        journal_update_list_scrollbar(i);
}

// 0x41abf0  Confidence: exact (VC6 byte match, match22; count <= 0 branch first; was likely) (count and page are locals because fild needs them in memory: count goes
// to the dead argument slot; the count > 0 guard of the else branch can never fail here).
void journal_update_list_scrollbar(int list)
{
    if (list == -1)
        return;

    int count = journal_list_count[list];
    int page = journal_list_page[resolution_index];

    if (count <= page) {
        journal_list_scrollbar_visible[list] = false;
        journal_buttons[5 + list * 4].flags |= 4;
        journal_buttons[7 + list * 4].flags |= 4;
        return;
    }

    journal_list_scrollbar_visible[list] = true;
    journal_buttons[5 + list * 4].flags &= ~4;
    journal_buttons[7 + list * 4].flags &= ~4;

    if (count <= 0) {
        journal_thumb_pos[list][0] = journal_list_track_pos[resolution_index][list][0];
        journal_thumb_pos[list][1] = journal_list_track_pos[resolution_index][list][1];
        journal_list_scrollbar_size[list][0] = journal_scrollbar_bm_w;
        journal_list_scrollbar_size[list][1] = journal_list_track_h[resolution_index];
    } else {
        journal_thumb_pos[list][0] = journal_list_track_pos[resolution_index][list][0];
        journal_thumb_pos[list][1] = (int)((float)journal_list_scroll_pos[list] / count
                                           * journal_list_track_h[resolution_index] + 0.5f)
                                     + journal_list_track_pos[resolution_index][list][1];
        journal_list_scrollbar_size[list][0] = journal_scrollbar_bm_w;
        journal_list_scrollbar_size[list][1] = (int)((float)page / count * journal_list_track_h[resolution_index]
                                                     + 0.5f);
    }
}

// 0x41ad40  Confidence: exact. The line count is the split lines plus the name and the blank line.
void journal_select_quest(quest_info *quest)
{
    journal_selected_quest = quest;
    journal_desc_line_count = 0;
    journal_desc_first_line = 0;

    if (quest) {
        int step = quest_get_current_step(quest - quest_table);
        if (step >= 0) {
            int lengths[16];
            int starts[16];
            const char *text = journal_selected_quest->steps[step].text;
            journal_desc_line_count = gr_split_str(lengths, starts, text,
                                                   journal_desc_width[resolution_index], 16, 0, -1) + 2;
        }
    }
    journal_update_desc_scrollbar();
}

// 0x41ade0  Confidence: exact (VC6 byte match, match22; count <= 0 branch first; was likely) (same shape as journal_update_list_scrollbar).
void journal_update_desc_scrollbar()
{
    int count = journal_desc_line_count;
    int page = journal_desc_page[resolution_index];

    if (count <= page) {
        journal_buttons[13].flags |= 4;
        journal_buttons[15].flags |= 4;
        journal_desc_scrollbar_visible = false;
        return;
    }

    journal_buttons[13].flags &= ~4;
    journal_desc_scrollbar_visible = true;
    journal_buttons[15].flags &= ~4;

    if (count <= 0) {
        journal_desc_scrollbar_x = journal_desc_track_pos[resolution_index][0];
        journal_desc_scrollbar_y = journal_desc_track_pos[resolution_index][1];
        journal_desc_scrollbar_w = journal_scrollbar_bm_w;
        journal_desc_scrollbar_h = journal_desc_track_h[resolution_index];
    } else {
        journal_desc_scrollbar_x = journal_desc_track_pos[resolution_index][0];
        journal_desc_scrollbar_y = (int)((float)journal_desc_first_line / journal_desc_line_count
                                         * journal_desc_track_h[resolution_index] + 0.5f)
                                   + journal_desc_track_pos[resolution_index][1];
        journal_desc_scrollbar_w = journal_scrollbar_bm_w;
        journal_desc_scrollbar_h = (int)((float)page / journal_desc_line_count
                                         * journal_desc_track_h[resolution_index] + 0.5f);
    }
}

// 0x41aef0  Confidence: exact. Bits 2 and 4 are cleared by the screen-switch handlers so that the next
// screen keeps the status model and the shopkeeper.
void journal_leave()
{
    if (journal_leave_flags & 1)
        game_unpause();
    if (journal_leave_flags & 2)
        statscreen_refresh_anim(0);
    if (journal_leave_flags & 4)
        shop_set_shopkeeper_from_char(0);
}

// folded (0x4063a0, ui_return_true): the screen has no panel of its own, so its mouse_in_panel returns true;
// the linker folded the identical copies into one (identical-code folding). interface_mouse_over_panel
// calls each by name (D7903); name ours.
bool journal_mouse_in_panel(int x, int y)
{
    return true;
}

// 0x41af30  Confidence: exact.
int journal_frame(int key)
{
    int i;

    for (i = 0; i < JOURNAL_NUM_BUTTONS; i++)
        journal_buttons[i].flags &= ~2;

    switch (journal_mouse_mode) {
    case 0:
        return journal_handle_click(key);
    case 1:
        journal_update_buttons();
        return key;
    case 2:
        journal_drag_scrollbar();
        break;
    }
    return key;
}

// 0x41af80  Confidence: likely (the order of the key cases is read from the body layout: B and I jump
// into R's call, so R is written last; the 'I' case's zero byte lives in the dead key slot).
int journal_handle_click(int key)
{
    int i;
    int font_h;
    int dx, dy, dz;

    journal_pressed_button = -1;
    journal_drag_scrollbar_index = -1;
    journal_hover_quest = 0;

    for (i = 0; i < JOURNAL_NUM_BUTTONS; i++) {
        const ui_button_def *d = &journal_button_defs[i];
        ui_button_state *b = &journal_buttons[i];

        if (!(b->flags & 4) && (d->flags & 3) && point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h)) {
            journal_pressed_button = i;
            b->flags |= 2;
            if (d->flags & 1) {
                if (mouse_was_pressed(0)) {
                    journal_mouse_mode = 1;
                    journal_repeat_timer.invalidate();
                    return key;
                }
            } else if (d->flags & 2) {
                if (mouse_was_pressed(0)) {
                    journal_mouse_mode = 1;
                    journal_button_action(journal_pressed_button + 1);
                    journal_repeat_timer.SetTimeout(500);
                    return key;
                }
            }
            break;
        }
    }

    // scrollbar thumbs
    for (i = 0; i < 2; i++) {
        if (point_in_rect(game_mouse_x, game_mouse_y, journal_thumb_pos[i][0], journal_thumb_pos[i][1],
                          journal_list_scrollbar_size[i][0], journal_list_scrollbar_size[i][1])
            && mouse_was_pressed(0)) {
            journal_drag_scrollbar_index = i;
            journal_mouse_mode = 2;
            journal_drag_start_thumb = journal_thumb_pos[i][1];
            journal_drag_start_y = game_mouse_y;
            return key;
        }
    }
    if (point_in_rect(game_mouse_x, game_mouse_y, journal_desc_scrollbar_x, journal_desc_scrollbar_y,
                      journal_desc_scrollbar_w, journal_desc_scrollbar_h)
        && mouse_was_pressed(0)) {
        journal_mouse_mode = 2;
        journal_drag_start_thumb = journal_desc_scrollbar_y;
        journal_drag_scrollbar_index = 3;
        journal_drag_start_y = game_mouse_y;
        return key;
    }

    // quest rows; the pane under the mouse also becomes the mouse wheel target
    font_h = gr_get_font_height(-1);
    for (i = 0; i < 2; i++) {
        int rows = journal_list_count[i];
        if (rows >= journal_list_page[resolution_index])
            rows = journal_list_page[resolution_index];

        if (point_in_rect(game_mouse_x, game_mouse_y, journal_list_pos[resolution_index].pos[i][0],
                          journal_list_pos[resolution_index].pos[i][1], journal_list_width[resolution_index],
                          rows * font_h)) {
            int row;
            int y;

            journal_drag_scrollbar_index = i;
            for (row = 0, y = 0; row < rows; row++, y += font_h) {
                if (point_in_rect(game_mouse_x, game_mouse_y, journal_list_pos[resolution_index].pos[i][0],
                                  journal_list_pos[resolution_index].pos[i][1] + y,
                                  journal_list_width[resolution_index], font_h)) {
                    journal_hover_quest = journal_get_quest(i, journal_list_scroll_pos[i] + row);
                    if (mouse_was_pressed(0)) {
                        journal_select_quest(journal_hover_quest);
                        return key;
                    }
                    break;
                }
            }
        }
    }

    {
        int lines = journal_desc_line_count;
        if (lines >= journal_desc_page[resolution_index])
            lines = journal_desc_page[resolution_index];
        if (point_in_rect(game_mouse_x, game_mouse_y, journal_desc_pos[resolution_index][0],
                          journal_desc_pos[resolution_index][1], journal_desc_width[resolution_index],
                          lines * font_h))
            journal_drag_scrollbar_index = 3;
    }

    switch (key) {
    case KEY_ESC:
    case KEY_Q:
        interface_set_screen(0, 0);
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        return 0;
    case KEY_B:
        if (!ui_has_owner_character())
            break;
        journal_leave_flags &= ~6;
        interface_set_screen(7, 0);
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;
    case KEY_I: {
        char page = 0;
        journal_leave_flags &= ~6;
        interface_set_screen(1, &page);
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;
    }
    case KEY_R:
        journal_leave_flags &= ~6;
        interface_set_screen(2, 0);
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;
    }

    {
        mouse_get_delta(&dx, &dy, &dz);
        if (dz > 0) {
            if (journal_drag_scrollbar_index == 3)
                journal_desc_page_up();
            else
                journal_list_page_up(journal_drag_scrollbar_index);
        } else if (dz < 0) {
            if (journal_drag_scrollbar_index == 3)
                journal_desc_page_down();
            else
                journal_list_page_down(journal_drag_scrollbar_index);
        }
    }
    return key;
}

// 0x41b470  Confidence: exact (VC6 byte match) (the side test (list == 1) is hoisted out of the loop by the compiler).
// The n-th quest of a list whose quest bit is set.
quest_info *journal_get_quest(int list, int n)
{
    int i;

    bool side = list == 1;

    for (i = 0; i < quest_count; i++) {
        quest_info *q = &quest_table[i];

        if (q->side_quest == side && savegame_state.get_quest_bit(i)) {
            if (n == 0)
                return q;
            n--;
        }
    }
    return 0;
}

// 0x41b4d0  Confidence: exact (VC6 byte match) (two stores of the element: the subtraction, then the clamp).
void journal_list_page_up(int list)
{
    if (list == -1)
        return;
    if (journal_list_scroll_pos[list] > 0) {
        journal_list_scroll_pos[list] -= journal_list_page[resolution_index];
        journal_list_scroll_pos[list] = journal_list_scroll_pos[list] > 0 ? journal_list_scroll_pos[list] : 0;
        journal_update_list_scrollbar(list);
    }
}

// 0x41b520  Confidence: exact (VC6 byte match) (as journal_list_page_up).
void journal_list_page_down(int list)
{
    if (list == -1)
        return;
    int last = journal_list_count[list] - journal_list_page[resolution_index];
    if (journal_list_scroll_pos[list] < last) {
        journal_list_scroll_pos[list] += journal_list_page[resolution_index];
        journal_list_scroll_pos[list] = journal_list_scroll_pos[list] < last ? journal_list_scroll_pos[list] : last;
        journal_update_list_scrollbar(list);
    }
}

// 0x41b570  Confidence: exact (single store; tail call; the clamp at 0 in D10600's form, D11700).
void journal_desc_page_up()
{
    if (journal_desc_first_line > 0) {
        journal_desc_first_line -= journal_desc_page[resolution_index];
        journal_desc_first_line &= (journal_desc_first_line <= 0) - 1;
        journal_update_desc_scrollbar();
    }
}

// 0x41b5a0  Confidence: exact.
void journal_desc_page_down()
{
    int first = journal_desc_first_line;
    int page = journal_desc_page[resolution_index];
    int last = journal_desc_line_count - page;
    if (first < last) {
        int line = first + page;
        if (line < last)
            last = line;
        journal_desc_first_line = last;
        journal_update_desc_scrollbar();
    }
}

// 0x41b5d0  Confidence: exact (byte table 0x41b6d4 over button-2, 0..16, into the jump table 0x41b6ac;
// the four list cases share the sound call).
void journal_button_action(int button)
{
    switch (button) {
    case 2:
        journal_goto_shop_or_status();
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        break;
    case 4:
        journal_goto_inventory();
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        break;
    case 6:
        journal_list_scroll_up(0);
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 8:
        journal_list_scroll_down(0);
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 10:
        journal_list_scroll_up(1);
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 12:
        journal_list_scroll_down(1);
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 14:
        journal_desc_scroll_up();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 16:
        journal_desc_scroll_down();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 18:
        interface_close_screen();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    }
}

// 0x41b6f0  Confidence: exact. Buy/sell (7) while a shopkeeper is set, else status (2).
void journal_goto_shop_or_status()
{
    journal_leave_flags &= ~6;
    if (ui_has_owner_character())
        interface_set_screen(7, 0);
    else
        interface_set_screen(2, 0);
}

// 0x41b720  Confidence: exact. Inventory (1) with a zero byte argument (page 0).
void journal_goto_inventory()
{
    char page[4];

    journal_leave_flags &= ~6;
    page[0] = 0;
    interface_set_screen(1, page);
}

// 0x41b750  Confidence: exact.
void journal_list_scroll_up(int list)
{
    if (list == -1)
        return;
    if (journal_list_scroll_pos[list] > 0) {
        journal_list_scroll_pos[list]--;
        journal_update_list_scrollbar(list);
    }
}

// 0x41b780  Confidence: exact.
void journal_list_scroll_down(int list)
{
    if (list == -1)
        return;
    if (journal_list_scroll_pos[list] < journal_list_count[list] - journal_list_page[resolution_index]) {
        journal_list_scroll_pos[list]++;
        journal_update_list_scrollbar(list);
    }
}

// 0x41b7c0  Confidence: exact.
void journal_desc_scroll_up()
{
    if (journal_desc_first_line > 0) {
        journal_desc_first_line--;
        journal_update_desc_scrollbar();
    }
}

// 0x41b7e0  Confidence: exact.
void journal_desc_scroll_down()
{
    if (journal_desc_first_line < journal_desc_line_count - journal_desc_page[resolution_index]) {
        journal_desc_first_line++;
        journal_update_desc_scrollbar();
    }
}

// 0x41b810  Confidence: exact. The Press entry after the pressed HiLite acts: on release when the repeat
// timer is not running (flag-1 buttons), every 100 ms while held once the 500 ms delay set by
// journal_handle_click has passed (flag-2 buttons).
void journal_update_buttons()
{
    int button = journal_pressed_button + 1;

    if (journal_pressed_button >= 0 && journal_pressed_button < JOURNAL_NUM_BUTTONS) {
        ui_button_state *b = &journal_buttons[button];

        if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
            b->flags |= 2;

        if (!mouse_is_down(0)) {
            if ((b->flags & 2) && !journal_repeat_timer.valid())
                journal_button_action(button);
        } else {
            if (journal_repeat_timer.valid() && journal_repeat_timer.elapsed() && (b->flags & 2)) {
                journal_button_action(button);
                journal_repeat_timer.SetTimeout(100);
            }
            return;
        }
    }
    journal_mouse_mode = 0;
}

// 0x41b8e0  Confidence: exact (VC6 byte match, match22; the max before the last-line difference; was likely) (the list branch stores the element three times, the description branch
// once: written as three assignments and one nested clamp).
void journal_drag_scrollbar()
{
    int y = journal_drag_start_thumb + (game_mouse_y - journal_drag_start_y);

    if (journal_drag_scrollbar_index == 3) {
        int line = (int)((float)(y - journal_desc_track_pos[resolution_index][1])
                         / journal_desc_track_h[resolution_index] * journal_desc_line_count + 0.5f);
        line = line > 0 ? line : 0;
        int last = journal_desc_line_count - journal_desc_page[resolution_index];
        if (line < last)
            last = line;
        journal_desc_first_line = last;
        journal_update_desc_scrollbar();
    } else {
        int list = journal_drag_scrollbar_index;
        int count = journal_list_count[list];
        journal_list_scroll_pos[list] = (int)((float)(y - journal_list_track_pos[resolution_index][list][1])
                                              / journal_list_track_h[resolution_index] * count + 0.5f);
        journal_list_scroll_pos[list] = journal_list_scroll_pos[list] > 0 ? journal_list_scroll_pos[list] : 0;
        count -= journal_list_page[resolution_index];
        journal_list_scroll_pos[list] = journal_list_scroll_pos[list] < count ? journal_list_scroll_pos[list] : count;
        journal_update_list_scrollbar(list);
    }

    if (!mouse_is_down(0))
        journal_mouse_mode = 0;
}

// 0x41b9f0  Confidence: exact.
void journal_draw()
{
    int i;

    gr_set_color(255, 255, 255, 255);
    for (i = 0; i < JOURNAL_NUM_BUTTONS; i++) {
        ui_button_state *b = &journal_buttons[i];

        if (b->flags & 3)
            ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
    }
    journal_draw_quest_list();
    journal_draw_description();
    for (i = 0; i < 2; i++)
        journal_draw_list_scrollbar(i);
    journal_draw_desc_scrollbar();
}

// 0x41ba60  Confidence: exact (VC6 byte match, match22; the origins copied element by element; was likely) (the struct copy of the two row origins; the early exit once both lists
// are past their page). Rows before the scroll position are counted but not drawn.
void journal_draw_quest_list()
{
    char buf[128];
    int font_h = gr_get_font_height(-1);
    journal_list_origins pos;
    const journal_list_origins *src = &journal_list_pos[resolution_index];
    pos.pos[0][0] = src->pos[0][0];
    pos.pos[0][1] = src->pos[0][1];
    pos.pos[1][0] = src->pos[1][0];
    pos.pos[1][1] = src->pos[1][1];
    int row[2];
    int i, k;

    for (k = 0; k < 2; k++)
        row[k] = -journal_list_scroll_pos[k];

    for (i = 0; i < quest_count; i++) {
        quest_info *quest = &quest_table[i];

        if (!savegame_state.get_quest_bit(i))
            continue;

        int list = quest->side_quest != 0;
        if (row[list] >= 0 && row[list] < journal_list_page[resolution_index]) {
            if (quest == journal_hover_quest || quest == journal_selected_quest)
                gr_set_color_ptr(&ui_color_highlight);
            else
                gr_set_color_ptr(&ui_color_text);
            gr_string_truncate_ellipsis(buf, localize_quest_name(quest->name),
                                        journal_list_width[resolution_index], -1);
            gr_string(pos.pos[list][0], pos.pos[list][1], buf, -1, gr_string_mode);
            pos.pos[list][1] += font_h;
        }
        row[list]++;

        for (k = 0; k < 2; k++) {
            if (row[k] < journal_list_page[resolution_index])
                break;
        }
        if (k >= 2)
            break;
    }
}

// 0x41bbf0  Confidence: exact (VC6 byte match) (the min() of the visible line count; lines are copied with strncpy and
// terminated by hand).
void journal_draw_description()
{
    int font_h;
    int end;
    int lengths[16];
    int starts[16];
    char line[256];

    if (!journal_selected_quest)
        return;

    int x = journal_desc_pos[resolution_index][0];
    int y = journal_desc_pos[resolution_index][1];
    font_h = gr_get_font_height(-1);

    int step = quest_get_current_step(journal_selected_quest - quest_table);
    if (step < 0)
        return;

    gr_set_color_ptr(&ui_color_text);
    const char *text = journal_selected_quest->steps[step].text;
    int count = gr_split_str(lengths, starts, text, journal_desc_width[resolution_index], 16, 0, -1) + 2;
    if (count >= journal_desc_page[resolution_index])
        count = journal_desc_page[resolution_index];
    end = count + journal_desc_first_line;

    for (int i = journal_desc_first_line; i < end; i++) {
        if (i == 0) {
            gr_string(x, y, localize_quest_name(journal_selected_quest->name), -1, gr_string_mode);
        } else if (i >= 2) {
            strncpy(line, text + starts[i - 2], lengths[i - 2]);
            line[lengths[i - 2]] = 0;
            gr_string(x, y, line, -1, gr_string_mode);
        }
        y += font_h;
    }
}

// 0x41bd40  Confidence: exact (VC6 byte match) (thumb y - tile h + thumb h for the last tile, in the binary's order).
// The thumb is drawn as whole tiles plus one tile aligned to its bottom edge.
void journal_draw_list_scrollbar(int list)
{
    int tiles, i;

    if (list == -1 || !journal_list_scrollbar_visible[list])
        return;

    gr_set_color(255, 255, 255, 255);
    tiles = journal_list_scrollbar_size[list][1] / journal_scrollbar_bm_h;
    for (i = 0; i < tiles; i++)
        ui_bitmap_scaled(journal_scrollbar_bm, journal_thumb_pos[list][0],
                         journal_scrollbar_bm_h * i + journal_thumb_pos[list][1], gr_bitmap_wrap_mode);
    ui_bitmap_scaled(journal_scrollbar_bm, journal_thumb_pos[list][0],
                     journal_thumb_pos[list][1] - journal_scrollbar_bm_h + journal_list_scrollbar_size[list][1],
                     gr_bitmap_wrap_mode);
}

// 0x41be00  Confidence: exact (VC6 byte match) (as journal_draw_list_scrollbar). No visibility test: the description
// thumb is always drawn, with the values of the last scrolling quest (journal_update_desc_scrollbar
// leaves them alone when the text fits; all 0 before the first one).
void journal_draw_desc_scrollbar()
{
    int tiles, i;

    gr_set_color(255, 255, 255, 255);
    tiles = journal_desc_scrollbar_h / journal_scrollbar_bm_h;
    for (i = 0; i < tiles; i++)
        ui_bitmap_scaled(journal_scrollbar_bm, journal_desc_scrollbar_x,
                         journal_scrollbar_bm_h * i + journal_desc_scrollbar_y, gr_bitmap_wrap_mode);
    ui_bitmap_scaled(journal_scrollbar_bm, journal_desc_scrollbar_x,
                     journal_desc_scrollbar_y - journal_scrollbar_bm_h + journal_desc_scrollbar_h,
                     gr_bitmap_wrap_mode);
}
