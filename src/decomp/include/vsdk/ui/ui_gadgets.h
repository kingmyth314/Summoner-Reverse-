// vsdk/ui/ui_gadgets.h - the widget classes: button, static, checkbox, radio button, input box, list box,
// slider (vsdk\ui, 0x526940-0x55ed30; file names not in the binary). Only the members the pilots use.
// Not original source. Sizes are the operator new sizes in the ui_parse_* functions; fields at the
// offsets the parsers write. ui_container has its own header (vsdk/ui/ui_container.h). decomp61 wrote
// ui_button (vsdk/unnamed_ui_button_0x526940.cpp) and ui_static (vsdk/unnamed_ui_static_0x527e20.cpp).
#ifndef VSDK_UI_UI_GADGETS_H
#define VSDK_UI_UI_GADGETS_H

#include "vsdk/ui/ui.h"
#include "vsdk/os/timestamp.h"

// decomp51 (D3100): the constructor and the destructor are out of line. Because the destructor exists,
// ui_button, ui_static and ui_listbox get implicit destructors; their COMDAT copies the linker kept are
// 0x4317e0 (sum.symbols ui_button__dtor), 0x431780 (ui_static__dtor) and 0x431950 (ui_listbox__dtor),
// emitted by the editor dialog and boot / load-game units (game/unnamed_editor_dialogs_0x430bc0.cpp,
// game/unnamed_states_0x431870.cpp). They are compiler-generated, not declared members.
// decomp61: the bitmaps of one gadget form a ring through +0x14 (link); +0 is the state (0 none, 1 set,
// 2 = draw without the clip rectangle), +4 the bitmap.
class ui_state_bitmap {               // 0x18 bytes
public:
    ui_state_bitmap();                // 0x52c930
    ~ui_state_bitmap();               // 0x52c950 (jumps to ui_state_bitmap__clear)
    void set(int bm);                 // 0x52c980
    // decomp61 (ui_button, ui_static).
    void link(ui_state_bitmap *other);   // 0x52c960 into other's ring
    void clear();                     // 0x52c970 state = 0
    // (x, y, w, h, u, v): the bitmap at (x, y); with u >= 0 (and state != 2) only its w x h part at
    // (u, v) shows (gr::set_clip(x + u, y + v, w, h)). False when no bitmap is set.
    bool draw(int x, int y, int w, int h, int u, int v);   // 0x52c9a0
    // decomp62: no sum.symbols entry (0x52ca50 sits in the padding after draw; no caller). Name ours.
    int  get_bitmap();                // 0x52ca50 +4
    // decomp62: the fields (was private ubyte _data[0x18]).
    int state;                        // +0x00
    int bm;                           // +0x04 -1 none
    ubyte _unk08[0x14 - 0x08];        // never read or written by the gadget base unit
    ui_state_bitmap *next;            // +0x14 ring (the constructor points it at itself)
};
LAYOUT_CHECK(ui_state_bitmap, sizeof(ui_state_bitmap) == 0x18);

// ui_button::flags (+0x274; names ours, from ui_button::process / draw).
#define UI_BUTTON_FLAT        0x001   // filled face, no bevel
#define UI_BUTTON_REPEATS     0x002   // auto-repeat while held (set_repeats; create_in_parent forces it)
#define UI_BUTTON_NO_BEVEL    0x004   // filled face, no bevel, in every state
#define UI_BUTTON_BITMAP_FACE 0x008   // with a state bitmap: fill the face over it
#define UI_BUTTON_DRAGGABLE   0x010   // drag and drop (drag_type / drop_types / count / capacity)
#define UI_BUTTON_LATCHED     0x020   // drawn sunken (set_latched)
#define UI_BUTTON_ARROW_V1    0x040   // draw_arrow: vertical lines from the bottom edge
#define UI_BUTTON_ARROW_V2    0x080   // draw_arrow: vertical lines from the top edge (0x40 wins)
#define UI_BUTTON_ARROW_H1    0x100   // draw_arrow: horizontal lines from the right edge
#define UI_BUTTON_ARROW_H2    0x200   // draw_arrow: horizontal lines from the left edge
#define UI_BUTTON_RIGHT_CLICK 0x400   // also reacts to the right mouse button

