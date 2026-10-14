// Custom key bindings with live reload.
// Every key event goes through key_mark (0x5093a0), which first calls key_remap_layout (0x509560) to
// swap scan codes for French/German keyboards (see docs/sum-exe/controls.md). We redirect that one call
// to KeyRemap: the original layout swap runs first, then our table from [KeyBindings] in summonerfix.ini.
// The result feeds both the key queue (the game's hard-coded key map) and key_is_down/key_down_time.
// A watcher thread reloads the table whenever summonerfix.ini changes (e.g. saved by SummonerFixConfig.exe),
// so bindings can be changed while the game runs.
#include "summonerfix.h"
#include "keynames.h"

#define VA_KEY_MARK_REMAP_CALL 0x5093a6u // CALL key_remap_layout inside key_mark
#define VA_KEY_REMAP_LAYOUT 0x509560u

typedef int(__cdecl *PFN_KeyRemapLayout)(int code);
static PFN_KeyRemapLayout g_layout;
static Bindings g_sets[2];
static Bindings *volatile g_bind = &g_sets[0]; // set in use; the other one is filled on reload
static volatile int g_injecting;               // KeyInject in progress: pass codes through unchanged
static char g_lastSection[8192];
static void LoadGameKeys(void);

// Text entry (save names, console, chat, input boxes) turns queued key codes into characters with
// key_to_ascii (0x509ca0). The queued codes are already remapped, so typing E with E=I bound gave "i".
// KeyRemap remembers which physical key produced each game key code, and our key_to_ascii replacement
// translates back before the character lookup, so text gets the keys actually pressed.
#define VA_KEY_TO_ASCII 0x509ca0u
#define VA_KEY_INITED 0x25daa74u
#define VA_ASCII_TABLE 0x5a64c0u
#define VA_ASCII_TABLE_SHIFTED 0x5a66c0u
static volatile unsigned char g_physical[256]; // game key code -> physical key that last produced it

static int __cdecl KeyRemap(int code)
{
    if (g_injecting)
        return code; // already a game key code (from a mouse binding)
    int c = g_layout(code);
    if (c >= 0 && c < 256) {
        int p = c;
        c = g_bind->map[p];
        g_physical[c] = (unsigned char)p;
    }
    return c;
}

// ---- Mouse bindings ---------------------------------------------------------------------------------
// The game's mouse message handler (0x500f60) turns WM_L/R/MBUTTONDOWN/UP into mouse_mark_button(button,
// down) (0x501100) at six call sites; we redirect them to MouseMark. Side buttons (WM_XBUTTON*) are ignored
// by the game, and keys bound to mouse buttons need their WM_KEYDOWN/UP, so we also register our own window
// message handler with os_add_msg_handler (0x530c20). All of this runs on the game's main thread.
#define VA_MOUSE_MARK_BUTTON 0x501100u
#define VA_KEY_MARK 0x5093a0u
#define VA_OS_ADD_MSG_HANDLER 0x530c20u
static const DWORD k_mouseCalls[6] = {0x500f79, 0x500f86, 0x500f93, 0x500fa0, 0x500fad, 0x500fba};

typedef void(__cdecl *PFN_MouseMark)(int button, char down);
typedef void(__cdecl *PFN_KeyMark)(int code, int down, int latency);
typedef void(__cdecl *PFN_MsgHandler)(UINT msg, WPARAM wp, LPARAM lp);
typedef void(__cdecl *PFN_AddMsgHandler)(PFN_MsgHandler);
static PFN_MouseMark g_mouseMark;
static PFN_KeyMark g_keyMark;

static void KeyInject(int code, int down)
{
    g_injecting = 1;
    g_keyMark(code, down, 0);
    g_injecting = 0;
}

// Agent mode (agent.c): press game keys / mouse buttons directly (no remapping; thread-safe in the game).
void KeysInjectKey(int code, int down)
{
    if (g_keyMark)
        KeyInject(code, down);
}

void KeysInjectMouse(int button, int down)
{
    if (g_mouseMark)
        g_mouseMark(button, (char)down);
}

// A mouse button (0-4) went down/up: apply its binding, or the default for left/right/middle.
static void MouseEvent(int button, int down)
{
    int to = g_bind->mouseTo[button];
    if (to < 0) {
        if (button < 3)
            g_mouseMark(button, (char)down);
    } else if (to >= KEY_MOUSE_BASE) {
        g_mouseMark(to - KEY_MOUSE_BASE, (char)down);
    } else {
        KeyInject(to, down);
    }
}

static void __cdecl MouseMark(int button, char down)
{
    MouseEvent(button, down != 0);
}

