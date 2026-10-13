// SummonerFixConfig - edits the [KeyBindings] section of summonerfix.ini next to this program.
// Pick an action, click "Change key..." and press the key you want for it. Changes are saved at once;
// a running Sum.exe with SummonerFix reloads them live. See docs/summonerfix/index.md.
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "../summonerfix/keynames.h"

// The game's hard-coded key map (docs/sum-exe/controls.md). gameKey is the code the game reacts to.
typedef struct { unsigned char gameKey; const char *action; } Action;
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
    {0x23, "H key action"}, {0x3c, "F2 screen"},
};
#define ACTION_COUNT (int)(sizeof k_actions / sizeof k_actions[0])

enum { ID_LIST = 100, ID_CHANGE, ID_CLEAR, ID_RESET, ID_CLOSE, ID_STATUS };

static char g_ini[MAX_PATH];
static unsigned char g_map[256]; // physical -> game
static HWND g_wnd, g_list, g_status;
static HHOOK g_hook;
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
    KeyParseSection(section, g_map, NULL);
}

static void Save(void)
{
    static char buf[8192];
    char *p = buf, a[8], b[8];
    p += sprintf(p, "; PhysicalKey=GameKey, written by SummonerFixConfig") + 1;
    for (int i = 0; i < 256; i++)
        if (g_map[i] != i)
            p += sprintf(p, "%s=%s", KeyNameOf(i, a, sizeof a), KeyNameOf(g_map[i], b, sizeof b)) + 1;
    *p = 0;
    if (!WritePrivateProfileSectionA("KeyBindings", buf, g_ini))
        SetStatus("Could not write %s (error %lu)", g_ini, GetLastError());
}

// Text of the extra keys bound to a game key ("E, Numpad5"), or "" if none.
static void BoundKeys(int gameKey, char *out, size_t size)
{
    char tmp[8];
    out[0] = 0;
    for (int i = 0; i < 256; i++) {
        if (i == gameKey || g_map[i] != gameKey)
            continue;
        if (out[0])
            strncat(out, ", ", size - strlen(out) - 1);
        strncat(out, KeyNameOf(i, tmp, sizeof tmp), size - strlen(out) - 1);
    }
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
        snprintf(def, sizeof def, "%s%s", KeyNameOf(k, tmp, sizeof tmp), g_map[k] != k ? " (moved)" : "");
        BoundKeys(k, extra, sizeof extra);
        ListView_SetItemText(g_list, i, 1, def);
        ListView_SetItemText(g_list, i, 2, extra);
    }
}

static void StopCapture(void)
{
    if (g_hook)
        UnhookWindowsHookEx(g_hook);
    g_hook = NULL;
    g_capturing = -1;
    SetWindowTextA(GetDlgItem(g_wnd, ID_CHANGE), "&Change key...");
}

static void Assign(int row, int physical)
{
    const Action *a = &k_actions[row];
    char name[8];
    KeyNameOf(physical, name, sizeof name);
    if (physical == a->gameKey) {
        SetStatus("%s is already the default key for %s.", name, a->action);
        return;
    }
    const char *other = ActionUsing(physical);
    g_map[physical] = a->gameKey;
    Save();
    Refresh();
    if (other)
        SetStatus("%s now does \"%s\" (it no longer does \"%s\"). Saved.", name, a->action, other);
    else
        SetStatus("%s now does \"%s\". Saved.", name, a->action);
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
    SetWindowTextA(GetDlgItem(g_wnd, ID_CHANGE), "&Cancel");
    SetStatus("Press the key you want for \"%s\"...", k_actions[row].action);
}

static void ClearRow(void)
{
    int row = SelectedRow();
    if (row < 0) {
        SetStatus("Select an action first.");
        return;
    }
    int k = k_actions[row].gameKey, n = 0;
    for (int i = 0; i < 256; i++)
        if (i != k && g_map[i] == k) {
            g_map[i] = (unsigned char)i;
            n++;
        }
    Save();
    Refresh();
    SetStatus(n ? "Removed the extra keys for \"%s\". Saved." : "\"%s\" has no extra keys.", k_actions[row].action);
}

static void ResetAll(void)
{
    if (MessageBoxA(g_wnd, "Remove all custom key bindings?", "SummonerFix", MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;
    for (int i = 0; i < 256; i++)
        g_map[i] = (unsigned char)i;
    Save();
    Refresh();
    SetStatus("All keys are back to the game defaults. Saved.");
}

static HWND Button(HWND parent, int id, const char *text, int x, int y, int w)
{
    HWND b = CreateWindowA("BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, x, y, w, 28, parent,
                           (HMENU)(INT_PTR)id, NULL, NULL);
    SendMessageA(b, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    return b;
}

static LRESULT CALLBACK WndProc(HWND w, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE: {
        g_list = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                                 10, 10, 520, 420, w, (HMENU)ID_LIST, NULL, NULL);
        ListView_SetExtendedListViewStyle(g_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        LVCOLUMNA col;
        ZeroMemory(&col, sizeof col);
        col.mask = LVCF_TEXT | LVCF_WIDTH;
        col.pszText = "Action"; col.cx = 250; ListView_InsertColumn(g_list, 0, &col);
        col.pszText = "Game key"; col.cx = 110; ListView_InsertColumn(g_list, 1, &col);
        col.pszText = "Also on"; col.cx = 150; ListView_InsertColumn(g_list, 2, &col);
        for (int i = 0; i < ACTION_COUNT; i++) {
            LVITEMA it;
            ZeroMemory(&it, sizeof it);
            it.mask = LVIF_TEXT;
            it.iItem = i;
            it.pszText = (char *)k_actions[i].action;
            ListView_InsertItem(g_list, &it);
        }
        Button(w, ID_CHANGE, "&Change key...", 540, 10, 120);
        Button(w, ID_CLEAR, "C&lear", 540, 45, 120);
        Button(w, ID_RESET, "&Reset all", 540, 80, 120);
        Button(w, ID_CLOSE, "Close", 540, 402, 120);
        g_status = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE, 10, 440, 650, 20, w, (HMENU)ID_STATUS, NULL, NULL);
        SendMessageA(g_status, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        Load();
        Refresh();
        ListView_SetItemState(g_list, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        SetStatus(GetFileAttributesA(g_ini) == INVALID_FILE_ATTRIBUTES
                      ? "summonerfix.ini not found next to this program; it will be created."
                      : "Select an action and click Change key. The game picks up changes immediately.");
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

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show)
{
    (void)prev; (void)cmd;
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
    RECT r = {0, 0, 670, 470};
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&r, style, FALSE);
    g_wnd = CreateWindowA(wc.lpszClassName, "SummonerFix - Key bindings", style, CW_USEDEFAULT, CW_USEDEFAULT,
                          r.right - r.left, r.bottom - r.top, NULL, NULL, inst, NULL);
    ShowWindow(g_wnd, show);
    MSG m;
    while (GetMessageA(&m, NULL, 0, 0) > 0) {
        if (!IsDialogMessageA(g_wnd, &m)) {
            TranslateMessage(&m);
            DispatchMessageA(&m);
        }
    }
    return 0;
}
