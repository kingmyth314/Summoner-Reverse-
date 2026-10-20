// unnamed_ui_static_0x527e20.cpp - Sum.exe v1.40, code 0x527e20-0x528429 (vsdk\ui; the unit's file name
// is not in the binary, so the name is a placeholder). Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2. Confidence per function: exact = expect
// identical code, likely = same semantics, small layout or scheduling risk, unsure = the source shape is
// a guess.
//
// The static label gadget (ui_static, vtable 0x581dac): a text with justification or a fixed text
// position over a state bitmap or a filled rectangle.
//
// Unit bounds (NOTES.md): after ui_button::is_selectable (0x527df0; int3 padding separates the two
// objects' EH funclets, 0x57d3c8-0x57d3cf) and before window.cpp (0x528430, tagged vsdk\ui\window.cpp).
// No .data or .bss of its own. Whether ui_static is its own object or the head of window.cpp cannot be
// told from the funclets (ui_static's block ends at 0x57d3ef and window.cpp's starts at the 16-byte
// boundary right after it, so no padding is needed); it is kept apart because window.cpp's tagged
// functions start at 0x528430.

#include <string.h>
#include "vsdk/ui/ui_gadgets.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/gr_font.h"

// 0x527e20  vtable slot 0. Confidence: exact.
void ui_static::dtor()
{
    state_normal.clear();
    state_disabled.clear();
    ui_gadget::dtor();
}

// 0x527e50  vtable slot 5. Confidence: exact. Event 0x12 on a left press over the label.
int ui_static::process(int key)
{
    if (!is_disabled() && !is_hidden() && ui_left_pressed && is_mouse_over())
        send_event(0x12, UI_USE_ID);
    return key;
}

// 0x527ea0  Confidence: exact.
ui_static::ui_static()
{
    text[0] = '\0';
}

// 0x527f10  Confidence: exact (VC6 byte match, match11; was likely) (store order). The bitmap window is the whole gadget at offset 0, 0;
// the text is left aligned, vertically centred.
void ui_static::create(ui_element *window, int x, int y, int w, int h, int id, bool owned)
{
    init(window, UI_TYPE_STATIC, x, y, w, h, id, owned);
    bm_h = h;
    text[0] = '\0';
    font = -1;
    bm_w = w;
    bm_u = 0;
    bm_v = 0;
    h_justify = 0;
    v_justify = 1;
    text_x = -1;
    text_y = -1;
    colors = ui_static_colors;
}

// 0x527fa0  Confidence: exact. The same code as ui_button::set_tooltip (the first tooltip is unlinked
// and leaked).
void ui_static::set_tooltip(const char *text, int x, int y, int delay)
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
    t->owner = this;
    strcpy(t->text, text);
    t->delay = delay;
    t->x = x;
    ui_tooltip *last = tooltips.prev;
    t->next = (ui_tooltip *)&tooltips;
    tooltips.prev = t;
    t->colors = ui_tooltip_colors;
    t->y = y;
    t->font = -1;
    t->prev = last;
    last->next = t;
}

// 0x528060  Confidence: exact.
void ui_static::set_text(const char *text)
{
    strcpy(this->text, text);
}

// 0x528090  Confidence: exact. No caller.
char *ui_static::get_text()
{
    return text;
}

// 0x5280a0  Confidence: exact. No caller (sum.symbols name; D4107).
void ui_static::set_justification(int w, int h)
{
    bm_w = w;
    bm_h = h;
}

// 0x5280c0  Confidence: exact. No caller (sum.symbols name; D4107).
void ui_static::set_text_offset(int u, int v)
{
    if (u >= 0 && v >= 0) {
        bm_u = u;
        bm_v = v;
    }
}

// 0x5280e0  vtable slot 7. Confidence: exact (VC6 8966 byte match: each branch has its own copy of the
// whole text code, shadow and string included, and sets c itself; the compiler merges the two copies from
// the shadow call on, D13801). Disabled: nothing. Hidden (sic, as ui_button::draw): the
// disabled bitmap and colour set 1 ([3..5]). Without a bitmap the rectangle is filled with [0]; the text
// is drawn with [1], its shadow with [2] when that alpha is not 0. Then the children (ui_gadget::draw).
void ui_static::draw()
{
    if (is_disabled())
        return;
    int sx = get_screen_x(0);
    int sy = get_screen_y(0);
    Color *c;
    int tx, ty;

    if (is_hidden()) {
        c = colors + 3;
        if (!state_disabled.draw(sx, sy, bm_w, bm_h, bm_u, bm_v)) {
            gr_set_color_ptr(c);
            gr_rect(sx, sy, w, h, gr_rect_mode, 1.0f);
        }
        if (strlen(text) == 0)
            goto done;
        int tw, th;
        gr_get_string_size(&tw, &th, text, -1, font);
        th = th - gr_get_font_height(font) + gr_get_font_height2(font) + 1;
        if (text_x >= 0) {
            tx = text_x + sx;
        } else {
            switch (h_justify) {
            case 1:
                tx = w / 2 - tw / 2 + sx;
                break;
            case 2:
                tx = w - tw + sx;
                break;
            default:
                tx = sx;
                break;
            }
        }
        if (text_y >= 0) {
            ty = text_y + sy;
        } else {
            switch (v_justify) {
            case 0:
                ty = sy;
                break;
            case 2:
                ty = h - th + sy;
                break;
            default:
                ty = h / 2 - th / 2 + sy;
                break;
            }
        }
        if (c[2].a) {
            gr_set_color_ptr(&c[2]);
            gr_string(tx + 1, ty + 1, text, font, gr_string_mode);
        }
        gr_set_color_ptr(&c[1]);
        gr_string(tx, ty, text, font, gr_string_mode);
    } else {
        c = colors;
        if (!state_normal.draw(sx, sy, bm_w, bm_h, bm_u, bm_v)) {
            gr_set_color_ptr(c);
            gr_rect(sx, sy, w, h, gr_rect_mode, 1.0f);
        }
        if (strlen(text) == 0)
            goto done;
        int tw, th;
        gr_get_string_size(&tw, &th, text, -1, font);
        th = th - gr_get_font_height(font) + gr_get_font_height2(font) + 1;
        if (text_x >= 0) {
            tx = text_x + sx;
        } else {
            switch (h_justify) {
            case 1:
                tx = w / 2 - tw / 2 + sx;
                break;
            case 2:
                tx = w - tw + sx;
                break;
            default:
                tx = sx;
                break;
            }
        }
        if (text_y >= 0) {
            ty = text_y + sy;
        } else {
            switch (v_justify) {
            case 0:
                ty = sy;
                break;
            case 2:
                ty = h - th + sy;
                break;
            default:
                ty = h / 2 - th / 2 + sy;
                break;
            }
        }
        if (c[2].a) {
            gr_set_color_ptr(&c[2]);
            gr_string(tx + 1, ty + 1, text, font, gr_string_mode);
        }
        gr_set_color_ptr(&c[1]);
        gr_string(tx, ty, text, font, gr_string_mode);
    }
done:
    gr::reset_clip();
    ui_gadget::draw();
}
