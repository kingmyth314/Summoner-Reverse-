// window.cpp - Sum.exe v1.40, code 0x528430-0x52bf92 (vsdk\ui\window.cpp), plus the two compiler-generated
// COMDATs the linker kept from this object (0x52bfa0, 0x52bfc0; see NOTES.md).
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966),
// /O2. Confidence per function: exact = expect identical code, likely = same semantics, small layout or
// scheduling risk, unsure = the source shape is a guess.
//
// The unit builds windows (screens, dialogs) from UI table files: ui_load_table reads the window header and
// hands each "#Button", "#Checkbox", ... block to the matching parser, which creates the gadget and reads its
// optional "+Field:" lines. The rest of the unit is the ui_window class: list insertion, destruction,
// keyboard navigation, drawing and per-frame input.
//
// String literals: no pooling in this object (every "," and "+Font:" has its own copy, .data
// 0x5a8b6c-0x5a95e3). The nine "" literals are in .bss (0x2cbf560-0x2cbf580), one per use, as in the
// gr_font.cpp pilot (all-zero literals land in .bss).

#include <string.h>
#include <stdio.h>
#include "vsdk/ui/window.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/console.h"
#include "vsdk/os/key.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/bmpman/bmpman.h"

// ---- .bss (0x2cbf554-0x2cbf55f) -------------------------------------------------------------------
ui_gadget *ui_nav_best_gadget;      // 0x2cbf554 read and written by ui_gadget__find_nearest_* too
int ui_nav_best_distance;           // 0x2cbf558
bool ui_window_processing;          // 0x2cbf55c ui_window::process is running (dtor defers)

// 0x528430  Confidence: exact (ctrl/shift stores before the Expect calls).
// "+Hotkey:" value: optional "Ctrl+", "Shift+", "Alt+" prefixes and a key name looked up in key_names.
// "Alt+" sets the shift bit (0x1000) like "Shift+"; there is no alt bit. A name that is not found gives
// 256. Thiscall on the window, which it does not use.
int ui_window::parse_hotkey(TextFile *tf)
{
    bool ctrl = false;
    bool shift = false;
    char line[256];
    char key[256];

    for (;;) {
        if (tf->Accept("Ctrl")) {
            ctrl = true;
            tf->Expect("+");
        } else if (tf->Accept("Shift")) {
            shift = true;
            tf->Expect("+");
        } else if (tf->Accept("Alt")) {
            shift = true;
            tf->Expect("+");
        } else {
            break;
        }
    }
    tf->SkipWhitespace();
    tf->ReadLine(line, 256);
    trim_whitespace(key, line, 255);

    int i;
    for (i = 0; i < 256; i++) {
        if (!_stricmp(key, key_names[i]))
            break;
    }
    if (ctrl)
        i |= KEY_CTRLED;
    if (shift)
        i |= KEY_SHIFTED;
    return i;
}

// 0x528530  Confidence: exact (the eol pair is two byte stores in the binary; local layout).
// "#Button". Positions in the table are absolute: the parent's screen position is subtracted. Inside a
// slider (type 7) the button is created in the slider. "+State bitmap:" takes the file, then a comma list
// of states; the bitmap is loaded once for all of them. A state name that matches nothing reuses the
// previous target (NULL on the first).
ui_button *ui_window::parse_button(TextFile *tf, ui_element *parent)
{
    int flags = 0;
    int parent_y = 0;
    int parent_x = 0;
    int type = 0;
    char eol[2] = { '\r', '\0' };
    char name[64];
    char buf[64];
    char filename[260];

    int kind = parent->get_kind();
    switch (kind) {
    case 0:
        type = ((ui_gadget *)parent)->get_type();
        parent_x = ((ui_gadget *)parent)->get_screen_x(0);
        parent_y = ((ui_gadget *)parent)->get_screen_y(0);
        break;
    case 1:
        parent_x = x;
        parent_y = y;
        break;
    }

    if (tf->Accept(":"))
        tf->ParseString(name, 64, '"', '"');
    else
        strcpy(name, "");
    tf->Expect("$Id:");
    int id = tf->ParseInt();
    tf->Expect("$Position:");
    int gx = tf->ParseInt() - parent_x;
    tf->Expect(",");
    int gy = tf->ParseInt() - parent_y;
    tf->Expect("$Size:");
    int gw = tf->ParseInt();
    tf->Expect(",");
    int gh = tf->ParseInt();
    if (tf->Accept("+Drag and drop"))
        flags |= 0x10;
    if (tf->Accept("+Repeats"))
        flags |= 0x2;
    if (tf->Accept("+Always use face color"))
        flags |= 0x8;
    if (tf->Accept("+Use right mouse button"))
        flags |= 0x400;

    ui_button *b = new ui_button;
    if (kind == 0) {
        if (type == UI_TYPE_CONTAINER) {
            b->create(this, gx, gy, gw, gh, id, flags, true);
            b->set_parent((ui_gadget *)parent);
        } else if (type == UI_TYPE_SLIDER) {
            b->create_in_parent((ui_gadget *)parent, gx, gy, gw, gh, id, flags, true);
        }
    } else if (kind == 1) {
        b->create(this, gx, gy, gw, gh, id, flags, true);
    }
    strcpy(b->name, name);

    if (tf->Accept("+Text:")) {
        tf->ParseString(buf, 64, '"', '"');
        b->set_text(buf);
    }
    if (tf->Accept("+Font:"))
        b->font = tf->ParseInt();
    int i = 0;
    while (tf->Accept("+Hotkey:")) {
        b->set_hotkey(parse_hotkey(tf), i);
        i++;
    }
    if (tf->Accept("+Hotspot:"))
        b->set_hotspot(tf->ParseInt());
    while (1) {
        if (!tf->Accept("+State bitmap:"))
            break;
        ui_state_bitmap *state = NULL;
        int bm = -1;
        tf->ParseString(filename, 260, '"', '"');
        while (tf->Accept(",")) {
            tf->SkipWhitespace();
            tf->CopyUntil(buf, 64, ",", eol);
            if (!_stricmp(buf, "Normal"))
                state = &b->state_normal;
            else if (!_stricmp(buf, "Highlighted"))
                state = &b->state_highlighted;
            else if (!_stricmp(buf, "Pressed"))
                state = &b->state_pressed;
            else if (!_stricmp(buf, "Disabled"))
                state = &b->state_disabled;
            else if (!_stricmp(buf, "Drag"))
                state = &b->state_drag;
            if (bm == -1)
                bm = bm_load(filename, -1, -1);
            state->set(bm);
        }
    }
    if (tf->Accept("+Tooltip:")) {
        tf->ParseString(buf, 64, '"', '"');
        tf->Expect(",");
        int tx = tf->ParseInt();
        tf->Expect(",");
        int ty = tf->ParseInt();
        int delay = tf->Accept(",") ? tf->ParseInt() : -1;
        b->set_tooltip(buf, tx, ty, delay);
    }
    return b;
}

