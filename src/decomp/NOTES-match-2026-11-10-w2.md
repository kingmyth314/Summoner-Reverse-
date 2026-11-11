# Byte-match round 2026-11-10, worker 2 (D26500-D26599)

Batch: 37 differing functions in 15 units (pathfinding, gr_direct3d, the AI combat unit, the float helpers, new_hud,
ai, boss_ai, the DirectSound unit, the polygon split, automap, buy / sell, the dialogue table, the editor dialogs,
matrix, parse). Distances are the `vc6-compare --score` distances of 2026-11-10 (VC6 8966, strict).

Result: **no new match, 1 closer** (gr_d3d_set_state 326 -> 314). Strict sweep over the 15 units with `--check`:
564 matched there, 564 now, **0 lost**.

Every function at distance 24 or less in this batch already has 30-100+ forms recorded in earlier rounds as register
choice or scheduling. I read those lists first and did not repeat them. The work went to functions whose
difference is a block, a return type or a constant register rather than plain register choice.

Helpers (worktree `v/`, not committed): `tryd.mjs` (applies variants, file restored, prints the --score distance;
takes an anchor so a substitution only applies from one function on), `apply.mjs` (applies one variant in place
for `--align`), `score.sh` / `align.sh` / `sweep.sh` (run the main checkout's tools with this tree as root).
**Note for the distance**: for a function with a jump table, `--score` also counts the table data, which our
listing has as `DD` / `DB` lines and the original disassembles as code. boss_move_in_range_dispatch (241),
hud_handle_keys (198), gr_d3d_set_state (314) are far closer than their numbers: read `--align` and skip the tail.

## Closer, kept

| file | address | name | D | what did it | distance before -> after |
|---|---|---|---|---|---|
| vsdk/gr_direct3d.cpp | 0x534a40 | gr_d3d_set_state | D26500 | the fog-off path of cases 0-2 falls through into case 3 | 326 -> 314 |

**D26500 (vsdk/gr_direct3d.cpp; closer only). gr_d3d_set_state 0x534a40: fog off falls into case 3.**
```
switch (fog_type) {
case 0: case 1: case 2:
    if (gr_fog_enabled) {
        gr_d3d_set_render_state(D3DRENDERSTATE_FOGENABLE, TRUE);
        break;
    }
    // fall through
case 3:
    gr_d3d_set_render_state(D3DRENDERSTATE_FOGENABLE, FALSE);
    break;
}
```
replaces `if (gr_fog_enabled) set(TRUE); else set(FALSE); break; case 3: set(FALSE);`. The binary has one
`set(FOGENABLE, FALSE)` block, at the very end of the function (0x5353a1). Case 3 (`cmp eax, 3; je`) and the
fog-off test (`je`) both jump to it. The if / else form gave 8966 two copies: case 3's inline after the compare
chain, and the else's. With the fall-through, there is one block and it comes last, as in the binary. A `goto` to a
label on case 3 gives the same code (314). Not better: case 3 written first (326), `if (!gr_fog_enabled)` with the
branches swapped (327). Same behaviour: fog types 0-2 with fog off and type 3 make the same single call; other
values make none. **Observed** (compiled, aligned); that it is the original form is **Inferred**.

Left (the whole remaining difference): the binary keeps the constant 1 in EBX (`mov ebx, 1` after the six getters,
`push ebx` at the very start before the early-return test), and every `push 1` and both `gr_d3d_vertex_* = true`
stores use EBX / BL. 8966 at /Ox pushes the immediate. The frame offsets of the other locals then differ by the
4 bytes of that push. Tried: `#pragma optimize("s", on)` around the function (EBX = 1 appears, but the whole
function changes, 943), `"t", off` (943), `"y", off` (352), the early return as an `if (!(mode == cur)) { ... }`
block (314), `= 1` for the two bools (314). This is the "constant in a callee-saved register" left open since
D7203 (ls_ikaemostopint_init) and D15403 (a goto moved 1 into EBX in version_check_run). Every case in Sum.exe
that I found with a constant in a register (buysell_update_category_buttons and others) has the uses inside a loop.
gr_d3d_set_state has no loop, so the trigger is still unknown.

## Proposal (header; not applied)

**D26501 (proposal, include/engine/ai/boss_ai.h). tiger_move_in_range and giant_salamanka_target_in_reach return
bool.** This is D7603 (3) again, with new evidence. In boss_move_in_range_dispatch 0x4a0ea0 the binary returns the
tiger / salamanka results with no `neg / sbb / neg` conversion. It also returns serpent / tentacle /
boss_move_target_in_range (bool) with no zero extension. Its only caller, living_combat_resolve_hit, tests `al`
(0x467eb0). So every callee has the dispatcher's own type, bool. D7603 checked that tiger's and salamanka's units
keep their matches with bool (`return d < reach;`). New: the Luminar / Murod case written `return r > d;`
(`float r = boss->info->attack_radius;` before the call) gives the binary's `mov eax, 1` (as
boss_move_target_in_range does for its bool). The `if (...) return true; break;` form gives `mov al, 1`. What still
differs in that case is the compare: the binary does `fld r; fcomp st(1); test ah, 0x41; fstp st(0)` and 8966
`fcomp r; test ah, 1`. The binary's shared false is `xor eax, eax`; ours is `xor al, al`. Not applied: the header
change alone does not give a match.

