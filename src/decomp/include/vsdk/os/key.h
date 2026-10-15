// vsdk/os/key.h - keyboard: DirectInput scan codes and key_to_ascii (0x509ca0; unit path not in the
// binary). Not original source. The KEY_* names are ours (DirectInput DIK_* values).
#ifndef VSDK_OS_KEY_H
#define VSDK_OS_KEY_H

#include "vsdk/vsdk.h"

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
void key_init(int layout, int fix_numlock);   // 0x509770 (LoadSettings: 0, 1)

// The DirectInput key names, indexed by scan code (256 pointers, 0x5a68c0-0x5a6cbf; [1] is "Esc").
// No name in sum.symbols; label ours.
extern char *key_names[256];          // 0x5a68c0

int  key_inkey();                     // 0x509ce0 next queued key code, 0 none (decomp47: cd_prompt_disc2)

#endif
