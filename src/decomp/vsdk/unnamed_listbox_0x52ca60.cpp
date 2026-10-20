// unnamed_listbox_0x52ca60.cpp - Sum.exe v1.40, code 0x52ca60-0x52f444 (vsdk\ui; the unit's file name is
// not in the binary, so the name is a placeholder). Written from the Sum.exe disassembly (tools/asm.mjs).
// Target: VC++ 6.0 (cl 12.00.8966), /O2. Confidence per function: exact = expect identical code,
// likely = same semantics, small layout or scheduling risk, unsure = the source shape is a guess.
//
// The list box gadget (ui_listbox, vtable 0x581e34) and its entries (ui_listbox_entry 0x581e30, the text
// entry ui_listbox_text_entry 0x581e2c): word wrapping, scrolling by pixels / lines / entries with the
// view aligned to the top or the bottom, keyboard and type-ahead selection, drawing.
//
// Unit bounds (NOTES.md): the .bss block 0x2cbf584-0x2cbf623 (the two line tables) follows window.cpp's
// literals (..0x2cbf583) and precedes the UI manager's (0x2cbf628); .data 0x5a95e4-0x5a9601 (one literal)
// follows window.cpp's (..0x5a95e3); the EH funclets of its three EH functions form one int3-padded block
// 0x57d510-0x57d556 after window.cpp's; the object ends with its compiler-generated COMDATs (the two
// scalar deleting destructors and the text entry's implicit destructor, 0x52f400-0x52f444). The start
// is not proven: the gadget base before it has no data and no EH.

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "vsdk/ui/ui_gadgets.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/gr_font.h"
#include "vsdk/os/key.h"
#include "vsdk/os/plainstring.h"

// ---- .bss (0x2cbf584-0x2cbf623) -------------------------------------------------------------------
char *ui_listbox_line_starts[20];   // 0x2cbf584
int ui_listbox_line_lengths[20];    // 0x2cbf5d4

// 0x52ca60  Confidence: likely (the line width store `lw += indent`, register use of the callback's
// first argument). Lays out the text in lines; the first line is indented by indent, the others by
// wrap_indent; width_cb, when set and non-zero, gives the width of the line at y. F6202: more than 20
// lines overrun the line tables.
int ui_listbox_entry::wrap_text(int width, int y, int (*width_cb)(int y, int line_h))
{
    int line_h = gr_get_font_height(font) + line_spacing;
    char *s = text;
    int cur_indent;
    int max_w;

    num_lines = 0;
    this->width = 0;
    cur_indent = indent;
    for (;;) {
        int split;
        int lw, lh;

        max_w = 0;
        if (width_cb)
            max_w = width_cb(y, line_h);
        if (max_w == 0)
            max_w = width;

        char *next = gr_font_fit_string(&split, s, max_w - cur_indent, font);
        ui_listbox_line_starts[num_lines] = s;
        if (split >= 0)
            ui_listbox_line_lengths[num_lines] = split;
        else
            ui_listbox_line_lengths[num_lines] = strlen(s);
        num_lines++;

        gr_get_string_size(&lw, &lh, s, split, font);
        lw += cur_indent;
        if (lw > this->width)
            this->width = lw;
        if (!next)
            break;
        cur_indent = wrap_indent;
        y += line_h;
        s = next;
    }
    height = num_lines * line_h - line_spacing;
    return num_lines;
}

// 0x52cb70  Confidence: exact.
int ui_listbox_entry::line_top(int line)
{
    return (gr_get_font_height(font) + line_spacing) * line + y;
}

// 0x52cba0  Confidence: exact (operand order of the final adds).
int ui_listbox_entry::line_bottom(int line)
{
    int fh = gr_get_font_height(font);
    return (line_spacing + fh) * line + y + fh;
}

// 0x52cbd0  Confidence: exact (operand order). The line under y, counting the spacing above a line
// as part of it.
int ui_listbox_entry::line_at_y_with_spacing(int y)
{
    int line = (line_spacing - this->y + y) / (gr_get_font_height(font) + line_spacing);
    if (line < 0)
        return 0;
    if (line >= num_lines)
        line = num_lines - 1;
    return line;
}

// 0x52cc20  Confidence: exact.
int ui_listbox_entry::line_at_y(int y)
{
    int line = (y - this->y) / (gr_get_font_height(font) + line_spacing);
    if (line < 0)
        return 0;
    if (line >= num_lines)
        line = num_lines - 1;
    return line;
}

// 0x52cc60  Confidence: exact (shape of the width choice). Wrapping lists wrap to max_content_w, or to
// the view width without one; the others never wrap.
int ui_listbox::layout_entry(ui_listbox_entry *e, int y)
{
    int width = 999999;
    if (flags & UI_LISTBOX_WRAP) {
        width = max_content_w;
        if (width == 0)
            width = get_view_width();
    }
    return e->wrap_text(width, y, wrap_width_cb);
}

// 0x52cca0  Confidence: exact.
bool ui_listbox::is_entry_visible(ui_listbox_entry *e)
{
    if (vscroll >= e->height + e->y)
        return false;
    if (get_view_height() + vscroll > e->y)
        return true;
    return false;
}

// 0x52cce0  Confidence: exact.
bool ui_listbox::is_entry_fully_visible(ui_listbox_entry *e)
{
    if (vscroll > e->y)
        return false;
    if (get_view_height() + vscroll >= e->height + e->y)
        return true;
    return false;
}

// 0x52cd20  Confidence: exact. No caller.
bool ui_listbox::is_entry_bottom_visible(ui_listbox_entry *e)
{
    if (get_view_height() + vscroll >= e->height + e->y)
        return true;
    return false;
}

// 0x52cd50  Confidence: exact.
void ui_listbox::update_vscrollbar()
{
    if (vscrollbar)
        vscrollbar->set_range(get_content_height());
}

// 0x52cd70  Confidence: exact.
void ui_listbox::update_hscrollbar()
{
    if (hscrollbar)
        hscrollbar->set_range(get_content_width());
}