// 0x528a40  Confidence: exact.
// "#Checkbox". Always created in the window, then moved into a container parent. Each "+State bitmap:"
// line names one state.
ui_checkbox *ui_window::parse_checkbox(TextFile *tf, ui_element *parent)
{
    int type = 0;
    int parent_y = 0;
    int label_x = 0;
    int label_y = 0;
    int font = -1;
    int parent_x = 0;
    bool checked = false;
    char name[64];
    char buf[64];
    char filename[260];

    int kind = parent->get_kind();
    switch (kind) {
    case 0:
        type = ((ui_gadget *)parent)->get_type();
        parent_x = ((ui_gadget *)parent)->get_screen_x(0);
        parent_y = ((ui_gadget *)parent)->get_screen_y(0);
        break;
    case 1:
        parent_x = x;
        parent_y = y;
        break;
    }

    if (tf->Accept(":"))
        tf->ParseString(name, 64, '"', '"');
    else
        strcpy(name, "");
    tf->Expect("$Id:");
    int id = tf->ParseInt();
    tf->Expect("$Position:");
    int gx = tf->ParseInt() - parent_x;
    tf->Expect(",");
    int gy = tf->ParseInt() - parent_y;
    tf->Expect("$Box size:");
    int box_w = tf->ParseInt();
    tf->Expect(",");
    int box_h = tf->ParseInt();
    tf->Expect("$Label:");
    tf->ParseString(buf, 64, '"', '"');
    if (tf->Accept("+Initially checked"))
        checked = true;
    if (tf->Accept("+Font:"))
        font = tf->ParseInt();
    if (tf->Accept("+Label offset:")) {
        label_x = tf->ParseInt();
        tf->Expect(",");
        label_y = tf->ParseInt();
    }

    ui_checkbox *cb = new ui_checkbox;
    cb->create(this, gx, gy, box_w, box_h, buf, label_x, label_y, font, checked, id, true);
    if (kind == 0 && type == UI_TYPE_CONTAINER)
        cb->set_parent((ui_gadget *)parent);
    strcpy(cb->name, name);

    int i = 0;
    while (tf->Accept("+Hotkey:")) {
        cb->set_hotkey(parse_hotkey(tf), i);
        i++;
    }
    while (tf->Accept("+State bitmap:")) {
        tf->ParseString(filename, 260, '"', '"');
        if (tf->Accept(",")) {
            tf->SkipWhitespace();
            tf->ReadLine(buf, 64);
            if (!_stricmp(buf, "Unchecked"))
                cb->bm_unchecked = bm_load(filename, -1, -1);
            else if (!_stricmp(buf, "Checked"))
                cb->bm_checked = bm_load(filename, -1, -1);
            else if (!_stricmp(buf, "Pressed"))
                cb->bm_pressed = bm_load(filename, -1, -1);
            else if (!_stricmp(buf, "Disabled"))
                cb->bm_disabled = bm_load(filename, -1, -1);
        }
    }
    if (tf->Accept("+Tooltip:")) {
        tf->ParseString(buf, 64, '"', '"');
        tf->Expect(",");
        int tx = tf->ParseInt();
        tf->Expect(",");
        int ty = tf->ParseInt();
        int delay = tf->Accept(",") ? tf->ParseInt() : -1;
        cb->set_tooltip(buf, tx, ty, delay);
    }
    return cb;
}

// 0x528ec0  Confidence: exact.
// "#Container": a group, then its gadgets up to "$Inventory end" (the block markers are named
// "$Inventory start" / "$Inventory end"). "+Tooltip:" lines are only read after a "+Mask bitmap:".
ui_container *ui_window::parse_container(TextFile *tf, ui_element *parent)
{
    int type = 0;
    int parent_x = 0;
    int parent_y = 0;
    char name[64];
    char buf[64];
    char filename[260];

    int kind = parent->get_kind();
    switch (kind) {
    case 0:
        type = ((ui_gadget *)parent)->get_type();
        parent_x = ((ui_gadget *)parent)->get_screen_x(0);
        parent_y = ((ui_gadget *)parent)->get_screen_y(0);
        break;
    case 1:
        parent_x = x;
        parent_y = y;
        break;
    }

    if (tf->Accept(":"))
        tf->ParseString(name, 64, '"', '"');
    else
        strcpy(name, "");
    tf->Expect("$Id:");
    int id = tf->ParseInt();
    tf->Expect("$Position:");
    int gx = tf->ParseInt() - parent_x;
    tf->Expect(",");
    int gy = tf->ParseInt() - parent_y;
    tf->Expect("$Size:");
    int gw = tf->ParseInt();
    tf->Expect(",");
    int gh = tf->ParseInt();

    ui_container *c = new ui_container;
    c->create(this, gx, gy, gw, gh, id, 0, true);
    if (kind == 0 && type == UI_TYPE_CONTAINER)
        c->set_parent((ui_gadget *)parent);
    strcpy(c->name, name);

    if (tf->Accept("+Font:"))
        c->font = tf->ParseInt();
    if (tf->Accept("$Label:")) {
        tf->ParseString(buf, 64, '"', '"');
        c->set_label(buf);
    }
    if (tf->Accept("+Mask bitmap:")) {
        tf->ParseString(filename, 260, '"', '"');
        c->load_mask_bitmap(filename);
        while (tf->Accept("+Tooltip:")) {
            tf->ParseString(buf, 64, '"', '"');
            tf->Expect(",");
            int font = tf->ParseInt();
            tf->Expect(",");
            int tx = tf->ParseInt();
            tf->Expect(",");
            int ty = tf->ParseInt();
            int delay = tf->Accept(",") ? tf->ParseInt() : -1;
            c->set_tooltip(buf, font, tx, ty, delay);
        }
    }

    tf->Expect("$Inventory start");
    for (;;) {
        if (tf->Accept("#Button"))
            parse_button(tf, c);
        else if (tf->Accept("#Checkbox"))
            parse_checkbox(tf, c);
        else if (tf->Accept("#Container"))
            parse_container(tf, c);
        else if (tf->Accept("#Inputbox"))
            parse_inputbox(tf, c);
        else if (tf->Accept("#Listbox"))
            parse_listbox(tf, c);
        else if (tf->Accept("#Radio button"))
            parse_radio_button(tf, c);
        else if (tf->Accept("#Slider"))
            parse_slider(tf, c);
        else if (tf->Accept("#Static"))
            parse_static(tf, c);
        else
            break;
    }
    tf->Expect("$Inventory end");
    return c;
}

