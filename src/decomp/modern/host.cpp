// host.cpp - the in-source host's settings, address table, save folder and log (include/modern/modern.h; rung L3b).
// Not original source; compiled only into MODERN_BUILD.
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "modern/modern.h"

static bool host_on;
static char host_dir[MAX_PATH];
static char host_ini[MAX_PATH];
static char host_log[MAX_PATH];
static bool host_test_saves;   // [Debug] TestSaves=1 without SummonerFix

// The address table: Sum.exe address, size, this exe's address against the preferred base 0x400000
// (tools/link-build.mjs writes <exe>.addr for the 32-bit and the x64 clang builds).
struct addr_row { unsigned int orig, size; unsigned long long va; };
static addr_row *host_rows;
static int host_num_rows;
static long long host_slide;   // the actual load address minus 0x400000 (ASLR may move a 64-bit exe)

void modern_log(const char *fmt, ...)
{
    char line[1024];
    SYSTEMTIME t;
    GetLocalTime(&t);
    int n = _snprintf(line, sizeof line, "%02d:%02d:%02d.%03d ", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
    va_list ap;
    va_start(ap, fmt);
    n += _vsnprintf(line + n, sizeof line - n - 3, fmt, ap);
    va_end(ap);
    if (n < 0 || n > (int)sizeof line - 3)
        n = (int)sizeof line - 3;
    line[n++] = '\r';
    line[n++] = '\n';
    HANDLE f = CreateFileA(host_log, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, 0, NULL);
    if (f != INVALID_HANDLE_VALUE) {
        DWORD wr;
        WriteFile(f, line, (DWORD)n, &wr, NULL);
        CloseHandle(f);
    }
}

static void load_address_table()
{
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char *dot = strrchr(path, '.');
    if (!dot)
        return;
    strcpy(dot, ".addr");
    FILE *f = fopen(path, "r");
    if (!f) {
        modern_log("modern host: no address table (%s); agent commands with Sum.exe addresses will fail", path);
        return;
    }
    int cap = 16384;
    host_rows = (addr_row *)malloc(cap * sizeof(addr_row));
    addr_row r;
    while (host_rows && fscanf(f, "%x %x %I64x", &r.orig, &r.size, &r.va) == 3) {   // %I64x: VC6's runtime too
        if (host_num_rows == cap)
            host_rows = (addr_row *)realloc(host_rows, (cap *= 2) * sizeof(addr_row));
        if (host_rows)
            host_rows[host_num_rows++] = r;
    }
    fclose(f);
    qsort(host_rows, host_num_rows, sizeof(addr_row), [](const void *a, const void *b) {
        unsigned int x = ((const addr_row *)a)->orig, y = ((const addr_row *)b)->orig;
        return x < y ? -1 : x > y;
    });
    host_slide = (long long)(size_t)GetModuleHandleA(NULL) - 0x400000;
    modern_log("modern host: %d addresses from %s", host_num_rows, path);
}

void *modern_sum_va(unsigned int sum_addr)
{
    size_t lo = 0, hi = (size_t)host_num_rows;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (host_rows[mid].orig <= sum_addr)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo == 0)
        return NULL;
    const addr_row &r = host_rows[lo - 1];
    if (sum_addr - r.orig >= (r.size ? r.size : 1))
        return NULL;
    return (void *)(size_t)((long long)r.va + host_slide + (sum_addr - r.orig));
}

void modern_host_init()
{
    char exe[MAX_PATH];
    GetModuleFileNameA(NULL, exe, MAX_PATH);
    lstrcpynA(host_dir, exe, MAX_PATH);
    char *slash = strrchr(host_dir, '\\');
    if (slash)
        *slash = 0;
    _snprintf(host_ini, MAX_PATH, "%s\\summonerfix.ini", host_dir);
    _snprintf(host_log, MAX_PATH, "%s\\summonerfix.log", host_dir);

    // SummonerFix is the dinput.dll beside the exe; Windows loads the system one for a 64-bit exe (it skips the
    // 32-bit DLL) or when there is none.
    char dll[MAX_PATH] = "";
    HMODULE di = GetModuleHandleA("dinput.dll");
    if (di)
        GetModuleFileNameA(di, dll, MAX_PATH);
    char *dslash = strrchr(dll, '\\');
    host_on = !(dslash && (size_t)(dslash - dll) == strlen(host_dir) && _strnicmp(dll, host_dir, dslash - dll) == 0);
    if (!host_on)
        return;

    DeleteFileA(host_log);
    modern_log("modern host (no SummonerFix): %s, %d-bit", exe, (int)sizeof(void *) * 8);
    load_address_table();
    if (modern_ini_int("Debug", "TestSaves", 0)) {
        char dir[MAX_PATH];
        _snprintf(dir, MAX_PATH, "%s\\savetest", host_dir);
        CreateDirectoryA(dir, NULL);
        host_test_saves = true;
        modern_log("modern host: save folder -> savetest (TestSaves)");
    }
}

bool modern_host() { return host_on; }
const char *modern_dir() { return host_dir; }
// "savetest" with [Debug] TestSaves=1 without SummonerFix, else NULL: the caller keeps the original's "savegame"
// literal, the one string SummonerFix's TestSaves finds and changes when it is loaded (a second copy in this unit
// took its search away).
const char *modern_save_folder() { return host_test_saves ? "savetest" : NULL; }

int modern_ini_int(const char *section, const char *key, int def)
{
    return (int)GetPrivateProfileIntA(section, key, def, host_ini);
}

void modern_ini_str(const char *section, const char *key, const char *def, char *out, int size)
{
    GetPrivateProfileStringA(section, key, def, out, (DWORD)size, host_ini);
}
