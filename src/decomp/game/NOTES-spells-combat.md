# Pilot notes: spells.cpp and combat.cpp (decomp23)

Source written from the Sum.exe v1.40 disassembly (`node tools/asm.mjs fn`; jump tables decoded from the
raw bytes of `Summoner/Sum.exe`; .data read the same way). Summoner 2 and Dash Faction were not used for
code. Header base: the include tree of commit 9c82c5a (copied before any other step); the repo has moved
on since (86d1792, decomp20's conversation units), so CHANGED-HEADERS.txt lists what to merge. Where
decomp20 already declared something this unit needs, the same text is used (DECISIONS.md D310).

Both files pass `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security
-I include` with no warnings, and every `.cpp` of 9c82c5a (game/ and vsdk/) still compiles against this
header copy with the same warnings as before (new_status 2, objects 3, character_instance 9, window 2).
Decisions D300-D310 are appended to include/DECISIONS.md.

Symbol counts are sum.symbols entries (thunk / initialiser pairs count as two).

## spells.cpp (0x479b60-0x47f857), /O2: 44 of 53 written, 30 exact / 14 likely / 0 unsure

| Address | Function | Confidence |
|---|---|---|
| 0x479b60 / 0x479b70 | init_strptrs_22e92a0 (`spell_descriptions`, 38 slots, "Unused." x5) | likely |
| 0x479ce0 / 0x479cf0 | init_strptrs_22e9338 (`spell_display_names`, 108 slots) | likely |
| 0x47a130-0x47a160 | sinit / init / register / dtor of `StringPool spell_string_pool(0x3000)` | exact |
| 0x47a170 / 0x47a180 | init_empty_47a180 (empty initialiser) | not reproduced |
| 0x47a190 | spells_parse_table | likely |
| 0x47a6c0 | spells_parse_sound | exact |
| 0x47a760 | spells_free_strings | exact |
| 0x47a770 | spells_reset_runtime | exact |
| 0x47a7c0 | spell_find_by_name | exact |
| 0x47a800 | spell_find_by_cast_id | exact |
| 0x47a830 | spell_get_group | exact |
| 0x47a870 | spell_get_cast_effect | exact |
| 0x47a880 | spell_get_effect_1c | exact |
| 0x47a890 | spell_get_hit_effect | exact |
| 0x47a8c0 | damage_type_to_status | exact |
| 0x47a9c0 | status_effect_modifier | exact |
| 0x47aa80 | spell_get_duration_ms | exact |
| 0x47ab60 | status_effect_duration | exact |
| 0x47abe0 | spell_prepare_cast (1275 instr.) | **not written** |
| 0x47bd30 | spell_spawn_creature (436) | **not written** |
| 0x47c510 | spell_compute_resist | likely |
| 0x47c5d0 | spell_cast_execute (1143) | **not written** |
| 0x47d420 | spell_cast_launch_effects (886) | **not written** |
| 0x47dfc0 | spell_effect_pool_full | exact |
| 0x47dfe0 | spell_play_cast_sound | exact |
| 0x47e020 | spell_play_hit_sound | exact |
| 0x47e090 | spell_play_sound_34 | exact |
| 0x47e100 | spell_place_ground_effect | likely |
| 0x47e280 | spell_find_ground_points | likely |
| 0x47e330 | spell_probe_ground_point | likely |
| 0x47e4d0 | spell_effect_obj_set_timer | exact |
| 0x47e590 | spell_spawn_hit_fx | exact |
| 0x47e680 | spell_missile_remove | exact |
| 0x47e6c0 | spell_launch_missile | likely |
| 0x47e830 | spell_fire_missile | exact |
| 0x47e8e0 / 0x47e8f0 | init_empty_47e8f0 (empty initialiser) | not reproduced |
| 0x47e900 | summon_create (640) | **not written** |
| 0x47f270 | summon_dismiss | exact |
| 0x47f2a0 | spell_target_value | likely |
| 0x47f4d0 | spell_roll_amount | likely |
| 0x47f660 | spell_effect_add | exact |
| 0x47f690 | spell_effect_alloc | likely |
| 0x47f6d0 | spell_effect_free | exact |
| 0x47f700 / 0x47f710 | init_ccmd_summon (`ccmd_summon`) | exact |
| 0x47f730 | cmd_summon | likely |

Left: the casting half (spell_prepare_cast, spell_spawn_creature, spell_cast_execute,
spell_cast_launch_effects, summon_create; about 4,400 instructions). They need the 0x34-byte cast
parameter block decoded (`spell_cast_params` is a placeholder of 13 ints) and the 108-way switches.

Data: .data 0x59b988 `spell_group_skill[7]`, 0x59b9a4 an unreferenced -1, 0x59b9a8 an unreferenced
capitalised class-name table, 0x59b9c8 the +Class: names, 0x59b9e8 "GenericHit01.vfx", 0x59b9ec an
unreferenced 0, 0x59b9f0 four {species, float} pairs read by summon_create (meaning open), then the
literals. .bss in address order: spell_missiles, spells, spell_effects, spell_string_pool, ccmd_summon,
spell_sound_path (0x22e925c), spell_groups, spell_descriptions, spell_display_names. 0x22e923c and
0x22e924c-0x22e925b are not referenced.

## combat.cpp (0x463540-0x468c77), /O2: 59 of 63 written, 38 exact / 20 likely / 1 unsure

| Address | Function | Confidence |
|---|---|---|
| 0x463540 | living_combat::send_net | likely |
| 0x4635e0 | living_combat::reset | exact |
| 0x463620 | living_combat::play_attack | exact |
| 0x463660 | living_combat::play_special | exact |
| 0x4636a0 | living_combat::play_flinch_secondary | exact |
| 0x463730 | living_combat::play_flinch | exact |
| 0x4637d0 | living_combat::pick_parry | exact |
| 0x463830 | living_combat::pick_dodge | exact |
| 0x463860 | combat_queue_counter_attack | exact |
| 0x4638c0 | living_combat::play_defense | exact |
| 0x463910 | combat_fire_weapon_missile | unsure (inline matrix copy) |
| 0x463b40 | combat_attack_rating | exact |
| 0x463b90 | combat_weapon_skill | exact |
| 0x463be0 | combat_has_weapon_skill | exact |
| 0x463c00 | living_combat::speed_share | exact |
| 0x463d00 | living_combat::ai_choose_special | exact |
| 0x463e20 | living_combat::pick_attack_move | likely |
| 0x463f30 | living_combat::begin_attack | exact |
| 0x463f70 | living_combat_begin_attack_thunk | likely |
| 0x463f80 | living_combat::ranged_attack | exact |
| 0x463fc0 | living_combat_test_defense | exact |
| 0x463ff0 | living_combat_resolve_hit_vs | exact |
| 0x464010 / 0x464020 | init_ccmd_Super_power | exact |
| 0x464040 | cmd_Super_power | exact |
| 0x464100 | living_combat::roll_defense | likely |
| 0x4645f0 | living_combat::play_skill_attack | exact |
| 0x464700 | combat_resolve_exchange | likely |
| 0x464cb0 | living_combat::defense_in_time | exact |
| 0x464d00 | combat_position_modifier | exact |
| 0x464de0 | character_get_protection | likely |
| 0x464f50 | combat_melee_damage | likely |
| 0x465df0 | combat_apply_melee_hit | likely |
| 0x4665f0 | combat_get_weapon_tag_position | likely |
| 0x466920 | character_get_bone_world_matrix | likely |
| 0x4669c0 | combat_spawn_hit_vfx | exact |
| 0x466a80 | combat_play_weapon_hit_sound | exact |
| 0x466ae0 | chain_attack_check_window | likely |
| 0x466d20 | living_combat::update_exchange | likely |
| 0x466e90 | combat_spawn_spark_vfx | exact |
| 0x466f00 | living_combat::update_attack | exact |
| 0x467020 | living_combat::update_ranged | likely |
| 0x4670a0 | combat_trigger_special_ability (455 instr.) | **not written** |
| 0x467670 | combat_trigger_reflex_ability (452) | **not written** |
| 0x467cb0 | living_combat::update_special | likely |
| 0x467db0 | character_break_paralysis_on_hit | exact |
| 0x467df0 | living_combat::resolve_hit | likely |
| 0x468160 | living_combat::flinch_if_idle | exact |
| 0x4681b0 | living_combat__ctor (written as construct(), D300) | likely |
| 0x4681d0 | set_owner | exact |
| 0x4681e0 | get_target | exact |
| 0x4681f0 | set_target | exact |
| 0x468210 | living_combat::update | likely |
| 0x468360 | living_combat_update_boss_attack (224) | **not written** |
| 0x4685c0 | living_combat::apply_net_exchange | likely |
| 0x468740 | is_busy | exact |
| 0x468750 | set_38 | exact |
| 0x468760 | blood_splot_place (234) | **not written** |
| 0x468a90 | vector3_scale_to = operator*(float, const vector3 &) | likely |
| 0x468ac0 | combat_spawn_blood | likely |
| 0x468c40 | set_special_skill_species89 | exact |
| 0x468c60 | get_special_skill | exact |
| 0x468c70 | set_40 | exact |

Left: the two ability triggers (the spell casts of char_info +0x3c / +0x44), the boss attack update and
blood_splot_place (vector temporaries and decal_place), about 1,370 instructions.

.bss: Super_power 0x94173d and ccmd_Super_power 0x941720 lie between living_entity.cpp's ccmd_death
0x941710, ccmd_bloodspots 0x941730 and Bloodspots 0x94173c (the static initialisers 198-200 run
bloodspots, death, Super_power): the two units' .bss is interleaved, as in the other pilots.

## Open questions

1. The two empty initialisers of spells.cpp (0x47a180 after the string pool, 0x47e8f0 after
   spell_fire_missile): objects with inline empty constructors, probably in the unreferenced .bss holes
   0x22e923c / 0x22e924c-0x22e925b. Not reproduced (as new_automap / new_inventory).
2. The inline / out-of-line split of vector3 and matrix: combat.cpp expands matrix assignment as
   three vector3 copies, partly calling vector3::operator= (0x413190) and the copy constructor
   (0x410cd0), but calls neither in other places; dot products are evaluated z, y, x (an inline helper
   in the original math header?). Same question as D66.
3. spell_roll_amount has an 8-byte aligned EBP frame (`and esp, -8`): a double local in the source?
4. 0x59b9f0 {species, float} pairs (0xc2 7.0, 0xb5 13.0, 0xb6 35.0, 0xc3 14.0) used by summon_create.
5. living_combat +0x10 is never touched in this unit; +0x24 (chain_index) is incremented by 2 by a push
   hit (melee_damage) although chain_attack_check_window uses it as a chain counter.
6. combat_resolve_exchange's multiplayer filter repeats the host test (written as client / host / else
   return); the shape is a guess.
