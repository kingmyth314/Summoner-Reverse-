# decomp37: living_entity.cpp (Sum.exe v1.40)

Output: game/living_entity.cpp (concatenation of game/p*.cpp by build.sh). 166 of 166 functions written
(sum.symbols module living_entity.cpp, 0x44d9f0-0x463480): 97 exact, 69 likely, 0 unsure.
Compiles with `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security
-I decomp37/include` (0 errors; 4 warnings, all faithful to the binary: two set-but-unused locals, an
unused fx handle, and the "jade dmg: %d" printf given a double, see game bugs). The 62 src/decomp .cpp of
b5e94ab still compile against decomp37/include. Header changes: CHANGED-HEADERS.txt, headers.patch.
Decisions: D1700-D1712 in include/DECISIONS.md.

## Build observations

- Unoptimised (/Od) except 0x461dc0-0x46353f (character_say ... cmd_death): no frames, 0x90 padding
  (D1712). Kept in the unit: strings.tbl ids 314-352 and the .data/.bss interleave.
- Inline functions expanded (vector3 =, +=, -, len, len_sq, matrix =, lobj_ray ctor); nested calls stay
  out of line (0x410cd0, 0x413190, 0x43b580); COMDATs 0x45b680 / 0x45b6b0 called where the inline budget
  runs out (clip_move_to_navmesh).
- XSTR shows the `0 ? text : localize_strings[id]` test; redefined in the unit (D1701).
- profile_label objects appear only as EH states (unwind maps, e.g. 0x583a80 for find_ground).
- .data order: 0x599644 Character_los_enabled = 1, 0x599648 locomotion_anim_names, 0x599690
  Chance_to_drop_gold = 0.25, 0x599694 Use_spell_ap = 1, 0x599698 {250, 312, 400}, 0x5996a4 {6, 6, 2, 2};
  Debug_death = 1 at 0x59a16c (between the Spell_ap and bloodspots commands).

## Open questions

- Whether the optimised tail is `#pragma optimize` or a second object (D1712).
- Frame layout of the many case locals in get_hit_by_spell (flow likely, locals unsure).
- lobj_hit +0x24 meaning (face index vs lobj), +0x14..+0x20 unknown.
- combat union forces combat.construct() in the Character ctor body (D1702); split the D1 union to get
  the binary's construction order.
- hero_model_prefix returns an int mask but is declared const char * for two pilots (D1711).

## Doc disagreements (docs/ left untouched)

- characters.md: "Ids 7, 8 and 14-16 exist (max 0x10)". There are 18 slots, ids 0-17; 7 Light,
  8 Invisible, 14 Firewall, 15 Blizzard, 16 Ice coffin, 17 Dark wall are all handled by status_effect_add.
- combat.md: character_kill "records the killer (+0xa4)". +0xa4 is the death-move sub id (0x22 default);
  the "dead flag 0x8" it sets is the dying flag, dead (0x80) is set later in on_death.
- characters.md add_xp: the "+Level!" floating text is shown for every character; ring XP is given only
  in single player with summon skill > 0, and rings_add_xp_all gives 1/8 to every ring.
- combat.md: the resist test is `roll <= resist` only in the status-effect block; most spell handlers
  in get_hit_by_spell use `roll < resist`.

## Game bugs (in the binary, reproduced in the source)

1. add_xp quiet path: infinite loop when skill points remain but no progression entry is eligible.
2. character_size_height computes the boss heights and discards them.
3. get_hit_by_spell: `amount` stored then overwritten by spell_roll_amount (dead stores) for spells
   0x3e leech, 0x40-0x42 ice/fire/energy shield, 0x66 Laharah's scream, 0x68-0x6b dragons (non-party
   targets), 0x0c death, 0x54 Ghost death, 0x57 Urath death, 0x65 Dark void.
4. get_hit_by_spell spell 0x18 (Jade Beam): the type-9 / flag-0x40 branch keeps the fireball timer test
   (spell == 0x1a), which can never be true there; the other branch (half the current HP as damage)
   prints "jade dmg: %d" with a double argument (wrong number in the debug output).
5. Ring unlock needs XP > 60000; exactly 60000 never unlocks.
6. load_state, species 0xce: sets attachments[0].slot = 1, probably meant the count.
7. walk_to_point normalises a direction and never uses it.
8. move_trigger_time_left / move_has_trigger do not NULL-check the move or track ended moves.

## Functions