// 0x5292f0  Confidence: exact.
// "#Inputbox". "+Password" may be followed by ":" and a string whose first character replaces '*'.
ui_inputbox *ui_window::parse_inputbox(TextFile *tf, ui_element *parent)
{
    int type = 0;
    int parent_x = 0;
    int parent_y = 0;
    int flags = 0;
    char password_char = '*';
    char name[64];
    char buf[64];
    char filename[260];

    int kind = parent->get_kind();
    switch (kind) {
    case 0:
        type = ((ui_gadget *)parent)->get_type();
        parent_x = ((ui_gadget *)parent)->get_screen_x(0);
        parent_y = ((ui_gadget *)parent)->get_screen_y(0);
        break;
    case 1:
        parent_x = x;
        parent_y = y;
        break;
    }

    if (tf->Accept(":"))
        tf->ParseString(name, 64, '"', '"');
    else
        strcpy(name, "");
    tf->Expect("$Id:");
    int id = tf->ParseInt();
    tf->Expect("$Position:");
    int gx = tf->ParseInt() - parent_x;
    tf->Expect(",");
    int gy = tf->ParseInt() - parent_y;
    tf->Expect("$Size:");
    int gw = tf->ParseInt();
    tf->Expect(",");
    int gh = tf->ParseInt();
    if (tf->Accept("+Ignore esc"))
        flags |= 0x2;
    if (tf->Accept("+No scroll"))
        flags |= 0x4;
    if (tf->Accept("+Password")) {
        flags |= 0x1;
        if (tf->Accept(":")) {
            tf->ParseString(buf, 64, '"', '"');
            password_char = buf[0];
        }
    }

    ui_inputbox *ib = new ui_inputbox;
    ib->create(this, gx, gy, gw, gh, id, flags, true);
    if (kind == 0 && type == UI_TYPE_CONTAINER)
        ib->set_parent((ui_gadget *)parent);
    strcpy(ib->name, name);
    ib->password_char = password_char;

    if (tf->Accept("+Valid chars:")) {
        tf->ParseString(buf, 64, '"', '"');
        ib->set_valid_chars(buf);
    }
    if (tf->Accept("+Invalid chars:")) {
        tf->ParseString(buf, 64, '"', '"');
        ib->set_invalid_chars(buf);
    }
    if (tf->Accept("+Max text length:"))
        ib->max_length = tf->ParseInt();
    while (tf->Accept("+State bitmap:")) {
        tf->ParseString(filename, 260, '"', '"');
        if (tf->Accept(",")) {
            tf->SkipWhitespace();
            tf->ReadLine(buf, 64);
            if (!_stricmp(buf, "Normal"))
                ib->bm_normal = bm_load(filename, -1, -1);
            else if (!_stricmp(buf, "Disabled"))
                ib->bm_disabled = bm_load(filename, -1, -1);
        }
    }
    if (tf->Accept("+Margin:"))
        ib->margin = tf->ParseInt();
    if (tf->Accept("+Text:")) {
        tf->ParseString(buf, 64, '"', '"');
        ib->set_text(buf);
    }
    if (tf->Accept("+Font:"))
        ib->font = tf->ParseInt();
    if (tf->Accept("+Text y offset:"))
        ib->text_y = tf->ParseInt();
    if (tf->Accept("+Vertical justification:")) {
        tf->SkipWhitespace();
        tf->ReadLine(buf, 64);
        if (!_stricmp(buf, "Top"))
            ib->v_justify = 0;
        else if (!_stricmp(buf, "Center"))
            ib->v_justify = 1;
        else if (!_stricmp(buf, "Bottom"))
            ib->v_justify = 2;
    }
    if (tf->Accept("+Cursor bitmap:")) {
        tf->ParseString(filename, 260, '"', '"');
        ib->bm_cursor = bm_load(filename, -1, -1);
    }
    if (tf->Accept("+Cursor size:")) {
        ib->cursor_w = tf->ParseInt();
        tf->Expect(",");
        ib->cursor_h = tf->ParseInt();
    }
    if (tf->Accept("+Cursor y offset:"))
        ib->cursor_y = tf->ParseInt();
    if (tf->Accept("+Tooltip:")) {
        tf->ParseString(buf, 64, '"', '"');
        tf->Expect(",");
        int tx = tf->ParseInt();
        tf->Expect(",");
        int ty = tf->ParseInt();
        int delay = tf->Accept(",") ? tf->ParseInt() : -1;
        ib->set_tooltip(buf, tx, ty, delay);
    }
    return ib;
}

// 0x5298c0  Confidence: exact ("+Add both scrollbars" sets only flag 2, like the vertical one; the two
// |= 2 blocks are one in the binary).
// "#Listbox". "+Horizontal scrollbar" / "+Vertical scrollbar" are followed by a slider block parsed with
// the list box as parent (ui_parse_slider creates it as a scroll bar of the list box).
ui_listbox *ui_window::parse_listbox(TextFile *tf, ui_element *parent)
{
    int type = 0;
    int parent_x = 0;
    int parent_y = 0;
    int content_h = 0;
    int flags = 0;
    char name[64];
    char buf[64];
    char filename[260];

    int kind = parent->get_kind();
    switch (kind) {
    case 0:
        type = ((ui_gadget *)parent)->get_type();
        parent_x = ((ui_gadget *)parent)->get_screen_x(0);
        parent_y = ((ui_gadget *)parent)->get_screen_y(0);
        break;
    case 1:
        parent_x = x;
        parent_y = y;
        break;
    }

    if (tf->Accept(":"))
        tf->ParseString(name, 64, '"', '"');
    else
        strcpy(name, "");
    tf->Expect("$Id:");
    int id = tf->ParseInt();
    tf->Expect("$Position:");
    int gx = tf->ParseInt() - parent_x;
    tf->Expect(",");
    int gy = tf->ParseInt() - parent_y;
    tf->Expect("$Size:");
    int gw = tf->ParseInt();
    tf->Expect(",");
    int gh = tf->ParseInt();
    tf->Expect("$Content size:");
    int content_w = tf->ParseInt();
    if (tf->Accept(","))
        content_h = tf->ParseInt();
    if (tf->Accept("+Allow wrap"))
        flags |= 0x1;
    if (tf->Accept("+Sort entries"))
        flags |= 0x40;
    if (tf->Accept("+No select"))
        flags |= 0x8;
    if (tf->Accept("+Scroll by entry"))
        flags |= 0x10;
    if (tf->Accept("+Scroll by pixels"))
        flags |= 0x20;
    if (tf->Accept("+Align view with bottom"))
        flags |= 0x80;
    if (tf->Accept("+No background"))
        flags |= 0x100;
    if (tf->Accept("+Mouseover highlights"))
        flags |= 0x200;
    if (tf->Accept("+Preallocated text"))
        flags |= 0x400;
    if (tf->Accept("+Add horizontal scrollbar"))
        flags |= 0x4;
    else if (tf->Accept("+Add vertical scrollbar"))
        flags |= 0x2;
    else if (tf->Accept("+Add both scrollbars"))
        flags |= 0x2;

    ui_listbox *lb = new ui_listbox;
    lb->create(this, gx, gy, gw, gh, content_w, content_h, id, flags, true);
    if (kind == 0 && type == UI_TYPE_CONTAINER)
        lb->set_parent((ui_gadget *)parent);
    strcpy(lb->name, name);

    if (tf->Accept("+Entry spacing:"))
        lb->entry_spacing = tf->ParseInt();
    if (tf->Accept("+Margin:"))
        lb->margin = tf->ParseInt();
    if (tf->Accept("+Vertical margin:"))
        lb->v_margin = tf->ParseInt();
    if (tf->Accept("+Background bitmap:")) {
        tf->ParseString(filename, 260, '"', '"');
        lb->bm_background = bm_load(filename, -1, -1);
    }
    if (tf->Accept("+Tooltip:")) {
        tf->ParseString(buf, 64, '"', '"');
        tf->Expect(",");
        int tx = tf->ParseInt();
        tf->Expect(",");
        int ty = tf->ParseInt();
        int delay = tf->Accept(",") ? tf->ParseInt() : -1;
        lb->set_tooltip(buf, tx, ty, delay);
    }
    if (tf->Accept("+Horizontal scrollbar"))
        parse_slider(tf, lb);
    if (tf->Accept("+Vertical scrollbar"))
        parse_slider(tf, lb);
    return lb;
}

