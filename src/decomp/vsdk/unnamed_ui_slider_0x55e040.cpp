// unnamed_ui_slider_0x55e040.cpp - Sum.exe v1.40, code 0x55e040-0x55f10b: the UI bevel drawing, the gadget
// bevel helpers, the slider / scroll bar gadget and the tooltip (vsdk\ui in sum.symbols). The file name is
// a placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp66 NOTES.md): after the ds3d object (0x55e03f), before the gr_poly section code
// (0x55f110, vsdk\gr). No .data, no .bss of its own (the colours, the mouse state and the modes are the UI
// manager's and gr's). Exception handling is on (/GX): the constructor and create (the four `new
// ui_button`) carry cleanup states.

#include <string.h>
#include "vsdk/ui/ui.h"
#include "vsdk/ui/ui_gadgets.h"
#include "vsdk/gr/gr.h"
#include "vsdk/os/mouse.h"

// 0x55e040  Confidence: likely (VC6 keeps each of x1, x1 + 1, y1, y1 + 1, x2, y2, y2 - 1 as its own
// induction variable). width rings of lines, the dark (bottom, right) ones first.
void ui_draw_bevel(int x, int y, int w, int h, Color *light, Color *dark, int width, int inset)
{
    int x1 = x + inset;
    int y1 = y + inset;
    int x2 = x + w - 1 - inset;
    int y2 = y + h - 1 - inset;

    while (width-- != 0) {
        gr_set_color_ptr(dark);
        gr_line((float)(x1 + 1), (float)y2, (float)x2, (float)y2, gr_line_mode);
        gr_line((float)x2, (float)(y1 + 1), (float)x2, (float)(y2 - 1), gr_line_mode);
        gr_set_color_ptr(light);
        gr_line((float)x1, (float)y1, (float)x2, (float)y1, gr_line_mode);
        gr_line((float)x1, (float)(y1 + 1), (float)x1, (float)y2, gr_line_mode);
        x1++;
        y1++;
        x2--;
        y2--;
    }
}

// 0x55e1a0  Confidence: exact. colors[2] face, [0] light, [1] dark.
void ui_draw_raised(int x, int y, int w, int h, Color *colors, int width)
{
    gr_set_color_ptr(&colors[2]);
    gr_rect(x + width, y + width, w - width * 2, h - width * 2, gr_rect_mode, 1.0f);
    ui_draw_bevel(x, y, w, h, &colors[0], &colors[1], width, 0);
}

// 0x55e210  Confidence: exact.
void ui_draw_sunken(int x, int y, int w, int h, Color *colors, int width)
{
    gr_set_color_ptr(&colors[2]);
    gr_rect(x + width, y + width, w - width * 2, h - width * 2, gr_rect_mode, 1.0f);
    ui_draw_bevel(x, y, w, h, &colors[1], &colors[0], width, 0);
}

// 0x55e280  Confidence: exact.
void ui_gadget::fill_inset(Color c, int inset)
{
    gr_set_color_ptr(&c);
    gr_rect(get_screen_x(0) + inset, get_screen_y(0) + inset, w - inset * 2, h - inset * 2, gr_rect_mode, 1.0f);
}

// 0x55e2e0  Confidence: exact.
void ui_gadget::draw_bevel(Color *light, Color *dark, int width, int inset)
{
    ui_draw_bevel(get_screen_x(0), get_screen_y(0), w, h, light, dark, width, inset);
}

// 0x55e320  Confidence: exact.
void ui_gadget::draw_raised(Color *colors, int width)
{
    int sx = get_screen_x(0);
    int sy = get_screen_y(0);

    gr_set_color_ptr(&colors[2]);
    gr_rect(sx + width, sy + width, w - width * 2, h - width * 2, gr_rect_mode, 1.0f);
    ui_draw_bevel(sx, sy, w, h, &colors[0], &colors[1], width, 0);
}

