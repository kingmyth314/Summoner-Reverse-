// vsdk/os/os.h - the main window, the message pump and the process-level helpers (vsdk\os in
// sum.symbols, 0x530c00-0x531280; file name guessed). Not original source.
#ifndef VSDK_OS_OS_H
#define VSDK_OS_OS_H

#include "vsdk/vsdk.h"
#include <windows.h>

void os_init(const char *company, const char *app);   // 0x530d10 single-instance mutex, CoInitialize, window
void os_poll();                                       // 0x530ca0 message pump wrapper (Dash Faction name)
HWND os_get_main_wnd();                               // 0x530c00 main_wnd (0x2cbff10)
void os_close_main_window();                          // 0x531260 ReleaseMutex, PostMessage(WM_DESTROY)
void os_seed_random(int seed);                        // 0x531280 -1 = srand(time(0))

#endif
