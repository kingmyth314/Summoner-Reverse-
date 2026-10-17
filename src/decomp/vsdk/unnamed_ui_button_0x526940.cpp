// unnamed_ui_button_0x526940.cpp - Sum.exe v1.40, code 0x526940-0x527e11 (vsdk\ui; the unit's file name
// is not in the binary, so the name is a placeholder). Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2. Confidence per function: exact = expect
// identical code, likely = same semantics, small layout or scheduling risk, unsure = the source shape is
// a guess.
//
// The push button gadget (ui_button, vtable 0x581d88): mouse and hotkey input as a small state machine
// (ui_button::process), auto-repeat, drag and drop between buttons, and drawing with three colour sets
// and five state bitmaps.
//
// Unit bounds (NOTES.md): after File::sync_bytes (0x526900, the vfile readers / writers unit) and before
// ui_static's slot 0 (0x527e20). No .data or .bss of its own (the globals are the UI manager's). The EH
// funclets of the constructor (0x57d370-0x57d3c7) end in int3 padding before ui_static's (0x57d3d0),
// which within one object's funclet block does not happen (the vfile unit's four functions share one
// block), so ui_static is a separate object.

#include <string.h>
#include "vsdk/ui/ui_gadgets.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/gr_font.h"
#include "vsdk/os/mouse.h"
#include "vsdk/math/vector.h"

// 0x526940  Confidence: exact. Called from process in states 6 and 7 (each frame of a held repeat too).
void ui_button::fire_click(int right)
{
    if (!right) {
        send_event(0xc, UI_USE_ID);
        state = 6;
    } else {
        send_event(0xd, UI_USE_ID);
        state = 7;
    }
}

// 0x526990  Confidence: exact.
void ui_button::set_latched(bool latched)
{
    if (latched)
        flags |= UI_BUTTON_LATCHED;
    else
        flags &= ~UI_BUTTON_LATCHED;
}

// 0x5269c0  Confidence: exact.
void ui_button::toggle_latched()
{
    if (flags & UI_BUTTON_LATCHED)
        flags &= ~UI_BUTTON_LATCHED;
    else
        flags |= UI_BUTTON_LATCHED;
}

// 0x5269e0  Confidence: exact. Reads the element's own hidden byte (not is_hidden()).
bool ui_button::is_latched()
{
    if (hidden)
        return false;
    return (flags >> 5) & 1;
}

// 0x526a00  Confidence: exact. A drag is going on, this button takes drops, the types match and the
// dragged amount fits.
bool ui_button::can_accept_drop()
{
    ui_button *d = ui_drag_gadget;

    if (d && (flags & UI_BUTTON_DRAGGABLE) && (drop_types & d->drag_type)) {
        if (capacity < 0 || d->drag_amount + count <= capacity)
            return true;
    }
    return false;
}

