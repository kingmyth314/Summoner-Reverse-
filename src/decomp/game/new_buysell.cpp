// new_buysell.cpp - Sum.exe v1.40, code 0x4048b0-0x4063a2 (screen 7, the shop)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .rdata 0x57e99c-0x57ea1f (layout tables; 0.5f at 0x57e6b0 is the pooled __real of new_assess.cpp),
// .data 0x588e08-0x589822 (button table, scrollbar names, "bs_cameras.tbl", "%d"), .bss 0x5b2db8-0x5b6803.
//
// Mode 0 (buy) lists the shopkeeper's +Sell List; mode 1 (sell) lists the party items that are on the shop's
// +Buy List (shop_find_item_type). Single click selects a row (and pages its mesh into item slot 0x12 for
// the 3D view), double click buys / sells it at once. Prices are shop_item.price unchanged
// (docs/sum-exe/shops-loot.md).

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
#include "vsdk/ca/character.h"
#include "vsdk/os/plainstring.h"
#include "engine/objects/living_entity.h"
#include "engine/objects/item_paging.h"
#include "levelscripts/level_scripts.h"
#include "summoner/multi/multi.h"
#include "summoner/interface/new_buysell.h"

#define BUYSELL_NUM_BUTTONS 40
#define BUYSELL_PAGE_SLOT   0x12      // item paging slot of the shop's 3D view (ITEM_PAGE_SLOT_POPUP)

// ---- .rdata --------------------------------------------------------------------------------
// Per resolution_index (640, 800, 1024). All names here are *new* (sum.symbols has none).
const int buysell_list_pos[3][2]         = { { 27, 123 }, { 34, 154 }, { 43, 197 } };   // 0x57e99c first row
const int buysell_list_width[3]          = { 300, 375, 480 };                           // 0x57e9b4 row width, price right edge
const int buysell_page_size[3]           = { 16, 16, 16 };                              // 0x57e9c0 visible rows
const int buysell_scrollbar_track[3][2]  = { { 345, 152 }, { 431, 190 }, { 552, 243 } };// 0x57e9cc
const int buysell_scrollbar_track_h[3]   = { 174, 218, 278 };                           // 0x57e9e4
const int buysell_model_view_offset[3][2] = { { 13, -25 }, { 16, -31 }, { 21, -40 } }; // 0x57e9f0
const int buysell_gold_pos[3][2]         = { { 475, 410 }, { 594, 513 }, { 760, 656 } };// 0x57ea08 party gold

