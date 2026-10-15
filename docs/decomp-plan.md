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

## Phase 3 status (2026-10-15)

The source half of the pilot has started without the compiler. Agents rewrote whole translation units in C++ from
the disassembly; I reviewed them, spot-checked a function of each against the listing, and checked that they compile
with mingw (syntax only). They are in [`src/decomp/`](../src/decomp/README.md):
- engine: `frametime`, `timestamp`, `registry`, `cmdline`, `gr_font`, `ui_container`, `targautils`, `window`,
  `character_instance`, `effect_mem`, `memory`, `stringpool`, `plainstring`, `parse`, `file_packfile`, the mouse, exception-handler, localisation, network and two os modules (placeholder names), `console`, `norlsallocpool`, `matrix`, `skeleton`, `character`, `bmpman`, `gr`, `gr_opengl`, `gr_direct3d`, `gr_glide`;
- game: the credits, help, main and play menus, dialogue, assess, pickup, game create, journal, character select,
  level select, shop, item trade, options, save/load, status, automap, inventory, HUD, skills, spells, tutorial, multiplayer game list and
  PXO chat screens, the interface manager, editor dialogs, boot/title states, frame end, input, the cutscene player, the multiplayer menus, lobby, net session and packets (under placeholder file names), the multiplayer save, trade, chat, chat-tracker, spell, container, end-game and data code, the CD check, targeting, plus `gamesound`, `item_paging`, `player`, `objects`, `living_entity`, `items`, `characterinfo`, `spells`, `combat`, `s3d`, `levelrender`, `ai`, `boss_ai`, `pathfinding`, `gameplay`, `summoner` (the main module, with WinMain), `conversation`, `level_scripts` (with the savegame code), `level_scripts_common`, all 23 tagged per-level scripts and
  `conversationctl`.

`node tools/decomp-progress.mjs` counts the result: 3,806 functions written (44.0%). Of these, 2,479 match the
disassembly exactly as far as can be read, 1,193 likely, 58 unsure. All 126 files compile against one header tree
(`src/decomp/include`, decisions D1-D67 in its `DECISIONS.md`). Pilots written in parallel each changed a copy of the
headers; they were merged with a three-way merge against the common base. Only list layouts and a few signatures
needed hand resolution.

`objects.cpp` was compiled without optimisation, so its source can be read almost line by line from the code. It also
expands small inline helpers, `vector3` operators and `profile_label`, which the optimised files call out of line.
The real build may therefore have used different options per file.

**Lessons so far.**
- The per-batch stub headers conflict, for example `File` three times and `interface_set_screen`'s second
  argument. They are being merged into one set of engine headers. `symbols/types.h` stays the C reference, because
  the game calls methods on most of these types.
- Writing source finds symbol errors fast. The pilot notes gave about 120 verified fixes: argument orders,
  arrays typed as scalars, and inverted names (`saveload_save_mode`, the inventory drag fields).
- Recurring compiler questions only VC6 can answer: the static-initialiser order against `.bss` order, whether
  divisions by constants become reciprocal multiplications, and how switches with shared tails were written.

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

**2026-10-14, confidence round:** three agents re-checked the 1,824 function names that were still **I**, each
against the disassembly. They raised 1,317 to **O** (7 to **V**, matched to Summoner 2 or Dash Faction) and proposed
72 renames. I verified a sample of each batch before applying it, and comments that mentioned the old names were
updated. Notable fixes:
- `buysell_recharge_item` is `buysell_buy_item` ("fully charged" is only the warning for an item already owned);
- `mp_trade_error` is `mp_send_it_rqst` (packet 0x2a);
- `spell_description` is `camera_set_focus_override`;
- `script_editor_any_window_open` was inverted: it is `script_editor_no_window_open`;
- `journal_format_event` is `conversation_execute_action`, the 13-case dialogue action switch;
- the H key handler is `hud_toggle_solo_mode`;
- `cmd_charinfo` is a bare `ret` shared by every empty function body after identical-code folding, so it is now
  `empty_function`;
- engine: `gr_glide_tcache_init`, `gr_d3d_create_texture`, `snd_get_handle`, `ds3d_init_listener`,
  `File__write_line`, `file_dir_exists`.

After this round, about 530 symbols are still **I**.

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

**Data globals, agent batches (2026-10-14 night):** four agents named 2,205 more globals: every unnamed
address used by two or more functions, then single-function globals with 3+ references. The pattern on the UI
screens is regular: a 0x40-byte static button table, 0x18-byte runtime button states, a scrollbar rectangle and
bitmap, the scroll position, the mouse mode, the pressed button and a RealtimeTimer. About 580 frequently used
addresses are string literals and were left as they are. Corrections that came out of this:
- `skill_table` has 84 entries, not 85 (`SKILL_WEAK` has none);
- `0x5c1888` is the journal's line count (`journal_desc_line_count`; the scroll position is `0x5c1ac8`);
- GameTimer is 4 bytes;
- `multitrade_load_bitmaps` is `itemtrade_load_bitmaps`;
- the difficulty selection, formerly "game type", is `gamecreate_set_difficulty` / `mp_game_difficulty`.

