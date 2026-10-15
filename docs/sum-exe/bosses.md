---
title: "Bosses"
parent: Sum.exe
nav_order: 6.28
---

# Bosses

**Summary:** 14 boss species have their own AI code in `0x495290`-`0x4ca600`. `ai_reset` (`0x472450`) calls
`boss_ai_load` (`0x4a0600`) for every type-9 entity and for Luminar. Each boss then runs through a set of
species dispatchers: load, stage check, idle, combat think, attack test and resolve, unload. Most bosses have
three **stages**. The stage usually comes from the HP fraction (thresholds 66 % and 30 %, or 2/3 and 1/3).
Some stage changes wait for a level-script cutscene, and every stage can be forced from the console. All of
this is **observed** in the disassembly unless marked otherwise. Nothing here was checked in a running game.

{: .important }
**Corrected 2026-10-15.** The `+Plus Spell:` hit goes to the boss's target, not to its attacker. The table's
offense point is parsed as a number, so the retail tables (which give tag names) leave every move on point -1.
Boss heights are computed and then thrown away. See "Changes (2026-10-15)" at the end.

See [AI](ai.md) for `ai_state`, modes and goals, [Combat](combat.md) for damage,
[Characters](characters.md) for the `Character` fields, [Level scripts](level-scripts.md) for cutscenes and
story flags, and [Console](console.md) for the full command list.

## Which entities are bosses

- `script_parse_objects` (`0x4dba30`) gives a level character type 9 when it has `+Boss`, else type 4.
  It also forces type 9 for any species where `species_is_boss` (`0x4a14f0`) returns 1. **Observed.**
- `ai_reset` tests `Character +0xc == 9`, or species (`char_info +0x10`) `0x51` (Luminar). **Observed.**
- `boss_ai_load` switches on the species (`0x51`..`0xc9`). Species without a case do nothing. So a `+Boss`
  character such as the Ikaemos Priest King probably uses the normal AI plus its level script. *Inferred.*
- Type 9 also changes generic code: `character_play_flinch` (`0x45de20`) returns at once for type 9, so
  bosses never play the normal flinch move. **Observed.**
- The radius of a type-9 character comes from `charinfo_boss_radius` (`0x4a16a0`), through
  `character_size_radius` (`0x4542b0`) and `character_distance_between_edges`. **Observed.**
- The height does not. `character_size_height` (`0x4542f0`) calls `charinfo_boss_height` (`0x4a15a0`) for
  type 9, drops the result (`fstp st(0)` at `0x45430f`) and returns the size-class table
  (`0x5818d8`: 0.9, 1.8, 2.5). **Observed.** The unused heights: Luminar 5.5, Giant Salamanka 4, Tentacle
  Beast 15, Titus and Pyrul 3, Evil Urath 8, Machival 5; the riders, Murod, Murod Oni and Evil Joseph use the
  size table anyway. So every boss uses its size-class height for picking, line of sight, overlap, health
  bars and floating text. See [game bugs](game-bugs.md).
- `ai_retaliate_on_hit` (`0x4b5790`) returns at once for a type-9 defender whose species passes
  `species_is_boss_b` (`0x4a24c0`). That list leaves out the Serpent Rider (`0x59`), Giant Salamanka (`0xac`)
  and Luminar (`0x51`). Luminar retaliates only when `luminar_can_switch_target` (`0x4ca5c0`) allows it.
  The Serpent Rider, Giant Salamanka and a `+Boss` character of any other species retaliate like
  normal characters. **Observed.**

```
45430a: call 0x4a15a0                       ; charinfo_boss_height
45430f: fstp st(0)                          ; result dropped
454314: mov eax, dword ptr [ebp - 0x4]
454317: mov ecx, dword ptr [eax + 0x71c]
45431d: mov edx, dword ptr [ecx + 0x90]     ; size class
454323: fld dword ptr [4*edx + 0x5818d8]    ; size-class height
```

```
472457: mov eax, dword ptr [esi + 0x4c]     ; owner Character
47245a: cmp dword ptr [eax + 0xc], 0x9      ; type 9 = boss
47245e: je  0x47246c
472460: mov eax, dword ptr [eax + 0x71c]    ; char_info
472466: cmp dword ptr [eax + 0x10], 0x51    ; species Luminar
47246a: jne 0x472473
47246e: call 0x4a0600                       ; boss_ai_load
```

## Boss table files

Nine loaders read a `*_boss.tbl` with `boss_ai_parse_table` (`0x4a1790`, source `boss_ai.cpp`):
`serp_boss.tbl`, `phoenix_boss.tbl`, `ghost_rider_boss.tbl`, `pyrul_boss.tbl`, `gsal_boss.tbl`,
`titus_boss.tbl`, `machival_boss.tbl`, `darkjo_boss.tbl` and `murod_oni_boss.tbl`. **Observed.**
Tiger, Murod, Evil Urath, Luminar and the Tentacle Beast have no table. Their moves are set in code.
`tables.vpp` also holds a `murod_boss.tbl`, but no loader names it (the string is not in `Sum.exe`).
**Observed.**

The format, from the parser (**observed**; the meaning of `$Variation:` is *inferred*):

```
#Boss Info
$Num Stages: <int>                    ; read and dropped
$Stage: <int>                         ; 1-based index into the boss's table, not bounds-checked
  $Offense point: "<string>"          ; every record starts here; stored as atoi(string) - 1
  $Range: <float> <float>             ; min, max
  $Max angle: <float>                 ; degrees, stored as cos(angle x 0.0174533)
  $Variation: "<name>"                ; index into variation_names (0x595cf8, 0xc2 names)
  [+Debug: "<string>"]                ; read and dropped
  [+Trip:] [+Push:]                   ; flags 1, 2
  [+Spell: "<name>"                   ; spell_find_by_name (0x47a7c0), else -1
     [+Projectile:] [+Spell Only:] [+Plus Spell:]]   ; flags 4, 8, 0x10, only after +Spell:
  $Offense point: ...                 ; next record of the same stage
$Stage: ...
#End
```