// 0x52cd90  Confidence: exact (VC6 byte match, match29; was likely) (a byte match under /Ox /Ow, D7502). F6203: an empty list
// ends with content_height = -entry_spacing.
void ui_listbox::recalc_content_size()
{
    content_height = 0;
    content_width = 0;
    for (ui_listbox_entry *e = entries.next; e != &entries; e = e->next) {
        if (e->width > content_width)
            content_width = e->width;
        e->y = content_height;
        content_height = e->height + entry_spacing + content_height;
    }
    if (max_content_w && content_width > max_content_w)
        content_width = max_content_w;
    content_height -= entry_spacing;
}

// 0x52ce20  Confidence: exact. The entry under a content point; with clamp, a point above the first
// entry or in the spacing between entries gives the entry below it.
ui_listbox_entry *ui_listbox::entry_at(int x, int y, bool clamp)
{
    ui_listbox_entry *e;

    if (entries.next == &entries || x < 0 || x >= content_width || y >= content_height)
        return NULL;
    if (y < 0)
        return clamp ? entries.next : NULL;
    for (e = entries.next; e != &entries; e = e->next) {
        if ((y >= e->y || clamp) && y < e->height + e->y)
            return e;
    }
    return NULL;
}

// 0x52ce90  Confidence: exact. The entry before the first one starting below y (the head's prev, the
// last entry, when none does).
ui_listbox_entry *ui_listbox::entry_above(int y)
{
    ui_listbox_entry *e;

    if (entries.next == &entries)
        return NULL;
    if (y < 0)
        return NULL;
    for (e = entries.next; e != &entries; e = e->next) {
        if (e->y > y)
            break;
    }
    return e->prev;
}

// 0x52ced0  Confidence: exact. The first entry whose bottom is below y.
ui_listbox_entry *ui_listbox::entry_at_y(int y)
{
    ui_listbox_entry *e;

    if (entries.next != &entries && y < content_height) {
        for (e = entries.next; e != &entries; e = e->next) {
            if (e->height + e->y > y)
                return e;
        }
    }
    return NULL;
}

// 0x52cf10  Confidence: exact. F6201: dirty is never cleared, so a dirty entry is laid out again on every
// process and draw (and the sizes recomputed).
void ui_listbox::relayout_dirty()
{
    bool changed = false;
    int y = 0;

    for (ui_listbox_entry *e = entries.next; e != &entries; e = e->next) {
        if (e->dirty) {
            layout_entry(e, y - vscroll);
            changed = true;
        }
        y += e->height;
    }
    if (changed) {
        recalc_content_size();
        update_vscrollbar();
        update_hscrollbar();
    }
}

// 0x52cf70  Confidence: exact. The scroll bars' arrow buttons and markers into the scroll positions.
void ui_listbox::sync_scrollbars()
{
    if (hscrollbar) {
        if (hscrollbar->inc_button && hscrollbar->inc_button->was_clicked()) {
            scroll_right();
            return;
        }
        if (hscrollbar->dec_button && hscrollbar->dec_button->was_clicked()) {
            scroll_left();
            return;
        }
        if (hscroll != hscrollbar->get_pos()) {
            set_hscroll(hscrollbar->get_pos());
            hscrollbar->set_pos(hscroll);
        }
    }
    if (vscrollbar) {
        if (vscrollbar->inc_button && vscrollbar->inc_button->was_clicked()) {
            scroll_line_down();
            return;
        }
        if (vscrollbar->dec_button && vscrollbar->dec_button->was_clicked()) {
            scroll_line_up();
            return;
        }
        if (vscroll != vscrollbar->get_pos()) {
            set_vscroll(vscrollbar->get_pos(), false);
            vscrollbar->set_pos(vscroll);
        }
    }
}

// 0x52d080  vtable slot 5. Confidence: likely (the `key = 0; return key;` of the arrow cases stores
// into the argument as the code does; the scroll bar visibility tests are written as the branches
// fall). Mouse selection (event 0x18, a second click on the selection 0x19), the arrow and page keys,
// the wheel, and type-ahead: typed characters select the first entry that starts with them
// (case-insensitively), forgotten after 1.5 s. A hidden list box plays sound 7 on a click or its hotkey.
// F6205: the clicked y is not corrected for v_margin.
int ui_listbox::process(int key)
{
    if (is_disabled())
        return key;

    relayout_dirty();
    if (vscrollbar) {
        if (content_height <= get_view_height())
            vscrollbar->set_hidden(true);
        else
            vscrollbar->set_visible(true);
    }
    if (hscrollbar) {
        if (content_width <= get_view_width())
            hscrollbar->set_hidden(true);
        else
            hscrollbar->set_visible(true);
    }

    if (is_hidden()) {
        if (ui_left_pressed && is_mouse_over())
            ui_play_sound(7);
        if (has_hotkey(key))
            ui_play_sound(7);
        return key;
    }

    key = ui_gadget::process(key);
    sync_scrollbars();

    if (is_mouse_over() && ui_left_pressed) {
        if (!(flags & UI_LISTBOX_NO_SELECT)) {
            int mx = ui_mouse_x - get_screen_x(0);
            int my = ui_mouse_y - get_screen_y(0);
            if (mx >= margin && mx < w - margin && my >= v_margin && my < h - v_margin) {
                ui_listbox_entry *e = entry_at(0, vscroll + my, false);
                if (e) {
                    if (selected != e) {
                        selected = e;
                        send_event(UI_EVENT_LIST_SELECT, UI_USE_ID);
                    } else if (ui_left_double_clicked) {
                        send_event(UI_EVENT_LIST_DBLCLICK, UI_USE_ID);
                    }
                    user_selected = true;
                }
            }
        }
        set_focus();
    }

    if (typeahead_len > 0 && typeahead_timer.GetElapsedTime() > 1500)
        typeahead_len = 0;

    if (this == ui_current_gadget) {
        if (ui_mouse_wheel > 0)
            scroll_line_up();
        else if (ui_mouse_wheel < 0)
            scroll_line_down();

        if (key) {
            switch (key) {
            case KEY_UP:
                select_prev();
                key = 0;
                return key;
            case KEY_DOWN:
                select_next();
                key = 0;
                return key;
            case KEY_LEFT:
                scroll_left();
                key = 0;
                return key;
            case KEY_RIGHT:
                scroll_right();
                key = 0;
                return key;
            case KEY_PAGEUP:
                select_page_up();
                key = 0;
                return key;
            case KEY_PAGEDOWN:
                select_page_down();
                key = 0;
                return key;
            }

            if (key == KEY_BACKSPACE || key == KEY_ENTER) {
                typeahead_len = 0;
            } else if (typeahead_len < 0x20) {
                key &= ~KEY_SHIFTED;
                int c = key_to_ascii(key);
                if (c != 0xff) {
                    typeahead[typeahead_len] = (char)c;
                    typeahead_len++;
                    typeahead_timer.SetTimeout(0);
                }
            }

            if (typeahead_len) {
                for (ui_listbox_entry *e = entries.next; e != &entries; e = e->next) {
                    char *t = e->text;
                    int i;
                    for (i = 0; i < typeahead_len; i++) {
                        if (tolower(t[i]) != typeahead[i])
                            break;
                    }
                    if (i == typeahead_len) {
                        select_entry(e, true);
                        send_event(UI_EVENT_LIST_SELECT, UI_USE_ID);
                        break;
                    }
                }
            }
        }
    }
    return key;
}