**C runtime:** the code is the static multithreaded **LIBCMT** of Visual C++ 6. It has TLS per-thread data
(`_getptd`), `_lock`/`_unlock`, `_beginthread`, and both the V6 (`__sbh_*`) and V5 (`__old_sbh_*`) small-block
heaps. Ghidra's function-ID matches added 38 functions (`_ftol`, `strncpy`, `rand`, ...) and 352 compiler
exception-unwind funclets (`eh_unwind_*`) at the end of `.text`. An agent named 317 more from the CRT sources (heap,
stdio, locale, mbcs, x87 libm and EH helpers). Corrections:
- `GetOEMCP` / `GetACP` at `0x578ea5` / `0x578eba` were tail jumps inside `getSystemCP`, not import thunks;
- `0x404880` is the plain `vector_constructor_iterator` (no SEH); the eh version is at `0x56db8c`.

**Hidden function starts:** a scan for 16-byte-aligned code after `ret`/`jmp` plus padding found 291 starts that
Ghidra had merged into the function before them, and a scan of direct call targets found 7 more. Examples:
`mp_forward_cur_packet` (24 callers, hidden inside `multi_send_packet`), 40 script-editor gadget callbacks, about 50
cutscene end callbacks, the worldmap encounter pickers, and `ix_ray_plane` (`0x53ce30`). The real
`ix_linesegment_box_oriented` is `0x53ced0`, which holds that profile label. 119 of the starts have no reference
(dead code). Game and engine code now has no unnamed direct call target.

**State (2026-10-14 night):** about 11,470 symbols.
- 8,633 functions: 8,329 **O**, 162 **V**, 142 **I**.
- 2,838 data: 2,768 **O**, 37 **V**, 33 **I**.

Two more confidence rounds produced these counts. The data round fixed 16 types: array sizes that ran into the next
symbol, bool vs int, and `game_camera`. The function round renamed 15, for example `gr_d3d_set_bump_env` (D3D7 bump-map
stage states), `ix_sphere_sphere` and `LineFile__flush`.

**2026-10-15, last I round:** one agent re-checked the 106 symbols still at **I**. Result: 68 confirmed, 28
renamed (checked in a sample before merging), and 10 still unclear. The unclear ones are mostly write-only OpenGL flags and
trivial setters with no caller. Renames that change the meaning:
- `matrix43__multiply_b` is `matrix43__equals`;
- `gr_clip_vertex_codes` is `gr_classify_sphere_view` (it computes no codes);
- `gr_poly_b` is `gr_poly_project_to_screen`;
- `nw_send_to` always sends to 255.255.255.255, so it is `nw_send_broadcast` (the LAN game search);
- `avoid_setting__init` fills the per-action defaults of the script system: `script_action_defaults__init`.

`vector3__random_direction` and `vector3__random_unit_vector` have identical bodies. Summoner 2's order would swap the two
names, but nothing in Sum.exe tells them apart, so they are left as they are. `timer_wait` (`0x530bf0`) moved from
`timer.cpp` to `vsdk\os`: it sits between two os functions, and `timer.cpp` starts at `0x532820`.
Now 10 symbols are **I**, 11,253 **O** and 187 **V**.

**Next for the decompilation:**
1. Raise the **I** names, mostly the newly found starts, the CRT libm helpers and data.
2. Split the binary into translation units. `tools/tu-split.mjs` proposes 119 units (see the
   [source-file map](sum-exe/source-map.md#translation-units-from-data-locality)). First fix the stray module tags that
   `--score` lists.
3. Phase 3 (a byte-matching pilot) needs a licensed VC6 compiler.

**The analysed Sum.exe is not byte-clean (2026-10-15).** The copy in `Summoner/` carries a no-CD edit in `cd_find_drive` / `cd_set_root` that no compiler emits: `jne +0` (`0f 85 00 00 00 00`) at `0x43e6b0` and `0x43e6e3`, `75 00` at `0x43e872`, a forced `.` drive letter at `0x43e695` and `0x43e914`, and the strings at `0x594f50` / `0x594f70` changed from `A:\` / `%c:\` to `.\\` and `.\` (bytes `2e 5c 5c 00`, `2e 5c 00 00`). This is separate from Timeslip's patch (`0x43e9cb`, [Timeslip](patches/timeslip.md)). `src/decomp/game/summoner_cd.cpp` is written as the unpatched code (D2600), so a byte-matching build will differ at these bytes unless it is compared against an unmodified v1.40 executable.

**Toolchain (2026-10-15).** Sum.exe's PE header gives linker 6.0 and the link date 2001-11-21. Its Rich header
(decoded with a small script, **observed**) lists:

| Tool (Rich product id) | Build | Objects |
|---|---|---|
| C++ compiler (`Utc12_CPP`, id 11) | 8966 | 348 |
| C compiler (`Utc12_C`, id 10) | 8047 | 161 |
| C++ compiler | 8047 / 8168 | 8 / 1 |
| linker (`Linker600`, id 4) | 8447 / 8168 | |
| MASM 6.13 (id 14) | 7299 | 38 |
| import entries | 0 | 235 |

So the game was built with **Visual C++ 6.0** (compiler version 12.00), most of it with a late service-pack build
(8966; which service pack is *inferred*, not confirmed), plus static libraries built with older VC6 builds (the C
runtime, 8047). A matching build needs Visual Studio 6.0 with the matching service pack. **Microsoft C 6.00** (1990)
is a different, 16-bit DOS compiler and cannot produce this code.
