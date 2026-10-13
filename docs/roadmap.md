---
title: Roadmap
nav_order: 6.5
---

# Roadmap

Ideas and planned work that are not started yet. Each item lists what we already know, so
work can start from evidence instead of from scratch.

## Custom key bindings for player actions

**Requested 2026-10-12.** Let players bind their own keys to in-game actions.

**Update 2026-10-13: the input path is mapped.** See
[Controls and keyboard map](sum-exe/controls.md).

- The keyboard does **not** use DirectInput. Keys come from window messages through
  `key_mark`; only the mouse uses DirectInput.
- The controls are a hard-coded `switch` in `hud_handle_keys` (`0x40d930`). There is no
  binding table to edit and no in-game control options.
- Every key event first goes through `key_remap_layout` (`0x509560`), a scan code → scan
  code function used for AZERTY/QWERTZ layouts.

Plan (step 1 **implemented 2026-10-13, untested**: `src/summonerfix/keys.c`, which redirects the single `CALL` at `0x5093a6` instead of patching the function):

1. In SummonerFix, patch a `JMP` at `0x509560` to our own `remap(code)`. It applies a
   256-entry table from a new `[KeyBindings]` section of `summonerfix.ini` (for example
   `I=Tab`), then the original layout swaps.
2. Test that remapped keys open the right screens and that held keys (camera arrows) follow
   the remap.
3. Optional later steps:
   - mouse-button remapping in `mouse_update`;
   - binding console commands to keys, as Timeslip's patch does.

Timeslip's patch already binds console commands to keys (`[console] k###=command`). That
stays a separate route for actions that only exist as console commands.

## Turn the frame-rate speed bug into a "fast travel" speed control

**Requested 2026-10-12.** The frame-limiter bug that made the game run at 2x speed (see
[Timing bugs](sum-exe/timing-bugs.md#3-frame-limiter-double-counts-its-sleep-2x-game-speed))
shows the game copes with game time running faster than real time. Offer it deliberately,
for example as a hotkey to speed up walking long distances.

What is known so far:

- Game time comes from `FrameTimer` (`FUN_00501510`). Its result `frame_dt` (`0x5a5740`)
  drives gameplay, and is read at 80+ sites.
- Three possible levers:
  1. `time_scale` (`0x5a5748`): `FrameTimer` already divides `frame_dt` by it, and it is
     always 1.0. A value of 0.5 would mean 2x speed. This is the cleanest option.
  2. SummonerFix's `FrameSleep` (`src/summonerfix/timing.c`) could report several
     `MIN_DT` per real frame. This is closest to what the bug did.
  3. Timeslip's approach: a virtual `QueryPerformanceCounter` clock running at 50-200%
     (see [Timeslip's patch](patches/timeslip.md#the-speed-control)).
- Limits to respect: `frame_dt` is clamped to `MAX_DT` = 0.25 s, and some logic is
  frame-based rather than time-based (for example the `ai_frame_update_rate` console
  variable). Very high multipliers may change gameplay or physics.
- The cutscene `SequenceTimer` uses `timeGetTime`, not `FrameTimer`, so cutscene timing
  stays unaffected by levers 1 and 2. Whether that is right during a speed-up is to be
  decided.

First steps:

1. Prototype lever 1: a hotkey that toggles `time_scale` between 1.0 and 0.5 (and 0.25),
   with a small on-screen indicator.
2. Play-test walking, combat, AI and scripted events at 2x and 4x.
3. Add `[Game] FastTravelKey` and `FastTravelSpeed` to `summonerfix.ini`.

## Decompilation

See the [Decompilation plan](decomp-plan.md): map the whole binary first (engine core
first), structured so it can become a byte-matching decompilation later.

## Other planned work

- Widescreen (16:9) instead of 4:3 pillarboxing, with the 1024-wide UI kept centred.
- Port the remaining Timeslip features: speed keys, Bink 32-bit fixes, intro-movie skip,
  camera zoom range, interaction ranges, console key bindings.
- Fix the cutscene uptime bug
  ([Timing bugs](sum-exe/timing-bugs.md#2-cutscene-timer-sign-bug-249-day-uptime)) by hooking
  `timeGetTime`.
- Direct3D renderer: find which DirectDraw/Direct3D call fails at `0x539b41`
  (low priority while OpenGL works).