// 0x52d420  vtable slot 0 (ui_listbox__destroy). Confidence: exact (the head's prev as the temporary,
// as the code does). Every entry is deleted as a base entry (non-virtual destructor; operator delete
// does not need the size).
void ui_listbox::dtor()
{
    while (entries.next != &entries) {
        entries.prev = entries.next;
        entries.next = entries.next->next;
        delete entries.prev;
    }
    list_init(&entries);
    selected = NULL;
    ui_gadget::dtor();
}

// 0x52d480  Confidence: exact.
ui_listbox_text_entry::ui_listbox_text_entry()
{
    text = buffer;
}

// 0x52d4a0  vtable slot 0 of 0x581e2c. Confidence: exact (the two copies share one inline strcpy in
// the code; an if / else of two strcpy calls tail-merges to it). Does not set dirty.
void ui_listbox_text_entry::set_text(const char *s)
{
    if (strlen(s) >= 255)
        strcpy(buffer, "text too long for pc version");
    else
        strcpy(buffer, s);
}

// 0x52d4f0  Confidence: exact.
int ui_listbox_entry::line_at_y_or_none(int y)
{
    if (y >= this->y && y < height + this->y)
        return (y - this->y) / (gr_get_font_height(font) + line_spacing);
    return -1;
}

// 0x52d540  Confidence: exact (store order: dirty last). next / prev are not set.
ui_listbox_entry::ui_listbox_entry()
{
    indent = 0;
    wrap_indent = 0;
    line_spacing = 0;
    font = -1;
    data = -1;
    dirty = 0;
}

// 0x52d560  Confidence: exact (the vtable store only).
ui_listbox_entry::~ui_listbox_entry()
{
}

// 0x52d570  Confidence: exact.
void ui_listbox_entry::set_indent(int indent)
{
    dirty = 1;
    this->indent = indent;
}

// 0x52d580  Confidence: exact.
void ui_listbox_entry::set_wrap_indent(int indent)
{
    dirty = 1;
    wrap_indent = indent;
}

// 0x52d590  Confidence: exact.
void ui_listbox_entry::set_line_spacing(int spacing)
{
    dirty = 1;
    line_spacing = spacing;
}

// 0x52d5a0  Confidence: exact.
void ui_listbox_entry::set_font(int font)
{
    dirty = 1;
    this->font = font;
}

// 0x52d5b0  vtable slot 0 of 0x581e30. Confidence: exact. Keeps the caller's pointer.
void ui_listbox_entry::set_text(const char *s)
{
    text = (char *)s;
}

// 0x52d5c0  Confidence: exact (the members' constructors: ui_gadget, typeahead_timer, entries; then
// the body, whose list_init the scheduler puts ahead of the vtable store, D14300).
ui_listbox::ui_listbox()
{
    list_init(&entries);
    flags = 0;
    vscrollbar = NULL;
    hscrollbar = NULL;
    selected = NULL;
    wrap_width_cb = NULL;
}

// 0x52d650  Confidence: exact (VC6 byte match, m-1020 w9; was likely) (store order). Scroll bars are created here for flags 2 / 4
// (ui_slider::create_scrollbar stores them in vscrollbar / hscrollbar).
void ui_listbox::create(ui_element *window, int x, int y, int w, int h, int content_w, int content_h,
                        int id, int flags, bool owned)
{
    init(window, UI_TYPE_LISTBOX, x, y, w, h, id, owned);
    this->flags = flags;
    bm_background = -1;
    max_content_w = content_w;
    max_content_h = content_h;
    hscroll = 0;
    vscroll = 0;
    content_width = 0;
    content_height = 0;
    user_selected = false;
    margin = 2;
    v_margin = 2;
    entry_spacing = 2;
    hscroll_step = 10;
    colors = &ui_listbox_colors.g0.c0;
    typeahead_len = 0;
    typeahead_timer.SetTimeout(0);
    vscrollbar = NULL;
    hscrollbar = NULL;

    if (flags & UI_LISTBOX_VSCROLLBAR) {
        ui_slider *s = new ui_slider;
        s->create_scrollbar(this, false, 11, -1, 1, false);
        s->owned = true;
    }
    if (flags & UI_LISTBOX_HSCROLLBAR) {
        ui_slider *s = new ui_slider;
        s->create_scrollbar(this, true, 11, -1, 1, false);
        s->owned = true;
    }
}

