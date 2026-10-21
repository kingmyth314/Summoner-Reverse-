// SummonerFixConfig - edits the [KeyBindings] section and the optional [Game] settings of summonerfix.ini
// next to this program.
// Pick an action, click "Change key..." and press the key you want for it. Changes are saved at once;
// a running Sum.exe with SummonerFix reloads them live. See docs/summonerfix/index.md.
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include "../summonerfix/keynames.h"
#include "../summonerfix/padmap.h"

// The game's hard-coded key map (docs/sum-exe/controls.md). gameKey is the code the game reacts to.
typedef struct { unsigned short gameKey; const char *action; } Action;
static const Action k_actions[] = {
    {0x01, "Menu"},
    {0x02, "Quick slot 1"}, {0x03, "Quick slot 2"}, {0x04, "Quick slot 3"},
    {0x05, "Quick slot 4"}, {0x06, "Quick slot 5"}, {0x07, "Quick slot 6"},
    {0x0f, "Next party member (Shift: previous)"},
    {0x17, "Inventory"}, {0x13, "Status"}, {0x1f, "Skills"}, {0x2e, "Spells"},
    {0x32, "Map"}, {0x2d, "Interact with nearest"}, {0x39, "Pause"}, {0x3b, "Help"},
    {0x3f, "Select party member 1"}, {0x40, "Select party member 2"}, {0x41, "Select party member 3"},
    {0x42, "Select party member 4"}, {0x43, "Select party member 5"},
    {0x58, "Camera mode"},
    {0xcb, "Turn camera left"}, {0xcd, "Turn camera right"}, {0xc8, "Tilt camera up"}, {0xd0, "Tilt camera down"},
    {0x23, "H key action"}, {0x3c, "Options"},
    // Chain attacks: chain_attack_select (0x43c830) checks these keys while held when a chain is possible.
    // Binding a key here makes it act as that modifier everywhere (e.g. Shift+Tab, Ctrl+click).
    {0x2a, "Chain attack slot 1 (hold; also Shift)"},
    {0x1d, "Chain attack slot 2 (hold; also Ctrl)"},
    {0x38, "Chain attack slot 3 (hold; also Alt)"},
    {0x3a, "Chain attack slot 0 (hold)"},
    // Mouse buttons (KEY_MOUSE_BASE + 0/1/2). Right click starts chain attacks.
    {0x100, "Left click (move / attack)"},
    {0x101, "Right click (chain attack)"},
    {0x102, "Middle click (interact)"},
};
#define ACTION_COUNT (int)(sizeof k_actions / sizeof k_actions[0])

// SummonerFix's own hotkeys, stored as key names in [Game] (not remappings). Listed after the actions.
typedef struct { const char *iniKey, *def, *action; } Hotkey;
static const Hotkey k_hotkeys[] = {
    {"SpeedKey", "F11", "Fast mode on/off (or hold)  [SummonerFix]"},
    {"ConfigKey", "F10", "Open this window while playing  [SummonerFix]"},
};
#define HOTKEY_COUNT (int)(sizeof k_hotkeys / sizeof k_hotkeys[0])
#define ROW_COUNT (ACTION_COUNT + HOTKEY_COUNT)

enum { ID_LIST = 100, ID_CHANGE, ID_CLEAR, ID_RESET, ID_CLOSE, ID_STATUS, ID_SKIPMOVIES, ID_ZOOM, ID_INTERACT, ID_CONTAINER, ID_APPLY, ID_SPEEDTOGGLE,
       ID_CONSOLE_CMD, ID_CONSOLE_BIND, ID_FIXALL, ID_FIXLIST, ID_FIXSAVE, ID_FIXCLOSE, ID_TENTACLE, ID_TENTCHANCE,
       ID_PADOPEN, ID_PADENABLED, ID_PADSAVE, ID_PADRESET, ID_PADCLOSE };

// [ConsoleKeys] Key=command entries (keys that run a developer console command). Shown as list rows after the
// hotkeys; Change key / Clear work on them, and "Bind key..." adds one for the command typed in the box.
#define MAX_CONSOLE_KEYS 64
#define NEW_CONSOLE_ROW 100000 // g_capturing value while waiting for the key of a new console binding
typedef struct { char key[32]; char cmd[96]; } ConsoleKey;
static ConsoleKey g_ck[MAX_CONSOLE_KEYS];
static int g_ckCount;
static char g_pendingCmd[96];

// Optional [Game] settings (src/summonerfix/gameopts.c), read once at game start. Empty = the game's own value.
typedef struct { int id; const char *iniKey, *label, *hint; } NumOption;
static const NumOption k_numOptions[] = {
    {ID_ZOOM, "CameraZoomMax", "Camera zoom-out limit:", "(standard game is 1)"},
    {ID_INTERACT, "InteractRange", "Talk / use range:", "(standard game is 5)"},
    {ID_CONTAINER, "ContainerRange", "Container open range:", "(standard game is 5)"},
};
#define NUM_OPTION_COUNT (int)(sizeof k_numOptions / sizeof k_numOptions[0])