// ---- Console-command keys -----------------------------------------------------------------------------
// [ConsoleKeys] Key=command (e.g. F9=add_gold 1000, F=show_framerate) runs a developer console command when
// the key is pressed, through the game's execute_command_line (0x50a210, Summoner 2 name). It runs from
// MsgHandler, so on the game thread. Double-buffered like the bindings so the ini can be reloaded live.
#define VA_EXECUTE_COMMAND_LINE 0x50a210u
#define CONSOLE_CMD_LEN 96
typedef int(__cdecl *PFN_ExecuteCommandLine)(const char *line);
typedef struct {
    char cmd[256][CONSOLE_CMD_LEN];
} ConsoleKeys;
static ConsoleKeys g_ckSets[2];
static ConsoleKeys *volatile g_ck = &g_ckSets[0];

static void LoadConsoleKeys(void)
{
    ConsoleKeys *next = g_ck == &g_ckSets[0] ? &g_ckSets[1] : &g_ckSets[0];
    ZeroMemory(next, sizeof *next);
    static char buf[8192];
    DWORD n = GetPrivateProfileSectionA("ConsoleKeys", buf, sizeof buf, g_cfg.iniPath);
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
            Log("console keys: ignored \"%s\"", line);
            continue;
        }
        lstrcpynA(next->cmd[scan], eq + 1, CONSOLE_CMD_LEN);
        count++;
    }
    // Log only on a change: the log file is in the watched folder, so logging on every reload would
    // trigger the next reload.
    BOOL changed = memcmp(next, g_ck, sizeof *next) != 0;
    g_ck = next;
    if (changed)
        Log("console keys: %d command key(s)", count);
}

static void RunConsoleKey(int scan)
{
    const char *cmd = g_ck->cmd[scan & 0xff];
    if (!cmd[0])
        return;
    char line[CONSOLE_CMD_LEN];
    lstrcpynA(line, cmd, sizeof line); // the parser works on its own copy
    Log("console key: %s", line);
    ((PFN_ExecuteCommandLine)(UINT_PTR)SumVa(VA_EXECUTE_COMMAND_LINE))(line);
}

static void __cdecl MsgHandler(UINT msg, WPARAM wp, LPARAM lp)
{
    if ((msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) && !(lp & 0x40000000))
        RunConsoleKey((int)((lp >> 16) & 0xff) | ((lp & 0x01000000) ? 0x80 : 0));
    switch (msg) {
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
        if (HIWORD(wp) == XBUTTON1 || HIWORD(wp) == XBUTTON2)
            MouseEvent(HIWORD(wp) == XBUTTON1 ? 3 : 4, msg == WM_XBUTTONDOWN);
        break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP: {
        int scan = (int)((lp >> 16) & 0xff) | ((lp & 0x01000000) ? 0x80 : 0);
        int target = g_bind->keyMouse[scan];
        if (target < 0)
            break;
        int down = msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN;
        if (down && (lp & 0x40000000))
            break; // auto-repeat
        g_mouseMark(target, (char)down);
        break;
    }
    }
}

static void MouseBindInstall(void)
{
    g_mouseMark = (PFN_MouseMark)(UINT_PTR)SumVa(VA_MOUSE_MARK_BUTTON);
    g_keyMark = (PFN_KeyMark)(UINT_PTR)SumVa(VA_KEY_MARK);
    for (int i = 0; i < 6; i++) {
        DWORD site = SumVa(k_mouseCalls[i]);
        BYTE orig[5] = {0xE8}, repl[5] = {0xE8};
        DWORD rel = SumVa(VA_MOUSE_MARK_BUTTON) - (site + 5);
        memcpy(orig + 1, &rel, 4);
        rel = (DWORD)(UINT_PTR)MouseMark - (site + 5);
        memcpy(repl + 1, &rel, 4);
        PatchBytes(k_mouseCalls[i], orig, repl, 5, "mouse button -> MouseMark (mouse bindings)");
    }
    ((PFN_AddMsgHandler)(UINT_PTR)SumVa(VA_OS_ADD_MSG_HANDLER))(MsgHandler);
}

static int __cdecl KeyToAscii(int code)
{
    if (!*(volatile char *)(UINT_PTR)SumVa(VA_KEY_INITED))
        return 0xff;
    int c = g_physical[code & 0xff];
    if (c >= 0x7f)
        return 0xff;
    const int *table = (const int *)(UINT_PTR)SumVa((code & 0x1000) ? VA_ASCII_TABLE_SHIFTED : VA_ASCII_TABLE);
    return table[c];
}

static void LogBad(const char *line)
{
    Log("key bindings: ignoring \"%s\" (unknown key name or unsupported combination)", line);
}

