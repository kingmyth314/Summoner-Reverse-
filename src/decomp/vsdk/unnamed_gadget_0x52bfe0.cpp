// unnamed_gadget_0x52bfe0.cpp - Sum.exe v1.40, code 0x52bfe0-0x52ca53 (vsdk\ui; the unit's file name is
// not in the binary, so the name is a placeholder). Written from the Sum.exe disassembly (tools/asm.mjs).
// Target: VC++ 6.0 (cl 12.00.8966), /O2. Confidence per function: exact = expect identical code,
// likely = same semantics, small layout or scheduling risk, unsure = the source shape is a guess.
//
// The gadget base class (ui_gadget, vtable 0x581e08): window and parent links, screen position,
// visibility, focus, hotkeys, hit testing and the keyboard-navigation search, then the state bitmap
// (ui_state_bitmap) that buttons and statics draw.
//
// Unit bounds (NOTES.md): window.cpp ends with its two COMDATs at 0x52bfa0 / 0x52bfc0, so 0x52bfe0 starts
// a new object. The end at 0x52ca53 is not proven: the list box code that follows has the first .bss of
// the next block (0x2cbf584) and its own EH funclets (0x57d510-0x57d556), but this unit has no data and no
// EH, so it may also be the head of the list box object. No .data or .bss: ui_nav_best_gadget /
// _distance are window.cpp's, ui_current_gadget is the UI manager's.

#include "vsdk/ui/window.h"
#include "vsdk/gr/gr.h"
#include "experimental/ui_shift.h"   // EXPERIMENTAL_BUILD only (widescreen); keep it the last include

// 0x52bfe0  Confidence: exact (the store order parent, *link, next_sibling is read from the code; a
// plain `*link = next_sibling;` first would give the same instructions only if VC6 reordered the stores).
// Unlinks the gadget from its parent's child chain, or from the window's top-level chain.
void ui_gadget::unlink()
{
    ui_gadget **link;
    ui_gadget *g;

    if (parent)
        link = &parent->first_child;
    else
        link = &((ui_window *)window)->first_gadget;

    for (g = *link; g; g = *link) {
        if (g == this) {
            ui_gadget *next = next_sibling;
            parent = NULL;
            *link = next;
            next_sibling = NULL;
            return;
        }
        link = &g->next_sibling;
    }
}

// 0x52c040  Confidence: exact (VC6 byte match, match23; owned stored before gadget_type in the source).
void ui_gadget::init(ui_element *window, int type, int x, int y, int w, int h, int id, bool owned)
{
    ui_window *win;

    ui_element::init(0, x, y, w, h, id);
    this->window = window;
    first_child = NULL;
    win = (ui_window *)window;
    ui_gadget *last = win->gadgets.window_prev;
    window_next = &win->gadgets;
    window_prev = last;
    last->window_next = this;
    win->gadgets.window_prev = this;
    next_sibling = NULL;
    parent = NULL;
    set_parent(NULL);
    this->owned = owned;
    gadget_type = type;
    clear_hotkeys();
    hotspot = 0;
}

// 0x52c0e0  Confidence: exact. No caller.
int ui_gadget::identity(int x)
{
    return x;
}

// 0x52c0f0  Confidence: exact (written as the tail recursion over the siblings that VC6 turns into the
// loop; a do / while over next_sibling gives the same code). Keyboard navigation: the selectable gadget
// above (x, y) with the smallest vertical distance plus horizontal miss, into ui_nav_best_gadget.
void ui_gadget::find_nearest_up(int x, int y)
{
    int miss = 0;
    if (is_selectable()) {
        int gx = get_screen_x(0);
        int gy = get_screen_y(0);

        if (gx > x)
            miss = gx - x;
        else if (gx + w <= x)
            miss = x - w - gx + 1;

        int dist = y - h - gy;
        if (dist >= miss && dist + miss < ui_nav_best_distance) {
            ui_nav_best_distance = dist + miss;
            ui_nav_best_gadget = this;
        }
    }
    if (first_child)
        first_child->find_nearest_up(x, y);
    if (next_sibling)
        next_sibling->find_nearest_up(x, y);
}