// 0x52d7e0  Confidence: exact (VC6 byte match, match29; was likely) (a byte match under /Ox /Ow, D7502). Replaces the first tooltip (F6206: the old node is
// unlinked but not deleted, as in ui_button::set_tooltip).
void ui_listbox::set_tooltip(const char *text, int x, int y, int delay)
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
    t->delay = delay;
    t->x = x;
    t->y = y;
    t->font = -1;
    t->owner = this;
    t->colors = &ui_tooltip_colors.c0;
    strcpy(t->text, text);
    ui_tooltip *last = tooltips.prev;
    tooltips.prev = t;
    t->next = (ui_tooltip *)&tooltips;
    t->prev = last;
    last->next = t;
}

// 0x52d8a0  Confidence: exact.
ui_listbox_entry *ui_listbox::get_selected()
{
    return selected;
}

// 0x52d8b0  Confidence: exact.
bool ui_listbox::select_by_data(int data, bool scroll_into_view)
{
    for (ui_listbox_entry *e = entries.next; e != &entries; e = e->next) {
        if (e->data == data) {
            select_entry(e, scroll_into_view);
            return true;
        }
    }
    return false;
}

// 0x52d8f0  Confidence: exact.
bool ui_listbox::select_by_text(const char *text, bool scroll_into_view)
{
    ui_listbox_entry *e = find_by_text(text);
    if (!e)
        return false;
    select_entry(e, scroll_into_view);
    return true;
}

// 0x52d920  Confidence: exact. Scrolls so the entry is at the top (at the bottom with
// UI_LISTBOX_ALIGN_BOTTOM) when it is not fully visible.
void ui_listbox::select_entry(ui_listbox_entry *e, bool scroll_into_view)
{
    selected = e;
    if (scroll_into_view && !is_entry_fully_visible(e)) {
        if (flags & UI_LISTBOX_ALIGN_BOTTOM)
            vscroll = selected->height + selected->y - get_view_height();
        else
            vscroll = selected->y;
        if (vscrollbar)
            vscrollbar->set_pos(vscroll);
    }
}

// 0x52d9a0  Confidence: exact.
void ui_listbox::clear_selection()
{
    selected = NULL;
}

// 0x52d9b0  Confidence: exact (strcmp is the inline intrinsic).
ui_listbox_entry *ui_listbox::find_by_text(const char *text)
{
    char trimmed[256];

    trim_whitespace(trimmed, text, 256);
    for (ui_listbox_entry *e = entries.next; e != &entries; e = e->next) {
        if (!strcmp(e->text, trimmed))
            return e;
    }
    return NULL;
}

// 0x52da40  Confidence: likely (store order of the fields; the goto that skips the append after a
// sorted insert). With max_content_h, the oldest entries are dropped while the content is too high.
ui_listbox_entry *ui_listbox::add_entry(const char *text, int font, int indent, int wrap_indent,
                                        int line_spacing)
{
    ui_listbox_entry *e;
    ui_listbox_entry *first;

    if (flags & UI_LISTBOX_PREALLOCATED)
        e = new ui_listbox_entry;
    else
        e = new ui_listbox_text_entry;

    e->set_text(text);
    e->indent = indent;
    e->font = font;
    e->wrap_indent = wrap_indent;
    e->colors = &ui_listbox_entry_colors.g0.c0;
    e->line_spacing = line_spacing;
    layout_entry(e, -vscroll);

    first = entries.next;
    if (first == &entries) {
        e->y = 0;
        content_height = e->height;
    } else {
        if (max_content_h > 0) {
            content_height = e->height + entry_spacing + content_height;
            while (content_height > max_content_h) {
                content_height = content_height - first->height - entry_spacing;
                remove_entry(first);
                first = entries.next;
                if (first == &entries)
                    break;
            }
        }
        if (flags & UI_LISTBOX_SORTED) {
            ui_listbox_entry *p;
            for (p = entries.next; p != &entries; p = p->next) {
                if (_stricmp(e->text, p->text) < 0)
                    break;
            }
            if (p != &entries && p) {
                ui_listbox_entry *prev = p->prev;
                p->prev = e;
                e->prev = prev;
                prev->next = e;
                e->next = p;
                goto inserted;
            }
        }
    }
    list_append(&entries, e);   // the binary's store order (D14301)

inserted:
    recalc_content_size();
    update_vscrollbar();
    update_hscrollbar();
    if (flags & UI_LISTBOX_ALIGN_BOTTOM)
        vscroll = e->height + e->y - get_view_height();
    return e;
}

// 0x52dc30  Confidence: exact (the unlink with temporaries in the code's store order).
void ui_listbox::clear()
{
    ui_listbox_entry *e;

    while ((e = entries.next) != &entries) {
        ui_listbox_entry *prev = e->prev;
        ui_listbox_entry *next = e->next;
        e->next = NULL;
        e->prev = NULL;
        prev->next = next;
        next->prev = prev;
        if (flags & UI_LISTBOX_PREALLOCATED)
            delete e;
        else
            delete (ui_listbox_text_entry *)e;
    }
    selected = NULL;
    content_height = 0;
    content_width = 0;
    set_hscroll(0);
    set_vscroll(0, false);
    update_vscrollbar();
    update_hscrollbar();
}

// 0x52dcc0  Confidence: exact. No caller.
bool ui_listbox::remove_by_text(const char *text)
{
    if (text && strlen(text)) {
        ui_listbox_entry *e = find_by_text(text);
        if (e) {
            remove_entry(e);
            return true;
        }
    }
    return false;
}

// 0x52dd00  Confidence: exact (unlink store order). Keeps the view on the entry that was at the top.
// F6207: the new scroll position is not passed to the scroll bar, so the next process (sync_scrollbars)
// can move the view back to the scroll bar's old position.
void ui_listbox::remove_entry(ui_listbox_entry *e)
{
    ui_listbox_entry *prev = e->prev;
    ui_listbox_entry *next = e->next;
    ui_listbox_entry *top;

    e->next = NULL;
    e->prev = NULL;
    prev->next = next;
    next->prev = prev;
    if (flags & UI_LISTBOX_PREALLOCATED)
        delete e;
    else
        delete (ui_listbox_text_entry *)e;

    if (selected == e)
        selected = NULL;
    recalc_content_size();
    update_vscrollbar();
    update_hscrollbar();

    if (vscroll > content_height)
        top = entries.prev;
    else
        top = entry_at(0, vscroll, true);
    if (top) {
        if (flags & UI_LISTBOX_ALIGN_BOTTOM)
            vscroll = top->height + top->y - get_view_height();
        else
            vscroll = top->y;
    } else {
        vscroll = 0;
    }
}

