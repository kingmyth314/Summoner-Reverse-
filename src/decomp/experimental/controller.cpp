// [Controller] tier 1 in the source (docs/experimental-build.md): an XInput pad drives the game's own mouse and
// keyboard input. The specification is SummonerFix's controller.c / padmap.h (src/summonerfix/NOTES-controller.md):
// the same [Controller] keys and defaults in summonerfix.ini, off by default. The left stick moves the cursor, the
// right stick holds the camera keys (arrows), the buttons press game keys and mouse buttons.
//
// Where SummonerFix needs hooks (SwapBuffers for the poll, ScreenToClient for the cursor, key_mark's remap call to
// skip [KeyBindings]), the source uses the game's own input path: exp_controller_frame runs in GameTick right after
// mouse_update / mouse_get_pos and before the state's frame handler, on the game thread, every frame, with any
// renderer.
//   - cursor: the new position goes into mouse_x / mouse_y and game_mouse_x / game_mouse_y (this frame sees it) and
//     through mouse_set_pos to the Windows cursor, so the next mouse_update reads it back and the real mouse carries
//     on from there. mouse_set_pos is the game's warp (SummonerFix maps it to the scaled window, and moves agent
//     mode's cursor along).
//   - keys: key_mark(code, down, 0), as the window messages do, with key_layout held at 0 during the call so the
//     French / German scan code swap does not change the game key the pad sends.
//   - mouse buttons: mouse_mark_button (the live record; mouse_update latches it at the start of the next frame,
//     as for a real click).
// Idle (left stick inside the dead zone, nothing pressed) touches neither the cursor nor the keys: inputs are
// pressed and released only on a change. XInput is loaded at run time (no header, no import library).
//
// Testing without a controller: the exe exports ExpControllerSetSynthetic / ExpControllerSyntheticDone (the same
// arguments as SummonerFix's ControllerSetSynthetic / ControllerSyntheticDone), for agent mode's "pad" command.
#include <windows.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "experimental/experimental.h"

#ifdef EXPERIMENTAL_BUILD

// EXPERIMENTAL_BUILD: controller Controller

#include "vsdk/os/mouse.h"
#include "vsdk/os/key.h"
#include "vsdk/os/os.h"
#include "summoner/summoner.h"

extern int key_layout;                  // 0x25daa50 (vsdk/unnamed_key_0x5093a0.cpp; 1-4 = key_remap_layout's swaps)

// ---- the mapping: mirrors src/summonerfix/padmap.h and keynames.h ---------------------------------------------
// Bits of one "down" word: the low 16 are XINPUT_GAMEPAD wButtons bits, then the triggers and the right stick.
#define PAD_DPAD_UP    0x0001u
#define PAD_DPAD_DOWN  0x0002u
#define PAD_DPAD_LEFT  0x0004u
#define PAD_DPAD_RIGHT 0x0008u
#define PAD_START      0x0010u
#define PAD_BACK       0x0020u
#define PAD_LTHUMB     0x0040u
#define PAD_RTHUMB     0x0080u
#define PAD_LSHOULDER  0x0100u
#define PAD_RSHOULDER  0x0200u
#define PAD_A          0x1000u
#define PAD_B          0x2000u
#define PAD_X          0x4000u
#define PAD_Y          0x8000u
#define PAD_LTRIGGER   0x10000u
#define PAD_RTRIGGER   0x20000u
#define PAD_CAM_LEFT   0x40000u         // right stick pushed left
#define PAD_CAM_RIGHT  0x80000u
#define PAD_CAM_UP     0x100000u
#define PAD_CAM_DOWN   0x200000u
#define PAD_BUTTON_BITS 0xF3FFu         // the 14 XInput buttons

#define PAD_DEF_DEADZONE          7849  // left stick, radial (XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
#define PAD_DEF_CURSOR_SPEED      800   // game pixels per second at full deflection
#define PAD_DEF_CAMERA_THRESHOLD  16000 // right stick deflection that holds a camera key
#define PAD_DEF_TRIGGER_THRESHOLD 30    // 0-255 (XINPUT_GAMEPAD_TRIGGER_THRESHOLD)

#define KEY_MOUSE_BASE 0x100            // MouseLeft / MouseRight / MouseMiddle = 0x100 + 0..2 (Mouse4/5 refused)

struct PadInput {
    const char *iniKey;                 // [Controller] key
    unsigned mask;
    const char *def;                    // default target
};

