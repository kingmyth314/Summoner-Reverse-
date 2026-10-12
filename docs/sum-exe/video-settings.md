---
title: Video settings and renderers
parent: Sum.exe
nav_order: 3
---

# Video settings and renderers
{: .no_toc }

1. TOC
{:toc}

**Summary:** Sum.exe reads its video settings from the registry itself; the launcher is only
a front end that writes them. With no settings, the game falls back to 640x480 and the
**Glide** renderer, which needs a 3dfx `glide2x.dll`. The game also contains a complete
**OpenGL renderer that the shipped settings code never selects**.

## Why the launcher blocks the game

On the test PC (Windows 11), the launcher (`Summoner.exe`) cannot detect a video card or
offer a resolution, and refuses to start the game until both are set (**observed** by the
user). The `Software\Volition\Summoner` registry key is empty there (**observed**).

The launcher detects cards with `DirectDrawEnumerateA` and `DirectDrawCreateEx`, and stores
`Video Card %d`, `Video Card %d Driver`, `Video Card %d Vendor ID`, `Video Card %d Device ID`,
`Video Card %d Resolutions`, `Detect D3D` and `Detect Glide` (**observed** strings).
*Inferred, not yet confirmed:* it only accepts 16-bit-colour display modes or old
DirectDraw device types, which Windows 10/11 no longer reports.

## Settings loader: `LoadSettings` (`FUN_0043d7f0`)

This is the function earlier labelled `InitTimers`; it does all of the settings setup.
**Observed:**

1. Sets the registry key to `Software\Volition\Summoner` (`FUN_00530d10`, `FUN_00512110`).
2. Writes `Version\Major = 1`, `Minor = 40` (`0x28`), `Build = 0`.
3. Reads `Resolution Width` (default `0x280` = 640) and `Resolution Height` (default
   `0x1e0` = 480) via `RegReadDword` (`FUN_00512340`).
4. Command-line flags override this: one gives 800x600, another 1024x768 (the strings
   `-800` and `-1024` exist; which flag maps to which branch is *inferred*).
5. Looks the size up in the **resolution table** (`0x580e38`). If there is no match, the
   previous index is kept.
6. Reads `Soundcard` (default `DirectSound`; `No Sound` disables audio).
7. **Renderer choice:** reads `Selected Video Card` (default -1) and then the string value
   `Video Card <n>`. If that string is not `None`, does not contain `3DFX` and does contain
   `Direct 3D`, the renderer is Direct3D (`0x66`). The `-direct3d` flag also forces `0x66`.
   **Otherwise the renderer is Glide (`0x67`).**
8. Calls `GrInit(width, height, renderer, 201 /*fullscreen*/, ...)` (`FUN_00505e00`).
9. Reads `InstallPath`.

### Resolution table (`0x580e38`)

| Index | Width | Height |
|---|---|---|
| 0 | 640 | 480 |
| 1 | 800 | 600 |
| 2 | 1024 | 768 |
| - | `0xffffffff` | end marker |

The lookup loop stops at `0x580e53`, so only these three sizes are accepted. Timeslip's
patch overwrites entry 2 (`0x580e48`, `0x580e4c`) with the custom resolution.

### Command-line flags (strings near `0x594868`)

`-port`, `-directsound3d`, `-eax`, `-pack`, `-glide`, `-direct3d`, `-800`, `-1024`,
`-dsinit`. Parsed by the `vsdk\os\cmdline.cpp` module (source path string in the launcher).

## Graphics init: `GrInit` (`FUN_00505e00`)

**Observed.** Arguments: width, height, renderer, window mode, window handle.

| Window mode | Behaviour |
|---|---|
| 200 | Windowed: caption window centred on the work area |
| 201 | Fullscreen: borderless popup sized to `GetSystemMetrics` (the desktop), topmost |
| 202 | Desktop size, no window changes |

The UI aspect factor is computed as `width / height x 0.75` (so 1.0 at 4:3).

| Renderer | Init function | Source file (from the binary) | API |
|---|---|---|---|
| `0x66` | `FUN_005394e0` | `vsdk\gr\directx\gr_direct3d.cpp` | Direct3D through `ddraw.dll` |
| `0x67` | `FUN_00549d10` | `vsdk\gr\glide\gr_glide.cpp` | 3dfx Glide (`glide2x.dll`, fullscreen only) |
| `0x68` | `FUN_0054f6f0` | `vsdk\gr\opengl\gr_opengl.cpp` | OpenGL (`ChoosePixelFormat`, `SetPixelFormat`, `wglCreateContext`) |

Renderer-to-file mapping is **inferred** from each init function's reachable imports plus
the embedded source paths and error strings (`Glide DLL not found!`,
`Invalid window_mode passed to gr_direct3d_init`).

{: .important }
`LoadSettings` only ever picks `0x66` or `0x67`. No code path found so far selects the
OpenGL renderer `0x68`. Whether it works (or is complete) is **unknown**; it is a
candidate for the modern-driver fix, because OpenGL runs natively at any resolution.

## Evidence

| Claim | REA evidence ID |
|---|---|
| `LoadSettings` pseudocode | `ev_8f5bb708b74e437932dece08d6df9de98f0dda8f35911cb6b1eb40fa31199ab7` |
| Resolution table bytes | `ev_b1550c23668ff2e89d99328c63aac1ca8b069591c4d0142304f874195a62a367` |
| `GrInit` pseudocode | `ev_23a788dd1e5d4556cd12f253d01ff48f43e9e5b54e256cb9db3cfe0a4d283082` |
| Registry key builder | `ev_eebcc982b1993c452fefd10a0ea00ef3d194282709fc6a43807adcb9d85c9f5c` |
| `RegReadDword` | `ev_511654e29688143f5c66a35bbd33458167ccd6a09086d35c56c20931ec9cb713` |
| `0x549d10` call graph (Glide) | `ev_980f22ba14e83c8c13295032c9d75584a650d65dc42a8d397525c332d6c22dcc` |