// 0x55e3a0  Confidence: exact.
void ui_gadget::draw_sunken(Color *colors, int width)
{
    int sx = get_screen_x(0);
    int sy = get_screen_y(0);

    gr_set_color_ptr(&colors[2]);
    gr_rect(sx + width, sy + width, w - width * 2, h - width * 2, gr_rect_mode, 1.0f);
    ui_draw_bevel(sx, sy, w, h, &colors[1], &colors[0], width, 0);
}

// 0x55e420  Confidence: exact.
void ui_slider_base::get_marker_rect(int *mx, int *my, int *mw, int *mh)
{
    float f = (float)pos / (count - 1);

    *mh = h;
    *my = 0;
    *mx = 0;
    *mw = w;
    if (horizontal) {
        *mx = (int)((w - marker_size) * f);
        *mw = marker_size;
    } else {
        *my = (int)((h - marker_size) * f);
        *mh = marker_size;
    }
}

// 0x55e4c0  Confidence: likely (the page repeat's flow). A press on the marker drags it, a press before
// or after it pages, held presses repeat every 150 ms after 450 ms; the right button cancels a drag.
int ui_slider_base::process(int key)
{
    bool clicked = false;
    int mx, my, mw, mh;
    int cx, cy;
    int state;

    if (is_disabled())
        return key;
    if (is_hidden()) {
        if (ui_left_pressed && is_mouse_over())
            ui_play_sound(6);
        if (has_hotkey(key))
            ui_play_sound(6);
        return key;
    }
    key = ui_gadget::process(key);
    if (dec_button != NULL && dec_button->was_clicked())
        pos--;
    if (inc_button != NULL && inc_button->was_clicked())
        pos++;
    get_marker_rect(&mx, &my, &mw, &mh);
    cx = ui_mouse_x - get_screen_x(0);
    cy = ui_mouse_y - get_screen_y(0);
    if (ui_left_pressed && is_mouse_over()) {
        repeat_timer.SetTimeout(450);
        clicked = true;
        if (cx < mx) {
            drag_state = UI_SLIDER_PAGE_DOWN;
        } else if (cx < mx + mw && cy >= my && cy < my + mh) {
            drag_state = UI_SLIDER_DRAGGING;
            drag_pos = pos;
            if (horizontal)
                drag_start = cx;
            else
                drag_start = cy;
        } else if (cy < my) {
            drag_state = UI_SLIDER_PAGE_DOWN;
        } else {
            drag_state = UI_SLIDER_PAGE_UP;
        }
    }
    if (mouse_is_down(0) && is_mouse_over() && repeat_timer.elapsed()) {
        repeat_timer.SetTimeout(150);
        clicked = true;
    }
    if (clicked && is_mouse_over()) {
        if (drag_state == UI_SLIDER_PAGE_DOWN)
            pos -= page_size;
        if (drag_state == UI_SLIDER_PAGE_UP)
            pos += page_size;
    }
    state = drag_state;
    if (state == UI_SLIDER_DRAGGING) {
        int d;
        int len;
        int num;

        if (horizontal) {
            d = cx - drag_start;
            len = w;
        } else {
            d = cy - drag_start;
            len = h;
        }
        len -= marker_size;
        if (len != 0) {
            num = (count - 1) * d;
            pos = (num + (num < 0 ? -(len / 2) : len / 2)) / len + drag_pos;
        }
    }
    if (state == UI_SLIDER_DRAGGING && ui_right_pressed) {
        drag_state = UI_SLIDER_IDLE;
        pos = drag_pos;
    }
    if (!mouse_is_down(0))
        drag_state = UI_SLIDER_IDLE;
    return key;
}

// 0x55e780  Confidence: exact (the vtable store after the members).
ui_slider_base::ui_slider_base()
{
    dec_button = NULL;
    inc_button = NULL;
    drag_state = UI_SLIDER_IDLE;
}

