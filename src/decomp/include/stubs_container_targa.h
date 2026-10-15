// stubs_container_targa.h - declarations of the Sum.exe v1.40 engine classes, functions and globals that
// ui_container.cpp and targautils.cpp use but do not define, and that no compatible header in
// src/decomp/include/ declares. Not original source. Names and addresses from symbols/sum.symbols;
// signatures read from the call sites and callees in build/sum.asm. Layouts give only what the two units
// touch (offsets in comments, from symbols/types.h ui_element where it exists).
//
// Compatibility: this header can be included together with stubs_credits_help.h (ui_container.cpp does)
// or with <stdio.h>/<string.h> (targautils.cpp does). It cannot be included together with
//  - stubs_sound_items_registry.h: both define class File (that one has only find(); targautils needs
//    open/size/read/seek/tell/write..., see NOTES.md);
//  - symbols/types.h (through stubs_menus_dialogue.h): types.h defines ui_element as a C struct, here it
//    is the C++ base class whose vtable (0x581ea0) the gadget classes extend.
#ifndef STUBS_CONTAINER_TARGA_H
#define STUBS_CONTAINER_TARGA_H

typedef unsigned char ubyte;    // same typedef as stubs_sound_items_registry.h
typedef unsigned short ushort;
struct Color;                   // 4 bytes r, g, b, a (stubs_credits_help.h)

// ---- debug (0x531cb0) -------------------------------------------------------------------------
// Same declaration as stubs_sound_items_registry.h, repeated here because that header cannot be
// included next to stubs_credits_help.h (both define TextFile). Callers write the error macro as
//     for (;;) DebugError(file, line, msg);
void DebugError(const char *file, int line, const char *msg);

// ---- vsdk\bmpman\bmpman.cpp ---------------------------------------------------------------------
// bm_lock returns the bitmap type (0 = failure), the pixel data through *data and the pixel format
// through *format.
int  bm_lock(int bm, ubyte **data, int *format);     // 0x503650
void bm_unlock(int bm);                              // 0x503b50 bm_free_data(bm, 0)
void bm_get_dimensions(int bm, int *w, int *h);      // 0x503500 (also in stubs_assess_pickup.h)
// The format out parameter ui_container__set_mask_bitmap passes to bm_lock: the first dword after
// bm_bitmaps (0x2591c78 + n * 0x48), so it is a bmpman.cpp global. No name in sum.symbols.
extern int bm_lock_format_unused;                    // 0x25c2ff8

// ---- vsdk\gr ------------------------------------------------------------------------------------
namespace gr {
void reset_clip();                                   // 0x5069a0 gr__reset_clip
}
void gr_line(float x1, float y1, float x2, float y2, unsigned int mode);  // 0x507310
extern unsigned int gr_line_mode;                    // 0x25d8ce4

// ---- vsdk\ui: element and gadget base classes ----------------------------------------------------
// There is no RTTI; class names are ours (docs/sum-exe/vsdk-ui.md). Slot 0 of every widget vtable is
// NOT the C++ destructor: it takes no flags argument and never frees the object (ui_element: free the
// tooltips; ui_gadget: also clear ui_current_gadget and the window; ui_container: also unlock the mask).
// The real destructors (~ui_element 0x542d40, ~ui_gadget 0x52c410) are non-virtual; the atexit handlers
// of the static ui_container objects jump straight to ~ui_gadget, so ui_container declares none.
class ui_gadget;
struct ui_tooltip;

struct ui_tooltip_head {               // list sentinel inside the element (+0x124)
    ui_tooltip *next;                  // +0x0
    ui_tooltip *prev;                  // +0x4
};

// One tooltip (0x120 bytes, operator new, no constructor; empty destructor 0x55ef50).
struct ui_tooltip {
    ~ui_tooltip();                     // 0x55ef50 (scalar deleting dtor 0x543010)
    ui_tooltip *next;                  // +0x000
    ui_tooltip *prev;                  // +0x004
    class ui_element *owner;           // +0x008
    char text[0x100];                  // +0x00C
    int delay;                         // +0x10C (ui_button__set_tooltip's "delay"; meaning unsure)
    Color *colors;                     // +0x110 ui_tooltip_colors
    int x;                             // +0x114 offset from the owner's screen position
    int y;                             // +0x118
    int font;                          // +0x11C -1 = default (ui_button__set_tooltip)
};
extern Color ui_tooltip_colors[4];     // 0x2cbf708 (a ui_colors4-style object, init_ui_tooltip_state)

class ui_element {                     // vtable 0x581ea0 (5 slots)
public:
    ui_element();                      // 0x542d20
    ~ui_element();                     // 0x542d40
    virtual void dtor();               // slot 0, 0x542cd0 ui_element__free_tooltips
    virtual bool send_event(int event, int value) = 0;   // slot 1, _purecall (0x56e903)
    virtual void set_disabled(bool disabled);            // slot 2, 0x542f10
    virtual void set_enabled(bool enabled);              // slot 3, 0x542f70
    virtual bool is_disabled();                          // slot 4, 0x543000

