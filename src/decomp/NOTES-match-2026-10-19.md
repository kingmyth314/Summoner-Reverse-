# Byte-match round 2026-10-19: the ten workers' notes

Merged into src/decomp (4,963 -> 5,077 strict matches; DECISIONS D10000-D10999). One section per worker, as written.

---

## Worker 1

### m-1019 worker 1 notes

Strict VC6 (8966) sweeps of the 21 files: 487 -> 500 match (+13), 1 `target`, 83 differ, 0 lost
(`w1-sweep-before.txt` / `w1-sweep-after.txt`). Full-tree strict sweep of the copy: 4976 match, 0 lost against
src/decomp/vc6-matches.txt (`w1-sweep-full.txt`). Decisions D10000 (shapes) and D10001 (header change).

## Per file (match before -> after)

| File | Before | After |
|---|---|---|
| game/characterinfo.cpp | 56/70 | 60/70 (+1 bytes-only `target`) |
| game/unnamed_ai_think_0x4b5140.cpp | 20/29 | 22/29 |
| game/unnamed_experience_0x489930.cpp | 5/8 | 6/8 |
| game/unnamed_murod_0x4c5140.cpp | 9/12 | 10/12 |
| vsdk/gr_direct3d.cpp | 36/50 | 40/50 |
| vsdk/gr_font.cpp | 13/17 | 14/17 |
| all other 15 files | unchanged | unchanged |

## New matches (Confidence raised to exact)

- 0x4899d0 experience_parse_level: walking `xp_level_gain *g`.
- 0x535eb0 gr_d3d_make_line_verts: `v1->rhw = v0->rhw = 1.0f` (v0 is stored first).
- 0x535550 gr_d3d_clear, 0x5355e0 gr_d3d_zbuffer_clear: `clip_left + offset_x` operand order.
- 0x53a7f0 gr_d3d_read_backbuffer: per-row memcpy with indexed addresses (no walking pointers).
- 0x448500 summon_restore_after_cutscene: body inside `if (stashed_summon) { ...; return; }`, offset store twice.
- 0x448000 summon_stash_for_cutscene: anim length in a float local before process().
- 0x448f60 statscreen_load_anim: HEADER `int heavy` (D10001), if/else of two find_move calls, `int anim` locals.
- 0x447b10 level_load_preload: name[0x20] / hash[0x20]; mark_anims_loaded as if/else of two calls.
- 0x4b5790 ai_state::retaliate_on_hit: `da * (intelligence * 4.0f) >= dt`.
- 0x4b5a80 ai_state::follow_repath: one vector3 for destination and prediction; float local for the time.
- 0x4c53c0 ai_state::murod_ai_leap: find_move result in a local before play_move.
- 0x508440 gr_font_get_char: `int n = f->num_kern_pairs` before the kern loop.

## What worked (8966)

Call-in-argument into a local (D6005), `c ? a : b` argument as if/else of two calls, commutative operand
order, chained-assignment order, walking vs indexed pointers, merging two locals that share one slot in the
binary, buffer sizes read from the binary's frame offsets. 8966 places a local in a dead argument slot by
itself (level_load_preload's mp).

## Closer, still differing (forms kept, behaviour identical)

- characterinfo_alloc_level_buffers 0x448800: apply_hero_model as if/else of two calls -> bytes match, strict
  `target`: the four anim-buffer-size globals are loaded in another order (sum order has no effect, all 24
  tried). Probably global symbol order; not changed (would move definitions).
- gr_d3d_tmapper_multipass: `float w` + `d3d_tlvertex *d` makes the first loop identical; later the binary
  hoists the pass->mode load above the sectioned test.
- d3d_compute_pixel_format_shifts: rewritten per channel (`for (x_shift = 0, m = mask; ...)`); frame 0x1c now
  as the binary, register choice differs.
- character_learn_chain_skill: `int ap = floor(..) + 1; add_ap((float)-ap)` gives the binary's inc/neg and frame
  0xa0; EBP pushed in the prologue (binary pushes it late).
- living_entity_setup_head: `tag` local for $head; remaining eax/ecx/edx rotation in the argument leas.
- sound_emitter_delete: `return 0` inside the if (D8701) gives the entry pushes; link-store scheduling differs.
- ls_jadetemple_action: no `t` local gives the binary's registers; item_name / line slots stay swapped (names,
  declaration order and block scope tried).
- script_action_combat_mode: switch on the action type gives the binary's range compares; `on` in a's
  argument slot and skip in AL remain.
- vfx_tex_layer::get_frame_bitmap: `float fps` local fixes the FPU sequence; this/n register swap remains.

## Still differing, no form found

xp_for_level (step init scheduled before the loop test), xp_level_up_gains (one load order), ls_catacombs_use
(ball1 blocks merged at the end), gr_d3d_set_window (lea base/index), gr_d3d_init (store order),
d3d_enumerate_ddraw_devices (final branch polarity), d3d_enum_texture_format_callback, init_char_types,
character_use_inventory_item (register allocation), titus_ai_select_action (block order of the stage-1 retarget),
camera_capture (binary calls vector3(x, y, z) out of line), mp_levelselect_add_chat_line (max(x, 0) `and`
register, D7506), line_talker (register rotation), cmd_tentacles (constant 8 kept in EBP), and the large
functions (set_state, bitmap, parse_table, parse_objects, entry_start, conversation frame, ...) not attempted.

## Behaviour errors / game bugs

None found. All changes are equivalent forms.

## Files changed (in w1-src)

game/characterinfo.cpp, game/unnamed_ai_think_0x4b5140.cpp, game/unnamed_experience_0x489930.cpp,
game/unnamed_murod_0x4c5140.cpp, game/script_internal.cpp, game/level_script_jadetemple.cpp,
vsdk/gr_direct3d.cpp, vsdk/gr_font.cpp, vsdk/unnamed_vfx_material_0x564d70.cpp,
vsdk/unnamed_sound_emitter_0x546150.cpp, include/engine/characterinfo/characterinfo.h (D10001),
include/DECISIONS.md (D10000, D10001).

Checks: mingw -fsyntax-only on all 259 .cpp: errors only in game/unnamed_decals_0x4bbe80.cpp (9, identical in the
untouched base copy; not mine). The header change's only other user, game/unnamed_interface_0x411270.cpp,
compiles to identical VC6 output (56/69).

---

## Worker 2

### m1019 worker 2: VC6 8966 byte-match pass over 19 units

Tree: `m-1019/w2-src` (copy of src/decomp). Compiler: SP5 VC98 (cl 12.00.8966), strict mode, the tree's
vc6-flags.txt (unchanged). Before / after lists: `w2-before.txt` / `w2-sweep.txt` (vc6-sweep, one line per marked
function).

**477 -> 490 of 573 functions; none lost** (checked address by address against w2-before.txt). No header and no
flags change. mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in all 9 changed .cpp. Decisions D10100
(shapes that worked) and D10101 (tried, not reproduced) appended to `w2-src/include/DECISIONS.md`.

## Per file (matches before -> after / total)

