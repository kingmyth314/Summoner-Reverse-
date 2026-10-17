# match31: second VC6 8966 pass over 33 units

Tree: `match31/src` = copy of `snapV/src/decomp`. Compiler: `scratchpad/vc6sp5/VC98` (cl 12.00.8966).
Flags: this tree's `vc6-flags.txt`, unchanged (game /O2, vsdk /Ox /Ow per D7502, some /Ob0).
Before / after: `before8966.txt` / `after8966.txt` (sweep.mjs, one line per marked function).

**845 -> 867 of 1,014 functions; none lost** (checked address by address). No header or flags change, so no
other unit is affected. mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in all 21 changed .cpp.
Decisions D9000-D9008 appended to `src/include/DECISIONS.md`. Changed files: CHANGED.txt.

## Per file (before -> after / total)

| file | before | after | won |
|---|---|---|---|
| game/new_skill.cpp | 34 | 38 / 39 | 0x427240 load_bitmaps, 0x427f30 handle_click, 0x428cf0 draw_list, 0x4291d0 skill_is_available |
| game/unnamed_serpent_0x495290.cpp | 35 | 38 / 43 | 0x4955e0 spawn_rider, 0x495960 boss_load, 0x4961b0 evaluate_target |
| game/unnamed_ghost_rider_0x4b9e70.cpp | 33 | 36 / 38 | 0x4ba260 boss_load, 0x4bac20 choose_attack, 0x4bb120 choose_move_target |
| vsdk/file_packfile.cpp | 16 | 19 / 20 | 0x5340d0 assign_offsets, 0x534830 fseek, 0x5348a0 fread |
| vsdk/skeleton.cpp | 23 | 26 / 28 | 0x543bb0 vim_copy_bitmaps, 0x543bd0 vim_save_bitmaps, 0x543e10 ~skeleton |
| game/s3d.cpp | 26 | 28 / 34 | 0x46f100 load_lightmap, 0x46fdc0 level_free_s3d |
| vsdk/effect_mem.cpp | 31 | 33 / 39 | 0x51c370 effect_base_release, 0x51c720 effect_get_base |
| game/unnamed_machival_0x4c0c40.cpp | 24 | 25 / 26 | 0x4c11c0 count_targets_in_reach |
| game/unnamed_phoenix_0x497ac0.cpp | 32 | 33 / 36 | 0x498420 boss_load |
| all other 24 units | | unchanged | (conversationctl 19/27, new_charselect 25/32, new_gamecreate 16/20, new_itemtrade 29/36, new_journal 25/27, new_saveload 31/37, new_status 48/51, pathfinding_visibility 4/11, dialogue_box 24/32, luminar 20/23, murod 9/12, pfgeom 16/24, pyrul 20/23, titus 26/27, matrix 43/46, parse 45/51, gr_line 11/19, gr_poly 19/24, lights 35/41, localize 23/28, sndstream 21/26, vfile_rw 50/51, vfx 24/28, vif 8/15) |

## What worked (details in D9001-D9005)

- **Brute force over dot-product term orders.** perm.mjs writes all 48 orders (6 term orders x which operand
  comes first in each product) of a three-term dot; try.mjs (with an `all` flag that replaces every
  occurrence) compiles each. machival_count_targets_in_reach matched with `dir.y * d.y + dir.x * d.x + d.z *
  dir.z`. Reusable for any other hand-written sum 8966 reorders.
- Facing tests through the inline `vector3::dot` on a difference vector with y cleared (serpent_evaluate_target).
- One vector3 local assigned twice for two spawn points (frame sharing), `pos = owner->pos + fvec` for the
  binary's temporary copy; the mesh-tag lookup through `name` / `ch` locals (as machival, D7700).
- Block-scoped locals so the temporaries share slots (ghost_choose_move_target), `p.y = 0; p *= r; p += pos`.
- A shared `return 0` via `goto none` (ghost_choose_attack), and the `if (x != -1) work; else goto none;`
  layout for the last of three copies.
