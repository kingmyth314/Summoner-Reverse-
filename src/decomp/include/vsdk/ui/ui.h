// vsdk/ui/ui.h - the UI element and gadget base classes, tooltips and the UI globals (vsdk\ui; the base
// classes are at 0x52c040-0x52c8c0 and 0x542cd0-0x543010, the unit file names are not in the binary).
// Not original source. Class names are ours (no RTTI; docs/sum-exe/vsdk-ui.md). One merged declaration of
// the two stub copies (DECISIONS.md D5). symbols/types.h ui_element is the C view of ui_element +
// ui_gadget together (its +0x248..+0x270 are ui_gadget's; a window keeps other data there).
#ifndef VSDK_UI_UI_H
#define VSDK_UI_UI_H

#include "vsdk/vsdk.h"
#include "vsdk/gr/gr.h"

class ui_element;
class ui_gadget;
class ui_window;
struct ui_tooltip;

// Slot 0 of every widget vtable is NOT the C++ destructor: it takes no flags argument and never frees the
// object (ui_element: free the tooltips; ui_gadget: also clear ui_current_gadget and the window;
// ui_container: also unlock the mask). It is declared as the virtual `dtor()`. The real destructors
// (~ui_element 0x542d40, ~ui_gadget 0x52c410) are non-virtual; the atexit handlers of the static
// ui_container objects jump straight to ~ui_gadget.

struct ui_tooltip_head {               // list sentinel inside the element (+0x124)
    ui_tooltip *next;                  // +0x0
    ui_tooltip *prev;                  // +0x4
};

// One tooltip (0x120 bytes, operator new, no constructor; empty destructor 0x55ef50).
struct ui_tooltip {
    ~ui_tooltip();                     // 0x55ef50 (scalar deleting dtor 0x543010)
    ui_tooltip *next;                  // +0x000
    ui_tooltip *prev;                  // +0x004
    ui_element *owner;                 // +0x008
    char text[0x100];                  // +0x00C
    int delay;                         // +0x10C (ui_button__set_tooltip's "delay"; meaning unsure)
    Color *colors;                     // +0x110 ui_tooltip_colors
    int x;                             // +0x114 offset from the owner's screen position
    int y;                             // +0x118
    int font;                          // +0x11C -1 = default
};
LAYOUT_CHECK(ui_tooltip, sizeof(ui_tooltip) == 0x120);

class ui_element {                     // vtable 0x581ea0 (5 slots)
public:
    ui_element();                      // 0x542d20
    ~ui_element();                     // 0x542d40 (non-virtual)
    virtual void dtor();               // slot 0, 0x542cd0 ui_element__free_tooltips
    virtual bool send_event(int event, int value) = 0;    // slot 1, _purecall (0x56e903)
    virtual void set_disabled(bool disabled);             // slot 2, 0x542f10
    virtual void set_enabled(bool enabled);               // slot 3, 0x542f70
    virtual bool is_disabled();                           // slot 4, 0x543000

    void init(int kind, int x, int y, int w, int h, int id);   // 0x542da0
    int  get_kind();                   // 0x542e10
    void get_size(int *w, int *h);     // 0x542e80
    bool is_hidden();                  // 0x542fe0
    bool is_shown();                   // 0x542ff0
    void set_size(int w, int h);       // 0x542e60 (decomp53, state 6)

    ubyte initialized;                 // +0x004 0 from the constructor, 1 from init
    int kind;                          // +0x008 1 window, 0 gadget
    int x, y, w, h;                    // +0x00C rectangle
    ubyte hidden;                      // +0x01C
    ubyte disabled;                    // +0x01D
    char name[0x102];                  // +0x01E
    int id;                            // +0x120 default event value
    ui_tooltip_head tooltips;          // +0x124
    ubyte _unk12c[0x118];              // +0x12C
    bool (*callback)(int event, int value);   // +0x244 cdecl (ui_window::send_event returns its result)
};
LAYOUT_CHECK(ui_element, sizeof(ui_element) == 0x248);

class ui_gadget : public ui_element {  // vtable 0x581e08 (9 slots)
public:
    ui_gadget();                       // 0x52c3f0
    ~ui_gadget();                      // 0x52c410 ui_gadget__dtor_base (non-virtual)
    virtual void dtor();                                  // slot 0, 0x52c3d0 ui_gadget__destroy
    virtual bool send_event(int event, int value);        // slot 1, 0x52c330
    virtual void set_disabled(bool disabled);             // slot 2, 0x52c890
    virtual void set_enabled(bool enabled);               // slot 3, 0x52c8c0
    virtual bool is_disabled();                           // slot 4, 0x52c530
    virtual int  process(int key);     // slot 5, 0x52c390 ui_gadget__process (returns key)
    virtual int  get_mask_pixel(int x, int y);            // slot 6, 0x52c6f0
    virtual void draw();               // slot 7, 0x52c840 ui_gadget__draw
    virtual bool is_selectable();      // slot 8, 0x52c880