// 0x529d70  Confidence: exact. VC6 calls get_screen_x / _y before the ParseInt of `ParseInt() - get_screen_x(0)`.
// "#Radio button": only inside a container, so the parent is always a gadget (no kind test).
// D23800: gx / gy as one expression each (no px / py temporaries) and label_x declared before label_y.
ui_radio_button *ui_window::parse_radio_button(TextFile *tf, ui_element *parent)
{
    int label_x = 0;
    int label_y = 0;
    int font = -1;
    char name[64];
    char buf[64];
    char filename[260];

    if (tf->Accept(":"))
        tf->ParseString(name, 64, '"', '"');
    else
        strcpy(name, "");
    tf->Expect("$Id:");
    int id = tf->ParseInt();
    tf->Expect("$Position:");
    int gx = tf->ParseInt() - ((ui_gadget *)parent)->get_screen_x(0);
    tf->Expect(",");
    int gy = tf->ParseInt() - ((ui_gadget *)parent)->get_screen_y(0);
    tf->Expect("$Box size:");
    int box_w = tf->ParseInt();
    tf->Expect(",");
    int box_h = tf->ParseInt();
    tf->Expect("$Label:");
    tf->ParseString(buf, 64, '"', '"');
    if (tf->Accept("+Font:"))
        font = tf->ParseInt();
    if (tf->Accept("+Label offset:")) {
        label_x = tf->ParseInt();
        tf->Expect(",");
        label_y = tf->ParseInt();
    }

    ui_radio_button *rb = new ui_radio_button;
    rb->create((ui_gadget *)parent, gx, gy, box_w, box_h, buf, label_x, label_y, font, id, true);
    strcpy(rb->name, name);

    int i = 0;
    while (tf->Accept("+Hotkey:")) {
        rb->set_hotkey(parse_hotkey(tf), i);
        i++;
    }
    while (tf->Accept("+State bitmap:")) {
        tf->ParseString(filename, 260, '"', '"');
        if (tf->Accept(",")) {
            tf->SkipWhitespace();
            tf->ReadLine(buf, 64);
            if (!_stricmp(buf, "Unchecked"))
                rb->bm_unchecked = bm_load(filename, -1, -1);
            else if (!_stricmp(buf, "Checked"))
                rb->bm_checked = bm_load(filename, -1, -1);
            else if (!_stricmp(buf, "Pressed"))
                rb->bm_pressed = bm_load(filename, -1, -1);
            else if (!_stricmp(buf, "Disabled"))
                rb->bm_disabled = bm_load(filename, -1, -1);
        }
    }
    if (tf->Accept("+Tooltip:")) {
        tf->ParseString(buf, 64, '"', '"');
        tf->Expect(",");
        int tx = tf->ParseInt();
        tf->Expect(",");
        int ty = tf->ParseInt();
        int delay = tf->Accept(",") ? tf->ParseInt() : -1;
        rb->set_tooltip(buf, tx, ty, delay);
    }
    return rb;
}

// 0x52a180  Confidence: exact (two pointer variables for the two constructions: scroll is NULL unless
// "+Scrollbar"; the binary calls create_scrollbar on that variable, which is NULL for a plain slider).
// D23801: scroll = NULL in the else branch (not at its declaration) and parent_x declared first.
// "#Slider". Looks ahead (GetPosition / SkipTo / SetPosition) for the arrow-button blocks before the
// next '#': if they are there, flag 1 is set and they are parsed at the end as buttons of the slider.
// "+Marker bitmap:" and "+Background bitmap:" are read but not used.
ui_slider_base *ui_window::parse_slider(TextFile *tf, ui_element *parent)
{
    int parent_x = 0;
    char marker_bitmap[64] = "";
    int parent_y = 0;
    int marker_pos = 0;
    int positions = 1;
    int flags = 0;
    int type = 0;
    char name[64];
    char buf[64];
    char background[260];
    char background2[260];

    int kind = parent->get_kind();
    switch (kind) {
    case 0:
        type = ((ui_gadget *)parent)->get_type();
        parent_x = ((ui_gadget *)parent)->get_screen_x(0);
        parent_y = ((ui_gadget *)parent)->get_screen_y(0);
        break;
    case 1:
        parent_y = y;
        parent_x = x;
        break;
    }

    if (tf->Accept(":"))
        tf->ParseString(name, 64, '"', '"');
    else
        strcpy(name, "");
    tf->Expect("$Id:");
    int id = tf->ParseInt();
    tf->Expect("$Position:");
    int gx = tf->ParseInt() - parent_x;
    tf->Expect(",");
    int gy = tf->ParseInt() - parent_y;
    tf->Expect("$Size:");
    int gw = tf->ParseInt();
    tf->Expect(",");
    int gh = tf->ParseInt();
    bool horizontal;
    if (tf->Accept("$Horizontal")) {
        horizontal = true;
    } else {
        tf->Expect("$Vertical");
        horizontal = false;
    }
    tf->Expect("$Marker size:");
    int marker_size = tf->ParseInt();
    bool scrollbar = false;
    if (tf->Accept("+Scrollbar"))
        scrollbar = true;
    if (tf->Accept("+Dot slider"))
        flags |= 0x4;
    if (tf->Accept("+No process"))
        flags |= 0x8;
    if (tf->Accept("+Positions:"))
        positions = tf->ParseInt();
    if (tf->Accept("+Marker bitmap:"))
        tf->ParseString(marker_bitmap, 64, '"', '"');
    if (tf->Accept("+Marker position:"))
        marker_pos = tf->ParseInt();

    TextFilePosition pos;
    tf->GetPosition(&pos);
    if (horizontal) {
        if (tf->SkipTo("+Left button", "#"))
            flags |= 0x1;
    } else if (tf->SkipTo("+Up button", "#")) {
        flags |= 0x1;
    }
    tf->SetPosition(&pos);

    ui_slider *scroll;
    ui_slider_base *s;
    if (scrollbar) {
        s = scroll = new ui_slider;
    } else {
        scroll = NULL;
        s = new ui_slider_base;
    }

    if (kind == 0) {
        if (type == UI_TYPE_CONTAINER) {
            s->create(this, gx, gy, gw, gh, horizontal, marker_size, positions, id, flags, true);
            s->set_parent((ui_gadget *)parent);
        } else if (type == UI_TYPE_LISTBOX) {
            scroll->create_scrollbar((ui_gadget *)parent, horizontal, marker_size, id, flags, true);
        }
    } else if (kind == 1) {
        s->create(this, gx, gy, gw, gh, horizontal, marker_size, positions, id, flags, true);
    }
    strcpy(s->name, name);

    if (tf->Accept("+Background bitmap:")) {
        tf->ParseString(background, 260, '"', '"');
        if (tf->Accept(","))
            tf->ParseString(background2, 260, '"', '"');
    }
    if (tf->Accept("+Tooltip:")) {
        tf->ParseString(buf, 64, '"', '"');
        tf->Expect(",");
        int tx = tf->ParseInt();
        tf->Expect(",");
        int ty = tf->ParseInt();
        int delay = tf->Accept(",") ? tf->ParseInt() : -1;
        s->set_tooltip(buf, tx, ty, delay);
    }
    if (flags & 0x1) {
        if (horizontal) {
            tf->Expect("+Left button");
            s->dec_button = parse_button(tf, s);
            tf->Expect("+Right button");
            s->inc_button = parse_button(tf, s);
        } else {
            tf->Expect("+Up button");
            s->dec_button = parse_button(tf, s);
            tf->Expect("+Down button");
            s->inc_button = parse_button(tf, s);
        }
    }
    s->set_pos(marker_pos);
    return s;
}

