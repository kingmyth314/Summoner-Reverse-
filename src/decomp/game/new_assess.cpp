// new_assess.cpp - screen 20, the assess window (skill "assess" on a target; Sum.exe v1.40,
// 0x4011b0-0x401e7c). Written from the Sum.exe disassembly only (tools/asm.mjs). Target: VC++ 6.0
// (cl 12.00.8966), /O2.
//
// The window shows the target's name, level, hit points, damage, protection and magic resistance
// (L828) followed by one "<damage type>: <resistance level>" line per resistance that is not
// "standard" (magic is skipped, it is already in L828), in a clipped text box with a scroll bar,
// two arrow buttons and a close button.
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
#include "engine/objects/living_entity.h"
#include "summoner/interface/new_assess.h"

// ---------------------------------------------------------------- .rdata (0x57e4c0-0x57e6af)
// Seven buttons in hilite/press pairs; assess_update_buttons draws and fires the "Press" record
// that follows the pressed "Hilite" record (index + 1), and the button action ids are that index.
const screen_button_def assess_button_defs[7] = {                        // 0x57e4c0 (*new* name)
    { "Assess-Bkgrnd",      { { 170, 122 }, { 213, 153 }, { 272, 195 } }, 0 },
    { "Icon-ArrwUPHilite",  { { 441, 178 }, { 551, 223 }, { 706, 286 } }, 4 },
    { "Icon-ArrwUPPress",   { { 441, 178 }, { 551, 223 }, { 706, 286 } }, 0 },
    { "Icon-ArrwDNHilite",  { { 441, 298 }, { 551, 373 }, { 706, 470 } }, 4 },
    { "Icon-ArrwDNPress",   { { 441, 298 }, { 551, 373 }, { 706, 470 } }, 0 },
    { "Icon-GrnClseHilite", { { 427, 143 }, { 534, 179 }, { 683, 230 } }, 1 },
    { "Icon-GrnClsePress",  { { 427, 143 }, { 534, 179 }, { 683, 230 } }, 0 },
};
struct assess_point { int x, y; };
const assess_point assess_text_origin[3] = { { 190, 181 }, { 238, 226 }, { 304, 290 } };  // 0x57e680 (*new*)
const int assess_text_width[3] = { 239, 299, 382 };                                     // 0x57e698 (*new*)
const int assess_page_size[3]  = { 9, 9, 9 };                                           // 0x57e6a4 (*new*) visible lines
// 0.5f (0x57e6b0) and 100.0f (0x57e6b4) follow: __real@ COMDATs first emitted by this object,
// shared with later units (new_pickup.cpp reads 0x57e6b0).

// ---------------------------------------------------------------- .data (0x588b48-0x588bbb)
// Strings stored in reverse order (1024 at 0x588b74, 640 at 0x588b8c); new_pickup.cpp's identical
// literals were merged into these.
char *assess_scrollbar_names[2] = { "ScrollBarMid640.tga", "ScrollBarMid1024.tga" };   // 0x588b48 (*new*)
// Scroll bar track per resolution_index; sum.symbols names the y member (0x588b54).
assess_point assess_scrollbar_track[3] = { { 444, 209 }, { 555, 261 }, { 713, 338 } };  // 0x588b50
int assess_scrollbar_track_height[3] = { 80, 100, 125 };                               // 0x588b68
// Then the literals of assess_format_info, in reverse order of use: ": " 0x588ba0, "\n" 0x588ba4,
// "#01234567890" 0x588ba8, "onif" 0x588bb8.

// ---------------------------------------------------------------- .bss (0x5b27f0-0x5b29d7)
int assess_scrollbar_w;            // 0x5b27f0 thumb rectangle (w, h, x, y in that address order)
int assess_scrollbar_h;            // 0x5b27f4
int assess_scrollbar_x;            // 0x5b27f8
int assess_scrollbar_y;            // 0x5b27fc
int assess_paused_game;            // 0x5b2800 bit 0: assess_init paused the game
int assess_pressed_button;         // 0x5b2804
int assess_scrollbar_bm_w;         // 0x5b2808
int assess_scrollbar_bm_h;         // 0x5b280c
screen_button assess_buttons[7];   // 0x5b2810
int assess_scroll_pos;             // 0x5b28b8 first visible line
RealtimeTimer assess_repeat_timer; // 0x5b28bc constructed by init_realtimetimer_5b28bc
int assess_drag_start_thumb;       // 0x5b28c0
bool assess_scrollbar_visible;     // 0x5b28c4
int assess_scrollbar_bm;           // 0x5b28c8
int assess_line_count;             // 0x5b28cc
int assess_drag_start_y;           // 0x5b28d0
char assess_text[256];             // 0x5b28d4
int assess_mouse_mode;             // 0x5b29d4 0 idle, 1 button held, 2 dragging the thumb

