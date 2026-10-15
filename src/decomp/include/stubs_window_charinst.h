// stubs_window_charinst.h - declarations of the Sum.exe v1.40 engine classes, functions and globals that
// vsdk\ui\window.cpp and vsdk\ca\character_instance.cpp use but do not define, and that no compatible
// header in src/decomp/include/ declares. Not original source. Names and addresses from
// symbols/sum.symbols; signatures read from the call sites and callees in build/sum.asm.
//
// Why the existing headers are (mostly) not used (details in NOTES.md):
//  - stubs_container_targa.h declares ui_element / ui_gadget / ui_tooltip, but without the members
//    window.cpp calls (ui_element::init, get_kind, get_size, is_hidden, is_shown; ui_gadget::get_type,
//    set_parent, set_hotkey, set_hotspot, is_owned, hit_test, clear_focus, find_nearest_*). A class cannot
//    be extended from outside, so the two classes are declared again here with the same field names and
//    the same slot names (slot 0 "dtor", send_event returning bool), plus the missing members. Its File
//    also clashes with stubs_cmdline_grfont.h. ui_container.h includes it, so ui_container is declared
//    here too, with only what window.cpp calls.
//  - The TextFile classes in stubs_assess_pickup.h / stubs_credits_help.h / stubs_sound_items_registry.h
//    lack SkipWhitespace, CopyUntil, ParseInt and ParseString; declared again here.
//  - stubs_player_effectmem.h declares vector3 / matrix with constructors only; character_instance.cpp
//    calls their operators, so the vsdk math classes are declared again here.
//  - symbols/types.h is not included: its C structs (ui_element, TextFile, vector3, matrix43, ...) cannot
//    carry thiscall members. Layouts below were checked against it.
// stubs_cmdline_grfont.h is used as is (DebugError, VSDK_ERROR).
#ifndef STUBS_WINDOW_CHARINST_H
#define STUBS_WINDOW_CHARINST_H

#include "stubs_cmdline_grfont.h"     // DebugError, VSDK_ERROR

typedef unsigned char ubyte;          // same typedef as the other stub headers

// Compile-time layout check (VC6 has no static_assert).
#define WC_LAYOUT_CHECK(name, cond) typedef char wc_layout_check_##name[(cond) ? 1 : -1]

extern char string_buffer[];                          // 0x257ed80 shared sprintf buffer

// ================================================================ vsdk\parse\parse.cpp
struct TextFilePosition {             // {line, cursor} (GetPosition / SetPosition)
    int line;
    char *cursor;
};

class TextFile {                      // 0x50 bytes (stack frame of ui_load_table)
public:
    TextFile();                                           // 0x515960
    ~TextFile();                                          // 0x5159d0 (thunk to Close)
    int  Open(const char *name, int path_id);             // 0x515d20, nonzero = failure
    void Close();                                         // 0x515e90
    void SkipWhitespace();                                // 0x515a80
    bool SkipTo(const char *str, const char *stop);       // 0x515ba0 (al tested)
    void CopyUntil(char *dest, int size, const char *str, const char *stop);  // 0x515c80
    void GetPosition(TextFilePosition *pos);              // 0x515ce0
    void SetPosition(TextFilePosition *pos);              // 0x515d00
    bool ReadLine(char *dest, int dest_size);             // 0x516010 (result unused here)
    bool Accept(const char *str);                         // 0x516110
    void Expect(const char *str);                         // 0x5161e0
    int  ParseInt();                                      // 0x516290
    void ParseString(char *dest, int size, char open_quote, char close_quote);  // 0x5166b0
private:
    char *buffer;                     // +0x00
    char *cursor;                     // +0x04
    ubyte flag08;                     // +0x08
    ubyte _unmapped[0x3f];            // +0x09
    int line;                         // +0x48
    int size;                         // +0x4c
};
WC_LAYOUT_CHECK(TextFile, sizeof(TextFile) == 0x50);

