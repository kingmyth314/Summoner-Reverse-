# match6: VC6 byte-match pass over ten game units

Base: `scratchpad/snapD/src/decomp` (the repo's current src/decomp), copied to `match6/src`. Compiler: VC6
Professional (cl 12.00.8168) through `tools/vc6-compare.mjs`; all ten units use the default `/O2 /GX /MT` (none is in
vc6-flags.txt). Headers from `match6/src/include`. `base/<unit>.txt` and `after/<unit>.txt` are the `vc6-compare -v`
outputs. Decisions D6400-D6406 are at the end of `src/include/DECISIONS.md`. Helper scripts (not for the repo) are in
`match6/tools` (match4's, retargeted, plus `fnspec.mjs` and `tw.mjs`, which also reports relocation-target warnings).

## Totals

**246 -> 302 of 439 functions byte-identical (56.0% -> 68.8%, +56).** No function that matched before lost its match
(checked address by address, base/*.txt against after/*.txt). Every function that matches says `Confidence: exact`
(50 markers raised by this pass, earlier matches included).

| File | Before | After | Functions |
|---|---|---|---|
| game/level_scripts.cpp | 69 | 76 | 124 |
| game/unnamed_serpent_0x495290.cpp | 27 | 31 | 43 |
| game/unnamed_ai_move_0x4b1410.cpp | 4 | 6 | 19 |
| game/spells.cpp | 18 | 22 | 39 |
| game/unnamed_fx_0x489c40.cpp | 42 | 45 | 57 |
| game/new_journal.cpp | 12 | 19 | 27 |
| game/new_itemtrade.cpp | 21 | 29 | 36 |
| game/unnamed_pfgeom_0x4a4ee0.cpp | 11 | 16 | 24 |
| game/unnamed_dialogue_box_0x4d6660.cpp | 18 | 23 | 32 |
| game/new_spell.cpp | 24 | 35 | 38 |
| **Total** | **246** | **302** | **439** |

One header change (D6401: `seg2d_point_near_middle` returns int; its only caller is in the geometry unit). mingw
(`i686-w64-mingw32-g++ -fsyntax-only -std=c++98 -w -Iinclude`): 0 `error:` lines for all 259 .cpp files.

Relocation-target warnings: the base had six (journal 3, itemtrade 2, new_spell 1). itemtrade_drag_scrollbar now
matches without one (its natural operand order matched only with swapped `game_mouse_y` / `drag_start_thumb` loads);
the one left (spells_set_page) is in a function that still differs. No matching function has a warning.

## Logic errors and game bugs

- **Logic fix (D6400): serpent_anim_remap_out** (0x495db0) copied `mount->loco_anims[slot]`; the binary copies the
  character's own table (EBX = the argument; the mount is only tested for NULL), exactly as tiger_anim_remap_out
  (D6204). Verified on the disassembly, fixed, now exact. The remap_in comment ("puts the mount's current anims back")
  is corrected.
- **Source correction (D6404): automap_label_add** copies the label text with `memcpy(dst, text, strlen(text))` after
  the 0x20-byte memset (the binary's `dec ecx` after the strlen scan); the source had strcpy. Same result below 32
  characters; at 32 or more both overrun the slot, strcpy by one more byte.
- **Declaration fixes:** seg2d_point_near_middle returns int (D6401); the geometry and wall-probe segments are 0x10-byte
  pf_edge records (D6402); serpent_rider_set_flinch's buffer is 0x80 (as the tiger's, predicted by match4).
- No new game bug. Behaviour is unchanged everywhere else (each rewrite is a pure source-shape change).

## What worked (D6403, each verified by a byte match)

- **Unrolled source** for straight-line binary code: spells_update_species_tab (16 explicit clears) and
  spells_count_page (one case per class with its own call, 168 -> 508 bytes): both exact. new_skill.cpp's
  skills_count_page / species tab have the same shape (not in this pass; worth trying there).
- **Element pointers for both arrays of a button loader** (`b = &x_buttons[i]; d = &x_button_defs[i];`): all three
  load_bitmaps of this pass exact. automap / skills / status_load_bitmaps (other units) differ the same way.
- **Index loop vs pointer loop** decides `jl` (index) or `jb` (pointer): fx_lists_init, itemtrade_frame, write_groups.
- **Case order = block order**: 8168 lays out jump-table cases in source order (damage_type_to_status,
  spell_get_duration_ms).
- **Clamps**: `if (line < last) last = line; g = last;` instead of a ternary stored to a global (three functions).
- **Segments as pf_edge** (frames, D6402): pf_trace_edge_crossings, segments_intersect_xz, pf_layer_from_edges.
- **vector3 from components** (`vector3 end(p.x + m.x, ...)`) is built in place; with a while loop
  ai_probe_move_for_walls is exact.
- **A call argument that is a call into a local** (mode_sneaking_frame's find_move, the two dialogue inits,
  journal_select_quest), rectangle arguments through locals (row hit tests, list mouse handlers).
- Block layout, bool shapes, and small local tricks (read_living's bitfield into an int, set_quest_bit's index / bit,
  get_float's bit copy, split_quotes `if (*p) do ... while`, journal_frame's `return key` in case 1): see D6403.

## What still differs (D6405 / D6406)

137 functions. Most differ by what no source form changed (8168 against the original 8966):
- jump tables indexed from 0 although the smallest case is 2 / 5 (status_effect_modifier / _duration);
- one call whose constant argument is selected by `push 1 / jmp / push 0` (quest_journal_set_entry,
  savegame_refresh_character_flags);
- float members copied through an FPU temporary (write_character, automap_label_add, spell_effect_obj_set_timer);
- the vector3 copy constructor called out of line after an operator= chain (the ai_move EH functions: steer_jitter,
  path_closest_point, follow_path, avoid_characters, travel_steer_update, ...; fx_create_* with negated matrix rows);
- register choice, scheduling and loop rotation (dialogue visible-line loops, level_item_*, get_int, spells_set_page,
  shadow_create_blob_bitmap, serpent_wander_start's block order);
- the long functions (spell_prepare_cast, spell_cast_execute, spell_cast_launch_effects, fx_frame,
  conversation_execute_action, events_parse_table, avoid_characters, follow_path) by the above and by block layout.
- 41 markers say exact (by reading, before the tool) but differ; left as they were (list in D6406).
seg2d_point_near_middle (rewritten here) is now marked likely.

## For later passes

- Try the unrolled forms and the loader element pointers in new_skill.cpp, new_automap.cpp, new_status.cpp.
- The `jl` / `jb` rule (index vs pointer loop) and the case-order rule apply to any unit with button arrays or
  jump-table switches.
- The vector3 copy-constructor call is the main blocker of the AI movement unit (13 of its 19 functions).