// ---- .data ---------------------------------------------------------------------------------
// 0x588e08  40 x 0x40 (*new* name). flags: 1 HiLite acting on release through the Press entry after it,
// 2 acting on press with auto-repeat (the scroll arrows), 4 shows a category tooltip on hover.
// Action id = HiLite index + 1 (buysell_button_action).
ui_button_def buysell_button_defs[BUYSELL_NUM_BUTTONS] = {
    { "BuySell-Bkgrnd",      { { 0, 0 },     { 0, 0 },     { 0, 0 } },     0 },
    { "BuySell-Buy",         { { 120, 50 },  { 150, 63 },  { 226, 78 } },  0 },   // title, mode 0
    { "BuySell-Sell",        { { 120, 50 },  { 150, 63 },  { 226, 78 } },  0 },   // title, mode 1
    { "BuySell-Price",       { { 242, 90 },  { 303, 113 }, { 387, 148 } }, 0 },   // column, mode 0
    { "BuySell-Offer",       { { 242, 90 },  { 303, 113 }, { 387, 148 } }, 0 },   // column, mode 1
    { "Icon-ArrwLFHilite",   { { 21, 23 },   { 26, 29 },   { 31, 35 } },   1 },   // 6 status screen
    { "Icon-ArrwLFPress",    { { 21, 23 },   { 26, 29 },   { 31, 35 } },   0 },
    { "Icon-ArrwRTHilite",   { { 316, 23 },  { 395, 29 },  { 503, 35 } },  1 },   // 8 journal / inventory
    { "Icon-ArrwRTPress",    { { 316, 23 },  { 395, 29 },  { 503, 35 } },  0 },
    { "Icon-ArrwLFHilite",   { { 21, 61 },   { 26, 76 },   { 31, 94 } },   1 },   // 10 toggle buy / sell
    { "Icon-ArrwLFPress",    { { 21, 61 },   { 26, 76 },   { 31, 94 } },   0 },
    { "Icon-ArrwRTHilite",   { { 316, 61 },  { 395, 76 },  { 503, 94 } },  1 },   // 12 toggle buy / sell
    { "Icon-ArrwRTPress",    { { 316, 61 },  { 395, 76 },  { 503, 94 } },  0 },
    { "Icon-ArrwUPHilite",   { { 342, 121 }, { 429, 152 }, { 546, 192 } }, 2 },   // 14 scroll up
    { "Icon-ArrwUPPress",    { { 342, 121 }, { 429, 152 }, { 546, 192 } }, 0 },
    { "Icon-ArrwDNHilite",   { { 342, 341 }, { 429, 426 }, { 546, 539 } }, 2 },   // 16 scroll down
    { "Icon-ArrwDNPress",    { { 342, 341 }, { 429, 426 }, { 546, 539 } }, 0 },
    { "BuySell-Weap",        { { 80, 391 },  { 100, 489 }, { 128, 625 } }, 0 },
    { "BuySell-WeapHiLite",  { { 80, 391 },  { 100, 489 }, { 128, 625 } }, 5 },   // 19 weapons
    { "BuySell-WeapPress",   { { 80, 391 },  { 100, 489 }, { 128, 625 } }, 0 },
    { "BuySell-Armr",        { { 120, 391 }, { 150, 489 }, { 193, 625 } }, 0 },
    { "BuySell-ArmrHiLite",  { { 120, 391 }, { 150, 489 }, { 193, 625 } }, 5 },   // 22 armour
    { "BuySell-ArmrPress",   { { 120, 391 }, { 150, 489 }, { 193, 625 } }, 0 },
    { "Buysell-Mgk",         { { 161, 391 }, { 201, 489 }, { 258, 625 } }, 0 },
    { "Buysell-MgkHiLite",   { { 161, 391 }, { 201, 489 }, { 258, 625 } }, 5 },   // 25 magic items
    { "Buysell-MgkPress",    { { 161, 391 }, { 201, 489 }, { 258, 625 } }, 0 },
    { "Buysell-Jwlry",       { { 201, 391 }, { 251, 489 }, { 323, 625 } }, 0 },
    { "Buysell-JwlryHiLite", { { 201, 391 }, { 251, 489 }, { 323, 625 } }, 5 },   // 28 jewelry
    { "Buysell-JwlryPress",  { { 201, 391 }, { 251, 489 }, { 323, 625 } }, 0 },
    { "Buysell-Misc",        { { 242, 391 }, { 303, 489 }, { 387, 625 } }, 0 },
    { "Buysell-MiscHiLite",  { { 242, 391 }, { 303, 489 }, { 387, 625 } }, 5 },   // 31 miscellaneous
    { "Buysell-MiscPress",   { { 242, 391 }, { 303, 489 }, { 387, 625 } }, 0 },
    { "BuySell-BuyButton",   { { 421, 352 }, { 526, 440 }, { 675, 563 } }, 0 },   // label, mode 0
    { "BuySell-SellButton",  { { 421, 352 }, { 526, 440 }, { 675, 563 } }, 0 },   // label, mode 1
    { "PXOChat-HelpHilite",  { { 421, 352 }, { 526, 440 }, { 675, 563 } }, 1 },   // 35 buy / sell
    { "PXOChat-HelpPress",   { { 421, 352 }, { 526, 440 }, { 675, 563 } }, 0 },
    { "PXOChat-HelpHilite",  { { 502, 352 }, { 628, 440 }, { 808, 563 } }, 1 },   // 37 item info
    { "PXOChat-HelpPress",   { { 502, 352 }, { 628, 440 }, { 808, 563 } }, 0 },
    { "Icon-GrnClseHilite",  { { 599, 18 },  { 749, 23 },  { 959, 32 } },  1 },   // 39 close
    { "Icon-GrnClsePress",   { { 599, 18 },  { 749, 23 },  { 959, 32 } },  0 },
};
// 0x589808 (*new*). The strings are the pooled copies at 0x588b8c / 0x588b74, outside this unit.
char *buysell_scrollbar_names[2] = { "ScrollBarMid640.tga", "ScrollBarMid1024.tga" };
// "bs_cameras.tbl" (0x589810) and "%d" (0x589820, also used by new_pickup.cpp and new_itemtrade.cpp)
// are this unit's literals.

// ---- .bss ----------------------------------------------------------------------------------
int buysell_scroll_pos;                                       // 0x5b2db8 (0x5b2dbc: 4-byte hole, see NOTES)
unsigned char buysell_cameras[0x3628];                        // 0x5b2dc0 new_icam table (size of status_cameras)
int buysell_scrollbar_x;                                      // 0x5b63e8 thumb x
int buysell_scrollbar_y;                                      // 0x5b63ec thumb y
int buysell_scrollbar_w;                                      // 0x5b63f0 thumb w
int buysell_scrollbar_h;                                      // 0x5b63f4 thumb h
int buysell_hover_row;                                        // 0x5b63f8 visible row under the mouse, -1
int buysell_row_count;                                        // 0x5b63fc rows in the list
int buysell_list_count;                                       // 0x5b6400 entries of the current shop list
int buysell_mouse_mode;                                       // 0x5b6404 0 idle, 1 button held, 2 dragging
int buysell_drag_start_thumb;                                 // 0x5b6408
RealtimeTimer buysell_repeat_timer;                           // 0x5b640c
int buysell_mode;                                             // 0x5b6410 0 buy, 1 sell
int buysell_saved_view_x;                                     // 0x5b6414
int buysell_saved_view_y;                                     // 0x5b6418
int buysell_scrollbar_bm_w;                                   // 0x5b641c
int buysell_scrollbar_bm_h;                                   // 0x5b6420
int buysell_selected_item;                                    // 0x5b6424 selected row (scroll included), -1
int buysell_paused_game;                                      // 0x5b6428 1 unpause, 2 refresh statscreen, 4 clear shop (on leave)
shop_item *buysell_list_entries;                              // 0x5b642c the shop list of the current mode
ui_button_state buysell_buttons[BUYSELL_NUM_BUTTONS];         // 0x5b6430
int buysell_drag_start_y;                                     // 0x5b67f0
shopkeeper *buysell_shop;                                     // 0x5b67f4
int buysell_scrollbar_bm;                                     // 0x5b67f8
bool buysell_scrollbar_visible;                               // 0x5b67fc
int buysell_pressed_button;                                   // 0x5b6800 pressed button (sum.symbols name; see NOTES)

