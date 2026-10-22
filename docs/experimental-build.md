---
title: Experimental build
nav_order: 6.48
---

# Experimental build: new features in the source

**Status (2026-10-21):** the framework and four features are written (controller, tentacle grab, autosave, frame-rate cap and vsync); nothing here has run in
the game yet.

The experimental build (`node tools/link-build.mjs --config experimental`, `build/link-exp/Sum-exp.exe`) is the
[fixed build](fixed-build.md) plus new features, as planned in [Fixed and experimental builds](builds.md). The user
decided (2026-10-21) on one tree with three configurations, so the features are written in the same source, behind
`EXPERIMENTAL_BUILD`.

## How it is built

- **The code** is in `src/decomp/experimental/`, one file per feature, plus `#ifdef EXPERIMENTAL_BUILD` blocks in game
  files where a feature must change the game's own code (the Tentacle Beast grab). `link-build` compiles that folder
  only for the experimental configuration. The byte matching never reads it, and the shipped `Sum-rebuilt.exe` stays
  byte-identical apart from the timestamp (checked when the framework was added).
- **The hooks.** Three calls from the game's code, each under the define (`include/experimental/experimental.h`):
  `exp_frame_begin` in `GameTick` (0x401070) after the mouse is read, for input; `exp_frame_end` before `FrameEnd`,
  for per-frame work; `exp_level_loaded` at the end of `level_load` (0x438570). Each feature's init runs at the first
  frame.
- **The switches** are in `summonerfix.ini` next to the exe, the file `SummonerFixConfig.exe` edits, each off by
  default: a problem can always be ruled out by turning a feature off. `[Debug] ExperimentalLog=1` writes
  `experimental.log` next to the exe.
- **SummonerFix stands down** for a feature the exe has in its source: such code carries
  `// EXPERIMENTAL_BUILD: <feature> <Key>`, `link-build` writes the key into `Sum-exp.fixes`, and SummonerFix's
  `RebuiltFixInSource` check leaves its own version out (the controller and the tentacle grab check it).

## Features

All are written, compile, and keep the shipped build unchanged; none has run in the game yet (in-game tests next).
Switches are in the config app: **Experimental...** (autosave, frame-rate cap, vsync, the log), **Controller...**,
and the Tentacle Beast box on the main window.

| Feature | Switches | Spec / where | Status |
|---|---|---|---|
| Controller, tier 1 | `[Controller] Enabled` and the mapping keys | SummonerFix `controller.c` (the user's test: Xbox-style controllers work, a PS5 DualSense does not); `experimental/controller.cpp` on the game's own input (`mouse_set_pos`, `key_mark`, `mouse_mark_button`), any renderer | written |
| Tentacle Beast grab and throw (G250, G251) | `[Experimental] TentacleGrab`, `TentacleGrabChance`; `[Debug] TraceTentacle` | SummonerFix `tentacle.c`, [Tentacle grab](future/tentacle-grab.md); in `game/unnamed_tentacle_beast_0x4c6480.cpp` | written |
| Autosave | `[Experimental] Autosave`, `AutosaveMinutes` (5), `AutosaveOnLevelChange` (1), `AutosaveSlots` (3) | new; `experimental/autosave.cpp`, two small changes in `game/new_saveload.cpp` | written |
| Frame-rate cap and vsync | `[Experimental] FrameRateCap` (0 = the game's 120; 30-240), `VSync` (OpenGL) | new; `experimental/display.cpp`, one block in `vsdk/frametime.cpp` | written |
| Field of view | none yet | the draw lists hold 500 entries with no bounds check; a wider view could overflow them | proposal only |
| dgVoodoo 2 | none (an outside wrapper; `[Display] Renderer` exists in SummonerFix) | [dgVoodoo 2 (design)](future/dgvoodoo.md) | research done; waits for the user's dgVoodoo files |

**Autosave.** Writes `autosave1.sav` .. `autosaveN.sav` in rotation (the oldest replaced), never `summonerN.sav` or
`quicksave.sav`, and only when the game itself would allow a save (the checks of `savegame_location_allowed`, plus no
multiplayer, menu, message box, cutscene or conversation). Every `AutosaveMinutes` of play (paused time and menus do
not count) and two seconds into a newly entered level (not after loading a save). The description is "Autosave <n> -
<level> - <time>". The Load screen lists the files with "(Auto)" before the level name; the Save screen does not offer
them, so a manual save never lands in the rotation. It does not add a file when the folder already has the 32 saves
the Load screen can show.

**Frame-rate cap.** Sets the frame timer's floor (`min_reported_frametime`); the fixed build's frame limiter (G1)
then reports the real time, so the game's speed does not change, only the smoothness. The cap is put back after a
cutscene, which restores 1/120 (`frametime_restore_limits`; the same happens to SummonerFix's `[Game] MaxFPS` on the
original exe, from the code, not tested). While a cap is set, `FrameDone` carries the sub-millisecond rest into the
next frame, so the game's millisecond timers do not run slow at caps that do not divide 1000 (4% at 120, 13.5% at
144 in the original's truncation). Two frame-rate dependent truncations stay as in the original: the level fade
(slower above 120) and the animation ticks (about 1% slow at 144).