// 0x52dde0  Confidence: exact.
void ui_listbox::select_prev()
{
    if (entries.next == &entries)
        return;
    if (flags & UI_LISTBOX_NO_SELECT) {
        scroll_line_up();
        return;
    }
    if (selected == entries.next)
        return;

    if (selected == NULL) {
        if (flags & UI_LISTBOX_ALIGN_BOTTOM)
            selected = entry_at(0, get_view_height() + vscroll - 1, true);
        else
            selected = entry_at(0, vscroll, true);
    } else {
        selected = selected->prev;
    }
    if (selected && !is_entry_fully_visible(selected))
        set_vscroll(selected->y, false);
    send_event(UI_EVENT_LIST_SELECT, UI_USE_ID);
}

// 0x52deb0  Confidence: exact.
void ui_listbox::select_next()
{
    if (entries.next == &entries)
        return;
    if (flags & UI_LISTBOX_NO_SELECT) {
        scroll_line_down();
        return;
    }
    if (selected == entries.prev)
        return;

    if (selected == NULL) {
        if (flags & UI_LISTBOX_ALIGN_BOTTOM)
            selected = entry_at(0, get_view_height() + vscroll - 1, true);
        else
            selected = entry_at(0, vscroll, true);
    } else {
        selected = selected->next;
    }
    if (selected && !is_entry_fully_visible(selected))
        set_vscroll(selected->height + selected->y - get_view_height(), false);
    send_event(UI_EVENT_LIST_SELECT, UI_USE_ID);
}

// 0x52df90  Confidence: exact.
void ui_listbox::scroll_left()
{
    set_hscroll(hscroll - hscroll_step);
}

// 0x52dfb0  Confidence: exact.
void ui_listbox::scroll_right()
{
    set_hscroll(hscroll_step + hscroll);
}

// 0x52dfd0  Confidence: exact.
void ui_listbox::select_page_up()
{
    if (entries.next == &entries)
        return;
    if (selected) {
        if (is_entry_visible(selected))
            scroll_page_up();

        int y = selected->y - get_view_height();
        if (y < 0)
            y = 0;
        ui_listbox_entry *e = entry_at(0, y, true);
        if (selected != e) {
            selected = e;
            send_event(UI_EVENT_LIST_SELECT, UI_USE_ID);
        }
        user_selected = true;
    } else {
        scroll_page_up();
    }
}

// 0x52e050  Confidence: exact (the `if (next)` test of a ring pointer). F6208: when the selected entry
// is the last one and taller than the view, next is the ring head, which becomes the selection.
void ui_listbox::select_page_down()
{
    if (entries.next == &entries)
        return;
    if (selected) {
        if (is_entry_visible(selected))
            scroll_page_down();

        ui_listbox_entry *e = selected;
        if (e->height > get_view_height()) {
            e = e->next;
            if (e) {
                selected = e;
                send_event(UI_EVENT_LIST_SELECT, UI_USE_ID);
            }
        } else {
            int y = get_view_height() + selected->y;
            if (y > content_height - 1)
                y = content_height - 1;
            e = entry_at(0, y, true);
            if (selected != e) {
                selected = e;
                send_event(UI_EVENT_LIST_SELECT, UI_USE_ID);
            }
        }
        user_selected = true;
    } else {
        scroll_page_down();
    }
}

// 0x52e110  Confidence: exact. One line (or one entry) up; bottom-aligned lists
// move the view's bottom edge.
void ui_listbox::scroll_line_up()
{
    ui_listbox_entry *e;

    if (entries.next == &entries)
        return;
    if (is_entry_fully_visible(entries.next))
        return;

    if (flags & UI_LISTBOX_ALIGN_BOTTOM) {
        e = entry_at_y(get_view_height() + vscroll - 1);
        if (!e) {
            e = entries.prev;
            set_vscroll(e->height + e->y - get_view_height(), false);
            return;
        }
        if (flags & UI_LISTBOX_BY_ENTRY) {
            if (e == entries.next)
                return;
            e = e->prev;
            set_vscroll(e->height + e->y - get_view_height(), false);
            return;
        }
        int line = e->line_at_y_with_spacing(get_view_height() + vscroll - 1);
        if (line == 0) {
            if (e == entries.next)
                return;
            e = e->prev;
            set_vscroll(e->height + e->y - get_view_height(), false);
            return;
        }
        set_vscroll(e->line_bottom(line - 1) - get_view_height(), false);
        return;
    }

    e = entry_above(vscroll - 1);
    if (!e) {
        set_vscroll(entries.next->y, false);
        return;
    }
    if (flags & UI_LISTBOX_BY_ENTRY) {
        set_vscroll(e->y, false);
        return;
    }
    int line = e->line_at_y(vscroll - 1);
    set_vscroll(e->line_top(line), false);
}

// 0x52e290  Confidence: exact. Top-aligned lists step one pixel and let set_vscroll snap.
void ui_listbox::scroll_line_down()
{
    ui_listbox_entry *e;

    if (entries.next == &entries)
        return;
    if (is_entry_fully_visible(entries.prev))
        return;

    if (flags & UI_LISTBOX_ALIGN_BOTTOM) {
        e = entry_at_y(get_view_height() + vscroll);
        if (!e) {
            e = entries.prev;
            set_vscroll(e->height + e->y - get_view_height(), false);
            return;
        }
        if (flags & UI_LISTBOX_BY_ENTRY) {
            set_vscroll(e->height + e->y - get_view_height(), false);
            return;
        }
        int line = e->line_at_y_with_spacing(get_view_height() + vscroll);
        set_vscroll(e->line_bottom(line) - get_view_height(), false);
        return;
    }
    set_vscroll(vscroll + 1, true);
}

