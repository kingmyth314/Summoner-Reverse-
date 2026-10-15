---
title: "Bosses"
parent: Sum.exe
nav_order: 6.28
---

# Bosses

**Summary:** 14 boss species have their own AI code in `0x495000`-`0x4ca600`. `ai_reset` (`0x472450`) calls
`boss_ai_load` (`0x4a0600`) for every type-9 entity and for Luminar. Each boss then runs through a set of
species dispatchers: load, stage check, idle, combat think, attack test and resolve, unload. Most bosses have
three **stages**. The stage usually comes from the HP fraction (thresholds 66 % and 30 %, or 2/3 and 1/3).
Some stage changes wait for a level-script cutscene, and every stage can be forced from the console. All of
this is **observed** in the disassembly unless marked otherwise. Nothing here was checked in a running game.

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
  bosses never play the normal flinch move. Size and radius come from `charinfo_boss_height` (`0x4a15a0`) and
  `charinfo_boss_radius` (`0x4a16a0`). `ai_retaliate_on_hit` (`0x4b5790`) skips bosses. **Observed.**

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

The format, from the keywords the parser expects (**observed**; the meaning of each field is *inferred*):

```
#Boss Info
$Num Stages: <int>
$Stage: <int>
  $Offense point: <name> ...      ; optional, mesh tag of an attack bone
  <move>: $Range: <float> <float>  $Max angle: <float>  $Variation: <type string>
          [+Debug: <string>] [+Trip:] [+Push:] [+Spell: <name>]
          [+Projectile:] [+Spell Only:] [+Plus Spell:]
$Stage: ...
#End
```

- Each record is 0x20 bytes. Spell names go through `spell_find_by_name` (`0x47a7c0`).
- `+Trip:` and `+Push:` become move flags 1 and 2 (`+0x1c`). For Giant Salamanka these flags decide a
  knock-down for 3-7 s (`giant_salamanka_move_knocks_down`, `0x4c8c20`) or a push/disable
  (`giant_salamanka_move_pushes`, `0x4c8c00`). *Inferred* for the mapping, **observed** for the effect.
- Offense points are mesh tags named by `offense_point_name` (`0x4487f0`, `"$offense01"`..). Each frame the
  boss updates their world positions from bones, and an attack only hits targets inside that point's range
  and cone (for example `pyrul_point_in_attack_cone`, `0x4b9c20`). **Observed.**

## Dispatchers

Every per-boss function is reached through a species switch. The main ones:

| Dispatcher | Address | Called from | Purpose |
|---|---|---|---|
| `boss_ai_load` | `0x4a0600` | `ai_reset` | per-boss setup, table, weapons, teleport navpoints |
| `boss_ai_unload` | `0x4a04d0` | `ai_state_shutdown` | per-boss cleanup (frees the tiger weapons, bitmaps) |
| `boss_ai_check_stage` | `0x4a0730` | `ai_frame` (every frame, type 9) | threat decay, then the stage check and rider seating |
| `boss_ai_idle_dispatch` | `0x4a08c0` | `ai_frame`, idle mode | waits for the boss's "combat" flag, then `ai_enter_combat` |
| `boss_ai_frame` | `0x4a09f0` | `ai_mode_combat_frame` | the boss's combat think function |
| `boss_ai_do_frame_hook` | `0x4a2560` | `ai_do_frame` | grab handling for Luminar and the Tentacle Beast |
| `boss_target_filter_dispatch` | `0x4a2e00` | `boss_target_list_build` | per-boss flags on each candidate target |
| `boss_attack_test_dispatch` | `0x4a2920` | `living_combat_update_boss_attack` | who is in reach of the current move |
| `boss_attack_resolve_dispatch` | `0x4a2b80` | same | applies the hits found by the test |
| `boss_attack_trigger_dispatch` | `0x4a0ca0` | same | move trigger: missiles, spells, party knock-down |
| `boss_cast_special_dispatch` | `0x4a0b40` | same | spell part of a move |
| `boss_get_move_range` | `0x4a26f0` | `ai_combat_approach` and others | range of the current move |
| `boss_clear_current_move` | `0x4a2590` | combat update | ends the current move |
| `boss_get_threat_table` | `0x4a1200` | threat code | the boss's 5-entry threat table |

All three species switches above (`check_stage`, `idle`, `frame`) were decoded. Giant Salamanka's idle case
is `empty_function` (`0x4f13e0`, a bare `ret` shared by every empty body); its frame function checks the combat flag itself. Murod Oni has no stage
check (empty case). Luminar is not in `boss_ai_check_stage` (its switch starts at species `0x57`). **Observed.**

## Targets, threat and attack choice

- **Threat.** Each boss keeps 5 threat records of 0x18 bytes (handle, threat floats, a 20 s timer).
  `boss_threat_add` (`0x4a13b0`) adds damage threat from `character_add_hp` and
  `living_entity_get_hit_by_spell`; `boss_threat_decay` (`0x4a1310`) decays it every frame. **Observed.**
- **Target list.** `boss_target_list_build` (`0x4a2f60`) scores `cur_character` and the party with
  `ai_score_target`, adds threat and edge distance, and calls the boss's filter. Ten think functions use it.
  `boss_target_list_pick` (`0x4a31c0`) takes the best score or makes a weighted random choice. **Observed.**