// The compiler's dynamic initialiser pair for buysell_repeat_timer (no source of their own):
// 0x4048b0 init_realtimetimer_5b640c_thunk ("jmp 0x4048c0", static_initializers entry 8). Confidence: exact.
// 0x4048c0 init_realtimetimer_5b640c (mov ecx, 0x5b640c / jmp RealtimeTimer::RealtimeTimer). Confidence: exact.

// 0x4048d0  Confidence: exact (VC6 byte match) (VC6 merged the two "info button disabled" tails; written as plain if/else).
// Buy / sell button (34) disabled while buying something the party cannot afford; info button (36) needs
// an identified item type.
void buysell_update_button_states()
{
    if (buysell_selected_item >= 0) {
        shop_item *entry = buysell_get_list_item(buysell_selected_item);
        ItemType *type = &item_types[entry->item_type];

        if (buysell_mode != 1 && party_gold < entry->price)
            buysell_buttons[34].flags |= 4;
        else
            buysell_buttons[34].flags &= ~4;
        if (item_type_is_identified(type))
            buysell_buttons[36].flags &= ~4;
        else
            buysell_buttons[36].flags |= 4;
    } else {
        buysell_buttons[34].flags |= 4;
        buysell_buttons[36].flags |= 4;
    }
}

// 0x404960  Confidence: exact. Mode 1 walks the party inventory and counts only items the shop buys;
// the entry returned is the shop's buy-list record for that item.
shop_item *buysell_get_list_item(int row)
{
    if (buysell_mode == 0)
        return &buysell_list_entries[row];

    for (int i = 0; i < party_inventory_count; i++) {
        int index = shop_find_item_type(party_inventory[i].item->type);
        if (index >= 0) {
            if (row == 0)
                return &buysell_list_entries[index];
            row--;
        }
    }
    return 0;
}

// 0x4049c0  Confidence: exact. Index of the type in the shop's +Buy List, -1.
int shop_find_item_type(ItemType *type)
{
    int index = type - item_types;

    for (int i = 0; i < buysell_shop->num_buy; i++) {
        if (buysell_shop->buy[i].item_type == index)
            return i;
    }
    return -1;
}

// 0x404a00  Confidence: exact.
void bs_cameras_parse_table()
{
    new_icam_load(buysell_cameras, "bs_cameras.tbl");
}

