// keys.cpp - key bindings and hotkeys in the source (include/modern/modern.h; L3 player build). Not original source.
//
// SummonerFix's keys.c for a build that runs without it, reading the same summonerfix.ini sections, so
// SummonerFixConfig.exe edits both:
//   [KeyBindings]  PhysicalKey=GameKey (keys and mouse buttons, keynames.h); key_mark calls modern_key_bind after
//                  the layout swap, mouse_msg_handler sends buttons through modern_mouse_button, and key_to_ascii
//                  asks modern_key_physical, so typing gives the keys actually pressed.
//   [ConsoleKeys]  Key=command: the developer console command runs when the key is pressed (execute_command_line).
//   [Game]         ConfigKey (F10) opens SummonerFixConfig.exe; SpeedKey (F11), SpeedFactor (3), SpeedMode
//                  (Toggle or Hold): fast mode through frametime_divisor.
//   [Debug]        ScriptEditorKey toggles the level script editor (game_flags 0x80) in play.
// The sections are re-read when the ini changes (a watcher thread), so the config app's changes apply live. Off,
// with every call passing codes through, while SummonerFix is loaded (it installs its own).
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "modern/modern.h"
#include "../../summonerfix/keynames.h"
#include "vsdk/os/os.h"
#include "vsdk/os/key.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/console.h"
#include "vsdk/os/frametime.h"
#include "summoner/summoner.h"

#if defined(MODERN_BUILD)

#ifndef WM_XBUTTONDOWN   // VC6's headers (the 32-bit clang build) predate the side buttons
#define WM_XBUTTONDOWN 0x020B
#define WM_XBUTTONUP 0x020C
#define XBUTTON1 0x0001
#define XBUTTON2 0x0002
#endif

static bool keys_on;
static Bindings key_sets[2];
static Bindings *volatile key_bind = &key_sets[0];   // the set in use; the other one is filled on reload
static volatile int key_injecting;                   // modern_key_mark_raw: codes pass unchanged
static volatile unsigned char key_physical[256];      // game key code -> the physical key that last produced it
static char key_last_section[8192];

int modern_key_bind(int code)
{
    if (!keys_on || key_injecting || code < 0 || code >= 256)
        return code;
    int c = key_bind->map[code];
    key_physical[c & 0xff] = (unsigned char)code;
    return c;
}

int modern_key_physical(int code)
{
    return keys_on ? key_physical[code & 0xff] : (code & 0xff);
}

void modern_key_mark_raw(int code, int down)
{
    key_injecting = 1;
    key_mark(code, down, 0);
    key_injecting = 0;
}

// A mouse button (0-4) went down or up: its binding, or the default for left, right and middle.
static void mouse_event(int button, bool down)
{
    int to = keys_on ? key_bind->mouseTo[button] : -1;
    if (to < 0) {
        if (button < 3)
            mouse_mark_button(button, down);
    } else if (to >= KEY_MOUSE_BASE) {
        mouse_mark_button(to - KEY_MOUSE_BASE, down);
    } else {
        modern_key_mark_raw(to, down);
    }
}

void modern_mouse_button(int button, bool down)
{
    mouse_event(button, down);
}

// ---- [KeyBindings] ----

static void log_bad(const char *line)
{
    modern_log("key bindings: ignoring \"%s\" (unknown key name or unsupported combination)", line);
}

// Re-reads [KeyBindings]; false when the section text did not change.
static bool load_bindings(bool initial)
{
    static char section[sizeof key_last_section];
    DWORD len = GetPrivateProfileSectionA("KeyBindings", section, sizeof section, modern_ini_path());
    if (len == 0)
        section[0] = section[1] = 0;
    if (!initial && memcmp(section, key_last_section, len + 2) == 0)
        return false;
    memcpy(key_last_section, section, len + 2);
    Bindings *next = key_bind == &key_sets[0] ? &key_sets[1] : &key_sets[0];
    int count = KeyParseSection(section, next, log_bad);
    key_bind = next;   // one pointer store: readers see the old set or the new one
    char a[8], b[8];
    for (int i = 0; i < 256; i++) {
        if (next->keyMouse[i] >= 0)
            modern_log("key bindings: %s acts as %s", KeyNameOf(i, a, sizeof a), KeyNameOf(KEY_MOUSE_BASE + next->keyMouse[i], b, sizeof b));
        else if (next->map[i] != i)
            modern_log("key bindings: %s acts as %s", KeyNameOf(i, a, sizeof a), KeyNameOf(next->map[i], b, sizeof b));
    }
    for (int i = 0; i < KEY_MOUSE_COUNT; i++)
        if (next->mouseTo[i] >= 0)
            modern_log("key bindings: %s acts as %s", KeyNameOf(KEY_MOUSE_BASE + i, a, sizeof a), KeyNameOf(next->mouseTo[i], b, sizeof b));
    modern_log("key bindings: %d %s", count, initial ? "loaded" : "reloaded");
    return true;
}

