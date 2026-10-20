// new_itemtrade.cpp - Sum.exe v1.40, code 0x419170-0x41a932 (screen 19, the multiplayer item trade)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .rdata 0x57fe40-0x57ff23 (layout tables; 0.5f at 0x57e6b0 is new_assess.cpp's pooled __real),
// .data 0x58cda0-0x58d4b3 (button table, scrollbar names, "Gold", "%s|"), .bss 0x5c1570-0x5c1881.
// The trade state and both offer lists belong to multitrade.cpp (mp_trade_*, extern here).
//
// Three lists: list 1 = what we offer (mp_trade_local_items, click a row to take it back), list 2 = what
// the partner offers (mp_trade_remote_items, display only), and the party inventory (click a row to offer
// one more unit; row 0 is a gold pseudo entry while party_gold > 0, which opens a numeric input box,
// "TradeGold-Bkgrnd", whose buttons carry def flag 8). Packets: IT_ADDI / IT_RMVI / IT_OKAY / IT_CNCL
// (docs/sum-exe/multiplayer.md).

#include <stdio.h>
#include <string.h>
#include <ctype.h>
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
#include "engine/objects/living_entity.h"
#include "summoner/multi/multi.h"
#include "summoner/interface/new_itemtrade.h"

#define ITEMTRADE_NUM_BUTTONS 28
#define ITEMTRADE_MAX_GOLD    9999999

// ---- .rdata --------------------------------------------------------------------------------
// Per resolution_index (640, 800, 1024). All names here are *new* (sum.symbols has none).
const int itemtrade_list1_pos[3][2]       = { { 21, 70 }, { 26, 88 }, { 34, 112 } };     // 0x57fe40 our offer
const int itemtrade_list1_width[3]        = { 265, 331, 424 };                           // 0x57fe58
const int itemtrade_list1_page[3]         = { 3, 3, 3 };                                 // 0x57fe64
const int itemtrade_list2_pos[3][2]       = { { 21, 151 }, { 26, 189 }, { 34, 242 } };   // 0x57fe70 partner's offer
const int itemtrade_list2_width[3]        = { 265, 331, 424 };                           // 0x57fe88
const int itemtrade_list2_page[3]         = { 3, 3, 3 };                                 // 0x57fe94
const int itemtrade_party_pos[3][2]       = { { 21, 248 }, { 26, 310 }, { 34, 397 } };   // 0x57fea0 party items
const int itemtrade_party_width[3]        = { 265, 331, 424 };                           // 0x57feb8
const int itemtrade_party_page[3]         = { 7, 7, 7 };                                 // 0x57fec4
const int itemtrade_scrollbar_track[3][2] = { { 298, 285 }, { 373, 356 }, { 477, 456 } };// 0x57fed0 party list bar
const int itemtrade_scrollbar_track_h[3]  = { 32, 40, 51 };                              // 0x57fee8
const int itemtrade_gold_input_pos[3][2]  = { { 242, 252 }, { 309, 316 }, { 388, 400 } };// 0x57fef4 clip origin
const int itemtrade_gold_input_size[3][2] = { { 154, 20 }, { 303, 25 }, { 246, 32 } };   // 0x57ff0c clip w, h
                                                                     // (303 at 800 is what the binary holds)

