# match9: VC6 byte-match pass over ten game units

Tree: `match9/src` (copy of `snapE/src/decomp`). Compiler: cl 12.00.8168 (`vc6p`), flags from `vc6-flags.txt`
(all ten units use the default `/O2 /GX /MT`). Decisions D6700-D6705 are appended to `src/include/DECISIONS.md`.

## Counts (tools/vc6-compare.mjs, functions with an address comment)

| unit | before | after | of |
|---|---:|---:|---:|
| game/new_pickup.cpp | 8 | 18 | 20 |
| game/new_options.cpp | 23 | 32 | 35 |
| game/level_script_khosani_lab.cpp | 33 | 39 | 45 |
| game/gameplay.cpp | 28 | 32 | 39 |
| game/conversationctl.cpp | 15 | 17 | 27 |
| game/unnamed_tentacle_beast_0x4c6480.cpp | 14 | 19 | 25 |
| game/s3d.cpp | 23 | 24 | 34 |
| game/level_script_worldmap.cpp | 13 | 20 | 23 |
| game/unnamed_pyrul_0x4b87c0.cpp | 14 | 19 | 23 |
| game/new_levelselect.cpp | 16 | 21 | 25 |
| **total** | **187** | **241** | **296** |

Whole tree (all 259 .cpp, `sweep.mjs`): 4107 -> 4161 matches; the per-address lists of snapE and match9 differ only
in the 54 new matches (no match lost in any unit, no address added or dropped). Out files: `out/sweep_snapE.txt`,
`out/sweep_final.txt`, `out/newmatch.txt`.

Checks: mingw `i686-w64-mingw32-g++ -fsyntax-only -std=c++98 -w -Iinclude`: 259 files, 0 `error:` lines (two
headers changed, so all files were checked). VC6 before / after for every unit: the sweep above.

## Header changes

- **D6700** `levelrender.h`: `hover_tint_active` returns bool (its caller tests AL). The definition in
  `unnamed_render_scene_0x481680.cpp` (not one of the ten files) changes its return type only, as the header
  requires; that unit keeps its 48 / 77 matches at the same addresses. gameplay_update_cursor matches.
- **D6701** `boss_ai.h`: the Pyrul's state 0x247fd28-0x24802cf is `struct pyrul_boss pyrul` (as the Machival and
  Ghost Rider, D6510 / D6511), old names as macros. Gives pyrul_boss_load and both attack tests (the reload of the
  count after the array store). Users boss_ai, level_scripts_common, level_script_khosanilab2, living_entity, the
  Ghost Rider and Titus units: identical address lists before / after.

## What worked (D6702 has the full list)

- Element pointers `b` / `d` in the button loaders, draw loops, click handlers (pickup, options, levelselect);
  index loops instead of pointer loops for `jl`.
- `if (!mouse_is_down(0)) { release } else { held; return; }` for *_update_buttons.
- Clamp `last = n - page; if (pos < last) last = pos; pos = last;`.
- Call arguments that are calls through a local (`move_info *m = find_move(..)`, `int music = ..`,
  `char *fmt = XSTR(..)`).
- `vector3 v = a - b; v.len()` for the worldmap distance searches (5 functions); in-place `vector3 d(x, 0, z)`,
  `d * (2.0f / 3.0f)`, `pos + c->centre_offset` through the inline operators.
- Bool shapes: `f() ? true : false`, `x > 0 ? true : false`, `if (f() == 0) return true; return false;`, one `||`
  condition for a shared `return true`.
- Statement / store order (pickup_init, open_dialogue, idle_path, pick_encounter_early, puzzle_use), a global as
  the counter (`levels[count[1]++] = i`, 8168 promotes it), `int n = num_lobjs` before a countdown loop.
- Frame sizes: pickup_take_selected's msg is 128 bytes (D6704), level_load_layers' name 32 bytes; block-scoped
  buffers (closer for s3d_read_navpoint / s3d_read_geometry).
- options_set_texture_quality: `case 0` and `case 1` with separate bodies give the `sub eax, 0 / dec / dec` chain.

## What still differs (D6703)

- pickup: page_up (max AND register), draw_item_list (ebx / ebp swap of x and i; no declaration order changed it).
- options: load_bitmaps (switch range test `jg` vs `ja` on the pointer), init (8168 forwards Summoner_options past
  the struct copy, also with memcpy), drag_sliders (float width copied through an int temporary).
- khosani: trace_beam and the three corner_* (dead argument slots reused), choose_turn (register allocation),
  emit_segment (shared return block, float copies through the FPU; the four fabs are now fabsf, closer).
- gameplay: level_load_report / level_unload / help_draw_page (stack pops across varargs), party_level_reset
  (callee-saved pushes and CF_SUMMON constant register), state_game_enter (shared failure tail), game_render_frame
  (`not al / shr al, 5` byte narrowing; the fog_near local fixed the rest), state_game_frame (static guard
  scheduling, matrix operator*).
- conversationctl: line_talker (registers), the camera functions (vector3 constructor out of line, float
  scheduling), end / select_topic (bitfield `shr / test cl, 1`), end_cleanup (frame, copy elision).
- tentacle: select_action, boss_load, ai_frame (shared return blocks), frame_hook, cmd_tentacles / cmd_tentacle_test
  (constant 8 in EBP, store scheduling).
- s3d: read_navpoint (quaternion copied through EAX, frame now right), read_geometry (loop index in an argument
  slot, frame now right), level_free_s3d (palette pointer in an argument slot), read_color_record / read_door /
  read_textured_record / read_mesh_ref / load_lightmap / read_meshes / level_load_s3d (scheduling, FPU copies,
  argument order around an inner call).
- worldmap: frame (float members through the FPU), stage9 (8966 block placement), stage15 (8966 reuses two of the
  four float slots).
- pyrul: ai_frame and cast_special (case -1 table slot; 8168 merges cast_special's `return true`), choose_attack and
  fire_attack (frames 0x6c / 0x60 against 0x68 / 0x9c).
- levelselect: handle_click (frame 0x10c vs 0x110), drag_scrollbar (8168 canonicalises the order of the thumb sum,
  every source order gives the same code), draw_levels (late pushes), add_chat_line (max AND register).

## Logic errors / game bugs

No behaviour change was needed in this pass: every rewrite computes what the old source did (or, for
emit_segment's fabsf, the float compare the binary does). Known game bugs already in the notes stay as they were
(s3d_read_color_record's blue / green mix-up, tentacle squeeze heal, level_free_s3d's counts). Markers that say
exact but differ under 8168 are listed in D6705 and left as they were.

## Tools written for this pass (scratchpad only)

`cmp.mjs` / `c.sh` (side-by-side listing of compiled and original code per function), `sweep.mjs` (all units, 4 at
a time, per-address status), `repl.js` (line-range replace), `markers.js` (raise markers of new matches),
`wrap.js` (wrap long marker lines), `pyrulhdr.js` (the boss_ai.h edit).
