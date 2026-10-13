// Key names <-> game key codes (DirectInput-style scan codes; extended keys have bit 0x80 set).
// Shared by SummonerFix (keys.c) and SummonerFixConfig. Header-only: include in one file per program.
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { const char *name; unsigned short code; } KeyName;

// Mouse buttons use codes above the keyboard range. The game itself knows left/right/middle (0-2);
// Mouse4/Mouse5 (side buttons) are only usable as binding sources.
#define KEY_MOUSE_BASE 0x100
#define KEY_MOUSE_COUNT 5
// Keyboard keys bound to a mouse button are sent to the game as this unused code, so they lose their own action.
#define KEY_DEAD 0x54

static const KeyName k_keyNames[] = {
    {"Esc", 0x01}, {"1", 0x02}, {"2", 0x03}, {"3", 0x04}, {"4", 0x05}, {"5", 0x06},
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
    {"MouseLeft", 0x100}, {"MouseRight", 0x101}, {"MouseMiddle", 0x102}, {"Mouse4", 0x103}, {"Mouse5", 0x104},
};
#define KEYNAME_COUNT (sizeof k_keyNames / sizeof k_keyNames[0])

// "E", "escape", "0x17" -> code; -1 if unknown. Stops at whitespace, ';' or end of string.
static int KeyParse(const char *s)
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
    if (_stricmp(buf, "Escape") == 0)
        return 0x01;
    for (size_t i = 0; i < KEYNAME_COUNT; i++)
        if (_stricmp(buf, k_keyNames[i].name) == 0)
            return k_keyNames[i].code;
    return -1;
}

// code -> name, or "0xNN" written into buf (buf needs 8 bytes).
static const char *KeyNameOf(int code, char *buf, size_t size)
{
    for (size_t i = 0; i < KEYNAME_COUNT; i++)
        if (k_keyNames[i].code == code)
            return k_keyNames[i].name;
    _snprintf(buf, size, "0x%02X", code & 0xff);
    buf[size - 1] = 0;
    return buf;
}

// All bindings from a [KeyBindings] section.
typedef struct {
    unsigned char map[256];     // physical key -> game key (identity when unbound; KEY_DEAD when bound to a mouse button)
    signed char keyMouse[256];  // physical key -> mouse button 0-2 it presses, or -1
    short mouseTo[KEY_MOUSE_COUNT]; // mouse button (left, right, middle, 4, 5) -> game key, or 0x100+button, or -1 = default
} Bindings;

static void BindingsReset(Bindings *b)
{
    for (int i = 0; i < 256; i++) {
        b->map[i] = (unsigned char)i;
        b->keyMouse[i] = -1;
    }
    for (int i = 0; i < KEY_MOUSE_COUNT; i++)
        b->mouseTo[i] = -1;
}

// Apply one binding; FALSE if the combination is not supported.
static int BindingsSet(Bindings *b, int from, int to)
{
    int toMouse = to >= KEY_MOUSE_BASE;
    if (toMouse && to >= KEY_MOUSE_BASE + 3)
        return 0; // the game has no side buttons to press
    if (from < KEY_MOUSE_BASE) {
        if (toMouse) {
            b->keyMouse[from] = (signed char)(to - KEY_MOUSE_BASE);
            b->map[from] = KEY_DEAD;
        } else {
            b->map[from] = (unsigned char)to;
            b->keyMouse[from] = -1;
        }
    } else {
        b->mouseTo[from - KEY_MOUSE_BASE] = (short)to;
    }
    return 1;
}

// Parse a [KeyBindings] section (GetPrivateProfileSection format: NUL-separated "A=B" strings, double NUL at
// the end). Calls bad(line) for unparseable entries. Returns the count.
static int KeyParseSection(const char *section, Bindings *b, void (*bad)(const char *line))
{
    BindingsReset(b);
    int count = 0;
    for (const char *line = section; *line; line += strlen(line) + 1) {
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
        int from = KeyParse(lhs), to = KeyParse(eq + 1);
        if (from < 0 || to < 0 || !BindingsSet(b, from, to)) {
            if (bad)
                bad(line);
            continue;
        }
        count++;
    }
    return count;
}