// 0x404a20  Confidence: exact. Same loop as journal_load_bitmaps (40 entries).
void buysell_load_bitmaps()
{
    float scale = ui_get_scale();
    int i;

    for (i = 0; i < BUYSELL_NUM_BUTTONS; i++) {
        ui_button_state *b = &buysell_buttons[i];
        ui_button_def *d = &buysell_button_defs[i];

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

    buysell_scrollbar_bm = bm_load(buysell_scrollbar_names[ui_hires], path_interface, -1);
    bm_get_dimensions(buysell_scrollbar_bm, &buysell_scrollbar_bm_w, &buysell_scrollbar_bm_h);
    buysell_scrollbar_bm_w = (int)(buysell_scrollbar_bm_w * scale);
    buysell_scrollbar_bm_h = (int)(buysell_scrollbar_bm_h * scale);
}

// 0x404bd0  Confidence: exact (VC6 byte match) (the second living_list test after statscreen_refresh_anim is what VC6
// leaves of the loop exit test; the loop shape is a guess). Pauses single player, stops the player in
// multiplayer; needs a shopkeeper (shop_set_shopkeeper_from_char) and a usable party member for the
// status_character model (the first type 7 member without flags 0x108 when the current one is unusable).
void buysell_init()
{
    buysell_paused_game = 6;
    if (game_flags & 1) {
        if (!game_is_paused()) {
            buysell_paused_game |= 1;
            game_pause(0, 0);
        }
    } else {
        ui_screen_stop_player();
    }

    if (!buysell_shop) {
        interface_set_screen(0, 0);
        return;
    }

    buysell_mouse_mode = 0;
    buysell_pressed_button = -1;
    buysell_hover_row = -1;

    if (!status_character || (status_character->flags & 0x100)) {
        Character *c = living_list.live_next;

        while (c != &living_list) {
            if (c->type == 7 && !(c->flags & 0x108)) {
                statscreen_refresh_anim(c);
                break;
            }
            c = c->live_next;
        }
        if (c == &living_list) {
            interface_set_screen(0, 0);
            return;
        }
    }
    buysell_set_category(0);
}

// 0x404cc0  Confidence: exact. Shows the title / column / button labels of the mode (bit 0 = drawn).
void buysell_set_category(int mode)
{
    buysell_scroll_pos = 0;
    buysell_mode = mode;
    buysell_selected_item = -1;
    if (mode == 0) {
        buysell_buttons[1].flags |= 1;
        buysell_buttons[3].flags |= 1;
        buysell_buttons[32].flags |= 1;
        buysell_buttons[2].flags &= ~1;
        buysell_buttons[4].flags &= ~1;
        buysell_buttons[33].flags &= ~1;
    } else {
        buysell_buttons[2].flags |= 1;
        buysell_buttons[4].flags |= 1;
        buysell_buttons[33].flags |= 1;
        buysell_buttons[1].flags &= ~1;
        buysell_buttons[3].flags &= ~1;
        buysell_buttons[32].flags &= ~1;
    }
    if (buysell_shop) {
        buysell_count_party_items();
        buysell_update_category_buttons();
        buysell_update_scrollbar();
    }
}

// 0x404dc0  Confidence: exact (VC6 byte match) (a switch on the loop index over all 40 records, jump table 0x404f48 /
// index bytes 0x404f60 for i - 17; the cases use constant records, so they may have been written as
// buysell_buttons[i] / [i + 1]). A category with no entry hides its icon and disables its hot spot.
void buysell_update_category_buttons()
{
    for (int i = 0; i < BUYSELL_NUM_BUTTONS; i++) {
        switch (i) {
        case 17:
            if (buysell_category_first_index(0) >= 0) {
                buysell_buttons[17].flags |= 1;
                buysell_buttons[18].flags &= ~4;
            } else {
                buysell_buttons[17].flags &= ~1;
                buysell_buttons[18].flags |= 4;
            }
            break;
        case 20:
            if (buysell_category_first_index(1) >= 0) {
                buysell_buttons[20].flags |= 1;
                buysell_buttons[21].flags &= ~4;
            } else {
                buysell_buttons[20].flags &= ~1;
                buysell_buttons[21].flags |= 4;
            }
            break;
        case 23:
            if (buysell_category_first_index(2) >= 0) {
                buysell_buttons[23].flags |= 1;
                buysell_buttons[24].flags &= ~4;
            } else {
                buysell_buttons[23].flags &= ~1;
                buysell_buttons[24].flags |= 4;
            }
            break;
        case 26:
            if (buysell_category_first_index(3) >= 0) {
                buysell_buttons[26].flags |= 1;
                buysell_buttons[27].flags &= ~4;
            } else {
                buysell_buttons[26].flags &= ~1;
                buysell_buttons[27].flags |= 4;
            }
            break;
        case 29:
            if (buysell_category_first_index(4) >= 0) {
                buysell_buttons[29].flags |= 1;
                buysell_buttons[30].flags &= ~4;
            } else {
                buysell_buttons[29].flags &= ~1;
                buysell_buttons[30].flags |= 4;
            }
            break;
        }
    }
}

// 0x404f70  Confidence: exact. In mode 1 the index is into the shop's buy list, not into the rows
// shown (only owned items are rows), see NOTES.
int buysell_category_first_index(int category)
{
    if (buysell_mode == 0)
        return shop_inventory_find_first_in_category(category, buysell_list_entries, buysell_list_count);

#ifdef FIXED_BUILD
    // FIXED_BUILD: K3 ShopCategoryButtons. The sell list shows the party's items that the shop buys, so return the
    // first shown row in the category (-1 when there is none, which turns the button off), not an index into the
    // shop's buy list.
    shop_item *entry;
    for (int row = 0; (entry = buysell_get_list_item(row)) != 0; row++) {
        if (item_type_shop_category(entry->item_type) == category)
            return row;
    }
    return -1;
#else
    for (int i = 0; i < buysell_list_count; i++) {
        if (item_type_shop_category(buysell_list_entries[i].item_type) == category)
            return i;
    }
    return -1;
#endif
}

// 0x404fe0  Confidence: exact (VC6 byte match) (the second test is on buysell_row_count, which is always > 0 there;
// pickup_update_scrollbar tests the scroll position instead). The thumb is page / rows of the track.
void buysell_update_scrollbar()
{
    int page = buysell_page_size[resolution_index];

    if (buysell_row_count <= page) {
        buysell_scrollbar_visible = 0;
        buysell_buttons[13].flags |= 4;
        buysell_buttons[15].flags |= 4;
        return;
    }

    buysell_buttons[13].flags &= ~4;
    buysell_scrollbar_visible = 1;
    buysell_buttons[15].flags &= ~4;
    if (buysell_row_count <= 0) {
        buysell_scrollbar_x = buysell_scrollbar_track[resolution_index][0];
        buysell_scrollbar_y = buysell_scrollbar_track[resolution_index][1];
        buysell_scrollbar_w = buysell_scrollbar_bm_w;
        buysell_scrollbar_h = buysell_scrollbar_track_h[resolution_index];
    } else {
        buysell_scrollbar_x = buysell_scrollbar_track[resolution_index][0];
        buysell_scrollbar_y = (int)((float)buysell_scroll_pos / (float)buysell_row_count
                                    * (float)buysell_scrollbar_track_h[resolution_index] + 0.5f)
                            + buysell_scrollbar_track[resolution_index][1];
        buysell_scrollbar_w = buysell_scrollbar_bm_w;
        buysell_scrollbar_h = (int)((float)page / (float)buysell_row_count
                                    * (float)buysell_scrollbar_track_h[resolution_index] + 0.5f);
    }
}

// 0x4050f0  Confidence: likely (in mode 0 num_sell is read twice, as two statements would). Picks the
// shop list of the mode and counts the rows; clamps the scroll position. Despite the name it only counts
// in mode 1 (owned items the shop buys).
void buysell_count_party_items()
{
    if (buysell_mode == 0) {
        buysell_list_entries = buysell_shop->sell;
        buysell_list_count = buysell_shop->num_sell;
        buysell_row_count = buysell_shop->num_sell;
    } else {
        buysell_list_entries = buysell_shop->buy;
        buysell_list_count = buysell_shop->num_buy;
        buysell_row_count = 0;
        for (int i = 0; i < buysell_list_count; i++) {
            if (party_inventory_find_type(&item_types[buysell_list_entries[i].item_type]) >= 0)
                buysell_row_count++;
        }
    }
    int limit = buysell_row_count - buysell_page_size[resolution_index];
    int pos = min(buysell_scroll_pos, limit);
    buysell_scroll_pos = max(pos, 0);
}

// 0x4051a0  Confidence: exact.
void buysell_leave()
{
    item_page_out_all(BUYSELL_PAGE_SLOT);
    if (buysell_paused_game & 1)
        game_unpause();
    if (buysell_paused_game & 2)
        statscreen_refresh_anim(0);
    if (buysell_paused_game & 4)
        buysell_shop = 0;
}

// 0x4051e0  Confidence: exact. Screen 7 per-frame input; the message box and the item info popup take
// the input while they are open.
int buysell_frame(int key)
{
    for (int i = 0; i < BUYSELL_NUM_BUTTONS; i++)
        buysell_buttons[i].flags &= ~2;

    buysell_update_button_states();
    if (ui_message_box_is_active())
        return key;
    if (item_info_popup_is_active())
        return item_info_popup_frame(key);

    switch (buysell_mouse_mode) {
    case 0:
        key = buysell_handle_click(key);
        break;
    case 1:
        buysell_update_buttons();
        break;
    case 2:
        buysell_drag_scrollbar();
        break;
    }
    return key;
}

// 0x405260  Confidence: likely (the B / Esc and I / Q / R bodies share tails; key jump table 0x405668,
// index bytes 0x40567c for key - 1 in 0..0x2f). Buttons, thumb, list rows, keys, mouse wheel.
int buysell_handle_click(int key)
{
    int i;

    buysell_pressed_button = -1;
    buysell_hover_row = -1;

    for (i = 0; i < BUYSELL_NUM_BUTTONS; i++) {
        ui_button_def *d = &buysell_button_defs[i];
        ui_button_state *b = &buysell_buttons[i];
        if ((d->flags & 7)
            && point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h)) {
            if ((d->flags & 4) && interface_input_ready())
                buysell_category_tooltip(i);
            if (!(b->flags & 4)) {
                b->flags |= 2;
                buysell_pressed_button = i;
                if (d->flags & 1) {
                    if (mouse_was_pressed(0)) {
                        buysell_mouse_mode = 1;
                        buysell_repeat_timer.invalidate();
                        return key;
                    }
                } else if (d->flags & 2) {
                    if (mouse_was_pressed(0)) {
                        buysell_mouse_mode = 1;
                        buysell_button_action(buysell_pressed_button + 1);
                        buysell_repeat_timer.SetTimeout(500);
                        return key;
                    }
                }
                break;
            }
        }
    }

    if (point_in_rect(game_mouse_x, game_mouse_y, buysell_scrollbar_x, buysell_scrollbar_y,
                      buysell_scrollbar_w, buysell_scrollbar_h)
        && mouse_was_pressed(0)) {
        buysell_mouse_mode = 2;
        buysell_drag_start_thumb = buysell_scrollbar_y;
        buysell_drag_start_y = game_mouse_y;
        return key;
    }

    int rows = buysell_row_count;
    if (rows >= buysell_page_size[resolution_index])
        rows = buysell_page_size[resolution_index];
    int fh = gr_get_font_height(-1);
    if (point_in_rect(game_mouse_x, game_mouse_y, buysell_list_pos[resolution_index][0],
                      buysell_list_pos[resolution_index][1], buysell_list_width[resolution_index], fh * rows)) {
        for (i = 0; i < rows; i++) {
            if (point_in_rect(game_mouse_x, game_mouse_y, buysell_list_pos[resolution_index][0],
                              buysell_list_pos[resolution_index][1] + i * fh,
                              buysell_list_width[resolution_index], fh)) {
                buysell_hover_row = i;
                if (mouse_was_double_clicked(0)) {
                    if (buysell_selected_item != buysell_scroll_pos + i) {
                        buysell_selected_item = buysell_scroll_pos + i;
                        item_page_out_all(BUYSELL_PAGE_SLOT);
                        item_page_in(buysell_get_list_item(buysell_selected_item)->item_type,
                                     BUYSELL_PAGE_SLOT, 0);
                    }
                    if (buysell_mode == 0)
                        buysell_buy_item(buysell_selected_item);
                    else
                        buysell_sell_item(buysell_selected_item);
                    return key;
                }
                if (mouse_was_pressed(0)) {
                    if (buysell_selected_item != buysell_scroll_pos + i) {
                        buysell_selected_item = buysell_scroll_pos + i;
                        item_page_out_all(BUYSELL_PAGE_SLOT);
                        item_page_in(buysell_get_list_item(buysell_selected_item)->item_type,
                                     BUYSELL_PAGE_SLOT, 0);
                    }
                    return key;
                }
                break;
            }
        }
    }

    switch (key) {
    case KEY_ESC:
    case KEY_B:
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        interface_set_screen(0, 0);
        return 0;
    case KEY_I: {
        char page = 0;
        buysell_paused_game &= ~6;
        interface_set_screen(1, &page);
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;
    }
    case KEY_Q:
        if (!(game_flags & 1))
            break;
        buysell_paused_game &= ~6;
        interface_set_screen(10, 0);
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;
    case KEY_R:
        buysell_paused_game &= ~6;
        interface_set_screen(2, 0);
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;
    }

    int dx, dy, dz;
    mouse_get_delta(&dx, &dy, &dz);
    if (dz > 0)
        buysell_page_up();
    else if (dz < 0)
        buysell_page_down();
    return key;
}