// plainstring.cpp (Summoner 2 name): first/last non-blank of src into dest, at most max_length.
void trim_whitespace(char *dest, const char *src, int max_length);   // 0x514fb0

// ================================================================ vsdk\bmpman, vsdk\gr, mouse
int  bm_load(const char *name, int a, int b);         // 0x5026e0 (always (-1, -1) here)
void bm_unlock(int bm);                               // 0x503b50
void bm_get_dimensions(int bm, int *w, int *h);       // 0x503500
int  progress_update(int check_keys);                 // 0x503e80 (called with 0)

struct Color {                                        // same layout as stubs_credits_help.h
    ubyte r, g, b, a;
};
namespace gr {
void reset_clip();                                    // 0x5069a0 gr__reset_clip
}
void gr_set_color(int r, int g, int b, int a);        // 0x506b90
void gr_set_color_ptr(Color *c);                      // 0x506c10
void gr_rect(int x, int y, int w, int h, unsigned int mode, float z);     // 0x5077d0 (z always 1.0f here)
void gr_bitmap_stretched(int bm, int x, int y, int w, int h, int sx, int sy, int sw, int sh,
                         unsigned int mode);          // 0x506ec0
void gr_page_in(int bm, int flags);                   // 0x5069e0
extern unsigned int gr_rect_mode;                     // 0x25d88ac
extern unsigned int gr_bitmap_wrap_mode;              // 0x25d88c4

void mouse_get_delta(int *dx, int *dy, int *dz);      // 0x5013c0
bool mouse_is_down(int button);                       // 0x5012c0
extern int game_mouse_z;                              // 0x257ef80

// keyboard: the DirectInput key names, indexed by scan code (256 pointers, 0x5a68c0-0x5a6cbf; [1] is
// "Esc"). No name in sum.symbols; label ours.
extern char *key_names[256];                          // 0x5a68c0

// ================================================================ vsdk\ui: element and gadget bases
// Same classes as stubs_container_targa.h (see the note at the top). There is no RTTI; class names are
// ours. Slot 0 of every widget vtable is not the C++ destructor: it takes no flags argument and never
// frees the object (ui_element: free the tooltips). The real destructors are non-virtual.
class ui_element;
class ui_gadget;
class ui_window;
struct ui_tooltip;

struct ui_tooltip_head {              // list sentinel inside the element (+0x124)
    ui_tooltip *next;                 // +0x0
    ui_tooltip *prev;                 // +0x4
};

// One tooltip (0x120 bytes, operator new, no constructor; empty destructor 0x55ef50). Field names from
// stubs_container_targa.h.
struct ui_tooltip {
    ~ui_tooltip();                    // 0x55ef50
    ui_tooltip *next;                 // +0x000
    ui_tooltip *prev;                 // +0x004
    ui_element *owner;                // +0x008
    char text[0x100];                 // +0x00C
    int delay;                        // +0x10C
    Color *colors;                    // +0x110 ui_tooltip_colors
    int x;                            // +0x114
    int y;                            // +0x118
    int font;                         // +0x11C
};
extern Color ui_tooltip_colors[4];    // 0x2cbf708

class ui_element {                    // vtable 0x581ea0 (5 slots)
public:
    ui_element();                     // 0x542d20
    ~ui_element();                    // 0x542d40
    virtual void dtor();              // slot 0, 0x542cd0 ui_element__free_tooltips
    virtual bool send_event(int event, int value) = 0;    // slot 1, _purecall
    virtual void set_disabled(bool disabled);             // slot 2, 0x542f10
    virtual void set_enabled(bool enabled);               // slot 3, 0x542f70
    virtual bool is_disabled();                           // slot 4, 0x543000

    void init(int kind, int x, int y, int w, int h, int id);   // 0x542da0
    int  get_kind();                  // 0x542e10
    void get_size(int *w, int *h);    // 0x542e80
    bool is_hidden();                 // 0x542fe0
    bool is_shown();                  // 0x542ff0

