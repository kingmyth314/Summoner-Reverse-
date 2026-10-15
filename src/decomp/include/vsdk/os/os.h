// vsdk/os/os.h - the main window, the message pump and the process-level helpers (vsdk\os in
// sum.symbols, 0x530c00-0x531280; file name guessed). Not original source.
#ifndef VSDK_OS_OS_H
#define VSDK_OS_OS_H

#include "vsdk/vsdk.h"
#include <windows.h>

void os_init(const char *company, const char *app);   // 0x530d10 single-instance mutex, CoInitialize, window
void os_poll();                                       // 0x530ca0 message pump wrapper (Dash Faction name)
HWND os_get_main_wnd();                               // 0x530c00 main_wnd (0x2cbff10)
bool os_foreground();                                 // 0x530c10 is_main_wnd_active (0x2cbff1c; decomp49, mouse_set_pos)
void os_close_main_window();                          // 0x531260 ReleaseMutex, PostMessage(WM_DESTROY)
void os_seed_random(int seed);                        // 0x531280 -1 = srand(time(0))
// decomp45 (gr_glide.cpp): the window procedure (0x531000) calls every registered handler with
// (msg, wparam, lparam) before its own switch (cdecl, add esp 0xc).
typedef void (*os_msg_handler)(UINT msg, WPARAM wparam, LPARAM lparam);
void os_add_msg_handler(os_msg_handler handler);      // 0x530c20 (Dash Faction name)

#endif
