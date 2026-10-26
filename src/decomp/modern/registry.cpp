// registry.cpp - the game's registry key in summonerfix.ini (include/modern/modern.h; L3 player build). Not original
// source.
//
// SummonerFix's reghook.c for a build that runs without it. Sum.exe reads its video and sound settings from
// Software\Volition\Summoner (docs/sum-exe/video-settings.md), which the original launcher wrote. vsdk/registry.cpp's
// seven registry calls come here: under that key the display and sound values come from summonerfix.ini [Display]
// and [Sound] (GameWidth / GameHeight, Renderer, Soundcard; InstallPath is the exe's folder), and anything else the
// game writes is kept in [Registry] ("dword:...", "sz:...", "hex:..."), so the 32-bit builds under SummonerFix and
// this build share one set of settings, the one SummonerFixConfig.exe edits. Every other key, and every key while
// SummonerFix is loaded, goes to the real registry.
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "modern/modern.h"
#include "../../summonerfix/screensize.h"

#if defined(MODERN_BUILD)

#define FAKE_BASE 0x5EED0000u
#define MAX_FAKE 16

static struct {
    bool used;
    char sub[128];   // the path below Software\Volition\Summoner, "" for the key itself
} fake[MAX_FAKE];

static int reg_width, reg_height;   // [Display] GameWidth / GameHeight, read once

static void read_size()
{
    if (reg_width)
        return;
    char gw[16];
    modern_ini_str("Display", "GameWidth", "Auto", gw, sizeof gw);
    if (!_stricmp(gw, "Auto")) {
        DisplaySizeForScreen(&reg_width, &reg_height);   // the 1024x768 layout at the screen's shape
    } else {
        reg_width = atoi(gw) > 0 ? atoi(gw) : 1024;
        reg_height = modern_ini_int("Display", "GameHeight", 768);
    }
    modern_log("registry: game %dx%d (summonerfix.ini [Display] GameWidth=%s)", reg_width, reg_height, gw);
}

static int fake_index(HKEY h)
{
    UINT_PTR v = (UINT_PTR)h;
    if (v >= FAKE_BASE && v < FAKE_BASE + MAX_FAKE && fake[v - FAKE_BASE].used)
        return (int)(v - FAKE_BASE);
    return -1;
}

// If parent + sub names Software\Volition\Summoner (or below), the part below it.
static bool resolve_game_key(HKEY parent, LPCSTR sub_key, char *out, size_t out_len)
{
    const char *s = sub_key ? sub_key : "";
    int pi = fake_index(parent);
    if (pi >= 0) {
        _snprintf(out, out_len, "%s%s%s", fake[pi].sub, (fake[pi].sub[0] && s[0]) ? "\\" : "", s);
        out[out_len - 1] = 0;
        return true;
    }
    static const char marker[] = "volition\\summoner";
    char lower[512];
    lstrcpynA(lower, s, sizeof lower);
    CharLowerA(lower);
    const char *hit = strstr(lower, marker);
    if (!hit)
        return false;
    const char *rest = s + (hit - lower) + (sizeof marker - 1);
    if (*rest == '\\')
        rest++;
    lstrcpynA(out, rest, (int)out_len);
    return true;
}

static HKEY make_fake(const char *sub)
{
    for (int i = 0; i < MAX_FAKE; i++) {
        if (!fake[i].used) {
            fake[i].used = true;
            lstrcpynA(fake[i].sub, sub, sizeof fake[i].sub);
            return (HKEY)(UINT_PTR)(FAKE_BASE + i);
        }
    }
    return NULL;
}

static void store_name(int fi, LPCSTR name, char *out, size_t out_len)
{
    _snprintf(out, out_len, "%s%s%s", fake[fi].sub, fake[fi].sub[0] ? "\\" : "", name ? name : "");
    out[out_len - 1] = 0;
}

