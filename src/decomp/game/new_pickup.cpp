// new_pickup.cpp - screen 4, the pickup window for a container (Sum.exe v1.40, 0x4205b0-0x42158c).
// Written from the Sum.exe disassembly only (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
//
// Lists the container's items (name, stack count, a check box per row; all checked on open),
// with a scroll bar, a "take" button (check mark; also ENTER) and a close button (also ESC).
// In multiplayer the container is released with mp_send_container_close when the window closes.
//
// Confidence per function: exact = every instruction accounted for and the source shape is the
// obvious one; likely = same behaviour and probably the same code, but register/stack choices or
// an equivalent source spelling could differ; unsure = behaviour right, shape guessed.

#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/key.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/localize/localize.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "engine/gamesound/gamesound.h"
#include "engine/objects/objects.h"
#include "summoner/multi/multi.h"
#include "summoner/player/player.h"
#include "summoner/interface/new_pickup.h"

// ---------------------------------------------------------------- .rdata (0x580220-0x580477)
// Nine buttons in hilite/press pairs (see new_assess.cpp); action id = hilite index + 1.
const screen_button_def pickup_button_defs[9] = {                        // 0x580220 (*new* name)
    { "Pickup",             { { 170, 121 }, { 213, 151 }, { 172, 194 } }, 0 },
    { "Icon-CheckHilite",   { { 427, 302 }, { 534, 378 }, { 585, 484 } }, 1 },
    { "Icon-CheckPress",    { { 427, 302 }, { 534, 378 }, { 585, 484 } }, 0 },
    { "Icon-ArrwUPHilite",  { { 444, 179 }, { 555, 223 }, { 615, 285 } }, 4 },
    { "Icon-ArrwUPPress",   { { 444, 179 }, { 555, 223 }, { 615, 285 } }, 0 },
    { "Icon-ArrwDNHilite",  { { 444, 265 }, { 555, 331 }, { 615, 421 } }, 4 },
    { "Icon-ArrwDNPress",   { { 444, 265 }, { 555, 331 }, { 615, 421 } }, 0 },
    { "Icon-GrnClseHilite", { { 427, 142 }, { 534, 178 }, { 584, 229 } }, 1 },
    { "Icon-GrnClsePress",  { { 427, 142 }, { 534, 178 }, { 584, 229 } }, 0 },
};
struct pickup_point { int x, y; };
// Top left of the first list row per resolution_index (the 1024 entry is {210, 293} in the binary).
const pickup_point pickup_list_origin[3] = { { 190, 183 }, { 238, 229 }, { 210, 293 } };  // 0x580460 (*new*)

// ---------------------------------------------------------------- .data (0x58f390-0x58f4b7)
int pickup_list_row_width[3]  = { 245, 307, 398 };     // 0x58f390
int pickup_list_count_x[3]    = { 218, 275, 363 };     // 0x58f39c (*new*) right edge of the stack count
int pickup_list_button_x[3]   = { 230, 288, 380 };     // 0x58f3a8 x of the check box bitmap
// [ui_hires][normal, highlighted]; strings in reverse order at 0x58f40c-0x58f4b3
char *pickup_checkbox_names[2][2] = {                  // 0x58f3b4 (*new*) -> pickup_item_pressed_bm
    { "checkbox640.tga", "checkbox-hilite640.tga" },
    { "checkbox1024.tga", "checkbox-hilite1024.tga" },
};
char *pickup_crossbox_names[2][2] = {                  // 0x58f3c4 (*new*) -> pickup_item_bm
    { "crossbox640.tga", "crossbox-hilite640.tga" },
    { "crossbox1024.tga", "crossbox-hilite1024.tga" },
};
// Same literals as new_assess.cpp; the binary points at assess's copies (0x588b8c, 0x588b74).
char *pickup_scrollbar_names[2] = { "ScrollBarMid640.tga", "ScrollBarMid1024.tga" };   // 0x58f3d4 (*new*)
pickup_point pickup_scrollbar_track[3] = { { 446, 210 }, { 559, 262 }, { 622, 335 } }; // 0x58f3dc (y = 0x58f3e0 in sum.symbols)
int pickup_scrollbar_track_height[3] = { 35, 44, 56 };                                 // 0x58f3f4
int pickup_page_size[3] = { 7, 7, 7 };                                                 // 0x58f400
// "%d" (pickup_draw_item_list) is the merged literal at 0x589820, outside this unit's range.

