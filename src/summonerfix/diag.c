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

#define CALLER() OrigVa((DWORD)(UINT_PTR)__builtin_return_address(0))

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
        // Relative to the exe's own load address (SumVa(SUM_IMAGE_BASE) has no table entry in a rebuilt exe).
        n += _snprintf(line + n, sizeof line - n, " 0x%06lX",
                       v - (DWORD)(UINT_PTR)GetModuleHandleA(NULL) + SUM_IMAGE_BASE);
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

// Sleep statistics: the frame limiter sleeps through SleepMs (FUN_00530bf0 -> Sleep).
typedef void(WINAPI *PFN_Sleep)(DWORD);
static PFN_Sleep real_Sleep;
static volatile LONG g_sleepCalls, g_sleepAskedMs, g_sleepActualUs;
static LARGE_INTEGER g_qpf;

static void WINAPI Diag_Sleep(DWORD ms)
{
    LARGE_INTEGER a, b;
    QueryPerformanceCounter(&a);
    real_Sleep(ms);
    QueryPerformanceCounter(&b);
    InterlockedIncrement(&g_sleepCalls);
    InterlockedExchangeAdd(&g_sleepAskedMs, (LONG)ms);
    InterlockedExchangeAdd(&g_sleepActualUs, (LONG)((b.QuadPart - a.QuadPart) * 1000000 / g_qpf.QuadPart));
}

// Every 5 s, log the game's own frame statistics (FrameTimer globals, docs/sum-exe/main-loop.md):
// fps_avg 0x5a5744, frame_dt 0x5a5740, frame count 0x257f088.
static DWORD WINAPI FpsLoggerThread(LPVOID unused)
{
    (void)unused;
    DWORD lastFrames = 0, lastTick = GetTickCount();
    typedef int(__cdecl * PFN_Clock)(int);
    PFN_Clock gameClock = (PFN_Clock)(UINT_PTR)SumVa(0x5328d0);
    LARGE_INTEGER q0, q1, qf;
    QueryPerformanceFrequency(&qf);
    QueryPerformanceCounter(&q0);
    int c0 = gameClock(1000000);
    for (;;) {
        Sleep(5000);
        QueryPerformanceCounter(&q1);
        int c1 = gameClock(1000000);
        Log("clock check: game FrameClockRead %+.1f ms vs QPC %+.1f ms", (c1 - c0) / 1000.0,
            (double)(q1.QuadPart - q0.QuadPart) * 1000.0 / (double)qf.QuadPart);
        q0 = q1;
        c0 = c1;
        DWORD frames = *(volatile DWORD *)(UINT_PTR)SumVa(0x257f088);
        float fps = *(volatile float *)(UINT_PTR)SumVa(0x5a5744);
        float dt = *(volatile float *)(UINT_PTR)SumVa(0x5a5740);
        DWORD now = GetTickCount();
        double measured = (frames - lastFrames) * 1000.0 / (double)(now - lastTick);
        float raw = *(volatile float *)(UINT_PTR)SumVa(0x5a573c);
        LONG calls = InterlockedExchange(&g_sleepCalls, 0), asked = InterlockedExchange(&g_sleepAskedMs, 0),
             actual = InterlockedExchange(&g_sleepActualUs, 0);
        Log("frames %lu: measured %.1f FPS (game avg %.1f), raw_dt %.4f, frame_dt %.4f s; Sleep x%ld asked %ld ms, "
            "got %.1f ms",
            (unsigned long)frames, measured, (double)fps, (double)raw, (double)dt, calls, asked, actual / 1000.0);
        lastFrames = frames;
        lastTick = now;
    }
    return 0;
}

void FpsLoggerStart(void)
{
    CloseHandle(CreateThread(NULL, 0, FpsLoggerThread, NULL, 0, NULL));
}

// Access violations: log where they happen (the game's CRT turns them into a silent ExitProcess(0xC0000005)).
// First chance, so one the game would handle itself is logged too; at most 8 per run.
static LONG CALLBACK Diag_Exception(PEXCEPTION_POINTERS ep)
{
    static volatile LONG logged;
    const EXCEPTION_RECORD *er = ep->ExceptionRecord;
    if (er->ExceptionCode != EXCEPTION_ACCESS_VIOLATION || InterlockedIncrement(&logged) > 8)
        return EXCEPTION_CONTINUE_SEARCH;
    const CONTEXT *c = ep->ContextRecord;
    HMODULE mod = NULL;
    char name[MAX_PATH] = "?";
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCSTR)er->ExceptionAddress, &mod);
    if (mod)
        GetModuleFileNameA(mod, name, sizeof name);
    const char *base = strrchr(name, '\\');
    DWORD at = (DWORD)(UINT_PTR)er->ExceptionAddress;
    if (mod == GetModuleHandleA(NULL))
        at = at - (DWORD)(UINT_PTR)mod + SUM_IMAGE_BASE;
    else if (mod)
        at -= (DWORD)(UINT_PTR)mod;
    Log("access violation (%s 0x%08lX) at %s+0x%06lX: eax %08lX ebx %08lX ecx %08lX edx %08lX esi %08lX edi %08lX "
        "ebp %08lX esp %08lX",
        er->ExceptionInformation[0] ? "write" : "read", (unsigned long)er->ExceptionInformation[1],
        base ? base + 1 : name, (unsigned long)at, c->Eax, c->Ebx, c->Ecx, c->Edx, c->Esi, c->Edi, c->Ebp, c->Esp);
    LogStackTrace((const void *)(UINT_PTR)c->Esp);
    return EXCEPTION_CONTINUE_SEARCH;
}

void DiagInstall(HMODULE exe)
{
    real_MessageBoxA = HookImport(exe, "USER32.dll", "MessageBoxA", Diag_MessageBoxA);
    real_ExitProcess = HookImport(exe, "KERNEL32.dll", "ExitProcess", Diag_ExitProcess);
    real_HeapAlloc = HookImport(exe, "KERNEL32.dll", "HeapAlloc", Diag_HeapAlloc);
    real_VirtualAlloc = HookImport(exe, "KERNEL32.dll", "VirtualAlloc", Diag_VirtualAlloc);
    real_GlobalMemoryStatus = HookImport(exe, "KERNEL32.dll", "GlobalMemoryStatus", Diag_GlobalMemoryStatus);
    QueryPerformanceFrequency(&g_qpf);
    real_Sleep = HookImport(exe, "KERNEL32.dll", "Sleep", Diag_Sleep);
    AddVectoredExceptionHandler(1, Diag_Exception);
}
