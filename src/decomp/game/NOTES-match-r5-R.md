# match28: second 8966 pass on script_editor, living_entity, render_scene, combat

Base: snapS/src/decomp (the merged state of this round). Compiler: vc6sp5 (c2 12.00.8966), per-unit flags from
src/vc6-flags.txt (unchanged). Decisions D8700-D8708 (appended to src/include/DECISIONS.md). Files: CHANGED.txt.

| File | Before (8966) | After | Won |
|---|---|---|---|
| game/unnamed_script_editor_0x4de0b0.cpp | 83 / 120 | 92 / 120 | 0x4e0080 remove_redundant_npc_navpoints, 0x4e6ad0 on_delete, 0x4e71e0 list_scripts_filter_cb, 0x4eaf70 script_name_cb, 0x4eb1c0 ambient_name_cb, 0x4ec000 door_key_cb, 0x4ec100 door_item_cb, 0x4ec570 door_lock_ok_cb, 0x4eca00 level_item_cb |
| game/living_entity.cpp | 98 / 132 | 107 / 132 | 0x45a6e0 set_locomotion_speed, 0x45b6e0 update_status_effects, 0x45d690 update_footsteps, 0x45da60 perform_spell_cast, 0x4627c0 equip_bonus, 0x462a20 set_underwear_textures, 0x462c80 set_armour_alt_texture, 0x462d60 is_mount, 0x462d70 is_rider |
| game/unnamed_render_scene_0x481680.cpp | 50 / 77 | 51 / 77 | 0x481c40 character_draw_overhead_bitmap |
| game/combat.cpp | 46 / 63 | 47 / 63 | 0x466f00 update_attack |

Total 277 -> 297 (+20), none lost (address by address, out/base_*.json vs out/final_*.json). Whole tree with the
header change: 4,783 -> 4,803 (match9/sweep.mjs on snapS and on this tree: the 20 wins, 0 lost). mingw
`-fsyntax-only -std=c++98` gives 0 errors in all 259 .cpp. Every function that matches is marked exact; no
exact marker on a differing function in the four files.

## What worked

- **Shrink-wrapping (D8701).** 8966 pushes EBX / ESI / EDI late, inside the region that needs them, when that
  region has one exit; with two `return`s in it the pushes move to the entry. This is the "partial prologue"
  of the type-ahead callbacks the first pass could not reproduce: SE_COMPLETE now ends its found branch with
  `goto complete_done;` (one `return true` at the end) and the callers declare `ui_listbox_entry *first =
  NULL;` at function scope (+5). The same rule gave on_delete (one `return true` for the whole pressed
  branch), door_lock_ok_cb (`rec = NULL` after the early return), remove_redundant_npc_navpoints (the element
  pointer set inside the index loop) and list_scripts_filter_cb (positive `if (has_suffix) {...} return
  false;`, name 0x24 bytes for the 0xb4 frame).
- **Header: navpoint's anonymous unions (D8703).** VC6's implicit struct copy copies every member of an
  anonymous union, so `navpoints[i] = navpoints[i + 1]` copied +0x24 and +0x28 twice; Sum.exe copies each once.
  The unions are now plain `float ground_height; int type;` (six `_unk24` / one `_unk28` uses renamed).
- **/Od frames (D8707).** The D6101 bucket renames work under 8966 (update_footsteps, update_status_effects,
  perform_spell_cast: the binary's slots are in the first decomp's declaration order, which 8966 here does not
  give). slotmap.mjs in this folder reads the binary's slot of each named local by instruction alignment;
  pickn.mjs picks names with nondecreasing buckets (ties declared newest-first). Also: /Od does no jump
  threading, so a jump straight to the epilogue is a `return;` (set_locomotion_speed, plus 16 unused frame
  bytes); a /Od switch with only `case 0: case 2:` holds the code that follows (update_status_effects);
  `if ((v = expr) < 0.0f) v = v; else v = 0.0f;` for an `fst / fcomp` without a reload; a 23-bit field read
  unoptimised is `(id_bits >> 9) & 0x7fffff`.