// 0x4056b0  Confidence: exact (jump table 0x405768, index bytes 0x405780 for button - 18). Hovering a
// category hot spot (records 18, 21, 24, 27, 30) shows its name as a tooltip over the hot spot.
void buysell_category_tooltip(int button)
{
    char text[256];

    switch (button) {
    case 18:
        strcpy(text, localize_strings[640]);      // "Weapons"
        break;
    case 21:
        strcpy(text, localize_strings[641]);      // "Armor"
        break;
    case 24:
        strcpy(text, localize_strings[642]);      // "Magic items"
        break;
    case 27:
        strcpy(text, localize_strings[643]);      // "Jewelry"
        break;
    case 30:
        strcpy(text, localize_strings[644]);      // "Miscellaneous items"
        break;
    default:
        return;
    }
    ui_button_state *b = &buysell_buttons[button];
    ui_tooltip_set(b->x, b->y, b->w, b->h, text);
}

// 0x405790  Confidence: exact (VC6 8966 byte match, w1 / D11005: `-=` then the clamp at 0 as `&= (pos <= 0) - 1`, as
// inventory_page_up, D10600) (the test is "jl": a position of 0 still pages, to 0).
void buysell_page_up()
{
    if (buysell_scroll_pos >= 0) {
        buysell_scroll_pos -= buysell_page_size[resolution_index];
        buysell_scroll_pos &= (buysell_scroll_pos <= 0) - 1;
        buysell_update_scrollbar();
    }
}

