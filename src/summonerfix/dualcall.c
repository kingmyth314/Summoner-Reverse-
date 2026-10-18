// Dual calls: test a decompiled function against the original on the game's own arguments (the hybrid game,
// [Hybrid] Dual=<file of addresses>, DualSamples=N). For each listed function the loader's jmp goes to a stub
// that enters DualRun. For the first N calls DualRun
//   1. copies the game's data (.data/.bss of Sum.exe), the caller's stack above the arguments and the C
//      runtime's rand state (holdrand, _getptd()+0x14);
//   2. calls the original function (its first 5 bytes put back for the call) with the caller's registers and
//      arguments, and keeps the result: eax, edx, st0 when the call left a value on the FPU stack, the bytes it
//      popped (stdcall / thiscall), the memory afterwards;
//   3. puts the copies back and calls the decompiled function the same way;
//   4. logs every difference (memory ranges, registers, rand state);
// then the game continues with the decompiled function's result. Later calls go to the decompiled function only.
// Not covered: heap memory, files and devices. A function that frees memory must not be listed (the original's
// free would be undone in the data but not in the heap).
#include "summonerfix.h"
#include <stdio.h>
#include <string.h>

#define VA_GETPTD 0x571b43u   // _getptd: the C runtime's per-thread data; holdrand at +0x14 (rand, 0x56d790)
#define DUAL_ARGS 12          // dwords of the caller's stack passed on (extra ones are harmless)
#define DUAL_STACK 0x1000     // bytes of the caller's stack copied and compared (from the arguments up)
#define DUAL_MAX 256
#define VA_SND_DISABLED 0x2cbf480u // every snd_* entry point returns early when set: no device state during the calls

// State outside the game's data that the two calls would see differently (a file opened, a texture uploaded by
// the first call): not compared. More from [Hybrid] DualIgnore=ADDR:LEN,... (hex).
static DWORD g_ignore[32][2] = {
    {0x2694068, 0x20100}, // file_slots: open files
    {0x5ad7ec, 4},        // gr_opengl_next_texture_id
    {0x2f13328, 8},       // gl_texture_mem_used, gl_texture_mem_frame
    {0x2f54720, 0x69450}, // ds_buffers .. snd_streams: the sound and music streaming the sound thread writes
    {0x2cbffa8, 0x30},    // timer_qpc_last .. timer_qpc_base: the frame clock reads the counter through
                          // SummonerFix's own import
    {0x22e48e4, 0x4c},    // ai_prof (averages), ai_prof_timer, ai_prof_*_us: the AI's timing counters (microseconds)
    // The C runtime's own state: its heap (small-block headers, the old heap, _crtheap), locks, FILE table and
    // atexit table. Putting it back after the original's call would rewind the heap's bookkeeping under blocks
    // that were allocated or freed meanwhile (on any thread).
    {0x5af4f0, 0x2cd0},   // crt_app_type .. crt_days (initialised data)
    {0x2fbdbc0, 0x1738},  // _environ .. the end of .bss (with _cflush, the sbh headers, __onexitend)
};
static int g_numIgnore = 8;

typedef struct {
    DWORD edi, esi, ebp, esp, ebx, edx, ecx, eax; // pushad order, lowest address first
} DualRegs;

typedef struct {
    DWORD eax, edx, popped, has_st0;
    BYTE st0[10];
} DualResult;

static struct {
    DWORD va;
    BYTE *dll;      // the decompiled function (or its coverage stub)
    BYTE orig[5];   // the original's first 5 bytes
    BYTE jmp[5];    // the loader's jmp
    BYTE stub[16];  // push i; jmp DualEntry
    char kind;      // what it returns (the list's second column, from hybrid.txt): v nothing, f st0, b al, s ax, i eax
    int calls, differ;
} g_dual[DUAL_MAX];

// Both calls see the same clock, so timers set from it and the AI's timing counters agree: during a check,
// QueryPerformanceCounter, timeGetTime and GetTickCount run a virtual clock that starts where the real one stood
// when the check began and advances with every read (the counter by one, the millisecond clocks by one per 256
// reads, so a time budget does not run out early; a wait loop still ends), restarted for the second call.
typedef BOOL(WINAPI *PFN_Qpc)(LARGE_INTEGER *);
typedef DWORD(WINAPI *PFN_Ticks)(void);
static PFN_Qpc g_realQpc;
static PFN_Ticks g_realTgt, g_realTick;
static volatile int g_freeze;
static LARGE_INTEGER g_startQpc;
static DWORD g_startTgt, g_startTick, g_steps;