// ---------------------------------------------------------------- .bss (0x5c3a30-0x5c3b73)
bool pickup_item_selected[16];        // 0x5c3a30
int pickup_mouse_mode;                // 0x5c3a40 0 idle, 1 button held, 2 list pressed, 3 dragging
int pickup_unused_5c3a44;             // 0x5c3a44 never referenced (a 4-byte hole; *new*, see NOTES)
screen_button pickup_buttons[9];      // 0x5c3a48
int pickup_thumb_x;                   // 0x5c3b20
int pickup_thumb_y;                   // 0x5c3b24
int pickup_item_bm[2];                // 0x5c3b28 crossbox (unchecked) normal / highlighted
int pickup_thumb_w;                   // 0x5c3b30
int pickup_thumb_h;                   // 0x5c3b34
int pickup_drag_start_thumb_y;        // 0x5c3b38
RealtimeTimer pickup_button_timer;    // 0x5c3b3c constructed by init_realtimetimer_5c3b3c
int pickup_item_pressed_bm[2];        // 0x5c3b40 checkbox (checked) normal / highlighted
int pickup_thumb_bm_w;                // 0x5c3b48
int pickup_thumb_bm_h;                // 0x5c3b4c
unsigned int pickup_flags;            // 0x5c3b50 bit 0 paused the game, bit 1 container held (multiplayer)
int pickup_pressed_button;            // 0x5c3b54
bool pickup_list_pressed;             // 0x5c3b58
int pickup_hover_item;                // 0x5c3b5c
int pickup_drag_start_mouse_y;        // 0x5c3b60
int pickup_scroll_pos;                // 0x5c3b64
int pickup_scrollbar_bm;              // 0x5c3b68
container *pickup_container;          // 0x5c3b6c
bool pickup_scrollbar_visible;        // 0x5c3b70

// init_realtimetimer_5c3b3c_thunk (0x4205b0, static_initializers entry 62) and
// init_realtimetimer_5c3b3c (0x4205c0: mov ecx, 0x5c3b3c / jmp RealtimeTimer__RealtimeTimer) are
// the compiler-generated dynamic initialiser pair for pickup_button_timer. Confidence: exact.
// 0x4205b0 init_realtimetimer_5c3b3c_thunk (jmp, entry 62). Confidence: exact.
// 0x4205c0 init_realtimetimer_5c3b3c (mov ecx, 0x5c3b3c / jmp RealtimeTimer::RealtimeTimer). Confidence: exact.

// 0x4205d0, called from interface_load_all. Confidence: exact
void pickup_load_bitmaps()
{
    float scale = ui_get_scale();
    char name[128];
    int i;

    for (i = 0; i < 9; i++) {
        screen_button *b = &pickup_buttons[i];
        const screen_button_def *d = &pickup_button_defs[i];
        b->flags = 0;
        b->bitmap = -1;
        b->x = 0;
        b->y = 0;
        b->w = 0;
        b->h = 0;
        progress_update(0);
        if (strlen(d->name) == 0)
            continue;
        strcpy(name, d->name);
        strcat(name, ui_bitmap_suffix_tga[ui_hires]);
        b->bitmap = bm_load(name, path_interface, -1);
        b->x = d->pos[resolution_index].x;
        b->y = d->pos[resolution_index].y;
        bm_get_dimensions(b->bitmap, &b->w, &b->h);
        b->w = (int)(b->w * scale + 0.5f);
        b->h = (int)(b->h * scale + 0.5f);
        if (i == 0)
            b->flags |= 1;
    }

    for (i = 0; i < 2; i++) {
        pickup_item_pressed_bm[i] = bm_load(pickup_checkbox_names[ui_hires][i], path_interface, -1);
        pickup_item_bm[i] = bm_load(pickup_crossbox_names[ui_hires][i], path_interface, -1);
    }

    pickup_scrollbar_bm = bm_load(pickup_scrollbar_names[ui_hires], path_interface, -1);
    bm_get_dimensions(pickup_scrollbar_bm, &pickup_thumb_bm_w, &pickup_thumb_bm_h);
    pickup_thumb_bm_w = (int)(pickup_thumb_bm_w * scale);
    pickup_thumb_bm_h = (int)(pickup_thumb_bm_h * scale);
}

