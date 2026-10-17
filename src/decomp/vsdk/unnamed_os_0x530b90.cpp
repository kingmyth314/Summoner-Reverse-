// unnamed_os_0x530b90.cpp - Sum.exe v1.40, code 0x530b90-0x531bea (sum.symbols module vsdk\os): the
// single-instance check, the main window and its window procedure, the message handler list, timer_wait,
// the random seed; after them the CPU identification / "sysinfo" code and the clipboard reader, which are
// built without optimisation.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// File name not known (no path string, no strings.tbl block; Summoner 2 has timer_wait in timer.cpp but the
// PC timer.cpp starts at 0x532820), so a placeholder. One object: the .bss 0x2cbfce0-0x2cbff27 holds the os
// globals with ccmd_sysinfo (0x2cbff00) in their middle, and the gap after os_seed_random is nop padding (the
// compiler's), while the /Od code ends with int3 fill (the linker's gap before the next object at 0x531bf0).
// So the original most likely switched optimisation off (`#pragma optimize("", off)`) before the CPU code;
// intrinsics stay on (os_clipboard_get_text's strlen is repne scasb).
//
// Module - in sum.symbols, same object: os_poll 0x530ca0, os_pump_messages 0x530cb0 (decomp76, D5806),
// and (decomp74, D5608) cpu_read_tsc 0x5312c0, cpu_identify 0x5312d6, PrintSystemInfo 0x53171f,
// init_ccmd_sysinfo_entry 0x531a80, init_ccmd_sysinfo 0x531a8a, cmd_sysinfo 0x531aa8 and ccmd_sysinfo
// (0x2cbff00).

#include <windows.h>
#include <objbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "vsdk/os/os.h"
#include "vsdk/os/timer.h"
#include "vsdk/os/console.h"
#include "vsdk/os/vstring.h"

// ---- .bss (0x2cbfce0-0x2cbff27, address order) ------------------------------------------------------
char os_mutex_name[0x80];                // 0x2cbfce0
os_msg_handler os_msg_handlers[32];      // 0x2cbfd60 (no bounds check in os_add_msg_handler)
char os_window_title[0x80];              // 0x2cbfde0 (sum.symbols 0x98: the critical section starts at 0x2cbfe60)
static CRITICAL_SECTION os_critical_section;   // 0x2cbfe60 (name ours) initialised by os_init, no other use
char os_window_class[0x88];              // 0x2cbfe78
// 0x2cbff00 ccmd_sysinfo (console_command, defined with the sysinfo code).
int os_prev_instance_found;              // 0x2cbff0c
HWND main_wnd;                           // 0x2cbff10
static HANDLE os_main_thread;            // 0x2cbff14 (name ours) SetThreadPriority target, never set (NULL)
HANDLE os_instance_mutex;                // 0x2cbff18
bool is_main_wnd_active;                 // 0x2cbff1c
int os_inited;                           // 0x2cbff20 (name ours) set by os_init, never read
int os_num_msg_handlers;                 // 0x2cbff24

static void os_atexit_nop();

// 0x530b90  Confidence: exact.
BOOL CALLBACK os_find_window_callback(HWND hwnd, LPARAM title)
{
    char text[128];

    if (GetWindowTextA(hwnd, text, 127) && strstr(text, (const char *)title)) {
        os_prev_instance_found = 1;
        return FALSE;
    }
    return TRUE;
}

// 0x530bf0  Confidence: exact (sum.symbols module vsdk\os; declared in vsdk/os/timer.h).
void timer_wait(int ms)
{
    Sleep(ms);
}

// 0x530c00  Confidence: exact.
HWND os_get_main_wnd()
{
    return main_wnd;
}

// 0x530c10  Confidence: exact.
bool os_foreground()
{
    return is_main_wnd_active;
}

// 0x530c20  Confidence: exact.
void os_add_msg_handler(os_msg_handler handler)
{
    os_msg_handlers[os_num_msg_handlers++] = handler;
}

// 0x530c40  Confidence: likely (bare ret; no reference, so the signature is unknown).
void os_stub_ret_530c40()
{
}

// 0x530c50  Confidence: likely (bare ret; no reference).
void os_stub_ret_530c50()
{
}

// 0x530c60  Confidence: exact.
void os_set_window_title(const char *title)
{
    strcpy(os_window_title, title);
    SetWindowTextA(main_wnd, os_window_title);
}

