// vsdk/ui/ui.h - the UI element and gadget base classes, tooltips and the UI globals (vsdk\ui; the base
// classes are at 0x52c040-0x52c8c0 and 0x542cd0-0x543010, the unit file names are not in the binary).
// Not original source. Class names are ours (no RTTI; docs/sum-exe/vsdk-ui.md). One merged declaration of
// the two stub copies (DECISIONS.md D5). symbols/types.h ui_element is the C view of ui_element +
// ui_gadget together (its +0x248..+0x270 are ui_gadget's; a window keeps other data there).
#ifndef VSDK_UI_UI_H
#define VSDK_UI_UI_H

#include "vsdk/vsdk.h"
#include "vsdk/gr/gr.h"
#include "vsdk/os/timestamp.h"

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
    // +0x11C -1 = default. decomp62: ui_draw picks a container's or window's tooltip by comparing this
    // field with the mask pixel under the mouse (ui_container::set_tooltip's "font" is the mask value).
    int font;
    // decomp62 (ui_draw): text at the owner's screen position plus (x, y) in a box.
    void draw();                       // 0x55ef60 ui_tooltip__draw
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
    void get_position(int *x, int *y); // 0x542e40 (decomp62, ui_process)
    bool is_hidden();                  // 0x542fe0
    bool is_shown();                   // 0x542ff0
    // decomp55 (the script editor): hidden = hide / = !show; hiding the focused gadget sends it event 4
    // and clears ui_current_gadget.
    void set_hidden(bool hide);        // 0x542ea0 ui_element__set_hidden
    void set_visible(bool show);       // 0x542ed0 ui_element__set_visible
    void set_size(int w, int h);       // 0x542e60 (decomp53, state 6)
    void set_position(int x, int y);   // 0x542e20 (decomp67; no caller found)

    ubyte initialized;                 // +0x004 0 from the constructor, 1 from init
    int kind;                          // +0x008 1 window, 0 gadget
    int x, y, w, h;                    // +0x00C rectangle
    ubyte hidden;                      // +0x01C
    ubyte disabled;                    // +0x01D
    char name[0x102];                  // +0x01E
    int id;                            // +0x120 default event value
    // decomp67 (D4707): the list sentinel is a whole ui_tooltip (0x120 bytes up to +0x244): ~ui_element
    // destroys it (ui_tooltip::~ui_tooltip on this + 0x124, 0x542d7e). Was ui_tooltip_head tooltips and
    // ubyte _unk12c[0x118].
    ui_tooltip tooltips;               // +0x124
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
    bool is_shown();                   // 0x52c4f0 window, parent and gadget all shown (decomp55)
    void set_parent(ui_gadget *parent);                   // 0x52c570
    void set_focus();                  // 0x52c600 event 4 to ui_current_gadget, event 3 to this (decomp51)
    void clear_focus();                // 0x52c630
    int  get_type();                   // 0x52c680
    void set_hotkey(int key, int slot);                   // 0x52c690
    // decomp61 (ui_button, ui_static).
    ui_element *get_window();          // 0x52c5c0 +0x25c
    bool is_selected();                // 0x52c660 the window's keyboard selection (+0x4cc) is this
    bool is_mouse_over();              // 0x52c670 the window's gadget under the mouse (+0x4c8) is this
    bool has_hotkey(int key);          // 0x52c6c0 key > 0 and one of the two hotkeys
    void fill_inset(Color c, int inset);                  // 0x55e280 gr_rect over the rect shrunk by inset
    void draw_raised(Color *colors, int width);           // 0x55e320 face colors[2], bevel light / dark
    void draw_sunken(Color *colors, int width);           // 0x55e3a0 the bevel colours swapped
    ui_gadget *hit_test(int x, int y);                    // 0x52c730
    void set_hotspot(int size);        // 0x52c800
    void draw_bevel(Color *light, Color *dark, int width, int inset);   // 0x55e2e0 (last two unsure)
    // decomp62 (the gadget base unit, vsdk/unnamed_gadget_0x52bfe0.cpp).
    void unlink();                     // 0x52bfe0 out of the parent's child chain or the window's chain
    int  identity(int x);              // 0x52c0e0 returns x (no caller)
    ui_gadget *get_parent();           // 0x52c5d0 +0x268 (no caller)
    ui_gadget *get_first_child();      // 0x52c5e0 +0x26c (no caller)
    ui_gadget *get_next_sibling();     // 0x52c5f0 +0x270 (no caller)
    void clear_hotkeys();              // 0x52c6b0 both -1
    void clear_hotspot();              // 0x52c820 (no caller)
    int  get_hotspot();                // 0x52c830 (no caller)
    void clear_window_refs();          // 0x52c8f0 the window's mouse-over / selected pointers

    bool owned;                        // +0x248
    int hotkeys[2];                    // +0x24C -1 from ui_gadget__clear_hotkeys
    int gadget_type;                   // +0x254 UI_TYPE_*
    // +0x258 (decomp62: was _unk258) the mask value a click must hit (hit_test); < 0 any, 0 from init.
    int hotspot;
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

