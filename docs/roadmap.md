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

**Status 2026-10-17.** Mapping is done (every function named) and all game and engine code is rewritten in C++
(7,725 functions; the C runtime and libjpeg 6b are not rewritten). The compiler is identified (VC6 SP5, build 8966)
and 4,963 of the 5,925 compiled functions give the original bytes, with their calls and data references checked
against the original targets (2026-10-17; the check found and fixed 25 differences, among them a wrong party sort
order and wrong text in 45 strings). What is left of the per-function matching is mostly register choice, frame slots and instruction scheduling.

Proposed next steps (2026-10-17 review, not decided yet): let a script set the Confidence markers from the sweep, then try a first **link** of a
rebuilt executable, the step the post-recompile projects below need. For a link, the C runtime can come from VC6's
own `LIBCMT.LIB` (Sum.exe links the multithreaded CRT); VC6 ships no libjpeg, so libjpeg 6b would have to be built
from the Independent JPEG Group's source with the same compiler.

**Status 2026-10-19: the hybrid game.** Instead of a full link, the whole decompiled source (5,922 functions) runs
inside the real game as a DLL that replaces the original functions, so each one can be tested in play: save
comparisons, and dual calls that run the original and the decompiled function on the same live inputs and compare
the results. This has found thirteen decompilation errors byte matching could not see, all fixed (DECISIONS
D9200-D9204, D9800-D9810). 628 of the 890 functions that do not match byte for byte have run in a checked
single-player scenario; what has not run is mostly multiplayer (left out for now), the Direct3D and Glide renderers,
functions with no caller, and a few rare branches. Details: [the hybrid game](decomp-plan.md) and
[never-run function review](hybrid-review.md).

**Status 2026-10-19: the first standalone link.** The decompiled source also links into an executable of its own
(`tools/link-build.mjs`, VC6 SP5 and its libraries, nothing from the original Sum.exe), with libjpeg 6b built from the IJG source.
SummonerFix's agent mode drives it through an address table. Comparing its data with the original's found three more
errors that the hybrid could not see, because there the byte-matched functions used Sum.exe's own data (D9811,
D9813, D9814). A new game reaches Masad and plays. A 75-minute play-test through the start of Act 1 found one
more error, the stalled prologue cinematic (D9815, fixed), and a review of the functions it ran for the first time two more (D9816), the rest of those functions one more (D9817); with the same SummonerFix timing fix, it runs at the original's frame rate. Two more play-tests (party members, the first summon, shops, compared pixel for pixel) and the level save check over 28 levels (`tools/link-savecheck.mjs`: saves and run-time memory against three original runs) found no further error. Next: an automated screen comparison, since the save check cannot see rendering. See [the standalone link](decomp-plan.md).

## Future projects

Added 2026-10-16 at the user's request.

### Soon

- **A save editor for players.** `tools/savedit.mjs` already edits gold and skill points from the command line and
  writes new files only; `tools/savinfo.mjs` reads every section ([Savegame](sum-exe/savegame.md) has the full
  layout). A player-facing editor would put this behind a simple window (like `SummonerFixConfig.exe`): party gold,
  levels, skill points, inventory, story flags, with a backup of the original save. *Engine done 2026-10-16:* `savedit` / `savinfo` now handle items by name, story flags by name,
  repeatable options and `savinfo --json` (shared reader `tools/lib/savedata.mjs`); the window itself is next.
- **More debug tools a player can reach.** What exists now is for testing: 149 console commands
  ([Console](sum-exe/console.md), cheats such as `immortal`, `Give_all_items`, `max_skills`, `add_xp`,
  `gamestage`, the boss `*_stage` commands), SummonerFix `[ConsoleKeys]`, `[Debug] ScriptEditorKey`,
  `TestSaves`, and agent-mode `flag` / `navgoto`. A player-facing version could add a console window with the
  game's own output, a cheats page in the config app, and a level-warp menu (`play_and_load_cutscene` with the
  return-to-menu bit cleared, see [Tools](tools.md)).

### Game mechanics the developers left switched off

- **The Tentacle Beast's grab and throw (G250).** Added 2026-10-18 at the user's request. The tentacles have a
  complete grab, hold and throw in `Sum.exe` and in `Tentacle Beast.tbl` ("Luminar grab" / "Luminar throw"), but
  the selection can never pick it (`frand() < 0` at `0x4c666d`..`0x4c667f`) and the pick would be discarded anyway
  (`0x4c66a5`). The user suspects the developers switched it off on purpose. The dead code has its own slips: the
  6-second hold tick heals the victim by 3 instead of hurting it (`push 0x40400000` at `0x4c714b`), and the release
  uses the victim without a NULL test (`0x4c6b53`). The project: work out how the mechanic was meant to play (grab
  chance, hold, throw, release on death, level end, cutscene and load), get every part working, and offer it as an
  experimental SummonerFix switch, off by default. **Done 2026-10-18:** `[Experimental] TentacleGrab=1` in
  `summonerfix.ini` turns it on (tested in game). The analysis, enable plan and test results:
  [Tentacle Beast grab and throw](future/tentacle-grab.md) (two edits switched it off; the gate meant to allow one
  grab at a time is never read; a hero held when the battle ends would stay stuck; the hold heals; the release
  mixes up the tentacle's and the victim's HP).

