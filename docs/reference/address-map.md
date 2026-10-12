---
title: Address map
parent: Reference
nav_order: 1
---

# Address map

Sum.exe v1.40 (SHA-256 `f22c84a1…f8bf9d3`). Labels are ours; the binary has no symbols.
Keep this table sorted by address, and add every new address that appears in any page.
"Volition:" notes give the original engine name where a reference project confirms it (see
[Engine references](engine-references.md)).

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
| `0x437c80` | `FUN_00437c80` | Loading-screen progress draw (also calls `FrameTimer`) | [Timing bugs](../sum-exe/timing-bugs.md) |
| `0x43d7b0` | `FUN_0043d7b0` | `MainLoop` | [Main loop](../sum-exe/main-loop.md#outer-loop-mainloop-fun_0043d7b0) |
| `0x43d7f0` | `FUN_0043d7f0` | `LoadSettings` (earlier labelled `InitTimers`) | [Video settings](../sum-exe/video-settings.md#settings-loader-loadsettings-fun_0043d7f0) |
| `0x43da64` | (instruction) | `MOV EDI, 0x67`: default renderer; SummonerFix patches to `0x68` | [SummonerFix](../summonerfix/index.md#fixes) |
| `0x43db45` | (instruction) | `PUSH 0xc9`: window mode; SummonerFix patches to `0xc8` | [SummonerFix](../summonerfix/index.md#fixes) |
| `0x43df40` | `FUN_0043df40` | `GameInit` | [Main loop](../sum-exe/main-loop.md) |
| `0x43e9b0` | `FUN_0043e9b0` | `CdCheck` (JGE at `0x43e9cb`) | [Timeslip](../patches/timeslip.md#how-it-hooks-the-game) |
| `0x494310` | `FUN_00494310` | another `timeGetTime` user of `0x601e78` | [Timing bugs](../sum-exe/timing-bugs.md) |
| `0x500b3f` | (instruction) | Mouse poll: `GetCursorPos` + `ScreenToClient` (`0x500b50`); SummonerFix maps coordinates | [SummonerFix](../summonerfix/index.md#display-scaling) |
| `0x500ca7` | (instruction) | Cursor warp: `ClientToScreen` + `SetCursorPos` (`0x500cb7`) | [SummonerFix](../summonerfix/index.md#display-scaling) |
| `0x5014b0` | `FUN_005014b0` | `FrameTimerInit` | [Main loop](../sum-exe/main-loop.md#frame-timer-frametimer-fun_00501510); Volition: `frametime_initialize` |
| `0x5014e4` | (instruction) | `MOV [0x5a574c], 1/120f`: `MIN_DT` init; `MaxFPS` patches the immediate at `0x5014ea` | [SummonerFix](../summonerfix/index.md#fixes) |
| `0x501510` | `FUN_00501510` | `FrameTimer` | [Main loop](../sum-exe/main-loop.md#frame-timer-frametimer-fun_00501510); Volition: `FrameDone` |
| `0x5015bc` | (instruction) | `CALL SleepMs` in `FrameTimer`; SummonerFix redirects to `FrameSleep` | [Timing bugs](../sum-exe/timing-bugs.md#3-frame-limiter-double-counts-its-sleep-2x-game-speed) |
| `0x5016f0` | `FUN_005016f0` | `SetFrameTimeLimits(min, max)` | [Main loop](../sum-exe/main-loop.md) |
| `0x5017f0` | `FUN_005017f0` | `GameClockAdvance(ms)` | [Main loop](../sum-exe/main-loop.md#game-clocks-gameclockadvance-fun_005017f0); Volition: `UpdateTicker` |
| `0x505e00` | `FUN_00505e00` | `GrInit(w, h, renderer, mode, hwnd)` | [Video settings](../sum-exe/video-settings.md#graphics-init-grinit-fun_00505e00) |
| `0x506840` | `FUN_00506840` | `SetClip(x, y, w, h)`: clip rectangle, 27 callers; re-applies the viewport | [SummonerFix](../summonerfix/index.md#display-scaling); Volition: `gr::set_clip` |
| `0x5069a0` | `FUN_005069a0` | `ResetClip()`: full-screen clip | [SummonerFix](../summonerfix/index.md#display-scaling); Volition: `gr::reset_clip` |
| `0x512110` | `FUN_00512110` | `RegSetAppKey` (builds `Software\Volition\Summoner`) | [Video settings](../sum-exe/video-settings.md) |
| `0x512340` | `FUN_00512340` | `RegReadDword(root, name, out, default)` | [Video settings](../sum-exe/video-settings.md) |
| `0x530bf0` | `FUN_00530bf0` | `SleepMs` | [Main loop](../sum-exe/main-loop.md); Volition: `timer_wait` |
| `0x530ca0` | `FUN_00530ca0` | `PumpMessages` wrapper | [Main loop](../sum-exe/main-loop.md) |
| `0x530cb0` | `FUN_00530cb0` | `PumpMessages` (PeekMessage loop) | [Main loop](../sum-exe/main-loop.md) |
| `0x5312d6` | `FUN_005312d6` | CPU identification | - |
| `0x53171f` | `FUN_0053171f` | `PrintSystemInfo` (CPU, RAM, RAM speed) | [Main loop](../sum-exe/main-loop.md#other-timers-not-yet-analysed) |
| `0x532840` | `FUN_00532840` | `FrameClockInit` | [Timing bugs](../sum-exe/timing-bugs.md#1-qpc-frequency-truncation-4-ghz-bug) |
| `0x5328d0` | `FUN_005328d0` | `FrameClockRead(scale)` | [Main loop](../sum-exe/main-loop.md#frame-clock-frameclockinit--frameclockread); Volition: `timer_get` |
| `0x5329b1` | `FUN_005329b1` | `ReadTSC` (RDTSC) | [Timing bugs](../sum-exe/timing-bugs.md#false-lead) |
| `0x5394e0` | `FUN_005394e0` | Direct3D renderer init (`0x66`) | [Video settings](../sum-exe/video-settings.md) |
| `0x5395c0` | `FUN_005395c0` | Direct3D device setup; any failure -> `exit(0)` at `0x539b41` | [SummonerFix](../summonerfix/index.md#direct3d-silent-exit-not-fixed) |
| `0x549d10` | `FUN_00549d10` | Glide renderer init (`0x67`) | [Video settings](../sum-exe/video-settings.md) |
| `0x54e690` | `FUN_0054e690` | `GlTextureInit`: allocates max_texture_size² x 4 bytes | [SummonerFix](../summonerfix/index.md#opengl-failed-to-allocate-1073741824-bytes) |
| `0x54f410` | `FUN_0054f410` | OpenGL init step that calls `GlTextureInit` | [SummonerFix](../summonerfix/index.md) |
| `0x54f6f0` | `FUN_0054f6f0` | OpenGL renderer init (`0x68`, never selected) | [Video settings](../sum-exe/video-settings.md) |
| `0x54f8c0` | `FUN_0054f8c0` | OpenGL `glOrtho` + `glViewport` from the clip rectangle | [SummonerFix](../summonerfix/index.md#display-scaling) |
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
| `0x580e38` | int[2][3] | `resolution_table` | 640x480, 800x600, 1024x768, then `0xffffffff` |
| `0x581d20` | float | 1e-6 | |
| `0x581d24` | float | 1e6 | |
| `0x588a70` | struct[] | `state_table` | `{enter, frame, leave}` x 12 bytes |
| `0x5931f0` | string | `Gamewide_show_framerate` | |
| `0x593314` | string | `FPS=%3.1f` | |
| `0x594868` | strings | command-line flags | `-port` ... `-dsinit` |
| `0x594a04` | string | `InstallPath` | registry value |
| `0x594a10` | string | `Direct 3D` | renderer match string |
| `0x594a1c` | string | `3DFX` | renderer match string |
| `0x594a2c` | string | `Video Card %d` | registry value |
| `0x594a3c` | string | `Selected Video Card` | registry value |
| `0x594a78` | string | `Resolution Height` | registry value |
| `0x594a8c` | string | `Resolution Width` | registry value |
| `0x594ac0` | string | `Volition` | registry company key |
| `0x5a573c` | float | `raw_dt` |; Volition: `actual_frametime` |
| `0x5a5740` | float | `frame_dt` |; Volition: `reported_frametime` |
| `0x5a5744` | float | `fps_avg` |; Volition: `fps` |
| `0x5a5748` | float | `time_scale` | Always 1.0; Volition: `frametime_divisor` |
| `0x5a574c` | float | `MIN_DT` | 1/120; Volition: `min_reported_frametime` |
| `0x5a5750` | float | `MAX_DT` | 0.25; Volition: `max_reported_frametime` |
| `0x5a5754` | int | `ring_index` |; Volition: `frametime_history_index` |
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
| `0x257f040` | float[16] | frame-time ring buffer |; Volition: `frametime_history` |
| `0x257f088` | int | frame count |; Volition: `frames_rendered` |
| `0x257f08c` | int | total ms |; Volition: `milliseconds_unpaused` |
| `0x257f098` | int | last clock reading (µs) |; Volition: `previous_done_time` |
| `0x257f0a4` | int | timer frozen |; Volition: `Frametime_paused` |
| `0x257f0a8` | int | total-ms paused |; Volition: `Animation_timer_paused` |
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