// 0x52a700  Confidence: exact.
// "#Static": text and/or bitmap label.
ui_static *ui_window::parse_static(TextFile *tf, ui_element *parent)
{
    int type = 0;
    int parent_x = 0;
    int parent_y = 0;
    char name[64];
    char buf[64];
    char filename[260];

    int kind = parent->get_kind();
    switch (kind) {
    case 0:
        type = ((ui_gadget *)parent)->get_type();
        parent_x = ((ui_gadget *)parent)->get_screen_x(0);
        parent_y = ((ui_gadget *)parent)->get_screen_y(0);
        break;
    case 1:
        parent_x = x;
        parent_y = y;
        break;
    }

    if (tf->Accept(":"))
        tf->ParseString(name, 64, '"', '"');
    else
        strcpy(name, "");
    tf->Expect("$Id:");
    int id = tf->ParseInt();
    tf->Expect("$Position:");
    int gx = tf->ParseInt() - parent_x;
    tf->Expect(",");
    int gy = tf->ParseInt() - parent_y;
    tf->Expect("$Size:");
    int gw = tf->ParseInt();
    tf->Expect(",");
    int gh = tf->ParseInt();

    ui_static *st = new ui_static;
    st->create(this, gx, gy, gw, gh, id, true);
    if (kind == 0 && type == UI_TYPE_CONTAINER)
        st->set_parent((ui_gadget *)parent);
    strcpy(st->name, name);

    if (tf->Accept("+Horizontal justification:")) {
        tf->SkipWhitespace();
        tf->ReadLine(buf, 64);
        if (!_stricmp(buf, "Left"))
            st->h_justify = 0;
        else if (!_stricmp(buf, "Center"))
            st->h_justify = 1;
        else if (!_stricmp(buf, "Right"))
            st->h_justify = 2;
    }
    if (tf->Accept("+Vertical justification:")) {
        tf->SkipWhitespace();
        tf->ReadLine(buf, 64);
        if (!_stricmp(buf, "Top"))
            st->v_justify = 0;
        else if (!_stricmp(buf, "Center"))
            st->v_justify = 1;
        else if (!_stricmp(buf, "Bottom"))
            st->v_justify = 2;
    }
    if (tf->Accept("+Font:"))
        st->font = tf->ParseInt();
    if (tf->Accept("+Text:")) {
        tf->ParseString(buf, 64, '"', '"');
        st->set_text(buf);
    }
    if (tf->Accept("+Text x position:"))
        st->text_x = tf->ParseInt();
    if (tf->Accept("+Text y position:"))
        st->text_y = tf->ParseInt();
    while (tf->Accept("+State bitmap:")) {
        tf->ParseString(filename, 260, '"', '"');
        if (tf->Accept(",")) {
            tf->SkipWhitespace();
            tf->ReadLine(buf, 64);
            if (!_stricmp(buf, "Normal"))
                st->state_normal.set(bm_load(filename, -1, -1));
            else if (!_stricmp(buf, "Disabled"))
                st->state_disabled.set(bm_load(filename, -1, -1));
        }
    }
    if (tf->Accept("+Tooltip:")) {
        tf->ParseString(buf, 64, '"', '"');
        tf->Expect(",");
        int tx = tf->ParseInt();
        tf->Expect(",");
        int ty = tf->ParseInt();
        int delay = tf->Accept(",") ? tf->ParseInt() : -1;
        st->set_tooltip(buf, tx, ty, delay);
    }
    return st;
}

// 0x52abc0  Confidence: exact.
// Forgets the bitmaps without unlocking or releasing them.
void ui_window::clear_bitmaps()
{
    if (mask_bm >= 0)
        mask_bm = -1;
    if (background_bm >= 0)
        background_bm = -1;
}