// ---- .data ---------------------------------------------------------------------------------
// 0x58cda0  28 x 0x40 (sum.symbols puts itemtrade_button_defs at 0x58cddc, the flags of entry 0).
// flags: 1 HiLite acting on release through the Press entry after it, 4 acting on press with auto-repeat
// (scroll arrows), 8 part of the gold input box (drawn and clickable only while it is open).
// Action id = HiLite index + 1 (itemtrade_button_action).
ui_button_def itemtrade_button_defs[ITEMTRADE_NUM_BUTTONS] = {
    { "MultiTrade-Bkgrnd",        { { 0, 0 },     { 0, 0 },     { 0, 0 } },     0 },
    { "Icon-ArrwUPHilite",        { { 295, 69 },  { 369, 86 },  { 472, 110 } }, 4 },   // 2 list 1 up
    { "Icon-ArrwUPPress",         { { 295, 69 },  { 369, 86 },  { 472, 110 } }, 0 },
    { "Icon-ArrwDNHilite",        { { 295, 103 }, { 369, 129 }, { 472, 165 } }, 4 },   // 4 list 1 down
    { "Icon-ArrwDNPress",         { { 295, 103 }, { 369, 129 }, { 472, 165 } }, 0 },
    { "Icon-ArrwUPHilite",        { { 295, 152 }, { 369, 190 }, { 472, 243 } }, 4 },   // 6 list 2 up
    { "Icon-ArrwUPPress",         { { 295, 152 }, { 369, 190 }, { 472, 243 } }, 0 },
    { "Icon-ArrwDNHilite",        { { 295, 186 }, { 369, 233 }, { 472, 298 } }, 4 },   // 8 list 2 down
    { "Icon-ArrwDNPress",         { { 295, 186 }, { 369, 233 }, { 472, 298 } }, 0 },
    { "Icon-ArrwUPHilite",        { { 295, 251 }, { 369, 314 }, { 472, 402 } }, 4 },   // 10 party up
    { "Icon-ArrwUPPress",         { { 295, 251 }, { 369, 314 }, { 472, 402 } }, 0 },
    { "Icon-ArrwDNHilite",        { { 295, 329 }, { 369, 411 }, { 472, 526 } }, 4 },   // 12 party down
    { "Icon-ArrwDNPress",         { { 295, 329 }, { 369, 411 }, { 472, 526 } }, 0 },
    { "MultiTrade-Ready",         { { 15, 69 },   { 19, 86 },   { 24, 110 } },  0 },   // we accepted (state 3)
    { "MultiTrade-Ready",         { { 15, 151 },  { 19, 189 },  { 24, 242 } },  0 },   // partner accepted (state 4)
    { "LevelSelectProfileHiLite", { { 46, 373 },  { 58, 466 },  { 74, 597 } },  1 },   // 16 accept
    { "LevelSelectProfilePress",  { { 46, 373 },  { 58, 466 },  { 74, 597 } },  0 },
    { "LevelSelectProfileHiLite", { { 179, 373 }, { 224, 466 }, { 286, 597 } }, 1 },   // 18 cancel trade
    { "LevelSelectProfilePress",  { { 179, 373 }, { 224, 466 }, { 286, 597 } }, 0 },
    { "Icon-GrnClseHilite",       { { 277, 21 },  { 346, 26 },  { 443, 34 } },  1 },   // 20 close (cancel trade)
    { "Icon-GrnClsePress",        { { 277, 21 },  { 346, 26 },  { 443, 34 } },  0 },
    { "TradeGold-Bkgrnd",         { { 170, 122 }, { 213, 153 }, { 272, 195 } }, 8 },
    { "LevelSelectProfileHiLite", { { 204, 289 }, { 255, 361 }, { 326, 462 } }, 9 },   // 23 gold: OK
    { "LevelSelectProfilePress",  { { 204, 289 }, { 255, 361 }, { 326, 462 } }, 8 },
    { "LevelSelectProfileHiLite", { { 337, 289 }, { 421, 361 }, { 539, 462 } }, 9 },   // 25 gold: cancel
    { "LevelSelectProfilePress",  { { 337, 289 }, { 421, 361 }, { 539, 462 } }, 8 },
    { "Icon-GrnClseHilite",       { { 427, 143 }, { 534, 179 }, { 683, 286 } }, 9 },   // 27 gold: close
    { "Icon-GrnClsePress",        { { 427, 143 }, { 534, 179 }, { 683, 286 } }, 8 },
};
// 0x58d4a0 (*new*). The strings are the pooled copies at 0x588b8c / 0x588b74, outside this unit.
char *itemtrade_scrollbar_names[2] = { "ScrollBarMid640.tga", "ScrollBarMid1024.tga" };
// "Gold" (0x58d4a8) and "%s|" (0x58d4b0) are this unit's literals; "%d" is new_buysell.cpp's (0x589820).

// ---- .bss ----------------------------------------------------------------------------------
int itemtrade_party_hover_row;                                // 0x5c1570 visible party row under the mouse, -1
int itemtrade_list1_hover_row;                                // 0x5c1574 visible list 1 row under the mouse, -1
party_inventory_entry itemtrade_gold_entry;                   // 0x5c1578 {party_gold at init, &itemtrade_gold_item}
int itemtrade_party_scroll_pos;                               // 0x5c1580
int itemtrade_scrollbar_w;                                    // 0x5c1584 thumb w
int itemtrade_scrollbar_h;                                    // 0x5c1588 thumb h
int itemtrade_drag_start_y;                                   // 0x5c158c
bool itemtrade_gold_input_active;                             // 0x5c1590
int itemtrade_party_row_count;                                // 0x5c1594
ui_button_state itemtrade_buttons[ITEMTRADE_NUM_BUTTONS];     // 0x5c1598
inv_item itemtrade_gold_item;                                 // 0x5c1838 pseudo item {?, Gold type, charges}
int itemtrade_unused_5c1844;                                  // 0x5c1844 never referenced (*new*, see NOTES)
int itemtrade_scrollbar_bm;                                   // 0x5c1848
int itemtrade_scrollbar_bm_w;                                 // 0x5c184c
int itemtrade_scrollbar_bm_h;                                 // 0x5c1850
int itemtrade_mouse_mode;                                     // 0x5c1854 0 idle, 1 button held, 2 dragging,
                                                              //          3 list 1 row held, 4 party row held
RealtimeTimer itemtrade_repeat_timer;                         // 0x5c1858
int itemtrade_drag_start_thumb;                               // 0x5c185c
RealtimeTimer itemtrade_cursor_timer;                         // 0x5c1860
bool itemtrade_scrollbar_visible;                             // 0x5c1864
int itemtrade_list2_scroll_pos;                               // 0x5c1868
int itemtrade_pressed_button;                                 // 0x5c186c
int itemtrade_scrollbar_x;                                    // 0x5c1870 thumb x
int itemtrade_scrollbar_y;                                    // 0x5c1874 thumb y
int itemtrade_gold_amount;                                    // 0x5c1878 typed gold amount
int itemtrade_list1_scroll_pos;                               // 0x5c187c
bool itemtrade_mouse_in_list;                                 // 0x5c1880
bool itemtrade_cursor_on;                                     // 0x5c1881

