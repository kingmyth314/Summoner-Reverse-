// Running inside the standalone rebuilt exe (tools/link-build.mjs, Sum-rebuilt.exe). Its code and data sit at other
// addresses than Sum.exe's, so SummonerFix only works there through an address table: <exe name>.addr next to the
// exe (written by link-build.mjs: Sum.exe address, size, rebuilt address of every symbol both builds know, hex).
// SumVa translates through it; an address inside a known symbol keeps its offset. A Sum.exe address no symbol covers
// translates to 0 (logged once per address). Only the parts that need no code patches are installed in this mode
// (main.c): PatchBytes compares the original bytes first, so a patch site that differs in the rebuilt code is refused.
#include "summonerfix.h"

typedef struct { DWORD orig, size, now; } AddrRow;
static AddrRow *g_rows;
static int g_numRows;
BOOL g_rebuilt;

BOOL RebuiltInit(void)
{
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char *dot = strrchr(path, '.');
    if (!dot)
        return FALSE;
    strcpy(dot, ".addr");
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE)
        return FALSE;
    DWORD size = GetFileSize(f, NULL), got = 0;
    char *text = (char *)HeapAlloc(GetProcessHeap(), 0, size + 1);
    ReadFile(f, text, size, &got, NULL);
    CloseHandle(f);
    text[got] = 0;
    int lines = 0;
    for (char *p = text; *p; p++)
        if (*p == '\n')
            lines++;
    g_rows = (AddrRow *)HeapAlloc(GetProcessHeap(), 0, (lines + 1) * sizeof(AddrRow));
    for (char *p = text; *p;) {
        AddrRow r;
        char *end;
        r.orig = strtoul(p, &end, 16);
        r.size = strtoul(end, &end, 16);
        r.now = strtoul(end, &end, 16);
        if (end != p && r.orig)
            g_rows[g_numRows++] = r; // the file is sorted by Sum.exe address
        while (*end && *end != '\n')
            end++;
        p = *end ? end + 1 : end;
    }
    HeapFree(GetProcessHeap(), 0, text);
    g_rebuilt = g_numRows > 0;
    Log("rebuilt exe: %d addresses from %s", g_numRows, path);
    return g_rebuilt;
}

DWORD RebuiltVa(DWORD va)
{
    int lo = 0, hi = g_numRows - 1, best = -1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (g_rows[mid].orig <= va) {
            best = mid;
            lo = mid + 1;
        } else
            hi = mid - 1;
    }
    if (best >= 0 && va - g_rows[best].orig < g_rows[best].size)
        return g_rows[best].now + (va - g_rows[best].orig);
    static DWORD logged[64];
    static int numLogged;
    for (int i = 0; i < numLogged; i++)
        if (logged[i] == va)
            return 0;
    if (numLogged < 64)
        logged[numLogged++] = va;
    Log("rebuilt exe: no address for Sum.exe 0x%06lX", va);
    return 0;
}

// The reverse: an address in the rebuilt exe -> the Sum.exe address of the same symbol plus the same offset (kept
// inside the original symbol: rebuilt functions can be longer). For the hooks that recognise their caller by its
// return address. 0 when no symbol covers it.
static AddrRow *g_byNow;

static int CompareNow(const void *a, const void *b)
{
    DWORD x = ((const AddrRow *)a)->now, y = ((const AddrRow *)b)->now;
    return x < y ? -1 : x > y;
}

DWORD RebuiltOrigVa(DWORD now)
{
    if (!g_byNow) {
        AddrRow *t = (AddrRow *)HeapAlloc(GetProcessHeap(), 0, g_numRows * sizeof(AddrRow));
        memcpy(t, g_rows, g_numRows * sizeof(AddrRow));
        qsort(t, g_numRows, sizeof(AddrRow), CompareNow);
        g_byNow = t;
    }
    int lo = 0, hi = g_numRows - 1, best = -1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (g_byNow[mid].now <= now) {
            best = mid;
            lo = mid + 1;
        } else
            hi = mid - 1;
    }
    if (best < 0)
        return 0;
    DWORD off = now - g_byNow[best].now;
    if (best + 1 < g_numRows && now >= g_byNow[best + 1].now)
        return 0;
    if (off >= g_byNow[best].size)
        off = g_byNow[best].size ? g_byNow[best].size - 1 : 0;
    return g_byNow[best].orig + off;
}