// 0x52abf0  Confidence: likely (the distance expressions: the binary computes y - g->h - g->y + h; D15407: one
// sw / sh pair for both branches gives the binary's frame slots; the registers of the left case still differ).
// Arrow keys move the keyboard selection to the nearest gadget in that direction (ui_gadget find_nearest_*
// from the selection's edge). Without a selection the gadget nearest the window's opposite edge is taken;
// that pass reads the gadget's own rectangle and throws away the get_screen_* / get_size results. Returns
// the key if it was not an arrow, else 0.
int ui_window::handle_arrow_key(int key)
{
    int sw, sh;
    ui_nav_best_distance = 999999999;
    ui_nav_best_gadget = NULL;

    if (selected) {
        selected->get_size(&sw, &sh);
        int sx = selected->get_screen_x(0);
        int sy = selected->get_screen_y(0);
        int right = sx + sw;
        int bottom = sy + sh;
        int cx = (sx + right) / 2;
        int cy = (bottom + sy) / 2;
        switch (key) {
        case KEY_UP:
            nav_up(cx, sy);
            break;
        case KEY_DOWN:
            nav_down(cx, bottom);
            break;
        case KEY_LEFT:
            nav_left(sx, cy);
            break;
        case KEY_RIGHT:
            nav_right(right, cy);
            break;
        default:
            return key;
        }
    } else {
        ui_gadget *g;
        int d;
        switch (key) {
        case KEY_UP:
            for (g = gadgets.window_next; g != &gadgets; g = g->window_next) {
                g->get_screen_y(0);
                g->get_size(&sw, &sh);
                d = (y + h) - (g->y + g->h);
                if (d < ui_nav_best_distance) {
                    ui_nav_best_distance = d;
                    ui_nav_best_gadget = g;
                }
            }
            break;
        case KEY_DOWN:
            for (g = gadgets.window_next; g != &gadgets; g = g->window_next) {
                g->get_screen_y(0);
                d = g->y - y;
                if (d < ui_nav_best_distance) {
                    ui_nav_best_distance = d;
                    ui_nav_best_gadget = g;
                }
            }
            break;
        case KEY_LEFT:
            for (g = gadgets.window_next; g != &gadgets; g = g->window_next) {
                g->get_screen_x(0);
                g->get_size(&sw, &sh);
                // D21503 (closer): w first, then x added in its own statement (the binary's operand order).
                d = w - g->w - g->x;
                d += x;
                if (d < ui_nav_best_distance) {
                    ui_nav_best_distance = d;
                    ui_nav_best_gadget = g;
                }
            }
            break;
        case KEY_RIGHT:
            for (g = gadgets.window_next; g != &gadgets; g = g->window_next) {
                g->get_screen_x(0);
                d = g->x - x;
                if (d < ui_nav_best_distance) {
                    ui_nav_best_distance = d;
                    ui_nav_best_gadget = g;
                }
            }
            break;
        default:
            return key;
        }
    }
    if (ui_nav_best_gadget)
        selected = ui_nav_best_gadget;
    return 0;
}

// 0x52aea0  Confidence: exact.
// With "+Allow wrap", a miss searches again from the bottom edge.
void ui_window::nav_up(int px, int py)
{
    if (first_gadget)
        first_gadget->find_nearest_up(px, py);
    if (!ui_nav_best_gadget && first_gadget && (flags & UI_WINDOW_ALLOW_WRAP))
        first_gadget->find_nearest_up(px, y + h - 1);
}

// 0x52aef0  Confidence: exact.
void ui_window::nav_down(int px, int py)
{
    if (first_gadget)
        first_gadget->find_nearest_down(px, py);
    if (!ui_nav_best_gadget && first_gadget && (flags & UI_WINDOW_ALLOW_WRAP))
        first_gadget->find_nearest_down(px, y);
}

// 0x52af40  Confidence: exact.
void ui_window::nav_left(int px, int py)
{
    if (first_gadget)
        first_gadget->find_nearest_left(px, py);
    if (!ui_nav_best_gadget && first_gadget && (flags & UI_WINDOW_ALLOW_WRAP))
        first_gadget->find_nearest_left(x + w - 1, py);
}

// 0x52af90  Confidence: exact.
void ui_window::nav_right(int px, int py)
{
    if (first_gadget)
        first_gadget->find_nearest_right(px, py);
    if (!ui_nav_best_gadget && first_gadget && (flags & UI_WINDOW_ALLOW_WRAP))
        first_gadget->find_nearest_right(x, py);
}

// 0x52afe0  Confidence: exact (a bare ret with no reference in the image; an empty function of this
// unit, name and signature unknown).
void ui_stub_ret_52afe0()
{
}

// 0x52aff0  Confidence: exact (VC6 byte match; D15400: no return after the if, so a NULL callback returns the
// tested pointer's EAX, whose AL is 0 = false; VC6 warns C4715).
// Vtable slot 1. -99999 means "use the window's id".
bool ui_window::send_event(int event, int value)
{
    if (value == UI_USE_ID)
        value = id;
    if (callback)
        return callback(event, value);
}

// 0x52b020  Confidence: exact (store order: the vtable store sits between the list and the other
// fields in the binary).
ui_window::ui_window()
{
    mask_bm = -1;
    background_bm = -1;
    gadgets.window_next = &gadgets;
    gadgets.window_prev = &gadgets;
    mask_data = NULL;
    hidden = 0;
    next = NULL;
}

// 0x52b0a0  Confidence: exact.
// Calls ui_window::dtor directly (virtual calls are static in a destructor), then ~ui_gadget on the
// sentinel and ~ui_element.
ui_window::~ui_window()
{
    dtor();
}

// 0x52b100  Confidence: unsure (the two list walks: each calls get_flags on every window and ignores
// the result, so a window is always inserted last; the lost test was presumably "stop at the first modal
// window").
// A modal window clears the modeless flag and goes last. Otherwise the walk below ends at the sentinel.
void ui_window::init(int x, int y, int w, int h, int id, int flags)
{
    ui_init();
    ui_element::init(1, x, y, w, h, id);
    if (flags & UI_WINDOW_MODAL)
        flags &= ~UI_WINDOW_MODELESS;
    this->flags = flags;
    group = ui_group_depth;
    first_gadget = NULL;
    mouse_over = NULL;
    selected = NULL;
    colors = &ui_window_colors.c0;

    if (flags & UI_WINDOW_MODAL) {
        next = &ui_window_sentinel;
        prev = ui_window_sentinel.prev;
        ui_window_sentinel.prev->next = this;
        ui_window_sentinel.prev = this;
        return;
    }

    ui_window *win = ui_window_sentinel.next;
    if (flags & UI_WINDOW_MODELESS) {
        while (win != &ui_window_sentinel) {
            if (win->get_flags() & UI_WINDOW_MODAL) {
            }
            win = win->next;
        }
    } else {
        while (win != &ui_window_sentinel) {
            if (win->get_flags() & UI_WINDOW_MODAL) {
            }
            win = win->next;
        }
    }
    ui_window *p = win->prev;
    prev = p;
    win->prev = this;
    p->next = this;
    next = win;
}