// [Fixes] Key=0|1 (src/summonerfix/bugfix.c FixOn): fixes for original-game bugs, default on, read at game start.
// [Fixes] Enabled=0 turns them all off at once (the main window's checkbox); the second window (Choose fixes...)
// switches single fixes.
#define ID_FIX0 300
typedef struct { const char *iniKey, *label; } FixOption;
static const FixOption k_fixes[] = {
    {"RobeOfRestoration", "Robe of Restoration heals its wearer"},
    {"WorldSoundVolume", "World sounds follow the volume slider"},
    {"OpenGLTextures", "Full textures on a fresh install"},
    {"MetalHitEffects", "Hit effects on metal golems"},
    {"PopupRowOffset", "Popup rows light up under the cursor"},
    {"SaveOverwriteQuestion", "Ask before overwriting a save"},
    {"PhoenixInferno", "Phoenix Inferno hits each member once"},
    {"LuminarBeamTarget", "No Luminar beam crash"},
    {"SummonStashNoMove", "No Mist Elemental crash at cutscenes"},
    {"SummonKeepsLeader", "Control stays when a summon goes"},
    {"SpellRecoveryTimes", "Spell cooldowns from spells.tbl"},
    {"GoldPileAmount", "Gold piles keep their amount on load"},
    {"MessageBoxPause", "Warnings keep the game paused"},
    {"CanisTenGold", "Canis takes 10 gold, as offered"},
    {"AiBlessCount", "AI healer counts members once"},
    {"AreaCasterGone", "Fire walls burn after a caster goes"},
    {"SupportAiTarget", "Support AI keeps its chosen enemy"},
    {"DetectionCone", "Summons notice enemies ahead"},
    {"LeechDrain", "Leech drains HP"},
    {"ConversationHover", "Conversation more/end hover"},
    {"SaveListThumbnail", "Scrolled save list pictures"},
    {"ZaneEncounter", "Zane's encounters as intended"},
    {"MountedRiders", "AI sees mounted Phoenix / Ghost"},
    {"BossMusic", "Boss music starts and ends"},
    {"RingSixtyThousand", "Ring summons from 60,000 power"},
    {"MapLabelLimit", "33rd map label is refused"},
    {"ShopCategoryButtons", "Shop sell list category buttons"},
    {"InventoryCategoryButtons", "Identify list category buttons"},
    {"AmbientSounds", "Ambient sounds survive level changes"},
    {"PickupScrollbar", "Container scrollbar stays in its track"},
    {"EdgeTurnAfterDrag", "No edge turn right after a drag"},
    {"LootHighlightColour", "Loot flashes pale gold, not yellow"},
    {"KnockDownMessage", "No \"Knocked Down!\" while casting"},
    {"LongInstallPath", "Game folder with a long path starts"},
    {"EffectSlotSpell", "AI fireball not blocked by old effects"},
    {"AreaSpellDistances", "Enemy area spells use real distances"},
    {"SpellHitSoundPosition", "Spell hit sounds where spells land"},
    {"MapLabelsFull", "Map label edits saved at 128 labels"},
    {"SupportSpellPick", "AI healers heal when they should"},
    {"PrimarySoundBuffer", "No crash when 3D sound fails"},
    {"QpcFrequency", "Normal speed with a 4+ GHz timer"},
    {"ScreenshotNumbers", "Screenshots never overwrite each other"},
    {"EffectSearchGuard", "No crash in the Masad tutorial step"},
    {"BossAnchorsAlive", "Murod leaps beside living heroes only"},
    {"MurodBackBiteTimer", "Murod Oni back bite on its 5 s timer"},
    {"BossSpellInRange", "Boss poison/fear only when in reach"},
    {"SerpentSleepTest", "Serpent checks its new target asleep"},
};
#define FIX_COUNT (int)(sizeof k_fixes / sizeof k_fixes[0])

static char g_ini[MAX_PATH];
static Bindings g_b; // physical key/button -> game key/button
static HWND g_wnd, g_list, g_status, g_fixWnd, g_padWnd;
static HHOOK g_hook, g_mouseHook;
static int g_capturing = -1; // action row waiting for a key

static void SetStatus(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    buf[sizeof buf - 1] = 0;
    SetWindowTextA(g_status, buf);
}

static void Load(void)
{
    static char section[8192];
    DWORD len = GetPrivateProfileSectionA("KeyBindings", section, sizeof section, g_ini);
    if (!len)
        section[0] = section[1] = 0;
    KeyParseSection(section, &g_b, NULL);
}

static void Save(void)
{
    static char buf[8192];
    char *p = buf, a[8], b[8];
    p += sprintf(p, "; PhysicalKey=GameKey, written by SummonerFixConfig") + 1;
    for (int i = 0; i < 256; i++) {
        if (g_b.keyMouse[i] >= 0)
            p += sprintf(p, "%s=%s", KeyNameOf(i, a, sizeof a), KeyNameOf(KEY_MOUSE_BASE + g_b.keyMouse[i], b, sizeof b)) + 1;
        else if (g_b.map[i] != i)
            p += sprintf(p, "%s=%s", KeyNameOf(i, a, sizeof a), KeyNameOf(g_b.map[i], b, sizeof b)) + 1;
    }
    for (int m = 0; m < KEY_MOUSE_COUNT; m++)
        if (g_b.mouseTo[m] >= 0)
            p += sprintf(p, "%s=%s", KeyNameOf(KEY_MOUSE_BASE + m, a, sizeof a), KeyNameOf(g_b.mouseTo[m], b, sizeof b)) + 1;
    *p = 0;
    if (!WritePrivateProfileSectionA("KeyBindings", buf, g_ini))
        SetStatus("Could not write %s (error %lu)", g_ini, GetLastError());
}

static void LoadConsoleKeys(void)
{
    static char section[8192];
    DWORD len = GetPrivateProfileSectionA("ConsoleKeys", section, sizeof section, g_ini);
    g_ckCount = 0;
    for (char *line = section; len && *line && g_ckCount < MAX_CONSOLE_KEYS; line += strlen(line) + 1) {
        char *eq = strchr(line, '=');
        if (!eq || *line == ';' || eq == line || !eq[1] || (size_t)(eq - line) >= sizeof g_ck[0].key)
            continue;
        memcpy(g_ck[g_ckCount].key, line, (size_t)(eq - line));
        g_ck[g_ckCount].key[eq - line] = 0;
        lstrcpynA(g_ck[g_ckCount].cmd, eq + 1, sizeof g_ck[0].cmd);
        g_ckCount++;
    }
}

static void SaveConsoleKeys(void)
{
    static char buf[8192];
    char *p = buf;
    p += sprintf(p, "; Key=command, written by SummonerFixConfig (commands: docs/sum-exe/console.md)") + 1;
    for (int i = 0; i < g_ckCount; i++)
        p += sprintf(p, "%s=%s", g_ck[i].key, g_ck[i].cmd) + 1;
    *p = 0;
    if (!WritePrivateProfileSectionA("ConsoleKeys", buf, g_ini))
        SetStatus("Could not write %s (error %lu)", g_ini, GetLastError());
}

