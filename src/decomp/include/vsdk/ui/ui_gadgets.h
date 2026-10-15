// vsdk/ui/ui_gadgets.h - the widget classes: button, static, checkbox, radio button, input box, list box,
// slider (vsdk\ui, 0x526f70-0x55ed30; file names not in the binary). Only the members the pilots use.
// Not original source. Sizes are the operator new sizes in the ui_parse_* functions; fields at the
// offsets the parsers write. ui_container has its own header (vsdk/ui/ui_container.h).
#ifndef VSDK_UI_UI_GADGETS_H
#define VSDK_UI_UI_GADGETS_H

#include "vsdk/ui/ui.h"

class ui_state_bitmap {               // 0x18 bytes
public:
    void set(int bm);                 // 0x52c980
private:
    ubyte _data[0x18];
};

class ui_button : public ui_gadget {  // 0x428, vtable 0x581d88
public:
    ui_button();                      // 0x526f70
    void create(ui_element *window, int x, int y, int w, int h, int id, int flags, bool owned); // 0x527050
    void create_in_parent(ui_gadget *parent, int x, int y, int w, int h, int id, int flags,
                          bool owned);                // 0x527100
    void set_tooltip(const char *text, int x, int y, int delay);   // 0x5271c0
    void set_text(const char *text);  // 0x5272c0
    ubyte _unk274[0x38c - 0x274];
    int font;                         // +0x38C
    ubyte _unk390[4];
    ui_state_bitmap state_normal;     // +0x394
    ui_state_bitmap state_highlighted;// +0x3AC
    ui_state_bitmap state_pressed;    // +0x3C4
    ui_state_bitmap state_disabled;   // +0x3DC
    ui_state_bitmap state_drag;       // +0x3F4
    ubyte _unk40c[0x428 - 0x40c];
};
LAYOUT_CHECK(ui_button, sizeof(ui_button) == 0x428);

class ui_static : public ui_gadget {  // 0x3cc, vtable 0x581dac
public:
    ui_static();                      // 0x527ea0
    void create(ui_element *window, int x, int y, int w, int h, int id, bool owned);   // 0x527f10
    void set_tooltip(const char *text, int x, int y, int delay);   // 0x527fa0
    void set_text(const char *text);  // 0x528060
    ubyte _unk274[0x384 - 0x274];
    int font;                         // +0x384
    ubyte _unk388[4];
    ui_state_bitmap state_normal;     // +0x38C
    ui_state_bitmap state_disabled;   // +0x3A4
    int h_justify;                    // +0x3BC 0 left, 1 center, 2 right
    int v_justify;                    // +0x3C0 0 top, 1 center, 2 bottom
    int text_x;                       // +0x3C4
    int text_y;                       // +0x3C8
};
LAYOUT_CHECK(ui_static, sizeof(ui_static) == 0x3cc);

class ui_checkbox : public ui_gadget {  // 0x3a4, vtable 0x581f68
public:
    ui_checkbox();                    // 0x553ce0
    virtual void dtor();              // slot 0, 0x553cd0
    virtual int  process(int key);    // slot 5, 0x553ba0
    virtual void draw();              // slot 7, 0x553f00
    virtual bool is_selectable();     // slot 8, 0x5546e0
    void create(ui_element *window, int x, int y, int box_w, int box_h, const char *label,
                int label_x, int label_y, int font, bool checked, int id, bool owned);  // 0x553d00
    void set_tooltip(const char *text, int x, int y, int delay);   // 0x553e00
    ubyte _unk274[0x394 - 0x274];
    int bm_unchecked;                 // +0x394
    int bm_checked;                   // +0x398
    int bm_pressed;                   // +0x39C
    int bm_disabled;                  // +0x3A0
};
LAYOUT_CHECK(ui_checkbox, sizeof(ui_checkbox) == 0x3a4);