// 0x4207d0, screen 4 init (interface_set_screen passes the container). Confidence: exact (VC6 byte match,
// match9: the container is stored before the memset, D6704)
void pickup_init(container *c)
{
    pickup_container = c;
    memset(pickup_item_selected, 1, sizeof(pickup_item_selected));
    pickup_flags = 0;
    if (game_flags & 1) {
        if (!game_is_paused()) {
            pickup_flags |= 1;
            game_pause(0, 0);
        }
    } else {
        ui_screen_stop_player();
        pickup_flags |= 2;
    }
    pickup_scroll_pos = 0;
    pickup_update_scrollbar();
}

// 0x420850. Unlike the assess bar the thumb keeps the full track height. Confidence: exact
void pickup_update_scrollbar()
{
    if (pickup_container->num_items <= pickup_page_size[resolution_index]) {
        pickup_scrollbar_visible = 0;
        pickup_buttons[3].flags |= 4;
        pickup_buttons[5].flags |= 4;
        return;
    }
    pickup_buttons[3].flags &= ~4;
    pickup_buttons[5].flags &= ~4;
    pickup_scrollbar_visible = 1;
    if (pickup_scroll_pos <= 0) {
        pickup_thumb_x = pickup_scrollbar_track[resolution_index].x;
        pickup_thumb_y = pickup_scrollbar_track[resolution_index].y;
        pickup_thumb_w = pickup_thumb_bm_w;
        pickup_thumb_h = pickup_scrollbar_track_height[resolution_index];
    } else {
        pickup_thumb_x = pickup_scrollbar_track[resolution_index].x;
        pickup_thumb_y = (int)((float)pickup_scroll_pos / pickup_container->num_items
                               * pickup_scrollbar_track_height[resolution_index] + 0.5f)
                       + pickup_scrollbar_track[resolution_index].y;
        pickup_thumb_w = pickup_thumb_bm_w;
        pickup_thumb_h = pickup_scrollbar_track_height[resolution_index];
    }
}

// 0x420960, screen 4 leave. Confidence: exact
void pickup_leave()
{
    if (pickup_flags & 1)
        game_unpause();
    if ((game_flags & 2) && (pickup_flags & 2))
        mp_send_container_close(pickup_container->handle, 0, 0, 0, pickup_item_selected);
}

// 0x4209b0, screen 4 per-frame input. Jump table 0x420a14 (4 cases). Confidence: exact (VC6 byte match, match9; was
// likely)
int pickup_frame(int key)
{
    for (int i = 0; i < 9; i++)
        pickup_buttons[i].flags &= ~2;

    if (!(player_input[0] & 0x10)) {
        switch (pickup_mouse_mode) {
        case 0:
            key = pickup_handle_click(key);
            break;
        case 1:
            pickup_update_buttons();
            break;
        case 2:
            pickup_list_handle_click();
            break;
        case 3:
            pickup_drag_scrollbar();
            break;
        }
    }
    return key;
}

