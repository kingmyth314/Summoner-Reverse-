// unnamed_item_info_popup_0x418580.cpp - Sum.exe v1.40, code 0x418580-0x419166 (the item info popup: an
// item's description text with a scroll bar and the item's 3D model, opened over the inventory screen
// and the shop screen).
// FILE NAME NOT KNOWN (placeholder). No assert path, no strings.tbl block, no reference name; module "-".
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit: new_inventory.cpp ends with world_use_item_at_point (0x418540, code to 0x418572; its .bss ends
// 0x5c146b); this object starts with its own dynamic initialiser (0x418580, static_initializers entry 53)
// and new_itemtrade.cpp starts with its two (0x419170, entries 54 and 55). D5202.
// Data: .rdata 0x57fdbc-0x57fe3f (layout tables, between new_inventory.cpp's 0x57f040-0x57fdbb and
// new_itemtrade.cpp's 0x57fe40), .data 0x58cbd8-0x58cd9f (the button table and the scroll bar names; the
// two names are new_assess.cpp's literals 0x588b8c / 0x588b74, pooled), .bss 0x5c1470-0x5c156b.
// 0.5f (0x57e6b0) is new_assess.cpp's pooled constant. The camera table is the shop's (buysell_cameras,
// bs_cameras.tbl) for both screens.
//
// Opened from the inventory with an item of the party (item_info_popup_open_item: the item is paged into
// slot 18, ITEM_PAGE_SLOT_POPUP, and paged out on close) or from the shop with an item type
// (item_info_popup_open_type: a temporary item from item_create, freed on close).

#include <stdio.h>
#include <string.h>
#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/key.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/matrix.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "summoner/interface/new_inventory.h"
#include "engine/gamesound/gamesound.h"
#include "engine/objects/items.h"
#include "engine/objects/item_paging.h"
#include "summoner/interface/unnamed_item_info_popup_0x418580.h"

#define POPUP_NUM_BUTTONS 7
#define POPUP_SOURCE_TYPE 1                 // item_create'd: freed on close
#define POPUP_SOURCE_ITEM 2                 // a party item: its model paged into ITEM_PAGE_SLOT_POPUP

// ---- .rdata (0x57fdbc-0x57fe3f) ------------------------------------------------------------------------
// Per resolution_index (640, 800, 1024). Names ours.
const int item_info_popup_title_pos[3][2]       = { { 152, 141 }, { 190, 179 }, { 243, 226 } };  // 0x57fdbc
const int item_info_popup_text_pos[3][2]        = { { 79, 210 }, { 99, 263 }, { 126, 326 } };    // 0x57fdd4
const int item_info_popup_text_width[3]         = { 292, 365, 467 };                             // 0x57fdec
const int item_info_popup_page_size[3]          = { 12, 12, 13 };                                // 0x57fdf8 lines
const int item_info_popup_scrollbar_track[3][2] = { { 383, 246 }, { 479, 308 }, { 613, 394 } };  // 0x57fe04
const int item_info_popup_scrollbar_track_h[3]  = { 116, 145, 186 };                             // 0x57fe1c
const int item_info_popup_model_offset[3][2]    = { { 17, 14 }, { 21, 18 }, { 27, 22 } };        // 0x57fe28

// ---- .data (0x58cbd8-0x58cd9f) -------------------------------------------------------------------------
// flags: 4 arrows (act on press, auto-repeat), 2 the close button (acts on release through the Press
// record after it). Action id = Hilite index + 1 (item_info_popup_button_action).
ui_button_def item_info_popup_button_defs[POPUP_NUM_BUTTONS] = {                       // 0x58cbd8
    { "ItemInfoPopupbkgrnd",  { { 45, 75 },   { 56, 94 },   { 72, 120 } },  0 },
    { "Icon-ArrwUPHilite",    { { 381, 211 }, { 476, 264 }, { 608, 334 } }, 4 },     // 2 up
    { "Icon-ArrwUPPress",     { { 381, 211 }, { 476, 264 }, { 608, 334 } }, 0 },
    { "Icon-ArrwDNHilite",    { { 381, 371 }, { 476, 464 }, { 608, 590 } }, 4 },     // 4 down
    { "Icon-ArrwDNPress",     { { 381, 371 }, { 476, 464 }, { 608, 590 } }, 0 },
    { "Icon-GrnClseHilite",   { { 552, 96 },  { 690, 120 }, { 883, 155 } }, 2 },     // 6 close
    { "Icon-GrnClsePress",    { { 552, 96 },  { 690, 120 }, { 883, 155 } }, 0 },
};
char *item_info_popup_scrollbar_names[2] = { "ScrollBarMid640.tga", "ScrollBarMid1024.tga" };  // 0x58cd98

// ---- .bss (0x5c1470-0x5c156b), address order ------------------------------------------------------------
ui_button_state item_info_popup_buttons[POPUP_NUM_BUTTONS];   // 0x5c1470
bool item_info_popup_visible;                                 // 0x5c1518
int  item_info_popup_drag_start_thumb;                        // 0x5c151c
int  item_info_popup_line_count;                              // 0x5c1520
int  item_info_popup_scrollbar_x;                             // 0x5c1524 thumb rectangle
int  item_info_popup_scrollbar_y;                             // 0x5c1528
int  item_info_popup_drag_start_y;                            // 0x5c152c
int  item_info_popup_mouse_mode;                              // 0x5c1530 0 idle, 1 button held, 2 dragging
item_info *item_info_popup_item;                              // 0x5c1534
int  item_info_popup_source;                                  // 0x5c1538 POPUP_SOURCE_*
char *item_info_popup_text;                                   // 0x5c153c ring_format_info's buffer
int  item_info_popup_pressed_button;                          // 0x5c1540
RealtimeTimer item_info_popup_repeat_timer;                   // 0x5c1544
int  item_info_popup_saved_view_x;                            // 0x5c1548
int  item_info_popup_saved_view_y;                            // 0x5c154c
int  item_info_popup_scrollbar_bm_w;                          // 0x5c1550
int  item_info_popup_scrollbar_bm_h;                          // 0x5c1554
int  item_info_popup_scrollbar_bm;                            // 0x5c1558
int  item_info_popup_scrollbar_w;                             // 0x5c155c
int  item_info_popup_scrollbar_h;                             // 0x5c1560
bool item_info_popup_scrollbar_visible;                       // 0x5c1564
int  item_info_popup_scroll_pos;                              // 0x5c1568 first visible line

// 0x418580 init_realtimetimer_5c1544_thunk (jmp) / 0x418590 init_realtimetimer_5c1544 (mov ecx, 0x5c1544 /
// jmp RealtimeTimer__RealtimeTimer): the compiler's dynamic initialiser pair of
// item_info_popup_repeat_timer (static_initializers entry 53). Confidence: exact (no source of their own).
// 0x418580 init_realtimetimer_5c1544_thunk (jmp, entry 53). Confidence: exact.
// 0x418590 init_realtimetimer_5c1544 (mov ecx, 0x5c1544 / jmp RealtimeTimer::RealtimeTimer). Confidence: exact.

// 0x4185a0  Confidence: exact (the new_assess.cpp loader; the button table is in .data here). Called from
// interface_load_all.
void item_info_popup_load_bitmaps()
{
    float scale = ui_get_scale();
    char name[128];

    for (int i = 0; i < POPUP_NUM_BUTTONS; i++) {
        ui_button_state *b = &item_info_popup_buttons[i];
        b->flags = 0;
        b->bitmap = -1;
        b->x = 0;
        b->y = 0;
        b->w = 0;
        b->h = 0;
        progress_update(0);
        ui_button_def *d = &item_info_popup_button_defs[i];
        if (strlen(d->name) == 0)
            continue;
        strcpy(name, d->name);
        strcat(name, ui_bitmap_suffix_tga[ui_hires]);
        b->bitmap = bm_load(name, path_interface, -1);
        b->x = d->pos[resolution_index][0];
        b->y = d->pos[resolution_index][1];
        bm_get_dimensions(b->bitmap, &b->w, &b->h);
        b->w = (int)(b->w * scale + 0.5f);
        b->h = (int)(b->h * scale + 0.5f);
        if (i == 0)
            b->flags |= 1;
    }
    progress_update(0);

    item_info_popup_scrollbar_bm = bm_load(item_info_popup_scrollbar_names[ui_hires], path_interface, -1);
    bm_get_dimensions(item_info_popup_scrollbar_bm, &item_info_popup_scrollbar_bm_w,
                      &item_info_popup_scrollbar_bm_h);
    item_info_popup_scrollbar_bm_w = (int)(item_info_popup_scrollbar_bm_w * scale);
    item_info_popup_scrollbar_bm_h = (int)(item_info_popup_scrollbar_bm_h * scale);
    item_info_popup_visible = false;
}

// 0x418760  Confidence: exact (the setup call is a tail jump).
void item_info_popup_open_item(item_info *item)
{
    if (!item)
        return;
    item_info_popup_item = item;
    item_page_in(item->type - item_types, ITEM_PAGE_SLOT_POPUP, 0);
    item_info_popup_source = POPUP_SOURCE_ITEM;
    item_info_popup_setup();
}

// 0x4187a0  Confidence: likely (the line counter starts at 0, unlike assess's 1: the last line is not
// counted unless the text ends with a newline).
void item_info_popup_setup()
{
    item_info_popup_mouse_mode = 0;
    item_info_popup_pressed_button = -1;
    item_info_popup_text = ring_format_info(item_info_popup_item);
    item_info_popup_line_count = 0;
    int len = strlen(item_info_popup_text);
    for (int i = 0; i < len; i++) {
        if (item_info_popup_text[i] == '\n')
            item_info_popup_line_count++;
    }
    item_info_popup_scroll_pos = 0;
    item_info_popup_update_scrollbar();
    item_info_popup_visible = true;
}

// 0x418810  Confidence: likely (new_assess.cpp's template, including the dead "line_count <= 0" branch).
void item_info_popup_update_scrollbar()
{
    int page = item_info_popup_page_size[resolution_index];

    if (item_info_popup_line_count <= page) {
        item_info_popup_scrollbar_visible = false;
        item_info_popup_buttons[1].flags |= 4;
        item_info_popup_buttons[3].flags |= 4;
        return;
    }
    item_info_popup_buttons[1].flags &= ~4;
    item_info_popup_scrollbar_visible = true;
    item_info_popup_buttons[3].flags &= ~4;
    if (item_info_popup_line_count <= 0) {
        item_info_popup_scrollbar_x = item_info_popup_scrollbar_track[resolution_index][0];
        item_info_popup_scrollbar_y = item_info_popup_scrollbar_track[resolution_index][1];
        item_info_popup_scrollbar_w = item_info_popup_scrollbar_bm_w;
        item_info_popup_scrollbar_h = item_info_popup_scrollbar_track_h[resolution_index];
    } else {
        item_info_popup_scrollbar_x = item_info_popup_scrollbar_track[resolution_index][0];
        item_info_popup_scrollbar_y =
            (int)((float)item_info_popup_scroll_pos / (float)item_info_popup_line_count
                  * (float)item_info_popup_scrollbar_track_h[resolution_index] + 0.5f)
            + item_info_popup_scrollbar_track[resolution_index][1];
        item_info_popup_scrollbar_w = item_info_popup_scrollbar_bm_w;
        item_info_popup_scrollbar_h =
            (int)((float)page / (float)item_info_popup_line_count
                  * (float)item_info_popup_scrollbar_track_h[resolution_index] + 0.5f);
    }
}

// 0x418920  Confidence: exact.
void item_info_popup_open_type(int type)
{
    if (type < 0 || type >= num_item_types)
        return;
    item_info_popup_item = item_create(type);
    if (!item_info_popup_item)
        return;
    item_info_popup_source = POPUP_SOURCE_TYPE;
    item_info_popup_setup();
}

// 0x418960  Confidence: exact. Called first by the inventory and shop frames while the popup is open.
int item_info_popup_frame(int key)
{
    if (!item_info_popup_visible)
        return key;
    for (ui_button_state *b = item_info_popup_buttons; b < item_info_popup_buttons + POPUP_NUM_BUTTONS; b++)
        b->flags &= ~2;

    switch (item_info_popup_mouse_mode) {
    case 0:
        key = item_info_popup_handle_click(key);
        break;
    case 1:
        item_info_popup_update_buttons();
        break;
    case 2:
        item_info_popup_drag_scrollbar();
        break;
    }
    return key;
}

// 0x4189c0  Confidence: likely (new_assess.cpp's template with def flag 2 for "act on release"; ESC closes
// the popup only).
int item_info_popup_handle_click(int key)
{
    item_info_popup_pressed_button = -1;
    for (int i = 0; i < POPUP_NUM_BUTTONS; i++) {
        ui_button_state *b = &item_info_popup_buttons[i];
        if (!(b->flags & 4) && (item_info_popup_button_defs[i].flags & 6)
            && point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h)) {
            b->flags |= 2;
            item_info_popup_pressed_button = i;
            if (item_info_popup_button_defs[i].flags & 2) {
                if (mouse_was_pressed(0)) {
                    item_info_popup_mouse_mode = 1;
                    item_info_popup_repeat_timer.invalidate();
                    return key;
                }
            } else if (item_info_popup_button_defs[i].flags & 4) {
                if (mouse_was_pressed(0)) {
                    item_info_popup_mouse_mode = 1;
                    item_info_popup_button_action(item_info_popup_pressed_button + 1);
                    item_info_popup_repeat_timer.SetTimeout(500);
                    return key;
                }
            }
            break;
        }
    }

    if (point_in_rect(game_mouse_x, game_mouse_y, item_info_popup_scrollbar_x, item_info_popup_scrollbar_y,
                      item_info_popup_scrollbar_w, item_info_popup_scrollbar_h)
        && mouse_was_pressed(0)) {
        item_info_popup_mouse_mode = 2;
        item_info_popup_drag_start_thumb = item_info_popup_scrollbar_y;
        item_info_popup_drag_start_y = game_mouse_y;
        return key;
    }

    switch (key) {
    case KEY_ESC:
        item_info_popup_close();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        return 0;
    }

    int dx, dy, dz;
    mouse_get_delta(&dx, &dy, &dz);
    if (dz > 0)
        item_info_popup_page_up();
    else if (dz < 0)
        item_info_popup_page_down();
    return key;
}