- **Filters** set bit flags in the candidate record `+0xc`, for example "is attacking", "is attacking the
  boss", "in reach of the current move". Ghost Rider skips paralysed (status 10) or silenced (status 1)
  targets for some moves (`ghost_target_flags`, `0x4bba20`). **Observed.**
- **Attack choice.** Each think function uses GameTimers with random spans, the target flags and a random
  weighted choice (`rand_weighted_index` or `frand` against a constant, often 0.7). Titus, Machival, Evil
  Joseph, Evil Urath, Murod, Murod Oni, Giant Salamanka and the Tentacle Beast use a `*_ai_select_action`
  function that returns an action code for their frame function. **Observed**; the weights are in the tables
  and were not read.

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
- **All four riders** start at forced stage 3 (on foot) in `Liangshan` and `Jadetemple`, and elsewhere at
  stage 1 mounted (`serpent_boss_load` `0x495960`, `phoenix_boss_load` `0x498420`, `tiger_boss_load`
  `0x49ad20`, `ghost_rider_boss_load` `0x4ba260`).
  `cutscene_play_escape` (`0x437130`) also forces stage 3 for Tiger-Escape, Serpent-Escape and PHNX-Escape, but only for
  a cutscene started from the console (`play_and_load_cutscene`); see [Cutscenes](cutscenes.md#console-cutscenes-and-pending-playback).
- **Giant Salamanka** only moves 1 to 2 and 2 to 3, never skips. Each stage also has an 8-byte attack enable
  table (`giant_salamanka_stage1_attacks` `0x248cd98` and following) read by
  `boss_stage_attack_enabled_dispatch` (`0x4a1ac0`).
- **Luminar** computes its stage inside `luminar_ai_select_action` (`0x4c9410`), not in the check-stage
  dispatcher. The same HP fraction is written to `level_light_scale` (`0x59c358`), so the level gets darker
  as Luminar loses HP. *Inferred* for the visible effect.
- Giant Salamanka, Pyrul and Titus at HP 0 go idle and set owner flag 8, but only when
  `game_debug_flags` bit `0x20` is set. Bit `0x20` means "cutscene running" ([Cutscenes](cutscenes.md)), so
  this is the death pose kept while the boss's Death cutscene plays. *Inferred* for the purpose.
- Bosses without HP stages: Machival, Evil Urath and Murod only clear their "casting" state when the cast
  move has finished. Murod Oni has no stage check. Evil Joseph's check revives helpers (below).
- Stage variables: `serpent_stage` `0x2451d98`, `phoenix_stage` `0x2452420`, `tiger_stage` `0x2452a90`,
  ghost `0x2480310` (forced `0x2480314`), pyrul `0x247fd28` (forced `0x247fd2c`), `titus_stage`
  `0x248b3a8`, `giant_salamanka_stage` `0x248c7d4`, `luminar_stage` `0x248cdb8`.

## Special mechanics

| Mechanic | Boss | Functions | Notes |
|---|---|---|---|
| Mount and rider | Serpent, Phoenix, Tiger, Ghost | `boss_ensure_mount` `0x4a1dc0`, `*_seat_rider`, `boss_release_mount` `0x4a1fe0` | the rider sits on the mount's `$sit` bone; a released mount runs to the farthest graph point with goal `0xd` for 5 s **observed** |
| Illusion copies | Serpent | `serpent_spawn_rider` `0x4955e0`, `serpent_illusion_start` `0x496ba0`, `serpent_part_on_hit` `0x4a28b0` | two extra species-`0x59` type-9 copies, `Serp-illusion.vfx`; a copy that is hit is deactivated **observed**; that they are the "illusion" clones is *inferred* |
| Whip | Serpent | `serpent_whip_test` `0x497890`, `serpent_whip_resolve_hit` `0x4979b0` | hits a party member in whip reach **observed** |
| Counter hit | Serpent parts, Ghost | `boss_counter_hit_dispatch` `0x4a2230` | a move with flag `0x10` hits the melee attacker with its spell **observed** |
| Minion revive | Ghost Rider | `ghost_revive_minions` `0x4bbc70`, `ghost_fire_attack` `0x4bb9a0` | spells `0x51`-`0x53` raise a counter (`0x24808c8`, max 2); dead minions are brought back with `boss_resurrect_full` `0x4c3fb0` while it is positive **observed** |
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
| Rider flinch | Serpent, Phoenix, Tiger, Giant Salamanka | `serpent_rider_set_flinch` `0x4954c0` and others | reads anims named `stage<N>_flinch_area` / `_status` into per-stage tables; how they are used is *unknown* |

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
| Evil Urath, Evil Joseph, Machival | `templeint2` | Machival-intro, Machival-Trans1, Machival-Trans2, Machival-Death | Urath fights first; when Urath dies, Trans1 enables Evil Joseph; when Joseph dies, Trans2 enables Machival (`0x248b034`) |
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
| Tiger Rider | `0x57` | templeint, jadetemple | 3 | `0x499e50`-`0x49c7f0` | 4 spear moves, then HP 50 % and Dismount |
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
`0x4a04b0`-`0x4a3500`. Level names come from the level-script function names. *Inferred* for the exact
range ends.

## Open questions

- The meaning of each `*_boss.tbl` field and the stage numbers in the tables (the `.tbl` files were not read).
- How the per-stage rider flinch tables (`0x24523c0`.. for the serpent) are used.
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