// ---- [ConsoleKeys] ----

#define CONSOLE_CMD_LEN 96
struct console_keys {
    char cmd[256][CONSOLE_CMD_LEN];
};
static console_keys ck_sets[2];
static console_keys *volatile ck = &ck_sets[0];

static void load_console_keys()
{
    console_keys *next = ck == &ck_sets[0] ? &ck_sets[1] : &ck_sets[0];
    memset(next, 0, sizeof *next);
    static char buf[8192];
    DWORD n = GetPrivateProfileSectionA("ConsoleKeys", buf, sizeof buf, modern_ini_path());
    int count = 0;
    for (char *line = buf; n && *line; line += strlen(line) + 1) {
        char *eq = strchr(line, '=');
        if (!eq || *line == ';')
            continue;
        char name[32];
        size_t len = (size_t)(eq - line);
        if (len >= sizeof name)
            continue;
        memcpy(name, line, len);
        name[len] = 0;
        int scan = KeyParse(name);
        if (scan <= 0 || scan > 0xff || !eq[1]) {
            modern_log("console keys: ignored \"%s\"", line);
            continue;
        }
        lstrcpynA(next->cmd[scan], eq + 1, CONSOLE_CMD_LEN);
        count++;
    }
    // Logged only on a change: the log is in the watched folder, so every log line would trigger another reload.
    bool changed = memcmp(next, ck, sizeof *next) != 0;
    ck = next;
    if (changed)
        modern_log("console keys: %d command key(s)", count);
}

static void run_console_key(int scan)
{
    const char *cmd = ck->cmd[scan & 0xff];
    if (!cmd[0])
        return;
    // Before the first game state the command system is not set up (a console key crashed the game, 2026-10-16).
    if (gameseq_state_cur < 1) {
        modern_log("console key ignored during start-up: %s", cmd);
        return;
    }
    char line[CONSOLE_CMD_LEN];
    lstrcpynA(line, cmd, sizeof line);   // the parser works on its own copy
    modern_log("console key: %s", line);
    execute_command_line(line);
}

// ---- [Game] hotkeys and the script editor key ----

static volatile UINT config_vk, speed_vk;
static volatile int speed_toggle = 1;   // SpeedMode: 1 Toggle, 0 Hold
static volatile float speed_factor = 3.0f;
static volatile int editor_scan;        // [Debug] ScriptEditorKey, 0 = off
static volatile int speed_on;          // fast mode active (the badge, modern/gamma.cpp)
static volatile int agent_speed_req;    // agent mode's speedkey: 1 = fast mode off, 2 = on

float modern_keys_speed_active()
{
    return speed_on ? speed_factor : 0.0f;
}

void modern_keys_agent_speed(bool on)
{
    agent_speed_req = on ? 2 : 1;
}

static UINT vk_from_ini(const char *key, const char *def)
{
    char name[32];
    modern_ini_str("Game", key, def, name, sizeof name);
    if (!name[0] || _stricmp(name, "none") == 0)
        return 0;
    int scan = KeyParse(name);
    UINT vk = scan > 0 ? MapVirtualKeyA((scan & 0x7f) | (scan & 0x80 ? 0xe000 : 0), 3 /* MAPVK_VSC_TO_VK_EX */) : 0;
    if (!vk)
        modern_log("hotkeys: unknown key \"%s\" for %s", name, key);
    return vk;
}

static void load_game_keys()
{
    UINT config = vk_from_ini("ConfigKey", "F10"), speed = vk_from_ini("SpeedKey", "F11");
    char f[16];
    modern_ini_str("Game", "SpeedFactor", "3", f, sizeof f);
    float factor = (float)atof(f);
    if (factor < 1.0f) factor = 1.0f;
    if (factor > 8.0f) factor = 8.0f;
    if (config != config_vk || speed != speed_vk || factor != speed_factor)
        modern_log("hotkeys: config key vk 0x%02X, speed key vk 0x%02X, speed x%.1f", config, speed, (double)factor);
    char mode[16];
    modern_ini_str("Game", "SpeedMode", "Toggle", mode, sizeof mode);
    int toggle = _stricmp(mode, "Hold") != 0;
    if (toggle != speed_toggle)
        modern_log("hotkeys: speed key mode %s", toggle ? "toggle" : "hold");
    char ed[32];
    modern_ini_str("Debug", "ScriptEditorKey", "", ed, sizeof ed);
    int scan = ed[0] && _stricmp(ed, "none") != 0 ? KeyParse(ed) : 0;
    if (scan < 0)
        scan = 0;
    if (scan != editor_scan)
        modern_log("hotkeys: script editor key %s", scan ? ed : "off");
    editor_scan = scan;
    config_vk = config;
    speed_vk = speed;
    speed_factor = factor;
    speed_toggle = toggle;
}