7. melee_damage calls character_get_resistance(-1) when the weapon has no elemental damage (and for
   unarmed attacks); whether that reads out of bounds depends on 0x4626b0 (living_entity.cpp).

## Where docs/sum-exe/combat.md disagrees with the code

- Step 8, backstab: besides x2.5 with the +0x50 flag, the damage is **halved** without it (the `je` at
  0x465193 goes to the aimed attacks' x0.5); the flag is set by living_combat_update_special when the
  target is not in combat mode. Attackers of type 9 or with flag 0x40 just double. The kick (0xf) also
  replaces the damage (level x 4, blunt), and the "level x 8 / x 10" cases are the serpent whip skills
  (0x53, 0x50), not chain attacks; push (0xe) adds 2 to +0x24.
- Steps 3: the weapon is ignored when char_info flag 0x200 is set; unarmed damage is blunt (6),
  slashing (4) for species 0xb0.
- After step 9 the doc only mentions the side effects. The code then also multiplies by
  `1 - resistance(damage type)` and `1 - resistance(elemental)` (the first negative one wins, else the
  product), adds `position modifier x damage` (stored at +0x48), multiplies by `1 + curse value` when the
  defender is cursed (status 3), doubles on a critical hit (skill 0xc vs frand), sets 200 with
  Super_power for party members, and returns `(int)floor(...)`.
- "Life leech (0x450c70 with -0.1 / 0.1 of the damage)": 0x450c70 is character_add_ap, used by **mind
  drain** (0x3f: the defender loses 10 % of the damage as AP, the attacker gains it, damage 0). Life
  leech (0x3e) halves the damage and heals the attacker by it (character_add_hp). Sacrifice (0x44)
  doubles the damage and costs the attacker 2/3 of it; confusion (0x45) drains AP; stamina burst and
  desperation scale by AP / max AP and 1.3 - HP / max HP.
- Spell roll: the party factor is not applied to healing (it is zeroed when the total is positive), and
  damage reduced by the resist may not turn into healing (clamped to 0).

## Game bugs noticed (literal behaviour kept)

- spells_parse_table stores the record index instead of the parsed +Recovery time:, +AI Recovery time:
  and +Loop Time: values. status_effect_duration takes invisibility's duration from spells[11]
  +Loop Time:, so invisibility lasts 11 s (its record index) instead of the table's 10 (`+Loop time: 10`; the
  table also gives +Recovery time: 0 and +AI Recovery Time: 30, both replaced by 11).
- spell_launch_missile: with all 20 missile slots taken it frees the stale ones but does not record the
  current missile.
- combat_apply_melee_hit: the aimed attacks' blind / cripple / slow chance uses the aimed-attack skill
  of cur_character, not of the attacker (monsters use the selected party member's skill).
- combat_spawn_hit_vfx picks "blood.vfx" for skins 1-2 and "sparkhit.vfx" for 3, 9-16 but spawns only
  for skins 1, 3 and 9; "sparkhit.vfx" is never used.
- spell_target_value (cure) tests STATUS_BLIND twice.
- cmd_summon has no calling_a_command_function test (the console's help and status passes summon too).
- spell_effect_pool_full returns true when a record is free (the name is ours from sum.symbols; the
  caller uses it correctly).
