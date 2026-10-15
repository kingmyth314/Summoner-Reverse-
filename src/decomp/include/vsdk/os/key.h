// vsdk/os/key.h - keyboard: DirectInput scan codes and key_to_ascii (0x509ca0; unit path not in the
// binary). Not original source. The KEY_* names are ours (DirectInput DIK_* values).
#ifndef VSDK_OS_KEY_H
#define VSDK_OS_KEY_H

#include "vsdk/vsdk.h"

#define KEY_ESC       0x01
#define KEY_BACKSPACE 0x0e
#define KEY_Q         0x10
#define KEY_R         0x13
#define KEY_I         0x17
#define KEY_ENTER     0x1c
#define KEY_S         0x1f
#define KEY_L         0x26
#define KEY_B         0x30
#define KEY_N         0x31
#define KEY_M         0x32
#define KEY_SPACE     0x39
#define KEY_F1        0x3b
#define KEY_F2        0x3c
#define KEY_PAD8      0x48
#define KEY_PAD4      0x4b
#define KEY_PAD6      0x4d
#define KEY_PAD2      0x50
#define KEY_UP        0xc8
#define KEY_PAGEUP    0xc9
#define KEY_LEFT      0xcb
#define KEY_RIGHT     0xcd
#define KEY_DOWN      0xd0
#define KEY_PAGEDOWN  0xd1
#define KEY_SHIFTED   0x1000          // modifier bits ored into a key code (window.cpp)
#define KEY_CTRLED    0x4000

int key_to_ascii(int key);            // 0x509ca0, 0xff = no character

// The DirectInput key names, indexed by scan code (256 pointers, 0x5a68c0-0x5a6cbf; [1] is "Esc").
// No name in sum.symbols; label ours.
extern char *key_names[256];          // 0x5a68c0

#endif
