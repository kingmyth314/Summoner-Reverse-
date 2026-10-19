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

## The hybrid game (link experiment, 2026-10-18)

**The whole decompiled source runs inside the game.** All 259 files are compiled with VC6 SP5 and linked into one DLL
of 5,922 functions. SummonerFix loads it (`[Hybrid] Dll=hybrid.dll`) and writes a `jmp` at each original function,
so the game runs the decompiled code instead. A test save loads and looks the same as in the original game (0.17%
of the pixels differ, the same as between two runs of the original). Walking, the inventory and the automap work
(**observed**, agent mode). 5,736 functions are replaced; 186 stay original because SummonerFix patches them.
Tools: [`tools/hybrid-build.mjs`](tools.md#toolshybrid-buildmjs-the-hybrid-game) and `src/summonerfix/hybrid.c`.

Not covered: the about 1,130 compiler-generated initialisers and destructors of global objects (written as
comments, not compared). The DLL has no entry point, so its own static initialisers never run; the game's run as
before, and the decompiled globals are the game's own.

This is the first link experiment, done as a hybrid rather than a whole new executable. The rest of the program
(the C runtime, the import table, the data, the functions not yet written) stays the original. Each decompiled
function can therefore be tested in the real game on its own, including the about 970 that compile but do not
match byte for byte. The first one, `new_credits.cpp`, ran the credits screen correctly.

What the experiment found:

| Finding | Kind |
|---|---|
| `cd_find_drive` and `cd_set_root` ask for the disc: the decomp writes them unpatched, and the shipped Sum.exe has a no-CD edit in exactly those bytes (D2600) | not a decomp error: the build leaves them original |
| SummonerFix's display hooks recognise their callers by return address (GrInit's windowed branch, the mouse poll, the window procedure): with those functions replaced, the game window was not found and nothing was captured | handled: those ranges stay original (`KeepOriginal`) |
| Every SummonerFix patch inside a replaced function would be lost (for example C310's short install path in `file_init`) | handled: the loader leaves any patched function original |
| `setup_data_paths` (a byte match) used its own copy of `"savegame"`, which TestSaves patches in the game's data: the hybrid game read the player's real save folder (read only; nothing was written) | handled: in byte-matching functions every data reference uses the original's address |
| LINK 6.00 mishandles absolute symbols (rel32 gets the image base added, static ones read as 0) | worked around in the build |

How a failure is found: build the whole tree once, then run the game with `[Hybrid] Only=<list>` and halve the list
(a delta-debugging search: when neither half fails alone, each half is searched with the other present). A test
loads the newest test save and compares the screen with the original game's capture; a message box, an access
violation, no frames or more than 3% of the pixels different count as failures. Each startup failure above was
found that way in 10-20 game starts.

**The first decompilation error found by playing (2026-10-18).** A level change with the full party, the Murod
cutscene and fight run on the hybrid without errors, but a fight never plays out the same twice. Saving is exact:
load the test save, quicksave at once, and compare the file with the original game's. Six runs of the original
vary in 140 bytes (NPC timing); outside those, the hybrid's save differed in 13 bytes of one NPC's record, the same
in every run. The same halving search with a quicksave as the test named `levelrender_build_draw_lists`
(`0x482e20`): its distance test for AI flag 0x40 was inverted (DECISIONS.md D9200). The function differs from the
original in its frame set-up as well, so the byte sweep could not see the error. After the fix the hybrid's
quicksave matches the original's.

The same test after a level change with the full party (`level`, wait, quicksave) needs a value-aware comparison:
timers count from the level start and some fields depend on timing, so a dword that varies between three runs of
the original may vary by as much in the hybrid; one that never varies must be equal. Three more levels found
two more errors, each the same in every run:

| Level | Hybrid's save | Function | Error |
|---|---|---|---|
| Lenele (the test save itself) | one NPC's position, heading and a timer | `levelrender_build_draw_lists` `0x482e20` | AI flag 0x40 for near instead of far culled NPCs (D9200) |
| Iona exterior | bit 0 of `+0x320` clear on all 29 monsters | `Character::init` `0x44e1c0` | `CF_COMBAT_READY` when the stand and ready animations differ instead of when they are the same (D9201) |
| Jade Temple, sewer boss | one more topic for some speakers in the conversation state | `conversation::parse_table` `0x4fee50` | the "has a farewell topic" test read bit 0 instead of the `+Farewell` bit 2, so speakers got a second "Farewell." (D9202) |

None of the three functions matches byte for byte, so the byte sweep could not see them. With the fixes, the four
levels' quicksaves match the original's apart from run-to-run variation. Rand-forest01 could not be compared:
the original refuses a quicksave there right after arriving.

Thirteen more levels, three original runs each (2026-10-18): Catacombs, Masad, the sewer, TempleInt, Liangshan,
Eleh, WolongCaverns, IkaemosBottomInt and Wolong match apart from run-to-run variation (in Wolong the party's
timers are about 200 ms later: the hybrid saved later on that large level, a timing effect). lenele3a and
KhosaniLab could not be compared (the original refuses the quicksave). Two candidates turned out to be
run-to-run variation:
- `lenele2aa`: six NPCs' base max AP (type-5 record `+0x78`) was 8 instead of 7 in about half the hybrid runs and
  in none of the first ten original runs, but then in three of the next four original runs too;
- `tancredhouse`: one flag bit in one NPC's record, seen once and not in two more runs.

After the never-run fixes (2026-10-18, rebuilt DLL): in Jade Temple the hybrid's monsters had level 17 in two
runs and 18 in a third, against 18 in the first three original runs. Six more original runs gave 21, 22, 22, 22,
18 and 18, so the monster level there varies by several levels from run to run (**Observed**; what it depends on is
**Unknown**). The full-set step of the halving search matched, and with nine original runs the comparison passes.

Lesson for the method: an intermittent field needs more than a handful of original runs before a difference
counts, and a search step should count as failing only on a majority of repeated runs.

**Coverage (2026-10-18).** `[Hybrid] Coverage=1` counts the calls of every replaced function
(`tools/hybrid-coverage.mjs`). Loading the test save runs 1,536 of the 5,736 replaced functions (288 of the 895
that do not match byte for byte); after the Iona level change 1,765 (343 non-matching). The save tests above
therefore exercised about 40% of the non-matching functions; the others are listed by
`hybrid-coverage.mjs <file> --list never` and need other scenarios (combat, shops, conversations, the world map,
multiplayer last). The quicksaves themselves are written by the DLL's `savegame_write*` chain, and no level
showed a whole-section difference, so those writers are checked as well.

Two UI scenarios, saved and compared the same way: the inventory (unequip the weapon, destroy and use from the
list through the screen's own functions) and a conversation with the dock trader in Lenele that buys a Recovery
Draught and sells a Fire Arrow Rune (`player_talk_to`, then clicks; both games end with 58 gold). Both match the
original apart from NPC positions. With the thirteen levels rerun with coverage, the scenarios have now executed
**403 of the 895 non-matching functions** (and 1,809 of the matching ones) with saves that match the original. The
rest are mostly combat (no quicksave in a fight), the script editor, the Direct3D and Glide renderers,
multiplayer and screens not yet opened.

**Combat by dual calls (2026-10-18).** A fight cannot be compared run against run, so the combat functions are
checked call by call: `[Hybrid] Dual` runs the original and the decompiled function on the same copy of the
game's data, with the caller's real arguments, and compares the results (docs: [Tools](tools.md)). In the Murod
fight, ten calls each: `living_combat_pick_attack_move`, `living_combat_roll_defense`, `combat_resolve_exchange`,
`combat_position_modifier`, `character_get_protection`, `combat_melee_damage`, `combat_apply_melee_hit` and
`chain_attack_check_window` (none of them a byte match) give the same memory, return value and `rand` state as
the original in every checked call; so do the byte-matching controls (`combat_attack_rating`,
`combat_weapon_skill`, `character_get_bone_world_matrix`). `blood_splot_place` differed in about 4 of 10 calls by
one bit of a decal axis: the original multiplies by the unrounded 80-bit radius (D9203); fixed, 8 of 8 the same.
Making the checker trustworthy took three corrections, each shown by a byte-matching control that "differed": sound
switched off during the calls (DirectSound state is outside the copy, and the restore crashed it), the sound
thread's streaming buffers left out of the comparison and of the restore, and still copies compared instead of
the live data.

Then every non-matching function that ran in the fight, from the gameplay files (combat, avoidance, pathfinding,
AI, bosses, targeting, spells, the camera, vectors), 126 functions with loaders and constructors left out: five
calls each, through the Jade Temple level change and the Murod fight. More corrections first: a virtual clock for
the two calls (timers set from the clock differed), return values compared as the mangled name says they are
returned (void functions leave junk in eax), the C runtime's heap bookkeeping left alone (restoring it corrupted
the heap), the frame clock's state put back after a check. Result: 114 functions give the same memory and return
value as the original in every checked call. One real difference: `party_spread_attackers` kept its 2-second
timer in the DLL because the function-static had no address comment (D9204). The rest are x87 precision, a value
one unit off in its last bits where the original keeps an 80-bit intermediate and the decompiled code rounds it
to a float or the other way round: `blood_splot_place` (the decal position), `camera::start_sequence` and
`update_sequence`, `closest_approach_time`, `ai_choose_attack_slot` (about 1 in 3 million), an avoidance area
and two approach functions. Those need the original's instruction order to disappear (byte matching), and do not
change what the game does in any way a player could see.

**Bosses and spells (2026-10-18).** Three more fights with the party immortal (`poke 8ed970 1`): Luminar
(`level IonaEXT02 IonaEXT02_v2`), the Serpent (`level rand-forest01 Rand-Forest01_v2`) and the Tentacle Beast
(`level Sewerboss sewerboss`, `flag sewer_found_tb 0`, `navgoto $tbintro`). With coverage they brought the
non-matching functions that have run to 466 of 895. Dual calls over the earlier 126 plus 20 functions these
fights reach (the boss hooks and AI: `luminar_frame_hook`, `serpent_anim_remap_in`,
`tentacle_beast_ai_select_action`, `tentacle_beast_ai_frame`, `tentacle_beast_frame_hook`,
`boss_move_in_range_dispatch`; movement, head look, the avoidance splines, `character_equip_item`, the cutscene
camera sampler, the snow effect, CRC, animation tangents, polygon clipping): 93, 84 and 116 functions the same in
every checked call, every one of the 19 new ones that was called included. Besides the precision cases above, two
functions differ in pointers inside freshly built paths (`approach_random_offset` in the Tentacle fight,
`pf_graph_search` once in the Luminar fight): both call the path builder with the same arguments as the original,
and its allocations come from the C runtime heap, which the checker leaves running, so the second call's blocks
sit 8 or 16 bytes further on. That is the checker's limit, not a difference in the code.

**The precision round (2026-10-18, five agents, one file each).** For each of the eight functions with a last-bit
difference, the agent first pinned the instruction where the original keeps or rounds a value differently, then
searched for source with a strict byte match (no `volatile` or casts to force rounding; about 40 variants each):

| Function | Site | Result |
|---|---|---|
| `camera::start_sequence`, `camera::update_sequence` | not precision: the constant `0.3183099f` rounds one bit above the original's 1/pi `0x3ea2f983` | byte match (D9400) |
| `closest_approach_time` | the squares are stored as floats and summed twice; written on the products in the test and on the named squares in the division, VC6 no longer merges the two sums | byte match (D9500) |
| `avoid_area_add` | the vector2 operators round the y product before the add and keep x; needs `vector2::operator+` as a constructor return (shared header) | byte match (D9700, D9701) |
| `path_closest_point` | `q = path->pos - path->dir * 50` through `operator=` rounded x instead of z; declared with its initialiser it follows the original | same values, frame still differs (D9510) |
| `approach_attack_slot` | the slot points and the mid waypoint written per component kept y and z unrounded; vector operators give the original's sequence | same values, registers differ (D9610, D9611) |
| `blood_splot_place` | after D9203, the small case: `v` takes the rounded radius after the copy constructor; a cross product for `v` gives the original's frame and the small and medium cases | same values in 20 checked calls; the large case loses an inline (D9300, D9301). Later (2026-10-19): `pos += off` written per component frees the inline budget, the large case is the original's; 30/30 the same (D9809) |
| `ai_choose_attack_slot` | the attacker's centre goes through an out-of-line `vector3` constructor that rounds `d.x`; no source form made VC6 call it out of line | still differs (D9600, D9601). Later (2026-10-19): `#pragma inline_depth(0)` around the function makes the operators calls that round as the original; 18/18 the same (D9810) |

The strict sweep after the merge: 4,960 matches (4,956 before), none lost; every unit compiles with mingw. In the
Murod and Tentacle Beast fights, ten dual calls each, seven of the eight give the original's results in every
call; `ai_choose_attack_slot` still differs in the 7th digit.

**Review (2026-10-18, advisor).** Checked after the first results:
- Globals that stay in the DLL: two game variables with no address on their line got one
  (`ui_right_double_click_ok` 0x2cbfcd9, which replaced and original functions would otherwise each have kept
  separately, and `tutorial_controls_pages` 0x5f9628, which only the game's initialiser fills). The build now
  lists any such variable; 12 constant tables and three function-local statics remain, which is harmless.
- The rule "a byte-matching function uses the original's data" no longer trusts `vc6-matches.txt` alone: the build
  compares each listed function's bytes with Sum.exe (relocated fields masked) and lists any that no longer match
  (tested by changing one argument of `credits_init`).
- No code outside a replaced function branches into its first 5 bytes (18 hits in the disassembly: 14 loops back
  to the function's own start, 4 switch jump tables decoded as instructions).
- Sum.exe never compares vtable pointers (the comparisons with read-only data are loop ends over tables).
- With `[Fixes] Enabled=0` 5,783 functions are replaced (139 stay original for the display, timing and key hooks)
  and the loaded save still matches the original game with fixes off (0.17%). The skills and spells panels work.

**The never-run review (2026-10-18, five agents, read-only).** The 429 non-matching functions that no scenario
had run were split by area and each one read against the original: VC6 listing beside `asm.mjs fn`, with scripts
comparing calls, constants, masks, struct offsets, jump kinds and relocation targets, jump tables decoded from
Sum.exe, and every remaining difference read by hand. Each agent also listed how its functions can be reached
([the reports](hybrid-review.md)).

| Group | Functions | Errors found | Reachable in single player |
|---|---|---|---|
| Multiplayer, network | 86 | `irc_channel::users` signed instead of unsigned (D9802; `irc_get_channel_list` now byte-matches) | 4 plain, 17 more through the LAN menus on one machine, about 15 more hosting a LAN game alone; the rest need a peer or the old PXO hosts |
| Graphics | 96 | none (six last-bit x87 notes, four in functions with no caller) | 37 (17 of them only with particular content or settings); 32 Direct3D / Glide only, 26 never called |
| UI, HUD, maps, script editor | 90 | the editor's size table filled four size classes instead of three (D9800); the gamma slider's 0.1f * 1.5f folded into 0.15f (D9801) | 79, 40 of them only in the debug script editor |
| Game logic | 113 | none (three x87 notes) | most, through 23 proposed scenarios (bosses by stage, level scripts, console commands); 13 multiplayer or dead |
| Engine | 44 | none (`float_format_buf`'s early-exit value; no caller) | 22; 13 by direct call only; 9 not (PCM sound paths: every shipped WAV is ADPCM) |

None of the three changes the retail game: the size class is unused by the shipped creatures, the gamma value moves
one bit at 53/64-bit FPU precision, and the user count only for an IRC channel of 32,768 users. No variable without
an address (the D9204 kind) turned up. The quicksave chain (`quicksave`, `savegame_location_allowed`, five `savegame::write_*` and
`Character::save_state` functions) had in fact run in every save test: `hybrid-test.mjs` dumped coverage before the
quicksave; it now dumps after it.

**State (2026-10-18, evening).** Done since: the save test over 18 levels, the inventory and trader scenarios,
coverage, dual calls over combat, AI, bosses and spells (the paragraphs above). Thirteen decompilation errors found and
fixed, none of them in a byte-matching function: D9200-D9202 by comparing saves, D9203-D9204 by dual calls,
D9800-D9802 by reviewing the functions no scenario had run, D9803-D9807 by dual calls in the review's scenarios and a broad round
([in-game checks](hybrid-review.md#in-game-checks)). 628 of the 890 non-matching functions have run in a checked
scenario. The x87 precision cases were taken through byte matching (the precision round above); the last two were closed in value on 2026-10-19 (D9809, D9810). The
single-player scenarios are done apart from a few rare branches (listed in the review); multiplayer is left out at
the user's request. Method rule from these runs: never dual-check a function together with its callees.

## The standalone link (2026-10-19)

The whole decompiled source now links into an executable of its own, `Sum-rebuilt.exe`
(`tools/link-build.mjs`), with VC6 SP5's libraries and no part of the original Sum.exe. **Observed:** it starts,
opens its window and reaches the main menu, in OpenGL in a window (`REBUILT_OPENGL`) with the shipped no-CD
behaviour (`SHIPPED_NOCD`).

What it took:
- A first link of all 259 units left only 52 C++ symbols declared and defined nowhere, all of them functions the
  original linker folded into one copy and data whose definition no unit held (the hybrid had resolved them to
  Sum.exe's addresses). They are defined now, with the original's values (D9811). One pair, `help_page`, turned
  out to be two different variables.
- Outside the decomp (`src/link`): the DirectX 7 GUIDs, import libraries for binkw32.dll and EAX.DLL built from
  stubs, MSACM32 from VC6, and a temporary no-op libjpeg (JPEG screenshots write nothing until libjpeg 6b is
  built from the IJG source).
- Two test-build options (D9812), off by default so the default build stays the shipped code: the shipped exe's
  no-CD edit, and the OpenGL renderer as SummonerFix selects it, including SummonerFix's cap on the reported
  maximum texture size: with a modern driver the game's 16384^2 * 4-byte scratch buffer is 1 GB and fails.
- It runs from a separate folder (hard links to the archives, copies of the data and a save), because
  SummonerFix's patches assume Sum.exe's addresses.

**Later the same day.** libjpeg 6b is built from the IJG source into the link. SummonerFix runs inside the rebuilt
exe through an address table, so agent mode drives it like the original ([SummonerFix](summonerfix/index.md),
[tools](tools.md)). Starting a new game then showed what the hybrid could not:
- The first level render crashed (`levelrender_fade_occluders`, a garbage level object index). The `$vis_base`
  visibility set was declared as three separate globals; the code uses them as one `s3d_vis_set`, and the
  standalone link placed them apart (D9813). The hybrid used Sum.exe's addresses, where they are together.
- A static comparison of the initial data (`tools/link-datacheck.mjs`) found three tables with wrong values,
  read only by byte-matching functions, which in the hybrid used Sum.exe's own copies (D9814).
- 71 non-matching functions never ran decompiled in the hybrid, because SummonerFix patches them and the hybrid
  then keeps them original (`gr_init`, `os_wndproc`, `key_mark`, `world_mouse_frame`, `spell_cast_execute`,
  `living_combat::resolve_hit`, `fx_frame`, the dialogue and list drawing...). The standalone exe runs them. With
  the user's settings, 22 of them are kept only for the bug-fix module (`[Fixes] Enabled=0` replaces them) and 49 for
  the display, widescreen, key-binding and other patches (kept original even then).

A new game now reaches Masad.

**Play-test (2026-10-19).** An agent played the rebuilt exe for 75 minutes (still under the original's
double-speed frame limiter, see below, so its timing observations are not comparable; its crash results stand), replaying anything doubtful in the
original: Masad from the start to the boat (dialogue, the tutorials, scout and barbarian fights, loot, inventory,
the character screen and levelling, the Heal spell, map, journal, game over), the boat cutscene, the world map and a
random encounter, Lenele Outskirts (a quest accepted), the Lenele Marketplace trader (buy and sell) and the sewer
golem fight; saving and loading (8 saves). No crash, hang or `errorlog.txt`. Everything matched the original except:
- The prologue cinematic stalled on one sunset shot (the original shows the painted narration panels). The camera
  shot, the cutscene clock and the effect handles were the same as in the original; a hybrid with
  `[Fixes] Enabled=0` (which also replaces the 47 functions only the bug-fix module patches) showed the stall, and
  keeping `fx_frame` original alone (`[Hybrid] Skip=0x48a230`) removed it. Its camera-effect case turned the
  effect the wrong way (D9815, fixed); the prologue now plays in the rebuilt exe.
- The frame rate read lower in the rebuilt exe (75-88 against 105-120 fps at the same places; not visible as
  stutter). Not the code: SummonerFix's rebuilt mode did not install the frame-limiter fix, so the rebuilt exe ran
  with the original game's double-speed limiter while Sum-orig had the fix. With the timing fixes installed in
  rebuilt mode too, both hold 120 fps in Lenele and in Masad.

A run-time data difference at the main menu (`spells`, spell 4's AI recovery time: 0x0a in the original, 4 in
the rebuilt exe) was the same kind of thing: the original's dump had SummonerFix's SpellRecoveryTimes fix on; with
fixes off both hold the record index the unfixed parser stores.

**Second play-test (2026-10-19, after D9815/D9816 and the timing fix).** About an hour further into Act 1 in the
rebuilt exe, partly by level warps and story-stage settings: Flece joining in the sewer and her AI in a golem fight,
party control (Tab, AI modes), Heal and other spells, the Rosalind arrival in Iona (Nar-Iona, IonaIntro, ROSL-Intro,
the Abbot Laurent conversation), a four-member fight, the first summon (Blackfire Elemental from the Ring of
Darkness, its portal and HUD portrait), the Ghost-Dismount cutscene and boss fight, and 8 more saves loaded in both
exes. No crash, hang or `errorlog.txt`; the character sheets matched the original number for number. One
unconfirmed observation: control seemed to pass from Joseph to Jekhar in a grassland fight once; a rerun watching the
current-character pointer did not show it. HP does not come back over time in the rebuilt exe, and not in the original either (checked: Joseph's HP set to
32 of 64 in Sum-orig stayed at 32 over 60 s of play); the game only regenerates AP
(Character::update_regen_and_status, every 7.5 s). Not yet covered: shops and XP levelling in the
later areas, Jekhar's own join, the sewer quests and boss.

A read-through against the disassembly of the 47 functions that `[Fixes] Enabled=0` replaces (the 22 non-matching
ones of the 71 above, and 25 that match), fx_frame's other cases included, found two more errors, in rare branches: the level-drain spell tested the spell's level instead of the target's, and a beneficial armour-triggered spell had the wrong target (D9816, fixed).

## Phase 3 status (2026-10-16)

The source half of the pilot has started without the compiler. Agents rewrote whole translation units in C++ from
the disassembly; I reviewed them, spot-checked a function of each against the listing, and checked that they compile
with mingw (syntax only). They are in [`src/decomp/`](../src/decomp/README.md):
- engine: `frametime`, `timestamp`, `registry`, `cmdline`, `gr_font`, `ui_container`, `targautils`, `window`,
  `character_instance`, `effect_mem`, `memory`, `stringpool`, `plainstring`, `parse`, `file_packfile`, the File code (three placeholder files), the UI gadget base, list box, button, static, build stamp and UI manager, lights, attached lights, the sound system, EAX, quaternions and vectors, containers, experience, effect objects, the PXO user tracker, the level frame and draw lists, the shadow table and equip packets, the mouse, exception-handler, localisation, network and two os modules (placeholder names), `console`, `norlsallocpool`, `matrix`, `skeleton`, `character`, `bmpman`, `gr`, `gr_opengl`, `gr_direct3d`, `gr_glide`;
- game: the credits, help, main and play menus, dialogue, assess, pickup, game create, journal, character select,
  level select, shop, item trade, options, save/load, status, automap, inventory, HUD, skills, spells, tutorial, multiplayer game list and
  PXO chat screens, the interface manager, the level script editor, editor dialogs, boot/title states, frame end, input, the cutscene player, the multiplayer menus, lobby, net session and packets, AI groups, cutscene camera, snow every boss (Serpent, Phoenix, Tiger, Pyrul, Ghost Rider, Machival, Titus, Evil Joseph, Evil Urath, Murod, Murod Oni, Tentacle Beast, Giant Salamanka, Luminar), projectors, lamp glows, decals, the AI combat modes, blob shadows, pathfinding geometry, obstacle avoidance, AI goals, doors and the game camera (under placeholder file names), the multiplayer save, trade, chat, chat-tracker, spell, container, end-game and data code, the CD check, targeting, plus `gamesound`, `item_paging`, `player`, `objects`, `living_entity`, `items`, `characterinfo`, `spells`, `combat`, `s3d`, `levelrender`, `ai`, `boss_ai`, `pathfinding`, `gameplay`, `summoner` (the main module, with WinMain), `conversation`, `level_scripts` (with the savegame code), `level_scripts_common`, all 23 tagged per-level scripts and
  `conversationctl`.

`node tools/decomp-progress.mjs` counts the result (2026-10-18): 7,725 functions written (89.4%). 5,905 of them are
compiled and compared by VC6 SP5, and 4,942 of those give the original bytes and call / data targets (the sweep list
`src/decomp/vc6-matches.txt` has 4,961: 19 compared functions are not `func` lines in sum.symbols). The
`Confidence:` markers follow the sweep (`tools/decomp-confidence.mjs`): every compared function that matches is
`exact`, none that differs is. The other 1,819 written functions are not compared (`tools/decomp-uncompared.mjs`):
about 1,130 are the compiler-generated helpers of global objects (static initialiser, constructor call, atexit
registration, destructor), written as comments above the object's definition; the rest are notes and functions a
macro defines. Their markers (6,387 exact, 1,047 likely, 102 unsure in all) come from reading the code, not from the
compiler. All 259 files compile against one header tree
(`src/decomp/include`; decisions in its `DECISIONS.md`, one numbered block per pilot, last D5703). Pilots written in parallel each changed a copy of the
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
3. *Done 2026-10-16:* the compiler. VC6 Professional builds 251 of 253 files (`levelrender.cpp` needs the
   DirectX 7 headers, `new_help.cpp` a two-reference brace initialiser), and `tools/vc6-compare.mjs` compares each
   function with Sum.exe: about 63% of the compiled functions match byte for byte (best flags `/O2 /GX /MT`,
   `/O2 /Ob0 /GX /MT` for 31 files, `/Od /Ob1 /GX /MT` for the unoptimised units).
4. Raise the match rate. The differences, by kind (68 read by hand): branch layout 24, register allocation 15,
   stack frame 10, wrong types 8; constant folding shows up in many and is avoided by splitting expressions.
5. *Done 2026-10-16:* `living_entity.cpp` is not two units. The VC6 linker keeps an object's data together, and the
   tail's tables and console commands sit among living_entity's globals, so it is one `/O2` object that switches
   optimisation off with `#pragma optimize("", off)` and back on before `character_say`, like the cutscene file
   (D6100). The /Od frames of build 8168 order locals by a hash of their names, so local names decide the frame
   (D6101). First match round (four agents, 38 files): engine 299 -> 369 of 564, game and unoptimised units
   590 -> 743 of 1103, no match lost. What build 8168 cannot reproduce (by-value class returns, some `?:`
   temporaries, register choices) is listed in D6005 and D6107.
6. *Done 2026-10-16:* all game and engine code below the C runtime has source, except libjpeg 6b (about 140
   functions, identified only). The remaining functions of the 8,645 are the C runtime and other library code.
   `levelrender.cpp` and (since D6500) `new_help.cpp` now compile under VC6, so all 259 files do.
7. Keep raising the byte-match rate, file by file, with the techniques in D5904, D6005, D6107, D6205 and D6305.
   Second round (2026-10-16, 20 files): game units 256 -> 329 of 438, engine units 193 -> 216 of 333, no match
   lost. Most of what still differs looks like the compiler build (8168 here, 8966 in Sum.exe): about 25 engine
   functions build a returned `vector3` / `matrix` in place in Sum.exe and copy it here (D6306).
   Third round (20 files): 462 -> 564 of 762, no match lost; the serpent rider's animation remap read the mount's
   animations, like the tiger's (D6400, D6204). Nine more game files: 150 -> 211 of 267 (D6500-D6515).
   Fourth round (17 files): game 187 -> 241 of 296, engine 125 -> 143 of 198, no match lost; over all 259 files VC6
   now matches 4,179 functions (4,107 before). The Pyrul's globals became one `pyrul_boss` struct like the other
   bosses (D6701); `bbox_straddles_plane` takes (min, max, plane) (D6802). No flag set from D6602 gains without a loss.
   Fifth round (2026-10-16, the first under SP5's build 8966; 21 agents and the main session): 4,231 -> 4,956
   matching functions of 5,925 compared over all 259 files, none lost (address by address against the 8966
   baseline). The 144 units no round had touched went first, then every other unit, then second passes on the
   most-differing ones. What it took is in DECISIONS.md D6900-D9099 and the per-agent notes
   `src/decomp/game/NOTES-match-r5-*.md`: the vsdk flags (D7502), source shapes 8966 needs (D7401, D7001, D6900,
   D7301, D7700, D7800, D7901, D8001, D8102, D8302 packet reads through memcpy, D8501, D8701 register saves delayed
   only into single-exit blocks), and header facts: `gr_vertex` derives from `vector3` (D7100), the level name /
   entry point globals are one `level_info` struct (D7201), the Titus state is one object (D8100), `light::enabled`,
   `vfx_dummy_desc` and vfx_material's blend are bit-fields (D7904, D7600, D8202), `character_instance`'s tint is three
   members (D8604), navpoint +0x24 / +0x28 are plain members (D8703), `snd_play_looping` takes five arguments (D8400).
   The rewrite also fixed decomp logic errors (among them D6901-D6903, D7302, D7504, D7604, D7701, D8101, D8200,
   D8600, D8702, D8706, D8801, D8901) and found four bugs in the original game, filed 2026-10-17 in
   [Bugs](sum-exe/game-bugs.md) as C314, C315, C316 and G311: giant_salamanka_set_flinch loops forever on a move with no
   action (D6901), characterinfo_find_multi_slot loops on its error when all four multiplayer slots are used (D7604),
   avoid_area_rebuild copies up to eight obstacle indices into a three-entry stack array (D7503), and
   script_object_receive_go never advances its action pointer (D8602). 969 compared functions still differ; what is
   left is mostly register choice, frame-slot reuse, prologue placement and FPU scheduling (the "still differs"
   blocks of each pass list the forms tried).

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

**Visual C++ 6.0 Standard Edition (tested 2026-10-16).** A Japanese VC6 Standard Edition disc (cl 12.00.8168, the
release build) compiles from its extracted `VC98\BIN` without setup, but its compiler ignores `/O2`: the output is the
frame-based code of `/Od` (**observed** on a test loop). Standard Edition has no optimiser, so it can only be
compared against the units Sum.exe ships unoptimised (living_entity.cpp, objects.cpp, the cutscene file). Even those
need the `src/decomp` headers made VC6-clean first (they include `stdint.h`, which VC6 lacks). The optimised units
need the Professional or Enterprise Edition, ideally with the service pack that gives build 8966.

**Visual C++ 6.0 Professional Edition (tested 2026-10-16).** A German VC6 Professional disc (February 1999, part
X04-63216; cl 12.00.8168, the release build) optimises. `vector2::len` (`0x523f80`) and `vector2::normalize`
(`0x523fc0`), written as in `src/decomp/vsdk/unnamed_vector_0x523f30.cpp` and compiled with `cl /O2`, give the same
bytes as Sum.exe apart from the relocated addresses (**observed**). So byte-level comparison is possible now, even
before the service pack that gives build 8966 is found; where the release build and 8966 differ is not known yet.

**First comparison with VC6 (2026-10-16).** `src/decomp` now compiles under VC6 Professional (cl 12.00.8168,
`/c /O2 /MT`): 202 of 204 units (**observed**); the header changes are DECISIONS.md D4900-D4905. The two left are
new_help.cpp (a table VC6 cannot brace-initialise, D4902) and levelrender.cpp (needs the DirectX 7 SDK headers,
D4904); both compile since (levelrender's DirectX types stubbed by decomp76, new_help's tables as `[][2]`
arrays in D6500). [`tools/vc6-compare.mjs`](tools.md#toolsvc6-comparemjs) compares the compiled functions with Sum.exe:

| Sample | Flags | Byte-identical functions |
|---|---|---|
| vector (unnamed_vector_0x523f30), matrix | `/O2 /Ob0` (the units call their inline members) | 62 of 97 |
| frametime, timestamp, registry, stringpool, plainstring, cmdline, memory | `/O2` | 90 of 120 |
| objects, living_entity, cutscene (unnamed_cutscene_0x434900) | `/Od /Ob1 /GX` | 100 of 200 (objects 35 of 40) |

Most differing functions keep their size: the differences are scheduling and register choice, branch and block
order, the order of x87 operations and constants (a few `double` literals where Sum.exe has `float`), and, in
the unoptimised units, the stack frame size and the order of the locals (declaration order) and exception frames.
**Inferred:** most are source shape (one pattern, `return strcpy(..)`, turned six functions into matches, D4905);
some may be the 8168 / 8966 compiler difference, which can't be told apart until build 8966 is found.

**Build 8966 is Visual Studio 6.0 Service Pack 5 (found 2026-10-16).** The `@comp.id` record that the Rich header
counts is written by the code generator, `c2.dll`, so its build is the one to find. SP5 (`vs6sp5.exe`, February
2001; the copy used is archive.org item `vcpp5`, MD5 971e5b1791ee90e6f397ec7fae520ba9, extracted with 7-Zip, never
installed) ships `c2.dll` 12.00.8966 under the name `msvcep.dll`. Its setup script (`sp598ent.inf` / `.stf`) renames it
to `c2.dll` for the Professional and Enterprise editions; the other `c2.dll` variants in the package are for the
Learning and introductory editions (11.99.8966, 11.98.8798) and for the Processor Pack (13.00.9044). SP5 also has
`c1xx.dll` 12.00.8964, `c1.dll` 12.00.8867, `cl.exe` 12.00.8804 and `link.exe` 6.00.8447, the linker build Sum.exe
records. A test object compiled with these files carries `Utc12_CPP` build 8966, as the 348 C++ objects of Sum.exe
do (**observed**). The Visual C++ 6.0 Processor Pack is not it: its `c2.dll` is 13.00.9044.

All VC6 comparisons from this point use a `VC98` folder with SP5's `BIN` files (`cl.exe`, `c1.dll`, `c1xx.dll`,
`link.exe`, and `msvcep.dll` copied as `c2.dll`) over the Professional Edition's. Over all 259 files the same
source gave 4,179 matching functions under 8168 and 4,231 under 8966 (65 gained, 13 lost: source shapes tuned to the
release build's quirks, D7400 / D7401).

**Compiler options.** The game units are `/O2 /GX /MT`. The vsdk engine library was built with `/Ox /Ow` (no string
pooling, aliasing assumed only across calls), not `/O2` (D7502). Compiled per file both ways, 90 vsdk units gain or
stay level under `/Ox /Ow` (102 more matches, none lost) and two (gr_glide, unnamed_net) keep `/O2`. Several patterns
earlier rounds listed as impossible (a `strcat` of a short literal, loads kept ahead of stores through pointers)
were this flag. `src/decomp/vc6-flags.txt` holds the line for each unit that does not use the default.
