# decomp56 notes: 0x4b87c0-0x4c0245 (Pyrul, Ghost Rider, decals, AI combat)

Written from the Sum.exe v1.40 disassembly (`node tools/asm.mjs fn`, jump tables decoded from the raw
bytes, floats read from .rdata). Summoner 2 was not used. Header base: the include tree of e718f3d
(rebased from ca313cb during the work); `CHANGED-HEADERS.txt` / `CHANGED-HEADERS.diff` list the edits.
All four files pass `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security`
with no errors and no warnings; all 132 existing `src/decomp` .cpp files compile against this header copy
with no errors. Decisions D3600-D3606 are appended to `include/DECISIONS.md`.

The whole range is written: every sum.symbols function of 0x4b87c0-0x4c0245 plus 3 folded empty bodies;
nothing is left.

## Units and evidence (all four file names are placeholders: no path string, no strings.tbl block)

| File | Range | Evidence |
|---|---|---|
| `game/unnamed_pyrul_0x4b87c0.cpp` | 0x4b87c0-0x4b9e62 | initialiser entries 306-308 (0x5884cc-0x5884d4); .data 0x59d934-0x59d9eb ("pyrul_stage" .. "$missile01"); .bss 0x247fcf0-0x24802df; follows pathfinding_visibility.cpp (last fn 0x4b86e0); random_stun_duration (no data) sits before the next initialiser, so it ends this unit |
| `game/unnamed_ghost_rider_0x4b9e70.cpp` | 0x4b9e70-0x4bbe77 | entries 309-313; .data 0x59d9ec-0x59daeb; .bss 0x24802e0-0x2480947 (starts at the ghost move pointers written by ghost_init_move_ptrs, D3604) |
| `game/unnamed_decals_0x4bbe80.cpp` | 0x4bbe80-0x4bce55 | entries 314-315; .data 0x59daf0-0x59db6f (decal_vert_ptrs, 32 pointers; lamp_glow_bitmap follows at 0x59db70); .bss 0x2480948-0x2483557; includes the s3d_geom_block members (0x4bcd40-0x4bcdc0) |
| `game/unnamed_ai_combat_0x4bce60.cpp` | 0x4bce60-0x4c0245 | entry 316 (empty initialiser); .bss 0x2483558-0x2483563 only; the projector unit 0x4c0250 has no initialiser and its .bss starts 0x2483564 |

No unit is named, so `modules.txt` holds only its header line.

## Functions

Total: 131 (65 / 60 / 6)

### game/unnamed_pyrul_0x4b87c0.cpp: 31 functions (19 exact / 11 likely / 1 unsure)

| Address | Function | Confidence |
|---|---|---|
| `0x4b87c0` | static_init_pyrul_timers | exact |
| `0x4b87d0` | init_pyrul_timers | likely |
| `0x4b8820` | static_init_ccmd_pyrul_stage | exact |
| `0x4b8830` | init_ccmd_pyrul_stage | exact |
| `0x4b8850` | cmd_pyrul_stage | exact |
| `0x4b8890` | static_init_ccmd_pyrul_combat | exact |
| `0x4b88a0` | init_ccmd_pyrul_combat | exact |
| `0x4b88c0` | cmd_pyrul_combat | exact |
| `0x4b88e0` | pyrul_target_filter | exact |
| `0x4b89b0` | pyrul_timer_a_reset | exact |
| `0x4b89c0` | pyrul_timer_b_reset | exact |
| folded (0x4f13e0) | pyrul_stage1_reset (name ours / boss_ai.h) | likely |
| folded (0x4f13e0) | pyrul_unload (name ours / boss_ai.h) | likely |
| `0x4b89d0` | pyrul_ai_idle | exact |
| `0x4b8a10` | pyrul_boss_load | likely |
| `0x4b8b20` | pyrul_init_move_ptrs | exact |
| `0x4b8b90` | pyrul_check_stage | likely |
| `0x4b8ce0` | pyrul_ai_frame | likely |
| `0x4b8da0` | pyrul_choose_attack | unsure |
| `0x4b93d0` | pyrul_timer_set_random | exact |
| `0x4b9400` | pyrul_update_attack_points | likely |
| `0x4b94a0` | pyrul_cast_special | likely |
| `0x4b95f0` | pyrul_get_missile_origin | likely |
| `0x4b9670` | pyrul_fire_attack | likely |
| `0x4b9a00` | pyrul_move_range | exact |
| `0x4b9a50` | pyrul_is_own_move | exact |
| `0x4b9ae0` | pyrul_attack_test_class0 | exact |
| `0x4b9c20` | pyrul_point_in_attack_cone | likely |
| `0x4b9cc0` | pyrul_attack_test_class2 | exact |
| `0x4b9e00` | pyrul_resolve_hit | exact |
| `0x4b9e50` | random_stun_duration | exact |

