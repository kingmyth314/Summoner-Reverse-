// Logging, guarded memory patching and import-table hooking.
#include "summonerfix.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static char g_logPath[MAX_PATH];

void LogInit(const char *path)
{
    lstrcpynA(g_logPath, path, MAX_PATH);
    DeleteFileA(g_logPath);
}

void Log(const char *fmt, ...)
{
    char line[1024];
    SYSTEMTIME t;
    GetLocalTime(&t);
    int n = _snprintf(line, sizeof line, "%02d:%02d:%02d.%03d ", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf(line + n, sizeof line - n - 3, fmt, ap);
    va_end(ap);
    line[sizeof line - 3] = 0;
    strcat(line, "\r\n");

    HANDLE f = CreateFileA(g_logPath, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE)
        return;
    DWORD written;
    WriteFile(f, line, (DWORD)strlen(line), &written, NULL);
    CloseHandle(f);
}

DWORD OrigVa(DWORD addr)
{
    if (g_rebuilt)
        return RebuiltOrigVa(addr);
    return addr - (DWORD)(UINT_PTR)GetModuleHandleA(NULL) + SUM_IMAGE_BASE;
}

DWORD SumVa(DWORD va)
{
    if (g_rebuilt)
        return RebuiltVa(va);
    return va - SUM_IMAGE_BASE + (DWORD)(UINT_PTR)GetModuleHandleA(NULL);
}

// Every range PatchBytes changed, and ranges a hook recognises its caller by (KeepOriginal), so the hybrid loader
// (hybrid.c) leaves the functions they are in original.
static struct { DWORD va, n; } *g_patches;
static int g_numPatches, g_maxPatches;

void KeepOriginal(DWORD va, DWORD n)
{
    if (g_numPatches == g_maxPatches) {
        int max = g_maxPatches ? g_maxPatches * 2 : 1024;
        void *p = g_patches ? HeapReAlloc(GetProcessHeap(), 0, g_patches, max * sizeof *g_patches)
                            : HeapAlloc(GetProcessHeap(), 0, max * sizeof *g_patches);
        if (!p)
            return;
        g_patches = p;
        g_maxPatches = max;
    }
    g_patches[g_numPatches].va = va;
    g_patches[g_numPatches].n = n;
    g_numPatches++;
}

// The first patched address in [va, va + n), or 0.
DWORD PatchedWithin(DWORD va, DWORD n)
{
    for (int i = 0; i < g_numPatches; i++)
        if (g_patches[i].va < va + n && va < g_patches[i].va + g_patches[i].n)
            return g_patches[i].va > va ? g_patches[i].va : va;
    return 0;
}

// Write `repl` over `n` bytes at a Sum.exe address, but only if the current bytes equal `expect`.
// A mismatch means a different game build (or another patch) and the change is skipped.
// what == NULL: bulk patch, only failures are logged.
BOOL PatchBytes(DWORD va, const void *expect, const void *repl, SIZE_T n, const char *what)
{
    BYTE *p = (BYTE *)(UINT_PTR)SumVa(va);
    const char *quiet = what;
    if (!what)
        what = "(bulk patch)";
    if (memcmp(p, expect, n) != 0) {
        Log("SKIP %s: unexpected bytes at 0x%06lX", what, va);
        return FALSE;
    }
    DWORD old;
    if (!VirtualProtect(p, n, PAGE_EXECUTE_READWRITE, &old)) {
        Log("FAIL %s: VirtualProtect error %lu", what, GetLastError());
        return FALSE;
    }
    memcpy(p, repl, n);
    VirtualProtect(p, n, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, n);
    KeepOriginal(va, (DWORD)n);
    if (quiet)
        Log("patched %s at 0x%06lX (%u bytes)", what, va, (unsigned)n);
    return TRUE;
}

BOOL WriteDword(DWORD va, DWORD expect, DWORD value, const char *what)
{
    return PatchBytes(va, &expect, &value, 4, what);
}

// Replace one imported function in `module`'s import table. Returns the original pointer.
void *HookImport(HMODULE module, const char *dll, const char *func, void *replacement)
{
    BYTE *base = (BYTE *)module;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    IMAGE_DATA_DIRECTORY dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    IMAGE_IMPORT_DESCRIPTOR *imp = (IMAGE_IMPORT_DESCRIPTOR *)(base + dir.VirtualAddress);

    for (; imp->Name; imp++) {
        if (lstrcmpiA((char *)(base + imp->Name), dll) != 0)
            continue;
        IMAGE_THUNK_DATA *names = (IMAGE_THUNK_DATA *)(base + (imp->OriginalFirstThunk ? imp->OriginalFirstThunk
                                                                                        : imp->FirstThunk));
        IMAGE_THUNK_DATA *slots = (IMAGE_THUNK_DATA *)(base + imp->FirstThunk);
        for (; names->u1.AddressOfData; names++, slots++) {
            if (IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal))
                continue;
            IMAGE_IMPORT_BY_NAME *ibn = (IMAGE_IMPORT_BY_NAME *)(base + names->u1.AddressOfData);
            if (strcmp((char *)ibn->Name, func) != 0)
                continue;
            DWORD old;
            void *orig = (void *)(UINT_PTR)slots->u1.Function;
            VirtualProtect(&slots->u1.Function, sizeof(void *), PAGE_READWRITE, &old);
            slots->u1.Function = (UINT_PTR)replacement;
            VirtualProtect(&slots->u1.Function, sizeof(void *), old, &old);
            Log("hooked %s!%s (IAT slot 0x%06lX)", dll, func,
                (DWORD)((UINT_PTR)&slots->u1.Function - (UINT_PTR)base + SUM_IMAGE_BASE));
            return orig;
        }
    }
    Log("FAIL hook %s!%s: import not found", dll, func);
    return NULL;
}