// 0x55e7e0  Confidence: likely (the arrow buttons' rectangles; the flag words of ui_button::create are
// 0x102 / 0x202 left / right, 0x42 / 0x82 up / down).
void ui_slider_base::create(ui_element *window, int x, int y, int w, int h, bool horiz, int msize,
                            int positions, int id, int flags, bool owned)
{
    dec_button = NULL;
    inc_button = NULL;
    flags &= 1;
    if (flags) {
        if (horiz) {
            x += h + 1;
            w += (-1 - h) * 2;
        } else {
            y += w + 1;
            h += (-1 - w) * 2;
        }
    }
    ui_gadget::init(window, UI_TYPE_SLIDER, x, y, w, h, id, owned);
    if (flags) {
        if (horiz) {
            dec_button = new ui_button;
            dec_button->create(window, -1 - h, 0, h, h, -1, 0x102, true);
            dec_button->set_parent(this);
            inc_button = new ui_button;
            inc_button->create(window, w + 1, 0, h, h, -1, 0x202, true);
            inc_button->set_parent(this);
        } else {
            dec_button = new ui_button;
            dec_button->create(window, 0, -1 - w, w, w, -1, 0x42, true);
            dec_button->set_parent(this);
            inc_button = new ui_button;
            inc_button->create(window, 0, h + 1, w, w, -1, 0x82, true);
            inc_button->set_parent(this);
        }
    }
    horizontal = horiz;
    marker_size = msize;
    count = positions;
    pos = 0;
    page_size = 1;
    drag_state = UI_SLIDER_IDLE;
    colors = &ui_slider_colors.c0;
}

// 0x55ea50  Confidence: likely (store order; as ui_button::set_tooltip, the first tooltip is unlinked
// and leaked).
void ui_slider_base::set_tooltip(const char *text, int x, int y, int delay)
{
    ui_tooltip *t = tooltips.next;

    if (t != (ui_tooltip *)&tooltips) {
        t->prev->next = t->next;
        t->next->prev = t->prev;
        t->next = NULL;
        t->prev = NULL;
    }
    t = new ui_tooltip;
    strcpy(t->text, text);
    t->owner = this;
    t->x = x;
    t->delay = delay;
    t->next = (ui_tooltip *)&tooltips;
    t->prev = tooltips.prev;
    tooltips.prev = t;
    t->colors = &ui_tooltip_colors.c0;
    t->y = y;
    t->font = -1;
    t->prev->next = t;
}

// 0x55eb10  Confidence: exact.
int ui_slider_base::get_pos()
{
    return pos;
}

// 0x55eb20  Confidence: exact.
int ui_slider_base::get_count()
{
    return count;
}

// 0x55eb30  Confidence: exact.
void ui_slider_base::set_pos(int p)
{
    pos = p;
}

// 0x55eb40  Confidence: exact.
void ui_slider_base::set_count(int c)
{
    count = c;
}

// 0x55eb50  Confidence: exact.
void ui_slider_base::set_marker_size(int size)
{
    marker_size = size;
}

// 0x55eb60  Confidence: exact.
void ui_slider_base::set_page_size(int size)
{
    page_size = size;
}

// 0x55eb70  Confidence: likely (the clip rectangles' arithmetic order). The two parts of the track
// around the marker in colours [3] and [4], then the raised marker.
void ui_slider_base::draw()
{
    int sx, sy;
    int mx, my, mw, mh;
    int cx, cy, cw, ch;

    if (pos >= count)
        pos = count - 1;
    if (pos < 0)
        pos = 0;
    if (is_disabled())
        return;
    gr::reset_clip();
    sx = get_screen_x(0);
    sy = get_screen_y(0);
    get_marker_rect(&mx, &my, &mw, &mh);
    if (!is_hidden()) {
        cw = w;
        ch = h;
        if (horizontal)
            cw = mx;
        else
            ch = my;
        if (cw != 0 && ch != 0) {
            gr::set_clip(sx, sy, cw, ch);
            gr_set_color_ptr(&colors[3]);
            gr_rect(0, 0, w, h, gr_rect_mode, 1.0f);
        }
        ch = h;
        cy = sy;
        cw = w;
        cx = sx;
        if (horizontal) {
            cx = mx + mw + sx;
            cw = w - mw - mx;
        } else {
            cy = my + mh + sy;
            ch = h - mh - my;
        }
        if (cw != 0 && ch != 0) {
            gr::set_clip(cx, cy, cw, ch);
            gr_set_color_ptr(&colors[4]);
            gr_rect(0, 0, w, h, gr_rect_mode, 1.0f);
        }
        gr::set_clip(sx, sy, w, h);
        ui_draw_raised(mx, my, mw, mh, colors, 2);
    }
    ui_gadget::draw();
}

