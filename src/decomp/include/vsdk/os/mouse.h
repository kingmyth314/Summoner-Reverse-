// vsdk/os/mouse.h - mouse state (sum.symbols module vsdk\os\mouse, code 0x500a30-0x501468; no path string:
// the file name is a placeholder, decomp49 wrote it as vsdk/mouse.cpp). Not original source.
// The globals whose types need <windows.h> / <dinput.h> (mouse_lock, mouse_dinput, mouse_di_device) are
// declared in the unit only.
#ifndef VSDK_OS_MOUSE_H
#define VSDK_OS_MOUSE_H

#include "vsdk/vsdk.h"
#include "vsdk/os/timestamp.h"

#define MOUSE_NUM_BUTTONS 3           // left, right, middle (mouse_msg_handler)

// One button (0x10 bytes; mouse_buttons 0x257ef90 latched per frame, mouse_buttons_live 0x257efe8 written by
// the message handler). Field names ours. The constructor (0x501440) is the implicit one: it only runs the
// RealtimeTimer constructor; the struct is copied as four dwords by mouse_update.
struct mouse_button {
    bool down;                        // +0x00
    bool double_clicked;              // +0x01 pressed again before the timer ran out
    int presses;                      // +0x04 press count since the last latch
    int releases;                     // +0x08
    RealtimeTimer timer;              // +0x0c double-click window (mouse_double_click_time)
};
LAYOUT_CHECK(mouse_button, sizeof(mouse_button) == 0x10);

extern int mouse_use_cursor;                      // 0x5a5734 (.data, 1: DirectInput is never used, D2903)
extern int mouse_double_click_time;               // 0x5a5738 (.data, 400 ms)
extern int mouse_screen_w;                        // 0x257ef84
extern int mouse_wheel;                           // 0x257ef88
extern int mouse_screen_h;                        // 0x257ef8c
extern mouse_button mouse_buttons[MOUSE_NUM_BUTTONS];        // 0x257ef90
extern int mouse_z;                               // 0x257efd8
extern int mouse_y;                               // 0x257efdc
extern int mouse_x;                               // 0x257efe0
extern mouse_button mouse_buttons_live[MOUSE_NUM_BUTTONS];   // 0x257efe8
extern bool mouse_di_active;                      // 0x257f020
extern bool mouse_inited;                         // 0x257f021
extern int mouse_dx;                              // 0x257f024
extern int mouse_dy;                              // 0x257f028
extern int mouse_dz;                              // 0x257f02c
extern bool mouse_hidden;                         // 0x257f030
extern bool keep_mouse_centered;                  // 0x257f031
extern int mouse_prev_x;                          // 0x257f034
extern int mouse_prev_y;                          // 0x257f038
extern int mouse_last_z;                          // 0x257f03c (sum.symbols module "-")

void mouse_update();                              // 0x500a90 (GameTick)
void mouse_set_pos(int x, int y);                 // 0x500c80 client coordinates, only in the foreground
void mouse_di_read();                             // 0x500cd0
void mouse_init(int w, int h);                    // 0x500de0
int  mouse_di_init();                             // 0x500e90 0, or -1 (always -1 in v1.40: mouse_use_cursor)
void mouse_close();                               // 0x501020 atexit
void mouse_di_close();                            // 0x501050
void mouse_flush();                               // 0x501090 clears deltas and button state
void mouse_mark_button(int button, bool down);    // 0x501100
void mouse_clear_clicks();                        // 0x5011a0 (ui_process)
int  mouse_get_pos(int *x, int *y, int *z);       // 0x5011e0 -1 (and zeros) before mouse_init
void mouse_move_to(int x, int y);                 // 0x501240
void mouse_set_pos_xyz(int x, int y, int z);      // 0x501280 (no caller)
bool mouse_is_down(int button);                   // 0x5012c0
bool mouse_was_double_clicked(int button);        // 0x5012e0
int  mouse_time_since_press(int button);          // 0x501300 (decomp35)
int  mouse_get_press_count(int button);           // 0x501320 (no caller)
int  mouse_get_release_count(int button);         // 0x501340 (no caller)
bool mouse_was_pressed(int button);               // 0x501360
bool mouse_was_released(int button);              // 0x501390
void mouse_get_delta(int *dx, int *dy, int *dz);  // 0x5013c0
bool mouse_is_hidden();                           // 0x5013f0 (no caller)
void mouse_set_hidden(bool hidden);               // 0x501400 (no caller)
void mouse_set_visible(bool visible);             // 0x501410 (no caller)
void mouse_keep_centered_enable();                // 0x501420 (no caller)
void mouse_keep_centered_disable();               // 0x501430 (no caller)
// (v, lo, hi): *v = lo below lo, hi above hi. Emitted after the mouse code, before frametime.cpp; called
// by mouse_update, gr_circle, gr_d3d_apply_tint and gr_draw_sphere. Moved here from gr_direct3d.h (D2902).
void clamp_int(int *v, int lo, int hi);           // 0x501450

extern int game_mouse_z;                          // 0x257ef80 (wheel; sum.symbols module vsdk\ui)

#endif
