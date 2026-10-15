# Pilot notes: objects.cpp and items.cpp (decomp19)

Source written from the Sum.exe v1.40 disassembly (`tools/asm.mjs fn`; llvm-objdump `--start-address`
where the cached listing is out of step after a jump table: 0x44b200 and the start of 0x44b7a0; raw
.data bytes read from `Summoner/Sum.exe`). Summoner 2 and Dash Faction were not used for code. Both files
pass `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include`;
objects.cpp gives two `-Wdelete-non-abstract-non-virtual-dtor` warnings (the literal `delete` of an obj,
see DECISIONS.md D61), items.cpp none. Header changes: CHANGED-HEADERS.txt, DECISIONS.md D59-D67.

## objects.cpp (0x4697e0-0x46b45b), unoptimised: 48 functions, 37 exact / 10 likely / 1 unsure

Built without optimisation (ebp frames, 0xcc padding), but with inline expansion: vector3's constructor,
operator=, operator- and operator+=, matrix::operator=, obj's scalar deleting destructor and
profile_label's constructor / destructor are expanded (D66). Locals are declared so that the first one
gets ebp-4 (VC6 /Od order); the frame offsets are noted per function.

| Address | Function | Confidence |
|---|---|---|
| 0x4697e0-0x469830 | sinit / init / register / dtor of `obj obj_active_list(0)` | exact |
| 0x469840-0x4698a0 | sinit / init / register / dtor of `item_obj item_obj_pool[1000]` (eh vector ctor/dtor) | exact |
| 0x4698c0 | living_entities_update_footsteps | exact |
| 0x469900 | objects_init | exact |
| 0x469a70 | obj::obj (obj__obj) | likely |
| 0x469b10 | obj::obj(int) (obj__ctor_link) | likely |
| 0x469b40 | obj::~obj | exact |
| 0x469b60 | obj::link_new | likely |
| 0x469c40 | obj_create | exact |
| 0x469dd0 | obj_handle_base_next | exact |
| 0x469e10 | obj::deinit | exact |
| 0x469fb0 | objects_free_all_thunk | exact |
| 0x469fc0 | objects_free_all | likely |
| 0x46a0b0 | obj::is_living | exact |
| 0x46a100 | obj::is_dead_type (character_is_dead_type) | exact |
| 0x46a130 | item_obj::~item_obj | exact |
| 0x46a150 | item_obj::item_obj | exact |
| 0x46a1b0 | item_obj_spawn | likely |
| 0x46a640 | level_item_spawn | likely |
| 0x46a710 | item_obj::deinit | exact |
| 0x46a790 | item_obj::is_gold (obj_is_gold) | exact |
| 0x46a7e0 | item_obj::is_in_view | likely |
| 0x46a890 | container::container | exact |
| 0x46a8e0 | container::~container | exact |
| 0x46a920 | container::deinit | exact |
| 0x46a960 | container::is_in_view | likely |
| 0x46aa30 | container::add_item | exact |
| 0x46ab60 | container::remove_item | likely |
| 0x46ac20 | container::clear | exact |
| 0x46acd0 | container::get_item | exact |
| 0x46ad10 | Character::anim_lod_skip | exact |
| 0x46adf0 | Character::process_anim | unsure (profile label text), else likely |
| 0x46af70 | Character::fill_locomotion_anims | exact |
| 0x46b130 | obj_from_handle | exact |
| 0x46b1a0 | living_from_handle | exact |
| 0x46b1e0 | handle_to_index | exact |
| 0x46b200 | objects_reset | exact |
| 0x46b210 | living_entity_level_start | exact |
| 0x46b220 | objects_do_frame | exact |
| 0x46b330 | objects_delete_marked | exact |
| 0x46b390 | living_entities_move_all | likely |
| 0x46b3d0 | objects_update_frame | exact |

Data: .data 0x59a5a0 `obj_handle_next = 1`, 0x59a5a4 `obj_handle_max = 0x7fffffff / 1000` (*new*),
literals 0x59a5a8-0x59a639; .bss num_objects 0x9438a0, obj_used_list, obj_slots_unk_9438a8 (*new*,
written once, never read), obj_free_list, obj_slots[1000] (12 bytes), item_obj_pool, obj_active_list,
objects_initialized 0x970809. .rdata: the three one-slot vtables and 225.0f (0x5819d0).

Findings (literal behaviour kept):
- container::add_item: gold merged into the gold in slot 0 is deinit-ed and then still stored in slot 0
  and counted.
- item_obj_spawn: no overflow check on the free item_obj search; the layer probe loop has no break, so
  `i >= num_lobjs` always holds afterwards and the floor is probed again against all level objects.
- objects_free_all's first loop only ends if the list is already empty (~obj does not unlink).
- objects_update_frame animates objects of type 1 as characters; the living types are 4/5/7/9.

## items.cpp (0x449d90-0x44d9ee), /O2: 41 functions, 31 exact / 10 likely / 0 unsure