| Address | sum.symbols name | Confidence |
|---|---|---|
| 0x44d9f0 | sinit_living_entity_8eca60 | exact |
| 0x44da00 | init_living_entity_8eca60 | exact |
| 0x44da10 | register_dtor_living_entity_8eca60 | exact |
| 0x44da30 | dtor_living_entity_8eca60 | exact |
| 0x44da40 | sinit_living_entity_8ed1e8 | exact |
| 0x44da50 | init_living_entity_8ed1e8 | exact |
| 0x44da60 | register_dtor_living_entity_8ed1e8 | exact |
| 0x44da80 | dtor_living_entity_8ed1e8 | exact |
| 0x44db50 | sinit_living_entity_pool_8ed980 | exact |
| 0x44db60 | init_living_entity_pool_8ed980 | exact |
| 0x44db90 | register_dtor_living_entity_pool_8ed980 | exact |
| 0x44dbb0 | dtor_living_entity_pool_8ed980 | exact |
| 0x44da90 | init_gametimer_array_93ff80_thunk | exact |
| 0x44daa0 | init_gametimer_array_93ff80 | exact |
| 0x44daf0 | init_gametimer_array_941514_thunk | exact |
| 0x44db00 | init_gametimer_array_941514 | exact |
| 0x44dbd0 | spell_cooldown_start | exact |
| 0x44dc70 | character_start_spell_recovery | likely |
| 0x44dd30 | character_spell_on_cooldown | exact |
| 0x44ddb0 | character_scale_stats_to_level | likely |
| 0x44e060 | living_entity__living_entity | likely |
| 0x44e150 | living_entity__dtor | likely |
| 0x44e1c0 | living_entity_init | likely |
| 0x44ed90 | character_get_name | exact |
| 0x44edb0 | character_set_name | exact |
| 0x44edf0 | living_entity__vfunc0 | likely |
| 0x44f020 | character_is_in_view | likely |
| 0x44f1a0 | character_place_at_party_start | likely |
| 0x44f5f0 | character_place_at_marker | likely |
| 0x44f880 | character_has_line_of_sight_to_point | likely |
| 0x44fa20 | character_has_line_of_sight_to | likely |
| 0x44fab0 | character_add_xp | likely |
| 0x44ffc0 | ring_add_xp | likely |
| 0x450220 | ring_check_summon_unlock | exact |
| 0x450420 | rings_add_xp_all | likely |
| 0x450850 | character_add_hp | likely |
| 0x450c70 | character_add_ap | likely |
| 0x450d90 | character_kill | likely |
| 0x450e30 | monster_drop_loot | likely |
| 0x4530d0 | character_update_head_look | likely |
| 0x4536c0 | summon_return_lent_max_hp | likely |
| 0x4537f0 | character_on_death | likely |
| 0x454070 | character_resurrect | likely |
| 0x4542b0 | character_size_radius | exact |
| 0x4542f0 | character_size_height | exact |
| 0x454330 | character_distance_between_edges | likely |
| 0x454610 | character_set_team | exact |
| 0x454640 | character_set_team_mask | exact |
| 0x454670 | character_set_flag_2_4 | exact |
| 0x4546b0 | character_clear_flag_2_4 | exact |
| 0x454700 | status_effect_add | likely |
| 0x454ff0 | status_effect_add_from_item | likely |
| 0x4553a0 | status_effect_cure | likely |
| 0x455950 | character_has_status | exact |
| 0x455990 | character_status_active | exact |
| 0x4559e0 | character_status_time_left | exact |
| 0x455a30 | character_status_value | exact |
| 0x455a50 | character_status_set_1c | exact |
| 0x455a70 | character_status_set_8 | exact |
| 0x455a90 | character_clear_all_effects | exact |
| 0x455b20 | character_refresh_item_status_effects | likely |
| 0x455e00 | character_ai_travel_offset | likely |
| 0x455e70 | character_walk_to_point | likely |
| 0x455f50 | character_ai_add_goal9 | exact |
| 0x455fb0 | character_find_ground | likely |
| 0x456750 | character_probe_lobj_ground | likely |
| 0x4568e0 | character_update_layer | exact |
| 0x4569c0 | character_find_overlap | likely |
| 0x456ed0 | character_anim_speed_at_blend | likely |
| 0x456f80 | character_get_698 | exact |
| 0x456fb0 | character_get_forward_velocity | likely |
| 0x457060 | character_get_speed | likely |
| 0x4570d0 | character_set_6ac | exact |
| 0x4570f0 | character_boss_anim_remap_in | exact |
| 0x457120 | character_boss_anim_remap_out | exact |
| 0x457150 | character_save_state | likely |
| 0x4587f0 | character_save_state_size | exact |
| 0x458800 | character_load_state | likely |
| 0x45a120 | character_update_locomotion_anim | likely |
| 0x45a6e0 | character_set_locomotion_speed | likely |
| 0x45a840 | character_stopping_distance | likely |
| 0x45a930 | character_distance_to_reach_blend | likely |
| 0x45aa20 | character_distance_to_reach_speed | likely |
| 0x45ab00 | character_clip_move_to_navmesh | likely |
| 0x45b6e0 | character_update_status_effects | likely |
| 0x45bf70 | character_update_regen_and_status | likely |
| 0x45c340 | character_ai_move_frame | likely |
| 0x45ccb0 | character_update_render_flag | exact |
| 0x45cd00 | living_entity_move_frame | likely |
| 0x45d4c0 | character_find_foot_bones | likely |
| 0x45d540 | character_play_footstep | likely |
| 0x45d690 | character_update_footsteps | likely |
| 0x45d830 | character_get_attack_range | likely |
| 0x45d980 | character_can_cast_spell | exact |
| 0x45da60 | character_perform_spell_cast | likely |
| 0x45dd80 | character_start_move_fade | exact |
| 0x45de20 | character_play_flinch | exact |
| 0x45dea0 | living_entity_get_hit_by_spell | likely |
| 0x460190 | spell_apply_hit_effects | likely |
| 0x460f20 | character_play_move | likely |
| 0x461370 | character_find_move | exact |
| 0x4613b0 | character_find_move_alt | exact |
| 0x4613f0 | character_move_progress | likely |
| 0x461450 | character_move_playing | exact |
| 0x4614d0 | character_move_finished | exact |
| 0x461500 | character_get_current_move | exact |
| 0x461540 | character_pick_talk_move | exact |
| 0x461620 | character_move_has_trigger | exact |
| 0x461670 | character_move_trigger_time_left | exact |
| 0x4616f0 | character_move_trigger_mark | exact |
| 0x461740 | character_move_trigger_fired | exact |
| 0x4617f0 | character_is_badly_hurt | exact |
| 0x461840 | character_get_move_effect | exact |
| 0x461860 | init_ccmd_immortal_thunk | exact |
| 0x461870 | init_ccmd_immortal | exact |
| 0x461890 | cmd_immortal | exact |
| 0x461980 | init_ccmd_health_thunk | exact |
| 0x461990 | init_ccmd_health | exact |
| 0x4619b0 | cmd_health | likely |
| 0x461ad0 | character_is_enemy | exact |
| 0x461b60 | character_get_team | exact |
| 0x461b80 | living_entity_create | likely |
| 0x461dc0 | character_say | likely |
| 0x461fe0 | character_set_floating_text | exact |
| 0x462070 | character_get_display_name | likely |
| 0x462200 | character_set_ai_active | exact |
| 0x462260 | character_set_speed_scale | exact |
| 0x4622b0 | character_get_move_fade_alpha | likely |
| 0x462380 | character_is_main_hero | exact |
| 0x462400 | character_skill_allowed | exact |
| 0x462440 | character_get_skill | likely |
| 0x462540 | character_skill_equip_mod | likely |
| 0x462660 | character_resist_equip_bonus | exact |
| 0x4626b0 | character_get_resistance | likely |
| 0x4627c0 | character_equip_bonus | exact |
| 0x462810 | character_ap_regen_mult | exact |
| 0x462840 | character_get_max_stat | exact |
| 0x4628a0 | character_apply_hero_model | likely |
| 0x462a20 | character_set_underwear_textures | likely |
| 0x462c80 | character_set_armour_alt_texture | likely |
| 0x462d20 | character_get_mount | exact |
| 0x462d40 | character_get_rider | exact |
| 0x462d60 | character_is_mount | exact |
| 0x462d70 | character_is_rider | exact |
| 0x462d80 | living_entity_reset_tables | exact |
| 0x462de0 | character_ai_suspended | likely |
| 0x462e40 | character_is_moving | exact |
| 0x462ec0 | character_hero_model_prefix | likely |
| 0x463050 | init_ccmd_chance_gold_thunk | exact |
| 0x463060 | init_ccmd_chance_gold | exact |
| 0x463080 | cmd_chance_gold | exact |
| 0x463110 | init_ccmd_kill_summon_thunk | exact |
| 0x463120 | init_ccmd_kill_summon | exact |
| 0x463140 | cmd_kill_summon | exact |
| 0x463180 | init_ccmd_no_resist_thunk | exact |
| 0x463190 | init_ccmd_no_resist | exact |
| 0x4631b0 | cmd_no_resist | exact |
| 0x463270 | init_ccmd_Spell_ap_thunk | exact |
| 0x463280 | init_ccmd_Spell_ap | exact |
| 0x4632a0 | cmd_Spell_ap | exact |
| 0x463360 | init_ccmd_bloodspots_thunk | exact |
| 0x463370 | init_ccmd_bloodspots | exact |
| 0x463390 | cmd_bloodspots | exact |
| 0x463450 | init_ccmd_death_thunk | exact |
| 0x463460 | init_ccmd_death | exact |
| 0x463480 | cmd_death | exact |