### After the code fully recompiles

Once `src/decomp` builds a working `Sum.exe`, these become source changes instead of byte patches:

- **dgVoodoo 2.** It wraps Glide and old DirectX for modern Direct3D 11/12. SummonerFix now forces the OpenGL
  renderer; dgVoodoo 2 would make the original Glide and Direct3D renderers usable too (with their own gamma,
  K90 does not apply there). To find out: which dgVoodoo DLLs Summoner loads, and whether the launcher-free start
  and the widescreen output can work with it.
- **Auto-saving.** The game already has `quicksave` (`quicksave.sav`) and the rules for when saving is allowed
  (`savegame_location_allowed`, [Savegame](sum-exe/savegame.md#saving-and-loading)). An autosave could write a
  rotating slot on level change and every few minutes when saving is allowed.
- **Wider display options.** Today: any `GameWidth` / `GameHeight` (resolution table slot 2), borderless or
  window output, centred menus and HUD, brightness. Candidates: a proper resolution list, exclusive fullscreen,
  vsync and frame-rate options, anti-aliasing, field of view, UI scale.

## Other planned work

- Widescreen (16:9) instead of 4:3 pillarboxing, with the 1024-wide UI kept centred.
- Port the remaining Timeslip features. Done 2026-10-14: intro-movie skip, camera zoom range and interaction
  ranges (`[Game]` options, see [SummonerFix](summonerfix/index.md#fixes)). The speed key was done 2026-10-13.
  Console-command keys were done the same day (`[ConsoleKeys]`, also editable in the config app). The save-image problem showed up as black thumbnails under SummonerFix's scaling and is fixed
  (`glReadPixels` hook). Still open: Timeslip's Bink 32-bit fix (no movie corruption seen with OpenGL).
- ~~Fix the cutscene uptime bug~~: done 2026-10-14 (see [SummonerFix](summonerfix/index.md#fixes)).
- **World sounds too quiet with the default sound setting: done 2026-10-16** (SummonerFix `bugfix.c`, K260; the slider is now linear).
- **Story state lost on load: no fix needed (2026-10-16).** G130 and G131 cannot happen in retail play: the game refuses saving in TempleINT2 once `mach_intro` is set and in the Jade Temple once `got_laharah_spirit` is set (`savegame_location_allowed`, tested in agent mode). See [Bugs](sum-exe/game-bugs.md).
- **Flickering right bar after cutscenes: fixed 2026-10-16** (a SummonerFix bug, K280: the strip right of the centred menus was never cleared).
- **Sounds cut off after a fade (candidate for SummonerFix, K294): not reproduced 2026-10-18** (only the music stream faded in 80 s of play and fighting; see the bug page). A sound that ends during its fade leaves the fade flag on its channel; the next sound on that channel is cut within 75 ms and the old DirectSound buffer leaks. `character_play_move`'s 100 ms fade makes it happen in normal play. A fix: wrap the `ds_channel_is_playing` call in `ds_channel_update_fade` (`0x55d6f8`) so a stopped channel also clears flag 8 and is released. To reproduce before fixing (the user's rule for fixes).
- **Bug-fix pass 2026-10-17/18:** 42 fixes in SummonerFix `[Fixes]`, each with its own switch; the list is in
  [SummonerFix](summonerfix/index.md) and [Bug fix priorities](sum-exe/bug-priorities.md). Decisions: G15 dice and K30
  boss bars kept, leech drains (G100), party healers always heal and enemy healers a little more than before (G229),
  K11 camera angle and K63 click reach kept, G250 moved to Future projects.
- **Robe of Restoration heals the attacker: fixed 2026-10-16** (G300, reproduced in game first; SummonerFix `bugfix.c`).
- **Print Screen above 1024 rows (C290): avoided 2026-10-16.** With `GameHeight` above 1024 SummonerFix switches screenshots to Targa. Not tested in game.
- **Brightness under OpenGL: done 2026-10-16** (SummonerFix `gamma.c`, K90): a shader pass, with the monitor gamma ramp as the fallback; the user chose the most compatible option with a small cost.
- **Bug fix order: ranked 2026-10-16** in [Bug fix priorities](sum-exe/bug-priorities.md) (impact on a single-player game times ease of reproduction; visual-only, multiplayer-only and latent bugs lower). Next from the top: G4, G215, C250, G8, G12.
- **Golem hit effects (K20) and the death popup's row offset (K170): fixed 2026-10-16**, both reproduced in game first (SummonerFix `bugfix.c`). K170 turned out to be multiplayer only. K272 (one shadow size for everyone) is deferred: a fix needs a design for the table's sizes.
- Direct3D renderer: find which DirectDraw/Direct3D call fails at `0x539b41`
  (low priority while OpenGL works).