    ubyte initialized;                // +0x004
    int kind;                         // +0x008 1 window, 0 gadget
    int x, y, w, h;                   // +0x00C rectangle
    ubyte hidden;                     // +0x01C
    ubyte disabled;                   // +0x01D
    char name[0x102];                 // +0x01E
    int id;                           // +0x120
    ui_tooltip_head tooltips;         // +0x124
    ubyte _unk12c[0x118];             // +0x12C
    bool (*callback)(int event, int value);   // +0x244 (cdecl, ui_window__send_event)
};
WC_LAYOUT_CHECK(ui_element, sizeof(ui_element) == 0x248);

class ui_gadget : public ui_element { // vtable 0x581e08 (9 slots)
public:
    ui_gadget();                      // 0x52c3f0
    ~ui_gadget();                     // 0x52c410 ui_gadget__dtor_base (non-virtual)
    virtual void dtor();              // slot 0, 0x52c3d0 ui_gadget__dtor
    virtual bool send_event(int event, int value);        // slot 1, 0x52c330
    virtual void set_disabled(bool disabled);             // slot 2, 0x52c890
    virtual void set_enabled(bool enabled);               // slot 3, 0x52c8c0
    virtual bool is_disabled();                           // slot 4, 0x52c530
    virtual int  process(int key);    // slot 5, 0x52c390 ui_gadget__process_children (returns key)
    virtual int  get_mask_pixel(int x, int y);            // slot 6, 0x52c6f0
    virtual void draw();              // slot 7, 0x52c840 ui_gadget__draw_children
    virtual bool is_selectable();     // slot 8, 0x52c880

    int  get_screen_x(int offset);    // 0x52c420
    int  get_screen_y(int offset);    // 0x52c460
    bool is_owned();                  // 0x52c4a0
    void set_parent(ui_gadget *parent);                   // 0x52c570
    void clear_focus();               // 0x52c630
    int  get_type();                  // 0x52c680
    void set_hotkey(int key, int slot);                   // 0x52c690
    ui_gadget *hit_test(int x, int y);                    // 0x52c730
    void set_hotspot(int size);       // 0x52c800
    void find_nearest_up(int x, int y);                   // 0x52c0f0 (result in ui_nav_best_gadget)
    void find_nearest_down(int x, int y);                 // 0x52c180
    void find_nearest_left(int x, int y);                 // 0x52c210
    void find_nearest_right(int x, int y);                // 0x52c2a0

    bool owned;                       // +0x248
    int hotkeys[2];                   // +0x24C
    int gadget_type;                  // +0x254 1 container, 2 button, 3 static, 4 checkbox, 5 radio,
                                      //        6 inputbox, 7 slider, 8 listbox
    int _unk258;                      // +0x258 hotspot
    ui_element *window;               // +0x25C
    ui_gadget *window_prev;           // +0x260 the window's gadget list (sentinel window +0x250)
    ui_gadget *window_next;           // +0x264
    ui_gadget *parent;                // +0x268
    ui_gadget *first_child;           // +0x26C
    ui_gadget *next_sibling;          // +0x270
};
WC_LAYOUT_CHECK(ui_gadget, sizeof(ui_gadget) == 0x274);

#define UI_TYPE_CONTAINER 1           // ui_gadget::gadget_type values the parsers test
#define UI_TYPE_SLIDER    7
#define UI_TYPE_LISTBOX   8

// The id a send_event value of -99999 stands for (use the element's own id, ui_window__send_event).
#define UI_USE_ID         (-99999)    // 0xfffe7961
#define UI_NO_ID          (-99998)    // 0xfffe7962 ui_load_table's default and ui_window__draw's value