// 0x52e380  Confidence: likely (the `if (vscroll != top)` shape that stores top on both paths).
// One view height up, then snapped to an entry (BY_ENTRY) or a line. Moves vscroll directly (no scroll
// bar update).
void ui_listbox::scroll_page_up()
{
    ui_listbox_entry *e;
    int fh;

    user_selected = false;
    if (entries.next == &entries)
        return;

    vscroll -= get_view_height();
    if (flags & UI_LISTBOX_ALIGN_BOTTOM) {
        e = entries.next;
        if (e != NULL && vscroll < e->height - get_view_height())
            vscroll = e->height - get_view_height();
    } else if (vscroll < 0) {
        vscroll = 0;
    }

    if (!(flags & UI_LISTBOX_BY_ENTRY)) {
        if (flags & UI_LISTBOX_ALIGN_BOTTOM) {
            e = entry_at(0, get_view_height() + vscroll - 1, true);
            fh = gr_get_font_height(e->font);
            if (get_view_height() + vscroll - 1 <= e->y) {
                vscroll = e->y + fh - get_view_height();
            } else {
                int line = e->line_at_y_or_none(get_view_height() + vscroll - 1);
                vscroll = (line + 1) * fh + e->y - get_view_height();
            }
        } else {
            e = entry_at(0, vscroll, true);
            fh = gr_get_font_height(e->font);
            if (vscroll <= e->y) {
                vscroll = e->y;
            } else {
                int line = e->line_at_y_or_none(vscroll);
                int top = line * fh + e->y;
                if (vscroll == top)
                    vscroll = top;
                else if (line == e->num_lines - 1)
                    vscroll = e->next->y;
                else
                    vscroll = (line + 1) * fh + e->y;
            }
        }
    } else {
        if (flags & UI_LISTBOX_ALIGN_BOTTOM) {
            e = entry_at(0, get_view_height() + vscroll - 1, true);
            vscroll = e->height + e->y - get_view_height();
        } else {
            vscroll = entry_at(0, vscroll, true)->y;
        }
    }
}

// 0x52e590  Confidence: exact (VC6 byte match, m-1020 w9; was likely). The inverse of scroll_page_up; does nothing when the view is already
// past the end. In BY_ENTRY mode an entry taller than the view is stepped over as a whole.
void ui_listbox::scroll_page_down()
{
    ui_listbox_entry *e;
    ui_listbox_entry *last;
    int fh;

    if (entries.next == &entries)
        return;
    if (vscroll > content_height - get_view_height())
        return;
    user_selected = false;

    if (!(flags & UI_LISTBOX_BY_ENTRY)) {
        vscroll += get_view_height();
        if (flags & UI_LISTBOX_ALIGN_BOTTOM) {
            last = entries.prev;
            if (vscroll > last->y + last->height - get_view_height())
                vscroll = last->y + last->height - get_view_height();
            e = entry_at(0, get_view_height() + vscroll - 1, true);
            fh = gr_get_font_height(e->font);
            if (vscroll <= e->y) {
                vscroll = e->y + fh - get_view_height();
            } else {
                int line = e->line_at_y_or_none(get_view_height() + vscroll - 1);
                vscroll = (line + 1) * fh + e->y - get_view_height();
            }
        } else {
            if (vscroll >= content_height)
                vscroll = content_height - 1;
            e = entry_at(0, vscroll, true);
            fh = gr_get_font_height(e->font);
            if (vscroll <= e->y)
                vscroll = e->y;
            else
                vscroll = e->line_at_y_or_none(vscroll) * fh + e->y;
        }
    } else {
        if (flags & UI_LISTBOX_ALIGN_BOTTOM) {
            e = entry_at(0, get_view_height() + vscroll - 1, true);
            if (e->height > get_view_height() && e != entries.prev) {
                e = e->next;
                vscroll = e->height + e->y - get_view_height();
            } else {
                vscroll += get_view_height();
                last = entries.prev;
                if (vscroll > last->y + last->height - get_view_height())
                    vscroll = last->y + last->height - get_view_height();
                e = entry_at(0, get_view_height() + vscroll - 1, true);
                vscroll = e->height + e->y - get_view_height();
            }
        } else {
            e = entry_at(0, vscroll, true);
            if (e->height > get_view_height() && e != entries.prev) {
                vscroll = e->next->y;
                return;
            }
            vscroll += get_view_height();
            if (vscroll >= content_height)
                vscroll = content_height - 1;
            vscroll = entry_at(0, vscroll, true)->y;
        }
    }
}

// 0x52e870  Confidence: exact. No caller.
void ui_listbox::scroll_page_right()
{
    hscroll += get_view_width();
    if (hscroll > content_width - get_view_width())
        hscroll = content_width - get_view_width();
}

// 0x52e8c0  Confidence: exact. No caller.
void ui_listbox::scroll_page_left()
{
    hscroll -= get_view_width();
    if (hscroll < 0)
        hscroll = 0;
}

// 0x52e8f0  Confidence: exact (the view width is fetched before hscroll is loaded). No caller.
float ui_listbox::get_hscroll_fraction()
{
    int range = content_width - get_view_width();

    return (float)hscroll / range;
}