### game/unnamed_ghost_rider_0x4b9e70.cpp: 49 functions (30 exact / 18 likely / 1 unsure)

| Address | Function | Confidence |
|---|---|---|
| `0x4b9e70` | static_init_ccmd_ghost_stage | exact |
| `0x4b9e80` | init_ccmd_ghost_stage | exact |
| `0x4b9ea0` | cmd_ghost_stage | exact |
| `0x4b9ed0` | static_init_ccmd_ghost_unlink | exact |
| `0x4b9ee0` | init_ccmd_ghost_unlink | exact |
| `0x4b9f00` | cmd_ghost_unlink | likely |
| `0x4b9f90` | static_init_ccmd_ghost_combat | exact |
| `0x4b9fa0` | init_ccmd_ghost_combat | exact |
| `0x4b9fc0` | cmd_ghost_combat | exact |
| `0x4b9fe0` | ghost_find_rider | exact |
| `0x4ba020` | static_init_ccmd_ghost_solo | exact |
| `0x4ba030` | init_ccmd_ghost_solo | exact |
| `0x4ba050` | cmd_ghost_solo | exact |
| `0x4ba070` | static_init_ghost_timers | exact |
| `0x4ba080` | init_ghost_timers | likely |
| `0x4ba0d0` | ghost_find_floor_below | likely |
| `0x4ba260` | ghost_rider_boss_load | likely |
| `0x4ba690` | ghost_timer_set_random | exact |
| `0x4ba6c0` | ghost_set_stage1_anims | exact |
| `0x4ba780` | ghost_setup_anims_unmounted | exact |
| `0x4ba7a0` | ghost_setup_anims | exact |
| `0x4ba870` | ghost_init_move_ptrs | exact |
| `0x4ba8d0` | ghost_unload | exact |
| folded (0x4f13e0) | ghost_script_hook (name ours / boss_ai.h) | likely |
| `0x4ba8e0` | ghost_seat_rider | likely |
| `0x4baa60` | ghost_ai_idle | exact |
| `0x4baaa0` | ghost_wander_update | exact |
| `0x4baae0` | ghost_wander_start | likely |
| `0x4bac20` | ghost_choose_attack | unsure |
| `0x4bb050` | ghost_timer2_set_random | exact |
| `0x4bb080` | ghost_update_attack_points | likely |
| `0x4bb120` | ghost_choose_move_target | likely |
| `0x4bb380` | ghost_ai_frame | likely |
| `0x4bb4c0` | ghost_update_stage | likely |
| `0x4bb610` | ghost_anim_remap_in | likely |
| `0x4bb6c0` | ghost_anim_remap_out | likely |
| `0x4bb720` | ghost_set_foot_anims | exact |
| `0x4bb810` | ghost_dismount | likely |
| `0x4bb870` | ghost_set_ride_anims | exact |
| `0x4bb970` | ghost_cast_special | exact |
| `0x4bb9a0` | ghost_fire_attack | likely |
| `0x4bba20` | ghost_target_flags | exact |
| `0x4bba60` | ghost_is_own_move | exact |
| `0x4bbae0` | ghost_attack_test | exact |
| `0x4bbb80` | ghost_point_in_attack_cone | likely |
| `0x4bbc20` | ghost_resolve_hit | exact |
| `0x4bbc70` | ghost_revive_minions | likely |
| `0x4bbd80` | ghost_set_solo | exact |
| `0x4bbdd0` | ghost_minions_deactivate | likely |

### game/unnamed_decals_0x4bbe80.cpp: 21 functions (12 exact / 6 likely / 3 unsure)