// init_realtimetimer_5c1858_thunk (0x419170) / init_realtimetimer_5c1858 (0x419180) and
// init_realtimetimer_5c1860_thunk (0x419190) / init_realtimetimer_5c1860 (0x4191a0) are the compiler's
// dynamic initialiser pairs for itemtrade_repeat_timer and itemtrade_cursor_timer (static_initializers
// entries 54 and 55; "jmp" thunk, then "mov ecx, obj / jmp RealtimeTimer__RealtimeTimer").
// 0x419170 init_realtimetimer_5c1858_thunk (jmp, entry 54). Confidence: exact.
// 0x419180 init_realtimetimer_5c1858 (itemtrade_repeat_timer). Confidence: exact.
// 0x419190 init_realtimetimer_5c1860_thunk (jmp, entry 55). Confidence: exact.
// 0x4191a0 init_realtimetimer_5c1860 (itemtrade_cursor_timer). Confidence: exact.
// Confidence: exact (no source of their own).

// 0x4191b0  Confidence: exact (VC6 byte match) (VC6 walks the records with pointers and tests i == 0 / 21 as pointer
// compares). Unlike the journal / buysell loaders the position is set for every record, a nameless one
// gets bitmap -1 explicitly, and the size is only taken when the bitmap loaded. Records 0 and 21 (the two
// backgrounds) are always drawn.
void itemtrade_load_bitmaps()
{
    float scale = ui_get_scale();
    int i;

    for (i = 0; i < ITEMTRADE_NUM_BUTTONS; i++) {
        ui_button_state *b = &itemtrade_buttons[i];
        const ui_button_def *d = &itemtrade_button_defs[i];

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
        } else {
            b->bitmap = -1;
        }
        b->x = d->pos[resolution_index][0];
        b->y = d->pos[resolution_index][1];
        if (b->bitmap >= 0) {
            bm_get_dimensions(b->bitmap, &b->w, &b->h);
            b->w = (int)(b->w * scale + 0.5f);
            b->h = (int)(b->h * scale + 0.5f);
        } else {
            b->w = 0;
            b->h = 0;
        }
        if (i == 0 || i == 21)
            b->flags |= 1;
    }

    progress_update(0);
    itemtrade_scrollbar_bm = bm_load(itemtrade_scrollbar_names[ui_hires], path_interface, -1);
    bm_get_dimensions(itemtrade_scrollbar_bm, &itemtrade_scrollbar_bm_w, &itemtrade_scrollbar_bm_h);
    itemtrade_scrollbar_bm_w = (int)(itemtrade_scrollbar_bm_w * scale);
    itemtrade_scrollbar_bm_h = (int)(itemtrade_scrollbar_bm_h * scale);
}

// 0x419380  Confidence: exact (VC6 byte match) (store order around the item_type_find call is the scheduler's).
// "Trading with %s" names the partner's player.
void itemtrade_init()
{
    char msg[64];

    itemtrade_list1_scroll_pos = 0;
    itemtrade_list2_scroll_pos = 0;
    itemtrade_party_scroll_pos = 0;
    itemtrade_count_party_rows();
    itemtrade_update_scrollbar();
    itemtrade_gold_input_active = 0;
    itemtrade_gold_amount = 0;
    itemtrade_mouse_mode = 0;
    itemtrade_pressed_button = -1;
    itemtrade_gold_item.type = &item_types[item_type_find("Gold")];
    itemtrade_gold_entry.count = party_gold;
    itemtrade_gold_entry.item = &itemtrade_gold_item;
    if (mp_trade_partner) {
        sprintf(msg, localize_strings[820], MP_PLAYER_NAME(mp_trade_partner->player));   // "Trading with %s"
        hud_show_message(msg);
    }
}

// 0x419420  Confidence: exact (VC6 byte match) (same template as buysell_update_scrollbar, including the row count test).
// The party list bar; records 9 / 11 are its arrows.
void itemtrade_update_scrollbar()
{
    int page = itemtrade_party_page[resolution_index];

    if (itemtrade_party_row_count <= page) {
        itemtrade_scrollbar_visible = 0;
        itemtrade_buttons[9].flags |= 4;
        itemtrade_buttons[11].flags |= 4;
        return;
    }

    itemtrade_buttons[9].flags &= ~4;
    itemtrade_scrollbar_visible = 1;
    itemtrade_buttons[11].flags &= ~4;
    if (itemtrade_party_row_count <= 0) {
        itemtrade_scrollbar_x = itemtrade_scrollbar_track[resolution_index][0];
        itemtrade_scrollbar_y = itemtrade_scrollbar_track[resolution_index][1];
        itemtrade_scrollbar_w = itemtrade_scrollbar_bm_w;
        itemtrade_scrollbar_h = itemtrade_scrollbar_track_h[resolution_index];
    } else {
        itemtrade_scrollbar_x = itemtrade_scrollbar_track[resolution_index][0];
        itemtrade_scrollbar_y = (int)((float)itemtrade_party_scroll_pos / (float)itemtrade_party_row_count
                                      * (float)itemtrade_scrollbar_track_h[resolution_index] + 0.5f)
                              + itemtrade_scrollbar_track[resolution_index][1];
        itemtrade_scrollbar_w = itemtrade_scrollbar_bm_w;
        itemtrade_scrollbar_h = (int)((float)page / (float)itemtrade_party_row_count
                                      * (float)itemtrade_scrollbar_track_h[resolution_index] + 0.5f);
    }
}