// 0x418ba0  Confidence: exact.
void item_info_popup_page_up()
{
    if (item_info_popup_scroll_pos > 0) {
        item_info_popup_scroll_pos = max(item_info_popup_scroll_pos - item_info_popup_page_size[resolution_index], 0);
        item_info_popup_update_scrollbar();
    }
}

// 0x418bd0  Confidence: exact.
void item_info_popup_page_down()
{
    if (item_info_popup_scroll_pos < item_info_popup_line_count - item_info_popup_page_size[resolution_index]) {
        item_info_popup_scroll_pos =
            min(item_info_popup_scroll_pos + item_info_popup_page_size[resolution_index],
                item_info_popup_line_count - item_info_popup_page_size[resolution_index]);
        item_info_popup_update_scrollbar();
    }
}

// 0x418c00  Confidence: exact. The source is tested as bits.
void item_info_popup_close()
{
    if (item_info_popup_source & POPUP_SOURCE_TYPE)
        item_free(item_info_popup_item);
    if (item_info_popup_source & POPUP_SOURCE_ITEM)
        item_page_out_all(ITEM_PAGE_SLOT_POPUP);
    item_info_popup_visible = false;
}

// 0x418c40  Confidence: likely (VC6 laid the bodies out 6, 4, 2 with the two 0x25 sounds tail-merged).
void item_info_popup_button_action(int id)
{
    switch (id) {
    case 2:
        item_info_popup_scroll_up();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 4:
        item_info_popup_scroll_down();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 6:
        item_info_popup_close();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    }
}

