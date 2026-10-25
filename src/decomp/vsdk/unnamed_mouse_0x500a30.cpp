// unnamed_mouse_0x500a30.cpp - Sum.exe v1.40, code 0x500a30-0x501468 (sum.symbols module vsdk\os\mouse)
// FILE NAME IS A PLACEHOLDER: the unit has no assert and no path string (it never calls DebugError), so
// neither the file name nor the directory is known; vsdk\os comes from the sum.symbols tag.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit bounds: it starts with the two static initialisers of the button arrays (0x500a30, table entries
// 0x5887c4 / 0x5887c8) after the dialogue table code (0x5009f0) and ends with clamp_int (0x501450), just
// before frametime.cpp's first initialiser (0x501470).
// .data 0x5a5734-0x5a573b: mouse_use_cursor (1), mouse_double_click_time (400). No literals.
// .bss 0x257ef84-0x257f03f (0x257ef80 game_mouse_z is tagged vsdk\ui; 0x257efe4 has no reference).
//
// Imports: KERNEL32 InitializeCriticalSection 0x57e204, EnterCriticalSection 0x57e1fc,
// LeaveCriticalSection 0x57e1f8, DeleteCriticalSection 0x57e1a8, Sleep 0x57e200, GetModuleHandleA
// 0x57e1a0; USER32 GetCursorPos 0x57e3c8, ScreenToClient 0x57e3a8, ClientToScreen 0x57e3c0, SetCursorPos
// 0x57e330; DINPUT DirectInputCreateA (thunk 0x57b4b0). GUID_SysMouse (0x583310) and c_dfDIMouse
// (0x5832c8) are linked in from dxguid.lib / dinput.lib.
//
// mouse_use_cursor starts at 1 and nothing clears it, so mouse_di_init returns -1 at once: v1.40 never
// creates DirectInput, mouse_di_read is dead, and the position always comes from GetCursorPos (D2903).
// keep_mouse_centered is never set (mouse_keep_centered_enable has no caller), so the re-centring branch
// of mouse_update is dead too.

#define DIRECTINPUT_VERSION 0x0700
#include <windows.h>
#include <dinput.h>
#include <stdlib.h>
#include "vsdk/os/mouse.h"
#include "vsdk/os/os.h"
#include "vsdk/os/timestamp.h"

#ifdef MODERN_BUILD
// The in-source host (modern/display.cpp, L3b): the poll maps window pixels to game pixels (and takes agent mode's
// cursor), the warp maps them back. Both pass through unchanged while SummonerFix is loaded.
#include "modern/modern.h"
#define ScreenToClient modern_screen_to_client
#define ClientToScreen modern_client_to_screen
#endif

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020a              // winuser.h has it only for _WIN32_WINNT >= 0x400
#endif

// ---- .data ----------------------------------------------------------------------------------
int mouse_use_cursor = 1;               // 0x5a5734
int mouse_double_click_time = 400;      // 0x5a5738

// ---- .bss, in address order ----------------------------------------------------------------
int mouse_screen_w;                     // 0x257ef84
int mouse_wheel;                        // 0x257ef88
int mouse_screen_h;                     // 0x257ef8c

// 0x500a30 init_mouse_buttons (thunk `jmp 0x500a40`, initialiser table 0x5887c4): compiler-generated.
// 0x500a40 (body): `vector constructor iterator`(mouse_buttons, 0x10, 3, mouse_button::mouse_button).
// Confidence: exact.
mouse_button mouse_buttons[MOUSE_NUM_BUTTONS];          // 0x257ef90

CRITICAL_SECTION mouse_lock;            // 0x257efc0
int mouse_z;                            // 0x257efd8
int mouse_y;                            // 0x257efdc
int mouse_x;                            // 0x257efe0

// 0x500a60 init_mouse_buttons_live (thunk `jmp 0x500a70`, initialiser table 0x5887c8): compiler-generated.
// 0x500a70 (body): `vector constructor iterator`(mouse_buttons_live, 0x10, 3, mouse_button::mouse_button).
// Confidence: exact.
mouse_button mouse_buttons_live[MOUSE_NUM_BUTTONS];     // 0x257efe8

LPDIRECTINPUT mouse_dinput;             // 0x257f018
LPDIRECTINPUTDEVICE mouse_di_device;    // 0x257f01c
bool mouse_di_active;                   // 0x257f020
bool mouse_inited;                      // 0x257f021
int mouse_dx;                           // 0x257f024
int mouse_dy;                           // 0x257f028
int mouse_dz;                           // 0x257f02c
bool mouse_hidden;                      // 0x257f030
bool keep_mouse_centered;               // 0x257f031
int mouse_prev_x;                       // 0x257f034
int mouse_prev_y;                       // 0x257f038
int mouse_last_z;                       // 0x257f03c