// 0x530ca0  Confidence: exact. The message pump the game loops call (decomp76).
void os_poll()
{
    os_pump_messages(0);
}

// 0x530cb0  Confidence: likely (the return value: 0 on both paths, the loop's from PeekMessage's 0). Every
// waiting message is translated and dispatched; the argument is not read (decomp76).
int os_pump_messages(int unused)
{
    MSG msg;

    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return 0;
}

// 0x530d10  Confidence: exact.
// A copy that finds the mutex calls ReleaseMutex on the handle it opened (it does not own it) and never
// closes it; either way the running copy's window is brought to the front and this one exits.
void os_init(const char *name, const char *title)
{
    strcpy(os_mutex_name, name);
    strcpy(os_window_title, title);
    strcpy(os_window_class, name);

    HANDLE mutex = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, os_mutex_name);
    if (mutex) {
        ReleaseMutex(mutex);
        HWND wnd = FindWindowA(os_window_class, os_window_title);
        if (wnd)
            SetForegroundWindow(wnd);
        exit(1);
    }
    os_instance_mutex = CreateMutexA(NULL, TRUE, os_mutex_name);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND wnd = FindWindowA(os_window_class, os_window_title);
        if (wnd)
            SetForegroundWindow(wnd);
        exit(1);
    }
    CoInitialize(NULL);
    InitializeCriticalSection(&os_critical_section);
    os_init_window(0);
    os_inited = 1;
    os_find_previous_instance();
    atexit(os_atexit_nop);
}

// 0x530e40  Confidence: likely (the backward scan: VC6 folded the first '\\' test against the NUL it had just
// stored, so the loop tests '/' and ':' first).
// Looks for a window titled "[run] - <exe name> -"; the result (os_prev_instance_found) is never read.
void os_find_previous_instance()
{
    char path[128];
    char title[256];

    os_prev_instance_found = 0;
    HMODULE module = GetModuleHandleA(NULL);
    if (module == NULL)
        return;
    if ((int)GetModuleFileNameA(module, path, 127) < 1)
        return;
    char *p = strstr(path, ".exe");
    if (p == NULL)
        return;
    *p = '\0';
    while (*p != '\\' && *p != '/' && *p != ':')
        p--;
    p++;
    if (strlen(p) < 1)
        return;
    sprintf(title, "[run] - %s -", p);
    EnumWindows(os_find_window_callback, (LPARAM)title);
}

// 0x530ee0  Confidence: exact (os_init pushes 0; the argument is not read).
int os_init_window(int unused)
{
    os_create_window();
    return 0;
}

// 0x530ef0  Confidence: exact (VC6 byte match, cl 8966; was likely: the WNDCLASSEX field stores are scheduled; the
// source order is a guess).
// A 32x32 WS_POPUP window in the screen centre; the renderers resize it.
int os_create_window()
{
    WNDCLASSEXA wc;
    HINSTANCE instance = GetModuleHandleA(NULL);

    wc.hInstance = instance;
    wc.lpszClassName = os_window_class;
    wc.lpfnWndProc = os_wndproc;
    wc.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC | CS_BYTEALIGNCLIENT;
    wc.cbSize = sizeof(wc);
    wc.hIcon = LoadIconA(instance, "APP_ICON");
    wc.hIconSm = NULL;
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.lpszMenuName = NULL;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = NULL;
    if (!RegisterClassExA(&wc))
        return 0;

    main_wnd = CreateWindowExA(0, os_window_class, os_window_title, WS_POPUP,
                               (GetSystemMetrics(SM_CXSCREEN) - 32) / 2,
                               (GetSystemMetrics(SM_CYSCREEN) - 32) / 2,
                               32, 32, NULL, NULL, instance, NULL);
    ShowCursor(FALSE);
    ClipCursor(NULL);
    SetActiveWindow(main_wnd);
    SetForegroundWindow(main_wnd);
    return 1;
}

