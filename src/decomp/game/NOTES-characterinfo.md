# characterinfo.cpp (decomp38)

`game/characterinfo.cpp`: Engine\characterinfo\characterinfo.cpp, code 0x4425b0-0x449d88, all 122
functions of the unit (sum.symbols module column). Confidence: 81 exact, 26 likely, 15 unsure (the 15
empty Mempool initialisers: which body belongs to which pool is not visible). Written from Sum.exe's
disassembly only (tools/asm.mjs); no Summoner 2 code used. Syntax-checked with i686-w64-mingw32-g++
(-fsyntax-only -Wall, no warnings); every existing src/decomp .cpp compiles against the header copy with
the same diagnostics as against the repo headers. Header changes: CHANGED-HEADERS.txt, decisions
D1800-D1808 in include/DECISIONS.md.

## Layout of the unit

- Static initialisers first (entries 159-177): chain_skill_table, skill_table, resistance_level_names
  (dynamic because they store `&localize_strings[id]`), 15 empty initialisers (the Mempools, D1801),
  ccmd_All_summons, the two StringPools. ccmd_Quick_combo (0x449560), ccmd_charinfo (0x449ba0) and
  ccmd_levelchars (0x449bd0) are defined between the functions, so the source defines them there.
- .data 0x595b24-0x596173: summon_creature_names, skill_names (84) + count, resistance level type names,
  team_names, skill_group_names (no reader), variation_names (194), hurt_variation_names (10) + count,
  default_footstep_sounds, size_names, move_type_names (15 + NULL), location_names, anim_event_names,
  prop_tag_names, attack_point_names, offense_point_names. Labels ours where sum.symbols has none.
- .bss 0x65e968-0x8b50b2: the hero char / model buffers and pools, char_infos, the 3250 move records
  (move_infos 0x7e12e8, 0x4c bytes) and their free list, the per-level anim block pointers and sizes,
  multi_charinfo_slots, the string pool buffers, charinfo_cast_spells (0x88904c), chain_skill_table,
  skill_table, resistance_level_names, All_summons / charinfo_party_loaded / Quick_combo. The objects
  with dynamic initialisers are defined in initialiser order, the rest in address order (comments give
  every address).

## Functions

