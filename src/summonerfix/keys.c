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
static unsigned char g_maps[2][256];
static unsigned char *volatile g_map = g_maps[0]; // table in use; the other one is filled on reload
static char g_lastSection[8192];

static int __cdecl KeyRemap(int code)
{
    int c = g_layout(code);
    if (c >= 0 && c < 256)
        c = g_map[c];
    return c;
}

static void LogBad(const char *line)
{
    Log("key bindings: ignoring \"%s\" (unknown key name)", line);
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

    unsigned char *next = g_map == g_maps[0] ? g_maps[1] : g_maps[0];
    int count = KeyParseSection(section, next, LogBad);
    g_map = next; // single pointer store: KeyRemap sees either the old or the new table
    char a[8], b[8];
    for (int i = 0; i < 256; i++)
        if (next[i] != i)
            Log("key bindings: %s acts as %s", KeyNameOf(i, a, sizeof a), KeyNameOf(next[i], b, sizeof b));
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
        if (!FindNextChangeNotification(h))
            break;
    }
    FindCloseChangeNotification(h);
    return 0;
}

void KeyBindInstall(void)
{
    LoadBindings(TRUE);

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
}