// 0x530ff0  Confidence: likely (switch case order; the 256-byte buffer is the sprintf target).
// Activation makes the window topmost with SetWindowPos flags 0 (so it also activates and resizes it to the
// screen) and raises the thread priority of os_main_thread, which nothing sets. SC_SCREENSAVE is swallowed.
LRESULT CALLBACK os_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    char buf[256];
    int i;

    for (i = 0; i < os_num_msg_handlers; i++)
        os_msg_handlers[i](msg, wparam, lparam);

    switch (msg) {
    case WM_ACTIVATE:
    case WM_ACTIVATEAPP: {
        bool was_active = is_main_wnd_active;
        if (msg == WM_ACTIVATE)
            is_main_wnd_active = LOWORD(wparam) == WA_ACTIVE || LOWORD(wparam) == WA_CLICKACTIVE;
        else
            is_main_wnd_active = wparam != 0;
        if (was_active != is_main_wnd_active) {
            if (is_main_wnd_active) {
                SetWindowPos(main_wnd, HWND_TOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN),
                             GetSystemMetrics(SM_CYSCREEN), 0);
                SetThreadPriority(os_main_thread, THREAD_PRIORITY_HIGHEST);
            } else {
                SetWindowPos(main_wnd, HWND_NOTOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN),
                             GetSystemMetrics(SM_CYSCREEN), 0);
                SetThreadPriority(os_main_thread, THREAD_PRIORITY_NORMAL);
            }
        }
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_SYSCOMMAND:
        sprintf(buf, "Sys command called '%d'\n", wparam);
        if (wparam == SC_SCREENSAVE)
            return 0;
        return DefWindowProcA(hwnd, WM_SYSCOMMAND, wparam, lparam);
    case WM_TIMER:
    case WM_PALETTEISCHANGING:
    case WM_PALETTECHANGED:
    case WM_DISPLAYCHANGE:
        return 0;
    case WM_QUERYNEWPALETTE:
        return 1;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

// 0x531250  Confidence: likely (bare ret; os_init registers it with atexit; no sum.symbols entry, name ours).
static void os_atexit_nop()
{
}

// 0x531260  Confidence: exact.
void os_close_main_window()
{
    ReleaseMutex(os_instance_mutex);
    PostMessageA(main_wnd, WM_DESTROY, 0, 0);
}

// 0x531280  Confidence: exact (the last rand is a tail jump).
void os_seed_random(int seed)
{
    if (seed == -1)
        srand(time(NULL));
    else
        srand(seed);
    rand();
    rand();
    rand();
}

// 0x5312c0-0x531b43: cpu_read_tsc, cpu_identify, PrintSystemInfo, the sysinfo console command (module -;
// written by decomp74, D5608). The original's optimisation switch stands before them:
#pragma optimize("", off)

// 0x5312c0  Confidence: exact (/Od: frame, one local; the __asm block saves EBX / ESI / EDI). The low
// dword of the time stamp counter.
unsigned int cpu_read_tsc()
{
    unsigned int t;

#if defined(_MSC_VER)
    __asm {
        rdtsc
        mov t, eax
    }
#else
    t = 0;                                           // (syntax check only)
#endif
    return t;
}