// The window message handler (defined at its address below; registered by mouse_init).
void mouse_msg_handler(UINT msg, WPARAM wparam, LPARAM lparam);

// 0x500a90  Confidence: exact (the latch loop's stores are interleaved by the scheduler; the centre is
// computed before GetCursorPos and kept in ESI / EDI). Called every frame by GameTick.
void mouse_update()
{
    int x, y;
    POINT pt;
    int cx, cy;
    int i;

    mouse_dx = mouse_dy = mouse_dz = 0;
    if (!mouse_inited)
        return;

    EnterCriticalSection(&mouse_lock);
    for (i = 0; i < MOUSE_NUM_BUTTONS; i++) {
        mouse_buttons[i] = mouse_buttons_live[i];
        mouse_buttons_live[i].presses = 0;
        mouse_buttons_live[i].releases = 0;
        mouse_buttons_live[i].double_clicked = false;
    }
    LeaveCriticalSection(&mouse_lock);

    if (!mouse_use_cursor) {
        mouse_di_read();
        return;
    }

    cx = mouse_screen_w / 2;
    cy = mouse_screen_h / 2;
    GetCursorPos(&pt);
    ScreenToClient(os_get_main_wnd(), &pt);
    x = pt.x;
    y = pt.y;
    clamp_int(&x, 0, mouse_screen_w - 1);
    clamp_int(&y, 0, mouse_screen_h - 1);
    if (x != pt.x || y != pt.y)
        mouse_set_pos(x, y);

    EnterCriticalSection(&mouse_lock);
    mouse_x = x;
    mouse_y = y;
    mouse_z = mouse_wheel;
    mouse_dx = mouse_x - mouse_prev_x;
    mouse_dy = mouse_y - mouse_prev_y;
    mouse_dz = mouse_z - mouse_last_z;
    LeaveCriticalSection(&mouse_lock);

    if (keep_mouse_centered && mouse_hidden) {
        if (mouse_dx != 0 || mouse_dy != 0)
            mouse_set_pos(cx, cy);
        mouse_prev_x = cx;
        mouse_prev_y = cy;
    } else {
        mouse_prev_x = x;
        mouse_prev_y = y;
    }
    mouse_last_z = mouse_z;
}

// 0x500c80  Confidence: exact.
void mouse_set_pos(int x, int y)
{
    POINT pt;

    if (os_foreground()) {
        pt.x = x;
        pt.y = y;
        ClientToScreen(os_get_main_wnd(), &pt);
        SetCursorPos(pt.x, pt.y);
    }
}

// 0x500cd0  Confidence: exact (the retry loop's shape: a failed Acquire falls into the FAILED test with
// its own result). Dead in v1.40 (mouse_use_cursor). z is not clamped.
void mouse_di_read()
{
    DIMOUSESTATE state;
    HRESULT hr;

    memset(&state, 0, sizeof(state));
    mouse_dx = mouse_dy = mouse_dz = 0;
    if (!mouse_di_active)
        return;

    for (;;) {
        hr = mouse_di_device->GetDeviceState(sizeof(state), &state);
        if (hr != DIERR_INPUTLOST && hr != DIERR_NOTACQUIRED)
            break;
        Sleep(500);
        hr = mouse_di_device->Acquire();
        if (FAILED(hr))
            break;
    }
    if (SUCCEEDED(hr)) {
        mouse_dx = state.lX;
        mouse_dy = state.lY;
        mouse_dz = state.lZ;
    } else {
        mouse_dx = mouse_dy = mouse_dz = 0;
    }

    mouse_x += mouse_dx;
    mouse_y += mouse_dy;
    mouse_z += mouse_dz;
    if (mouse_x < 0)
        mouse_x = 0;
    if (mouse_y < 0)
        mouse_y = 0;
    if (mouse_x >= mouse_screen_w)
        mouse_x = mouse_screen_w - 1;
    if (mouse_y >= mouse_screen_h)
        mouse_y = mouse_screen_h - 1;
}

// 0x500de0  Confidence: exact. Called by LoadSettings. mouse_update and mouse_flush enter the critical
// section again (recursive entry is allowed).
void mouse_init(int w, int h)
{
    if (mouse_inited)
        return;

    mouse_inited = true;
    mouse_screen_w = w;
    mouse_screen_h = h;
    InitializeCriticalSection(&mouse_lock);
    EnterCriticalSection(&mouse_lock);
    mouse_wheel = 0;
    mouse_x = mouse_screen_w / 2;
    mouse_y = mouse_screen_h / 2;
    mouse_update();
    mouse_flush();
    if (mouse_di_init() == -1)
        mouse_use_cursor = 1;
    os_add_msg_handler(mouse_msg_handler);
    LeaveCriticalSection(&mouse_lock);
    atexit(mouse_close);
}

