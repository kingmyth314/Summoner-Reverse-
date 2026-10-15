# Phase 3 pilot: frametime.cpp and timestamp.cpp (source half)

Written from build/sum.asm only. Summoner 2 headers were used for names only (they match the
names already in symbols/sum.symbols); no Summoner 2 source was read. Syntax-checked with
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wextra` (clean).

## frametime.cpp (0x501470-0x5017e6)

| Address | Function | Confidence |
|---|---|---|
| 0x501470 | init_frametime_saved_min (dynamic initialiser, `static float frametime_saved_min = min_reported_frametime;`) | likely |
| 0x501490 | init_frametime_saved_max (same for max) | likely |
| 0x5014b0 | frametime_initialize | exact |
| 0x501510 | FrameDone | likely |
| 0x5016d0 | frametime_set_divisor | exact |
| 0x5016e0 | frametime_get_divisor | exact |
| 0x5016f0 | frametime_set_limits | exact |
| 0x501720 | frametime_get_limits | exact |
| 0x501750 | frametime_unlimit | exact |
| 0x501780 | frametime_restore_limits | exact |
| 0x5017a0 | increase_frametime_pause_counter | exact |
| 0x5017b0 | decrease_frametime_pause_counter | exact |
| 0x5017d0 | increase_animation_pause_counter | exact |
| 0x5017e0 | decrease_animation_pause_counter | exact |

## timestamp.cpp (0x5017f0-0x501b16)

| Address | Function | Confidence |
|---|---|---|
| 0x5017f0 | UpdateTicker | likely |
| 0x501840 | IncrementPauseCounter | exact |
| 0x501850 | DecrementPauseCounter | exact |
| 0x501860 / 0x5019e0 | GameTimer / RealtimeTimer constructor | exact |
| 0x501870 / 0x5019f0 | reset | exact |
| 0x501880 / 0x501a00 | SetTimeout | likely |
| 0x5018d0 / 0x501a50 | SetRandomTimeout | exact |
| 0x501900 / 0x501a80 | invalidate | exact |
| 0x501910 / 0x501a90 | elapsed | exact |
| 0x501940 / 0x501ac0 | GetRemainingTime | likely |
| 0x501980 / 0x501b00 | GetElapsedTime | exact |
| 0x501990 / 0x501b10 | valid | exact |
| 0x5019a0 | GameTimer::read(File*) | exact |
| 0x5019c0 | GameTimer::write(File*) | exact |

Totals: 37 functions (14 + 23), 29 exact, 8 likely, 0 unsure. 0x501b20 (init_ccmd_load_mipmaps_thunk)
starts the next unit and is not included.

## Facts used

- Constants: 1e-6f (0x581d20), 1e6f (0x581d24), 1000.0f (0x57ef28), 16.0f (0x580df0), 0.0f
  (0x57eb84), 10.0f and 1e-5f pushed by frametime_unlimit. All float, no double anywhere.
- Every float->int is `_ftol` (0x56c7e4): delta from reported_frametime*1e6, the sleep length,
  and the frame's milliseconds.
- QPC math is not in these units: `timer_get(1000000)` (timer.cpp) returns a 32-bit int that
  FrameDone compares signed.
- MAX_TIME = 0x3ff1a100; the half-range test uses 0x1ff8d080 = MAX_TIME/2;
  `add eax, 0xc00e5f00` is `- MAX_TIME`.
- /Ob1 behaviour: GetElapsedTime calls GetRemainingTime and SetRandomTimeout calls SetTimeout
  instead of inlining, so nothing here is declared inline.

## Open questions

1. **.bss order.** frametime_saved_max (0x257f080) sits before frametime_saved_min (0x257f084),
   but the initialiser table runs min first (0x5887cc, then 0x5887d0). Either VC6 lays out these
   slots in reverse, or the definitions and the initialisers are in different orders. The
   compiler test will show which; the other .bss/.data variables are written in address order.
2. **cutscene_active_count / cutscene_frametime (0x257f090/94)** sit inside frametime.cpp's .bss
   but only cutscene and game code use them. I define them here (C++ globals are not communal in
   MSVC, so they must belong to this object). Their real names are unknown.
3. **FrameDone, reported_frametime in ST0.** After timer_wait the binary loads
   min_reported_frametime and never stores it until the final `/= frametime_divisor`. That
   matches the global form only if VC6 drops the dead store; otherwise the source used a local
   `float` for the clamped time. The order of `frames_rendered++` against the divide is also a
   guess (the INC sits between FDIV and FSTP).
4. **History reset.** `memset` vs a 16-step zero loop: both may give the `rep stosd`.
5. **static vs extern.** Only reported_frametime, fps (FrameEnd, levelrender_fog_enable) and
   milliseconds_unpaused (bm_get_current_frame) are read outside these units. The others are
   written `static`, which does not change the code but changes the object's symbol table.
   Timestamp_ticker and Timestamp_realtime_ticker are also only used here; I left them extern.
6. **File** interface is a stub (`read_int(int,int)`, `write_int(int)` thiscall); the
   write_int return type is unknown.