// Re-read [KeyBindings]; returns FALSE if the section text did not change.
static BOOL LoadBindings(BOOL initial)
{
    char section[sizeof g_lastSection];
    DWORD len = GetPrivateProfileSectionA("KeyBindings", section, sizeof section, g_cfg.iniPath);
    if (len == 0)
        section[0] = section[1] = 0;
    if (!initial && memcmp(section, g_lastSection, len + 2) == 0)
        return FALSE;
    memcpy(g_lastSection, section, len + 2);

    Bindings *next = g_bind == &g_sets[0] ? &g_sets[1] : &g_sets[0];
    int count = KeyParseSection(section, next, LogBad);
    g_bind = next; // single pointer store: readers see either the old or the new set
    char a[8], b[8];
    for (int i = 0; i < 256; i++) {
        if (next->keyMouse[i] >= 0)
            Log("key bindings: %s acts as %s", KeyNameOf(i, a, sizeof a), KeyNameOf(KEY_MOUSE_BASE + next->keyMouse[i], b, sizeof b));
        else if (next->map[i] != i)
            Log("key bindings: %s acts as %s", KeyNameOf(i, a, sizeof a), KeyNameOf(next->map[i], b, sizeof b));
    }
    for (int i = 0; i < KEY_MOUSE_COUNT; i++)
        if (next->mouseTo[i] >= 0)
            Log("key bindings: %s acts as %s", KeyNameOf(KEY_MOUSE_BASE + i, a, sizeof a), KeyNameOf(next->mouseTo[i], b, sizeof b));
    Log("key bindings: %d %s", count, initial ? "loaded" : "reloaded");
    return TRUE;
}

static DWORD WINAPI WatchIni(LPVOID unused)
{
    char dir[MAX_PATH];
    strcpy(dir, g_cfg.iniPath);
    char *slash = strrchr(dir, '\\');
    if (slash)
        *slash = 0;
    HANDLE h = FindFirstChangeNotificationA(dir, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE);
    if (h == INVALID_HANDLE_VALUE) {
        Log("key bindings: cannot watch %s (live reload off)", dir);
        return 0;
    }
    for (;;) {
        if (WaitForSingleObject(h, INFINITE) != WAIT_OBJECT_0)
            break;
        Sleep(150); // let the writer finish
        LoadBindings(FALSE);
        LoadGameKeys();
        LoadConsoleKeys();
        if (!FindNextChangeNotification(h))
            break;
    }
    FindCloseChangeNotification(h);
    return 0;
}

// In-game hotkeys from [Game], polled (so they work whatever the game does with keys) and re-read on reload:
//   ConfigKey (default F10): opens SummonerFixConfig.exe, or brings it forward.
//   SpeedKey (default F11): with SpeedMode=Toggle (default) each press turns fast mode on or off; with
//   SpeedMode=Hold it is on while held. In fast mode the game runs SpeedFactor (default 3) times faster. FrameDone (0x501510)
//   reports frame time / frametime_divisor (0x5a5748, normally 1.0), and everything in the game advances by
//   reported_frametime, so lowering the divisor speeds up the whole game consistently.
#define VA_MAIN_WND 0x2cbff10u
#define VA_FRAMETIME_DIVISOR 0x5a5748u

static volatile UINT g_configVk, g_speedVk;
static volatile int g_agentSpeedReq; // agent mode: 1 = switch fast mode off, 2 = on
static volatile int g_speedToggle = 1; // SpeedMode: 1 Toggle, 0 Hold
static volatile int g_speedOn;         // fast mode active (read by the on-screen indicator)

void KeysAgentSpeed(int on)
{
    g_agentSpeedReq = on ? 2 : 1;
}
static volatile float g_speedFactor = 3.0f;

float KeysSpeedActive(void)
{
    return g_speedOn ? g_speedFactor : 0.0f;
}

static UINT VkFromIni(const char *key, const char *def)
{
    char name[32];
    GetPrivateProfileStringA("Game", key, def, name, sizeof name, g_cfg.iniPath);
    if (!name[0] || _stricmp(name, "none") == 0)
        return 0;
    int scan = KeyParse(name);
    UINT vk = scan > 0 ? MapVirtualKeyA((scan & 0x7f) | (scan & 0x80 ? 0xe000 : 0), 3 /* MAPVK_VSC_TO_VK_EX */) : 0;
    if (!vk)
        Log("hotkeys: unknown key \"%s\" for %s", name, key);
    return vk;
}

static void LoadGameKeys(void)
{
    UINT config = VkFromIni("ConfigKey", "F10"), speed = VkFromIni("SpeedKey", "F11");
    char f[16];
    GetPrivateProfileStringA("Game", "SpeedFactor", "3", f, sizeof f, g_cfg.iniPath);
    float factor = (float)atof(f);
    if (factor < 1.0f) factor = 1.0f;
    if (factor > 8.0f) factor = 8.0f;
    if (config != g_configVk || speed != g_speedVk || factor != g_speedFactor)
        Log("hotkeys: config key vk 0x%02X, speed key vk 0x%02X, speed x%.1f", config, speed, factor);
    char mode[16];
    GetPrivateProfileStringA("Game", "SpeedMode", "Toggle", mode, sizeof mode, g_cfg.iniPath);
    int toggle = _stricmp(mode, "Hold") != 0;
    if (toggle != g_speedToggle)
        Log("hotkeys: speed key mode %s", toggle ? "toggle" : "hold");
    g_configVk = config;
    g_speedVk = speed;
    g_speedFactor = factor;
    g_speedToggle = toggle;
}

