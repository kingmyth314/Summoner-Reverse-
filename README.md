# Summoner Reverse

Reverse-engineering notes for **Summoner** (Volition / THQ, 2000), PC version 1.40.
The work is done with [REA](https://github.com/morluto/rea) driving
[Ghidra 12.1.4](https://github.com/NationalSecurityAgency/ghidra) against the shipped
game executable `Sum.exe`.

Full documentation lives in [`docs/`](docs/index.md) and is set up to publish as a
GitHub Pages site.

## Findings so far

| Topic | Summary | Page |
|---|---|---|
| SummonerFix | Our own `dinput.dll` patch: runs on Windows 11 without the launcher (hidden OpenGL renderer), fixes 2x game speed, the >4 GHz timer and the 24.9-day cutscene skip, black save thumbnails; widescreen; key and mouse rebinding with a config app; fast-mode key with an on-screen badge; console-command keys; optional zoom, interaction-range and intro-skip settings | [SummonerFix](docs/summonerfix/index.md) |
| Main loop | Variable timestep, capped at 120 FPS, clamped at 0.25 s per frame, driven by a 15-entry game-state table | [Main loop and frame timer](docs/sum-exe/main-loop.md) |
| Timing bugs | QPC frequency truncated to 32 bits (>4 GHz); frame limiter double-counts its sleep (2x speed); cutscene timer breaks after ~24.9 days of uptime (reproduced and fixed) | [Timing bugs](docs/sum-exe/timing-bugs.md) |
| Characters | Character object, player record, 85 skills, status effects, experience table, resistances | [Characters](docs/sum-exe/characters.md) |
| AI | 13 AI states, goals, attacker slots, paths | [AI](docs/sum-exe/ai.md) |
| Savegame | File format, read-only inspector and an editor that writes new files | [Savegame format](docs/sum-exe/savegame.md) |
| Multiplayer | All 66 packet types and their handlers | [Multiplayer packets](docs/sum-exe/multiplayer.md) |
| Combat | Melee and spell damage formulas | [Combat](docs/sum-exe/combat.md) |
| Game data | items, materials, spells (all 108), character info tables in memory | [Game data tables](docs/sum-exe/game-tables.md) |
| Level scripts | Per-level story logic through a 51-entry handler table | [Level scripts](docs/sum-exe/level-scripts.md) |
| Interface | All 23 interface screens and their functions; UI widget classes | [Interface](docs/sum-exe/interface.md), [UI](docs/sum-exe/vsdk-ui.md) |
| Decompilation | Every function Ghidra finds is named (plus about 2,000 it misses) in `symbols/sum.symbols`; about a quarter of the names are still marked inferred | [Decompilation plan](docs/decomp-plan.md) |
| Timeslip's patch (v3) | Hooks QPC/QPF through the import table, adds a virtual-clock speed control and removes the CD check | [Timeslip's patch](docs/patches/timeslip.md) |

## Repository layout

```
docs/            GitHub Pages site (Markdown, Jekyll "Just the Docs" theme)
src/summonerfix/ SummonerFix source (C, built with LLVM-MinGW)
src/summonerfix-config/ SummonerFixConfig.exe (key bindings and options)
symbols/         Symbol file and structure layouts for Sum.exe
tools/           Build, test and analysis helper scripts
Summoner/        Local game install - NOT committed (see .gitignore)
```

## Legal

This repository contains documentation and original helper scripts only. It does not
contain game files, game executables or third-party patch binaries. You need your own
copy of Summoner to reproduce the analysis.
