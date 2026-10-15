# decomp64: 0x489730-0x48db2f (containers, experience table, effect objects, user tracker login)

Written from the Sum.exe v1.40 disassembly (`node tools/asm.mjs fn`, raw .data / .rdata bytes and the jump tables
read from the exe). Target VC++ 6.0, /O2 /Ob1. Base of the header copy: src/decomp/include at 66fb364.
Every .cpp passes `i686-w64-mingw32-g++ -fsyntax-only -Wall ...`; the 161 .cpp files of 66fb364 still compile
against this header copy.

**85 functions** (+ 2 folded hooks): **59 exact / 26 likely / 0 unsure**. Nothing in 0x489730-0x48db2f is left.

## Units (D4400; all four names are placeholders: no path string, no strings.tbl block, module "-")

| File | Code | .data | .bss | Evidence |
|---|---|---|---|---|
| `game/unnamed_containers_0x489730.cpp` | 0x489730-0x48992f | 0x59c230-0x59c247 | 0x2302fe0-0x230541f | static initialiser entry 250 (the pool), after multicontainer.cpp (ends 0x48972a, no data); the pool follows multichat.cpp's .bss exactly |
| `game/unnamed_experience_0x489930.cpp` | 0x489930-0x489c3f | 0x59c248-0x59c25b | 0x2305420-0x2305617 | "experience.tbl", "D"; xp_level_table |
| `game/unnamed_fx_0x489c40.cpp` | 0x489c40-0x48d7bf | 0x59c25c-0x59c357 | 0x2305618-0x2318e43 | initialiser entries 251 / 252 open the code; .data inversion (fx_obj_destroy's literal after the blob handles shadow_create_blob_bitmap writes) proves the blob code and the effect objects are one file; "sky" ends where levelrender.cpp's .data starts |
| `game/unnamed_utrack_0x48d7c0.cpp` | 0x48d7c0-0x48db2f | none | 0x2318e48-0x231906f | its own topic; .bss between fx_count and levelrender.cpp's 0x2319070; levelrender.cpp's code follows |

Open: the first three may be one file (no gap and no out-of-order use between their data). The cuts follow the
initialisers and topics.

## Functions

### unnamed_containers_0x489730.cpp (8: 7 exact / 1 likely)

| Address | Function | Confidence |
|---|---|---|
| 0x489730 | sinit_containers (compiler-generated) | exact |
| 0x489740 | containers_construct (eh vector ctor, compiler-generated) | exact |
| 0x489760 | register_dtor_containers (compiler-generated) | exact |
| 0x489770 | dtor_containers (compiler-generated) | exact |
| 0x489790 | containers_reset | exact |
| 0x4897c0 | containers_free_all | exact |
| 0x4897f0 | container_obj_create | likely (not-found exit; redundant type store) |
| 0x489920 | obj_mark_for_delete | exact |

### unnamed_experience_0x489930.cpp (8: 6 exact / 2 likely)

| Address | Function | Confidence |
|---|---|---|
| 0x489930 | experience_parse_table | exact |
| 0x4899d0 | experience_parse_level | exact |
| 0x489a30 | experience_parse_stat_gain | exact |
| 0x489a60 | xp_for_level | likely (loop form) |
| 0x489aa0 | xp_level_reached | exact |
| 0x489ac0 | xp_level_up_gains | likely (clamp form) |
| 0x489bf0 | xp_level_skill_cap | exact |
| 0x489c20 | xp_max_level | exact |

### unnamed_fx_0x489c40.cpp (65 + 2 folded: 44 exact / 21 likely; the hooks likely)

| Address | Function | Confidence |
|---|---|---|
| 0x489c40 / 0x489c50 / 0x489c60 / 0x489c70 | sinit / init / register / dtor of fx_active_list (compiler-generated) | exact (4) |
| 0x489c80 / 0x489c90 / 0x489cb0 / 0x489cc0 | sinit / construct / register / dtor of fx_pool (compiler-generated) | exact (4) |
| 0x489ce0 | fx_alloc | exact |
| 0x489d30 | effect_object::effect_object (fx_obj__ctor) | likely (store order) |
| 0x489db0 | effect_object::~effect_object (fx_obj__dtor) | exact |
| 0x489dc0 | effect_object::deinit (fx_obj_destroy) | likely (loop form) |
| 0x489f00 | obj_from_handle_b | exact |
| 0x489f10 | shadow_create_blob_bitmap | likely (unsigned scale conversion) |
| 0x48a060 | shadows_init | exact |
| 0x48a090 | fx_lists_init | likely (head store order) |
| 0x48a0e0 | fx_shutdown | exact |
| 0x48a0f0 | fx_destroy_all | exact |
| 0x48a190 | fx_level_enter | exact |
| 0x48a1a0 | fx_sky_list_clear | exact |
| 0x48a1d0 | fx_level_leave | exact |
| 0x48a1f0 | fx_sky_list_clear_thunk | likely (a function whose body is the call; a jmp) |
| 0x48a200 | fx_level_restart | exact |
| 0x48a230 | fx_frame | likely (tail merging decides where the stop tests sit; case order from the literals) |
| 0x48b690 | fx_bind_character_tags | likely (inline strcmp) |
| 0x48b750 | fx_update_instance | exact |
| 0x48b7a0 | fx_update_missile | likely (plane written out) |
| 0x48bac0 | fx_missile_hit | likely (result variable) |
| 0x48bd40 | fx_count_spells_1a_1d_4e | exact |
| 0x48bd80 | fx_add_effect | exact |
| 0x48bdc0 | fx_create_instance | likely (NULL defaults as locals, node walk) |
| 0x48c080 | fx_attach_to_character | exact |
| 0x48c140 | fx_create_at_camera | likely |
| 0x48c220 | fx_create_missile | likely |
| 0x48c3e0 | fx_new | likely (store order) |
| 0x48c470 | status_effect_update_fx | likely |
| 0x48c540 | fx_create_ground_effect | exact |
| 0x48c5c0 | fx_create_status_effect | exact |
| 0x48c630 | fx_create_spell_area | likely (two identical loops) |
| 0x48c800 | fx_create_spell_effect_near | exact |
| 0x48c8a0 | fx_create_spell_effect_facing | likely (cross product form) |
| 0x48c9a0 | titus_rock_erupt_fx | exact |
| 0x48ca40 | character_equip_item_fx | likely (switch) |
| 0x48cab0 | fx_create_spell_effect_at | exact |
| 0x48cb40 | summon_spawn_effect | likely (record store order) |
| 0x48cc60 | fx_get_status_holder | exact |
| 0x48cd20 | status_stone_fx | exact |
| 0x48cdf0 | fx_stop_status_effects | exact |
| 0x48ce50 | fx_stop_status_aura | exact |
| 0x48cea0 | fx_create_world | exact |
| 0x48cf50 | fx_set_target | exact |
| 0x48cf80 | fx_stop | exact |
| 0x48cfc0 | fx_destroy | exact |
| 0x48d000 | fx_is_finished | likely (progress expression) |
| 0x48d0a0 | fx_remove_missiles_from | exact |
| 0x48d100 | fx_end_owned | exact |
| 0x48d180 | level_light_get | exact |
| 0x48d1b0 | fx_summon_hook_stub | exact |
| 0x48d1d0 | fx_link_character_obj | likely (store order) |
| 0x48d280 | fx_link_character | exact |
| 0x48d2b0 | fx_find_spawn_point | likely (row assignments) |
| 0x48d660 | fx_disable_by_effect_name | exact |
| 0x48d6d0 | fx_pause_for_cutscene | exact |
| 0x48d720 | fx_destroy_spell_effects | exact |
| 0x48d770 | fx_resume_after_cutscene | exact |
| folded 0x4f13e0 | fx_link_hook, fx_summon_hook (names ours, D4411) | likely |

### unnamed_utrack_0x48d7c0.cpp (4: 2 exact / 2 likely)

| Address | Function | Confidence |
|---|---|---|
| 0x48d7c0 | pxo_login_send | exact |
| 0x48d810 | utrack_init | exact |
| 0x48d870 | pxo_login_request | likely (state switch, jump table 0x48d9e0) |
| 0x48da00 | pxo_login_poll | likely (in-place length decrement) |

## Facts found

- Effect kinds (+0xa4), now all traced: 0 world / added, 1 / 2 attached (2 follows the body centre), 3 camera
  (turned around each frame), 4 missile, 5 / 6 fading status effects (the character's alpha: 255 - 235 *
  progress), 7 ground effect (sound off after 20 s), 8 summon appearance (begin_emerge when the timer runs out),
  9 status holder ("spell-status.vfx" every 10 s), 10 stone ("stone-fade.vfx" when the paralysis ends), 11
  spell area (radius grows 4.72 / s to 5.003, hits each target once), 12 near (hit at frame 14), 13 facing, 14
  at (hit when the timer runs out; spell 0x66 hits everyone but the caster), 15 Titus' rock eruption (damage at
  frame 55.25), 16 an equipped item's effect (weapon effect only while the weapon is in the hand).
