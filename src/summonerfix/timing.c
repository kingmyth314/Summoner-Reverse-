// Frame-limiter fix (game ran at 2x speed on fast PCs).
//
// FrameTimer (FUN_00501510) measures dt = now - last and stores last = now *before* sleeping. When a
// frame is shorter than MIN_DT it sleeps the remainder and reports dt = MIN_DT, but the next frame's
// dt then includes that same sleep again. On hardware where frames take far less than MIN_DT this
// alternates "sleep, count MIN_DT" / "no sleep, count the sleep": two frames of game time per
// MIN_DT of real time, i.e. double speed. Observed 2026-10-12: ~218 FrameTimer calls/s,
// ~112 sleeps/s of ~7.8 ms each, game believing ~115 FPS.
//
// Fix: replace the limiter's `CALL SleepMs` (0x5015bc) with FrameSleep, which waits precisely until
// last + MIN_DT and then sets last to the time the wait ended.
#include "summonerfix.h"
#include <string.h>

#define VA_FRAMETIMER_SLEEP_CALL 0x5015bcu
#define VA_SLEEPMS 0x530bf0u
#define VA_FRAMECLOCKREAD 0x5328d0u // int __cdecl FrameClockRead(int scale): elapsed * scale / QPF
#define VA_LAST_US 0x257f098u       // FrameTimer's previous clock reading (microseconds)
#define VA_MIN_DT 0x5a574cu         // float seconds

typedef int(__cdecl *PFN_FrameClockRead)(int);
typedef HANDLE(WINAPI *PFN_CreateWaitableTimerExW)(LPSECURITY_ATTRIBUTES, LPCWSTR, DWORD, DWORD);

static PFN_FrameClockRead g_clock;
static HANDLE g_timer;

static void WaitMicroseconds(int us)
{
    if (g_timer) {
        LARGE_INTEGER due;
        due.QuadPart = -(LONGLONG)us * 10; // relative, 100 ns units
        if (SetWaitableTimer(g_timer, &due, 0, NULL, NULL, FALSE)) {
            WaitForSingleObject(g_timer, INFINITE);
            return;
        }
    }
    Sleep((DWORD)(us / 1000));
}

static void __cdecl FrameSleep(DWORD requestedMs)
{
    // FrameTimer has already stored last = now (this frame's start) before calling us, so the
    // previous frame's end time is kept here instead.
    static int s_end;
    static BOOL s_valid;
    volatile int *last = (volatile int *)(UINT_PTR)SumVa(VA_LAST_US);
    float minDt = *(volatile float *)(UINT_PTR)SumVa(VA_MIN_DT);
    int minUs = (int)(minDt * 1000000.0f);
    int start = *last;
    int target;
    if (s_valid && start - s_end >= 0 && start - s_end < minUs)
        target = s_end + minUs;                  // steady state: exactly one MIN_DT after the last frame
    else
        target = start + (int)requestedMs * 1000; // first frame or after a slow frame
    for (;;) {
        int remain = target - g_clock(1000000); // wrap-safe signed difference
        if (remain <= 0)
            break;
        if (remain > 1500)
            WaitMicroseconds(remain - 1000); // coarse wait, then finish by yielding
        else
            SwitchToThread();
    }
    // Continue from the target time, not the (slightly later) wake-up time: the overshoot is then
    // counted in the next frame instead of being lost, so game time tracks real time exactly.
    // After a long stall (overshoot of a whole frame or more) resynchronise instead.
    int now = g_clock(1000000);
    s_end = (now - target) < minUs ? target : now;
    s_valid = TRUE;
    *last = s_end;
}

// Cutscene timer fix (cutscenes "at mach speed" after ~24.9 days of Windows uptime).
//
// SequenceTimer (0x436a90) subtracts its start time with a signed 32-bit FISUB, so once timeGetTime()
// passes 2^31 ms the start reads as about -2.1e9 and every sequence event looks overdue. Fix: return
// time since start-up instead. GetMessageTime is rebased by the same amount because 0x509880
// computes timeGetTime() - GetMessageTime() for key timing, and both must keep a common epoch.
typedef DWORD(WINAPI *PFN_timeGetTime)(void);
typedef LONG(WINAPI *PFN_GetMessageTime)(void);
static PFN_timeGetTime real_timeGetTime;
static PFN_GetMessageTime real_GetMessageTime;
static DWORD g_timeBase;

static DWORD WINAPI Hook_timeGetTime(void)
{
    return real_timeGetTime() - g_timeBase;
}

static LONG WINAPI Hook_GetMessageTime(void)
{
    return (LONG)((DWORD)real_GetMessageTime() - g_timeBase);
}

static void CutsceneTimerFixInstall(void)
{
    HMODULE exe = GetModuleHandleA(NULL);
    real_timeGetTime = (PFN_timeGetTime)HookImport(exe, "WINMM.dll", "timeGetTime", (void *)Hook_timeGetTime);
    if (!real_timeGetTime)
        return;
    real_GetMessageTime = (PFN_GetMessageTime)HookImport(exe, "USER32.dll", "GetMessageTime", (void *)Hook_GetMessageTime);
    if (!real_GetMessageTime)
        real_GetMessageTime = GetMessageTime;
    // Start one second in, so the rebased clock is never 0 (some callers treat 0 as "not started").
    // [Debug] FakeUptimeDays=N instead starts the clock at N days, to reproduce the original bug.
    DWORD fakeDays = (DWORD)GetPrivateProfileIntA("Debug", "FakeUptimeDays", 0, g_cfg.iniPath);
    DWORD now = real_timeGetTime();
    g_timeBase = now - 1000 - fakeDays * 86400000u;
    Log("cutscene timer: timeGetTime rebased to start-up (Windows uptime %.1f days%s)", now / 86400000.0,
        now >= 0x80000000u ? ", past the 24.9-day limit: the fix is needed now" : "");
    if (fakeDays)
        Log("cutscene timer: FakeUptimeDays=%lu, testing only: the game sees %lu days of uptime", fakeDays, fakeDays);
}

void TimingFixInstall(void)
{
    CutsceneTimerFixInstall();
    g_clock = (PFN_FrameClockRead)(UINT_PTR)SumVa(VA_FRAMECLOCKREAD);

    // High-resolution waitable timer (Windows 10 1803+); otherwise fall back to Sleep.
    PFN_CreateWaitableTimerExW create =
        (PFN_CreateWaitableTimerExW)GetProcAddress(GetModuleHandleA("kernel32.dll"), "CreateWaitableTimerExW");
    if (create)
        g_timer = create(NULL, NULL, 0x2 /* CREATE_WAITABLE_TIMER_HIGH_RESOLUTION */, TIMER_ALL_ACCESS);

    DWORD site = SumVa(VA_FRAMETIMER_SLEEP_CALL);
    BYTE orig[5] = {0xE8};
    DWORD rel = SumVa(VA_SLEEPMS) - (site + 5);
    memcpy(orig + 1, &rel, 4);
    BYTE repl[5] = {0xE8};
    rel = (DWORD)(UINT_PTR)FrameSleep - (site + 5);
    memcpy(repl + 1, &rel, 4);
    PatchBytes(VA_FRAMETIMER_SLEEP_CALL, orig, repl, 5, "frame limiter sleep -> FrameSleep");
    Log("frame limiter: high-resolution timer %s", g_timer ? "available" : "NOT available (using Sleep)");
}