static LONG return_value(DWORD type, const void *data, DWORD len, LPDWORD lp_type, LPBYTE lp_data, LPDWORD lp_cb)
{
    if (lp_type)
        *lp_type = type;
    if (!lp_cb)
        return lp_data ? ERROR_INVALID_PARAMETER : ERROR_SUCCESS;
    DWORD cap = *lp_cb;
    *lp_cb = len;
    if (!lp_data)
        return ERROR_SUCCESS;
    if (cap < len)
        return ERROR_MORE_DATA;
    memcpy(lp_data, data, len);
    return ERROR_SUCCESS;
}

static LONG return_dword(DWORD v, LPDWORD lp_type, LPBYTE lp_data, LPDWORD lp_cb)
{
    return return_value(REG_DWORD, &v, sizeof v, lp_type, lp_data, lp_cb);
}

static LONG return_string(const char *s, LPDWORD lp_type, LPBYTE lp_data, LPDWORD lp_cb)
{
    return return_value(REG_SZ, s, (DWORD)strlen(s) + 1, lp_type, lp_data, lp_cb);
}

// The values summonerfix.ini [Display] / [Sound] control; -1 when name is not one of them.
static LONG query_config_value(const char *sub, const char *name, LPDWORD lp_type, LPBYTE lp_data, LPDWORD lp_cb)
{
    if (sub[0] != 0 || !name)
        return -1;
    read_size();
    if (!_stricmp(name, "Resolution Width"))
        return return_dword((DWORD)reg_width, lp_type, lp_data, lp_cb);
    if (!_stricmp(name, "Resolution Height"))
        return return_dword((DWORD)reg_height, lp_type, lp_data, lp_cb);
    if (!_stricmp(name, "Selected Video Card"))
        return return_dword(0, lp_type, lp_data, lp_cb);
    if (!_stricmp(name, "Video Card 0")) {
        // The game picks Direct3D only for a card string with "Direct 3D" and not "3DFX"; anything else selects
        // Glide's renderer number, which the rebuilt builds' REBUILT_OPENGL turns into OpenGL.
        char r[32];
        modern_ini_str("Display", "Renderer", "OpenGL", r, sizeof r);
        const char *card = !_stricmp(r, "Direct3D") ? "Direct 3D (SummonerFix)"
                         : !_stricmp(r, "Glide")    ? "3DFX Glide (SummonerFix)"
                                                    : "OpenGL (SummonerFix)";
        return return_string(card, lp_type, lp_data, lp_cb);
    }
    if (!_stricmp(name, "Soundcard")) {
        char snd[64];
        modern_ini_str("Sound", "Soundcard", "DirectSound", snd, sizeof snd);
        return return_string(snd, lp_type, lp_data, lp_cb);
    }
    if (!_stricmp(name, "InstallPath"))
        return return_string(modern_dir(), lp_type, lp_data, lp_cb);
    return -1;
}

static LONG query_stored_value(const char *key, LPDWORD lp_type, LPBYTE lp_data, LPDWORD lp_cb)
{
    char raw[2100];
    modern_ini_str("Registry", key, "", raw, sizeof raw);
    if (!raw[0])
        return ERROR_FILE_NOT_FOUND;
    if (!strncmp(raw, "dword:", 6))
        return return_dword(strtoul(raw + 6, NULL, 16), lp_type, lp_data, lp_cb);
    if (!strncmp(raw, "sz:", 3))
        return return_string(raw + 3, lp_type, lp_data, lp_cb);
    if (!strncmp(raw, "hex:", 4)) {
        BYTE buf[1024];
        DWORD n = 0;
        for (const char *p = raw + 4; p[0] && p[1] && n < sizeof buf; p += 2) {
            unsigned b;
            if (sscanf(p, "%2x", &b) != 1)
                break;
            buf[n++] = (BYTE)b;
        }
        return return_value(REG_BINARY, buf, n, lp_type, lp_data, lp_cb);
    }
    return ERROR_FILE_NOT_FOUND;
}