// 0x420a30, mouse idle: buttons, thumb, list rows, ENTER / ESC and the mouse wheel.
// Confidence: exact (VC6 byte match, match9; was likely)
int pickup_handle_click(int key)
{
    pickup_pressed_button = -1;
    pickup_hover_item = -1;
    pickup_list_pressed = 0;

    int i;
    for (i = 0; i < 9; i++) {
        screen_button *b = &pickup_buttons[i];
        const screen_button_def *d = &pickup_button_defs[i];
        if (!(b->flags & 4) && (d->flags & 5)
            && point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h)) {
            b->flags |= 2;
            pickup_pressed_button = i;
            if (d->flags & 1) {
                if (mouse_was_pressed(0)) {
                    pickup_mouse_mode = 1;
                    pickup_button_timer.invalidate();
                    return key;
                }
            } else if (d->flags & 4) {
                if (mouse_was_pressed(0)) {
                    pickup_mouse_mode = 1;
                    pickup_button_action(pickup_pressed_button + 1);
                    pickup_button_timer.SetTimeout(500);
                    return key;
                }
            }
            break;
        }
    }

    if (point_in_rect(game_mouse_x, game_mouse_y, pickup_thumb_x, pickup_thumb_y,
                      pickup_thumb_w, pickup_thumb_h)
        && mouse_was_pressed(0)) {
        pickup_mouse_mode = 3;
        pickup_drag_start_thumb_y = pickup_thumb_y;
        pickup_drag_start_mouse_y = game_mouse_y;
        return key;
    }

    int fh = gr_get_font_height(-1);
    int first = pickup_scroll_pos;
    int end = first + pickup_page_size[resolution_index];
    if (end >= pickup_container->num_items)
        end = pickup_container->num_items;
    for (i = first; i < end; i++) {
        int x = pickup_list_origin[resolution_index].x;
        int y = pickup_list_origin[resolution_index].y + (i - first) * fh;
        int w = pickup_list_row_width[resolution_index];
        if (point_in_rect(game_mouse_x, game_mouse_y, x, y, w, fh)) {
            pickup_list_pressed = 1;
            pickup_hover_item = i;
            if (mouse_was_pressed(0)) {
                pickup_mouse_mode = 2;
                return key;
            }
            break;
        }
    }

    switch (key) {
    case KEY_ENTER:
        pickup_take_selected();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        return 0;
    case KEY_ESC:
        pickup_close();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        return 0;
    }

    int dx, dy, dz;
    mouse_get_delta(&dx, &dy, &dz);
    if (dz > 0)
        pickup_page_up();
    else if (dz < 0)
        pickup_page_down();
    return key;
}

// 0x420d20. Confidence: likely (the max(x, 0) AND register, D7506).
void pickup_page_up()
{
    if (pickup_scroll_pos > 0) {
        pickup_scroll_pos = max(pickup_scroll_pos - pickup_page_size[resolution_index], 0);
        pickup_update_scrollbar();
    }
}

// 0x420d50 (two stores to pickup_scroll_pos, the count re-read after the first). Confidence: exact
void pickup_page_down()
{
    if (pickup_scroll_pos < pickup_container->num_items - pickup_page_size[resolution_index]) {
        pickup_scroll_pos += pickup_page_size[resolution_index];
        int last = pickup_container->num_items - pickup_page_size[resolution_index];
        if (pickup_scroll_pos < last)
            last = pickup_scroll_pos;
        pickup_scroll_pos = last;
        pickup_update_scrollbar();
    }
}

// 0x420da0, takes every checked item: the party gets them (item_obj_pickup), the inventory is
// sorted, an empty container is deleted, "N items picked up" counts stack sizes of non-gold items.
// Confidence: exact (VC6 byte match, match9; items at esp+0x10, msg at +0x50: msg is 128 bytes, filling the
// 0xc0 frame; the old reading "msg 0x70 bytes, 16 unused" is replaced, D6704)
void pickup_take_selected()
{
    item_obj *items[16];
    char msg[128];
    int picked = 0;
    int num_items = 0;
    int i;

    for (i = 0; i < pickup_container->num_items; i++)
        items[num_items++] = pickup_container->get_item(i);

    if (game_flags & 2) {
        int num_taken = 0;
        for (i = 0; i < num_items; i++) {
            if (pickup_item_selected[i])
                num_taken++;
        }
        mp_send_container_close(pickup_container->handle, num_taken, num_items, items,
                                pickup_item_selected);
        pickup_flags &= ~2;
    }

    for (i = 0; i < num_items; i++) {
        if (pickup_item_selected[i]) {
            pickup_container->remove_item(items[i]);
            if (!items[i]->is_gold())
                picked += items[i]->count;
            item_obj_pickup(items[i], 0);
        }
    }

    party_inventory_sort(party_inventory, party_inventory_count);
    sound_play_2d(0x23, 1.0f);
    if (pickup_container->num_items <= 0)
        obj_mark_for_delete(pickup_container);

    if (picked == 1) {
        sprintf(msg, XSTR("1 item picked up", 700));
        hud_show_message(msg);
    } else if (picked > 1) {
        sprintf(msg, XSTR("%d items picked up", 701), picked);
        hud_show_message(msg);
    }
    interface_set_screen(0, 0);
}

