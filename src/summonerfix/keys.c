// Custom key bindings.
// Every key event goes through key_mark (0x5093a0), which first calls key_remap_layout (0x509560) to
// swap scan codes for French/German keyboards (see docs/sum-exe/controls.md). We redirect that one call
// to KeyRemap: the original layout swap runs first, then our table from [KeyBindings] in summonerfix.ini.
// The result feeds both the key queue (the game's hard-coded key map) and key_is_down/key_down_time.
#include "summonerfix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VA_KEY_MARK_REMAP_CALL 0x5093a6u // CALL key_remap_layout inside key_mark
#define VA_KEY_REMAP_LAYOUT 0x509560u

typedef int(__cdecl *PFN_KeyRemapLayout)(int code);
static PFN_KeyRemapLayout g_layout;
static BYTE g_map[256];

// DirectInput-style scan codes, as used by the game (extended keys have bit 0x80 set).
static const struct { const char *name; BYTE code; } k_names[] = {
    {"Esc", 0x01}, {"Escape", 0x01}, {"1", 0x02}, {"2", 0x03}, {"3", 0x04}, {"4", 0x05}, {"5", 0x06},
    {"6", 0x07}, {"7", 0x08}, {"8", 0x09}, {"9", 0x0a}, {"0", 0x0b}, {"Minus", 0x0c}, {"Equals", 0x0d},
    {"Backspace", 0x0e}, {"Tab", 0x0f}, {"Q", 0x10}, {"W", 0x11}, {"E", 0x12}, {"R", 0x13}, {"T", 0x14},
    {"Y", 0x15}, {"U", 0x16}, {"I", 0x17}, {"O", 0x18}, {"P", 0x19}, {"LBracket", 0x1a}, {"RBracket", 0x1b},
    {"Enter", 0x1c}, {"LCtrl", 0x1d}, {"A", 0x1e}, {"S", 0x1f}, {"D", 0x20}, {"F", 0x21}, {"G", 0x22},
    {"H", 0x23}, {"J", 0x24}, {"K", 0x25}, {"L", 0x26}, {"Semicolon", 0x27}, {"Apostrophe", 0x28},
    {"Grave", 0x29}, {"LShift", 0x2a}, {"Backslash", 0x2b}, {"Z", 0x2c}, {"X", 0x2d}, {"C", 0x2e},
    {"V", 0x2f}, {"B", 0x30}, {"N", 0x31}, {"M", 0x32}, {"Comma", 0x33}, {"Period", 0x34}, {"Slash", 0x35},
    {"RShift", 0x36}, {"NumpadStar", 0x37}, {"LAlt", 0x38}, {"Space", 0x39}, {"CapsLock", 0x3a},
    {"F1", 0x3b}, {"F2", 0x3c}, {"F3", 0x3d}, {"F4", 0x3e}, {"F5", 0x3f}, {"F6", 0x40}, {"F7", 0x41},
    {"F8", 0x42}, {"F9", 0x43}, {"F10", 0x44}, {"NumLock", 0x45}, {"ScrollLock", 0x46}, {"Numpad7", 0x47},
    {"Numpad8", 0x48}, {"Numpad9", 0x49}, {"NumpadMinus", 0x4a}, {"Numpad4", 0x4b}, {"Numpad5", 0x4c},
    {"Numpad6", 0x4d}, {"NumpadPlus", 0x4e}, {"Numpad1", 0x4f}, {"Numpad2", 0x50}, {"Numpad3", 0x51},
    {"Numpad0", 0x52}, {"NumpadPeriod", 0x53}, {"F11", 0x57}, {"F12", 0x58}, {"NumpadEnter", 0x9c},
    {"RCtrl", 0x9d}, {"NumpadSlash", 0xb5}, {"RAlt", 0xb8}, {"Home", 0xc7}, {"Up", 0xc8}, {"PageUp", 0xc9},
    {"Left", 0xcb}, {"Right", 0xcd}, {"End", 0xcf}, {"Down", 0xd0}, {"PageDown", 0xd1}, {"Insert", 0xd2},
    {"Delete", 0xd3},
};

static int ParseKey(const char *s)
{
    while (*s == ' ' || *s == '\t')
        s++;
    char buf[32];
    size_t n = strcspn(s, " \t;\r\n");
    if (n == 0 || n >= sizeof buf)
        return -1;
    memcpy(buf, s, n);
    buf[n] = 0;
    if (buf[0] == '0' && (buf[1] == 'x' || buf[1] == 'X')) {
        long v = strtol(buf, NULL, 16);
        return v > 0 && v < 256 ? (int)v : -1;
    }
    for (size_t i = 0; i < sizeof k_names / sizeof k_names[0]; i++)
        if (_stricmp(buf, k_names[i].name) == 0)
            return k_names[i].code;
    return -1;
}

static int __cdecl KeyRemap(int code)
{
    int c = g_layout(code);
    if (c >= 0 && c < 256)
        c = g_map[c];
    return c;
}

void KeyBindInstall(void)
{
    for (int i = 0; i < 256; i++)
        g_map[i] = (BYTE)i;

    char section[8192];
    DWORD len = GetPrivateProfileSectionA("KeyBindings", section, sizeof section, g_cfg.iniPath);
    int count = 0;
    for (const char *line = section; len && *line; line += strlen(line) + 1) {
        if (*line == ';' || *line == '#')
            continue;
        const char *eq = strchr(line, '=');
        if (!eq)
            continue;
        char lhs[32];
        size_t n = (size_t)(eq - line);
        if (n >= sizeof lhs)
            continue;
        memcpy(lhs, line, n);
        lhs[n] = 0;
        int from = ParseKey(lhs), to = ParseKey(eq + 1);
        if (from < 0 || to < 0) {
            Log("key bindings: ignoring \"%s\" (unknown key name)", line);
            continue;
        }
        g_map[from] = (BYTE)to;
        Log("key bindings: physical 0x%02X acts as 0x%02X (%s)", from, to, line);
        count++;
    }
    if (!count)
        return; // nothing configured: leave the game untouched

    g_layout = (PFN_KeyRemapLayout)(UINT_PTR)SumVa(VA_KEY_REMAP_LAYOUT);
    DWORD site = SumVa(VA_KEY_MARK_REMAP_CALL);
    BYTE orig[5] = {0xE8}, repl[5] = {0xE8};
    DWORD rel = SumVa(VA_KEY_REMAP_LAYOUT) - (site + 5);
    memcpy(orig + 1, &rel, 4);
    rel = (DWORD)(UINT_PTR)KeyRemap - (site + 5);
    memcpy(repl + 1, &rel, 4);
    PatchBytes(VA_KEY_MARK_REMAP_CALL, orig, repl, 5, "key_mark -> KeyRemap (custom key bindings)");
}