- **Optimised tail (D8700).** equip_bonus through an `ItemType *t` local; is_mount / is_rider as a dword copy
  narrowed to ubyte (`shr al`); set_underwear_textures with `defaults` declared first; set_armour_alt_texture
  as one else-if chain.
- **render_scene (D8704, D8705).** The overhead-bitmap tag fallback as its own `if`; add_effect_entry's two
  gr_cull_sphere calls; the draw-entry allocator as a macro that yields the success (Sum.exe never NULL-tests
  the new entry; the inline function returning NULL makes 8966 test it). The macro wins nothing alone yet.
- **update_attack (D8706)**: the jump table read from Sum.exe (0x467008: 2, 3 -> 0x466fff) shows HIT and
  DEFENCE go idle at once: `case COMBAT_HIT: case COMBAT_DEFENCE: break;` (a logic fix and a match).

Tools here: cmp.mjs / try.mjs (variants; `FROM=n` prints the side-by-side from line n) / apply.mjs (apply one
variant) / tv.sh and td.sh (apply, compile, report one address, restore) / v.sh + sd.sh (verbose compare and
side-by-side) / slotmap.mjs / pickn.mjs / fnren.mjs / renvar.mjs / probe.mjs; probes/ has the variant files.

## What still differs and why

- script_editor (28): door_sound_inputbox_cb (the binary holds the list pointer in ESI from before the region,
  pushed at entry: no source form found), default_action_navpoint (the NULL is kept in ESI), focus_selection,
  edit_button_cb (one exit gets the late pushes, but the binary lays the post-switch tail right after the
  WANDER case), the large ones (create_ui, handle_input, write_* tables, script_create, place_creature,
  fill_action_lists, select_prev, action_format).
- living_entity (25): get_resistance (8 frame bytes with the curse kept on the FPU; the else-branch form gives
  4), say (quoted[256] and the index stores now right; the strncat loop loads lens / offsets in the other
  order), scale_stats_to_level (fidiv and the two ?: temporaries), the vector3::len float temporary (D7804:
  get_speed, anim_speed_at_blend, stopping_distance, distance_to_reach_*), ai_travel_offset (temporary store
  order), load_state (its function-scope frame is in declaration order too, but the LE_GET macros use i / b / h
  / ofs: renaming reaches into the macros, not done), the large /Od functions (init, monster_drop_loot,
  update_head_look, on_death, clip_move_to_navmesh, save_state, find_ground, get_hit_by_spell, ...).
- render_scene (26): add_effect_entry (8966 emits fvec.z * dz first for every term order; fvec.dot(d) gives the
  order but keeps dx alive), push_attachment / focus_selection / combat_get_weapon_tag_position: in Sum.exe
  the matrix assignment's row copies call vector3's copy constructor (0x410cd0) and operator= (0x413190) out
  of line after two expansions, 8966 here expands them all (an inlining-budget question for the vector.h /
  matrix.h inline members; not touched), add_items / add_containers / add_lobj (FPU order, D7803), the EH-frame
  draw functions.
- combat (16): chain_check_window (8966 keeps the second branch's copy of the merged learn tail and cross-jumps
  the first NONE return into it), resolve_hit (no_flinch lives in EDI in the binary), get_protection,
  ai_choose_special, roll_defense, melee_damage, apply_melee_hit, resolve_exchange, trigger_*_ability,
  blood_splot_place, send_net, combat_position_modifier.

## Logic errors fixed (decomp) and game bugs

- D8706, living_combat::update_attack 0x466f00: states HIT (2) and DEFENCE (3) set `state = COMBAT_IDLE` at
  once (table 0x467008 -> 0x466fff); the decomp waited for move trigger 6 for them.
- D8702, script_editor_default_action_navpoint 0x4e8cf0: when the level search loop runs out, the binary
  returns &navpoints[navpoint_count - 1] (0x4e8d41 returns EAX), not NULL. Game bug (known): the loop tests the
  type through a NULL pointer (reads 0x28), so with any level navpoint the Cutscene branch faults.
- Not a decomp change: list_scripts_filter_cb's name buffer is 0x24 bytes in the binary's frame; with a long
  level name plus "_script.tbl" str_copy_append_32 bounds the copy (32 characters), so it fits.
