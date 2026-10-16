// vsdk/os/os.h - the main window, the message pump and the process-level helpers (vsdk\os in
// sum.symbols; one object 0x530b90-0x531bea with .bss 0x2cbfce0-0x2cbff27, file name not known; source
// src/decomp/vsdk/unnamed_os_0x530b90.cpp, decomp50). Not original source.
#ifndef VSDK_OS_OS_H
#define VSDK_OS_OS_H

#include "vsdk/vsdk.h"
#include <windows.h>

// name: the mutex and window class name ("Volition"), title: the window title ("Summoner").
void os_init(const char *name, const char *title);   // 0x530d10 single-instance mutex, CoInitialize, window
void os_poll();                                       // 0x530ca0 message pump wrapper (Dash Faction name)
// decomp76: PeekMessage / TranslateMessage / DispatchMessage until the queue is empty; the argument is not
// read (os_poll passes 0); returns 0.
int  os_pump_messages(int unused);                    // 0x530cb0
HWND os_get_main_wnd();                               // 0x530c00 main_wnd (0x2cbff10)
bool os_foreground();                                 // 0x530c10 is_main_wnd_active (0x2cbff1c; decomp49, mouse_set_pos)
void os_close_main_window();                          // 0x531260 ReleaseMutex, PostMessage(WM_DESTROY)
void os_seed_random(int seed);                        // 0x531280 -1 = srand(time(0))
// decomp45 (gr_glide.cpp): the window procedure (0x531000) calls every registered handler with
// (msg, wparam, lparam) before its own switch (cdecl, add esp 0xc).
typedef void (*os_msg_handler)(UINT msg, WPARAM wparam, LPARAM lparam);
void os_add_msg_handler(os_msg_handler handler);      // 0x530c20 (Dash Faction name)

// decomp50: the rest of the unit (timer_wait 0x530bf0 is declared in vsdk/os/timer.h).
BOOL CALLBACK os_find_window_callback(HWND hwnd, LPARAM title);   // 0x530b90 EnumWindows callback
void os_stub_ret_530c40();                            // 0x530c40 bare ret, no reference
void os_stub_ret_530c50();                            // 0x530c50 bare ret, no reference
void os_set_window_title(const char *title);          // 0x530c60
void os_find_previous_instance();                     // 0x530e40
int  os_init_window(int unused);                      // 0x530ee0 os_init pushes 0; returns 0
int  os_create_window();                              // 0x530ef0 0 when RegisterClassEx fails, else 1
LRESULT CALLBACK os_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);   // 0x530ff0
void os_clipboard_get_text(char *buf, int max_len);   // 0x531b44 (no caller; built unoptimised)
int  os_stub_ret_m1_531bf0();                         // 0x531bf0 returns -1, no reference
int  os_stub_ret_m1_531c00();                         // 0x531c00 returns -1, no reference

extern HWND main_wnd;                                 // 0x2cbff10 (Dash Faction name)
extern bool is_main_wnd_active;                       // 0x2cbff1c (Dash Faction name)

#endif