| Address | Function | Confidence |
|---|---|---|
| `0x4bbe80` | static_init_decal_records | exact |
| `0x4bbe90` | init_decal_records | exact |
| `0x4bbeb0` | static_init_empty_4bbec0 | exact |
| `0x4bbec0` | init_empty_4bbec0 | likely |
| `0x4bbed0` | decal_record__ctor | exact |
| `0x4bbf10` | decal_build_clip_planes | unsure |
| `0x4bc290` | vector3__len | exact |
| `0x4bc2c0` | decal_set_frame | exact |
| `0x4bc2e0` | decal_set_u_axis | exact |
| `0x4bc300` | decal_set_v_axis | exact |
| `0x4bc320` | decal_place | likely |
| `0x4bc560` | decal_recycle | exact |
| `0x4bc640` | decals_reset | exact |
| `0x4bc6b0` | decals_draw_for_lobj | unsure |
| `0x4bcbb0` | decals_collect_for_lobj | likely |
| `0x4bcc20` | decals_recycle_unused | likely |
| `0x4bcd40` | s3d_geom_block__ctor | exact |
| `0x4bcd50` | s3d_geom_block__dtor | likely |
| `0x4bcd90` | s3d_geom_block__dtor_thunk | unsure |
| `0x4bcda0` | s3d_geom_block_clear | exact |
| `0x4bcdc0` | s3d_geom_block_read | likely |

### game/unnamed_ai_combat_0x4bce60.cpp: 30 functions (4 exact / 25 likely / 1 unsure)

| Address | Function | Confidence |
|---|---|---|
| `0x4bce60` | static_init_empty_4bce70 | exact |
| `0x4bce70` | init_empty_4bce70 | likely |
| `0x4bce80` | ai_state__within_attack_radius | exact |
| `0x4bcea0` | ai_approach_random_offset | likely |
| `0x4bcff0` | ai_approach_target_point | likely |
| `0x4bd2a0` | ai_approach_attack_slot | unsure |
| `0x4bd9c0` | ai_combat_start_approach | exact |
| `0x4bda50` | ai_combat_approach | likely |
| `0x4bdce0` | ai_combat_toggle_ready_stance | likely |
| `0x4be0a0` | ai_chase_target_until_in_range | likely |
| `0x4be280` | ai_combat_close_in | likely |
| `0x4be470` | ai_combat_face_target | likely |
| `0x4be5f0` | ai_combat_melee_attack | likely |
| `0x4be760` | ai_combat_ranged_attack | likely |
| `0x4be840` | ai_combat_follow_and_strike | exact |
| `0x4be8a0` | ai_combat_close_strike | likely |
| `0x4be940` | ai_score_support_targets | likely |
| `0x4bea80` | ai_choose_support_spell | likely |
| `0x4beba0` | ai_rate_spell_targets | likely |
| `0x4bee40` | ai_consider_blind_or_silence | likely |
| `0x4befe0` | ai_consider_area_spell | likely |
| `0x4bf0c0` | ai_consider_power_leech | likely |
| `0x4bf1d0` | ai_consider_death | likely |
| `0x4bf320` | ai_consider_poison | likely |
| `0x4bf460` | ai_consider_jade_beam | likely |
| `0x4bf5a0` | ai_consider_life_drain | likely |
| `0x4bf700` | ai_choose_offensive_spell | likely |
| `0x4bfaa0` | ai_combat_cast_frame | likely |
| `0x4bfed0` | ai_alert_guards_of_target | likely |
| `0x4bff60` | ai_mode_combat_frame | likely |

## Notes on the source shape

- Small switches compiled as `dec` chains have their bodies laid out in descending case order (the
  stage switches of pyrul_check_stage / ghost_update_stage, the weighted pick in pyrul_choose_attack). The
  code is written in binary order (boss_ai.cpp's convention); whether the original was descending is open.
- Several switches have an explicit `case -1` with its own jump-table slot (byte 0 of the index table maps
  to a separate entry that points at the default body): pyrul_ai_frame, pyrul_cast_special, ghost_ai_frame.
- pyrul_fire_attack's wall of fire falls through into the meteor storm case in multiplayer only (the
  single-player branch breaks); written as a fall-through.