| file | before | after | won |
|---|---|---|---|
| game/combat.cpp | 47 | 49 / 63 | 0x463540 send_net, 0x463e20 pick_attack_move |
| game/gamesound.cpp | 14 | 15 / 16 | 0x4691e0 music_feedback_parse_tables |
| game/level_script_sewers.cpp | 18 | 18 / 19 | |
| game/levelrender.cpp | 6 | 6 / 12 | (0x48de60 closer, see below) |
| game/new_buysell.cpp | 38 | 38 / 42 | |
| game/new_itemtrade.cpp | 29 | 30 / 36 | 0x41a7b0 itemtrade_draw_gold_input |
| game/unnamed_effectstream_0x440400.cpp | 11 | 12 / 14 | 0x4408a0 effect_stream_update |
| game/unnamed_mlayout_0x43f9d0.cpp | 7 | 8 / 9 | 0x43fb60 state6_enter |
| game/unnamed_mpmenus_0x43eb00.cpp | 13 | 13 / 14 | |
| game/unnamed_phoenix_0x497ac0.cpp | 33 | 33 / 36 | |
| game/unnamed_serpent_0x495290.cpp | 38 | 38 / 43 | |
| game/unnamed_swi_0x488230.cpp | 12 | 13 / 14 | 0x488ab0 mp_handle_swi_sneak |
| vsdk/file_packfile.cpp | 19 | 19 / 20 | |
| vsdk/gr_glide.cpp | 48 | 51 / 61 | 0x5473b0 glide_load_dll, 0x549500 get_section_rect, 0x549f50 glide_msg_handler |
| vsdk/parse.cpp | 45 | 45 / 51 | |
| vsdk/unnamed_ca_mesh_0x50af80.cpp | 8 | 8 / 12 | |
| vsdk/unnamed_movie_0x5172f0.cpp | 1 | 1 / 2 | |
| vsdk/unnamed_snd_0x5204c0.cpp | 48 | 51 / 58 | 0x5211f0 snd_calc_pan, 0x521270 sound_emitter_calc_pan, 0x521a60 sounds_parse_table |
| vsdk/unnamed_vector_0x523f30.cpp | 42 | 42 / 51 | |

All 13 won functions are now `Confidence: exact` with the shape named in the comment.

## What worked (D10100)

- Ternary call argument -> if / else of two identical calls (8966 tail-merges them): draw_gold_input, plus
  `min(0, x)` for `setg`.
- Assign the existing result local and return it once (calc_pan x2); per-case state stores + `break` + one return
  after the switch (effect_stream_update).
- Hoist nested calls / `i++` out of argument lists (swi_sneak, send_net, sounds_parse_table); read an array
  element into a local before passing it, unsigned loop index (music_feedback_parse_tables).
- Outer element pointer first (get_section_rect); if / else assignment instead of init-then-overwrite
  (glide_msg_handler); `first = NULL` just before the loop (state6_enter).
- Vector difference with y cleared + inline `dot` (pick_attack_move): the coordinator's tip that 8966 builds vector
  temporaries as the binary does held here.
- glide_load_dll: odd but exact: the first GetProcAddress reads the global, the rest a local `h` copied after it.
  Same behaviour; recorded as odd in D10100.

## Closer, still differs

- levelrender_wave_lobj 0x48de60: the static GameTimer is now declared after the lobj_find_textured_record call
  (the binary constructs it after the call); first difference moved from +0x0 to the loop counter (the binary keeps
  i in a frame slot, frame 0xb8 vs 0xb0).

## Still differing, tried (D10101)

snd_set_volume / set_volume_in_group / set_pan / set_group_volume (out-of-line clamp stores, 26 forms);
snd_calc_volume_3d / sound_emitter_calc_volume (`zero` kept in memory); effect_stream_finish_entry (block
placement of the post-switch tail; jump-table layout recorded); effect_stream_start; chain_check_window (identical
returns not merged in the binary); get_protection; combat_position_modifier (lea scheduled before the dot; 48 dot
orders and vector forms tried); fire_weapon_missile (frame); unrotated (48 orders + structural forms);
length_quick (`t + t*0.5f` folded); random_unit_vector / random_direction / _in_cone (FPU leftover);
vector2_closest_point_on_segment / dist_sq_to_line; itemtrade_count_party_rows; itemtrade_draw_list1/2
(register choice); mp_handle_swi_path (bool through dead arg slot, six forms); serpent_part_is_busy_or_asleep;
movie_play (File frame slot); packfile_read_directory (p in pf's dead slot: declaration / scope / walking
`sector` forms). Not attempted beyond the first-difference look (register allocation / frame layout in large
functions): the rest of combat, levelrender draw lists, gr_glide texture functions, ca_mesh renderers, parse
register choices, phoenix / serpent AI, buysell / itemtrade click handlers, sewers spline.

## Behaviour errors / game bugs

None found in this pass. Every change is a same-behaviour source reshaping.

## Files changed (in w2-src)

game/combat.cpp, game/gamesound.cpp, game/levelrender.cpp, game/new_itemtrade.cpp,
game/unnamed_effectstream_0x440400.cpp, game/unnamed_mlayout_0x43f9d0.cpp, game/unnamed_swi_0x488230.cpp,
vsdk/gr_glide.cpp, vsdk/unnamed_snd_0x5204c0.cpp, include/DECISIONS.md.

---

## Worker 3

### m-1019 worker 3 notes

Strict VC6 (8966) sweep of the worker copy w3-src. Before: 488 match / 96 differ in the 18 files. After: 502 / 82
(+14). Whole tree: 4977 match, `--check src/decomp/vc6-matches.txt`: 0 lost (4963 there). mingw -fsyntax-only over
all 259 units: 0 errors except game/unnamed_decals_0x4bbe80.cpp (9, the same in the repo; not touched).

## Per file (matches before -> after, of total)

| file | before | after | total |
|---|---|---|---|
| game/unnamed_script_editor_0x4de0b0.cpp | 92 | 96 | 120 |
| game/level_scripts_common.cpp | 67 | 70 | 78 |
| vsdk/unnamed_ds_0x55bf90.cpp | 51 | 52 | 60 |
| game/pathfinding_visibility.cpp | 4 | 4 | 11 |
| vsdk/gr.cpp | 55 | 55 | 61 |
| game/new_automap.cpp | 26 | 26 | 32 |
| game/unnamed_goals_doors_0x475180.cpp | 62 | 62 | 67 |
| vsdk/unnamed_clip_0x5590b0.cpp | 13 | 15 | 17 |
| game/multisave.cpp | 4 | 4 | 8 |
| game/unnamed_pyrul_0x4b87c0.cpp | 20 | 21 | 23 |
| game/unnamed_cutscene_camera_0x493e50.cpp | 9 | 9 | 12 |
| vsdk/skeleton.cpp | 26 | 26 | 28 |
| game/unnamed_level_complete_0x4b0960.cpp | 8 | 9 | 10 |
| game/item_paging.cpp | 6 | 7 | 8 |
| vsdk/unnamed_movement_0x531d20.cpp | 3 | 3 | 4 |
| vsdk/frametime.cpp | 11 | 12 | 12 |
| game/unnamed_machival_0x4c0c40.cpp | 25 | 25 | 26 |
| game/level_script_slums.cpp | 6 | 6 | 7 |

## New matches (Confidence raised to exact)