static const PadInput pad_inputs[] = {
    { "A", PAD_A, "MouseLeft" },                    // move / attack / talk
    { "B", PAD_B, "MouseRight" },                   // chain attack
    { "X", PAD_X, "X" },                            // interact with the nearest object
    { "Y", PAD_Y, "I" },                            // inventory
    { "LB", PAD_LSHOULDER, "Tab" },                 // next party member (with LT: previous)
    { "RB", PAD_RSHOULDER, "H" },                   // solo mode on/off
    { "LT", PAD_LTRIGGER, "LShift" },               // Shift (with B: chain slot 1)
    { "RT", PAD_RTRIGGER, "Space" },                // pause
    { "Start", PAD_START, "Esc" },                  // play menu
    { "Back", PAD_BACK, "M" },                      // automap
    { "DPadUp", PAD_DPAD_UP, "1" },                 // quick slots 1-4
    { "DPadRight", PAD_DPAD_RIGHT, "2" },
    { "DPadDown", PAD_DPAD_DOWN, "3" },
    { "DPadLeft", PAD_DPAD_LEFT, "4" },
    { "LS", PAD_LTHUMB, "C" },                      // spells panel
    { "RS", PAD_RTHUMB, "F12" },                    // camera mode
    { "CameraLeft", PAD_CAM_LEFT, "Left" },         // right stick: turn the camera
    { "CameraRight", PAD_CAM_RIGHT, "Right" },
    { "CameraUp", PAD_CAM_UP, "Up" },               // right stick: zoom in / out
    { "CameraDown", PAD_CAM_DOWN, "Down" },
};
#define PAD_INPUT_COUNT ((int)(sizeof(pad_inputs) / sizeof(pad_inputs[0])))

struct PadKeyName {
    const char *name;
    unsigned short code;
};

static const PadKeyName pad_key_names[] = {
    { "Esc", 0x01 }, { "1", 0x02 }, { "2", 0x03 }, { "3", 0x04 }, { "4", 0x05 }, { "5", 0x06 },
    { "6", 0x07 }, { "7", 0x08 }, { "8", 0x09 }, { "9", 0x0a }, { "0", 0x0b }, { "Minus", 0x0c }, { "Equals", 0x0d },
    { "Backspace", 0x0e }, { "Tab", 0x0f }, { "Q", 0x10 }, { "W", 0x11 }, { "E", 0x12 }, { "R", 0x13 }, { "T", 0x14 },
    { "Y", 0x15 }, { "U", 0x16 }, { "I", 0x17 }, { "O", 0x18 }, { "P", 0x19 }, { "LBracket", 0x1a }, { "RBracket", 0x1b },
    { "Enter", 0x1c }, { "LCtrl", 0x1d }, { "A", 0x1e }, { "S", 0x1f }, { "D", 0x20 }, { "F", 0x21 }, { "G", 0x22 },
    { "H", 0x23 }, { "J", 0x24 }, { "K", 0x25 }, { "L", 0x26 }, { "Semicolon", 0x27 }, { "Apostrophe", 0x28 },
    { "Grave", 0x29 }, { "LShift", 0x2a }, { "Backslash", 0x2b }, { "Z", 0x2c }, { "X", 0x2d }, { "C", 0x2e },
    { "V", 0x2f }, { "B", 0x30 }, { "N", 0x31 }, { "M", 0x32 }, { "Comma", 0x33 }, { "Period", 0x34 }, { "Slash", 0x35 },
    { "RShift", 0x36 }, { "NumpadStar", 0x37 }, { "LAlt", 0x38 }, { "Space", 0x39 }, { "CapsLock", 0x3a },
    { "F1", 0x3b }, { "F2", 0x3c }, { "F3", 0x3d }, { "F4", 0x3e }, { "F5", 0x3f }, { "F6", 0x40 }, { "F7", 0x41 },
    { "F8", 0x42 }, { "F9", 0x43 }, { "F10", 0x44 }, { "NumLock", 0x45 }, { "ScrollLock", 0x46 }, { "Numpad7", 0x47 },
    { "Numpad8", 0x48 }, { "Numpad9", 0x49 }, { "NumpadMinus", 0x4a }, { "Numpad4", 0x4b }, { "Numpad5", 0x4c },
    { "Numpad6", 0x4d }, { "NumpadPlus", 0x4e }, { "Numpad1", 0x4f }, { "Numpad2", 0x50 }, { "Numpad3", 0x51 },
    { "Numpad0", 0x52 }, { "NumpadPeriod", 0x53 }, { "F11", 0x57 }, { "F12", 0x58 }, { "NumpadEnter", 0x9c },
    { "RCtrl", 0x9d }, { "NumpadSlash", 0xb5 }, { "RAlt", 0xb8 }, { "Home", 0xc7 }, { "Up", 0xc8 }, { "PageUp", 0xc9 },
    { "Left", 0xcb }, { "Right", 0xcd }, { "End", 0xcf }, { "Down", 0xd0 }, { "PageDown", 0xd1 }, { "Insert", 0xd2 },
    { "Delete", 0xd3 },
    { "MouseLeft", 0x100 }, { "MouseRight", 0x101 }, { "MouseMiddle", 0x102 }, { "Mouse4", 0x103 }, { "Mouse5", 0x104 },
};
#define PAD_KEY_NAME_COUNT ((int)(sizeof(pad_key_names) / sizeof(pad_key_names[0])))