// What a physical key or mouse button does now (a game key/button code), or -1 if nothing.
static int TargetOf(int physical)
{
    if (physical >= KEY_MOUSE_BASE) {
        int t = g_b.mouseTo[physical - KEY_MOUSE_BASE];
        return t >= 0 ? t : (physical < KEY_MOUSE_BASE + 3 ? physical : -1);
    }
    if (g_b.keyMouse[physical] >= 0)
        return KEY_MOUSE_BASE + g_b.keyMouse[physical];
    return g_b.map[physical];
}

static void AppendName(char *out, size_t size, int code)
{
    char tmp[8];
    if (out[0])
        strncat(out, ", ", size - strlen(out) - 1);
    strncat(out, KeyNameOf(code, tmp, sizeof tmp), size - strlen(out) - 1);
}

// Text of the extra keys/buttons that do a game action ("E, Mouse4"), or "" if none.
static void BoundKeys(int gameKey, char *out, size_t size)
{
    out[0] = 0;
    for (int i = 0; i < KEY_MOUSE_BASE + KEY_MOUSE_COUNT; i++) {
        if (i == gameKey || (i >= 256 && i < KEY_MOUSE_BASE))
            continue;
        if (TargetOf(i) == gameKey)
            AppendName(out, size, i);
    }
}

static void Unbind(int physical)
{
    if (physical >= KEY_MOUSE_BASE) {
        g_b.mouseTo[physical - KEY_MOUSE_BASE] = -1;
    } else {
        g_b.map[physical] = (unsigned char)physical;
        g_b.keyMouse[physical] = -1;
    }
}

static const char *RowLabel(int row)
{
    static char label[128];
    if (row == NEW_CONSOLE_ROW) {
        snprintf(label, sizeof label, "console command \"%s\"", g_pendingCmd);
        return label;
    }
    if (row >= ROW_COUNT) {
        snprintf(label, sizeof label, "Console: %s", g_ck[row - ROW_COUNT].cmd);
        return label;
    }
    return row < ACTION_COUNT ? k_actions[row].action : k_hotkeys[row - ACTION_COUNT].action;
}

static void HotkeyValue(int h, char *out, DWORD size)
{
    GetPrivateProfileStringA("Game", k_hotkeys[h].iniKey, k_hotkeys[h].def, out, size, g_ini);
}

static void SetHotkey(int h, const char *value)
{
    if (!WritePrivateProfileStringA("Game", k_hotkeys[h].iniKey, value, g_ini))
        SetStatus("Could not write %s (error %lu)", g_ini, GetLastError());
}

static const char *ActionUsing(int key)
{
    for (int i = 0; i < ACTION_COUNT; i++)
        if (k_actions[i].gameKey == key)
            return k_actions[i].action;
    return NULL;
}

static void Refresh(void)
{
    char def[16], extra[128], tmp[8];
    for (int i = 0; i < ACTION_COUNT; i++) {
        int k = k_actions[i].gameKey;
        snprintf(def, sizeof def, "%s%s", KeyNameOf(k, tmp, sizeof tmp), TargetOf(k) != k ? " (moved)" : "");
        BoundKeys(k, extra, sizeof extra);
        ListView_SetItemText(g_list, i, 1, def);
        ListView_SetItemText(g_list, i, 2, extra);
    }
    for (int h = 0; h < HOTKEY_COUNT; h++) {
        char v[32];
        HotkeyValue(h, v, sizeof v);
        ListView_SetItemText(g_list, ACTION_COUNT + h, 1, v);
        ListView_SetItemText(g_list, ACTION_COUNT + h, 2, "");
    }
    // Console-key rows: rebuilt each time (their number changes).
    while (ListView_GetItemCount(g_list) > ROW_COUNT)
        ListView_DeleteItem(g_list, ROW_COUNT);
    for (int i = 0; i < g_ckCount; i++) {
        LVITEMA it;
        ZeroMemory(&it, sizeof it);
        it.mask = LVIF_TEXT;
        it.iItem = ROW_COUNT + i;
        it.pszText = (char *)RowLabel(ROW_COUNT + i);
        ListView_InsertItem(g_list, &it);
        ListView_SetItemText(g_list, ROW_COUNT + i, 1, g_ck[i].key);
        ListView_SetItemText(g_list, ROW_COUNT + i, 2, "[ConsoleKeys]");
    }
}

static int ConsoleKeyIndex(const char *key)
{
    for (int i = 0; i < g_ckCount; i++)
        if (!_stricmp(g_ck[i].key, key))
            return i;
    return -1;
}

// A console binding gets its key: a new entry (NEW_CONSOLE_ROW) or a changed key for an existing row.
static void AssignConsole(int row, int physical)
{
    char name[8];
    if (physical >= KEY_MOUSE_BASE) {
        SetStatus("Console-command keys must be keyboard keys.");
        return;
    }
    { char tmp[8]; lstrcpynA(name, KeyNameOf(physical, tmp, sizeof tmp), sizeof name); } // KeyNameOf returns a table string for named keys
    int other = ConsoleKeyIndex(name);
    if (row == NEW_CONSOLE_ROW) {
        if (other >= 0) {
            lstrcpynA(g_ck[other].cmd, g_pendingCmd, sizeof g_ck[0].cmd);
        } else if (g_ckCount < MAX_CONSOLE_KEYS) {
            lstrcpynA(g_ck[g_ckCount].key, name, sizeof g_ck[0].key);
            lstrcpynA(g_ck[g_ckCount].cmd, g_pendingCmd, sizeof g_ck[0].cmd);
            g_ckCount++;
        } else {
            SetStatus("Too many console keys (at most %d).", MAX_CONSOLE_KEYS);
            return;
        }
    } else {
        int i = row - ROW_COUNT;
        if (other >= 0 && other != i) { // the key already ran another command: that binding moves to this key
            g_ck[other] = g_ck[--g_ckCount];
            if (i == g_ckCount)
                i = other;
        }
        lstrcpynA(g_ck[i].key, name, sizeof g_ck[0].key);
    }
    SaveConsoleKeys();
    Refresh();
    const char *game = ActionUsing(TargetOf(physical));
    if (game)
        SetStatus("%s now runs the console command (it still also does \"%s\" in the game). Saved.", name, game);
    else
        SetStatus("%s now runs the console command. Saved.", name);
}

