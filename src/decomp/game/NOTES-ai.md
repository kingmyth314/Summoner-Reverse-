# ai.cpp (decomp39)

`game/ai.cpp`: Sum.exe v1.40 code 0x471ff0-0x475178, written from the disassembly (`tools/asm.mjs fn`,
jump tables and data read from Summoner/Sum.exe). Headers: `include/` (copy of src/decomp/include at
b3482e7 plus the changes in CHANGED-HEADERS.txt, decisions D1900-D1908 in include/DECISIONS.md).

**110 functions written: the 107 of module ai.cpp (all of them) plus the three cast_effect_* functions
that follow it (D1905). Nothing is left.** Confidence: 76 exact, 30 likely, 4 unsure (ai.cpp alone:
74 / 29 / 4).

Syntax check: `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include
game/ai.cpp` is clean, and every src/decomp/game and src/decomp/vsdk .cpp gives the same output against
this include copy as against the repo's (only their existing warnings).

## Functions

| Address | Name | Conf. | Note |
|---|---|---|---|
| 0x471ff0 / 0x472000 | init_empty_472000(_thunk) | unsure | one of the two vector3 objects (empty ctor) |
| 0x472010 / 0x472020 | init_empty_472020(_thunk) | unsure | the other (ai_directions / ai_move_dir: assignment open) |
| 0x472030 / 0x472040 | init_realtimetimers_22e48e0(_thunk) | likely | ai_profile's generated ctor (D1900) |
| 0x472060 / 0x472070 | init_strptrs_22e49a0(_thunk) | likely | ai_combat_style_names (XSTR_SLOT 0..5) |
| 0x4720b0 / 0x4720c0 | init_ccmd_ai_frame_update_rate(_thunk) | exact | |
| 0x4720e0 | cmd_ai_frame_update_rate | exact | value never read |
| 0x472160 / 0x472170 | init_ccmd_slowdown_scaler(_thunk) | exact | |
| 0x472190 | cmd_slowdown_scaler | exact | |
| 0x472200 | ai_state::call_for_help | likely | team / type test is a branch-free AND |
| 0x4722f0 | ai_state::ai_state | exact | GameTimer members only |
| 0x472380 | ai_state::init | exact | memset(attack_point_users, -1) |
| 0x472420 | ai_state::shutdown | exact | |
| 0x472450 | ai_state::reset | likely | species style table loop |
| 0x4725e0 | ai_state::reset_party_member | exact | |
| 0x4726a0 | count_attack_slots_used | exact | |
| 0x4726d0 | attack_slot_direction | exact | jump table 0x472768 |
| 0x472780 | pick_attack_slot | likely | dot operand order |
| 0x472840 | claim_attack_slot | likely | (self, attacker, ms, slot) |
| 0x4729b0 | offset_point_sideways | likely | (pos, offset vector) -> travel_dest |
| 0x472a90 | ai_frame | likely | cdecl (D1901), profile_label, jump table 0x472e40 |
| 0x472e80 | ai_update_perception | likely | cdecl; really an activity / view cull |
| 0x4730c0-0x473200 | is_traveling_sub0/1, is_following_leader, is_combat_e_or_sneak_attack, is_attacking, is_combat_sub10, is_combat_sub16_17, is_combat_sub16, is_casting, is_combat_sub11, is_mode | exact (sub1 likely) | |
| 0x473230 / 0x473240 | init_ccmd_attack_point(_thunk) | exact | |
| 0x473260 | cmd_attack_point | likely | store between the range tests |
| 0x4732d0 / 0x4732e0 | init_ccmd_offense_point(_thunk) | exact | |
| 0x473300 | cmd_offense_point | exact | |
| 0x473320 | ai_state::set_mode | likely | byte jump table 0x4735f0 / 0x4735e0 |
| 0x473600 / 0x473610 / 0x473620 | get_mode, set_18, get_combat_style | exact | |
| 0x473660 | path_get_destination | exact | |
| 0x4736a0 | count_engaged_attackers | likely | |
| 0x473750 | count_attackers | exact | |
| 0x4737a0 | can_backstab_target | likely | dot operand order |
| 0x473880 / 0x4738c0 / 0x473940 | go_inactive, go_idle, go_mount_idle | exact | go_idle's mount recursion is a loop (tail recursion) |
| 0x473970 / 0x4739e0 | begin_emerge, class_die | exact | |
| 0x473a40 / 0x473a90 | travel_to_xyz, travel_to_point | likely | |
| 0x473ac0 | path_begin | likely | |
| 0x473bc0 | set_path | exact | |
| 0x473c40 / 0x473d80 / 0x473e50 | path_find_new, path_find_append, path_to | likely | |
| 0x473f40-0x474190 | add_travel, add_attack, add_cast_spell, add_cast_at_target, add_cast_at_point, add_move_to, add_move_to_timed | exact | set_combat_mode_goal (0x473fd0) likely |
| 0x4741d0 | add_skill_from_behind | likely | |
| 0x4742b0-0x474480 | knock_down_local, knock_down, disable_2f_local, disable_2f, disable_31, add_hide, add_sneak | exact | |
| 0x4744a0-0x474690 | attack_target, attack_target_sub10, net_attack_target, start_travel, charge_target, travel_to_current_dest, enter_sneaking, enter_hiding | exact | |
| 0x4746d0 / 0x4747e0 | save, load | likely | floats through a stack temporary |
| 0x474900 | ai_global_init | likely | atexit of an empty function (folded) |
| 0x474950 | ai_do_frame | likely | jump tables 0x474cac / 0x474cc4 |
| 0x474ce0 / 0x474cf0, 0x474d10 | init_ccmd_ai_state, cmd_ai_state | exact | |
| 0x474de0 / 0x474df0, 0x474e10 | init_ccmd_pause_ai, cmd_pause_ai | exact | |
| 0x474ed0 / 0x474ee0, 0x474f00 | init_ccmd_ai_wander, cmd_ai_wander | exact | |
| 0x474fc0-0x475030 | enter_combat, is_disabled_2c, is_knocked_down, is_disabled_2e, is_disabled_any (likely), can_be_knocked_down | exact | |
| 0x475090 / 0x4750b0 / 0x475160 | cast_effect_tracks_reset, cast_effect_track_add (likely), cast_effect_track_count | exact | module "-" (D1905) |