| Address | Name | Confidence |
|---|---|---|
| 0x4425b0 | init_chain_skill_names_thunk | exact |
| 0x4425c0 | init_chain_skill_names | likely |
| 0x442b90 | init_skill_table_thunk | exact |
| 0x442ba0 | init_skill_table | likely |
| 0x4447f0 | init_strptrs_8b5098_thunk | exact |
| 0x444800 | init_strptrs_8b5098 | exact |
| 0x444840 | init_empty_444850_thunk | exact |
| 0x444850 | init_empty_444850 | unsure |
| 0x444860 | init_empty_444870_thunk | exact |
| 0x444870 | init_empty_444870 | unsure |
| 0x444880 | init_empty_444890_thunk | exact |
| 0x444890 | init_empty_444890 | unsure |
| 0x4448a0 | init_empty_4448b0_thunk | exact |
| 0x4448b0 | init_empty_4448b0 | unsure |
| 0x4448c0 | init_empty_4448d0_thunk | exact |
| 0x4448d0 | init_empty_4448d0 | unsure |
| 0x4448e0 | init_empty_4448f0_thunk | exact |
| 0x4448f0 | init_empty_4448f0 | unsure |
| 0x444900 | init_empty_444910_thunk | exact |
| 0x444910 | init_empty_444910 | unsure |
| 0x444920 | init_empty_444930_thunk | exact |
| 0x444930 | init_empty_444930 | unsure |
| 0x444940 | init_empty_444950_thunk | exact |
| 0x444950 | init_empty_444950 | unsure |
| 0x444960 | init_empty_444970_thunk | exact |
| 0x444970 | init_empty_444970 | unsure |
| 0x444980 | init_empty_444990_thunk | exact |
| 0x444990 | init_empty_444990 | unsure |
| 0x4449a0 | init_empty_4449b0_thunk | exact |
| 0x4449b0 | init_empty_4449b0 | unsure |
| 0x4449c0 | init_empty_4449d0_thunk | exact |
| 0x4449d0 | init_empty_4449d0 | unsure |
| 0x4449e0 | init_empty_4449f0_thunk | exact |
| 0x4449f0 | init_empty_4449f0 | unsure |
| 0x444a00 | init_empty_444a10_thunk | exact |
| 0x444a10 | init_empty_444a10 | unsure |
| 0x444a20 | init_ccmd_All_summons_thunk | exact |
| 0x444a30 | init_ccmd_All_summons | exact |
| 0x444a50 | cmd_All_summons | exact |
| 0x444b10 | sinit_stringpool_68e1a8 | exact |
| 0x444b20 | init_stringpool_68e1a8 | exact |
| 0x444b40 | register_dtor_stringpool_68e1a8 | exact |
| 0x444b50 | dtor_stringpool_68e1a8 | exact |
| 0x444b60 | sinit_stringpool_68e1c0 | exact |
| 0x444b70 | init_stringpool_68e1c0 | exact |
| 0x444b90 | register_dtor_stringpool_68e1c0 | exact |
| 0x444ba0 | dtor_stringpool_68e1c0 | exact |
| 0x444bb0 | move_used_on_level | likely |
| 0x4450c0 | characterinfo_load_party | likely |
| 0x4452f0 | characterinfo_parse_table | likely |
| 0x446740 | characterinfo_add_move | exact |
| 0x446770 | move_info_free | exact |
| 0x446790 | sound_ref_parse | exact |
| 0x446890 | characterinfo_parse_name | exact |
| 0x446930 | characterinfo_parse_resistances | exact |
| 0x4469c0 | living_entity_setup_head | likely |
| 0x446b80 | charinfo_find_by_name | exact |
| 0x446bd0 | charinfo_find_by_species | exact |
| 0x446c00 | summon_find_charinfo | exact |
| 0x446c40 | summon_skill_creature_name | exact |
| 0x446c60 | charinfo_find_move_any | exact |
| 0x446cb0 | charinfo_find_move | exact |
| 0x446d10 | charinfo_find_move_alt | exact |
| 0x446d70 | charinfo_collect_moves | exact |
| 0x446de0 | multi_charinfo_slot | exact |
| 0x446e20 | move_get_trigger_time | exact |
| 0x446e70 | skill_is_summon | likely |
| 0x446e90 | character_can_use_summon | likely |
| 0x4473d0 | skill_has_flag8 | likely |
| 0x4473f0 | skill_is_combat_special | exact |
| 0x447410 | characterinfo_load_party_anims | likely |
| 0x447780 | charinfo_load_move_anim | exact |
| 0x447850 | charinfo_remap_town_moves | likely |
| 0x447b10 | level_load_preload | likely |
| 0x447e80 | str_hash_nocase_rol6 | exact |
| 0x447ec0 | charinfo_mark_anims_loaded | exact |
| 0x447f20 | character_reinit_anims | exact |
| 0x448000 | summon_stash_for_cutscene | likely |
| 0x4482a0 | charinfo_load_script_anim | likely |
| 0x4484a0 | charinfo_free_moves | exact |
| 0x448500 | summon_restore_after_cutscene | likely |
| 0x4486b0 | charinfo_mark_anims_unloaded | exact |
| 0x448710 | charinfo_unload_anims | likely |
| 0x448790 | charinfo_page_in_sounds | exact |
| 0x4487f0 | offense_point_name | exact |
| 0x448800 | characterinfo_alloc_level_buffers | likely |
| 0x448c10 | characterinfo_level_unload | exact |
| 0x448c80 | resistance_level_damage_mult | likely |
| 0x448d00 | summon_skill_from_creature_name | exact |
| 0x448d40 | skill_find_by_display_name | exact |
| 0x448d90 | charinfo_find_anim | exact |
| 0x448de0 | charinfo_create_character_instance | likely |
| 0x448f20 | charinfo_select_mempool | exact |
| 0x448f60 | statscreen_load_anim | likely |
| 0x449100 | statscreen_unload_anim | likely |
| 0x449200 | chain_config_set | exact |
| 0x449240 | chain_config_get | exact |
| 0x449290 | chain_points_set | exact |
| 0x4492d0 | chain_points_get | exact |
| 0x449310 | chain_config_reset_defaults | exact |
| 0x4493e0 | character_set_chain_skill | exact |
| 0x449440 | character_get_chain_skill | likely |
| 0x449510 | character_cycle_default_chain | exact |
| 0x449560 | init_ccmd_Quick_combo_thunk | exact |
| 0x449570 | init_ccmd_Quick_combo | exact |
| 0x449590 | cmd_Quick_combo | exact |
| 0x449650 | character_learn_chain_skill | likely |
| 0x449830 | chain_count_unlocked_skills | exact |
| 0x449890 | chain_get_unlocked_skill | exact |
| 0x449910 | chain_config_find_skill | exact |
| 0x449960 | chain_skill_get_points | exact |
| 0x4499c0 | chain_skill_set_points | exact |
| 0x449a20 | species_is_hero | exact |
| 0x449a30 | characterinfo_find_multi_slot | likely |
| 0x449b00 | multi_charinfo_slot_release | exact |
| 0x449b60 | multi_charinfo_slots_clear | exact |
| 0x449b80 | charinfo_get_multi_appearance | exact |
| 0x449ba0 | init_ccmd_charinfo_thunk | exact |
| 0x449bb0 | init_ccmd_charinfo | likely |
| 0x449bd0 | init_ccmd_levelchars_thunk | exact |
| 0x449be0 | init_ccmd_levelchars | exact |
| 0x449c00 | cmd_levelchars | likely |