// 0x419530  Confidence: likely (marked exact earlier; differs under cl 12.00.8966, match31). 
void itemtrade_count_party_rows()
{
    itemtrade_party_row_count = party_inventory_count;
    if (party_gold > 0)
        itemtrade_party_row_count++;
}

// 0x419550  Confidence: exact.
void itemtrade_leave()
{
    if (mp_trade_state)
        mp_trade_cancel();
}

// 0x419560  Confidence: exact (jump table 0x4195f8, 5 cases). Blinks the gold input cursor every 500 ms.
int itemtrade_frame(int key)
{
    for (int i = 0; i < ITEMTRADE_NUM_BUTTONS; i++)
        itemtrade_buttons[i].flags &= ~2;

    itemtrade_update_trade_buttons();
    itemtrade_update_scroll_buttons();
    if (itemtrade_cursor_timer.elapsed()) {
        itemtrade_cursor_timer.SetTimeout(500);
        itemtrade_cursor_on = !itemtrade_cursor_on;
    }

    switch (itemtrade_mouse_mode) {
    case 0:
        key = itemtrade_handle_click(key);
        break;
    case 1:
        itemtrade_update_buttons();
        break;
    case 2:
        itemtrade_drag_scrollbar();
        break;
    case 3:
        itemtrade_list1_handle_mouse();
        break;
    case 4:
        itemtrade_party_handle_mouse();
        break;
    }
    return key;
}

// 0x419610  Confidence: exact (VC6 byte match) (the case 4 body is laid out first). State 3: we accepted (tick 13 shown,
// accept hot spot 15 disabled); state 4: the partner accepted (tick 14 shown).
void itemtrade_update_trade_buttons()
{
    itemtrade_buttons[13].flags &= ~2;
    itemtrade_buttons[14].flags &= ~2;
    itemtrade_buttons[15].flags &= ~4;
    switch (mp_trade_state) {
    case 3:
        itemtrade_buttons[13].flags |= 2;
        itemtrade_buttons[15].flags |= 4;
        break;
    case 4:
        itemtrade_buttons[14].flags |= 2;
        break;
    }
}

// 0x419670  Confidence: exact. The arrows of the two offer lists (records 1 / 3 and 5 / 7) are disabled
// while the list is shorter than its page.
void itemtrade_update_scroll_buttons()
{
    if (mp_trade_local_count < itemtrade_list1_page[resolution_index]) {
        itemtrade_buttons[1].flags |= 4;
        itemtrade_buttons[3].flags |= 4;
    } else {
        itemtrade_buttons[1].flags &= ~4;
        itemtrade_buttons[3].flags &= ~4;
    }
    if (mp_trade_remote_count < itemtrade_list2_page[resolution_index]) {
        itemtrade_buttons[5].flags |= 4;
        itemtrade_buttons[7].flags |= 4;
    } else {
        itemtrade_buttons[5].flags &= ~4;
        itemtrade_buttons[7].flags &= ~4;
    }
}

