# Byte-match round 2026-10-20 (fourth): the fifteen workers' notes

Merged into src/decomp (5,404 -> 5,445 strict matches; DECISIONS D15500-D16999). One section per worker, as written.

---

## Worker 1

### m-1020d worker 1 notes

Strict sweeps (VC6_STRICT=1) of the nine assigned units in `w1-src`: before 185 match / 35 differ, after
187 match / 33 differ; `--check` against the before list: 0 lost. Lists: `w1-before.txt`, `w1-after.txt`.

## Counts per file (match / functions compared)

| File | Before | After | New matches |
|---|---|---|---|
| game/unnamed_avoidance_0x4a6710.cpp | 50 / 67 | **51** / 67 | 0x4a8aa0 avoid_sort_refs_along_segment |
| vsdk/window.cpp | 36 / 40 | 36 / 40 | |
| game/unnamed_pfgeom_0x4a4ee0.cpp | 20 / 24 | 20 / 24 | |
| game/new_automap.cpp | 29 / 32 | **30** / 32 | 0x4023b0 automap_center_on_player |
| game/unnamed_projectors_0x4c0250.cpp | 9 / 11 | 9 / 11 | |
| game/multispell.cpp | 20 / 22 | 20 / 22 | |
| game/unnamed_level_complete_0x4b0960.cpp | 9 / 10 | 9 / 10 | |
| game/unnamed_cutscene_script_0x4ed050.cpp | 5 / 6 | 5 / 6 | |
| game/item_paging.cpp | 7 / 8 | 7 / 8 | |

Confidence raised to exact on the two new matches only. Decisions D15500-D15504 appended to
`w1-src/include/DECISIONS.md`.

## What worked

- avoid_sort_refs_along_segment (D15500): the corner pointer as if / else of two assignments instead of a ternary
  in the subscript (each branch gets its own movsx; the EBX / EBP roles follow).
- automap_center_on_player (D15501): **an int array `xy[2]` instead of two int locals stops 8966 sinking the two
  conversions into the only branch that uses them.** A general device for "8966 sinks, the binary does not".
- Closer only: avoid_box_box_overlap (D15502, byte-level LCS 38 -> 12 instructions: operator- for d, three k
  products as inline dots, found by a 256-spelling search) and avoid_circle_contains_point (D15503, the squared
  sum gives `fld st(0) / fmulp`).
- Tooling (in `w1-try/`, my folder): `try.mjs` (variants with the listing kept in my own temp folder, so nothing
  is deleted), `bscore.mjs` (byte-level instruction LCS of the listing against Sum.exe, relocations masked: ranks
  register / slot / operand-order variants that the mnemonic side-by-side and the first-difference offset cannot),
  `gen_bb2.mjs` (combinatorial spelling generator).

## Still differs (tried; details in D15502-D15504)

- avoid_box_box_overlap: k_ab's x product order, the k-term order of the first two tests.
- avoid_circle_contains_point: r in p's dead slot (binary: ob's).
- ui_window::handle_arrow_key (left-case distance, every spelling the same), ui_window::init (this in ESI, both
  pushes at entry), cmd_avoidance (help jump threaded), mp_read_spell_targets (req walked in EDI).
- automap_center_on_player's earlier shapes; avoid_spline_point_get / get, item_page_out_all,
  mp_handle_combat_sequence, seg2d_point_near_middle, spline_point_create: not retried beyond prior notes.
- Not attempted (large, frames / EH, many prior attempts): automap handle_click / draw_map, cutscene_script_load,
  pfgeom segment_hits_walls / segment_intersect / grid_cells_along_segment, projector_collect_lobj_faces /
  to_lobj_space, window parse_radio_button / parse_slider, avoidance merge / clip / create / add / seglist /
  search-node functions, avoid_prim_overlap (its branch structure differs: 32 instructions).

## Behaviour errors / bug candidates

None found or changed. All edits keep behaviour (D15502's dots add the same products in the binary's order).

## Changed files (in w1-src)

- game/unnamed_avoidance_0x4a6710.cpp (sort_refs match; box_box_overlap and circle_contains_point closer)
- game/new_automap.cpp (center_on_player match)
- include/DECISIONS.md (D15500-D15504 appended)

No header changed. mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors on both changed .cpp. No file was
written into the repository folder.

---

## Worker 2

### m-1020d worker 2: notes

Strict sweeps: `w2-sweep-before.txt` (my 9 units: 304 match of 338) and `w2-sweep-after.txt` (305 of 338),
taken from a full strict sweep of all 259 units (`w2-sweep-full.txt`: 5,405 match; `--check
src/decomp/vc6-matches.txt`: 5,404 there, 0 lost). The full sweep was run because a header changed (D15603).
mingw `-fsyntax-only` on every .cpp: 0 errors except unnamed_decals' 9, as in the base copy.

## Counts per file (strict)

| File | Before | After | Of |
|---|---|---|---|
| game/new_buysell.cpp | 40 | 40 | 42 |
| game/new_options.cpp | 32 | 32 | 35 |
| game/pathfinding.cpp | 23 | 23 | 36 |
| game/unnamed_tiger_0x499e50.cpp | 43 | 43 | 48 |
| game/unnamed_ghost_rider_0x4b9e70.cpp | 37 | 37 | 38 |
| game/unnamed_netsession_0x440b40.cpp | 19 | 19 | 21 |
| game/unnamed_dialogue_table_0x4feb50.cpp | 20 | 20 | 21 |
| vsdk/unnamed_vfile_0x5128b0.cpp | 79 | 79 | 82 |
| vsdk/unnamed_vif_0x5329f0.cpp | 11 | 12 | 15 |

New match: 0x533620 vif_draw_particles (D15602: an if / else of two gr_bitmap_3d calls). Nothing lost, checked
address by address.

## What worked

- D15602 vif_draw_particles: the if / else of two calls fixed the register rotation of the arguments.
- Closer only (kept, plausible source, same behaviour):
  - D15601 pf_sweep_test_wall: vector2 operator forms; first 34 instructions exact (313 -> 279 differing lines).
  - D15603 vif_emitter_update: vfx_inst21c::tag is `ushort` (header; its only load zero-extends) and
    `force = curve; force *= reported_frametime;` (354 -> 309).
  - D15604 tiger_combat_update: nested ifs with a single go_idle at the end give the binary's block layout
    (239 -> 220).

## Behaviour errors fixed (listed separately)

- D15600 pf_check_direct_walk 0x49e620: the binary's clip length uses d.y = to->y, not to->y - from->y
  (0x49e663 / 0x49e667 copy end.y into d.y with no fsub; x and z subtract at 0x49e657 / 0x49e66f; from->y is
  loaded only in the clip, 0x49e6c1). The decomp computed a different length whenever from->y != 0. Now per
  component. A decomp-fidelity fix; whether the original meant it is unknowable (a bug candidate in the game:
  the walk check clips by a length that includes the destination's absolute height). Still differs (the copy of
  *to into end).

## Still differs (tried this round; do not repeat)

- mp_session_reset: 6 loop / append / pointer forms, all compile the same (register pair swap).
- File::get_async_status: slot pointer local (in and out of the if), owner local, `(file_slots + slot)`,
  early return, int copy of slot: all fold the slot address; the binary computes it unfolded.
- vif_record_get_bounds: fild temporary in rec's argument slot, the binary uses radius's (cast forms, inv
  local, radius / centre pointer locals, store order, a path pointer).
- vif_compact_particles: the binary's record pointer is biased to +0x70, ours +0xa0; new_base in EBP in the
  binary. Index loop, top = new_base, statement orders, a count local: no change (top = new_base 219 -> 211, not
  kept).
- vif_emitter_update: 24 declaration orders of force / interval / remaining / spawn, block-scoped spawn and
  counter: no frame change.
- pf_check_direct_walk: per-component d, `vector3 end = *to`, `end(*to)`, a from copy, `f.y = 0`, statement
  orders; `#pragma inline_depth` before the function breaks the marker (not found).
- pf_sweep_test_wall: n = (dir.y, -dir.x) through the x87 stack (ctor, both orders, `0.0f - dir.x`); the frame
  (ours 0x58, binary 0x48) and kind in EBX.
