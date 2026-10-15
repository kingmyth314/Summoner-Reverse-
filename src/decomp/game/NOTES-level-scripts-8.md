# Phase 3: eight level script units (decomp43)

`game/level_script_sewers.cpp`, `level_script_templeint2.cpp`, `level_script_jadetemple.cpp`,
`level_script_wolong2.cpp`, `level_script_lpalaceint.cpp`, `level_script_liangshan.cpp`,
`level_script_wolong_caverns.cpp` and `level_script_catacombs.cpp` are the levelscripts\Scripts units of the
module column of sum.symbols, written from Sum.exe v1.40's disassembly only (`node tools/asm.mjs fn`; jump
tables, the palace guard tables, the level_script_table names and every float constant read from the raw
bytes of `Summoner/Sum.exe`). No Summoner 2 or Dash Faction code was read.

Each file passes `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include
<file>` with no diagnostics. `include/` is the repo's tree at fccd4a9 (after gameplay.cpp, decomp42) plus
CHANGED-HEADERS.txt; every existing `src/decomp/game/*.cpp` and `src/decomp/vsdk/*.cpp` (74 files) gives
the same diagnostics with this tree as with the repo's. Decisions D2400-D2407 are appended to
`include/DECISIONS.md`. The header additions are also in `reapply.mjs` (re-applies them to a fresh copy of
src/decomp/include, skipping what is already there): the repo moved twice while this batch was written
(level_scripts_common.cpp decomp41 and gameplay.cpp decomp42 declared most of the helpers I first added).

**Everything in the eight units is written.** 145 function comment lines: every sum.symbols address of the
eight modules (the static initialiser thunks and the two compiler atexit stubs included); none left.

| File | Functions | Exact | Likely | Unsure |
|---|---|---|---|---|
| level_script_sewers.cpp | 21 | 18 | 3 | 0 |
| level_script_templeint2.cpp | 16 | 14 | 2 | 0 |
| level_script_jadetemple.cpp | 13 | 8 | 5 | 0 |
| level_script_wolong2.cpp | 15 | 14 | 1 | 0 |
| level_script_lpalaceint.cpp | 21 | 12 | 9 | 0 |
| level_script_liangshan.cpp | 20 | 19 | 1 | 0 |
| level_script_wolong_caverns.cpp | 24 | 15 | 5 | 4 |
| level_script_catacombs.cpp | 15 | 13 | 2 | 0 |
| **total** | **145** | **113** | **28** | **4** |

The 4 unsure ones are the two empty initialiser pairs of wolong_caverns (which vector3 object each
belongs to is not visible, as in ai.cpp).

## Scheme

As decomp34 (D1400): no unit header; the helpers are declared at the top of each .cpp, the table hooks
come from levelscripts/level_scripts.h. Functions are in address order. Function-local statics (D2405) for
the two timers that VC6 guards with a bit and an empty atexit handler. Constant level indices are
per-unit #defines (D2407).

## Function tables

### sewers

| Address | Function | Confidence |
|---|---|---|
| 0x4ed7c0 | static_init_ls_sewerboss_timer | exact |
| 0x4ed7d0 | init_ls_sewerboss_timer | exact |
| 0x4ed7e0 | ls_sewer_init | likely |
| 0x4edc30 | ls_sewerboss_init | exact |
| 0x4edd30 | ls_sewerboss_leave | exact |
| 0x4edd50 | ls_sewer_frame | exact |
| 0x4edd90 | lsf_sewer_flag_scene_a | exact |
| 0x4edde0 | lsf_sewer_flag_found_crate | exact |
| 0x4ede50 | lsf_sewers_flag_crate_comment | exact |
| 0x4edec0 | lsf_sewers_flag_different_comment | exact |
| 0x4edf30 | lsf_sewers_flag_passage_comment | exact |
| 0x4edfa0 | lsf_sewers_flag_doll_comment | exact |
| 0x4ee000 | lsf_sewers_flag_where_comment | exact |
| 0x4ee070 | lsf_sewers_flag_smell_comment | exact |
| 0x4ee0e0 | ls_sewerboss_frame | exact |
| 0x4ee220 | ls_sewerboss_golems_dead | exact |
| 0x4ee260 | lsf_sewer_found_tb | exact |
| 0x4ee2d0 | ls_sewer_tb_intro_cs_end | likely |
| 0x4ee3c0 | ls_sewer_create_avoid_spline | likely |
| 0x4ee4f0 | ls_sewer_use | exact |
| 0x4ee580 | ls_sewer_action | exact |