// 0x4057c0  Confidence: exact.
void buysell_page_down()
{
    int pos = buysell_scroll_pos;
    int page = buysell_page_size[resolution_index];
    int limit = buysell_row_count - page;

    if (pos < limit) {
        pos += page;
        if (pos < limit)
            limit = pos;
        buysell_scroll_pos = limit;
        buysell_update_scrollbar();
    }
}

// 0x4057f0  Confidence: exact (VC6 byte match) (the "Ok" button array lives in the argument slot, so it was declared
// after the last use of row). A non-stackable chargeable item the party already owns at full charge is
// refused; otherwise the full price is paid and party_inventory_add_type adds (or recharges) one.
void buysell_buy_item(int row)
{
    if (row < 0 || row >= buysell_row_count)
        return;
    shop_item *entry = buysell_get_list_item(row);
    if (!entry)
        return;

    if (party_gold >= entry->price) {
        ItemType *type = &item_types[entry->item_type];
        int slot = party_inventory_find_type(type);
        if (slot >= 0 && !item_type_is_stackable(type)) {
            item_charge_effect *charge = (item_charge_effect *)type->charge_effect;
            if (charge && party_inventory[slot].item->charges >= charge->max_charges) {
                char *buttons[1];
                buttons[0] = localize_strings[213];   // "Ok"
                ui_message_box_show(localize_strings[645], localize_strings[646], -1, -1, 1, buttons, 0, 0);
                                                      // "Warning", "This item is fully charged."
                sound_play_2d(0x24, 1.0f);
                return;
            }
        }

        party_gold -= entry->price;
        party_inventory_add_type(entry->item_type, 1, -1, 1);
        sound_play_2d(0x2b, 1.0f);
        buysell_update_category_buttons();
        return;
    }
    sound_play_2d(0x24, 1.0f);
}

// 0x405900  Confidence: exact. Sells one unit at the buy-list price; the last unit removes the entry.
void buysell_sell_item(int row)
{
    shop_item *entry = buysell_get_list_item(row);
    if (!entry)
        return;
    int slot = party_inventory_find_type(&item_types[entry->item_type]);
    if (slot < 0)
        return;

    sound_play_2d(0x2b, 1.0f);
    party_gold += entry->price;
    if (--party_inventory[slot].count <= 0) {
        party_inventory_remove(slot, 1, 1);
        buysell_selected_item = -1;
        buysell_count_party_items();
        buysell_update_scrollbar();
        item_page_out_all(BUYSELL_PAGE_SLOT);
    }
    buysell_update_category_buttons();
}

// 0x4059a0  Confidence: exact (jump table 0x405b3c, index bytes 0x405b74 for id - 6; bodies in case order).
// id = HiLite record + 1. Scroll up plays its sound before scrolling, every other case after.
void buysell_button_action(int id)
{
    switch (id) {
    case 6:
        buysell_goto_status();
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        break;
    case 8:
        buysell_goto_inventory();
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        break;
    case 10:
    case 12:
        if (buysell_mode == 0)
            buysell_set_category(1);
        else
            buysell_set_category(0);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 14:
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        buysell_scroll_up();
        break;
    case 16:
        buysell_scroll_down();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 19:
        buysell_goto_weapons();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 22:
        buysell_goto_armor();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 25:
        buysell_goto_magic_items();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 28:
        buysell_goto_jewelry();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 31:
        buysell_goto_misc();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 35:
        if (buysell_mode == 0)
            buysell_buy_item(buysell_selected_item);
        else
            buysell_sell_item(buysell_selected_item);
        break;
    case 37:
        buysell_show_item_info();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 39:
        interface_close_screen();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    }
}

// 0x405ba0  Confidence: exact.
void buysell_goto_status()
{
    buysell_paused_game &= ~6;
    interface_set_screen(2, 0);
}

// 0x405bc0  Confidence: exact. Single player: the journal (10); multiplayer: inventory page 0.
void buysell_goto_inventory()
{
    buysell_paused_game &= ~6;
    if (game_flags & 1) {
        interface_set_screen(10, 0);
    } else {
        char page[4]; // D8300: the byte sits at the bottom of its dword slot (as D7200's save[4])
        page[0] = 0;
        interface_set_screen(1, page);
    }
}