// 0x526a50  vtable slot 5. Confidence: likely (the order of the tests inside the cases).
// The click states (6 - 9) from the last frame end first: a held repeat button goes back to 2 / 3.
// A disabled button passes the key on; a hidden one only plays the press sound. Then the state machine
// runs until a case returns: a state change goes round again (a state above 9 would loop for ever).
// A hotkey (unless an input box has the focus) or Enter on the selected button presses it and eats the
// key; everything else returns the key.
int ui_button::process(int key)
{
    if (state == 6) {
        if (mouse_is_down(0) && (flags & UI_BUTTON_REPEATS))
            state = 2;
        else
            state = 0;
    } else if (state == 7) {
        if (mouse_is_down(1) && (flags & UI_BUTTON_REPEATS))
            state = 3;
        else
            state = 0;
    } else if (state == 8 || state == 9) {
        state = 0;
    }

    if (is_disabled())
        return key;
    if (is_hidden()) {
        if ((ui_left_pressed || ((flags & UI_BUTTON_RIGHT_CLICK) && ui_right_pressed)) && is_mouse_over())
            ui_play_sound(2);
        if (has_hotkey(key))
            ui_play_sound(2);
        return key;
    }

    for (;;) {
        switch (state) {
        case 0:
            if ((ui_current_gadget == NULL || ui_current_gadget->get_type() != UI_TYPE_INPUTBOX) &&
                has_hotkey(key))
                goto pressed;
            if (is_selected() && key == 0x1c)
                goto pressed;
            if (!is_mouse_over())
                return key;
            if (ui_left_double_clicked) {
                state = 8;
                break;
            }
            if ((flags & UI_BUTTON_RIGHT_CLICK) && ui_right_double_clicked) {
                state = 9;
                break;
            }
            if (ui_left_pressed) {
                press_x = ui_mouse_x;
                press_y = ui_mouse_y;
                if (flags & UI_BUTTON_REPEATS) {
                    repeat_timer.SetTimeout(450);
                    state = 6;
                } else {
                    state = 2;
                }
                break;
            }
            if ((flags & UI_BUTTON_RIGHT_CLICK) && ui_right_pressed) {
                press_x = ui_mouse_x;
                press_y = ui_mouse_y;
                if (flags & UI_BUTTON_REPEATS) {
                    repeat_timer.SetTimeout(450);
                    state = 7;
                } else {
                    state = 3;
                }
                break;
            }
            if (!can_accept_drop())
                return key;
            state = 5;
            break;
        pressed:
            press_timer.SetTimeout(100);
            state = 1;
            key = 0;
            break;

        case 1:
            if (!press_timer.elapsed())
                return key;
            state = 6;
            break;

        case 2:
            if (is_mouse_over()) {
                if (ui_left_released) {
                    state = (flags & UI_BUTTON_REPEATS) ? 0 : 6;
                    break;
                }
                if (!(flags & UI_BUTTON_REPEATS) || !repeat_timer.elapsed())
                    return key;
                repeat_timer.SetTimeout(150);
                state = 6;
                break;
            }
            if (ui_left_released) {
                state = 0;
                break;
            }
            if (!(flags & UI_BUTTON_DRAGGABLE) || drag_type == 0)
                return key;
            if (capacity >= 0 && count < drag_amount)
                return key;
            state = 4;
            ui_drag_gadget = this;
            break;

        case 3:
            if (is_mouse_over()) {
                if (ui_right_released) {
                    state = (flags & UI_BUTTON_REPEATS) ? 0 : 7;
                    break;
                }
                if (!(flags & UI_BUTTON_REPEATS) || !repeat_timer.elapsed())
                    return key;
                repeat_timer.SetTimeout(150);
                state = 7;
                break;
            }
            if (!ui_right_released)
                return key;
            state = 0;
            break;

        case 4:
            if (!ui_left_released)
                return key;
            state = 0;
            break;

        case 5:
            if (!is_mouse_over()) {
                state = 0;
                break;
            }
            if (!ui_left_released)
                return key;
            ui_drop_target = this;
            if (capacity >= 0)
                count += ui_drag_gadget->drag_amount;
            send_event(0x11, UI_USE_ID);
            if (ui_drag_gadget->capacity >= 0)
                ui_drag_gadget->count -= ui_drag_gadget->drag_amount;
            ui_drag_gadget->send_event(0x10, UI_USE_ID);
            ui_drag_dropped = true;
            ui_drag_gadget = NULL;
            state = 0;
            break;

        case 6:
            fire_click(0);
            return key;

        case 7:
            fire_click(1);
            return key;

        case 8:
            send_event(0xe, UI_USE_ID);
            return key;

        case 9:
            send_event(0xf, UI_USE_ID);
            return key;
        }
    }
}

// 0x526f30  vtable slot 0. Confidence: exact (the clear order; the drag bitmap is not cleared).
void ui_button::dtor()
{
    state_normal.clear();
    state_disabled.clear();
    state_highlighted.clear();
    state_pressed.clear();
    ui_gadget::dtor();
}

// 0x526f70  Confidence: exact. The four state bitmaps are linked into one ring (the drag bitmap stays
// alone).
ui_button::ui_button()
{
    text[0] = '\0';
    state_highlighted.link(&state_normal);
    state_pressed.link(&state_highlighted);
    state_disabled.link(&state_pressed);
}