0x46c940 item_page_in, 0x4cb430 level_items_parse_table, 0x4cd120 level_trigger_trip, 0x4cd330
level_ambient_find_at, 0x4b0b50 mp_level_find_boss, 0x4b94a0 pyrul_cast_special, 0x4e53a0
script_editor_list_door_sounds, 0x4e6040 script_editor_update_buttons, 0x4eb5c0 script_editor_params_ok_cb,
0x4ec730 script_editor_creature_ok, 0x5016f0 frametime_set_limits, 0x559350 gr_clip_line, 0x5593a0
gr_clip_plane, 0x55d2b0 ds_play_streamed.

## What worked (details in DECISIONS D10200-D10203)

- Inline `vector3::operator==` instead of three written compares (loop layout, biased pointer).
- Case bodies in the binary's order; `break` to a shared `return true` plus `default: return false`.
- gotos for shared tails (mp_level_find_boss: one `mp_level_boss = c` for 14 boss loops).
- else-branches placed where the binary has them (failure message as else; body inside `if (event == PRESSED)`).
- Constants first in float compares (`0.0f < v || 100.0f > v`).
- Local pointer / global-index choices (trigger path local; item_page_slots[slot]; ds_channels[ch]).
- Statement order (pointer increment after the stores; swap order).
- Frame layout from the binary's stack offsets (two buffers 8 + 16; one buffer shared between door key and click
  name, which fixed level_script_parse_table's frame).
- Float copied as int through int pointers (frametime_set_limits, coordinator tip).
- HEADER CHANGE D10200: ds_play_streamed's looping is bool (passed on to snd_stream_start's bool unnormalised).

## Improved but still differs

level_parse_shopkeeper (name stored in the search loop, unknown-item message as else; the duplicate scans jump
straight to the next item), automap_label_text_input (typed branch first, signed strlen compare; `changed = true`
scheduling), level_script_parse_table (frame now 0x408 as the binary; register use of the zero constant),
ai_state::goal_find (pointer bias right; loop tail), bbox_silhouette_corners (table row pointer; out / i registers).

## Still differs (not reproduced; D10203)

pathvis_add_edge_node (list unlink through &prev/&next), matrix3_pack_orientation (dead argument slots: tried
block scopes, frame drops to 8 but `a` not in the arg slots), ds_is_3d_buffer (byte shr/and), ds_play (channel
index register), default_action_navpoint, automap_center_on_player (conversions sunk), pyrul_fire_attack (vector3
constructor 0x43b580 called out of line in the binary), draw_navpoints (cross-jumped draw calls),
edit_button_cb (late register pushes), mp_player_file_create (`cmp eax, ebx`), many register-choice / FPU
scheduling cases (gr.cpp, skeleton, movement, machival, multisave, cutscene camera).

## Behaviour errors / game bugs

- No behaviour error fixed. All changes keep behaviour (the ds_play_streamed priority argument is ignored by
  ds_find_free_channel, so passing its raw bits instead of (int) changes nothing).
- Possible game bug (not verified in game): script_editor_list_door_sounds formats the three door sound
  parameters with "%.1f" (already noted: bytes read as a double, garbage) into an 8-byte stack buffer (binary
  frame: buffer at +0xc, the lock-percent buffer at +0x14). A garbage value of 100000 or more writes past it into
  the next buffer / the file list (0x4e53a0 region; sprintf calls in the open/close sound blocks). Editor only.

## Files changed (in w3-src)

game/item_paging.cpp, game/level_scripts_common.cpp, game/new_automap.cpp, game/unnamed_goals_doors_0x475180.cpp,
game/unnamed_level_complete_0x4b0960.cpp, game/unnamed_pyrul_0x4b87c0.cpp, game/unnamed_script_editor_0x4de0b0.cpp,
vsdk/frametime.cpp, vsdk/unnamed_clip_0x5590b0.cpp, vsdk/unnamed_ds_0x55bf90.cpp, include/vsdk/snd/ds.h (D10200),
include/DECISIONS.md (D10200-D10203).

---

## Worker 4

### Worker 4 notes (match round 2026-10-19)

Strict VC6 sweep (8966, VC6_STRICT=1) of the 19 units, base snapshot against my copy: 368 -> 381 matching
functions (+13), none lost (checked address by address: w4-before2.txt / w4-after.txt in this folder).
Decisions D10300-D10311 are in w4-src/include/DECISIONS.md. No header changed. mingw -fsyntax-only: 0 errors in
every changed file.

## Match counts per file (before -> after, of compared)

- game/pathfinding.cpp: 22 -> 22 of 36
- game/new_pxochat.cpp: 43 -> 46 of 57
- game/new_hud.cpp: 49 -> 50 of 60
- game/unnamed_tiger_0x499e50.cpp: 39 -> 42 of 48
- vsdk/unnamed_vif_0x5329f0.cpp: 8 -> 8 of 15
- vsdk/effect_mem.cpp: 33 -> 33 of 39
- game/new_saveload.cpp: 31 -> 32 of 37
- game/unnamed_evil_urath_0x4c4170.cpp: 13 -> 15 of 18
- vsdk/unnamed_d3d_tcache_0x54bb30.cpp: 10 -> 11 of 14
- game/multi_chattracker.cpp: 16 -> 16 of 20
- game/unnamed_snow_0x4946a0.cpp: 8 -> 9 of 11
- game/summoner_cd.cpp: 3 -> 3 of 6
- vsdk/unnamed_exceptionhandler_0x5419a0.cpp: 6 -> 6 of 8
- game/unnamed_giant_salamanka_0x4c8030.cpp: 17 -> 18 of 19
- game/new_assess.cpp: 16 -> 16 of 18
- vsdk/unnamed_inetgetfile_0x51b760.cpp: 9 -> 9 of 10
- vsdk/memory.cpp: 18 -> 18 of 19
- game/unnamed_item_info_popup_0x418580.cpp: 22 -> 22 of 23
- game/multi_door.cpp: 5 -> 5 of 6

## New matches (13)

- new_pxochat.cpp 0x423370, 0x4233d0, 0x423420 (popup frames): a dead compare is VC6 merging two identical blocks;
  written as `if (choice < 0) {...}` before the same code, and `else if (choice == 1) X; else X;` (D10300).
- unnamed_snow 0x4948a0 snow_cell::fill_count: SNOW_FALL_RATE is a const float variable (fdiv from memory; a
  literal becomes a reciprocal multiply) (D10301).
- unnamed_evil_urath 0x4c4ca0 leap_land: `vector3 facing; facing = a + b;` gives the temporary + copy (D10302).
- unnamed_evil_urath 0x4c4200 leap_check_path: vector2 ctors in place, loop count in a local, if-form bool (D10303).
- unnamed_giant_salamanka 0x4c86b0 ai_frame: `owner->combat.target = target;` (D10304).
- unnamed_tiger 0x49b350 tiger_choose_attack, 0x49c240 tiger_stage_move_update: branch shapes (D10305).
- unnamed_tiger 0x49a830 tiger_pick_wander_point: in-place dir *= / +=, dot product order found by trying all 48
  orders (D10308).
