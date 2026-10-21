// Controller (gamepad) support, tier 1: an XInput pad drives the game's existing mouse and keyboard input.
// No game logic changes: the left stick moves the cursor, the right stick holds the camera arrow keys, and the
// buttons press game keys and mouse buttons ([Controller] in summonerfix.ini, off by default).
//
// Everything goes through the paths agent mode uses: the cursor through display.c (DisplayMoveCursor, the same
// position the ScreenToClient hook hands the game), keys through key_mark and buttons through mouse_mark_button
// (keys.c KeysInjectKey / KeysInjectMouse). The pad is polled once per frame from the SwapBuffers hook
// (display.c), so on the game thread, and only the OpenGL renderer is supported (as for agent mode's cursor).
//
// Idle (left stick inside the dead zone, nothing pressed) the controller touches neither the cursor nor the
// keys: keys are pressed and released only on a change, so agent mode and the real mouse and keyboard work as
// before. Agent mode's "pad LX LY RX RY BUTTONS [MS]" feeds a synthetic pad state into the same code, for
// testing without a controller (works with Enabled=0 too).
#include "summonerfix.h"
#include "keynames.h"
#include "padmap.h"
#include <math.h>
#include <string.h>

#define VA_IS_MAIN_WND_ACTIVE 0x2cbff1cu // the game pauses while this is 0 (agent mode keeps it 1)

// XINPUT_STATE / XINPUT_GAMEPAD, declared here so no XInput header or import library is needed.
typedef struct {
    WORD wButtons;
    BYTE bLeftTrigger, bRightTrigger;
    SHORT sThumbLX, sThumbLY, sThumbRX, sThumbRY;
} PadGamepad;
typedef struct {
    DWORD dwPacketNumber;
    PadGamepad Gamepad;
} PadState;
typedef DWORD(WINAPI *PFN_XInputGetState)(DWORD, PadState *);

static PFN_XInputGetState g_getState;
static int g_enabled;     // [Controller] Enabled
static int g_deadZone = PAD_DEF_DEADZONE, g_cursorSpeed = PAD_DEF_CURSOR_SPEED;
static int g_camThreshold = PAD_DEF_CAMERA_THRESHOLD, g_trigThreshold = PAD_DEF_TRIGGER_THRESHOLD;
static int g_target[PAD_INPUT_COUNT]; // game key 0-255, KEY_MOUSE_BASE + 0..2 for a mouse button, or -1
static int g_slot = -1;               // connected XInput user index, or -1
static DWORD g_nextScan;              // when to look for a controller again (GetTickCount)
static unsigned g_down;               // inputs currently held (PAD_* bits), as last sent to the game
static LARGE_INTEGER g_lastFrame, g_freq;

// Synthetic pad state from agent mode (agent thread writes, game thread reads).
static CRITICAL_SECTION g_synthLock;
static PadGamepad g_synth;
static DWORD g_synthStart, g_synthMs;
static volatile LONG g_synthActive; // 1 while the synthetic state applies
static volatile LONG g_synthDone = 1; // 1 once a frame after the synthetic state has released its inputs

static int ParseTarget(const char *iniKey, const char *def)
{
    char name[32];
    GetPrivateProfileStringA("Controller", iniKey, def, name, sizeof name, g_cfg.iniPath);
    if (!name[0] || !_stricmp(name, "none"))
        return -1;
    int code = KeyParse(name);
    if (code < 0 || code >= KEY_MOUSE_BASE + 3) { // the game has no side buttons to press
        Log("controller: %s=%s is not a game key or MouseLeft/MouseRight/MouseMiddle; using %s", iniKey, name, def);
        code = KeyParse(def);
    }
    return code;
}

