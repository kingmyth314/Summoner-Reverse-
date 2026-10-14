// Agent mode ([Debug] Agent=1): lets a script play the game without the real mouse and keyboard.
// A thread polls <game>\agent\cmd.txt (written atomically by tools/agent.mjs), runs its commands and
// writes agent\result.txt. Input goes straight into the game (key_mark / mouse_mark_button, see keys.c),
// the cursor position replaces the real one (display.c ScreenToClient hook), and screenshots are taken
// from the back buffer. Coordinates are in screenshot pixels (half the window's client size).
// The game is kept "in the foreground" so it does not pause when another window has focus.
//
// Commands (one per line, ';' also separates):
//   key NAME [ms]      press and release a game key (names as in keynames.h, or 0xNN), default 80 ms
//   keydown NAME / keyup NAME
//   move X Y           set the cursor
//   click [left|right|middle] [X Y] [ms]   move (optional), press, hold (default 80 ms), release
//   down BUTTON / up BUTTON                mouse button 0-2 or left/right/middle
//   drag X1 Y1 X2 Y2   left-button drag
//   wait MS
//   capture NAME       screenshot to agent\NAME.bmp (waits until written)
//   info               game state, screen, fps, cursor
//   peek ADDR [N]      hex dump of game memory (N bytes, default 16)
//   quit               ask the game to exit (game state 3)
#include "summonerfix.h"
#include "keynames.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VA_GAMESEQ_STATE_CUR 0x5b27e0u
#define VA_INTERFACE_CUR_SCREEN 0x5bd814u
#define VA_FPS 0x5a5744u
#define VA_IS_MAIN_WND_ACTIVE 0x2cbff1cu
#define VA_GAMESEQ_SET_STATE 0x401020u

static char g_dir[MAX_PATH];
static char g_out[16384];
static size_t g_outLen;

static void Out(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = _vsnprintf(g_out + g_outLen, sizeof g_out - g_outLen - 2, fmt, ap);
    va_end(ap);
    if (n > 0)
        g_outLen += (size_t)n;
    g_out[g_outLen++] = '\n';
    g_out[g_outLen] = 0;
}

static void Path(char *out, const char *name)
{
    _snprintf(out, MAX_PATH, "%s\\%s", g_dir, name);
    out[MAX_PATH - 1] = 0;
}

static int Button(const char *s)
{
    if (!s || !*s) return 0;
    if (!_stricmp(s, "left")) return 0;
    if (!_stricmp(s, "right")) return 1;
    if (!_stricmp(s, "middle")) return 2;
    return atoi(s);
}

static void Move(int x, int y) { DisplaySetAgentCursor(x * 2, y * 2); }

static void Capture(const char *name)
{
    char path[MAX_PATH], file[MAX_PATH];
    _snprintf(file, sizeof file, "%s.bmp", name);
    file[sizeof file - 1] = 0;
    Path(path, file);
    DisplayRequestCapture(path);
    for (int i = 0; i < 200 && DisplayCapturePending(); i++)
        Sleep(10);
    Out(DisplayCapturePending() ? "capture %s: timed out (is the game drawing?)" : "capture %s: ok", file);
}

static void Info(void)
{
    int cx, cy;
    DisplayGetAgentCursor(&cx, &cy);
    Out("state=%d screen=%d fps=%.1f foreground=%d cursor=%d,%d",
        *(volatile int *)(UINT_PTR)SumVa(VA_GAMESEQ_STATE_CUR), *(volatile int *)(UINT_PTR)SumVa(VA_INTERFACE_CUR_SCREEN),
        (double)*(volatile float *)(UINT_PTR)SumVa(VA_FPS), *(volatile BYTE *)(UINT_PTR)SumVa(VA_IS_MAIN_WND_ACTIVE),
        cx / 2, cy / 2);
}

static void Peek(DWORD va, int n)
{
    if (n <= 0 || n > 256) n = 16;
    char line[3 * 256 + 32];
    int len = _snprintf(line, sizeof line, "%06lX:", va);
    const BYTE *p = (const BYTE *)(UINT_PTR)SumVa(va);
    if (IsBadReadPtr(p, (UINT_PTR)n)) {
        Out("peek %06lX: unreadable", va);
        return;
    }
    for (int i = 0; i < n; i++)
        len += _snprintf(line + len, sizeof line - (size_t)len, " %02X", p[i]);
    Out("%s", line);
}