// 0x52c180  Confidence: exact (as find_nearest_up).
void ui_gadget::find_nearest_down(int x, int y)
{
    int miss = 0;
    if (is_selectable()) {
        int gx = get_screen_x(0);
        int gy = get_screen_y(0);

        if (gx > x)
            miss = gx - x;
        else if (gx + w <= x)
            miss = x - w - gx + 1;

        int dist = gy - y;
        if (dist >= miss && miss + dist < ui_nav_best_distance) {
            ui_nav_best_distance = miss + dist;
            ui_nav_best_gadget = this;
        }
    }
    if (first_child)
        first_child->find_nearest_down(x, y);
    if (next_sibling)
        next_sibling->find_nearest_down(x, y);
}

// 0x52c210  Confidence: exact (as find_nearest_up, the axes swapped).
void ui_gadget::find_nearest_left(int x, int y)
{
    int miss = 0;
    if (is_selectable()) {
        int gx = get_screen_x(0);
        int gy = get_screen_y(0);

        if (gy > y)
            miss = gy - y;
        else if (gy + h <= y)
            miss = y - h - gy + 1;

        int dist = x - w - gx;
        if (dist >= miss && dist + miss < ui_nav_best_distance) {
            ui_nav_best_distance = dist + miss;
            ui_nav_best_gadget = this;
        }
    }
    if (first_child)
        first_child->find_nearest_left(x, y);
    if (next_sibling)
        next_sibling->find_nearest_left(x, y);
}

// 0x52c2a0  Confidence: exact.
void ui_gadget::find_nearest_right(int x, int y)
{
    int miss = 0;
    if (is_selectable()) {
        int gx = get_screen_x(0);
        int gy = get_screen_y(0);

        if (gy > y)
            miss = gy - y;
        else if (gy + h <= y)
            miss = y - h - gy + 1;

        int dist = gx - x;
        if (dist >= miss && miss + dist < ui_nav_best_distance) {
            ui_nav_best_distance = miss + dist;
            ui_nav_best_gadget = this;
        }
    }
    if (first_child)
        first_child->find_nearest_right(x, y);
    if (next_sibling)
        next_sibling->find_nearest_right(x, y);
}

// 0x52c330  vtable slot 1. Confidence: exact (D21001). The callback first; unhandled events go to the parent
// container, else to the window.
// D21001: no return after the handled test (as ui_window::send_event, D15400): when the callback returns
// true the function falls off the end with the callback's AL, which is the binary's jump to the window call's
// epilogue. VC6 warns C4715; same behaviour (the caller reads AL = the nonzero result).
bool ui_gadget::send_event(int event, int value)
{
    if (value == UI_USE_ID)
        value = id;
    if (!callback || !callback(event, value)) {
        if (parent)
            return parent->send_event(event, value);
        return window->send_event(event, value);
    }
}

// 0x52c390  vtable slot 5. Confidence: exact. LOGIC FIX (D10700): each child gets the previous child's
// result (the loop jumps back to 0x52c3aa, past the load of key at 0x52c3a6, and pushes EAX, the last
// call's result); the result is the last child's; hidden or childless gadgets return key.
int ui_gadget::process(int key)
{
    if (!is_hidden()) {
        ui_gadget *g = first_child;
        if (g) {
            for (; g; g = g->next_sibling)
                key = g->process(key);
            return key;
        }
    }
    return key;
}

// 0x52c3d0  vtable slot 0 (ui_gadget__destroy). Confidence: exact.
void ui_gadget::dtor()
{
    if (ui_current_gadget == this)
        ui_current_gadget = NULL;
    window = NULL;
    ui_element::dtor();
}

// 0x52c3f0  Confidence: exact.
ui_gadget::ui_gadget()
{
}

// 0x52c410  ui_gadget__dtor_base. Confidence: exact.
ui_gadget::~ui_gadget()
{
}

// 0x52c420  Confidence: exact.
int ui_gadget::get_screen_x(int offset)
{
    if (parent)
        return parent->get_screen_x(0) + x + offset;
    return window->x + x + offset;
}

// 0x52c460  Confidence: exact.
int ui_gadget::get_screen_y(int offset)
{
    if (parent)
        return parent->get_screen_y(0) + y + offset;
    return window->y + y + offset;
}

// 0x52c4a0  Confidence: exact.
bool ui_gadget::is_owned()
{
    return owned;
}

