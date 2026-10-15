# decomp54: 0x492b00-0x49c83a (AI groups, camera sequences, snow, serpent / phoenix / tiger riders)

All 220 functions of the range are written (every address has its own `// 0xADDR` line with a confidence;
220 checked against sum.symbols). 0x492ae0 / 0x492af0 (atexit_nop_*) already belong to levelrender.cpp and
are not repeated. All six units are placeholder files: no assert path, no strings.tbl read, no Summoner 2
file match (D3406). Nothing goes to modules.txt.

| File | Code | Static init entries | .bss / .data | Functions | exact / likely / unsure |
|---|---|---|---|---|---|
| game/unnamed_ai_groups_0x492b00.cpp | 0x492b00-0x493e41 | 263-264 | .bss 0x243e040-0x243e3d7, .data 0x59c594-0x59c5ef | 22 | 16 / 6 / 0 |
| game/unnamed_cutscene_camera_0x493e50.cpp | 0x493e50-0x494696 | 265-268 | .bss 0x243e3d8-0x2444597 | 20 | 14 / 6 / 0 |
| game/unnamed_snow_0x4946a0.cpp | 0x4946a0-0x495281 | 269-272 | .bss 0x2444598-0x2451d7b, .data 0x59c5e8-0x59c65b | 17 (+ folded snow_cell::reset) | 10 / 5 / 2 |
| game/unnamed_serpent_0x495290.cpp | 0x495290-0x497aba | 273-277 | .bss 0x2451d80-0x2452417, .data 0x59c65c-0x59c80b | 49 | 32 / 16 / 1 |
| game/unnamed_phoenix_0x497ac0.cpp | 0x497ac0-0x499e41 | 278-282 | .bss 0x2452418-0x2452a6b, .data 0x59c80c-0x59c97f | 42 | 28 / 13 / 1 |
| game/unnamed_tiger_0x499e50.cpp | 0x499e50-0x49c83a | 283-287 | .bss 0x2452a70-0x2452cb7, .data 0x59c980-0x59cb27 | 54 (incl. boss_dismount) | 31 / 21 / 2 |

The per-function list (address, confidence, reason) is the comment block above each definition in the .cpp
files; the compiler-generated initialiser thunks, array constructors and scalar deleting destructors have
their own comment lines. Unsure: cutscene camera none; snow 0x4946b0/0x4946d0 (which array each empty
initialiser constructs); serpent_choose_attack 0x496330 (flag variables of stage 1); phoenix_choose_attack
0x498a20 (re-pick shape, discarded frand calls); tiger_path_to_target 0x49b520 (out-of-line vector3
temporaries) and tiger_combat_update 0x49b6e0 (the x87 closing-speed formula).

## Layout findings (DECISIONS D3400-D3406 in include/DECISIONS.md)

- ai_group +0x1c/+0x20 = num_enemies / enemies[10]; sum.symbols' ai_think_enemies / ai_think_members hold the
  enemies / the group's members, the other way round (D3400).
- camera_blend_sequence__time_left / _advance are methods of a never-constructed state-1 "blend"
  sequence with a different layout: new class camera_blend_sequence (D3401). anim_track +0x14..+0x20 named (D3402).
- Each rider boss's state is one object (one timer initialiser; the serpent's is cleared with one rep stosd):
  serpent_boss 0x2451d98, phoenix_boss 0x2452420, tiger_boss 0x2452a90; old externs became macros (D3403).
- living_combat::update's argument is a move id, not a float frame time; declaration left unchanged (D3404).
- Return types: the three *_seat_rider return bool, tiger_move_in_range int, combat_approach bool; ai_state
  +0x68 is the cast point (D3405).

## Game bugs (new; for docs/game-bugs)

1. ai_group_remove_member (0x492ca0) writes -1 to members[6], which is the group's enemy count (+0x1c).
2. ai_group_collect_enemies (0x493ac0): beyond its detection range a member notices an enemy only when the
   dot product with its forward vector is below 0.71, i.e. outside the 45-degree cone in front of it
   (looks inverted; behaviour as coded).
3. ai_group_think_style1 (0x493290): after ordering an attack it also orders an attack on the first enemy
   (missing return), so support members end up on enemy 0.
4. ai_group_think_caster (0x4936c0): the candidate count is incremented twice per scored member, and its
   five-entry candidate array overflows for a six-member group.
5. snow_update (0x494ba0): cells scrolled in on the +x side are refilled only when snow_cell::update returns
   true (inverted test), so they keep their old flakes.
6. serpent_boss_load (0x495960): serpent_has_rider is set before the 0x640-byte clear, so it is always false
   after the first load (the rider is not seated until serpent_set_ride_anims).
7. serpent_choose_attack (0x496330): after re-picking a target in stage 1, serpent_part_is_asleep gets the
   previous asleep flag (0/1) as the candidate index.
8. serpent_illusion_start (0x496ba0): deactivates parts 1..num-1 only (other loops use num+1).
9. serpent / phoenix / tiger *_rider_set_flinch: the "bad $Action" message is sprintf'd into a local buffer
   and never shown.
10. phoenix_reposition_update (0x497da0): when the side point has no path it paths to the negated point
    (-dest), not to the mirrored side point.
11. phoenix_seat_rider (0x4982a0): reads and writes the mount's own animation time; the rider is never synced.
12. phoenix_special_strike (0x499890), spell 0x1e: tests the target-to-phoenix distance instead of each
    member's, hits the target once per party member and knocks down every member.
13. phoenix_set_foot_anims (0x499be0): the ready_hurt lookup is overwritten (no store).
14. tiger_pick_target (0x49b0a0): the threat base is assigned per candidate (last one wins), not summed.
15. tiger_set_stage2_anims (0x49a140): the stand slots get the stage-2 walk anim; ride_horse_ready_axe is
    loaded but never used.
16. tiger_unload (0x49ae90): frees tiger_axe without clearing the pointer.

## Doc disagreements (bosses.md / cutscenes.md / effects.md not edited)

- sum.symbols / docs name 0x243e394 "members" and 0x243e3c0 "enemies"; the contents are swapped (D3400).
- serpent_whip_reach (0x2452160) is not a separate global but attacks[2][0].range_max of serp_boss.tbl.
- The cutscene camera "time_left / advance" are not cutscene-shot methods (D3401); types.h
  cutscene_camera_shot's note that time_left "subtracts fov as elapsed time" describes the other class.
- 0x22e5e98 in phoenix_special_strike is spells[0x1e].distance and 0x22e6b18 in tiger_move_in_range is
  spells[0x37].distance (fields of the spell table, not loose globals).
- The ai.cpp toggle help string ends "toggles it.\n" while 0x588c50 in this range reads "toggles it\n"
  (check ai.cpp's cmd_pause_ai literal).

## Open questions

- Extents of the phoenix and tiger state objects are taken from the serpent's pattern (no clear in one go).
- tiger_boss +0xb4..+0xbf and serpent_boss +0x00c / +0x54c / +0x624 have no reader in these units.
- serpent part busy check (0x4960f0) repeats serpent_part_is_busy instead of calling it; source or inlining?
- Static .bss order versus definition order (as in the other units): globals are defined in address order.

## Left

Nothing in the assigned range. Neighbouring unwritten code used here: ai_follow_path, ai_turn_towards,
ai_combat_* (0x4b3010-0x4be760), ai_choose_support_spell 0x4bea80, ai_alert_guards_of_target 0x4bfed0,
boss_target_list_find(_b) 0x4c3c70 / 0x4c4580, anim_track / tcb_ease (0x550f90-0x551610), gr_bitmap_3d.