static void RunLine(char *line)
{
    char *argv[8];
    int argc = 0;
    for (char *t = strtok(line, " \t\r\n"); t && argc < 8; t = strtok(NULL, " \t\r\n"))
        argv[argc++] = t;
    if (!argc || argv[0][0] == '#')
        return;
    const char *c = argv[0];
    if (!_stricmp(c, "key") || !_stricmp(c, "keydown") || !_stricmp(c, "keyup")) {
        int code = argc > 1 ? KeyParse(argv[1]) : -1;
        if (code < 0 || code >= 256) {
            Out("%s: unknown key", c);
            return;
        }
        if (!_stricmp(c, "keyup")) {
            KeysInjectKey(code, 0);
        } else {
            KeysInjectKey(code, 1);
            if (!_stricmp(c, "key")) {
                Sleep(argc > 2 ? (DWORD)atoi(argv[2]) : 80);
                KeysInjectKey(code, 0);
            }
        }
        Out("%s %s: ok", c, argv[1]);
    } else if (!_stricmp(c, "postkey") && argc > 1) {
        // A real WM_KEYDOWN/WM_KEYUP through the game's window procedure (unlike "key", which calls
        // key_mark directly), so window-message handlers such as [ConsoleKeys] see it.
        int code = KeyParse(argv[1]);
        HWND wnd = *(volatile HWND *)(UINT_PTR)SumVa(0x2cbff10);
        if (code < 0 || code >= 256 || !wnd) {
            Out("postkey: unknown key or no window");
            return;
        }
        UINT vk = MapVirtualKeyA((code & 0x7f) | (code & 0x80 ? 0xe000 : 0), 3 /* MAPVK_VSC_TO_VK_EX */);
        LPARAM lp = 1 | ((LPARAM)(code & 0x7f) << 16) | (code & 0x80 ? 0x01000000 : 0);
        PostMessageA(wnd, WM_KEYDOWN, vk, lp);
        Sleep(argc > 2 ? (DWORD)atoi(argv[2]) : 80);
        PostMessageA(wnd, WM_KEYUP, vk, lp | 0xC0000000);
        Out("postkey %s: ok (vk 0x%02X)", argv[1], vk);
    } else if (!_stricmp(c, "move") && argc >= 3) {
        Move(atoi(argv[1]), atoi(argv[2]));
        Out("move: ok");
    } else if (!_stricmp(c, "click")) {
        int a = 1, b = 0;
        if (argc > 1 && (argv[1][0] < '0' || argv[1][0] > '9')) {
            b = Button(argv[1]);
            a = 2;
        }
        if (argc >= a + 2) {
            Move(atoi(argv[a]), atoi(argv[a + 1]));
            Sleep(60); // let the game see the new position for a frame or two
            a += 2;
        }
        KeysInjectMouse(b, 1);
        Sleep(argc > a ? (DWORD)atoi(argv[a]) : 80);
        KeysInjectMouse(b, 0);
        Out("click: ok");
    } else if ((!_stricmp(c, "down") || !_stricmp(c, "up")) && argc >= 2) {
        KeysInjectMouse(Button(argv[1]), !_stricmp(c, "down"));
        Out("%s: ok", c);
    } else if (!_stricmp(c, "drag") && argc >= 5) {
        Move(atoi(argv[1]), atoi(argv[2]));
        Sleep(60);
        KeysInjectMouse(0, 1);
        Sleep(80);
        for (int i = 1; i <= 10; i++) {
            Move(atoi(argv[1]) + (atoi(argv[3]) - atoi(argv[1])) * i / 10, atoi(argv[2]) + (atoi(argv[4]) - atoi(argv[2])) * i / 10);
            Sleep(30);
        }
        Sleep(60);
        KeysInjectMouse(0, 0);
        Out("drag: ok");
    } else if (!_stricmp(c, "wait") && argc >= 2) {
        Sleep((DWORD)atoi(argv[1]));
    } else if (!_stricmp(c, "capture") && argc >= 2) {
        Capture(argv[1]);
    } else if (!_stricmp(c, "info")) {
        Info();
    } else if (!_stricmp(c, "peek") && argc >= 2) {
        Peek((DWORD)strtoul(argv[1], NULL, 16), argc > 2 ? atoi(argv[2]) : 16);
    } else if (!_stricmp(c, "quit")) {
        ((void(__cdecl *)(int))(UINT_PTR)SumVa(VA_GAMESEQ_SET_STATE))(3);
        Out("quit: requested");
    } else {
        Out("unknown command: %s", c);
    }
}

static void WriteResult(void)
{
    char tmp[MAX_PATH], dst[MAX_PATH];
    Path(tmp, "result.tmp");
    Path(dst, "result.txt");
    HANDLE f = CreateFileA(tmp, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE)
        return;
    DWORD wr;
    WriteFile(f, g_out, (DWORD)g_outLen, &wr, NULL);
    CloseHandle(f);
    MoveFileExA(tmp, dst, MOVEFILE_REPLACE_EXISTING);
}

static DWORD WINAPI AgentThread(LPVOID unused)
{
    char cmd[MAX_PATH], run[MAX_PATH], ready[MAX_PATH];
    Path(cmd, "cmd.txt");
    Path(run, "cmd.run");
    Path(ready, "ready.txt");
    HANDLE r = CreateFileA(ready, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (r != INVALID_HANDLE_VALUE)
        CloseHandle(r);
    for (;;) {
        Sleep(20);
        *(volatile BYTE *)(UINT_PTR)SumVa(VA_IS_MAIN_WND_ACTIVE) = 1; // never pause for lost focus
        if (!MoveFileExA(cmd, run, MOVEFILE_REPLACE_EXISTING))
            continue;
        static char text[16384];
        HANDLE f = CreateFileA(run, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        DWORD n = 0;
        if (f != INVALID_HANDLE_VALUE) {
            ReadFile(f, text, sizeof text - 1, &n, NULL);
            CloseHandle(f);
        }
        text[n] = 0;
        DeleteFileA(run);
        g_outLen = 0;
        g_out[0] = 0;
        for (char *p = text; *p;) {
            char *end = p + strcspn(p, ";\n");
            char save = *end;
            *end = 0;
            char line[512];
            lstrcpynA(line, p, sizeof line);
            RunLine(line);
            if (!save)
                break;
            p = end + 1;
        }
        Out("done");
        WriteResult();
    }
    return 0;
}

void AgentInstall(void)
{
    if (!GetPrivateProfileIntA("Debug", "Agent", 0, g_cfg.iniPath))
        return;
    lstrcpynA(g_dir, g_cfg.iniPath, MAX_PATH);
    char *slash = strrchr(g_dir, '\\');
    strcpy(slash ? slash + 1 : g_dir, "agent");
    CreateDirectoryA(g_dir, NULL);
    char ready[MAX_PATH];
    Path(ready, "ready.txt");
    DeleteFileA(ready);
    CloseHandle(CreateThread(NULL, 0, AgentThread, NULL, 0, NULL));
    Log("agent mode: commands from %s\\cmd.txt", g_dir);
}
