// registry.cpp - Sum.exe v1.40, code 0x511d80-0x5124b5 (vsdk\os\registry.cpp)
// Written from the Sum.exe disassembly (build/sum.asm). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// ADVAPI32 imports (IAT 0x57e000..): RegCreateKeyExA 0x57e000, RegQueryValueExA 0x57e004,
// RegDeleteKeyA 0x57e008, RegCloseKey 0x57e00c, RegSetValueExA 0x57e010, RegOpenKeyExA 0x57e014,
// RegDeleteValueA 0x57e018.
//
// Every error path sprintf's a message into a local buffer and never prints it (the print was
// probably a debug-only macro); the sprintf calls are kept because they are in the binary.

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "vsdk/os/registry.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/vstring.h"

// Original __FILE__ of this unit (the DebugError call pushes this literal, 0x5a75b4).
#define REGISTRY_CPP "D:\\projects\\Summoner\\pccode\\vsdk\\os\\registry.cpp"

// ---- .bss, in address order ----------------------------------------------------------------
char os_registry_app_key[256];          // 0x2692ef8
bool os_registry_app_key_set;           // 0x2692ff8  (written as a byte)
// lpClass arguments of the two RegCreateKeyExA calls: two separate zero-filled 4-byte .bss
// objects, each referenced once. Their names and exact type are unknown (an empty class string).
static char os_registry_write_class[4];  // 0x2692ffc  os_registry_write
static char os_registry_delete_class[4]; // 0x2693000  os_registry_delete

// 0x511d80  Confidence: exact (stack slot order of the four locals; the two failing sprintf
// calls share one call site, which VC6 does by tail merging).
int os_registry_write(const char *subkey, const char *name, DWORD type, const void *data, DWORD size)
{
    HKEY hkey;
    DWORD disposition;
    char error[256];
    char path[1024];
    int rval = 0;

    hkey = NULL;
    HKEY root = os_registry_resolve_key(path, subkey);
    if (RegCreateKeyExA(root, path, 0, os_registry_write_class, 0, KEY_ALL_ACCESS, NULL, &hkey,
                        &disposition) != ERROR_SUCCESS) {
        sprintf(error, "Error opening registry key '%s'\n", path);
        rval = -1;
    } else if (RegSetValueExA(hkey, name, 0, type, (const BYTE *)data, size) != ERROR_SUCCESS) {
        sprintf(error, "Error writing registry key '%s'\n", name);
        rval = -1;
    }

    if (hkey)
        RegCloseKey(hkey);
    return rval;
}

// 0x511e50  Confidence: likely (if/else nesting: the NULL case is laid out last, the "hkey"
// case inline, the relative case in the middle; the copy loops are written with indexes because
// the first one addresses name[] as (name - subkey) + p).
// The 256-byte name buffer is cleared with rep stosd using the 0 left in EAX by _strnicmp.
// Known defect: an "HKEY_xxx" string without a backslash copies from past its terminator.
HKEY os_registry_resolve_key(char *out_path, const char *subkey)
{
    HKEY root = HKEY_CURRENT_USER;

    if (subkey != NULL) {
        if (_strnicmp(subkey, "hkey", 4) == 0) {
            char name[256];
            int i, j;

            memset(name, 0, sizeof(name));
            for (i = 0; subkey[i] != '\0' && subkey[i] != '\\'; i++)
                name[i] = subkey[i];
            root = os_registry_hkey_from_string(name);
            i++;
            for (j = 0; subkey[i] != '\0'; i++, j++)
                out_path[j] = subkey[i];
            out_path[j] = '\0';
            return root;
        }
        strcpy(out_path, os_registry_app_key);
        strcat(out_path, "\\");
        strcat(out_path, subkey);
        return root;
    }

    strcpy(out_path, os_registry_app_key);
    return root;
}

// 0x511f90  Confidence: exact. Error line 185 (0xb9). The loop after sprintf is the error macro
// (DebugError never returns, but the compiler does not know it).
HKEY os_registry_hkey_from_string(const char *s)
{
    char error[256];

    if (_stricmp(s, "HKEY_CLASSES_ROOT") == 0)
        return HKEY_CLASSES_ROOT;
    if (_stricmp(s, "HKEY_CURRENT_USER") == 0)
        return HKEY_CURRENT_USER;
    if (_stricmp(s, "HKEY_LOCAL_MACHINE") == 0)
        return HKEY_LOCAL_MACHINE;
    if (_stricmp(s, "HKEY_USERS") == 0)
        return HKEY_USERS;

    sprintf(error, "Bogus HKEY_ string: %s", s);
    for (;;)
        DebugError(REGISTRY_CPP, 185, error);
}