// 0x52e920  Confidence: exact (VC6 byte match, m-1020b w13: lo through a float temporary assigned after
// last is read, the font height in a local; D13700). No caller. The scroll position between the first and
// the last stop. F6209: the line stops use the font height without line_spacing.
float ui_listbox::get_vscroll_fraction()
{
    float lo, hi;
    ui_listbox_entry *first = entries.next;
    ui_listbox_entry *last;

    if (first == &entries) {
        hi = 0.0f;
        lo = 0;
    } else if (flags & UI_LISTBOX_ALIGN_BOTTOM) {
        float t;
        if (!(flags & UI_LISTBOX_BY_ENTRY))
            t = (float)(gr_get_font_height(first->font) - get_view_height());
        else
            t = (float)(first->height - get_view_height());
        last = entries.prev;
        lo = t;
        hi = (float)(last->height + last->y - get_view_height());
    } else {
        last = entries.prev;
        lo = 0;
        if (!(flags & UI_LISTBOX_BY_ENTRY)) {
            int fh = gr_get_font_height(last->font);
            hi = (float)((last->num_lines - 1) * fh + last->y);
        } else {
            hi = (float)last->y;
        }
    }
    return (vscroll - lo) / (hi - lo);
}

// 0x52ea00  Confidence: exact.
void ui_listbox::set_hscroll(int pos)
{
    if (pos > content_width - get_view_width())
        pos = content_width - get_view_width();
    if (pos < 0)
        pos = 0;
    hscroll = pos;
    if (hscrollbar)
        hscrollbar->set_pos(pos);
}

// 0x52ea50  Confidence: exact. Clamped to the content, then snapped to an entry or line edge unless the
// list scrolls by pixels (force snaps anyway). Bottom-aligned lists shorter than the view get a negative
// position (the second clamp), which keeps their entries at the bottom.
void ui_listbox::set_vscroll(int pos, bool force)
{
    if (pos > content_height - get_view_height())
        pos = content_height - get_view_height();
    if (pos < 0)
        pos = 0;
    if ((flags & UI_LISTBOX_ALIGN_BOTTOM) && pos > content_height - get_view_height())
        pos = content_height - get_view_height();

    if (!(flags & UI_LISTBOX_BY_PIXELS) || force) {
        if (flags & UI_LISTBOX_ALIGN_BOTTOM)
            pos = snap_scroll_bottom(pos);
        else
            pos = snap_scroll_top(pos);
    }
    if (flags & UI_LISTBOX_BY_PIXELS) {
        if (pos > content_height - get_view_height())
            pos = content_height - get_view_height();
        if (pos < 0)
            pos = 0;
    }
    vscroll = pos;
    if (vscrollbar)
        vscrollbar->set_pos(pos);
}

// 0x52eb20  Confidence: exact (VC6 byte match, match29; was likely) (loop shapes). The top of the first line (or entry) at or below y.
int ui_listbox::snap_scroll_top(int y)
{
    int result = y;
    ui_listbox_entry *e;

    if (entries.next == &entries)
        return y;

    for (e = entries.next; e != &entries; e = e->next) {
        if (e->y >= y)
            break;
    }
    if (e == &entries)
        return entries.prev->y;
    if (flags & UI_LISTBOX_BY_ENTRY)
        return e->y;
    if (e == entries.next)
        return e->y;

    ui_listbox_entry *p = e->prev;
    int fh = gr_get_font_height(p->font);
    int n = p->num_lines;
    int i = 0;
    if (n > 0) {
        int line_h = p->line_spacing + fh;
        int top = p->y;
        while (1) {
            result = top;
            if (top >= y)
                break;
            i++;
            top += line_h;
            if (i >= n)
                break;
        }
    }
    if (i == n)
        return p->next->y;
    return result;
}

// 0x52ebe0  Confidence: likely (loop shapes, the argument slot reused for the result). The bottom of
// the last line (or entry) that ends at or above y + the view height, minus the view height. F6210:
// at the very end of the content the search stops at the ring head and reads its uninitialised
// num_lines / y.
int ui_listbox::snap_scroll_bottom(int y)
{
    ui_listbox_entry *e;

    if (entries.next == &entries)
        return y;

    int bottom = get_view_height() + y;
    ui_listbox_entry *first = entries.next;
    for (e = first; e != &entries; e = e->next) {
        if (e->height + e->y > bottom)
            break;
    }

    if (e == first) {
        if (flags & UI_LISTBOX_BY_ENTRY)
            return e->height + e->y - get_view_height();
        return e->y + gr_get_font_height(e->font) - get_view_height();
    }
    if (flags & UI_LISTBOX_BY_ENTRY) {
        e = e->prev;
        return e->height + e->y - get_view_height();
    }

    int fh = gr_get_font_height(e->font);
    int i = 0;
    if (e->num_lines > 0) {
        int line_h = e->line_spacing + fh;
        int line_bottom = e->y + fh;
        while (line_bottom <= bottom) {
            y = line_bottom;
            i++;
            line_bottom += line_h;
            if (i >= e->num_lines)
                break;
        }
    }
    if (i == 0) {
        e = e->prev;
        return e->height + e->y - get_view_height();
    }
    return y - get_view_height();
}

// 0x52ed00  Confidence: exact. No caller; the second argument is not read; no scroll bar update.
void ui_listbox::set_hscroll_fraction(float f, bool unused)
{
    if (f < 0.0f)
        f = 0.0f;
    else if (f > 1.0f)
        f = 1.0f;
    int range = content_width - get_view_width();
    if (range < 0)
        range = 0;
    hscroll = (int)(range * f);
}

