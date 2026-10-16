# match7: VC6 byte-match pass over nine game units

Base: `scratchpad/snapD/src/decomp` (the repo's current src/decomp), copied to `match7/src`. Compiler: VC6
(cl 12.00.8168) through `tools/vc6-compare.mjs` with the per-unit flags (all nine units use the default
`/O2 /GX /MT`). Decisions D6500-D6515 are at the end of `src/include/DECISIONS.md` (and a pointer from D4902 to
D6500). Every function that matches now says `Confidence: exact`. Helper scripts (not for the repo) in
`match7/tools`: match4's, retargeted, plus `elp.mjs` (element pointer in a loop), `pmfm.mjs` (find_move result
through a local), `chkE.mjs` (compares the help tables' `$E` initialisers with init_strptrs_*), `hdrcheck.mjs`
(VC6 before / after per unit, two header trees), `chkconf.mjs` (markers against match status). Raw runs:
`before/`, `after/`, `final_check.txt`, `hdrcheck.txt`.

## Totals

**150 -> 211 of 267 functions byte-identical (56.2% -> 79.0%, +61).** new_help.cpp did not compile under VC6
before (0 of 11), it does now. No function that matched before lost its match (checked address by address,
`final_check.txt`).

| File | Before | After | Functions |
|---|---|---|---|
| game/new_saveload.cpp | 23 | 30 | 37 |
| game/new_charselect.cpp | 18 | 25 | 32 |
| game/new_buysell.cpp | 28 | 37 | 42 |
| game/unnamed_machival_0x4c0c40.cpp | 13 | 22 | 26 |
| game/unnamed_luminar_0x4c8d10.cpp | 10 | 16 | 23 |
| game/unnamed_ghost_rider_0x4b9e70.cpp | 25 | 31 | 38 |
| game/gametarget.cpp | 9 | 10 | 22 |
| game/unnamed_phoenix_0x497ac0.cpp | 24 | 30 | 36 |
| game/new_help.cpp | compile error (0) | 10 | 11 |

Plus the five init_strptrs_* initialisers of new_help.cpp (no definition to pair, so not counted by the tool):
identical store for store (`tools/chkE.mjs`, D6500).

## new_help.cpp (D6500, D6501)

VC6 rejects every brace form of a struct with two reference members, and of an aggregate whose members have
constructors (C2552); only the one-reference struct with a converting constructor (D4902) brace-initialises. So the
tables are `help_string help_*_lines[][2]` with `struct help_string { char *&str; }`, rows `[i][0]` (key) and
`[i][1]` (description). Same layout (two dwords per row), same five `$E` initialisers in the same order, and the four
page drawers match byte for byte: the drawing code was never different, only the source spelling. help_frame matched
once the ESC screen choice was an if / else of two interface_set_screen calls. Left: help_draw (the sprintf pop
merging of D6206).

## Header changes (each with every caller compiling; D6510, D6511, D6513)

- boss_ai.h: the Machival's .bss 0x248b030-0x248b377 and the Ghost Rider's 0x2480310-0x248090f are one object each
  (struct machival_boss machival, struct ghost_boss ghost; the old names are macros). Evidence: one static
  initialiser constructs all of each unit's GameTimers (VC6 emits one `$E` per global), and the binary reloads the
  offense-point / hit / remap counts after array stores, which 8168 reproduces only for fields of one object. Seven
  functions match from that alone. The Luminar's 0x248cdb0-0x248ce2f is one object by the same rule but was not
  converted (no code change; D6512).
- new_charselect.h: mp_charselect_get_selected_profile(bool) (`mov al, [esp+8]`).
- mingw: 0 `error:` lines in all 259 .cpp. VC6 before / after for the 17 other units that use a changed name: all
  identical address sets (D6510-D6513 list them).

## What worked (D6502 has the list with every function)

- **Branch polarity and shared exits**: write the binary's fall-through as the then-block (party_order_attack,
  buysell_buy_item, the update_scrollbar pair, pick_weighted_move's last test), nested ifs with one `return false`
  (both seat_rider), cmd_phoenix_unlink acting inside its search loop.
- **The update_buttons template**: `if (!mouse_is_down(0)) { release } else { held; return; }` with an element pointer
  (three units).
- **Element pointers** in the load_bitmaps / draw loops, the category tooltip; index loops instead of pointer loops
  where the binary compares with `jl`; the reverse (no pointer) in draw_char_list.
- **Rectangle arguments through locals** (x, y, w, h).
- **Min clamps as `if (pos < max) max = pos; scroll = max;`** (four page down / drag functions).
- **Vector expressions**: `fvec * s + pos`, `a - b`, `pos + centre_offset`, `vector3 d(x, 0, z)`, `vector3 v =
  vector3(...)`; natural x, y, z dot products.
- **Calls as arguments through locals**: `move_info *m = owner->find_move(..)` before play_move (four boss functions),
  `int r = rand() % 6`, the variation before an if / else.
- **If / else of two calls** for a ternary argument (help_frame, buysell_button_action).
- Store order (`cast_anim` before `casting_spell` in the source), a 0x80-byte dead message buffer, fabsf, a bool
  parameter.

## What still differs (D6514)

56 functions. Most by what no source form changed under 8168: shared `return 0` blocks that 8966 jumps to from
several early returns (saveload_handle_input, ghost_choose_attack, character_can_talk_to), jump tables with their own
slot for an empty `case -1` (the three ai_frame functions), do-while loops whose success block 8966 moves to the end
(ghost_wander_start, luminar_teleport_out), the `max(x, 0)` AND register (five functions that otherwise match),
loop-bound and argument scheduling (ghost_update_attack_points 0.96, the boss_load find_mesh_tag call 0.94 / 0.99),
tail merging of identical message-box calls (container_examine), a float passed through the FPU
(saveload_capture_thumbnail: probably an inline camera accessor). The long ones (saveload_handle_input,
buysell_handle_click, mp_charselect_handle_click, world_mouse_frame, world_pick_under_mouse,
object_try_unlock_or_open, phoenix_choose_attack, phoenix_reposition_update, luminar_frame_hook) differ by these and
by block layout. gametarget.cpp gained least: its functions are long and FPU-heavy (min / max of floats in
world_pick_ground_point, the hover / pick functions).

Markers that say exact but differ (left as they were, D6515): mp_charselect_update_create_button,
buysell_page_up, buysell_goto_inventory, phoenix_anim_remap_out.

## Logic errors, game bugs

- No new logic error in the decompiled source; every change keeps the behaviour (float `h * (x / c)` and
  `x / c * h` are the same IEEE value; the vector operators compute the same components).
- **Observed (binary, minor):** luminar_beam_hit (0x4ca47f-0x4ca49c) calls ai_state::go_idle twice when the beam
  ends in stage 3 (once after rearming the 20 s beam timer, once on the common path). The second call repeats the
  first; probably harmless, not tested in game. Not in docs/sum-exe/game-bugs.md (C250 there is the NULL target).
- Already documented and unchanged: phoenix_special_strike's blast tests the target, not each party member (NOTES /
  game-bugs), luminar_beam_hit's NULL target (C250).
