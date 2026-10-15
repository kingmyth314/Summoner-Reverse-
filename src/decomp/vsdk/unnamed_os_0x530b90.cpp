// unnamed_os_0x530b90.cpp - Sum.exe v1.40, code 0x530b90-0x531bea (sum.symbols module vsdk\os): the
// single-instance check, the main window and its window procedure, the message handler list, timer_wait,
// the random seed; after them the CPU identification / "sysinfo" code and the clipboard reader, which are
// built without optimisation.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// File name not known (no path string, no strings.tbl block; Summoner 2 has timer_wait in timer.cpp but the
// PC timer.cpp starts at 0x532820), so a placeholder. One object: the .bss 0x2cbfce0-0x2cbff27 holds the os
// globals with ccmd_sysinfo (0x2cbff00) in their middle, and the gap after os_seed_random is nop padding (the
// compiler's), while the /Od code ends with int3 fill (the linker's gap before the next object at 0x531bf0).
// So the original most likely switched optimisation off (`#pragma optimize("", off)`) before the CPU code;
// intrinsics stay on (os_clipboard_get_text's strlen is repne scasb).
//
// Not written here (sum.symbols module -, same object): os_poll 0x530ca0, os_pump_messages 0x530cb0,
// cpu_read_tsc 0x5312c0, cpu_identify 0x5312d6, PrintSystemInfo 0x53171f, init_ccmd_sysinfo_entry 0x531a80,
// init_ccmd_sysinfo 0x531a8a, cmd_sysinfo 0x531aa8 (and ccmd_sysinfo 0x2cbff00).

#include <windows.h>
#include <objbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "vsdk/os/os.h"
#include "vsdk/os/timer.h"

// ---- .bss (0x2cbfce0-0x2cbff27, address order) ------------------------------------------------------
char os_mutex_name[0x80];                // 0x2cbfce0
os_msg_handler os_msg_handlers[32];      // 0x2cbfd60 (no bounds check in os_add_msg_handler)
char os_window_title[0x80];              // 0x2cbfde0 (sum.symbols 0x98: the critical section starts at 0x2cbfe60)
static CRITICAL_SECTION os_critical_section;   // 0x2cbfe60 (name ours) initialised by os_init, no other use
char os_window_class[0x88];              // 0x2cbfe78
// 0x2cbff00 ccmd_sysinfo (console_command, defined with the sysinfo code).
int os_prev_instance_found;              // 0x2cbff0c
HWND main_wnd;                           // 0x2cbff10
static HANDLE os_main_thread;            // 0x2cbff14 (name ours) SetThreadPriority target, never set (NULL)
HANDLE os_instance_mutex;                // 0x2cbff18
bool is_main_wnd_active;                 // 0x2cbff1c
int os_inited;                           // 0x2cbff20 (name ours) set by os_init, never read
int os_num_msg_handlers;                 // 0x2cbff24

static void os_atexit_nop();

// 0x530b90  Confidence: exact.
BOOL CALLBACK os_find_window_callback(HWND hwnd, LPARAM title)
{
    char text[128];

    if (GetWindowTextA(hwnd, text, 127) && strstr(text, (const char *)title)) {
        os_prev_instance_found = 1;
        return FALSE;
    }
    return TRUE;
}

// 0x530bf0  Confidence: exact (sum.symbols module vsdk\os; declared in vsdk/os/timer.h).
void timer_wait(int ms)
{
    Sleep(ms);
}

// 0x530c00  Confidence: exact.
HWND os_get_main_wnd()
{
    return main_wnd;
}

// 0x530c10  Confidence: exact.
bool os_foreground()
{
    return is_main_wnd_active;
}

// 0x530c20  Confidence: exact.
void os_add_msg_handler(os_msg_handler handler)
{
    os_msg_handlers[os_num_msg_handlers++] = handler;
}

// 0x530c40  Confidence: likely (bare ret; no reference, so the signature is unknown).
void os_stub_ret_530c40()
{
}

// 0x530c50  Confidence: likely (bare ret; no reference).
void os_stub_ret_530c50()
{
}

// 0x530c60  Confidence: exact.
void os_set_window_title(const char *title)
{
    strcpy(os_window_title, title);
    SetWindowTextA(main_wnd, os_window_title);
}

// 0x530ca0 os_poll and 0x530cb0 os_pump_messages: module -, not written here.

// 0x530d10  Confidence: exact.
// A copy that finds the mutex calls ReleaseMutex on the handle it opened (it does not own it) and never
// closes it; either way the running copy's window is brought to the front and this one exits.
void os_init(const char *name, const char *title)
{
    strcpy(os_mutex_name, name);
    strcpy(os_window_title, title);
    strcpy(os_window_class, name);

    HANDLE mutex = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, os_mutex_name);
    if (mutex) {
        ReleaseMutex(mutex);
        HWND wnd = FindWindowA(os_window_class, os_window_title);
        if (wnd)
            SetForegroundWindow(wnd);
        exit(1);
    }
    os_instance_mutex = CreateMutexA(NULL, TRUE, os_mutex_name);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND wnd = FindWindowA(os_window_class, os_window_title);
        if (wnd)
            SetForegroundWindow(wnd);
        exit(1);
    }
    CoInitialize(NULL);
    InitializeCriticalSection(&os_critical_section);
    os_init_window(0);
    os_inited = 1;
    os_find_previous_instance();
    atexit(os_atexit_nop);
}