// 0x527050  Confidence: exact (store order).
void ui_button::create(ui_element *window, int x, int y, int w, int h, int id, int flags, bool owned)
{
    init(window, UI_TYPE_BUTTON, x, y, w, h, id, owned);
    this->flags = flags;
    state = 0;
    text[0] = '\0';
    font = -1;
    press_timer.SetTimeout(0);
    press_x = 0;
    press_y = 0;
    colors = ui_button_colors;
    drag_type = 0;
    drop_types = 0;
    count = 0;
    capacity = 0;
    drag_amount = 1;
    _unk420 = 0xff;
    drag_data = this;
}

// 0x527100  Confidence: exact. As create, in the parent's window, and always with UI_BUTTON_REPEATS
// (or al, 2; NOTES.md).
void ui_button::create_in_parent(ui_gadget *parent, int x, int y, int w, int h, int id, int flags,
                                 bool owned)
{
    init(parent->get_window(), UI_TYPE_BUTTON, x, y, w, h, id, owned);
    state = 0;
    text[0] = '\0';
    this->flags = flags | UI_BUTTON_REPEATS;
    font = -1;
    press_timer.SetTimeout(0);
    press_x = 0;
    press_y = 0;
    colors = ui_button_colors;
    drag_type = 0;
    drop_types = 0;
    count = 0;
    capacity = 0;
    drag_amount = 1;
    _unk420 = 0xff;
    drag_data = this;
    set_parent(parent);
}

// 0x5271c0  Confidence: likely (store order). The first tooltip is unlinked (and leaked, not deleted)
// before the new one is appended.
void ui_button::set_tooltip(const char *text, int x, int y, int delay)
{
    ui_tooltip *t = tooltips.next;

    if (t != (ui_tooltip *)&tooltips) {
        ui_tooltip *prev = t->prev;
        ui_tooltip *next = t->next;
        t->next = NULL;
        t->prev = NULL;
        prev->next = next;
        next->prev = prev;
    }
    t = new ui_tooltip;
    strcpy(t->text, text);
    t->owner = this;
    t->x = x;
    ui_tooltip *last = tooltips.prev;
    t->delay = delay;
    t->next = (ui_tooltip *)&tooltips;
    tooltips.prev = t;
    t->colors = ui_tooltip_colors;
    t->y = y;
    t->font = -1;
    t->prev = last;
    last->next = t;
}

// 0x527280  Confidence: exact. No caller.
void ui_button::set_repeats(bool repeats)
{
    if (repeats) {
        flags |= UI_BUTTON_REPEATS;
        repeat_timer.SetTimeout(0);
    } else {
        flags &= ~UI_BUTTON_REPEATS;
    }
}

// 0x5272c0  Confidence: exact.
void ui_button::set_text(const char *text)
{
    strcpy(this->text, text);
}

// 0x5272f0  Confidence: exact.
bool ui_button::was_clicked()
{
    if (is_disabled() || is_hidden())
        return false;
    return state == 6 || state == 8;
}

// 0x527330  Confidence: exact. No caller.
bool ui_button::was_left_clicked()
{
    if (is_disabled() || is_hidden())
        return false;
    return state == 6;
}

// 0x527360  Confidence: exact. No caller.
bool ui_button::was_double_clicked()
{
    return state == 8;
}

// 0x527370  Confidence: exact.
bool ui_button::is_pressed()
{
    return state == 1 || ((state == 2 || state == 3) && is_mouse_over())
        || ((flags & UI_BUTTON_REPEATS) && (state == 6 || state == 7));
}

// 0x5273c0  Confidence: exact.
int ui_button::get_flags()
{
    return flags;
}

// 0x5273d0  Confidence: exact.
char *ui_button::get_text()
{
    return text;
}

// 0x5273e0  Confidence: exact.
void ui_button::get_drag_origin(int *x, int *y)
{
    *x = get_screen_x(0) - press_x;
    *y = get_screen_y(0) - press_y;
}

// 0x527420  Confidence: exact.
void ui_button::press()
{
    press_timer.SetTimeout(100);
    state = 1;
}