static BOOL WINAPI Freeze_Qpc(LARGE_INTEGER *t)
{
    if (g_freeze && GetCurrentThreadId() == (DWORD)g_freeze) {
        t->QuadPart = g_startQpc.QuadPart + ++g_steps;
        return TRUE;
    }
    return g_realQpc(t);
}
static DWORD WINAPI Freeze_timeGetTime(void)
{
    return g_freeze && GetCurrentThreadId() == (DWORD)g_freeze ? g_startTgt + (++g_steps >> 8) : g_realTgt();
}
static DWORD WINAPI Freeze_GetTickCount(void)
{
    return g_freeze && GetCurrentThreadId() == (DWORD)g_freeze ? g_startTick + (++g_steps >> 8) : g_realTick();
}

// Start the virtual clock (first call) or restart it at the same point (second call).
static void FreezeTime(BOOL restart)
{
    if (!restart) {
        if (g_realQpc)
            g_realQpc(&g_startQpc);
        if (g_realTgt)
            g_startTgt = g_realTgt();
        if (g_realTick)
            g_startTick = g_realTick();
    }
    g_steps = 0;
    g_freeze = (int)GetCurrentThreadId();
}
static int g_numDual, g_samples;
static BYTE *g_pre, *g_post;
static BYTE *g_data;
static DWORD g_dataSize;
static int g_inDual;
__attribute__((used)) DualResult g_dualOut;

// Call target with ecx / edx / eax set and DUAL_ARGS argument dwords; fills *out.
void __cdecl DualCallWith(const void *target, const DualRegs *regs, const DWORD *args,
                          DualResult *out);
__asm__(".text\n"
        "_DualCallWith:\n"
        "    pushl %ebp\n"
        "    movl %esp, %ebp\n"
        "    pushl %ebx\n"
        "    pushl %esi\n"
        "    pushl %edi\n"
        "    subl $8, %esp\n"            // -0x14: FPU status before, -0x10: saved esp
        "    fnstsw -0x14(%ebp)\n"
        "    movl 0x10(%ebp), %esi\n"    // args
        "    movl %esp, -0x10(%ebp)\n"
        "    movl $12, %ecx\n"           // DUAL_ARGS
        "1:  pushl -4(%esi,%ecx,4)\n"
        "    loop 1b\n"
        "    movl 0xc(%ebp), %ebx\n"     // regs
        "    movl 0x18(%ebx), %ecx\n"
        "    movl 0x14(%ebx), %edx\n"
        "    movl 0x1c(%ebx), %eax\n"
        "    movl 0x10(%ebx), %ebx\n"    // the caller's ebx (callee-saved; some code passes values in it)
        "    call *0x8(%ebp)\n"
        "    movl 0x14(%ebp), %ebx\n"    // out
        "    movl %eax, 0x0(%ebx)\n"
        "    movl %edx, 0x4(%ebx)\n"
        "    movl -0x10(%ebp), %eax\n"
        "    subl %esp, %eax\n"          // bytes still pushed
        "    movl $48, %ecx\n"
        "    subl %eax, %ecx\n"          // popped by the callee
        "    movl %ecx, 0x8(%ebx)\n"
        "    movl -0x10(%ebp), %esp\n"
        "    movl $0, 0xc(%ebx)\n"
        "    fnstsw %ax\n"
        "    movw -0x14(%ebp), %cx\n"
        "    andw $0x3800, %ax\n"
        "    andw $0x3800, %cx\n"
        "    cmpw %ax, %cx\n"
        "    je 2f\n"
        "    fstpt 0x10(%ebx)\n"         // the value the call left in st0
        "    movl $1, 0xc(%ebx)\n"
        "2:  addl $8, %esp\n"
        "    popl %edi\n"
        "    popl %esi\n"
        "    popl %ebx\n"
        "    popl %ebp\n"
        "    ret\n");

__attribute__((used)) void __cdecl DualRun(int i, DualRegs *regs, DWORD *args);