static void open_config_app()
{
    // AllowSetForegroundWindow(ASFW_ANY), looked up: VC6's headers and import library (the 32-bit clang build) lack it.
    typedef BOOL(WINAPI * allow_fn)(DWORD);
    allow_fn allow = (allow_fn)(void *)GetProcAddress(GetModuleHandleA("user32.dll"), "AllowSetForegroundWindow");
    if (allow)
        allow((DWORD)-1);
    HWND cfg = FindWindowA("SummonerFixConfig", NULL);
    if (cfg) {
        ShowWindow(cfg, SW_RESTORE);
        SetForegroundWindow(cfg);
        modern_log("config hotkey: brought SummonerFixConfig forward");
        return;
    }
    char exe[MAX_PATH];
    _snprintf(exe, sizeof exe, "%s\\SummonerFixConfig.exe", modern_dir());
    exe[sizeof exe - 1] = 0;
    STARTUPINFOA si;
    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    PROCESS_INFORMATION pi;
    if (CreateProcessA(exe, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        modern_log("config hotkey: started %s", exe);
    } else {
        modern_log("config hotkey: cannot start %s (error %lu)", exe, GetLastError());
    }
}

// Polled, so the hotkeys work whatever the game does with keys. Fast mode lowers frametime_divisor: FrameDone reports
// the frame time divided by it, and everything in the game advances by the reported frame time.
static DWORD WINAPI poll_hotkeys(LPVOID)
{
    bool config_was = false, speed_was = false, on = false;
    float applied = 1.0f;
    for (;;) {
        Sleep(15);
        HWND game = os_get_main_wnd();
        bool focused = game && GetForegroundWindow() == game;
        UINT cvk = config_vk, svk = speed_vk;
        bool config_down = cvk && (GetAsyncKeyState((int)cvk) & 0x8000);
        if (config_down && !config_was && focused)
            open_config_app();
        config_was = config_down;
        bool speed_down = focused && svk && (GetAsyncKeyState((int)svk) & 0x8000);
        if (speed_toggle) {
            if (speed_down && !speed_was)
                on = !on;
        } else {
            on = speed_down;
        }
        speed_was = speed_down;
        if (agent_speed_req) {
            on = agent_speed_req == 2;
            agent_speed_req = 0;
        }
        float want = on ? 1.0f / speed_factor : 1.0f;
        if (want != applied) {
            frametime_divisor = want;
            applied = want;
            modern_log("fast mode %s", on ? "on" : "off");
        }
        speed_on = on;
    }
    return 0;
}

// Re-reads the sections whenever a file in the exe's folder changes (the config app saving the ini).
static DWORD WINAPI watch_ini(LPVOID)
{
    HANDLE h = FindFirstChangeNotificationA(modern_dir(), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE);
    if (h == INVALID_HANDLE_VALUE) {
        modern_log("key bindings: cannot watch %s (live reload off)", modern_dir());
        return 0;
    }
    for (;;) {
        if (WaitForSingleObject(h, INFINITE) != WAIT_OBJECT_0)
            break;
        Sleep(150);   // let the writer finish
        load_bindings(false);
        load_game_keys();
        load_console_keys();
        if (!FindNextChangeNotification(h))
            break;
    }
    FindCloseChangeNotification(h);
    return 0;
}

// On the game thread (os_add_msg_handler): side buttons, keys bound to mouse buttons, console keys, the editor key.
static void keys_msg_handler(UINT msg, WPARAM wp, LPARAM lp)
{
    if ((msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) && !(lp & 0x40000000)) {
        int scan = (int)((lp >> 16) & 0xff) | ((lp & 0x01000000) ? 0x80 : 0);
        if (editor_scan && scan == editor_scan && gameseq_state_cur == 4) {
            game_flags ^= 0x80;
            modern_log("script editor %s", (game_flags & 0x80) ? "on" : "off");
        }
        run_console_key(scan);
    }
    switch (msg) {
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
        if (HIWORD(wp) == XBUTTON1 || HIWORD(wp) == XBUTTON2)
            mouse_event(HIWORD(wp) == XBUTTON1 ? 3 : 4, msg == WM_XBUTTONDOWN);
        break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP: {
        int scan = (int)((lp >> 16) & 0xff) | ((lp & 0x01000000) ? 0x80 : 0);
        int target = key_bind->keyMouse[scan];
        if (target < 0)
            break;
        bool down = msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN;
        if (down && (lp & 0x40000000))
            break;   // auto-repeat
        mouse_mark_button(target, down);
        break;
    }
    }
}

void modern_keys_init()
{
    if (!modern_host())
        return;
    for (int i = 0; i < 256; i++)
        key_physical[i] = (unsigned char)i;
    load_bindings(true);
    load_game_keys();
    load_console_keys();
    keys_on = true;
    os_add_msg_handler(keys_msg_handler);
    CloseHandle(CreateThread(NULL, 0, poll_hotkeys, NULL, 0, NULL));
    CloseHandle(CreateThread(NULL, 0, watch_ini, NULL, 0, NULL));
}

#endif