// 0x419710  Confidence: likely (the digit and Backspace paths share the cursor restart tail; the
// flag-8 test compares bytes, "shr 3 / and 1 / cmp cl, al"). While the gold box is open only its buttons
// react and the keys edit the amount (digits up to min(party_gold, 9999999), Backspace, Enter submits,
// Esc closes); otherwise list 1 / party rows, Esc cancels the trade, the wheel pages the party list.
int itemtrade_handle_click(int key)
{
    int i;

    itemtrade_pressed_button = -1;
    itemtrade_list1_hover_row = -1;
    itemtrade_party_hover_row = -1;
    itemtrade_buttons[0].flags |= 2;

    for (i = 0; i < ITEMTRADE_NUM_BUTTONS; i++) {
        if (!(itemtrade_buttons[i].flags & 4) && (itemtrade_button_defs[i].flags & 5)
            && itemtrade_gold_input_active == (bool)((itemtrade_button_defs[i].flags >> 3) & 1)
            && point_in_rect(game_mouse_x, game_mouse_y, itemtrade_buttons[i].x, itemtrade_buttons[i].y,
                             itemtrade_buttons[i].w, itemtrade_buttons[i].h)) {
            itemtrade_buttons[i].flags |= 2;
            itemtrade_pressed_button = i;
            if (itemtrade_button_defs[i].flags & 1) {
                if (mouse_was_pressed(0)) {
                    itemtrade_mouse_mode = 1;
                    itemtrade_repeat_timer.invalidate();
                    return key;
                }
            } else if (itemtrade_button_defs[i].flags & 4) {
                if (mouse_was_pressed(0)) {
                    itemtrade_mouse_mode = 1;
                    itemtrade_button_action(itemtrade_pressed_button + 1);
                    itemtrade_repeat_timer.SetTimeout(500);
                    return key;
                }
            }
            break;
        }
    }

    if (point_in_rect(game_mouse_x, game_mouse_y, itemtrade_scrollbar_x, itemtrade_scrollbar_y,
                      itemtrade_scrollbar_w, itemtrade_scrollbar_h)
        && mouse_was_pressed(0)) {
        itemtrade_mouse_mode = 2;
        itemtrade_drag_start_thumb = itemtrade_scrollbar_y;
        itemtrade_drag_start_y = game_mouse_y;
        return key;
    }

    if (itemtrade_gold_input_active) {
        if (key) {
            int c = key_to_ascii(key);
            if (c != 0xff) {
                if (isdigit(c)) {
                    int amount = itemtrade_gold_amount * 10 + (c - '0');
                    if (amount <= min(party_gold, ITEMTRADE_MAX_GOLD)) {
                        itemtrade_gold_amount = amount;
                        itemtrade_cursor_timer.SetTimeout(500);
                        itemtrade_cursor_on = 1;
                    }
                }
            } else {
                switch (key) {
                case KEY_ESC:
                    itemtrade_clear_gold_input();
                    break;
                case KEY_BACKSPACE:
                    if (itemtrade_gold_amount > 0) {
                        itemtrade_gold_amount /= 10;
                        itemtrade_cursor_timer.SetTimeout(500);
                        itemtrade_cursor_on = 1;
                    }
                    break;
                case KEY_ENTER:
                    itemtrade_submit_gold();
                    itemtrade_clear_gold_input();
                    break;
                }
            }
        }
        return 0;
    }

    int rows = mp_trade_local_count;
    if (rows >= itemtrade_list1_page[resolution_index])
        rows = itemtrade_list1_page[resolution_index];
    int fh = gr_get_font_height(-1);
    if (point_in_rect(game_mouse_x, game_mouse_y, itemtrade_list1_pos[resolution_index][0],
                      itemtrade_list1_pos[resolution_index][1], itemtrade_list1_width[resolution_index],
                      fh * rows)) {
        for (i = 0; i < rows; i++) {
            if (point_in_rect(game_mouse_x, game_mouse_y, itemtrade_list1_pos[resolution_index][0],
                              itemtrade_list1_pos[resolution_index][1] + i * fh,
                              itemtrade_list1_width[resolution_index], fh)) {
                itemtrade_list1_hover_row = i;
                if (mouse_was_pressed(0)) {
                    itemtrade_mouse_in_list = 1;
                    itemtrade_mouse_mode = 3;
                    return key;
                }
                break;
            }
        }
    }

    rows = itemtrade_party_row_count;
    if (rows >= itemtrade_party_page[resolution_index])
        rows = itemtrade_party_page[resolution_index];
    fh = gr_get_font_height(-1);
    if (point_in_rect(game_mouse_x, game_mouse_y, itemtrade_party_pos[resolution_index][0],
                      itemtrade_party_pos[resolution_index][1], itemtrade_party_width[resolution_index],
                      fh * rows)) {
        for (i = 0; i < rows; i++) {
            if (point_in_rect(game_mouse_x, game_mouse_y, itemtrade_party_pos[resolution_index][0],
                              itemtrade_party_pos[resolution_index][1] + i * fh,
                              itemtrade_party_width[resolution_index], fh)) {
                itemtrade_party_hover_row = i;
                if (mouse_was_pressed(0)) {
                    itemtrade_mouse_in_list = 1;
                    itemtrade_mouse_mode = 4;
                    return key;
                }
                break;
            }
        }
    }

    if (key == KEY_ESC) {
        itemtrade_cancel();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        return 0;
    }

    int dx, dy, dz;
    mouse_get_delta(&dx, &dy, &dz);
    if (dz > 0)
        itemtrade_party_page_up();
    else if (dz < 0)
        itemtrade_party_page_down();
    return key;
}

// 0x419bb0  Confidence: exact (VC6 8966 byte match, w1 / D11005: `-=` then `&= (pos <= 0) - 1`, D10600).
void itemtrade_party_page_up()
{
    if (itemtrade_party_scroll_pos > 0) {
        itemtrade_party_scroll_pos -= itemtrade_party_page[resolution_index];
        itemtrade_party_scroll_pos &= (itemtrade_party_scroll_pos <= 0) - 1;
        itemtrade_update_scrollbar();
    }
}

// 0x419be0  Confidence: exact.
void itemtrade_party_page_down()
{
    int pos = itemtrade_party_scroll_pos;
    int page = itemtrade_party_page[resolution_index];
    int limit = itemtrade_party_row_count - page;

    if (pos < limit) {
        pos += page;
        if (pos < limit)
            limit = pos;
        itemtrade_party_scroll_pos = limit;
        itemtrade_update_scrollbar();
    }
}

// 0x419c10  Confidence: exact (tail jump).
void itemtrade_cancel()
{
    mp_trade_cancel();
}

// 0x419c20  Confidence: exact. Replaces the offered gold: remove the Gold entry, add the typed amount.
void itemtrade_submit_gold()
{
    mp_send_it_rmvi(itemtrade_gold_item.type);
    mp_send_it_addi(&itemtrade_gold_item, itemtrade_gold_amount);
}

// 0x419c40  Confidence: exact.
void itemtrade_clear_gold_input()
{
    itemtrade_gold_input_active = 0;
}