LONG WINAPI modern_RegOpenKeyExA(HKEY h, LPCSTR sub, DWORD opt, REGSAM sam, PHKEY out)
{
    char rel[256];
    if (!modern_host() || !resolve_game_key(h, sub, rel, sizeof rel))
        return RegOpenKeyExA(h, sub, opt, sam, out);
    HKEY f = make_fake(rel);
    if (!f)
        return ERROR_NOT_ENOUGH_MEMORY;
    *out = f;
    return ERROR_SUCCESS;
}

LONG WINAPI modern_RegCreateKeyExA(HKEY h, LPCSTR sub, DWORD res, LPSTR cls, DWORD opt, REGSAM sam,
                                   LPSECURITY_ATTRIBUTES sa, PHKEY out, LPDWORD disp)
{
    char rel[256];
    if (!modern_host() || !resolve_game_key(h, sub, rel, sizeof rel))
        return RegCreateKeyExA(h, sub, res, cls, opt, sam, sa, out, disp);
    HKEY f = make_fake(rel);
    if (!f)
        return ERROR_NOT_ENOUGH_MEMORY;
    *out = f;
    if (disp)
        *disp = REG_OPENED_EXISTING_KEY;
    return ERROR_SUCCESS;
}

LONG WINAPI modern_RegQueryValueExA(HKEY h, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD cb)
{
    int fi = fake_index(h);
    if (fi < 0)
        return RegQueryValueExA(h, name, reserved, type, data, cb);
    char key[256];
    store_name(fi, name, key, sizeof key);
    LONG r = query_config_value(fake[fi].sub, name, type, data, cb);
    const char *src = "ini [Display]/[Sound]";
    if (r == -1) {
        r = query_stored_value(key, type, data, cb);
        src = "ini [Registry]";
    }
    modern_log("reg query '%s' -> %s (%s)", key, r == ERROR_SUCCESS ? "ok" : r == ERROR_FILE_NOT_FOUND ? "not set" : "error",
               src);
    return r;
}

LONG WINAPI modern_RegSetValueExA(HKEY h, LPCSTR name, DWORD reserved, DWORD type, const BYTE *data, DWORD cb)
{
    int fi = fake_index(h);
    if (fi < 0)
        return RegSetValueExA(h, name, reserved, type, data, cb);
    char key[256], val[2100];
    store_name(fi, name, key, sizeof key);
    if (type == REG_DWORD && cb >= 4) {
        _snprintf(val, sizeof val, "dword:%08lX", (unsigned long)*(const DWORD *)data);
    } else if (type == REG_SZ || type == REG_EXPAND_SZ) {
        _snprintf(val, sizeof val, "sz:%.*s", (int)(cb ? cb - 1 : 0), (const char *)data);
    } else {
        int n = _snprintf(val, sizeof val, "hex:");
        for (DWORD i = 0; i < cb && n < (int)sizeof val - 3; i++)
            n += _snprintf(val + n, sizeof val - (size_t)n, "%02x", data[i]);
    }
    val[sizeof val - 1] = 0;
    WritePrivateProfileStringA("Registry", key, val, modern_ini_path());
    modern_log("reg set '%s' = %s", key, val);
    return ERROR_SUCCESS;
}

LONG WINAPI modern_RegDeleteValueA(HKEY h, LPCSTR name)
{
    int fi = fake_index(h);
    if (fi < 0)
        return RegDeleteValueA(h, name);
    char key[256];
    store_name(fi, name, key, sizeof key);
    WritePrivateProfileStringA("Registry", key, NULL, modern_ini_path());
    return ERROR_SUCCESS;
}

LONG WINAPI modern_RegDeleteKeyA(HKEY h, LPCSTR sub)
{
    char rel[256];
    if (!modern_host() || !resolve_game_key(h, sub, rel, sizeof rel))
        return RegDeleteKeyA(h, sub);
    return ERROR_SUCCESS;   // nothing to delete: the values live in the ini
}

LONG WINAPI modern_RegCloseKey(HKEY h)
{
    int fi = fake_index(h);
    if (fi < 0)
        return RegCloseKey(h);
    fake[fi].used = false;
    return ERROR_SUCCESS;
}

#endif
