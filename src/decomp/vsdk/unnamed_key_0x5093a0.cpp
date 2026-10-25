// unnamed_key_0x5093a0.cpp - Sum.exe v1.40, code 0x5093a0-0x509f57 (and the jump tables to 0x509ffb).
// Written from the Sum.exe disassembly (tools/asm.mjs; the tables read from the image). Target: VC++ 6.0
// (cl 12.00.8966), /O2 /Ob1. Confidence per function: exact (control flow, calls, constants and globals match),
// likely (one detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// The keyboard: Windows key messages (os_add_msg_handler) into scan-code states, press / release counters,
// held times and a 16-entry key queue with the modifier bits; layout swaps; the Windows 9x NumLock fix.
// FILE NAME NOT KNOWN (D5313): no path string, no assert, no strings.tbl block, no reference name; a
// placeholder (the header stays vsdk/os/key.h, an earlier placeholder). The unit follows gr_font.cpp
// (gr_split_str 0x509130) and precedes console.cpp's first initialiser (0x50a000).
// .data 0x5a64bc-0x5a6cbf (key_repeat_enabled, the two ASCII tables, key_names) and the key names'
// literals 0x5a6cc0-0x5a6f1b; .bss 0x25d94e0-0x25dace7. The names' empty strings are separate 4-byte
// .bss slots (0x25daa7c-0x25dace4), one per "" entry: zero-filled literals that were not pooled (D5313).
//
// Imports: KERNEL32 InitializeCriticalSection, Enter/Leave/DeleteCriticalSection, GetVersionExA; USER32
// GetKeyboardState, SetKeyboardState, GetMessageTime; WINMM timeGetTime.
//
// Header: vsdk/os/key.h.

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "vsdk/os/key.h"
#include "vsdk/os/os.h"
#include "vsdk/os/timer.h"

// ---- .data (0x5a64bc-0x5a6cbf) -------------------------------------------------------------------------
bool key_repeat_enabled = true;                      // 0x5a64bc held keys repeat in the queue (key_init sets it; name ours)
// key_to_ascii: scan code -> character, 0xff none (128 entries each; key_to_ascii reads the first 127).
int key_ascii_table[128] = {                         // 0x5a64c0
    0xff, 0xff, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '=', 0xff, 0xff, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']', 0xff, 0xff, 'a', 's', 'd', 'f', 'g', 'h',
    'j', 'k', 'l', ';', '\'', '`', 0xff, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0xff, '*', 0xff, ' ', 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};
