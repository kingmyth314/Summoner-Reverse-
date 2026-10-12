// Registry emulation for Software\Volition\Summoner.
//
// Sum.exe reads its video/sound settings from this key (see docs/sum-exe/video-settings.md).
// The original launcher writes them, but it cannot detect modern video cards, so we answer the
// game's registry calls ourselves: display/sound values come from summonerfix.ini [Display] and
// [Sound]; anything else the game writes is persisted in summonerfix.ini [Registry].
// All other registry keys pass through to the real registry untouched.
#include "summonerfix.h"
#include <stdio.h>
#include <string.h>

#define FAKE_BASE 0x5EED0000u
#define MAX_FAKE 16

typedef LONG(WINAPI *PFN_RegOpenKeyExA)(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
typedef LONG(WINAPI *PFN_RegCreateKeyExA)(HKEY, LPCSTR, DWORD, LPSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY,
                                         LPDWORD);
typedef LONG(WINAPI *PFN_RegQueryValueExA)(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef LONG(WINAPI *PFN_RegSetValueExA)(HKEY, LPCSTR, DWORD, DWORD, const BYTE *, DWORD);
typedef LONG(WINAPI *PFN_RegDeleteValueA)(HKEY, LPCSTR);
typedef LONG(WINAPI *PFN_RegDeleteKeyA)(HKEY, LPCSTR);
typedef LONG(WINAPI *PFN_RegCloseKey)(HKEY);

static PFN_RegOpenKeyExA real_RegOpenKeyExA;
static PFN_RegCreateKeyExA real_RegCreateKeyExA;
static PFN_RegQueryValueExA real_RegQueryValueExA;
static PFN_RegSetValueExA real_RegSetValueExA;
static PFN_RegDeleteValueA real_RegDeleteValueA;
static PFN_RegDeleteKeyA real_RegDeleteKeyA;
static PFN_RegCloseKey real_RegCloseKey;

static struct {
    BOOL used;
    char sub[128]; // path below Software\Volition\Summoner, "" for the key itself
} g_fake[MAX_FAKE];

static int FakeIndex(HKEY h)
{
    UINT_PTR v = (UINT_PTR)h;
    if (v >= FAKE_BASE && v < FAKE_BASE + MAX_FAKE && g_fake[v - FAKE_BASE].used)
        return (int)(v - FAKE_BASE);
    return -1;
}

// If `parent`+`subKey` names Software\Volition\Summoner (or below), return the part below it.
static BOOL ResolveGameKey(HKEY parent, LPCSTR subKey, char *out, size_t outLen)
{
    const char *s = subKey ? subKey : "";
    int pi = FakeIndex(parent);
    if (pi >= 0) {
        _snprintf(out, outLen, "%s%s%s", g_fake[pi].sub, (g_fake[pi].sub[0] && s[0]) ? "\\" : "", s);
        out[outLen - 1] = 0;
        return TRUE;
    }
    static const char marker[] = "volition\\summoner";
    char lower[512];
    lstrcpynA(lower, s, sizeof lower);
    CharLowerA(lower);
    const char *hit = strstr(lower, marker);
    if (!hit)
        return FALSE;
    const char *rest = s + (hit - lower) + (sizeof marker - 1);
    if (*rest == '\\')
        rest++;
    lstrcpynA(out, rest, (int)outLen);
    return TRUE;
}

static HKEY MakeFake(const char *sub)
{
    for (int i = 0; i < MAX_FAKE; i++) {
        if (!g_fake[i].used) {
            g_fake[i].used = TRUE;
            lstrcpynA(g_fake[i].sub, sub, sizeof g_fake[i].sub);
            return (HKEY)(UINT_PTR)(FAKE_BASE + i);
        }
    }
    return NULL;
}

static void StoreName(int fi, LPCSTR name, char *out, size_t outLen)
{
    _snprintf(out, outLen, "%s%s%s", g_fake[fi].sub, g_fake[fi].sub[0] ? "\\" : "", name ? name : "");
    out[outLen - 1] = 0;
}

static LONG ReturnValue(DWORD type, const void *data, DWORD len, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    if (lpType)
        *lpType = type;
    if (!lpcbData)
        return lpData ? ERROR_INVALID_PARAMETER : ERROR_SUCCESS;
    DWORD cap = *lpcbData;
    *lpcbData = len;
    if (!lpData)
        return ERROR_SUCCESS;
    if (cap < len)
        return ERROR_MORE_DATA;
    memcpy(lpData, data, len);
    return ERROR_SUCCESS;
}

static LONG ReturnDword(DWORD v, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    return ReturnValue(REG_DWORD, &v, sizeof v, lpType, lpData, lpcbData);
}

static LONG ReturnString(const char *s, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    return ReturnValue(REG_SZ, s, (DWORD)strlen(s) + 1, lpType, lpData, lpcbData);
}

// Values controlled by summonerfix.ini [Display]/[Sound]. Returns -1 if `name` is not one of them.
static LONG QueryConfigValue(const char *sub, const char *name, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    if (sub[0] != 0 || !name)
        return -1;
    if (!lstrcmpiA(name, "Resolution Width"))
        return ReturnDword((DWORD)g_cfg.width, lpType, lpData, lpcbData);
    if (!lstrcmpiA(name, "Resolution Height"))
        return ReturnDword((DWORD)g_cfg.height, lpType, lpData, lpcbData);
    if (!lstrcmpiA(name, "Selected Video Card"))
        return ReturnDword(0, lpType, lpData, lpcbData);
    if (!lstrcmpiA(name, "Video Card 0")) {
        // The game picks Direct3D only for a card string containing "Direct 3D" and not "3DFX";
        // anything else selects renderer 0x67 (Glide), which the DLL redirects to OpenGL when asked.
        const char *card = g_cfg.renderer == RENDERER_DIRECT3D ? "Direct 3D (SummonerFix)"
                           : g_cfg.renderer == RENDERER_GLIDE  ? "3DFX Glide (SummonerFix)"
                                                               : "OpenGL (SummonerFix)";
        return ReturnString(card, lpType, lpData, lpcbData);
    }
    if (!lstrcmpiA(name, "Soundcard"))
        return ReturnString(g_cfg.soundcard, lpType, lpData, lpcbData);
    if (!lstrcmpiA(name, "InstallPath"))
        return ReturnString(g_cfg.installPath, lpType, lpData, lpcbData);
    return -1;
}

// Persisted values are stored as "dword:XXXXXXXX", "sz:text" or "hex:0a1b..." in [Registry].
static LONG QueryStoredValue(const char *key, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    char raw[2100];
    if (!GetPrivateProfileStringA("Registry", key, "", raw, sizeof raw, g_cfg.iniPath))
        return ERROR_FILE_NOT_FOUND;
    if (!strncmp(raw, "dword:", 6))
        return ReturnDword(strtoul(raw + 6, NULL, 16), lpType, lpData, lpcbData);
    if (!strncmp(raw, "sz:", 3))
        return ReturnString(raw + 3, lpType, lpData, lpcbData);
    if (!strncmp(raw, "hex:", 4)) {
        BYTE buf[1024];
        DWORD n = 0;
        for (const char *p = raw + 4; p[0] && p[1] && n < sizeof buf; p += 2) {
            unsigned b;
            if (sscanf(p, "%2x", &b) != 1)
                break;
            buf[n++] = (BYTE)b;
        }
        return ReturnValue(REG_BINARY, buf, n, lpType, lpData, lpcbData);
    }
    return ERROR_FILE_NOT_FOUND;
}

static LONG WINAPI Hook_RegOpenKeyExA(HKEY h, LPCSTR sub, DWORD opt, REGSAM sam, PHKEY out)
{
    char rel[256];
    if (!ResolveGameKey(h, sub, rel, sizeof rel))
        return real_RegOpenKeyExA(h, sub, opt, sam, out);
    HKEY f = MakeFake(rel);
    if (!f)
        return ERROR_NOT_ENOUGH_MEMORY;
    *out = f;
    return ERROR_SUCCESS;
}

static LONG WINAPI Hook_RegCreateKeyExA(HKEY h, LPCSTR sub, DWORD res, LPSTR cls, DWORD opt, REGSAM sam,
                                        LPSECURITY_ATTRIBUTES sa, PHKEY out, LPDWORD disp)
{
    char rel[256];
    if (!ResolveGameKey(h, sub, rel, sizeof rel))
        return real_RegCreateKeyExA(h, sub, res, cls, opt, sam, sa, out, disp);
    HKEY f = MakeFake(rel);
    if (!f)
        return ERROR_NOT_ENOUGH_MEMORY;
    *out = f;
    if (disp)
        *disp = REG_OPENED_EXISTING_KEY;
    return ERROR_SUCCESS;
}

static LONG WINAPI Hook_RegQueryValueExA(HKEY h, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data,
                                         LPDWORD cb)
{
    int fi = FakeIndex(h);
    if (fi < 0)
        return real_RegQueryValueExA(h, name, reserved, type, data, cb);
    char key[256];
    StoreName(fi, name, key, sizeof key);
    LONG r = QueryConfigValue(g_fake[fi].sub, name, type, data, cb);
    const char *src = "ini [Display]/[Sound]";
    if (r == -1) {
        r = QueryStoredValue(key, type, data, cb);
        src = "ini [Registry]";
    }
    Log("reg query '%s' -> %s (%s)", key, r == ERROR_SUCCESS ? "ok" : r == ERROR_FILE_NOT_FOUND ? "not set" : "error",
        src);
    return r;
}

static LONG WINAPI Hook_RegSetValueExA(HKEY h, LPCSTR name, DWORD reserved, DWORD type, const BYTE *data, DWORD cb)
{
    int fi = FakeIndex(h);
    if (fi < 0)
        return real_RegSetValueExA(h, name, reserved, type, data, cb);
    char key[256], val[2100];
    StoreName(fi, name, key, sizeof key);
    if (type == REG_DWORD && cb >= 4) {
        _snprintf(val, sizeof val, "dword:%08lX", *(const DWORD *)data);
    } else if (type == REG_SZ || type == REG_EXPAND_SZ) {
        _snprintf(val, sizeof val, "sz:%.*s", (int)(cb ? cb - 1 : 0), (const char *)data);
    } else {
        int n = _snprintf(val, sizeof val, "hex:");
        for (DWORD i = 0; i < cb && n < (int)sizeof val - 3; i++)
            n += _snprintf(val + n, sizeof val - n, "%02x", data[i]);
    }
    val[sizeof val - 1] = 0;
    WritePrivateProfileStringA("Registry", key, val, g_cfg.iniPath);
    Log("reg set '%s' = %s", key, val);
    return ERROR_SUCCESS;
}

static LONG WINAPI Hook_RegDeleteValueA(HKEY h, LPCSTR name)
{
    int fi = FakeIndex(h);
    if (fi < 0)
        return real_RegDeleteValueA(h, name);
    char key[256];
    StoreName(fi, name, key, sizeof key);
    WritePrivateProfileStringA("Registry", key, NULL, g_cfg.iniPath);
    return ERROR_SUCCESS;
}

static LONG WINAPI Hook_RegDeleteKeyA(HKEY h, LPCSTR sub)
{
    char rel[256];
    if (!ResolveGameKey(h, sub, rel, sizeof rel))
        return real_RegDeleteKeyA(h, sub);
    return ERROR_SUCCESS; // nothing to delete: values live in the ini
}

static LONG WINAPI Hook_RegCloseKey(HKEY h)
{
    int fi = FakeIndex(h);
    if (fi < 0)
        return real_RegCloseKey(h);
    g_fake[fi].used = FALSE;
    return ERROR_SUCCESS;
}

void RegHookInstall(HMODULE exe)
{
    real_RegOpenKeyExA = HookImport(exe, "ADVAPI32.dll", "RegOpenKeyExA", Hook_RegOpenKeyExA);
    real_RegCreateKeyExA = HookImport(exe, "ADVAPI32.dll", "RegCreateKeyExA", Hook_RegCreateKeyExA);
    real_RegQueryValueExA = HookImport(exe, "ADVAPI32.dll", "RegQueryValueExA", Hook_RegQueryValueExA);
    real_RegSetValueExA = HookImport(exe, "ADVAPI32.dll", "RegSetValueExA", Hook_RegSetValueExA);
    real_RegDeleteValueA = HookImport(exe, "ADVAPI32.dll", "RegDeleteValueA", Hook_RegDeleteValueA);
    real_RegDeleteKeyA = HookImport(exe, "ADVAPI32.dll", "RegDeleteKeyA", Hook_RegDeleteKeyA);
    real_RegCloseKey = HookImport(exe, "ADVAPI32.dll", "RegCloseKey", Hook_RegCloseKey);
}
