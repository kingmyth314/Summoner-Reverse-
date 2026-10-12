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
| Main loop | Variable timestep, capped at 120 FPS, clamped at 0.25 s per frame, driven by a six-entry game-state table | [Main loop and frame timer](docs/sum-exe/main-loop.md) |
| ">4 GHz" timer bug | The QueryPerformanceFrequency result is truncated to 32 bits | [Timing bugs](docs/sum-exe/timing-bugs.md#1-qpc-frequency-truncation-4-ghz-bug) |
| Cutscene skip bug | Cutscene timer reads `timeGetTime` as a signed 32-bit value and breaks after ~24.9 days of uptime (inferred) | [Timing bugs](docs/sum-exe/timing-bugs.md#2-cutscene-timer-sign-bug-249-day-uptime) |
| Video settings | The game reads its own registry settings, accepts only 640x480 / 800x600 / 1024x768, and has an unused OpenGL renderer | [Video settings](docs/sum-exe/video-settings.md) |
| Timeslip's patch (v3) | Hooks QPC/QPF through the import table, adds a virtual-clock speed control and removes the CD check | [Timeslip's patch](docs/patches/timeslip.md) |

## Repository layout

```
docs/            GitHub Pages site (Markdown, Jekyll "Just the Docs" theme)
tools/           Helper scripts used during analysis
Summoner/        Local game install - NOT committed (see .gitignore)
```

## Legal

This repository contains documentation and original helper scripts only. It does not
contain game files, game executables or third-party patch binaries. You need your own
copy of Summoner to reproduce the analysis.