// 0x530e40  Confidence: likely (the backward scan: VC6 folded the first '\\' test against the NUL it had just
// stored, so the loop tests '/' and ':' first).
// Looks for a window titled "[run] - <exe name> -"; the result (os_prev_instance_found) is never read.
void os_find_previous_instance()
{
    char path[128];
    char title[256];

    os_prev_instance_found = 0;
    HMODULE module = GetModuleHandleA(NULL);
    if (module == NULL)
        return;
    if ((int)GetModuleFileNameA(module, path, 127) < 1)
        return;
    char *p = strstr(path, ".exe");
    if (p == NULL)
        return;
    *p = '\0';
    while (*p != '\\' && *p != '/' && *p != ':')
        p--;
    p++;
    if (strlen(p) < 1)
        return;
    sprintf(title, "[run] - %s -", p);
    EnumWindows(os_find_window_callback, (LPARAM)title);
}

// 0x530ee0  Confidence: exact (os_init pushes 0; the argument is not read).
int os_init_window(int unused)
{
    os_create_window();
    return 0;
}

// 0x530ef0  Confidence: likely (the WNDCLASSEX field stores are scheduled; the source order is a guess).
// A 32x32 WS_POPUP window in the screen centre; the renderers resize it.
int os_create_window()
{
    WNDCLASSEXA wc;
    HINSTANCE instance = GetModuleHandleA(NULL);

    wc.cbSize = sizeof(wc);
    wc.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC | CS_BYTEALIGNCLIENT;
    wc.lpfnWndProc = os_wndproc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance;
    wc.hIcon = LoadIconA(instance, "APP_ICON");
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = os_window_class;
    wc.hIconSm = NULL;
    if (!RegisterClassExA(&wc))
        return 0;

    main_wnd = CreateWindowExA(0, os_window_class, os_window_title, WS_POPUP,
                               (GetSystemMetrics(SM_CXSCREEN) - 32) / 2,
                               (GetSystemMetrics(SM_CYSCREEN) - 32) / 2,
                               32, 32, NULL, NULL, instance, NULL);
    ShowCursor(FALSE);
    ClipCursor(NULL);
    SetActiveWindow(main_wnd);
    SetForegroundWindow(main_wnd);
    return 1;
}

// 0x530ff0  Confidence: likely (switch case order; the 256-byte buffer is the sprintf target).
// Activation makes the window topmost with SetWindowPos flags 0 (so it also activates and resizes it to the
// screen) and raises the thread priority of os_main_thread, which nothing sets. SC_SCREENSAVE is swallowed.
LRESULT CALLBACK os_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    char buf[256];
    int i;

    for (i = 0; i < os_num_msg_handlers; i++)
        os_msg_handlers[i](msg, wparam, lparam);

    switch (msg) {
    case WM_ACTIVATE:
    case WM_ACTIVATEAPP: {
        bool was_active = is_main_wnd_active;
        if (msg == WM_ACTIVATE)
            is_main_wnd_active = LOWORD(wparam) == WA_ACTIVE || LOWORD(wparam) == WA_CLICKACTIVE;
        else
            is_main_wnd_active = wparam != 0;
        if (was_active != is_main_wnd_active) {
            if (is_main_wnd_active) {
                SetWindowPos(main_wnd, HWND_TOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN),
                             GetSystemMetrics(SM_CYSCREEN), 0);
                SetThreadPriority(os_main_thread, THREAD_PRIORITY_HIGHEST);
            } else {
                SetWindowPos(main_wnd, HWND_NOTOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN),
                             GetSystemMetrics(SM_CYSCREEN), 0);
                SetThreadPriority(os_main_thread, THREAD_PRIORITY_NORMAL);
            }
        }
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_SYSCOMMAND:
        sprintf(buf, "Sys command called '%d'\n", wparam);
        if (wparam == SC_SCREENSAVE)
            return 0;
        return DefWindowProcA(hwnd, WM_SYSCOMMAND, wparam, lparam);
    case WM_TIMER:
    case WM_PALETTEISCHANGING:
    case WM_PALETTECHANGED:
    case WM_DISPLAYCHANGE:
        return 0;
    case WM_QUERYNEWPALETTE:
        return 1;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

// 0x531250  Confidence: likely (bare ret; os_init registers it with atexit; no sum.symbols entry, name ours).
static void os_atexit_nop()
{
}

// 0x531260  Confidence: exact.
void os_close_main_window()
{
    ReleaseMutex(os_instance_mutex);
    PostMessageA(main_wnd, WM_DESTROY, 0, 0);
}

// 0x531280  Confidence: exact (the last rand is a tail jump).
void os_seed_random(int seed)
{
    if (seed == -1)
        srand(time(NULL));
    else
        srand(seed);
    rand();
    rand();
    rand();
}

// 0x5312c0-0x531b43: cpu_read_tsc, cpu_identify, PrintSystemInfo, the sysinfo console command (module -,
// not written here). The original's optimisation switch would stand before them:
#pragma optimize("", off)

// 0x531b44  Confidence: likely (/Od code: frame pointer, locals h -4, text -8, i -0xc, len -0x10).
// Both failure returns skip CloseClipboard, leaving the clipboard open; buf needs max_len + 1 bytes.
void os_clipboard_get_text(char *buf, int max_len)
{
    HANDLE h;
    char *text;
    int i;
    int len;

    *buf = '\0';
    OpenClipboard(NULL);
    h = GetClipboardData(CF_TEXT);
    if (h == NULL)
        return;
    text = (char *)GlobalLock(h);
    if (text == NULL)
        return;
    len = strlen(text);
    if (len > max_len)
        len = max_len;
    for (i = 0; i < len; i++)
        buf[i] = text[i];
    buf[i] = '\0';
    GlobalUnlock(h);
    CloseClipboard();
}

#pragma optimize("", on)
