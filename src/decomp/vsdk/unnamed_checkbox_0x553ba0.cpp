// unnamed_checkbox_0x553ba0.cpp - Sum.exe v1.40, code 0x553ba0-0x5549b9: the ui_checkbox and
// ui_radio_button gadgets (vsdk\ui in sum.symbols; vtables 0x581f68 / 0x581c7c). The file name is a
// placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout or
// scheduling risk, unsure = the source shape is a guess.
//
// Unit range: after ui_container.cpp's last function (0x553b9b) and before targautils.cpp (0x5549c0).
// No .data or .bss of its own (ui_checkbox_colors 0x2cbf630 is the UI manager's). The radio button's
// implicit constructor is a COMDAT of the script editor (0x4deeb0). Only the script editor uses these
// gadgets.

#include <string.h>
#include "vsdk/ui/ui_gadgets.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/gr_font.h"
#include "vsdk/os/key.h"
#include "vsdk/os/mouse.h"

// 0x553ba0  vtable slot 5. Confidence: exact. state is recomputed each frame (the previous one decides
// the release click). A hidden box only plays sound 3 on a click or its hotkey. The hotkey, or Enter
// while it has the keyboard selection, toggles it and swallows the key; a release over it after a press
// toggles it too. Event 0x13 (name ours: toggled) each time.
int ui_checkbox::process(int key)
{
    int prev_state = state;
    state = 0;

    if (is_disabled())
        return key;
    if (is_hidden()) {
        bool pressed = ui_left_pressed;
        state = 3;
        if (pressed && is_mouse_over())
            ui_play_sound(3);
        if (has_hotkey(key))
            ui_play_sound(3);
        return key;
    }
    if (has_hotkey(key) || (is_selected() && key == KEY_ENTER)) {
        toggle();
        send_event(0x13, UI_USE_ID);
        key = 0;
    }
    if (is_mouse_over() && is_selected()) {
        if (mouse_is_down(0)) {
            set_focus();
            state = 2;
        } else {
            state = 1;
        }
    }
    if (ui_left_released && prev_state == 2 && is_mouse_over()) {
        toggle();
        send_event(0x13, UI_USE_ID);
    }
    return key;
}

// 0x553cd0  vtable slot 0 (sum.symbols ui_checkbox__dtor). Confidence: exact (jmp ui_gadget::dtor).
void ui_checkbox::dtor()
{
    ui_gadget::dtor();
}

// 0x553ce0  Confidence: exact.
ui_checkbox::ui_checkbox()
{
    state = 0;
    label[0] = '\0';
}

// 0x553d00  Confidence: exact (VC6 byte match, m-1020: the width in a local computed before the height; 8966
// then keeps label / font in EDI / EBP and puts tw / th in their dead argument slots). The gadget is as wide as the box, the label offset and
// the label, as high as the higher of the box and the label. The label is measured with
// gr_get_string_size(.., label, font, -1): the font goes in as the length and -1 as the font, harmless
// for the default font -1 (see NOTES.md).
void ui_checkbox::create(ui_element *window, int x, int y, int box_w, int box_h, const char *label,
                         int label_x, int label_y, int font, bool checked, int id, bool owned)
{
    int tw, th;

    gr_get_string_size(&tw, &th, label, font, -1);
    int w = box_w + tw + label_x;
    int h = th + label_y;
    if (box_h > h)
        h = box_h;
    init(window, UI_TYPE_CHECKBOX, x, y, w, h, id, owned);
    this->checked = checked;
    this->box_w = box_w;
    this->box_h = box_h;
    strcpy(this->label, label);
    this->font = font;
    this->label_x = label_x;
    this->label_y = label_y;
    bm_unchecked = -1;
    bm_checked = -1;
    bm_pressed = -1;
    bm_disabled = -1;
    colors = &ui_checkbox_colors.g0.c0;
}

// 0x553e00  Confidence: exact (as ui_inputbox::set_tooltip, the same code).
void ui_checkbox::set_tooltip(const char *text, int x, int y, int delay)
{
    ui_tooltip *t = tooltips.next;

    if (t != &tooltips) {
        ui_tooltip *prev = t->prev;
        ui_tooltip *next = t->next;
        t->next = NULL;
        t->prev = NULL;
        prev->next = next;
        next->prev = prev;
    }
    t = new ui_tooltip;
    t->owner = this;
    strcpy(t->text, text);
    t->delay = delay;
    t->x = x;
    ui_tooltip *last = tooltips.prev;
    t->next = &tooltips;
    tooltips.prev = t;
    t->colors = &ui_tooltip_colors.c0;
    t->y = y;
    t->font = -1;
    t->prev = last;
    last->next = t;
}

