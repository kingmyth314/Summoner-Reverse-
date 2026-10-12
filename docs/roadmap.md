---
title: Roadmap
nav_order: 6.5
---

# Roadmap

Ideas and planned work that are not started yet. Each item lists what we already know, so
work can start from evidence instead of from scratch.

## Custom key bindings for player actions

**Requested 2026-10-12.** Let players bind their own keys to in-game actions.

What is known so far:

- The game reads the keyboard through DirectInput: Sum.exe imports only
  `DINPUT.dll!DirectInputCreateA`, and SummonerFix is already the `dinput.dll` it loads.
  Every keyboard device the game creates therefore passes through our proxy, so we could
  wrap the DirectInput device and remap key states before the game sees them. *Inferred;*
  the device wrapper is not written yet.
- The game also calls `GetKeyboardState`/`SetKeyboardState` (USER32 imports). Which input
  path the action controls actually use is **unknown**.
- The game has a developer console with named commands. Timeslip's patch binds console
  commands to keys (`[console] k###=command` in its `dinput.ini`). The command list ships in
  `console_commands.txt`. This is a second route for actions that exist as commands.
- Controls the player sets in the game's own options menu are presumably saved through
  the registry calls SummonerFix now redirects into `summonerfix.ini [Registry]`. Not yet
  observed.

First steps:

1. Change a control in the in-game options menu and look at what appears in
   `summonerfix.ini [Registry]`, to find the game's own binding format.
2. Find the code that turns key state into actions (xrefs from the DirectInput
   `GetDeviceState` result buffer).
3. Decide between remapping inside the DirectInput proxy and editing the game's own
   binding table.

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