// 0x420f10. Confidence: exact
void pickup_close()
{
    if (game_flags & 2) {
        mp_send_container_close(pickup_container->handle, 0, 0, 0, pickup_item_selected);
        pickup_flags &= ~2;
    }
    interface_set_screen(0, 0);
}

// 0x420f60, id = button index + 1 (2 take, 4 up, 6 down, 8 close). Jump table 0x420fe0 for id - 2
// in 0..6. The bodies are laid out up, down, take, close, which I read as the source order.
// Confidence: likely
void pickup_button_action(int id)
{
    switch (id) {
    case 4:
        pickup_scroll_up();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 6:
        pickup_scroll_down();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 2:
        pickup_take_selected();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 8:
        pickup_close();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    }
}

// 0x421000 (asm.mjs mis-decodes it after the jump table; bytes a1 64 3b 5c 00 85 c0 7e 0b 48 ...).
// Confidence: exact
void pickup_scroll_up()
{
    if (pickup_scroll_pos > 0) {
        pickup_scroll_pos--;
        pickup_update_scrollbar();
    }
}

// 0x421020. Confidence: exact
void pickup_scroll_down()
{
    if (pickup_scroll_pos < pickup_container->num_items - pickup_page_size[resolution_index]) {
        pickup_scroll_pos++;
        pickup_update_scrollbar();
    }
}

// 0x421050, mouse mode 1. Same as assess_update_buttons except that the release fires without
// checking the repeat timer. Confidence: exact (VC6 byte match, match9; was likely)
void pickup_update_buttons()
{
    int id = pickup_pressed_button + 1;
    if (pickup_pressed_button >= 0 && pickup_pressed_button < 9) {
        screen_button *b = &pickup_buttons[id];
        if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
            b->flags |= 2;
        if (!mouse_is_down(0)) {
            if (b->flags & 2)
                pickup_button_action(id);
        } else {
            if (pickup_button_timer.valid() && pickup_button_timer.elapsed() && (b->flags & 2)) {
                pickup_button_action(id);
                pickup_button_timer.SetTimeout(100);
            }
            return;
        }
    }
    pickup_mouse_mode = 0;
}

// 0x421110, mouse mode 2: a row was pressed; on release over the same row its check box toggles.
// Confidence: exact
void pickup_list_handle_click()
{
    int fh = gr_get_font_height(-1);
    int dy = (pickup_hover_item - pickup_scroll_pos) * fh;
    int x = pickup_list_origin[resolution_index].x;
    int y = pickup_list_origin[resolution_index].y + dy;
    int w = pickup_list_row_width[resolution_index];
    pickup_list_pressed = point_in_rect(game_mouse_x, game_mouse_y, x, y, w, fh);
    if (mouse_is_down(0))
        return;
    if (pickup_list_pressed) {
        pickup_item_selected[pickup_hover_item] = !pickup_item_selected[pickup_hover_item];
        game_play_sound(0x27, 1.0f, 0, 0, 0);
    }
    pickup_mouse_mode = 0;
}

// 0x4211c0, mouse mode 3 (two stores to pickup_scroll_pos, as in pickup_page_down).
// Confidence: exact (VC6 byte match, match9; was likely)
void pickup_drag_scrollbar()
{
    int offset = pickup_drag_start_thumb_y + (game_mouse_y - pickup_scrollbar_track[resolution_index].y - pickup_drag_start_mouse_y);
    int pos = (int)((float)offset / pickup_scrollbar_track_height[resolution_index]
                    * pickup_container->num_items + 0.5f);
    pickup_scroll_pos = max(pos, 0);
    int last = pickup_container->num_items - pickup_page_size[resolution_index];
    if (pickup_scroll_pos < last)
        last = pickup_scroll_pos;
    pickup_scroll_pos = last;
    pickup_update_scrollbar();
    if (!mouse_is_down(0))
        pickup_mouse_mode = 0;
}