// 0x52b200  Confidence: exact (one 64-byte buffer for the name, both bitmaps and the tooltips, as in the
// binary's frame; if +Name: is missing, the uninitialised buffer is copied).
// Reads a UI table: the window rectangle, name, id and flags, its bitmaps and tooltips, then the gadget
// blocks up to "#End". A missing file only formats a message into string_buffer and parsing goes on.
void ui_window::load_table(const char *filename, int path_id)
{
    int id = UI_NO_ID;
    int flags = 1;
    TextFile tf;
    char buf[64];

    if (tf.Open(filename, path_id))
        sprintf(string_buffer, "unable to open UI table file \"%s\"\n", filename);
    tf.Expect("$Position:");
    int wx = tf.ParseInt();
    tf.Expect(",");
    int wy = tf.ParseInt();
    tf.Expect("$Size:");
    int ww = tf.ParseInt();
    tf.Expect(",");
    int wh = tf.ParseInt();
    if (tf.Accept("+Name:"))
        tf.ParseString(buf, 64, '"', '"');
    if (tf.Accept("+Id:"))
        id = tf.ParseInt();
    if (tf.Accept("+Modal dialog"))
        flags |= UI_WINDOW_MODAL;
    if (tf.Accept("+Modeless dialog"))
        flags |= UI_WINDOW_MODELESS;
    if (tf.Accept("+Allow arrows"))
        flags |= UI_WINDOW_ALLOW_ARROWS;
    if (tf.Accept("+Allow wrap"))
        flags |= UI_WINDOW_ALLOW_WRAP;
    if (tf.Accept("+Always a selection"))
        flags |= UI_WINDOW_ALWAYS_SEL;

    init(wx, wy, ww, wh, id, flags);
    strcpy(name, buf);

    if (tf.Accept("+Background bitmap:")) {
        tf.ParseString(buf, 64, '"', '"');
        load_background_bitmap(buf);
    }
    if (tf.Accept("+Mask bitmap:")) {
        tf.ParseString(buf, 64, '"', '"');
        load_mask_bitmap(buf);
        while (tf.Accept("+Tooltip:")) {
            tf.ParseString(buf, 64, '"', '"');
            tf.Expect(",");
            int font = tf.ParseInt();
            tf.Expect(",");
            int tx = tf.ParseInt();
            tf.Expect(",");
            int ty = tf.ParseInt();
            int delay = tf.Accept(",") ? tf.ParseInt() : -1;
            set_tooltip(buf, font, tx, ty, delay);
        }
    }

    for (;;) {
        if (tf.Accept("#Button"))
            parse_button(&tf, this);
        else if (tf.Accept("#Checkbox"))
            parse_checkbox(&tf, this);
        else if (tf.Accept("#Container"))
            parse_container(&tf, this);
        else if (tf.Accept("#Inputbox"))
            parse_inputbox(&tf, this);
        else if (tf.Accept("#Listbox"))
            parse_listbox(&tf, this);
        else if (tf.Accept("#Slider"))
            parse_slider(&tf, this);
        else if (tf.Accept("#Static"))
            parse_static(&tf, this);
        else
            break;
    }
    tf.Expect("#End");
    tf.Close();
}

// 0x52b630  Confidence: exact (the unlink store order is read from the code; it may be an inline list
// helper in the original).
// Vtable slot 0 (ui_window__destroy). While ui_window::process runs it only sets UI_WINDOW_DESTROY and
// process calls it again at the end. Otherwise every gadget is unlinked and destroyed (owned ones also
// deleted), the window leaves the window list and its tooltips are freed. The bitmaps are forgotten, not
// released.
void ui_window::dtor()
{
    if (ui_window_processing) {
        flags |= UI_WINDOW_DESTROY;
        return;
    }
    clear_bitmaps();
    while (gadgets.window_next != &gadgets) {
        ui_gadget *g = gadgets.window_next;
        ui_gadget *gp = g->window_prev;
        ui_gadget *gn = g->window_next;
        g->window_next = NULL;
        g->window_prev = NULL;
        gp->window_next = gn;
        gn->window_prev = gp;
        if (g->is_owned()) {
            g->dtor();
            delete g;
        } else {
            g->dtor();
        }
    }
    ui_window *wn = next;
    if (wn) {
        ui_window *wp = prev;
        next = NULL;
        wn->prev = wp;
        prev = NULL;
        wp->next = wn;
    }
    ui_element::dtor();
}

// 0x52b700  Confidence: exact.
int ui_window::get_flags()
{
    return flags;
}

// 0x52b710  Confidence: exact (VC6 byte match, match23; was likely; store order).
// Appends a tooltip to the window's list (same node layout as the gadget set_tooltip functions).
void ui_window::set_tooltip(const char *text, int font, int x, int y, int delay)
{
    ui_tooltip *t = new ui_tooltip;
    t->owner = this;
    strcpy(t->text, text);
    t->delay = delay;
    t->x = x;
    t->y = y;
    t->font = font;
    t->colors = &ui_tooltip_colors.c0;
    t->next = (ui_tooltip *)&tooltips;
    t->prev = tooltips.prev;
    tooltips.prev->next = t;
    tooltips.prev = t;
}

// 0x52b7a0  Confidence: exact.
ui_gadget *ui_window::get_selected_gadget()
{
    return selected;
}

// 0x52b7b0  Confidence: exact.
ui_gadget *ui_window::get_mouse_over_gadget()
{
    return mouse_over;
}

// 0x52b7c0  Confidence: exact.
bool ui_window::contains_point(int px, int py)
{
    if (!is_disabled()) {
        int dx = px - x;
        int dy = py - y;
        if (dx >= 0 && dy >= 0 && dx < w && dy < h)
            return true;
    }
    return false;
}

// 0x52b800  Confidence: exact.
// The last top-level gadget that reports a hit wins.
ui_gadget *ui_window::gadget_at(int px, int py)
{
    ui_gadget *found = NULL;
    for (ui_gadget *g = first_gadget; g; g = g->next_sibling) {
        ui_gadget *hit = g->hit_test(px, py);
        if (hit)
            found = hit;
    }
    return found;
}

// 0x52b850  Confidence: exact.
// The last gadget with that id wins; negative ids are never found.
ui_gadget *ui_window::find_gadget_by_id(int id)
{
    ui_gadget *found = NULL;
    if (id < 0)
        return NULL;
    for (ui_gadget *g = gadgets.window_next; g != &gadgets; g = g->window_next) {
        if (g->id == id)
            found = g;
    }
    return found;
}

// 0x52b890  Confidence: exact.
ui_gadget *ui_window::find_gadget_by_name(const char *name)
{
    for (ui_gadget *g = gadgets.window_next; g != &gadgets; g = g->window_next) {
        if (!_stricmp(g->name, name))
            return g;
    }
    return NULL;
}

// 0x52b8e0  Confidence: exact (the message and the path are this function's own literals; the
// error macro repeats only DebugError).
void ui_window::load_mask_bitmap(const char *name)
{
    int bm = bm_load(name, -1, -1);
    if (bm < 0) {
        sprintf(string_buffer, "Could not load in \"%s\"!", name);
        VSDK_ERROR("D:\\projects\\Summoner\\pccode\\vsdk\\ui\\window.cpp", 2532, string_buffer);
    }
    set_mask_bitmap(bm, name);
}

// 0x52b930  Confidence: exact.
// Unlocks the old mask bitmap; the size it reads is not kept and mask_data is not set here.
void ui_window::set_mask_bitmap(int bm, const char *name)
{
    int bw, bh;
    if (bm != mask_bm) {
        if (mask_bm >= 0)
            bm_unlock(mask_bm);
        mask_bm = bm;
        bm_get_dimensions(bm, &bw, &bh);
    }
}

// 0x52b970  Confidence: exact.
void ui_window::load_background_bitmap(const char *name)
{
    int bm = bm_load(name, -1, -1);
    if (bm < 0) {
        sprintf(string_buffer, "Could not load in \"%s\"!", name);
        VSDK_ERROR("D:\\projects\\Summoner\\pccode\\vsdk\\ui\\window.cpp", 2599, string_buffer);
    }
    set_background_bitmap(bm, name);
}