- new_hud 0x40e7c0 quicksave: wrong local sizes; head is a whole 0x38 node, filename[32], message[64] (D10306).
- new_saveload 0x427100 saveload_capture_thumbnail: `float fov = camera_fov;` (fld/fstp argument) (D10307).
- unnamed_d3d_tcache 0x54d000 gr_d3d_get_section_rect: no local section pointer (D10310).

## What worked (techniques)

- Merged-identical-branch shapes for compares that have no branch.
- Brute force of term / factor order in a 3-term dot product with vc6-try (48 variants in one run).
- Recounting frame offsets with the pending pushes before trusting an old comment about buffer sizes.
- const float variables for constants the binary divides by from memory.
- Float locals for float arguments the binary passes through the FPU.
- Flags: vsdk/unnamed_vif_0x5329f0.cpp to /Ox /Ob0 /GX /MT /Ow (binary calls the vector operators out of line;
  same 8 matches, the others much closer) (D10309, vc6-flags.txt changed).

## Still differs (D10311 for the list)

max(x,0) and-register (5 functions), shrink-wrapped pushes, the segment search of vif_emitter_spawn_point /
vif_curve__evaluate, door_pick_by_mesh's fcompp compare, frame-slot order in leap_travel / leap_rise /
pxochat_handle_click / saveload_draw_info, register choice (pxochat_page_down, tiger_ai_frame, salamanka
select_action, tiger_anim_remap_in), no-CD patch in summoner_cd.cpp, and the large pathfinding functions (not
attempted in depth). Partial improvements kept (still differ): leap_travel / leap_rise vector forms, snow_update
`atanf` (now matches up to the grid-shift loops).

## Behaviour errors / game bugs

None found in source vs binary. quicksave's message buffer is 64 bytes in the binary (the source had 72): a
localized message over 63 characters would overflow it in the original (noted, not a change of behaviour now that
the source follows the binary).

## Files changed (in w4-src)

- game/new_pxochat.cpp
- game/new_hud.cpp
- game/unnamed_tiger_0x499e50.cpp
- game/new_saveload.cpp
- game/unnamed_evil_urath_0x4c4170.cpp
- vsdk/unnamed_d3d_tcache_0x54bb30.cpp
- game/unnamed_snow_0x4946a0.cpp
- game/unnamed_giant_salamanka_0x4c8030.cpp
- vc6-flags.txt
- include/DECISIONS.md (D10300-D10311 appended)

---

## Worker 5

### Worker 5: match round 2026-10-19

Strict VC6 sweep (cl 12.00.8966, `VC6_STRICT=1`) of the 17 files. Before: 398 of 494 matched (96 differ).
After: 405 of 494 (89 differ). Checked address by address with `vc6-sweep.mjs --check w5-before.txt`:
0 lost. mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in the four changed files. No header changed.

| File | Before | After | Functions |
|---|---|---|---|
| game/level_scripts.cpp | 85 | 87 | 124 |
| game/unnamed_ai_combat_0x4bce60.cpp | 19 | 21 | 28 |
| game/gametarget.cpp | 14 | 15 | 22 |
| game/items.cpp | 27 | 29 | 31 |
| game/unnamed_shadows_0x4a37d0.cpp | 7 | 7 | 13 |
| game/level_script_khosani_lab.cpp | 39 | 39 | 45 |
| vsdk/unnamed_vfx_0x536950.cpp | 24 | 24 | 28 |
| vsdk/unnamed_floatmath_0x542500.cpp | 16 | 16 | 20 |
| game/unnamed_states_0x431870.cpp | 11 | 11 | 14 |
| game/new_status.cpp | 48 | 48 | 51 |
| vsdk/unnamed_poly_clip_0x5519c0.cpp | 6 | 6 | 8 |
| game/unnamed_ghost_rider_0x4b9e70.cpp | 36 | 36 | 38 |
| game/new_credits.cpp | 7 | 7 | 9 |
| vsdk/unnamed_ftp_0x559ea0.cpp | 15 | 15 | 16 |
| vsdk/plainstring.cpp | 19 | 19 | 20 |
| game/unnamed_gamelist_0x4705b0.cpp | 13 | 13 | 14 |
| game/multicontainer.cpp | 12 | 12 | 13 |

## Won (7), Confidence raised to exact

- items.cpp item_create 0x44b2d0: `it = NULL` after the range test, each case assigns and breaks, one return
  (D10400).
- items.cpp character_meets_item_requirements 0x44b550: early `if (c->type != 7 || (c->flags & 0x100)) return
  true;` (D10400).
- gametarget.cpp container_examine 0x43c420: `char *fmt = XSTR(...)` before the localize_item_name call (D10401).
- level_scripts.cpp savegame::write_character 0x4cf6c0 and savegame::automap_label_add 0x4d12b0: the float
  fields written through `float t = (src); *(int *)dst = *(int *)&t;` (fld / fstp / integer move through a dead
  argument slot; D10402).
- unnamed_ai_combat approach_random_offset 0x4bcea0: `dir = vector3(...)` via `vector3 *from = &owner->pos`,
  `dest = dest - dir * r`, success branch first (D10403).
- unnamed_ai_combat approach_target_point 0x4bcff0: candidate formed in dest, best kept in best_pos and copied
  back, `best = -1.0f` before best_pos (D10403).

## What worked

Finding: the compiler in the toolchain is 8966, so several D6306 "not reproducible" items are reproducible now:
arguments slots reused for locals (write_character, automap_label_add, approach_target_point) and vector
temporaries constructed like the binary's. Useful patterns: early-return restructuring, format strings into
locals, parenthesised float temporaries for `fld/fstp/mov` copies, `a = vector3(...)` / `dest = dest - v * s`
for vector code, the order of initial assignments deciding which local takes a dead argument slot.

## Kept partial progress in functions that still differ (behaviour-equivalent, intentional)

- gametarget.cpp object_try_unlock_or_open 0x43c060: key search as if / else with a `Character *c` local; client
  door branch tests `d->key` first. Remaining: the binary's duplicated message tails.
- gametarget.cpp world_pick_lobj 0x43a770: `best > h.t`. Remaining: the binary stores zeros as immediates where
  ours caches 0 in EAX.
- level_scripts.cpp savegame::write_item_objects 0x4d2370: float temporaries, the container loop over an index
  (`for (int ci ...)`, `c = &containers[ci]` after the tests). Now same size and instructions; remaining: frame
  order of count and the temporary.
- level_scripts.cpp savegame::automap_label_remove 0x4d1390: rx / rz integer copies, `off = 4` before the count
  read. Same mnemonics and size; remaining: the slot of `start`.

## Still differs (D10404)

Register choice with identical code (floatmath fast_* trio, state_title_enter, loadgame_level_list_cb,
level_format_display_name, get_int, level_item_has), the binary re-testing `i >= n` after search loops
(get_speaker_bit, set_speaker_bit, get/set_topic_bit, read_level_items, level_item_add/remove), tail
duplication in the binary (savegame_location_allowed, object_try_unlock_or_open), stack-slot order
(status_handle_click, the two above), prologue push placement (consider_life_drain, int_to_string_with_commas),
dead-argument-slot vector2 temporaries in the khosani corner_nn/np/pp (vector2 forms tried; the binary forwards
x but stores y), credits_parse_table / credits_draw_lines register and memory-resident alpha, approach_attack_slot
and the shadows / vfx functions (register and frame layout; not attempted in depth).