// No user constructor: the implicit one (checkbox constructor, then vtable 0x581c7c) is the COMDAT
// 0x4deeb0 that the linker kept from another unit.
class ui_radio_button : public ui_checkbox {   // 0x3a4, vtable 0x581c7c
public:
    virtual int process(int key);     // slot 5, 0x554710
    void create(ui_gadget *parent, int x, int y, int box_w, int box_h, const char *label,
                int label_x, int label_y, int font, int id, bool owned);   // 0x5548a0
};

class ui_inputbox : public ui_gadget {  // 0x5b0, vtable 0x581f1c
public:
    ui_inputbox();                    // 0x5526b0
    void create(ui_element *window, int x, int y, int w, int h, int id, int flags, bool owned); // 0x552700
    void set_tooltip(const char *text, int x, int y, int delay);   // 0x5527b0
    void set_valid_chars(const char *chars);              // 0x552870
    void set_invalid_chars(const char *chars);            // 0x552c50
    void set_text(const char *text);  // 0x553030
    ubyte _unk274[0x578 - 0x274];
    int font;                         // +0x578
    ubyte _unk57c[4];
    int max_length;                   // +0x580
    char password_char;               // +0x584
    ubyte _unk585[3];
    int v_justify;                    // +0x588 0 top, 1 center, 2 bottom
    int margin;                       // +0x58C
    int text_y;                       // +0x590
    int cursor_y;                     // +0x594
    int cursor_w;                     // +0x598
    int cursor_h;                     // +0x59C
    int bm_cursor;                    // +0x5A0
    int bm_normal;                    // +0x5A4
    int bm_disabled;                  // +0x5A8
    ubyte _unk5ac[4];
};
LAYOUT_CHECK(ui_inputbox, sizeof(ui_inputbox) == 0x5b0);

class ui_listbox : public ui_gadget {   // 0x320, vtable 0x581e34
public:
    ui_listbox();                     // 0x52d5c0
    void create(ui_element *window, int x, int y, int w, int h, int content_w, int content_h,
                int id, int flags, bool owned);       // 0x52d650
    void set_tooltip(const char *text, int x, int y, int delay);   // 0x52d7e0
    ubyte _unk274[0x2b8 - 0x274];
    int bm_background;                // +0x2B8
    int margin;                       // +0x2BC
    int v_margin;                     // +0x2C0
    ubyte _unk2c4[8];
    int entry_spacing;                // +0x2CC
    ubyte _unk2d0[0x320 - 0x2d0];
};
LAYOUT_CHECK(ui_listbox, sizeof(ui_listbox) == 0x320);

// The slider (0x2a8, vtable 0x582308; constructor 0x55e780 ui_slider__ui_slider_base). sum.symbols names
// the methods ui_slider__*; they are members of this base class.
class ui_slider_base : public ui_gadget {
public:
    ui_slider_base();                 // 0x55e780
    virtual int  process(int key);    // slot 5, 0x55e4c0
    virtual void draw();              // slot 7, 0x55eb70
    void create(ui_element *window, int x, int y, int w, int h, bool horizontal, int marker_size,
                int positions, int id, int flags, bool owned);   // 0x55e7e0 ui_slider__create
    void set_tooltip(const char *text, int x, int y, int delay);   // 0x55ea50
    void set_pos(int pos);            // 0x55eb30
    void create_scrollbar(ui_gadget *listbox, bool horizontal, int marker_size, int id, int flags,
                          bool owned);                // 0x55ed30
    ubyte _unk274[0x2a0 - 0x274];
    ui_button *inc_button;            // +0x2A0 right / down
    ui_button *dec_button;            // +0x2A4 left / up
};
LAYOUT_CHECK(ui_slider_base, sizeof(ui_slider_base) == 0x2a8);

// The scroll bar / table slider (0x2ac, vtable 0x581dd0 with the same nine entries as 0x582308). No user
// constructor: the implicit one is the COMDAT 0x52bfa0 that window.cpp emits (ui_slider__ui_slider).
class ui_slider : public ui_slider_base {
public:
    int _unk2a8;                      // +0x2A8 (never written by window.cpp)
};
LAYOUT_CHECK(ui_slider, sizeof(ui_slider) == 0x2ac);

#endif