Left: nothing in this unit. The experience functions (0x489930-0x489bf0) that characterinfo.h also
declares are a different unit (not written here).

## What the code says (for the docs)

- **chars tables.** characterinfo_load_party parses "<name>.tbl" (lower-cased) for every species_table
  entry (223, 0x580e60) whose byte +8 is 1. Each file starts with "#Character Info"; all "$Character:"
  records of one file go into the same char_info (one record pointer is taken before the loop; the
  retail files have one record each), "+Alternate character:" lines add copies with their own name,
  resistances and alt texture entry (alt_parent points at the base record). Returns the base record.
- Field order the parser requires: $Character:, +Portrait Filename:, +Texture Effect:, the flag lines
  (+No Shadow 1, +Edged 2, +Elongated 4, +Notrip 8, +Nopush 0x10, +Nobackstab 0x20, +Noaimedattack 0x40,
  +Noblind 0x80, +Nosilence 0x100, +UseMonsterDamage 0x200, +Flying 0x800, +ShortDialogueCamera 0x1000,
  +ReallyShortDialogueCamera 0x2000, +TallDialogueCamera 0x4000, +No draw 0x8000; each may be followed by
  ':', as "+Edged:" in Golem.tbl), +Vfx:, $Size:, $Team:, HP/AP, $Aggressiveness: / $Teamwork: /
  $Conservation: (parsed and dropped), +Intelligence: (percent, default 0.5), $Experience Gained:
  (default 0), $Level: (default 1), $Attack Radius:, $Default Weapon: "item" [made of "material"],
  $Damage:, +Special ability: / +Reflex ability: "spell" chance [arg], the AI ranges and rates ($Speedup
  rate: and $Slowdown rate: are halved), $Icon Height: (default 2.0), $Default Prop Tag: / $Prop Tag:
  "tag" is "bone", +Resistances: ... #Resistances End, $Up/$Down/$Left/$Right Skill:, $Skill: "name" value
  (speed defaults to 10), +Footstep sounds: min-dist volume rolloff + 10 files (default: the footsteps\
  wood/stone/grass/sand/wet set at 5.0 / 0.9 / 1.0), +Skin: (material, default 1 "flesh"), +Allowed weapon
  classes: (one line, tab/quote separated anim class names; default all), +Anim Folder: (default
  data\ca\<name>), +Sound Folder:, the +Move: blocks, +Drop: (first 5 kept), +Multiplayer Drop: item
  chance level (first 17), +Alert:, +CharacterLightPos: / +LightColor: / +LightAtten:, +Alternate character:.
- **+Move:** type (move_type_names), $Animation:, optional +Turning (ignored), +Animation class: list
  (default 1 = "none"), $Action:, $Sound: "file" volume p2 p3 (a file without '\' is looked for in the
  sound folder, else in a folder named after the character; volume x 255), $Effect:, $Location: /
  $Variation: in any order, $Contact tag: [on primary prop | on secondary prop] (the prop words are
  ignored), $Trigger: event frame (stored as frame / 30, turned back into frames for state moves).
- Moves whose variation is a hurt / turn name (stand_to_ready_hurt, ready_to_stand_hurt, stand hurt,
  ready hurt, walk hurt, run hurt, norm walk hurt, norm run hurt, turn left, turn right) are dropped at
  load for every record that is not a boss, the heroes included, so the retail tables' many "... hurt"
  moves (Alligator Beast, Aoqi, the bacites, ...) are never used outside boss records.
- No trip move (state 0x8f) sets the no-trip flag, no push move (misc 0x91) the no-push flag.
- For non-hero, non-boss records the spells of the "cast" moves (spell_find_by_cast_id of the variation)
  are listed in charinfo_cast_spells (+0x19c count, +0x21c pointer).
- **Anim loading.** Heroes load per weapon class into "<Hero> Anims" pools whose size depends on the
  level (town 0x19000 each, Rosalind 0x25800; elsewhere Joseph 0x4f800, Flece 0x57800, Jekhar / Rosalind
  0x46c00 / 0x70800, templeint 0x19000 / 0x25800; masad: 300 bytes for the three non-Josephs;
  multiplayer 0x7e400 / 0x8ac00 / 0x69c00 / 0xa1800); the summon pool is 0xfa000, the stat screen 0x7800.
  Other records use their .svf preload index (first four letters of the skeleton set + the last four
  digits of its rol-6 name hash). move_used_on_level skips (returns 1) the low parry / flinch moves, the
  non-combat flinches, several hero moves in multiplayer, all combat moves in towns (and templeint for
  Jekhar / Rosalind), and boss rider moves on masad / Liangshan / JadeTemple.
