// unnamed_progress_0x503dd0.cpp - Sum.exe v1.40, code 0x503dd0-0x503f54: the loading progress callback
// (module "-" in sum.symbols). Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0
// (cl 12.00.8966), /O2. Confidence per function: exact = expect identical code, likely = same semantics,
// small layout risk, unsure = the source shape is a guess.
//
// A unit of its own (D5605): bmpman.cpp's out-of-line imin COMDAT (0x503dc0) closes bmpman's object right
// before it, and its data follows bmpman's (.data 0x5a5c24-0x5a5c2b after bmpman's 0x5a5c20, .bss
// 0x25c6898-0x25c68ab after bm_preload_on_load 0x25c6894); localize (0x503f60, .bss 0x25c68b0) follows.
// File name not known (no path string, no strings.tbl block), so a placeholder; the functions are declared
// in vsdk/bmpman/bmpman.h, where the earlier pilots put them.
//
// The level loader installs a callback that draws the loading screen; the loaders call progress_update
// between steps, which runs the callback every 100 ms (or every 0x9b updates when the step count is
// known) and, when the caller allowed it, drains the key queue: Escape asks the load to stop.

#include <stddef.h>
#include "vsdk/os/timer.h"
#include "vsdk/os/key.h"
#include "vsdk/bmpman/bmpman.h"

// ---- .data (0x5a5c24-0x5a5c2b) -------------------------------------------------------------------------
int progress_next_time = -1;                         // 0x5a5c24 timer_get(1000) of the next callback, -1 none
int progress_step_count = -1;                        // 0x5a5c28 the steps passed to progress_set_callback

// ---- .bss (0x25c6898-0x25c68ab) ------------------------------------------------------------------------
bool progress_abortable;                             // 0x25c6898 progress_update polls the keys
bool progress_aborted;                               // 0x25c6899 Escape was pressed
void (*progress_callback)(int step, int flags);      // 0x25c689c
int progress_current;                                // 0x25c68a0 nonzero while the callback runs
int progress_update_count;                           // 0x25c68a4 progress_update calls since the start
int progress_last_draw;                              // 0x25c68a8 progress_update_count at the last callback

// 0x503dd0  Confidence: exact (VC6 byte match). NULL ends the progress with a final callback (-1, 0); a
// callback starts it with (0, 0). Returns the progress_update calls counted so far (the level loader's real
// step count).
int progress_set_callback(void (*callback)(int step, int flags), int num_steps, bool abortable)
{
    if (callback == NULL) {
        progress_current++;
        progress_callback(-1, 0);
        progress_current--;
        progress_next_time = -1;
        progress_callback = NULL;
    } else {
        progress_update_count = 0;
        progress_last_draw = 0;
        progress_step_count = num_steps;
        progress_next_time = timer_get(1000) + 100;
        progress_callback = callback;
        progress_current++;
        callback(0, 0);
        progress_current--;
    }
    progress_aborted = false;
    progress_abortable = abortable;
    return progress_update_count;
}

// 0x503e80  Confidence: exact (VC6 byte match; the key loop as a while loop; the aborted store reuses AL = 1).
// Not while
// the callback runs, not before the start, not without a callback. Returns true once Escape was pressed.
bool progress_update(int flags)
{
    int now;

    if (progress_current != 0)
        return false;
    if (progress_next_time < 0)
        return false;
    if (progress_callback == NULL)
        return false;
    progress_update_count++;
    now = timer_get(1000);
    if (now > progress_next_time
        || (progress_update_count > progress_last_draw + 0x9b && progress_step_count > 0)) {
        if (progress_abortable) {
            while (key_queue_not_empty()) {
                if (key_inkey() == KEY_ESC)
                    progress_aborted = true;
            }
        }
        progress_last_draw = progress_update_count;
        progress_current++;
        progress_callback(progress_update_count, flags);
        progress_current--;
        progress_next_time = now + 100;
    }
    return progress_aborted;
}
