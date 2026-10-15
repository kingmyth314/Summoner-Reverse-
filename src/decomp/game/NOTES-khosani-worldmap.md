# Phase 3: level_script_khosani_lab.cpp / level_script_worldmap.cpp (decomp34)

`game/level_script_khosani_lab.cpp` and `game/level_script_worldmap.cpp` are levelscripts\Scripts\
level_script_khosani_lab.cpp (code 0x4f77c0-0x4f9e4b, initialiser entry 491) and
level_script_worldmap.cpp (0x4fb4d0-0x4fd6e1, entries 492-495) of Sum.exe v1.40, written from the
disassembly only (`node tools/asm.mjs fn`; the jump tables, the two 10 x 7 tables, the encounter level list
and every float constant read from the raw bytes of `Summoner/Sum.exe`). No Summoner 2 or Dash Faction code
was read.

Both files pass `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include
<file>` with no diagnostics against `include/` (the repo's tree at b5e94ab plus CHANGED-HEADERS.txt). Every
existing `src/decomp/game/*.cpp` and `src/decomp/vsdk/*.cpp` (62 files) gives the same diagnostics with this
tree as with the repo's. Decisions D1400-D1407 are appended to `include/DECISIONS.md`.

**Everything in both units is written.** Module column of sum.symbols: 48 + 31 functions, all with a
`// 0xADDR  Confidence:` line.

| File | Functions | Exact | Likely | Unsure |
|---|---|---|---|---|
| level_script_khosani_lab.cpp | 48 (incl. 2 initialiser helpers and the update__trace block) | 33 | 15 | 0 |
| level_script_worldmap.cpp | 31 (incl. 8 initialiser helpers) | 18 | 13 | 0 |

## Header scheme (D1400)

Each per-level unit had, as far as the binary shows, no header anyone else needed: the hooks are reached only
through level_script_table (declared in `levelscripts/level_scripts.h`, decomp27) and no other code reads
the units' globals or calls their helpers (`tools/asm.mjs grep` on every .bss address and helper). So no
`level_script_<name>.h`; the helpers are declared at the top of each .cpp, not `static` (no evidence either
way), and the unit-local type `ls_khosanilab_cell` lives in the .cpp. New declarations of other units went
into the matching headers (levelrender.h, living_entity.h, ai.h, boss_ai.h, summoner.h, level_scripts.h) and
one new header, `levelscripts/script_internal.h` (navpoint_get_position).

## level_script_khosani_lab.cpp (0x4f77c0-0x4f9e4b)

| Address | Function | Confidence |
|---|---|---|
| 0x4f77c0 | static_init_ls_khosanilab_timer | exact |
| 0x4f77d0 | init_ls_khosanilab_timer | exact |
| 0x4f77e0 | ls_khosanilab_puzzle_update | likely |
| 0x4f77fa | ls_khosanilab_puzzle_update__trace (tail block of 0x4f77e0) | likely |
| 0x4f78a0 | ls_khosanilab_puzzle_flush_path | exact |
| 0x4f78e0 | ls_khosanilab_puzzle_clear_paths | exact |
| 0x4f7930 | ls_khosanilab_puzzle_flood_connected | likely |
| 0x4f7ad0 | ls_khosanilab_puzzle_cell_open | likely |
| 0x4f7b20 | ls_khosanilab_puzzle_cell_valid | exact |
| 0x4f7b50 | ls_khosanilab_puzzle_trace_beam | likely |
| 0x4f7ed0 | ls_khosanilab_puzzle_turn_dirs | likely |
| 0x4f7fb0 | ls_khosanilab_puzzle_reflect_dir | likely |
| 0x4f7fe0 | ls_khosanilab_puzzle_corner_nn | likely |
| 0x4f8070 | ls_khosanilab_puzzle_corner_np | likely |
| 0x4f8100 | ls_khosanilab_puzzle_corner_pp | likely |
| 0x4f8190 | ls_khosanilab_puzzle_choose_turn | likely |
| 0x4f83d0 | ls_khosanilab_puzzle_dir_to_side_bit | exact |
| 0x4f8400 | ls_khosanilab_puzzle_emit_segment | likely |
| 0x4f8810 | ls_khosanilab_puzzle_cell_to_world | likely |
| 0x4f8850 | ls_khosanilab_puzzle_add_point | likely |
| 0x4f88c0 | ls_khosanilab_puzzle_idle_path | exact |
| 0x4f8920 | ls_khosanilab_init | exact |
| 0x4f89c0 | ls_khosanilab_apply_lava_platforms | exact |
| 0x4f8a30 | ls_khosanilab_frame | exact |
| 0x4f8c60 | ls_khosanilab_room1_frame | exact |
| 0x4f8d00 | lsf_room1_killed | exact |
| 0x4f8d70 | ls_khosanilab_room4_frame | exact |
| 0x4f8eb0 | ls_khosanilab_room4_check_done | exact |
| 0x4f8f30 | ls_khosanilab_reset_runes | exact |
| 0x4f8f80 | ls_khosanilab_room1_intro | exact |
| 0x4f8ff0 | ls_khosanilab_room2_intro | exact |
| 0x4f90a0 | ls_khosanilab_room4_intro | exact |
| 0x4f9130 | ls_khosanilab_room5_intro | exact |
| 0x4f91a0 | ls_khosanilab_room7_intro | exact |
| 0x4f9210 | lsf_room8 | exact |
| 0x4f92a0 | ls_khosanilab_salamanka_fight | exact |
| 0x4f9520 | ls_khosanilab_salamanka_intro_cs_end | exact |
| 0x4f9540 | ls_khosanilab_salamanka_death_cs_end | exact |
| 0x4f9560 | ls_khosanilab_salamanka_trans1_cs_end | exact |
| 0x4f9570 | ls_khosanilab_salamanka_trans2_cs_end | exact |
| 0x4f9580 | ls_khosanilab_leave | exact |
| 0x4f95a0 | ls_khosanilab_use | exact |
| 0x4f96f0 | ls_khosanilab_pylon_use | exact |
| 0x4f99e0 | ls_khosanilab_room2_doors | exact |
| 0x4f9b10 | ls_khosanilab_puzzle_use | likely |
| 0x4f9c40 | ls_khosanilab_puzzle_build_grid | exact |
| 0x4f9ca0 | ls_khosanilab_set_flippers | exact |
| 0x4f9d10 | ls_khosanilab_room7_doors | exact |

Data: .data `ls_khosanilab_flipper_ids` (0x5a3c78) and `ls_khosanilab_lava_platform_ids` (0x5a3d90), both
`int[10][7]` (names ours; the 0x5a3ea8 "sal_killed" compare in apply_lava_platforms is the second table's
end). .bss 0x257eb10-0x257ecf9 in address order, the timer at its place. Types differing from sum.symbols:
`ls_khosanilab_puzzle_grid` int[8][5] (sum.symbols int[40]; indexed x * 5 + y), `ls_khosanilab_path`
avoid_spline_def (uint8[0xc4]; D1402), `ls_khosanilab_flippers` bool[10][7] (uint8[0x46]; pushed as is to
door_apply_saved_state, toggled with test / sete).

How the puzzle works (all observed in the code): the 32 flipper doors sit on the 10 x 7 board with a zero
border; using one toggles it and its four neighbours (`set_flippers` has no bounds check; the zero border is
the check). Rows 1..8, columns 1..5 become the 8 x 5 beam grid (`grid[8 - row][5 - col]`, bit 0 = up); the
beam enters at grid cell (0, 2) (= flipper 28) going in direction 1. `turn_dirs` gives per direction code
the straight-on vector b and the two turns a / c; `trace_beam` walks until it is back at the start cell in the
start direction, adding corner points (0.3 / 0.7 inside the cell) as avoidance-spline segments
(`emit_segment` -> `cell_to_world` -> `add_point`, 8 pairs per spline). Closed cells next to the lit region
(`flood_connected`) start second beams with state 0. After each use, everyone within 9 units of `$badplace`
is moved to player start 3. `apply_lava_platforms` shows `lavaplatformNN` of every raised flipper.

### Open questions

- 0x257ebbc (between path_len and the path) is never referenced: padding or an unused variable.
- `flood_connected` takes the cell by value (the neighbours are built in the argument slots with a dead
  store of the unchanged coordinate first, `n = c; n.x--`). sum.symbols writes `(x, y)`: the same ABI.
- `trace_beam`'s loop: the head test (`cur == start && dir == start_dir && !first`) and the two inner
  tests are separate in the code; written as a while loop with a `first` flag and `continue`s.
- The turn_dirs switch cases are written in the code order 2, 0, 3, 1 (jump table 0x4f7f98: 0x4f7f10,
  0x4f7f56, 0x4f7eed, 0x4f7f33).
- `cell_open` returns `grid & 1` / `~grid & 1` without a bool conversion (written as bool; a `char` return
  type would explain the code as well).
- `cell_to_world` stores the origin constants first and then computes from the constants, not from a
  reload: written as two stores and two `-=`.

## level_script_worldmap.cpp (0x4fb4d0-0x4fd6e1)

| Address | Function | Confidence |
|---|---|---|
| 0x4fb4d0 | static_init_empty_4fb4e0 | likely |
| 0x4fb4e0 | init_empty_4fb4e0 | likely |
| 0x4fb4f0 | static_init_ls_worldmap1_timer | exact |
| 0x4fb500 | init_ls_worldmap1_timer | exact |
| 0x4fb510 | ls_worldmap1_init | likely |
| 0x4fb6d0 | ls_worldmap1_setup_lenele | exact |
| 0x4fb830 | ls_worldmap1_setup_masad | exact |
| 0x4fb860 | ls_worldmap1_setup_iona | exact |
| 0x4fb940 | ls_worldmap1_setup_ikaemos | exact |
| 0x4fb9c0 | ls_worldmap1_setup_khosani | exact |
| 0x4fbcb0 | ls_worldmap1_setup_jadetemple | exact |
| 0x4fbd00 | ls_worldmap1_setup_wolong | exact |
| 0x4fbd50 | ls_worldmap1_setup_liangshan | exact |
| 0x4fbd80 | ls_worldmap1_frame | exact |
| 0x4fbed0 | ls_worldmap1_roll_encounter | likely |
| 0x4fbfd0 | ls_worldmap1_pick_encounter_early | likely |
| 0x4fc2a0 | ls_worldmap1_nearest_hills_nav | likely |
| 0x4fc350 | ls_worldmap1_nearest_forest_nav | likely |
| 0x4fc400 | ls_worldmap1_nearest_grass_nav | likely |
| 0x4fc4b0 | ls_worldmap1_pick_encounter_stage4 | likely |
| 0x4fcaf0 | ls_worldmap1_pick_encounter_stage9 | likely |
| 0x4fcce0 | ls_worldmap1_nearest_desert_nav | likely |
| 0x4fcd90 | ls_worldmap1_nearest_orenian_nav | likely |
| 0x4fce40 | ls_worldmap1_pick_encounter_stage15 | likely |
| 0x4fd560 | ls_worldmap1_leave | exact |
| 0x4fd5c0 | static_init_ccmd_iw | exact |
| 0x4fd5d0 | init_ccmd_iw | exact |
| 0x4fd5f0 | cmd_iw | exact |
| 0x4fd600 | static_init_ccmd_random_encounters | exact |
| 0x4fd610 | init_ccmd_random_encounters | exact |
| 0x4fd630 | cmd_random_encounters | exact |

Data: .data `ls_worldmap1_encounter_levels` (0x5a4c18, 12 names) and `ls_worldmap1_num_encounter_levels`
(0x5a4c48 = 12, re-read every loop iteration, so a variable), `Random_encounters_enabled` (0x5a4c4c = 1).
.bss: `ls_worldmap1_start` (0x257ed20, a `navpoint`, name ours: sum.symbols' `ls_worldmap1_heading`
0x257ed30 is its +0x10 field; init writes +0x14 / +0x18 / +0x1c / +0x20 / +0x10 and points all four
level_player_starts at it), the timer (0x257ed08), ccmd_iw (0x257ed50), ccmd_random_encounters (0x257ed10).

### Open questions

- The empty initialiser 0x4fb4e0 (entry 492, before the timer's): written as the implicit constructor of
  `ls_worldmap1_start` (navpoint holds a vector3, whose constructor is empty and inline, so VC6 emits a
  dynamic initialiser that does nothing). That is the only object of the unit with such a type. The same
  reasoning may explain level_scripts.cpp's unreproduced empty initialiser 0x4d5020 (navpoints /
  level_items hold vector3s) - not checked.
- .bss order vs initialiser order: initialisers run start, timer, ccmd_iw, ccmd_random_encounters, but the
  addresses are timer 0x257ed08 < ccmd_random_encounters 0x257ed10 < start 0x257ed20 < ccmd_iw 0x257ed50
  (the same open question as in the other pilots). 0x257ed0c is never referenced.
- `roll_encounter`: the ring of stone case returns true at once while light / fire zero the roll; a third
  `roll = 0` folded by the compiler would give the same code.
- The pickers' shared tails (one strcpy / strcat to level_script_name reached from several branches) are
  written as separate statements; the Korel / Zane printf branches in `_early` share their continuation.
- The navpoint searches square z, x, y in that order; written as `sqrt(dx*dx + dy*dy + dz*dz)`.

## Disagreements with the docs and sum.symbols

- **docs/sum-exe/source-map.md**, strings.tbl id table: the row `0x4f9d10`-`0x4fa230` | 3 |
  level_script_ikaemosu.cpp starts at ls_khosanilab_room7_doors (this unit; it reads no strings.tbl id);
  the ikaemosu id readers are 0x4f9f50 (L485), 0x4fa140 (L484), 0x4fa230 (L486). The row `0x4fbd50` | 1 |
  level_script_worldmap.cpp names ls_worldmap1_setup_liangshan; the reader of L568 is ls_worldmap1_frame
  (0x4fbd80). Both look like starts taken before the hidden starts were split.
- **docs/sum-exe/worldmap.md** agrees with the code on every exit, roll and encounter row checked. Small
  omissions: init also re-activates the leader (`set_ai_active(1)`, bit 8 cleared) and calls
  game_stage_get once without using it; the stage 9..14 picker throws one frand() away before the suffix
  rolls; got_ikaemos_map is read from level 5's block and got_ring_of_fire from level 14's.
- **docs/sum-exe/level-scripts.md** ("72 helpers named lsf_<flag> after the first flag they test"):
  lsf_room1_killed tests no flag itself (its caller ls_khosanilab_frame does); fine as a name, the rule
  is approximate.
- **sum.symbols**: ls_khosanilab_room4_frame opens prisonlid01..08 (not 01..06);
  ls_khosanilab_room2_doors / room7_doors handle doors 1..4 (not 1..3); ls_khosanilab_path's points are
  at +0x44 / +0x84 (not +0x7c); ls_khosanilab_timer is not used by ls_ionaext_action and
  ls_worldmap1_encounter_timer not by ls_lenele1b_action (no references outside their units);
  ls_worldmap1_setup_jadetemple makes jadetemple the live exit at stage 14+ and eleh a script-use trigger
  (the comment says "eleh on, jadetemple off"); ls_worldmap1_setup_liangshan makes liangshan a live exit at
  13+ ("disables" is the old enable / disable naming).

## Game bugs and quirks noticed (not run in the game)

- **Khosani lab, flipper puzzle state is not saved (inferred).** ls_khosanilab_init clears
  ls_khosanilab_flippers on every entry and hides the matching lava platforms, but the flipper doors move
  through door_apply_saved_state, i.e. their positions are in the save. After a reload (or leaving and
  coming back) raised flipper doors stay raised while the board array says down, the platforms of those
  flippers are hidden, and the next use toggles the array out of step with the doors. Needs a game test.
- **Khosani lab, room 4 (observed):** the four prisoners' script objects get `end_mode = 1` through
  `script_objects[script_object_find(name)]` without the -1 check; a missing prisoner writes 0x550 bytes
  before script_objects. Harmless with the shipped level.
- **Khosani lab, rewards (observed, design):** Ulsadana's room 8 line gives 2000 XP unless all four steel
  golems were killed (room7_killed is set only when all four are dead, so releasing and killing three
  still pays); room 2's 1000 XP needs the symbol never stepped on, room 4's 1200 XP needs the fourth onyx
  golem door never opened.
- **Khosani lab, use handler (observed):** the flipper name test compares 14 characters
  ("puzzle-flipper"); every zero board cell names "lavaplatform00" in apply_lava_platforms (40 hide / show
  calls of one prefix per update; the last zero cell, hide, wins). emit_segment's top-edge detour tests
  only p0's y (both are tested on the bottom edge); harmless because the corner x values never equal 5.5.
- **World map, stage 4..8 grass (observed, already in worldmap.md):** the Zane branch copies
  rand-grassland01_v3 and falls through to the general rolls, which overwrite it.
- **World map, stage 4..8 hills (observed, theoretical):** `_stricmp(nearest->name, "$hills001")` runs on
  the hills result even when no `$hills` navpoint exists (NULL) and hills won the compare because every
  distance was 10000; a crash only on a map without terrain navpoints.
- **World map (observed):** the 10 s timer restarts before the moving / Ai_paused tests, so standing still
  costs a check (as worldmap.md says); the script names "rand-grasslandnite012" / "rand-grasslandnite013"
  (stage 4..8 roll 0.1..0.55, stage 15+ fallback) look like `_v2` / `_v3` typed without the underscore -
  not checked against script_filenames.tbl (not extracted).