// 0x405c00  Confidence: exact.
void buysell_scroll_up()
{
    if (buysell_scroll_pos > 0) {
        buysell_scroll_pos--;
        buysell_update_scrollbar();
    }
}

// 0x405c20  Confidence: exact.
void buysell_scroll_down()
{
    if (buysell_scroll_pos < buysell_row_count - buysell_page_size[resolution_index]) {
        buysell_scroll_pos++;
        buysell_update_scrollbar();
    }
}

// 0x405c50 .. 0x405d50: scroll to the first entry of a category (0 weapons .. 4 misc), kept a page from
// the end. Five copies of the same code. Confidence: exact (one call, 0x405c53: the result is kept in a
// local before min(), which would otherwise expand the call twice).
void buysell_goto_weapons()
{
    int first = buysell_category_first_index(0);
    int pos = min(first, buysell_row_count - buysell_page_size[resolution_index]);
    buysell_scroll_pos = max(pos, 0);
    buysell_update_scrollbar();
}

// 0x405c90  Confidence: exact (as buysell_goto_weapons).
void buysell_goto_armor()
{
    int first = buysell_category_first_index(1);
    int pos = min(first, buysell_row_count - buysell_page_size[resolution_index]);
    buysell_scroll_pos = max(pos, 0);
    buysell_update_scrollbar();
}

// 0x405cd0  Confidence: exact (as buysell_goto_weapons).
void buysell_goto_magic_items()
{
    int first = buysell_category_first_index(2);
    int pos = min(first, buysell_row_count - buysell_page_size[resolution_index]);
    buysell_scroll_pos = max(pos, 0);
    buysell_update_scrollbar();
}

// 0x405d10  Confidence: exact (as buysell_goto_weapons).
void buysell_goto_jewelry()
{
    int first = buysell_category_first_index(3);
    int pos = min(first, buysell_row_count - buysell_page_size[resolution_index]);
    buysell_scroll_pos = max(pos, 0);
    buysell_update_scrollbar();
}

// 0x405d50  Confidence: exact (as buysell_goto_weapons).
void buysell_goto_misc()
{
    int first = buysell_category_first_index(4);
    int pos = min(first, buysell_row_count - buysell_page_size[resolution_index]);
    buysell_scroll_pos = max(pos, 0);
    buysell_update_scrollbar();
}

// 0x405d90  Confidence: exact.
void buysell_show_item_info()
{
    if (buysell_selected_item >= 0) {
        shop_item *entry = buysell_get_list_item(buysell_selected_item);
        if (entry)
            item_info_popup_open_type(entry->item_type);
    }
}

// 0x405db0  Confidence: exact. Mouse mode 1 (same template as journal_update_buttons): the action fires
// on release over the button unless the repeat timer runs, and repeats every 100 ms while held.
void buysell_update_buttons()
{
    int id = buysell_pressed_button + 1;

    if (buysell_pressed_button >= 0 && buysell_pressed_button < BUYSELL_NUM_BUTTONS) {
        ui_button_state *b = &buysell_buttons[id];

        if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
            b->flags |= 2;
        if (!mouse_is_down(0)) {
            if ((b->flags & 2) && !buysell_repeat_timer.valid())
                buysell_button_action(id);
        } else {
            if (buysell_repeat_timer.valid() && buysell_repeat_timer.elapsed() && (b->flags & 2)) {
                buysell_button_action(id);
                buysell_repeat_timer.SetTimeout(100);
            }
            return;
        }
    }
    buysell_mouse_mode = 0;
}

// 0x405e80  Confidence: exact (VC6 byte match) (one store to buysell_scroll_pos, so the clamp is one expression).
void buysell_drag_scrollbar()
{
    int offset = buysell_drag_start_thumb + (game_mouse_y - buysell_scrollbar_track[resolution_index][1] - buysell_drag_start_y);
    int pos = (int)((float)offset / buysell_scrollbar_track_h[resolution_index] * buysell_row_count + 0.5f);

    int limit;

    pos = max(pos, 0);
    limit = buysell_row_count - buysell_page_size[resolution_index];
    if (pos < limit)
        limit = pos;
    buysell_scroll_pos = limit;
    buysell_update_scrollbar();
    if (!mouse_is_down(0))
        buysell_mouse_mode = 0;
}

// 0x405f10  Confidence: exact. Screen 7 draw: buttons, list, scrollbar, item model, party gold, popup.
void buysell_draw()
{
    char gold[16];

    gr_set_color(255, 255, 255, 255);
    for (int i = 0; i < BUYSELL_NUM_BUTTONS; i++) {
        ui_button_state *b = &buysell_buttons[i];
        if (b->flags & 3)
            ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
    }
    buysell_draw_item_list();
    buysell_draw_scrollbar();
    buysell_draw_item_model();

    sprintf(gold, "%d", party_gold);
    gr_set_color_ptr(&ui_color_gold);
    gr_string(buysell_gold_pos[resolution_index][0], buysell_gold_pos[resolution_index][1], gold, -1,
              gr_string_mode);
    if (item_info_popup_is_active())
        item_info_popup_render();
}