// 0x553ec0  Confidence: exact.
void ui_checkbox::toggle()
{
    checked = !checked;
}

// 0x553ee0  Confidence: exact.
void ui_checkbox::set_checked(bool checked)
{
    this->checked = checked;
}

// 0x553ef0  Confidence: exact.
bool ui_checkbox::is_checked()
{
    return checked;
}

// A line in gr_line_mode, for ui_checkbox::draw's cross (D24800; ours: the helper's call boundary is what
// gives 8966 the binary's order of the int-to-float temporaries and the far-corner lea).
static inline void cb_line(float x0, float y0, float x1, float y1)
{
    gr_line(x0, y0, x1, y1, gr_line_mode);
}

// 0x553f00  vtable slot 7. Confidence: exact (D24800: the cross's lines through cb_line). The box is a bitmap when
// one is set for the state, else a filled rectangle with a cross (two lines) when checked; the label
// right of it, with a shadow one pixel down-right when the shadow colour is not transparent.
void ui_checkbox::draw()
{
    int sx = get_screen_x(0);
    int sy = get_screen_y(0);

    if (is_disabled())
        return;
    gr::reset_clip();

    if (is_hidden()) {
        if (bm_disabled >= 0) {
            gr_set_color(255, 255, 255, 255);
            gr_bitmap(bm_disabled, get_screen_x(0), get_screen_y(0), gr_bitmap_wrap_mode);
        } else {
            gr_set_color_ptr(&colors[12]);
            gr_rect(sx, sy, box_w, box_h, gr_rect_mode, 1.0f);
        }
        if (colors[15].a) {
            gr_set_color_ptr(&colors[15]);
            gr_string(sx + box_w + label_x + 1, sy + label_y + 1, label, font, gr_string_mode);
        }
        gr_set_color_ptr(&colors[14]);
        gr_string(sx + box_w + label_x, sy + label_y, label, font, gr_string_mode);
        return;
    }

    if (state == 1) {
        if (checked) {
            if (bm_checked >= 0) {
                gr_set_color(255, 255, 255, 255);
                gr_bitmap(bm_checked, get_screen_x(0), get_screen_y(0), gr_bitmap_wrap_mode);
            } else {
                gr_set_color_ptr(&colors[0]);
                gr_rect(sx, sy, box_w, box_h, gr_rect_mode, 1.0f);
                gr_set_color_ptr(&colors[1]);
                cb_line((float)sx, (float)sy, (float)(sx + box_w - 1), (float)(sy + box_h - 1));
                cb_line((float)(sx + box_w - 1), (float)sy, (float)sx, (float)(sy + box_h - 1));
            }
        } else if (bm_unchecked >= 0) {
            gr_set_color(255, 255, 255, 255);
            gr_bitmap(bm_unchecked, get_screen_x(0), get_screen_y(0), gr_bitmap_wrap_mode);
        } else {
            gr_set_color_ptr(&colors[0]);
            gr_rect(sx, sy, box_w, box_h, gr_rect_mode, 1.0f);
        }
        if (colors[7].a) {
            gr_set_color_ptr(&colors[7]);
            gr_string(sx + box_w + label_x + 1, sy + label_y + 1, label, font, gr_string_mode);
        }
        gr_set_color_ptr(&colors[6]);
        gr_string(sx + box_w + label_x, sy + label_y, label, font, gr_string_mode);
    } else if (state == 2) {
        if (bm_pressed >= 0) {
            gr_set_color(255, 255, 255, 255);
            gr_bitmap(bm_pressed, get_screen_x(0), get_screen_y(0), gr_bitmap_wrap_mode);
        } else {
            gr_set_color_ptr(&colors[8]);
            gr_rect(sx, sy, box_w, box_h, gr_rect_mode, 1.0f);
            if (checked) {
                gr_set_color_ptr(&colors[9]);
                cb_line((float)sx, (float)sy, (float)(sx + box_w - 1), (float)(sy + box_h - 1));
                cb_line((float)(sx + box_w - 1), (float)sy, (float)sx, (float)(sy + box_h - 1));
            }
        }
        if (colors[11].a) {
            gr_set_color_ptr(&colors[11]);
            gr_string(sx + box_w + label_x + 1, sy + label_y + 1, label, font, gr_string_mode);
        }
        gr_set_color_ptr(&colors[10]);
        gr_string(sx + box_w + label_x, sy + label_y, label, font, gr_string_mode);
    } else {
        if (checked) {
            if (bm_checked >= 0) {
                gr_set_color(255, 255, 255, 255);
                gr_bitmap(bm_checked, get_screen_x(0), get_screen_y(0), gr_bitmap_wrap_mode);
            } else {
                gr_set_color_ptr(&colors[0]);
                gr_rect(sx, sy, box_w, box_h, gr_rect_mode, 1.0f);
                gr_set_color_ptr(&colors[1]);
                cb_line((float)sx, (float)sy, (float)(sx + box_w - 1), (float)(sy + box_h - 1));
                cb_line((float)(sx + box_w - 1), (float)sy, (float)sx, (float)(sy + box_h - 1));
            }
        } else if (bm_unchecked >= 0) {
            gr_set_color(255, 255, 255, 255);
            gr_bitmap(bm_unchecked, get_screen_x(0), get_screen_y(0), gr_bitmap_wrap_mode);
        } else {
            gr_set_color_ptr(&colors[0]);
            gr_rect(sx, sy, box_w, box_h, gr_rect_mode, 1.0f);
        }
        if (colors[3].a) {
            gr_set_color_ptr(&colors[3]);
            gr_string(sx + box_w + label_x + 1, sy + label_y + 1, label, font, gr_string_mode);
        }
        gr_set_color_ptr(&colors[2]);
        gr_string(sx + box_w + label_x, sy + label_y, label, font, gr_string_mode);
    }
}