- The decal unit calls the inline vector3 members out of line (ctor 0x43b580, copy 0x410cd0, operator=
  0x413190, +/- 0x45b680 / 0x45b6b0, scale 0x468a90) but expands vector3::len once; decals_draw_for_lobj has
  an exception frame for a local whose destructor is not visible. Both written plainly (unsure).
- 0x4bbec0 and 0x4bce70 are empty registered initialisers; which definitions they belong to is a guess
  (decal_verts / ai_combat_aim_point).
- living_combat::update(int move_id) (D3404, merged meanwhile) matches this unit's calls (move id, -1, 0).

## Open questions

- AIS value 0x22 (ai_mode_combat_frame: follow the path, re-decide after 2.5 s) has no safe table name
  (D1906 shift); written AIS_22.
- Species 0xca, 0x09, 0xb9 get poison / jade beam / life drain in ai_choose_offensive_spell; the names
  (poison elemental, jade golem, "life drainer") are ours. Character +0x690 (life drain's weight) is unnamed.
- ai_state +0x88 (_unk88): a spell id that weighs x4 in ai_rate_spell_targets; its writer is outside this range.
- ai_follow_target (0x247e878) and ai_combat_frames_watched (0x247e84c) belong to the AI movement unit
  (0x4b0210-0x4b77f0); declared extern here.
- ghost_find_floor_below's first argument (the target) is not read.

## Doc disagreements

- docs/sum-exe/source-map.md (hand cuts, 0x4b9e70 row): the ghost .bss starts at 0x24802e0, not 0x2480300
  (D3604). The units table starts its unnamed row at 0x4b9e80; Pyrul is its own unit 0x4b87c0-0x4b9e62
  (bosses.md has the range right).
- bosses.md "Minion revive": ghost_revive_minions resets every dead, inactive minion and decrements the
  counter without testing that it is positive (not "while it is positive"); the summon gate (< 2) is in
  ghost_choose_attack.
- bosses.md (stage-machine notes): the death-pose test is HP <= 0 (`fcomp 0; test ah, 0x41`) together with
  GF_CUTSCENE and !CF_HIDDEN.
- bosses.md says the riders are "at stage 3 but still mounted" in the first Jade Temple visit; for the
  Ghost Rider the Liangshan / Jadetemple branch of ghost_rider_boss_load runs ghost_setup_anims_unmounted,
  which calls boss_release_mount and clears ghost_mounted.
- sum.symbols: ai_state__within_attack_radius (0x4bce80) is an ai_state member (D3603); the ai_combat_* and
  ai_consider_* functions are thiscall on ai_state; plane__plane is plane's (n, p) constructor
  (D3601); ghost_find_floor_below takes (target, layer, pos).
- ai.md / effects.md: nothing contradicted in the parts this code covers.

## Game bugs (candidates)

1. pyrul_get_missile_origin (0x4b95f0): without the "$missile01" tag it copies the boss frame and then
   still calls the bone transform with tag -1 (no else).
2. pyrul_fire_attack (0x4b97b4-0x4b98f6): in multiplayer Pyrul's wall of fire also casts a meteor storm
   (spell 0x1c): the multiplayer branch falls into the next case.
3. ai_approach_target_point (0x4bd279-0x4bd285): with no free attack point the index stays -1 and
   `attack_point_users[-1] = handle` overwrites the target's fifth attacker-slot timer (ai_state +0x1d4).
4. ai_choose_support_spell (0x4beb3d-0x4beb62): the healer's random pick compares one number with each
   normalised score instead of the running sum, so it favours early records and often picks nothing.
5. ai_consider_blind_or_silence (0x4bef1c-0x4bef24): at cur_character the running total (not the record's
   weight) is multiplied by 1.5, inflating every later record.
6. ai_consider_life_drain (0x4bf66f-0x4bf674): the running total is added to the record's old score (left
   by an earlier consider function) instead of stored.
7. ai_combat_cast_frame (0x4bfd5d-0x4bfdc4) fills only the character of the offensive candidates; the
   distances ai_consider_area_spell tests (+0x04) are stale stack values.
8. ai_combat_cast_frame (0x4bfc36, 0x4bfd43): the target's / own AI group pointer is used without a NULL test.
9. cmd_ghost_unlink acts on the first Ghost Rider of the living list, ghost_find_rider (ghost_solo) on the last.