static void OpenConfigApp(void)
{
    AllowSetForegroundWindow(ASFW_ANY);
    HWND cfg = FindWindowA("SummonerFixConfig", NULL);
    if (cfg) {
        ShowWindow(cfg, SW_RESTORE);
        SetForegroundWindow(cfg);
        Log("config hotkey: brought SummonerFixConfig forward");
        return;
    }
    char exe[MAX_PATH];
    strcpy(exe, g_cfg.iniPath);
    char *slash = strrchr(exe, '\\');
    strcpy(slash ? slash + 1 : exe, "SummonerFixConfig.exe");
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof si);
    si.cb = sizeof si;
    PROCESS_INFORMATION pi;
    if (CreateProcessA(exe, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        Log("config hotkey: started %s", exe);
    } else {
        Log("config hotkey: cannot start %s (error %lu)", exe, GetLastError());
    }
}

static DWORD WINAPI PollHotkeys(LPVOID unused)
{
    volatile float *divisor = (volatile float *)(UINT_PTR)SumVa(VA_FRAMETIME_DIVISOR);
    BOOL configWas = FALSE, speedWas = FALSE, on = FALSE;
    float applied = 1.0f;
    for (;;) {
        Sleep(15);
        HWND game = *(volatile HWND *)(UINT_PTR)SumVa(VA_MAIN_WND);
        BOOL focused = game && GetForegroundWindow() == game;
        UINT cvk = g_configVk, svk = g_speedVk;

        BOOL configDown = cvk && (GetAsyncKeyState((int)cvk) & 0x8000);
        if (configDown && !configWas && focused)
            OpenConfigApp();
        configWas = configDown;

        BOOL speedDown = focused && svk && (GetAsyncKeyState((int)svk) & 0x8000);
        if (g_speedToggle) {
            if (speedDown && !speedWas)
                on = !on;
        } else {
            on = speedDown;
        }
        speedWas = speedDown;
        if (g_agentSpeedReq) {
            on = g_agentSpeedReq == 2;
            g_agentSpeedReq = 0;
        }
        float want = on ? 1.0f / g_speedFactor : 1.0f;
        if (want != applied) {
            *divisor = want;
            applied = want;
            Log("fast mode %s", on ? "on" : "off");
        }
        g_speedOn = on;
    }
    return 0;
}

void KeyBindInstall(void)
{
    LoadBindings(TRUE);
    LoadGameKeys();
    LoadConsoleKeys();
    CloseHandle(CreateThread(NULL, 0, PollHotkeys, NULL, 0, NULL));

    // Always hooked (an identity table changes nothing), so bindings added later take effect live.
    g_layout = (PFN_KeyRemapLayout)(UINT_PTR)SumVa(VA_KEY_REMAP_LAYOUT);
    DWORD site = SumVa(VA_KEY_MARK_REMAP_CALL);
    BYTE orig[5] = {0xE8}, repl[5] = {0xE8};
    DWORD rel = SumVa(VA_KEY_REMAP_LAYOUT) - (site + 5);
    memcpy(orig + 1, &rel, 4);
    rel = (DWORD)(UINT_PTR)KeyRemap - (site + 5);
    memcpy(repl + 1, &rel, 4);
    if (PatchBytes(VA_KEY_MARK_REMAP_CALL, orig, repl, 5, "key_mark -> KeyRemap (custom key bindings)"))
        CloseHandle(CreateThread(NULL, 0, WatchIni, NULL, 0, NULL));

    MouseBindInstall();

    // key_to_ascii: replace its first instruction (MOV AL, [key_inited], 5 bytes) with JMP KeyToAscii.
    for (int i = 0; i < 256; i++)
        g_physical[i] = (unsigned char)i;
    static const BYTE kOrig[5] = {0xA0, 0x74, 0xAA, 0x5D, 0x02};
    BYTE jmp[5] = {0xE9};
    rel = (DWORD)(UINT_PTR)KeyToAscii - (SumVa(VA_KEY_TO_ASCII) + 5);
    memcpy(jmp + 1, &rel, 4);
    PatchBytes(VA_KEY_TO_ASCII, kOrig, jmp, 5, "key_to_ascii -> KeyToAscii (typing ignores key bindings)");
}