// 0x5312d6  Confidence: likely (/Od; locals renamed and declared in reverse so that the /Od frame order
// (D6101 hash buckets, the same under 8966) gives the binary's slots, D7003; the zeroed CPUID result as a struct
// whose first member is stored
// with an immediate, the rest from EAX; the brand string buffer cleared with memset). (family, mmx, 3dnow,
// katmai, tsc, name, mhz): CPUID level 1 gives the family and the feature bits (without CPUID: a 486, no
// features); AMD's extended levels give 3DNow! and the brand string; the speed is the TSC over 100 ms at
// raised priority. Bugs (NOTES): mmx is always set to 1 (mmx_ok is never cleared), and the AMD / Intel
// names test the pointer `mmx`, not *mmx, so they always say MMX; the priority calls pass a NULL handle,
// so they change nothing.
void cpu_identify(int *family, int *mmx, int *amd3dnow, int *katmai, int *tsc, char *name, int *mhz)
{
    DWORD time_end;
    DWORD begin;
    unsigned int tsc_end;
    int tsc_diff;
    unsigned int tsc0;
    int prev_priority;
    int rate;
    DWORD saved_pclass;
    char brand_text[64];
    unsigned int eax1;
    int mmx_ok = 1;
    unsigned int features;

    *family = 0;
    *mmx = 0;
    *amd3dnow = 0;
    *tsc = 0;
    *katmai = 0;
    if (mhz)
        *mhz = 0;
    struct {
        unsigned int max_level;
        char vendor[16];
    } regs = { 0 };

#if defined(_MSC_VER)
    __asm {
        push eax
        push ebx
        push ecx
        push edx
        pushfd
        pop eax
        mov ebx, eax
        xor eax, 0x200000
        push eax
        popfd
        pushfd
        pop eax
        xor eax, ebx
        je no_cpuid
        mov eax, 0
        cpuid
        mov regs.max_level, eax
        mov dword ptr [regs + 4], ebx
        mov dword ptr [regs + 8], edx
        mov dword ptr [regs + 12], ecx
        mov eax, 1
        cpuid
        mov eax1, eax
        mov features, edx
        jmp done
no_cpuid:
        mov eax1, 0x400
        mov features, 0
done:
        pop edx
        pop ecx
        pop ebx
        pop eax
    }
#else
    eax1 = 0x400;                               // (syntax check only)
    features = 0;
#endif

    *family = (eax1 >> 8) & 0xf;
    if (*family < 5) {
        *family = 4;
        if (name)
            sprintf(name, "Not Pentium Compatible");
        return;
    }
    if (features & 0x10)
        *tsc = 1;
    if (features & 0x800000) {
    } else {
        *mmx = 0;
    }
    if (mmx_ok == 0)
        *mmx = 0;
    else
        *mmx = 1;
    if (*family > 5 && (features & 0x2000000))
        *katmai = 1;

    if (!_stricmp(regs.vendor, "AuthenticAMD")) {
#if defined(_MSC_VER)
        __asm {
            mov eax, 0x80000000
            cpuid
            mov eax1, eax
        }
#endif
        if (eax1 < 0x80000005) {
            if (name)
                sprintf(name, "AMD-K%d, MMX:%s", *family, mmx ? "Yes" : "No");
        } else {
#if defined(_MSC_VER)
            __asm {
                mov eax, 0x80000001
                cpuid
                mov eax1, eax
                mov features, edx
            }
#endif
            if (features & 0x80000000)
                *amd3dnow = 1;
            if (name) {
                memset(brand_text, 0, sizeof(brand_text));
#if defined(_MSC_VER)
                __asm {
                    mov eax, 0x80000002
                    cpuid
                    mov dword ptr brand_text[0], eax
                    mov dword ptr brand_text[4], ebx
                    mov dword ptr brand_text[8], ecx
                    mov dword ptr brand_text[12], edx
                    mov eax, 0x80000003
                    cpuid
                    mov dword ptr brand_text[16], eax
                    mov dword ptr brand_text[20], ebx
                    mov dword ptr brand_text[24], ecx
                    mov dword ptr brand_text[28], edx
                    mov eax, 0x80000004
                    cpuid
                    mov dword ptr brand_text[32], eax
                    mov dword ptr brand_text[36], ebx
                    mov dword ptr brand_text[40], ecx
                    mov dword ptr brand_text[44], edx
                }
#endif
                strcpy(name, brand_text);
            }
        }
    } else if (!_stricmp(regs.vendor, "GenuineIntel")) {
        if (name) {
            switch (*family) {
            case 5:
                sprintf(name, "Intel Pentium");
                if (mmx)
                    strcat(name, " w/ MMX");
                break;
            case 6:
                if (*katmai)
                    sprintf(name, "Intel Pentium III");
                else if (mmx)
                    sprintf(name, "Intel Pentium II w/ MMX");
                else
                    sprintf(name, "Intel Pentium Pro");
                break;
            default:
                sprintf(name, "Intel i%d86", *family);
                break;
            }
        }
    } else if (name) {
        if (*mmx)
            sprintf(name, "%d86 w/ MMX Compatible", *family);
        else
            sprintf(name, "%d86 Compatible", *family);
    }

    if (mhz) {
        *mhz = 0;
        if (*tsc) {
            saved_pclass = GetPriorityClass(0);
            prev_priority = GetThreadPriority(0);
            SetPriorityClass(0, REALTIME_PRIORITY_CLASS);
            SetThreadPriority(0, THREAD_PRIORITY_TIME_CRITICAL);
            Sleep(100);
            do {
                begin = timeGetTime();
                tsc0 = cpu_read_tsc();
                while (timeGetTime() < begin + 100)
                    ;
                tsc_end = cpu_read_tsc();
                time_end = timeGetTime();
            } while (tsc0 > tsc_end);
            tsc_diff = tsc_end - tsc0;
            rate = (tsc_diff + 833333) / 1666667;
            *mhz = rate * 1666667 / 100000;
            SetPriorityClass(0, saved_pclass);
            SetThreadPriority(0, prev_priority);
        }
    }
}

