// Hybrid game: run decompiled functions in place of the original ones (the link experiment, docs/decomp-plan.md).
// tools/hybrid-build.mjs compiles src/decomp files with Visual C++ 6.0 into a DLL at a fixed base, exporting each
// decompiled function as f_<Sum.exe address>_<size>; every other name in it already points at Sum.exe. This loads
// that DLL and writes a jmp at each address, so the game calls the decompiled function instead. A function that a
// SummonerFix patch (PatchBytes) changed anywhere stays original, so no fix or hook is lost.
//   [Hybrid] Dll   the DLL, relative to the game folder (empty: off)
//            Skip  addresses to leave original, comma separated (0x408620,0x408ba0)
//            Only  a file (relative to the game folder) listing the only addresses to replace, any separators:
//                  for finding which function breaks something, with one DLL
#include "summonerfix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// [Hybrid] Coverage=1: every replaced function is reached through a counting stub (inc [counter]; jmp function);
// HybridWriteCoverage (agent command hybridcov) writes the counts.
static BYTE *g_covStubs;
static DWORD *g_covCounts, *g_covAddr;
static DWORD g_covCount;

// "0xADDRESS calls" for every replaced function, to a file. Returns the number of functions that ran, or -1.
int HybridWriteCoverage(const char *path)
{
    if (!g_covCount)
        return -1;
    HANDLE f = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE)
        return -1;
    int ran = 0;
    char line[32];
    for (DWORD i = 0; i < g_covCount; i++) {
        DWORD n = g_covCounts[i], written;
        if (n)
            ran++;
        int len = _snprintf(line, sizeof line, "0x%06lx %lu\r\n", g_covAddr[i], n);
        WriteFile(f, line, len, &written, NULL);
    }
    CloseHandle(f);
    return ran;
}

// The whole text of a file, or NULL.
static char *ReadText(const char *path)
{
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE)
        return NULL;
    DWORD size = GetFileSize(f, NULL), got = 0;
    char *text = HeapAlloc(GetProcessHeap(), 0, size + 1);
    if (text && ReadFile(f, text, size, &got, NULL))
        text[got] = 0;
    else if (text) {
        HeapFree(GetProcessHeap(), 0, text);
        text = NULL;
    }
    CloseHandle(f);
    return text;
}

static BOOL Skipped(const char *skip, DWORD va)
{
    for (const char *p = skip; *p;) {
        char *end;
        DWORD v = strtoul(p, &end, 16);
        if (end == p) {
            p++;
            continue;
        }
        if (v == va)
            return TRUE;
        p = end;
    }
    return FALSE;
}