// 0x55ed30  Confidence: likely. A 20-position bar along the list box's bottom or right edge, outside it.
void ui_slider::create_scrollbar(ui_gadget *listbox, bool horiz, int msize, int id, int flags, bool owned)
{
    ui_listbox *lb = (ui_listbox *)listbox;
    ui_element *window = listbox->get_window();
    int x, y, w, h;
    int view;

    if (horiz) {
        x = 0;
        h = msize;
        listbox->get_size(&w, &y);
        y++;
    } else {
        y = 0;
        w = msize;
        listbox->get_size(&x, &h);
        x++;
    }
    create(window, x, y, w, h, horiz, msize, 20, id, flags, owned);
    set_parent(listbox);
    if (horiz) {
        lb->hscrollbar = this;
        visible = view = lb->get_view_width();
        count = lb->get_content_width() + (1 - view);
    } else {
        lb->vscrollbar = this;
        visible = view = lb->get_view_height();
        count = lb->get_content_height() + (1 - view);
    }
    set_page_size(view);
    update_marker();
}

// 0x55ee50  Confidence: exact.
int ui_slider::get_range()
{
    int r = visible + count - 1;

    if (r < 1)
        r = 1;
    return r;
}

// 0x55ee70  Confidence: exact.
void ui_slider::set_visible_count(int n)
{
    count += visible - n;
    visible = n;
    set_page_size(n);
    update_marker();
}

// 0x55eeb0  Confidence: exact.
void ui_slider::set_range(int content)
{
    count = content - visible + 1;
    update_marker();
}

// 0x55eed0  Confidence: exact.
void ui_slider::update_marker()
{
    float f = (float)visible / (count + visible - 1);
    int size;

    if (f < 0.0f)
        f = 0.0f;
    else if (f > 1.0f)
        f = 1.0f;
    size = (int)((horizontal ? w : h) * f);
    if (size < 7)
        size = 7;
    set_marker_size(size);
}

// 0x55ef50  Confidence: exact.
ui_tooltip::~ui_tooltip()
{
}

// 0x55ef60  Confidence: likely (an owner of another kind leaves the position unset). The box is kept
// two pixels inside the screen; colours [0] box, [2] shadow (when its alpha is not 0), [1] text.
void ui_tooltip::draw()
{
    int tw, th;
    int px, py;
    int kind;

    gr_get_string_size(&tw, &th, text, -1, -1);
    kind = owner->get_kind();
    if (kind == 0) {
        px = ((ui_gadget *)owner)->get_screen_x(0) + x;
        py = ((ui_gadget *)owner)->get_screen_y(0) + y;
    } else if (kind == 1) {
        owner->get_position(&px, &py);
        px += x;
        py += y;
    }
    if (px < 2)
        px = 2;
    if (py < 2)
        py = 2;
    if (px + tw > gr_screen_width() - 2)
        px = gr_screen_width() + (-5 - tw);
    if (py + th > gr_screen_height() - 2)
        py = gr_screen_height() + (-5 - th);
    gr_set_color_ptr(&colors[0]);
    gr_rect(px, py, tw + 3, th + 3, gr_rect_mode, 1.0f);
    if (colors[2].a != 0) {
        gr_set_color_ptr(&colors[2]);
        gr_string(px + 2, py + 2, text, -1, gr_string_mode);
    }
    gr_set_color_ptr(&colors[1]);
    gr_string(px + 1, py + 1, text, -1, gr_string_mode);
}