- Fire, blizzard and dark wall areas: spells 0x1d / 0x1a / 0x4e put status 14 (firewall; 20 s, 11 s for the
  fireballs), 0x23 status 15 (blizzard; only after 3 s), 0x1f status 17 (dark wall), each re-added every frame
  with 100 s and the caster's level (or the casting item's charge level) as the value; leaving all areas of a
  kind cures it (force).
- experience.tbl gains with a "D" before the number are random: floor(frand() * value + 1), at least 3.
- Arrows (spell 0x37) are weapon missiles without an effect file: a trail bitmap, 20 / s, and the caster's
  melee hit resolves on arrival.
- The PXO user tracker login: request type 0xd (login 33 + password 27 bytes), replies 0xe refused / 0xf
  accepted (10 bytes copied to pxo_login_reply), every reply acknowledged (type 0, code 5); resend every 3.5 s,
  give up after 30 s; port 10556.

## Doc / symbol disagreements

1. sum.symbols fx_find_spawn_point "(out pos, character, distance) point in front of the character": it fills an
   orientation (matrix) turned toward a free side (forward / right / back / left), D4403.
2. sum.symbols fx_create_spell_effect_near "(spell, owner handle, ..., time) ... at a point found by
   fx_find_spawn_point": (spell, caster, owner, ms), at the owner's position with that orientation.
