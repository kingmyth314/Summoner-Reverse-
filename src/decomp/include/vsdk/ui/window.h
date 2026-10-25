// window.h - Sum.exe v1.40 UI window (vsdk\ui\window.cpp, 0x528430-0x52bf92, COMDATs to 0x52bfdb)
// Written from the Sum.exe disassembly. Names from symbols/sum.symbols; the class name is ours (no RTTI).
#ifndef WINDOW_H
#define WINDOW_H

#include "vsdk/ui/ui.h"
#include "vsdk/ui/ui_gadgets.h"
#include "vsdk/ui/ui_container.h"
#include "vsdk/parse/parse.h"

// ui_window flags (+0x248), from ui_load_table and the methods that test them
#define UI_WINDOW_BORDER_MASK  0x03        // ui_window__draw border style when there is no background
#define UI_WINDOW_ALLOW_ARROWS 0x04        // "+Allow arrows"
#define UI_WINDOW_ALLOW_WRAP   0x08        // "+Allow wrap" (arrow navigation wraps around)
#define UI_WINDOW_ALWAYS_SEL   0x10        // "+Always a selection"
#define UI_WINDOW_MODAL        0x20        // "+Modal dialog" (0x21 with border style 1)
#define UI_WINDOW_MODELESS     0x40        // "+Modeless dialog"
#define UI_WINDOW_DESTROY      0x80000000  // destroy requested while ui_window_processing was set

// One screen or dialog (0x4ec bytes; vtable 0x581df4, 5 slots). The windows form a list through
// next / prev with ui_window_sentinel as head; the gadgets form a list through the gadgets member
// (a ui_gadget used only as sentinel), the top-level ones also a chain from first_gadget.
class ui_window MODERN_FINAL : public ui_element {
public:
    ui_window();                                      // 0x52b020
    ~ui_window();                                     // 0x52b0a0 (non-virtual)
    virtual void dtor();                              // slot 0, 0x52b630 ui_window__destroy
    virtual bool send_event(int event, int value);    // slot 1, 0x52aff0
    virtual bool is_disabled();                       // slot 4, 0x52bf80

    // The table parsers are thiscall on the window (sum.symbols: ui_parse_*, ui_load_table).
    int        parse_hotkey(TextFile *tf);                        // 0x528430 ui_parse_hotkey
    ui_button *parse_button(TextFile *tf, ui_element *parent);    // 0x528530 ui_parse_button
    ui_checkbox *parse_checkbox(TextFile *tf, ui_element *parent);// 0x528a40 ui_parse_checkbox
    ui_container *parse_container(TextFile *tf, ui_element *parent); // 0x528ec0 ui_parse_container
    ui_inputbox *parse_inputbox(TextFile *tf, ui_element *parent);// 0x5292f0 ui_parse_inputbox
    ui_listbox *parse_listbox(TextFile *tf, ui_element *parent);  // 0x5298c0 ui_parse_listbox
    ui_radio_button *parse_radio_button(TextFile *tf, ui_element *parent); // 0x529d70
    ui_slider_base *parse_slider(TextFile *tf, ui_element *parent);        // 0x52a180 ui_parse_slider
    ui_static *parse_static(TextFile *tf, ui_element *parent);    // 0x52a700 ui_parse_static
    void load_table(const char *filename, int path_id);           // 0x52b200 ui_load_table

    void clear_bitmaps();                             // 0x52abc0
    int  handle_arrow_key(int key);                   // 0x52abf0
    void nav_up(int x, int y);                        // 0x52aea0
    void nav_down(int x, int y);                      // 0x52aef0
    void nav_left(int x, int y);                      // 0x52af40
    void nav_right(int x, int y);                     // 0x52af90
    void init(int x, int y, int w, int h, int id, int flags);    // 0x52b100
    int  get_flags();                                 // 0x52b700
    void set_tooltip(const char *text, int font, int x, int y, int delay);   // 0x52b710
    ui_gadget *get_selected_gadget();                 // 0x52b7a0
    ui_gadget *get_mouse_over_gadget();               // 0x52b7b0
    bool contains_point(int x, int y);                // 0x52b7c0
    ui_gadget *gadget_at(int x, int y);               // 0x52b800
    ui_gadget *find_gadget_by_id(int id);             // 0x52b850
    ui_gadget *find_gadget_by_name(const char *name); // 0x52b890
    void load_mask_bitmap(const char *name);          // 0x52b8e0
    void set_mask_bitmap(int bm, const char *name);   // 0x52b930
    void load_background_bitmap(const char *name);    // 0x52b970
    void set_background_bitmap(int bm, const char *name);         // 0x52b9c0
    void draw();                                      // 0x52b9f0
    int  process(int key);                            // 0x52bc20
    int  get_mask_pixel(int x, int y);                // 0x52be90
    bool is_hidden();                                 // 0x52bee0
    bool is_shown();                                  // 0x52bf30
    // D8403 (amends D4206): not inline; defined at the end of the UI manager unit (ui_draw calls 0x530b80),
    // the last function before the os unit.
    ubyte *get_mask_data();                           // 0x530b80 ui_window__get_mask_data

    int flags;                    // +0x248 UI_WINDOW_*
    int group;                    // +0x24C ui_group_depth when created (ui_window__is_disabled)
    ui_gadget gadgets;            // +0x250 gadget list sentinel (window_prev +0x4b0, window_next +0x4b4)
    ui_gadget *first_gadget;      // +0x4C4 top-level gadgets, chained through next_sibling
    ui_gadget *mouse_over;        // +0x4C8
    ui_gadget *selected;          // +0x4CC keyboard selection
    int mask_bm;                  // +0x4D0 -1 none
    int background_bm;            // +0x4D4 -1 none
    ubyte *mask_data;             // +0x4D8 one byte per pixel (ui_window__get_mask_pixel)
    bool refresh;                 // +0x4DC forces one selection update in ui_window__process
    Color *colors;                // +0x4E0 ui_window_colors: [0] light, [1] dark, [2] face, [3] middle
    ui_window *next;              // +0x4E4
    ui_window *prev;              // +0x4E8
};
LAYOUT_CHECK_32(ui_window, sizeof(ui_window) == 0x4ec);

extern ui_gadget *ui_nav_best_gadget;   // 0x2cbf554
extern int ui_nav_best_distance;        // 0x2cbf558
extern bool ui_window_processing;       // 0x2cbf55c

void ui_stub_ret_52afe0();             // 0x52afe0 (bare ret, unreferenced; signature unknown)

#endif
