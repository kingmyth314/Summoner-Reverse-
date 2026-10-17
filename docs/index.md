---
title: Home
layout: home
nav_order: 1
---

# Summoner Reverse

Reverse-engineering notes for **Summoner** (Volition / THQ, 2000), PC version 1.40,
produced with [REA](https://github.com/morluto/rea) and Ghidra 12.1.4.

## Start here

- [Tooling setup](setup.md) - how the REA + Ghidra environment was installed and verified.
- [Sum.exe overview](sum-exe/index.md) - identity, layout and imports of the game executable.
- [Main loop and frame timer](sum-exe/main-loop.md) - how the game advances time each frame.
- [Timing bugs](sum-exe/timing-bugs.md) - the ">4 GHz" bug and the cutscene uptime bug.
- [Bugs in the original game](sum-exe/game-bugs.md) - 358 defects found in the code, and which ones SummonerFix fixes.
- [Bug fix priorities](sum-exe/bug-priorities.md) - the unfixed bugs ranked by gameplay impact and how easily we can reproduce them.
- [Timeslip's patch](patches/timeslip.md) - what the community patch actually changes.
- [SummonerFix](summonerfix/index.md) - our replacement patch: runs the game on Windows 11 via OpenGL.
- [Decompilation plan](decomp-plan.md) - the symbol file, the C++ rewrite in `src/decomp`, and the byte matching: Sum.exe was built with Visual C++ 6.0 SP5 (build 8966), and 4,956 of 5,925 compiled functions match byte for byte, calls and data references included.
- [Roadmap](roadmap.md) - planned work: custom key bindings, a fast-travel speed control, widescreen.
- [Address map](reference/address-map.md) - every address and label used in these notes.
- [Documentation standard](contributing.md) - how new findings are written up.

## Key findings

| Finding | Status |
|---|---|
| The game uses a variable timestep with a 120 FPS cap (`Sleep`) and a 0.25 s clamp | Observed |
| The frame clock is QueryPerformanceCounter-based and truncates the frequency to 32 bits | Observed |
| Timeslip's ">4 GHz" fix halves the QPC frequency until it fits in 32 bits | Observed in the patch DLL |
| Timeslip's speed keys scale a virtual QPC clock (50-200 %) | Observed in the patch DLL |
| Cutscenes skip after ~24.9 days of Windows uptime | Inferred from code; not reproduced |
| Timeslip's patch also removes the CD check (not in its readme) | Observed in the patch DLL |
| The repository's `Sum.exe` already has a separate no-CD edit in the file (`cd_find_drive`, `cd_set_root`) | Observed 2026-10-15 |
| Sum.exe reads its own video settings from `Software\Volition\Summoner`; the launcher isn't required | Observed |
| With no settings the game defaults to the Glide renderer, which needs a 3dfx `glide2x.dll` | Observed |
| Sum.exe contains a complete OpenGL renderer that its settings code never selects | Observed |
| With SummonerFix, the OpenGL renderer reaches the main menu on Windows 11 (windowed) | Observed 2026-10-12 |
| Start New Game crashes when no audio device is present (`snd_init` leaves sound half-enabled) | Observed and fixed 2026-10-14 |
| The retail game still contains Volition's level script editor, gated by `game_flags` bit `0x80` | Observed and tested 2026-10-14 |
| World map encounters: one 50% roll every 10 s while moving | Observed |

## Evidence levels

Every claim in these notes is tagged as one of:

- **Observed** - read directly from decompiled code, assembly or bytes.
- **Inferred** - a conclusion drawn from observed code but not confirmed by running the game.
- **Unknown** - open question.

Function names such as `FrameTimer` are our labels; the shipped binary has no symbols.
Ghidra's default name (for example `FUN_00501510`) is always given alongside.