// 0x52c4b0  Confidence: exact.
bool ui_gadget::is_hidden()
{
    if (!window)
        return true;
    if (((ui_window *)window)->is_hidden())
        return true;
    if (parent && parent->is_hidden())
        return true;
    return ui_element::is_hidden();
}

// 0x52c4f0  Confidence: exact. The parent is tested with is_hidden, not is_shown.
bool ui_gadget::is_shown()
{
    if (!window)
        return false;
    if (((ui_window *)window)->is_hidden())
        return false;
    if (parent && parent->is_hidden())
        return false;
    return ui_element::is_shown();
}

// 0x52c530  vtable slot 4. Confidence: exact.
bool ui_gadget::is_disabled()
{
    if (!window)
        return true;
    if (window->is_disabled())
        return true;
    if (parent && parent->is_disabled())
        return true;
    return ui_element::is_disabled();
}

// 0x52c570  Confidence: exact (D25700: next_sibling assigned straight from the old head in both branches,
// with no temporary; 8966 still stores the new head first, and the else branch then takes EDX as the
// binary. A temporary in either branch alone leaves the else branch's head in ECX).
void ui_gadget::set_parent(ui_gadget *parent)
{
    unlink();
    this->parent = parent;
    if (parent) {
        next_sibling = parent->first_child;
        parent->first_child = this;
    } else {
        ui_window *win = (ui_window *)window;
        next_sibling = win->first_gadget;
        win->first_gadget = this;
    }
}

// 0x52c5c0  Confidence: exact.
ui_element *ui_gadget::get_window()
{
    return window;
}

// 0x52c5d0  Confidence: exact.
ui_gadget *ui_gadget::get_parent()
{
    return parent;
}

// 0x52c5e0  Confidence: exact.
ui_gadget *ui_gadget::get_first_child()
{
    return first_child;
}

// 0x52c5f0  Confidence: exact.
ui_gadget *ui_gadget::get_next_sibling()
{
    return next_sibling;
}

// 0x52c600  Confidence: exact. Event 4 (focus lost) to the old focus, event 3 to the new one.
void ui_gadget::set_focus()
{
    if (ui_current_gadget)
        ui_current_gadget->send_event(4, UI_USE_ID);
    ui_current_gadget = this;
    send_event(3, UI_USE_ID);
}

// 0x52c630  Confidence: exact.
void ui_gadget::clear_focus()
{
    if (ui_current_gadget == this && this) {
        ui_current_gadget = NULL;
        send_event(4, UI_USE_ID);
    }
}

// 0x52c660  Confidence: exact.
bool ui_gadget::is_selected()
{
    if (((ui_window *)window)->selected == this)
        return true;
    return false;
}

// 0x52c670  Confidence: exact.
bool ui_gadget::is_mouse_over()
{
    if (((ui_window *)window)->mouse_over == this)
        return true;
    return false;
}

// 0x52c680  Confidence: exact.
int ui_gadget::get_type()
{
    return gadget_type;
}

// 0x52c690  Confidence: exact.
void ui_gadget::set_hotkey(int key, int slot)
{
    hotkeys[slot] = key;
}

// 0x52c6b0  Confidence: exact.
void ui_gadget::clear_hotkeys()
{
    for (int i = 0; i < 2; i++)
        hotkeys[i] = -1;
}

// 0x52c6c0  Confidence: exact.
bool ui_gadget::has_hotkey(int key)
{
    if (key > 0) {
        for (int i = 0; i < 2; i++) {
            if (key == hotkeys[i])
                return true;
        }
    }
    return false;
}

// 0x52c6f0  vtable slot 6. Confidence: exact. The parent's (virtual) mask, else the window's.
int ui_gadget::get_mask_pixel(int x, int y)
{
    if (parent)
        return parent->get_mask_pixel(x, y);
    return ((ui_window *)window)->get_mask_pixel(x, y);
}