// 0x500e90  Confidence: exact. Version 0x700 first, then 0x300; the system mouse, non-exclusive and
// foreground (6). The result of Acquire is not checked.
int mouse_di_init()
{
    if (mouse_use_cursor == 1)
        return -1;

    mouse_di_active = false;
    if (FAILED(DirectInputCreateA(GetModuleHandleA(NULL), 0x700, &mouse_dinput, NULL))) {
        if (FAILED(DirectInputCreateA(GetModuleHandleA(NULL), 0x300, &mouse_dinput, NULL)))
            return -1;
    }
    if (FAILED(mouse_dinput->CreateDevice(GUID_SysMouse, &mouse_di_device, NULL)))
        return -1;
    if (FAILED(mouse_di_device->SetDataFormat(&c_dfDIMouse)))
        return -1;
    if (FAILED(mouse_di_device->SetCooperativeLevel(os_get_main_wnd(), DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)))
        return -1;
    mouse_di_device->Acquire();
    mouse_di_active = true;
    return 0;
}

// 0x500f60  Confidence: exact (switch on msg - WM_LBUTTONDOWN through a 10-entry jump table at 0x500fec;
// the double-click messages 0x203 / 0x206 / 0x209 go to the default). The wheel delta is the signed
// high word of wparam. WM_XBUTTON* (0x20b-0x20d) are outside the table.
void mouse_msg_handler(UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
    case WM_LBUTTONDOWN:
        mouse_mark_button(0, true);
        break;
    case WM_LBUTTONUP:
        mouse_mark_button(0, false);
        break;
    case WM_RBUTTONDOWN:
        mouse_mark_button(1, true);
        break;
    case WM_RBUTTONUP:
        mouse_mark_button(1, false);
        break;
    case WM_MBUTTONDOWN:
        mouse_mark_button(2, true);
        break;
    case WM_MBUTTONUP:
        mouse_mark_button(2, false);
        break;
    case WM_MOUSEWHEEL:
        EnterCriticalSection(&mouse_lock);
        mouse_wheel += (short)HIWORD(wparam);
        LeaveCriticalSection(&mouse_lock);
        break;
    }
}

// 0x501020  Confidence: exact. Registered with atexit by mouse_init.
void mouse_close()
{
    if (mouse_inited) {
        mouse_di_close();
        mouse_inited = false;
        DeleteCriticalSection(&mouse_lock);
    }
}

// 0x501050  Confidence: exact.
void mouse_di_close()
{
    if (mouse_di_device != NULL) {
        mouse_di_device->Unacquire();
        mouse_di_device->Release();
        mouse_di_device = NULL;
    }
    if (mouse_dinput != NULL) {
        mouse_dinput->Release();
        mouse_dinput = NULL;
    }
    mouse_di_active = false;
}

// 0x501090  Confidence: exact (the delta stores sit between the push of &mouse_lock and the call). It
// clears the double-click flags and counts of the latched records, and the timers and down states of the
// live ones.
void mouse_flush()
{
    int i;

    if (!mouse_inited)
        return;

    mouse_update();
    mouse_dx = mouse_dy = mouse_dz = 0;
    EnterCriticalSection(&mouse_lock);
    for (i = 0; i < MOUSE_NUM_BUTTONS; i++) {
        mouse_buttons[i].double_clicked = false;
        mouse_buttons[i].releases = 0;
        mouse_buttons[i].presses = 0;
        mouse_buttons_live[i].timer.SetTimeout(0);
        mouse_buttons_live[i].down = false;
    }
    LeaveCriticalSection(&mouse_lock);
}

// 0x501100  Confidence: exact. A press while the previous press's window is still running sets the
// double-click flag; every press restarts the window.
void mouse_mark_button(int button, bool down)
{
    if (!mouse_inited)
        return;
    if (button < 0 || button >= MOUSE_NUM_BUTTONS)
        return;

    EnterCriticalSection(&mouse_lock);
    if (mouse_buttons_live[button].down != down) {
        mouse_buttons_live[button].down = down;
        if (down) {
            mouse_buttons_live[button].presses++;
            if (!mouse_buttons_live[button].timer.elapsed())
                mouse_buttons_live[button].double_clicked = true;
            mouse_buttons_live[button].timer.SetTimeout(mouse_double_click_time);
        } else {
            mouse_buttons_live[button].releases++;
        }
    }
    LeaveCriticalSection(&mouse_lock);
}

