# decomp59 notes: 0x4c0250-0x4ca6ff (projectors, lamp glows, eight boss units)

Written from the Sum.exe v1.40 disassembly (`node tools/asm.mjs`, a range-dump copy of it in the scratchpad,
jump tables decoded from the raw bytes, floats read from .rdata). Summoner 2 was not used. Header base: the
include tree of 09884b6 (the work started on 893c6a5; the edits were rebased onto 09884b6, whose decomp57
already had `shadow_record` and the `lobj_count` bits, see D3900). All eleven new files pass
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include` with no errors
and no warnings; all 143 existing `src/decomp` .cpp files (game/ and vsdk/) compile against this header copy
with no errors. Decisions D3900-D3908 are appended to `include/DECISIONS.md`; `CHANGED-HEADERS.txt` /
`CHANGED-HEADERS.diff` list the header edits (against 09884b6).

The whole range is written: every sum.symbols function of 0x4c0250-0x4ca6ff (244) plus 12 folded empty bodies
(the boss_ai.h / ai.h "folded" names); nothing is left in the range.

## Units and evidence (all file names are placeholders: no path string, no strings.tbl block in the range)

| File | Range | Evidence |
|---|---|---|
| `game/unnamed_projectors_0x4c0250.cpp` | 0x4c0250-0x4c0a3e | no initialiser; .bss 0x2483564-0x24893e7 (lobj_face_marks after the AI combat unit's 0x2483558-0x2483563; the face walk's static word 0x24893e4 last); the plane::operator= COMDAT 0x4c0a20 sits before the next initialiser |
| `game/unnamed_lamp_glow_0x4c0a40.cpp` | 0x4c0a40-0x4c0c3a | entry 317 (0x5884f8, empty); .data 0x59db70-0x59db88 (lamp_glow_bitmap, "WolongLampGlow01.tga"); .bss 0x24893e8-0x248b00b |
| `game/unnamed_machival_0x4c0c40.cpp` | 0x4c0c40-0x4c20d8 | entries 318-319; .data 0x59db8c-0x59dc01; .bss 0x248b010-0x248b37b |
| `game/unnamed_titus_0x4c20e0.cpp` | 0x4c20e0-0x4c3467 | entries 320-322; .data 0x59dc08-0x59dcb7; .bss 0x248b380-0x248b977 |
| `game/unnamed_evil_joseph_0x4c3470.cpp` | 0x4c3470-0x4c4167 | entries 323-324; .data 0x59dcb8-0x59dd3b; .bss 0x248b978-0x248bcfc; holds boss_target_list_find 0x4c3c70 and boss_resurrect_full 0x4c3fb0 |
| `game/unnamed_evil_urath_0x4c4170.cpp` | 0x4c4170-0x4c513b | entries 325-326; .data 0x59dd3c-0x59ddaf; .bss 0x248bd00-0x248bf33; its own copy boss_target_list_find_b 0x4c4580 |
| `game/unnamed_murod_0x4c5140.cpp` | 0x4c5140-0x4c5c1b | entry 327; .data 0x59ddb0-0x59ddbf (murod_cure_statuses); .bss 0x248bf38-0x248bfef |
| `game/unnamed_murod_oni_0x4c5c20.cpp` | 0x4c5c20-0x4c6470 | entry 328; .data 0x59ddd4-0x59dde6; .bss 0x248bff0-0x248c31b |
| `game/unnamed_tentacle_beast_0x4c6480.cpp` | 0x4c6480-0x4c8020 | entries 329-332; .data 0x59dde8-0x59debb; .bss 0x248c320-0x248c7ad |
| `game/unnamed_giant_salamanka_0x4c8030.cpp` | 0x4c8030-0x4c8d03 | entries 333-335; .data 0x59debc-0x59dfcd; .bss 0x248c7b0-0x248cdaf (the state object 0x248c7d0-0x248cdaf, D3904) |
| `game/unnamed_luminar_0x4c8d10.cpp` | 0x4c8d10-0x4ca6f3 | entries 336-337; .data 0x59dfd0-0x59e0e3; .bss 0x248cdb0-0x248ce3b; ai_predict_owner_position (no data) ends it before level_scripts_common.cpp's entry 338 (0x4ca700) |

No unit is named, so `modules.txt` holds only its header line.

Order notes: the initialisers run in definition order, while .bss addresses sometimes run the other way
(Titus's two commands, the Giant Salamanka's gsal / gs_stage / gsal_combat); the files define objects in
initialiser order and list the .bss in address order in comments.

## Functions

Total: 256 (154 / 100 / 2)

### game/unnamed_projectors_0x4c0250.cpp: 12 functions (8 exact / 3 likely / 1 unsure)

| Address | Function | Confidence |
|---|---|---|
| `0x4c0250` | projector_collect_lobj_faces | likely |
| `0x4c0670` | projector_record__ctor | exact |
| `0x4c0690` | shadow_record__ctor | exact |
| `0x4c06b0` | projector_record__clear_planes | exact |
| `0x4c06c0` | projector_record__set | exact |
| `0x4c06f0` | projector_record__add_plane | exact |
| `0x4c0740` | projector_record__to_lobj_space | likely |
| `0x4c0870` | projector_record__test_bounds | likely |
| `0x4c08d0` | lobj_project_records | unsure |
| `0x4c09a0` | lobj_face_marks_init | exact |
| `0x4c0a00` | lobj_face_marks_clear | exact |
| `0x4c0a20` | plane__assign | exact |

### game/unnamed_lamp_glow_0x4c0a40.cpp: 9 functions (8 exact / 1 likely / 0 unsure)

| Address | Function | Confidence |
|---|---|---|
| `0x4c0a40` | static_init_empty_4c0a50 | exact |
| `0x4c0a50` | init_empty_4c0a50 | likely |
| `0x4c0a60` | lamp_glow_load | exact |
| `0x4c0a90` | lamp_glow_add | exact |
| `0x4c0af0` | lamp_glow_forget | exact |
| `0x4c0b00` | lamp_glow_set | exact |
| `0x4c0b40` | lamp_glow_draw | exact |
| `0x4c0ba0` | lamp_glow_fade_out | exact |
| `0x4c0bf0` | lamp_glow_fade_in | exact |

### game/unnamed_machival_0x4c0c40.cpp: 31 functions (20 exact / 11 likely / 0 unsure)

| Address | Function | Confidence |
|---|---|---|
| `0x4c0c40` | static_init_machival_timers | exact |
| `0x4c0c50` | init_machival_timers | likely |
| `0x4c0cc0` | static_init_ccmd_machival_combat | exact |
| `0x4c0cd0` | init_ccmd_machival_combat | exact |
| `0x4c0cf0` | cmd_machival_combat | exact |
| `0x4c0d10` | machival_filter_target | exact |
| `0x4c0dd0` | machival_offense_point_reaches | likely |
| `0x4c0e70` | machival_reset_timers | exact |
| `0x4c0ee0` | machival_find_teleports | exact |
| `0x4c0f20` | machival_ai_idle | exact |
| `0x4c0f60` | machival_boss_load | likely |
| `0x4c1030` | machival_init_move_tables | exact |
| `0x4c1070` | machival_ai_frame | likely |
| `0x4c11c0` | machival_count_targets_in_reach | likely |
| `0x4c1330` | machival_pick_random_party_member | exact |
| `0x4c13a0` | machival_ai_select_action | likely |
| `0x4c17f0` | machival_set_attack_timer | exact |
| `0x4c1820` | machival_update_offense_points | likely |
| `0x4c18c0` | machival_cast_spell_at | exact |
| `0x4c1940` | machival_cast_spell | exact |
| `0x4c19c0` | machival_check_stage | exact |
| `0x4c19f0` | machival_ai_melee | exact |
| `0x4c1a80` | machival_knock_down_party | likely |
| `0x4c1b60` | machival_get_attack_kind | exact |
| `0x4c1bc0` | machival_attack_test | exact |
| `0x4c1d10` | machival_collect_area_hits | exact |
| `0x4c1dd0` | machival_apply_area_hits | exact |
| `0x4c1e20` | machival_teleport | likely |
| `0x4c1f90` | machival_ai_special_move | likely |
| `0x4c2090` | machival_ai_wait_move_end | exact |
| `folded (0x4f13e0, empty_function)` | machival_unload (name ours) | likely |

### game/unnamed_titus_0x4c20e0.cpp: 35 functions (25 exact / 9 likely / 1 unsure)

| Address | Function | Confidence |
|---|---|---|
| `0x4c20e0` | static_init_titus_timers | exact |
| `0x4c20f0` | init_titus_timers | likely |
| `0x4c2150` | static_init_ccmd_titus_stage | exact |
| `0x4c2160` | init_ccmd_titus_stage | exact |
| `0x4c2180` | cmd_titus_stage | exact |
| `0x4c21c0` | static_init_ccmd_titus_combat | exact |
| `0x4c21d0` | init_ccmd_titus_combat | exact |
| `0x4c21f0` | cmd_titus_combat | exact |
| `0x4c2210` | titus_filter_target | exact |
| `0x4c22a0` | titus_target_list_any_within | exact |
| `0x4c22e0` | titus_update_turn_lock | likely |
| `0x4c2330` | titus_reset_attack_timers | exact |
| `0x4c2360` | titus_reset_stage_timers | exact |
| `folded (0x4f13e0, empty_function)` | titus_stage1_reset (name ours) | likely |
| `folded (0x4f13e0, empty_function)` | titus_unload (name ours) | likely |
| `0x4c2380` | titus_boss_load | likely |
| `0x4c2450` | titus_init_move_tables | exact |
| `0x4c24c0` | titus_check_stage | likely |
| `0x4c2610` | titus_end_attack | exact |
| `0x4c2630` | titus_ai_frame | likely |
| `0x4c2790` | titus_ai_select_action | unsure |
| `0x4c2d90` | titus_set_attack_timer | exact |
| `0x4c2dc0` | titus_update_offense_points | likely |
| `0x4c2e60` | titus_ai_idle | exact |
| `0x4c2ea0` | titus_get_attack_reach | exact |
| `0x4c2ef0` | titus_knock_down_party | likely |
| `0x4c3060` | titus_get_attack_kind | exact |
| `0x4c30e0` | titus_hold_attack | exact |
| `0x4c3130` | titus_collect_area_hits | exact |
| `0x4c31f0` | titus_apply_area_hits | exact |
| `0x4c3240` | titus_ai_rock_erupt | exact |
| `0x4c32a0` | titus_start_rock_erupt | exact |
| `0x4c32b0` | titus_ai_wait_rock_erupt_fx | exact |
| `0x4c32e0` | titus_cast_special | exact |
| `0x4c3340` | titus_rock_erupt_damage | exact |

### game/unnamed_evil_joseph_0x4c3470.cpp: 24 functions (14 exact / 10 likely / 0 unsure)

| Address | Function | Confidence |
|---|---|---|
| `0x4c3470` | static_init_evil_joseph_timers | exact |
| `0x4c3480` | init_evil_joseph_timers | likely |
| `0x4c34b0` | static_init_ccmd_darkjo_combat | exact |
| `0x4c34c0` | init_ccmd_darkjo_combat | exact |
| `0x4c34e0` | cmd_darkjo_combat | exact |
| `0x4c3500` | evil_joseph_spawn_helpers | likely |
| `0x4c35e0` | evil_joseph_boss_load | likely |
| `0x4c3720` | evil_joseph_find_teleports | likely |
| `0x4c3790` | evil_joseph_init_move_tables | exact |
| `0x4c37f0` | evil_joseph_unload | exact |
| `0x4c3800` | evil_joseph_ai_idle | exact |
| `0x4c3860` | evil_joseph_ai_frame | likely |
| `0x4c3980` | evil_joseph_cast_special | exact |
| `0x4c39b0` | evil_joseph_ai_select_action | likely |
| `0x4c3c70` | boss_target_list_find | exact |
| `0x4c3ca0` | evil_joseph_set_attack_timer | exact |
| `0x4c3cd0` | evil_joseph_pick_helper_summon | likely |
| `0x4c3f20` | evil_joseph_check_stage | likely |
| `folded (0x4f13e0, empty_function)` | evil_joseph_check_stage_b (name ours) | likely |
| `0x4c3fb0` | boss_resurrect_full | exact |
| `0x4c4020` | evil_joseph_fire_attack | exact |
| `0x4c4080` | evil_joseph_filter_target | exact |
| `0x4c40b0` | evil_joseph_get_attack_move_id | exact |
| `0x4c40d0` | evil_joseph_destroy_effects | likely |

### game/unnamed_evil_urath_0x4c4170.cpp: 24 functions (14 exact / 10 likely / 0 unsure)

| Address | Function | Confidence |
|---|---|---|
| `0x4c4170` | static_init_evil_urath_timers | exact |
| `0x4c4180` | init_evil_urath_timers | likely |
| `0x4c41b0` | static_init_ccmd_urath_combat | exact |
| `0x4c41c0` | init_ccmd_urath_combat | exact |
| `0x4c41e0` | cmd_urath_combat | exact |
| `0x4c4200` | evil_urath_leap_check_path | likely |
| `0x4c4380` | evil_urath_pick_teleport_point | exact |
| `0x4c43f0` | evil_urath_cast_spell | exact |
| `0x4c4470` | evil_urath_filter_target | likely |
| `0x4c4580` | boss_target_list_find_b | exact |
| `folded (0x40acc0, empty_frame_hook_40acc0)` | evil_urath_load_hook (name ours) | likely |
| `folded (0x4f13e0, empty_function)` | evil_urath_unload (name ours) | likely |
| `0x4c45b0` | evil_urath_boss_load | exact |
| `0x4c4640` | evil_urath_find_teleports | exact |
| `0x4c46f0` | evil_urath_hold_height | exact |
| `0x4c4710` | evil_urath_ai_idle | exact |
| `0x4c4760` | evil_urath_ai_frame | likely |
| `0x4c47e0` | evil_urath_check_stage | exact |
| `0x4c4810` | evil_urath_ai_leap_start | exact |
| `0x4c4870` | evil_urath_ai_leap_rise | likely |
| `0x4c4ad0` | evil_urath_ai_leap_travel | likely |
| `0x4c4ca0` | evil_urath_ai_leap_land | likely |
| `0x4c4db0` | evil_urath_ai_select_action | likely |
| `0x4c5110` | evil_urath_set_attack_timer | exact |

### game/unnamed_murod_0x4c5140.cpp: 15 functions (8 exact / 7 likely / 0 unsure)

| Address | Function | Confidence |
|---|---|---|
| `0x4c5140` | static_init_murod_timers | exact |
| `0x4c5150` | init_murod_timers | likely |
| `0x4c51a0` | murod_start_leap | likely |
| `0x4c53c0` | murod_ai_leap | likely |
| `0x4c5550` | murod_end_move | exact |
| `0x4c5570` | murod_try_self_heal | exact |
| `0x4c5600` | murod_cast_spell | exact |
| `0x4c5680` | murod_ai_idle | exact |
| `folded (0x4f13e0, empty_function)` | murod_unload (name ours) | likely |
| `0x4c56c0` | murod_boss_load | exact |
| `0x4c5750` | murod_check_stage | exact |
| `0x4c57b0` | murod_ai_select_action | likely |
| `0x4c5a30` | murod_set_attack_timer | exact |
| `0x4c5a60` | murod_ai_frame | likely |
| `0x4c5b70` | murod_filter_target | likely |

### game/unnamed_murod_oni_0x4c5c20.cpp: 20 functions (9 exact / 11 likely / 0 unsure)

| Address | Function | Confidence |
|---|---|---|
| `0x4c5c20` | static_init_murod_oni_timers | exact |
| `0x4c5c30` | init_murod_oni_timers | likely |
| `0x4c5c80` | murod_oni_ai_idle | exact |
| `0x4c5cc0` | murod_oni_boss_load | likely |
| `0x4c5d80` | murod_oni_init_move_tables | exact |
| `0x4c5dc0` | murod_oni_init_timers | exact |
| `0x4c5de0` | murod_oni_ai_select_action | likely |
| `0x4c6010` | murod_oni_update_offense_points | likely |
| `0x4c60b0` | murod_oni_set_attack_timer | exact |
| `0x4c60e0` | murod_oni_ai_frame | likely |
| `0x4c6190` | murod_oni_evaluate_target | exact |
| `0x4c6230` | murod_oni_offense_point_reaches | likely |
| `0x4c62d0` | murod_oni_get_attack_kind | exact |
| `0x4c6330` | murod_oni_collect_area_hits | likely |
| `0x4c6410` | murod_oni_apply_area_hits | exact |
| `0x4c6460` | murod_oni_move_is_kick | exact |
| `folded (0x4f13e0, empty_function)` | murod_oni_unload (name ours) | likely |
| `folded (0x4f13e0, empty_function)` | murod_oni_check_stage (name ours) | likely |
| `folded (empty_function)` | murod_oni_fire_attack (name ours) | likely |
| `folded (ls_test_use: xor al, al)` | murod_oni_cast_special (name ours) | likely |

### game/unnamed_tentacle_beast_0x4c6480.cpp: 33 functions (18 exact / 15 likely / 0 unsure)

| Address | Function | Confidence |
|---|---|---|
| `0x4c6480` | static_init_tentacle_records | exact |
| `0x4c6490` | init_tentacle_records | likely |
| `0x4c6500` | static_init_tentacle_beast_timer | exact |
| `0x4c6510` | init_tentacle_beast_timer | exact |
| `0x4c64f0` | tentacle_slot__ctor | exact |
| `0x4c6520` | tentacle_beast_ai_select_action | likely |
| `0x4c6800` | tentacle_beast_start_grab | exact |
| `0x4c6850` | tentacle_beast_ai_grab | likely |
| `0x4c69e0` | tentacle_beast_start_release | exact |
| `0x4c6a80` | tentacle_beast_ai_release | likely |
| `0x4c6c20` | tentacle_begin_rise | exact |
| `0x4c6c50` | tentacle_begin_sink | likely |
| `0x4c6d10` | tentacle_beast_boss_load | likely |
| `0x4c7050` | tentacle_beast_unload | exact |
| `0x4c7060` | tentacle_beast_ai_idle | exact |
| `0x4c7070` | tentacle_beast_ai_frame | likely |
| `0x4c7330` | tentacle_find | exact |
| `0x4c7360` | tentacle_beast_check_stage | likely |
| `0x4c74f0` | tentacle_beast_tentacle_submerge | exact |
| `0x4c7570` | tentacle_beast_tentacle_resurface | likely |
| `0x4c7640` | tentacle_beast_battle_update | likely |
| `0x4c76d0` | tentacle_beast_cycle_tentacles | likely |
| `0x4c7770` | tentacle_get_attack_kind | exact |
| `0x4c77c0` | tentacle_collect_area_hits | likely |
| `0x4c78b0` | tentacle_apply_area_hits | exact |
| `0x4c7910` | tentacle_target_in_reach | exact |
| `0x4c7960` | tentacle_beast_frame_hook | likely |
| `0x4c7c90` | static_init_ccmd_tentacles | exact |
| `0x4c7ca0` | init_ccmd_tentacles | exact |
| `0x4c7cc0` | cmd_tentacles | likely |
| `0x4c7e60` | static_init_ccmd_tentacle_test | exact |
| `0x4c7e70` | init_ccmd_tentacle_test | exact |
| `0x4c7e90` | cmd_tentacle_test | likely |

### game/unnamed_giant_salamanka_0x4c8030.cpp: 26 functions (18 exact / 8 likely / 0 unsure)

| Address | Function | Confidence |
|---|---|---|
| `0x4c8030` | static_init_giant_salamanka_timers | exact |
| `0x4c8040` | init_giant_salamanka_timers | likely |
| `0x4c8070` | static_init_ccmd_gs_stage | exact |
| `0x4c8080` | init_ccmd_gs_stage | exact |
| `0x4c80d0` | static_init_ccmd_gsal_combat | exact |
| `0x4c80e0` | init_ccmd_gsal_combat | exact |
| `0x4c80a0` | cmd_gs_stage | exact |
| `0x4c8100` | cmd_gsal_combat | exact |
| `0x4c8120` | character_set_locomotion_anims | exact |
| `0x4c8160` | giant_salamanka_place_boss1 | exact |
| `0x4c81c0` | giant_salamanka_place_boss2 | exact |
| `0x4c8220` | giant_salamanka_place_boss3 | exact |
| `0x4c8280` | giant_salamanka_set_flinch | likely |
| `0x4c83a0` | giant_salamanka_boss_load | likely |
| `0x4c8540` | giant_salamanka_free_bitmap | exact |
| `0x4c8560` | giant_salamanka_check_stage | likely |
| `folded (0x4f13e0, empty_function)` | giant_salamanka_ai_idle (name ours) | likely |
| `0x4c86b0` | giant_salamanka_ai_frame | likely |
| `0x4c87e0` | giant_salamanka_ai_select_action | likely |
| `0x4c8b30` | giant_salamanka_get_missile_bone | exact |
| `0x4c8b60` | giant_salamanka_move_has_spell | exact |
| `0x4c8b80` | giant_salamanka_fire_attack | exact |
| `0x4c8c00` | giant_salamanka_move_pushes | exact |
| `0x4c8c20` | giant_salamanka_move_knocks_down | exact |
| `0x4c8c40` | giant_salamanka_target_in_reach | likely |
| `0x4c8cc0` | giant_salamanka_stage_attack_enabled | exact |

### game/unnamed_luminar_0x4c8d10.cpp: 27 functions (12 exact / 15 likely / 0 unsure)

| Address | Function | Confidence |
|---|---|---|
| `0x4c8d10` | static_init_luminar_timers | exact |
| `0x4c8d20` | init_luminar_timers | likely |
| `0x4c8d60` | static_init_ccmd_stage | exact |
| `0x4c8d70` | init_ccmd_stage | exact |
| `0x4c8d90` | cmd_stage | exact |
| `0x4c8dc0` | luminar_boss_load | likely |
| `0x4c8fd0` | luminar_free_bitmap | exact |
| `0x4c9000` | luminar_frame_hook | likely |
| `0x4c9410` | luminar_ai_select_action | likely |
| `0x4c95d0` | luminar_ai_idle | exact |
| `0x4c95f0` | luminar_ai_frame | likely |
| `0x4c97b0` | luminar_choose_melee_move | likely |
| `0x4c98a0` | luminar_ai_melee | likely |
| `0x4c9990` | luminar_try_start_grab | exact |
| `0x4c9a40` | luminar_ai_grab_approach | likely |
| `0x4c9b00` | luminar_ai_grab | likely |
| `0x4c9c40` | luminar_start_release | exact |
| `0x4c9cd0` | luminar_ai_release | likely |
| `0x4c9da0` | luminar_teleport_out | likely |
| `0x4ca000` | luminar_teleport_in | likely |
| `0x4ca1d0` | luminar_ai_beam_move_end | exact |
| `0x4ca230` | luminar_beam_start | likely |
| `0x4ca3a0` | luminar_beam_hit | likely |
| `0x4ca5a0` | luminar_hold_attack | exact |
| `0x4ca5c0` | luminar_can_switch_target | exact |
| `0x4ca5f0` | luminar_delay_next_attack | exact |
| `0x4ca650` | ai_predict_owner_position | likely |


## Source-shape notes and open questions

- **Exception frame without an object.** lobj_project_records (0x4c08d0) sets up an SEH frame with trylevel 0
  for its whole body but destroys nothing (as decals_draw_for_lobj, decomp56). Which local has a destructor
  is not known; marked unsure.
- **plane temporaries.** projector_record::to_lobj_space builds each local plane in a temporary, copies the
  normal once more through the out-of-line vector3 copy constructor (vector3::operator= returns a copy) and
  moves the plane through a second temporary with the out-of-line plane::operator= (0x4c0a20, emitted in this
  unit). The exact source expression is not known.
- **Shared tails.** The *_ai_select_action functions (Titus, Machival, Evil Joseph, Evil Urath, Murod) share
  their `return 1` / `return 3` tails; the order of the tests matches, the block shape is a guess (Titus is
  unsure).
- **boss_target_list_find twice.** 0x4c3c70 (Evil Joseph unit) and 0x4c4580 (Evil Urath unit) are the same
  body. Two copies mean the original had it file-static (or a non-inline header function with internal
  linkage) in each unit; the phoenix and Machival units call the first, the serpent the second.
- **The pf_cells / seg2d call in evil_urath_leap_check_path** reuses its parameter slots as the two
  intersection parameters; written with locals.
- **Unknown fields.** machival_248b310 (a constructed GameTimer), titus_248b950, evil_joseph_248b9a4 /
  _248bc90 / _248bccc, evil_urath_248bf00 / _248bf24, murod_oni_248bff8 / _248c2f0, tentacle_248c324 /
  _248c784, luminar_248cdb4 and gsal._unk010 / _unk018 / _unk588 are never accessed; the *_248b030-style
  dwords are written (0 or -1) and never read. evil_urath_unused_moves (0x140 bytes) looks like an attack table
  that the boss (no .tbl file) never parses.
- **Game flag 8** (game_flags & 8) changes the Luminar's grab and teleport candidates (any living entity, not
  only type 7). Its meaning is not known.
- **ai_state +0x194** (emerge_move) holds the tentacles' current move serial.
- **Tentacle +0x0c0** is a constructed GameTimer nobody uses; the slots' four dwords and timer are not used.

## Doc disagreements (to check)

- source-map.md hand cuts: the projector unit ends at 0x4c0a3e (the plane::operator= COMDAT 0x4c0a20 is
  its last function), not 0x4c0a20; Titus's .bss runs to 0x248b977 (ccmd_titus_stage 0x248b968), the
  Luminar's to 0x248ce3b (ccmd_stage 0x248ce30). The other rows agree.
- bosses.md "Special mechanics", Grab row: "the victim is held, leadership moves off it" holds for the
  tentacles (tentacle_beast_ai_grab calls party_set_leader on the first other member) but not for the Luminar
  (luminar_ai_grab does not touch the leader). The tentacle grab itself never happens (game bugs below), so
  in practice neither boss moves the leadership.
- sum.symbols machival_spell100/101/102_timer: "random 56-104 s" is only the rearm (SetRandomTimeout(55999,
  103999)); the first arming (machival_reset_timers, at the first target) is 0-60 s / 0-45 s / 0-30 s
  (rand() % 1000 * 60 / 45 / 30), and the teleport 70 s.
- sum.symbols luminar_thrown_handle: "the grabbed character carried by luminar_frame_hook". In the code
  luminar_grabbed_handle is the carried one (luminar_ai_grab stores it; the frame hook moves it with the bone)
  and luminar_thrown_handle the thrown one (set at the release; the frame hook flies and lands it).
- sum.symbols murod_oni_move_is_kick: it returns current == murod_oni_move_3 (moves[0][3]), not move 0.
- sum.symbols giant_salamanka_current_move "0x20 bytes from 0x248c844": the rows are &moves[stage - 1][i]
  computed from that base (D3904); the table itself starts at 0x248c984.
- bosses.md Murod row "jumps to a spot near a random party member": 15 units away from it, in a random
  horizontal direction (five tries, navigable for the member's size).
- effects.md lamp glows: the record's fourth dword is zeroed by lamp_glow_add and not read in this unit
  (sum.symbols "{pos, size, alpha, ?}").

## Game bugs (new; IDs to assign)

- **Tentacles never grab (dead code).** tentacle_beast_ai_select_action picks the grab move when `frand() <
  0` (0x4c666d-0x4c667f); frand is rand() * k >= 0, so tentacle_beast_start_grab / _ai_grab / the throw and
  the squeeze tick never run in v1.40. Observed in the code; not tested in game.
- **The tentacle squeeze tick heals.** While a tentacle holds someone, every 6 s add_hp(+3.0) (0x4c714b),
  a heal, not damage (dead with the grab above).
- **Titus stage 3 target -1.** When stage_timer_a runs out at a paralysed target and no flag-8 candidate
  exists, the local handle becomes -1 and is stored as titus_target_handle with the next move 21 / 22 / 23
  (0x4c2c7b, 0x4c2cf0, 0x4c2d4e, 0x4c2d61); the frame then goes idle (unless move 21) until the next pick.
- **NULL dereferences.** titus_rock_erupt_damage (living_from_handle unchecked, 0x4c3384 / 0x4c340a),
  titus_cast_special (titus_current_move unchecked, 0x4c32e5), evil_joseph_get_attack_move_id (0x4c40c5),
  evil_urath_filter_target and murod_filter_target (the handle read before the NULL test, 0x4c4482 /
  0x4c5b82), boss_target_list_find / _b (candidates unchecked), tentacle_beast_ai_release (the victim,
  0x4c6b53), tentacle_collect_area_hits and tentacle_beast_battle_update (members unchecked), luminar_beam_hit
  (trigger 7 hits the target without the NULL test the turn has, 0x4ca502).
- **Shadow receiver list wraps** (D3900): the 3-bit lobj_count; the eighth object is stored at +0xfc and the
  ninth overwrites the first; the `>= 16` exit is dead.
- **tentacle_beast_boss_load** takes the first free record without a bound (a fifth tentacle character would
  write past tentacle_beast_tentacles); tentacle hits hold 4 (a fifth hit overwrites death_anim).
- **evil_urath_leap_check_path** returns the distance of the last wall crossing found, not the nearest (the
  minimum is kept in a local that is never used, 0x4c4321-0x4c4338).
- **giant_salamanka_set_flinch** formats "Unable to set Giant Salamanka flinch for %s -- bad $Action" into a
  local buffer and never prints it; it also assumes every type-4 move is named "stageN_flinch_*".
- **Giant Salamanka forced stage**: a forced value outside 1-3 would set the stage to 0 (not reset the forced
  stage as the other bosses do); unreachable through cmd_gs_stage. The forced stage is never cleared.
- **Murod Oni's second timer** (murod_oni_timer_b, 5 s at load) is never rearmed: after 5 s every think may
  switch to a flag-4 target with chance 0.3.
- **luminar_teleport_out with game flag 8** may pick any living entity, the boss included, as the teleport
  anchor and target.
- Text: "Invalid stage for titus rider" (Titus is not a rider); "Sets dark josesph in combat state".

## Header changes (summary; CHANGED-HEADERS.txt)

See CHANGED-HEADERS.txt. No existing declaration changed its parameter list. One return type changed
(giant_salamanka_target_in_reach bool -> int, D3905; its caller converts). Six boss_ai.h externs became macros
of the new state object (D3904); boss_ai.cpp and level_script_khosani_lab.cpp compile unchanged.
tentacle_info's padding became named fields (D3903). lobj_face_mark_offsets got its size.
