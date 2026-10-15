---
title: Roadmap
nav_order: 6.5
---

# Roadmap

Ideas and planned work that are not started yet. Each item lists what we already know, so
work can start from evidence instead of from scratch.

## Custom key bindings for player actions

**Requested 2026-10-12.** Let players bind their own keys to in-game actions.

**Update 2026-10-13: the input path is mapped.** See
[Controls and keyboard map](sum-exe/controls.md).

- The keyboard does **not** use DirectInput. Keys come from window messages through
  `key_mark`; only the mouse uses DirectInput.
- The controls are a hard-coded `switch` in `hud_handle_keys` (`0x40d930`). There is no
  binding table to edit and no in-game control options.
- Every key event first goes through `key_remap_layout` (`0x509560`), a scan code → scan
  code function used for AZERTY/QWERTZ layouts.

**Done 2026-10-13:** `src/summonerfix/keys.c` redirects the single `CALL` at `0x5093a6` and reloads `summonerfix.ini` live. It was tested in game (E=I, A=Left). `SummonerFixConfig.exe` edits the bindings; see [SummonerFix](summonerfix/index.md#key-binding-app-summonerfixconfigexe).

Original plan:

1. In SummonerFix, send the key code through our own `KeyRemap(code)`. It runs the original
   layout swap, then a 256-entry table from a new `[KeyBindings]` section of
   `summonerfix.ini`. Entries are `PhysicalKey=GameKey`; for example `E=I` makes E open the
   inventory.
2. Test that remapped keys open the right screens and that held keys (camera arrows) follow
   the remap.
3. Optional later steps:
   - mouse-button remapping in `mouse_update`;
   - binding console commands to keys, as Timeslip's patch does.

Timeslip's patch already binds console commands to keys (`[console] k###=command`). That
stays a separate route for actions that only exist as console commands.

## Turn the frame-rate speed bug into a "fast travel" speed control

**Requested 2026-10-12.** The frame-limiter bug that made the game run at 2x speed (see
[Timing bugs](sum-exe/timing-bugs.md#3-frame-limiter-double-counts-its-sleep-2x-game-speed))
shows the game copes with game time running faster than real time. Offer it deliberately,
for example as a hotkey to speed up walking long distances.

**Implemented 2026-10-13 (lever 1); mechanism verified 2026-10-14** (game clock runs 3.0x with the key held): hold `[Game] SpeedKey` (default F11) to set `frametime_divisor` (`0x5a5748`) to `1 / SpeedFactor` (default 3, range 1-8). It is polled every 15 ms by SummonerFix and only applies while the game has focus. The key can be changed in `SummonerFixConfig.exe`. Still open: an on-screen indicator, and play-testing combat/AI/scripts at high factors.

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
- The cutscene `cutscene_sequence_update` uses `timeGetTime`, not `FrameTimer`, so cutscene timing
  stays unaffected by levers 1 and 2. Whether that is right during a speed-up is to be
  decided.

First steps:

1. Prototype lever 1: a hotkey that toggles `time_scale` between 1.0 and 0.5 (and 0.25),
   with a small on-screen indicator.
2. Play-test walking, combat, AI and scripted events at 2x and 4x.
3. Add `[Game] FastTravelKey` and `FastTravelSpeed` to `summonerfix.ini`.

## Widescreen

**Started 2026-10-13.** The goal is to use the whole width of a 16:9 or 16:10 screen in game,
not a 4:3 picture with bars.

What is known:

- SummonerFix can already run the game at a wide internal resolution: `GameWidth=1366`,
  `GameHeight=768` goes into resolution-table slot 2. Its scaling keeps the aspect ratio, so
  1366x768 fills a 2560x1440 screen exactly. The 3D aspect follows the resolution (`gr_screen.aspect`).
- **Test 2026-10-13 (main menu, frame capture):** it runs normally at 120 FPS. The menu is
  drawn at the left in its original 1024x768 layout, and the right 342 pixels stay black.
  Table-driven menus use absolute pixel positions ([UI tables](sum-exe/vsdk-ui.md)).
- In game, layout is partly relative to the screen:
  - `gr_screen_width`/`gr_screen_height` have 91 call sites, mostly HUD code (`0x403690`-`0x4122ab`);
  - `interface_set_view_clip` sizes the 3D view from the width;
  - camera edge-scrolling uses the width.
- Every 2D draw adds `gr_screen.offset_x`, the clip origin set by `gr::set_clip`.
  Centring a 1024-wide UI could therefore be done with the clip origin, but mouse
  hit-testing for that UI would need the same offset.

- **Done 2026-10-13:** menus (every game state except 4) are centred by shifting SummonerFix's output mapping (`[Display] CenterMenus`). Checked by frame capture.
- User test 2026-10-13: works in game, but leaves empty space in the UI (in-game screens sit in the left 1024 pixels) and at the right of the menus (now fixed).

- **User capture 2026-10-13 at 1366x768:** the 3D world fills the width; the HUD bar, full-screen panels (inventory, character, journal, options) and side panels sit in the left 1024 pixels. The play menu is centred in that 1024 area.
- **Implemented 2026-10-13** (since play-tested, see below): `widescreen.c` shifts the UI layer by redirecting UI call sites (see [SummonerFix](summonerfix/index.md)).

- **User test 2026-10-13 (first version):** HUD, character screen and journal centred. Side panels shifted while the 3D view did not, so the scene appeared to jump left. In the inventory, models and equipment icons stayed at the old position. Options was not moved.
- **Second version 2026-10-13:** full-screen panels shift the whole output; side panels stay flush left with only the HUD centred.
- **User test of the second version:** the inventory model and the character portrait sat about 171 px too far right. Cause: the 3D projection is centred on the wide 3D view. Side panels still moved the scene, because the 3D view narrows to the right of the panel (original behaviour).
- **Third version 2026-10-13:** full panels get a 1024-wide 3D view, and side panels keep the full-width world behind them. User test: the portrait and side panels are fine; the inventory models were still too small.
- **Fourth version 2026-10-13:** 4:3 aspect while a full panel is open. Agent-mode screenshots show the inventory identical to 1024x768.
- **Agent-mode check of every reachable screen at 1366x768 (2026-10-13):**
  - correct: HUD, play menu (centred over the world), Options, Save, Load, Inventory, Character, Quest Journal;
  - side panels: Map, Spells and Skills flush left with the world behind;
  - world: clicking into the world walks to the right spot; the world health-bar overlay is unshifted.
  - **Dialogue (13), checked 2026-10-14** (new game, talking to Mulik in Masad): at first the frame was shifted but the text and
    choices were drawn at the far left, because the conversation text widgets (`0x4d6600`-`0x4d7ab0`) were outside the
    redirected ranges. With that range added, text, choices and choice clicks are correct. The floating speech above NPCs
    stays on the NPC (a world overlay, correctly unshifted).
  - The new-game intro's "Presents" credit (a bitmap placed by the cutscene, not a string) sits left of centre at
    1366. Not checked against 1024 yet (the intro timing differs between runs).
  - **User test 2026-10-14:** shops (buy/sell, 7) and looting/pickup work at 1366. Not seen yet: item trade (19),
    assess (20).

Still open: item trade (19) uses the multiplayer trade background, so it probably needs a multiplayer game. Assess (20)
needs the Assess skill: giving Joseph the skill byte in an edited save was not enough, because the Skills panel only lists
skills the character may have (2026-10-14). The intro credit position against a 1024 run is also unchecked.

## Decompilation

See the [Decompilation plan](decomp-plan.md): map the whole binary first (engine core
first), structured so it can become a byte-matching decompilation later.

## Other planned work

- Widescreen (16:9) instead of 4:3 pillarboxing, with the 1024-wide UI kept centred.
- Port the remaining Timeslip features. Done 2026-10-14: intro-movie skip, camera zoom range and interaction
  ranges (`[Game]` options, see [SummonerFix](summonerfix/index.md#fixes)). The speed key was done 2026-10-13.
  Console-command keys were done the same day (`[ConsoleKeys]`, also editable in the config app). The save-image problem showed up as black thumbnails under SummonerFix's scaling and is fixed
  (`glReadPixels` hook). Still open: Timeslip's Bink 32-bit fix (no movie corruption seen with OpenGL).
- ~~Fix the cutscene uptime bug~~: done 2026-10-14 (see [SummonerFix](summonerfix/index.md#fixes)).
- **Brightness under OpenGL (needs a decision).** The in-game gamma slider (0.7-2.2) does nothing with the OpenGL renderer that SummonerFix uses: the vertex-colour table `gr_color_gamma_lut` (`0x25d88d0`) is an identity table, and only Direct3D (hardware gamma ramp) and Glide (`grGammaCorrectionValue`) apply `gr_gamma_ramp`. Two ways to fix it from SummonerFix's SwapBuffers hook: `SetDeviceGammaRamp` built from `gr_gamma_ramp` (simple, but changes the whole monitor while the game runs), or drawing the frame back through the curve (cleanest in a window). See [Bugs](sum-exe/game-bugs.md) K90.
- Direct3D renderer: find which DirectDraw/Direct3D call fails at `0x539b41`
  (low priority while OpenGL works).