// "E", "escape", "0x17" -> code; -1 if unknown. Stops at whitespace, ';' or the end (keynames.h KeyParse).
static int pad_key_parse(const char *s)
{
    char buf[32];
    size_t n;
    int i;

    while (*s == ' ' || *s == '\t')
        s++;
    n = strcspn(s, " \t;\r\n");
    if (n == 0 || n >= sizeof(buf))
        return -1;
    memcpy(buf, s, n);
    buf[n] = 0;
    if (buf[0] == '0' && (buf[1] == 'x' || buf[1] == 'X')) {
        long v = strtol(buf, NULL, 16);
        return v > 0 && v < 256 ? (int)v : -1;
    }
    if (_stricmp(buf, "Escape") == 0)
        return 0x01;
    for (i = 0; i < PAD_KEY_NAME_COUNT; i++)
        if (_stricmp(buf, pad_key_names[i].name) == 0)
            return pad_key_names[i].code;
    return -1;
}

// ---- XInput, declared here (no header, no import library) ----------------------------------------------------
struct PadGamepad {                     // XINPUT_GAMEPAD
    WORD wButtons;
    BYTE bLeftTrigger, bRightTrigger;
    SHORT sThumbLX, sThumbLY, sThumbRX, sThumbRY;
};
struct PadState {                       // XINPUT_STATE
    DWORD dwPacketNumber;
    PadGamepad Gamepad;
};
typedef DWORD (WINAPI *PFN_XInputGetState)(DWORD, PadState *);

// ---- state ---------------------------------------------------------------------------------------------------
static PFN_XInputGetState pad_get_state;
static bool pad_inited;                 // exp_controller_init ran
static int pad_enabled;                 // [Controller] Enabled (and XInput found)
static int pad_dead_zone = PAD_DEF_DEADZONE;
static int pad_cursor_speed = PAD_DEF_CURSOR_SPEED;
static int pad_cam_threshold = PAD_DEF_CAMERA_THRESHOLD;
static int pad_trig_threshold = PAD_DEF_TRIGGER_THRESHOLD;
static int pad_target[PAD_INPUT_COUNT]; // key 0-255, KEY_MOUSE_BASE + 0..2, or -1 (none)
static int pad_slot = -1;               // connected XInput user index, or -1
static DWORD pad_next_scan;             // when to look for a controller again (GetTickCount)
static bool pad_logged_none;
static unsigned pad_down;               // inputs held, as last sent to the game
static LARGE_INTEGER pad_last_frame, pad_freq;
static double pad_cur_x, pad_cur_y;     // the cursor in game pixels, with the fraction
static int pad_set_x = -1, pad_set_y = -1; // the position last written (-1: none yet)

// Synthetic pad state (agent mode, through the exports below): the agent thread writes, the game thread reads.
// A spin lock (InterlockedExchange), so nothing needs initialising before the first call.
static LONG pad_synth_lock;
static PadGamepad pad_synth;
static DWORD pad_synth_start, pad_synth_ms;
static LONG pad_synth_active;           // 1 while the synthetic state applies
static LONG pad_synth_done = 1;         // 1 once a frame after it has released its inputs

static void synth_lock()
{
    while (InterlockedExchange(&pad_synth_lock, 1))
        Sleep(0);
}

static void synth_unlock()
{
    InterlockedExchange(&pad_synth_lock, 0);
}

static LONG synth_active()
{
    return *(volatile LONG *)&pad_synth_active;
}