// 0x512050  Confidence: exact (rval starts at -1 and only the success path clears it: XOR ESI
// at 0x5120f3; size is the by-value argument whose address is passed as lpcbData).
int os_registry_read(const char *subkey, const char *name, void *data, DWORD size)
{
    HKEY hkey;
    char error[256];
    char path[1024];
    int rval;

    hkey = NULL;
    rval = -1;
    HKEY root = os_registry_resolve_key(path, subkey);
    if (RegOpenKeyExA(root, path, 0, KEY_QUERY_VALUE, &hkey) != ERROR_SUCCESS) {
        sprintf(error, "Error opening registry key '%s'\n", path);
    } else if (RegQueryValueExA(hkey, name, NULL, NULL, (BYTE *)data, &size) != ERROR_SUCCESS) {
        sprintf(error, "Couldn't find registry key '%s', using default value.\n", name);
    } else {
        rval = 0;
    }

    if (hkey)
        RegCloseKey(hkey);
    return rval;
}

// 0x512110  Confidence: exact. Called by LoadSettings (company "Volition", app "Summoner").
int os_registry_set_app_key(const char *company, const char *app)
{
    strcpy(os_registry_app_key, "Software\\");
    if (company)
        strcat(os_registry_app_key, company);
    if (app) {
        strcat(os_registry_app_key, "\\");
        strcat(os_registry_app_key, app);
    }
    os_registry_app_key_set = true;
    return 0;
}

// 0x5121c0  Confidence: exact. No callers in Sum.exe.
int os_registry_read_String(const char *subkey, const char *name, String *out)
{
    char buf[1024];

    if (os_registry_read(subkey, name, buf, sizeof(buf)) == 0) {
        *out = buf;
        return 0;
    }
    return -1;
}

// 0x512210  Confidence: exact. Callers nw_init_networking, mp_charselect_init.
int os_registry_read_string_nodefault(const char *subkey, const char *name, char *out)
{
    char buf[1024];

    if (os_registry_read(subkey, name, buf, sizeof(buf)) == 0) {
        strcpy(out, buf);
        return 0;
    }
    return -1;
}

// 0x512280  Confidence: exact. No return value: EAX is left as the strcpy length on both paths.
void os_registry_read_string(const char *subkey, const char *name, char *out, const char *default_value)
{
    char buf[1024];

    if (os_registry_read(subkey, name, buf, sizeof(buf)) == 0)
        strcpy(out, buf);
    else
        strcpy(out, default_value);
}

// 0x512310  Confidence: exact. The size is strlen(s), without the terminator.
int os_registry_write_string(const char *subkey, const char *name, const char *s)
{
    return os_registry_write(subkey, name, REG_SZ, s, strlen(s));
}

// 0x512340  Confidence: exact. The success path returns the 0 still in EAX.
int os_registry_read_dword(const char *subkey, const char *name, DWORD *out, DWORD default_value)
{
    if (os_registry_read(subkey, name, out, 4) == 0)
        return 0;
    *out = default_value;
    return -1;
}

// 0x512370  Confidence: exact.
int os_registry_write_dword(const char *subkey, const char *name, DWORD value)
{
    return os_registry_write(subkey, name, REG_DWORD, &value, 4);
}

// 0x512390  Confidence: exact. Same code as os_registry_read_dword.
int os_registry_read_int(const char *subkey, const char *name, int *out, int default_value)
{
    if (os_registry_read(subkey, name, out, 4) == 0)
        return 0;
    *out = default_value;
    return -1;
}

// 0x5123c0  Confidence: exact. No callers in Sum.exe.
int os_registry_write_binary_int(const char *subkey, const char *name, int value)
{
    return os_registry_write(subkey, name, REG_BINARY, &value, 4);
}

// 0x5123e0  Confidence: exact (same layout risks as os_registry_write). No callers in Sum.exe.
// name == NULL deletes the key itself. Known defect: that branch formats its error with name,
// which is NULL there (the binary pushes EDI = name), not with the path.
int os_registry_delete(const char *subkey, const char *name)
{
    HKEY hkey;
    DWORD disposition;
    char error[256];
    char path[1024];
    int rval;

    hkey = NULL;
    rval = 0;
    HKEY root = os_registry_resolve_key(path, subkey);
    if (name == NULL) {
        if (RegDeleteKeyA(root, path) != ERROR_SUCCESS) {
            sprintf(error, "Error removing registry key '%s'\n", name);
            rval = -1;
        }
    } else if (RegCreateKeyExA(root, path, 0, os_registry_delete_class, 0, KEY_ALL_ACCESS, NULL,
                               &hkey, &disposition) != ERROR_SUCCESS) {
        sprintf(error, "Error opening registry key '%s'\n", path);
        rval = -1;
    } else if (RegDeleteValueA(hkey, name) != ERROR_SUCCESS) {
        sprintf(error, "Error removing registry value '%s'\n", name);
        rval = -1;
    }

    if (hkey)
        RegCloseKey(hkey);
    return rval;
}