// init_realtimetimer_5b28bc_thunk (0x4011b0, static_initializers entry 0) and
// init_realtimetimer_5b28bc (0x4011c0: mov ecx, 0x5b28bc / jmp RealtimeTimer__RealtimeTimer) are
// the compiler-generated dynamic initialiser pair for assess_repeat_timer. Confidence: exact.

// 0x4011d0, called from interface_load_all. Confidence: exact
void assess_load_bitmaps()
{
    float scale = ui_get_scale();
    char name[128];

    for (int i = 0; i < 7; i++) {
        screen_button *b = &assess_buttons[i];
        b->flags = 0;
        b->bitmap = -1;
        b->x = 0;
        b->y = 0;
        b->w = 0;
        b->h = 0;
        progress_update(0);
        const screen_button_def *d = &assess_button_defs[i];
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
    progress_update(0);

    assess_scrollbar_bm = bm_load(assess_scrollbar_names[ui_hires], path_interface, -1);
    bm_get_dimensions(assess_scrollbar_bm, &assess_scrollbar_bm_w, &assess_scrollbar_bm_h);
    // no + 0.5 here (plain _ftol of the product)
    assess_scrollbar_bm_w = (int)(assess_scrollbar_bm_w * scale);
    assess_scrollbar_bm_h = (int)(assess_scrollbar_bm_h * scale);
}

// 0x401380, screen 20 init (interface_set_screen passes the target). Confidence: exact
void assess_init(Character *target)
{
    assess_paused_game = 0;
    if (game_flags & 1) {
        if (!game_is_paused()) {
            assess_paused_game |= 1;
            game_pause(0, 0);
        }
    } else {
        ui_screen_stop_player();
    }
    assess_format_info(target);
    assess_scroll_pos = 0;
    assess_update_scrollbar();
    assess_mouse_mode = 0;
    assess_pressed_button = -1;
}

// 0x4013f0, builds assess_text and counts its lines. Confidence: likely (the position of the
// resistance copy and the local layout - levels 0x18, name 0x34, level 0x14, hp 0x10, max hp 0x0c,
// damage reusing the argument slot - follow VC6's choices, not necessarily the declaration order)
void assess_format_info(Character *target)
{
    char name[64];
    resistance_set res;
    int level, hp, max_hp, damage, protection, magic;

    if (game_flags & 2) {
        // multiplayer: the char_info name, "onif" spelled out, the "#..." / digit suffix cut
        if (_stricmp(target->info->name, "onif") == 0) {
            strcpy(name, XSTR("Oni Female", 351));
        } else {
            int n = strcspn(target->info->name, "#01234567890");
            if (n != 0) {
                strncpy(name, target->info->name, n);
                name[n] = 0;
            } else {
                strcpy(name, target->info->name);
            }
        }
    } else {
        target->get_display_name(name);
    }

    level = target->level;
    hp = (int)target->hp;
    max_hp = target->get_max_stat(0);
    if (target->equipment[5] && !(target->info->flags & 0x200))
        damage = target->equipment[5]->type->damage;
    else
        damage = target->info->damage;
    protection = (int)(target->protection.get_protection() * 100.0f + 0.5f);
    magic = (int)(target->get_resistance(3) * 100.0f + 0.5f);
    res = target->info->resistance;

    sprintf(assess_text,
            XSTR("%s\n\nLevel: %d\nHit Points: %d/%d\nDamage: %d\nProtection: %d\nMagic Resist: %d", 828),
            name, level, hp, max_hp, damage, protection, magic);

    for (int i = 0; i < 7; i++) {
        if (i == 3)                       // magic, already in L828
            continue;
        int r = res.level[i];
        if (r == 2)                       // "standard" is not listed
            continue;
        strcat(assess_text, "\n");
        strcat(assess_text, localize_string_or_invalid(damage_type_loc_names[i]));
        strcat(assess_text, ": ");
        strcat(assess_text, localize_string_or_invalid(resistance_level_names[r]));
    }

    assess_line_count = 1;
    int len = strlen(assess_text);
    for (int j = 0; j < len; j++) {
        if (assess_text[j] == '\n')
            assess_line_count++;
    }
}

// 0x4016d0. Disables the arrow buttons when everything fits, else places and sizes the thumb.
// Confidence: likely (the if / else polarity is read from the block order; the inner test is on
// assess_line_count, which is always > 0 there)
void assess_update_scrollbar()
{
    int page = assess_page_size[resolution_index];

    if (assess_line_count <= page) {
        assess_scrollbar_visible = 0;
        assess_buttons[1].flags |= 4;
        assess_buttons[3].flags |= 4;
        return;
    }
    assess_buttons[1].flags &= ~4;
    assess_buttons[3].flags &= ~4;
    assess_scrollbar_visible = 1;
    if (assess_line_count <= 0) {
        assess_scrollbar_x = assess_scrollbar_track[resolution_index].x;
        assess_scrollbar_y = assess_scrollbar_track[resolution_index].y;
        assess_scrollbar_w = assess_scrollbar_bm_w;
        assess_scrollbar_h = assess_scrollbar_track_height[resolution_index];
    } else {
        assess_scrollbar_x = assess_scrollbar_track[resolution_index].x;
        assess_scrollbar_y = (int)((float)assess_scroll_pos / (float)assess_line_count
                                   * (float)assess_scrollbar_track_height[resolution_index] + 0.5f)
                           + assess_scrollbar_track[resolution_index].y;
        assess_scrollbar_w = assess_scrollbar_bm_w;
        assess_scrollbar_h = (int)((float)page / (float)assess_line_count
                                   * (float)assess_scrollbar_track_height[resolution_index] + 0.5f);
    }
}

// 0x4017e0, screen 20 leave. Confidence: exact
void assess_leave()
{
    if (assess_paused_game & 1)
        game_unpause();
}

// 0x4017f0, screen 20 per-frame input. Confidence: likely (case 1 returns on its own; probably
// tail duplication of the common return)
int assess_frame(int key)
{
    for (screen_button *b = assess_buttons; b < assess_buttons + 7; b++)
        b->flags &= ~2;

    switch (assess_mouse_mode) {
    case 0:
        return assess_handle_click(key);
    case 1:
        assess_update_buttons();
        break;
    case 2:
        assess_drag_scrollbar();
        break;
    }
    return key;
}

// 0x401840, mouse idle: button hover / press, thumb drag start, ESC and the mouse wheel.
// Confidence: likely
int assess_handle_click(int key)
{
    assess_pressed_button = -1;
    for (int i = 0; i < 7; i++) {
        screen_button *b = &assess_buttons[i];
        if (!(b->flags & 4) && (assess_button_defs[i].flags & 5)
            && point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h)) {
            b->flags |= 2;
            assess_pressed_button = i;
            if (assess_button_defs[i].flags & 1) {
                if (mouse_was_pressed(0)) {
                    assess_mouse_mode = 1;
                    assess_repeat_timer.invalidate();
                    return key;
                }
            } else if (assess_button_defs[i].flags & 4) {
                if (mouse_was_pressed(0)) {
                    assess_mouse_mode = 1;
                    assess_button_action(assess_pressed_button + 1);
                    assess_repeat_timer.SetTimeout(500);
                    return key;
                }
            }
            break;
        }
    }

    if (point_in_rect(game_mouse_x, game_mouse_y, assess_scrollbar_x, assess_scrollbar_y,
                      assess_scrollbar_w, assess_scrollbar_h)
        && mouse_was_pressed(0)) {
        assess_mouse_mode = 2;
        assess_drag_start_thumb = assess_scrollbar_y;
        assess_drag_start_y = game_mouse_y;
        return key;
    }

    switch (key) {
    case KEY_ESC:
        interface_close_screen();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        return 0;
    }

    int dx, dy, dz;
    mouse_get_delta(&dx, &dy, &dz);
    if (dz > 0)
        assess_page_up();
    else if (dz < 0)
        assess_page_down();
    return key;
}

