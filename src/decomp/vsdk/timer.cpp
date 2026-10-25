// timer.cpp - Sum.exe v1.40, code 0x532820-0x5329d1 (sum.symbols module timer.cpp, Summoner 2's file name;
// the folder is not known). Written from the Sum.exe disassembly (tools/asm.mjs; 0x580e00 = 0.25 read from
// the image). Target: VC++ 6.0 (cl 12.00.8966), /O2; timer_get and read_tsc hold __asm blocks, so they keep
// an EBP frame and their locals in memory. Confidence per function: exact (control flow, calls, constants and
// globals match), likely (one detail of statement order or expression form is a guess), unsure (the shape
// itself is a guess).
//
// The QueryPerformanceCounter clock (timer_get(scale): elapsed ticks * scale / frequency) and the TSC rate.
// .bss 0x2cbffa8-0x2cbffdf (after the free-movement unit's, before the vif particles 0x2cbffe0). No .data.
// The time formatter before it (0x532660) is a placeholder unit of its own (D5316); read_tsc (0x5329b1) and
// the TSC conversion (0x5329c1, no sum.symbols entry) follow timer_get without alignment padding.
// As in unnamed_floatmath_0x542500.cpp, the __asm blocks have a non-MSVC branch for the mingw syntax check.
//
// Imports: KERNEL32 QueryPerformanceFrequency, QueryPerformanceCounter, InitializeCriticalSection,
// Enter/Leave/DeleteCriticalSection, Sleep.
//
// Header: vsdk/os/timer.h.

#include <windows.h>
#include <stdlib.h>
#include "vsdk/os/timer.h"

// ---- .bss (0x2cbffa8-0x2cbffdf) ------------------------------------------------------------------------
__int64 timer_qpc_last;                              // 0x2cbffa8
int timer_tsc_ticks;                                 // 0x2cbffb0 TSC ticks in 250 ms (only the conversion below reads it)
CRITICAL_SECTION timer_lock;                         // 0x2cbffb8
__int64 timer_qpc_base;                              // 0x2cbffd0
bool timer_initialized;                              // 0x2cbffd8
unsigned int timer_qpc_freq_low32;                   // 0x2cbffdc the low dword of the QPC frequency
#ifdef FIXED_BUILD
// part of FIXED_BUILD G2: the counter and the frequency are divided by 2^timer_qpc_shift (timer_init).
static int timer_qpc_shift;
#endif

// 0x532820  Confidence: exact. atexit handler of timer_init.
void timer_close()
{
    if (timer_initialized) {
        timer_initialized = false;
        DeleteCriticalSection(&timer_lock);
    }
}

// 0x532840  Confidence: exact (the flag stored after the tick count). Only the low dword of the
// frequency is kept (a counter above 4 GHz breaks it; see NOTES). The TSC is read three times 250 ms apart;
// the middle reading is not used, so timer_tsc_ticks is the ticks in 500 ms halved.
void timer_init()
{
    LARGE_INTEGER freq;
    int tsc;

    if (timer_initialized)
        return;
    QueryPerformanceFrequency(&freq);
#ifdef FIXED_BUILD
    // FIXED_BUILD: G2 QpcFrequency. A frequency of 2^32 Hz or more lost its high dword (the clock then ran many
    // times too fast). Scale the counter and the frequency down by the same power of two until it fits.
    timer_qpc_shift = 0;
    while ((freq.QuadPart >> timer_qpc_shift) > 0xffffffffi64)
        timer_qpc_shift++;
    timer_qpc_freq_low32 = (unsigned int)(freq.QuadPart >> timer_qpc_shift);
    QueryPerformanceCounter((LARGE_INTEGER *)&timer_qpc_base);
    timer_qpc_base >>= timer_qpc_shift;
    QueryPerformanceCounter((LARGE_INTEGER *)&timer_qpc_last);
    timer_qpc_last >>= timer_qpc_shift;
#else
    timer_qpc_freq_low32 = freq.LowPart;
    QueryPerformanceCounter((LARGE_INTEGER *)&timer_qpc_base);
    QueryPerformanceCounter((LARGE_INTEGER *)&timer_qpc_last);
#endif
    InitializeCriticalSection(&timer_lock);
    tsc = read_tsc();
    Sleep(250);
    read_tsc();
    Sleep(250);
    timer_tsc_ticks = (read_tsc() - tsc) / 2;
    timer_initialized = true;
    atexit(timer_close);
}

#pragma optimize("", off)  // timer_get is unoptimised code in Sum.exe (D8401)

// 0x5328d0  Confidence: exact (unoptimised, D8401; locals named and declared for the /Od frame). The copy of the counter into the elapsed time on the backwards path is a
// dead store in the code; its source form is a guess). A counter that went backwards becomes the new base.
// The 64-bit product is divided by the 32-bit frequency in __asm: the high dword is reduced by subtracting
// the frequency until it goes negative (an endless loop when the frequency is 0), so only the low 32 bits of
// the quotient come back.
int timer_get(int scale)
{
    __int64 ticks;
    __int64 elapsed;
    LARGE_INTEGER reading;
    int result;

    if (!timer_initialized)
        return 0;
    EnterCriticalSection(&timer_lock);
    QueryPerformanceCounter(&reading);
#ifdef FIXED_BUILD
    // part of FIXED_BUILD G2: the same scale as the frequency (timer_init).
    reading.QuadPart >>= timer_qpc_shift;
#endif
    if (reading.QuadPart < timer_qpc_last) {
        timer_qpc_base = reading.QuadPart;
        elapsed = reading.QuadPart;
    }
    elapsed = reading.QuadPart - timer_qpc_base;
    timer_qpc_last = reading.QuadPart;
    ticks = elapsed;
    LeaveCriticalSection(&timer_lock);
    ticks = ticks * scale;
#if defined(_MSC_VER) && !defined(MODERN_BUILD)
    __asm {
        mov edx, dword ptr [ticks + 4]
        mov eax, dword ptr [ticks]
        mov ebx, timer_qpc_freq_low32
    reduce:
        sub edx, ebx
        jns reduce
        add edx, ebx
        div ebx
        mov result, eax
    }
#else
    // The same value: the low 32 bits of the quotient (the modern build; for the mingw syntax check too).
    result = (int)(unsigned int)((unsigned __int64)ticks / timer_qpc_freq_low32);
#endif
    return result;
}
#pragma optimize("", on)

// 0x5329b1  Confidence: exact. The low dword of the time stamp counter.
int read_tsc()
{
    int t;

#if defined(_MSC_VER) && !defined(MODERN_BUILD)
    __asm {
        rdtsc
        mov t, eax
    }
#elif defined(MODERN_BUILD)
    t = (int)(unsigned int)__builtin_ia32_rdtsc();   // the modern build has no __asm (L3)
#else
    t = 0;                                           // (syntax check only)
#endif
    return t;
}

// 0x5329c1  Confidence: exact (no sum.symbols entry and no caller: name and use ours). TSC ticks to seconds.
float timer_tsc_to_seconds(int ticks)
{
    return (float)ticks / timer_tsc_ticks * 0.25f;
}