3. sum.symbols fx_create_at_camera "with the negated camera matrix 0x60a29c": the negated copy is unused; the
   instance gets camera_view_orient (fx_frame negates it every frame).
4. sum.symbols fx_get_status_holder "the existing kind-10 or kind-9 effect object": a kind-10 (stone) effect makes
   it return -1; only kind 9 is returned.
5. sum.symbols status_effect_update_fx "(character, status)": the second argument becomes the effect's kind (5 / 6).
6. sum.symbols level_light_get "the 0x98-byte level light record": the 24 attached light slots (attached_light_slots).
7. sum.symbols xp_level_up_gains "level++ (cap 20)": the level is not capped, only the table index.
8. sum.symbols experience_parse_level "xp, skill points, level, 4 species groups": the third value is the skill
   cap (+5).
9. symbols/types.h xp_level hp_extra / ap_extra "perhaps a random range": flags, "D" in the table (D4406).
10. docs/sum-exe/effects.md kind table: kind 10 is the stone effect (status_stone_fx, "stone-loop.vfx"), not an
    aura; kinds 0, 1, 5, 6, 8, 12-16 are now known (see Facts); the 0x200 flag in the `+0x84` row is a bit of
    +0xbc (fx_flags, status effects), not of +0x84.
11. engine/ai/unnamed_avoidance_0x4a6710.h avoid_area_desc::_unk00 "0 at every caller": summon_spawn_effect passes 1.
12. engine/rendering/levelrender.h places shadows_init / fx_shutdown "between options_apply_detail and the
    experience table": they are after the experience table, in the effect object unit.

## Game bugs (candidates, unchecked in play)

1. **fx_frame area list overflow (latent):** the fire / blizzard / dark wall areas go into a 10-entry stack array
   (esp+0x230..0x257, 0x48a40a-0x48a416) with no bound check; an 11th active area overwrites the saved
   registers / return address of fx_frame.
2. **fx_frame area loop stops at a dead caster:** `if (!caster) break` (0x48b40a) ends the loop for that
   character, so areas after one whose caster is gone neither burn nor freeze, and their status is cured at
   once (the cures after the loop run). A wall outliving its caster can switch off the next walls.
3. **fx_create_world sky list (latent):** 5 entries, no bound; a sixth "sky" world effect writes fx_count
   (0x2318e40), the next global.
4. **fx_new keeps the slot's old spell / owner:** status holders (kind 9) and item effects (kind 16) inherit a
   stale spell (+0xa8): fx_count_spells_1a_1d_4e (boss AI) can count a holder that reuses a fireball's slot, and
   fx_destroy_spell_effects (cutscene start) destroys such holders.
5. **fx_disable_by_effect_name NULL instance (latent crash):** reads instance->category without a NULL check
   (0x48d67a); arrows (kind 4 weapon missiles), status holders (kind 9) and sheathed item effects (kind 16)
   have no instance. Callers include ls_masad_reset_fire and tutorial_cond_combat_part2, which run mid-level.
6. **fx_is_finished NULL instance (latent):** no check before get_time_left (0x48d02c).
7. **Network game: spell_effect not checked in fx_frame kinds 12 / 14** (0x48abcf, 0x48ae06, 0x48af99) while
   kind 11 and fx_missile_hit check it; a NULL record is read.
8. **fx_create_spell_area single player:** calls character_is_enemy on the caster without checking that the
   caster still exists (0x48c77e).
9. **Dead code:** the 16 blob shadow bitmaps are built at start-up and never used (D4410); fx_frame's
   `duration < frametime` exit can only trigger for an area whose spell matches none of the five (none).

## Open questions

- Whether the containers / experience / effect blocks are one file (D4400).
- fx_frame: in the binary case 16's "keep" lives in done's stack byte; written as a second variable.
- vfx_base +0x16 / +0x2c records (0x21c) and +0x40: meaning not known (names ours).
- fx_create_instance's NULL-position / orientation defaults: probably the addresses of temporaries in the
  original (VC6 allows it); written as locals.

## README row (src/decomp/README.md)

| `game/unnamed_containers_0x489730.cpp`, `unnamed_experience_0x489930.cpp`, `unnamed_fx_0x489c40.cpp`, `unnamed_utrack_0x48d7c0.cpp` | container pool, experience table, effect objects (fx_frame and the fx_* creators, blob shadow bitmaps), PXO user tracker login (**names not known**) | 85 + 2 folded | 59 / 26 / 0 (`NOTES-unnamed-containers-fx-utrack.md`) |
