# Pilot notes: boss_ai.cpp (decomp29)

Source written from the Sum.exe v1.40 disassembly (`node tools/asm.mjs fn`; the byte-indexed jump tables
decoded from the raw bytes of `Summoner/Sum.exe`; floats read the same way). Summoner 2 and Dash Faction
were not used. Header base: the include tree of d790a53, rebased during the work onto each merge that
landed (a38189f, 6b657d4, 851799b, 824a635); CHANGED-HEADERS.txt has the diff against 824a635.

`game/boss_ai.cpp` passes `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings
-Wno-format-security -I include` with no warnings, and every `.cpp` under `src/decomp/game` and
`src/decomp/vsdk` gives the same warning counts against this header copy as against the repo's
(new_status 2, objects 3, character_instance 9, matrix 5, skeleton 3, window 2; all 57 files). Decisions D900-D909
are appended to `include/DECISIONS.md`; `CHANGED-HEADERS.txt` lists the header edits.

## boss_ai.cpp (0x4a04b0-0x4a3575), /O2: 59 of 59 written, 42 exact / 17 likely / 0 unsure

`level_page_in_characters` (0x4a3580) is also tagged boss_ai.cpp in sum.symbols but starts at the unit
end the task gives; not written.

| Address | Function | Confidence |
|---|---|---|
| `0x4a04b0` | static_init_boss_rider_sound_timer | exact |
| `0x4a04c0` | init_boss_rider_sound_timer | exact |
| `0x4a04d0` | boss_ai_unload | exact |
| `0x4a0600` | boss_ai_load | exact |
| `0x4a0730` | boss_ai_check_stage | exact |
| `0x4a08c0` | boss_ai_idle_dispatch | exact |
| `0x4a09f0` | boss_ai_frame | exact |
| `0x4a0b40` | boss_cast_special_dispatch | likely |
| `0x4a0ca0` | boss_attack_trigger_dispatch | exact |
| `0x4a0e60` | boss_move_target_in_range | exact |
| `0x4a0ea0` | boss_move_in_range_dispatch | likely |
| `0x4a1090` | boss_attack_stuns_dispatch | likely |
| `0x4a1150` | boss_attack_knocks_down_dispatch | likely |
| `0x4a1200` | boss_get_threat_table | exact |
| `0x4a1310` | boss_threat_decay | exact |
| `0x4a13b0` | boss_threat_add | likely |
| `0x4a14f0` | species_is_boss | exact |
| `0x4a15a0` | charinfo_boss_height | exact |
| `0x4a16a0` | charinfo_boss_radius | exact |
| `0x4a1790` | boss_ai_parse_table | likely |
| `0x4a1ac0` | boss_stage_attack_enabled_dispatch | likely |
| `0x4a1b70` | boss_anim_remap_in | exact |
| `0x4a1be0` | boss_anim_remap_out | exact |
| `0x4a1c50` | boss_turn_rate_override | likely |
| `0x4a1d90` | charinfo_index_from_species | exact |
| `0x4a1dc0` | boss_ensure_mount | likely |
| `0x4a1ef0` | boss_mount_pick_flee_point | likely |
| `0x4a1fe0` | boss_release_mount | likely |
| `0x4a2070` | boss_get_attack_move_id | exact |
| `0x4a21b0` | character_target_point_count | exact |
| `0x4a2230` | boss_counter_hit_dispatch | exact |
| `0x4a2300` | boss_set_ride_anims | exact |
| `0x4a2360` | boss_script_hook_stub | exact |
| `0x4a23c0` | boss_set_foot_anims | exact |
| `0x4a2420` | boss_dismount_dispatch | exact |
| `0x4a2480` | boss_species_mounted_stage | exact |
| `0x4a24c0` | species_is_boss_b | exact |
| `0x4a2560` | boss_ai_do_frame_hook | exact |
| `0x4a2590` | boss_clear_current_move | exact |
| `0x4a26f0` | boss_get_move_range | exact |
| `0x4a2870` | boss_get_attack_range | likely |
| `0x4a28b0` | serpent_part_on_hit | exact |
| `0x4a28f0` | character_is_serpent_part | exact |
| `0x4a2920` | boss_attack_test_dispatch | likely |
| `0x4a2b80` | boss_attack_resolve_dispatch | exact |
| `0x4a2d40` | character_is_not_boss | exact |
| `0x4a2d50` | boss_move_trigger_dispatch | exact |
| `0x4a2d70` | character_uses_pathfinding | exact |
| `0x4a2da0` | boss_hook_dispatch_51_c1 | exact |
| `0x4a2de0` | boss_threat_find | exact |
| `0x4a2e00` | boss_target_filter_dispatch | exact |
| `0x4a2f60` | boss_target_list_build | likely |
| `0x4a31c0` | boss_target_list_pick | likely |
| `0x4a33b0` | rider_unlink_mount | likely |
| `0x4a3410` | rand_int_range | exact |
| `0x4a3440` | rand_weighted_index | likely |
| `0x4a34d0` | boss_rider_sound_timer_reset | exact |
| `0x4a34e0` | boss_rider_random_sound | exact |
| `0x4a3570` | level_preload_effects | exact |