static int clamp_i(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

// ---- configuration -------------------------------------------------------------------------------------------
static int parse_target(const char *ini_key, const char *def)
{
    char name[32];
    int code;

    exp_ini_string("Controller", ini_key, def, name, sizeof(name));
    if (!name[0] || !_stricmp(name, "none"))
        return -1;
    code = pad_key_parse(name);
    if (code < 0 || code >= KEY_MOUSE_BASE + 3) { // the game has no side buttons to press
        exp_log("controller: %s=%s is not a game key or MouseLeft/MouseRight/MouseMiddle; using %s", ini_key, name, def);
        code = pad_key_parse(def);
    }
    return code;
}

static void load_config()
{
    int i;

    pad_enabled = exp_ini_int("Controller", "Enabled", 0);
    pad_dead_zone = clamp_i(exp_ini_int("Controller", "DeadZone", PAD_DEF_DEADZONE), 0, 30000);
    pad_cursor_speed = clamp_i(exp_ini_int("Controller", "CursorSpeed", PAD_DEF_CURSOR_SPEED), 50, 5000);
    pad_cam_threshold = clamp_i(exp_ini_int("Controller", "CameraThreshold", PAD_DEF_CAMERA_THRESHOLD), 1000, 32000);
    pad_trig_threshold = clamp_i(exp_ini_int("Controller", "TriggerThreshold", PAD_DEF_TRIGGER_THRESHOLD), 1, 254);
    for (i = 0; i < PAD_INPUT_COUNT; i++)
        pad_target[i] = parse_target(pad_inputs[i].iniKey, pad_inputs[i].def);
}

static void load_xinput()
{
    static const char *const dlls[] = { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" };
    int i;

    for (i = 0; i < 3 && !pad_get_state; i++) {
        HMODULE m = LoadLibraryA(dlls[i]);
        if (!m)
            continue;
        pad_get_state = (PFN_XInputGetState)GetProcAddress(m, "XInputGetState");
        if (pad_get_state)
            exp_log("controller: using %s", dlls[i]);
        else
            FreeLibrary(m);
    }
    if (!pad_get_state)
        exp_log("controller: no XInput DLL found; controller support off");
}

// ---- output to the game --------------------------------------------------------------------------------------
// Press or release what an input is mapped to: a game key through key_mark (the layout swap held off, so the pad
// sends the game key named in the ini), a mouse button through mouse_mark_button.
static void send(int input, bool down)
{
    int t = pad_target[input];
    int layout;

    if (t < 0)
        return;
    exp_log("controller: %s %s", pad_inputs[input].iniKey, down ? "down" : "up");
    if (t >= KEY_MOUSE_BASE) {
        mouse_mark_button(t - KEY_MOUSE_BASE, down);
    } else {
        layout = key_layout;
        key_layout = 0;
        key_mark(t, down ? 1 : 0, 0);
        key_layout = layout;
    }
}

// Shift, Ctrl, Alt and CapsLock: held modifiers (chain attack slots, Shift+Tab, Shift/Alt+click).
static bool is_modifier(int code)
{
    return code == KEY_LSHIFT || code == KEY_RSHIFT || code == KEY_LCONTROL || code == KEY_RCONTROL ||
           code == KEY_LALT || code == KEY_RALT || code == KEY_CAPSLOCK;
}

// Move the cursor by (dx, dy) game pixels, inside the game's mouse area.
static void move_cursor(double dx, double dy)
{
    int w = mouse_screen_w, h = mouse_screen_h;
    int x, y;

    if (!mouse_inited || w <= 0 || h <= 0)
        return;
    // Outside game play the UI is a 1024-wide layout (wider screens show it at the left, or centred by
    // SummonerFix's [Display] CenterMenus): keep the cursor on it.
    if (gameseq_state_cur != 4 && w > 1024)
        w = 1024;
    // Start from the game's position unless it is where this code put it (one pixel of rounding allowed: the
    // position goes through the window's scale and back): the real mouse or agent mode moved it.
    if (pad_set_x < 0 || abs(game_mouse_x - pad_set_x) > 1 || abs(game_mouse_y - pad_set_y) > 1) {
        pad_cur_x = game_mouse_x + 0.5;
        pad_cur_y = game_mouse_y + 0.5;
    }
    pad_cur_x += dx;
    pad_cur_y += dy;
    if (pad_cur_x < 0.0)
        pad_cur_x = 0.0;
    if (pad_cur_x > w - 0.001)
        pad_cur_x = w - 0.001;
    if (pad_cur_y < 0.0)
        pad_cur_y = 0.0;
    if (pad_cur_y > h - 0.001)
        pad_cur_y = h - 0.001;
    x = (int)pad_cur_x;
    y = (int)pad_cur_y;
    pad_set_x = x;
    pad_set_y = y;
    if (x == game_mouse_x && y == game_mouse_y)
        return;
    // This frame sees the new position; mouse_set_pos moves the Windows cursor there, so the next mouse_update
    // reads it back (and the real mouse carries on from it).
    mouse_x = x;
    mouse_y = y;
    game_mouse_x = x;
    game_mouse_y = y;
    mouse_set_pos(x, y);
}

// ---- input ---------------------------------------------------------------------------------------------------
// The real controller's state, or false (none connected, or not looked for this frame).
static bool read_real_pad(PadGamepad *pad)
{
    PadState s;
    DWORD i;

    if (!pad_get_state)
        return false;
    if (pad_slot < 0) {
        // XInputGetState on an empty slot is slow, so look for a controller only once a second.
        DWORD now = GetTickCount();
        if ((int)(now - pad_next_scan) < 0)
            return false;
        pad_next_scan = now + 1000;
        for (i = 0; i < 4; i++) {
            if (pad_get_state(i, &s) == ERROR_SUCCESS) {
                pad_slot = (int)i;
                exp_log("controller: connected (XInput user %lu)", (unsigned long)i);
                break;
            }
        }
        if (pad_slot < 0) {
            if (!pad_logged_none)
                exp_log("controller: none connected (checking once a second)");
            pad_logged_none = true;
            return false;
        }
    } else if (pad_get_state((DWORD)pad_slot, &s) != ERROR_SUCCESS) {
        exp_log("controller: disconnected (XInput user %d)", pad_slot);
        pad_slot = -1;
        pad_next_scan = GetTickCount() + 1000;
        return false;
    }
    *pad = s.Gamepad;
    return true;
}

static bool read_synthetic(PadGamepad *pad)
{
    bool on;

    if (!synth_active())
        return false;
    synth_lock();
    on = GetTickCount() - pad_synth_start < pad_synth_ms;
    if (on)
        *pad = pad_synth;
    else
        InterlockedExchange(&pad_synth_active, 0);
    synth_unlock();
    return on;
}

// One stick axis held past the threshold (3/4 of it to stay held, so a key does not chatter at the edge).
static bool axis_held(int v, int sign, bool was_down)
{
    int t = was_down ? pad_cam_threshold * 3 / 4 : pad_cam_threshold;
    return sign > 0 ? v > t : v < -t;
}

// ---- entry points --------------------------------------------------------------------------------------------
void exp_controller_init()
{
    QueryPerformanceFrequency(&pad_freq);
    load_config();                      // the mapping also serves the synthetic state (agent mode)
    pad_inited = true;
    if (!pad_enabled) {
        exp_log("controller: off ([Controller] Enabled=0)");
        return;
    }
    load_xinput();
    if (!pad_get_state) {
        pad_enabled = 0;
        return;
    }
    exp_log("controller: on (dead zone %d, cursor speed %d, camera threshold %d, trigger threshold %d)",
            pad_dead_zone, pad_cursor_speed, pad_cam_threshold, pad_trig_threshold);
}

// GameTick, after mouse_get_pos, before the state's frame handler (exp_frame_begin).
void exp_controller_frame()
{
    LARGE_INTEGER now;
    double dt, x, y, mag, n, v;
    PadGamepad pad;
    bool have;
    unsigned down, changed, m;
    int pass, i, press, want_mod;

    if (!pad_enabled && !synth_active() && !pad_down)
        return;                         // off (and no agent "pad" state): the game's own input only

    QueryPerformanceCounter(&now);
    dt = pad_last_frame.QuadPart && pad_freq.QuadPart
             ? (double)(now.QuadPart - pad_last_frame.QuadPart) / (double)pad_freq.QuadPart : 0.0;
    pad_last_frame = now;
    if (dt > 0.05)
        dt = 0.05;                      // after a stall (loading) do not jump the cursor across the screen

    have = read_synthetic(&pad);
    if (!have && pad_enabled) {
        have = read_real_pad(&pad);
        // A real pad only counts while the game is in front (it pauses otherwise); agent mode keeps it in front.
        if (have && !os_foreground())
            have = false;
    }
    if (!have) {
        if (!pad_down) {
            if (!synth_active())
                InterlockedExchange(&pad_synth_done, 1);
            return;                     // idle: touch nothing
        }
        memset(&pad, 0, sizeof(pad));   // release whatever is still held
    }

    down = pad.wButtons & PAD_BUTTON_BITS;
    if (pad.bLeftTrigger > pad_trig_threshold)
        down |= PAD_LTRIGGER;
    if (pad.bRightTrigger > pad_trig_threshold)
        down |= PAD_RTRIGGER;
    if (axis_held(pad.sThumbRX, -1, (pad_down & PAD_CAM_LEFT) != 0))
        down |= PAD_CAM_LEFT;
    if (axis_held(pad.sThumbRX, +1, (pad_down & PAD_CAM_RIGHT) != 0))
        down |= PAD_CAM_RIGHT;
    if (axis_held(pad.sThumbRY, +1, (pad_down & PAD_CAM_UP) != 0))
        down |= PAD_CAM_UP;
    if (axis_held(pad.sThumbRY, -1, (pad_down & PAD_CAM_DOWN) != 0))
        down |= PAD_CAM_DOWN;

    // Press on down, release on up. Order: other releases, modifier releases, modifier presses, other presses, so
    // a modifier pressed with a button in the same frame (LT+B: Shift + right click = chain slot 1) is down before
    // the button and let go after it.
    changed = down ^ pad_down;
    for (pass = 0; pass < 4 && changed; pass++) {
        press = pass >= 2;
        want_mod = pass == 1 || pass == 2;
        for (i = 0; i < PAD_INPUT_COUNT; i++) {
            m = pad_inputs[i].mask;
            if ((changed & m) && !press == !(down & m) && (is_modifier(pad_target[i]) ? 1 : 0) == want_mod)
                send(i, press != 0);
        }
    }
    pad_down = down;

    // Left stick: cursor speed proportional to the deflection beyond the (radial) dead zone.
    x = pad.sThumbLX;
    y = pad.sThumbLY;
    mag = sqrt(x * x + y * y);
    if (mag > pad_dead_zone && dt > 0.0) {
        n = (mag - pad_dead_zone) / (32767.0 - pad_dead_zone);
        if (n > 1.0)
            n = 1.0;
        v = n * pad_cursor_speed * dt / mag;
        move_cursor(x * v, -y * v);     // stick up = screen up
    }

    if (!synth_active() && !pad_down)
        InterlockedExchange(&pad_synth_done, 1);
}

// ---- agent mode: a synthetic pad state (exported; SummonerFix's "pad" command can forward to it) ------------
// lx..ry as XInput thumb values, buttons as wButtons bits plus 0x10000 (LT) / 0x20000 (RT) fully pressed, for ms
// milliseconds. Any thread; the game thread uses it instead of XInput while it lasts (with Enabled=0 too).
extern "C" __declspec(dllexport) void __cdecl ExpControllerSetSynthetic(int lx, int ly, int rx, int ry,
                                                                       unsigned buttons, unsigned long ms)
{
    PadGamepad p;

    memset(&p, 0, sizeof(p));
    p.sThumbLX = (SHORT)clamp_i(lx, -32768, 32767);
    p.sThumbLY = (SHORT)clamp_i(ly, -32768, 32767);
    p.sThumbRX = (SHORT)clamp_i(rx, -32768, 32767);
    p.sThumbRY = (SHORT)clamp_i(ry, -32768, 32767);
    p.wButtons = (WORD)(buttons & PAD_BUTTON_BITS);
    p.bLeftTrigger = (buttons & PAD_LTRIGGER) ? 255 : 0;
    p.bRightTrigger = (buttons & PAD_RTRIGGER) ? 255 : 0;
    synth_lock();
    pad_synth = p;
    pad_synth_start = GetTickCount();
    pad_synth_ms = ms;
    InterlockedExchange(&pad_synth_active, 1); // before clearing "done": the game thread sets it only while inactive
    InterlockedExchange(&pad_synth_done, 0);
    synth_unlock();
}

// 1 once the synthetic state has ended and a frame has released its inputs (0 before the first frame ran).
extern "C" __declspec(dllexport) int __cdecl ExpControllerSyntheticDone()
{
    return pad_inited && *(volatile LONG *)&pad_synth_done != 0;
}

#endif // EXPERIMENTAL_BUILD
