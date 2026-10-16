// vsdk/os/key.h - keyboard: DirectInput scan codes and key_to_ascii (0x509ca0; unit path not in the
// binary). Not original source. The KEY_* names are ours (DirectInput DIK_* values).
#ifndef VSDK_OS_KEY_H
#define VSDK_OS_KEY_H

#include "vsdk/vsdk.h"
#include <windows.h>                    // UINT / WPARAM / LPARAM (key_msg_handler; decomp72)

#define KEY_ESC       0x01
#define KEY_1         0x02
#define KEY_6         0x07
#define KEY_BACKSPACE 0x0e
#define KEY_TAB       0x0f
#define KEY_Q         0x10
#define KEY_R         0x13
#define KEY_I         0x17
#define KEY_ENTER     0x1c
#define KEY_LCONTROL  0x1d
#define KEY_S         0x1f
#define KEY_F         0x21
#define KEY_H         0x23
#define KEY_L         0x26
#define KEY_X         0x2d
#define KEY_C         0x2e
#define KEY_B         0x30
#define KEY_N         0x31
#define KEY_M         0x32
#define KEY_SPACE     0x39
#define KEY_F1        0x3b
#define KEY_F2        0x3c
#define KEY_F3        0x3d
#define KEY_F4        0x3e
#define KEY_F5        0x3f
#define KEY_F6        0x40
#define KEY_F7        0x41
#define KEY_F8        0x42
#define KEY_F9        0x43
#define KEY_NUMLOCK   0x45
#define KEY_PAD8      0x48
#define KEY_PAD4      0x4b
#define KEY_PAD6      0x4d
#define KEY_PAD2      0x50
#define KEY_F12       0x58
#define KEY_RCONTROL  0x9d
// decomp35 (chain_attack_select):
#define KEY_LSHIFT    0x2a
#define KEY_RSHIFT    0x36
#define KEY_LALT      0x38
#define KEY_CAPSLOCK  0x3a
#define KEY_RALT      0xb8
#define KEY_UP        0xc8
#define KEY_PAGEUP    0xc9
#define KEY_LEFT      0xcb
#define KEY_RIGHT     0xcd
#define KEY_DOWN      0xd0
#define KEY_PAGEDOWN  0xd1
#define KEY_DELETE    0xd3
// Modifier bits ored into a key code by key_mark (0x5094c3-0x5094f1: 0x1000 from scan codes 0x2a / 0x36,
// 0x2000 from 0x38 / 0xb8, 0x4000 from 0x1d / 0x9d; DECISIONS.md D44).
#define KEY_SHIFTED   0x1000
#define KEY_ALTED     0x2000
#define KEY_CTRLED    0x4000

int  key_to_ascii(int key);           // 0x509ca0, 0xff = no character
bool key_is_down(int key);            // 0x509f00 (returns the state byte key_down_state[key])
// decomp72: fix_numlock is a bool (stored as a byte and compared with 1, 0x5097c5; was int, D5312).
void key_init(int layout, bool fix_numlock);   // 0x509770 (LoadSettings: 0, 1)

// The DirectInput key names, indexed by scan code (256 pointers, 0x5a68c0-0x5a6cbf; [1] is "Esc").
// No name in sum.symbols; label ours.
extern char *key_names[256];          // 0x5a68c0

int  key_inkey();                     // 0x509ce0 next queued key code, 0 none (decomp47: cd_prompt_disc2)
// decomp52 (camera_key_yaw / camera_key_zoom): seconds the key has been held (0 when up).
float key_down_time(int key);         // 0x509bb0

// decomp51 (PollInputEvents, game/unnamed_frame_0x4323e0.cpp).
#define KEY_LAPOSTRO  0x29            // ` / ~ (the debug key: key_mark adds 0x800 to keys pressed with it)
#define KEY_SYSRQ     0xb7            // Print Screen (key_on_keydown maps VK 0x2c to it)
// PollInputEvents masks and tests 0x8000 (`and ah, 0x7f` at 0x432789, case 0x800e at 0x4327d9), but
// key_mark sets 0x800 for the debug key (0x509508), so no queued key carries 0x8000 (D3103). Name ours.
#define KEY_DEBUGGED  0x8000
void key_flush();                     // 0x509ab0 clears the queue and key states
int  key_get_press_count(int key);    // 0x509e20 returns and clears key_press_counts[key]

// ---- decomp72: the rest of the keyboard unit (vsdk/unnamed_key_0x5093a0.cpp, 0x5093a0-0x509f57) --------
// A key event from the window messages: (scan code, 1 down / 0 up, latency in ms).
void key_mark(int code, int state, int latency);      // 0x5093a0
int  key_remap_layout(int code);                      // 0x509560 key_layout 1-4 scan code swaps
void key_numlock_off();                               // 0x509630
void key_on_keydown(int vk, int lparam, int latency); // 0x509660 WM_KEYDOWN / WM_SYSKEYDOWN
void key_on_keyup(int vk, int lparam, int latency);   // 0x5096e0 WM_KEYUP / WM_SYSKEYUP
void key_close();                                     // 0x509810 atexit handler of key_init
void key_numlock_on();                                // 0x509850
void key_msg_handler(UINT msg, WPARAM wparam, LPARAM lparam);   // 0x509880 (os_add_msg_handler)
int  key_numlock_fix();                               // 0x509a00 -1 on Windows NT, else 0
bool key_numlock_is_on();                             // 0x509a50
int  key_getch();                                     // 0x509a80 waits for a key; 0 when not initialised
void key_clear(int key);                              // 0x509b50
void key_putkey(int code);                            // 0x509d40 (no full-queue check)
int  key_get_shift_status();                          // 0x509d90 KEY_SHIFTED / KEY_ALTED / KEY_CTRLED, 0x800
int  key_get_release_count(int key);                  // 0x509e70 returns and clears key_release_counts[key]
int  key_queue_not_empty();                           // 0x509ec0
void key_enable_debug_modifier();                     // 0x509f10
void key_disable_debug_modifier();                    // 0x509f20
bool key_is_modifier(int code);                       // 0x509f30 the shift, ctrl, alt and ` scan codes

#endif