## Behaviour errors / game bugs

No behaviour errors or game bugs found. (distance(&owner->pos, &dest) in approach_target_point swaps the
arguments of a symmetric |a - b|; no behaviour change.)

## Files changed (in w5-src)

- game/items.cpp
- game/gametarget.cpp
- game/level_scripts.cpp
- game/unnamed_ai_combat_0x4bce60.cpp
- include/DECISIONS.md (D10400-D10404)

---

## Worker 6

### w6 notes (match pass, cl 12.00.8966, strict)

Tree: `m-1019/w6-src`. Before / after sweeps: `w6-before.txt` / `w6-after.txt` (my 19 files);
full-tree strict sweep with `--check w6-src/vc6-matches.txt`: **4963 -> 4991 matches, 0 lost** (`w6work/full.txt`).
My files: **453 -> 481 of 549, no match lost** (checked address by address). Decisions D10500-D10506 appended to
`w6-src/include/DECISIONS.md`. mingw `-fsyntax-only -std=c++98 -w -Iinclude` over all 259 .cpp: 0 errors except
game/unnamed_decals_0x4bbe80.cpp (address of a temporary, pre-existing: the base tree gives the same errors).

## Per file (matches before -> after / functions)

| file | before | after | won |
|---|---|---|---|
| vsdk/unnamed_vfile_0x5128b0.cpp | 64 | 73 / 82 | FileReader::read, read_int, read_float, read_int16, read_uint16, read_int8, read_uint8, read_vector, read_quaternion |
| game/unnamed_interface_0x411270.cpp | 56 | 58 / 69 | ui_draw_text_box 0x4135a0, ui_message_box_hit_test 0x414110 |
| vsdk/unnamed_ix_0x53ac20.cpp | 25 | 25 / 35 | |
| game/ai.cpp | 79 | 81 / 88 | offset_point_sideways 0x4729b0, add_skill_from_behind 0x4741d0 |
| game/multispell.cpp | 15 | 20 / 22 | 0x4afca0, 0x4afd60, 0x4b02a0, 0x4b06a0, 0x4b08f0 |
| game/unnamed_netpackets_0x441820.cpp | 6 | 10 / 12 | multi_process_packet, mp_send_tracked, multi_send_packet_buffered, multi_send_packet |
| vsdk/unnamed_localize_0x503f60.cpp | 23 | 23 / 28 | |
| game/unnamed_evil_joseph_0x4c3470.cpp | 14 | 16 / 19 | spawn_helpers 0x4c3500, boss_load 0x4c35e0 |
| game/unnamed_murod_oni_0x4c5c20.cpp | 10 | 13 / 14 | boss_load, update_offense_points, collect_area_hits |
| vsdk/unnamed_vfx_channel_0x55b440.cpp | 16 | 16 / 19 | |
| vsdk/character.cpp | 31 | 31 / 34 | |
| game/new_spell.cpp | 35 | 35 / 38 | |
| vsdk/unnamed_ui_button_0x526940.cpp | 22 | 22 / 24 | |
| game/unnamed_editor_dialogs_0x430bc0.cpp | 3 | 3 / 5 | |
| game/new_dialogue.cpp | 4 | 4 / 6 | |
| vsdk/unnamed_eax_0x521e40.cpp | 13 | 13 / 14 | |
| vsdk/registry.cpp | 13 | 13 / 14 | |
| game/unnamed_equip_packets_0x4878a0.cpp | 3 | 4 / 4 | mp_handle_equip_item |
| game/multitrade.cpp | 21 | 21 / 22 | |

(ai.cpp lists 0x472200 twice in the sweep; the counts above are sweep lines.)

## What worked (details in D10500-D10505)

- **Boss state objects (header changes D10503, D10504):** the Murod Oni's and the Evil Joseph's globals made one
  struct each, like the Titus (D8100). Evidence: the binary reloads the offense-point / area-hit counts after the
  array stores, which 8966 does for fields of one object only. Probably applies to other bosses still split into
  globals (worth checking: murod, evil_urath, tentacle beast).
- **Header change D10505:** debug_return_zero / debug_noop_disp_rate take an `unsigned char` display flag (callers
  push a byte-loaded register unwidened).
- memcpy for single-int packet reads into a local; float temporaries (MP_PUT_FLOAT / MP_ADD_FLOAT) for float packet
  fields (they stay in memory); FileReader readers through `pos` directly, memcpy for the float.
- Direct array indexing instead of a `buf` pointer local (netpackets); if / else of two strcpy calls instead of a
  ternary argument.
- vector2 corner points in declaration order a, d, b, c (24 orders tried: one matches).
- Vector temporaries under 8966: `v = vector3(...)` for a temporary plus copy; block scope to share a dead slot;
  `d.dot(back)` instead of a hand-written dot; `d = *pos - owner->pos; d.y = 0`.
- Call evaluated last in an expression to read a global before it (ui_message_box_hit_test).
- file_list_files: the directory buffer is 256 bytes (frame 0x150, as the binary); still differs in slot order.

## Still differing (tried; D10506)

file_list_files, read_string, set_cipher_key, file_add_path / find_files / File::find / seek / calc_crc32 (not
tried at length), eax_init, mp_handle_it_okay, pick_attack_slot / claim_attack_slot (only the first get_center x
operand order), ui_3d_begin (frame now right), mp_read_spell_targets, mp_handle_release_spell (late push ebx),
mp_reset_net_stats, cmd_disp_rate, party_find_next/prev_member, localize_unescape (shrink-wrapped prologue),
evil_joseph_pick_helper_summon (vector temps in the FPU), spells_set_page (scheduling), and the ix, vfx_channel,
character, ui_button, editor_dialogs, new_dialogue, registry units (register allocation / FPU scheduling; looked at
briefly). ix_ray_parallelogram would need `&(a + b)` temporaries passed to the plane constructor, which mingw rejects.

## Behaviour errors / game bugs

None found. All changes keep the behaviour (byte reads in the same order, the same values stored; the struct
layouts are LAYOUT_CHECKed and the old names are macros).

## Files changed (in w6-src)