    void init(ui_element *window, int type, int x, int y, int w, int h, int id, bool owned); // 0x52c040
    void find_nearest_up(int x, int y);                   // 0x52c0f0 (result in ui_nav_best_gadget)
    void find_nearest_down(int x, int y);                 // 0x52c180
    void find_nearest_left(int x, int y);                 // 0x52c210
    void find_nearest_right(int x, int y);                // 0x52c2a0
    int  get_screen_x(int offset);     // 0x52c420
    int  get_screen_y(int offset);     // 0x52c460
    bool is_owned();                   // 0x52c4a0
    bool is_hidden();                  // 0x52c4b0
    void set_parent(ui_gadget *parent);                   // 0x52c570
    void set_focus();                  // 0x52c600 event 4 to ui_current_gadget, event 3 to this (decomp51)
    void clear_focus();                // 0x52c630
    int  get_type();                   // 0x52c680
    void set_hotkey(int key, int slot);                   // 0x52c690
    ui_gadget *hit_test(int x, int y);                    // 0x52c730
    void set_hotspot(int size);        // 0x52c800
    void draw_bevel(Color *light, Color *dark, int width, int inset);   // 0x55e2e0 (last two unsure)

    bool owned;                        // +0x248
    int hotkeys[2];                    // +0x24C -1 from ui_gadget__clear_hotkeys
    int gadget_type;                   // +0x254 UI_TYPE_*
    int _unk258;                       // +0x258 hotspot (zeroed by init)
    ui_element *window;                // +0x25C owning ui_window
    ui_gadget *window_prev;            // +0x260 the window's gadget list (sentinel window +0x250)
    ui_gadget *window_next;            // +0x264
    ui_gadget *parent;                 // +0x268
    ui_gadget *first_child;            // +0x26C
    ui_gadget *next_sibling;           // +0x270
};
LAYOUT_CHECK(ui_gadget, sizeof(ui_gadget) == 0x274);

// ui_gadget::gadget_type
#define UI_TYPE_CONTAINER 1
#define UI_TYPE_BUTTON    2
#define UI_TYPE_STATIC    3
#define UI_TYPE_CHECKBOX  4
#define UI_TYPE_RADIO     5
#define UI_TYPE_INPUTBOX  6
#define UI_TYPE_SLIDER    7
#define UI_TYPE_LISTBOX   8

// The id a send_event value of -99999 stands for (use the element's own id, ui_window::send_event).
#define UI_USE_ID         (-99999)    // 0xfffe7961
#define UI_NO_ID          (-99998)    // 0xfffe7962 ui_load_table's default and ui_window::draw's value

// Events sent to the gadgets (ui_gadget::send_event) and to the window callback (window.cpp).
#define UI_EVENT_DRAW_BEGIN 0
#define UI_EVENT_DRAW_END   1
#define UI_EVENT_MOUSE_LEFT 5
#define UI_EVENT_SELECTED   7
#define UI_EVENT_DESELECTED 8
// decomp51 (the editor dialogs and the load-game screen; names ours): a button press (value = the
// button's id), a list box selection change and a list box double click.
#define UI_EVENT_PRESSED      0x0c
#define UI_EVENT_LIST_SELECT  0x18
#define UI_EVENT_LIST_DBLCLICK 0x19

// ---- globals ---------------------------------------------------------------------------------------
// decomp53 (state 6): windows of another group report disabled (ui_window::is_disabled).
void ui_push_group();                 // 0x530910
void ui_pop_group();                  // 0x530920
extern ui_window ui_window_sentinel;  // 0x2cbf718 (list head: next 0x2cbfbfc, prev 0x2cbfc00)
extern Color ui_window_colors[4];     // 0x2cbf670
extern Color ui_tooltip_colors[4];    // 0x2cbf708 (init_ui_tooltip_state)
extern Color ui_container_colors[4];  // 0x2cbfc20 (init_ui_container_state)
extern Color ui_button_colors[15];    // 0x2cbf6a8 (LoadSettings sets [7])
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
// decomp51: every window's draw, then callback (NULL in the load-game state), then the tooltip.
void ui_draw(void (*callback)());     // 0x52ff30
// The mouse buttons into the UI globals, then the windows' process; returns the key left over. The
// second argument is 1 at both callers (meaning not checked).
int  ui_process(int key, int arg);    // 0x530330
// The build stamp (vsdk\ui in sum.symbols): 3 unstamped (retail), 2 expired, 1 bad name checksum, 0 valid.
int  build_stamp_check();             // 0x52f450
void build_stamp_get_name(char *out, int size);   // 0x52f540 (build_stamp_show passes 512)

#endif