class ui_button MODERN_FINAL : public ui_gadget {  // 0x428, vtable 0x581d88
public:
    ui_button();                      // 0x526f70
    // decomp61: slots 0, 5, 7, 8 of 0x581d88 (the others are ui_gadget's).
    virtual void dtor();              // slot 0, 0x526f30 ui_button__vfunc0 (clears four state bitmaps)
    virtual int  process(int key);    // slot 5, 0x526a50
    virtual void draw();              // slot 7, 0x527690
    virtual bool is_selectable();     // slot 8, 0x527df0
    void create(ui_element *window, int x, int y, int w, int h, int id, int flags, bool owned); // 0x527050
    void create_in_parent(ui_gadget *parent, int x, int y, int w, int h, int id, int flags,
                          bool owned);                // 0x527100
    void set_tooltip(const char *text, int x, int y, int delay);   // 0x5271c0
    void set_text(const char *text);  // 0x5272c0
    void press();                     // 0x527420 programmatic click (state 1, 100 ms timer; decomp51)
    // decomp55 (the script editor): flag 0x20 of +0x274, a toggle state; is_latched is false while hidden.
    void set_latched(bool latched);   // 0x526990
    void toggle_latched();            // 0x5269c0
    bool is_latched();                // 0x5269e0
    // decomp61.
    void fire_click(int right);       // 0x526940 event 0xc / 0xd, state 6 / 7
    bool can_accept_drop();           // 0x526a00 the dragged button fits this one
    void set_repeats(bool repeats);   // 0x527280 (no caller)
    bool was_clicked();               // 0x5272f0 state 6 or 8
    bool was_left_clicked();          // 0x527330 state 6 (no caller)
    bool was_double_clicked();        // 0x527360 state 8 (no caller)
    bool is_pressed();                // 0x527370
    int  get_flags();                 // 0x5273c0
    char *get_text();                 // 0x5273d0
    void get_drag_origin(int *x, int *y);   // 0x5273e0 screen position minus the press point (ui_draw)
    void draw_arrow();                // 0x527440

    int flags;                        // +0x274 UI_BUTTON_*
    // +0x278 0 idle, 1 pressed by key / press() (press_timer), 2 / 3 held left / right, 4 dragged,
    // 5 drop target under a drag, 6 / 7 left / right click, 8 / 9 left / right double click.
    int state;
    char text[0x100];                 // +0x27C
    RealtimeTimer repeat_timer;       // +0x37C 450 ms to the first repeat, then 150 ms
    RealtimeTimer press_timer;        // +0x380 100 ms of state 1
    int press_x;                      // +0x384 ui_mouse_x / _y at the press (get_drag_origin)
    int press_y;                      // +0x388
    int font;                         // +0x38C
    Color *colors;                    // +0x390 ui_button_colors: three sets of five
    ui_state_bitmap state_normal;     // +0x394
    ui_state_bitmap state_highlighted;// +0x3AC
    ui_state_bitmap state_pressed;    // +0x3C4
    ui_state_bitmap state_disabled;   // +0x3DC
    ui_state_bitmap state_drag;       // +0x3F4
    int drag_type;                    // +0x40C type bits as a drag source (0 = cannot be dragged)
    int drop_types;                   // +0x410 the types it accepts
    int count;                        // +0x414 items held
    int capacity;                     // +0x418 < 0 unlimited
    int drag_amount;                  // +0x41C items moved by one drop (1)
    int _unk420;                      // +0x420 0xff from create, not read here
    ui_button *drag_data;             // +0x424 this, from create (not read here)
};
LAYOUT_CHECK(ui_button, sizeof(ui_button) == 0x428);

// decomp61: the button being dragged (ui_button::process sets it; ui_process ends the drag).
extern ui_button *ui_drag_gadget;     // 0x2cbfcb8