// ---- vsdk\ui globals (other units) ------------------------------------------------------------
extern ui_window ui_window_sentinel;  // 0x2cbf718 (list head: next 0x2cbfbfc, prev 0x2cbfc00)
extern Color ui_window_colors[4];     // 0x2cbf670
extern ui_gadget *ui_current_gadget;  // 0x2cbfcc4
extern int ui_group_depth;            // 0x2cbfcc8
extern int ui_mouse_x;                // 0x2cbfca8
extern int ui_mouse_y;                // 0x2cbfcac
extern bool ui_left_pressed;          // 0x2cbf628
extern bool ui_right_released;        // 0x2cbf629
extern bool ui_right_pressed;         // 0x2cbf62a
extern bool ui_left_double_clicked;   // 0x2cbf700
extern bool ui_left_released;         // 0x2cbfca4
extern bool ui_right_double_clicked;  // 0x2cbfc04
void ui_init();                       // 0x52f760 (guarded, once)
void ui_draw_bevel(int x, int y, int w, int h, Color *light, Color *dark, int width, int inset); // 0x55e040

// ================================================================ vsdk\ui widgets (only what window.cpp uses)
// Sizes are the operator new sizes in the ui_parse_* functions; fields at the offsets the parsers write.
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
WC_LAYOUT_CHECK(ui_button, sizeof(ui_button) == 0x428);

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
WC_LAYOUT_CHECK(ui_static, sizeof(ui_static) == 0x3cc);

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
WC_LAYOUT_CHECK(ui_checkbox, sizeof(ui_checkbox) == 0x3a4);

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
WC_LAYOUT_CHECK(ui_inputbox, sizeof(ui_inputbox) == 0x5b0);

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
WC_LAYOUT_CHECK(ui_listbox, sizeof(ui_listbox) == 0x320);

class ui_container : public ui_gadget { // 0x388, vtable 0x581f40 (full class: src/decomp/vsdk/ui_container.h)
public:
    ui_container();                   // 0x553560
    void create(ui_element *window, int x, int y, int w, int h, int id, int flags, bool owned); // 0x553590
    void set_tooltip(const char *text, int font, int x, int y, int delay);   // 0x5535f0
    void set_label(const char *label);                    // 0x553680
    void load_mask_bitmap(const char *name);              // 0x5536b0
    ubyte _unk274[0x380 - 0x274];
    int font;                         // +0x380
    ubyte _unk384[4];
};
WC_LAYOUT_CHECK(ui_container, sizeof(ui_container) == 0x388);

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
WC_LAYOUT_CHECK(ui_slider_base, sizeof(ui_slider_base) == 0x2a8);

// The scroll bar / table slider (0x2ac, vtable 0x581dd0 with the same nine entries as 0x582308). No user
// constructor: the implicit one is the COMDAT 0x52bfa0 that window.cpp emits (ui_slider__ui_slider).
class ui_slider : public ui_slider_base {
public:
    int _unk2a8;                      // +0x2A8 (never written by window.cpp)
};
WC_LAYOUT_CHECK(ui_slider, sizeof(ui_slider) == 0x2ac);

// ================================================================ vsdk\math
// All out of line in Sum.exe (vector3() 0x505530 and matrix43() / quaternion() are one folded
// "mov eax, ecx; ret"). The operators return by value through a hidden pointer.
class matrix;
class vector3 {
public:
    vector3();                                        // 0x505530 vector3__vector3_default (empty)
    vector3(float x, float y, float z);               // 0x43b580
    vector3(const vector3 &v);                        // 0x410cd0 vector3__vector3_copy
    vector3 operator=(const vector3 &v);              // 0x413190 vector3__assign (returns a copy)
    vector3 operator+(const vector3 &v) const;        // 0x45b680
    vector3 operator-(const vector3 &v) const;        // 0x45b6b0
    vector3 operator*(float s) const;                 // 0x50f790 vector3__scaled
    vector3 operator-() const;                        // 0x51b700 vector3__negated (COMDAT emitted here)
    void operator+=(const vector3 &v);                // 0x48e440 vector3__add_assign
    void operator*=(float s);                         // 0x50f6a0 vector3__scale
    float len() const;                                // 0x4bc290
    int  is_valid() const;                            // 0x524220
    void unrotate(const matrix &m);                   // 0x5245f0
    float x, y, z;
};
WC_LAYOUT_CHECK(vector3, sizeof(vector3) == 0xc);