Data owned here: `boss_rider_sound_timer` (.bss 0x245c9e0) and the .data literals 0x59ccb8-0x59cd87
(0.86f, the table keywords, the path string). Everything else the unit reads is per-boss .bss
(declared extern in boss_ai.h).

## How the source was shaped

- Case bodies are in the order the binary lays them out (VC6 keeps source order); the case values come
  from the decoded byte tables. Tail jumps (`jmp per_boss_fn`) are plain calls in a `break` case.
- Switches whose table is wider than its real cases get the bounding species as cases that share the
  default (D908): boss_cast_special_dispatch (Luminar), stun / knock-down tests (Luminar, Evil Joseph),
  boss_stage_attack_enabled_dispatch (Luminar, Machival).
- Bool results built from x87 compares come out as `mov eax, 1` / `xor eax, eax` (the callers test AL),
  so they are written `return a < b;`.
- boss_ai_parse_table has an unused `File` local: its constructor / destructor bracket the TextFile in
  the EH states, so it is in the source. `msg` and `spell_name` share a frame slot (+0x7c), so they are
  in disjoint block scopes.
- boss_target_list_build's candidate block appears twice in the binary (group loop, party loop); it is
  written out twice, not as a helper.

## Disagreements with docs/sum-exe/bosses.md (and sum.symbols)

1. **Counter hit** (bosses.md "Special mechanics"; sum.symbols 0x4a2230 "(boss, attacker)"): the first
   argument is the boss as attacker, and the move's spell goes to the *defender*. It is the table flag
   +Plus Spell: (0x10) on the boss's own melee hit, not a counter on the melee attacker. Ghost death
   (0x54) is a direct hit; the Ghost's other spells are an effect (fx_create_spell_effect_at, 1 s).
2. **Table format**: "$Offense point:" takes a quoted number (atoi - 1 is stored as the point index),
   not a mesh tag name, and each record starts with it (no "<move>:" keyword). "$Num Stages:" is read
   and dropped; the stage number indexes the table without a bounds check.
3. **Heights**: "Size and radius come from charinfo_boss_height / _radius". The radius is used, but
   character_size_height (0x4542f0) drops charinfo_boss_height's result (`fstp st(0)` at 0x45430f) and
   returns the size table: the boss heights are never used. charinfo_boss_height's default is 1.0, the
   size table only for the riders, Murod, Murod Oni and Evil Joseph (sum.symbols says "else the size table").
4. **"ai_retaliate_on_hit skips bosses"**: it calls species_is_boss_b (0x4a24c0), which leaves out
   Luminar, the Serpent Rider and Giant Salamanka.
5. **Shared helpers range** "0x4a04b0-0x4a3500": the unit ends at 0x4a3575 (level_preload_effects).
6. sum.symbols, character_target_point_count: the count is forced to 0 when the riders are **on foot**
   (tiger stage 3, the others stage > 1), not "in their mounted stages".
7. sum.symbols, boss_threat_add: argument order is (boss, amount, attacker, flag) (D904).
8. sum.symbols, fx_create_spell_effect_at: the timer is the 4th argument (D906); tentacle_get_attack_kind
   gets the Character, not a handle.
9. bosses.md is right that Murod Oni's stage check is empty; Evil Joseph's case also makes a second,
   empty call, and Giant Salamanka's idle case is empty.

## Game bugs noticed (not checked in a running game)

- **Boss heights unused** (character_size_height 0x4542f0, living_entity.cpp): result of
  charinfo_boss_height discarded; every boss uses the size-class height.
- **Phoenix mounted test reads the serpent's stage**: boss_species_mounted_stage (0x4a2480) uses
  serpent_stage (0x2451d98) for species 0x58 too; phoenix_stage is 0x2452420. ai_get_combat_style uses it.
- **Evil Urath's melee never lands** (probable): boss_move_in_range_dispatch has no 0xb1 case, so
  living_combat_resolve_hit returns false for any type-9 Urath melee hit. Whether Urath has melee moves
  was not checked.
- **Machival's area tests return false** (possible): boss_attack_test_dispatch sets hits but never
  returns true for 0xc5, so living_combat_update_boss_attack still runs the opponent's defence roll.
- **+Plus Spell before the range test** (possible): living_combat_resolve_hit calls boss_counter_hit_dispatch
  before boss_move_in_range_dispatch, so the Ghost / serpent move spell lands even when the melee hit is
  then rejected as out of range.
- boss_get_attack_move_id dereferences the current-move pointers without a NULL test (crash only if a
  caller runs it with no current move).

## Open questions

- game_flags 0x200 (summoner.h GF_LEADER_SOUND) also gates the rider mount sounds: probably a general
  "sound feedback on" bit.
- ai_target_query (ai_score_target's record): only `best` (+0x10) is read back here.
- Whether the bounding cases of D908 were real cases with other species.
- The folded functions' real names (D907) and whether luminar_ai_idle / tentacle_beast_ai_idle are
  members (they do not read ECX).
- Which units own the per-boss data declared in boss_ai.h (names from sum.symbols; ghost_cur_move 0x2480810
  and pyrul_cur_move 0x2480290 are ours).
