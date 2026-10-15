// frametime.cpp - Sum.exe v1.40, code 0x501470-0x5017e6
// Written from the Sum.exe disassembly (build/sum.asm). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.

#include <string.h>
#include "frametime.h"
#include "timestamp.h"

// timer.cpp (0x5328d0, 0x530bf0)
int timer_get(int scale);   // elapsed QPC ticks * scale / frequency (32-bit result)
void timer_wait(int ms);    // Sleep wrapper

// ---- .data, in address order ---------------------------------------------------------------
float actual_frametime = 1.0f / 30.0f;        // 0x5a573c  0x3d088889
float reported_frametime = 1.0f / 30.0f;      // 0x5a5740  0x3d088889
float fps = 30.0f;                            // 0x5a5744
float frametime_divisor = 1.0f;               // 0x5a5748
float min_reported_frametime = 1.0f / 120.0f; // 0x5a574c  0x3c088889
float max_reported_frametime = 0.25f;         // 0x5a5750
static int frametime_history_index = -1;      // 0x5a5754  -1 = history needs a reset

// ---- .bss, in address order ----------------------------------------------------------------
// Only FrameDone touches the history; nothing outside this file reads the statics below.
static float frametime_history[16];           // 0x257f040
// The two saved limits are dynamically initialised (static initialiser table 0x5887cc, 0x5887d0),
// so they live in .bss. The table order (min first) gives the definition order; the addresses are
// the other way round (max 0x257f080, min 0x257f084). VC6 may allocate .bss in reverse of
// definition order for these, or the source defined max first and initialised min first - open.
static float frametime_saved_min = min_reported_frametime; // 0x257f084  init_frametime_saved_min 0x501470
static float frametime_saved_max = max_reported_frametime; // 0x257f080  init_frametime_saved_max 0x501490
static int frames_rendered;                   // 0x257f088
int milliseconds_unpaused;                    // 0x257f08c  read by bm_get_current_frame
// The next two are only used by cutscene and game code, but they sit inside this file's .bss,
// so this file defines them (labels are ours).
int cutscene_active_count;                    // 0x257f090
float cutscene_frametime;                     // 0x257f094
static int previous_done_time;                // 0x257f098  microseconds (timer_get(1000000))
static float total_history_frametime;         // 0x257f09c
static bool Frametime_initialized;            // 0x257f0a0  (dword slot)
static int Frametime_paused;                  // 0x257f0a4
static int Animation_timer_paused;            // 0x257f0a8

// 0x501470 init_frametime_saved_min / 0x501490 init_frametime_saved_max: the compiler-generated
// dynamic initialisers for the two definitions above (a jmp thunk into the store, VC6 $E pair).
// Confidence: likely (order of the two .bss slots, see above).

// 0x5014b0  Confidence: exact.
// The store order in the binary matches this statement order. total_history_frametime = 0.0f is
// stored as integer 0 from EAX (VC6 does this for float zero constants).
void frametime_initialize()
{
    actual_frametime = 1.0f / 30.0f;
    reported_frametime = 1.0f / 30.0f;
    fps = 30.0f;
    frames_rendered = 0;
    milliseconds_unpaused = 0;
    frametime_divisor = 1.0f;
    min_reported_frametime = 1.0f / 120.0f;
    max_reported_frametime = 0.25f;
    previous_done_time = 0;
    frametime_history_index = -1;
    total_history_frametime = 0.0f;
}