// 0x401a20. Confidence: exact
void assess_page_up()
{
    if (assess_scroll_pos > 0) {
        assess_scroll_pos = max(assess_scroll_pos - assess_page_size[resolution_index], 0);
        assess_update_scrollbar();
    }
}

// 0x401a50. Confidence: exact
void assess_page_down()
{
    if (assess_scroll_pos < assess_line_count - assess_page_size[resolution_index]) {
        assess_scroll_pos = min(assess_scroll_pos + assess_page_size[resolution_index],
                                assess_line_count - assess_page_size[resolution_index]);
        assess_update_scrollbar();
    }
}

// 0x401a80, id = button index + 1 (2 up, 4 down, 6 close). Confidence: likely (VC6 laid the
// bodies out 6, 4, 2 with the two 0x25 sounds tail-merged; the source order may differ)
void assess_button_action(int id)
{
    switch (id) {
    case 2:
        assess_scroll_up();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 4:
        assess_scroll_down();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 6:
        interface_close_screen();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    }
}

// 0x401ad0. Confidence: exact
void assess_scroll_up()
{
    if (assess_scroll_pos > 0) {
        assess_scroll_pos--;
        assess_update_scrollbar();
    }
}

// 0x401af0. Confidence: exact
void assess_scroll_down()
{
    if (assess_scroll_pos < assess_line_count - assess_page_size[resolution_index]) {
        assess_scroll_pos++;
        assess_update_scrollbar();
    }
}