// 0x527440  Confidence: likely (the loop counters). A triangle of gr_line strokes inside a quarter
// margin: horizontal lines (one per row, the length rising to aw and back) from the left edge, or from
// the right edge with flag 0x100; with 0x40 or 0x80 vertical lines from the top edge, or from the bottom
// with 0x40. The divisions by ah / aw trap when the inner height or width is 0 (a 5 pixel button;
// NOTES.md).
void ui_button::draw_arrow()
{
    int aw = w;
    int ah = h;
    int mx = (aw - 4) / 4 + 2;
    int my = (ah - 4) / 4 + 2;
    int x = get_screen_x(0) + mx;
    int y = get_screen_y(0) + my;
    aw += -1 - 2 * mx;
    ah += -1 - 2 * my;
    bool from_bottom = (flags >> 6) & 1;
    bool from_right = (flags >> 8) & 1;
    int i, n, t, u, d;

    if (!from_bottom && !((flags >> 7) & 1)) {
        if (ah < 0)
            return;
        u = ah * aw * 2;
        t = 0;
        for (i = y, n = ah + 1; n; i++, n--) {
            d = min(t / ah, u / ah);
            if (from_right)
                gr_line((float)(x + aw - d), (float)i, (float)(x + aw), (float)i, gr_line_mode);
            else
                gr_line((float)x, (float)i, (float)(x + d), (float)i, gr_line_mode);
            t += 2 * aw;
            u -= 2 * aw;
        }
    } else {
        if (aw < 0)
            return;
        u = ah * aw * 2;
        t = 0;
        for (i = x, n = aw + 1; n; i++, n--) {
            d = min(t / aw, u / aw);
            if (from_bottom)
                gr_line((float)i, (float)(y + ah - d), (float)i, (float)(y + ah), gr_line_mode);
            else
                gr_line((float)i, (float)y, (float)i, (float)(y + d), gr_line_mode);
            t += 2 * ah;
            u -= 2 * ah;
        }
    }
}