// 0x419c50  Confidence: exact (jump table 0x419d70, index bytes 0x419d9c for id - 2; bodies in case
// order; the cancel and gold-cancel cases share the 0x24 sound tail).
void itemtrade_button_action(int id)
{
    switch (id) {
    case 2:
        itemtrade_list1_scroll_up();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 4:
        itemtrade_list1_scroll_down();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 6:
        itemtrade_list2_scroll_up();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 8:
        itemtrade_list2_scroll_down();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 10:
        itemtrade_party_scroll_up();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 12:
        itemtrade_party_scroll_down();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 16:
        itemtrade_accept();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 18:
    case 20:
        itemtrade_cancel();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    case 23:
        itemtrade_submit_gold();
        itemtrade_clear_gold_input();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 25:
    case 27:
        itemtrade_clear_gold_input();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    }
}

// 0x419dc0  Confidence: exact. All six scroll functions update the party list's scrollbar.
void itemtrade_list1_scroll_up()
{
    if (itemtrade_list1_scroll_pos > 0) {
        itemtrade_list1_scroll_pos--;
        itemtrade_update_scrollbar();
    }
}

// 0x419de0  Confidence: exact.
void itemtrade_list1_scroll_down()
{
    if (itemtrade_list1_scroll_pos < mp_trade_local_count - itemtrade_list1_page[resolution_index]) {
        itemtrade_list1_scroll_pos++;
        itemtrade_update_scrollbar();
    }
}

// 0x419e10  Confidence: exact.
void itemtrade_list2_scroll_up()
{
    if (itemtrade_list2_scroll_pos > 0) {
        itemtrade_list2_scroll_pos--;
        itemtrade_update_scrollbar();
    }
}

// 0x419e30  Confidence: exact.
void itemtrade_list2_scroll_down()
{
    if (itemtrade_list2_scroll_pos < mp_trade_remote_count - itemtrade_list2_page[resolution_index]) {
        itemtrade_list2_scroll_pos++;
        itemtrade_update_scrollbar();
    }
}

// 0x419e60  Confidence: exact.
void itemtrade_party_scroll_up()
{
    if (itemtrade_party_scroll_pos > 0) {
        itemtrade_party_scroll_pos--;
        itemtrade_update_scrollbar();
    }
}

// 0x419e80  Confidence: exact.
void itemtrade_party_scroll_down()
{
    if (itemtrade_party_scroll_pos < itemtrade_party_row_count - itemtrade_party_page[resolution_index]) {
        itemtrade_party_scroll_pos++;
        itemtrade_update_scrollbar();
    }
}

// 0x419eb0  Confidence: exact (tail jump).
void itemtrade_accept()
{
    mp_send_it_okay();
}

// 0x419ec0  Confidence: exact (same template as buysell_update_buttons, 28 records).
void itemtrade_update_buttons()
{
    int id = itemtrade_pressed_button + 1;

    if (itemtrade_pressed_button >= 0 && itemtrade_pressed_button < ITEMTRADE_NUM_BUTTONS) {
        ui_button_state *b = &itemtrade_buttons[id];

        if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
            b->flags |= 2;
        if (!mouse_is_down(0)) {
            if ((b->flags & 2) && !itemtrade_repeat_timer.valid())
                itemtrade_button_action(id);
        } else {
            if (itemtrade_repeat_timer.valid() && itemtrade_repeat_timer.elapsed() && (b->flags & 2)) {
                itemtrade_button_action(id);
                itemtrade_repeat_timer.SetTimeout(100);
            }
            return;
        }
    }
    itemtrade_mouse_mode = 0;
}

// 0x419f90  Confidence: exact (VC6 byte match) (one store to the scroll position, as in buysell_drag_scrollbar).
void itemtrade_drag_scrollbar()
{
    int offset = itemtrade_drag_start_thumb
               + (game_mouse_y - itemtrade_scrollbar_track[resolution_index][1] - itemtrade_drag_start_y);
    int pos = (int)((float)offset / itemtrade_scrollbar_track_h[resolution_index] * itemtrade_party_row_count
                    + 0.5f);

    pos = max(pos, 0);
    int last = itemtrade_party_row_count - itemtrade_party_page[resolution_index];
    if (pos < last)
        last = pos;
    itemtrade_party_scroll_pos = last;
    itemtrade_update_scrollbar();
    if (!mouse_is_down(0))
        itemtrade_mouse_mode = 0;
}

// 0x41a020  Confidence: exact. Mouse mode 3: released over the pressed row of list 1 takes that offer back.
void itemtrade_list1_handle_mouse()
{
    int fh = gr_get_font_height(-1);

    int x = itemtrade_list1_pos[resolution_index][0];
    int y = itemtrade_list1_pos[resolution_index][1] + itemtrade_list1_hover_row * fh;
    int w = itemtrade_list1_width[resolution_index];

    itemtrade_mouse_in_list = point_in_rect(game_mouse_x, game_mouse_y, x, y, w, fh);
    if (mouse_is_down(0))
        return;
    if (itemtrade_mouse_in_list) {
        mp_send_it_rmvi(mp_trade_local_items[itemtrade_list1_scroll_pos + itemtrade_list1_hover_row].type);
        game_play_sound(0x27, 1.0f, 0, 0, 0);
    }
    itemtrade_mouse_mode = 0;
}