### templeint2

| Address | Function | Confidence |
|---|---|---|
| 0x4ef190 | ls_templeint2_init | exact |
| 0x4ef1e0 | ls_templeint2_frame | exact |
| 0x4ef220 | ls_templeint2_act4_cs_end | exact |
| 0x4ef290 | ls_templeint2_machival_intro_cs_end | exact |
| 0x4ef2d0 | lsf_urath_killed | exact |
| 0x4ef330 | ls_templeint2_machival_trans1_cs_end | exact |
| 0x4ef360 | lsf_joseph_killed | exact |
| 0x4ef3c0 | ls_templeint2_machival_trans2_cs_end | exact |
| 0x4ef3f0 | lsf_mach_killed | exact |
| 0x4ef4d0 | ls_templeint2_machival_death_cs_end | likely |
| 0x4ef5d0 | ls_templeint2_enter_forge | exact |
| 0x4ef680 | ls_templeint2_enter_forge_cs_end | exact |
| 0x4ef6a0 | goto_credits | exact |
| 0x4ef6b0 | ls_templeint2_black_knight_revive | likely |
| 0x4ef770 | ls_templeint2_use | exact |
| 0x4ef830 | ls_templeint2_leave_temple_cs_end | exact |

### jadetemple

| Address | Function | Confidence |
|---|---|---|
| 0x4effb0 | static_init_ls_jadetemple_timers | likely |
| 0x4effc0 | init_ls_jadetemple_timers | likely |
| 0x4effe0 | ls_jadetemple_init | likely |
| 0x4f0110 | ls_jadetemple_frame | exact |
| 0x4f0160 | lsf_tg_intro | exact |
| 0x4f0320 | ls_jadetemple_tiger_ghost_intro_cs_end | exact |
| 0x4f0360 | lsf_tiger2_dead | likely |
| 0x4f0680 | lsf_murod_intro | exact |
| 0x4f0840 | ls_jadetemple_murod_intro_cs_end | exact |
| 0x4f0860 | ls_jadetemple_murod_trans_cs_end | exact |
| 0x4f0880 | ls_jadetemple_murod_death_cs_end | exact |
| 0x4f0990 | ls_jadetemple_finish | exact |
| 0x4f0a40 | ls_jadetemple_action | likely |

### wolong2

| Address | Function | Confidence |
|---|---|---|
| 0x4f1950 | lsf_wolong2_fight1_won | exact |
| 0x4f1a00 | lsf_wolong2_fight2_won | exact |
| 0x4f1ab0 | lsf_wolong2_fight3_won | exact |
| 0x4f1b60 | lsf_wolong2_fight4_won | exact |
| 0x4f1bf0 | ls_wolong2_reinforcements | exact |
| 0x4f1c80 | ls_wolong2_force_recruit | exact |
| 0x4f1d00 | lsf_wolong2_pijian_intro | exact |
| 0x4f1dd0 | ls_wolong2_pijian_death_cs_end | exact |
| 0x4f1e20 | ls_wolong2_pijian_intro_cs_end | exact |
| 0x4f1e40 | ls_wolong2_force_alert | exact |
| 0x4f1eb0 | ls_wolong2_force_alert_action | exact |
| 0x4f2060 | ls_wolong2_init | exact |
| 0x4f2140 | ls_wolong2_frame | likely |
| 0x4f21e0 | ls_wolong2_action | exact |
| 0x4f21f0 | ls_wolong2_use | exact |

### lpalaceint

| Address | Function | Confidence |
|---|---|---|
| 0x4f29d0 | static_init_ls_lpalaceint_timer | exact |
| 0x4f29e0 | init_ls_lpalaceint_timer | exact |
| 0x4f29f0 | static_init_lpalaceint_guard_halt_lines | likely |
| 0x4f2a00 | init_lpalaceint_guard_halt_lines | likely |
| 0x4f2a50 | static_init_lpalaceint_guard_servant_lines | likely |
| 0x4f2a60 | init_lpalaceint_guard_servant_lines | likely |
| 0x4f2aa0 | static_init_lpalaceint_guard_servant_line | likely |
| 0x4f2ab0 | init_lpalaceint_guard_servant_line | likely |
| 0x4f2ac0 | ls_lpalaceint_init | exact |
| 0x4f2ce0 | ls_lpalaceint_frame | exact |
| 0x4f2f00 | ls_lpalaceint_sneak_frame | likely |
| 0x4f34a0 | ls_lpalaceint_guard_in_group | likely |
| 0x4f3510 | atexit_nop_4f3510 | exact |
| 0x4f3520 | ls_lpalaceint_sneak_remark | likely |
| 0x4f36c0 | lsf_lpalaceint_talked_to_joe | exact |
| 0x4f3710 | ls_lpalaceint_king_meeting_aoqi_cs_end | exact |
| 0x4f3730 | ls_lpalaceint_meeting_with_yago_cs_end | exact |
| 0x4f3820 | ls_lpalaceint_king_audience_b_cs_end | exact |
| 0x4f3880 | ls_lpalaceint_talk_to_king | exact |
| 0x4f3890 | ls_lpalaceint_action | exact |
| 0x4f3a40 | ls_lpalaceint_use | exact |