// Entered from the stub with the function's index pushed: [esp] = i, [esp+4] = the caller's return address,
// [esp+8] = the arguments. Returns to the caller as the function would (eax, edx, st0, ret n).
void DualEntry(void);
__asm__(".text\n"
        "_DualEntry:\n"
        "    pushl %ebp\n"
        "    movl %esp, %ebp\n"          // [ebp+4] = i, [ebp+8] = return address, [ebp+12] = arguments
        "    pushal\n"
        "    movl %esp, %eax\n"          // DualRegs *
        "    leal 12(%ebp), %edx\n"      // arguments
        "    pushl %edx\n"
        "    pushl %eax\n"
        "    pushl 4(%ebp)\n"
        "    call _DualRun\n"
        "    addl $12, %esp\n"
        "    cmpl $0, _g_dualOut+12\n"
        "    je 1f\n"
        "    fldt _g_dualOut+16\n"
        "1:  popal\n"
        "    popl %ebp\n"
        "    movl _g_dualOut, %eax\n"
        "    movl _g_dualOut+4, %edx\n"
        "    movl 4(%esp), %ecx\n"       // the return address
        "    leal 8(%esp), %esp\n"       // drop the index and the return address
        "    addl _g_dualOut+8, %esp\n"  // and what the function pops itself
        "    jmp *%ecx\n");

static void Patch5(BYTE *at, const BYTE *bytes)
{
    DWORD old;
    VirtualProtect(at, 5, PAGE_EXECUTE_READWRITE, &old);
    memcpy(at, bytes, 5);
    VirtualProtect(at, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), at, 5);
}

static DWORD *Holdrand(void)
{
    BYTE *(*getptd)(void) = (BYTE * (*)(void))(UINT_PTR)SumVa(VA_GETPTD);
    return (DWORD *)(getptd() + 0x14);
}

// An 80-bit st0 value as text: the number and its 10 bytes (which = 0 or 1: two buffers for one log line).
static const char *St0Text(const BYTE *st0, int which)
{
    static char text[2][64];
    long double v;
    memcpy(&v, st0, 10);
    double d = (double)v;
    _snprintf(text[which], sizeof text[which], "%.9g (%02X%02X..%02X%02X)", d, st0[9], st0[8], st0[1], st0[0]);
    return text[which];
}

// Log the ranges where a and b differ (at most 8), as Sum.exe addresses from base; returns the byte count.
static DWORD LogRanges(const char *what, const BYTE *a, const BYTE *b, DWORD n, DWORD base)
{
    DWORD total = 0;
    int shown = 0;
    for (DWORD i = 0; i < n;) {
        if (a[i] == b[i]) {
            i++;
            continue;
        }
        DWORD s = i;
        while (i < n && (a[i] != b[i] || (i + 4 < n && memcmp(a + i, b + i, 4) != 0)))
            i++;
        total += i - s;
        if (shown++ < 8) {
            char ha[40] = "", hb[40] = "";
            for (DWORD k = 0; k < i - s && k < 12; k++) {
                sprintf(ha + k * 3, "%02X ", a[s + k]);
                sprintf(hb + k * 3, "%02X ", b[s + k]);
            }
            Log("dual:   %s 0x%08lX +%lu: original %s| decompiled %s", what, base + s, i - s, ha, hb);
        }
    }
    return total;
}

