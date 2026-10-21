---
title: Experimental build
nav_order: 6.48
---

# Experimental build: new features in the source

**Status (2026-10-21):** the framework is in place; the first features are being written. Nothing here has run in
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

| Feature | Switch | Spec | Status |
|---|---|---|---|
| Controller, tier 1 | `[Controller] Enabled` | SummonerFix `controller.c` (tested by the user: Xbox-style controllers work) | being written |
| Tentacle Beast grab and throw | `[Experimental] TentacleGrab` | SummonerFix `tentacle.c`, [Tentacle grab](future/tentacle-grab.md) | being written |
| Autosave | `[Experimental] Autosave` | new; the saving rules of [Savegame](sum-exe/savegame.md#saving-and-loading) | being written |
| Display options (frame-rate cap, vsync) | `[Experimental] ...` | new | being written |
| dgVoodoo 2 | none (an outside wrapper) | [Roadmap](roadmap.md#future-projects) | research |