// 0x5011a0  Confidence: exact. The latched records only (ui_process, after a click was used).
void mouse_clear_clicks()
{
    int i;

    if (!mouse_inited)
        return;

    EnterCriticalSection(&mouse_lock);
    for (i = 0; i < MOUSE_NUM_BUTTONS; i++) {
        mouse_buttons[i].double_clicked = false;
        mouse_buttons[i].releases = 0;
        mouse_buttons[i].presses = 0;
    }
    LeaveCriticalSection(&mouse_lock);
}

// 0x5011e0  Confidence: exact.
int mouse_get_pos(int *x, int *y, int *z)
{
    if (!mouse_inited) {
        *z = 0;
        *y = 0;
        *x = 0;
        return -1;
    }
    *x = mouse_x;
    *y = mouse_y;
    *z = mouse_z;
    return 0;
}

// 0x501240  Confidence: exact. In cursor mode only the OS cursor moves; mouse_x / mouse_y follow on the
// next mouse_update.
void mouse_move_to(int x, int y)
{
    if (!mouse_use_cursor) {
        mouse_x = x;
        mouse_y = y;
        return;
    }
    if (x != mouse_x || y != mouse_y)
        mouse_set_pos(x, y);
}

// 0x501280  Confidence: exact. No caller.
void mouse_set_pos_xyz(int x, int y, int z)
{
    EnterCriticalSection(&mouse_lock);
    mouse_move_to(x, y);
    mouse_z = z;
    mouse_wheel = z;
    LeaveCriticalSection(&mouse_lock);
}

// 0x5012c0  Confidence: exact. No range check of button (nor in the accessors below).
bool mouse_is_down(int button)
{
    if (!mouse_inited)
        return false;
    return mouse_buttons[button].down;
}

// 0x5012e0  Confidence: exact.
bool mouse_was_double_clicked(int button)
{
    if (!mouse_inited)
        return false;
    return mouse_buttons[button].double_clicked;
}

// 0x501300  Confidence: exact. The latched copy of the timer (copied each frame by mouse_update); no
// mouse_inited test.
int mouse_time_since_press(int button)
{
    return mouse_double_click_time - mouse_buttons[button].timer.GetRemainingTime();
}

// 0x501320  Confidence: exact. No caller.
int mouse_get_press_count(int button)
{
    if (!mouse_inited)
        return 0;
    return mouse_buttons[button].presses;
}

// 0x501340  Confidence: exact. No caller.
int mouse_get_release_count(int button)
{
    if (!mouse_inited)
        return 0;
    return mouse_buttons[button].releases;
}

// 0x501360  Confidence: exact.
bool mouse_was_pressed(int button)
{
    if (!mouse_inited)
        return false;
    return mouse_buttons[button].presses > 0;
}

// 0x501390  Confidence: exact.
bool mouse_was_released(int button)
{
    if (!mouse_inited)
        return false;
    return mouse_buttons[button].releases > 0;
}

// 0x5013c0  Confidence: exact.
void mouse_get_delta(int *dx, int *dy, int *dz)
{
    *dx = mouse_dx;
    *dy = mouse_dy;
    *dz = mouse_dz;
}

// 0x5013f0  Confidence: exact. No caller.
bool mouse_is_hidden()
{
    return mouse_hidden;
}

// 0x501400  Confidence: exact. No caller.
void mouse_set_hidden(bool hidden)
{
    mouse_hidden = hidden;
}

// 0x501410  Confidence: exact. No caller.
void mouse_set_visible(bool visible)
{
    mouse_hidden = !visible;
}

// 0x501420  Confidence: exact. No caller.
void mouse_keep_centered_enable()
{
    keep_mouse_centered = true;
}

// 0x501430  Confidence: exact. No caller.
void mouse_keep_centered_disable()
{
    keep_mouse_centered = false;
}

// 0x501440 mouse_button::mouse_button: compiler-generated (the implicit constructor runs the
// RealtimeTimer member's constructor on this + 0xc and returns this). Confidence: exact.

// 0x501450  Confidence: exact. Not in the sum.symbols mouse module (tag "-"); its place after the implicit
// constructor and before frametime.cpp puts it in this object (D2902). It may be the out-of-line copy of a
// header function; written here as a plain function.
void clamp_int(int *v, int lo, int hi)
{
    if (*v < lo)
        *v = lo;
    else if (*v > hi)
        *v = hi;
}
