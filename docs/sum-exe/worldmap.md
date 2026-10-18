---
title: World map and encounters
parent: Sum.exe
nav_order: 6.32
---

# World map and encounters
{: .no_toc }

1. TOC
{:toc}

**Summary:** The world map is an ordinary level, `worldmap1` (level index 4). Only the leader walks on
it. Locations are level-exit triggers, and the game stage decides which exit of each town is live.
While you move, the game checks for a random encounter every 10 seconds (the first check comes after
20 seconds). Each check has a 50% chance. An encounter loads a `rand-*` level that matches the
terrain under the nearest `$hills` / `$forest` / `$grass` (or `$desert` / `$orenian`) navpoint. The
stage bands are < 4, 4..8, 9..14 and 15+. The console command `random_encounters` turns encounters
off.

{: .note }
Function names are our labels from `symbols/sum.symbols`. The evidence table at the end lists
function addresses, not REA evidence ids. Written 2026-10-14 from the disassembly (`tools/asm.mjs`),
agent draft; the roll (10 s timer, moving check, 0.5) was re-checked. The game was not run for this page.

## The world map as a level

The handlers are in the `worldmap1` record of `level_script_table`. See [Level scripts](level-scripts.md).

| Slot | Function | What it does |
|---|---|---|
| init | `ls_worldmap1_init` (`0x4fb510`) | restores the saved position after an encounter, sets up the location exits, starts the encounter timer at 20 000 ms, plays music `worldmap1`, sets the leader's speed, turns off the other party members |
| frame | `ls_worldmap1_frame` (`0x4fbd80`) | rolls for an encounter and loads the encounter level |
| leave | `ls_worldmap1_leave` (`0x4fd560`) | restores the leader's run speed and turns the party members back on |

- **Observed:** init sets the leader's speed scale to `(walk + run) * 0.5` (`+0x6b8`, `+0x6b4`,
  constant `0x57e6b0` = 0.5) through `character_set_speed_scale` (`0x462260`). Leave puts back the run speed.
- **Observed:** for every living entity of type 7 (party) other than `cur_character`, init calls
  `character_set_ai_active(0)` and sets bit `0x8` of `+0x320`. Leave clears it. *Inferred:* bit `0x8`
  hides the member, so only the leader is seen on the map.
- **Observed:** console command `iw` (`cmd_iw`, `0x4fd5f0`) is a bare `jmp ls_worldmap1_init`. It
  re-runs the setup and resets the encounter timer to 20 s.

## Locations and travel

**Observed:** you travel by walking into a location's level trigger. The normal exit code then runs
(`level_check_triggers`, `0x4caa00`). Each location has several triggers: one per destination level
and script variant (for example `lenele1d` with variant `lenele1d_v3`). The `ls_worldmap1_setup_*`
functions pick one trigger per location by game stage (`game_stage_get`, `0x4d47b0`) and story flags.

{: .important }
`level_trigger_set_script_use` (`0x4ccff0`, formerly "level_trigger_enable") **sets** bit `0x2` of
trigger `+0x54`. `level_check_triggers` calls `level_script_use` for a trigger with that bit set. It
treats the trigger as a level exit only when the bit is clear (`test byte ptr [esi-4], 2` at
`0x4cabbf` for a line trigger and at `0x4cac4f` for a radius trigger). `level_trigger_set_exit` (`0x4ccf90`,
formerly "level_trigger_disable") clears the bit, so it is the one that makes an exit live. In every setup
function, exactly one trigger per location gets `level_trigger_set_exit`. The table below is written in
terms of the live exit.

- **Observed:** a trigger record starts with flags `+0x54` = 1 (`0x4cc1d8`), so every trigger of the
  level `.tbl` is a live exit until a script sets bit `0x2`. Bit `0x4` is `+Multi`: in single player
  (`game_flags` bit `0x1`) those triggers are skipped, in multiplayer (bit `0x2`) only those are used
  (`0x4caa79`..`0x4caa95`).