// 0x418c90  Confidence: exact.
void item_info_popup_scroll_up()
{
    if (item_info_popup_scroll_pos > 0) {
        item_info_popup_scroll_pos--;
        item_info_popup_update_scrollbar();
    }
}

// 0x418cb0  Confidence: exact.
void item_info_popup_scroll_down()
{
    if (item_info_popup_scroll_pos < item_info_popup_line_count - item_info_popup_page_size[resolution_index]) {
        item_info_popup_scroll_pos++;
        item_info_popup_update_scrollbar();
    }
}

// 0x418ce0  Confidence: likely (new_assess.cpp's template).
void item_info_popup_update_buttons()
{
    int id = item_info_popup_pressed_button + 1;
    if (item_info_popup_pressed_button >= 0 && item_info_popup_pressed_button < POPUP_NUM_BUTTONS) {
        ui_button_state *b = &item_info_popup_buttons[id];
        if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
            b->flags |= 2;
        if (mouse_is_down(0)) {
            if (item_info_popup_repeat_timer.valid() && item_info_popup_repeat_timer.elapsed()
                && (b->flags & 2)) {
                item_info_popup_button_action(id);
                item_info_popup_repeat_timer.SetTimeout(100);
            }
            return;
        }
        if ((b->flags & 2) && !item_info_popup_repeat_timer.valid())
            item_info_popup_button_action(id);
    }
    item_info_popup_mouse_mode = 0;
}