// 0x53171f  Confidence: likely (/Od; the two buffers through new and delete temporaries). Appends the CPU
// type, speed, extensions and RAM size to tb; with test_ram, copies 256 KB blocks for one second and
// prints freq / (ticks per copy), the copies per second, as "MB/s" (four times the real rate, NOTES).
void PrintSystemInfo(TextBuffer *tb, int unused, bool test_ram)
{
    int mhz;
    int katmai;
    char ext[0x400];
    int mmx;
    int tsc;
    int amd3dnow;
    char name[0x80];
    MEMORYSTATUS ms;
    int family;
    int mb;
    char *dst;
    int speed;
    char *src;
    int count;
    LARGE_INTEGER freq;
    __int64 per_copy;
    DWORD end_time;
    LARGE_INTEGER start;
    LARGE_INTEGER stop;

    memset(name, 0, sizeof(name));
    cpu_identify(&family, &mmx, &amd3dnow, &katmai, &tsc, name, &mhz);
    TextBuffer_printf(tb, "CPU Type: %s\n", name);
    TextBuffer_printf(tb, "CPU Speed: %d Mhz\n", mhz);
    strcpy(ext, "");
    if (family > 5)
        strcat(ext, "[PPro] ");
    if (mmx)
        strcat(ext, "[MMX] ");
    if (amd3dnow)
        strcat(ext, "[3DNow] ");
    if (katmai)
        strcat(ext, "[Katmai] ");
    TextBuffer_printf(tb, "CPU Extensions: %s\n", ext);

    ms.dwLength = sizeof(ms);
    GlobalMemoryStatus(&ms);
    mb = (ms.dwTotalPhys + 0x200000) >> 22;
    mb <<= 2;
    sprintf(ext, "%d MB", mb);
    TextBuffer_printf(tb, "Total RAM: %s\n", ext);

    if (test_ram) {
        dst = new char[0x400000];
        src = new char[0x400000];
        QueryPerformanceCounter(&start);
        count = 0;
        end_time = timeGetTime() + 1000;
        while (timeGetTime() < end_time) {
#if defined(_MSC_VER)
            __asm {
                push esi
                push edi
                push ecx
                mov esi, src
                mov edi, dst
                mov ecx, 0x10000
                rep movsd
                pop ecx
                pop edi
                pop esi
            }
#endif
            count++;
        }
        QueryPerformanceCounter(&stop);
        QueryPerformanceFrequency(&freq);
        delete dst;
        delete src;
        per_copy = (stop.QuadPart - start.QuadPart) / count;
        speed = (int)(freq.QuadPart / per_copy);
        TextBuffer_printf(tb, "RAM Speed: %d MB/s\n", speed);
    }
}

// 0x531a80 init_ccmd_sysinfo_entry (/Od: push ebp; call 0x531a8a; pop ebp) /
// 0x531a8a init_ccmd_sysinfo: the dynamic initialiser of the definition below. Compiler-generated.
// Confidence: likely (the object's .bss slot 0x2cbff00 lies among the os globals above).
void cmd_sysinfo();
console_command ccmd_sysinfo("sysinfo", "Shows system info", cmd_sysinfo);   // 0x2cbff00

// 0x531aa8  Confidence: likely (/Od; EH frame for the TextBuffer). The console's "sysinfo": the system
// info through a 2 KB text buffer.
void cmd_sysinfo()
{
    if (calling_a_command_function) {
        char buf[0x800];
        TextBuffer tb(buf, 0x800, 0);

        PrintSystemInfo(&tb, 1, true);
        print_to_console(tb.get_text(), 0);
    }
    if (doing_help_for_comand)
        print_to_console("Shows system info", 0);
}

// 0x531b44  Confidence: exact (VC6 byte match, cl 8966; was likely: /Od code: frame pointer, locals h -4, text -8,
// i -0xc, len -0x10).
// Both failure returns skip CloseClipboard, leaving the clipboard open; buf needs max_len + 1 bytes.
void os_clipboard_get_text(char *buf, int max_len)
{
    char *text;
    HANDLE h;
    int i;
    int len;

    *buf = '\0';
    OpenClipboard(NULL);
    h = GetClipboardData(CF_TEXT);
    if (h == NULL)
        return;
    text = (char *)GlobalLock(h);
    if (text == NULL)
        return;
    len = strlen(text);
    if (len > max_len)
        len = max_len;
    for (i = 0; i < len; i++)
        buf[i] = text[i];
    buf[i] = '\0';
    GlobalUnlock(h);
    CloseClipboard();
}

#pragma optimize("", on)