    ubyte initialized;                 // +0x004
    int kind;                          // +0x008 1 window, 0 gadget
    int x, y, w, h;                    // +0x00C rectangle
    ubyte hidden;                      // +0x01C
    ubyte disabled;                    // +0x01D
    char name[0x102];                  // +0x01E
    int id;                            // +0x120
    ui_tooltip_head tooltips;          // +0x124
    ubyte _unk12c[0x118];              // +0x12C
    void *callback;                    // +0x244
};

class ui_gadget : public ui_element {  // vtable 0x581e08 (9 slots)
public:
    ui_gadget();                       // 0x52c3f0
    ~ui_gadget();                      // 0x52c410 ui_gadget__dtor_base
    virtual void dtor();                                 // slot 0, 0x52c3d0 ui_gadget__dtor
    virtual bool send_event(int event, int value);       // slot 1, 0x52c330
    virtual void set_disabled(bool disabled);            // slot 2, 0x52c890
    virtual void set_enabled(bool enabled);              // slot 3, 0x52c8c0
    virtual bool is_disabled();                          // slot 4, 0x52c530
    virtual int  process(int key);     // slot 5, 0x52c390 ui_gadget__process_children (returns key)
    virtual int  get_mask_pixel(int x, int y);           // slot 6, 0x52c6f0
    virtual void draw();               // slot 7, 0x52c840 ui_gadget__draw_children
    virtual bool is_selectable();      // slot 8, 0x52c880

    void init(ui_element *window, int type, int x, int y, int w, int h, int id, bool owned); // 0x52c040
    int  get_screen_x(int offset);     // 0x52c420
    int  get_screen_y(int offset);     // 0x52c460
    bool is_hidden();                  // 0x52c4b0
    void draw_bevel(Color *light, Color *dark, int width, int inset);   // 0x55e2e0 (last two unsure)

    bool owned;                        // +0x248
    int hotkeys[2];                    // +0x24C
    int gadget_type;                   // +0x254 1 container
    int _unk258;                       // +0x258
    ui_element *window;                // +0x25C
    ui_gadget *window_prev;            // +0x260
    ui_gadget *window_next;            // +0x264
    ui_gadget *parent;                 // +0x268
    ui_gadget *first_child;            // +0x26C
    ui_gadget *next_sibling;           // +0x270
};

// ---- vsdk\ui (other units) ------------------------------------------------------------------------
extern Color ui_container_colors[4];   // 0x2cbfc20 (init_ui_container_state)

// ---- vsdk\vfile -----------------------------------------------------------------------------------
#ifndef PATH_ANY
#define PATH_ANY 9999999               // 0x98967f, the "search every path" id
#endif

// File (0x114 bytes). Superset of the File in stubs_sound_items_registry.h; see the note at the top.
class File {
public:
    File();                                              // 0x5140d0
    ~File();                                             // 0x5140f0 closes if open
    int  open(const char *name, int mode, int path_id);  // 0x514630 mode 1 read, 2 write; < 0 failure
    void close();                                        // 0x514740
    int  size(const char *name, int path_id);            // 0x514860 (called as size(NULL, PATH_ANY))
    int  seek(int pos, int origin);                      // 0x514910 < 0 failure
    int  tell();                                         // 0x5149f0
    int  error();                                        // 0x514a40
    int  read(void *buf, int len, int min_ver, int unused);   // 0x525fe0
    char  read_int8(int min_ver, char def);              // 0x525850
    ubyte read_uint8(int min_ver, ubyte def);            // 0x5258a0
    short read_int16(int min_ver, short def);            // 0x5258f0
    int  write(const void *buf, int len);                // 0x526410
    void write_uint8(ubyte v);                           // 0x526220
    void write_uint16(ushort v);                         // 0x526240

    int path_id;                       // +0x000 path id of the found file (File__find_in_path)
private:
    char _data[0x110];
};
void file_change_ext(char *out, const char *name, const char *ext);   // 0x5130a0

// ---- MPEG-2 bit reader (targautils.cpp m2v_* helpers) ---------------------------------------------
// The m2v_* helpers take a pointer to an object whose vtable has get_bits at +0x14, show_bits at +0x18
// and next_start_code at +0x1c. No such class (and no caller of the helpers) exists in Sum.exe v1.40,
// so the class name and slots 0-4 are placeholders.
class m2v_bitstream {
public:
    virtual void vfunc0();
    virtual void vfunc1();
    virtual void vfunc2();
    virtual void vfunc3();
    virtual void vfunc4();
    virtual int  get_bits(int n);      // slot 5 (+0x14)
    virtual int  show_bits(int n);     // slot 6 (+0x18)
    virtual void next_start_code();    // slot 7 (+0x1c)
};

#endif
