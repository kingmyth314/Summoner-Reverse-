# Byte-match round 2026-11-10, worker 6 (D26900-D26999)

Batch: 39 differing functions in 16 units (avoidance, fx, ai_think, gr_opengl, script_internal, vif, decals,
linefile, characterinfo, khosani_lab, new_journal, new_status, lobj_raycast, swi, bezier, localize).
Distances are `vc6-compare --score` under VC6_STRICT=1 (Observed). Helper: a local variant runner (vc6-try with the
--score distance and an aligned listing per variant), not committed.

Result: 2 new matches, 1 closer; the strict sweep over the changed units reports 0 lost (159 -> 161 there).

## New matches and closer functions

| file | address | name | D | what did it | distance before / after |
|---|---|---|---|---|---|
| game/unnamed_avoidance_0x4a6710.cpp | 0x4a8680 | avoid_seglist_add_segment | D26900 | corner searches as guarded do / while loops through a corner pointer; nc / ns read before the -1 stores; `p = &corners[nc].pos; ia = nc++;`; wall corners as `corners[nc - 1]` after `idx = nc++`; a block-scope `idx` in each wall block | 262 / match |
| game/unnamed_avoidance_0x4a6710.cpp | 0x4a8b80 | avoid_merge_shared_segments | D26901 | c / cc / l / n block-scoped per branch; the k entry and k's list fetched into locals before the call; `n = index - 1` local; the side test on the call result; `done[40]`, `refs[40]` | 128 / match |
| game/new_status.cpp | 0x42bb00 | status_handle_click | D26902 | the wheel delta's dx / dy / dz at function scope (frame 0x18 -> 0x1c) | 237 / 191 |

**D26900 (game/unnamed_avoidance_0x4a6710.cpp). avoid_seglist_add_segment 0x4a8680 is exact.** Five changes,
each found by the aligned listing (Observed: each moved the distance, 262 -> 246 -> 129 -> 101 -> 8 -> 4 -> 0):
1. The two corner searches compare through `p = &sl->corners[i].pos` (the binary's loop pointer is at `pos`, +0x270,
   not at `pos.y`), and the new corners are stored through `p` too (246).
2. In the wall blocks the new corner is addressed as `sl->corners[nc - 1]` after `idx = nc++`: the binary computes
   the address from the incremented count (`lea ecx, [eax + 0x22c]`, 0x270 - 0x44). This also gives the binary's
   frame, 0x1c (129).
3. `nc = sl->num_corners; ns = sl->num_segs;` before `ia = -1; ib = -1;`: ib then takes sl's dead argument slot, as
   in the binary (101).
4. `p = &sl->corners[nc].pos; ia = nc++;` (the pointer taken before the increment): nc goes to ESI and sl to EDI as
   in the binary (8).
5. `int idx` declared in each wall block: the second block's idx takes wall_a's dead argument slot (4).
6. Each search as `i = nc - 1; if (i >= 0) { do { ... } while (--i >= 0); }`: the binary's loop bottom is
   `test; jl exit; jmp top`, which the `for` form (`jge top; jmp exit`) does not give (0).
Same behaviour: the do / while with its guard runs the same iterations as the `for`; `p` is the same corner taken
before or after the increment; corners[nc - 1] after `idx = nc++` is corners[idx]. Devices: the pointer-local
operand order (D17202), dead argument slots for block locals (D25400), the do / while loop shape (D18300).

**D26901 (game/unnamed_avoidance_0x4a6710.cpp). avoid_merge_shared_segments 0x4a8b80 is exact.** D22802 had noted
that c / cc / l use different slots in the two branches; declaring them in the branches alone changed nothing
(Observed, also in 2026-10-21e). Together with the call hoisting they match:
1. `ml = refs[k].list; e = avoid_ref_entry(&refs[k]);` before the calc_shared_corner_offset call, and the call
   given `ml` and `e`: the binary loads k's list into EDI and calls avoid_ref_entry before pushing any argument
   (128 -> 100); with c / cc / l block-scoped in each branch, 66.
2. `int n = refs[k -/+ 1].index - 1; sp = &l->segs[n]; if (sp->b != c) sp = &l->segs[n - 1];`: the binary's
   `dec eax` before the scaling, and its two `lea`s at +0x18 / +0x4 from one base (56).