int key_ascii_table_shifted[128] = {                 // 0x5a66c0
    0xff, 0xff, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
    '_', '+', 0xff, 0xff, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{', '}', 0xff, 0xff, 'A', 'S', 'D', 'F', 'G', 'H',
    'J', 'K', 'L', ':', '"', '~', 0xff, '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0xff, 0xff, 0xff, ' ', 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};
const char *key_names[256] = {                             // 0x5a68c0
    "", "Esc", "1", "2", "3", "4", "5", "6",
    "7", "8", "9", "0", "-", "=", "Backspace", "Tab",
    "Q", "W", "E", "R", "T", "Y", "U", "I",
    "O", "P", "[", "]", "Enter", "Left Ctrl", "A", "S",
    "D", "F", "G", "H", "J", "K", "L", ";",
    "'", "`", "Shift", "\\", "Z", "X", "C", "V",
    "B", "N", "M", ",", ".", "/", "Shift", "Pad *",
    "Alt", "Spacebar", "Caps Lock", "F1", "F2", "F3", "F4", "F5",
    "F6", "F7", "F8", "F9", "F10", "Pause", "Scroll Lock", "Pad 7",
    "Pad 8", "Pad 9", "Pad -", "Pad 4", "Pad 5", "Pad 6", "Pad +", "Pad 1",
    "Pad 2", "Pad 3", "Pad 0", "Pad .", "", "", "", "F11",
    "F12", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "Pad Enter", "Right Ctrl", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "Pad /", "", "Print Scrn",
    "Alt", "", "", "", "", "", "", "",
    "", "", "", "", "", "Num Lock", "", "Home",
    "Up Arrow", "Page Up", "", "Left Arrow", "", "Right Arrow", "", "End",
    "Down Arrow", "Page Down", "Insert", "Delete", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", ""
};

// ---- .bss (0x25d94e0-0x25daa77) ------------------------------------------------------------------------
// The queue, the per-key time and count tables and the queue indices are one object (D14001): key_flush
// clears the five 256-entry tables through one pointer with fixed offsets, key_mark loads a press count
// only after the timestamp store and re-reads the tail after the queue store, as 8966 does for fields of
// one object. Struct, type and field names ours; the old names are macros.
struct key_tables {
    ushort queue[16];                  // 0x25d94e0
    unsigned int queue_times[16];      // 0x25d9500
    unsigned int timestamps[256];      // 0x25d9540 when the key went down / was last measured
    unsigned int total[256];           // 0x25d9940 down time collected on release
    unsigned int checked[256];         // 0x25d9d40 time of the last key_down_time call
    int press_counts[256];             // 0x25da140
    int release_counts[256];           // 0x25da540
    int queue_head;                    // 0x25da940
    int queue_tail;                    // 0x25da944
};
LAYOUT_CHECK(key_tables, sizeof(key_tables) == 0x1468);
key_tables key_tab;                                  // 0x25d94e0
#define key_queue key_tab.queue
#define key_queue_times key_tab.queue_times
#define key_down_timestamps key_tab.timestamps
#define key_down_time_total key_tab.total
#define key_down_time_checked key_tab.checked
#define key_press_counts key_tab.press_counts
#define key_release_counts key_tab.release_counts
#define key_queue_head key_tab.queue_head
#define key_queue_tail key_tab.queue_tail
int key_last_released;                               // 0x25da948 (written only; name ours)
bool key_down_state[256];                           // 0x25da94c
int key_last_pressed;                                // 0x25daa4c (written only; name ours)
int key_layout;                                      // 0x25daa50
CRITICAL_SECTION key_lock;                           // 0x25daa58
unsigned int key_last_event_time;                    // 0x25daa70
bool key_inited;                                     // 0x25daa74
bool key_numlock_restore;                            // 0x25daa75
bool key_numlock_fix_enabled;                        // 0x25daa76
bool key_debug_modifier;                             // 0x25daa77

// 0x5093a0  Confidence: exact (D14001: key_tab as one object). The repeat test as an else-if that leaves
// early; the compiler merged its unlock with the common one. The layout swap first; scan code 0xc5 (Pause) turns NumLock off again when the
// fix is on. A release counts and adds the held time (0 when the clock went backwards); a press counts once,
// a held key (auto-repeat) only queues again when repeats are on and it is not Esc. 0xaa (the fake shift of
// extended keys) is never queued. The queued word carries the modifier bits (0x800 for keys pressed with `
// while the debug modifier is on); a full queue drops the key.
void key_mark(int code, int state, int latency)
{
    unsigned int scancode;
    unsigned int event_time;

    code = key_remap_layout(code);
    if (code == 0xc5 && key_numlock_fix_enabled == true)
        key_numlock_off();
    if (!key_inited)
        return;
    EnterCriticalSection(&key_lock);
    event_time = timer_get(1000) - latency;
    scancode = code & 0xff;
    if (!state) {
        key_last_released = scancode;
        key_down_state[scancode] = 0;
        key_release_counts[scancode]++;
        if (event_time < key_down_timestamps[scancode])
            key_down_time_total[scancode] = 0;
        else
            key_down_time_total[scancode] += event_time - key_down_timestamps[scancode];
    } else {
        key_last_pressed = scancode;
        key_last_event_time = event_time;
        if (!key_down_state[scancode]) {
            key_down_timestamps[scancode] = event_time;
            key_press_counts[scancode]++;
            key_down_state[scancode] = 1;
        } else if (!key_repeat_enabled || scancode == KEY_ESC) {
            LeaveCriticalSection(&key_lock);
            return;
        }
        if (scancode != 0xaa) {
            ushort keycode = (ushort)scancode;
            if (key_down_state[KEY_LSHIFT] || key_down_state[KEY_RSHIFT])
                keycode |= KEY_SHIFTED;
            if (key_down_state[KEY_LALT] || key_down_state[KEY_RALT])
                keycode |= KEY_ALTED;
            if (key_down_state[KEY_LCONTROL] || key_down_state[KEY_RCONTROL])
                keycode |= KEY_CTRLED;
            if (key_down_state[KEY_LAPOSTRO] && key_debug_modifier)
                keycode |= 0x800;
            if (keycode) {
                int next = key_queue_tail + 1;
                if ((unsigned int)next >= 16)
                    next = 0;
                if (next != key_queue_head) {
                    key_queue[key_queue_tail] = keycode;
                    key_queue_times[key_queue_tail] = event_time;
                    key_queue_tail = next;
                }
            }
        }
    }
    LeaveCriticalSection(&key_lock);
}

// 0x509560  Confidence: exact (D11400: the four layouts as an if chain in the code's order 1, 4, 2, 3, each
// assigning code; layouts 2 and 3 are switches, layout 2's cases in the binary's block order). 1: the UK backslash 0x56 -> 0x2b; 2: AZERTY (A/Q, Z/W, M/;/,
// rotated); 3: QWERTZ (Y/Z); 4: scan code 0 -> A (see NOTES).
int key_remap_layout(int code)
{
    if (key_layout == 1) {
        if (code == 0x56)
            code = 0x2b;
    } else if (key_layout == 4) {
        if (code == 0)
            code = 0x1e;
    } else if (key_layout == 2) {
        switch (code) {
        case 0x1e: code = 0x10; break;
        case 0x32: code = 0x33; break;
        case 0x2c: code = 0x11; break;
        case 0x27: code = 0x32; break;
        case 0x33: code = 0x27; break;
        case 0x10: code = 0x1e; break;
        case 0x11: code = 0x2c; break;
        }
    } else if (key_layout == 3) {
        switch (code) {
        case 0x15: code = 0x2c; break;
        case 0x2c: code = 0x15; break;
        }
    }
    return code;
}

// 0x509630  Confidence: exact.
void key_numlock_off()
{
    BYTE state[256];

    GetKeyboardState(state);
    state[VK_NUMLOCK] = 0;
    SetKeyboardState(state);
}

// 0x509660  Confidence: exact (the three key_mark calls are the compiler's tail copies of one). The scan
// code from lParam bits 16-23 (+0x80 for extended keys); Print Screen (VK_SNAPSHOT) is KEY_SYSRQ, the right
// shift and alt keys count as the left ones.
void key_on_keydown(int vk, int lparam, int latency)
{
    int scancode;

    if (!key_inited)
        return;
    scancode = (lparam >> 16) & 0xff;
    if (lparam & 0x1000000)
        scancode += 0x80;
    if (vk == VK_SNAPSHOT)
        scancode = KEY_SYSRQ;
    else if (scancode == KEY_RSHIFT)
        scancode = KEY_LSHIFT;
    else if (scancode == KEY_RALT)
        scancode = KEY_LALT;
    key_mark(scancode, 1, latency);
}

// 0x5096e0  Confidence: exact (as key_on_keydown). Windows sends Print Screen only on the way up, so it is
// marked down and up here.
void key_on_keyup(int vk, int lparam, int latency)
{
    int scancode;

    if (!key_inited)
        return;
    scancode = (lparam >> 16) & 0xff;
    if (lparam & 0x1000000)
        scancode += 0x80;
    if (vk == VK_SNAPSHOT)
        scancode = KEY_SYSRQ;
    else if (scancode == KEY_RSHIFT)
        scancode = KEY_LSHIFT;
    else if (scancode == KEY_RALT)
        scancode = KEY_LALT;
    if (scancode == KEY_SYSRQ) {
        key_mark(KEY_SYSRQ, 1, latency);
        key_mark(KEY_SYSRQ, 0, latency);
    } else {
        key_mark(scancode, 0, latency);
    }
}

// 0x509770  Confidence: exact. key_flush runs before key_inited is set, so it returns at once (see NOTES).
void key_init(int layout, bool fix_numlock)
{
    if (key_inited)
        return;
    key_layout = layout;
    key_repeat_enabled = true;
    InitializeCriticalSection(&key_lock);
    EnterCriticalSection(&key_lock);
    key_last_event_time = timer_get(1000);
    key_flush();
    LeaveCriticalSection(&key_lock);
    key_numlock_fix_enabled = fix_numlock;
    if (fix_numlock == true && key_numlock_fix() == -1)
        key_numlock_fix_enabled = false;
    os_add_msg_handler(key_msg_handler);
    atexit(key_close);
    key_inited = true;
}

// 0x509810  Confidence: exact.
void key_close()
{
    if (!key_inited)
        return;
    if (key_numlock_restore) {
        key_numlock_on();
        key_numlock_restore = false;
    }
    key_inited = false;
    DeleteCriticalSection(&key_lock);
}

// 0x509850  Confidence: exact.
void key_numlock_on()
{
    BYTE state[256];

    GetKeyboardState(state);
    state[VK_NUMLOCK] = 1;
    SetKeyboardState(state);
}

// 0x509880  Confidence: exact (GetMessageTime is called before timeGetTime; the order the two calls of the
// subtraction are written in is a guess). The latency of the message (never negative); focus changes flush
// the key states (two identical cases).
void key_msg_handler(UINT msg, WPARAM wparam, LPARAM lparam)
{
    int msg_time = GetMessageTime();
    int latency = timeGetTime() - msg_time;

    if (latency < 0)
        latency = 0;
    switch (msg) {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        key_on_keydown(wparam, lparam, latency);
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        key_on_keyup(wparam, lparam, latency);
        break;
    case WM_KILLFOCUS:
        key_flush();
        break;
    case WM_SETFOCUS:
        key_flush();
        break;
    }
}

// 0x509a00  Confidence: exact. Windows 9x only: NumLock off (restored by key_close). -1 on NT.
int key_numlock_fix()
{
    OSVERSIONINFOA info;

    info.dwOSVersionInfoSize = sizeof(info);
    GetVersionExA(&info);
    if (info.dwPlatformId == VER_PLATFORM_WIN32_NT)
        return -1;
    if (key_numlock_is_on()) {
        key_numlock_restore = true;
        key_numlock_off();
    }
    return 0;
}

// 0x509a50  Confidence: exact.
bool key_numlock_is_on()
{
    BYTE state[256];

    GetKeyboardState(state);
    return state[VK_NUMLOCK] ? true : false;
}

// 0x509a80  Confidence: exact (a tail jump to key_inkey). Spins until a key is queued.
int key_getch()
{
    if (!key_inited)
        return 0;
    while (!key_queue_not_empty())
        ;
    return key_inkey();
}

// 0x509ab0  Confidence: exact (D14001: with key_tab one object the table loops stay loops; the key_down_state
// clear inside the table loop, which 8966 moves out as rep stosd before it). Empties the queue and every key's state and counters.
void key_flush()
{
    unsigned int now;
    int i;

    if (!key_inited)
        return;
    EnterCriticalSection(&key_lock);
    key_queue_tail = 0;
    key_queue_head = 0;
    for (i = 0; i < 16; i++) {
        key_queue[i] = 0;
        key_queue_times[i] = 0;
    }
    now = timer_get(1000);
    for (i = 0; i < 256; i++) {
        key_down_state[i] = 0;
        key_down_time_checked[i] = now;
        key_down_timestamps[i] = now;
        key_down_time_total[i] = 0;
        key_press_counts[i] = 0;
        key_release_counts[i] = 0;
    }
    LeaveCriticalSection(&key_lock);
}

// 0x509b50  Confidence: exact (D14001: key_tab as one object). One key's state (no bounds check).
void key_clear(int key)
{
    unsigned int now;

    if (!key_inited)
        return;
    EnterCriticalSection(&key_lock);
    now = timer_get(1000);
    key_down_time_checked[key] = now;
    key_down_timestamps[key] = now;
    key_down_time_total[key] = 0;
    key_press_counts[key] = 0;
    key_down_state[key] = 0;
    key_release_counts[key] = 0;
    LeaveCriticalSection(&key_lock);
}

// 0x509bb0  Confidence: exact (the unsigned to float conversions: the code loads the held time as a 64-bit
// integer and divides by the low dword of the elapsed time). The fraction of the time since the last call
// the key was down: within 1 ms of the last call 1.0 while down, else 0.0; a released key returns its
// collected time (cleared), a held one the time since its timestamp (moved to now).
float key_down_time(int key)
{
    unsigned int now;
    unsigned int elapsed;
    unsigned int held;

    if (!key_inited || (unsigned int)key >= 0x100)
        return 0.0f;
    EnterCriticalSection(&key_lock);
    now = timer_get(1000);
    elapsed = now - key_down_time_checked[key];
    key_down_time_checked[key] = now;
    if (elapsed <= 1) {
        key_down_timestamps[key] = now;
        if (key_down_state[key]) {
            LeaveCriticalSection(&key_lock);
            return 1.0f;
        }
        LeaveCriticalSection(&key_lock);
        return 0.0f;
    }
    if (!key_down_state[key]) {
        held = key_down_time_total[key];
        key_down_time_total[key] = 0;
    } else {
        held = now - key_down_timestamps[key];
        key_down_timestamps[key] = now;
    }
    LeaveCriticalSection(&key_lock);
    return (float)held / (float)elapsed;
}

// 0x509ca0  Confidence: exact.
int key_to_ascii(int key)
{
    int shifted;

    if (!key_inited)
        return 0xff;
    shifted = key & KEY_SHIFTED;
    key &= 0xff;
    if (key >= 0x7f)
        return 0xff;
    if (shifted)
        return key_ascii_table_shifted[key];
    return key_ascii_table[key];
}

// 0x509ce0  Confidence: exact.
int key_inkey()
{
    int key = 0;

    if (!key_inited)
        return 0;
    EnterCriticalSection(&key_lock);
    if (key_queue_tail != key_queue_head) {
        key = key_queue[key_queue_head];
        key_queue_head++;
        if ((unsigned int)key_queue_head >= 16)
            key_queue_head = 0;
    }
    LeaveCriticalSection(&key_lock);
    return key;
}

// 0x509d40  Confidence: exact. A full queue is not checked: the tail then meets the head and the queued keys
// are lost (see NOTES).
void key_putkey(int code)
{
    int next;

    if (!key_inited)
        return;
    EnterCriticalSection(&key_lock);
    next = key_queue_tail + 1;
    if (next >= 16)
        next = 0;
    key_queue[key_queue_tail] = (ushort)code;
    key_queue_tail = next;
    LeaveCriticalSection(&key_lock);
}

// 0x509d90  Confidence: exact.
int key_get_shift_status()
{
    int status = 0;

    if (!key_inited)
        return 0;
    EnterCriticalSection(&key_lock);
    if (key_down_state[KEY_LSHIFT] || key_down_state[KEY_RSHIFT])
        status = KEY_SHIFTED;
    if (key_down_state[KEY_LALT] || key_down_state[KEY_RALT])
        status |= KEY_ALTED;
    if (key_down_state[KEY_LCONTROL] || key_down_state[KEY_RCONTROL])
        status |= KEY_CTRLED;
    if (key_down_state[KEY_LAPOSTRO] && key_debug_modifier)
        status |= 0x800;
    LeaveCriticalSection(&key_lock);
    return status;
}

// 0x509e20  Confidence: exact.
int key_get_press_count(int key)
{
    int count;

    if (!key_inited || key < 0 || key >= 0x100)
        return 0;
    EnterCriticalSection(&key_lock);
    count = key_press_counts[key];
    key_press_counts[key] = 0;
    LeaveCriticalSection(&key_lock);
    return count;
}

// 0x509e70  Confidence: exact.
int key_get_release_count(int key)
{
    int count;

    if (!key_inited || key < 0 || key >= 0x100)
        return 0;
    EnterCriticalSection(&key_lock);
    count = key_release_counts[key];
    key_release_counts[key] = 0;
    LeaveCriticalSection(&key_lock);
    return count;
}

// 0x509ec0  Confidence: exact.
int key_queue_not_empty()
{
    int result = 0;

    if (!key_inited)
        return 0;
    EnterCriticalSection(&key_lock);
    if (key_queue_tail != key_queue_head)
        result = 1;
    LeaveCriticalSection(&key_lock);
    return result;
}

// 0x509f00  Confidence: exact. No bounds check.
bool key_is_down(int key)
{
    return key_down_state[key];
}

// 0x509f10  Confidence: exact.
void key_enable_debug_modifier()
{
    key_debug_modifier = true;
}

// 0x509f20  Confidence: exact.
void key_disable_debug_modifier()
{
    key_debug_modifier = false;
}

// 0x509f30  Confidence: exact (a jump table over 0x1d..0xb8).
bool key_is_modifier(int code)
{
    switch (code & 0xff) {
    case KEY_LCONTROL:
    case KEY_LAPOSTRO:
    case KEY_LSHIFT:
    case KEY_RSHIFT:
    case KEY_LALT:
    case KEY_RCONTROL:
    case KEY_RALT:
        return true;
    }
    return false;
}