// 0x52c730  Confidence: exact (whether the `if (found)` sits inside a test of first_child: the code
// skips it when there are no children). The last child that reports a hit wins; then the gadget itself:
// enabled, inside its rectangle, and on its hotspot value of the mask (a container: any non-zero pixel of
// its own mask, or its parent's mask when it has none).
ui_gadget *ui_gadget::hit_test(int x, int y)
{
    ui_gadget *found = NULL;
    ui_gadget *g;
    int pixel;

    for (g = first_child; g; g = g->next_sibling) {
        ui_gadget *hit = g->hit_test(x, y);
        if (hit)
            found = hit;
    }
    if (found)
        return found;

    if (is_disabled())
        return NULL;

    int lx = x - get_screen_x(0);
    int ly = y - get_screen_y(0);
    if (lx < 0 || ly < 0 || lx >= w || ly >= h)
        return NULL;

    pixel = get_mask_pixel(x, y);
    if (gadget_type == UI_TYPE_CONTAINER) {
        if (pixel >= 0)
            return pixel ? this : NULL;
        pixel = ui_gadget::get_mask_pixel(x, y);
    }
    if (pixel < 0 || hotspot == pixel)
        return this;
    return NULL;
}

// 0x52c800  Confidence: exact.
void ui_gadget::set_hotspot(int size)
{
    if (size < 1)
        size = 0;
    hotspot = size;
}

// 0x52c820  Confidence: exact. No caller.
void ui_gadget::clear_hotspot()
{
    hotspot = 0;
}

// 0x52c830  Confidence: exact. No caller.
int ui_gadget::get_hotspot()
{
    return hotspot;
}

// 0x52c840  vtable slot 7. Confidence: exact. The children, then event 2 (drawn) to the gadget itself.
void ui_gadget::draw()
{
    if (is_disabled())
        return;
    for (ui_gadget *g = first_child; g; g = g->next_sibling)
        g->draw();
    send_event(2, UI_USE_ID);
}

// 0x52c880  vtable slot 8. Confidence: exact.
bool ui_gadget::is_selectable()
{
    return false;
}

// 0x52c890  vtable slot 2. Confidence: exact.
void ui_gadget::set_disabled(bool disabled)
{
    ui_element::set_disabled(disabled);
    if (is_disabled())
        clear_window_refs();
}

// 0x52c8c0  vtable slot 3. Confidence: exact.
void ui_gadget::set_enabled(bool enabled)
{
    ui_element::set_enabled(enabled);
    if (is_disabled())
        clear_window_refs();
}

// 0x52c8f0  Confidence: exact.
void ui_gadget::clear_window_refs()
{
    if (is_mouse_over())
        ((ui_window *)window)->mouse_over = NULL;
    if (is_selected())
        ((ui_window *)window)->selected = NULL;
}

// ---- ui_state_bitmap --------------------------------------------------------------------------------

// 0x52c930  Confidence: exact (store order state, next, bm).
ui_state_bitmap::ui_state_bitmap()
{
    state = 0;
    next = this;
    bm = -1;
}

// 0x52c950  Confidence: exact (a jmp to clear).
ui_state_bitmap::~ui_state_bitmap()
{
    clear();
}

// 0x52c960  Confidence: exact. Splices this (a one-element ring) in after other.
void ui_state_bitmap::link(ui_state_bitmap *other)
{
    ui_state_bitmap *n = other->next;
    other->next = this;
    next = n;
}

// 0x52c970  Confidence: exact.
void ui_state_bitmap::clear()
{
    state = 0;
}

// 0x52c980  Confidence: exact.
void ui_state_bitmap::set(int bm)
{
    clear();
    state = 1;
    this->bm = bm;
}

// 0x52c9a0  Confidence: exact (the clip test is done once for both paths). In clip mode the bitmap is
// drawn relative to the clip origin (x + u, y + v), i.e. still at (x, y). F6204: with state 1 and no
// bitmap it returns without gr::reset_clip, leaving the clip rectangle set.
bool ui_state_bitmap::draw(int x, int y, int w, int h, int u, int v)
{
    if (state != 2 && u >= 0) {
        gr::set_clip(x + u, y + v, w, h);
        x = -u;
        y = -v;
    }
    switch (state) {
    case 1:
        if (bm < 0)
            return false;
        gr_set_color(255, 255, 255, 255);
        gr_bitmap(bm, x, y, gr_bitmap_wrap_mode);
        if (u >= 0)
            gr::reset_clip();
        return true;
    default:
        if (u >= 0)
            gr::reset_clip();
        return false;
    }
}

// 0x52ca50  Confidence: exact. No caller, no sum.symbols entry.
int ui_state_bitmap::get_bitmap()
{
    return bm;
}