static void StopCapture(void)
{
    if (g_hook)
        UnhookWindowsHookEx(g_hook);
    if (g_mouseHook)
        UnhookWindowsHookEx(g_mouseHook);
    g_hook = g_mouseHook = NULL;
    g_capturing = -1;
    SetWindowTextA(GetDlgItem(g_wnd, ID_CHANGE), "&Change key...");
}

static void Assign(int row, int physical)
{
    char name[8];
    { char tmp[8]; lstrcpynA(name, KeyNameOf(physical, tmp, sizeof tmp), sizeof name); } // KeyNameOf returns a table string for named keys
    if (row >= ROW_COUNT) {
        AssignConsole(row, physical);
        return;
    }
    if (row >= ACTION_COUNT) {
        int h = row - ACTION_COUNT;
        if (physical >= KEY_MOUSE_BASE) {
            SetStatus("SummonerFix hotkeys must be keyboard keys.");
            return;
        }
        SetHotkey(h, name);
        Refresh();
        const char *other = ActionUsing(TargetOf(physical));
        if (other)
            SetStatus("%s now does \"%s\" (it still also does \"%s\" in the game). Saved.", name, k_hotkeys[h].action, other);
        else
            SetStatus("%s now does \"%s\". Saved.", name, k_hotkeys[h].action);
        return;
    }
    const Action *a = &k_actions[row];
    if (physical == a->gameKey) {
        SetStatus("%s is already the default key for %s.", name, a->action);
        return;
    }
    if (!BindingsSet(&g_b, physical, a->gameKey)) {
        SetStatus("Mouse4/Mouse5 can't be used as \"%s\" (the game has no side buttons).", a->action);
        return;
    }
    const char *other = ActionUsing(physical); // what physical did before (NULL for side buttons)
    Save();
    Refresh();
    if (other)
        SetStatus("%s now does \"%s\" (it no longer does \"%s\"). Saved.", name, a->action, other);
    else
        SetStatus("%s now does \"%s\". Saved.", name, a->action);
}

// Thread mouse hook: a mouse button pressed while capturing is the new binding (a left click on the
// Cancel button still cancels).
static LRESULT CALLBACK MouseHook(int code, WPARAM wp, LPARAM lp)
{
    if (code == HC_ACTION && g_capturing >= 0) {
        MOUSEHOOKSTRUCTEX *m = (MOUSEHOOKSTRUCTEX *)lp;
        int button = -1;
        if (wp == WM_LBUTTONDOWN && ((MOUSEHOOKSTRUCT *)lp)->hwnd != GetDlgItem(g_wnd, ID_CHANGE))
            button = 0;
        else if (wp == WM_RBUTTONDOWN)
            button = 1;
        else if (wp == WM_MBUTTONDOWN)
            button = 2;
        else if (wp == WM_XBUTTONDOWN)
            button = HIWORD(m->mouseData) == XBUTTON1 ? 3 : 4;
        if (button >= 0) {
            int row = g_capturing;
            StopCapture();
            Assign(row, KEY_MOUSE_BASE + button);
            return 1;
        }
    }
    return CallNextHookEx(g_mouseHook, code, wp, lp);
}

// Thread keyboard hook: catches the next key press while capturing, whatever control has focus.
static LRESULT CALLBACK KeyHook(int code, WPARAM wp, LPARAM lp)
{
    if (code == HC_ACTION && g_capturing >= 0 && !(lp & 0x80000000)) { // key down
        int scan = (int)((lp >> 16) & 0xff) | ((lp & 0x01000000) ? 0x80 : 0);
        int row = g_capturing;
        StopCapture();
        if (scan)
            Assign(row, scan);
        return 1; // swallow it
    }
    return CallNextHookEx(g_hook, code, wp, lp);
}

static int SelectedRow(void)
{
    return ListView_GetNextItem(g_list, -1, LVNI_SELECTED);
}

static void StartCapture(void)
{
    int row = SelectedRow();
    if (row < 0) {
        SetStatus("Select an action first.");
        return;
    }
    if (g_capturing >= 0) {
        StopCapture();
        SetStatus("Cancelled.");
        return;
    }
    g_capturing = row;
    g_hook = SetWindowsHookExA(WH_KEYBOARD, KeyHook, NULL, GetCurrentThreadId());
    g_mouseHook = SetWindowsHookExA(WH_MOUSE, MouseHook, NULL, GetCurrentThreadId());
    SetWindowTextA(GetDlgItem(g_wnd, ID_CHANGE), "&Cancel");
    SetStatus("Press the key or mouse button you want for \"%s\"...", RowLabel(row));
}

// "Bind key...": waits for the key that should run the command typed in the console-command box.
static void StartConsoleBind(void)
{
    if (g_capturing >= 0) {
        StopCapture();
        SetStatus("Cancelled.");
        return;
    }
    GetDlgItemTextA(g_wnd, ID_CONSOLE_CMD, g_pendingCmd, sizeof g_pendingCmd);
    if (!g_pendingCmd[0]) {
        SetStatus("Type a console command first (for example: add_gold 1000).");
        return;
    }
    g_capturing = NEW_CONSOLE_ROW;
    g_hook = SetWindowsHookExA(WH_KEYBOARD, KeyHook, NULL, GetCurrentThreadId());
    SetWindowTextA(GetDlgItem(g_wnd, ID_CHANGE), "&Cancel");
    SetStatus("Press the key that should run %s...", RowLabel(NEW_CONSOLE_ROW));
}

static void ClearRow(void)
{
    int row = SelectedRow();
    if (row < 0) {
        SetStatus("Select an action first.");
        return;
    }
    if (row >= ROW_COUNT) {
        int i = row - ROW_COUNT;
        char cmd[96];
        lstrcpynA(cmd, g_ck[i].cmd, sizeof cmd);
        for (int j = i; j + 1 < g_ckCount; j++)
            g_ck[j] = g_ck[j + 1];
        g_ckCount--;
        SaveConsoleKeys();
        Refresh();
        SetStatus("Removed the console key for \"%s\". Saved.", cmd);
        return;
    }
    if (row >= ACTION_COUNT) {
        SetHotkey(row - ACTION_COUNT, "none");
        Refresh();
        SetStatus("\"%s\" is now off. Saved.", RowLabel(row));
        return;
    }
    int k = k_actions[row].gameKey, n = 0;
    for (int i = 0; i < KEY_MOUSE_BASE + KEY_MOUSE_COUNT; i++) {
        if (i == k || (i >= 256 && i < KEY_MOUSE_BASE) || TargetOf(i) != k)
            continue;
        Unbind(i);
        n++;
    }
    Save();
    Refresh();
    SetStatus(n ? "Removed the extra keys for \"%s\". Saved." : "\"%s\" has no extra keys.", k_actions[row].action);
}