// 0x527690  vtable slot 7. Confidence: exact (the four copies of the label code, each with its own
// locals, are in the binary; they share only the final gr_string call).
// A disabled button is not drawn at all; a hidden one (sic) is drawn with the disabled bitmap and colour
// set 2 (NOTES.md). Otherwise pressed (or being dragged) uses the pressed bitmap and colour set 1,
// highlighted (mouse over or keyboard selection, or a drop target) the highlighted bitmap and set 1, else
// the normal bitmap and set 0. Without a bitmap: a flat face (flag 1 / 4), sunken (pressed or latched,
// the label moves by a pixel) or raised. Colours of a set: [2] face, [3] label, [4] label shadow (when
// its alpha is not 0); the arrow (flags 0x3c0) uses [3].
void ui_button::draw()
{
    int sunken = 0;
    Color *c;

    if (is_disabled())
        return;
    gr::reset_clip();
    if (is_hidden()) {
        c = colors + 10;
        if (state_disabled.draw(get_screen_x(0), get_screen_y(0), w, h, -1, -1)) {
            if (flags & UI_BUTTON_BITMAP_FACE)
                fill_inset(c[2], 0);
        } else if (flags & UI_BUTTON_NO_BEVEL) {
            fill_inset(c[2], 0);
        } else if (is_pressed() || (flags & UI_BUTTON_LATCHED)) {
            draw_sunken(c, 2);
            sunken = 1;
        } else if (flags & UI_BUTTON_FLAT) {
            fill_inset(c[2], 0);
        } else {
            draw_raised(c, 2);
        }
        if (strlen(text)) {
            int tw, th;
            gr_get_string_size(&tw, &th, text, -1, font);
            th = th - gr_get_font_height(font) + gr_get_font_height2(font) + 1;
            int tx = get_screen_x(0) + sunken + (w / 2 - tw / 2);
            int ty = h / 2 - th / 2 + get_screen_y(0) + sunken;
            if (c[4].a) {
                gr_set_color_ptr(&c[4]);
                gr_string(tx + 1, ty + 1, text, font, gr_string_mode);
            }
            gr_set_color_ptr(&c[3]);
            gr_string(tx, ty, text, font, gr_string_mode);
        }
    } else if (!is_pressed() && this != ui_drag_gadget) {
        if (state == 5 || (is_selected() && (is_mouse_over() || !mouse_is_down(0)))) {
            c = colors + 5;
            if (state_highlighted.draw(get_screen_x(0), get_screen_y(0), w, h, -1, -1)) {
                if (flags & UI_BUTTON_BITMAP_FACE)
                    fill_inset(c[2], 0);
            } else if (flags & UI_BUTTON_NO_BEVEL) {
                fill_inset(c[2], 0);
            } else if (flags & UI_BUTTON_LATCHED) {
                draw_sunken(c, 2);
                sunken = 1;
            } else if (flags & UI_BUTTON_FLAT) {
                fill_inset(c[2], 0);
            } else {
                draw_raised(c, 2);
            }
            if (strlen(text)) {
                int tw, th;
                gr_get_string_size(&tw, &th, text, -1, font);
                th = th - gr_get_font_height(font) + gr_get_font_height2(font) + 1;
                int tx = get_screen_x(0) + sunken + (w / 2 - tw / 2);
                int ty = h / 2 - th / 2 + get_screen_y(0) + sunken;
                if (c[4].a) {
                    gr_set_color_ptr(&c[4]);
                    gr_string(tx + 1, ty + 1, text, font, gr_string_mode);
                }
                gr_set_color_ptr(&c[3]);
                gr_string(tx, ty, text, font, gr_string_mode);
            }
        } else {
            c = colors;
            if (state_normal.draw(get_screen_x(0), get_screen_y(0), w, h, -1, -1)) {
                if (flags & UI_BUTTON_BITMAP_FACE)
                    fill_inset(c[2], 0);
            } else if (flags & UI_BUTTON_NO_BEVEL) {
                fill_inset(c[2], 0);
            } else if (flags & UI_BUTTON_LATCHED) {
                draw_sunken(c, 2);
                sunken = 1;
            } else if (flags & UI_BUTTON_FLAT) {
                fill_inset(c[2], 0);
            } else {
                draw_raised(c, 2);
            }
            if (strlen(text)) {
                int tw, th;
                gr_get_string_size(&tw, &th, text, -1, font);
                th = th - gr_get_font_height(font) + gr_get_font_height2(font) + 1;
                int tx = get_screen_x(0) + sunken + (w / 2 - tw / 2);
                int ty = h / 2 - th / 2 + get_screen_y(0) + sunken;
                if (c[4].a) {
                    gr_set_color_ptr(&c[4]);
                    gr_string(tx + 1, ty + 1, text, font, gr_string_mode);
                }
                gr_set_color_ptr(&c[3]);
                gr_string(tx, ty, text, font, gr_string_mode);
            }
        }
    } else {
        c = colors + 5;
        if (state_pressed.draw(get_screen_x(0), get_screen_y(0), w, h, -1, -1)) {
            if (flags & UI_BUTTON_BITMAP_FACE)
                fill_inset(c[2], 0);
        } else if (flags & UI_BUTTON_NO_BEVEL) {
            fill_inset(c[2], 0);
        } else if (is_pressed() || (flags & UI_BUTTON_LATCHED)) {
            draw_sunken(c, 2);
            sunken = 1;
        } else if (flags & UI_BUTTON_FLAT) {
            fill_inset(c[2], 0);
        } else {
            draw_raised(c, 2);
        }
        if (strlen(text)) {
            int tw, th;
            gr_get_string_size(&tw, &th, text, -1, font);
            th = th - gr_get_font_height(font) + gr_get_font_height2(font) + 1;
            int tx = get_screen_x(0) + sunken + (w / 2 - tw / 2);
            int ty = h / 2 - th / 2 + get_screen_y(0) + sunken;
            if (c[4].a) {
                gr_set_color_ptr(&c[4]);
                gr_string(tx + 1, ty + 1, text, font, gr_string_mode);
            }
            gr_set_color_ptr(&c[3]);
            gr_string(tx, ty, text, font, gr_string_mode);
        }
    }
    if (flags & (UI_BUTTON_ARROW_V1 | UI_BUTTON_ARROW_V2 | UI_BUTTON_ARROW_H1 | UI_BUTTON_ARROW_H2)) {
        gr_set_color_ptr(&c[3]);
        draw_arrow();
    }
}

// 0x527df0  vtable slot 8. Confidence: exact.
bool ui_button::is_selectable()
{
    return is_shown() && !is_disabled();
}