// 0x41a0c0  Confidence: exact (VC6 byte match) (the mode reset is duplicated into the gold branch, as VC6 may do for a
// common tail; the offered-count loop has no break, so the last match wins). Mouse mode 4: released over
// the pressed party row offers one more unit (while some are not yet offered), or opens the gold box.
void itemtrade_party_handle_mouse()
{
    int fh = gr_get_font_height(-1);

    int x = itemtrade_party_pos[resolution_index][0];
    int y = itemtrade_party_pos[resolution_index][1] + itemtrade_party_hover_row * fh;
    int w = itemtrade_party_width[resolution_index];

    itemtrade_mouse_in_list = point_in_rect(game_mouse_x, game_mouse_y, x, y, w, fh);
    if (mouse_is_down(0))
        return;
    if (itemtrade_mouse_in_list) {
        party_inventory_entry *entry = itemtrade_get_party_row(itemtrade_party_scroll_pos + itemtrade_party_hover_row);

        if (entry == &itemtrade_gold_entry) {
            itemtrade_begin_gold_input();
            game_play_sound(0x27, 1.0f, 0, 0, 0);
        } else {
            int offered = 0;
            for (int i = 0; i < mp_trade_local_count; i++) {
                if (entry->item->type == mp_trade_local_items[i].type)
                    offered = mp_trade_local_items[i].count;
            }
            if (entry->count - offered > 0) {
                mp_send_it_addi(entry->item, 1);
                game_play_sound(0x27, 1.0f, 0, 0, 0);
            }
        }
    }
    itemtrade_mouse_mode = 0;
}

// 0x41a1d0  Confidence: exact. Row 0 is the gold pseudo entry while the party has gold.
party_inventory_entry *itemtrade_get_party_row(int row)
{
    if (party_gold > 0) {
        if (row == 0)
            return &itemtrade_gold_entry;
        return &party_inventory[row - 1];
    }
    return &party_inventory[row];
}

// 0x41a200  Confidence: exact.
void itemtrade_begin_gold_input()
{
    itemtrade_gold_input_active = 1;
    itemtrade_cursor_timer.SetTimeout(500);
    itemtrade_cursor_on = 1;
    chat_input_cancel();
}

// 0x41a230  Confidence: exact. Full-screen clip; the gold box records (def flag 8) are drawn on top of
// the lists, and only while the box is open.
void itemtrade_draw()
{
    int clip_x, clip_y, clip_w, clip_h;
    int i;

    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    gr__set_clip(0, 0, gr_screen_width(), gr_screen_height());
    gr_set_color(255, 255, 255, 255);
    for (i = 0; i < ITEMTRADE_NUM_BUTTONS; i++) {
        ui_button_state *b = &itemtrade_buttons[i];

        if (!(itemtrade_button_defs[i].flags & 8) && (b->flags & 3))
            ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
    }
    itemtrade_draw_list1();
    itemtrade_draw_list2();
    itemtrade_draw_party_list();
    itemtrade_draw_scrollbar();
    if (itemtrade_gold_input_active) {
        gr_set_color(255, 255, 255, 255);
        for (i = 0; i < ITEMTRADE_NUM_BUTTONS; i++) {
            ui_button_state *b = &itemtrade_buttons[i];

            if ((itemtrade_button_defs[i].flags & 8) && (b->flags & 3))
                ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
        }
        itemtrade_draw_gold_input();
    }
    gr__set_clip(clip_x, clip_y, clip_w, clip_h);
}

// 0x41a360  Confidence: exact (VC6 8966 byte match, w1 / D11005: rows as a ternary, the row index in a local).
// Our offer: name, count
// right-aligned at the row width.
void itemtrade_draw_list1()
{
    if (mp_trade_local_count <= 0)
        return;

    int x = itemtrade_list1_pos[resolution_index][0];
    int y = itemtrade_list1_pos[resolution_index][1];
    int rows = mp_trade_local_count < itemtrade_list1_page[resolution_index] ? mp_trade_local_count
                                                                             : itemtrade_list1_page[resolution_index];
    int fh = gr_get_font_height(-1);
    gr_set_color_ptr(&ui_color_text);

    for (int i = 0; i < rows; i++) {
        int row = itemtrade_list1_scroll_pos + i;
        mp_trade_item *item = &mp_trade_local_items[row];
        char buf[64];
        int w, h;

        if (i == itemtrade_list1_hover_row)
            gr_set_color_ptr(&ui_color_highlight);
        else
            gr_set_color_ptr(&ui_color_text);
        item_get_display_name(buf, item->type);
        gr_string(x, y, buf, -1, gr_string_mode);
        sprintf(buf, "%d", item->count);
        gr_get_string_size(&w, &h, buf, -1, -1);
        gr_string(itemtrade_list1_width[resolution_index] - w + x, y, buf, -1, gr_string_mode);
        y += fh;
    }
}

// 0x41a490  Confidence: exact (VC6 8966 byte match, w1 / D11005: as itemtrade_draw_list1). The partner's offer;
// no hover highlight.
void itemtrade_draw_list2()
{
    if (mp_trade_remote_count <= 0)
        return;

    int x = itemtrade_list2_pos[resolution_index][0];
    int y = itemtrade_list2_pos[resolution_index][1];
    int rows = mp_trade_remote_count < itemtrade_list2_page[resolution_index] ? mp_trade_remote_count
                                                                             : itemtrade_list2_page[resolution_index];
    int fh = gr_get_font_height(-1);
    gr_set_color_ptr(&ui_color_text);

    for (int i = 0; i < rows; i++) {
        int row = itemtrade_list2_scroll_pos + i;
        mp_trade_item *item = &mp_trade_remote_items[row];
        char buf[64];
        int w, h;

        item_get_display_name(buf, item->type);
        gr_string(x, y, buf, -1, gr_string_mode);
        sprintf(buf, "%d", item->count);
        gr_get_string_size(&w, &h, buf, -1, -1);
        gr_string(itemtrade_list2_width[resolution_index] - w + x, y, buf, -1, gr_string_mode);
        y += fh;
    }
}