__attribute__((used)) void __cdecl DualRun(int i, DualRegs *regs, DWORD *args)
{
    BYTE *at = (BYTE *)(UINT_PTR)SumVa(g_dual[i].va);
    if (g_inDual || g_dual[i].calls >= g_samples) {
        DualCallWith(g_dual[i].dll, regs, args, &g_dualOut);
        return;
    }
    g_inDual = 1;
    int n = ++g_dual[i].calls;
    if (GetPrivateProfileIntA("Debug", "TraceDual", 0, g_cfg.iniPath))
        Log("dual: 0x%06lX call %d begins", g_dual[i].va, n);
    BYTE stackPre[DUAL_STACK], stackPost[DUAL_STACK];
    // The copied stack stops at the top of the thread's stack.
    MEMORY_BASIC_INFORMATION mbi;
    DWORD stackLen = DUAL_STACK;
    if (VirtualQuery(args, &mbi, sizeof mbi)) {
        DWORD room = (DWORD)((BYTE *)mbi.BaseAddress + mbi.RegionSize - (BYTE *)args);
        if (room < stackLen)
            stackLen = room;
    }
    DWORD *hold = Holdrand(), holdPre = *hold, holdOrig;
    DualResult orig;

    volatile BYTE *sndOff = (volatile BYTE *)(UINT_PTR)SumVa(VA_SND_DISABLED);
    BYTE sndWas = *sndOff;
    // The frame clock's state (timer_qpc_last .. timer_qpc_base): the calls run on the virtual clock, so it is put
    // back as it was when they are done, and the game's clock goes on from there.
    BYTE *timers = (BYTE *)(UINT_PTR)SumVa(0x2cbffa8), timerSave[0x30];
    memcpy(timerSave, timers, sizeof timerSave);
    FreezeTime(FALSE);
    memcpy(g_pre, g_data, g_dataSize);
    memcpy(stackPre, args, stackLen);
    *sndOff = 1;
    Patch5(at, g_dual[i].orig);
    DualCallWith(at, regs, args, &orig);
    Patch5(at, g_dual[i].jmp);
    memcpy(g_post, g_data, g_dataSize);
    memcpy(stackPost, args, stackLen);
    holdOrig = *hold;

    // Put the data back, but not the ignored ranges: the sound thread owns those, live.
    {
        DWORD from = 0;
        for (;;) {
            DWORD next = g_dataSize, len = 0;
            for (int k = 0; k < g_numIgnore; k++) {
                DWORD s = g_ignore[k][0] - 0x588000;
                if (s >= from && s < next)
                    next = s, len = g_ignore[k][1];
            }
            memcpy(g_data + from, g_pre + from, next - from);
            if (next >= g_dataSize)
                break;
            from = next + len;
        }
    }
    memcpy(args, stackPre, stackLen);
    *hold = holdPre;
    *sndOff = 1;
    FreezeTime(TRUE);
    DualCallWith(g_dual[i].dll, regs, args, &g_dualOut);
    // A still copy of the data after the decompiled call (the sound thread keeps writing the live data), into
    // the buffer the restore no longer needs; the two copies are compared.
    memcpy(g_pre, g_data, g_dataSize);
    g_freeze = 0;
    memcpy(timers, timerSave, sizeof timerSave); // the game's clock state as before the check
    *sndOff = sndWas;
    BYTE *after = g_pre;
    g_post[VA_SND_DISABLED - 0x588000] = after[VA_SND_DISABLED - 0x588000];
    for (int k = 0; k < g_numIgnore; k++) // outside state: take the decompiled call's
        memcpy(g_post + (g_ignore[k][0] - 0x588000), after + (g_ignore[k][0] - 0x588000), g_ignore[k][1]);

    DWORD dataDiff = 0, stackDiff = 0;
    // The return value as the function returns one: nothing (v), st0 (f), al (b) or eax (i). edx is never compared:
    // a function that returns no 64-bit value leaves whatever it last held there.
    char k = g_dual[i].kind;
    BOOL retDiff = k == 'b' ? (orig.eax & 0xff) != (g_dualOut.eax & 0xff)
                 : k == 's' ? (orig.eax & 0xffff) != (g_dualOut.eax & 0xffff)
                 : k == 'i' ? orig.eax != g_dualOut.eax
                 : k == 'f' ? orig.has_st0 && g_dualOut.has_st0 && memcmp(orig.st0, g_dualOut.st0, 10) != 0
                            : FALSE;
    BOOL regDiff = retDiff || orig.popped != g_dualOut.popped || orig.has_st0 != g_dualOut.has_st0;
    if (memcmp(g_post, after, g_dataSize) != 0 || memcmp(stackPost, args, stackLen) != 0 || regDiff ||
        holdOrig != *hold) {
        g_dual[i].differ++;
        Log("dual: 0x%06lX call %d differs (ecx 0x%08lX, args %08lX %08lX %08lX %08lX):", g_dual[i].va, n,
            regs->ecx, args[0], args[1], args[2], args[3]);
        dataDiff = LogRanges("data", g_post, after, g_dataSize, 0x588000);
        stackDiff = LogRanges("stack", stackPost, (BYTE *)args, stackLen, (DWORD)(UINT_PTR)args);
        if (regDiff)
            Log("dual:   eax %08lX / %08lX, popped %lu / %lu, st0 %s / %s", orig.eax, g_dualOut.eax, orig.popped,
                g_dualOut.popped, orig.has_st0 ? St0Text(orig.st0, 0) : "no",
                g_dualOut.has_st0 ? St0Text(g_dualOut.st0, 1) : "no");
        if (holdOrig != *hold)
            Log("dual:   rand state %08lX / %08lX", holdOrig, *hold);
        Log("dual:   %lu data bytes, %lu stack bytes", dataDiff, stackDiff);
    } else {
        Log("dual: 0x%06lX call %d same (eax %08lX%s)", g_dual[i].va, n, g_dualOut.eax,
            g_dualOut.has_st0 ? ", st0" : "");
    }
    g_inDual = 0;
}