class matrix {                        // 3x3 rotation (matrix3 in sum.symbols)
public:
    matrix();                                         // 0x50f660 matrix__matrix_default
    matrix operator*(const matrix &m) const;          // 0x50fbc0 matrix3__multiply
    void operator=(const matrix &m);                  // 0x4a4320 matrix3__copy
    void extract_angles(float *p, float *b, float *h);// 0x510240 matrix3__extract_angles
    vector3 rvec, uvec, fvec;
};
WC_LAYOUT_CHECK(matrix, sizeof(matrix) == 0x24);

class quaternion {
public:
    quaternion();                                     // folded with 0x505530 (empty)
    quaternion(float x, float y, float z, float w);   // 0x50f990
    quaternion(const quaternion &q);                  // 0x51b730 quaternion__assign_b (COMDAT here)
    quaternion &operator=(const quaternion &q);       // 0x50f9b0 quaternion__assign
    float x, y, z, w;
};
WC_LAYOUT_CHECK(quaternion, sizeof(quaternion) == 0x10);
void quaternion_slerp(quaternion *out, const quaternion *a, const quaternion *b, float t);  // 0x5235a0

class matrix43 {                      // rotation rows m[0..2] + position m[3] (symbols/types.h)
public:
    matrix43();                                       // folded with 0x505530 (empty)
    matrix43(const vector3 &r, const vector3 &u, const vector3 &f, const vector3 &p);  // 0x5113f0
    matrix43 operator*(const matrix43 &m) const;      // 0x5114b0 matrix43__multiply_4x4
    void operator=(const matrix43 &m);                // 0x51b6c0 matrix43__copy (COMDAT here, no result)
    void set(const quaternion &q, const vector3 &pos);// 0x511840 matrix43__set_quat
    matrix43 inverse() const;                         // 0x5116f0
    void get_rotation(matrix *out) const;             // 0x511b70
    vector3 transform(const vector3 &v) const;        // 0x511c70 matrix43__transform_point
    void set_position(const vector3 &v);              // 0x511cf0
    void get_position(vector3 *out) const;            // 0x51b6e0 matrix43__get_position_b (COMDAT here)
    float m[4][3];
};
WC_LAYOUT_CHECK(matrix43, sizeof(matrix43) == 0x30);

int imax(int a, int b);                               // 0x51b750 (COMDAT here; imin is 0x503dc0)

// ================================================================ vsdk\ca (character.cpp, skeleton.cpp)
// Profiler scope: a one-byte object whose constructor returns this (0x51b6b0, COMDAT here) and whose
// destructor is empty (folded into the 91-caller bare ret 0x434650). Both are out of line here.
class profile_label {
public:
    profile_label(const char *label); // 0x51b6b0
    ~profile_label();                 // 0x434650 empty_hook_434650
};

// One key track of a skeleton (0x2c bytes per bone, skeleton +0x8c).
class anim_track {
public:
    quaternion get_rotation(int time, int flag);      // 0x551350 (flag always 0 here)
    vector3 get_position(int time);                   // 0x551090
private:
    ubyte _data[0x2c];
};

struct ca_trigger {                   // 8 bytes, skeleton +0xa8: "left-foot", "right-foot", ...
    const char *name;                 // +0x0 NULL = unused slot
    int time;                         // +0x4 anim time (ticks)
};

// One animation (.mvf), 0x4 + ... bytes; only the fields character_instance.cpp reads.
class skeleton {
public:
    vector3 get_root_velocity();      // 0x543fc0 skeleton__get_root_velocity
    ubyte _unk00[0x5c];
    bool is_state;                    // +0x5C state (looping, locomotion) anim; 0 = action
    bool root_rotation;               // +0x5D the anim turns the character (yaw taken from the root bone)
    ubyte _pad5e[2];
    int start;                        // +0x60 first tick
    int end;                          // +0x64 last tick
    ubyte _unk68[0x20];
    ubyte *bone_weights;              // +0x88 per bone, 0..10
    anim_track *tracks;               // +0x8C per bone
    ubyte _unk90[0x10];
    int fade_in;                      // +0xA0 ticks
    int fade_out;                     // +0xA4 ticks
    ca_trigger triggers[5];           // +0xA8
};