- Keywords are case-insensitive (`TextFile__Accept`); the retail tables mix `$Offense point:` and
  `$offense point:`. **Observed.**
- Each record is 0x20 bytes: offense point `+0x00`, range `+0x04` / `+0x08`, cosine `+0x0c`, "used" 1 at
  `+0x10`, spell `+0x14`, variation `+0x18`, flags `+0x1c`. A stage is 10 records (0x140 bytes; the record
  address is `table + 0x140 x (stage - 1)`). The parser's second and fourth arguments (stage count and offense
  point count) are not read, and nothing stops an eleventh record. The largest retail stage has 7 records
  (Giant Salamanka stage 1, Phoenix stage 3). **Observed.**
- `+Trip:` and `+Push:` become flags 1 and 2. For Giant Salamanka, flag 1 gives a knock-down for 3-7 s
  (`giant_salamanka_move_knocks_down`, `0x4c8c20`) and flag 2 a push/disable (`giant_salamanka_move_pushes`,
  `0x4c8c00`). **Observed.**
- The loaders call `offense_point_name` (`0x4487f0`) for `"$offense01"`..`"$offense10"` and keep every tag the
  mesh has, packed, in a 0x1c-byte record array (tag `+0x00`, direction `+0x04`, world position `+0x10`). The
  count sits just before the array (Pyrul `0x247fdb4`, Ghost `0x2480334`, Machival `0x248b0b0`, Murod Oni
  `0x248c080`, Giant Salamanka `0x248c868`). Each frame the boss updates the positions from the bones
  (for example `pyrul_update_attack_points`, `0x4b9400`). **Observed.**
- A move names its point by number: `$Offense point:` is read as a string and stored as `atoi(string) - 1`
  (`0x4a18b7`..`0x4a18c4`). Every retail record gives a tag name instead (`"$offense01"`..`"$offense07"`,
  all 89 records of the 9 loaded tables), and `atoi("$offense02")` is 0. So every retail move has offense point **-1**.
  **Observed** in code and data.
- The point tests index the array with that number and no check: `pyrul_point_in_attack_cone` (`0x4b9c20`),
  `ghost_point_in_attack_cone` (`0x4bbb80`), `machival_offense_point_reaches` (`0x4c0dd0`),
  `murod_oni_offense_point_reaches` (`0x4c6230`), `giant_salamanka_target_in_reach` (`0x4c8c40`) and a reach
  test in `giant_salamanka_ai_select_action` (`0x4c8a32`). Point -1 is the 0x1c bytes before the array.
  Its "position" is a word nobody writes (or, for Pyrul, the fifth threat record) and the count, so the
  horizontal test is measured from about the world origin. Its "direction" is near zero, so a cone test
  passes only for moves whose max angle is 90 degrees or more. **Observed** for the reads; *inferred* for the
  effect in game. See [game bugs](game-bugs.md).

```
4a18a5: push edx                           ; buffer
4a18ad: call 0x5166b0                      ; TextFile__ParseString -> "$offense02"
4a18b7: call 0x56e311                      ; atoi -> 0
4a18bf: dec eax                            ; -1
4a18c4: mov dword ptr [esi], eax           ; record +0x00
...
4b9c29: mov eax, dword ptr [edi]           ; pyrul_point_in_attack_cone: record +0x00
4b9c2b: lea ecx, [8*eax]
4b9c32: sub ecx, eax                       ; x 7 dwords = 0x1c
4b9c3b: fsub dword ptr [4*ecx + 0x247fdd0] ; point z (+0x18)
```

## Dispatchers

Every per-boss function is reached through a species switch. The main ones:

| Dispatcher | Address | Called from | Purpose |
|---|---|---|---|
| `boss_ai_load` | `0x4a0600` | `ai_reset` | per-boss setup, table, weapons, teleport navpoints |
| `boss_ai_unload` | `0x4a04d0` | `ai_state_shutdown` | per-boss cleanup (frees the Tiger's axe while it is not in hand, keeping the pointer; bitmaps) |
| `boss_ai_check_stage` | `0x4a0730` | `ai_frame` (every frame, type 9) | threat decay, then the stage check and rider seating |
| `boss_ai_idle_dispatch` | `0x4a08c0` | `ai_frame`, idle mode | waits for the boss's "combat" flag, then `ai_enter_combat` |
| `boss_ai_frame` | `0x4a09f0` | `ai_mode_combat_frame` | the boss's combat think function |
| `boss_ai_do_frame_hook` | `0x4a2560` | `ai_do_frame` | grab handling for Luminar and the Tentacle Beast |
| `boss_target_filter_dispatch` | `0x4a2e00` | `boss_target_list_build` | per-boss flags on each candidate target |
| `boss_attack_test_dispatch` | `0x4a2920` | `living_combat_update_boss_attack` | who is in reach of the current move |
| `boss_attack_resolve_dispatch` | `0x4a2b80` | same | applies the hits found by the test |
| `boss_attack_trigger_dispatch` | `0x4a0ca0` | same | move trigger: missiles, spells, party knock-down |
| `boss_cast_special_dispatch` | `0x4a0b40` | same | spell part of a move |
| `boss_counter_hit_dispatch` | `0x4a2230` | `living_combat_resolve_hit` | `+Plus Spell:` spell on the boss's target (below) |
| `boss_move_in_range_dispatch` | `0x4a0ea0` | `living_combat_resolve_hit` | range test of a type-9 attacker's single-target hit |
| `boss_get_move_range` | `0x4a26f0` | `ai_combat_approach` and others | range of the current move |
| `boss_clear_current_move` | `0x4a2590` | combat update | ends the current move |
| `boss_get_threat_table` | `0x4a1200` | threat code | the boss's 5-entry threat table |

All three species switches above (`check_stage`, `idle`, `frame`) were decoded. Giant Salamanka's idle case
is `empty_function` (`0x4f13e0`, a bare `ret` shared by every empty body); its frame function checks the combat flag itself. Murod Oni has no stage
check (empty case). Luminar is not in `boss_ai_check_stage` (its switch starts at species `0x57`). **Observed.**
The unit `boss_ai.cpp` runs from `0x4a04b0` to `0x4a3575` (`level_preload_effects` is its last function).

## How a boss hit is resolved

`ai_mode_combat_frame` (`0x4bff60`) calls only `boss_ai_frame` for a type-9 character, so a boss acts only
through its own think function. **Observed.** A boss move then runs in `living_combat_update_boss_attack`
(`0x468360`):

1. When the move starts, `boss_attack_test_dispatch` collects the targets in reach. If it finds none (or the
   species has no test), the target's normal defence roll runs (`living_combat_roll_defense`, `0x4683fc`).
2. At the hit trigger, `boss_attack_resolve_dispatch` applies the collected hits. If it returns false, the
   normal single-target `living_combat_resolve_hit` (`0x467df0`) runs. **Observed.**
3. In `living_combat_resolve_hit`, `boss_counter_hit_dispatch(attacker, defender)` runs first (`0x467e93`),
   then, for a type-9 attacker, `boss_move_in_range_dispatch` (`0x467ea8`). Out of range, no hit. **Observed.**

Tests and resolvers exist for the Serpent (whip, special skill `0x53`), the Ghost (move class 4), Pyrul
(classes 0 and 2), Titus (kind 3), Murod Oni (kind 4), Machival (kinds 3 and 7) and the Tentacle Beast.
The Ghost's resolver returns false after its own hits, so the normal hit on the main target follows.
**Observed.** `boss_move_in_range_dispatch` has cases for every boss except Evil Urath (`0xb1`); Luminar and
Murod use the plain edge distance. Evil Urath never needs one: its select function only casts seven spells
(`0x55`-`0x5b`, `evil_urath_cast_spell` `0x4c43f0`) or leaps. **Observed.**

**`+Plus Spell:`** (flag `0x10`). `boss_counter_hit_dispatch` (`0x4a2230`) is called with the attacker first.
It acts only when that attacker is a type-9 Serpent Rider (`0x59`, the move of `serpent_part_busy` for that
part) or Ghost Rider (`0x5a`, `ghost_cur_move` `0x2480810`) and the move has flag `0x10`. It then hits the
**defender**, the boss's target: spell `0x54` (ghost death) through `living_entity_get_hit_by_spell`, any
other spell through `fx_create_spell_effect_at` with a 1 s timer. So it is the boss's own melee hit carrying
its spell, not a counter on someone who hits the boss. Retail uses it for the Serpent's stage-1 sickle
("poison") and the Ghost's stage-3 scythe ("ghost paralyzing fear") and claw ("ghost death"). The call
comes before the range test, so the spell lands even when the range test then drops the hit. **Observed.**

```
467e8a: mov edx, dword ptr [esi + 0x8]     ; defender
467e8d: mov ecx, dword ptr [esi]           ; attacker (the boss)
467e8f: mov eax, dword ptr [edx]
467e91: push eax
467e92: push ecx
467e93: call 0x4a2230                      ; boss_counter_hit_dispatch(attacker, defender)
...
467ea8: call 0x4a0ea0                      ; boss_move_in_range_dispatch
```

## Targets, threat and attack choice

- **Threat.** Each boss keeps 5 threat records of 0x18 bytes (handle, threat floats, a 20 s timer).
  `boss_threat_add` (`0x4a13b0`) adds damage threat from `character_add_hp` and
  `living_entity_get_hit_by_spell`, but only for an attacker of character type 7 (`cmp [esi+0xc], 7` at
  `0x4a13de`); its arguments are (boss, amount, attacker, flag). `boss_threat_decay` (`0x4a1310`) decays it
  every frame. **Observed.**
- **Target list.** `boss_target_list_build` (`0x4a2f60`) scores `cur_character` and the party with
  `ai_score_target`, adds threat and edge distance, and calls the boss's filter. Ten think functions use it.
  `boss_target_list_pick` (`0x4a31c0`) takes the best score or makes a weighted random choice. **Observed.**
- **Filters** set bit flags in the candidate record `+0xc`, for example "is attacking", "is attacking the
  boss", "in reach of the current move". Ghost Rider skips paralysed (status 10) or silenced (status 1)
  targets for some moves (`ghost_target_flags`, `0x4bba20`). **Observed.**
- **Attack choice.** Each think function uses GameTimers with random spans, the target flags and a random
  weighted choice (`rand_weighted_index` or `frand` against a constant, often 0.7). Titus, Machival, Evil
  Joseph, Evil Urath, Murod, Murod Oni, Giant Salamanka and the Tentacle Beast use a `*_ai_select_action`
  function that returns an action code for their frame function. **Observed.** The weights are constants
  in the code, not table fields. For example Evil Urath gives each of its seven spells 0.143 and changes some to 0.1,
  0.2, 0.86 or 1.0 by the target flags; when `rand_weighted_index` fails it falls back to `rand() % 6`, which
  never picks the seventh spell (`0x4c5064`..`0x4c5071`). **Observed.**
- **Combat style against riders.** `ai_get_combat_style` (`0x473620`) turns a character's style 0 (Melee) into
  2 (Range) while its target is a mounted rider, by `boss_species_mounted_stage` (`0x4a2480`). That function
  tests `tiger_stage` 1 or 2 for the Tiger, but `serpent_stage` (`0x2451d98`) == 1 for **both** the Serpent and
  the Phoenix, and returns false for the Ghost Rider. **Observed.** See [game bugs](game-bugs.md).
- `character_target_point_count` (`0x4a21b0`) returns `char_info +0x188` but 0 for a rider **on foot**
  (`tiger_stage` 3, or the other riders' stage above 1). **Observed.**

## Stage machines

The common template (Serpent, Phoenix, Tiger, Ghost, Pyrul, Titus, Giant Salamanka) is:

1. If the forced stage variable is not 0, switch to it once and then keep it. HP is ignored while a stage
   is forced. Values outside 1-3 reset the forced stage to 0.
2. Otherwise compute `HP (+0x5f8) / character_get_max_stat(0)` and compare it with two thresholds.
3. For the rider bosses, the middle stage waits for the level script: the boss sets a "reached" byte, the
   script plays a Dismount (or Trans) cutscene, and the cutscene end callback sets a "trigger" byte.

```
495c2a: call 0x462840                    ; character_get_max_stat
495c33: fild dword ptr [esp + 0x8]
495c37: fdivr dword ptr [esi + 0x5f8]    ; HP / max
495c3d: fcom dword ptr [0x580e00]        ; 0.25
...
495c74: fcomp dword ptr [0x57e6b0]       ; 0.5
...
495c8f: mov al, byte ptr [0x2451da0]     ; serpent_dismount_stage_reached ("stage 2 reached")
495c99: mov byte ptr [0x2451da0], 0x1
495ca0: mov byte ptr [0x2451da1], 0x0    ; serpent_stage_trigger, set by the Dismount cutscene
```

| Boss (species) | Stage 1 | Stage 2 | Stage 3 | Gate |
|---|---|---|---|---|
| Serpent Rider (`0x59`) | HP >= 50 %, mounted (`serpent_spawn_rider`) | HP < 50 %, on foot (`serpent_setup_anims`) | HP < 25 %, illusion parts off (`serpent_deactivate_parts`) | stage 2 waits for Serpent-Dismount |
| Phoenix Rider (`0x58`) | HP >= 66 %, mounted | 30-66 %, on foot | HP < 30 % | stage 2 waits for PHNX-Dismount |
| Tiger Rider (`0x57`) | mounted, TigerSpear | mounted, TigerAxe (`tiger_swap_weapon`) | HP < 50 %, on foot | stage 3 waits for Tiger-Dismount and the current move to finish |
| Ghost Rider (`0x5a`) | HP >= 66 %, mounted | 30-66 %, on foot | HP < 30 %, unmounted variant | stage 2 waits for Ghost-Dismount |
| Pyrul (`0xc4`) | HP >= 66 % | 30-66 %, timer A reset | HP < 30 %, timer B reset | none |
| Titus (`0xc1`) | HP >= 66 % | 30-66 %, `titus_reset_attack_timers` | HP < 30 %, `titus_reset_stage_timers` | none |
| Giant Salamanka (`0xac`) | HP >= 66.67 %, navpoint `$boss1` | < 66.67 %, `$boss2`, anim `walk2` | < 33.33 %, `$boss3`, anim `walk3` | each step waits for Trans1 / Trans2 |
| Luminar (`0x51`) | HP >= 66.67 % | < 66.67 %, grab attack | < 33.33 %, teleports | none |

Notes, all **observed** unless marked:

- **Tiger stage 2** is not HP based. `tiger_equip_weapon` (`0x49a2c0`) sets a counter at `0x2452ab8` to 4,
  and `tiger_move_done_hook` (`0x49c670`) decrements it after each use of move index 0. At 0 (and HP still
  above 50 %) the tiger swaps the spear for the axe. That move 0 is the spear throw is *inferred*.
- **Phoenix second life.** When the phoenix's HP reaches 0 for the first time, `phoenix_stage_transition_move`
  (`0x4995b0`) plays move (9, 1), then sets HP to 50 % of max, stage 3 and `phoenix_flags` bit `0x10`.
- **All four riders** start at forced stage 3 in `Liangshan` and `Jadetemple`, and elsewhere at
  stage 1 mounted (`serpent_boss_load` `0x495960`, `phoenix_boss_load` `0x498420`, `tiger_boss_load`
  `0x49ad20`, `ghost_rider_boss_load` `0x4ba260`). The level scripts take them off their mounts
  (`*_set_solo`): `ls_liangshan_init` for the Serpent and Phoenix Riders, `ls_jadetemple_init` for all four
  in `jadetemple_v2`, and in the first Jade Temple visit only the end of Tiger-Ghost-Intro
  (`ls_jadetemple_tiger_ghost_intro_cs_end` `0x4f0320`). Until that cutscene ends, the Tiger Rider is at stage 3
  but still mounted. The Ghost Rider is not: in `Liangshan` and `Jadetemple` `ghost_rider_boss_load` calls
  `ghost_setup_anims_unmounted` (`0x4ba2f2`), whose `ghost_setup_anims` releases the mount (`boss_release_mount`,
  `0x4ba850`), and clears the mounted flag `0x248031b` (`0x4ba2fa`) (**observed**).
  `cutscene_play_escape` (`0x437130`) also forces stage 3 for Tiger-Escape, Serpent-Escape and PHNX-Escape, but only for
  a cutscene started from the console (`play_and_load_cutscene`); see [Cutscenes](cutscenes.md#console-cutscenes-and-pending-playback).
- **Giant Salamanka** only moves 1 to 2 and 2 to 3, never skips. Each stage also has an 8-byte attack enable
  table (`giant_salamanka_stage1_attacks` `0x248cd98` and following) read by
  `boss_stage_attack_enabled_dispatch` (`0x4a1ac0`).
- **Luminar** computes its stage inside `luminar_ai_select_action` (`0x4c9410`), not in the check-stage
  dispatcher. The same HP fraction is written to `level_light_scale` (`0x59c358`), so the level gets darker
  as Luminar loses HP. *Inferred* for the visible effect.
- Giant Salamanka, Pyrul and Titus at HP 0 or below (`fcomp` with 0.0, `test ah, 0x41`) go idle and set owner
  flag 8, once (the test skips a boss that already has flag 8), but only when
  `game_flags` bit `0x20` is set (`pyrul_check_stage` `0x4b8c9b`..`0x4b8cd5`, `titus_check_stage` `0x4c25d4`..,
  `giant_salamanka_check_stage` `0x4c8670`..`0x4c869f`). Bit `0x20` means "cutscene running" ([Cutscenes](cutscenes.md)), so
  this is the death pose kept while the boss's Death cutscene plays. *Inferred* for the purpose.
- Bosses without HP stages: Machival, Evil Urath and Murod only clear their "casting" state when the cast
  move has finished. Murod Oni has no stage check. Evil Joseph's check revives helpers (below).
- Stage variables: `serpent_stage` `0x2451d98`, `phoenix_stage` `0x2452420`, `tiger_stage` `0x2452a90`,
  ghost `0x2480310` (forced `0x2480314`), pyrul `0x247fd28` (forced `0x247fd2c`), `titus_stage`
  `0x248b3a8`, `giant_salamanka_stage` `0x248c7d4`, `luminar_stage` `0x248cdb8`.

## Special mechanics

| Mechanic | Boss | Functions | Notes |
|---|---|---|---|
| Mount and rider | Serpent, Phoenix, Tiger, Ghost | `boss_ensure_mount` `0x4a1dc0`, `*_seat_rider`, `boss_release_mount` `0x4a1fe0` | every frame the rider is placed on the mount's `$sit` bone (or its origin), the mount gets the rider's target and the rider the mount's animation time (not the Phoenix Rider, which reads and writes the mount's own time: game bug K211); a released mount runs to the farthest graph point with goal `0xd` for 5 s **observed** |
| Illusion copies | Serpent | `serpent_spawn_rider` `0x4955e0`, `serpent_illusion_start` `0x496ba0`, `serpent_part_on_hit` `0x4a28b0` | two extra species-`0x59` type-9 copies, made hidden and inactive. The illusion (sub-modes `0x38` / `0x39`: `Serp-illusion.vfx`, the serpent fades out, all three reappear at points 4 m around it, the copies vanish 8 s later or when hit) never runs in v1.40: `serpent_ai_frame` enters `0x38` only when the pending sub-mode word `0x24523a8` is `0x38`, and the only write stores `0x20` (`0x4965ec`). `serp_boss.tbl` still lists "stage2 illusion" **observed**; see game bug K213 |
| Whip | Serpent | `serpent_whip_test` `0x497890`, `serpent_whip_resolve_hit` `0x4979b0` | hits a party member in whip reach **observed** |
| `+Plus Spell:` hit | Serpent parts, Ghost | `boss_counter_hit_dispatch` `0x4a2230` | a boss melee move with flag `0x10` also hits its target with the move's spell, before the range test (see above) **observed** |
| Minion revive | Ghost Rider | `ghost_revive_minions` `0x4bbc70`, `ghost_fire_attack` `0x4bb9a0`, `ghost_choose_attack` `0x4bac20` | spells `0x51`-`0x53` raise a counter (`0x24808c8`); the Ghost chooses a summon only while it is below 2 (`ghost_choose_attack` `0x4bad95`, `0x4baec1`). Every check-stage call (each frame) brings back every dead minion that is no longer active with `boss_resurrect_full` `0x4c3fb0` and decrements the counter for each, without testing that it is positive; when it falls from non-zero to exactly 0 the two summon timers are reset **observed** |
| Ghost minions | Ghost Rider | `ghost_rider_boss_load` `0x4ba260` | six handles `0x24808b0`-`0x24808c4`: two species `0x43` (not in `masad`), two `0x45` and two `0x9c`; none extra in `jadetemple` **observed** |
| Helper revive | Evil Joseph | `evil_joseph_spawn_helpers` `0x4c3500`, `evil_joseph_check_stage` `0x4c3f20` | four helpers (species `0xc0`, `0xca`, `0xcb`, `0xb0`); a dead helper is revived once its death effect ends, with no count limit **observed** |
| Teleport | Machival | `machival_teleport` `0x4c1e20` | random `$tele_0N` navpoint, 70 s timer (2 s retry), synced with packet `0x1F` **observed** |
| Teleport | Evil Joseph | `evil_joseph_find_teleports` `0x4c3720` | `$tele_0N` navpoints **observed** |
| Leap | Evil Urath | `evil_urath_ai_leap_start` `0x4c4810` .. `_land` `0x4c4ca0` | sub-modes `0x23`-`0x26`: rise, fly to a `$balc01_0N` / `$temple_0N` / `$balc02_0N` navpoint, land **observed** |
| Leap | Murod | `murod_start_leap` `0x4c51a0`, `murod_ai_leap` `0x4c53c0` | jumps to a spot near a random party member **observed** |
| Self heal | Murod | `murod_try_self_heal` `0x4c5570` | every 20 s: cure (spell 1) against `murod_cure_statuses`, or heal (spell 0) after HP loss **observed** |
| Rock erupt | Titus | `titus_ai_rock_erupt` `0x4c3240`, `titus_rock_erupt_damage` `0x4c3340` | `Titus-RockErrupt.VFX`, random HP loss in a radius **observed** |
| Area knock-down | Titus, Machival | `titus_knock_down_party` `0x4c2ef0`, `machival_knock_down_party` `0x4c1a80` | `ai_knock_down` for 2-3 s **observed** |
| Grab | Luminar, Tentacle Beast | `luminar_ai_grab` `0x4c9b00`, `tentacle_beast_ai_grab` `0x4c6850` | Luminar's grab deals 96 HP; the victim is held, leadership moves off it, the camera follows **observed** |
| Beam and teleport | Luminar | `luminar_beam_start` `0x4ca230`, `luminar_teleport_out` `0x4c9da0` | beam in every stage, teleport only in stage 3 **observed** |
| Tentacles | Tentacle Beast | `tentacle_beast_cycle_tentacles` `0x4c76d0`, `tentacle_beast_check_stage` `0x4c7360` | four tentacle records; each rises and sinks on timers; a tentacle at HP 0 plays its death move and counts as lost **observed** |
| Rider flinch | Serpent, Phoenix, Tiger, Giant Salamanka | `serpent_rider_set_flinch` `0x4954c0`, `phoenix_rider_set_flinch` `0x498720`, `tiger_rider_set_flinch` `0x49a1a0` and others | files the flinch moves whose `$Action:` is `stage<N>_flinch_area` / `_status` into per-stage tables (Serpent `0x24523c0` / `0x24523cc`, Phoenix `0x24529dc` / `0x24529e8`, Tiger `0x2452c4c` / `0x2452c58`); the Serpent and Phoenix then copy stage 2 into stage 3, the Tiger copies stage 2 into stage 1. No other code references the riders' tables, so they are never used **observed** (Giant Salamanka not rechecked); other `stage<N>_flinch_*` names give a message that is never shown (game bug V211) |

## Console commands

All are in [Console](console.md). The stage commands take an integer and only set the forced-stage variable;
the next `boss_ai_check_stage` applies it. **Observed.**

| Command | Address | Range | Sets |
|---|---|---|---|
| `serpent_stage` / `phoenix_stage` / `tiger_stage` / `ghost_stage` | `0x495340` / `0x497af0` / `0x499ec0` / `0x4b9ea0` | 0-3 | forced stage (0 = back to HP) |
| `pyrul_stage` / `titus_stage` | `0x4b8850` / `0x4c2180` | 0-3 | forced stage |
| `gs_stage` | `0x4c80a0` | 1-3 | `giant_salamanka_forced_stage` |
| `stage` | `0x4c8d90` | 1-3 | `luminar_forced_stage` |
| `*_combat` (serpent, phoenix, tiger, ghost, pyrul, titus, machival, darkjo, urath, gsal) | | | the boss's combat-enabled flag |
| `*_solo`, `*_unlink` (serpent, phoenix, tiger, ghost) | | | dismount: unlink from the mount, stage 3 (`serpent_set_solo` `0x497a40`) |
| `tentacles` / `tentacle_test` | `0x4c7cc0` / `0x4c7e90` | | start the tentacle battle / cycle tentacles without attacks |

`gs_stage` and `stage` accept 1-3 only, so they cannot hand the stage back to HP. *Inferred* from the range
check; not tested.

## Level scripts and cutscenes

A boss does not fight until its combat flag is set. The idle handler only calls `ai_enter_combat` once that
flag is 1, and the Intro cutscene end callback sets it. **Observed.** See [Level scripts](level-scripts.md).

| Boss | Level script | Cutscenes | Script hooks |
|---|---|---|---|
| Serpent Rider | `rand-forest01` (`ls_rand_forest01_frame` `0x4f5b90`) | Serpent-Intro, Serpent-Dismount, Serpent-Escape | Dismount when `serpent_dismount_stage_reached` is 1; Escape when the rider's HP <= 0 (sets `serpent_killed`) |
| Phoenix Rider | `rand-hills01` | PHNX-Intro, PHNX-Dismount, PHNX-Escape | same pattern (`phoenix_stage_trigger`) |
| Ghost Rider | `rand-forestnite1` | Ghost-Intro, Ghost-Dismount, Ghost-Escape | same pattern (`0x2480319`) |
| Tiger Rider | `templeint` | Tiger-Intro, Tiger-Dismount, Tiger-Escape | Escape end plays Act2-Outro once `act2_done` |
| Serpent + Phoenix | `Liangshan` | SERP-PHNX-Intro | riders start on foot (stage 3) |
| Tiger + Ghost | `jadetemple` (`lsf_tg_intro` `0x4f0160`) | Tiger-Ghost-Intro | end callback calls `tiger_set_solo` and `ghost_set_solo` |
| Murod, Murod Oni | `jadetemple` (`lsf_murod_intro` `0x4f0680`) | Murod-Intro, Murod-Trans, Murod-Death | Trans when Murod is dead: Murod Oni takes over; Death gives the Ring of Jade |
| Pyrul | `khosanilab2` | Pyrul-Intro, Pyrul-Death | combat flag `0x247fd38` |
| Giant Salamanka | `khosanilab` (`ls_khosanilab_salamanka_fight` `0x4f92a0`) | GiantSalamanka-Intro, -Trans1, -Trans2, -Death | flags `salstart`, `sal_trans1`, `sal_trans2`, `sal_killed`; Trans1/Trans2 when stage 2/3 is reached, their end callbacks set the `_ack` bytes |
| Titus | `ikaemosbottomint2` | Titus-Intro, Titus-Death | `titus_combat_enabled` and a 30 s timer |
| Evil Urath, Evil Joseph, Machival | `templeint2` | Machival-intro, Machival-Trans1, Machival-Trans2, Machival-Death | Urath fights first; when Urath dies, Trans1 enables Evil Joseph; when Joseph dies, Trans2 enables Machival (`0x248b034`). Machival-Death's end callback (`0x4ef4d0`) takes Jekhar, Flece and Rosalind out of the party, ends every summon (`fx_end_owned`, `summon_return_lent_max_hp`, delete) and places the party at entry 2. Two endings follow, both ending in `goto_credits` (game sequence state 9): End1 (`$end2trig`, End1-EnterForge, End1-Montage) and End2 (the `lenele3a` exit, End2-LeaveTemple, End2-Montage); see [game bug G130](game-bugs.md) |
| Tentacle Beast | `sewerboss` | TB-Intro | end callback raises 4 tentacles and starts the battle |
| Luminar | `ionaext02` | Luminar-Intro, Luminar-Death | 30 s timer, then IonaExt2 music after the death |

So the four riders flee at 0 HP in their first fight and are fought again on foot later. *Inferred* from the
Escape cutscenes, the `*_killed` flags and the forced stage 3 in `Liangshan` / `Jadetemple`.
The Salamanka intro callback also gives the party the Ring of Fire (`script_party_acquire_item`).
**Observed**; why it is given there is *unknown*.

## Overview

| Boss | Species | Level(s) | Stages | Code range | Stage trigger |
|---|---|---|---|---|---|
| Serpent Rider | `0x59` | rand-forest01, Liangshan | 3 | `0x495290`-`0x497ac0` | HP 50 / 25 %, Dismount cutscene |
| Phoenix Rider | `0x58` | rand-hills01, Liangshan | 3 | `0x497ac0`-`0x499e50` | HP 66 / 30 %, Dismount, second life at 0 |
| Tiger Rider | `0x57` | templeint, jadetemple | 3 | `0x499e50`-`0x49c840` | 4 spear moves, then HP 50 % and Dismount |
| Ghost Rider | `0x5a` | rand-forestnite1, jadetemple | 3 | `0x4b9e70`-`0x4bbe50` | HP 66 / 30 %, Dismount |
| Pyrul | `0xc4` | khosanilab2 | 3 | `0x4b87c0`-`0x4b9e70` | HP 66 / 30 % |
| Machival | `0xc5` | templeint2 | none | `0x4c0c40`-`0x4c20e0` | script only |
| Titus | `0xc1` | ikaemosbottomint2 | 3 | `0x4c20e0`-`0x4c3470` | HP 66 / 30 % |
| Evil Joseph | `0xc9` | templeint2 | none | `0x4c3470`-`0x4c4170` | helpers revive |
| Evil Urath | `0xb1` | templeint2 | none | `0x4c4170`-`0x4c5140` | leaps between navpoints |
| Murod | `0xb7` | jadetemple | none | `0x4c5140`-`0x4c5c20` | dies, then Murod Oni |
| Murod Oni | `0xb8` | jadetemple | none | `0x4c5c20`-`0x4c6480` | none |
| Tentacle Beast | `0x93` | sewerboss | per tentacle | `0x4c6480`-`0x4c8030` | tentacle timers |
| Giant Salamanka | `0xac` | khosanilab | 3 | `0x4c8030`-`0x4c8d10` | HP 2/3 and 1/3, Trans1 / Trans2 |
| Luminar | `0x51` | ionaext02 | 3 | `0x4c8d10`-`0x4ca600` | HP 2/3 and 1/3 |

The code ranges run from each boss's first static initialiser to the next boss's; shared helpers sit in
`0x4a04b0`-`0x4a3575`. Level names come from the level-script function names. *Inferred* for the exact
range ends.

## Open questions

- What each `$Variation:` name selects (a move variation of the boss's animation set is *inferred*).
- Whether the bosses that rely on offense points (Pyrul, Ghost, Machival, Murod Oni, Giant Salamanka) really
  miss in game, or whether some arena lies near the world origin. A runtime check in agent mode would settle it.
- Whether the Ghost's class-4 moves hit the main target twice (its own resolver, then `living_combat_resolve_hit`).
- Whether anything reaches the riders' per-stage flinch tables through a pointer; no direct reference outside the three `*_rider_set_flinch` functions was found (2026-10-15).
- The names of the ghost minion species `0x43`, `0x45`, `0x9c` and the Evil Joseph helpers.
- Whether the Phoenix second life and the PHNX-Escape cutscene both happen at HP 0, and in which order.

## Evidence

This page lists function addresses instead of REA evidence ids. All were read with `tools/asm.mjs fn` or
`callers` (llvm-objdump), and the floats with a byte read of `Sum.exe`.

| Claim | Where |
|---|---|
| boss AI for type 9 or species `0x51` | `ai_reset` `0x472450` |
| species switch of loaders | `boss_ai_load` `0x4a0600` |
| stage dispatch table (13 cases, Murod Oni empty) | `boss_ai_check_stage` `0x4a0730`, tables `0x4a080c` / `0x4a0844` |
| check-stage runs each frame for type 9 | `ai_frame` `0x472bd4` |
| table format keywords | `boss_ai_parse_table` `0x4a1790` |
| offense point = `atoi - 1`, stage stride 0x140, args 2 and 4 unused | `boss_ai_parse_table` `0x4a1889`..`0x4a18c4` |
| offense tags packed, count before the array | `pyrul_boss_load` `0x4b8a5a`..`0x4b8a9f`; same in the eight other loaders |
| index -1 used unchecked | `pyrul_point_in_attack_cone` `0x4b9c29`, `ghost_point_in_attack_cone` `0x4bbb89`, `machival_offense_point_reaches` `0x4c0dd9`, `murod_oni_offense_point_reaches` `0x4c6239`, `giant_salamanka_target_in_reach` `0x4c8c4c` |
| retail tables give tag names | `tables.vpp`: the nine loaded `*_boss.tbl` files (`$offense01`..`$offense07`) |
| boss height dropped | `character_size_height` `0x45430a`..`0x454323`; heights `charinfo_boss_height` `0x4a15a0`, table `0x4a1620` |
| retaliate exceptions | `ai_retaliate_on_hit` `0x4b579d`..`0x4b57e2`; `species_is_boss_b` table `0x4a24ec` |
| `+Plus Spell:` on the defender, before the range test | `living_combat_resolve_hit` `0x467e8a`..`0x467ea8`; `boss_counter_hit_dispatch` `0x4a2231`..`0x4a22ea` |
| hit flow | `living_combat_update_boss_attack` `0x4683ea`..`0x4683fc`, `0x4684f7`..`0x468509`; `boss_attack_resolve_dispatch` table `0x4a2cc4` |
| no Urath range case, Urath only casts | `boss_move_in_range_dispatch` table `0x4a100c`; `evil_urath_ai_select_action` `0x4c5050`..`0x4c50cb` |
| mounted test | `boss_species_mounted_stage` `0x4a2480`..`0x4a24b9`; `ai_get_combat_style` `0x473644` |
| threat only from type 7 | `boss_threat_add` `0x4a13de` |
| serpent thresholds 0.5 / 0.25 and Dismount gate | `serpent_check_stage` `0x495b90`, floats `0x57e6b0`, `0x580e00` |
| phoenix thresholds 0.66 / 0.3 and second life | `phoenix_check_stage` `0x499690`, `phoenix_stage_transition_move` `0x4995b0` |
| tiger spear counter and stage 3 at 50 % | `tiger_check_stage` `0x49a6c0`, `tiger_equip_weapon` `0x49a3a8`, `tiger_move_done_hook` `0x49c670` |
| ghost thresholds and Dismount gate | `ghost_update_stage` `0x4bb4c0` |
| pyrul and titus thresholds | `pyrul_check_stage` `0x4b8b90`, `titus_check_stage` `0x4c24c0` |
| salamanka thresholds 0.6667 / 0.3333 and acks | `giant_salamanka_check_stage` `0x4c8560`, floats `0x581c30`, `0x581c34` |
| Luminar stage, grab cooldown 30 s, teleport in stage 3 | `luminar_ai_select_action` `0x4c9410` |
| console ranges | `cmd_serpent_stage` `0x495340`, `cmd_tiger_stage` `0x499ec0`, `cmd_ghost_stage` `0x4b9ea0`, `cmd_gs_stage` `0x4c80a0`, `cmd_stage` `0x4c8d90` |
| Escape forces stage 3 (console path only) | `cutscene_play_escape` `0x437130` |
| `+Boss` gives type 9 | `script_parse_objects` `0x4dbc8d`..`0x4dbd4b` |
| bosses skip the flinch move | `character_play_flinch` `0x45de20` |
| minion and helper revive | `ghost_revive_minions` `0x4bbc70`, `ghost_fire_attack` `0x4bb9a0`, `evil_joseph_check_stage` `0x4c3f20` |
| Salamanka cutscene flow | `ls_khosanilab_salamanka_fight` `0x4f92a0` and its four callbacks `0x4f9520`-`0x4f9570` |
| templeint2 boss order | `ls_templeint2_machival_intro_cs_end` `0x4ef290`, `_trans1_cs_end` `0x4ef330`, `_trans2_cs_end` `0x4ef3c0`, `lsf_mach_killed` `0x4ef3f0` |
| Serpent-Escape at HP 0 | `ls_rand_forest01_frame` `0x4f5ce0`..`0x4f5d1b` |

## Changes (2026-10-15)

- `+Plus Spell:` ("counter hit"): the spell goes to the boss's target on the boss's own hit, and it lands
  before the range test. It is not a counter on the melee attacker.
- Table format: every record starts with `$Offense point:`, which is parsed with `atoi`. The retail tag names
  give point -1 for every move. `$Num Stages:` and `+Debug:` are dropped; `+Projectile:`, `+Spell Only:` and
  `+Plus Spell:` count only after `+Spell:`. `murod_boss.tbl` exists but is never loaded.
- Boss heights are never used; only the radius is. `ai_retaliate_on_hit` does not skip the Serpent Rider,
  Giant Salamanka or (fully) Luminar.
- New: the hit flow, the Urath spell list, the rider combat-style test (Phoenix reads `serpent_stage`), the
  type-7 threat rule and the unit end `0x4a3575`; from the decomp54 units, that the Serpent's illusion never
  runs, the rider flinch tables are never read, the Phoenix Rider is not kept in step with its horse, the Tiger
  code ends at `0x49c840` and the boss code starts at `0x495290`.