static int ClampInt(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

static void LoadConfig(void)
{
    const char *ini = g_cfg.iniPath;
    g_enabled = GetPrivateProfileIntA("Controller", "Enabled", 0, ini);
    g_deadZone = ClampInt(GetPrivateProfileIntA("Controller", "DeadZone", PAD_DEF_DEADZONE, ini), 0, 30000);
    g_cursorSpeed = ClampInt(GetPrivateProfileIntA("Controller", "CursorSpeed", PAD_DEF_CURSOR_SPEED, ini), 50, 5000);
    g_camThreshold = ClampInt(GetPrivateProfileIntA("Controller", "CameraThreshold", PAD_DEF_CAMERA_THRESHOLD, ini), 1000, 32000);
    g_trigThreshold = ClampInt(GetPrivateProfileIntA("Controller", "TriggerThreshold", PAD_DEF_TRIGGER_THRESHOLD, ini), 1, 254);
    for (int i = 0; i < PAD_INPUT_COUNT; i++)
        g_target[i] = ParseTarget(k_padInputs[i].iniKey, k_padInputs[i].def);
}

static void LoadXInput(void)
{
    static const char *const dlls[] = {"xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll"};
    for (int i = 0; i < 3 && !g_getState; i++) {
        HMODULE m = LoadLibraryA(dlls[i]);
        if (!m)
            continue;
        g_getState = (PFN_XInputGetState)GetProcAddress(m, "XInputGetState");
        if (g_getState)
            Log("controller: using %s", dlls[i]);
        else
            FreeLibrary(m);
    }
    if (!g_getState)
        Log("controller: no XInput DLL found; controller support off");
}

// Press or release what an input is mapped to.
static void Send(int input, int down)
{
    int t = g_target[input];
    if (t < 0)
        return;
    if (t >= KEY_MOUSE_BASE)
        KeysInjectMouse(t - KEY_MOUSE_BASE, down);
    else
        KeysInjectKey(t, down);
}

// Shift, Ctrl, Alt and CapsLock: held modifiers (chain attack slots, Shift+Tab, Shift/Alt+click).
static int IsModifier(int code)
{
    return code == 0x2a || code == 0x36 || code == 0x1d || code == 0x9d || code == 0x38 || code == 0xb8 || code == 0x3a;
}

// The real controller's state, or FALSE (none connected, not polled this frame).
static BOOL ReadRealPad(PadGamepad *pad)
{
    if (!g_getState)
        return FALSE;
    PadState s;
    if (g_slot < 0) {
        // XInputGetState on an empty slot is slow, so look for a controller only once a second.
        static BOOL loggedNone;
        DWORD now = GetTickCount();
        if ((int)(now - g_nextScan) < 0)
            return FALSE;
        g_nextScan = now + 1000;
        for (DWORD i = 0; i < 4; i++) {
            if (g_getState(i, &s) == ERROR_SUCCESS) {
                g_slot = (int)i;
                Log("controller: connected (XInput user %lu)", i);
                break;
            }
        }
        if (g_slot < 0) {
            if (!loggedNone)
                Log("controller: none connected (checking once a second)");
            loggedNone = TRUE;
            return FALSE;
        }
    } else if (g_getState((DWORD)g_slot, &s) != ERROR_SUCCESS) {
        Log("controller: disconnected (XInput user %d)", g_slot);
        g_slot = -1;
        g_nextScan = GetTickCount() + 1000;
        return FALSE;
    }
    *pad = s.Gamepad;
    return TRUE;
}

static BOOL ReadSynthetic(PadGamepad *pad)
{
    if (!g_synthActive)
        return FALSE;
    BOOL on;
    EnterCriticalSection(&g_synthLock);
    on = GetTickCount() - g_synthStart < g_synthMs;
    if (on)
        *pad = g_synth;
    else
        InterlockedExchange(&g_synthActive, 0);
    LeaveCriticalSection(&g_synthLock);
    return on;
}

// One stick axis held past the threshold (with some hysteresis so a key does not chatter at the edge).
static BOOL AxisHeld(int v, int sign, BOOL wasDown)
{
    int t = wasDown ? g_camThreshold * 3 / 4 : g_camThreshold;
    return sign > 0 ? v > t : v < -t;
}

void ControllerFrame(void)
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double dt = g_lastFrame.QuadPart && g_freq.QuadPart ? (double)(now.QuadPart - g_lastFrame.QuadPart) / (double)g_freq.QuadPart : 0.0;
    g_lastFrame = now;
    if (dt > 0.05)
        dt = 0.05; // after a stall (loading) do not jump the cursor across the screen

    PadGamepad pad;
    BOOL have = ReadSynthetic(&pad);
    if (!have && g_enabled) {
        have = ReadRealPad(&pad);
        // A real pad only counts while the game is in front (it pauses otherwise); agent mode keeps it "in front".
        if (have && !*(volatile BYTE *)(UINT_PTR)SumVa(VA_IS_MAIN_WND_ACTIVE))
            have = FALSE;
    }
    if (!have) {
        if (!g_down) {
            if (!g_synthActive)
                InterlockedExchange(&g_synthDone, 1);
            return; // idle: touch nothing
        }
        ZeroMemory(&pad, sizeof pad); // release whatever is still held
    }

    unsigned down = pad.wButtons & 0xF3FFu; // the 14 XInput buttons
    if (pad.bLeftTrigger > g_trigThreshold)
        down |= PAD_LTRIGGER;
    if (pad.bRightTrigger > g_trigThreshold)
        down |= PAD_RTRIGGER;
    if (AxisHeld(pad.sThumbRX, -1, g_down & PAD_CAM_LEFT))
        down |= PAD_CAM_LEFT;
    if (AxisHeld(pad.sThumbRX, +1, g_down & PAD_CAM_RIGHT))
        down |= PAD_CAM_RIGHT;
    if (AxisHeld(pad.sThumbRY, +1, g_down & PAD_CAM_UP))
        down |= PAD_CAM_UP;
    if (AxisHeld(pad.sThumbRY, -1, g_down & PAD_CAM_DOWN))
        down |= PAD_CAM_DOWN;

    // Edge detection: press on down, release on up. Order: other releases, modifier releases, modifier presses,
    // other presses, so a modifier pressed with a button in the same frame (LT+B: Shift + right click = chain slot
    // 1) is down before the button, and is let go after it.
    unsigned changed = down ^ g_down;
    for (int pass = 0; pass < 4 && changed; pass++) {
        int press = pass >= 2, wantMod = pass == 1 || pass == 2;
        for (int i = 0; i < PAD_INPUT_COUNT; i++) {
            unsigned m = k_padInputs[i].mask;
            if ((changed & m) && !press == !(down & m) && IsModifier(g_target[i]) == wantMod)
                Send(i, press);
        }
    }
    g_down = down;

    // Left stick: cursor speed proportional to the deflection beyond the (radial) dead zone.
    double x = pad.sThumbLX, y = pad.sThumbLY, mag = sqrt(x * x + y * y);
    if (mag > g_deadZone && dt > 0.0) {
        double n = (mag - g_deadZone) / (32767.0 - g_deadZone);
        if (n > 1.0)
            n = 1.0;
        double v = n * g_cursorSpeed * dt / mag;
        DisplayMoveCursor(x * v, -y * v); // stick up = screen up
    }

    if (!g_synthActive && !g_down)
        InterlockedExchange(&g_synthDone, 1);
}