struct ca_bone {                      // 0xa0 bytes (ca_skin +0x4)
    ubyte _unk00[0x50];
    matrix43 bind;                    // +0x50 multiplied in front of the animated bone matrix
    int parent;                       // +0x80 -1 = root
    vector3 tag_offset;               // +0x84 character_instance__get_tag_pos
    int tag_value;                    // +0x90
    vector3 base_pos;                 // +0x94 character_instance__get_bone_bind_inverse
};
WC_LAYOUT_CHECK(ca_bone, sizeof(ca_bone) == 0xa0);

struct ca_bone_ext {                  // 0x38 bytes: extra bones (tags) after the real bones
    int _unk00;
    matrix43 m;                       // +0x04
    int parent;                       // +0x34
};
WC_LAYOUT_CHECK(ca_bone_ext, sizeof(ca_bone_ext) == 0x38);

struct ca_skin {                      // mesh +0x4
    int num_bones;                    // +0x00
    ca_bone *bones;                   // +0x04
    int _unk08;
    int num_ext;                      // +0x0C
    ca_bone_ext *ext;                 // +0x10
    int root_bone;                    // +0x14
};

struct ca_vim {                       // mesh +0x28 (.vim header)
    ubyte _unk00[0x10];
    int textures[6];                  // +0x10 bitmap handles
    int num_textures;                 // +0x28
    ubyte _unk2c[0x24];
    vector3 center;                   // +0x50 bounding sphere
    float radius;                     // +0x5C
};

struct ca_mesh {                      // character +0x1e4 (current LOD)
    int _unk00;
    ca_skin *skin;                    // +0x04
    ubyte _unk08[0x20];
    ca_vim *vim;                      // +0x28
};

// The base character (vsdk\ca\character.cpp); the instance points at it (+0x3c).
struct ca_character {
    char name[0x48];                  // +0x000 printed with %s (size up to the next field, ours)
    int num_anims;                    // +0x048
    ubyte _unk04c[8];
    skeleton *skeletons[100];         // +0x054
    ca_mesh *mesh;                    // +0x1E4
};
WC_LAYOUT_CHECK(ca_character, sizeof(ca_character) == 0x1e8);

// ---- vsdk\os\memory.cpp report helpers ------------------------------------------------------------
void mem_report_indent();             // 0x512600
void mem_report_unindent();           // 0x512610
int  mem_report_is_verbose();         // 0x512630
void mem_report_line(const char *label, const char *fmt, ...);   // 0x512640

// ---- vsdk\ca\character.cpp / skeleton.cpp -----------------------------------------------------------
void ca_init(int arg);                // 0x541290
void ca_close();                      // 0x541330
void ca_init_globals_2ee4598();       // 0x544a10
void ca_close_globals_544a40();       // 0x544a40 (no name in sum.symbols; label ours)

// ---- vsdk\os\console.cpp ------------------------------------------------------------------------
// Same as stubs_player_effectmem.h (0x10 bytes apart in .bss).
class console_command {
public:
    console_command(const char *name, const char *help, void (*func)());   // 0x50a180
private:
    char _data[0x10];
};
#define ARG_NONE  0x01
#define ARG_TRUE  0x40
#define ARG_FALSE 0x80
extern int calling_a_command_function;   // 0x25db218
extern int doing_help_for_comand;        // 0x25db2e0 (spelling as in Summoner 2)
extern int checking_status_for_command;  // 0x25dadf0
extern unsigned int arg_type;            // 0x25dae04
void process_command(int arg_types);     // 0x50a5b0
void print_to_console(const char *text, int flags);   // 0x50a5a0

#endif