| Address | Function | Confidence |
|---|---|---|
| 0x449d90 / 0x449da0 | init_strptrs_8ec9c4 (`damage_type_loc_names`) | likely |
| 0x449df0 / 0x449e00 | init_strptrs_8ec9e0 (`ring_stat_names`) | likely |
| 0x449e20 / 0x449e30 | init_strptrs_8ec9e8 (`status_effect_names`) | likely |
| 0x449ef0-0x449f20 | sinit / init / register / dtor of `NoRlsAllocPool items_alloc_pool` | exact |
| 0x449f30 | items_init | exact |
| 0x449f60 | materials_parse_table | exact |
| 0x44a090 | items_parse_table | likely |
| 0x44b200 | item_types_free | exact |
| 0x44b250 | item_types_reset_identified | exact |
| 0x44b280 | items_init_pool | likely |
| 0x44b2b0 | item_free | exact |
| 0x44b2d0 | item_create | exact |
| 0x44b320 | item_create_by_name | exact |
| 0x44b340 / 0x44b3b0 / 0x44b3f0 | item_create_weapon / _armor / _misc (identical) | exact |
| 0x44b380 | item_alloc | exact |
| 0x44b430 | item_create_misc_by_name | exact |
| 0x44b450 | item_type_find | exact |
| 0x44b4a0 | item_type_find_by_hash | exact |
| 0x44b4f0 | weapon_is_ranged | exact |
| 0x44b500 | item_type_is_stackable | exact |
| 0x44b530 | character_can_equip_item | exact |
| 0x44b550 | character_meets_item_requirements | exact |
| 0x44b670 | item_can_destroy | exact |
| 0x44b690 | item_is_identified | exact |
| 0x44b6b0 | item_type_is_identified | exact |
| 0x44b6d0 | item_has_magic_info | exact |
| 0x44b700 | character_species_can_use_item | exact |
| 0x44b720 | character_species_can_use_type | exact |
| 0x44b7a0 | ring_format_info (the whole item info popup text, 7.5 KB) | likely |
| 0x44d540 | items_level_load | likely |
| 0x44d870 | items_level_unload | exact |
| 0x44d8a0 | item_get_display_name | exact |
| 0x44d940 | hero_model_index | exact |

Data placed here by address: .data 0x598f5c-0x599043 material_names, anim_class_names, damage_type_names,
item_stat_names (*new*), item_slot_names (*new*), weapon_flag_names (*new*), num_weapon_flag_names = 4
(*new*, read from memory by the weapon +Flags: parser), item_flag_names (*new*); then the literals in
reverse order of use. .bss item_free_list, item_records[1224] (*new*), materials[17] (at 0x8b9d48:
sum.symbols' 0x8b9d4c is its +4), items_table_checksum, items_alloc_pool + its 0x12000 buffer
(items_alloc_buffer, *new*), items_num_allocated, item_types[0x209], num_item_types, ring_info_text, the
three localize slot tables. material_names / damage_type_names are also read by characterinfo.cpp (an
earlier unit), so they are externs there.

items.tbl format notes (from the parser): `#<language>` section, then `#Items` ... `#End`. Per record:
$Name, $Type (Misc / Armor / Weapon), +Slot, +Flags (list until $Value; "not identified" also sets bit 3),
$Value, $Icon (read and ignored), $Model, +Animation (kept only for weapons and slot-5 armour), +Bitmap,
+Skill Mod / +Resist Mod (x4), +Stat Mod (x2), then in any order +AP Regen / +Stealth / +Effect, then
+Charge Effect or +Self Only Charge Effect (spell, two ints, rest of line, optional +Disposable[:n]),
+Restricted (hero names), then armour "+Hero [Heavy] [Head|One Hand|Cape|Leg] Texture:" lines and
$Speed/$Weight/$Protection/$Default Material/+Triggered Effect, or weapon +Render distance, +Flags (until
$Skill or "prop only"), $Skill, $Class, $Damage, $Damage Type, +Elemental Damage, $Speed, $Size (ignored),
$Attack Radius, $Weight, $Default Material, +Triggered Effect; finally +Description and +RenderMask (hero
model name + list of mesh part numbers). Prop-only weapons stop after their +Flags.

Literal findings: items_level_load pages a non-party character's slot-9 item using equipment[5]'s type;
item_alloc does not check for an empty free list.

## Left / open questions

- Nothing of the two units is left. item_obj_pickup and the party-inventory functions (0x46b460-0x46c530,
  optimised, sum.symbols module "-") belong to a separate unit after objects.cpp and were not written;
  the task text mentions "pickup" for items.cpp, but the module column does not tag them items.cpp.
- .bss order: obj_active_list (initialiser 202) is defined before item_obj_pool (203) although its
  address is higher; the same open question as player.cpp / gamesound.cpp.
- objects.cpp's inline expansion with no optimisation (vector3, matrix, profile_label) contradicts the
  out-of-line declarations used by the optimised units: the real build needs per-unit options (/Od /Ob1?)
  or inline definitions in the headers. The profile_label text in character_process_anim is unknown.
- items.cpp: whether ITEMS_STRDUP was a helper (expanded inline) or written out each time cannot be seen;
  the two equal 0.01f constants (0x5818ec / 0x5818f0) suggest two spellings (e.g. 0.01f and 1/100.0f).
- ring_format_info's (int)(float)(int)(x * 10 + 0.5) double conversion is reproduced with a macro; the
  original was probably an inline rounding helper.
- items_level_load's third loop passes its slot variables in the opposite stack order: written as block
  locals, unsure which source produced it.
- 0x8b50bc-0x8b50c7 (between item_free_list and the item records) and 0x970808 are never referenced.