- **Observed:** when an exit is crossed, `level_check_triggers` stores the trigger in
  `level_exit_trigger` (`0x24902c4`), its script name in `level_pending_exit_name` and its `+Index:`
  value (trigger `+0x50`) in `0x2490c60`. After the player confirms, `level_apply_pending_change`
  (`0x4ccd10`) copies that value into `level_entry_point` (`0x60a33c`, `0x4ccda6`), which picks the
  `$player<index>-01`..`04` start navpoints of the next level.

| Location | Setup function | Live exit (level / variant) by stage |
|---|---|---|
| Lenele | `ls_worldmap1_setup_lenele` (`0x4fb6d0`) | < 4: `lenele1d`; 4..5: `lenele1d_v2`; 6: `lenele1d_v3`; 7..14: `lenele2d`; 15+: `lenele3d` |
| Masad | `ls_worldmap1_setup_masad` (`0x4fb830`) | < 9: `masad` / `masad_v2`; 9+: none, objects `Masad*` hidden |
| Iona | `ls_worldmap1_setup_iona` (`0x4fb860`) | < 2: none, `Iona*` hidden; 2..8: `ionaext`; 9..15: `ionaext02`; 16+: `ionaext02` / `ionaext02_v2` |
| Ikaemos | `ls_worldmap1_setup_ikaemos` (`0x4fb940`) | 9+: `ikaemosext2`; else with `got_ikaemos_map`: `ikaemosext`; else none, `Ikaemos*` hidden |
| Khosani | `ls_worldmap1_setup_khosani` (`0x4fb9c0`) | 20+: `_v6`; 18..19: `_v5`; 17: `_v4`; 9..16: `_v3`; `got_ring_of_fire`: `_v2`; `found_khosani`: `khosanistrng` (all `khosanistrng` variants) |
| Jade Temple / Eleh | `ls_worldmap1_setup_jadetemple` (`0x4fbcb0`) | 14+: `jadetemple` (`eleh` stays off); < 14: none, `JTE*` hidden |
| Wolong | `ls_worldmap1_setup_wolong` (`0x4fbd00`) | < 13: `wolong`; 13+: `wolong2` / `wolong2_v2` |
| Liangshan | `ls_worldmap1_setup_liangshan` (`0x4fbd50`) | < 13: none, `Liangshan*` hidden; 13+: `liangshan` |

- **Observed:** "hidden" means `lobjs_hide_by_prefix` (`0x4777f0`) sets flag `0x80` on every level
  object whose name starts with that prefix. *Inferred:* these are the town models on the map.
