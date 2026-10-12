// Diagnostics ([Debug] Diagnostics=1): log error dialogs, process exits and very large allocations,
// each with the Sum.exe return address of the call, so failures can be traced to game code.
#include "summonerfix.h"
#include <stdio.h>
#include <string.h>

#define BIG_ALLOC (64u * 1024u * 1024u)

typedef int(WINAPI *PFN_MessageBoxA)(HWND, LPCSTR, LPCSTR, UINT);
typedef void(WINAPI *PFN_ExitProcess)(UINT);
typedef LPVOID(WINAPI *PFN_HeapAlloc)(HANDLE, DWORD, SIZE_T);
typedef LPVOID(WINAPI *PFN_VirtualAlloc)(LPVOID, SIZE_T, DWORD, DWORD);
typedef void(WINAPI *PFN_GlobalMemoryStatus)(LPMEMORYSTATUS);

static PFN_MessageBoxA real_MessageBoxA;
static PFN_ExitProcess real_ExitProcess;
static PFN_HeapAlloc real_HeapAlloc;
static PFN_VirtualAlloc real_VirtualAlloc;
static PFN_GlobalMemoryStatus real_GlobalMemoryStatus;

#define CALLER() ((DWORD)(UINT_PTR)__builtin_return_address(0) - (DWORD)(UINT_PTR)GetModuleHandleA(NULL) + SUM_IMAGE_BASE)

// Heuristic stack trace: scan the stack for values that point into Sum.exe's .text right after a
// CALL instruction (E8 rel32, FF 15 [mem], FF D0-D7 reg, FF 50-57 disp8, FF 90-97 disp32).
static void LogStackTrace(const void *sp)
{
    const DWORD textLo = SumVa(0x401000), textHi = SumVa(0x57e000);
    const DWORD *p = (const DWORD *)sp;
    char line[900];
    int n = _snprintf(line, sizeof line, "  stack:");
    int found = 0;
    for (int i = 0; i < 1024 && found < 24; i++) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery(p + i, &mbi, sizeof mbi) || mbi.State != MEM_COMMIT)
            break;
        DWORD v = p[i];
        if (v < textLo + 8 || v >= textHi)
            continue;
        const BYTE *c = (const BYTE *)(UINT_PTR)v;
        BOOL isCall = c[-5] == 0xE8 || (c[-6] == 0xFF && c[-5] == 0x15) || (c[-2] == 0xFF && (c[-1] & 0xF8) == 0xD0) ||
                      (c[-3] == 0xFF && (c[-2] & 0xF8) == 0x50) || (c[-6] == 0xFF && (c[-5] & 0xF8) == 0x90);
        if (!isCall)
            continue;
        n += _snprintf(line + n, sizeof line - n, " 0x%06lX", v - SumVa(SUM_IMAGE_BASE) + SUM_IMAGE_BASE);
        found++;
    }
    Log("%s", line);
}

static int WINAPI Diag_MessageBoxA(HWND w, LPCSTR text, LPCSTR caption, UINT type)
{
    Log("MessageBox from 0x%06lX: [%s] %s", CALLER(), caption ? caption : "", text ? text : "");
    LogStackTrace(__builtin_frame_address(0));
    return real_MessageBoxA(w, text, caption, type);
}

static void WINAPI Diag_ExitProcess(UINT code)
{
    Log("ExitProcess(%u) from 0x%06lX", code, CALLER());
    LogStackTrace(__builtin_frame_address(0));
    real_ExitProcess(code);
}

static LPVOID WINAPI Diag_HeapAlloc(HANDLE heap, DWORD flags, SIZE_T n)
{
    LPVOID p = real_HeapAlloc(heap, flags, n);
    if (n >= BIG_ALLOC) {
        Log("HeapAlloc(%lu bytes) from 0x%06lX -> %p", (unsigned long)n, CALLER(), p);
        LogStackTrace(__builtin_frame_address(0));
    }
    return p;
}

static LPVOID WINAPI Diag_VirtualAlloc(LPVOID addr, SIZE_T n, DWORD type, DWORD prot)
{
    LPVOID p = real_VirtualAlloc(addr, n, type, prot);
    if (n >= BIG_ALLOC) {
        Log("VirtualAlloc(%lu bytes, type 0x%lX) from 0x%06lX -> %p", (unsigned long)n, type, CALLER(), p);
        LogStackTrace(__builtin_frame_address(0));
    }
    return p;
}

static void WINAPI Diag_GlobalMemoryStatus(LPMEMORYSTATUS ms)
{
    real_GlobalMemoryStatus(ms);
    Log("GlobalMemoryStatus from 0x%06lX: totalPhys %lu, availPhys %lu, totalVirtual %lu", CALLER(),
        (unsigned long)ms->dwTotalPhys, (unsigned long)ms->dwAvailPhys, (unsigned long)ms->dwTotalVirtual);
}

void DiagInstall(HMODULE exe)
{
    real_MessageBoxA = HookImport(exe, "USER32.dll", "MessageBoxA", Diag_MessageBoxA);
    real_ExitProcess = HookImport(exe, "KERNEL32.dll", "ExitProcess", Diag_ExitProcess);
    real_HeapAlloc = HookImport(exe, "KERNEL32.dll", "HeapAlloc", Diag_HeapAlloc);
    real_VirtualAlloc = HookImport(exe, "KERNEL32.dll", "VirtualAlloc", Diag_VirtualAlloc);
    real_GlobalMemoryStatus = HookImport(exe, "KERNEL32.dll", "GlobalMemoryStatus", Diag_GlobalMemoryStatus);
}