// 0x52ed70  Confidence: likely (float rounding expression, branch order). No caller; the second
// argument is not read. The fraction between the first and the last stop, rounded, then snapped as
// scroll_page_* do (with the font height only, F6209).
void ui_listbox::set_vscroll_fraction(float f, bool unused)
{
    ui_listbox_entry *e;
    ui_listbox_entry *last;
    int lo, hi;
    int fh;

    if (f < 0.0f)
        f = 0.0f;
    else if (f > 1.0f)
        f = 1.0f;

    e = entries.next;
    user_selected = false;
    if (e == &entries) {
        lo = 0;
        hi = 0;
    } else if (flags & UI_LISTBOX_ALIGN_BOTTOM) {
        if (!(flags & UI_LISTBOX_BY_ENTRY))
            lo = gr_get_font_height(e->font) - get_view_height();
        else
            lo = e->height - get_view_height();
        last = entries.prev;
        hi = last->height + last->y - get_view_height();
    } else {
        last = entries.prev;
        lo = 0;
        if (!(flags & UI_LISTBOX_BY_ENTRY))
        {
            fh = gr_get_font_height(last->font);
            hi = (last->num_lines - 1) * fh + last->y;
        }
        else
            hi = last->y;
    }

    float v = (hi - lo) * f + lo;
    if (v >= 0.0f)
        vscroll = (int)(v + 0.5f);
    else
        vscroll = (int)(v - 0.5f);

    if (flags & UI_LISTBOX_ALIGN_BOTTOM) {
        e = entry_at(0, get_view_height() + vscroll - 1, true);
        if (!(flags & UI_LISTBOX_BY_ENTRY)) {
            if (vscroll <= e->y - get_view_height()) {
                e = e->prev;
                vscroll = e->height + e->y - get_view_height();
            } else {
                int line = e->line_at_y_or_none(get_view_height() + vscroll - 1);
                fh = gr_get_font_height(e->font);
                vscroll = (line + 1) * fh + e->y - get_view_height();
            }
        } else {
            vscroll = e->height + e->y - get_view_height();
        }
    } else if (f < 1.0f) {
        e = entry_at(0, vscroll, true);
        if (!(flags & UI_LISTBOX_BY_ENTRY)) {
            fh = gr_get_font_height(e->font);
            int top = e->y;
            if (vscroll <= top) {
                vscroll = top;
            } else {
                int i;
                int line_top = top;
                int line_bottom = fh + top;
                for (i = 0; i < e->num_lines; i++) {
                    if (vscroll >= line_top && vscroll < line_bottom)
                        break;
                    line_top += fh;
                    line_bottom += fh;
                }
                vscroll = i * fh + top;
            }
        } else {
            vscroll = e->y;
        }
    }
}

// 0x52f020  Confidence: exact.
int ui_listbox::get_view_width()
{
    return w - margin * 2;
}

// 0x52f030  Confidence: exact.
int ui_listbox::get_view_height()
{
    return h - v_margin * 2;
}

// 0x52f040  Confidence: exact.
int ui_listbox::get_content_width()
{
    return content_width;
}

// 0x52f050  Confidence: exact.
int ui_listbox::get_content_height()
{
    return content_height;
}

// 0x52f060  Confidence: exact. No caller.
bool ui_listbox::is_empty()
{
    if (entries.next == &entries)
        return true;
    return false;
}

// 0x52f080  vtable slot 7. Confidence: likely (local layout; the background branch's argument
// evaluation). Background bitmap or sunken frame, then each visible entry line by line, clipped to the
// view; the selection, the mouse-over entry (UI_LISTBOX_MOUSEOVER) and the hidden state use other
// colour pairs (text, shadow). A disabled list box is not drawn at all.
void ui_listbox::draw()
{
    char line[512];
    ui_listbox_entry *hover;
    ui_listbox_entry *e;

    if (is_disabled())
        return;

    relayout_dirty();
    if (hscrollbar) {
        if (content_width > get_view_width())
            hscrollbar->set_visible(true);
        else
            hscrollbar->set_hidden(true);
    }
    if (vscrollbar) {
        if (content_height > get_view_height())
            vscrollbar->set_visible(true);
        else
            vscrollbar->set_hidden(true);
    }

    gr::reset_clip();
    if (!(flags & UI_LISTBOX_NO_BACKGROUND)) {
        if (bm_background >= 0) {
            gr_set_color(255, 255, 255, 255);
            gr_bitmap(bm_background, get_screen_x(0), get_screen_y(0), gr_bitmap_wrap_mode);
        } else {
            draw_sunken(is_hidden() ? &colors[3] : colors, 1);
        }
    }

    hover = NULL;
    if (flags & UI_LISTBOX_MOUSEOVER)
        hover = entry_at(hscroll + ui_mouse_x - get_screen_x(0), vscroll + ui_mouse_y - get_screen_y(0),
                         false);

    gr::set_clip(get_screen_x(0) + margin, get_screen_y(0) + v_margin, w - margin * 2,
                 h - v_margin * 2);

    for (e = entries.next; e != &entries; e = e->next) {
        if (!is_entry_visible(e))
            continue;

        int fh = gr_get_font_height(e->font);
        Color *c;
        if (is_hidden())
            c = &colors[6];
        else if (e == selected)
            c = &e->colors[2];
        else if ((flags & UI_LISTBOX_MOUSEOVER) && hover == e)
            c = &e->colors[4];
        else
            c = e->colors;

        int y = e->y - vscroll;
        int lines = layout_entry(e, y);
        for (int i = 0; i < lines; i++) {
            int x = (i ? e->wrap_indent : e->indent) - hscroll;
            if ((y >= 0 && fh + y <= get_view_height()) || (flags & UI_LISTBOX_BY_PIXELS)) {
                strncpy(line, ui_listbox_line_starts[i], ui_listbox_line_lengths[i]);
                line[ui_listbox_line_lengths[i]] = '\0';
                if (c[1].a) {
                    gr_set_color_ptr(&c[1]);
                    gr_string(x + 1, y + 1, line, e->font, gr_string_mode);
                }
                gr_set_color_ptr(c);
                gr_string(x, y, line, e->font, gr_string_mode);
            }
            y += e->line_spacing + fh;
        }
    }
    ui_gadget::draw();
}

// 0x52f3d0  Confidence: exact (`== true` gives the cmp al, 1).
void ui_listbox::set_sorted(bool sorted)
{
    if (sorted == true)
        flags |= UI_LISTBOX_SORTED;
    else
        flags &= ~UI_LISTBOX_SORTED;
}

// 0x52f400  ui_listbox_entry::`scalar deleting destructor' (from `delete e`). Compiler-generated.
// 0x52f420  ui_listbox_text_entry::`scalar deleting destructor' (from `delete (ui_listbox_text_entry *)e`).
//           Compiler-generated.
// 0x52f440  ui_listbox_text_entry::~ui_listbox_text_entry, implicit (jmp ~ui_listbox_entry).
//           Compiler-generated.