void HybridInstall(void)
{
    char rel[MAX_PATH], path[MAX_PATH], skip[1024];
    GetPrivateProfileStringA("Hybrid", "Dll", "", rel, sizeof rel, g_cfg.iniPath);
    if (!rel[0])
        return;
    GetPrivateProfileStringA("Hybrid", "Skip", "", skip, sizeof skip, g_cfg.iniPath);
    char *only = NULL;
    GetPrivateProfileStringA("Hybrid", "Only", "", path, sizeof path, g_cfg.iniPath);
    if (path[0]) {
        char full[MAX_PATH];
        _snprintf(full, MAX_PATH, "%s\\%s", g_cfg.installPath, path);
        full[MAX_PATH - 1] = 0;
        if (!(only = ReadText(full)))
            Log("hybrid: cannot read the Only list %s: replacing nothing", full);
        if (!only)
            return;
    }
    _snprintf(path, MAX_PATH, "%s\\%s", g_cfg.installPath, rel);
    path[MAX_PATH - 1] = 0;

    // The DLL has no imports, no entry point and no relocations (it must load at its own base), so loading it here
    // runs no code of its own.
    HMODULE dll = LoadLibraryA(path);
    if (!dll) {
        DWORD err = GetLastError();
        Log("hybrid: cannot load %s (error %lu; its base address may be taken)", path, err);
        // Say what is in the way: the DLL's preferred base, from its header.
        HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (f != INVALID_HANDLE_VALUE) {
            BYTE head[1024];
            DWORD got = 0;
            ReadFile(f, head, sizeof head, &got, NULL);
            CloseHandle(f);
            LONG lfanew = got >= 0x40 ? *(LONG *)(head + 0x3c) : 0;
            if (lfanew > 0 && lfanew + 0x38 < (LONG)got) {
                DWORD want = *(DWORD *)(head + lfanew + 0x34);
                MEMORY_BASIC_INFORMATION mbi;
                char owner[MAX_PATH] = "";
                if (VirtualQuery((void *)(UINT_PTR)want, &mbi, sizeof mbi)) {
                    GetModuleFileNameA((HMODULE)mbi.AllocationBase, owner, MAX_PATH);
                    Log("hybrid: at 0x%08lX: state 0x%lX, allocation 0x%08lX, size 0x%lX %s", want, mbi.State,
                        (DWORD)(UINT_PTR)mbi.AllocationBase, (DWORD)mbi.RegionSize, owner);
                }
            }
        }
        return;
    }
    BYTE *base = (BYTE *)dll;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    IMAGE_DATA_DIRECTORY dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!dir.Size) {
        Log("hybrid: %s exports nothing", path);
        return;
    }
    IMAGE_EXPORT_DIRECTORY *ex = (IMAGE_EXPORT_DIRECTORY *)(base + dir.VirtualAddress);
    DWORD *names = (DWORD *)(base + ex->AddressOfNames), *funcs = (DWORD *)(base + ex->AddressOfFunctions);
    WORD *ords = (WORD *)(base + ex->AddressOfNameOrdinals);
    int done = 0, skipped = 0;
    BOOL coverage = GetPrivateProfileIntA("Hybrid", "Coverage", 0, g_cfg.iniPath);
    if (coverage) {
        g_covStubs = VirtualAlloc(NULL, ex->NumberOfNames * 16, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        g_covCounts = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ex->NumberOfNames * sizeof *g_covCounts);
        g_covAddr = HeapAlloc(GetProcessHeap(), 0, ex->NumberOfNames * sizeof *g_covAddr);
        if (!g_covStubs || !g_covCounts || !g_covAddr) {
            Log("hybrid: no memory for the coverage counters: coverage off");
            coverage = FALSE;
        }
    }
    for (DWORD i = 0; i < ex->NumberOfNames; i++) {
        const char *name = (const char *)(base + names[i]);
        if (strncmp(name, "f_", 2) != 0)
            continue;
        // f_<address>_<size>: the size is the original's, up to the next function.
        char *end;
        DWORD va = strtoul(name + 2, &end, 16);
        DWORD size = *end == '_' ? strtoul(end + 1, NULL, 16) : 5;
        BYTE *target = base + funcs[ords[i]];
        if (va < SUM_IMAGE_BASE + 0x1000 || va >= 0x57db40) {
            Log("hybrid: %s is not a Sum.exe code address", name);
            continue;
        }
        if (Skipped(skip, va) || (only && !Skipped(only, va))) {
            skipped++;
            continue;
        }
        // A SummonerFix patch inside the original (a fix, a hook) would be lost: that one stays original.
        DWORD patched = PatchedWithin(va, size);
        if (patched) {
            Log("hybrid: 0x%06lX stays original: SummonerFix patches it at 0x%06lX", va, patched);
            skipped++;
            continue;
        }
        BYTE *at = (BYTE *)(UINT_PTR)SumVa(va);
        // Coverage: the jmp goes to a stub that counts the call, then jumps on to the decompiled function.
        if (coverage && g_covCount < ex->NumberOfNames) {
            BYTE *stub = g_covStubs + g_covCount * 16;
            DWORD *counter = &g_covCounts[g_covCount];
            g_covAddr[g_covCount++] = va;
            stub[0] = 0xFF; // inc dword ptr [counter]
            stub[1] = 0x05;
            *(DWORD *)(stub + 2) = (DWORD)(UINT_PTR)counter;
            stub[6] = 0xE9; // jmp target
            *(DWORD *)(stub + 7) = (DWORD)(target - (stub + 11));
            target = stub;
        }
        BYTE jmp[5] = {0xE9};
        *(DWORD *)(jmp + 1) = (DWORD)(target - (at + 5));
        DWORD old;
        if (!VirtualProtect(at, 5, PAGE_EXECUTE_READWRITE, &old)) {
            Log("hybrid: VirtualProtect at 0x%06lX failed (%lu)", va, GetLastError());
            continue;
        }
        memcpy(at, jmp, 5);
        VirtualProtect(at, 5, old, &old);
        done++;
    }
    FlushInstructionCache(GetCurrentProcess(), NULL, 0);
    Log("hybrid: %s at 0x%08lX, %d functions replaced, %d left original", path, (DWORD)(UINT_PTR)base, done, skipped);
}