// 0x501510  Confidence: likely.
// Known defect (docs/sum-exe/timing-bugs.md #3): previous_done_time is taken before the
// timer_wait sleep, so the next frame counts the sleep again.
// Layout notes:
// - one stack slot (push ecx) holds delta for the FILD.
// - reported_frametime stays in ST0 from the first store to the final divide. After timer_wait
//   the binary loads min_reported_frametime into ST0 and does not store it; the store is merged
//   into the final reported_frametime /= divisor. If VC6 does not drop that dead store, the
//   source used a local float instead of the global (same code otherwise).
// - the compares reload actual_frametime from memory, so the source compares the global.
// - the reset branch keeps 0.0f in ST0 for total and 0 in EAX for the index (stores elided);
//   memset becomes "mov ecx,16; xor eax,eax; rep stosd" (intrinsic). A for loop might too.
// - frames_rendered++ is scheduled between the FDIV and its FSTP; source order of those two
//   statements is a guess.
void FrameDone()
{
    if (!Frametime_initialized) {
        frametime_initialize();
        Frametime_initialized = true;
    }

    if (!Frametime_paused) {
        int now = timer_get(1000000);
        int delta;

        if (now < previous_done_time) {     // signed compare (JGE)
            if (now < 0)
                delta = (int)(reported_frametime * 1000000.0f);
            else
                delta = now;
        } else {
            delta = now - previous_done_time;
        }
        previous_done_time = now;

        actual_frametime = delta * 0.000001f;   // FILD, FMUL dword 1e-6
        reported_frametime = actual_frametime;

        if (actual_frametime < min_reported_frametime) {
            timer_wait((int)((min_reported_frametime - actual_frametime) * 1000.0f));
            reported_frametime = min_reported_frametime;
        } else if (actual_frametime > max_reported_frametime) {
            reported_frametime = max_reported_frametime;
        }

        if (frametime_history_index == -1) {
            memset(frametime_history, 0, sizeof(frametime_history));
            frametime_history_index = 0;
            total_history_frametime = 0.0f;
        }
        total_history_frametime = total_history_frametime - frametime_history[frametime_history_index] + reported_frametime;
        frametime_history[frametime_history_index] = reported_frametime;
        frametime_history_index = (frametime_history_index + 1) % 16;   // signed % (AND 0x8000000F / JNS fixup)

        if (total_history_frametime != 0.0f) {
            if (frames_rendered >= 16)
                fps = 16.0f / total_history_frametime;
            else
                fps = frames_rendered / total_history_frametime;
        } else {
            fps = 0.0f;
        }

        reported_frametime /= frametime_divisor;
        frames_rendered++;

        int ms = (int)(reported_frametime * 1000.0f);
        UpdateTicker(ms);
        if (!Animation_timer_paused)
            milliseconds_unpaused += ms;
    }
}

// 0x5016d0  Confidence: exact.
void frametime_set_divisor(float divisor)
{
    frametime_divisor = divisor;
}

// 0x5016e0  Confidence: exact.
float frametime_get_divisor()
{
    return frametime_divisor;
}

// 0x5016f0  Confidence: exact. FLD a / FCOM b; the smaller value becomes the minimum.
void frametime_set_limits(float a, float b)
{
    if (a < b) {
        min_reported_frametime = a;
        max_reported_frametime = b;
    } else {
        max_reported_frametime = a;
        min_reported_frametime = b;
    }
}

// 0x501720  Confidence: exact. No direct callers in Sum.exe.
void frametime_get_limits(float *min_out, float *max_out)
{
    if (min_out)
        *min_out = min_reported_frametime;
    if (max_out)
        *max_out = max_reported_frametime;
}

// 0x501750  Confidence: exact. 0x41200000 = 10.0f, 0x3727c5ac = 1e-5f.
void frametime_unlimit()
{
    frametime_saved_min = min_reported_frametime;
    frametime_saved_max = max_reported_frametime;
    frametime_set_limits(10.0f, 0.00001f);
}

// 0x501780  Confidence: exact. Only caller cutscene_cleanup.
void frametime_restore_limits()
{
    frametime_set_limits(frametime_saved_min, frametime_saved_max);
}

// 0x5017a0  Confidence: exact.
void increase_frametime_pause_counter()
{
    Frametime_paused++;
}

// 0x5017b0  Confidence: exact. Restarting the measurement keeps the pause out of the next frame.
void decrease_frametime_pause_counter()
{
    Frametime_paused--;
    previous_done_time = timer_get(1000000);
}

// 0x5017d0  Confidence: exact. Caller game_pause.
void increase_animation_pause_counter()
{
    Animation_timer_paused++;
}

// 0x5017e0  Confidence: exact. Caller game_unpause.
void decrease_animation_pause_counter()
{
    Animation_timer_paused--;
}