// 0x401b20, mouse mode 1: fires the button on release (when no repeat is running) and repeats it
// every 100 ms while held. It hit-tests and fires record pressed + 1 (the "Press" record).
// Confidence: likely
void assess_update_buttons()
{
    int id = assess_pressed_button + 1;
    if (assess_pressed_button >= 0 && assess_pressed_button < 7) {
        screen_button *b = &assess_buttons[id];
        if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
            b->flags |= 2;
        if (mouse_is_down(0)) {
            if (assess_repeat_timer.valid() && assess_repeat_timer.elapsed() && (b->flags & 2)) {
                assess_button_action(id);
                assess_repeat_timer.SetTimeout(100);
            }
            return;
        }
        if ((b->flags & 2) && !assess_repeat_timer.valid())
            assess_button_action(id);
    }
    assess_mouse_mode = 0;
}

// 0x401bf0, mouse mode 2. Confidence: exact
void assess_drag_scrollbar()
{
    int offset = game_mouse_y - assess_scrollbar_track[resolution_index].y - assess_drag_start_y
               + assess_drag_start_thumb;
    int pos = (int)((float)offset / assess_scrollbar_track_height[resolution_index]
                    * assess_line_count + 0.5f);
    pos = max(pos, 0);
    assess_scroll_pos = min(pos, assess_line_count - assess_page_size[resolution_index]);
    assess_update_scrollbar();
    if (!mouse_is_down(0))
        assess_mouse_mode = 0;
}

// 0x401c80, screen 20 draw. Confidence: exact
void assess_draw()
{
    gr_set_color(255, 255, 255, 255);
    for (screen_button *b = assess_buttons; b < assess_buttons + 7; b++) {
        if (b->flags & 3)
            ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
    }
    assess_draw_text();
    assess_draw_scrollbar();
}

// 0x401ce0. Confidence: likely (the second gr_get_font_height call is in the binary; the source
// probably called it once per expression)
void assess_draw_text()
{
    int clip_h, clip_w, clip_y, clip_x;
    int fh = gr_get_font_height(-1);
    int y = -(gr_get_font_height(-1) * assess_scroll_pos);
    int h = assess_page_size[resolution_index] * fh;

    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    gr::set_clip(assess_text_origin[resolution_index].x, assess_text_origin[resolution_index].y,
                 assess_text_width[resolution_index], h);
    gr_set_color_ptr(&ui_color_text);
    gr_string(0, y, assess_text, -1, gr_string_mode);
    gr::set_clip(clip_x, clip_y, clip_w, clip_h);
}

// 0x401da0, tiles the thumb bitmap down the thumb rectangle, the last tile flush with its bottom.
// Confidence: exact
void assess_draw_scrollbar()
{
    if (!assess_scrollbar_visible)
        return;
    gr_set_color(255, 255, 255, 255);
    int n = assess_scrollbar_h / assess_scrollbar_bm_h;
    for (int i = 0; i < n; i++)
        ui_bitmap_scaled(assess_scrollbar_bm, assess_scrollbar_x,
                         assess_scrollbar_y + assess_scrollbar_bm_h * i, gr_bitmap_wrap_mode);
    ui_bitmap_scaled(assess_scrollbar_bm, assess_scrollbar_x,
                     assess_scrollbar_y - assess_scrollbar_bm_h + assess_scrollbar_h, gr_bitmap_wrap_mode);
}

// 0x401e50, case 20 of interface_mouse_over_panel: inside the background bitmap. Confidence: exact
bool assess_mouse_in_panel(int x, int y)
{
    return point_in_rect(x, y, assess_buttons[0].x, assess_buttons[0].y,
                         assess_buttons[0].w, assess_buttons[0].h);
}
