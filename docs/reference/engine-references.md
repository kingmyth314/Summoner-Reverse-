---
title: Engine references
parent: Reference
nav_order: 3
---

# Engine references
{: .no_toc }

1. TOC
{:toc}

**Summary:** Summoner (2000), Summoner 2 / *Summoner: A Goddess Reborn* (PS2, 2002) and Red
Faction (2001) are all built on Volition's shared engine library (the `vsdk` source tree
whose paths appear in Sum.exe, for example `pccode\vsdk\gr\opengl\gr_opengl.cpp`). Two
public projects therefore give us **original names and structure layouts** for code we find
in Sum.exe:

| Project | What it is | License | Local copy |
|---|---|---|---|
| [mikegoforth/summoner-2-decomp](https://github.com/mikegoforth/summoner-2-decomp) | Matching decompilation of Summoner 2 (PS2, `SLUS_204.48`): C++ source rebuilt to byte-identical output, with 528 named symbols | **None stated** (all rights reserved by default). Read for reference only; do not copy code into this repository | `reference/summoner-2-decomp` (commit `bdcd147`, 2026-10-12) |
| [rafalh/dashfaction](https://github.com/rafalh/dashfaction) | Community patch for Red Faction (2001): fixes, widescreen, high-FPS fixes, input fixes. Its `game_patch/rf/` headers describe engine structures with addresses | MPL-2.0 | `reference/dashfaction` (commit `b2d61d9`, 2026-01-07) |

`reference/` is git-ignored: these are third-party projects, not part of ours. Code written
for SummonerFix is our own; these projects tell us what things are called and how they are
laid out.

Summoner 2 is a PS2 (MIPS) build and Red Faction a different Windows game, so their addresses
do not apply to Sum.exe. Only names, logic and layouts transfer.

## Confirmed match: frame timer (`frametime.cpp`)

The Summoner 2 decomp's `src/frametime.cpp` is the same function, statement for statement,
as Sum.exe's `FrameTimer` ([Main loop](../sum-exe/main-loop.md#frame-timer-frametimer-fun_00501510)):
the same 1/30 initial values, 1/120 and 0.25 limits, 16-entry history, backwards-clock guard
and `UpdateTicker` call. **Observed** by comparing it with our decompilation. It also shows
the [2x-speed bug](../sum-exe/timing-bugs.md#3-frame-limiter-double-counts-its-sleep-2x-game-speed):
`previous_done_time` is stored before `timer_wait()`.

| Sum.exe address | Our label | Volition name (Summoner 2) |
|---|---|---|
| `0x5014b0` | `FrameTimerInit` | `frametime_initialize()` |
| `0x501510` | `FrameTimer` | `FrameDone()` |
| `0x5017f0` | `GameClockAdvance` | `UpdateTicker(int ms)` |
| `0x5328d0` | `FrameClockRead(scale)` | `timer_get(int scale)` |
| `0x530bf0` | `SleepMs` | `timer_wait(int ms)` |
| `0x5a573c` | `raw_dt` | `actual_frametime` |
| `0x5a5740` | `frame_dt` | `reported_frametime` |
| `0x5a5744` | `fps_avg` | `fps` |
| `0x5a5748` | `time_scale` | `frametime_divisor` |
| `0x5a574c` | `MIN_DT` | `min_reported_frametime` |
| `0x5a5750` | `MAX_DT` | `max_reported_frametime` |
| `0x5a5754` | `ring_index` | `frametime_history_index` |
| `0x257f040` | ring buffer | `frametime_history[16]` |
| `0x257f088` | frame count | `frames_rendered` |
| `0x257f08c` | total ms | `milliseconds_unpaused` |
| `0x257f098` | `last` | `previous_done_time` |
| `0x257f09c` | ring sum | `total_history_frametime` |
| `0x257f0a0` | initialised flag | `Frametime_initialized` |
| `0x257f0a4` | timer frozen | `Frametime_paused` (`increase/decrease_frametime_pause_counter`) |
| `0x257f0a8` | total-ms paused | `Animation_timer_paused` |

## Confirmed match: graphics screen state (`gr::Screen`)

Dash Faction's `rf::gr::Screen` (`game_patch/rf/gr/gr.h`) matches, field for field, the
block that `GrInit` (`FUN_00505e00`) zeroes at `0x25d8d30` (0x2a dwords). **Observed**: every
offset agrees with how Sum.exe uses it.

| Offset | Sum.exe | Field |
|---|---|---|
| +0x00 | `0x25d8d30` | `signature` |
| +0x04 | `0x25d8d34` | `max_w` (screen width) |
| +0x08 | `0x25d8d38` | `max_h` |
| +0x0C | `0x25d8d3c` | `mode` (renderer: `0x66` Direct3D, `0x67` Glide, `0x68` OpenGL) |
| +0x10 | `0x25d8d40` | `window_mode` (200 windowed, 201 fullscreen, 202) |
| +0x18 | `0x25d8d48` | `aspect` |
| +0x28 | `0x25d8d58` | `far_zvalue` |
| +0x2C | `0x25d8d5c` | `offset_x` |
| +0x30 | `0x25d8d60` | `offset_y` |
| +0x34 | `0x25d8d64` | `clip_width` |
| +0x38 | `0x25d8d68` | `clip_height` |
| +0x3C | `0x25d8d6c` | `max_bitmap_w` (set from `GL_MAX_TEXTURE_SIZE` by `GlTextureInit`) |
| +0x40 | `0x25d8d70` | `max_bitmap_h` |
| +0x44 | `0x25d8d74` | `clip_left` |
| +0x48 | `0x25d8d78` | `clip_right` |
| +0x4C | `0x25d8d7c` | `clip_top` |
| +0x50 | `0x25d8d80` | `clip_bottom` |

So our `SetClip` (`FUN_00506840`) corresponds to `gr::set_clip`, and `ResetClip`
(`FUN_005069a0`) to `gr::reset_clip`. The rest of the structure (colour, texture, fog and tint
fields) is a strong lead for later graphics work.

## Where else to look

| Topic | Summoner 2 decomp | Dash Faction |
|---|---|---|
| Timers, pause counters | `src/timer.cpp`, `src/timestamp.cpp` (`GameTimer`, `RealtimeTimer`) | `rf/os/timer.h`, `rf/os/timestamp.h` |
| Console and commands | `src/console.cpp` | `rf/os/console.h`, `game_patch/os/` |
| `.vpp` packfiles | `src/file_packfile.cpp` | `game_patch/misc/vpackfile.cpp` |
| Memory pools | `src/memory.cpp`, `norlsallocpool.cpp` | |
| Parsing tables (`.tbl`) | `src/parse.cpp` | `rf/parse.h` |
| High-FPS problems | | `game_patch/misc/high_fps.cpp` (for example, animations broken by the dropped `ftol` remainder); relevant to the [fast-travel idea](../roadmap.md#turn-the-frame-rate-speed-bug-into-a-fast-travel-speed-control) |
| Keyboard and key bindings | | `game_patch/input/key.cpp`, `rf/input.h`; relevant to [custom key bindings](../roadmap.md#custom-key-bindings-for-player-actions) |
| Widescreen, windowed and borderless modes | | `game_patch/graphics/` |
| Frame-limiter sleep | | `game_patch/os/frametime.cpp` hooks the same frame-time sleep in Red Faction (`frametime_calculate_sleep_hook`) |

## Updating the references

```bash
git -C reference/summoner-2-decomp pull
```

```bash
git -C reference/dashfaction pull
```

The Summoner 2 decomp is very new and changes daily, so its symbol list will grow.