3. `if (!avoid_ref_entry(&refs[k])->side)` without the local e (e is now assigned only in the branches): w moves to
   EBP and &refs[k] to EBX as in the binary (0 distance, but the frame offsets still differed).
4. `pf_edge *done[40]; avoid_seg_ref_entry refs[40];`: read from the binary's frame (done at frame +0x24, refs at
   +0xc4, frame 0x2a4). With done[43] the frame size was right but refs sat 12 bytes higher.
Same behaviour: the calls and their arguments are the same and in the same order (the k entry is fetched after the
copy, as before, since the old code fetched it while pushing the arguments). done holds at most one wall per
segment of sl, and a segment list has at most 30 segments (AVOID_SEGLIST_MAX), so 40 entries are enough; refs
only grows (avoid_collect_segment_refs has no bound check, so the larger array is not less safe).

**D26902 (game/new_status.cpp; closer only). status_handle_click 0x42bb00, 237 -> 191.** `int dx, dy, dz;` at
function scope instead of in the closing block: the frame becomes the binary's 0x1c and the `key` reloads in the
returns line up. Left: the skill-row loop's register permutation (the binary has rows / i / i * font_h in
EBP / EDI / ESI; ours EDI / ESI / EBP). Tried on top, no change: a separate `j` for the row loop, rows declared at
its assignment; worse: a second font height local (196). Same behaviour.

## Tried without a match (do not repeat)

**D26903 (notes only).**
- avoid_poly_add_search_nodes 0x4aa400 (8): a plain `float d = 0.0f; if (!overlap) return d;` (at the top, first
  declared, also as the overlap call's margin, also as the last return): all 8, VC6 propagates the constant. The
  binary's store / reload needs a slot whose address escapes (see 2026-10-22).
- avoid_prim_overlap 0x4a6e00 (13): the dead `fst [esp+0x1c]` (into b's argument slot) is the third test's u
  term `fabs(b->axis_b.dot(*u))`, not the v term. kw assigned to the u term instead (in four spellings, with
  and without the first two projections swapped): 23. The swapped projections alone: 13.
- ls_khosanilab_puzzle_trace_beam 0x4f7b50 (63): a's block moved after b / c / the dead end (`if (!open(a))
  {...continue;} a...`, three forms: 128, the instruction count then equals the binary's); one shared check /
  turn_dirs tail after an if / else (90, 189); `cur = next` once at the loop end (129, 211). The binary keeps
  case a's copy of the tail and both b and c jump into it (b at `cur = next`, c at the dir compare); 8966
  keeps the last copy.
- avoid_add_primitive 0x4a9eb0 (355): tb / kb / ca / sum block-scoped per branch (the frame goes to 0x20 or
  stays 0x30; 347-359). The binary is 59 instructions longer in the circle branch (not common-subexpressed).
- avoid_merge_obstacle_polys 0x4a73f0 (233): out[32..35] and a block-scope edge move the frame size but not the
  distance; the slot layout was not worked out.
- gr_opengl_read_backbuffer 0x5502c0 (56): byte-index forms of the swap (`row[j * 4 + k]`, `j += 4`; 68, 90),
  the bottom byte read first (71, 85); the row pointers computed from i in the loop (`buf + i * w * 4`,
  `buf + (h - 1 - i) * w * 4`; 60: gives the binary's preheader after the h / 2 test, but 8966 then steps
  bottom as an offset from top instead of keeping two pointers).
- Not retried (each has 25+ forms in earlier notes): fx_create_spell_effect_facing, mp_handle_swi_path,
  goal_do_travel, decals_draw_for_lobj, spline::eval, vif_emitter_update, localize_load_names,
  LineFile::detect_eol / read_line, vif_record_get_bounds, gr_opengl_bitmap, fx_missile_hit. Not attempted:
  the large structural ones (fx_frame, fx_update_missile, fx_find_spawn_point, avoid_clip_segments,
  avoid_create_obstacle_polys, party_spread_attackers, gl_build_mipmaps, lobj_raycast_bsp,
  script_build_avoid_spline, decal_build_clip_planes, journal_handle_click, character_learn_chain_skill,
  pick_attack_target, ai_choose_attack_slot, gr_opengl_set_mode, vif_compact_particles,
  script_action_walk_to, script_action_replace_armor).

## Behaviour errors / bug candidates

None found.

## Header changes

None.
