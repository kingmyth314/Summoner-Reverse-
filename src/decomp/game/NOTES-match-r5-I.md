# match19: living_entity, combat, render_scene, script_editor under cl 12.00.8966

Base: snapH/src/decomp. Compiler: vc6sp5 (c2 12.00.8966), per-unit flags from src/vc6-flags.txt (unchanged).
Decisions D7800-D7807 (src/include/DECISIONS.md). Files: CHANGED.txt. No header changes, so no other unit is
affected; mingw `-fsyntax-only -std=c++98` gives 0 errors for all four files.

| File | Before (8966) | After | Won |
|---|---|---|---|
| game/living_entity.cpp | 91 / 132 | 98 / 132 | 0x44f020 is_in_view, 0x44ffc0 ring_add_xp, 0x450420 rings_add_xp_all, 0x450c70 add_ap, 0x455e70 walk_to_point, 0x45d4c0 find_foot_bones, 0x4628a0 apply_hero_model |
| game/combat.cpp | 35 / 63 | 46 / 63 | 0x4637d0 pick_parry, 0x4645f0 play_skill_attack, 0x464cb0 defense_in_time, 0x466a80 play_weapon_hit_sound, 0x466d20 update_exchange, 0x467020 update_ranged, 0x4681b0 construct, 0x468210 update, 0x4685c0 apply_net_exchange, 0x468740 is_busy, 0x468a90 operator* |
| game/unnamed_render_scene_0x481680.cpp | 48 / 77 | 50 / 77 | 0x4848c0 draw_health_bars, 0x485810 draw_speaker_icons |
| game/unnamed_script_editor_0x4de0b0.cpp | 77 / 120 | 83 / 120 | 0x4e50f0 list_ambient_sounds, 0x4e57a0 show_door_lock, 0x4e7120 list_scripts, 0x4e7800 select_next, 0x4eb2e0 ambient_field_cb, 0x4ebcf0 door_field_cb |

Total 251 -> 277 (+26), none lost (checked address by address against the base run, out/base_*.json vs
out/final_*.json). Matched functions are marked exact; ten "exact" markers from 8168 matches that differ under
8966 were lowered to likely (D7807).

## What worked (details D7800-D7806)

- Unoptimised part of living_entity: self-clamps `x = max(x, 0)` are direct stores per branch in the binary,
  so they are written as if / else (ASSIGN_MAX / ASSIGN_MIN macros, D7800: +3). The other ternaries do use a
  temporary in the binary (rewriting them lost add_hp and update_footsteps: reverted).
- ROUND_TO_STEP as an inline function (unfolded `1.0 * 0.5`, D7801); an inline wrapper for find_mesh_tag on
  `instance->character` (the /Od this-temporary, D7802); vectors constructed in place from the inline operators
  (`vector3 dirn = p - pos;` works under 8966); D6101's name-hash frame order still holds under 8966 (probe).
- Jump-table switches with the case bodies in the order of the table targets (read from Sum.exe), cases with
  their own bodies where the table has separate entries, two-value tests as switches, separate returns per
  branch where the binary repeats a tail (D7803).
- Smaller: `? true : false` bools, call results through locals, `ItemType *t` before a branch, `float v` for
  an atof result, whole file_find_node list heads with two separate head stores, len_sq() for a distance test,
  an explicit GameTimer constructor call (D7805), a unit const float 2.0 (D7806).

Tools in this folder: cmp.mjs (compare one file, won / lost against out/base_*), try.mjs (variants, root
fixed to this tree and its vc6-flags.txt), sd.sh (side-by-side after a fresh compile), probe.mjs (compile a
probe .cpp with 8966 and print its code; probes/ has the ones used for D7800-D7806).

## What still differs and why

- living_entity /Od (27): the `(float)sqrt` conversion temporary of vector3::len (D7804; get_speed,
  anim_speed_at_blend, stopping_distance, distance_to_reach_*), `(float)a / (float)b` emitted as fidiv where the
  binary has fild / fild / fdivp (scale_stats_to_level, nothing found), larger frames with more temporaries in
  the big functions (init, monster_drop_loot, update_head_look, on_death, clip_move_to_navmesh, save / load
  state, get_hit_by_spell). Optimised tail: is_mount / is_rider (`shr al` byte ops on a dword load),
  equip_bonus and say (register choice), set_armour_alt_texture (cross-jumping), get_resistance.
- combat (17): update_attack (the binary's table has states 2 / 3 pointing at the default body; every source
  form gives either no table or a second body), chain_check_window (now the right table and both tails; the
  SKILL_NONE return is cross-jumped by 8966), get_protection (the e6 load is hoisted in the binary, block
  placement), ai_choose_special (the push-skill test placed after the loop), send_net / combat_position_modifier
  (register choice, FPU scheduling), the big ones (melee_damage, apply_melee_hit, resolve_exchange,
  roll_defense, trigger_*_ability).
- render_scene (27): add_items / add_containers keep two differences on the FPU stack (no source order gives
  it: 8966 canonicalises the squares), level_vis_reset (biased pointer / register), setup_level_lights (count
  register), level_set_current_layer (camera flag bitfields would need a header change: not done), EH-frame
  functions (render_scope) and the draw lists.
- script_editor (37): the six type-ahead callbacks (SE_COMPLETE) save EBX / ESI / EDI only on the 0x15 path in
  the binary (partial prologue; 8966 pushes all at entry, a function-scope `first_ = NULL` gets the early
  `xor ebp, ebp` but not the deferred pushes); list_door_sounds' lock buffer sits 8 bytes above the
  block-scoped sprintf buffers; select_prev's compare order; the large ones (create_ui, handle_input,
  write_* tables).

## Logic errors and game bugs

None new. No behaviour was changed: every rewrite keeps the computation (defense_in_time keeps `!(mine <
theirs)` for NaN; the float `v` of the field callbacks is what the binary keeps on the FPU).