### liangshan

| Address | Function | Confidence |
|---|---|---|
| 0x4f4930 | ls_liangshan_init | likely |
| 0x4f4a20 | ls_liangshan_frame | exact |
| 0x4f4df0 | lsf_liangshan_flag_pit_comment | exact |
| 0x4f4e60 | lsf_liangshan_flag_stone_comment | exact |
| 0x4f4ed0 | lsf_liangshan_flag_village_comment | exact |
| 0x4f4f40 | lsf_liangshan_flag_lair_comment | exact |
| 0x4f4f90 | lsf_liangshan_flag_shrine_comment | exact |
| 0x4f5010 | lsf_liangshan_flag_grave_comment | exact |
| 0x4f5080 | lsf_liangshan_flag_noise_comment | exact |
| 0x4f50f0 | lsf_liangshan_flag_bones_comment | exact |
| 0x4f5160 | lsf_liangshan_flag_wave2 | exact |
| 0x4f51e0 | lsf_liangshan_flag_wave3 | exact |
| 0x4f5280 | lsf_liangshan_flag_wave4 | exact |
| 0x4f5320 | lsf_liangshan_flag_finished_quest | exact |
| 0x4f53e0 | ls_liangshan_poison_check | exact |
| 0x4f5490 | ls_liangshan_serp_phnx_intro_cs_end | exact |
| 0x4f54b0 | lsf_li_intro | exact |
| 0x4f54f0 | ls_liangshan_action | exact |
| 0x4f55f0 | ls_liangshan_start_wave1 | exact |
| 0x4f5630 | ls_liangshan_use | exact |

### wolong_caverns

| Address | Function | Confidence |
|---|---|---|
| 0x4f6620 | static_init_ls_wolongcaverns_trap1_timer | exact |
| 0x4f6630 | init_ls_wolongcaverns_trap1_timer | exact |
| 0x4f6640 | static_init_ls_wolongcaverns_trap2_timer | exact |
| 0x4f6650 | init_ls_wolongcaverns_trap2_timer | exact |
| 0x4f6660 | static_init_ls_wolongcaverns_ptrap_timer | exact |
| 0x4f6670 | init_ls_wolongcaverns_ptrap_timer | exact |
| 0x4f6680 | static_init_empty_4f6690 | unsure |
| 0x4f6690 | init_empty_4f6690 | unsure |
| 0x4f66a0 | static_init_empty_4f66b0 | unsure |
| 0x4f66b0 | init_empty_4f66b0 | unsure |
| 0x4f66c0 | ls_wolongcaverns_init | exact |
| 0x4f6850 | ls_wolongcaverns_update_triggers | exact |
| 0x4f68f0 | ls_wolongcaverns_frame | exact |
| 0x4f6910 | ls_wolongcaverns_trap1_frame | likely |
| 0x4f6990 | ls_wolongcaverns_trap1_fire | likely |
| 0x4f6aa0 | ls_wolongcaverns_ptrap_frame | likely |
| 0x4f6b20 | ls_wolongcaverns_ptrap_fire | likely |
| 0x4f6c30 | ls_wolongcaverns_demilich_sequence | exact |
| 0x4f6de0 | atexit_nop_4f6de0 | exact |
| 0x4f6df0 | lsf_wc_nali_used | exact |
| 0x4f6f10 | ls_wolongcaverns_plants_remark | exact |
| 0x4f6f80 | ls_wolongcaverns_action | exact |
| 0x4f6ff0 | ls_wolongcaverns_use | exact |
| 0x4f7110 | ls_wolongcaverns_trap2_fire | likely |

### catacombs