## Still differs (tried this round; do not repeat)

- **boss_move_in_range_dispatch 0x4a0ea0** (241 with the table; real difference: the two conversions above plus
  the Luminar compare). Luminar forms: `return r > d` / `return d < r` with and without a `d` local (all give
  `mov eax, 1`, 244), `if (r > d) return true; return false;`, `if (d < r) ... break`, the radius read inline in
  the compare (313-316: the read moves after the call), `(r) > d`, `(r) > (d)`, `if ((r) > d)`, `return 1` with
  a final `return 0`, `return 0 != 0` at the end, a `reach` / `dist` pair assigned in the case and compared after
  the switch (3 forms), static inline helpers (int, bool, bool returning the compare, a float distance helper;
  241-318). 8966 always folds the call result into `fcomp [r]`. In tiger and salamanka the binary's
  `fld; fcomp st(1)` comes from a value that reaches the compare from a join of two paths. The dispatcher's case
  has no join, and no form gave one.
- **hud_handle_keys 0x40d930** (198, with the tables): the shared 0x23 sound tail (kept after Esc in the binary,
  after Ctrl+S by 8966). Besides 21e's goto forms: Ctrl+L / Ctrl+S moved before Esc (both orders) or to the top of
  the switch, Esc moved last, Esc + F1 moved last. All give the same code (198). The compare-tree cases are
  not placed by source order.
- **TextFile::ParseUInt 0x516380** (12, digits in EBP / EBX): a bool digit flag (33), a `result` local (12),
  `int value` with an unsigned result (16), `digits += 1` (12), `digits++` after the value update (19 decimal,
  12 hex), `<= 0` (14), `< 1` (15), `cursor = cursor + 2` (12).
- **matrix43::transpose_in_place 0x511bb0** (46): D24304's rotations each in a block with its own `float t`
  (46); the same with one function-scope t (48); `static __forceinline` rot3 helpers with references or pointers
  (60) and a pair-of-swaps helper (80). /Ob0 keeps even `__forceinline` out of line in 8966, so a helper cannot
  produce the dead [esp] stores.
- **hud_portrait_get_color 0x40f880** (38), **combat_toggle_ready_stance 0x4bdce0** (24), **ai_state::load 0x4747e0**
  (18), **pf_read_edges** (2), **cast_effect_track_add** (2), **fast_sqrt / fast_sqrt_refined / fast_inv_sqrt**,
  **pf_find_cell_crossing** (16), **pf_sweep_test_wall** (235): looked at with `--align` only. The differences are
  the ones the earlier lists name (combat_toggle_ready_stance: the binary keeps `ready` in BL, reusing the
  constant 1 of the flag test; hud_portrait_get_color: the clamp through CL with the 255 block last). Nothing new
  was tried.
- Not looked at (big, register / x87 scheduling per earlier notes): approach_attack_slot, conversation::parse_table,
  gr_d3d_bitmap, gr_d3d_tmapper_multipass, gr_poly_split_sections_draw, automap_draw_map, buysell_handle_click,
  ds_load_buffer_acm, float_format_buf, pathfinding_load_graph, pf_check_direct_walk, pf_mark_nodes_visible_from_point,
  pf_graph_search, editor_dialog_show, d3d_compute_pixel_format_shifts, gr_d3d_init_device, boss_ai_unload
  (D7603: needs the unload callees as ai_state members, other units).

## Behaviour errors / bug candidates

None found.
