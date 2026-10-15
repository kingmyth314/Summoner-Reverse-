---
title: Decompilation plan
nav_order: 6.4
---

# Decompilation plan
{: .no_toc }

1. TOC
{:toc}

**Decision (2026-10-12):** first **map the whole binary**: every function, global and structure
named, typed and documented. Structure that work so it can later grow into a **byte-matching
decompilation** (C++ that the original compiler turns back into an identical Sum.exe, as the
Summoner 2 project does). Start with the **engine core** (`vsdk`), where reference projects
already supply names.

## Starting facts

| Fact | Value | Source |
|---|---|---|
| Compiler | Microsoft Visual C++ 6.0: C++ objects from cl 12.00 build 8966 (348), 8168 (1), 8047 (8); C objects build 8047 (161); masm 6.14/6.15 (38); linker 6.00 | Rich header (**observed**) |
| Size | 6,612 functions, 5,728 strings, `.text` 1.49 MB | Ghidra via REA |
| Debug info | None (no PDB path) | **Observed** |
| C runtime | Statically linked (MSVC 6 CRT, roughly `0x56a000`+ ; *inferred* from `__ftol`, `_strstr`, `__alldiv` locations) | Ghidra |
| Original source tree | `D:\projects\Summoner\pccode\` with `vsdk\` (shared engine), `engine\`, `summoner\`, `levelscripts\` | Assert/error strings |

Source files named in Sum.exe's own strings (35 of them):

| Folder | Files |
|---|---|
| `vsdk\os` | `cmdline.cpp`, `memory.cpp`, `registry.cpp`, `stringpool.cpp` |
| `vsdk\gr` | `gr.cpp`, `gr_font.cpp`; `directx\gr_direct3d.cpp`, `glide\gr_glide.cpp`, `opengl\gr_opengl.cpp` |
| `vsdk\ca` | `character.cpp`, `character_instance.cpp`, `skeleton.cpp` |
| `vsdk\bmpman` | `bmpman.cpp`, `targautils.cpp` |
| `vsdk\ui` | `ui_container.cpp`, `window.cpp` |
| `vsdk\math`, `vsdk\parse`, `vsdk\geom`, `vsdk\vfile\packfile` | `matrix.cpp`, `parse.cpp`, `effect_mem.cpp`, `file_packfile.cpp` |
| `engine\objects` | `item_paging.cpp`, `items.cpp`, `living_entity.cpp`, `objects.cpp` |
| `engine\ai` | `boss_ai.cpp`, `pathfinding.cpp`, `pathfinding_visibility.cpp` |
| `engine\...` | `characterinfo\characterinfo.cpp`, `gamesound\gamesound.cpp`, `s3d\s3d.cpp`, `rendering\levelrender.cpp` |
| `summoner\player` | `player.cpp` |
| `levelscripts` | `conversation.cpp`, `script_internal.cpp`, `scripts\level_scripts_common.cpp` |

## Phase 0: infrastructure

Do this first, because everything else depends on it.

1. **Symbol file as the source of truth.** REA's Ghidra sessions are ephemeral: names set in
   one session are gone in the next. So names live in the repository, in
   `symbols/sum.symbols`, one line per address:

   ```
   0x501510 FrameDone            func  vsdk\os\frametime.cpp  ; S2 name, verified
   0x5a5740 reported_frametime   float vsdk\os\frametime.cpp
   0x25d8d30 gr_screen           gr::Screen vsdk\gr\gr.cpp   ; Dash Faction layout
   ```

   Types (structures such as `gr::Screen`) go in `symbols/types.h`.
2. **Apply symbols everywhere automatically.** One script pushes the file into a REA/Ghidra
   session (`set_addresses_names`) and into x32dbg as labels, so names appear in both the
   static decompiler and the live debugger.
3. **Progress report.** A script that counts named and unnamed functions per address range
   and per source folder (the equivalent of the Summoner 2 project's `ninja progress`).
4. **Translation-unit map.** VC6 links each `.cpp` file's functions contiguously. Using
   assert strings (each names its `.cpp`), map **address ranges to source files**. This
   shows where every module sits, and it is exactly what a later matching decompilation
   needs to split the binary into objects.
5. **Mark the C runtime** range as library code so it is excluded from progress counts.

## Phase 1: engine core (`vsdk`)

Order chosen by how much the reference projects already tell us
([Engine references](reference/engine-references.md)):

| Order | Module | Why this order | Reference |
|---|---|---|---|
| 1 | `os`: frametime, timer, registry, cmdline, memory, stringpool, console | Partly mapped already; Summoner 2 has the source | S2 `src/*.cpp`, DF `rf/os/` |
| 2 | `parse` (`.tbl` tables) and `vfile\packfile` (`.vpp`) | Unlocks reading game data files | S2 `parse.cpp`, `file_packfile.cpp`; DF `vpackfile.cpp` |
| 3 | `gr` (+ OpenGL, Direct3D, Glide back-ends), `bmpman`, `gr_font` | Needed for widescreen and UI work; `gr::Screen` already identified | DF `rf/gr/`, `rf/bmpman.h` |
| 4 | input (keyboard, mouse, DirectInput) | Needed for custom key bindings | DF `rf/input.h`, `input/key.cpp` |
| 5 | `math`, `geom`, `ca` (character animation), `ui` | Large and shared by gameplay code | DF headers, S2 symbols |

Per module, the routine is:

1. Find the module's code: assert strings, imports, and calls from functions already named.
2. Compare it with the reference source or headers. Name the functions, globals and
   structures, and record the evidence level (**Observed** when the code matches the
   reference, **Inferred** otherwise).
3. Check uncertain cases live in x32dbg: breakpoints, memory reads, logging breakpoints.
4. Write a module page under `docs/sum-exe/` and add the names to `symbols/`.
5. Note anything that matters for SummonerFix or the [Roadmap](roadmap.md).

## Phase 2: game layers

`engine\objects`, `engine\ai`, `summoner\player`, `engine\characterinfo`, and
`levelscripts` (Summoner's own code, with no reference project), working outward from the
engine calls named in Phase 1 and from the [game-state table](sum-exe/main-loop.md#game-state-table-0x588a70).

## Phase 3: pilot for a byte-matching decompilation

When a small translation unit is fully mapped (for example `frametime.cpp`):

- Obtain the **VC6 compiler** (cl 12.00.8966 and the matching linker). Visual C++ 6.0 is
  proprietary, so this needs a legitimately licensed copy; we can't simply download it.
- Rewrite one translation unit in C++ and compare the compiled object with the original
  bytes function by function (objdiff, as the Summoner 2 project does).
- If it matches, set up the project layout and progress tracking for the rest.

## Status

| Phase 0 step | Status (2026-10-13) |
|---|---|
| 1. Symbol file | Done: `symbols/sum.symbols` and `symbols/types.h`; `node tools/symbols.mjs check` |
| 2. Apply symbols | x32dbg: `apply-x32dbg` (names show in the live disassembly). Ghidra: REA's Windows provider is **read-only** ("does not admit database mutation"), so `tools/decomp.mjs` applies names to Ghidra's output instead, including struct fields (`gr_screen.max_w`) |
| 3. Progress report | Done: `symbols/ghidra-functions.tsv` (6,329 functions) and `node tools/symbols.mjs progress` |
| 4. Translation-unit map | Done: [Source-file map](sum-exe/source-map.md) |
| 5. Mark C runtime | Done: it starts at `0x56c740` (*inferred*: first non-16-byte-aligned code; Ghidra's library names begin at `__ftol` `0x56c7e4`) |

**Baseline (2026-10-13):** 6,329 functions. 5,531 of them are game/engine code below
`0x56c740`; the rest are C runtime and library code (Ghidra recognised 441 by signature).
**43 / 5,531 named (0.8%).**

The 2026-10-12 REA blocker ("Open Windows path component failed [C:]") was cleared by a
Windows restart.

## Where we start

Phase 0 is complete. Progress (2026-10-14): **every function in Ghidra's list is named** (5,547 / 5,548; the last entry is not a real
function start), plus 2,007 functions that Ghidra's analysis missed (console-command initialisers and callbacks, level-script handlers).

| Module | Status | Page |
|---|---|---|
| `os`: timestamp, timer, memory/mempool, stringpool, debug, registry, cmdline, console, key state, networking, import thunks, crash reporter | Mostly done; a few helpers left | [OS layer](sum-exe/vsdk-os.md), [Console](sum-exe/console.md) |
| `parse` (TextFile) | Done (33 methods) | [Parser](sum-exe/vsdk-parse.md) |
| `vfile\packfile` | Done | [Packfiles](sum-exe/vsdk-packfile.md) |
| `bmpman` + image readers | Done (TGA/PCX/M2V, JPEG screenshots) | [Bitmap manager](sum-exe/vsdk-bmpman.md) |
| Localization (strings.tbl, name tables) | Done | [Localization](sum-exe/localize.md) |
| `gr`, `gr_font`, keyboard, mouse; back-end dispatch targets | Done (front end); back ends partly | [Graphics and keyboard](sum-exe/vsdk-gr.md) |
| `math\matrix` and vector helpers | Done (V for the Summoner 2 functions) | [Math](sum-exe/vsdk-math.md) |
| `ca` loading (characters, skeletons, meshes), `FileReader`, start-up fonts | Started | [Characters](sum-exe/vsdk-ca.md) |
| `ui` table parsers, build stamp | Started | [User interface](sum-exe/vsdk-ui.md) |
| Game: states, interface screens (23), controls/keyboard map | Started | [Interface](sum-exe/interface.md) |
| Next: `ca` animation, `geom`, `ui` widgets | Not started | |

**2026-10-14:** `tools/asm.mjs` (llvm-objdump, 2 seconds for the whole binary) replaced per-instruction
REA disassembly for bulk work. Its `strings` report, which shows the debug and in-game text each function
references, named about 130 functions. Data-table scans named another 280:
- console-command initialisers;
- the [level-script table](sum-exe/level-scripts.md);
- the chat commands.

Phase 2 has started. It now covers the [character object and player record](sum-exe/characters.md), the
[game data tables](sum-exe/game-tables.md), the [melee damage formula](sum-exe/combat.md) and the
[level scripts](sum-exe/level-scripts.md).

**2026-10-14, parallel pass:** three sub-agents proposed names for separate address ranges. Each worked read-only
with `tools/asm.mjs`. The proposals were spot-checked against the disassembly and merged with `tools/addsym.mjs`:
- 112 names: game layers (`0x440000`-`0x4c0000`): items, party inventory, living entities, AI, pathfinding,
  bosses, doors;
- 366 names: UI screens and scripts (`0x401000`-`0x440000`, `0x4c0000`-`0x520000`): the shared screen template
  helpers, popups, savegame chunks (tags `SAVg`, `ITEM`, `INVT`, `LENT`, `CMRA`, `CONT`), story flags
  (`savegame_get_flag`/`savegame_set_flag`), conversation, `events.tbl`, the script editor;
- 248 names: vsdk (`0x520000`-`0x56c740`): the sound layer and DirectSound/EAX back end, the vfile helpers, the
  `String` class (Red Faction layout, **V**), FTP/HTTP downloaders, renderer fog/alpha back ends, ui_gadget
  tree helpers.

The pass found these layouts: sound records (0x40 bytes, 640 at `0x2cb5458`); DirectSound buffers (0x250 bytes,
256 at `0x2f54720`) and channels (0x2c bytes, 32 at `0x2f798b8`), both sized as in Red Faction; character slots
(200 x 0x1f8 bytes at `0x2dc7088`). It also found the ui_gadget tree links: x/y `+0xc`/`+0x10`, window `+0x25c`,
parent `+0x268`, first child `+0x26c`, next sibling `+0x270`. Three earlier names were corrected:
`item_info_popup_load_bitmaps`, `multitrade_load_bitmaps` and `pxochat_reset_lists`.

**2026-10-14, second parallel pass:** another 1,100 symbols were merged after the same checks:
- the savegame format ([Savegame format](sum-exe/savegame.md), checked against eight real saves);
- 336 game names: AI states and goals, boss dispatch and stages, the level-script NPC API, multiplayer sync;
- 367 engine names: the 3D pipeline, Glide function pointers, math, mouse, vfile, sound emitters, ca.

New layouts in `types.h`:
- the 3D vertex (`gr_Vertex`), mouse buttons and sound emitters;
- the AI state names (`AI_STATE_*`, from the game's own string table) and the species table;
- many more `Character` fields.

Two earlier inferences were corrected: `Character +0x5ac` holds the locomotion animations, not textures, and
`0x471f40` is a packet send, not `ai_mode_name`. File names were only kept where an assert string or a reference
project confirms them; `gr_3d.cpp` was replaced by the folder `vsdkgr`.

**2026-10-14, third parallel pass:** 1,053 more names were merged:
- UI screens, HUD, chat, conversation, boss AI helpers;
- the game layers: movement, AI goals, combat reactions, spells, level objects, multiplayer;
- the vsdk UI widget system.

The agents also flagged eight existing names as wrong. Each was checked in the disassembly and renamed:
`world_mouse_frame`, `character_use_skill`, `character_spell_on_cooldown`, `cutscene_cleanup`, the
`ui_gadget` disabled/enabled methods, and the static-object destructors. The `ai_state` path is at `+0x210`, not
`+0x80`.

**2026-10-14, fourth parallel pass:** about 1,000 more names, including the `0x480000`-`0x4c0000` pass (multiplayer
packets: the [packet table](sum-exe/multiplayer.md) named 48 handlers hidden inside other functions; IRC and trackers;
effects; level rendering; boss AI; AI groups; pathfinding). It covered:
- boss AI and the script editor;
- 72 static UI constructors and the options setters;
- the statically linked **IJG libjpeg 6b** compressor (113 functions, named from its method tables and error codes,
  spot-checked against `jpeg_write_tables`);
- math (quaternions, compressed quaternions), frustum culling, and more renderer helpers.

Corrections: `mp_player_file_write` (it writes the `.plr` file), `hud_set_timed_highlight`, and the two hidden static
initialisers that had been merged into "level_display_name". About 400 function starts hidden after padding are
still not in Ghidra's list.

**2026-10-14, fifth parallel pass:** about 1,800 entries, of which about 1,240 are function starts that Ghidra had
missed. Most are the static-initialiser machinery: for each global object, a jump thunk in the 652-entry table at
`0x588004`, an init function, an `atexit` registration and a destructor. The rest are hidden callbacks: game-state
handlers, message-box and UI callbacks, script-editor buttons, multiplayer join and new-player handlers, sort
comparators and the lights API. Corrections, each checked: `chat_history_pump`, `mp_pack_player_info`,
`register_atexit_4d5110`, `dialogue_topic_box__ctor`, `register_atexit_439e60`, `init_chain_skill_names` and
`light_set_all_enabled`. The initialiser table has 652 entries, not 712.

**2026-10-14, sixth parallel pass:** each agent got an exact list of about 300 of the remaining unnamed functions.
- 0x40acc0-0x4bd9c0: 297 of 298 named (the last one is not a real function start).
- 0x544b20-0x566b10: all 297 named, plus 20 newly found function starts. This covered the sorted transparent-polygon
  buffer, the texture caches of all three renderers, animation key tracks, the VFX system, and Ken Perlin's reference
  noise.

Corrections: `vfx_mesh_render`, `init_ds_channels`, `ghost_revive_minions`; `xp_level +0x05` is a skill cap.
`tools/addsym.mjs` now reads only the kind field when keeping entries in address order, because a comment
containing "data:" had misplaced one entry.
- 0x4bdce0-0x544ad0: all 298 named, plus 5 hidden callbacks whose Ghidra starts were case blocks inside them. Areas:
  generic AI combat sub-modes and spell choosers, the remaining bosses, decals, lamp glows, dialogue boxes, the script
  editor, vfx/vif, D3D render states (`gr_d3d_set_state`, V, Dash Faction), the clipboard.

**State after the six passes:** about 8,240 symbols. 6,089 are **O** (seen in the code), 179 **V** (confirmed by a
reference project) and 1,968 **I** (inferred). The **I** entries are the main thing to improve: re-check them where the
behaviour matters, and raise them to **O** or rename them. Spot checks of every agent batch found about one wrong name
per 300-500, each corrected before merging.

**Data globals (2026-10-14):** the code references about 13,270 distinct addresses in `.data` and `.bss`. About 610
had names before this step, mostly console variables and command objects. Of the unnamed ones, about 3,600 are used
by two or more functions. The first data pass named these:
- the console parser state, with Summoner 2 names: `calling_a_command_function`, `doing_help_for_comand`,
  `checking_status_for_command`, `arg_type`, `next_arg`, `original_arg`, `command_argument_int` / `_float`,
  `rest_of_command_line`, `debug_command_prefixed`, and the shared `string_buffer`;
- the gr modes passed to `gr_string`, `gr_rect`, `gr_line` and `gr_bitmap`;
- `mp_local_player` and the multiplayer player lists;
- the living-entity list sentinel `0x8eca60`;
- the game's per-frame mouse position.

Twelve "set" console variables were typed `int` by the generator. They are `float`: the callback copies
`command_argument_float`, as with `chance_gold`, the LOD distances and the fog/far-plane values. Every console variable
whose callback pattern confirms the type is now **O**.

**Next for the decompilation:**
1. Raise the confidence of the **I** names in the game layers.
2. Name and type the remaining data globals (agent batches, most-used first).
3. Split the binary into translation units with the [source-file map](sum-exe/source-map.md) and the initialiser
   table order. Each translation unit's static initialisers are adjacent in the table.
4. Phase 3 (a byte-matching pilot) needs a licensed VC6 compiler.
