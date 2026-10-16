// unnamed_ui_element_0x542cd0.cpp - Sum.exe v1.40, code 0x542cd0-0x54302c: ui_element, the common base of
// ui_window and ui_gadget (vsdk\ui in sum.symbols; vtable 0x581ea0). The file name is a placeholder: no path
// string, no strings.tbl block (docs/sum-exe/vsdk-ui.md: the vsdk\ui file names are not in the binary).
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: after the float helper object (its last COMDATs 0x542cb0 / 0x542cc0) and before
// skeleton.cpp (vim_alloc 0x543040); ui_tooltip's compiler-generated scalar deleting destructor (0x543010)
// closes it. .data 0x5ac8dc (the automatic id counter; skeleton.cpp's "vim" follows at 0x5ac8e0), no .bss.
// The EH funclet of ~ui_element is 0x57d61e. 0x543030 (sum.symbols ui_stub_ret_543030, a bare ret with
// no reference) is written as skeleton.cpp's first function (D5609; NOTES decomp67).

#include "vsdk/ui/ui.h"

// The next automatic id (init with id < 0): -1, -2, ...; back to -1 should it ever turn positive (label ours).
int ui_next_auto_id = -1;                             // 0x5ac8dc

// 0x542cd0  vtable 0x581ea0 slot 0 (sum.symbols ui_element__free_tooltips). Confidence: likely (the
// unlink order). Frees every tooltip; the element counts as not initialised afterwards.
// 0x543010 ui_tooltip__scalar_deleting_dtor: the `delete t` helper (compiler-generated). Confidence: exact.
void ui_element::dtor()
{
    ui_tooltip *t;

    while ((t = tooltips.next) != &tooltips) {
        t->prev->next = t->next;
        t->next->prev = t->prev;
        t->next = NULL;
        t->prev = NULL;
        delete t;
    }
    initialized = 0;
}

// 0x542d20  Confidence: exact.
ui_element::ui_element()
{
    initialized = 0;
    tooltips.prev = &tooltips;
    tooltips.next = &tooltips;
}

// 0x542d40  Confidence: exact (the EH frame destroys the tooltips sentinel, a ui_tooltip member, D4707).
ui_element::~ui_element()
{
    ui_element::dtor();
}

// 0x542da0  Confidence: exact (the store order).
void ui_element::init(int kind, int x, int y, int w, int h, int id)
{
    if (id < 0) {
        id = ui_next_auto_id--;
        if (ui_next_auto_id > 0)
            ui_next_auto_id = -1;
    }
    name[0] = 0;
    this->kind = kind;
    this->x = x;
    this->y = y;
    this->w = w;
    callback = NULL;
    disabled = 0;
    hidden = 0;
    this->h = h;
    this->id = id;
    initialized = 1;
}

// 0x542e10  Confidence: exact.
int ui_element::get_kind()
{
    return kind;
}

// 0x542e20  Confidence: exact. No caller found.
void ui_element::set_position(int x, int y)
{
    this->x = x;
    this->y = y;
}

// 0x542e40  Confidence: exact.
void ui_element::get_position(int *x, int *y)
{
    *x = this->x;
    *y = this->y;
}

// 0x542e60  Confidence: exact.
void ui_element::set_size(int w, int h)
{
    this->w = w;
    this->h = h;
}

// 0x542e80  Confidence: exact.
void ui_element::get_size(int *w, int *h)
{
    *w = this->w;
    *h = this->h;
}

// 0x542ea0  Confidence: exact. Hiding the focused gadget takes the focus away first (event 4).
void ui_element::set_hidden(bool hide)
{
    hidden = hide;
    if (ui_current_gadget == this && hidden == true) {
        ui_current_gadget = NULL;
        send_event(4, UI_USE_ID);
    }
}

// 0x542ed0  Confidence: exact.
void ui_element::set_visible(bool show)
{
    hidden = !show;
    if (ui_current_gadget == this && hidden == true) {
        ui_current_gadget = NULL;
        send_event(4, UI_USE_ID);
    }
}

// 0x542f10  vtable slot 2. Confidence: exact. Event 9 when disabled, 10 when enabled.
void ui_element::set_disabled(bool d)
{
    if (disabled == d)
        return;
    disabled = d;
    if (ui_current_gadget == this && disabled == true) {
        ui_current_gadget = NULL;
        send_event(4, UI_USE_ID);
    }
    if (disabled)
        send_event(9, UI_USE_ID);
    else
        send_event(10, UI_USE_ID);
}

// 0x542f70  vtable slot 3. Confidence: exact.
void ui_element::set_enabled(bool enabled)
{
    if (disabled != enabled)
        return;
    disabled = !enabled;
    if (ui_current_gadget == this && disabled == true) {
        ui_current_gadget = NULL;
        send_event(4, UI_USE_ID);
    }
    if (disabled)
        send_event(9, UI_USE_ID);
    else
        send_event(10, UI_USE_ID);
}

// 0x542fe0  Confidence: exact.
bool ui_element::is_hidden()
{
    return hidden;
}

// 0x542ff0  Confidence: exact.
bool ui_element::is_shown()
{
    return !hidden;
}

// 0x543000  vtable slot 4. Confidence: exact.
bool ui_element::is_disabled()
{
    return disabled;
}