// Called by the loader for a listed function before it writes the jmp: returns the stub the jmp should reach
// (or dll unchanged when the function is not listed or dual calls are off).
BYTE *DualTarget(DWORD va, BYTE *dll, const char *list)
{
    if (!list || g_numDual >= DUAL_MAX)
        return dll;
    // The list holds this address? One per line: "0xADDRESS [kind]" (kind v / f / b / i as in hybrid.txt).
    char kind = 0;
    for (const char *p = list; *p;) {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
            p++;
        char *end;
        DWORD v = strtoul(p, &end, 16);
        const char *q = end;
        while (*q == ' ' || *q == '\t')
            q++;
        if (v == va && end != p) {
            kind = (*q == 'v' || *q == 'f' || *q == 'b' || *q == 's' || *q == 'i') ? *q : 'i';
            break;
        }
        while (*p && *p != '\n')
            p++;
    }
    if (!kind)
        return dll;
    if (!g_pre) {
        IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)((BYTE *)GetModuleHandleA(NULL) +
                                                    ((IMAGE_DOS_HEADER *)GetModuleHandleA(NULL))->e_lfanew);
        IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);
        for (int k = 0; k < nt->FileHeader.NumberOfSections; k++)
            if (!memcmp(sec[k].Name, ".data", 6)) {
                g_data = (BYTE *)GetModuleHandleA(NULL) + sec[k].VirtualAddress;
                g_dataSize = sec[k].Misc.VirtualSize;
            }
        g_pre = VirtualAlloc(NULL, g_dataSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        g_post = VirtualAlloc(NULL, g_dataSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        g_samples = GetPrivateProfileIntA("Hybrid", "DualSamples", 5, g_cfg.iniPath);
        char ign[512];
        GetPrivateProfileStringA("Hybrid", "DualIgnore", "", ign, sizeof ign, g_cfg.iniPath);
        for (char *p = ign; *p && g_numIgnore < 32;) {
            char *end;
            DWORD a = strtoul(p, &end, 16);
            if (end == p || *end != ':') {
                p++;
                continue;
            }
            DWORD n = strtoul(end + 1, &end, 16);
            if (a >= 0x588000 && n)
                g_ignore[g_numIgnore][0] = a, g_ignore[g_numIgnore++][1] = n;
            p = end;
        }
        if (!g_data || !g_pre || !g_post) {
            Log("dual: no memory for the copies of the game's data: dual calls off");
            g_numDual = DUAL_MAX;
            return dll;
        }
        Log("dual: data 0x%08lX, %lu bytes; %d calls per function", (DWORD)(UINT_PTR)g_data, g_dataSize, g_samples);
        HMODULE exe = GetModuleHandleA(NULL);
        g_realQpc = (PFN_Qpc)HookImport(exe, "KERNEL32.dll", "QueryPerformanceCounter", (void *)Freeze_Qpc);
        g_realTgt = (PFN_Ticks)HookImport(exe, "WINMM.dll", "timeGetTime", (void *)Freeze_timeGetTime);
        g_realTick = (PFN_Ticks)HookImport(exe, "KERNEL32.dll", "GetTickCount", (void *)Freeze_GetTickCount);
    }
    int i = g_numDual++;
    BYTE *at = (BYTE *)(UINT_PTR)SumVa(va);
    g_dual[i].va = va;
    g_dual[i].dll = dll;
    g_dual[i].kind = kind;
    memcpy(g_dual[i].orig, at, 5);
    BYTE *stub = g_dual[i].stub;
    stub[0] = 0x68; // push i
    *(DWORD *)(stub + 1) = (DWORD)i;
    stub[5] = 0xE9; // jmp DualEntry
    *(DWORD *)(stub + 6) = (DWORD)((BYTE *)DualEntry - (stub + 10));
    DWORD old;
    VirtualProtect(g_dual, sizeof g_dual, PAGE_EXECUTE_READWRITE, &old);
    g_dual[i].jmp[0] = 0xE9;
    *(DWORD *)(g_dual[i].jmp + 1) = (DWORD)(stub - (at + 5));
    Log("dual: 0x%06lX checked against the original for its first %d calls", va, g_samples);
    return stub;
}

// Per function: calls checked, how many differed.
void DualSummary(void)
{
    for (int i = 0; i < g_numDual && i < DUAL_MAX; i++)
        Log("dual: 0x%06lX: %d calls checked, %d differed", g_dual[i].va, g_dual[i].calls, g_dual[i].differ);
}