// 0x52b9c0  Confidence: exact.
void ui_window::set_background_bitmap(int bm, const char *name)
{
    int bw, bh;
    if (bm != background_bm) {
        background_bm = bm;
        bm_get_dimensions(bm, &bw, &bh);
    }
}

// 0x52b9f0  Confidence: exact (switch on the border style; case order).
// Background bitmap stretched over the window, else border style 1 (plain face), 2 (face and a 3-pixel
// bevel) or 3 (face and three bevels). The callback gets events 0 and 1 around the gadgets.
void ui_window::draw()
{
    if (is_disabled())
        return;
    gr::reset_clip();
    if (background_bm >= 0) {
        int bw, bh;
        bm_get_dimensions(background_bm, &bw, &bh);
        gr_set_color(255, 255, 255, 255);
        gr_bitmap_stretched(background_bm, x, y, w, h, 0, 0, bw, bh, gr_bitmap_wrap_mode);
    } else {
        switch (flags & UI_WINDOW_BORDER_MASK) {
        case 1:
            gr_set_color_ptr(&colors[2]);
            gr_rect(x, y, w, h, gr_rect_mode, 1.0f);
            break;
        case 2:
            gr_set_color_ptr(&colors[2]);
            gr_rect(x + 3, y + 3, w - 6, h - 6, gr_rect_mode, 1.0f);
            ui_draw_bevel(x, y, w, h, &colors[0], &colors[1], 3, 0);
            break;
        case 3:
            gr_set_color_ptr(&colors[2]);
            gr_rect(x + 5, y + 5, w - 10, h - 10, gr_rect_mode, 1.0f);
            ui_draw_bevel(x, y, w, h, &colors[0], &colors[1], 2, 0);
            ui_draw_bevel(x, y, w, h, &colors[3], &colors[3], 2, 2);
            ui_draw_bevel(x, y, w, h, &colors[1], &colors[0], 1, 4);
            break;
        }
    }
    send_event(UI_EVENT_DRAW_BEGIN, UI_NO_ID);
    for (ui_gadget *g = first_gadget; g; g = g->next_sibling)
        g->draw();
    send_event(UI_EVENT_DRAW_END, UI_NO_ID);
}

// 0x52bc20  Confidence: exact.
// One frame of input. A hidden or disabled window only passes the key through its gadgets. Otherwise the
// selection follows the mouse when it moved or a button was released; with "+Always a selection" the first
// selectable gadget is selected (that loop never advances: it hangs if the first gadget in the list is not
// selectable). A click on the window background clears the focus, the arrow keys move the selection, and
// mouse clicks over the window are consumed.
int ui_window::process(int key)
{
    bool update = false;
    ui_gadget *g;

    if (is_hidden() || is_disabled()) {
        for (g = first_gadget; g; g = g->next_sibling)
            key = g->process(key);
        return key;
    }

    int dx, dy;
    ui_window_processing = true;
    mouse_get_delta(&dx, &dy, &game_mouse_z);
    if (dx || dy || ui_left_released || ui_right_released || refresh) {
        update = true;
        refresh = false;
    }

    ui_gadget *over = gadget_at(ui_mouse_x, ui_mouse_y);
    if ((flags & UI_WINDOW_ALWAYS_SEL) && !selected) {
        g = gadgets.window_next;
        while (g != &gadgets) {
            if (g->is_selectable()) {
                selected = g;
                break;
            }
        }
    }

    if (!mouse_is_down(0) && !mouse_is_down(1) && update) {
        ui_gadget *sel = NULL;
        if (over && over->is_selectable())
            sel = over;
        else if (flags & UI_WINDOW_ALWAYS_SEL)
            sel = selected;
        if (selected != sel) {
            if (selected)
                selected->send_event(UI_EVENT_DESELECTED, UI_USE_ID);
            selected = sel;
            if (sel)
                sel->send_event(UI_EVENT_SELECTED, UI_USE_ID);
        }
    }

    if (over != mouse_over) {
        if (mouse_over)
            mouse_over->send_event(UI_EVENT_MOUSE_LEFT, UI_USE_ID);
        mouse_over = over;
    }

    for (g = first_gadget; g; g = g->next_sibling)
        key = g->process(key);

    if (!over && contains_point(ui_mouse_x, ui_mouse_y) && ui_left_pressed) {
        ui_current_gadget->clear_focus();
        ui_left_pressed = false;
    }
    if (flags & (UI_WINDOW_ALLOW_ARROWS | UI_WINDOW_ALLOW_WRAP))
        key = handle_arrow_key(key);
    if (over || contains_point(ui_mouse_x, ui_mouse_y)) {
        ui_left_pressed = false;
        ui_left_released = false;
        ui_right_pressed = false;
        ui_right_released = false;
        ui_left_double_clicked = false;
        ui_right_double_clicked = false;
    }

    int f = flags;
    ui_window_processing = false;
    if (f & UI_WINDOW_DESTROY)
        dtor();
    return key;
}

// 0x52be90  Confidence: exact.
// Mask byte under a screen point, -1 outside the window or without mask data.
int ui_window::get_mask_pixel(int px, int py)
{
    if (mask_data) {
        int dx = px - x;
        int dy = py - y;
        if (dx >= 0 && dy >= 0 && dx < w && dy < h)
            return mask_data[dy * w + dx];
    }
    return -1;
}

// 0x52bee0  Confidence: exact.
// Hidden behind an enabled modal window that comes later in the list, or hidden itself.
bool ui_window::is_hidden()
{
    for (ui_window *win = next; win != &ui_window_sentinel; win = win->next) {
        if (!win->is_disabled() && (win->flags & UI_WINDOW_MODAL))
            return true;
    }
    return ui_element::is_hidden();
}

// 0x52bf30  Confidence: exact.
bool ui_window::is_shown()
{
    for (ui_window *win = next; win != &ui_window_sentinel; win = win->next) {
        if (!win->is_disabled() && (win->flags & UI_WINDOW_MODAL))
            return false;
    }
    return ui_element::is_shown();
}

// 0x52bf80  Confidence: exact.
// Vtable slot 4. Windows of another group (ui_push_group / ui_pop_group) count as disabled.
bool ui_window::is_disabled()
{
    if (group != ui_group_depth)
        return true;
    return ui_element::is_disabled();
}

// 0x52bfa0  ui_slider::ui_slider - the implicit constructor (base constructor, then vtable 0x581dd0),
//           emitted by `new ui_slider` in parse_slider. Compiler-generated, no source.
// 0x52bfc0  ui_gadget::`scalar deleting destructor' - emitted by `delete g` in dtor (~ui_gadget, then
//           operator delete when bit 0 of the flags is set). Compiler-generated, no source.
