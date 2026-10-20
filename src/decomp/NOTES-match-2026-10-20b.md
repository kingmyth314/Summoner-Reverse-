# Byte-match round 2026-10-20 (second): the fifteen workers' notes

Merged into src/decomp (5,255 -> 5,317 strict matches; DECISIONS D12500-D13999). One section per worker, as written. The round's folder was wiped mid-run (see docs/decomp-plan.md); workers rebuilt their copies from the round's base commit.

---

## Worker 1

### m-1020b worker 1 notes

Strict sweep (VC6 8966, VC6_STRICT=1) of my 10 units. Before: 276 match, 45 differ (my first sweep, and the same
list as src/decomp/vc6-matches.txt for these units; saved as w1-before.txt). After: 277 match, 44 differ, 0 lost
(`vc6-sweep --check w1-before.txt` passed; w1-after.txt). mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors
in both changed files (no header changed). Decisions D12500-D12503 appended to w1-src/include/DECISIONS.md.

Incident: at about 07:21 the whole m-1020b folder (base, every wN-src, wN.txt) and the m-1020 folder were deleted
while I was working. I recreated w1-src from the repository's src/decomp (git clean at ded7d0b, the round's base)
and redid my one edit; my helper scripts now live in scratchpad/w1b-try. The coordinator's `base` snapshot is gone
too; it equals src/decomp at ded7d0b.

## Per file (matches before -> after, of total)

| file | before | after | total |
|---|---|---|---|
| game/unnamed_script_editor_0x4de0b0.cpp | 98 | 98 | 120 |
| vsdk/window.cpp | 35 | 35 | 40 |
| vsdk/unnamed_sndstream_0x566560.cpp | 22 | 22 | 26 |
| game/new_pxochat.cpp | 53 | 53 | 57 |
| game/summoner_cd.cpp | 3 | 3 | 6 |
| game/unnamed_pyrul_0x4b87c0.cpp | 21 | 21 | 23 |
| game/new_dialogue.cpp | 4 | 5 | 6 |
| game/unnamed_titus_0x4c20e0.cpp | 26 | 26 | 27 |
| game/unnamed_experience_0x489930.cpp | 7 | 7 | 8 |
| game/item_paging.cpp | 7 | 7 | 8 |

## New match

- 0x409650 dialogue_draw (D12500). The m-1020 w6 match was lost in the last merge: D11003 made gr_string take
  gr::Mode by value, so `gr_string_mode.value` became a call of the out-of-line gr::Mode(gr_mode) constructor.
  Passing `gr_string_mode` itself is exact. No other unit uses `.value` for a gr_string argument.

## Closer, kept (same behaviour)

- 0x4e8cf0 script_editor_default_action_navpoint (D12501): a function-scope `found = NULL` returned from and after
  the loop gives the binary's ESI zero across the call and the hoisted address; 37 -> 21 differing lines. Left:
  register choice (address in ESI, the binary's in EAX).
- 0x4e5b10 script_editor_add_navpoint (D12502): the type dispatch as a second switch (binary `jl / jle / jne`, ambient
  case first). The first difference moved from +0x59 to +0x365 (the level case's vector temporaries' slots).

## Still differs, tried (D12503)

wav_open (imul operand order only), item_page_out_all (EBX / EBP), pxochat_page_down / drag_scrollbar (register
roles), pyrul_fire_attack (VC6 inline budget: `#pragma inline_depth(0)` gives the 0x9c frame and the out-of-line
vector3 constructors but also calls what the binary expands), handle_arrow_key (frame slot order of sw / sh / bottom),
level_item_ok (e / count swapped between the dead argument slots; a restart-loop form gives the walking pointer but
was not applied), titus_ai_select_action (merged-tail forms worse), xp_level_up_gains and dialogue_frame (left as
D11403 / D11503 describe). summoner_cd's three are the no-CD patch. Not attempted in depth: the large script editor
functions (create_ui, handle_input, the write_* functions), parse_radio_button / parse_slider / ui_window::init,
pxo_parse_chat_banners_cfg, pxochat_handle_click, ui_window::send_event (a header question, D11310).

## Behaviour errors / bug candidates

None found. Every change keeps behaviour (checked against the binary listing).

## Changed files (in w1-src)

- game/new_dialogue.cpp
- game/unnamed_script_editor_0x4de0b0.cpp
- include/DECISIONS.md (D12500-D12503)

Tool note: scratchpad/w1b-try/rd.mjs is m-1020 w4's register-aware diff with `ESP=1` (resolve VC6 frame symbols to
esp offsets, rough) and `FULL=1` (whole side listing). Never pipe vc6-try.mjs into `select -first`: a stopped
pipeline leaves the variant in the file.

---

## Worker 2

### m-1020b worker 2 notes

Tree: `m-1020b/w2-src`. Compiler: SP5 VC98 (cl 12.00.8966), strict. Before / after: `w2-before.txt` /
`w2-sweep.txt`. **270 -> 274 of 314 functions; none lost** (`vc6-sweep.mjs --check w2-before.txt`: 0 lost;
the only changed lines are the four new matches). No header and no flags change. mingw `-fsyntax-only
-std=c++98 -w -Iinclude`: 0 errors in both changed .cpp. Decisions D12600-D12602 appended to
`w2-src/include/DECISIONS.md`.

**Redone after the Temp deletion:** at about 07:21 the whole `m-1020b` folder (my copy, before sweep and helper
scripts) was deleted by another worker's helper. I rebuilt `w2-src` from the repo's `src/decomp` (git HEAD
ded7d0b, the repo clean; the copy has the same 536 files and differs from the repo only in my two changed
files), restored `w2.txt`, re-ran the before sweep (same result: 270 / 44) and redid the one edit made before the
loss (vif_emitter_spawn_point). I never ran a recursive delete.

## Per file (matches before -> after / total)

| file | before | after | won |
|---|---|---|---|
| vsdk/gr_opengl.cpp | 41 | 43 / 54 | 0x54df90 gl_convert_888_to_rgba5551, 0x54e050 gl_convert_8888_to_rgba4444 |
| vsdk/unnamed_vif_0x5329f0.cpp | 8 | 10 / 15 | 0x532cb0 vif_emitter_spawn_point, 0x533ca0 vif_curve__evaluate |
| game/level_scripts_common.cpp | 72 | 72 / 78 | |
| game/boss_ai.cpp | 53 | 53 / 58 | |
| game/new_multichat.cpp | 8 | 8 / 12 | |
| game/unnamed_decals_0x4bbe80.cpp | 12 | 12 / 15 | |
| game/unnamed_projectors_0x4c0250.cpp | 9 | 9 / 11 | |
| game/new_itemtrade.cpp | 34 | 34 / 36 | |
| game/unnamed_swi_0x488230.cpp | 13 | 13 / 14 | |
| game/unnamed_frame_0x4323e0.cpp | 20 | 20 / 21 | |

The four won functions are `Confidence: exact` with the shape named in the comment.

## What worked

- **`int seg = -1; for (...) if (found) { seg = i; break; } if (seg < 0) last;` (D12600).** Build 8966 threads the
  loop's normal exit straight to the "not found" block and tests only `i < 0` after the break, exactly the binary
  shape D10311 could not reproduce. Worth trying on other search loops followed by an index test.