// 0x5546e0  vtable slot 8. Confidence: exact.
bool ui_checkbox::is_selectable()
{
    return is_shown() && !is_disabled();
}

// 0x554710  vtable slot 5. Confidence: exact. As ui_checkbox::process with sound 2, a pressed state only
// while unchecked, and the group behaviour: checking one clears the parent's other children. The
// hotkey / Enter path clears every child (set_checked writes byte +0x274 of whatever gadget it is, see
// NOTES.md) and sends no event; the click path clears only radio buttons and sends 0x13.
int ui_radio_button::process(int key)
{
    int prev_state = state;
    ui_gadget *g;
    state = 0;

    if (is_disabled())
        return key;
    if (is_hidden()) {
        bool pressed = ui_left_pressed;
        state = 3;
        if (pressed && is_mouse_over())
            ui_play_sound(2);
        if (has_hotkey(key))
            ui_play_sound(2);
        return key;
    }
    if (has_hotkey(key) || (is_selected() && key == KEY_ENTER)) {
        for (g = parent->get_first_child(); g; g = g->get_next_sibling())
            ((ui_checkbox *)g)->set_checked(false);
        toggle();
        key = 0;
    }
    if (is_mouse_over() && is_selected()) {
        if (!is_checked() && mouse_is_down(0)) {
            set_focus();
            state = 2;
        } else {
            state = 1;
        }
    }
    if (ui_left_released && prev_state == 2 && is_mouse_over()) {
        for (g = parent->get_first_child(); g; g = g->get_next_sibling()) {
            if (g->get_type() == UI_TYPE_RADIO)
                ((ui_checkbox *)g)->set_checked(false);
        }
        toggle();
        send_event(0x13, UI_USE_ID);
    }
    return key;
}

// 0x5548a0  Confidence: exact (m-1020c: the width in a local first, the height as an if / else of the two
// values, font / label_x / label_y stored in ui_checkbox::create's order; D14701). In the parent's window, checked when it is the
// parent's first child; the same measuring slip as ui_checkbox::create.
void ui_radio_button::create(ui_gadget *parent, int x, int y, int box_w, int box_h, const char *label,
                             int label_x, int label_y, int font, int id, bool owned)
{
    int tw, th;

    gr_get_string_size(&tw, &th, label, font, -1);
    int w = tw + box_w + label_x;
    int h;
    if (box_h > label_y + th)
        h = box_h;
    else
        h = label_y + th;
    init(parent->get_window(), UI_TYPE_RADIO, x, y, w, h, id, owned);
    if (parent->get_first_child())
        checked = false;
    else
        checked = true;
    this->box_w = box_w;
    this->box_h = box_h;
    strcpy(this->label, label);
    this->font = font;
    this->label_x = label_x;
    this->label_y = label_y;
    bm_unchecked = -1;
    bm_checked = -1;
    bm_pressed = -1;
    bm_disabled = -1;
    colors = &ui_checkbox_colors.g0.c0;
    set_parent(parent);
}