// 0x41a5a0  Confidence: exact (VC6 8966 byte match, w1 / D11005: rows as a ternary, the row index in a local).
// Party rows with the count not yet offered (the first matching offer).
// The gold row shows the party_gold copied by itemtrade_init.
void itemtrade_draw_party_list()
{
    if (itemtrade_party_row_count <= 0)
        return;

    int x = itemtrade_party_pos[resolution_index][0];
    int y = itemtrade_party_pos[resolution_index][1];
    int rows = itemtrade_party_row_count < itemtrade_party_page[resolution_index] ? itemtrade_party_row_count
                                                                                    : itemtrade_party_page[resolution_index];
    int fh = gr_get_font_height(-1);

    for (int i = 0; i < rows; i++) {
        int row = itemtrade_party_scroll_pos + i;
        party_inventory_entry *entry = itemtrade_get_party_row(row);
        char buf[64];
        int w, h;
        int offered;
        int j;

        if (i == itemtrade_party_hover_row)
            gr_set_color_ptr(&ui_color_highlight);
        else
            gr_set_color_ptr(&ui_color_text);
        item_get_display_name(buf, entry->item->type);
        gr_string(x, y, buf, -1, gr_string_mode);

        offered = 0;
        for (j = 0; j < mp_trade_local_count; j++) {
            if (entry->item->type == mp_trade_local_items[j].type) {
                offered = mp_trade_local_items[j].count;
                break;
            }
        }
        sprintf(buf, "%d", entry->count - offered);
        gr_get_string_size(&w, &h, buf, -1, -1);
        gr_string(itemtrade_party_width[resolution_index] - w + x, y, buf, -1, gr_string_mode);
        y += fh;
    }
}

// 0x41a700  Confidence: exact (scrollbar tile template).
void itemtrade_draw_scrollbar()
{
    if (!itemtrade_scrollbar_visible)
        return;
    gr_set_color(255, 255, 255, 255);
    int n = itemtrade_scrollbar_h / itemtrade_scrollbar_bm_h;
    for (int i = 0; i < n; i++)
        ui_bitmap_scaled(itemtrade_scrollbar_bm, itemtrade_scrollbar_x,
                         itemtrade_scrollbar_y + itemtrade_scrollbar_bm_h * i, gr_bitmap_wrap_mode);
    ui_bitmap_scaled(itemtrade_scrollbar_bm, itemtrade_scrollbar_x,
                     itemtrade_scrollbar_y - itemtrade_scrollbar_bm_h + itemtrade_scrollbar_h, gr_bitmap_wrap_mode);
}

// 0x41a7b0  Confidence: exact (VC6 8966 byte match, m1019: min(0, size - w) in a local and an if / else of two gr_string calls, which 8966 tail-merges) (locals: w +0x00, clip h/w/y/x +0x04..+0x10, h +0x14, the "%s|" text
// +0x18, the number +0x38 in a 0x58 frame). The amount, measured with the cursor bar, is drawn in a clip
// window and shifted left when it is wider; the bar blinks with itemtrade_cursor_on.
void itemtrade_draw_gold_input()
{
    int w;
    int clip_h, clip_w, clip_y, clip_x;
    int h;
    char text[32];
    char number[32];

    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    gr__set_clip(itemtrade_gold_input_pos[resolution_index][0], itemtrade_gold_input_pos[resolution_index][1],
                 itemtrade_gold_input_size[resolution_index][0], itemtrade_gold_input_size[resolution_index][1]);
    sprintf(number, "%d", itemtrade_gold_amount);
    sprintf(text, "%s|", number);
    gr_get_string_size(&w, &h, text, -1, -1);
    gr_set_color_ptr(&ui_color_text);
    int x = min(0, itemtrade_gold_input_size[resolution_index][0] - w);
    if (itemtrade_cursor_on)
        gr_string(x, 0, text, -1, gr_string_mode);
    else
        gr_string(x, 0, number, -1, gr_string_mode);
    gr__set_clip(clip_x, clip_y, clip_w, clip_h);
}

// 0x41a8b0  Confidence: exact. Width of the background record, for interface_set_view_clip.
int itemtrade_get_panel_width()
{
    return itemtrade_buttons[0].w;
}

// 0x41a8c0  Confidence: exact. Case 19 of interface_mouse_over_panel: the background, or the gold box
// while it is open.
bool itemtrade_mouse_in_panel(int x, int y)
{
    if (point_in_rect(x, y, itemtrade_buttons[0].x, itemtrade_buttons[0].y,
                      itemtrade_buttons[0].w, itemtrade_buttons[0].h))
        return true;
    if (itemtrade_gold_input_active
        && point_in_rect(x, y, itemtrade_buttons[21].x, itemtrade_buttons[21].y,
                         itemtrade_buttons[21].w, itemtrade_buttons[21].h))
        return true;
    return false;
}