// 0x405fd0  Confidence: exact (VC6 8966 byte match, w1 / D11005: rows as a ternary, `y += fh, i++` in the for
// header; one 64-byte buffer for the name and the price).
// Colours: highlight for the hovered / selected row, blue unidentified, red when status_character cannot
// use it (misc items, kind 2, are never red), else the text colour. Price right-aligned at the row width.
void buysell_draw_item_list()
{
    if (!buysell_shop)
        return;

    int x = buysell_list_pos[resolution_index][0];
    int y = buysell_list_pos[resolution_index][1];
    int rows = buysell_row_count < buysell_page_size[resolution_index] ? buysell_row_count
                                                                       : buysell_page_size[resolution_index];
    int fh = gr_get_font_height(-1);

    for (int i = 0; i < rows; y += fh, i++) {
        int row = buysell_scroll_pos + i;
        shop_item *entry = buysell_get_list_item(row);
        ItemType *type = &item_types[entry->item_type];
        char buf[64];
        int w, h;

        item_get_display_name(buf, type);
        if (i == buysell_hover_row || row == buysell_selected_item)
            gr_set_color_ptr(&ui_color_highlight);
        else if (!item_type_is_identified(type))
            gr_set_color_ptr(&ui_color_blue);
        else if (type->kind == 2 || character_meets_item_requirements(status_character, type))
            gr_set_color_ptr(&ui_color_text);
        else
            gr_set_color_ptr(&ui_color_red);
        gr_string(x, y, buf, -1, gr_string_mode);

        sprintf(buf, "%d", entry->price);
        gr_get_string_size(&w, &h, buf, -1, -1);
        gr_string(buysell_list_width[resolution_index] - w + x, y, buf, -1, gr_string_mode);
    }
}

// 0x406150  Confidence: exact (same as journal / pickup: whole tiles, then one tile flush with the end).
void buysell_draw_scrollbar()
{
    if (!buysell_scrollbar_visible)
        return;
    gr_set_color(255, 255, 255, 255);
    int n = buysell_scrollbar_h / buysell_scrollbar_bm_h;
    for (int i = 0; i < n; i++)
        ui_bitmap_scaled(buysell_scrollbar_bm, buysell_scrollbar_x, buysell_scrollbar_y + buysell_scrollbar_bm_h * i,
                         gr_bitmap_wrap_mode);
    ui_bitmap_scaled(buysell_scrollbar_bm, buysell_scrollbar_x,
                     buysell_scrollbar_y - buysell_scrollbar_bm_h + buysell_scrollbar_h, gr_bitmap_wrap_mode);
}

// 0x406200  Confidence: exact. The selected item's mesh (ItemType +0x10, paged in by handle_click) with
// the camera from bs_cameras.tbl.
void buysell_draw_item_model()
{
    if (buysell_selected_item < 0)
        return;
    shop_item *entry = buysell_get_list_item(buysell_selected_item);
    if (!entry)
        return;
    ItemType *type = &item_types[entry->item_type];
    void *mesh = type->mesh;
    if (!mesh)
        return;

    vector3 pos;
    matrix3 orient;
    new_icam_get(buysell_cameras, type->name, &orient, &pos);
    buysell_model_view_begin();
    ui_3d_begin(&pos);
    buysell_render_item_mesh(mesh, &pos, &orient);
    ui_3d_end();
    buysell_model_view_end();
}

// 0x406280  Confidence: exact. Shifts the clip origin by the per-resolution model view offset.
void buysell_model_view_begin()
{
    buysell_saved_view_x = gr_screen.offset_x;
    buysell_saved_view_y = gr_screen.offset_y;
    gr_screen.offset_x = buysell_saved_view_x + buysell_model_view_offset[resolution_index][0];
    gr_screen.offset_y = buysell_saved_view_y + buysell_model_view_offset[resolution_index][1];
}

// 0x4062c0  Confidence: exact.
void buysell_model_view_end()
{
    gr_screen.offset_x = buysell_saved_view_x;
    gr_screen.offset_y = buysell_saved_view_y;
}

// 0x4062e0  Confidence: exact (VC6 byte match) (the light is constructed before the mesh test, so it is declared first;
// local order guessed). Viewer at (0, 0, -3) with an identity orientation, fov 60.
void buysell_render_item_mesh(void *mesh, vector3 *pos, matrix3 *orient)
{
    gr_light_desc light;

    if (!mesh)
        return;

    vector3 view_pos;
    matrix3_t view_orient;
    view_pos.x = 0.0f;
    view_pos.y = 0.0f;
    view_pos.z = -3.0f;
    view_orient.make_identity();
    gr_setup_3d(&view_orient, &view_pos, 60.0f, 1, 1);
    ca_render_set_mesh(mesh);
    light.f14 = 0.15f;
    ca_render_mesh(pos, orient, &light);
    ca_render_set_mesh(0);
}

// 0x406360  Confidence: exact (VC6 byte match) (parameter type: see NOTES; stubs_gamecreate_journal.h has void*).
// The shopkeeper record whose name is the character's name, or none.
void shop_set_shopkeeper_from_char(void *character)
{
    if (character)
        buysell_shop = level_shopkeeper_find(((Character *)character)->name);
    else
        buysell_shop = 0;
}

// 0x406390  Confidence: exact.
bool ui_has_owner_character()
{
    return buysell_shop != 0;
}

// 0x4063a0  Confidence: exact (VC6 byte match) (name and parameters). "mov al, 1 / ret": the identical-code-folded
// *_mouse_in_panel at the end of this unit (the default of interface_mouse_over_panel; screen 7 covers
// the whole screen). The parameters are a guess from the other *_mouse_in_panel functions.
bool ui_return_true(int x, int y)
{
    return true;
}