| Address | Function | Confidence |
|---|---|---|
| 0x4fde80 | ls_catacombs_init | exact |
| 0x4fdfc0 | lsf_ball1_used | exact |
| 0x4fe030 | lsf_ball2_used | exact |
| 0x4fe0a0 | lsf_catacombs_boss_dead | exact |
| 0x4fe1c0 | ls_catacombs_frame | exact |
| 0x4fe250 | lsf_catacombs_flag_statues_comment | exact |
| 0x4fe2c0 | lsf_catacombs_flag_bones_comment | exact |
| 0x4fe330 | lsf_catacombs_flag_way_comment | exact |
| 0x4fe380 | lsf_maindoor_open | exact |
| 0x4fe410 | lsf_catacombs_flag_iona_comment | exact |
| 0x4fe460 | ls_catacombs_sync_object_positions | exact |
| 0x4fe4c0 | lsf_carados_cutscene | exact |
| 0x4fe580 | ls_carados_intro_cs_end | exact |
| 0x4fe5b0 | ls_catacombs_lobj_state | likely |
| 0x4fe5e0 | ls_catacombs_use | likely |

## Data

| Unit | .bss | .data |
|---|---|---|
| sewers | 0x257e998-0x257e9a7 (golems_done, music_timer, avoid_spline, arrival) | literals only |
| templeint2 | 0x257e9b4-0x257e9b8 (0x257e9b5 is written once and never read: ls_templeint2_unused_9b5, name ours) | literals only |
| jadetemple | 0x257e9bc-0x257e9cc | literals only |
| wolong2 | 0x257e9f4 | literals only |
| lpalaceint | 0x257ea00-0x257ea55 (0x257ea04 ls_lpalaceint_caught_counts, name ours; 0x257ea0c the static timer's guard) | 0x5a28f4 / 0x5a2904 the two guard name tables (char*[4], char*[7], names ours) |
| liangshan | 0x257ea60-0x257ea61 | literals only |
| wolong_caverns | 0x257ea78-0x257eaff (0x257eab0 ls_wolongcaverns_spike_pos[6], name ours; 0x257ea9c the static timer's guard) | literals only |
| catacombs | 0x257ed64-0x257ed7b | literals only |

The other .bss in 0x257e990-0x257eb10 (ikaemosl2, lenele3d, eleh, khosanilab2, lpalaceint02, iona_ext2,
rand_forest*, templeint, ionaext) belongs to the units between these, in address order, as sum.symbols has it.

## Open questions

- ls_sewer_init: on the first visit `_stricmp(level_prev_filename, "lenele1d")` is called and tested
  (0x4ed88b) but both outcomes call game_stage_set(0) (one call, no branch): written as if / else with equal
  bodies (likely). Something stage-dependent for an arrival from lenele1d seems to have been removed.
- ls_sewer_create_avoid_spline: the four corner vector2 locals and their store order are a guess (likely);
  the def contents are certain (count 4, cells (1,2) (1,2) (2,2) (2,2), the same two segments twice).
- ls_lpalaceint_sneak_remark: each case is written with its own call; the binary shares one call per stage
  (cross-jumping, or text / time variables in the source).
- ls_lpalaceint_guard_in_group's count is built branch-free (`neg al / sbb / and -3 / add 7`): written as
  `upper ? 4 : 7` (likely).
- ls_wolongcaverns_ptrap_fire: a three-pass loop around one loop-invariant distance (stored once to the
  stack, 0x4f6bc4) - most likely copied from the spear traps' loop over three spikes.
- ls_jadetemple_action: the four "lost item" blocks are expanded in place (item pointer formed before the
  index check, two buffers 0x100): an inline helper or a macro of the original; written out.
- level_trigger_trip's two outputs in ls_catacombs_frame are never read.
- The two init_empty pairs of wolong_caverns: which of ptrap_pos / spike_pos each constructs (unsure).
- 0x5fc014 (game_debug_5fc014) and decomp42's cheat_gold_5fbf19: no writer anywhere; probably console
  toggles whose command objects were compiled out.

## Disagreements with docs and symbols

- sum.symbols ls_jadetemple_tiger2_timer: "8 s after the second tiger dies". It is armed when all four
  riders of jadetemple_v2 are dead and the Spirit of Laharah is given (0x4f0565), then starts the 5 s fade.
- sum.symbols ls_liangshan_phoenix_dead_done: "set when phoenix_dead restores the level music". It is set
  to 1 unconditionally by ls_liangshan_init (0x4f49be), so the restore never runs (game bug below).
- sum.symbols ls_lpalaceint_caught_remark_pending: "set with the sneak attempt counter". It is set exactly
  when the capture does not count (the guard was Palace Guard#1a, 0x4f30b9); the counted case increments
  lpalaceint_int_sneak_attempts instead.
- sum.symbols ls_wolongcaverns_demilich_timer / ls_lpalaceint_sneak_frame_timer are function-local statics
  (D2405), and ls_lpalaceint_guard_*_lines are slot tables (char **, D2400), ls_catacombs_ball*_lobj lobj *.
- sum.symbols 0x4f0d80 static_init_ls_jadetemple_timer_b (module -): it jumps to init_ls_lenele3d_timer and
  belongs to level_script_lenele3d.cpp, not jadetemple.
- bosses.md "All four riders start at forced stage 3 (on foot) in Liangshan and Jadetemple": the level
  scripts call the *_set_solo functions in ls_liangshan_init (serpent, phoenix) and ls_jadetemple_init for
  jadetemple_v2 (all four), but in the first jadetemple visit only at the end of Tiger-Ghost-Intro
  (ls_jadetemple_tiger_ghost_intro_cs_end 0x4f0334). The stage forcing by boss_load is not contradicted.
- bosses.md (templeint2 row) is right; add: Machival-Death removes Jekhar, Flece and Rosalind, dismisses
  every summon (fx_end_owned, summon_return_lent_max_hp, deinit) and places Joseph at entry 2; the endings
  are End1 ($end2trig -> End1-EnterForge -> End1-Montage) and End2 (the lenele3a exit -> End2-LeaveTemple ->
  End2-Montage), both ending in goto_credits (game sequence state 9).
- level-scripts.md: the table record count and hook layout match; nothing in these units disagrees.

## Game bugs and oddities (story logic)

1. **Catacombs: cryptintro can never play.** ls_catacombs_frame plays it only when the never written byte
   0x5fc014 is set *and* catacombs_flag_intro_played is already set (`je` on a zero get_flag at 0x4fe206:
   the test is inverted); it then sets the flag it just tested. Even with the byte set it would replay the
   cutscene every frame after the first.
2. **Liangshan: the boss music never stops after the Serpent and Phoenix Riders.** ls_liangshan_init sets
   ls_liangshan_phoenix_dead_done = 1 on every entry, so the "both dead -> music_play(level_music)" branch of
   ls_liangshan_frame is dead; the "Boss" track keeps playing until the level is left.
3. **Liangshan wave 4 ignores Orenian Samurai#w3a.** lsf_liangshan_flag_wave4 tests only Samurai#w3b and
   Mage#w3; wave 4 (Wentao) can start while w3a is alive. finished_quest does not test w3a either.
4. **Wolong caverns: the poison mist trap applies Curse, not Poison.** ls_wolongcaverns_ptrap_fire adds
   status 3 (Curse, value 0.5, 40 min) after 15 damage (`push 3` at 0x4f6c09); the Liangshan marsh uses 11
   (Poison). Probably a wrong status id. Its damage loop also runs three times over one distance (copy of
   the spear traps; harmless, it breaks on the first hit).
5. **Wolong caverns: Joseph's "Not yet! I must find the Tail of the Rokhul." / "The Ring of Water must be
   around here somewhere."** are issued every frame while the party stands at $healpoint (no flag), so they
   restart continuously.
6. **Wolong2: the Charge action clears wolong2_charge** (set_flag(..., false), 0x4f1f06), the flag
   ls_wolong2_init sets to 1 on every wolong2 entry; nothing in Sum.exe reads it (only the conversation
   files can). Either the action or the init has the polarity backwards, or the flag is unused.
7. **Flag-only remarks.** lsf_catacombs_flag_way_comment and lsf_liangshan_flag_lair_comment set their
   flag and say nothing (the line was cut); lsf_sewer_flag_scene_a and lsf_sewer_flag_found_crate are
   flag-only by design (other code reads the flags).
8. **Palace: the servant line table has one entry** read with `rand() % 1` (D2400); harmless.
9. **Templeint2: Nar-TempleAct4 plays on every entry** (ls_templeint2_act4_played is .bss, cleared by
   init, not a saved flag); after a reload inside the temple the act 4 narration repeats. mach_intro keeps
   the rest from replaying.
10. **Unchecked results.** party_add_member / living_from_handle in ls_wolong2_frame (Jekhar)
    and the riders' script_object_find_character results go straight into *_set_solo (a missing rider
    would crash).
11. **Catacombs: the king tomb puzzle** opens the central crypt with E and F whatever the other tombs show;
    only the 2000 XP needs all of A-D, G-M closed (not a bug, but the reward test and the opening differ).
