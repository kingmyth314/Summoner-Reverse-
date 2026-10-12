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

void TimingFixInstall(void)
{
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
