---
title: Sum.exe
nav_order: 4
has_children: true
---

# Sum.exe overview

`Sum.exe` is the game executable. Timeslip's patch installs next to it and requires
v1.40. *Assumption, not yet verified:* `Summoner.exe` is the launcher that starts it.

## Identity

| Property | Value |
|---|---|
| File | `Summoner\Sum.exe` |
| Size | 1,785,856 bytes |
| SHA-256 | `f22c84a17ae4023a9113a68c76647feee41044ecde56ffb358744db47f8bf9d3` |
| Format | PE32, x86, image base `0x400000` |
| Version | 1.40 (assumed from Timeslip's version check passing on byte `0x53983f`; not separately verified) |
| Symbols | None. All names below are our labels. |
| Analysis | Ghidra 12.1.4 via REA 6.4.0, default analyzers |

## Layout

| Segment | Start | End | Size |
|---|---|---|---|
| Headers | `0x400000` | `0x401000` | 4 KB |
| `.text` | `0x401000` | `0x57e000` | 1.49 MB |
| `.rdata` | `0x57e000` | `0x588000` | 40 KB |
| `.data` | `0x588000` | `0x2fbf2f8` | 42.2 MB (mostly uninitialised) |
| `.rsrc` | `0x2fc0000` | `0x2fc1000` | 4 KB |

Ghidra found **6,612 functions** and **5,728 defined strings**.

## Timing-related imports

Ghidra names imports `DLL::Function`, for example `WINMM.DLL::timeGetTime`.

| Import | IAT slot | Used by |
|---|---|---|
| `KERNEL32::QueryPerformanceCounter` | `0x57e1c8` | Frame clock (`0x532840`, `0x5328d0`), CPU info |
| `KERNEL32::QueryPerformanceFrequency` | `0x57e1cc` | Frame clock init (`0x532840`), CPU info |
| `KERNEL32::Sleep` | `0x57e200` | Frame limiter wrapper `0x530bf0`, 20 call sites total |
| `WINMM::timeGetTime` | `0x57e3fc` | Cutscene/sequence timer `0x436a90`, `0x436c00`, `0x494310`, CPU info |
| `WINMM::timeBeginPeriod` | `0x57e3dc` | `0x566cf0`, called with 1 ms |
| `WINMM::timeSetEvent` | `0x57e408` | `0x566560` (probably audio; not investigated) |
| `USER32::PeekMessageA` | `0x57e3c4` | Message pump `0x530cb0` |

## Other files in the install

| File | Size | Notes |
|---|---|---|
| `Summoner.exe` | 3,047,424 bytes | Probably the launcher; not analysed yet |
| `UpdateLauncher.exe` | 491,520 bytes | Not analysed |
| `binkw32.dll` | | RAD Bink video runtime |
| `*.vpp` | | Volition packfiles (game data) |

## Pages in this section

- [Main loop and frame timer](main-loop.md)
- [Timing bugs](timing-bugs.md)
- [Video settings and renderers](video-settings.md)
- [Source-file map](source-map.md)
- [vsdk: OS layer](vsdk-os.md)
- [Console commands](console.md)
- [vsdk: parser (TextFile)](vsdk-parse.md)
- [vsdk: packfiles (.vpp)](vsdk-packfile.md)
- [vsdk: bitmap manager](vsdk-bmpman.md)
- [Localization](localize.md)
- [vsdk: graphics and keyboard](vsdk-gr.md)
- [vsdk: math](vsdk-math.md)
- [vsdk: characters](vsdk-ca.md)
- [vsdk: user interface](vsdk-ui.md)
- [Interface screens](interface.md)
- [Controls and keyboard map](controls.md)
- [Characters, experience and levels](characters.md)
- [Combat: melee and spell damage](combat.md)
- [AI](ai.md)
- [Level scripts](level-scripts.md)
- [Game data tables in memory](game-tables.md)
- [Savegame format](savegame.md)
- [Multiplayer packets](multiplayer.md)