class ui_static MODERN_FINAL : public ui_gadget {  // 0x3cc, vtable 0x581dac
public:
    ui_static();                      // 0x527ea0
    // decomp61: slots 0, 5, 7 of 0x581dac (slot 8 is ui_gadget's).
    virtual void dtor();              // slot 0, 0x527e20 ui_static__vfunc0
    virtual int  process(int key);    // slot 5, 0x527e50 event 0x12 on a left press over it
    virtual void draw();              // slot 7, 0x5280e0
    void create(ui_element *window, int x, int y, int w, int h, int id, bool owned);   // 0x527f10
    void set_tooltip(const char *text, int x, int y, int delay);   // 0x527fa0
    void set_text(const char *text);  // 0x528060
    char *get_text();                 // 0x528090 (no caller)
    // decomp61: sum.symbols names (no caller). They set the state bitmap's clip size and offset, not the
    // justification and the text offset (D4107).
    void set_justification(int w, int h);    // 0x5280a0 bm_w, bm_h
    void set_text_offset(int u, int v);      // 0x5280c0 bm_u, bm_v when both are >= 0
    char text[0x100];                 // +0x274
    int bm_w;                         // +0x374 state bitmap clip size (create: w, h)
    int bm_h;                         // +0x378
    int bm_u;                         // +0x37C state bitmap offset (create: 0, 0)
    int bm_v;                         // +0x380
    int font;                         // +0x384
    Color *colors;                    // +0x388 text colours (editor_dialog_show stores editor_dialog_colors; D3101)
    ui_state_bitmap state_normal;     // +0x38C
    ui_state_bitmap state_disabled;   // +0x3A4
    int h_justify;                    // +0x3BC 0 left, 1 center, 2 right
    int v_justify;                    // +0x3C0 0 top, 1 center, 2 bottom
    int text_x;                       // +0x3C4 -1 = by h_justify
    int text_y;                       // +0x3C8 -1 = by v_justify
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
    void set_checked(bool checked);   // 0x553ee0 byte +0x274 (decomp55)
    bool is_checked();                // 0x553ef0
    // decomp70 (vsdk/unnamed_checkbox_0x553ba0.cpp): the fields (were _unk274[0x120]) and toggle.
    void toggle();                    // 0x553ec0
    bool checked;                     // +0x274
    ubyte _unk275[3];
    int box_w;                        // +0x278
    int box_h;                        // +0x27C
    char label[0x100];                // +0x280 drawn right of the box
    int font;                         // +0x380
    int label_x;                      // +0x384 offset of the label from the box's right edge / top
    int label_y;                      // +0x388
    int state;                        // +0x38C 0 idle, 1 mouse over, 2 pressed, 3 hidden (process)
    // +0x390 ui_checkbox_colors, four sets of (box, cross, text, text shadow): [0..3] idle, [4..7] mouse
    // over (only its text and shadow are used; the box and cross are the idle ones), [8..11] pressed,
    // [12..15] hidden (no cross).
    Color *colors;
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

// ui_inputbox::flags (+0x274; decomp70, names ours).
#define UI_INPUTBOX_PASSWORD   0x1    // drawn as password_char
#define UI_INPUTBOX_NO_ESCAPE  0x2    // Esc does not clear the text
#define UI_INPUTBOX_FIT_WIDTH  0x4    // refuse a character that would not fit the box

// decomp70: the unit is vsdk/unnamed_inputbox_0x5521a0.cpp. Events to the window callback (names ours):
// 0x14 a key reached the focused box, 0x15 the text changed, 0x16 the text is full.
class ui_inputbox : public ui_gadget {  // 0x5b0, vtable 0x581f1c
public:
    ui_inputbox();                    // 0x5526b0
    // decomp70: slots 0, 5, 7, 8 of 0x581f1c (the others are ui_gadget's).
    virtual void dtor();              // slot 0, 0x5526a0 ui_inputbox__vfunc0 (ui_gadget::dtor)
    virtual int  process(int key);    // slot 5, 0x552310
    virtual void draw();              // slot 7, 0x5530b0
    virtual bool is_selectable();     // slot 8, 0x5534c0
    void create(ui_element *window, int x, int y, int w, int h, int id, int flags, bool owned); // 0x552700
    void set_tooltip(const char *text, int x, int y, int delay);   // 0x5527b0
    // decomp70 (D5105): "A-Za-z0-9$_-" style sets (ranges of digits / capitals / small letters);
    // 0 for a malformed set (the box is unchanged), else 1. Were void.
    int  set_valid_chars(const char *chars);              // 0x552870
    int  set_invalid_chars(const char *chars);            // 0x552c50
    // decomp70: 0 when the text is longer than max_length (unchanged), else 1 (NULL clears). Was void.
    int  set_text(const char *text);  // 0x553030
    char *get_text();                 // 0x5530a0 +0x278 (decomp55)
    // decomp70: thiscall helpers (this unused by the first).
    int  numpad_to_ascii(int key);    // 0x5521a0 ui_inputbox_numpad_to_ascii: keypad digits as digits
    bool is_char_allowed(char c);     // 0x552280 invalid set, valid set, then char_filter
    int flags;                        // +0x274 UI_INPUTBOX_* (decomp70)
    char text[0x100];                 // +0x278
    char valid_chars[0x100];          // +0x378 empty = any
    char invalid_chars[0x100];        // +0x478
    int font;                         // +0x578
    Color *colors;                    // +0x57C ui_inputbox_colors: [0..5] shown, [6..11] hidden (decomp70)
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
    // +0x5AC (decomp55): (text so far, typed character), false rejects the character; the script editor
    // stores script_editor_decimal_filter_cb here (D3504). Field name ours.
    bool (*char_filter)(const char *text, char c);
};
LAYOUT_CHECK(ui_inputbox, sizeof(ui_inputbox) == 0x5b0);

// One list box line (0x3c bytes, vtable 0x581e30 with one slot; the text entry 0x13c bytes, vtable
// 0x581e2c, is a subclass that ui_listbox::add_entry allocates unless the box has flag 0x400). The list
// box embeds one as the head of its entry ring (+0x2d4). decomp51 (D3100): only what the load-game
// screen and the implicit ui_listbox destructor need. Class name from sum.symbols (ui_listbox_entry__*).
// decomp62 wrote the class (vsdk/unnamed_listbox_0x52ca60.cpp): the line layout members and +0x14/+0x18.
class ui_listbox_entry {
public:
    ui_listbox_entry();               // 0x52d540 font -1, data -1
    ~ui_listbox_entry();              // 0x52d560 (restores the vtable; non-virtual)
    virtual void set_text(const char *text);   // slot 0, 0x52d5b0 (the text entry: 0x52d4a0)
    void set_font(int font);          // 0x52d5a0 font, dirty = 1 (decomp55)
    // decomp62. wrap_text splits the text into lines of at most width (or what width_cb returns for a
    // line's y and height, when it returns non-zero) into ui_listbox_line_starts / _lengths and returns
    // the line count.
    int  wrap_text(int width, int y, int (*width_cb)(int y, int line_h));   // 0x52ca60
    int  line_top(int line);          // 0x52cb70 y + line * (font height + line_spacing)
    int  line_bottom(int line);       // 0x52cba0 line_top + font height
    int  line_at_y_with_spacing(int y);   // 0x52cbd0 clamped to 0 .. num_lines - 1
    int  line_at_y(int y);            // 0x52cc20 clamped
    int  line_at_y_or_none(int y);    // 0x52d4f0 -1 outside the entry
    void set_indent(int indent);      // 0x52d570
    void set_wrap_indent(int indent); // 0x52d580
    void set_line_spacing(int spacing);   // 0x52d590
    ubyte dirty;                      // +0x04 set by the setters; nothing clears it (decomp62, F6201)
    ubyte _unk05[3];
    ui_listbox_entry *next;           // +0x08 entry ring (head: the list box's +0x2d4)
    ui_listbox_entry *prev;           // +0x0C
    int y;                            // +0x10 top of the line in the content (recalc_content_size; decomp55)
    int width;                        // +0x14 widest line plus indent (wrap_text; decomp62)
    int height;                       // +0x18 num_lines * line height - line_spacing (wrap_text)
    int indent;                       // +0x1C
    int wrap_indent;                  // +0x20
    int line_spacing;                 // +0x24
    int font;                         // +0x28
    int num_lines;                    // +0x2C
    char *text;                       // +0x30
    int data;                         // +0x34
    Color *colors;                    // +0x38 ui_listbox_entry_colors
};
LAYOUT_CHECK(ui_listbox_entry, sizeof(ui_listbox_entry) == 0x3c);

// decomp62: the entry that owns its text (0x13c bytes, vtable 0x581e2c). No destructor of its own: the
// implicit one (0x52f440, jmp ~ui_listbox_entry) and both scalar deleting destructors (0x52f400,
// 0x52f420) are COMDATs at the end of the list box unit (D4201).
class ui_listbox_text_entry : public ui_listbox_entry {
public:
    ui_listbox_text_entry();          // 0x52d480 text = buffer
    virtual void set_text(const char *text);   // slot 0, 0x52d4a0 copies; 255 characters or more give
                                               // "text too long for pc version"
    char buffer[0x100];               // +0x3C
};
LAYOUT_CHECK(ui_listbox_text_entry, sizeof(ui_listbox_text_entry) == 0x13c);

// The wrapped lines of the last entry laid out (ui_listbox_entry::wrap_text), read by ui_listbox::draw.
// sum.symbols types the first int[20]; it holds char pointers. No bounds check (F6202).
extern char *ui_listbox_line_starts[20];   // 0x2cbf584
extern int ui_listbox_line_lengths[20];    // 0x2cbf5d4

// ui_listbox::flags (+0x274), the "+..." lines of ui_window::parse_listbox. Names ours.
#define UI_LISTBOX_WRAP          0x001   // "+Allow wrap": wrap to max_content_w or the view width
#define UI_LISTBOX_VSCROLLBAR    0x002   // "+Add vertical scrollbar" (and "+Add both scrollbars")
#define UI_LISTBOX_HSCROLLBAR    0x004   // "+Add horizontal scrollbar"
#define UI_LISTBOX_NO_SELECT     0x008   // "+No select": up / down scroll instead
#define UI_LISTBOX_BY_ENTRY      0x010   // "+Scroll by entry"
#define UI_LISTBOX_BY_PIXELS     0x020   // "+Scroll by pixels": no snapping, partial lines drawn
#define UI_LISTBOX_SORTED        0x040   // "+Sort entries" (_stricmp order)
#define UI_LISTBOX_ALIGN_BOTTOM  0x080   // "+Align view with bottom"
#define UI_LISTBOX_NO_BACKGROUND 0x100   // "+No background"
#define UI_LISTBOX_MOUSEOVER     0x200   // "+Mouseover highlights"
#define UI_LISTBOX_PREALLOCATED  0x400   // "+Preallocated text": entries keep the caller's pointer

class ui_slider;

class ui_listbox : public ui_gadget {   // 0x320, vtable 0x581e34
public:
    ui_listbox();                     // 0x52d5c0
    // decomp62: slots 0, 5, 7 of 0x581e34 (the others are ui_gadget's).
    virtual void dtor();              // slot 0, 0x52d420 ui_listbox__destroy (deletes the entries)
    virtual int  process(int key);    // slot 5, 0x52d080
    virtual void draw();              // slot 7, 0x52f080
    void create(ui_element *window, int x, int y, int w, int h, int content_w, int content_h,
                int id, int flags, bool owned);       // 0x52d650
    void set_tooltip(const char *text, int x, int y, int delay);   // 0x52d7e0
    // decomp51 (the load-game screen, game/unnamed_states_0x431870.cpp).
    ui_listbox_entry *get_selected();                 // 0x52d8a0 +0x2b4
    // decomp55 (D3506): returns AL, false when no entry has the data (0x52d8d1 / 0x52d8e2).
    bool select_by_data(int data, bool scroll_into_view);   // 0x52d8b0 the entry whose +0x34 is data (decomp53)
    // decomp62 (D4205): returns AL like select_by_data (was void).
    bool select_by_text(const char *text, bool scroll_into_view);   // 0x52d8f0
    void select_entry(ui_listbox_entry *e, bool scroll_into_view);  // 0x52d920
    ui_listbox_entry *find_by_text(const char *text); // 0x52d9b0 strcmp of entry +0x30
    ui_listbox_entry *add_entry(const char *text, int font, int indent, int wrap_indent,
                                int line_spacing);    // 0x52da40 appends; returns the entry
    void clear();                     // 0x52dc30
    // decomp55 (the script editor).
    void recalc_content_size();       // 0x52cd90
    void clear_selection();           // 0x52d9a0 +0x2b4 = NULL
    void set_hscroll(int pos);        // 0x52ea00
    // decomp62: force snaps to the entry / line edges also with UI_LISTBOX_BY_PIXELS.
    void set_vscroll(int pos, bool force);  // 0x52ea50
    void set_sorted(bool sorted);     // 0x52f3d0 flag 0x40 of +0x274
    // decomp62.
    int  layout_entry(ui_listbox_entry *e, int y);    // 0x52cc60 wrap_text at the wrap width; line count
    bool is_entry_visible(ui_listbox_entry *e);       // 0x52cca0
    bool is_entry_fully_visible(ui_listbox_entry *e); // 0x52cce0
    bool is_entry_bottom_visible(ui_listbox_entry *e);   // 0x52cd20 (no caller)
    void update_vscrollbar();         // 0x52cd50
    void update_hscrollbar();         // 0x52cd70
    ui_listbox_entry *entry_at(int x, int y, bool clamp);   // 0x52ce20 content coordinates
    ui_listbox_entry *entry_above(int y);             // 0x52ce90
    ui_listbox_entry *entry_at_y(int y);              // 0x52ced0
    void relayout_dirty();            // 0x52cf10
    void sync_scrollbars();           // 0x52cf70
    bool remove_by_text(const char *text);            // 0x52dcc0 (no caller)
    void remove_entry(ui_listbox_entry *e);           // 0x52dd00
    void select_prev();               // 0x52dde0
    void select_next();               // 0x52deb0
    void scroll_left();               // 0x52df90
    void scroll_right();              // 0x52dfb0
    void select_page_up();            // 0x52dfd0
    void select_page_down();          // 0x52e050
    void scroll_line_up();            // 0x52e110
    void scroll_line_down();          // 0x52e290
    void scroll_page_up();            // 0x52e380
    void scroll_page_down();          // 0x52e590
    void scroll_page_right();         // 0x52e870 (no caller)
    void scroll_page_left();          // 0x52e8c0 (no caller)
    float get_hscroll_fraction();     // 0x52e8f0 (no caller)
    float get_vscroll_fraction();     // 0x52e920 (no caller)
    int  snap_scroll_top(int y);      // 0x52eb20
    int  snap_scroll_bottom(int y);   // 0x52ebe0
    void set_hscroll_fraction(float f, bool unused);  // 0x52ed00 (no caller; second argument not read)
    void set_vscroll_fraction(float f, bool unused);  // 0x52ed70 (no caller; second argument not read)
    int  get_view_width();            // 0x52f020 w - 2 * margin
    int  get_view_height();           // 0x52f030 h - 2 * v_margin
    int  get_content_width();         // 0x52f040
    int  get_content_height();        // 0x52f050
    bool is_empty();                  // 0x52f060 (no caller)