- pf_read_region: `k = 0` at the loop top (in the arguments too), while loop: no change (the binary's zero
  register EBP doubles as k's offset, pushed late).
- options_drag_sliders: float_min / float_max inline forms compile like the ifs (no width copy).
- options_init: memcpy for the struct copy, `1 & game_flags`: no change.
- tiger_combat_update: fire as bool / int, top / block declaration, operand order, an assignment from the
  condition: 8966 threads the constant and drops the variable (binary: BL).
- tiger_ai_frame: one go_idle via goto (three forms) compiles the same; only the zero register EBP differs.
- Looked at, not tried (register choice or known): buysell_count_party_items, buysell_handle_click (-1 in
  EBP), options_load_bitmaps, mp_player_alloc, file_list_files, set_cipher_key (bit-field), pf_read_edges,
  seg2d_closest_point, pf_find_cell_crossing, tiger_pick_target, the anim_remap_in pair, parse_table,
  pathfinding_load_graph and the other large pathfinding functions (frame slots throughout).

## Devices

None kept. D15603's header type follows the binary's only load.

## Changed files (in w2-src)

- game/pathfinding.cpp (D15600, D15601)
- vsdk/unnamed_vif_0x5329f0.cpp (D15602, D15603; 0x533620 Confidence exact)
- include/vsdk/geom/vfx.h (D15603: vfx_inst21c::tag ushort)
- game/unnamed_tiger_0x499e50.cpp (D15604)
- include/DECISIONS.md (D15600-D15604 appended)

---

## Worker 3

### m-1020d worker 3

Strict sweep (VC6_STRICT=1) of the nine files: 291 -> 294 matches, none lost (`--check` against
w3-sweep-before.txt passes). Lists: w3-sweep-before.txt, w3-sweep.txt.

| File | Before | After | Of | Won |
|---|---|---|---|---|
| game/combat.cpp | 52 | 52 | 63 | |
| game/level_scripts_common.cpp | 72 | **73** | 78 | 0x4caa00 level_check_triggers |
| game/ai.cpp | 83 | **84** | 88 | 0x472200 ai_state::call_for_help |
| vsdk/unnamed_inputbox_0x5521a0.cpp | 10 | 10 | 13 | |
| game/summoner_cd.cpp | 3 | 3 | 6 | (the no-CD patch) |
| game/unnamed_netpackets_0x441820.cpp | 10 | 10 | 12 | |
| game/new_charselect.cpp | 30 | **31** | 32 | 0x407410 mp_charselect_page_down |
| game/unnamed_gamelist_0x4705b0.cpp | 13 | 13 | 14 | (mp_handle_actv_gm closer) |
| game/unnamed_evil_joseph_0x4c3470.cpp | 18 | 18 | 19 | |

## What worked (DECISIONS.md D15700-D15703)

- **A call result in a local before it is compared with a member read** (D15701, D15702): `float d =
  distance(...); if (d < owner->info->detection_range)` makes 8966 read the member again after the call, as the
  binary. Won call_for_help and, with the exit stores as name / script / index, level_check_triggers. Worth trying
  on other functions whose difference is an extra saved register or a value loaded before a call.
- **A page-size local with `rows - page` written at each use** (D15700) instead of a `last` local: the binary's
  register order in mp_charselect_page_down. The other page-down functions that differ by register order only
  (other workers' files) may take the same form.
- Closer, kept: mp_handle_actv_gm (ping clamp as `e->ping > 0 ? e->ping : 0`, the binary's store / and / store;
  536 of 537 bytes); mp_charselect_handle_click (`info_open == (((unsigned)flags >> 3) & 1)`: shr and the compare
  order; the field stays int).

## Still differs (tried, see D15703)

melee_damage (frame and registers), ai_choose_special (push test placed after the return), mp_reset_net_stats
(second zero register), cast_effect_track_add, party_set_level (only the SIB order of the skill copy),
ai_state::load (register rotation), level_trigger_point_left_of_line (8 frame bytes), fire_weapon_missile (the
row copies' out-of-line copy constructor; inline_depth 1-3 do nothing), get_protection, pick_attack_slot /
claim_attack_slot and evil_joseph_pick_helper_summon (x87 operand commutation of vector sums: in the last,
an explicit vector3 with y and z swapped swaps only y). Not attempted beyond a look: resolve_exchange,
trigger_special / reflex_ability, chain_check_window (cross-jumping direction), level_parse_table /
level_script_parse_table (EBX / EBP), level_parse_shopkeeper, combat_position_modifier, blood_splot_place,
combat_get_weapon_tag_position, inputbox process / set_valid_chars / set_invalid_chars, cmd_disp_rate,
summoner_cd (patched bytes).

## Behaviour errors / bug candidates

None found. Every change has the same behaviour (independent store order, same evaluation order, max(x, 0) in
another form, an unsigned shift of bit 3).

## Devices

None. The forms are plausible source (locals, statement order, ternary, a cast).

## Changed files (in w3-src)

- game/ai.cpp (call_for_help)
- game/level_scripts_common.cpp (level_check_triggers)
- game/new_charselect.cpp (mp_charselect_page_down; mp_charselect_handle_click closer)
- game/unnamed_gamelist_0x4705b0.cpp (mp_handle_actv_gm closer)
- include/DECISIONS.md (D15700-D15703 appended)

No header declarations changed. mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in the four changed
units. Note: during the work I once wrote a stray scratch file `x` into the repository root by a wrong argument;
I deleted that one file right away (it was mine; `git status` is back to its starting state).

---

## Worker 4

### m-1020d worker 4 notes

Strict sweeps: w4-sweep-before.txt (149 match, 34 differ) and w4-sweep-after.txt (150 match, 33 differ).
No match lost (checked address by address against the before list).

| File | Before | After | Of |
|---|---|---|---|
| vsdk/unnamed_quaternion_0x5225b0.cpp | 21 | 21 | 32 |
| game/gametarget.cpp | 16 | 16 | 22 |
| game/levelrender.cpp | 7 | 7 | 12 |
| vsdk/unnamed_d3d_tcache_0x54bb30.cpp | 11 | 11 | 14 |
| game/unnamed_ai_groups_0x492b00.cpp | 15 | 15 | 18 |
| game/unnamed_luminar_0x4c8d10.cpp | 21 | 21 | 23 |
| game/new_itemtrade.cpp | 34 | **35** | 36 |
| game/unnamed_experience_0x489930.cpp | 7 | 7 | 8 |
| game/unnamed_evil_urath_0x4c4170.cpp | 17 | 17 | 18 |

## What worked

- 0x419710 itemtrade_handle_click: byte match (D15800). Block-scoped `d` / `b` button pointers (d first),
  `(bool)((d->flags & 8) >> 3)` for the binary's `shr`, a clamped `limit` local for the gold cap, every
  point_in_rect rectangle read into locals before the call, Esc as a one-case switch. Confidence -> exact.
- VC6's inline budget can be steered from the source (D15802, D15803): the set of vector members the binary calls
  out of line (copy 0x410cd0, (x, y, z) 0x43b580, += 0x48e440) changes with the operator forms used anywhere in
  the function. Matching the binary's call sequence fixed the frames and most of the code of:
  - luminar_frame_hook 0x4c9000: 268 -> 23 differing lines (left: ray.start's loads via EDI instead of ESI).
  - levelrender_wave_lobj 0x48de60: 242 -> 51 differing lines (scheduling only; one named edge is a device).
- cquat::multiply 0x522a40: the cross product as a named local, 88 -> 31 lines (D15801).

Method tip: compare the sequence of `call` targets of our listing with the binary's (`tools/asm.mjs fn` piped to
a call filter); when they agree the frame usually agrees too. My scratch runner (w4-var.ps1 -calls) prints it.

## Still differs (tried, D15801 / D15804)

quaternion (conversion scheduling in cquat::multiply / cquat_squad, to_matrix / _b factor order, exp / log FPU),
xp_level_up_gains (EDI load order), character_can_talk_to (d.z kept on the FPU), pick_test_character (dz2 + dx2
operand order), ai_group_think_style1 / style2 (g in EDI), world_pick_under_mouse (8966 always expands
plane::set_normal_point; the binary calls it), itemtrade_count_party_rows (`inc [mem]`), the large draw lists,
d3d_tcache's three, evil_urath select_action, luminar_boss_load, object_try_unlock_or_open, world_mouse_* (not
attempted at length).

## Behaviour errors / bug candidates

None found. All changes are same-behaviour reshapings.

## Devices

- levelrender_wave_lobj: the named `e2` edge only moves the inline budget (D15803).

## Changed files (in w4-src)

- game/new_itemtrade.cpp (itemtrade_handle_click, exact)
- game/unnamed_luminar_0x4c8d10.cpp (luminar_frame_hook, closer; comment)
- game/levelrender.cpp (levelrender_wave_lobj, closer; comment)
- vsdk/unnamed_quaternion_0x5225b0.cpp (cquat::multiply, closer; comment)
- include/DECISIONS.md (D15800-D15804)

No header changes. mingw -fsyntax-only: 0 errors in the four changed .cpp files.
Kept compile folders are in m-1020d\w4-tmp (TEMP pointed there); nothing deleted.

---

## Worker 5

### m-1020d worker 5 notes

Strict sweep (VC6 SP5 8966, VC6_STRICT=1): 296 -> 299 of 331 compared in my 11 files, none lost
(`--check w5-sweep-before.txt`: 0 lost). Lists: w5-sweep-before.txt, w5-sweep.txt.

| file | before | after | of |
|---|---|---|---|
| game/pathfinding_visibility.cpp | 4 | 4 | 11 |
| vsdk/unnamed_ix_0x53ac20.cpp | 29 | 30 | 35 |
| vsdk/effect_mem.cpp | 34 | 34 | 39 |
| vsdk/unnamed_vector_0x523f30.cpp | 47 | 47 | 51 |
| game/new_pxochat.cpp | 53 | 54 | 57 |
| vsdk/unnamed_vfx_mesh_0x562a30.cpp | 7 | 7 | 9 |
| game/unnamed_pyrul_0x4b87c0.cpp | 21 | 21 | 23 |
| game/level_script_worldmap.cpp | 21 | 22 | 23 |
| game/unnamed_machival_0x4c0c40.cpp | 25 | 25 | 26 |
| game/unnamed_cutscene_0x434900.cpp | 25 | 25 | 26 |
| game/items.cpp | 30 | 30 | 31 |

## New matches
- 0x4244f0 pxochat_drag_scrollbar (D15900): mp_charselect_drag_scrollbar's form (global list index each use,
  thumb first in the sum, count read after the clamp).
- 0x4fce40 ls_worldmap1_pick_encounter_stage15 (D15901): grassland roll block-scoped (frame order).
- 0x53af40 ix_ray_parallelogram (D15902): `&(a + b)` temporaries under _MSC_VER, one float t, named edge point,
  a parenthesised division (DEVICE: scheduling only).

## Closer, not matching (kept; same behaviour)
- 0x4b8da0 pyrul_choose_attack (D15903): frame now 0x68 (block-scoped weights2); the pick blocks' order differs.
- 0x53b130 ix_ray_quad (D15904): retries use tri[0..2]; 685/685 bytes, only the final mean's scheduling differs.

## Still differs (D15905 for what was tried)
Register / scheduling only: random_unit_vector x3 (fmulp), vector3_angle_and_axis, rfx_preload, pxochat_page_down,
pxochat_handle_click, pxochat_parse_chat_banners_cfg, pathvis EH functions, ix_sphere_polygon_edge,
ix_box_oriented_box_oriented, effects_load_bases. Layout: pathvis_bsp_contains, effects_parse_arr_line,
ls_worldmap1_pick_encounter_stage9, pathvis_add_edge_node. Large / not attempted: render_mesh, update_frame,
items_parse_table, cutscene_parse_table, vfx_instance_create, vfx_load, pyrul_fire_attack (D12503),
machival_ai_select_action (target kept in a stack slot, no EBP), seg2d_intersect (D15306), ix_point_in_polygon.

## Behaviour errors / bug candidates
None found; every change keeps behaviour.

## Changed files (in w5-src)
- game/new_pxochat.cpp, game/level_script_worldmap.cpp, game/unnamed_pyrul_0x4b87c0.cpp,
  vsdk/unnamed_ix_0x53ac20.cpp, include/DECISIONS.md (D15900-D15905). No header changes.
mingw -fsyntax-only: 0 errors in each changed .cpp.

---

## Worker 6

### m-1020d worker 6 notes (D16000-D16004)

Strict sweeps (VC6_STRICT=1) of the 11 units: before 301 match / 35 differ (w6-sweep-before.txt), after 304 / 32
(w6-sweep.txt). `--check` against the before list: 0 lost. mingw `-fsyntax-only`: 0 errors in every changed file.

| File | Before | After | Of | New matches |
|---|---|---|---|---|
| game/spells.cpp | 32 | 32 | 39 | |
| vsdk/gr_opengl.cpp | 48 | 49 | 54 | 0x54e840 gr_opengl_create_texture |
| vsdk/gr.cpp | 56 | 56 | 61 | (gr_bitmap_float closer) |
| vsdk/unnamed_sndstream_0x566560.cpp | 22 | 22 | 26 | |
| game/s3d.cpp | 30 | 31 | 34 | 0x470300 s3d_read_color_record (s3d_read_mesh_ref closer) |
| vsdk/unnamed_vfx_channel_0x55b440.cpp | 17 | 17 | 19 | |
| game/unnamed_worldmap_automap_0x42f1e0.cpp | 33 | 34 | 35 | 0x42f870 worldmap_marker_clamp_to_edge |
| game/conversation.cpp | 25 | 25 | 27 | |
| game/unnamed_party_inventory_0x46b460.cpp | 29 | 29 | 30 | |
| game/unnamed_alt_textures_0x4aac10.cpp | 3 | 3 | 4 | |
| game/level_script_ikaemosu.cpp | 6 | 6 | 7 | |

## What worked

- **Parentheses are not a no-op for 8966 on float expressions** (D16000, D16001, D16003). A parenthesised float
  operand or result changes how the value is loaded, stored and associated:
  - s3d_read_color_record: `#define S3D_CLAMP(x, hi) (float_min(float_max((x), 0.0f), (hi)))` (D14602's in-memory
    clamp as a macro). The argument parentheses give `fld; fst` into the argument slot instead of integer copies,
    the outer ones keep the bound on the FPU instead of an integer constant store in a duplicated epilogue.
  - worldmap_marker_clamp_to_edge: `offset[resolution_index] * ((float)cos(loc->angle))` loads the index before
    the fcos (D12107's open point).
  - gr_bitmap_float: `(hs - wc) + x` etc. stop 8966 reassociating `-ws - hc + y` into `y - (ws + hc)`.
  Worth trying wherever the binary's FPU order or load form differs in a float expression: a macro with
  parenthesised arguments is plausible original source.
- gr_opengl_create_texture (D16002): `/ ((float)tex_w)` plus the square size through `int m = max(...)`, the two
  only match together.
- s3d_read_mesh_ref: `(ubyte)hi` for the 8-bit field gives the binary's dword load and mask.

## Tools written (scratch, in m-1020d)

- w6-od.mjs: our object's function next to Sum.exe's with relocated globals resolved to Sum.exe addresses through
  sum.symbols (literals are wildcards) and jump targets relative, LCS-aligned: a "differing rows" count that does
  not drown in relocation noise (vc6-side marks only mnemonics).
- w6-try.mjs: variants with that row count (vc6-try only says match / differs).
- w6-gen-paren.js + w6-psweep.sh: wrap every call / cast / operand of a function in parentheses one at a time.
- w6-climb.mjs: statement-order hill climb over a block (moves within a window), stops at a match.
  Worth promoting to tools/ after a review (they keep their compile folders in w6-tmp; nothing is deleted).

## Still differs (tried, D16004)

spells_parse_table (EBX / EDI and the effect chain's `&tf` register), spell_prepare_cast / cast_execute /
launch_effects / summon_create (frames, cross-jumping; no flag stops summon_create's tail merging),
spell_target_value, spell_roll_amount (trip count), gl_convert_1555_to_rgba8888, gl_build_mipmaps, set_mode (EBX /
EBP), gr_opengl_bitmap (first corner schedule; parenthesised casts 36 -> 30, not kept), read_backbuffer, gr_init
(store schedule; best climb 92 lines, not kept), gr_screenshot, gr_line, gr_tmapper_multipass, the snd_stream_fill_*
argument-slot frames, wav_open's imul order, s3d_read_meshes, s3d_read_door (`mov ecx, edi`), fx_rec4c::update
(frame packing), vfx_channel::eval (the binary re-reads cursor), worldmap_frame (`fstp st(3)`), clear_speaker
(needs a header member, D10904), restore_state (argument load order), character_equip_item (block order),
alt_textures_parse_table (prologue shrink-wrap), ls_ikaemostopint_init (constant in EBX).

## Behaviour errors / bug candidates

None found. No behaviour was changed; all the changes give the same results (the clamp form differs from the
old ternary only for a NaN input).

## Changed files (in w6-src)

- game/s3d.cpp (0x470300 exact; 0x46ffe0 closer)
- game/unnamed_worldmap_automap_0x42f1e0.cpp (0x42f870 exact)
- vsdk/gr_opengl.cpp (0x54e840 exact)
- vsdk/gr.cpp (0x507100 closer)
- include/DECISIONS.md (D16000-D16004 appended)

No header changed.

---

## Worker 7

### m-1020d worker 7 (D16100-D16105)

Strict sweep of the 10 units: 285 -> 290 match, 0 lost (w7-before.txt vs w7-after.txt: only the five new matches
changed). mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in both changed .cpp. No header change
(DECISIONS.md only). No behaviour change.

| File | Before | After | Of |
|---|---|---|---|
| game/unnamed_script_editor_0x4de0b0.cpp | 102 | 106 | 120 |
| vsdk/unnamed_gr_line_0x53e520.cpp | 17 | 18 | 19 |
| vsdk/unnamed_lights_0x51e070.cpp | 37 | 37 | 41 |
| game/unnamed_mp_objects_0x47f860.cpp | 27 | 27 | 31 |
| vsdk/unnamed_ui_button_0x526940.cpp | 22 | 22 | 24 |
| vsdk/unnamed_poly_clip_0x5519c0.cpp | 7 | 7 | 8 |
| vsdk/unnamed_http_0x55a8e0.cpp | 14 | 14 | 15 |
| game/unnamed_snow_0x4946a0.cpp | 10 | 10 | 11 |
| game/objects.cpp | 39 | 39 | 40 |
| game/level_script_ionaext.cpp | 10 | 10 | 11 |

## New matches (Confidence raised to exact)
- 0x4e4910 script_action_format (D16100): type read once into a local, case bodies in the binary's layout
  order (from the jump table), and an explicit `default: break;`, which changes 8966's sprintf tail merges.
- 0x4e5b10 script_editor_add_navpoint (D16101): `ray.dir * hit.t + ray.start` in the cutscene case (operand
  order of the inline vector3 operator+).
- 0x53f720 gr_draw_sphere (D16102): the scaled radii in float locals rs / rc; `angle = 0` before `step =`.
- 0x4e2f30 script_editor_ray_hit_object (D16103): the distance stored into the radius parameter instead of a
  `d` local. This also fixed the partly expanded matrix copy.
- 0x4e75c0 script_editor_focus_selection (D16104): the matrix copy's inline-budget split, by changing the number
  of inline expansions in the function: object read at each use, size row as a vector3 copy, navpoint size via
  `size.set(...)`.

## What worked (reusable)
- `default: break;` in a jump-table switch followed by common code changes the cross-jumping.
- Commuting a vector `+` changes which component is loaded first (exact for IEEE adds).
- Swapping two independent assignments decides which frame slot each variable gets.
- Inline budget (D10808 cases): the binary's out-of-line copy constructor / operator= calls follow the number
  of inline expansions in the whole function. Remove or add inline member uses (local vs. global reread,
  `.set()`, vector3 copies) until ours expands the same ones. This is worth trying on item_obj_spawn,
  world_pick_under_mouse, levelrender_push_attachment and pyrul_fire_attack.
- Reusing a dead parameter for a float result (`radius = distance(..)`) gives the binary's argument-slot stores.

## Still differs (D16105, not kept)
snow_start (`jl` and the size right with an int i loop and a walking row pointer; two instructions in a
different order), select_prev (a `next` local gives the size; one register differs), action_type_allowed,
apply_action_params, ls_ionaext_frame, mp_handle_ai_disable, mp_send_new_lep, light_create_spot,
gr_bitmap_3d_oriented_quad, http_async_gethostbyname (all 120 store orders), door_sound_inputbox_cb.
Not attempted: handle_input, fill_action_lists, script_create, edit_button_cb, write_level_script /
write_level_table / level_items_write_table, level_item_ok, draw_navpoints, ui_button::process / draw_arrow,
polygon_clip_2d, lights_shade_point, mp_handle_new_path / status_add.

## Devices
- `*(vector3 *)script_editor_char_sizes[...]` in focus_selection. The cast is already used in create_ui. The
  binary copies the row as a vector3, so the table is probably `vector3[4]` in the original. This is a
  candidate header type fix and was not done.

## Behaviour errors / bug candidates
None found. All edits keep the behaviour.

## Changed files (in w7-src)
- game/unnamed_script_editor_0x4de0b0.cpp
- vsdk/unnamed_gr_line_0x53e520.cpp
- include/DECISIONS.md (D16100-D16105)

---

## Worker 8

### m-1020d worker 8

Strict sweep of the 12 assigned units: 436 -> 438 match (of 471), none lost (`--check w8-sweep-before.txt`).
Lists: w8-sweep-before.txt, w8-sweep.txt.

| File | Before | After | New matches |
|---|---|---|---|
| vsdk/unnamed_ui_slider_0x55e040.cpp | 24/26 | 25/26 | 0x55e040 ui_draw_bevel |
| vsdk/unnamed_poly_split_0x55f110.cpp | 1/5 | 2/5 | 0x55fb00 gr_poly_get_section |
| vsdk/unnamed_listbox_0x52ca60.cpp | 62/69 | 62/69 | |
| vsdk/gr_direct3d.cpp | 44/50 | 44/50 | (init_device closer) |
| vsdk/unnamed_net_0x517590.cpp | 26/31 | 26/31 | |
| game/unnamed_goals_doors_0x475180.cpp | 63/67 | 63/67 | (unpack_orientation closer) |
| vsdk/unnamed_gadget_0x52bfe0.cpp | 46/48 | 46/48 | |
| vsdk/unnamed_vfile_rw_0x525720.cpp | 50/51 | 50/51 | (File::read closer) |
| vsdk/unnamed_eax_0x521e40.cpp | 13/14 | 13/14 | |
| game/unnamed_states_0x431870.cpp | 13/14 | 13/14 | |
| game/new_status.cpp | 50/51 | 50/51 | |
| game/level_script_khosani_lab.cpp | 44/45 | 44/45 | |

## What worked
- Parameters assigned in place (D16200): when the binary keeps a value derived from a parameter in that
  parameter's argument slot, write the assignment to the parameter. ui_draw_bevel.
- A clamp written back into the address-taken local (`bw = imax(bw, ...)`) and a vertex-pointer local in the
  sum loop (D16201): gr_poly_get_section.
- Block-scoped locals let 8966 share slots (D16204): gr_d3d_init_device's frame now equals the binary's.
- vector2 copies for float pairs read with integer moves (D16203): unpack_orientation 111 -> 55 lines.
- len reused for the refill size, the remainder in its own local (D16202): File::read 84 -> 54 lines.

## Behaviour error fixed (sure)
- gr_poly_get_section 0x55fb00: u_scale / v_scale were max_bitmap / bitmap size; the binary computes bitmap /
  max_bitmap (0x55fc1e and 0x55fc3c, `fdiv st, st(1)` = d8 f1 with st0 = the bitmap size). The callers use
  `(u - u_off) * u_scale`, which needs bitmap / card. Now a byte match (D16201).

## Still differs (tried, see D16202-D16205)
File::read (one instruction: buf_start's add), unpack / pack_orientation (dead argument-slot packing, sign
multiplies), init_device (texture-caps `& 0xff`), send_event / set_parent / create_scrollbar / scroll_page_up /
wrap_text (register choice), door_find_floor_point (frame +8). Not attempted in depth: set_state, bitmap,
tmapper_multipass, WorkReliable, ai_goal_evaluate, listbox process / draw / add_entry, poly_split sections_draw /
at_tex_line / edge_vertex, status_handle_click, trace_beam, eax_init, state_title_enter, nw_* (earlier rounds'
notes cover them).

## Changed files (in w8-src)
- vsdk/unnamed_ui_slider_0x55e040.cpp (ui_draw_bevel, Confidence exact)
- vsdk/unnamed_poly_split_0x55f110.cpp (gr_poly_get_section, Confidence exact, logic fix)
- vsdk/unnamed_vfile_rw_0x525720.cpp (File::read)
- vsdk/gr_direct3d.cpp (gr_d3d_init_device block scopes)
- game/unnamed_goals_doors_0x475180.cpp (matrix3_unpack_orientation)
- include/DECISIONS.md (D16200-D16205)
No header changed. mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in each changed file.

## Housekeeping
Early in the run I used `vc6-compare.mjs --keep` 13 times. That left vc6cmp-* folders in
%TEMP%. Under the deletion rule I left them in place; the user may want to remove
them. Backups w8-bak-*.cpp and w8-tmp-goals.cpp are in my m-1020d folder. I wrote nothing into the repo
(git status is clean).

---

## Worker 9

## m-1020d worker 9 notes

Strict sweep of my 12 units: 313 -> 317 match of 348, 0 lost (`--check` against w9-sweep-before.txt).
Lists: w9-sweep-before.txt, w9-sweep-after.txt.

| File | Before | After | Of | New matches |
|---|---|---|---|---|
| game/unnamed_ai_move_0x4b1410.cpp | 12 | 13 | 19 | 0x4b2670 ai_state::path_closest_point |
| vsdk/unnamed_localize_0x503f60.cpp | 23 | 26 | 28 | 0x503fe0 localize_unescape, 0x5051a0 localize_remap_charset, 0x504870 localize_load_level_table |
| vsdk/gr_glide.cpp | 55 | 55 | 61 | |
| vsdk/unnamed_snd_0x5204c0.cpp | 54 | 54 | 58 | |
| game/unnamed_ai_think_0x4b5140.cpp | 25 | 25 | 29 | |
| vsdk/unnamed_vfx_0x536950.cpp | 26 | 26 | 28 | |
| vsdk/character.cpp | 32 | 32 | 34 | |
| vsdk/unnamed_vfx_material_0x564d70.cpp | 14 | 14 | 15 | |
| vsdk/unnamed_crc32_0x5520d0.cpp | 3 | 3 | 4 | |
| game/unnamed_swi_0x488230.cpp | 13 | 13 | 14 | |
| game/new_spell.cpp | 37 | 37 | 38 | |
| game/multi_chattracker.cpp | 19 | 19 | 20 | |

New matches are `Confidence: exact` with the shape in the comment.

## What worked (D16300-D16303)

- **Index loops instead of walking pointers (D16301, D16302).** When the binary tests `cmp byte ptr [reg], 0`
  before setting up its walking register and reloads the element at the loop top, the source is
  `for (i = 0; text[i]; i++)` with `text[i]` reads: 8966 strength-reduces it after the first test and gives the
  empty case its own epilogue (shrink-wrapped push). Three localize functions matched this way. Worth checking
  in other "shrink-wrapped prologue" or "first load not CSE'd" differences.
- **A too-small local type (D16300).** path_closest_point's frame was 8 bytes short with 8 unused bytes above the
  segment: the segment is a `pf_edge` (16 bytes), not a `seg2d` (8). If the binary's frame is larger by the size
  of a struct tail, look for a local declared with a smaller type.
- `x.y = x.x = 0` style chained zeroing reproduces reversed store orders (`best.x = best.y = 0.0f`).
- Statements before the profile_label declaration move register initialisations above the first call push.
- A function-scope vector reused for a later temporary keeps dead stores the binary has (its address escaped
  earlier; travel_steer_update, D16303).
- Tool: w9-vt.mjs (variant runner: whole-function `sig`/`body` replacements, multi-pair edits, `nd` = LCS distance
  of normalised instructions including stack-slot and field displacements). Note: never pipe a variant run into
  `Select-Object -First`: it kills the process before the file is restored (it happened once here; the file was
  restored by hand and every file checked against base).

## Closer, kept (behaviour identical)

- travel_steer_update 0x4b3c80: nd 187 -> 12 (dest pointer local, function-scope d, fvec + rvec). Left: the facing
  dot product (8966 emits y, z, x for every source order; the binary z, y, x) and the side vector's y / z operand
  order.

## Still differs (tried, D16304)

turn_towards (two frame-slot swaps that no declaration form moves), ai_find_nearest_wall_hit (register allocation:
`from` in EBP in the binary), glide_record::alloc (final store order), report_memory (canonical sum), and briefly
localize_set_string, localize_load_names, vfx_instance::init_nodes. Not attempted in depth: avoid_characters,
follow_path, mode_traveling_frame, the ai_think four, gr_glide_create_texture_section / bitmap / tmapper, snd_set_*
family, chat_process_irc_message, add_skeleton, vfx process, Crc32::update.

## Behaviour errors / bug candidates

None found. All changes are same-value reshapings (float additions commute; the index loops read and write the
same bytes; the pf_edge's extra fields are never read).

## Devices

None that are implausible as original source. The `best.x = best.y = 0.0f` chains are a little unusual but valid
original style.

## Changed files (my copy)

- game/unnamed_ai_move_0x4b1410.cpp (path_closest_point, travel_steer_update; comments)
- vsdk/unnamed_localize_0x503f60.cpp (localize_unescape, localize_load_level_table, localize_remap_charset)
- include/DECISIONS.md (D16300-D16304 appended). No header changed.
- mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in both changed .cpp.

Leftovers: compile folders in w9-tmp (inside my folder), and one vc6cmp-dT5cuC folder in the user Temp root from
an early `--keep` run (two files, .cod and .obj; not deleted because it is outside my folders).

---

## Worker 10

### m-1020d worker 10

Strict sweeps (VC6 SP5, VC6_STRICT=1). My 11 files: 360 -> 363 matches, none lost (w10-sweep-before.txt /
w10-sweep-after.txt). Full tree: 5,404 -> 5,407, `--check src/decomp/vc6-matches.txt` reports 0 lost
(w10-sweep-full.txt; the one `target` at characterinfo 0x448800 and the `not_found` summoner 0x43e5e0 were
there before). mingw `-fsyntax-only` passes on all .cpp (decals keeps its 9 errors), needed because of ix.h.

| File | Before | After | Of |
|---|---|---|---|
| game/living_entity.cpp | 125 | 125 | 132 |
| game/conversationctl.cpp | 20 | 21 | 27 |
| game/unnamed_dialogue_box_0x4d6660.cpp | 27 | 27 | 32 |
| vsdk/unnamed_gr_3d_0x50d9b0.cpp | 35 | 36 | 39 |
| game/unnamed_phoenix_0x497ac0.cpp | 33 | 33 | 36 |
| game/multisave.cpp | 5 | 5 | 8 |
| game/new_levelselect.cpp | 23 | 24 | 25 |
| vsdk/unnamed_clip_0x5590b0.cpp | 16 | 16 | 17 |
| game/unnamed_titus_0x4c20e0.cpp | 26 | 26 | 27 |
| game/new_skill.cpp | 38 | 38 | 39 |
| game/multicontainer.cpp | 12 | 12 | 13 |

## What worked (DECISIONS D16400-D16403)

- 0x41ce50 mp_levelselect_drag_scrollbar (D16400): D13100's charselect shape: the global index at every use, the
  count loaded again after the clamp to 0, the offset as `thumb + (mouse_y - track - start)`.
- 0x50ee70 gr_cull_bounding_box_transformed (D16401, header vsdk/math/ix.h): `plane::transform` returns the plane
  by value; the caller's `plane p = ...transform(pos, orient)` is the hidden result, so no constructor call. This
  also explains D4502's puzzle: 0x4c0a20 is plane's implicit copy constructor (from an optimised object, folded
  with the projector's identical operator=), not a second assignment type. transform has no other caller.
- 0x4d8e10 conversation_controller::camera_capture (D16402):
  `matrix43(game_camera.get_orient()).transform(vector3(0.0f, 0.0f, 1.0f))`. A temporary forward vector keeps its
  constructor call and real multiplies; a named local was folded to constants. The conversation_transform helper
  is gone.
- Closer only, kept: mp_handle_item_drop's matrix built from three temporary vectors (D16403; the binary's
  right-to-left store order).

## Still differs (tried, D16404)

- phoenix_choose_attack: `if (busy == true) { if (cur) return 5; } else if (cur) return 5;` gives the dead
  `cmp al, 1`, not kept (prologue load, `this` in EBP, stage-3 timer slot still differ).
- character_use_skill: pick-lock `fld; fcompp` (casts, local, swapped if / else, `!(a > b)`): none.
- gr_setup_3d threaded Glide test (ternaries, branch order): none.
- titus_ai_select_action block placement (condition order, inverted nesting): none.
- mp_handle_item_drop: pos via a temporary (four forms): none.
- Register choice only, not moved: last_visible_line (8 forms), mp_player_file_read_summary flag remap (7),
  gr_get_frustum_planes (10), line_talker, mp_player_file_write / _load (species / this register).
- update_head_look (/Od): the binary calls vector3's copy constructor and the third row's operator= where ours
  expands them (inline budget of `last_orient = orient`); not pursued.
- Not attempted in depth: the big /Od living_entity functions, camera_compute_shot / frame / blend / update /
  begin, phoenix_reposition_update (several temporaries, if / else of two path calls), mp_levelselect_handle_click.

## Behaviour errors / bug candidates

None found. camera_capture's old form folded the forward vector's zeros (same result except NaN / -0 edge cases).

## Changed files (in w10-src)

game/new_levelselect.cpp, game/conversationctl.cpp, game/multicontainer.cpp, vsdk/unnamed_gr_3d_0x50d9b0.cpp,
include/vsdk/math/ix.h (header: plane::transform signature), include/DECISIONS.md (D16400-D16404).

Housekeeping: my early `--keep` runs left a few `vc6cmp-*` folders in %TEMP% (not deleted, per the rule); later
runs put them under m-1020d\w10-tmp. Helper scripts: w10-search.mjs (variant search ranked by an LCS alignment),
w10-lcs.mjs, w10-side.ps1, w10-lcsv.ps1.

---

## Worker 11

### m-1020d worker 11 (D16500-D16506)

Strict sweep (VC6_STRICT=1) of my 11 units in w11-src: 332 -> 335 match, 0 lost
(`vc6-sweep.mjs --check w11-sweep-before.txt`: "332 matched there, 335 now, 0 lost").
Lists: m-1020d/w11-sweep-before.txt, w11-sweep-after.txt.
mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in each changed .cpp. No header changed (DECISIONS.md only).

| File | Before | After | Of | New matches |
|---|---|---|---|---|
| game/gameplay.cpp | 36 | 37 | 39 | 0x438dc0 game_render_frame (device, D16500) |
| game/script_internal.cpp | 20 | 21 | 28 | 0x4dcbc0 script_action_combat_mode (D16501) |
| vsdk/character_instance.cpp | 50 | 51 | 53 | 0x51abd0 character_instance::sum_root_velocity (D16502) |
| game/unnamed_interface_0x411270.cpp | 64 | 64 | 69 | (interface_init_global, ui_choice_popup_show closer) |
| vsdk/bmpman.cpp | 50 | 50 | 56 | (bm_load_internal closer) |
| vsdk/unnamed_floatmath_0x542500.cpp | 16 | 16 | 20 | |
| game/summoner.cpp | 13 | 13 | 15 | (WinMain not found, as before) |
| vsdk/unnamed_checkbox_0x553ba0.cpp | 11 | 11 | 12 | |
| vsdk/cmdline.cpp | 15 | 15 | 16 | |
| game/new_saveload.cpp | 36 | 36 | 37 | |
| game/multitrade.cpp | 21 | 21 | 22 | |

All three new matches are `Confidence: exact` with the reason in the comment.

## What worked

- **Where a local is initialised, not where it is declared (D16501).** combat_mode: `bool teleports_only = true;`
  moved after the species test. That moved skip to AL, `on` into a's dead argument slot and dropped a frame slot.
  Declaration order alone, with the initialisation at the top, changes nothing (the 24 earlier orders).
- **Reading the array element in the NULL test, the pointer local after it (D16502).** sum_root_velocity:
  8966 then builds the binary's byte-offset induction variable and register set.
- **Volatile dword read to stop byte narrowing (D16500, a device).** `(unsigned char)~*(volatile int *)&game_flags
  >> 5 & 1` gives `mov eax, [game_flags]; not al; shr al, 5`. Might also fit game_is_paused 0x432da0 (`shr al, 2`,
  another worker's file; not tried).
- An if / else if chain where Sum.exe has `cmp / jne / fstp st(0) / fld`: a switch here becomes `dec eax` tests and a
  memory-resident float (interface_init_global).
- Assigning a max to a temporary before the store (`lim = max(a, b); a = lim;`) lets 8966 sink the store as Sum.exe does.
- Tools (scratch, w11-tools/): side.mjs (compile with --keep into w11-tmp and print the side-by-side), tryv.mjs
  (variants with the side lines and a differing-line count; restores the file), gen.mjs (variants made by a JS
  function of the function's text, so there's no PowerShell quoting), apply.mjs.
  Warning: piping tryv.mjs into `Select-Object -First N` kills it before it restores the file. That happened once
  and left the bm_load_internal "value" form in place. I kept it (closer, same behaviour, checked in the sweep).

## Closer, still differs (kept; behaviour identical)

- interface_init_global 0x4116c0: 71 -> 21 lines, size equal (D16503). Left: where the 640 stores sit.
- ui_choice_popup_show 0x4136a0: loop body exact except the ESI / EDI pair (D16504).
- bm_load_internal 0x502720: 83 -> 65 lines (D16505).

## Still differs (tried; D16506)

mp_handle_it_okay (partial jump threading, 20 forms), summoner_cfg_load (tail register rotation),
fast_inv_sqrt / fast_sqrt / fast_sqrt_refined (also every #pragma optimize letter), bm_lock_pcx, bm_set_reduced,
bm_lock_vbm, script_action_replace_weapon, script_action_walk_to, script_action_replace_armor, ui_message_box_show,
ui_choice_popup_draw, cmdline_take_argument (looked at only; earlier rounds tried it at length).
Not attempted: state_game_enter, state_game_frame, saveload_handle_input, script_parse_objects,
script_entry_start / check_done, script_build_avoid_spline, bm_read_header, bm_lock_tga, build_bone_matrices,
increment_tick, float_format_buf, ui_checkbox::draw, ui_3d_begin (one look).

## Behaviour errors / bug candidates

None found. Each change keeps the value: the cursor_visible expression is bit 5 of ~game_flags, as before.
combat_mode's `teleports_only` is still true before the loop. sum_root_velocity tests the same pointer.
interface_init_global's screen width chain gives the same values: the old switch had no default and the width
stayed 640. base_w is preset to 640 and only case 2 overwrites it. fov is a copy. ui_choice_popup_show still
stores the max. bm_load_internal's `&&` value gives the same reduced flag.

## Changed files (in w11-src)

- game/gameplay.cpp (game_render_frame)
- game/script_internal.cpp (script_action_combat_mode)
- vsdk/character_instance.cpp (sum_root_velocity)
- game/unnamed_interface_0x411270.cpp (interface_init_global, ui_choice_popup_show; closer only)
- vsdk/bmpman.cpp (bm_load_internal; closer only)
- include/DECISIONS.md (D16500-D16506 appended)

---

## Worker 12

### m-1020d worker 12 notes

Strict sweeps (VC6 8966, VC6_STRICT=1): before `w12-sweep0.txt`, after `w12-sweep1.txt`; `--check` against the
before list: 359 matched there, 361 now, 0 lost.

| File | Before | After | New matches |
|---|---|---|---|
| game/unnamed_fx_0x489c40.cpp | 49/57 | **51/57** | 0x48c140 fx_create_at_camera, 0x48bdc0 fx_create_instance |
| game/unnamed_ai_combat_0x4bce60.cpp | 22/28 | 22/28 | |
| game/unnamed_serpent_0x495290.cpp | 38/43 | 38/43 | (serpent_choose_attack closer) |
| vsdk/unnamed_ds_0x55bf90.cpp | 56/60 | 56/60 | |
| vsdk/parse.cpp | 48/51 | 48/51 | |
| game/characterinfo.cpp | 67/70 | 67/70 | (0x448800 still a strict target failure) |
| game/unnamed_cutscene_camera_0x493e50.cpp | 10/12 | 10/12 | |
| vsdk/unnamed_bezier_0x558a30.cpp | 14/15 | 14/15 | |
| vsdk/file_packfile.cpp | 19/20 | 19/20 | |
| game/new_pickup.cpp | 19/20 | 19/20 | |
| game/new_assess.cpp | 17/18 | 17/18 | |
| **Total** | **359/394** | **361/394** | +2 |

## What worked (D16600, D16601)

- fx_create_at_camera: the turn written like fx_frame's kind 3 (old up and forward copied first, up = -f,
  forward = -u, right negated). It is a byte match, and it also corrects the source: it negated all three rows,
  but the binary swaps up and forward (the copy is never read, so nothing changes in game).
- fx_create_instance: `fx->orient = orient ? *orient : matrix(...)` (copies from the constructor's return), the yaw
  as `float a` plus a named `matrix rot`, and reset_flags as if / else of two calls. All three are needed.
  Lesson: when a function's frame is too big and it calls vector3's copy constructor or operator= out of line,
  look for a function-scope matrix or vector temporary that the binary does not have.

## Closer, kept (D16602)

- serpent_choose_attack: the mode tests are `owner->ai.is_mode(...)` in the binary, not `this->is_mode`. The source
  now does that. Register and frame differences remain.

## Still differs, tried (D16603)

advance (arrays `p[2], b[2], h[2]` give the binary's slot and evaluation order but the frame is 0x30, not
0x40), fx_find_spawn_point (`ends[i] = vector2(...)` closer, frame 0x88 against 0x8c, not applied),
alloc_level_buffers's target order (operand orders and declarations have no effect), ds_is_3d_buffer,
ParseInt / ParseUInt / ParseString, pickup_draw_item_list, serpent_illusion_start, living_entity_setup_head,
read_track. Not retried, with earlier rounds' notes: serpent_part_is_busy_or_asleep, packfile_read_directory,
choose_support_spell, fx_create_spell_effect_facing, fx_update_missile (the binary calls vector3 operators out of
line), combat_toggle_ready_stance, the rest of combat, ds_play / stream_open / load_buffer_acm,
assess_format_info, character_learn_chain_skill, spline::eval, fx_frame, fx_missile_hit, shadow_create_blob_bitmap.

## Behaviour errors / bug candidates

- fx_create_at_camera (0x48c14b-0x48c1ff): the dead orientation copy was turned the wrong way (all rows negated
  instead of right negated and up / forward swapped and negated). Fixed. It has no effect in game, because the
  copy is unused.
- serpent_choose_attack (0x49636b-0x4963b9): it now calls owner->ai.is_mode as the binary does. This is the same
  object when the state belongs to its owner.

## Devices

None. All changes are plausible original source.

## Changed files (in w12-src)

- game/unnamed_fx_0x489c40.cpp (fx_create_at_camera, fx_create_instance; both Confidence: exact)
- game/unnamed_serpent_0x495290.cpp (serpent_choose_attack mode tests)
- include/DECISIONS.md (D16600-D16603 appended)

mingw `-fsyntax-only` gives 0 errors on both changed .cpp files. No header was changed. My trial helpers are in
m-1020d\w12-try. The compile listings are in m-1020d\w12-tmp (TEMP was pointed there). Nothing was written in the
repo; `git status` is clean.

---

## Worker 13

### Worker 13 notes, m-1020d (byte-match round 5)

Strict sweeps (VC6_STRICT=1) of my 11 units: before `w13-sweep-before.txt`, after `w13-sweep.txt`.
Total: 208 -> 209 match, 35 -> 34 differ, none lost (checked address by address with Compare-Object).

| File | match before | match after | differ after |
|---|---|---|---|
| vsdk/unnamed_vfx_emitter_0x55ff00.cpp | 6 | 6 | 8 (move_particles closer) |
| game/new_inventory.cpp | 63 | 63 | 6 |
| game/unnamed_shadows_0x4a37d0.cpp | 8 | 9 | 4 (+0x4a46a0; obstacles / transform closer) |
| vsdk/unnamed_ca_mesh_0x50af80.cpp | 8 | 8 | 4 (morph_vertices closer) |
| vsdk/targautils.cpp | 18 | 18 | 3 |
| game/boss_ai.cpp | 55 | 55 | 3 |
| game/unnamed_editor_dialogs_0x430bc0.cpp | 3 | 3 | 2 |
| vsdk/unnamed_anim_track_0x550660.cpp | 12 | 12 | 1 |
| vsdk/gr_font.cpp | 16 | 16 | 1 |
| game/new_multichat.cpp | 11 | 11 | 1 |
| game/new_credits.cpp | 8 | 8 | 1 |

## New match
- 0x4a46a0 shadow_project_face (Confidence raised to exact, D16700): `buf.v[i]` instead of a walking vertex
  pointer, `vector3 d = clipped[i] - s->local_pos;` as a loop-body local, and `l->pos + clipped[i].unrotated(..)`.

## What worked (closer, kept; behaviour identical unless noted)
- Indexed form instead of a walking pointer, a block-scoped vector3 built by an operator, and operand order of
  a vector sum: these moved the induction pointer's bias and the scheduling (D16700).
- move_particles: `ceil(last_t)` instead of `ceil(prev)` drops the cached (double)prev slot. Frame now right,
  LCS 26 -> 4 (D16703).
- obstacles_debug_render: the segment loop rewritten in vector2 operators, an int index over the seg lists
  (`jl`), num_segs read once, corner-index locals, named to_vector3 results in the macro. LCS 150 -> 21 (D16702).
- shadow_transform_to_lobj: the door pose through vector3 operators. Frame right, LCS 130 -> 62 (D16704).
- ca_render_morph_vertices: `frame > key[-1].time` (fcompp form), ternary anim, direct last-key reads, and
  a float view of the buffer. The float view is a DEVICE (D16701). LCS 30 -> 17.

## Still differs (tried; see D16701-D16705)
- shadow_record_add / shadow_transform_to_lobj: the binary calls some inline vector3 / matrix members out of
  line and expands others. inline_depth 0-3, padding and operator forms did not reproduce it.
- fx_recb0::spawn and move_particles: prev is stored before the fstp of the ceil argument (12 forms).
- chat_input_handle_key (cancel block placement), lobj_raycast_draw_lists (ESI/EDI), pcx_read_bitmap
  (xor esi placement), credits_parse_table (zero register), get_fade (prologue), calc_rot_tangents.
- Not attempted in depth: the three ca_mesh renderers, editor_dialog_show, inventory_handle_click,
  boss_ai_unload / move_in_range_dispatch (they need other units' declarations, D7603), render_d3d,
  get_transform, init_particle, the inventory and-order trio (80+ forms tried before).

## Behaviour errors / bug candidates
- obstacles_debug_render (0x4a3890): the binary never writes mid.y in the segment loop. The normal tick starts
  at a stale frame value; the slot is not written in the loop (0x4a3ae4-0x4a3db0). The old source wrote the
  debug height there. The source now leaves it unset, as the binary does (debug drawing only, D16702).
- No other behaviour changes.

## Changed files (in w13-src)
- game/unnamed_shadows_0x4a37d0.cpp (0x4a46a0 match, 0x4a3890 and 0x4a4890 closer; the OBSTACLE_LINE macro)
- vsdk/unnamed_ca_mesh_0x50af80.cpp (0x50b150 closer)
- vsdk/unnamed_vfx_emitter_0x55ff00.cpp (0x560a00 closer)
- include/DECISIONS.md (D16700-D16705 appended)

mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in the three changed .cpp files. No header changed.
Scratch tools: w13-work/ (t.mjs: a vc6-try wrapper with LCS distance and frame size; gen.mjs: multi-edit
variants; apply.mjs).
Housekeeping: one early `vc6-compare.mjs --keep` run of the 11 units may have left vc6cmp-* folders in the
system Temp folder. I did not delete them (they are outside my folders).

---

## Worker 14

### m-1020d worker 14

Tree: `m-1020d/w14-src`. VC6 SP5 (cl 12.00.8966), strict (`VC6_STRICT=1`). Sweeps of my 8 units:
`w14-before.txt` (253 match / 35 differ) and `w14-sweep.txt` (**261 match / 27 differ**);
`vc6-sweep --check w14-before.txt`: 0 lost. mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in the four
changed .cpp files. No header and no flags change. Decisions D16800-D16808 appended to `w14-src/include/DECISIONS.md`.

## Per file (before -> after / total)

| file | before | after | won |
|---|---|---|---|
| game/level_scripts.cpp | 100 | 105 / 124 | 0x4cf500 write_party_state, 0x4d0d00 level_item_add, 0x4d0d60 level_item_remove, 0x4d0dd0 level_item_has, 0x4d33f0 read_level_items |
| game/unnamed_effectstream_0x440400.cpp | 12 | 14 / 14 | 0x440560 effect_stream_start, 0x440930 effect_stream_finish_entry |
| game/unnamed_camera_0x4778b0.cpp | 44 | 45 / 47 | 0x477f60 camera::update_orientation |
| game/new_dialogue.cpp | 5 | 5 / 6 | (dialogue_frame closer) |
| game/new_journal.cpp | 26 | 26 / 27 | |
| vsdk/matrix.cpp | 45 | 45 / 46 | |
| vsdk/ui_container.cpp | 9 | 9 / 10 | |
| vsdk/unnamed_linefile_0x558340.cpp | 12 | 12 / 14 | |

All 8 won functions are `Confidence: exact` with the form in the comment.

## What worked

- **One conditional store for a bit field (D16800)**: `ps.selected = cond ? -1 : species;` instead of if / else
  lets 8966 merge neighbouring bit-field stores (selected + summon = -1 -> `(sel | -8) << 7`).
- **Case order still matters for block placement (D16801)**: `case -1: return 0;` last; tried all 24 orders.
- **A shared `return 1` (D16804)**: a store of 1 followed by its own `return 1` gets CSE'd (`mov eax,1; mov [g],eax`);
  nested ifs with one `return 1` at the end give the binary's `mov [g],1; mov eax,1` in each path.
- **Byte-offset search loops (D16805, D16807)**: the level item list searched as level_item_add does
  (`data + off`, `off += 4`, index loop) fixes the pointer placement / re-test of three functions; for
  level_item_add itself, accessing `level_item_hashes.data` directly (no local) gives the `[data + off]` order.
- **Record pointer** `level_item_rec *r = &level_items[i]` for an EAX / ECX swap (read_level_items).
- **Ring step as a conditional expression (D16808)**: `i = i - 1 < 0 ? 3 : i - 1` gives `dec / test / jge`
  (`if (--i < 0)` gives `jns`).
- **`fmt = XSTR(...)` before a sprintf with a call argument** also removes a callee-saved register from other
  regions, so 8966 shrink-wraps EBX into the one switch case that needs it (D16802).
- Tools in my folder: `w14-t.mjs` (variants written to copies outside the tree, compiled in parallel; the source
  file is never modified; `W14_SIDE=<name>` adds the vc6-side output) and `w14-lcs.mjs` (LCS alignment of a
  vc6-side output, mnemonics or masked operands: shows the real differing instructions instead of a shifted
  column). Both could become repo tools.

## Closer, kept (behaviour unchanged)

- conversation_execute_action 0x4d5990: fmt locals + kind 11 as `>=` (unmatched 74 -> 48 instructions); only kind
  7's false call (`push 0; jmp` back into kind 7's own call) differs.
- dialogue_frame 0x409000: the second mode update stores 3 then 1 as the binary (43 -> 34); the `key` load before
  the tail's mode test still differs.
- get_speaker_bit 0x4d05f0: all but the data / bits_off register pair (EDI/EBP vs EBP/EBX) and two pops.

## Still differs (tried)

matrix43::transpose_in_place (dead temporary stores: volatile, address-taken, array, memcpy, int, flat
pointer, __forceinline swaps; none keeps the four stores); camera::auto_view_update (late `push ebx`: 4 more
nesting forms); camera::set_mode (not tried further); ui_container::draw (sy's fstp one slot early: 6
declaration orders); init_conversation_slot (CONVERSATION_SAVE_INFO macro forms: this in EDX vs ESI);
savegame::read (EAX/EDX swap in the File::read arguments: 6 forms); hot_button_set (store base / index:
5 spellings); LineFile::detect_eol (`char *end = cr + 1; if (cr > end || ...)` reproduces the binary's odd
`cr > cr + 1` test; the block order and the late `push edi` are still different, 6 nesting forms).
Not attempted: get_int, set_speaker_bit, topic bits, two_stage_*, read_items, read_two_stage,
read_automap_labels / automap_label_remove / layout_conversation / write_item_objects (frame slots),
level_add_navpoint, level_format_display_name, journal_handle_click, LineFile::read_line.

## Behaviour errors / bug candidates

None found; no behaviour change. Every change is an equivalent form (`!= 0` for a 1-bit field, a conditional
expression, case order, one shared return, the same bytes read at a byte offset, a record pointer, the count
re-read from the unchanged save table).

## Files changed (in w14-src)

- game/level_scripts.cpp
- game/unnamed_effectstream_0x440400.cpp
- game/unnamed_camera_0x4778b0.cpp
- game/new_dialogue.cpp
- include/DECISIONS.md (D16800-D16808 appended)

Note: an early vc6-try run whose output I piped into `Select-Object -First` was killed while restoring and left
w14-src/game/level_scripts.cpp empty; I restored it from the repo (identical to base) and from then on used
w14-t.mjs, which never writes the source file. Compiler temp folders went to m-1020d\w14-tmp (TEMP pointed
there); variant copies are in m-1020d\w14-var. Nothing deleted.

---

## Worker 15

### m-1020d worker 15 notes

Strict sweep (VC6_STRICT=1) of my ten units, before (w15-sweep-before.txt) and after (w15-sweep.txt):
242 match, 35 differ in both; no address lost, none gained.

| File | before | after | of |
|---|---|---|---|
| game/unnamed_render_scene_0x481680.cpp | 62 | 62 | 77 |
| game/new_hud.cpp | 55 | 55 | 60 |
| game/unnamed_tentacle_beast_0x4c6480.cpp | 21 | 21 | 25 |
| game/unnamed_decals_0x4bbe80.cpp | 12 | 12 | 15 |
| vsdk/unnamed_gr_poly_0x5450c0.cpp | 22 | 22 | 24 |
| game/unnamed_lobj_raycast_0x4ae890.cpp | 5 | 5 | 7 |
| vsdk/stringpool.cpp | 15 | 15 | 16 |
| vsdk/skeleton.cpp | 27 | 27 | 28 |
| game/new_int_multi_update.cpp | 4 | 4 | 5 |
| game/new_gamecreate.cpp | 19 | 19 | 20 |

## What worked (closer, no new match)
- D16900 levelrender_push_attachment 0x483430: inline budget. Writing the depth with locals and vector
  operators (center / radius copies, `d = center - eye`, `d.dot(fvec)`) makes 8966 expand matrix::operator=
  as the binary does (two rows inline with an out-of-line copy constructor call, the third row's operator=
  called). Only the radius store is two instructions early now (scheduling).
- D16901 tentacle_beast_frame_hook 0x4c7960: same idea plus ray / hit declared after the thrown test (the
  lobj_ray constructor's min_radius store); 170 -> 127 differing instructions.
- D16902 gamecreate_handle_click 0x409de0: the closing Escape test is a one-case switch; tail now matches.

## Still differs (see D16903 for what was tried)
level_vis_reset, level_vis_parse (zero register), hud_drag_icon_begin (bit-field fold), add_effect_entry
(POP ESI scheduling), StringPool::Add, hud_portrait_get_color (clamp), hud_hot_buttons_drag (ECX/EBP copy of
dst), lobj_sphere_test_faces (dead compare), gr_poly_clip_edge (address order), version_check_run (tail
merge), draw_name_labels (frame 44 bytes larger in the binary), decals_draw_for_lobj (scheduling),
tentacle boss_load (one push order). Not attempted in depth: render_scene, fade_occluders, build_draw_lists,
draw_list, add_effects, add_lobj, add_items, add_containers, draw_speech_text, overhead_label_place,
hud_handle_keys, hud_interact_nearest, tentacle select_action / ai_frame, decal_build_clip_planes,
decal_place, gr_poly_clip_and_draw, lobj_raycast_bsp, skeleton::read.

Tip for others: the inline budget is spent in source order; a function whose binary calls vector3 copy
constructors or operator= out of line late in the function often needs its *earlier* code written with the
inline vector operators (operator-, dot, operator* by float) instead of per-component arithmetic.

## Behaviour errors / bug candidates
None found. All changes keep the computed values (same operands, same order of evaluation per component).

## Devices
None added (the D16901 pieces are ordinary source forms).

## Checks
mingw `-fsyntax-only -std=c++98 -w -Iinclude` on the three changed units: 0 errors. No header changed.

## Changed files (in w15-src)
- game/unnamed_render_scene_0x481680.cpp (levelrender_push_attachment)
- game/unnamed_tentacle_beast_0x4c6480.cpp (tentacle_beast_frame_hook)
- game/new_gamecreate.cpp (gamecreate_handle_click)
- include/DECISIONS.md (D16900-D16903 appended)