// decomp62 (D4202): the colour sets are structs of named Colors, not Color arrays. Their static
// initialisers (init_ui_* 0x52f5d0-0x52f700) call implicit constructors that construct each Color in
// turn (0x5309c0-0x530b50, sum.symbols ui_colors*__ctor; Color() is the folded empty 0x505530, called
// because the UI manager unit is built without inline expansion); a Color array would go through the
// vector constructor iterator instead. Struct and member names are ours (the ui_colors* names are
// sum.symbols' constructor names). operator Color *() is a compatibility shim for the units that take the
// set's address (colors = ui_button_colors; ui_window_colors[2].set(...)): it is ours, and an /Ob0 unit
// would call it, so new /Ob0 code uses &set.c0 instead.
struct ui_colors2 {                   // 8 bytes, ctor 0x530b30
    Color c0, c1;
    operator Color *() { return &c0; }
};
struct ui_colors3 {                   // 0xc bytes, ctor 0x5309e0
    Color c0, c1, c2;
    operator Color *() { return &c0; }
};
struct ui_colors4 {                   // 0x10 bytes, ctor 0x530a70 (four Colors, no ui_colors3 base)
    Color c0, c1, c2, c3;
    operator Color *() { return &c0; }
};
struct ui_window_color_set : ui_colors3 {   // 0x10 bytes, ctor 0x5309c0 ui_window_colors__ctor
    Color c3;
};
struct ui_colors5 : ui_colors3 {      // 0x14 bytes, ctor 0x530af0
    Color c3, c4;
};
struct ui_colors6 : ui_colors3 {      // 0x18 bytes, ctor 0x530ac0
    Color c3, c4, c5;
};
struct ui_colors2_x3 {                // 0x18 bytes, ctor 0x530b10
    ui_colors2 g0, g1, g2;
    operator Color *() { return &g0.c0; }
};
struct ui_colors3_x2 {                // 0x18 bytes, ctor 0x530a20
    ui_colors3 g0, g1;
    operator Color *() { return &g0.c0; }
};
struct ui_colors4_x4 {                // 0x40 bytes, ctor 0x530a40
    ui_colors4 g0, g1, g2, g3;
    operator Color *() { return &g0.c0; }
};
struct ui_colors5_x3 {                // 0x3c bytes, ctor 0x530a00
    ui_colors5 g0, g1, g2;
    operator Color *() { return &g0.c0; }
};
struct ui_colors6_x2 {                // 0x30 bytes, ctor 0x530aa0
    ui_colors6 g0, g1;
    operator Color *() { return &g0.c0; }
};
struct ui_colors8 {                   // 0x20 bytes, ctor 0x530b50
    ui_colors3 g0, g1;
    ui_colors2 g2;
    operator Color *() { return &g0.c0; }
};
LAYOUT_CHECK(ui_colors5_x3, sizeof(ui_colors5_x3) == 0x3c);
LAYOUT_CHECK(ui_colors8, sizeof(ui_colors8) == 0x20);