// 0x418db0  Confidence: exact.
void item_info_popup_drag_scrollbar()
{
    int offset = game_mouse_y - item_info_popup_scrollbar_track[resolution_index][1]
               - item_info_popup_drag_start_y + item_info_popup_drag_start_thumb;
    int pos = (int)((float)offset / item_info_popup_scrollbar_track_h[resolution_index]
                    * item_info_popup_line_count + 0.5f);
    pos = max(pos, 0);
    item_info_popup_scroll_pos = min(pos, item_info_popup_line_count - item_info_popup_page_size[resolution_index]);
    item_info_popup_update_scrollbar();
    if (!mouse_is_down(0))
        item_info_popup_mouse_mode = 0;
}

// 0x418e40  Confidence: exact. Dims the whole screen, then draws the popup.
void item_info_popup_render()
{
    if (!item_info_popup_visible)
        return;
    gr_set_color_ptr(&ui_color_text_box_bg);
    gr_rect(0, 0, gr_screen_width(), gr_screen_height(), gr_rect_mode, 1.0f);
    gr_set_color(255, 255, 255, 255);
    for (ui_button_state *b = item_info_popup_buttons; b < item_info_popup_buttons + POPUP_NUM_BUTTONS; b++) {
        if (b->flags & 3)
            ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
    }
    item_info_popup_draw_title();
    item_info_popup_draw_text();
    item_info_popup_draw_scrollbar();
    item_info_popup_draw_model();
}