// 0x421260, screen 4 draw. Confidence: exact
void pickup_draw()
{
    gr_set_color(255, 255, 255, 255);
    for (int i = 0; i < 9; i++) {
        screen_button *b = &pickup_buttons[i];
        if (b->flags & 3)
            ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
    }
    pickup_draw_item_list();
    pickup_draw_scrollbar();
}

// 0x4212c0. Confidence: exact
void pickup_draw_scrollbar()
{
    if (!pickup_scrollbar_visible)
        return;
    gr_set_color(255, 255, 255, 255);
    int n = pickup_thumb_h / pickup_thumb_bm_h;
    for (int i = 0; i < n; i++)
        ui_bitmap_scaled(pickup_scrollbar_bm, pickup_thumb_x,
                         pickup_thumb_y + pickup_thumb_bm_h * i, gr_bitmap_wrap_mode);
    ui_bitmap_scaled(pickup_scrollbar_bm, pickup_thumb_x,
                     pickup_thumb_y - pickup_thumb_bm_h + pickup_thumb_h, gr_bitmap_wrap_mode);
}

// 0x421370, one row per visible item: name, stack count right-aligned at pickup_list_count_x,
// then the check box (checked / unchecked, highlighted while the row is pressed).
// Confidence: likely (one 64-byte buffer serves the name and the count; locals end 0x0c,
// w 0x10, fh 0x14, h 0x18, buf 0x1c)
void pickup_draw_item_list()
{
    if (!pickup_container)
        return;

    int end;
    int w;
    int fh;
    int h;
    char buf[64];
    int x = pickup_list_origin[resolution_index].x;
    int y = pickup_list_origin[resolution_index].y;
    fh = gr_get_font_height(-1);
    end = pickup_scroll_pos + pickup_page_size[resolution_index];
    if (end >= pickup_container->num_items)
        end = pickup_container->num_items;

    for (int i = pickup_scroll_pos; i < end; i++) {
        item_obj *it = pickup_container->get_item(i);
        gr_set_color_ptr(&ui_color_text);
        item_get_display_name(buf, it->item->type);
        gr_string(x, y, buf, -1, gr_string_mode);
        sprintf(buf, "%d", it->count);
        gr_get_string_size(&w, &h, buf, -1, -1);
        gr_string(pickup_list_count_x[resolution_index] - w + x, y, buf, -1, gr_string_mode);
        gr_set_color(255, 255, 255, 255);
        if (pickup_item_selected[i]) {
            if (pickup_list_pressed && i == pickup_hover_item)
                ui_bitmap_scaled(pickup_item_pressed_bm[1], pickup_list_button_x[resolution_index] + x, y,
                                 gr_bitmap_wrap_mode);
            else
                ui_bitmap_scaled(pickup_item_pressed_bm[0], pickup_list_button_x[resolution_index] + x, y,
                                 gr_bitmap_wrap_mode);
        } else {
            if (pickup_list_pressed && i == pickup_hover_item)
                ui_bitmap_scaled(pickup_item_bm[1], pickup_list_button_x[resolution_index] + x, y,
                                 gr_bitmap_wrap_mode);
            else
                ui_bitmap_scaled(pickup_item_bm[0], pickup_list_button_x[resolution_index] + x, y,
                                 gr_bitmap_wrap_mode);
        }
        y += fh;
    }
}

// 0x421560, case 4 of interface_mouse_over_panel. Confidence: exact
bool pickup_mouse_in_panel(int x, int y)
{
    return point_in_rect(x, y, pickup_buttons[0].x, pickup_buttons[0].y,
                         pickup_buttons[0].w, pickup_buttons[0].h);
}