## Data

.data 0x59ad60-0x59b7c7: ai_state_names[13], ai_substate_names[65], ai_goal_type_names[18],
ai_species_styles[14] + num_ai_species_styles, slowdown_scaler 4.0, Ai_path_wandering_enabled, the byte
0x59af59 (true; ai_follow_path), ai_frame_update_rate 8, Ai_attack_point / Ai_offense_point -1, then the
literals. .bss 0x22e47d0-0x22e49b7: the seven console commands, ai_directions[16], ai_move_flags,
ai_prof (0x50), ai_move_dir, cast_effect_tracks[5], Ai_paused, ai_do_frame_active, ai_combat_style_names.
Names not in sum.symbols are ours (marked in ai.h).

## Open questions

- profile_label's text in ai_frame (the function name is used, as pathfinding.cpp).
- Which of the two empty initialisers (entries 218 / 219) belongs to ai_directions and which to
  ai_move_dir; the definition order of the plain .bss variables.
- The AIS_* table lacks five values between 0x21 and 0x27 (D1906); 0x1f and 0x3f have no name. The
  first table entry that is wrong is AIS_SPELL (0x22 in the table, 0x27 in the code).
- goal_add's eighth (float) argument: only add_move_to_timed passes a value (boss_release_mount: 30.0f,
  with 5000 as the priority).
- path_to's fifth argument is not read; path_find_new adds char_info +0x260 (named head_height by
  decomp38) to the cost bias, which looks like a different field.
- ai_frame multiplies Character +0x6b0 (slowdown_rate) by slowdown_scaler (4.0) every frame while
  ai_state flag 0x10000000 is set; presumably character_ai_move_frame resets it.
- Return types: is_disabled_any returns through `mov eax, 1` / `xor eax, eax` but its callers test AL
  (kept bool); travel_to_point / enter_combat may return set_mode's value (`call; ret`).

## docs/sum-exe/ai.md (and sum.symbols) against the code

- "Per-frame update": ai_do_frame is not time-budgeted and does not spread the work over frames. Every
  living entity (type 4 always, 5 / 7 / 9 unless dead) gets ai_frame every frame; the RealtimeTimer and
  timer_get only drive the 500 ms profiler averages. ai_frame_update_rate is never read.
- "ai_set_mode ... frees the current path" / "ai_path_free ... including mode changes": ai_set_mode does
  not touch the path; it releases the handle slot at +0x80 (handle_slot_release). sum.symbols' "frees
  path +0x80" is the same mistake.
- ai_update_perception is not a perception pass: it decides whether the character's AI runs this frame
  (off-screen and inactive characters are skipped, the multiplayer host runs idle monsters on a 500-750 ms
  timer) and resets the accumulated frame time when it runs. ai_frame and it are cdecl on the character,
  not thiscall ai_state (D1901).
- Goals: 0x6c is not a priority but the spell argument of the non-cast goals; the priority is the
  caller's (ai_goal_add_travel's third argument). The goal type names are in the game's table 0x59ae98
  (9 "pathfind", 0xc "goto_navpoint", 0xd "wander", 0xe "backstab", 0xf "sneak", 0x10 "hide", ...).
  ai_goal_add_move_to_timed's 5000 at boss_release_mount is the goal priority, not 5 seconds.
- Attacker slots: ai_pick_attack_slot does not look for a free slot, it takes the best direction;
  ai_claim_attack_slot replaces an occupant only when the occupant has more time left.
- Disabled: ai_knock_down_local sets sub 0x2c ("AIS_TRIPPED_FALL" by the shifted table), not 0x2d;
  ai_is_knocked_down tests 0x2d. sum.symbols' ai_knock_down_local comment says 0x2d.
- Combat sub 0x11 is AIS_BACKSTAB (the doc lists it as another combat step; ai_net_attack_target uses it
  on the host); the doc's sub-mode list should name the AIS_* values from the table (D1906).
- Saving: only the combat style and, for NPCs and monsters (types 4 / 5), the five goals are saved;
  targets are stored as name hashes.
- sum.symbols: ai_path_to returns 0 / -1, not the path; ai_count_engaged_attackers' goal is type 1
  (attack) found with mask 4, not "type 4"; init_realtimetimers_22e48e0's timer is not used by
  mp_handle_kick_pl; cast_effect_* are ai.cpp's (module "-").

## Game bugs noticed

- **cmd_ai_state never works**: it compares the name table's pointers with next_arg's address instead of
  the strings (0x474d4f), so every name gives "Error: Invalid state name" (debug console only).
- **Loaded goal targets by name**: ai_state_load resolves a goal's target by the name hash of the first
  living entity with that name, so after a load a monster's attack / cast goal can point at another
  monster of the same kind.
- **Knock-down while down**: a second knock-down restarts the mode timer but keeps the first duration
  (disable_ms is not updated), so a longer knock-down is cut to the old length.
- **Dead console variables**: ai_frame_update_rate, attack_point and offense_point are stored and never
  read; cmd_attack_point also accepts an index equal to the attack point count (one past the end).
- ai_pick_attack_slot ignores whether a slot is taken, so attackers coming from the same side share it
  (possibly by design).