// 0x418ee0  Confidence: exact.
void item_info_popup_draw_title()
{
    char name[64];

    item_get_display_name(name, item_info_popup_item->type);
    gr_set_color_ptr(&ui_color_text);
    gr_string(item_info_popup_title_pos[resolution_index][0], item_info_popup_title_pos[resolution_index][1],
              name, -1, gr_string_mode);
}

// 0x418f30  Confidence: likely (new_assess.cpp's template: two gr_get_font_height calls).
void item_info_popup_draw_text()
{
    int clip_h, clip_w, clip_y, clip_x;
    int fh = gr_get_font_height(-1);
    int y = -(gr_get_font_height(-1) * item_info_popup_scroll_pos);
    int h = item_info_popup_page_size[resolution_index] * fh;

    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    gr::set_clip(item_info_popup_text_pos[resolution_index][0], item_info_popup_text_pos[resolution_index][1],
                 item_info_popup_text_width[resolution_index], h);
    gr_set_color_ptr(&ui_color_text);
    gr_string(0, y, item_info_popup_text, -1, gr_string_mode);
    gr::set_clip(clip_x, clip_y, clip_w, clip_h);
}

// 0x418ff0  Confidence: exact.
void item_info_popup_draw_scrollbar()
{
    if (!item_info_popup_scrollbar_visible)
        return;
    gr_set_color(255, 255, 255, 255);
    int n = item_info_popup_scrollbar_h / item_info_popup_scrollbar_bm_h;
    for (int i = 0; i < n; i++)
        ui_bitmap_scaled(item_info_popup_scrollbar_bm, item_info_popup_scrollbar_x,
                         item_info_popup_scrollbar_y + item_info_popup_scrollbar_bm_h * i, gr_bitmap_wrap_mode);
    ui_bitmap_scaled(item_info_popup_scrollbar_bm, item_info_popup_scrollbar_x,
                     item_info_popup_scrollbar_y - item_info_popup_scrollbar_bm_h + item_info_popup_scrollbar_h,
                     gr_bitmap_wrap_mode);
}

// 0x4190a0  Confidence: exact (as buysell_draw_model; the shop's camera table for every item).
void item_info_popup_draw_model()
{
    ItemType *type = item_info_popup_item->type;
    vim *mesh = type->mesh;
    if (!mesh)
        return;

    vector3 pos;
    matrix3 orient;
    new_icam_get(buysell_cameras, type->name, &orient, &pos);
    item_info_popup_view_begin();
    ui_3d_begin(&pos);
    ui_render_item_mesh(mesh, &pos, &orient);
    ui_3d_end();
    item_info_popup_view_end();
}

// 0x419100  Confidence: exact.
void item_info_popup_view_begin()
{
    item_info_popup_saved_view_x = gr_screen.offset_x;
    item_info_popup_saved_view_y = gr_screen.offset_y;
    gr_screen.offset_x = item_info_popup_saved_view_x + item_info_popup_model_offset[resolution_index][0];
    gr_screen.offset_y = item_info_popup_saved_view_y + item_info_popup_model_offset[resolution_index][1];
}

// 0x419140  Confidence: exact.
void item_info_popup_view_end()
{
    gr_screen.offset_x = item_info_popup_saved_view_x;
    gr_screen.offset_y = item_info_popup_saved_view_y;
}

// 0x419160  Confidence: exact.
bool item_info_popup_is_active()
{
    return item_info_popup_visible;
}