- vsdk/unnamed_vfile_0x5128b0.cpp
- game/multispell.cpp
- game/unnamed_netpackets_0x441820.cpp
- game/unnamed_equip_packets_0x4878a0.cpp
- game/unnamed_interface_0x411270.cpp
- game/ai.cpp
- game/unnamed_evil_joseph_0x4c3470.cpp
- game/unnamed_murod_oni_0x4c5c20.cpp
- include/engine/ai/boss_ai.h (HEADER: D10503, D10504; users boss_ai.cpp, level_script_templeint2.cpp unchanged and rechecked)
- include/vsdk/os/debug.h (HEADER: D10505)
- vsdk/unnamed_debug_stubs_0x531c10.cpp (not in my list: the two definitions' signatures only, D10505; still all match)
- include/DECISIONS.md (D10500-D10506)

---

## Worker 7

### Worker 7 notes (match round m-1019)

Strict VC6 sweep (cl 12.00.8966, VC6_STRICT=1) of the 19 files: 473 -> 481 matches, 0 lost
(w7-before.txt / w7-after.txt). Decisions D10600-D10602 in w7-src/include/DECISIONS.md.

## Match counts per file (before -> after, of total)

| file | before | after | total |
|---|---|---|---|
| vsdk/gr_opengl.cpp | 35 | 35 | 54 |
| game/new_inventory.cpp | 56 | 58 | 69 |
| vsdk/bmpman.cpp | 46 | 47 | 56 |
| vsdk/unnamed_net_0x517590.cpp | 23 | 23 | 31 |
| game/new_charselect.cpp | 25 | 26 | 32 |
| game/unnamed_mp_objects_0x47f860.cpp | 25 | 25 | 31 |
| vsdk/unnamed_sndstream_0x566560.cpp | 21 | 22 | 26 |
| game/unnamed_ai_groups_0x492b00.cpp | 13 | 13 | 18 |
| game/unnamed_netsession_0x440b40.cpp | 17 | 18 | 21 |
| vsdk/unnamed_ui_slider_0x55e040.cpp | 23 | 24 | 26 |
| vsdk/matrix.cpp | 43 | 43 | 46 |
| game/new_options.cpp | 32 | 32 | 35 |
| vsdk/unnamed_vfx_mesh_0x562a30.cpp | 7 | 7 | 9 |
| game/summoner.cpp | 13 | 13 | 15 (WinMain not found, as before) |
| game/new_gamecreate.cpp | 18 | 19 | 20 |
| vsdk/unnamed_crc32_0x5520d0.cpp | 3 | 3 | 4 |
| vsdk/stringpool.cpp | 15 | 15 | 16 |
| game/unnamed_dialogue_table_0x4feb50.cpp | 20 | 20 | 21 |
| game/new_skill.cpp | 38 | 38 | 39 |

New matches (Confidence raised to exact): acm_convert 0x566ed0, bm_replace_file 0x5031c0,
mp_player_disconnect 0x441660, inventory_page_up 0x4165c0, mp_charselect_wrap_text_input 0x4072e0,
ui_slider_base::get_marker_rect 0x55e420, gamecreate_frame 0x409cd0, inventory_hover_tooltips 0x416290.

## What worked
- Store order of independent statements (acm_convert, bm_replace_file, get_marker_rect). Trying all orders of a
  block of 4 stores found get_marker_rect's in one run (24 compiles).
- Call arguments into locals before point_in_rect (D6205) and an element pointer for the tooltip.
- list_append macro instead of a hand-written append.
- D10600: `x &= (x <= 0) - 1` on the stored variable gives Sum.exe's `and value, mask` select, which D7506 had
  listed as impossible.

## Still differs (main reasons)
- Register choice / scheduling with identical instructions: Crc32::update, StringPool::Add, nw_buffer_pop,
  nw_accept_reliable, mp_handle_ai_disable, mp_handle_status_add (dead `data` slot), mp_session_reset,
  mp_player_alloc (`&e->prev` kept), mp_charselect_page_down, ui_slider::create_scrollbar, gl_convert_* family,
  matrix43_interpolate_quat, options_init (first read propagated from Summoner_options), summoner_cfg_load,
  wav_open (imul operand order), mp_send_obj_update (frame), bm_lock_pcx, bm_prefer_vbm (shrink-wrapped push).
- Clamp after min through a local: inventory_identify_item / destroy_selected_item / equip_item (D10600).
- Cross-jumping of return tails: nw_ras_init, mp_char_create, mp_charselect_update_create_button (hoisted load).
- gr_opengl_bitmap: 50 -> 25 differing instructions (first corner's store schedule only).
- Large functions (parse_table, render_mesh, update_frame, handle_click functions, gl_build_mipmaps,
  nw_WorkReliable, ...) not attempted beyond a first look.
- Coordinator tip (dead argument slots under 8966): retried mp_handle_status_add and snd_stream_fill_silence with
  block-scope locals and parameter reuse; no gain.

## Behaviour errors found
- D10601 inventory_handle_click (0x415a0d-0x415a1d): the open-mode test was inverted. The binary lets a button
  act when `!inventory_open_mode || !(def.flags & 0x10)`; the source had `(def.flags & 0x10)`. Fixed (the
  function still differs by register choice), and the button-table comment is corrected (0x10 = not usable in open
  mode: screen/member arrows and paper-doll hot spots). No game bug (the decomp was wrong, not the game).

## Files changed (in w7-src)
- game/new_charselect.cpp, game/new_gamecreate.cpp, game/new_inventory.cpp,
  game/unnamed_netsession_0x440b40.cpp, vsdk/bmpman.cpp, vsdk/gr_opengl.cpp, vsdk/unnamed_net_0x517590.cpp,
  vsdk/unnamed_sndstream_0x566560.cpp, vsdk/unnamed_ui_slider_0x55e040.cpp, include/DECISIONS.md.
- No header changed. mingw -fsyntax-only: 0 errors in each changed .cpp.

---

## Worker 8

### Worker 8 notes (m-1019)

Strict VC6 sweep (8966) of the 18 files, before / after: **472 -> 477 match** (91 differ), 0 lost
(`vc6-sweep --check w8-sweep-before.txt`). Sweep lists: `w8-sweep-before.txt`, `w8-sweep.txt`.
mingw `-fsyntax-only -std=c++98`: 0 errors in the three changed files. No header changed.

## Per file (match before -> after, of total)

| File | Before | After |
|---|---|---|
| game/living_entity.cpp | 107 | 111 / 132 |
| vsdk/unnamed_gadget_0x52bfe0.cpp | 45 | 46 / 48 |
| vsdk/unnamed_listbox_0x52ca60.cpp | 58 | 58 / 69 |
| game/unnamed_ai_move_0x4b1410.cpp | 9 | 9 / 19 |
| game/unnamed_dialogue_box_0x4d6660.cpp | 24 | 24 / 32 |
| vsdk/unnamed_lights_0x51e070.cpp | 35 | 35 / 41 |
| game/s3d.cpp | 28 | 28 / 34 |
| vsdk/unnamed_gr_poly_0x5450c0.cpp | 19 | 19 / 24 |
| vsdk/unnamed_inputbox_0x5521a0.cpp | 9 | 9 / 13 |
| game/unnamed_lobj_raycast_0x4ae890.cpp | 3 | 3 / 7 |
| game/unnamed_luminar_0x4c8d10.cpp | 20 | 20 / 23 |
| game/gameplay.cpp | 36 | 36 / 39 |
| game/player.cpp | 21 | 21 / 23 |
| game/new_int_multi_update.cpp | 3 | 3 / 5 |
| vsdk/unnamed_build_stamp_0x52f450.cpp | 2 | 2 / 3 |
| vsdk/ui_container.cpp | 9 | 9 / 10 |
| game/unnamed_cutscene_script_0x4ed050.cpp | 5 | 5 / 6 |
| game/objects.cpp | 39 | 39 / 40 |

New matches (Confidence raised to exact): 0x52c390 ui_gadget::process, 0x45a120 update_locomotion_anim,
0x460190 spell_apply_hit_effects, 0x45bf70 update_regen_and_status, 0x4569c0 find_overlap.

## Behaviour error found and fixed (D10700)

- **ui_gadget::process (0x52c390)**: the binary loads key once (0x52c3a6) and the loop jumps back to 0x52c3aa,
  pushing EAX, so each child receives the previous child's result as its key (a consumed key passes on as 0);
  the result is the last child's. The source (D8200) passed the original key to every child. Now
  `key = g->process(key)`; byte match. Other process loops (ui_container, window) are other workers' files and
  were not checked for the same pattern.

## What worked

- /Od frames (D10701): the D6101 bucket rule holds under 8966; locals renamed for non-decreasing buckets from
  the top of the frame and declared bottom first. A helper pairs the k-th `[ebp+X]` reference of our listing
  with the k-th in Sum.exe to read the original slot of every local. Also covers profile_label locals and
  block arrays (keep declaration order for the initialisers, use strictly increasing buckets).
- `?:` inside an inline function puts its result temporary below `this`; an if / two-returns form puts it above,
  where the binary has it (short_min, D10703).
- Named locals with ASSIGN_MIN instead of `(short)min(...)` for the clamped AP values (scale_stats_to_level).
- Float accumulation `((float)r + g + b)`, `-(float)x` (fild / fchs), `ap = ap + regen` (int add), the
  self-copy `gain = gain` the binary contains (D10702).
- find_overlap: `if (hit != this) { ...rest... }` instead of `continue` (the jump goes to the loop end).

## Closer but still differing (kept)

- scale_stats_to_level: all slots right; only `(float)level / (float)info->level` differs (binary fild / fdivp,
  8966 fidiv in every form tried).
- load_state: all named slots right; LE_GET_BOOL `test / setne al` in the binary vs neg / sbb / neg (ubyte
  `suspended`; would need it declared bool in level_scripts.h: not done).
- item_obj_spawn: named slots right; the inline vector3 / matrix assignment temporaries differ (8 bytes).
- get_resistance: same size now; one register (edx vs eax) at the last call's argument.

## Still differs (tried, not reproduced)

- Register swaps: send_event, set_parent, last_visible_line, listbox create (param load order), get_resistance.
- Byte narrowing: Player::is_leader (`shr al`), game_render_frame cursor_visible (`not al / shr al`).
- Scheduling: s3d_read_door (`mov ecx, edi` placement), light_create_spot (fstp after push), listbox
  get_vscroll_fraction / scroll_page_up.
- Cross-jumping of identical tails: ui_inputbox::process (the binary keeps duplicates); cast_spell_update's
  fail block placement.
- /Od extra fst for inline len() (get_speed, anim_speed_at_blend, stopping_distance, distance_to_reach_*).
- version_read_nfo_file: frame 8 bytes larger in the binary (line 4 bytes lower, last 4 higher): unknown.

## Files changed (in w8-src)

- game/living_entity.cpp, game/objects.cpp, vsdk/unnamed_gadget_0x52bfe0.cpp, include/DECISIONS.md
  (D10700-D10703).

---

## Worker 9

### Worker 9 notes (m-1019)

Tree: `m-1019/w9-src`. Compiler: VC6 SP5 (cl 12.00.8966), strict sweep. Before: `w9-before.txt`, after: `w9-after.txt`.
**362 -> 369 of 458 functions in my 18 files; none lost** (vc6-sweep --check, address by address).
Decisions D10800-D10808 appended to `w9-src/include/DECISIONS.md`.

## Per file (matches before -> after / total)

| file | before | after | won |
|---|---|---|---|
| game/unnamed_render_scene_0x481680.cpp | 51 | 55 / 77 | 0x483260 add_character, 0x483850 add_lobjs, 0x485380 draw_float_text, 0x4865a0 level_load |
| game/unnamed_frame_0x4323e0.cpp | 18 | 19 / 21 | 0x4328b0 FrameEnd |
| game/unnamed_pfgeom_0x4a4ee0.cpp | 16 | 17 / 24 | 0x4a55f0 pf_straight_walk_clear |
| vsdk/targautils.cpp | 16 | 17 / 21 | 0x555330 tga_rle_decode |
| game/spells.cpp | 28 | 28 / 39 | |
| vsdk/unnamed_vfx_emitter_0x55ff00.cpp | 5 | 5 / 14 | |
| vsdk/unnamed_gr_3d_0x50d9b0.cpp | 33 | 33 / 39 | |
| game/unnamed_camera_0x4778b0.cpp | 41 | 41 / 47 | (update_eye_position closer) |
| vsdk/unnamed_key_0x5093a0.cpp | 22 | 22 / 26 | |
| game/new_multichat.cpp | 8 | 8 / 12 | |
| vsdk/unnamed_linefile_0x558340.cpp | 11 | 11 / 14 | |
| game/level_script_worldmap.cpp | 20 | 20 / 23 | |
| game/new_playmenu.cpp | 14 | 14 / 16 | |
| game/new_journal.cpp | 25 | 25 / 27 | |
| vsdk/unnamed_bezier_0x558a30.cpp | 14 | 14 / 15 | |
| vsdk/unnamed_anim_track_0x550660.cpp | 12 | 12 / 13 | |
| game/unnamed_cutscene_0x434900.cpp | 25 | 25 / 26 | |
| game/unnamed_alt_textures_0x4aac10.cpp | 3 | 3 / 4 | |

## What worked

- Frame layout by scope (D10802, D10804): 8966 lets sibling blocks share stack slots; function-scope locals never
  share. Reading the binary's slot offsets (push depth subtracted) and moving locals into blocks matched FrameEnd and
  draw_float_text. Declaration order and local names did not change the /O2 layout.
- Buffer sizes from the frame (D10801, D10803, D10804): level_load name 0x28, draw_float_text buf 8,
  level_vis_parse 0x44 / 0x44 / 0x4c / 0x100.
- `jb` with a biased pointer = an unsigned index bound (sizeof count) (D10803); `jae` = unsigned compare (D10801).
- Loop bounds and table pointers read once only when the source copies them into locals (D10806).
- `c &= ~0x80` gives the byte `and bl, 0x7f` (D10800); operand order of a two-term sum (D10805).
- A scratch helper (`w9v/dist.mjs`, `td.mjs`) compares our listing with the binary operand by operand (stack
  offsets resolved from the listing's equates), which shows slot and operand differences vc6-side does not mark.

## Still differing (tried; D10808)

Register choice and schedules: key_flush (8966 makes rep stosd of the zeroing loops), key_mark (the binary re-reads
key_queue_tail after the queue store), spell_fire_missile, camera::set_mode, gr_get_frustum_planes, pcx_read_bitmap,
level_vis_parse / level_vis_reset (zero in a register), add_effect_entry (store order), get_fade (prologue).
FPU factor order that 8966 sorts in every source form: update_eye_position (`zoom * elevation`),
seg2d_point_near_middle, add_containers. Inline expansion: push_attachment (two row copies of matrix::operator=
expanded, the third called). Layout: playmenu_open_save (save flag shares the button slot in the binary),
m2v_skip_extension_and_user_data (tail merge). The coordinator's tip (dead argument slots, vector temporaries) came
late; tga_rle_encode_row (dup in src's slot) and spell_probe_ground_point / gr_setup_frustum (vector temporaries,
a negated half width kept separately) are candidates for it.

## Behaviour errors / game bugs

- No behaviour error found. Buffer sizes corrected without behaviour change: level_vis_parse's console line
  (0x80 -> 0x100) and name buffers, level_load's name (0x20 -> 0x28).
- Observation (no effect): levelrender_draw_float_text formats into an 8-byte buffer (0x485380, buffer at frame +0x1c,
  the position at +0x24). A longer text would spill into the position and vertex, which are written only after
  sprintf, so nothing visible changes; not a game bug.
- Checked, not an error: summon_create's ring chain (0x47e9c5: 0xc0 / 0xcd use "dark_ring_xp" as the source has).

## Header change

- include/engine/rendering/unnamed_render_scene_0x481680.h: levelrender_add_effect_entry's `cull` is int (D10807).
  Callers only in unnamed_render_scene_0x481680.cpp. mingw over all 259 .cpp: only unnamed_decals_0x4bbe80.cpp
  errors (9), the same as the base tree.

## Files changed

- game/unnamed_render_scene_0x481680.cpp
- game/unnamed_frame_0x4323e0.cpp
- game/unnamed_pfgeom_0x4a4ee0.cpp
- game/unnamed_camera_0x4778b0.cpp (update_eye_position square order; still differs)
- vsdk/targautils.cpp (tga_rle_decode; m2v_skip_extension_and_user_data branch layout, still differs)
- include/engine/rendering/unnamed_render_scene_0x481680.h
- include/DECISIONS.md

---

## Worker 10

### Worker 10 notes (match pass 2026-10-19)

Strict VC6 sweep (cl 8966, `VC6_STRICT=1`) of the 20 units, run before and after:
20 units: 377 -> 383 matches, 97 -> 91 differ, 0 lost (address-by-address diff of w10-sweep-before.txt against
w10-sweep-after.txt shows only the six gains).

| Unit | before | after | of |
|---|---|---|---|
| game/unnamed_avoidance_0x4a6710.cpp | 45 | 45 | 67 |
| vsdk/unnamed_quaternion_0x5225b0.cpp | 21 | 21 | 32 |
| game/unnamed_fx_0x489c40.cpp | 47 | **48** | 57 |
| vsdk/unnamed_gr_line_0x53e520.cpp | 11 | **13** | 19 |
| game/boss_ai.cpp | 51 | 51 | 58 |
| game/unnamed_decals_0x4bbe80.cpp | 9 | 9 | 15 |
| vsdk/unnamed_http_0x55a8e0.cpp | 10 | 10 | 15 |
| vsdk/unnamed_poly_split_0x55f110.cpp | 1 | 1 | 5 |
| game/unnamed_worldmap_automap_0x42f1e0.cpp | 31 | 31 | 35 |
| vsdk/unnamed_os_0x530b90.cpp | 20 | 20 | 23 |
| vsdk/unnamed_checkbox_0x553ba0.cpp | 9 | 9 | 12 |
| game/new_gamelist.cpp | 26 | **28** | 29 |
| game/conversation.cpp | 24 | 24 | 27 |
| game/new_pickup.cpp | 18 | 18 | 20 |
| vsdk/unnamed_ui_static_0x527e20.cpp | 9 | 9 | 10 |
| vsdk/unnamed_ui_manager_0x52f570.cpp | 11 | 11 | 12 |
| vsdk/cmdline.cpp | 15 | 15 | 16 |
| game/unnamed_utrack_0x48d7c0.cpp | 3 | **4** | 4 |
| game/level_script_ionaext.cpp | 10 | 10 | 11 |
| game/level_script_ikaemosu.cpp | 6 | 6 | 7 |

## New matches (Confidence raised to exact)

- 0x48d870 pxo_login_request: cases 1 and 2 spelt out so 8966 builds the jump table over 1..5; the idle test
  positive with `return -3` after it.
- 0x40b5c0 mp_server_list_draw: the level and players colour tests positive (text colour first).
- 0x40acd0 gamelist_handle_click: rectangles through locals, row offset `row * font_h`, the drag thumb stored
  before the drag start (this one was a "target" mismatch in strict mode).
- 0x48c220 fx_create_missile: nested handle tests with one shared `return -1`; `dir = t->pos - c->pos`.
- 0x53e9d0 gr_clip_and_project_poly, 0x53e8d0 gr_poly_project_to_screen: `codes_or` declared before `codes_and`;
  the latter also reads the vertex through a local.

## What worked (DECISIONS.md D10900)

Branch polarity (failure block last = positive test), case labels that go to the default spelt out to get the
jump table, nested tests with one shared failure return, vector3 / vector2 operators for temporaries, call
arguments through locals, the declaration order of initialised locals, strength-reduced `i * k` instead of a
second loop variable.

## Closer but still differing (kept, behaviour identical)

- avoid_prim_from_segment, avoid_poly_nearest_edge_hit: vector2 operators (D10901). prim_from_segment differs only
  at axis_b, which the binary constructs in place; an explicit constructor call matches with VC6 but GCC rejects it.
- decals_collect_for_lobj: unsigned count test, `(bool)` bit read, single return (D10902); EBX / EDI swap left.
- http_get::send_get: nested form gives the block order up to 0x55ad9e; 8966 merges the loop-exit error (D10903).
- boss_move_in_range_dispatch: `float r` local fixes the Luminar case loads; needs the D7603 (3) header change.
- cquat::log: interleaved declarations reach the last conversions; one fxch left.
- cmd_avoidance: the type line as if / else of two calls; 8966 threads the help flag past its test.

## Still differing (no source form found)

page_up max-AND (gamelist, pickup), avoid spline getters' block order, ikaemostopint_init's constant in EBX,
ionaext_frame's sete, the conversation / pickup / boss_threat_add register and scheduling differences,
quaternion FPU / integer scheduling (to_matrix, multiply, squad, exp), decals_reset store order and decal_place's
specialised list removal, gr_line_3d and checkbox create's argument-slot reuse, ui_static::draw / http's merged
tails, the avoidance frame-size differences (merge / clip / seglist functions), fx_frame and the large fx / poly_split
/ worldmap / os functions (register choice, FPU stack). Details and tried shapes in D10904.

## Behaviour errors / game bugs

None found. decals_collect_for_lobj now tests the count unsigned as the binary does (a negative count is not
reachable). Evidence for a declaration (not applied, D10904 (1)): conversation::clear_speaker calls an empty
thiscall member of the speaker (ECX = speaker).

## Checks

- mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in the changed files except
  game/unnamed_decals_0x4bbe80.cpp, which has 9 "taking the address of a temporary" errors that the repo's copy
  has too (lines 93-95 etc., unchanged by this pass).
- No header changed.

## Files changed (in w10-src)

game/unnamed_utrack_0x48d7c0.cpp, game/new_gamelist.cpp, game/unnamed_fx_0x489c40.cpp,
vsdk/unnamed_gr_line_0x53e520.cpp, game/unnamed_avoidance_0x4a6710.cpp, game/unnamed_decals_0x4bbe80.cpp,
vsdk/unnamed_http_0x55a8e0.cpp, vsdk/unnamed_quaternion_0x5225b0.cpp, game/boss_ai.cpp,
include/DECISIONS.md (D10900-D10904).