extern ui_window_color_set ui_window_colors;   // 0x2cbf670 [0] light, [1] dark, [2] face, [3] middle
// 0x2cbf708: three Colors (the constructor builds three, ui_init sets three, ui_tooltip::draw reads
// [0]..[2]); sum.symbols' Color[4] is one too many (D4202).
extern ui_colors3 ui_tooltip_colors;
extern ui_colors4 ui_container_colors;   // 0x2cbfc20 (init_ui_container_state)
extern ui_colors5_x3 ui_button_colors;   // 0x2cbf6a8 (LoadSettings sets [7])
// decomp61: three sets of three (background, text, text shadow): [0..2] shown, [3..5] hidden.
extern ui_colors3_x2 ui_static_colors;   // 0x2cbf6e8
extern ui_colors4_x4 ui_checkbox_colors; // 0x2cbf630 (ui_checkbox / ui_radio_button create, +0x390)
extern ui_colors6_x2 ui_inputbox_colors; // 0x2cbfc70 (ui_inputbox::create, +0x57c)
extern ui_colors5 ui_slider_colors;      // 0x2cbfc08 (ui_slider::create, +0x29c)
extern ui_colors8 ui_listbox_colors;     // 0x2cbfc30 (ui_listbox::create, +0x318)
extern ui_colors2_x3 ui_listbox_entry_colors;   // 0x2cbfc50 (ui_listbox::add_entry, entry +0x38)
// decomp61: sound index 2 is the button press (ui_button::process).
void ui_play_sound(int index);        // 0x52f720 snd_play of ui_sounds[index], else entry 1 for index > 1
// decomp61 (ui_button::process): the drop target of the current drag and the "dropped" flag that
// ui_process reads (the dragged button itself: ui_drag_gadget, vsdk/ui/ui_gadgets.h).
extern ui_gadget *ui_drop_target;     // 0x2cbfcbc
extern bool ui_drag_dropped;          // 0x2cbfcc0
extern ui_gadget *ui_current_gadget;  // 0x2cbfcc4
extern int ui_group_depth;            // 0x2cbfcc8
extern int ui_mouse_x;                // 0x2cbfca8
extern int ui_mouse_y;                // 0x2cbfcac
extern bool ui_left_pressed;          // 0x2cbf628
extern bool ui_right_released;        // 0x2cbf629
extern bool ui_right_pressed;         // 0x2cbf62a
extern bool ui_left_double_clicked;   // 0x2cbf700
extern bool ui_left_released;         // 0x2cbfca4
extern int ui_key;                    // 0x2cbfc68 the key ui_process got (the editor's callbacks test it; decomp55)
extern bool ui_right_double_clicked;  // 0x2cbfc04
void ui_init();                       // 0x52f760 (guarded, once)
void ui_draw_bevel(int x, int y, int w, int h, Color *light, Color *dark, int width, int inset); // 0x55e040
// decomp66 (the slider unit): colors[2] filled inside the bevel, light colors[0] / dark colors[1]
// (sunken: swapped).
void ui_draw_raised(int x, int y, int w, int h, Color *colors, int width);   // 0x55e1a0
void ui_draw_sunken(int x, int y, int w, int h, Color *colors, int width);   // 0x55e210
// decomp51: every window's draw, then callback (NULL in the load-game state), then the tooltip.
void ui_draw(void (*callback)());     // 0x52ff30
// The mouse buttons into the UI globals, then the windows' process; returns the key left over.
// decomp62 (D4203): the second argument is a bool (`cmp byte ptr [esp+0x38], 1`): with it, the mouse
// clicks are cleared (mouse_clear_clicks) when a window changed the click flags. 1 at both callers.
int  ui_process(int key, bool clear_clicks);   // 0x530330
// decomp55 (the script editor): the topmost shown modal window that is enabled (NULL when none).
ui_window *ui_get_top_modal_window(); // 0x530970
// decomp62: the rest of the UI manager (vsdk/unnamed_ui_manager_0x52f570.cpp).
void ui_destroy_all_windows();        // 0x5308e0 slot 0 of the first window until the list is empty (no caller)
ui_gadget *ui_find_gadget_by_name(const char *name);   // 0x530930 over all windows (no caller)
extern RealtimeTimer ui_tooltip_timer;   // 0x2cbf680 1 s after the hovered gadget changes
extern int ui_sounds[8];              // 0x2cbf684 -1 from ui_init
extern bool ui_tooltip_ready;         // 0x2cbf6e4
extern ui_element *ui_tooltip_gadget; // 0x2cbfccc the gadget or window whose tooltip ui_draw shows
extern ui_gadget *ui_last_gadget;     // 0x2cbfcd4 the gadget the last press hit
extern int ui_mouse_wheel;            // 0x2cbfca0 (mouse_get_delta's third output)

// The build stamp (vsdk\ui in sum.symbols; decomp62: vsdk/unnamed_build_stamp_0x52f450.cpp): 3 unstamped
// (retail), 2 expired, 1 bad name checksum, 0 valid.
class String;
int  build_stamp_check();             // 0x52f450
void build_stamp_get_name_String(String *out);    // 0x52f530 (no caller)
// decomp62: a plain strcpy, the size is not read.
void build_stamp_get_name(char *out, int size);   // 0x52f540 (build_stamp_show passes 512)
extern int build_stamp_checksum;      // 0x5a9604 retail 0x01010101 (placeholder)
extern long build_stamp_expiry;       // 0x5a9608 time_t, retail 0x02020202 (placeholder)
extern char build_stamp_name[72];     // 0x5a960c (69 to 72 bytes; D4204)

#endif