void ControllerSetSynthetic(int lx, int ly, int rx, int ry, unsigned buttons, DWORD ms)
{
    PadGamepad p;
    ZeroMemory(&p, sizeof p);
    p.sThumbLX = (SHORT)ClampInt(lx, -32768, 32767);
    p.sThumbLY = (SHORT)ClampInt(ly, -32768, 32767);
    p.sThumbRX = (SHORT)ClampInt(rx, -32768, 32767);
    p.sThumbRY = (SHORT)ClampInt(ry, -32768, 32767);
    p.wButtons = (WORD)(buttons & 0xF3FFu);
    // Trigger bits above the XInput buttons (0x10000 LT, 0x20000 RT) fully press the triggers.
    p.bLeftTrigger = (buttons & PAD_LTRIGGER) ? 255 : 0;
    p.bRightTrigger = (buttons & PAD_RTRIGGER) ? 255 : 0;
    EnterCriticalSection(&g_synthLock);
    g_synth = p;
    g_synthStart = GetTickCount();
    g_synthMs = ms;
    InterlockedExchange(&g_synthActive, 1); // before clearing "done": the game thread sets it only while inactive
    InterlockedExchange(&g_synthDone, 0);
    LeaveCriticalSection(&g_synthLock);
}

BOOL ControllerSyntheticDone(void)
{
    return g_synthDone != 0;
}

void ControllerInstall(void)
{
    InitializeCriticalSection(&g_synthLock);
    QueryPerformanceFrequency(&g_freq);
    LoadConfig();
    if (!g_enabled) {
        Log("controller: off ([Controller] Enabled=0)");
        return;
    }
    if (RebuiltFixInSource("Controller")) {
        // The experimental build reads the controller itself (src/decomp/experimental/controller.cpp).
        Log("controller: in this exe's source (EXPERIMENTAL_BUILD), not driven by SummonerFix");
        g_enabled = 0;
        return;
    }
    if (g_cfg.renderer != RENDERER_OPENGL) {
        Log("controller: needs the OpenGL renderer (cursor and per-frame poll are in its display hooks); off");
        g_enabled = 0;
        return;
    }
    LoadXInput();
    if (!g_getState) {
        g_enabled = 0;
        return;
    }
    Log("controller: on (dead zone %d, cursor speed %d, camera threshold %d, trigger threshold %d)", g_deadZone,
        g_cursorSpeed, g_camThreshold, g_trigThreshold);
}
