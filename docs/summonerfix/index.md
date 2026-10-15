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
OpenGL renderer. The game keeps its native 1024x768 layout, and the image is scaled to fill a
borderless fullscreen window (4:3, centred), with the 3D world rasterised at the monitor's
native resolution. **Status (2026-10-14):** user-tested on Windows 11 at 2560x1440: key and mouse rebinding, the config app, Alt+Tab,
widescreen (`GameWidth=1366`), shops, looting, and the optional zoom and range settings all work. Fast mode (toggle,
"FAST x3" badge), the cutscene-uptime fix and the save-thumbnail fix were verified in agent mode. Earlier play-tests
confirmed 120 FPS at 1:1 game speed.

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
| Game resolution | `GameWidth`/`GameHeight` (default 1024x768); other sizes are written into resolution-table slot 2 (`0x580e48`, `0x580e4c`) | `LoadSettings` only accepts sizes in the table. Keep 1024x768: UI, clip rectangles and menus are laid out for it |
| OpenGL renderer | `0x43da64`: `BF 67` -> `BF 68` (`MOV EDI, 0x68`) | The default renderer is Glide; the OpenGL renderer `0x68` is never selected by the game |
| Windowed mode | `0x43db45`: `68 C9` -> `68 C8` (`PUSH 200`), always for OpenGL | `gr_opengl.cpp` refuses fullscreen mode 201 |
| Fullscreen output and scaling | Hooks on `SetWindowLongA`/`SetWindowPos` (only calls from `GrInit`), `glViewport`, `SwapBuffers` | See [Display scaling](#display-scaling) |
| Mouse mapping | Hooks on `ScreenToClient` (game poll at `0x500b50` in [`mouse_update`](../sum-exe/vsdk-gr.md#mouse-0x500a90-0x501090)) and `ClientToScreen` (cursor warp at `0x500ca7`) | Converts window pixels to game pixels and back |
| Correct 3D aspect | Hook on `GetSystemMetrics` for calls from `GrInit` | Windowed `GrInit` takes the aspect from the desktop (16:9) instead of the game's 4:3 |
| 2x game speed | `CALL SleepMs` at `0x5015bc` -> `FrameSleep` | See [Frame limiter](#frame-limiter-2x-speed) |
| Cutscenes skip after ~24.9 days of uptime | IAT hooks on `timeGetTime` and `GetMessageTime` return time since start-up (both rebased by the same amount, because `0x509880` subtracts one from the other for key timing) | `cutscene_sequence_update` subtracts its start time as a signed 32-bit value; see [Timing bugs](../sum-exe/timing-bugs.md#2-cutscene-timer-sign-bug-249-day-uptime). Tested 2026-10-14 in agent mode: with `[Debug] FakeUptimeDays=30` the new-game intro is skipped within 2 s (the reported bug), and with the fix it plays normally |
| Crash on Start New Game without sound (`bugfix.c`) | `mov eax, [esi+0xc]; test eax, eax` at `0x55c5db` in `ds_channel_release` -> `CALL` to a stub that returns -1 (no stream) unless `ds_init` finished (flag byte `0x2f8f73c` = 1) | When DirectSound cannot start (`DirectSoundCreate` fails, e.g. no audio output device connected), `ds_init` returns before `ds_init_channels` and `snd_stream_init`. The channel records stay zero and the stream lock (`0x2fbd998`) is never initialised. The level start then calls `ds_channel_release`, takes stream index 0 as valid and enters the zeroed lock: Access Violation in ntdll (write to `0x14`). Found 2026-10-14 in agent mode with every audio endpoint disconnected: the crash happened on every try, at 1024 and 1366. With the fix the intro cutscene plays. With sound working nothing changes. With `Soundcard=No Sound` a new game also starts (tested). That setting never had the bug: it sets `snd_disabled`, so no `ds_*` code runs. The bug is that `snd_init` skips setting `snd_disabled` when `ds_init` fails; see [Sound and music](../sum-exe/sound.md#start-up) |
| Low textures on a fresh install with OpenGL (`bugfix.c`) | `mov eax, 8` at `0x507a03` in `gr_get_video_memory_mb` -> `mov eax, 64` | The OpenGL back end reports a constant 8 MB of video memory. Its only caller, `summoner_cfg_load`, turns on reduced textures when `summoner.cfg` is missing or shorter than 64 bytes and the value is 8 or less, so a first start under SummonerFix chose low-resolution textures until the player saved the options. The patch applies (log, 2026-10-15); the texture choice was not tested, because that needs a missing `summoner.cfg`. See [Bugs in the original game](../sum-exe/game-bugs.md) K64 |
| Optional gameplay settings (`gameopts.c`) | `CameraZoomMax` writes `camera_zoom_max` (`0x59b800`, game 1.0). `SkipIntroMovies` clears `play_intro_movies` (`0x5930bd`). `InteractRange` and `ContainerRange` point the distance operands at `0x40e696` (`hud_interact_nearest`, 5.0) and `0x43ad38` (`world_mouse_frame`, 25.0 = 5 squared) at our own floats, because those constants are shared with many other sites | Timeslip's camera zoom, movie skip and interaction-range options, reimplemented from our own analysis. All are off by default. Checked 2026-10-14 in agent mode: the values are patched, and with `SkipIntroMovies=1` the main menu shows at once instead of the logo movies. User-tested 2026-10-14: zoom, talk/use range and container range all work |
| Console-command keys | `[ConsoleKeys] Key=command`: our window-message handler (registered with `os_add_msg_handler`, so it runs on the game thread) calls `execute_command_line` (`0x50a210`) on a key press. Live reload like the bindings | Timeslip's `[console] k###=command`. Tested 2026-10-14 in agent mode with a real posted `WM_KEYDOWN`: `F9=add_gold 1000` raised the party's gold from 61 to 1061 |
| Savegame thumbnails | Hook on `glReadPixels` (only caller: `gr_opengl_read_backbuffer`, `0x5502c0`): reads the game-space rectangle mapped to the scaled output and box-filters it to the requested size | `saveload_capture_thumbnail` (`0x427100`) renders the scene into a 201x151 clip and reads it back in game pixels, but our framebuffer is the scaled output, so thumbnails came out black. Fixed 2026-10-14 and checked in agent mode (the Save screen preview shows the scene). Saves made before the fix keep their black thumbnails |
| Frame cap (optional) | `MaxFPS` writes `MIN_DT` into the immediate at `0x5014ea` (`MOV [0x5a574c], imm` at `0x5014e4`) | Default 0 keeps the game's 120 FPS |
| 1 GB allocation | IAT hook on `glGetIntegerv`; caps `GL_MAX_TEXTURE_SIZE` (default 2048) | See below |
| Custom key bindings | `CALL key_remap_layout` at `0x5093a6` in `key_mark` -> `KeyRemap`: the original layout swap, then a 256-entry table from `[KeyBindings]`. A watcher thread reloads the table when `summonerfix.ini` changes, so bindings change live | The game key map is hard-coded; see [Controls](../sum-exe/controls.md). Tested 2026-10-13 (E=I, A=Left) |
| Centred menus at wide resolutions | `GetMap` (viewport, bar clearing and mouse mapping) shifts the picture by `(GameWidth - 1024) / 2` game pixels while the game state is not 4 (gameplay) | Menus are a fixed 1024-wide layout; at `GameWidth=1366` they sat at the left with an empty right side. Checked by frame capture 2026-10-13: centred with even bars. `[Display] CenterMenus=0` turns it off |
| Centred in-game UI at wide resolutions (user-tested 2026-10-13/14) | `src/summonerfix/widescreen.c`. In game state 4, calls from UI code (`0x401000`-`0x430000`, vsdk ui, conversation text widgets `0x4d6600`-`0x4d7ab0`) to the 2D primitives are redirected (about 400 call sites) and x is shifted by `(GameWidth - 1024) / 2`. UI-set clips get the shift instead of their draws. UI code sees `gr_screen_width() = 1024`. The mouse is given to UI code in layout space, and the 9 world-code reads use the raw position. `interface_set_view_clip` (3D view) is excluded. Full-screen panels (inventory, character, journal, options, save/load) instead shift the whole output like the menus, because their item and character models are rendered in screen coordinates. Side panels (map, spells, skills, item trade) stay flush left (their calls in `interface_draw`/`interface_frame` run unshifted with the raw cursor). In `interface_set_view_clip`, full panels get a 1024-wide 3D view, so models in the panel are projected around the layout centre. Side panels keep the world full width behind them (`[Display] SidePanelsOverWorld`), so the scene does not move. While a full panel is open, `gr_screen.aspect` is set to 1.0, because the projection divides by it and models were 25% too small. Checked with agent-mode screenshots 2026-10-13: the inventory matches 1024x768 exactly | HUD and in-game screens are a fixed 1024-wide layout at the left; the 3D world already uses the full width. `[Display] CenterHud=0` turns it off |
| Mouse bindings | The six `CALL mouse_mark_button` sites in the mouse message handler (`0x500f79`-`0x500fba`) -> `MouseMark`; our own handler registered with `os_add_msg_handler` (`0x530c20`) for `WM_XBUTTON*` (side buttons) and for keys bound to mouse buttons; mouse-to-key bindings are injected through `key_mark` | `[KeyBindings]` accepts `MouseLeft`, `MouseRight`, `MouseMiddle`, `Mouse4`, `Mouse5`, e.g. `Mouse4=M`, `Q=MouseRight`, `MouseMiddle=Space`. Tested 2026-10-14 in agent mode with posted window messages: `Q=MouseLeft` over the chest icon opened the inventory, and `Mouse4=R` (`WM_XBUTTONDOWN`) opened the Character screen |
| Speed-up key (fast travel) | `[Game] SpeedKey` (default F11) turns fast mode on and off (`SpeedMode=Toggle`, the default since 2026-10-14) or works while held (`SpeedMode=Hold`). While fast mode is on during gameplay, a "FAST x3" badge is drawn at the top left of the picture (OpenGL, in the `SwapBuffers` hook). In fast mode, `frametime_divisor` (`0x5a5748`) = `1 / SpeedFactor` | `FrameDone` reports frame time / divisor, so the whole game advances faster. Tested 2026-10-14 in agent mode (`speedkey 1`): in one real second the game clock `Timestamp_ticker` advanced 968 ms normally and 3000 ms with the key held (`SpeedFactor=3`). Combat and AI at high factors are not play-tested |
| Built-in script editor (`[Debug] ScriptEditorKey`, off by default) | A key handled on the game thread (our `os_add_msg_handler` handler) toggles bit `0x80` of `game_flags` (`0x5fbfd8`) during gameplay. `script_editor_frame` runs Volition's level script editor only while that bit is set, and nothing in the retail game sets it | For modders. Tested 2026-10-14 in agent mode with `F12`: the editor's main bar opens and closes, and the HUD comes back. **Its Save button writes `.tbl` files into the game's data folders**, so back up first. See [Script editor](../sum-exe/script-editor.md) |
| Agent mode (`[Debug] Agent=1`) | `agent.c`: commands from `Summoner\agent\cmd.txt` inject game keys/mouse buttons, set the cursor, take screenshots, report state; keeps the game from pausing | Lets scripts (`tools/agent.mjs`) play and test the game; tested 2026-10-13 (menu, load, inventory) |
| Typing ignores key bindings | `key_to_ascii` (`0x509ca0`) -> `KeyToAscii`: `KeyRemap` records which physical key produced each game key code; text entry translates back before the character lookup | With `E=I` bound, typing a save name with E gave "i" (user report 2026-10-13). Tested 2026-10-14 in agent mode: with W, A, D and E all rebound, posted key presses typed "wade" in the save description (cancelled, nothing saved) |
| Alt+Tab works | `SetWindowPos` hook: the call from the game's focus-loss handler (return address `0x5310ef`) becomes `HWND_BOTTOM` + `SWP_NOACTIVATE`, and `SWP_NOACTIVATE` is added to the focus-gain call (`0x5310b7`). The F10 hotkey (`[Game] ConfigKey`) opens the key binding app, which is topmost | The game re-activated itself on losing focus, so other programs (such as `SummonerFixConfig.exe`) could not be brought forward; see [Controls](../sum-exe/controls.md#switching-away-from-the-game-alttab). Tested 2026-10-13 |
| Diagnostics (optional) | Hooks `MessageBoxA`, `ExitProcess`, `HeapAlloc`, `VirtualAlloc`, `GlobalMemoryStatus`, `Sleep`; logs them with a heuristic stack trace; logs FPS, sleep and game-clock statistics every 5 s; optional frame captures from the back buffer | Tracing failures to game code |

## Problems found and fixed

### OpenGL: "can only be used in windowed modes for now"

**Observed** at runtime: in fullscreen mode (201) the OpenGL renderer stops with
`Error: OpenGl can only be used in windowed modes for now` (`gr_opengl.cpp` line 657).
Workaround: always use windowed mode (200) and turn that window into a borderless fullscreen
window (see below).

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

### Display scaling

**Why:** the game lays out everything in fixed pixels for its internal resolution: UI
positions, menu bitmaps, and `SetClip(x, y, w, h)` (`FUN_00506840`, 27 callers). Timeslip's
patch moved UI elements around (a position table at `0x58acbc`-`0x58bc40`), which left menus
misplaced. Running the OpenGL renderer at a larger size than 1024x768 drew everything in
the top-left 1024x768, and the uncovered area showed garbage under the mouse (user report).

**How:** the game keeps its 1024x768 coordinates. `glOrtho` stays in game pixels; only
`glViewport` is changed, mapped to a centred, aspect-correct rectangle of the real window
(`scale = min(clientW/1024, clientH/768)`). OpenGL then rasterises all geometry at the
window's native resolution. Before each `SwapBuffers` the unused bars are cleared with a
scissored `glClear`. Mouse coordinates are mapped back to game pixels.

**Observed:** main menu centred at 2560x1440 with black bars (frame capture, 2026-10-12).

### Frame limiter: 2x speed

**Observed.** The game's frame limiter double-counts its sleep, so on a fast PC game time
runs at twice real time. Details: [Timing bugs](../sum-exe/timing-bugs.md#3-frame-limiter-double-counts-its-sleep-2x-game-speed).

`FrameSleep` (in `timing.c`) replaces the limiter's sleep. It waits with a high-resolution
waitable timer until exactly one `MIN_DT` after the previous frame's end (kept internally,
because `FrameTimer` has already overwritten its own timestamp), then sets the game's
`last` timestamp (`0x257f098`) to that time.

| Build | Real FPS | Game's own FPS | Game speed |
|---|---|---|---|
| Original | ~218 | ~115 | about 2x |
| `last` = wake-up time | ~113 | 120 | 0.94x |
| Final (`last` = scheduled time) | **120.0** | **120.0** | **1x** |

The game clock (`FrameClockRead`) was checked against QPC: 5001.2 ms vs 5001.2 ms over 5 s.

## Key binding app (`SummonerFixConfig.exe`)

`SummonerFixConfig.exe` (source: `src/summonerfix-config/config.c`) is installed next to `Sum.exe`. It lists the game's keyboard actions, including the four chain-attack slots (held modifier keys) and SummonerFix's own speed-up and open-this-window hotkeys, with their default keys and any extra keys bound to them:

1. Select an action and click **Change key...** (or double-click it).
2. Press the key you want. The app writes `[KeyBindings]` in `summonerfix.ini` straight away.
3. A running game reloads the bindings within a fraction of a second, so you can switch to the app, rebind and return.

**Clear** removes the extra keys from one action, and **Reset all** restores the game defaults. Mouse buttons, including the side buttons Mouse4/Mouse5, can be bound like keys: click them while the app waits for a key. The rows for left, right and middle click let a key act as a mouse button, for example a key for right-click chain attacks. Rebinding the left button away also removes it from the menus; Reset all undoes that.

**Game options** (right-hand column, added 2026-10-14) edit the optional `[Game]` settings: **Skip intro movies**,
**Speed key toggles**, the zoom-out limit, the talk/use range and the container range (each shows the standard game's value). Leave a field empty for the game's own value.
**Save options** writes them; they take effect the next time the game starts.

**Console keys** (added 2026-10-14): type a console command (for example `add_gold 1000`) in the box under the
options and click **Bind key...**, then press the key. The binding is written to `[ConsoleKeys]` and shows as a
"Console: ..." row in the list, where **Change key...** and **Clear** work on it. Checked with a real key press. The same
change fixed a bug: rebinding the speed or config hotkey in the app saved a garbled key name, because the key name was
taken from an unused buffer.

A binding adds a key; the default key keeps working. If the new key already belonged to another action, that action loses it, and the app says so. The list shows "(moved)" next to a default key that now does something else.

## Configuration (`summonerfix.ini`)

| Key | Default | Meaning |
|---|---|---|
| `[Display] Renderer` | `OpenGL` | `OpenGL`, `Direct3D` or `Glide` |
| `[Display] Mode` | `Fullscreen` | OpenGL output: `Fullscreen` (borderless, monitor size) or `Window` |
| `[Display] WindowWidth`, `WindowHeight` | 1600, 1200 | Client size for `Mode=Window` (resizable) |
| `[Display] GameWidth`, `GameHeight` | 1024, 768 | The game's internal layout resolution; e.g. 1366x768 for 16:9 |
| `[Display] CenterMenus` | 1 | With a wider `GameWidth`, centre the 1024-wide menus |
| `[Display] SidePanelsOverWorld` | 1 | Wide screens: the 3D view stays full width behind the map/spells/skills panels |
| `[Display] CenterHud` | 1 | With a wider `GameWidth`, centre the HUD and in-game screens (OpenGL) |
| `[Display] Windowed` | 0 | Direct3D/Glide only: use the game's own windowed mode |
| `[Game] MaxFPS` | 0 | Frame cap; 0 = game default 120 |
| `[Display] MaxTextureSize` | 2048 | OpenGL texture-size cap |
| `[Sound] Soundcard` | `DirectSound` | or `No Sound` |
| `[Debug] Diagnostics` | 0 | 1 = log dialogs, exits, large allocations, FPS and sleep statistics |
| `[Debug] CaptureEvery` | 0 | Seconds between back-buffer captures (`summonerfix-capture-N.bmp`) |
| `[Debug] FakeUptimeDays` | 0 | Testing only: the game sees this many days of Windows uptime (25+ reproduces the cutscene timer bug) |
| `[Debug] ScriptEditorKey` | none | Key that toggles the built-in level script editor in game (for modders; its Save writes into the data folders) |
| `[Game] CameraZoomMax` | (empty) | Camera zoom-out limit; the game's is 1.0, Timeslip used 3 |
| `[Game] SkipIntroMovies` | 0 | 1 = skip the Volition and EAX logo movies at start-up (the credits movie still plays) |
| `[Game] InteractRange`, `ContainerRange` | (empty) | How far away you can talk to or use things, and open containers, in world units (game: 5) |
| `[ConsoleKeys]` | (empty) | `Key=command` lines, e.g. `F9=add_gold 1000`; commands are listed in [Console](../sum-exe/console.md) |
| `[Game] SpeedKey`, `SpeedFactor`, `SpeedMode` | `F11`, 3, `Toggle` | Run the game faster (1-8x): `Toggle` = press to switch on/off, `Hold` = while held; `none` disables the key |
| `[Game] ConfigKey` | `F10` | Opens `SummonerFixConfig.exe` while playing; `none` disables it |
| `[KeyBindings]` | (empty) | `PhysicalKey=GameKey` lines, e.g. `E=I`; key names or hex scan codes |
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
| 2026-10-12 | Same, user play-test (about 5 min) | Sound and cutscenes work; image limited to 1024x768 top-left; display glitches over the uncovered area; game "super fast" |
| 2026-10-12 | OpenGL, borderless fullscreen 2560x1440, viewport scaling | Menu centred 4:3 with bars (frame capture). 218 FPS measured, 2x speed |
| 2026-10-12 | Same + FrameSleep (final) | **120.0 FPS real = 120.0 game**; 1:1 speed |
| 2026-10-12 | Same, user play-test | Everything works; game feels better |
| 2026-10-13 | Key bindings (E=I, A=Left, WASD camera, CapsLock/Home remaps) | Bindings work, including chain-attack modifiers (hold + right-click) |
| 2026-10-13 | Alt+Tab and F10 to `SummonerFixConfig.exe` while playing; rebind live | Works: the app comes forward and new bindings apply on return |

## Still to do

See the [Roadmap](../roadmap.md).