- A byte count tested in place (`path->count == 1`, `> 2`) and the int copy read after the first test; the last
  key indexed by `c->count - 1` again (curve__evaluate's zero-extension order).
- A float local for an int that is added to a float (`float fi = (float)i; ... + fi`) gives fild / faddp instead
  of fiadd.
- The line case as an `else` block with a named `vector3 dir`: the temporaries' frame slots then follow the binary.
- Packed converters: `ubyte *s = src;` first, then the D11200 guarded do-while with the destination pointer set
  inside the guard (D12601).
- `&static_cast<const vector3 &>(a + b)` for the binary's use of operator+'s returned pointer as a call argument
  (legal C++, mingw accepts it); vif_draw_particles 92 -> 9 differing lines.
- `*d++ = ...` in both arms (a8 converter: every register now right).

## Closer, still differs (kept; behaviour identical)

- gl_convert_a8_to_rgba8888: only `mov bl, [esi + ecx]` (ours `[ecx + esi]`).
- vif_draw_particles: register rotation of gr_bitmap_3d's three arguments.
- vif_record_get_bounds: the int-to-float temporary's dead argument slot (rec's against radius's).
- vif_instantiate_level: prologue and desc register now right; used / count EDX / ECX swapped, plist_remove's
  block order.

## Still differs, tried (D12602)

itemtrade_count_party_rows (`inc [mem]`), level_trigger_point_left_of_line (8 unused frame bytes),
mp_handle_swi_path (walk flag through the dead data slot), party_set_level and the a8 converter (SIB operand
order), chat_input_handle_key (Enter falling into Esc gives the same code), the 4444 / 1555 scaled converters
(the binary's texel `lea`). Not attempted: boss_threat_add / parse_table / target_list_build / unload /
move_in_range_dispatch, level_check_triggers / parse_table / parse_shopkeeper / script_parse_table,
decal_build_clip_planes / decal_place / decals_draw_for_lobj, projector functions, itemtrade_handle_click,
game_is_paused, the large multichat / gr_opengl functions, vif_compact_particles / vif_emitter_update.

## Behaviour errors / game-bug candidates

None found. All changes keep behaviour: `seg < 0` holds exactly when the old `i >= n - 1 || i < 0` did;
vif_instantiate_level's `vif_particle_used = total` stores the same `count + used` as `+= desc->count` (nothing
writes the global in between).

## Files changed (in w2-src)

- vsdk/unnamed_vif_0x5329f0.cpp
- vsdk/gr_opengl.cpp
- include/DECISIONS.md (D12600-D12602 appended)

Helper scripts (scratch only): `m-1020b/w2-try/` (common.mjs, rd.mjs, rdall.mjs, side.mjs, frame.mjs, apply.mjs;
rewritten after the deletion from m-1020 worker 4's rd / rdall).

---

## Worker 3

### Worker 3, match round 2026-10-20b

Strict sweep of my 11 units: 286 -> 294 matches (330 functions), 0 lost (`--check src/decomp/vc6-matches.txt`).
Sweep lists: w3w/sweep-before.txt, w3w/sweep-after.txt. Decisions D12700-D12707 in w3-src/include/DECISIONS.md.

**Copy rebuilt:** the m-1020b folder was wiped around 07:21 (the other worker's `rm -rf` with an empty variable).
I had made no edits yet; w3-src was re-copied from the repo's src/decomp (clean at HEAD ded7d0b) and w3.txt
re-created, and the baseline sweep repeated (same 286 / 44). All edits below were made after that. My helper
scripts (w3w/) no longer delete anything (the compare temp folders they keep are left in place).

| File | Before | After | Won |
|---|---|---|---|
| game/new_levelselect.cpp | 23/25 | 23/25 | |
| game/unnamed_evil_urath_0x4c4170.cpp | 15/18 | 17/18 | 0x4c4870 leap_rise, 0x4c4ad0 leap_travel |
| game/unnamed_gamelist_0x4705b0.cpp | 13/14 | 13/14 | |
| game/unnamed_party_inventory_0x46b460.cpp | 28/30 | 29/30 | 0x46bc00 character_use_inventory_item |
| game/unnamed_states_0x431870.cpp | 13/14 | 13/14 | |
| vsdk/bmpman.cpp | 47/56 | 49/56 | 0x501f20 bm_release_from_mark, 0x5020f0 bm_free_data |
| vsdk/gr.cpp | 56/61 | 56/61 | |
| vsdk/unnamed_gr_3d_0x50d9b0.cpp | 35/39 | 35/39 | |
| vsdk/unnamed_gr_line_0x53e520.cpp | 13/19 | 13/19 | |
| vsdk/unnamed_ix_0x53ac20.cpp | 27/35 | 29/35 | 0x53b400 ix_linesegment_sphere, 0x53d4d0 ix_linesegment_boundingbox |
| vsdk/unnamed_vfx_channel_0x55b440.cpp | 16/19 | 17/19 | 0x55b730 fx_rec4c::get_transform |

## What worked
- Stack-slot evidence: reading the binary's frame offsets against the listing's `_x$ = n` lines showed which
  values share a slot. Two locals reused by three faces (ix_linesegment_boundingbox, D12702; D6305's "build
  difference" was this); one variable for two uses (ix_linesegment_sphere's t, D12703); one local per branch
  in if / else blocks instead of an early return (evil_urath leap_rise / leap_travel, D12701).
- A shared epilogue: `||` for two false tests and the last test as an if around the rest (D12703).
- frand() in a local inside its own block (D12700).
- A /Ob0 unit with an inline member that the binary does not call: write the member's expression
  (num_frames -> `header.frame_word >> 8`, D12704).
- A local that carries a re-read global across the loop (bm_release_from_mark's `first`, D12705) and a walking
  entry pointer (bm_free_data, D12706).

## Still differs (tried, D12707)
- state_title_enter, gr_get_frustum_planes, mp_levelselect_drag_scrollbar: register / schedule only.
- bm_prefer_vbm, bm_lock_tga: all callee-saved registers pushed before the first test in the binary.
- bm_set_reduced (`and eax, 0xff`), bm_load_internal (reduced in BL), bm_lock_pcx (`[8*esi+..]` addressing).
- gr_line (int copy scheduled early), ix_box_oriented_box_oriented (product order normalised), seg2d_intersect.
- gr_project_vertex: `ubyte flags = v->flags;` fixes the z <= 0 exit, but 8168 places the `flags & 3` exit last.
- gr_line_3d: locals v0 = p0, v1 = p1 and `ret` flowing to one end give the 8-byte frame and memory ret; the
  register order still differs (forms in D12707; not applied).
- fx_rec4c::update: num_frames inline and the end compare / quaternion local applied (closer, not matching); the
  binary gives every named local and temporary its own slot, VC6 8168 overlaps them.
- Not attempted in depth: gr_init, gr_screenshot, gr_bitmap_float (FPU schedule), gr_tmapper_multipass,
  gr_setup_3d, gr_poly, the gr_bitmap_3d_* and gr_draw_sphere frames, the remaining ix functions,
  vfx_channel::eval, bm_read_header, bm_lock_vbm, character_equip_item, mp_handle_actv_gm,
  mp_levelselect_handle_click, evil_urath_ai_select_action.

## Behaviour errors / bug candidates
- None found. Every change keeps the behaviour, including NaN cases (checked against the binary's
  `test ah, 0x41` branches in 0x46bc00 and 0x55b460).

## Changed files (in w3-src)
- game/unnamed_evil_urath_0x4c4170.cpp
- game/unnamed_party_inventory_0x46b460.cpp
- vsdk/bmpman.cpp
- vsdk/unnamed_ix_0x53ac20.cpp
- vsdk/unnamed_vfx_channel_0x55b440.cpp
- include/DECISIONS.md (D12700-D12707)

No header changed. mingw `-fsyntax-only -std=c++98`: 0 errors in the five changed .cpp files.

---

## Worker 4

### Worker 4 notes (match round m-1020b)

Strict VC6 sweep (cl 12.00.8966, VC6_STRICT=1) of the 10 files: **234 -> 237 matches, 0 lost**
(`vc6-sweep --check w4-before.txt`). Decisions D12800-D12803 in w4-src/include/DECISIONS.md. No header changed.
mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in each changed file.

**Folder wipe.** The m-1020b folder (my copy, file list, first before-sweep and helper scripts) was deleted during
the round by another worker's `rm -rf` (coordinator's message). I rebuilt w4-src from the repo's src/decomp
(HEAD ded7d0b, clean) and redid my two edits made by then (D12800, D12801). The original before-sweep list was lost;
`w4-before.txt` is reconstructed from the repo's src/decomp/vc6-matches.txt for these 10 files (278 lines, 234
match: the same as my first strict sweep's 234 / 44 summary). Backup of the changed files, DECISIONS.md and the
sweep lists: scratchpad\w4b-backup. My helper scripts no longer delete anything.

| File | Before | After / total | New matches |
|---|---|---|---|
| game/unnamed_render_scene_0x481680.cpp | 61 | 61 / 77 | (add_effects closer) |
| game/pathfinding_visibility.cpp | 4 | 4 / 11 | |
| game/unnamed_tiger_0x499e50.cpp | 43 | 43 / 48 | |
| vsdk/unnamed_inputbox_0x5521a0.cpp | 9 | 9 / 13 | |
| vsdk/unnamed_vfx_0x536950.cpp | 25 | 26 / 28 | 0x537a10 get_node_world_transform |
| game/unnamed_netsession_0x440b40.cpp | 18 | 19 / 21 | 0x441150 multi_process_client_status |
| game/unnamed_netpackets_0x441820.cpp | 10 | 10 / 12 | |
| game/new_spell.cpp | 36 | 37 / 38 | 0x42a030 spells_set_page |
| game/unnamed_snow_0x4946a0.cpp | 10 | 10 / 11 | |
| game/unnamed_giant_salamanka_0x4c8030.cpp | 18 | 18 / 19 | |

New matches have `Confidence: exact` with the shape in the comment.

## What worked

- A field read into a local before the packet header (multi_process_client_status, D12800): sets the load and
  store order of the packet fields.
- Store order: the argument stored to its global first (spells_set_page, D12801), found by trying each position
  with vc6-try.
- Frame size from offsets: the binary's 12 extra bytes were a second kind / list / sub in the loop block
  (get_node_world_transform, D12802). Mapping each local's esp offset in both listings showed the loop using
  other slots than the function-scope ones.

## Still differs (tried; D12803)

Documented-hopeless categories, not worth another round without a new idea:
- mp_player_alloc, pathvis_add_edge_node: list_remove's `lea ecx, [eax+4]` reload (D7006).
- mp_reset_net_stats: the second zero register (D7006). cmd_disp_rate, mp_session_reset, tiger_* (pick_target,
  combat_update, ai_frame, anim_remap_in), salamanka select_action, snow_start: register choice / scheduling.
  snow_start comes within two swapped instructions with a separate next-row pointer (not kept).
- character_use_skill_on: shrink-wrapped `push ebx` (D8701); five single-exit forms all push at entry.
- level_vis_reset: the -4 biased loop pointer with jb.
- levelrender_add_items / add_containers / add_lobj: the binary keeps d.y, d.z on the FPU stack and squares copies
  (len()'s pattern without the fsqrt); no float / double / len_sq / dot form gives it. add_effect_entry: integer
  stores right with type / fx before the depth, but not the depth's in-place x, z, y sum.
- set_valid_chars / set_invalid_chars: a function-scope `lo` gives the binary's frame (0x8c), but 8966 then
  cross-jumps the three identical range blocks that the binary keeps separate.
- effect_apply_vis: the tag offset lives in the first argument's dead slot (D7006).
- tiger_path_to_target: out-of-line vector3 constructors / copies / operator= in a mix inline_depth does not give.
- draw_speech_text: a shared return where 8966 duplicates the epilogue. draw_name_labels / level_vis_parse:
  frame and register layout far apart; not pursued.
- levelrender_add_effects: closer (a name local for the _stricmp argument; `vector3 c = fx->pos +
  vi->pos.unrotated(fx->orient);`), now differs only from the node loop's layout on.

## Behaviour errors / bug candidates

None. The three wins and the add_effects reshaping are same-behaviour: client_status reads p->type before the
packet-table counter increment (no aliasing); set_page reorders stores to independent globals;
get_node_world_transform's loop-block kind / list / sub shadow the outer ones, which are not read after the loop;
`_stricmp(base->name, ...)` through a local and `fx->pos + unrotated` compute the same values.

## Files changed (in w4-src)

- game/unnamed_netsession_0x440b40.cpp
- game/new_spell.cpp
- vsdk/unnamed_vfx_0x536950.cpp
- game/unnamed_render_scene_0x481680.cpp (add_effects closer only, no match change)
- include/DECISIONS.md (D12800-D12803)

---

## Worker 5

### m-1020b worker 5 notes

**HEADER CHANGE (merge deliberately): include/vsdk/math/vector.h, vector3::len body is now
`return (double)(float)sqrt(x * x + y * y + z * z);` (D12900).** It reproduces the two fst stores that every
unoptimised living_entity caller has. Full strict sweep of the whole tree with it: 5,255 -> 5,261, 0 lost
(w5-full-after.txt, checked against src/decomp/vc6-matches.txt). mingw -fsyntax-only on all 259 .cpp: only
the 9 known unnamed_decals errors. The COMDAT 0x4bc290 is in a comment block and not compared.

**Scratchpad wipe:** my copy w5-src was deleted mid-round with the rest of the Temp folder (not by me). I rebuilt
it from the repo's src/decomp (HEAD ded7d0b); the baseline sweep was the same (392 match / 43 differ / 1 not
found). No edits had been made before the wipe, so nothing was redone. Backups of changed files, sweeps and
these notes are also in %TEMP%\w5-backup. My helper no longer deletes anything.

## Counts (strict, before -> after)

| file | before | after | won |
|---|---|---|---|
| game/living_entity.cpp | 116 | 122 / 132 | anim_speed_at_blend 0x456ed0, get_speed 0x457060, stopping_distance 0x45a840, distance_to_reach_blend 0x45a930, distance_to_reach_speed 0x45aa20, ai_travel_offset 0x455e00 |
| game/unnamed_ai_combat_0x4bce60.cpp | 21 | 21 / 28 | (combat_approach closer) |
| game/unnamed_serpent_0x495290.cpp | 38 | 38 / 43 | |
| vsdk/unnamed_lights_0x51e070.cpp | 37 | 37 / 41 | |
| vsdk/unnamed_vfile_0x5128b0.cpp | 79 | 79 / 82 | |
| game/unnamed_phoenix_0x497ac0.cpp | 33 | 33 / 36 | |
| game/unnamed_luminar_0x4c8d10.cpp | 21 | 21 / 23 | |
| game/summoner.cpp | 13 | 13 / 15 | |
| game/unnamed_machival_0x4c0c40.cpp | 25 | 25 / 26 | |
| game/unnamed_level_complete_0x4b0960.cpp | 9 | 9 / 10 | |

My ten files: 392 -> 398, 0 lost (w5-before.txt / w5-sweep.txt). Whole tree: 5,255 -> 5,261.
All six wins are `Confidence: exact`.

## What worked
- vector3::len as round-then-promote (D12900): two /Od matches by itself.
- D6101 bucket renames: `l1` (bucket 7) for stopping_distance's length.
- A named `dst` local for a returned expression where the binary stores the result in a frame slot before
  returning (distance_to_reach_blend / _speed).
- Operand roles of operator+: `pos + offset` (ai_travel_offset).
- `? true : false` for an int call result returned as bool gives test / setne (combat_approach; 8966 gives
  neg / sbb / neg for `!= 0`, implicit conversion, `(bool)` and `!!`).

## Still differs (D12902)
combat_approach (boss-branch info reload, the goal_do_cast return), set_cipher_key (zero register),
get_async_status (unfolded slot address), scale_stats_to_level (fidiv), Character::say (loop tail scheduling),
luminar_boss_load (FPU registers), gr_env_map_uv / lights_calc_point_specular_color (lea scheduling),
move_frame (out-of-line matrix row: inline budget), the remap_in copies (D7702, not retried), and the rest of
the list untouched (save/load_state, update_head_look, clip_move_to_navmesh, ai_move_frame, get_hit_by_spell,
Character::Character, serpent/phoenix AI, machival, mp_handle_combat_sequence, summoner_cfg_load, lights).

## Behaviour errors / game-bug candidates
None found. All changes are same-value reshapings.

## Changed files (in w5-src)
- include/vsdk/math/vector.h (D12900)
- game/living_entity.cpp
- game/unnamed_ai_combat_0x4bce60.cpp
- include/DECISIONS.md (D12900-D12902)

---

## Worker 6

### m-1020b worker 6 notes

## Copy rebuilt

At about 07:21 the whole m-1020b folder was wiped (the empty-variable `rm -rf` another worker ran). I had not made
any edits by then. I rebuilt w6-src from the repo's src/decomp at HEAD ded7d0b (git status was clean) and recreated
w6.txt; all the edits below were made on the rebuilt copy. My helper scripts in w6-v/ no longer delete anything
recursively (the vc6-compare temp folders stay in %TEMP%).

## Counts (strict, VC6_STRICT=1)

Before = src/decomp/vc6-matches.txt (the w6-before.txt baseline was lost in the wipe; a sweep of the untouched copy
gave the same numbers). After = w6-sweep.txt, run with `--check src/decomp/vc6-matches.txt`: 354 -> 355, 0 lost.

| file | before | after | of |
|---|---|---|---|
| game/pathfinding.cpp | 23 | 23 | 36 |
| game/new_hud.cpp | 52 | 52 | 60 |
| game/ai.cpp | 82 | 82 | 88 |
| vsdk/unnamed_snd_0x5204c0.cpp | 54 | 54 | 58 |
| game/new_saveload.cpp | 33 | **34** | 37 |
| game/new_options.cpp | 32 | 32 | 35 |
| vsdk/character.cpp | 32 | 32 | 34 |
| game/new_credits.cpp | 7 | 7 | 9 |
| vsdk/cmdline.cpp | 15 | 15 | 16 |
| game/unnamed_evil_joseph_0x4c3470.cpp | 18 | 18 | 19 |
| game/level_script_ikaemosu.cpp | 6 | 6 | 7 |

New match: 0x426a20 saveload_draw_list (Confidence raised to exact).

## What worked

- saveload_draw_list (D13000): the row-count clamp compares the global (`if (saveload_num_saves >= page) rows =
  page;`), so `rows` stays only in its stack slot as in the binary. An if / else of the two stores or a ternary also
  match.
- saveload_draw_info (D13001, closer, still differs): the am / pm copy as an if / else of two strcpy calls. Every
  instruction and register now matches; only two stack slots differ (w / ampm swapped, 4 bytes between buf and line;
  frame 0x25c in the binary, 0x258 here). Declaration order does not move them.

## Still differs (tried, D13002)

- snd_set_volume, snd_set_volume_in_group, snd_set_pan, snd_set_group_volume: the clamp's block layout. if / else-if,
  nested ifs with an empty then, ternaries, gotos and a switch all give the plain layout.
- pf_read_edges: only `lea ebp, [eax]` against `mov ebp, eax`. 15 loop and count forms tried.
- seg2d_closest_point, pf_find_cell_crossing: the binary's frame has an untouched 8-byte slot. Vector, temporary and
  declaration forms do not reproduce it without extra code.
- options_init: the `test al, cl` operand order, and the first options_edit read is forwarded from the source struct.
- hud_drag_icon_begin: the binary does an OR into the loaded word and then masks it. Every store order and every OR
  or type form folds to the constant 13.
- report_memory: the order of the skeleton sum. 15 forms compile identically.
- cmdline_take_argument, ls_ikaemostopint_init: as in the earlier rounds.
- credits_draw_lines: `volatile float alpha` plus `a = alpha; if (!(alpha < 1.0f)) a = 1.0f;` gets close (store
  scheduling and the 0-case block placement left). Not kept.
- saveload_handle_input: its `return 0` for `saveload_selected_row < 0` is merged with the KEY_ESC tail in the
  binary.
- Not attempted beyond a first look: pathfinding's large functions (load_graph, sweep, graph_search, ...),
  hud_load_bitmaps, hud_update_portraits, hud_handle_keys, hud_interact_nearest, hud_hot_buttons_drag,
  hud_portrait_get_color, mp_draw_loading_status, options_load_bitmaps, options_drag_sliders, saveload_scan_saves,
  credits_parse_table, ca_character::add_skeleton, evil_joseph_pick_helper_summon, and the ai.cpp functions the
  earlier notes list.

## Behaviour errors / bug candidates

None found. Both edits keep the behaviour unchanged.

## Changed files (in w6-src)

- game/new_saveload.cpp
- include/DECISIONS.md (D13000-D13002 appended)

mingw `-fsyntax-only` on new_saveload.cpp: 0 errors. No header changed.

---

## Worker 7

### Worker 7 notes (match round m-1020b)

Strict sweep of my 11 units: before 352 match / 1 target / 44 differ; after 357 / 1 / 39. `--check
src/decomp/vc6-matches.txt`: 0 lost. Sweep lists: m-1020b\w7-sweep-after.txt (the before list was lost in the Temp
deletion; it is the committed vc6-matches.txt for these units, which my first sweep reproduced exactly).

**Copy rebuilt:** the Temp deletion (07:21) removed m-1020b including w7-src. I rebuilt w7-src from the repo's
src/decomp (git HEAD ded7d0b, clean) and redid the two edits made before (D13100, D13101). My helper scripts are in
scratchpad\w7tools-1020b (outside m-1020b). After the coordinator's rule I removed every recursive delete from them.

## Per file (match before -> after, of compared)

| file | before | after | new matches |
|---|---|---|---|
| game/combat.cpp | 49/63 | 51/63 | 0x464100 roll_defense, 0x465df0 apply_melee_hit |
| game/unnamed_interface_0x411270.cpp | 62/69 | 63/69 | 0x414270 ui_message_box_draw |
| game/characterinfo.cpp | 64/70 (+1 target) | 65/70 (+1 target) | 0x4450c0 characterinfo_load_party |
| game/new_charselect.cpp | 29/32 | 30/32 | 0x407e10 mp_charselect_drag_scrollbar |
| vsdk/unnamed_poly_split_0x55f110.cpp | 1/5 | 1/5 | |
| game/unnamed_camera_0x4778b0.cpp | 43/47 | 43/47 | |
| vsdk/gr_font.cpp | 15/17 | 15/17 | |
| game/new_buysell.cpp | 40/42 | 40/42 | |
| vsdk/file_packfile.cpp | 19/20 | 19/20 | |
| game/unnamed_dialogue_table_0x4feb50.cpp | 20/21 | 20/21 | |
| game/level_script_ionaext.cpp | 10/11 | 10/11 | |

Confidence raised to exact on the five.

## What worked (D13100-D13104)
- Index loops instead of pointer loops when the binary ends the loop with a signed `jl` against the end address
  (VC6 strength-reduces `for (i..N)` to a pointer compared signed; a pointer loop compares unsigned `jb`).
- Store order inside a loop body decides whether 8966 keeps a zero register (EBX) for the whole function.
- Reading a field through the array (`char_infos[i].x`) instead of the `info` pointer keeps the load below
  stores through `info` (alias barrier) and fixes the biased induction pointer.
- Float clamps at zero written `if (0.0f > x)` where the binary loads the constant first (fld 0 / fcomp x).
- Reducing a float in place (`dodge -= ..; dodge *= ..; d = dodge;`) instead of an if/else into another local.
- A call written in each branch where the binary cross-jumps the tails (apply_melee_hit's spawn_blood).
- Locals computed before a call when the binary loads globals before it (ui_message_box_draw's title).
- Global index read at each use instead of a local copy, and a load placed after a clamp (drag_scrollbar).

## Closer, still differs (kept, same behaviour)
- ui_choice_popup_draw 0x413c70: 38 -> 6 differing lines (`x = ..; x += ui_choice_popup_x;` in the loop). The rest
  is the scheduling of a load before `add esp` after a call; same pattern in ui_choice_popup_show.

## Still differs (D13105)
camera::set_mode (mode kept in ECX, binary reloads), mp_charselect_page_down, ai_choose_special, melee_damage
(frame: COMBAT_SELF_CAST temporaries not sharing slots), resolve_exchange, trigger_special/reflex_ability,
mp_charselect_handle_click (button_def flags shifted unsigned in the binary: shr vs sar; a header type question,
not changed), gr_poly_get_section, ui_message_box_show, the rest as in earlier notes.

## Behaviour errors / bug candidates
None found. All changes are behaviour-identical rewrites.

## Tool note
tools/vc6-side.mjs (through asm.mjs fn) dropped the binary's first instruction for apply_melee_hit (push ecx at
0x465df0), shifting its side-by-side by one line: check the alignment before trusting the star count.

## Files changed (in w7-src)
game/combat.cpp, game/characterinfo.cpp, game/new_charselect.cpp, game/unnamed_interface_0x411270.cpp,
include/DECISIONS.md (D13100-D13105). No header change. mingw -fsyntax-only: 0 errors in the four .cpp.

---

## Worker 8

### m-1020b worker 8 notes

Tree: `m-1020b/w8-src`. VC6 SP5 (cl 12.00.8966), strict (VC6_STRICT=1). Before: `w8-before.txt`, after:
`w8-sweep.txt` (copies in `%TEMP%\w8-keep`). **My 12 files: 209 -> 214 of 254 (+5); none lost**
(`vc6-sweep --check src/decomp/vc6-matches.txt`: 209 matched there, 214 now, 0 lost). No header changed, no flags
changed. mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in the 5 changed .cpp. Decisions D13200-D13204
appended to `w8-src/include/DECISIONS.md`.

**Copy rebuilt:** the scratchpad (with m-1020b and my first w8-src) was wiped at about 07:21 while I was still
measuring, before any edit. I rebuilt `w8-src` from the repo's `src/decomp` (clean at ded7d0b, 536 files, same
count), recreated `w8.txt`, re-ran the baseline sweep (same 209 / 45) and did all edits afterwards. I never ran a
recursive delete. The m-1020b `base` folder was also lost (the merge needs a base: HEAD's src/decomp is it).

## Per file (match before -> after / total)

| file | before | after | won |
|---|---|---|---|
| game/unnamed_ai_move_0x4b1410.cpp | 9 | 10 / 19 | 0x4b4920 ai_state::cast_spell_update |
| game/script_internal.cpp | 20 | 20 / 28 | |
| game/unnamed_dialogue_box_0x4d6660.cpp | 26 | 27 / 32 | 0x4d7190 dialogue_box::add_text |
| game/unnamed_mp_objects_0x47f860.cpp | 26 | 27 / 31 | 0x480530 mp_send_obj_update |
| game/unnamed_tentacle_beast_0x4c6480.cpp | 21 | 21 / 25 | |
| vsdk/unnamed_gr_poly_0x5450c0.cpp | 21 | 22 / 24 | 0x545d70 gr_poly_buffer_flush |
| vsdk/unnamed_vfx_mesh_0x562a30.cpp | 7 | 7 / 9 | |
| vsdk/skeleton.cpp | 26 | 26 / 28 | |
| game/multispell.cpp | 20 | 20 / 22 | |
| vsdk/memory.cpp | 18 | 18 / 19 | |
| game/unnamed_cutscene_script_0x4ed050.cpp | 5 | 5 / 6 | |
| game/level_script_jadetemple.cpp | 10 | 11 / 11 | 0x4f0a40 ls_jadetemple_action |

Confidence raised to exact for the five, with the shape in the comment.

## What worked

- `#pragma inline_depth(0)` where the binary calls an inline out of line. This includes a **constructor iterator**:
  gr_poly_buffer_flush builds its gr_vertex array with `vector constructor iterator` (D13200). It also put the
  clip-vertex helpers out of line in clip_edge / clip_and_draw (closer).
- **Cross-jumped arms need shared variables**: the two begin opcodes read into one function-scope a2 / z, so 8966
  merges their gr_poly_begin calls as the binary does.
- `for (;;) { switch (read()) { ... case END: goto next; } }` instead of `do { op = ...; } while (op != END)`.
- Statement before a declaration: `nv = 0;` before the array; `int summon = ...;` before the profile_label.
- **Where the shared block sits**: `if (lb) { ... } else { fail: return NULL; }`, with a goto from the other path,
  places the one `xor eax, eax` return where the binary has it (add_text). Two written-out failure tails, which 8966
  cross-jumps (cast_spell_update).
- `vector3 v; v = f();` (operator=) reproduces a half-copied return temporary: x / y read through the returned
  pointer, z copied with integer moves (cast_spell_update).
- MP_ADD_FLOAT (a block-scoped float temporary) instead of one function-scope `f`: this sets the frame slots of
  mp_send_obj_update.
- A `static inline` helper with its own buffer for a block repeated in place: the frame order of two 256-byte buffers
  under /O2 depends on which scope owns them, not on declaration order (jade temple).
- Read order of independent packet fields, searched with a permutation script (status_add: integer registers now
  right).

## Closer, still differing (kept, behaviour identical)

- gr_poly_clip_edge 0x545ac0: 3 instructions, the base / index order in `[esi + eax]` (uv lines).
- gr_poly_clip_and_draw 0x545870: only the shrink-wrapped prologue (the binary pushes EBX alone before the
  texture-count test).
- mp_handle_status_add 0x480d70: 20 -> 12 differing instructions (the value's register, the c reload).

## Still differing (tried, D13204)

mp_handle_ai_disable (from in ECX), last_visible_line / both dialogue draws (the ring-index register), steer_jitter
(temporary slots), mem_report_line (8966 normalises `65 - a - b`), cutscene_script_load (EH epilogue placement; goto /
else forms produce the same code), tentacle boss_load (one `mov ecx` scheduled before a push) and ai_frame (exit block
placement; labels / else-wrapping give the same code), script_action_combat_mode (register choice), mp_send_new_lep,
path_closest_point (frame size). Not attempted in depth: skeleton, vfx_mesh, the large script_internal and ai_move
functions, or multispell (register choice).

## Behaviour errors / game-bug candidates

None found. Every change keeps the behaviour. The replay loop exits only on the end opcode, as before. The NULL
return and the failure tails do the same work, and the reads have no side effects. The profile_label is an empty
inline (D6102), so moving the summon lookup before it changes nothing.

## Files changed (in w8-src)

- game/unnamed_ai_move_0x4b1410.cpp
- game/unnamed_dialogue_box_0x4d6660.cpp
- game/unnamed_mp_objects_0x47f860.cpp
- game/level_script_jadetemple.cpp
- vsdk/unnamed_gr_poly_0x5450c0.cpp
- include/DECISIONS.md (D13200-D13204)

---

## Worker 9

### m-1020b worker 9 notes

Tree: `m-1020b/w9-src`. Compiler: VC6 SP5 (cl 12.00.8966), strict. Before: `w9-sweep-before.txt` (my 11 units
plus the two other users of the changed header), after: `w9-sweep-after.txt` (same units) and `w9-sweep-full.txt`
(whole copy). **My 11 units: 230 -> 234 (+4), none lost.** Whole-copy strict sweep: 5259 match,
`--check src/decomp/vc6-matches.txt`: 5255 -> 5259, 0 lost. Decisions D13300-D13304 in `w9-src/include/DECISIONS.md`.

**Copy rebuilt:** at about 07:21 the whole `m-1020b` folder was emptied (the coordinator's notice: another worker's
`rm -rf` with an empty variable). My `w9-src` and `w9.txt` were rebuilt from the repo's `src/decomp` (HEAD ded7d0b,
clean) and the file list rewritten; the only edit lost was an experiment, which was redone (D13300). The before
sweep was run again on the rebuilt copy. My scratch tools (`w9tools/`) contain no recursive deletes any more.

## Per file (match before -> after / total)

| file | before | after | won |
|---|---|---|---|
| game/unnamed_avoidance_0x4a6710.cpp | 49 | 50 / 67 | 0x4a6d60 avoid_prim_from_segment |
| game/unnamed_ai_think_0x4b5140.cpp | 23 | 25 / 29 | 0x4b53c0 ai_score_target, 0x4b71c0 ai_state::think_party_update |
| game/level_script_sewers.cpp | 18 | 19 / 19 | 0x4ee3c0 ls_sewer_create_avoid_spline |
| game/unnamed_pfgeom_0x4a4ee0.cpp | 19 | 19 / 24 | |
| game/unnamed_lobj_raycast_0x4ae890.cpp | 3 | 3 / 7 | (two closer) |
| vsdk/unnamed_key_0x5093a0.cpp | 23 | 23 / 26 | |
| vsdk/unnamed_ui_slider_0x55e040.cpp | 24 | 24 / 26 | |
| vsdk/unnamed_checkbox_0x553ba0.cpp | 10 | 10 / 12 | |
| game/level_script_worldmap.cpp | 21 | 21 / 23 | |
| vsdk/stringpool.cpp | 15 | 15 / 16 | |
| game/unnamed_cutscene_0x434900.cpp | 25 | 25 / 26 | |
| (header users) game/level_script_khosani_lab.cpp | 41 | 41 / 45 | |
| (header users) game/script_internal.cpp | 20 | 20 / 28 | |

All four won functions are `Confidence: exact`.

## What worked

- **A constructor-less type for an array the binary never constructs** (D13300, header change): no `vector
  constructor iterator` in the binary means the element type has no default constructor. With a POD end-point pair
  the sewer spline's corners also stay in memory as in the binary; then the corners set in their use order.
- **Struct fields read through the pointer each time instead of local copies** (D13301): ai_score_target's
  q->self / q->candidate (the binary reloads them after every call; the locals made 8966 keep them in registers).
- **The inline dot (`a.dot(b)`) instead of a hand-written dot** (D13301, D13303): it gives the binary's z, y, x order
  where every hand-written order gives x first. Also `vector3 d; d = a - b;` (temporary + copy).
- **Reusing a dead vector local for a later value** (D13303): the binary's frame shares the slot; a new local adds 12.
- **An inline setter with both values as arguments** (D13302): arguments evaluated right to left, stored in place
  (x loaded first, then y, fchs, two fstp). A file-local helper `vec2_set` (maybe the original's vector2::set).
- Scoring tool: `w9tools/greedy.mjs` tries slot alternatives greedily and scores each by an LCS over normalised
  full instructions (`fullnorm.mjs`: listing stack symbols resolved to esp offsets, globals / jumps masked); the
  first-difference offset alone could not rank most variants.

## Closer, still differing (kept, behaviour identical; D13304)

- avoid_prim_overlap: `fabs(dot) > sum` for all four tests (the binary computes the projection first) and
  `b->axis_a.dot(*u)` for ku: mnemonic LCS distance 42 -> 3; one dot's mixed operand order left.
- avoid_box_box_overlap: the same comparison swap, mnemonics now identical; a / b in swapped registers
  (EDX / ECX) and k slots left.
- ai_state::pick_attack_target: the distance in a float local gives the binary's fxch; the -1 constant kept in
  EBP by the binary remains.
- lobj_ray_test_faces / lobj_sphere_test_faces: `ray->local_dir.dot(face->normal)` (first difference moves to the
  third term; the binary has face-first, face-first, dir-first).

## Still differing (tried, see D13304)

avoid_spline_point_get / avoid_spline_get (block order), worldmap stage9 (block placement) and stage15 (frame
slots), every hand-written dot order in prim_overlap / box_box_overlap (8966 emits one order), pf_grid_cells_along_
segment (frame 0x34 vs 0x3c, not attempted further), lobj_raycast_bsp (the binary has an 8-aligned EBP frame:
probably a double / __int64 local in the original), avoid_add_primitive (1022 vs 1253 bytes), the large avoidance
functions (frames), cutscene_parse_table, party_spread_attackers, key_*, StringPool::Add, ui_* (register choice;
earlier rounds' forms not repeated).

## Behaviour errors / game bugs

None found. All changes keep behaviour: comparisons rewritten as their mirror (`a < b` <-> `b > a`, also false for
NaN), dot products as the inline member (the same products, added in another order: the binary's order), the POD
type has vector2's layout.

## Header change

- include/engine/ai/unnamed_avoidance_0x4a6710.h: `struct avoid_spline_end { float x, y; }`; avoid_spline_def's
  p0 / p1 use it (D13300). Users: avoidance (cast at the avoid_spline_point_create call), sewers, khosani_lab (cast
  copy), script_internal (component access only, unchanged). mingw `-fsyntax-only` over all .cpp: only
  unnamed_decals_0x4bbe80.cpp fails, as in the repo. Full strict sweep: 0 lost.

## Note for other workers / the merge

script_build_avoid_spline (0x4ddc60, game/script_internal.cpp, not mine) has no iterator in the binary either;
with D13300 its frame should be closer.

## Files changed (in w9-src)

- include/engine/ai/unnamed_avoidance_0x4a6710.h
- game/unnamed_avoidance_0x4a6710.cpp
- game/unnamed_ai_think_0x4b5140.cpp
- game/unnamed_lobj_raycast_0x4ae890.cpp
- game/level_script_sewers.cpp
- game/level_script_khosani_lab.cpp
- include/DECISIONS.md (D13300-D13304)

---

## Worker 10

### Worker 10 notes (match round m-1020b)

Strict VC6 sweep (cl 12.00.8966, `VC6_STRICT=1`) of my 12 units: **290 -> 295 matches of 335, 0 lost**
(`w10-before.txt` = these units' lines of src/decomp/vc6-matches.txt, `w10-sweep.txt`; `vc6-sweep --check
src/decomp/vc6-matches.txt`: 290 there, 295 now, 0 lost). mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors
in the three changed files. No header changed. Decisions D13400-D13404 appended to w10-src/include/DECISIONS.md.

**Copy rebuilt:** the m-1020b folder was deleted while I worked (07:21). I rebuilt w10-src from the repo's
src/decomp (HEAD ded7d0b, clean) and redid the parse.cpp edits (the only ones made by then); everything after was
done on the rebuilt copy. My helper scripts now live outside m-1020b and no longer delete anything (an early version
of my compare wrapper removed its vc6cmp-* temp folder by a variable path, guarded by a non-empty check; removed
after the coordinator's rule).

## Per file (before -> after / functions)

| File | Before | After | New matches |
|---|---|---|---|
| vsdk/parse.cpp | 46 | 48 / 51 | 0x517180 file_write_text_block, 0x517220 file_blast_into_buffer |
| game/level_script_khosani_lab.cpp | 41 | 43 / 45 | 0x4f8190 choose_turn, 0x4f8400 emit_segment |
| game/multi_door.cpp | 5 | 6 / 6 | 0x4aaa10 door_pick_by_mesh |
| game/unnamed_fx_0x489c40.cpp | 48 | 48 / 57 | |
| vsdk/unnamed_net_0x517590.cpp | 23 | 23 / 31 | |
| vsdk/effect_mem.cpp | 33 | 33 / 39 | |
| vsdk/unnamed_floatmath_0x542500.cpp | 16 | 16 / 20 | |
| vsdk/unnamed_ui_button_0x526940.cpp | 22 | 22 / 24 | |
| vsdk/unnamed_clip_0x5590b0.cpp | 15 | 15 / 17 | |
| game/items.cpp | 29 | 29 / 31 | (not attempted: 4.5 KB / 7.5 KB functions) |
| vsdk/ui_container.cpp | 9 | 9 / 10 | (not attempted) |
| game/unnamed_alt_textures_0x4aac10.cpp | 3 | 3 / 4 | (not attempted: prologue push shrink-wrap, as last round) |

## What worked

- `while (i--)` post-decrement loop with one index serving as search index and advance (D13400).
- Statement order: global stores written before an inline strcpy that the compiler then schedules after it (D13401).
- **Struct copy to stop 8966's sum reassociation** (D13402): `ls_khosanilab_cell n = *cell; n.x += d.x; n.y += d.y;`
  then `grid[n.x][n.y]` keeps x and y as separate sums before `5 x + y`. Int locals do not (the 3-term y sum is
  folded into the index). Likely useful for other 2-D grid indexes with offsets.
- **Two-argument setter for float copies** (D13403, extends D12100): `v2_set(&g, 2.2f, p0->y)` gives the binary's
  `fld src ... fstp dst` copy (a plain `g.y = p0->y` is an integer move), which in turn restores the binary's
  cross-jumped tails.
- **`(double)` operand to get `fld mem; fcompp`** (D13404): comparing `sum >= (double)d->lock` loads the memory
  operand onto the FPU stack (binary) where a float compare uses `fcomp mem`. Also: bools scoped to the block that
  uses them let `open` share the door pointer's slot (frame 0x10c -> 0x108).

## Still differing (tried)

- fast_inv_sqrt / fast_sqrt / fast_sqrt_refined: EAX/ECX swapped (5 more forms: table first, local copy, two
  statements, int casts) - as D8405.
- float_format_buf: not attempted beyond a look (digit[] init from .bss, rem tested from memory).
- ParseInt / ParseUInt / ParseString: `this` in EDI vs ESI (7 declaration orders / types tried).
- nw_init_networking: the binary places the "none" return inline and the 56k / fast tests after the body
  (cross-jumping back); a `none:` label or nested ifs lose the zero register (`push 0` instead of `push edi`).
- nw_buffer_pop: the binary loads addr.ip before the `*size` store; 7 store orders / copies tried, none exact.
- corner_pp: 7 more forms for the first y sum (operator+, t + *out, named y, cell->y written out) - as D12100.
- trace_beam: frame 0x4c vs 0x48 because back_dir has its own slot (the binary keeps it in start's argument
  slot; `first` is in BL in both). A do-while instead of the `first` flag does not change the frame.
- fx_create_spell_effect_facing: right.x `fld 0; fld dir.z; fsubr` - 5 forms (components written out, temp up,
  matrix from the cross directly, up.set) none.
- rfx_preload: total = buf_size + offset loads the two in swapped registers (declaration orders, operand orders,
  statement order: no change).
- effect_base_read: the binary pushes ESI before the range test (single exit); early return, block-scoped File
  give the same shrink-wrapped push.
- gr_classify_sphere_view: the binary keeps z * -0.707 on the FPU stack (two copies) and shares one `return -1`
  block entered at different pop depths; nested success-first ifs get the start right but spill nz (8 forms).
- ui_button::draw_arrow: the binary tests `(flags >> 7) & 1` through a shift (unsigned) and has one more stack
  slot; the unsigned cast alone changes nothing.
- fx_update_missile and the other fx frame-size differences: the binary calls vector3 operator+ / operator=
  (returning by value) / copy constructor out of line (inline budget); not attempted further.

## Behaviour errors / bug candidates

None found. Every change is behaviour-identical: the same search and advance (D13400), the same stores
(D13401), the same grid cells (D13402), the same bend points (D13403), and the same lock comparison (x87 compares
at full precision in both forms, D13404).

## Files changed (in w10-src)

vsdk/parse.cpp, game/level_script_khosani_lab.cpp, game/multi_door.cpp, include/DECISIONS.md.

---

## Worker 11

### Worker 11 (m-1020b): byte-match notes

Strict sweep of the 12 units: 303 -> 307 matches, 0 lost (`vc6-sweep --check src/decomp/vc6-matches.txt`).
After-list: m-1020b\w11-after.txt. The before-list (w11-before.txt) was lost when the Temp folder was wiped
(see Incident); its totals were 303 match / 45 differ, which equal vc6-matches.txt for these units.

| File | Before | After / total | New matches |
|---|---|---|---|
| game/spells.cpp | 30 | 32 / 39 | 0x47e330 spell_probe_ground_point, 0x47e830 spell_fire_missile |
| game/unnamed_shadows_0x4a37d0.cpp | 7 | 8 / 13 | 0x4a4360 shadows_render |
| game/multisave.cpp | 4 | 5 / 8 | 0x46cc80 mp_player_file_create |
| vsdk/unnamed_vector_0x523f30.cpp | 43 | 43 / 51 | |
| vsdk/unnamed_localize_0x503f60.cpp | 23 | 23 / 28 | |
| vsdk/unnamed_ds_0x55bf90.cpp | 56 | 56 / 60 | |
| vsdk/unnamed_poly_clip_0x5519c0.cpp | 6 | 6 / 8 | |
| vsdk/unnamed_gadget_0x52bfe0.cpp | 46 | 46 / 48 | |
| game/conversation.cpp | 25 | 25 / 27 | |
| vsdk/unnamed_anim_track_0x550660.cpp | 12 | 12 / 13 | |
| game/objects.cpp | 39 | 39 / 40 | |
| game/multicontainer.cpp | 12 | 12 / 13 | |

## What worked (DECISIONS D13500-D13503)

- shadows_render: both loop counts read into locals once (num_blob_shadows; the 3-bit lobj_count, unsigned, so
  `jbe`), an index loop with `s = &blob_shadows[i]`, and `m >>= 1` in the for-increment after `bit++`.
- spell_fire_missile: early `return h` when missile == -1 (one h register, one epilogue).
- spell_probe_ground_point: `v = pos.unrotated(..); v += fx->pos;` and `lobj_ray ray;` declared at its set-up.
- mp_player_file_create: `int err = file_make_path_dirs(..); if (err)` (cmp with the zero register) plus
  `prog = NULL;` before `num_prog = 0;` in the default case. Both needed.

## Closer, still differing (kept)

- lobj_raycast_draw_lists: stage through the `blocked` parameter (`*(char *)blocked`), `n` block-scoped. Left:
  frame 0x10 vs 0xc (the binary puts `first` in hit's argument slot) and ESI / EDI swapped.
- mp_player_file_load: the unequip result shares old1 (the binary's slot). Left: ECX loaded before the species
  argument of the two savegame_state calls at the end.

## Still differs (tried; D13504)

spell_roll_amount (loop counter `dec / lea esi, [eax+1]`, ~20 shapes), mp_player_file_read_summary (EDX vs ECX),
mp_player_file_write (register rotation in the inventory loop), localize_load_names (case-1 block placement),
localize_remap_charset (loop test on *text first), shadow_project_face (needs `&(w + l->pos)` temporary; a
`world +=` form gets frame 0x1c0 of 0x1b8, reverted), polygon_clip_2d (n in EBX), mp_handle_item_drop (matrix from
argument temporaries gets the vector stores right but the rest differs; not applied), vector3::random_* (fmulp),
conversation::restore_state (register swap), conversation::clear_speaker (needs the D10904 (1) member declaration).
Not attempted: the large spells / localize / ds functions, obstacles_debug_render, shadow_record_add,
shadow_transform_to_lobj, item_obj_spawn (/Od temporaries), calc_rot_tangents, ui_gadget send_event / set_parent.

## Behaviour errors / bug candidates

None found. Every change keeps the behaviour (the counts read once are not changed inside the loops).

## Checks

- mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in the three changed files. No header changed.
- Strict sweep after: 307 match, 0 wrong target, 0 lost.

## Changed files (in w11-src)

- game/unnamed_shadows_0x4a37d0.cpp
- game/spells.cpp
- game/multisave.cpp
- include/DECISIONS.md (D13500-D13504)

## Incident

At about 07:21 the m-1020b folder was emptied (another worker's `rm -rf` on an empty variable). w11-src was rebuilt
from the repo's src/decomp (HEAD ded7d0b, clean), and the one edit made by then (shadows_render) was redone; all
later work was on the rebuilt copy. Checked afterwards: 536 files, same as src/decomp; only my three .cpp files
and DECISIONS.md differ. Helper scripts live in scratchpad\w11-tools (outside m-1020b).

---

## Worker 12

### m-1020b worker 12 notes

Strict sweep of the 12 units: 241 -> 244 matches (of 286), none lost (`--check src/decomp/vc6-matches.txt`: 0 lost).
Lists: w12-sweep.txt (after). Decisions D13600-D13606 in w12-src/include/DECISIONS.md.

## Incident (redone work)

Around 07:21 the whole m-1020b folder (every worker copy, base, wN.txt) vanished while I was working (the
coordinator later traced it to another worker's `rm -rf "$TEMP/$tmp"` with an empty variable). My copy was
rebuilt from the repo's src/decomp (HEAD ded7d0b, clean) and w12.txt recreated; the one edit made before
(d3d_enum_texture_format_callback) was redone and re-verified. My helper scripts no longer delete anything (they
had a guarded `Remove-Item` of vc6-compare's own `--keep` temp folder; removed after the coordinator's rule).
`diff -rq` against the repo shows only my three changed files.

## Per file (strict matches before -> after / compared)

| File | Before | After | Of |
|---|---|---|---|
| vsdk/unnamed_vfx_emitter_0x55ff00.cpp | 5 | 6 | 14 |
| vsdk/gr_direct3d.cpp | 42 | 43 | 50 |
| game/new_automap.cpp | 27 | 28 | 32 |
| game/gametarget.cpp | 15 | 15 | 22 |
| vsdk/unnamed_ca_mesh_0x50af80.cpp | 8 | 8 | 12 |
| vsdk/targautils.cpp | 18 | 18 | 21 |
| game/multi_chattracker.cpp | 17 | 17 | 20 |
| game/unnamed_cutscene_camera_0x493e50.cpp | 10 | 10 | 12 |
| vsdk/unnamed_vfx_material_0x564d70.cpp | 14 | 14 | 15 |
| vsdk/unnamed_bezier_0x558a30.cpp | 14 | 14 | 15 |
| game/new_status.cpp | 50 | 50 | 51 |
| game/multitrade.cpp | 21 | 21 | 22 |

New matches: 0x53a1d0 d3d_enum_texture_format_callback, 0x562840 fx_recb0::get_billboard_axes,
0x401ef0 automap_load_bitmaps (Confidence raised to exact).

## What worked

- A repeated 3-term sum instead of a local (`if (r + g + b > x) x = r + g + b;`): 8966 normalises the local's sum
  to lea/add, the repeated expression gives the binary's add/lea (D13600).
- Mask-count macro shapes: clear the count after the first loop, else-branch for a zero mask; variables pre-set
  to 0 without else (D13600).
- A float local for the first term of a 2-term float sum keeps the source order where 8966 otherwise reorders
  the operands (caught only by the strict target check: the globals were swapped, the bytes equal) (D13603).
- A dead float local (`float m = max(..)`, `float rate = get_rate(..)`) reproduces the binary's
  `fmul/fadd st, st(1)` ... `fstp st(0)` pattern (D13603, D13604).
- `&static_cast<const vector3 &>(a + b)` for the binary's address-of-a-temporary argument (D13602).
- Ternary polarity: `x == 0 ? A : B` where the binary loads A and skips B on zero (D13605).
- Element pointers in button loops (automap, as journal / buysell).

## Closer, still differing (kept; behaviour identical)

- fx_recb0::spawn 0x55ff30: only the prev store is scheduled before the fstp of the ceil argument (D13604).
- fx_recb0::apply_forces 0x5605f0: the 0.0625 factor loaded from .rdata 0x58232c into ECX (D13602).
- fx_recb0::render_d3d 0x561900: lock flag branch now matches; later: end store after the loop test, fild/fdivp
  for the aspect, registers (D13605).

## Still differing, tried (D13606)

init_particle, fx_recb0::render, get_transform, pcx_read_bitmap, d3d_compute_pixel_format_shifts, gr_d3d_bitmap,
gr_d3d_set_state, gr_d3d_draw_primitive, automap_draw_map. Not attempted (prior rounds tried, or very large):
the ca_mesh renderers, chat_process_irc_message, world_mouse_frame / hover_target, move_particles,
gr_d3d_init_device, status_handle_click, and the small ones the 10-19 / 10-20 notes list as tried.

## Behaviour errors / bug candidates

- get_billboard_axes: the source scaled the overwritten first tip by `size`; the binary uses `fade` (0x562947).
  The store is dead, so no behaviour change, but the source was wrong (D13601). No game bug.

## Files changed (in w12-src)

- vsdk/gr_direct3d.cpp
- vsdk/unnamed_vfx_emitter_0x55ff00.cpp
- game/new_automap.cpp
- include/DECISIONS.md (D13600-D13606 appended)

mingw `-fsyntax-only -std=c++98` on the three .cpp: 0 errors. No header changed.

---

## Worker 13

### m-1020b worker 13 notes

## Incident (read first)

Early in the session (about 07:20-07:21) a compare helper script of mine ran `rm -rf "$TEMP/$tmp"` with `$tmp`
possibly empty. The likely effect was a recursive delete of `%TEMP%\`. The evidence:
the session's task output file vanished during that command, every `vc6cmp-*` folder older than 07:20 is gone,
and the whole `scratchpad\m-1020b` folder disappeared (all worker copies, `base\`, `differ-by-file.txt`, the
`w*.txt` lists). Old files that were locked or in use in Temp survived. The other workers recreated their copies
at 07:21-07:22. Edits they made before that may have been lost. I recreated `w13-src` from the repo's
`src/decomp` (clean HEAD ded7d0b) and `w13.txt` from the task text. **`m-1020b\base` and `differ-by-file.txt`
were not recreated.** For the merge, regenerate base from git HEAD's `src/decomp`. The helper now refuses to
delete without a `vc6cmp-` name (w13-tools/cmp.sh). The repository itself was not touched.

## Per file (strict, matches before -> after / total)

| File | Before | After | New match |
|---|---|---|---|
| vsdk/unnamed_listbox_0x52ca60.cpp | 60 | 61 / 69 | 0x52e920 ui_listbox::get_vscroll_fraction |
| vsdk/gr_glide.cpp | 53 | 53 / 61 | (tcache_set and alloc closer) |
| game/conversationctl.cpp | 20 | 20 / 27 | |
| game/unnamed_ai_groups_0x492b00.cpp | 13 | 13 / 18 | |
| game/unnamed_worldmap_automap_0x42f1e0.cpp | 31 | 31 / 35 | |
| vsdk/unnamed_d3d_tcache_0x54bb30.cpp | 11 | 11 / 14 | |
| game/gameplay.cpp | 36 | 36 / 39 | |
| game/unnamed_editor_dialogs_0x430bc0.cpp | 3 | 3 / 5 | |
| vsdk/unnamed_vfile_rw_0x525720.cpp | 50 | 50 / 51 | |
| vsdk/unnamed_crc32_0x5520d0.cpp | 3 | 3 / 4 | |
| game/new_skill.cpp | 38 | 38 / 39 | |
| game/new_assess.cpp | 17 | 17 / 18 | |

Sweep: 335 -> 336 match of 380, 0 lost (`--check` against w13-before.txt). Lists: w13-before.txt, w13-after.txt.

## What worked (D13700, D13701)

- get_vscroll_fraction: lo through a float temporary assigned after `last = entries.prev` is read, and
  the font height in a local before the multiply (`imul ecx, eax` operand order). Exact.
- gr_glide_tcache_set: no `slot` local (`glide_tmus[tmu].is_bound` / `.load_section` in place), and
  `switch (entry->sections[i].format)`. Every instruction now matches. Only the stack slot of `data` differs:
  Sum.exe keeps it in the dead `tmu` argument slot (8966 argument-slot reuse).
- glide_record::alloc: a `bank` local (shift back, not mask), `used_tail = used_head`, and `end` reused as
  the loop limit. One instruction still differs: Sum.exe adds the limit after the first `next >= 0` test.

## Still differs (tried, D13702)

- game_render_frame: dword load then byte `not/shr` (8966 narrowing). Every byte form loads a byte.
- character_use_skill: get_skill before the inline fsqrt, with the dead root store in the `target` argument slot.
- editor_dialog_handle_event: a callback local plus a goto gives the tail exactly, but not the layout of the
  button 0 / 1 blocks (cross-jumping). Not kept.
- Register choice or scheduling only: Crc32::update, wrap_text, scroll_page_up, set_vscroll_fraction loop,
  ai_group_think_style1 / style2, line_talker (eax vs ecx changes the size of the cur_character load),
  load_section, worldmap_marker_clamp_to_edge, gr_glide_create_texture.
- Block placement: state_game_enter's default case is placed after the load-failure block in Sum.exe (no case
  order changes it).
- Frame layout (8966 slot use): ai_group_think_caster (0x80 against 0x6c), the ui_listbox constructor (the head's
  constructor runs inline in Sum.exe although the unit is /Ob0; `__forceinline` does not change it, and it was
  reverted).

## Behaviour errors / bug candidates

None found.

## Changed files (in w13-src)

- vsdk/unnamed_listbox_0x52ca60.cpp (get_vscroll_fraction; Confidence exact)
- vsdk/gr_glide.cpp (gr_glide_tcache_set, glide_record::alloc; comments; Confidence stays likely)
- include/DECISIONS.md (D13700-D13702)

No header change. mingw `-fsyntax-only` is clean for both .cpp files.

---

## Worker 14

### m-1020b worker 14 notes

Strict VC6 8966 sweep (`VC6_STRICT=1`) of my 8 units. Before (repo vc6-matches.txt, HEAD ded7d0b): 224 of 269
match. After (`w14-sweep.txt`): **227 of 269; none lost** (`vc6-sweep --check src/decomp/vc6-matches.txt` exits 0;
`w14-before.txt` is the before list for these units). mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in
both changed files. No header and no flags change. Decisions D13800-D13804 in `w14-src/include/DECISIONS.md`.

Redone work: the Temp clean-up deleted m-1020b (and my first copy with the find_flag_level edit) during the round.
I rebuilt `w14-src` from the repo's src/decomp (HEAD ded7d0b; 536 files, only my two files differ) and redid the
edit; everything else was done on the rebuilt copy.

## Per file (before -> after / total)

| file | before | after | won |
|---|---|---|---|
| game/level_scripts.cpp | 90 | 92 / 124 | 0x4d0240 find_flag_level, 0x4d4a50 savegame_location_allowed |
| vsdk/unnamed_ui_static_0x527e20.cpp | 9 | 10 / 10 | 0x5280e0 ui_static::draw |
| vsdk/character_instance.cpp | 50 | 50 / 53 | |
| vsdk/unnamed_linefile_0x558340.cpp | 12 | 12 / 14 | |
| game/unnamed_effectstream_0x440400.cpp | 12 | 12 / 14 | |
| vsdk/unnamed_eax_0x521e40.cpp | 13 | 13 / 14 | |
| game/new_pickup.cpp | 19 | 19 / 20 | |
| game/new_gamecreate.cpp | 19 | 19 / 20 | |

The three winners are now `Confidence: exact`.

## What worked

- Search loop written out with the found code inside the outer loop (find_flag_level, D13800): the index is
  assigned only on a hit, so no `or eax, -1` and no re-test.
- Duplicated tails written out (ui_static::draw, D13801): the whole text code in both branches; 8966 merges it
  from the shadow call on exactly like the binary. `c` set in each branch.
- Result variable + break in every case, explicit `default` (savegame_location_allowed, D13802): stops 8966
  cross-jumping identical cases (it duplicates the return per case instead). This may help other switch
  functions where the binary has per-case epilogues with identical bodies.
- Tool: `tryx.mjs` (in %TEMP%\w14-backup) prints each variant's compare line
  (first difference with operands), which shows operand-order and SIB differences vc6-side hides.

## Closer, still differing (kept, behaviour unchanged; D13803)

level_item_add (only the [edx+ecx] SIB order), level_item_remove and read_level_items (p's init placement and
register), the four speaker / topic bit functions (save info through the macro; get_speaker_bit then differs at
the search-loop threading), level_add_navpoint (only the z fld / fadd operand order), conversation_execute_action
(cases in the binary's order; EBX pushed inside case 1 in the binary).

## Still differs (D13804)

eax_init, effect_stream_start, effect_stream_finish_entry, gamecreate_handle_click, pickup_draw_item_list,
character_instance build_bone_matrices / increment_tick / sum_root_velocity, LineFile detect_eol / read_line,
has_conversation_data, hot_button_get, get_int, level_item_has, two_stage_set / get, write_party_state,
init_conversation_slot, the read_* / write_* savegame functions, events / script_filenames parse tables,
level_format_display_name, layout_conversation, automap_label_remove, hot_button_set, write_header.

## Behaviour errors / bug candidates

None found; no behaviour change made.

## Changed files (in w14-src)

- game/level_scripts.cpp
- vsdk/unnamed_ui_static_0x527e20.cpp
- include/DECISIONS.md (D13800-D13804)

---

## Worker 15

### m-1020b worker 15

Strict sweep of the 12 units: 335 -> 344 matches (of 380), none lost (`--check src/decomp/vc6-matches.txt`).
Lists: `w15-before.txt` (repo tree), `w15-sweep.txt` (my copy).

**Copy rebuilt:** the scratchpad was wiped at about 07:21 (another worker's `rm -rf`). I rebuilt `w15-src` from
the repo's src/decomp (git HEAD ded7d0b, clean) and redid my only edit at that point (matrix::determinant). Every
later edit was made on the rebuilt copy.

| File | Before | After | Of | New matches |
|---|---|---|---|---|
| vsdk/unnamed_quaternion_0x5225b0.cpp | 21 | 21 | 32 | |
| game/new_inventory.cpp | 61 | 62 | 69 | 0x414de0 inventory_load_bitmaps |
| game/s3d.cpp | 28 | 30 | 34 | 0x46e1b0 level_load_s3d, 0x46e910 s3d_read_geometry |
| game/levelrender.cpp | 7 | 7 | 12 | |
| game/unnamed_goals_doors_0x475180.cpp | 63 | 63 | 67 | |
| vsdk/matrix.cpp | 43 | 45 | 46 | 0x5106a0 matrix::determinant, 0x511920 matrix43_interpolate_quat |
| vsdk/unnamed_http_0x55a8e0.cpp | 13 | 14 | 15 | 0x55abd0 http_get::send_get |
| game/unnamed_ghost_rider_0x4b9e70.cpp | 36 | 37 | 38 | 0x4ba0d0 ghost_find_floor_below |
| vsdk/unnamed_ui_manager_0x52f570.cpp | 11 | 12 | 12 | 0x530330 ui_process |
| vsdk/unnamed_os_0x530b90.cpp | 22 | 23 | 23 | 0x530ff0 os_wndproc |
| game/new_journal.cpp | 26 | 26 | 27 | (closer, D13909) |
| game/new_int_multi_update.cpp | 4 | 4 | 5 | (closer, D13903) |

All nine new matches are `Confidence: exact` with the form and decision in the comment (D13900-D13908).

## What worked

- **A local per product** (D13900): 8966 sorts the terms of a hand-written float sum; one local per product keeps
  the source order (determinant, the position lerp of interpolate_quat).
- **A matched sibling's shape** (D13901): inventory_load_bitmaps with journal_load_bitmaps' `b` / `d` pointers.
- **Chained assignment** for flags cleared together (`restored = any_click = hovered = false;`, D13902), the
  order of the chain sets the stack slots.
- **Loop-variable update before a test** (`win = prev;` before `if (any_click)`, D13902).
- **Pointer local versus indexing**, both ways: a pointer helped level_load_s3d (D13904); dropping the face
  pointer fixed s3d_read_geometry (D13906).
- **Keeping a duplicated tail apart** by moving it inside a `for (;;)` (send_get, D13905).
- **Separate case labels**: one case group with several labels makes 8966 test consecutive values as a range,
  which changes the compare tree (os_wndproc, D13907); also `break` to one `return 0` with a `default:`.
- **Statement-order search** with a scorer (ghost_find_floor_below, D13908).

Tooling (scratch, `w15-work/`): `try.mjs` is vc6-try with a score (an LCS distance over normalised
instructions, against the full asm.mjs listing with `TRY_FULL=1`, since vc6-side cuts the original at about 400
lines and at jump tables); `fd.sh` prints that normalised diff. The score finds near misses that the
first-difference report hides. It would be worth turning into a repo tool.

## Still differs (tried)

- matrix43::transpose_in_place 0x511bb0: the binary stores a dead temporary four times; eight swap forms all
  compile to the same rotations without the stores.
- quaternion::to_matrix / _b: `w * z` and `w * x` factor order; source order of the factors is ignored (4 forms).
  quaternion::multiply: r.w's fstp after the copy-constructor push (6 statement orders / forms). cquat::log: the
  products `(short)(f * s)` normalised (8 orders). Not tried further: exp / log_in_place / exp_in_place, squad,
  cquat::multiply, quaternion::quaternion.
- inventory_identify_item / destroy_selected_item / equip_item: only `and edx, eax` vs `and eax, edx` in the
  clamp; 48 more min / clamp forms (6 min shapes x 8 clamp shapes) all give the same code.
- inventory_popup_result: key / zero register swap (ESI/EDI); 30+ forms (case tails as break or written out,
  inline helper, result local, switch on the negative codes).
- inventory_use_selected_item: self_only first for the list item (as the equipped path) is closer (74 -> 50),
  not kept; the list `(flags & 4) || (flags & 2)` stays `test [mem], 6` and the equipped path's two identical
  begin blocks are still cross-jumped (12 forms).
- inventory_drag_to_equip: ESI/EDI swap and the case 6/7 shared tail (7 forms). inventory_handle_click: not tried.
- s3d_read_door: with an inline sound_ref clear helper (two calls) and the order invalidate / lock / state / 5c /
  floor point, only `mov ecx, edi` is one block late (LCS 2); 360 orders / forms tried, not kept.
- s3d_read_mesh_ref (hi as int, box pointers), s3d_read_color_record (helpers, macro, volatile), s3d_read_meshes
  (orient through `mesh->orient.` instead of `m` is closer, 182 -> 160, not kept; the dot products are normalised).
- version_check_run: closer (D13903); the binary's tail merging is still different.
- journal_handle_click: closer (D13909); font_h's register.
- ghost_anim_remap_in (4 forms), matrix3_pack_orientation (6 declaration forms; the binary puts `a` in the dead
  argument slots), door_find_floor_point (7 forms), http_async_gethostbyname (4 forms), matrix3_unpack (not tried),
  levelrender (not tried: the three draw lists are 1,300-1,700 instructions).

## Behaviour errors / bug candidates

None found. All changes keep the behaviour (re-orderings of independent stores, pointer locals, the same
switch / loop logic). matrix43::transpose_in_place's game bug note stands.

## Changed files (my copy)

- vsdk/matrix.cpp, vsdk/unnamed_ui_manager_0x52f570.cpp, vsdk/unnamed_http_0x55a8e0.cpp, vsdk/unnamed_os_0x530b90.cpp
- game/new_inventory.cpp, game/s3d.cpp, game/unnamed_ghost_rider_0x4b9e70.cpp, game/new_journal.cpp,
  game/new_int_multi_update.cpp
- include/DECISIONS.md (D13900-D13909 appended). No other header changed.
- mingw `-fsyntax-only -std=c++98`: 0 errors in each changed .cpp.
