# match1: VC6 byte-match pass over ten game units

Base: `scratchpad/snapB/src/decomp` (commit 95bd3d5), copied to `match1/src`. Compiler: VC6 Professional
(cl 12.00.8168) through `tools/vc6-compare.mjs` with the per-unit flags (all ten units use the default
`/O2 /GX /MT`). Decisions D5900-D5905 are at the end of `src/include/DECISIONS.md`. Every function that matches now
says `Confidence: exact` (73 markers raised from likely / unsure, earlier matches included).

## Totals

**399 -> 491 of 723 functions byte-identical (55.2% -> 67.9%, +92).** No function that matched before lost its
match (checked against the base run, address by address). The relocation-target warning of hud_mouse_in_panel
and ui_tooltip_draw is gone (operands now in the original's order).

| File | Before | After | Functions |
|---|---|---|---|
| game/unnamed_script_editor_0x4de0b0.cpp | 62 | 71 | 120 |
| game/unnamed_avoidance_0x4a6710.cpp | 29 | 36 | 67 |
| game/unnamed_render_scene_0x481680.cpp | 44 | 48 | 77 |
| game/ai.cpp | 56 | 73 | 88 |
| game/characterinfo.cpp | 39 | 50 | 70 |
| game/unnamed_goals_doors_0x475180.cpp | 38 | 52 | 67 |
| game/unnamed_interface_0x411270.cpp | 41 | 52 | 69 |
| game/new_hud.cpp | 32 | 41 | 60 |
| game/new_inventory.cpp | 45 | 51 | 69 |
| game/pathfinding.cpp | 13 | 17 | 36 |

Checks: mingw `-fsyntax-only -std=c++98` gives 0 `error:` lines for all 253 .cpp files with the changed headers.
The other units that call the changed declarations keep their VC6 counts with the new headers (living_entity
49/134, new_multi 9/10, new_spell 24/38, unnamed_party_inventory_0x46b460 19/30).

## What worked (each verified by a byte match)

- **bool returns.** `return a && b;` in a bool function compiles to `mov eax, 1 / xor eax, eax`;
  `if (a && b) return true; return false;` to `mov al, 1 / xor al, al`. The binary uses both, so pick by the
  register: 10 ai_state::is_* tests, point_in_rect, skill_is_*, lobj_index_out_of_range,
  script_editor_no_window_open went to the if form; ai_state::is_disabled_any back to `return x || y || z;`.
- **Branch polarity.** VC6 lays the then-block out as the fall-through, so the original's layout gives the
  condition: `if (screen != 3) set(3) else set(0)` (three hud_key_*), `if (on) { ...; return true; } return false;`
  (pick overrides), `if (cond) { body; return true; } return false;` instead of an early `return false`
  (script editor callbacks, ai_goal_compare), `if (x >= 0) {...} else {...}` (show_level_item),
  `if (!mouse_down) {...} else {...}` (inventory_update_buttons).
- **Single exit / else-if.** A repeated `return key;` in the middle kept a second copy; `else if` falling through to
  one return matched (hud_death_timer_frame, ai_state::knock_down_local).
- **Loops.** `while ((i = f()) != -1) g(i);` duplicates the call (loop inversion); `while (1) { i = f();
  if (i == -1) return / break; g(i); }` gives the original single call (`for (;;)` does not). 4 goal functions.
- **Element pointers.** `T *p = &arr[i];` inside the loop / if gives the original's biased pointer and registers
  (hud_mouse_in_panel, inventory_get_list_item, pf_door_index_is_open, ai_global_init, five avoidance
  allocators, level_load's lobj loop).
- **Struct copies** instead of member copies (`pt->p0 = *a`, `out->points[i] = ob->points[i]`), and two plain
  assignments instead of `a = b = c` (integer moves instead of fld / fst).
- **Temporaries.** A float local stops VC6 folding constants (`t = x * 0.001f; t * 204.0f`; quantize's
  `(y + 1024) / 2048 * 65535`); `float fx = x;` gives `fild / fmulp` instead of `fimul`;
  `vector3 d = vector3(...)` (temporary plus copy) instead of `vector3 d(...)` (ai_path_node_create, _b);
  `item = f(); g(item, ...)` instead of nested calls; `int t = (3000 - timer) % 500;` before the colour copy.
- **Statement / declaration order.** Swapped local declarations (hud_draw_health_bars), `piece = f(); n->next =
  piece;` instead of a chained assignment, `n = 0` after the loop pointer, `sprintf(..., i); i++;`.
- **Operand order** of compares and adds (`species == x.species`, `info->anim_class & m->anim_class`,
  `id == value`), `case 0` with its own body next to `default`, a switch for an event if-chain, an EH object
  declared after the early return (lamp_glows_draw), `fabsf` instead of `fabs` on floats, `sinf` instead of `sin`.
- **Types.** int instead of bool where the binary reads or sets the whole register (D5900-D5902); an int temporary
  for `flags & 0x80000000` (neg / sbb / neg).

## What still differs, and why (D5905)

Most of the remaining 232 functions differ in ways no source form changed under 8168 (likely the 8168 / 8966
compiler difference, not provable without build 8966):
- strength-reduced loop compares: `jb` where the original has `jl` (multi_charinfo_slot*, ai_path_pool_init,
  inventory_frame, params_window_cb) and `jl` where it has `jb` (level_load, level_vis_parse), with the pointer
  bias chosen differently (level_vis_reset, avoid_seglists_clear_refs, avoid_clear_segment_flags);
- x87 sums of products: 8168 reorders the terms whatever the source order (all 48 permutations of add_items'
  depth give the same code; the original keeps the source order) - speaker icons, add_items/containers,
  pick_attack_slot, add_skill_from_behind;
- register allocation and scheduling (ecx/eax swaps, push order of callee-saved registers, `rep stosd` setup
  order, store order of struct copies, `lea ebp, [eax]` against `mov ebp, eax`);
- `max(x, 0)` (`setle / dec / and`) ANDs into the other register (page_up, identify_item, destroy_selected_item);
- stack frames: bool locals in the top byte of their slot, different frame sizes (often 8 or 16 bytes), pops
  merged across a following call (level_load_preload, door_lock_value_cb), a Color return copied instead of
  built in place (hud_draw).
Functions that are long (pf_graph_search, hud_handle_keys, script_editor_create_ui) mostly differ by these too.

## Logic errors found in the source

- **mp_player_find_by_addr (0x475180)** returned the first player whose address does NOT match:
  nw_addr_equal returns 0 for equal addresses (D5903). Fixed; the function now matches.
  `game/multiplayerdata.cpp:141` (not my file) also tests `if (nw_addr_equal(...))`; its comment says 0 is a
  match, so it may be intended (the not-equal branch) - worth a look by its owner.
- Relocation-target hints that remain (not mine to change; names, not bytes): script_editor_close calls
  ui_window::dtor 0x52b0a0 where the original calls 0x52b630 (a different destructor, maybe a derived window);
  characterinfo_alloc_level_buffers stores the three hero anim buffer sizes into rotated globals
  (jekhar / joseph / rosalind: the source's assignment of names to 0x7e12b8 / 0x847fe0 / 0x68e1d4 is rotated or
  the stores are); inventory_drag_scrollbar: inventory_drag_start_thumb_y and game_mouse_y swapped (its comment
  already says the two sum.symbols names are swapped); script_editor_create_ui: script_editor_char_sizes 4 bytes
  off.
- No new game bugs.

## Tools (scratchpad/match1/tools)

`w.mjs` (compare, side-by-side listing, lost / new matches against `state/`), `tv.mjs` (try source variants,
keep the best), `fnrep.mjs` / `perfn.mjs` / `tryall.mjs` (apply a rewrite per function, keep only matches),
`survey.mjs` (differing functions by number of differing instructions), and rewrite helpers (boolif, earlyret,
earlyret2, elemptr, loopptr, genperm). `exp/cc.sh` compiles a scratch file with VC6 and prints the listing.
