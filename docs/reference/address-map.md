---
title: Address map
parent: Reference
nav_order: 1
---

# Address map

Sum.exe v1.40 (SHA-256 `f22c84a1…f8bf9d3`). Labels are ours; the binary has no symbols.
Keep this table sorted by address, and add every new address that appears in any page.

## Functions

| Address | Ghidra name | Label | Page |
|---|---|---|---|
| `0x401070` | `FUN_00401070` | `GameTick` | [Main loop](../sum-exe/main-loop.md#one-frame-gametick-fun_00401070) |
| `0x401190` | `FUN_00401190` | `StateSwap` | [Main loop](../sum-exe/main-loop.md) |
| `0x4138a0` | `FUN_004138a0` | clears per-frame flag `0x5bd77c` | [Main loop](../sum-exe/main-loop.md) |
| `0x414020` | `FUN_00414020` | clears per-frame flag `0x5bd504` | [Main loop](../sum-exe/main-loop.md) |
| `0x432710` | `FUN_00432710` | input/event poll with wait loop (*inferred*) | - |
| `0x4328b0` | `FUN_004328b0` | `FrameEnd` / HUD overlays | [Main loop](../sum-exe/main-loop.md) |
| `0x436a90` | `FUN_00436a90` | `SequenceTimer` (cutscenes, *inferred*) | [Timing bugs](../sum-exe/timing-bugs.md#2-cutscene-timer-sign-bug-249-day-uptime) |
| `0x436c00` | `FUN_00436c00` | another `timeGetTime` user of `0x601e78` | [Timing bugs](../sum-exe/timing-bugs.md) |
| `0x43d7b0` | `FUN_0043d7b0` | `MainLoop` | [Main loop](../sum-exe/main-loop.md#outer-loop-mainloop-fun_0043d7b0) |
| `0x43d7f0` | `FUN_0043d7f0` | `InitTimers` | [Main loop](../sum-exe/main-loop.md) |
| `0x43df40` | `FUN_0043df40` | `GameInit` | [Main loop](../sum-exe/main-loop.md) |
| `0x43e9b0` | `FUN_0043e9b0` | `CdCheck` (JGE at `0x43e9cb`) | [Timeslip](../patches/timeslip.md#how-it-hooks-the-game) |
| `0x494310` | `FUN_00494310` | another `timeGetTime` user of `0x601e78` | [Timing bugs](../sum-exe/timing-bugs.md) |
| `0x5014b0` | `FUN_005014b0` | `FrameTimerInit` | [Main loop](../sum-exe/main-loop.md#frame-timer-frametimer-fun_00501510) |
| `0x501510` | `FUN_00501510` | `FrameTimer` | [Main loop](../sum-exe/main-loop.md#frame-timer-frametimer-fun_00501510) |
| `0x5016f0` | `FUN_005016f0` | `SetFrameTimeLimits(min, max)` | [Main loop](../sum-exe/main-loop.md) |
| `0x5017f0` | `FUN_005017f0` | `GameClockAdvance(ms)` | [Main loop](../sum-exe/main-loop.md#game-clocks-gameclockadvance-fun_005017f0) |
| `0x530bf0` | `FUN_00530bf0` | `SleepMs` | [Main loop](../sum-exe/main-loop.md) |
| `0x530ca0` | `FUN_00530ca0` | `PumpMessages` wrapper | [Main loop](../sum-exe/main-loop.md) |
| `0x530cb0` | `FUN_00530cb0` | `PumpMessages` (PeekMessage loop) | [Main loop](../sum-exe/main-loop.md) |
| `0x5312d6` | `FUN_005312d6` | CPU identification | - |
| `0x53171f` | `FUN_0053171f` | `PrintSystemInfo` (CPU, RAM, RAM speed) | [Main loop](../sum-exe/main-loop.md#other-timers-not-yet-analysed) |
| `0x532840` | `FUN_00532840` | `FrameClockInit` | [Timing bugs](../sum-exe/timing-bugs.md#1-qpc-frequency-truncation-4-ghz-bug) |
| `0x5328d0` | `FUN_005328d0` | `FrameClockRead(scale)` | [Main loop](../sum-exe/main-loop.md#frame-clock-frameclockinit--frameclockread) |
| `0x5329b1` | `FUN_005329b1` | `ReadTSC` (RDTSC) | [Timing bugs](../sum-exe/timing-bugs.md#false-lead) |
| `0x566560` | `FUN_00566560` | `timeSetEvent` user (audio, *inferred*) | - |
| `0x566cf0` | `FUN_00566cf0` | calls `timeBeginPeriod(1)` | [Main loop](../sum-exe/main-loop.md) |

## Data

| Address | Type | Label | Notes |
|---|---|---|---|
| `0x57e1c8` | IAT | `QueryPerformanceCounter` | Hooked by Timeslip |
| `0x57e1cc` | IAT | `QueryPerformanceFrequency` | Hooked by Timeslip |
| `0x57e200` | IAT | `Sleep` | |
| `0x57e3c4` | IAT | `PeekMessageA` | |
| `0x57e3dc` | IAT | `timeBeginPeriod` | |
| `0x57e3fc` | IAT | `timeGetTime` | Candidate hook for the cutscene fix |
| `0x57ef28` | float | 1000.0 | |
| `0x580df0` | float | 16.0 | |
| `0x581d20` | float | 1e-6 | |
| `0x581d24` | float | 1e6 | |
| `0x588a70` | struct[] | `state_table` | `{enter, frame, leave}` x 12 bytes |
| `0x5931f0` | string | `Gamewide_show_framerate` | |
| `0x593314` | string | `FPS=%3.1f` | |
| `0x5a573c` | float | `raw_dt` | |
| `0x5a5740` | float | `frame_dt` | |
| `0x5a5744` | float | `fps_avg` | |
| `0x5a5748` | float | `time_scale` | Always 1.0 |
| `0x5a574c` | float | `MIN_DT` | 1/120 |
| `0x5a5750` | float | `MAX_DT` | 0.25 |
| `0x5a5754` | int | `ring_index` | |
| `0x5a5758` | int | game clock (pause-aware, ms) | |
| `0x5a575c` | int | game clock (always, ms) | |
| `0x5b27e0` | int | `state_cur` | |
| `0x5b27e4` | int | `state_prev` | |
| `0x5b27e8` | int | `state_next` | |
| `0x5b27ec` | byte | `state_change_pending` | |
| `0x5fbfd8` | int | debug/feature flags | |
| `0x5fc01f` | byte | `show_framerate` flag | |
| `0x601e78` | int | sequence start time (`timeGetTime`) | |
| `0x2491a5c` | byte | frame freeze flag (*inferred:* pause) | |
| `0x257f040` | float[16] | frame-time ring buffer | |
| `0x257f088` | int | frame count | |
| `0x257f08c` | int | total ms | |
| `0x257f098` | int | last clock reading (µs) | |
| `0x257f0a4` | int | timer frozen | |
| `0x257f0a8` | int | total-ms paused | |
| `0x257f0ac` | int | game-clock paused | |
| `0x2cbffa8` | int64 | `qpc_last` | |
| `0x2cbffb0` | int | RDTSC result (unused) | |
| `0x2cbffb8` | CRITICAL_SECTION | clock lock | |
| `0x2cbffd0` | int64 | `qpc_base` | |
| `0x2cbffd8` | byte | clock initialised | |
| `0x2cbffdc` | uint32 | `freq_low32` | Truncated frequency |

## Timeslip patch (`dinput.dll`, base `0x10000000`)

| Address | Ghidra name | Label |
|---|---|---|
| `0x1000a640` | `DirectInputCreateA` | Proxy export |
| `0x1000a750` | `FUN_1000a750` | `Hook_QPF` |
| `0x1000aa00` | `FUN_1000aa00` | `Hook_QPC` |
| `0x1000aee0` | `FUN_1000aee0` | `DllMain` / patch installer |
| `0x10012fd0` | data | `speed_percent` (default 100) |
| `0x1001df48` | data | `freq_shift` |