- UI: element pointers with the def pointer first; direct global indexing instead of a cached skill id;
  switch case bodies in the binary's order, `case 0:` beside `default:` for a jump-table slot; `== 8 ? true :
  false` for `sete` without `xor`; the 128-byte second buffer at function scope (frame 0x10c).
- vsdk: element-copy loops instead of memcpy (8966 makes the rep movsd), counters before list_remove, a
  pointer read inside the loop (hoisted after the guard), two fread calls instead of a joined FILE pointer,
  the fseek offset sum as a statement, `int sector = first_sector`.
- s3d: data / palette scoping puts them in the binary's slots; `short w, h` for the lightmap size.

## Closer but not matching (kept)

vfx_load (result variable + nested ifs: now differs only in the copy of the base's two flag bits, which the
binary builds through 13-bit sign extensions, probably bit-field assignments of an unknown layout);
ghost_find_floor_below (frame now right; the z copy as an integer and the zero register remain);
phoenix_reposition_update (b copied into a local, `mid` assigned); murod_start_leap (constructor form);
character_use_skill (hide / sneak goto form; the appraise root stays a stored dead value in the binary);
gr_clip_and_project_poly (only the two flag-byte inits are swapped); journal_handle_click (element pointers;
the frame is 8 bytes short and 2 is kept in EBP in the binary); rfx_preload (only `lea edi, [ecx + edx]`).

## Still differing, tried (see D9007)

- serpent / ghost / phoenix anim_remap_in: `ready` in the dead `c` argument slot; every form keeps it in EBP
  and uses neg/sbb for `ready ? 3 : 0`.
- serpent_illusion_update: `rem` lives in memory in the binary, and the alpha is `base + (rem-300)*base /
  -1000` without 8966's fold to a subtraction; function-scope rem, if-forms for the bases tried.
- serpent_choose_attack (frame 0xb0 vs 0xa0), pyrul_choose_attack, titus_ai_select_action,
  machival_ai_select_action (target kept in a frame slot), murod_ai_select_action: large, not reached.
- effect_base_read (push esi before the bounds test: early-return and scope forms tried),
  effects_parse_arr_line (two `return -1` blocks: break / goto / result forms all merge),
  packfile_read_directory (p in pf's dead slot), localize_unescape (empty-string test without the CSE),
  wav_open (nBlockAlign operand load order: five forms).
- dialogue_box::draw (`first` loaded into EBP before adding i: index, local and order forms tried);
  itemtrade_draw_list1 / 2, status_draw_skills (rows kept in memory and x in a register in the binary:
  declaration orders tried); mp_charselect_page_down, update_create_button.
- conversation_controller::camera_capture: the binary calls the vector3 (x, y, z) constructor and the copy
  constructor out of line while it expands the transform; `#pragma inline_depth(0)` with a macro transform
  gives the right frame (0x48) and calls but not the expanded transform. Probably a different inline helper
  set in the original; not kept.
- matrix43::transpose_in_place: the binary's dead stores of a temporary (four assignments) are the
  mis-indexed swaps; no SWAP-macro sequence found that also gives the same final values.
- seg2d_point_left_of: dx / dy kept on the FPU stack (`fmul st, st(1)`, popped at the end), five forms.

## Logic errors / game bugs

None new found in this pass. (All changes keep behaviour; murod_start_leap's constructor form evaluates the z
frand first, as the binary and as VC6's right-to-left argument order does.)

## Markers

22 raised to exact; 11 exact markers that differ under 8966 lowered to likely (D9008).

## Tools (this folder)

cmp.sh (vc6-compare with this tree's env), sd.sh (side by side of one function; `~` marks a register-only
difference), side.mjs (register-aware), try.mjs (variants; `all: true` replaces every occurrence; SIDE /
SIDEN env), apply.mjs (apply one variant), perm.mjs (dot-product permutations), markers.mjs (raise / lower
markers from two sweep lists), sweep.mjs.