    int flags;                        // +0x274 UI_LISTBOX_* (decomp62: was _unk274)
    char typeahead[0x20];             // +0x278 typed characters (key_to_ascii)
    int typeahead_len;                // +0x298 cleared 1.5 s after the last one, by Backspace and Enter
    RealtimeTimer typeahead_timer;    // +0x29C
    int max_content_w;                // +0x2A0 create's content_w: wrap width and content width cap (0 none)
    int max_content_h;                // +0x2A4 create's content_h: > 0 drops the oldest entries above it
    int content_width;                // +0x2A8
    int content_height;               // +0x2AC - entry_spacing for an empty list (F6203)
    bool user_selected;               // +0x2B0 1 after a click or page key selects, 0 after a page scroll
    ui_listbox_entry *selected;       // +0x2B4
    int bm_background;                // +0x2B8
    int margin;                       // +0x2BC
    int v_margin;                     // +0x2C0
    int hscroll;                      // +0x2C4 horizontal scroll position (set_hscroll; decomp55)
    int vscroll;                      // +0x2C8 vertical scroll position (set_vscroll; decomp55)
    int entry_spacing;                // +0x2CC
    int hscroll_step;                 // +0x2D0 10 pixels per left / right key (decomp62: was _unk2d0)
    ui_listbox_entry entries;         // +0x2D4 entry ring head (D3100; ui_listbox() list_inits it, D14300)
    ui_slider *vscrollbar;            // +0x310 (decomp62: was _unk310; set by ui_slider::create_scrollbar)
    ui_slider *hscrollbar;            // +0x314
    Color *colors;                    // +0x318 ui_listbox_colors: [0..2] face, [3..5] hidden face, [6] hidden text
    int (*wrap_width_cb)(int y, int line_h);   // +0x31C passed to wrap_text (0 from the constructor)
};
LAYOUT_CHECK(ui_listbox, sizeof(ui_listbox) == 0x320);

// The slider (0x2a8, vtable 0x582308; constructor 0x55e780 ui_slider__ui_slider_base). sum.symbols names
// the methods ui_slider__*; they are members of this base class.
// decomp66: the slider unit (0x55e040-0x55f10b, src/decomp/vsdk/unnamed_ui_slider_0x55e040.cpp) wrote the
// members and named the fields.
// ui_slider_base::drag_state
#define UI_SLIDER_IDLE      0
#define UI_SLIDER_DRAGGING  1         // the marker follows the mouse
#define UI_SLIDER_PAGE_UP   2         // pressed after the marker: pos += page_size while held
#define UI_SLIDER_PAGE_DOWN 3         // pressed before it: pos -= page_size
class ui_slider_base : public ui_gadget {
public:
    ui_slider_base();                 // 0x55e780
    virtual int  process(int key);    // slot 5, 0x55e4c0
    virtual void draw();              // slot 7, 0x55eb70
    // (x, y, w, h) of the marker relative to the gadget (count 1 divides by zero).
    void get_marker_rect(int *x, int *y, int *w, int *h);    // 0x55e420
    // flags bit 0: two arrow buttons, taken off the length.
    void create(ui_element *window, int x, int y, int w, int h, bool horizontal, int marker_size,
                int positions, int id, int flags, bool owned);   // 0x55e7e0 ui_slider__create
    void set_tooltip(const char *text, int x, int y, int delay);   // 0x55ea50 (the old tooltip is unlinked, not freed)
    void set_pos(int pos);            // 0x55eb30
    // decomp62 (the list box).
    int  get_pos();                   // 0x55eb10 +0x28c
    int  get_count();                 // 0x55eb20 (decomp66)
    void set_count(int count);        // 0x55eb40 (decomp66)
    void set_marker_size(int size);   // 0x55eb50 (decomp66)
    void set_page_size(int size);     // 0x55eb60 (decomp66)
    RealtimeTimer repeat_timer;       // +0x274 450 ms, then 150 ms between page steps
    int drag_state;                   // +0x278 UI_SLIDER_*
    int drag_start;                   // +0x27C the mouse coordinate when the drag began
    int drag_pos;                     // +0x280 pos when the drag began (a right click restores it)
    int _unk284;                      // +0x284
    int count;                        // +0x288 positions
    int pos;                          // +0x28C 0 .. count - 1 (clamped by draw)
    int marker_size;                  // +0x290
    int page_size;                    // +0x294
    bool horizontal;                  // +0x298
    ubyte _pad299[3];
    Color *colors;                    // +0x29C ui_slider_colors: [0..2] marker, [3] before, [4] after it
    ui_button *inc_button;            // +0x2A0 right / down
    ui_button *dec_button;            // +0x2A4 left / up
};
LAYOUT_CHECK(ui_slider_base, sizeof(ui_slider_base) == 0x2a8);

// The scroll bar / table slider (0x2ac, vtable 0x581dd0 with the same nine entries as 0x582308). No user
// constructor: the implicit one is the COMDAT 0x52bfa0 that window.cpp emits (ui_slider__ui_slider).
// decomp66 (D4604): the members that use +0x2a8 are this class's (they were declared on the base; the
// callers hold ui_slider pointers).
class ui_slider : public ui_slider_base {
public:
    void create_scrollbar(ui_gadget *listbox, bool horizontal, int marker_size, int id, int flags,
                          bool owned);                // 0x55ed30
    int  get_range();                 // 0x55ee50 visible + count - 1, at least 1
    void set_visible_count(int n);    // 0x55ee70
    void set_range(int content);      // 0x55eeb0
    void update_marker();             // 0x55eed0 the marker the visible share of the length, 7 pixels or more
    int visible;                      // +0x2A8 the list box's view size (was _unk2a8, never written by window.cpp)
};
LAYOUT_CHECK(ui_slider, sizeof(ui_slider) == 0x2ac);

#endif