static void ResetAll(void)
{
    if (MessageBoxA(g_wnd, "Remove all custom key bindings and restore the F10/F11 hotkeys?", "SummonerFix", MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;
    BindingsReset(&g_b);
    Save();
    for (int h = 0; h < HOTKEY_COUNT; h++)
        SetHotkey(h, k_hotkeys[h].def);
    Refresh();
    SetStatus("All keys are back to the defaults. Saved.");
}

static void LoadOptions(void)
{
    CheckDlgButton(g_wnd, ID_SKIPMOVIES, GetPrivateProfileIntA("Game", "SkipIntroMovies", 0, g_ini) ? BST_CHECKED : BST_UNCHECKED);
    char mode[16];
    GetPrivateProfileStringA("Game", "SpeedMode", "Toggle", mode, sizeof mode, g_ini);
    CheckDlgButton(g_wnd, ID_SPEEDTOGGLE, _stricmp(mode, "Hold") ? BST_CHECKED : BST_UNCHECKED);
    for (int i = 0; i < NUM_OPTION_COUNT; i++) {
        char v[32];
        GetPrivateProfileStringA("Game", k_numOptions[i].iniKey, "", v, sizeof v, g_ini);
        SetDlgItemTextA(g_wnd, k_numOptions[i].id, v);
    }
    CheckDlgButton(g_wnd, ID_FIXALL, GetPrivateProfileIntA("Fixes", "Enabled", 1, g_ini) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(g_wnd, ID_TENTACLE, GetPrivateProfileIntA("Experimental", "TentacleGrab", 0, g_ini) ? BST_CHECKED : BST_UNCHECKED);
    char chance[16];
    GetPrivateProfileStringA("Experimental", "TentacleGrabChance", "10", chance, sizeof chance, g_ini);
    SetDlgItemTextA(g_wnd, ID_TENTCHANCE, chance);
}

static void ApplyOptions(void)
{
    WritePrivateProfileStringA("Game", "SkipIntroMovies", IsDlgButtonChecked(g_wnd, ID_SKIPMOVIES) ? "1" : "0", g_ini);
    WritePrivateProfileStringA("Game", "SpeedMode", IsDlgButtonChecked(g_wnd, ID_SPEEDTOGGLE) ? "Toggle" : "Hold", g_ini);
    for (int i = 0; i < NUM_OPTION_COUNT; i++) {
        char v[32];
        GetDlgItemTextA(g_wnd, k_numOptions[i].id, v, sizeof v);
        if (v[0] && atof(v) <= 0) {
            SetStatus("\"%s\" must be a positive number, or empty for the game's own value.", k_numOptions[i].iniKey);
            return;
        }
        WritePrivateProfileStringA("Game", k_numOptions[i].iniKey, v, g_ini);
    }
    WritePrivateProfileStringA("Fixes", "Enabled", IsDlgButtonChecked(g_wnd, ID_FIXALL) ? "1" : "0", g_ini);
    char chance[16];
    GetDlgItemTextA(g_wnd, ID_TENTCHANCE, chance, sizeof chance);
    int c = atoi(chance);
    if (c < 0 || c > 100 || !chance[0]) {
        SetStatus("\"Grab chance\" must be a number from 0 to 100.");
        return;
    }
    WritePrivateProfileStringA("Experimental", "TentacleGrab", IsDlgButtonChecked(g_wnd, ID_TENTACLE) ? "1" : "0", g_ini);
    WritePrivateProfileStringA("Experimental", "TentacleGrabChance", chance, g_ini);
    SetStatus("Options saved. The speed key mode applies at once; the others the next time the game starts.");
}

static HWND Label(HWND parent, const char *text, int x, int y, int w)
{
    HWND l = CreateWindowA("STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, 16, parent, NULL, NULL, NULL);
    SendMessageA(l, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    return l;
}

static HWND Button(HWND parent, int id, const char *text, int x, int y, int w)
{
    HWND b = CreateWindowA("BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, x, y, w, 28, parent,
                           (HMENU)(INT_PTR)id, NULL, NULL);
    SendMessageA(b, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    return b;
}

// The "Choose fixes" window: one checkbox per [Fixes] key, saved with its own Save button.
// Two columns of check boxes, filled top to bottom.
#define FIX_COL_W 300
#define FIX_ROWS ((FIX_COUNT + 1) / 2)
#define FIX_WND_W (2 * FIX_COL_W + 20)
static LRESULT CALLBACK FixWndProc(HWND w, UINT msg, WPARAM wp, LPARAM lp)
{
    (void)lp;
    switch (msg) {
    case WM_CREATE: {
        Label(w, "Fixes for bugs in the original game. Unticked = as the", 10, 10, FIX_WND_W - 20);
        Label(w, "original game. Read when the game starts.", 10, 26, FIX_WND_W - 20);
        for (int i = 0; i < FIX_COUNT; i++) {
            HWND fc = CreateWindowA("BUTTON", k_fixes[i].label, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                    10 + (i / FIX_ROWS) * FIX_COL_W, 50 + (i % FIX_ROWS) * 20, FIX_COL_W - 10, 20, w, (HMENU)(INT_PTR)(ID_FIX0 + i), NULL, NULL);
            SendMessageA(fc, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
            CheckDlgButton(w, ID_FIX0 + i, GetPrivateProfileIntA("Fixes", k_fixes[i].iniKey, 1, g_ini) ? BST_CHECKED : BST_UNCHECKED);
        }
        int y = 56 + FIX_ROWS * 20;
        if (!IsDlgButtonChecked(g_wnd, ID_FIXALL))
            Label(w, "\"Fix bugs in the original game\" is off: none apply.", 10, y, FIX_WND_W - 20);
        Button(w, ID_FIXSAVE, "&Save", 10, y + 22, 100);
        Button(w, ID_FIXCLOSE, "Close", 120, y + 22, 100);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == ID_FIXSAVE) {
            for (int i = 0; i < FIX_COUNT; i++)
                WritePrivateProfileStringA("Fixes", k_fixes[i].iniKey, IsDlgButtonChecked(w, ID_FIX0 + i) ? "1" : "0", g_ini);
            SetStatus("Bug fix choices saved. They apply the next time the game starts.");
            DestroyWindow(w);
        } else if (LOWORD(wp) == ID_FIXCLOSE) {
            DestroyWindow(w);
        }
        return 0;
    case WM_DESTROY:
        g_fixWnd = NULL;
        return 0;
    }
    return DefWindowProcA(w, msg, wp, lp);
}

static void OpenFixWindow(void)
{
    if (g_fixWnd) {
        SetForegroundWindow(g_fixWnd);
        return;
    }
    RECT r = {0, 0, FIX_WND_W, 56 + FIX_ROWS * 20 + 60};
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    AdjustWindowRect(&r, style, FALSE);
    RECT m;
    GetWindowRect(g_wnd, &m);
    g_fixWnd = CreateWindowExA(WS_EX_TOPMOST, "SummonerFixFixes", "SummonerFix - Bug fixes", style, m.left + 40, m.top + 40,
                               r.right - r.left, r.bottom - r.top, g_wnd, NULL, GetModuleHandleA(NULL), NULL);
    ShowWindow(g_fixWnd, SW_SHOW);
}

// The "Controller" window: [Controller] (src/summonerfix/controller.c), read when the game starts. The numbers and
// one edit box per input (a game key name, MouseLeft/MouseRight/MouseMiddle or none), saved with its own Save button.
#define ID_PADNUM0 400
#define ID_PADMAP0 420
typedef struct { const char *iniKey, *label; int def, lo, hi; } PadNumber;
static const PadNumber k_padNumbers[] = {
    {"DeadZone", "Left stick dead zone (of 32767):", PAD_DEF_DEADZONE, 0, 30000},
    {"CursorSpeed", "Cursor speed (game px/s):", PAD_DEF_CURSOR_SPEED, 50, 5000},
    {"CameraThreshold", "Right stick camera push (of 32767):", PAD_DEF_CAMERA_THRESHOLD, 1000, 32000},
    {"TriggerThreshold", "Trigger press (of 255):", PAD_DEF_TRIGGER_THRESHOLD, 1, 254},
};
#define PAD_NUMBER_COUNT (int)(sizeof k_padNumbers / sizeof k_padNumbers[0])
#define PAD_COL_W 340
#define PAD_ROWS ((PAD_INPUT_COUNT + 1) / 2)
#define PAD_MAP_Y 132
#define PAD_WND_W (2 * PAD_COL_W + 20)
#define PAD_WND_H (PAD_MAP_Y + PAD_ROWS * 26 + 50)

static void PadFill(HWND w, BOOL defaults)
{
    CheckDlgButton(w, ID_PADENABLED, !defaults && GetPrivateProfileIntA("Controller", "Enabled", 0, g_ini) ? BST_CHECKED : BST_UNCHECKED);
    for (int i = 0; i < PAD_NUMBER_COUNT; i++) {
        char v[16];
        snprintf(v, sizeof v, "%d", defaults ? k_padNumbers[i].def : (int)GetPrivateProfileIntA("Controller", k_padNumbers[i].iniKey, k_padNumbers[i].def, g_ini));
        SetDlgItemTextA(w, ID_PADNUM0 + i, v);
    }
    for (int i = 0; i < PAD_INPUT_COUNT; i++) {
        char v[32];
        if (defaults)
            lstrcpynA(v, k_padInputs[i].def, sizeof v);
        else
            GetPrivateProfileStringA("Controller", k_padInputs[i].iniKey, k_padInputs[i].def, v, sizeof v, g_ini);
        SetDlgItemTextA(w, ID_PADMAP0 + i, v);
    }
}

static BOOL PadSave(HWND w)
{
    char v[32];
    for (int i = 0; i < PAD_NUMBER_COUNT; i++) {
        GetDlgItemTextA(w, ID_PADNUM0 + i, v, sizeof v);
        int n = atoi(v);
        if (!v[0] || n < k_padNumbers[i].lo || n > k_padNumbers[i].hi) {
            MessageBoxA(w, "A number is out of range; nothing was saved.", k_padNumbers[i].label, MB_OK | MB_ICONWARNING);
            return FALSE;
        }
    }
    for (int i = 0; i < PAD_INPUT_COUNT; i++) {
        GetDlgItemTextA(w, ID_PADMAP0 + i, v, sizeof v);
        int code = v[0] && _stricmp(v, "none") ? KeyParse(v) : 0;
        if (code < 0 || code >= KEY_MOUSE_BASE + 3) {
            char msg[128];
            snprintf(msg, sizeof msg, "\"%s\" for %s is not a game key, MouseLeft, MouseRight, MouseMiddle or none; nothing was saved.", v,
                     k_padInputs[i].iniKey);
            MessageBoxA(w, msg, "SummonerFix - Controller", MB_OK | MB_ICONWARNING);
            return FALSE;
        }
    }
    WritePrivateProfileStringA("Controller", "Enabled", IsDlgButtonChecked(w, ID_PADENABLED) ? "1" : "0", g_ini);
    for (int i = 0; i < PAD_NUMBER_COUNT; i++) {
        GetDlgItemTextA(w, ID_PADNUM0 + i, v, sizeof v);
        snprintf(v, sizeof v, "%d", atoi(v));
        WritePrivateProfileStringA("Controller", k_padNumbers[i].iniKey, v, g_ini);
    }
    for (int i = 0; i < PAD_INPUT_COUNT; i++) {
        GetDlgItemTextA(w, ID_PADMAP0 + i, v, sizeof v);
        WritePrivateProfileStringA("Controller", k_padInputs[i].iniKey, v[0] ? v : "none", g_ini);
    }
    return TRUE;
}

static HWND PadEdit(HWND parent, int id, int x, int y, int width)
{
    HWND e = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, x, y, width, 22, parent,
                             (HMENU)(INT_PTR)id, NULL, NULL);
    SendMessageA(e, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    return e;
}

static LRESULT CALLBACK PadWndProc(HWND w, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE: {
        Label(w, "An XInput (Xbox-style) controller drives the mouse and keys. OpenGL renderer only; read when the game", 10, 10, PAD_WND_W - 20);
        Label(w, "starts. Keys are the game's own (key bindings do not change them). Not yet tested with a real controller.", 10, 26, PAD_WND_W - 20);
        HWND cb = CreateWindowA("BUTTON", "Use a controller", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 10, 48, 200, 20, w,
                                (HMENU)(INT_PTR)ID_PADENABLED, NULL, NULL);
        SendMessageA(cb, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        for (int i = 0; i < PAD_NUMBER_COUNT; i++) {
            int x = 10 + (i % 2) * PAD_COL_W, y = 76 + (i / 2) * 26;
            Label(w, k_padNumbers[i].label, x, y + 3, 200);
            PadEdit(w, ID_PADNUM0 + i, x + 205, y, 60);
        }
        for (int i = 0; i < PAD_INPUT_COUNT; i++) {
            int x = 10 + (i / PAD_ROWS) * PAD_COL_W, y = PAD_MAP_Y + (i % PAD_ROWS) * 26;
            char label[96];
            snprintf(label, sizeof label, "%s (default %s: %s)", k_padInputs[i].iniKey, k_padInputs[i].def, k_padInputs[i].label);
            Label(w, label, x + 95, y + 3, PAD_COL_W - 100);
            PadEdit(w, ID_PADMAP0 + i, x, y, 90);
        }
        int y = PAD_MAP_Y + PAD_ROWS * 26 + 10;
        Button(w, ID_PADSAVE, "&Save", 10, y, 100);
        Button(w, ID_PADRESET, "&Defaults", 120, y, 100);
        Button(w, ID_PADCLOSE, "Close", 230, y, 100);
        PadFill(w, FALSE);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == ID_PADSAVE) {
            if (PadSave(w)) {
                SetStatus("Controller settings saved. They apply the next time the game starts.");
                DestroyWindow(w);
            }
        } else if (LOWORD(wp) == ID_PADRESET) {
            PadFill(w, TRUE); // shown only; Save writes them
        } else if (LOWORD(wp) == ID_PADCLOSE) {
            DestroyWindow(w);
        }
        return 0;
    case WM_DESTROY:
        g_padWnd = NULL;
        return 0;
    }
    return DefWindowProcA(w, msg, wp, lp);
}

static void OpenPadWindow(void)
{
    if (g_padWnd) {
        SetForegroundWindow(g_padWnd);
        return;
    }
    RECT r = {0, 0, PAD_WND_W, PAD_WND_H};
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    AdjustWindowRect(&r, style, FALSE);
    RECT m;
    GetWindowRect(g_wnd, &m);
    g_padWnd = CreateWindowExA(WS_EX_TOPMOST, "SummonerFixPad", "SummonerFix - Controller", style, m.left + 40, m.top + 40,
                               r.right - r.left, r.bottom - r.top, g_wnd, NULL, GetModuleHandleA(NULL), NULL);
    ShowWindow(g_padWnd, SW_SHOW);
}

static LRESULT CALLBACK WndProc(HWND w, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE: {
        g_list = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                                 10, 10, 520, 490, w, (HMENU)ID_LIST, NULL, NULL);
        ListView_SetExtendedListViewStyle(g_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        LVCOLUMNA col;
        ZeroMemory(&col, sizeof col);
        col.mask = LVCF_TEXT | LVCF_WIDTH;
        col.pszText = "Action"; col.cx = 270; ListView_InsertColumn(g_list, 0, &col);
        col.pszText = "Game key"; col.cx = 110; ListView_InsertColumn(g_list, 1, &col);
        col.pszText = "Also on"; col.cx = 130; ListView_InsertColumn(g_list, 2, &col);
        for (int i = 0; i < ACTION_COUNT; i++) {
            LVITEMA it;
            ZeroMemory(&it, sizeof it);
            it.mask = LVIF_TEXT;
            it.iItem = i;
            it.pszText = (char *)k_actions[i].action;
            ListView_InsertItem(g_list, &it);
        }
        for (int h = 0; h < HOTKEY_COUNT; h++) {
            LVITEMA it;
            ZeroMemory(&it, sizeof it);
            it.mask = LVIF_TEXT;
            it.iItem = ACTION_COUNT + h;
            it.pszText = (char *)k_hotkeys[h].action;
            ListView_InsertItem(g_list, &it);
        }
        Button(w, ID_CHANGE, "&Change key...", 540, 10, 120);
        Button(w, ID_CLEAR, "C&lear", 540, 45, 120);
        Button(w, ID_RESET, "&Reset all", 540, 80, 120);
        Label(w, "Game options:", 540, 125, 200);
        HWND cb = CreateWindowA("BUTTON", "Skip intro movies", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 540, 143, 200, 20,
                                w, (HMENU)(INT_PTR)ID_SKIPMOVIES, NULL, NULL);
        SendMessageA(cb, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        cb = CreateWindowA("BUTTON", "Speed key toggles (off = hold)", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 540, 163, 200, 20,
                           w, (HMENU)(INT_PTR)ID_SPEEDTOGGLE, NULL, NULL);
        SendMessageA(cb, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        for (int i = 0; i < NUM_OPTION_COUNT; i++) {
            int y = 190 + i * 42;
            Label(w, k_numOptions[i].label, 540, y, 200);
            HWND e = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 540, y + 16, 60, 22,
                                     w, (HMENU)(INT_PTR)k_numOptions[i].id, NULL, NULL);
            SendMessageA(e, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
            Label(w, k_numOptions[i].hint, 606, y + 19, 140);
        }
        Label(w, "Bug fixes (next game start):", 770, 125, 230);
        cb = CreateWindowA("BUTTON", "Fix bugs in the original game", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 770, 143, 230, 20,
                           w, (HMENU)(INT_PTR)ID_FIXALL, NULL, NULL);
        SendMessageA(cb, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        Button(w, ID_FIXLIST, "C&hoose fixes...", 770, 168, 120);
        Label(w, "Off = as the original game. Saved with", 770, 202, 230);
        Label(w, "Save options.", 770, 218, 230);
        Label(w, "Experimental (next game start):", 770, 250, 230);
        cb = CreateWindowA("BUTTON", "Tentacle Beast grabs and throws", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 770, 268, 230, 20,
                           w, (HMENU)(INT_PTR)ID_TENTACLE, NULL, NULL);
        SendMessageA(cb, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        Label(w, "Grab chance (%):", 770, 296, 100);
        HWND te = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER, 870, 293, 40, 22,
                                  w, (HMENU)(INT_PTR)ID_TENTCHANCE, NULL, NULL);
        SendMessageA(te, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        Label(w, "Off = as the original game (the", 770, 322, 230);
        Label(w, "developers switched it off).", 770, 338, 230);
        Button(w, ID_APPLY, "Save &options", 540, 320, 120);
        Label(w, "Console command for a key:", 540, 372, 200);
        HWND ce = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 540, 390, 200, 22,
                                  w, (HMENU)(INT_PTR)ID_CONSOLE_CMD, NULL, NULL);
        SendMessageA(ce, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        Button(w, ID_CONSOLE_BIND, "&Bind key...", 540, 418, 120);
        Label(w, "Controller (next game start):", 770, 372, 230);
        Button(w, ID_PADOPEN, "Co&ntroller...", 770, 390, 120);
        Button(w, ID_CLOSE, "Close", 540, 472, 120);
        g_status = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE, 10, 510, 990, 20, w, (HMENU)ID_STATUS, NULL, NULL);
        SendMessageA(g_status, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        Load();
        LoadConsoleKeys();
        Refresh();
        g_wnd = w;
        LoadOptions();
        ListView_SetItemState(g_list, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        SetStatus(GetFileAttributesA(g_ini) == INVALID_FILE_ATTRIBUTES
                      ? "summonerfix.ini not found next to this program; it will be created."
                      : "Select an action and click Change key. Changes apply to a running game at once (F10 in game opens this window).");
        return 0;
    }
    case WM_NOTIFY:
        if (((NMHDR *)lp)->idFrom == ID_LIST && ((NMHDR *)lp)->code == NM_DBLCLK)
            StartCapture();
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case ID_CHANGE: StartCapture(); break;
        case ID_CLEAR: ClearRow(); break;
        case ID_RESET: ResetAll(); break;
        case ID_APPLY: ApplyOptions(); break;
        case ID_FIXLIST: OpenFixWindow(); break;
        case ID_PADOPEN: OpenPadWindow(); break;
        case ID_CONSOLE_BIND: StartConsoleBind(); break;
        case ID_CLOSE: DestroyWindow(w); break;
        }
        return 0;
    case WM_DESTROY:
        StopCapture();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(w, msg, wp, lp);
}

// Windows 11 Smart App Control: when it enforces (VerifiedAndReputablePolicyState = 1), Windows refuses to load the
// unsigned dinput.dll and the game starts silently without SummonerFix. Say so once at start-up (docs/summonerfix).
static void WarnSmartAppControl(void)
{
    HKEY k;
    DWORD state = 0, size = sizeof state, type = 0;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\CI\\Policy", 0, KEY_QUERY_VALUE, &k) != ERROR_SUCCESS)
        return;
    LONG rc = RegQueryValueExA(k, "VerifiedAndReputablePolicyState", NULL, &type, (BYTE *)&state, &size);
    RegCloseKey(k);
    if (rc != ERROR_SUCCESS || type != REG_DWORD || state != 1)
        return;
    MessageBoxA(NULL,
                "Smart App Control is on.\n\n"
                "Windows will not load SummonerFix (dinput.dll is not signed by a trusted publisher). The game still "
                "starts, but without SummonerFix: original speed, no OpenGL or widescreen fixes, no bug fixes, and "
                "these settings have no effect.\n\n"
                "Your choices: turn Smart App Control off (Windows Security, App & browser control; Windows cannot "
                "turn it back on without a reset), or play without SummonerFix.",
                "SummonerFix - Smart App Control", MB_OK | MB_ICONWARNING);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show)
{
    (void)prev; (void)cmd;
    WarnSmartAppControl();
    GetModuleFileNameA(NULL, g_ini, MAX_PATH);
    char *slash = strrchr(g_ini, '\\');
    strcpy(slash ? slash + 1 : g_ini, "summonerfix.ini");

    INITCOMMONCONTROLSEX icc = {sizeof icc, ICC_LISTVIEW_CLASSES};
    InitCommonControlsEx(&icc);
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "SummonerFixConfig";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassA(&wc);
    wc.lpfnWndProc = FixWndProc;
    wc.lpszClassName = "SummonerFixFixes";
    RegisterClassA(&wc);
    wc.lpfnWndProc = PadWndProc;
    wc.lpszClassName = "SummonerFixPad";
    RegisterClassA(&wc);
    wc.lpszClassName = "SummonerFixConfig"; // the main window below is created by this name
    RECT r = {0, 0, 1010, 540};
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&r, style, FALSE);
    g_wnd = CreateWindowExA(WS_EX_TOPMOST, wc.lpszClassName, "SummonerFix - Key bindings and options", style, CW_USEDEFAULT, CW_USEDEFAULT,
                          r.right - r.left, r.bottom - r.top, NULL, NULL, inst, NULL);
    ShowWindow(g_wnd, show);
    MSG m;
    while (GetMessageA(&m, NULL, 0, 0) > 0) {
        if (!IsDialogMessageA(g_wnd, &m) && !(g_fixWnd && IsDialogMessageA(g_fixWnd, &m)) &&
            !(g_padWnd && IsDialogMessageA(g_padWnd, &m))) {
            TranslateMessage(&m);
            DispatchMessageA(&m);
        }
    }
    return 0;
}