- **Chains.** chain_skill_table: per hero the slot 0 "added blow" and three slots of three skills each,
  learned at 0 / 10 / 30 chain points ("chainpts<species><slot>" savegame ints, 0..100). A landed chain
  (character_learn_chain_skill) costs floor(n x AP cost x 0.2) + 1 AP with chance AP cost x 0.23, and
  with chance 0.13 (always with Quick_combo, +10 instead of +1) gives the slot a point; crossing a
  threshold shows "%s learned %s!". The configured chain per direction is "chainconfig<species><slot>"
  (defaults: the record's up, down, left, right skills); non-heroes use their record's skills directly.
- resistance_level_damage_mult returns 1 - multiplier (weak -1, vulnerable -0.5, standard 0, strong 0.5,
  invulnerable 1, strengthen 1.5).

## Doc and symbol disagreements

1. docs/sum-exe/characters.md "Skills": skill_table has 84 records (ids 0..0x53; init_skill_table's last
   store is 0x8b5094, resistance_level_names starts at 0x8b5098), not 85.
2. characterinfo.h said skill_names[36] ending at 0x595bf4: 84 names + the count 84 (D1800).
3. sum.symbols offense_point_names comment: living_entity_setup_head walks attack_point_names
   ("$Attack01".."$Attack08", 0x59612c-0x59614b), not the "$offense" names (only offense_point_name
   reads those).
4. sum.symbols chain_skill_table "(name, member, skill, points, flag)": the fields are name slot,
   species, skill, chain slot (the chainpts counter) and the points threshold byte.
5. sum.symbols charinfo_load_move_anim "(char_instance, move, flag)": (char_info, ca_character, move,
   load_sounds, anim_folder, full_set).
6. sum.symbols charinfo_remap_town_moves "every move in the lists +0x4C..+0x60": lists 0-7 and 9-13
   (+0x4c..+0x80) get the anim; list 14 only for the ids 0x6f, 0x95, 0xa0, 0x27, 0x28, 0xa1, 0xa2, with
   the anim of state move 0x92. Outside towns the anims and sounds of moves (4,3) and (2,2) are copied.
7. characterinfo.h: charinfo_collect_moves' second argument is unused, the fourth is the list (D1803);
   characterinfo_load_party's argument is not read.
8. types.h char_info +0x1C "zeroed; compared with a move's class": it is the mask of loaded weapon
   classes (charinfo_mark_anims_loaded sets -1, ..._unloaded 0, load_party_anims ORs the class bit).
9. types.h species_info `_unk08` is the load byte (D1805). sum.symbols move_used_on_level: 1 means
   "skip", the opposite of the name.
10. resistance_level_damage_mult is 1 - the multiplier, not the multiplier.
11. charinfo_create_character_instance passes the alt texture entry as character_load's "unused" fourth
    argument (D1807).

## Game bugs noticed

- B1 +Animation class: ORs into a recycled move record without clearing it: the record freed by the
  hurt-move drop is the next +Move:'s record, so that move inherits the dropped move's class bits.
- B2 charinfo_remap_town_moves does not check charinfo_find_move(14, 0x92) for NULL before copying its
  anim to the listed state moves (a crash in a town for a record with those moves and no 0x92 move).
- B3 statscreen_load_anim / statscreen_unload_anim use the 0x9e / 0x9f move without a NULL check.
- B4 summon_stash_for_cutscene subtracts the remaining trigger times from the summon's emerge move in the
  shared move record: every cutscene during an emerge shifts that creature type's triggers again.
- B5 move_used_on_level sets m->anim = 1 on masad (not v2), Liangshan and JadeTemple before deciding: the
  moves it then skips keep skeleton index 1 instead of -1 (and play skeleton 1 if used).
- B6 +Drop: keeps 5 entries although the arrays hold 17 (later lines are silently ignored);
  $Default Weapon: and $Trigger: have no bound (2 and 3 entries).
- B7 species_is_hero has no lower bound (any negative species counts as a hero).
- B8 the console command "charinfo" does nothing (its function is the shared empty function).
- B9 character_learn_chain_skill builds "%s learned %s!" in a 0x50-byte buffer from a 0x40-byte name and
  a localized skill name (fits in English; no bound).
- B10 the hurt-move loop keeps comparing the freed record's variation after move_info_free (harmless:
  the record is only relinked).

## Open questions

- Q1 which empty initialiser belongs to which Mempool (address order assumed).
- Q2 the source of the flag lines: written as an if / else chain with one `tf.Accept(":")`; VC6 merged
  the flag stores into one tail (0x445616).
- Q3 level_load_preload's two strcat copies are expanded inline; whether the source used strcat or a
  helper is not visible.
- Q4 the stack frame of characterinfo_parse_table (0xc3c bytes): block-scope buffers assumed, the exact
  sharing of slots between blocks is VC6's.