- *Unknown:* what `level_script_use` does on the world map when you walk into an inactive trigger.
- **Observed:** the exit check skips the "Hostile monsters nearby" and "Party members too far away"
  tests on the world map (index 4), in towns (`level_is_town`) and in every level whose file name
  starts with `rand` (all the `rand-*` encounter maps). See
  [Leaving an encounter](#leaving-an-encounter).

### What is stored in the save

The game keeps no list of unlocked locations. **Observed:** every check above reads the game stage
(itself computed from story flags) or a story flag, so the save's flag bits decide everything
([Savegame, level data](savegame.md#level-data-story-flags-automap-and-variables-observed)). The
world map's own block holds these named variables:

| Name | Type | Written | Read |
|---|---|---|---|
| `worldmap_last_pos_x` / `_y` / `_z` | float | `ls_worldmap1_frame`, when an encounter starts | `ls_worldmap1_init` |
| `worldmap_last_pos_plane` | int (leader `+0x68c`) | same | same |
| `worldmap_last_pos_heading` | float (from `matrix3__extract_angles`) | same | same |
| `found_khosani` | flag (world map level) | *unknown* | `worldmap_init_locations`, Khosani setup, stage 4..8 picker |

**Observed:** `ls_worldmap1_init` restores the position only when `level_prev_filename` matches one
of the 12 names at `0x5a4c18` (count `0x5a4c48` = 12): `Rand-Forest01`, `Rand-Grassland01`,
`Rand-Iceland01`, `Rand-Hills01`, `Rand-Desert`, `Rand-Orenia01` and their `Nite` versions. It then points
all four `level_player_starts` at the saved marker `0x257ed20`. So after an encounter you come back
where you left the map. After a town you start at that town's normal entry point.

## On-screen location markers

These are part of the HUD ([Interface screens](interface.md)).

- **Observed:** `worldmap_load_overlay` (`0x42f320`) runs only when `cur_level_index` is 4. It loads
  the `LandmarkArrow` bitmap from the interface path, calls `worldmap_init_locations` (`0x42f3b0`) and sets
  `worldmap_show_locations` (`0x5f9834`).
- **Observed:** `worldmap_init_locations` fills 8 records of 0x2c bytes at `0x5f96c8`. Each has a
  navpoint (`navpoint_find`), a label and the label size. Bit `0x1` (shown) is set by these rules:

| Navpoint | Label | Shown when |
|---|---|---|
| `$masad` | Masad | stage < 9 |
| `$lenele` | Lenele | stage not in 9..14 |
| `$iona` | Iona | stage >= 2 and not in 9..14 |
| `$ikaemos` | Ikaemos | `got_ikaemos_map` and stage not in 9..14 |
| `$khosani` | Khosani | `found_khosani` and stage not in 9..14 |
| `$eleh` | Eleh | stage 14 |
| `$wolong` | Wolong | stage 9..14 |
| `$liangshan` | Liangshan | stage 13..14 |

- *Inferred:* stages 9..14 are spent on a different part of the map, which is why the first five
  markers go away then and come back at 15.
- **Observed:** each frame, `worldmap_frame` (`0x42f500`, called from `interface_frame` on screen 0)
  projects every shown location. If its navpoint is off screen, it gets an arrow (bit `0x2`) placed on the
  screen edge in the direction of the location (`worldmap_marker_screen_angle`, `_place_on_edge`,
  `_clamp_to_edge`). The arrow is scaled by distance: `clamp((50 - d) * 0.025, 0, 1) * 0.5 + 0.5`
  (`worldmap_marker_scale_for_distance`). If the location is on screen, it gets a label box at its position (bit `0x4`).
  `worldmap_draw_location` (`0x42fac0`) draws both.
- **Observed:** key code `0x26` (*inferred:* DirectInput `L`) toggles `worldmap_show_locations` and
  plays sound `0x27`. `worldmap_frame` has no mouse or click handling.

## Random encounters

### When a check happens

`ls_worldmap1_roll_encounter` (`0x4fbed0`) runs every frame. It returns true when:

1. `Random_encounters_enabled` (`0x5a4c4c`, default 1) is set, **and**
2. `ls_worldmap1_encounter_timer` (`0x257ed08`) has run out. It is then reset to 10 000 ms **before**
   the remaining tests, so a failed check still costs 10 s, **and**
3. `character_get_698` of the leader is not 0.0. *Inferred:* `+0x698` is the current movement speed,
   so standing still never gives an encounter, **and**
4. `Ai_paused` is clear, **and**
5. `frand() <= 0.5` (`fcomp [0x57e6b0]`, `test ah, 0x41`).

**Observed:** step 5 is skipped or forced by the boss rings. If you have `got_ring_of_light` but not
`ghost_intro_played`, or `got_ring_of_fire` but not `phoenix_intro_played`, the roll is set to 0. If you have `got_ring_of_stone`
but not `serpent_intro_played`, it returns true at once. So after you get one of those rings, the next check
always gives an encounter.

```
4fbf28: call frand
4fbf2d: fstp dword ptr [esp]          ; roll
...                                   ; ring / intro flags may set roll = 0
4fbfb7: fld dword ptr [esp]
4fbfbb: fcomp dword ptr [0x57e6b0]    ; 0.5
4fbfc3: test ah, 0x41                 ; roll < 0.5 or roll == 0.5
4fbfc6: je 0x4fbfcc                   ; no encounter
```

*Inferred:* while moving, there is one 50% check per 10 s. That gives about one encounter per 20 s of
walking. The first check comes 20 s after the map loads.

### What happens on an encounter

**Observed**, in `ls_worldmap1_frame`:

1. `hud_show_message` with localized string 568 ("Encounter!").
2. Saves the leader's position, plane and heading into the five `worldmap_last_pos_*` variables.
3. Calls a picker by game stage: 15+ `ls_worldmap1_pick_encounter_stage15` (`0x4fce40`), 9..14
   `_stage9` (`0x4fcaf0`), 4..8 `_stage4` (`0x4fc4b0`), < 4 `_early` (`0x4fbfd0`).
4. The picker writes the map to `level_filename` and the script variant to `level_script_name`
   (see `level_script_find_index`, `0x4d6340`).
5. `level_entry_point = 1`, `gameseq_set_state(4)`.

### Terrain

**Observed:** the `ls_worldmap1_nearest_*_nav` functions (`0x4fc2a0` hills, `0x4fc350` forest,
`0x4fc400` grass, `0x4fcce0` desert, `0x4fcd90` orenian) scan the navpoint table for names that
start with `$hills`, `$forest` and so on. They return the nearest one (by 3D distance from the leader)
and its distance. The distance is 10000.0 if there is none. The picker uses the smallest distance; on a tie
the earlier terrain wins (hills, then forest, then grass; desert over orenian only when strictly nearer).
The terrain is the nearest map marker, not the ground you stand on.

### Encounter table

Rolls are separate `frand()` calls unless noted. "Map" is `level_filename`. "Script" is
`level_script_name` (the same as the map name when not given). All **observed**.

| Stage | Terrain | Result |
|---|---|---|
| < 4 | hills | < 0.5 `rand-hills01`; else `rand-hillsnite01` |
| < 4 | forest | < 0.5 `rand-forest01`; else `rand-forestnite1` |
| < 4 | grass | Korel or Zane active (`unhide_*` and not `no_more_*`) and < 0.5: `rand-grassland01` script `_v3`; else < 0.45 `rand-grassland01`; < 0.55 script `rand-grassland01_v2`; else `rand-grasslandnite01` |
| 4..8 | any (checked first) | `got_ring_of_light`, not `ghost_killed`: `rand-forestnite1` script `_v2` (Ghost Rider); `got_ring_of_fire`, not `phoenix_killed`: `rand-hills01` script `_v2` (Phoenix); `got_ring_of_stone`, not `serpent_killed`: `rand-forest01` script `_v2` (Serpent) |
| 4..8 | hills | nearest is `$hills001`, roll < 0.7, no `found_khosani`: `rand-hills01` script `_v3`; else new roll < 0.33 `rand-hillsnite01` script `_v2`; < 0.66 script `_v3`; else `rand-hills01` |
| 4..8 | forest | < 0.25 `rand-forest01` script `_v3`; < 0.5 script `_v4`; < 0.75 and stage >= 7 `rand-forestnite1` script `_v3`; else `rand-forestnite1` |
| 4..8 | grass | Korel active and < 0.5: `rand-grassland01` script `_v3`; else first roll < 0.1 the same; < 0.55 `rand-grasslandnite01` script `rand-grasslandnite012`; else `rand-grasslandnite01` |
| 9..14 | desert | < 0.5 `rand-desert`, else `rand-desertnite01`; then suffix: < 0.35 `_v2`, else new roll < 0.65 `_v3`, else none |
| 9..14 | orenian | < 0.5 `rand-orenia01`, else `rand-orenianite01`; then suffix: < 0.45 `_v2`, else new roll < 0.55 `_v3`, else none |
| 15+ | any (checked first) | `nhuvasarim_mq_bought_page7`, no "Tome Page 8" item, no `got_laharah_spirit`: `rand-icelandnite01` script `_v2` |
| 15+ | hills | `khosanistrng_haenulquest`, no `extra_5`, no "Eraekor Gemstone": `rand-iceland01` script `_v4`, sets `extra_5` (once); else < 0.05 `rand-hills01` script `_v4`; < 0.16 `rand-iceland01_v2`; < 0.33 `rand-iceland01_v3`; < 0.5 `rand-iceland01`; < 0.75 `rand-icelandnite01_v3`; else `rand-icelandnite01` |
| 15+ | forest | Tome Pages 1-6 in the party, no `got_laharah_spirit`: `rand-forest01` script `_v6`; else first roll < 0.5: `rand-forest01` with second roll < 0.2 `_v5`, else `_v6`; else `rand-forestnite1` with second roll < 0.2 `_v3`, < 0.4 `_v4`, < 0.6 `_v5`, < 0.7 `_v6`, else `_v7` |
| 15+ | grass | < 0.1 with `ready_for_end` and `got_laharah_spirit`: `rand-grassland01` script `_v5`; < 0.5 and no `extra_1`: `rand-grassland01` script `_v4`, sets `extra_1` (once); else `rand-grasslandnite01` script `rand-grasslandnite013` |

Notes:

- **Observed:** "nite" is a coin flip. No clock or time of day is read.
- **Observed:** at stage 9..14 the hills/forest/grass navpoints are not checked. At 15+ "hills"
  gives iceland maps, because there is no `$iceland` search.
- **Observed** (a bug in the stage 4..8 grass branch): the Zane case (`0x4fc9d8`) copies
  `rand-grassland01_v3` and then falls through to the general rolls at `0x4fca22`, which overwrite it.
  Seen in game (2026-10-17): with Zane active 4 of 40 picks were his encounter, the general 10 %. Game bug G6;
  SummonerFix restores his 50 % (`[Fixes] ZaneEncounter`, 19 of 40 in the same test).
- *Unknown:* what each `_vN` variant spawns. The `ls_rand_*` handlers (`ls_rand_hills01_frame` and
  others) run the ring-boss fights (`PHNX-Intro`, `Serpent-Intro`, `Ghost-Intro` cutscenes).

### Leaving an encounter

{: .important }
Corrected 2026-10-15: the compare length is **4**, not 5 (`push ebx` with `ebx = 4`, `0x4cad56`), so the
skip applies to every level name that starts with `rand` (case-insensitive). All retail names that start
that way are the `rand-*` encounter maps, so the effect in the retail game is the same. The skip also
applies in towns and on the world map.

**Observed:** once an exit trigger is crossed, `level_check_triggers` clears
`level_exit_block_flags` (`0x2491a50`). It leaves it at 0 and goes straight on when
`level_is_town` (`0x60a40c`) is set, when `cur_level_index` is 4 (the world map), or when
`_strnicmp(level_filename, "rand-", 4)` is 0:

```
4cad37: mov al, byte ptr [0x60a40c]      ; level_is_town
4cad44: jne 0x4caeb7
4cad4a: cmp dword ptr [0x59f120], ebx    ; cur_level_index == 4
4cad50: je 0x4caeb7
4cad56: push ebx                         ; 4
4cad57: push 0x59e224                    ; "rand-"
4cad5c: push 0x60a2c8                    ; level_filename
4cad61: call 0x570140                    ; _strnicmp
```

In other levels it sets two bits:

- bit `0x1` when a party member other than the leader is more than 10 units from the leader
  (squared distance > 100, `0x57e6b4`). Summons (flag `0x100`) and members with flag `0x80` are
  not counted;
- bit `0x2` when a living entity of type 5 or 9 in AI mode 3 (combat) targets a party member
  (`+0x384`) and is within 20 units of it (squared distance <= 400, `0x58192c`). The search stops
  at the first one.

`hud_confirm_level_exit` (`0x40d640`, from `hud_frame`) then shows "Cannot Leave" with "Hostile
monsters nearby" (bit `0x2` first) or "Party members too far away" (bit `0x1`), and its OK button steps
the leader back to `level_exit_return_pos` (`0x2493a08`). *Inferred:* you can walk out of any encounter
level at once, even mid-fight.

## For players

- **Turn encounters off:** run the console command `random_encounters` (no argument toggles it,
  `0` turns it off). The retail game has no console window, so bind it to a key with
  [SummonerFix](../summonerfix/index.md) `[ConsoleKeys]`, for example `F7=random_encounters`. See
  [Console commands](console.md). *Inferred:* the value is not saved and resets to on when the game
  restarts (static default 1).
- **Fewer encounters:** encounters only roll while you move. Walk straight to the location. A
  check fails when you are standing still, but the 10 s timer still restarts.
- **Force an encounter:** keep walking; about half the checks hit. Rings of light, fire and stone
  force the boss encounter until its intro has played.
- **Pick the terrain:** stay near the `$forest`, `$hills` or `$grass` markers of the map to get that
  kind of encounter.
- **Escape:** walk to the encounter map's exit. Hostile monsters do not block it.
- **Markers:** press `L` on the world map to show or hide the location arrows.

## Evidence

Function and data addresses (no REA evidence ids).

| Claim | Where |
|---|---|
| 20 s first timeout, music, speed, party hidden | `ls_worldmap1_init` `0x4fb61c`, `0x4fb643`, `0x4fb673` |
| position restore list of 12 `Rand-*` names | `0x4fb526` loop over `0x5a4c18`, count `0x5a4c48` |
| 10 s reset, moving, `Ai_paused`, 50% roll, ring forcing | `ls_worldmap1_roll_encounter` `0x4fbef0`..`0x4fbfc8` |
| stage bands 4 / 9 / 15 | `ls_worldmap1_frame` `0x4fbe69`..`0x4fbe9c` |
| encounter table | the four pickers `0x4fbfd0`, `0x4fc4b0`, `0x4fcaf0`, `0x4fce40`; floats `0x57e6b0` 0.5, `0x5819bc` 0.45, `0x581948` 0.55, `0x58020c` 0.7, `0x581c08` 0.33, `0x581b24` 0.66, `0x580e00` 0.25, `0x580e04` 0.75, `0x580214` 0.1, `0x581cd8` 0.35, `0x581cd4` 0.65, `0x58021c` 0.05, `0x581cdc` 0.16, `0x57ef10` 0.2, `0x581ad8` 0.4, `0x5819a4` 0.6 |
| nearest navpoint returns a record pointer | `ls_worldmap1_nearest_hills_nav` `0x4fc33c` (`navpoints + 0x30*i`) |
| trigger bit `0x2` means "not an exit" | `level_trigger_set_script_use` `0x4cd038`, `level_check_triggers` `0x4cabbf`, `0x4cac4f` |
| default trigger flags, `+Multi` bit `0x4` | `level_parse_trigger` `0x4cc1d8`..`0x4cc1f9`; `level_check_triggers` `0x4caa79`..`0x4caa95` |
| `+Index:` becomes the entry point | `level_parse_trigger` `0x4cc331`..`0x4cc34f`; `level_check_triggers` `0x4cad26`..`0x4cad32`; `level_apply_pending_change` `0x4ccd9d`..`0x4ccda6` |
| exits in towns, on the world map and in `rand*` levels skip hostile/party checks | `level_check_triggers` `0x4cad37`..`0x4cad6b` (length 4 at `0x4cad56`), checks `0x4cad71`..`0x4cae97` |
| "Cannot Leave" messages | `hud_confirm_level_exit` `0x40d696`..`0x40d703` |
| location markers and visibility | `worldmap_init_locations` `0x42f3b0`, tables `0x592cb4` / `0x592cd4`, jump table `0x42f4d4` |
| `L` toggle, no click handling | `worldmap_frame` `0x42f51c` |
| console toggle | `cmd_random_encounters` `0x4fd630` |

## Changes (2026-10-15, level_scripts_common.cpp)

Checked against the disassembly while writing `src/decomp/game/level_scripts_common.cpp`
(`NOTES-level-scripts-common.md`).

- Leaving an encounter: the `_strnicmp` length is 4, not 5, so any level name starting with `rand`
  skips the checks. The skip also covers towns and the world map. Added the members that are not
  counted, the "first hostile" stop and the two "Cannot Leave" messages.
- Locations: the default trigger flags, the `+Multi` bit `0x4`, the second `test [esi-4], 2` site and
  how `+Index:` becomes `level_entry_point`.
