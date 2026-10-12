---
title: SummonerFix
nav_order: 4.5
has_children: false
---

# SummonerFix
{: .no_toc }

1. TOC
{:toc}

**Summary:** SummonerFix is our own open-source replacement for Timeslip's patch: a
`dinput.dll` proxy plus `summonerfix.ini` placed next to `Sum.exe`. It makes the game start
on Windows 11 without the original launcher, using the game's built-in but never-enabled
OpenGL renderer. **Status (2026-10-12): the game reaches its main menu in a 1600x900 window
on Windows 11.** The menu is still drawn at 1024x768 in the top-left corner.

Source: `src/summonerfix/`. Build: `tools/build-summonerfix.ps1 [-Install]` (LLVM-MinGW,
`i686-w64-mingw32-gcc`). Test harness: `tools/test-run.ps1`.

## How it works

Sum.exe imports `DINPUT.dll!DirectInputCreateA`, so Windows loads `dinput.dll` from the game
folder before any game code runs. `DllMain`:

1. Reads `summonerfix.ini` and starts `summonerfix.log`.
2. Checks the v1.40 signature (`0x8B52106A` at `0x53983f`). On another build, it applies no
   fixes and only forwards DirectInput.
3. Hooks Sum.exe imports and applies guarded byte patches. Each patch verifies the
   original bytes first and logs a skip if they differ.

`DirectInputCreateA` forwards to the system `dinput.dll` (`SysWOW64`).

## Fixes

| Fix | Mechanism | Why |
|---|---|---|
| No launcher needed | IAT hooks on `RegOpenKeyExA`, `RegCreateKeyExA`, `RegQueryValueExA`, `RegSetValueExA`, `RegDeleteValueA`, `RegDeleteKeyA`, `RegCloseKey` | The launcher can't detect modern cards, so the registry key stays empty. Display and sound values now come from the ini, and values the game writes are kept in ini `[Registry]`. Other keys pass through. See [Video settings](../sum-exe/video-settings.md). |
| Any resolution | Writes the size into resolution-table slot 2 (`0x580e48`, `0x580e4c`) | `LoadSettings` only accepts sizes in the table |
| OpenGL renderer | `0x43da64`: `BF 67` -> `BF 68` (`MOV EDI, 0x68`) | The default renderer is Glide; the OpenGL renderer `0x68` is never selected by the game |
| Windowed mode | `0x43db45`: `68 C9` -> `68 C8` (`PUSH 200`) | `gr_opengl.cpp` refuses fullscreen mode 201 |
| 1 GB allocation | IAT hook on `glGetIntegerv`; caps `GL_MAX_TEXTURE_SIZE` (default 2048) | See below |
| Diagnostics (optional) | Hooks `MessageBoxA`, `ExitProcess`, `HeapAlloc`, `VirtualAlloc`, `GlobalMemoryStatus`; logs them with a heuristic stack trace | Tracing failures to game code |

## Problems found and fixed

### OpenGL: "can only be used in windowed modes for now"

**Observed** at runtime: in fullscreen mode (201) the OpenGL renderer stops with
`Error: OpenGl can only be used in windowed modes for now` (`gr_opengl.cpp` line 657).
Workaround: windowed mode (200). A borderless fullscreen window is still to do.

### OpenGL: "Failed to allocate 1073741824 bytes"

**Observed.** `GlTextureInit` (`FUN_0054e690`, reached from `GrInit` via
`0x54f410`) calls `glGetIntegerv(GL_MAX_TEXTURE_SIZE)` and allocates a scratch buffer of
`size x size x 4` bytes. The test PC's driver reports **16384**, so the request is
16384² x 4 = exactly 1 GB, which fails in a 32-bit process (`memory.cpp` line 223). Stack
from diagnostics: `HeapAlloc` <- `0x5124F2` <- `0x51255E` <- `0x54E72D` <- `0x54F66D` <-
`0x54F73E` <- `0x5060A9` (`GrInit`) <- `0x43DB52` (`LoadSettings`).

Fix: report at most `MaxTextureSize` (2048, so a 16 MB buffer). The 2000-era game textures
are far smaller. *Inferred:* this should not reduce quality; not yet checked in-game.

### Direct3D: silent exit (not fixed)

**Observed.** With `Renderer=Direct3D`, the game exits with code 0 about 4 s after start.
Diagnostics show `exit(0)` called from `0x539B4D` in the Direct3D init
`FUN_005395c0`. Every failed DirectDraw/Direct3D call there jumps to one shared
`exit` block at `0x539b41`, so which call fails is **unknown**. Not pursued while OpenGL
works.

## Configuration (`summonerfix.ini`)

| Key | Default | Meaning |
|---|---|---|
| `[Display] Width`, `Height` | 1024, 768 | Render resolution |
| `[Display] Renderer` | `OpenGL` | `OpenGL`, `Direct3D` or `Glide` |
| `[Display] Windowed` | 0 | 1 = windowed (required for OpenGL for now) |
| `[Display] MaxTextureSize` | 2048 | OpenGL texture-size cap |
| `[Sound] Soundcard` | `DirectSound` | or `No Sound` |
| `[Debug] Diagnostics` | 0 | 1 = log dialogs, exits and large allocations |
| `[Registry]` | | Values the game saved (`Version\*`, and later options) |

Settings the game asked for at startup that were not set: `CmdLineOptions`,
`ConnectionSpeed`, `PackedPixels`, `ComputerSpeed`, `D3D Optimizations` (Direct3D only).
The game uses its defaults for these.

## Test log

| Date | Config | Result |
|---|---|---|
| 2026-10-12 | OpenGL, fullscreen, 1920x1080 | Error: OpenGL only in windowed modes |
| 2026-10-12 | OpenGL, windowed, 1600x900 | Error: failed to allocate 1 GB |
| 2026-10-12 | Direct3D, fullscreen, 1920x1080 | Silent `exit(0)` from D3D init |
| 2026-10-12 | OpenGL, windowed, 1600x900, texture cap 2048 | **Main menu shown**; drawn at 1024x768 in the top-left corner |

## Still to do

- Borderless fullscreen for OpenGL.
- Scale or centre the 1024x768 UI and menus at larger resolutions.
- Play-test in-game rendering, input (DirectInput) and sound.
- Port Timeslip's features (QPC fix, speed keys, Bink fixes, intro skip, zoom, interaction
  ranges, console bindings) and add the cutscene `timeGetTime` fix.
