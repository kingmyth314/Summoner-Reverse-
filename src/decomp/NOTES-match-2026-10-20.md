# Byte-match round 2026-10-20: the fifteen workers' notes

Merged into src/decomp (5,077 -> 5,255 strict matches; DECISIONS D11000-D12499). One section per worker, as written.

---

## Worker 1

## Worker 1 (m-1020)

Strict sweep of the 14 files: 235 -> 250 matches (14 of the 57 differing functions now match, plus one new
function, 0x50ab70). No match lost (`--check src/decomp/vc6-matches.txt`, own files and the full tree after the
header change). Decisions: D11000-D11006 in w1-src/include/DECISIONS.md.

## Per file (strict matches before -> after / compared)

| File | Before | After | New matches |
|---|---|---|---|
| game/gamesound.cpp | 15/16 | 16/16 | 0x468e30 surfaces_parse_table |
| game/new_buysell.cpp | 38/42 | 40/42 | 0x405790 buysell_page_up, 0x405fd0 buysell_draw_item_list |
| game/new_itemtrade.cpp | 30/36 | 34/36 | 0x419bb0 party_page_up, 0x41a360 draw_list1, 0x41a490 draw_list2, 0x41a5a0 draw_party_list |
| game/unnamed_alt_textures_0x4aac10.cpp | 3/4 | 3/4 | (closer) |
| game/unnamed_fx_0x489c40.cpp | 48/57 | 48/57 | (fx_create_spell_effect_facing closer) |
| game/unnamed_shadows_0x4a37d0.cpp | 7/13 | 7/13 | |
| game/unnamed_snow_0x4946a0.cpp | 9/11 | 10/11 | 0x494ba0 snow_update |
| vsdk/console.cpp | 16/17 | 18/18 | 0x50ab60 init_char_types, 0x50ab70 build_char_types (new) |
| vsdk/gr_font.cpp | 14/17 | 15/17 | 0x508c60 gr_string |
| vsdk/unnamed_exceptionhandler_0x5419a0.cpp | 6/8 | 8/8 | 0x5419a0 RecordExceptionInfo, 0x541fb0 ShowModuleInfo |
| vsdk/unnamed_movie_0x5172f0.cpp | 1/2 | 2/2 | 0x5172f0 movie_play |
| vsdk/unnamed_quaternion_0x5225b0.cpp | 21/32 | 21/32 | (cquat exp / exp_in_place / log_in_place closer) |
| vsdk/unnamed_sound_emitter_0x546150.cpp | 22/23 | 23/23 | 0x5467c0 sound_emitter_delete |
| vsdk/unnamed_vfx_emitter_0x55ff00.cpp | 5/14 | 5/14 | |

Confidence raised to exact on all 15 (comments name the form and the decision).

## What worked

- **Local names decide the frame slots of an EBP frame under /Ox (D11000).** The exception handler's two __try
  functions differed only in slot offsets; D6101's name hash applies to EBP frames (SEH) at /Ox too, not only
  /Od. The unit follows Dawson's ExceptionHandler.cpp, so its local names (and point-of-use declarations) gave
  the binary's frames. Renaming does not move /O2 ESP-frame slots (checked on gr_font_create).
- **Early return in EH functions (D11001):** surfaces_parse_table (`if (!file.find(..)) return;` keeps 0 in EBX
  and all pushes in the prologue), movie_play (early return, then the path buffer declared after it).
- **`je ret / jmp next` is a tail call (D11002):** init_char_types jumps to a separate function at 0x50ab70 (no
  symbol); written as `build_char_types()`.
- **Header: gr_string / gr_string_at_text_pos take gr::Mode by value (D11003)**: the binary passes the mode with no
  Mode constructor call; gr_string now matches.
- **Re-reading a global array element instead of caching it in a local (D11004)**: snow_update's centre cell.
  Plus float locals for a sum of squares whose load order 8966 otherwise sorts (strict target difference).
- **List drawers (D11005):** `rows = count < page ? count : page` (stored on both paths), the row index in a
  local, `y += fh, i++` in the for header; page_up with D10600's `&= (pos <= 0) - 1`.
- **Unlink through the element's own fields** (sound_emitter_delete) instead of prev / next locals.

## Closer, still differing (kept, behaviour identical)

- alt_textures_parse_table: TextFile declared after the table reset (the binary constructs it there); remaining:
  EBX / EBP pushed in the prologue in the binary, shrink-wrapped in ours (early return duplicates the tail).
- fx_create_spell_effect_facing: `vector3 *tp = &t->pos; dir.set(c->pos.x - tp->x, 0, c->pos.z - tp->z)`; remaining:
  the right vector's x (binary `fld 0 / fld dir.z / fsubr`, ours `fld 0 / fld st0 / fsub dir.z`).
- cquat::exp, exp_in_place, log_in_place: interleaved `fx, sx, fy, sy, fz, sz` declarations (as cquat::log);
  remaining: the binary keeps the square root on the FPU stack (`fstp st(3)`, `fst [esp]`).

## Still differing (tried; D11006)

itemtrade_count_party_rows (re-read of party_gold, inc in memory; also D11305's volatile store: the store then stays above the party_gold load, but the increment is still done in EAX, 6 volatile forms), buysell_count_party_items (max-0 AND register,
26 forms), snow_start (15 loop shapes; closest: a pointer to the record row), quaternion to_matrix / _b (factor
order within each product; source factor order, statement order and 24 local-copy orders tried), quaternion::multiply
(all 24 store orders of r), quaternion::exp (fst / fstp st(3) of the sqrt), init_particle (fild + faddp of
desc->frames after the call; 13 forms), fx_create_spell_area (prologue pushes), fx_create_at_camera (frame 0x3c:
one vector3 temporary more), fx_update_missile (the binary calls vector3 operator+ / operator= / the copy
constructor out of line in this function; ours expands them even written with operators), buysell /
itemtrade_handle_click (constant registers: binary keeps -1 in EBP, ours 4 in BL), shadows_render (lobj_count loop
counted down with the count in memory), get_fade (sub esp before the first fld), and the large fx / vfx / shadow /
quaternion functions.

## Behaviour errors / game bugs

- No behaviour error found in the matched functions.
- snow_update: the binary reads snow_cells[7][7] again for the z tests after an x scroll (the old code kept the cell
  read before the scroll). The new centre cell has the same z, so the result is unchanged; the source now does what
  the binary does.
- Game bug confirmed by the byte match: snow_update's +x row scroll refills a cell when snow_cell::update returns
  true (the other three directions when it returns false), already in NOTES.md (instructions in the 0x494ba0 body;
  the source matches byte for byte).
- sum.symbols has no entry for 0x50ab70 (the character class table builder reached by init_char_types' tail jump).

## Header change

include/vsdk/gr/gr_font.h: gr_string(.., gr::Mode mode) and gr_string_at_text_pos(.., gr::Mode mode) (were int),
plus `namespace gr { class Mode; }`. All .cpp files compile with mingw (only unnamed_decals' known 9 errors); full
strict sweep: no match lost.

## Files changed (in w1-src)

game/gamesound.cpp, game/new_buysell.cpp, game/new_itemtrade.cpp, game/unnamed_alt_textures_0x4aac10.cpp,
game/unnamed_fx_0x489c40.cpp, game/unnamed_snow_0x4946a0.cpp, vsdk/console.cpp, vsdk/gr_font.cpp,
vsdk/unnamed_exceptionhandler_0x5419a0.cpp, vsdk/unnamed_movie_0x5172f0.cpp, vsdk/unnamed_quaternion_0x5225b0.cpp,
vsdk/unnamed_sound_emitter_0x546150.cpp, include/vsdk/gr/gr_font.h, include/DECISIONS.md.

---

## Worker 2

### m-1020 worker 2 notes

Tree: `m-1020/w2-src`. Compiler: SP5 VC98 (cl 12.00.8966), strict. Before / after: `w2-before.txt` /
`w2-sweep.txt`. **255 -> 264 of 311 functions; none lost** (checked address by address). No header and no flags
change. mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in every changed .cpp except
game/unnamed_decals_0x4bbe80.cpp (9, the pre-existing "address of a temporary" errors, same as base).
Decisions D11100-D11104 appended to `w2-src/include/DECISIONS.md`.

## Per file (matches before -> after / total)

| file | before | after | won |
|---|---|---|---|
| game/pathfinding.cpp | 22 | 23 / 36 | 0x49f650 ai_path_smooth |
| vsdk/unnamed_vector_0x523f30.cpp | 42 | 43 / 51 | 0x524b50 vector3_rotate_about_axis |
| game/unnamed_ai_combat_0x4bce60.cpp | 21 | 21 / 28 | (combat_toggle_ready_stance closer) |
| game/unnamed_decals_0x4bbe80.cpp | 9 | 12 / 15 | 0x4bc640 decals_reset, 0x4bcbb0 decals_collect_for_lobj, 0x4bcc20 decals_recycle_unused |
| game/unnamed_serpent_0x495290.cpp | 38 | 38 / 43 | (serpent_illusion_update closer) |
| vsdk/targautils.cpp | 17 | 18 / 21 | 0x555110 tga_read_header |
| game/unnamed_luminar_0x4c8d10.cpp | 20 | 21 / 23 | 0x4c9da0 luminar_teleport_out |
| vsdk/unnamed_vfx_mesh_0x562a30.cpp | 7 | 7 / 9 | |
| game/unnamed_murod_0x4c5140.cpp | 10 | 12 / 12 | 0x4c51a0 murod_start_leap, 0x4c57b0 murod_ai_select_action |
| game/items.cpp | 29 | 29 / 31 | (items_parse_table 888 -> 29 differing lines) |
| vsdk/cmdline.cpp | 15 | 15 / 16 | |
| game/unnamed_cutscene_0x434900.cpp | 25 | 25 / 26 | (cutscene_parse_table 213 -> ~50) |

All 9 won functions are `Confidence: exact` with the shape named in the comment.

## What worked

- Loop entry tests (D11100): `if (head) while (d)`, a top-tested `while (d)` with a `next != head ? next : NULL`
  step, and a do-while where the pointer is known non-NULL.
- Block placement (D11101): 8966 lays out a `goto found` / `goto fail` pair and then-branch early returns
  differently from the binary. A found flag + `break` with `if (!ok) { fail; return; }`; the main path in the
  then-branch with `else return X;`; one flag so two failures share one out-of-line return.
- Float argument through a float local (D11102, as D10307).
- The coordinator's volatile-store device for a store the binary keeps above the next one (D11103).
- For large functions: an operand-level distance tool (copied from m-1019's w9 `dist.mjs`, with fixes for
  `rep` lines, negative immediates, scaled-index order and EH lines) plus cumulative variant application found
  many independent fixes in one function (items_parse_table: about 10).

## Closer, still differs (D11104)

items_parse_table (case-block register choice), cutscene_parse_table (/Od register rotation one step off from
the $Fog increment), serpent_illusion_update (8966 folds `x / -1000` into a subtraction; the `rem <= 300`
re-test is threaded away), combat_toggle_ready_stance (prologue order, `ready` in AL). The others listed in
D11104 were tried without result.

## Behaviour errors / game bugs

None found. All changes are same-behaviour reshapings (murod_start_leap / luminar_teleport_out keep the same
five tries; tga_read_header the same return values; the volatile in decals_reset and
combat_toggle_ready_stance only fixes store placement).

## Files changed (in w2-src)

game/items.cpp, game/pathfinding.cpp, game/unnamed_ai_combat_0x4bce60.cpp, game/unnamed_cutscene_0x434900.cpp,
game/unnamed_decals_0x4bbe80.cpp, game/unnamed_luminar_0x4c8d10.cpp, game/unnamed_murod_0x4c5140.cpp,
game/unnamed_serpent_0x495290.cpp, vsdk/targautils.cpp, vsdk/unnamed_vector_0x523f30.cpp, include/DECISIONS.md.
Helper scripts (scratch, not for the repo): `m-1020/w2t/` (mytry.mjs, dist.mjs, apply.mjs, triage.mjs, rename.mjs).

---

## Worker 3

### m-1020 worker 3 notes

Tree: `m-1020/w3-src`. Compiler: VC6 SP5 (cl 12.00.8966), strict sweep (`VC6_STRICT=1`).
Sweeps: `w3-before.txt` (300 match / 56 differ) and `w3-after.txt` (**308 match / 48 differ**), checked with
`vc6-sweep.mjs --check w3-before.txt`: **0 lost**. mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in the
three changed files. No header changed. Decisions D11200-D11205 appended to `w3-src/include/DECISIONS.md`.

## Per file (match before -> after, of compared)

| file | before | after | won |
|---|---|---|---|
| vsdk/gr_opengl.cpp | 35 | 41 / 54 | 0x54d630, 0x54dda0 (D11200), 0x54deb0, 0x54df20 (D11201), 0x54f410, 0x54f8c0 (D11202) |
| vsdk/unnamed_lights_0x51e070.cpp | 35 | 37 / 41 | 0x51e970 lights_select_nearest, 0x51f3b0 lights_shade_vertices (D11203) |
| vsdk/bmpman.cpp | 47 | 47 / 56 | |
| game/s3d.cpp | 28 | 28 / 34 | |
| vsdk/unnamed_floatmath_0x542500.cpp | 16 | 16 / 20 | |
| vsdk/unnamed_vfx_channel_0x55b440.cpp | 16 | 16 / 19 | |
| game/gameplay.cpp | 36 | 36 / 39 | |
| game/unnamed_ghost_rider_0x4b9e70.cpp | 36 | 36 / 38 | |
| game/multispell.cpp | 20 | 20 / 22 | (not attempted: w6 already logged register choice) |
| game/unnamed_titus_0x4c20e0.cpp | 26 | 26 / 27 | (not attempted: block order, w1 tried) |
| game/unnamed_cutscene_script_0x4ed050.cpp | 5 | 5 / 6 | |

Confidence raised to exact for the eight new matches; nothing else changed confidence.

## What worked (details in D11200-D11204)

- Resampling converters: the destination row as `(uint *)dst + y * tex_w` per row (the binary's induction
  variable set inside the guard), padding `for (x = bm_w; x < tex_w; x++)`, padding rows indexed from y; the
  zero stores in the binary's register order (b, g, r, a).
- A loop whose pointer the binary loads only after the outer count test: `int y = 0; if (tex_h > 0) { T *d =
  dst; do { ... y++; } while (y < tex_h); }` (two per-texel converters; makes the three others worse).
- glOrtho argument operand orders found by enumerating all 144 (generated variant list for vc6-try).
- A block-scoped PIXELFORMATDESCRIPTOR (frame 0x44 -> 0x3c: shares the ortho temporaries' slots).
- Reading a global index once into a local (`int cur = lights_selection_depth - 1`) and the store order of two
  counters; `int p = depth - 1` before `d = depth++`.
- A signed bit field compared through `int type = l->type` (movsx / sar / cmp eax) or a one-case switch
  (`sub eax, 2 / jne`).
- `p == NULL ? a : b` to get the binary's branch layout of a pointer-test ternary (8966 always lays the first
  arm out first).
- Splitting `s = float_max(expr, 0)` into `s = expr; s = float_max(s, 0)`; `(s * s) * atten * intensity`.
- Named locals for call results that the binary compares with its zero register (`cmp eax, ebx`).

## Closer, still differs (kept; behaviour identical)

- gl_convert_4444_to_rgba8888 / 1555_to_rgba8888 / 1555_to_rgba8888_scaled: frames and loops now right; the
  binary forms the texel address with a `lea` (dead in 0x54da80) before the load. 0x54da80's alpha is an int.
- gl_convert_a8_to_rgba8888: `y++, src += bm_w` increment; tex_w / bm_w registers swapped.
- lights_shade_point: frame 0xb0 now right, ternaries right; the slot of `dot` and the scheduling of ang_u's fstp.
- lights_calc_point_specular_color: down to 6 mnemonic differences (where the `2.0f * *normal` temporary's
  address is loaded relative to the dot product's fstp; gr_env_map_uv has the same one).
- cutscene_script_load: the binary places the shared failure tail after the level-name test.

## Still differs, tried (D11205)

gr_opengl_apply_tint (sum kept on the FPU), gr_opengl_read_backbuffer (two row pointers), bm_load_internal
(`reduced` in BL), bm_lock_pcx (`[8 * esi + field]` addressing), ghost_anim_remap_in (zero register, `ready` in
the dead c slot), state_game_enter (the default case placed after the failure block), state_game_frame (float
locals copied with integer moves), float_format_buf (rem tested from memory after an integer copy),
s3d_read_mesh_ref (hi read as a dword and masked), s3d_read_color_record / s3d_read_door (as D7702),
light_create_spot (fstp scheduling), gl_convert_888_to_rgba5551 / 8888_to_rgba4444 (register choice). Not
attempted in depth: gl_build_mipmaps (frame 0x10 vs 0x34), gr_opengl_create_texture, set_mode, bitmap, tmapper,
bm_read_header, bm_lock_tga / bm_lock_vbm / bm_free_data / bm_release_from_mark / bm_set_reduced, level_load_s3d,
s3d_read_geometry / s3d_read_meshes (the binary keeps `untextured` in a frame byte), vfx_channel (bit fields),
fx_rec4c, ghost_find_floor_below, game_render_frame, multispell, titus.

## Behaviour errors / game-bug candidates

No behaviour error found. One semantic alignment: the resampling converters' row padding now starts at x = bm_w as
in the binary (0x54d6f6-0x54d70c: `mov esi, [bm_w]; ...; cmp esi, edx; jge; sub ecx, esi; rep stosd`; with bm_w <= 0 the texel loop is skipped by `jle 0x54d702`, straight to that compare); the old source
continued from the texel loop's x, which differs only for a negative bm_w (not reachable).

## Files changed (in w3-src)

- vsdk/gr_opengl.cpp
- vsdk/unnamed_lights_0x51e070.cpp
- game/unnamed_cutscene_script_0x4ed050.cpp
- include/DECISIONS.md (D11200-D11205)

---

## Worker 4

### Worker 4 notes (match round 2026-10-20)

Strict VC6 sweep (cl 12.00.8966, VC6_STRICT=1, the tree's vc6-flags.txt, unchanged) of the 12 units:
**313 -> 329 matching functions (+16), none lost** (`vc6-sweep --check w4-before.txt`: 0 lost; lists
`m-1020/w4-before.txt` and `m-1020/w4-sweep.txt`). No header changed, no flags changed. mingw `-fsyntax-only -std=c++98
-w -Iinclude`: 0 errors in all 8 changed files. Decisions D11300-D11311 appended to `w4-src/include/DECISIONS.md`.
Every new match has `Confidence: exact` and the shape named in its comment.

## Match counts per file (before -> after / compared)

| file | before | after | won |
|---|---|---|---|
| game/new_pxochat.cpp | 46 | 53 / 57 | 0x422980 build_popup_text, 0x422b40 add_typed_channel, 0x422fc0 init, 0x423050 update_scrollbar, 0x4248c0 draw_channel_list, 0x424a60 draw_user_list, 0x425270 add_chat_line |
| vsdk/gr_glide.cpp | 51 | 53 / 61 | 0x549950 glide_record::evict_block, 0x549d10 gr_glide_init |
| vsdk/unnamed_os_0x530b90.cpp | 20 | 22 / 23 | 0x53171f PrintSystemInfo, 0x531aa8 cmd_sysinfo |
| game/conversationctl.cpp | 19 | 20 / 27 | 0x4dacb0 select_topic |
| game/level_script_worldmap.cpp | 20 | 21 / 23 | 0x4fbd80 ls_worldmap1_frame |
| game/unnamed_frame_0x4323e0.cpp | 19 | 20 / 21 | 0x4324f0 screenshot_save |
| game/new_assess.cpp | 16 | 17 / 18 | 0x401a20 assess_page_up |
| vsdk/parse.cpp | 45 | 46 / 51 | 0x515ee0 TextFile::Error |
| vsdk/window.cpp | 35 | 35 / 40 | |
| vsdk/unnamed_inputbox_0x5521a0.cpp | 9 | 9 / 13 | |
| game/unnamed_swi_0x488230.cpp | 13 | 13 / 14 | |
| game/unnamed_dialogue_table_0x4feb50.cpp | 20 | 20 / 21 | |

## What worked

- D10600's in-place clamp on the global, both polarities: `x &= (x <= 0) - 1` (setle) and `x &= (x < 0) - 1`
  (sets) - assess_page_up, pxochat_init, pxochat_add_chat_line (D11300).
- /Od frames by the D6101 / D7003 name-bucket rule: rename locals so their buckets follow the binary's slot order
  (PrintSystemInfo: 19 locals; cmd_sysinfo: `buf` -> `buffer`) (D11301). A small script computing buckets made this
  mechanical.
- Values read once into locals where the binary keeps them across a store (evict_block) (D11302).
- Parenthesised float temporary for a float argument the binary passes through fld / fstp (ls_worldmap1_frame) (D11303).
- Ternary call argument -> if / else of two calls (screenshot_save) (D11304).
- **New: a volatile store.** 8966 sinks plain global stores below the next call's argument pushes; the binary keeps
  glide_init_done's stores (and the stores before them) above. `*(volatile int *)&glide_init_done = ...` reproduces
  it without a header change (gr_glide_init) (D11305). Worth trying elsewhere for "store above the pushes" diffs.
- `(bool)a || (bool)b` for two bit-fields of one word that the binary tests separately; inline max / min macros
  instead of the out-of-line imax / imin; array indexing instead of an element pointer (select_topic) (D11306).
- Statement shapes: the found case handled inside the search loop (add_typed_channel), `len = 0` after the early
  return and `len += n` before the call that uses n (build_popup_text) (D11307).
- Row count as an if / else of two assignments instead of assign-then-clamp (both list draws) (D11308); the same
  `rows >= page` shape exists in buysell / charselect / gamelist / inventory / itemtrade (other workers' files).
- Read order / branch polarity / operand order for an FPU CSE (update_scrollbar) (D11309).
- Index the member directly instead of through a pointer copy (no strength reduction), and the loop condition
  order / increment order of a rotated loop (TextFile::Error) (D11311).

Method note: a register-aware diff (normalising both listings, comparing registers too, counting differing lines) was
much more useful than vc6-side's mnemonic stars for ranking variants; scripts in `m-1020/w4-try/` (rd.mjs: try
variants and print differing lines; rdall.mjs: every differing function of a unit; locals.mjs, bucket.mjs,
permdecl.mjs). Heredocs in Git Bash halved backslashes in JSON variant files: write them with the Write tool.

## Closer, still differs (kept, same behaviour)

- os_wndproc: walking handler pointer (`os_msg_handler *h`) gives the binary's prologue and loop; differs at the
  cross-jumped SetThreadPriority call.
- glide_record::load_section: largeLod before smallLod, u / v float locals; differs in the schedule of v's int copy.
- conversation_controller::frame: `if (r != 0 || cam_mode == 3) return r;`; differs in the frame size (0x48 vs 0x40).
- pxo_parse_chat_banners_cfg: num_found after the fill; only a whole-function EBX / EBP swap left.

## Still differing (D11310)

send_event (void / int callback: header question across the ui units), game_is_paused (byte-register shift),
register choice (page_down, drag_scrollbar, ParseInt / ParseUInt / ParseString, file_write_text_block,
file_blast_into_buffer, line_talker, camera_begin / camera_update, ui_window::init / parse_* / handle_arrow_key,
glide alloc / tcache_set / create_texture*, assess_format_info, banners_cfg), block placement (worldmap stage9),
/O2 frame slots (stage15, handle_click, conversation frame, gr_glide_bitmap / tmapper_multitexture),
set_valid / set_invalid_chars (shared lo slot, in-place memsets), camera_capture / compute_shot / blend (out-of-line
vector3 constructor), inputbox process / draw, mp_handle_swi_path, conversation::parse_table.

## Behaviour errors / game-bug candidates

None found: every change is a same-behaviour reshaping (checked each against the binary; the frame change returns r,
which is 0 on that path). ui_window::send_event: the binary returns the callback's result, or the NULL callback
pointer (0) when there is none, so callers see 0 either way; the source's `bool` return is a declaration question,
not a behaviour error.

## Files changed (in w4-src)

- game/new_pxochat.cpp
- vsdk/gr_glide.cpp
- vsdk/unnamed_os_0x530b90.cpp
- game/conversationctl.cpp
- game/level_script_worldmap.cpp
- game/unnamed_frame_0x4323e0.cpp
- game/new_assess.cpp
- vsdk/parse.cpp
- include/DECISIONS.md (D11300-D11311 appended)

---

## Worker 5

### m-1020 worker 5 notes

Tree: `m-1020/w5-src`. Compiler: VC6 SP5 (cl 12.00.8966), strict. Before: `w5-sweep-before.txt`, after:
`w5-sweep-after.txt` (my 11 units) and `w5-sweep-full.txt` (whole copy).
**My 11 units: 222 -> 234 of 278 (+12), none lost.** Whole-copy strict sweep: 5089 match, `--check
src/decomp/vc6-matches.txt`: 5077 -> 5089, 0 lost. Decisions D11400-D11403 in `w5-src/include/DECISIONS.md`.

## Per file (match before -> after / total)

| file | before | after | won |
|---|---|---|---|
| game/unnamed_avoidance_0x4a6710.cpp | 45 | 49 / 67 | 0x4a7210 poly_nearest_edge_hit, 0x4a7e30 calc_corner_offset, 0x4a8060 calc_shared_corner_offset, 0x4a82a0 ensure_corner_offset |
| game/unnamed_pfgeom_0x4a4ee0.cpp | 17 | 19 / 24 | 0x4a4f60 seg2d_point_left_of, 0x4a5c60 pf_bsp_node_find |
| vsdk/unnamed_gr_3d_0x50d9b0.cpp | 33 | 35 / 39 | 0x50e150 gr_setup_frustum, 0x50ea00 gr_offset_point_depth |
| vsdk/unnamed_localize_0x503f60.cpp | 23 | 23 / 28 | |
| vsdk/unnamed_key_0x5093a0.cpp | 22 | 23 / 26 | 0x509560 key_remap_layout |
| vsdk/unnamed_linefile_0x558340.cpp | 11 | 12 / 14 | 0x5587c0 LineFile::write |
| game/new_options.cpp | 32 | 32 / 35 | |
| game/unnamed_experience_0x489930.cpp | 6 | 7 / 8 | 0x489a60 xp_for_level |
| game/new_credits.cpp | 7 | 7 / 9 | |
| game/unnamed_murod_oni_0x4c5c20.cpp | 13 | 14 / 14 | 0x4c5de0 murod_oni_ai_select_action |
| game/unnamed_gamelist_0x4705b0.cpp | 13 | 13 / 14 | (mp_handle_actv_gm closer) |

All 12 won functions are `Confidence: exact`.

## What worked (D11400, D11401)

- **vector2 temporaries and in-place construction** (the big one in avoidance / pfgeom): `d = a - b` through
  operator-, `vector2 n(d.y, -d.x)` constructed in place and copied whole, `vector2 bis = n2 + n1`, an edge normal
  `vector2 n(...)` built before the dot product. Wins: both miter functions, seg2d_point_left_of, pf_bsp_node_find.
- **One float reused** where the binary has one slot (the negated cosine and the miter scale), float casts on
  asin so the PI / 0.5 constants are float (`fsubr dword`).
- **Assignments instead of returns** to get the binary's cross-jumped value blocks (key_remap_layout; 64 combinations
  searched with a scoring script, `w5tools/trysc.mjs`, which prints the first differing offset per variant).
- **A closed form for a stepped sum** (xp_for_level: `50000 + (i - 20) * 10000`).
- **Positive tests / nested ifs with the failure last** (murod_oni select, LineFile::write).
- **Indexing instead of a pointer local** (ensure_corner_offset), the polygon edge loop `j = i++` without an early
  return (nearest_edge_hit), a float re-read through a local (offset_point_depth), `-hx` / `-hy` locals and an
  operand order named by the strict check (gr_setup_frustum).
- Tool tip: `vc6-compare --json` gives `offset` of the first difference; scoring every variant by it finds the
  direction much faster than reading side-by-side listings (`w5tools/trysc.mjs`, `gen_*.js`).

## Closer, still differing (kept, behaviour identical)

- mp_handle_actv_gm: `e->ping &= (e->ping <= 0) - 1` gives the binary's frame (0x40) and the version in BL; the
  list unlink through `&e->prev` and the AND's register remain.
- gr_setup_3d: no `h` local and the tangent through `float a = fov * GR_DEG_TO_RAD` (now to +0x121); the two Glide
  tests are threaded into one by our build.
- avoid_sort_refs_along_segment: parameter order fixed (D11402) and `fabsf(dx) > fabsf(dy)`; EBX / EBP swapped.

## Still differing (D11403 lists what was tried)

Register / scheduling: xp_level_up_gains, options_init, options_load_bitmaps, credits_parse_table,
localize_set_string, key_clear, key_mark, key_flush, avoid_sort_refs, seg2d_point_near_middle, gr_get_frustum_planes.
Float kept in memory / FPU-stack choices: credits_draw_lines (alpha), avoid_circle_contains_point (r * r),
pf_first_edge_crossing (dx * dx once from memory), gr_project_vertex (flags reloaded for the final return),
options_drag_sliders (slider width copied once more). Structure not found: LineFile::detect_eol (`cr > cr + 1` test),
gr_cull_bounding_box_transformed (no plane constructor call), avoid_do_poly_seg_list_links (frame right with a vector2
temporary, the rest not), the large avoidance / pfgeom / localize functions (frames, EH) not attempted.

## Behaviour errors / game bugs

- No game-logic error found. Two declarations fixed (header changes, behaviour unchanged):
  - avoid_sort_refs_along_segment takes (count, refs) (D11402; callee 0x4a8aa6, caller 0x4a8c1a-0x4a8c22).
  - avoid_seglist_calc_corner_offset's swap is an int (D11401; read as a dword at 0x4a7eeb).
- Constant fix: PI_F in the avoidance unit was 3.141593f (0x40490fdc); the binary's 0x57f028 is 0x40490fdb. The
  miter angle now uses the binary's value (a last-bit difference in the miter scale before).
- Rounding now as in the binary (were tiny differences): gr_setup_3d's tangent argument (two multiplies, not one
  folded constant), the miter functions' half angle in float.
- Confirmed, not changed: pf_first_edge_crossing's `dx * dx + dx * dx` (the y term is missing in the binary too,
  the documented game bug).

## Header change

- include/engine/ai/unnamed_avoidance_0x4a6710.h (D11401 swap int, D11402 parameter order). Only user:
  game/unnamed_avoidance_0x4a6710.cpp. mingw `-fsyntax-only` over all 259 .cpp: only unnamed_decals_0x4bbe80.cpp
  (9 errors, as before). decomp-sigcheck: only the known charinfo_load_move_anim. Full VC6 strict sweep of the copy:
  0 lost.

## Files changed (in w5-src)

- game/unnamed_avoidance_0x4a6710.cpp
- game/unnamed_pfgeom_0x4a4ee0.cpp
- game/unnamed_experience_0x489930.cpp
- game/unnamed_murod_oni_0x4c5c20.cpp
- game/unnamed_gamelist_0x4705b0.cpp
- vsdk/unnamed_gr_3d_0x50d9b0.cpp
- vsdk/unnamed_key_0x5093a0.cpp
- vsdk/unnamed_linefile_0x558340.cpp
- include/engine/ai/unnamed_avoidance_0x4a6710.h
- include/DECISIONS.md (D11400-D11403)

---

## Worker 6

### m-1020 worker 6 notes

Strict VC6 8966 sweeps of my 11 units: **194 -> 207 match (+13), 0 lost** (`w6-sweep-before.txt` /
`w6-sweep-after.txt`). Full-tree strict sweep of the copy (header change): 5090 match, 0 lost against
src/decomp/vc6-matches.txt (`w6-full-after.txt`). Decisions D11500-D11503 in w6-src/include/DECISIONS.md.

## Per file (match before -> after, of total)

| File | Before | After | Won |
|---|---|---|---|
| game/unnamed_render_scene_0x481680.cpp | 55 | 61 / 77 | 0x482110 level_set_current_layer, 0x482850 fog_enable, 0x482d70 setup_level_lights, 0x4843e0 lamp_glow_update_lobjs, 0x484600 lamp_glow_update_effects, 0x4868f0 detail_levels_parse_table |
| vsdk/unnamed_http_0x55a8e0.cpp | 10 | 13 / 15 | 0x55a9a0 init, 0x55ae20 connect_socket, 0x55b0d0 get_line |
| game/new_dialogue.cpp | 4 | 5 / 6 | 0x409650 dialogue_draw |
| game/new_spell.cpp | 35 | 36 / 38 | 0x42a270 spells_handle_click |
| game/unnamed_ai_think_0x4b5140.cpp | 22 | 23 / 29 | 0x4b5160 group_assign_formation |
| game/unnamed_mpmenus_0x43eb00.cpp | 13 | 14 / 14 | 0x43edc0 mp_host_game |
| game/unnamed_effectstream_0x440400.cpp | 12 | 12 / 14 | |
| game/unnamed_giant_salamanka_0x4c8030.cpp | 18 | 18 / 19 | |
| vsdk/unnamed_gr_line_0x53e520.cpp | 13 | 13 / 19 | |
| vsdk/unnamed_d3d_tcache_0x54bb30.cpp | 11 | 11 / 14 | |
| vsdk/unnamed_poly_split_0x55f110.cpp | 1 | 1 / 5 | |

## What worked (details D11500)

- Constant-first float compares and a float local read once (fog_enable).
- `(bool)(~x & 1)` / `(bool)((x >> n) & 1)` and `!(bool)bitfield` for the binary's dword-load bit tests.
- Ternary min clamp into a global (mp_host_game).
- A global store written before an object's declaration (detail_levels_parse_table; render_scene's light).
- `while (1) { if (call != ERR) break; ... }` (not `for (;;)`), store order, chained assignment before strcpy.
- Vector operator forms (`ray.dir = eye - c; ray.dir.len_sq()`, `p = vector3(base->pos)`), an instance pointer local
  before effect_get_base.
- **/Ob0 units: pass `gr_string_mode.value`**; gr::Mode's inline conversion is otherwise an out-of-line call. Other
  /Ob0 game units calling gr_string with a gr::Mode global may win the same way (tip for other workers).
- Sibling blocks each with their own `int w, h;` (slot layout per block); case bodies in binary layout order;
  operand order of a sum of two fields.
- Coordinator tips: the volatile store did not help effect_stream_start (the 1 is CSE'd with the return value);
  no row-count clamp in my files.

## Still differs (D11503)

Closer and kept: levelrender_render_scene (first 146 instructions now match), dialogue_frame (only the final mode
update / shared epilogue left), levelrender_effect_apply_vis (two spill slots swapped), pick_attack_target,
giant_salamanka_ai_select_action, dialogue_frame, the sky loop. Not reproduced: level_vis_reset, effect_stream_start,
goal_do_travel, character_use_skill_on, spells_set_page (scheduling), http_async_gethostbyname (one push scheduled
before a store), overhead_label_place / gr_poly / add_items (register or FPU allocation), gr_bitmap_3d_projected,
add_effect_entry (48 dot orders tried), add_effects, level_vis_parse, the large draw_list / build_draw_lists /
copy_bitmap_to_surface / poly_split functions (not attempted in depth).

## Behaviour errors / game bugs

- **Fixed (D11502):** lamp_glow_update_effects read the glow point from +0x48 of the node record; the binary reads
  the vfx_base's pos (0x484690-0x484696, EAX = effect_get_base's result). This was known (D5107) but not applied.
- No game bug candidates found.

## Header change

- include/engine/rendering/levelrender.h: lobj_ray() clears `min_radius = 0.0f` (was `_unk30 = 0`, same bits; D11501).
  All 259 .cpp checked with mingw: only game/unnamed_decals_0x4bbe80.cpp's 9 known errors. Full strict sweep: no loss.

## Files changed (in w6-src)

game/unnamed_render_scene_0x481680.cpp, game/new_dialogue.cpp, game/new_spell.cpp,
game/unnamed_ai_think_0x4b5140.cpp, game/unnamed_mpmenus_0x43eb00.cpp, game/unnamed_giant_salamanka_0x4c8030.cpp
(closer only), vsdk/unnamed_http_0x55a8e0.cpp, include/engine/rendering/levelrender.h, include/DECISIONS.md.

---

## Worker 7

### Worker 7 notes (match round m-1020)

Strict VC6 sweep (cl 12.00.8966, VC6_STRICT=1) of the 12 files: 306 -> 320 matches, 0 lost
(w7-before.txt / w7-after.txt, `--check` against the before list). Decisions D11600-D11603 in
w7-src/include/DECISIONS.md. No header changed. mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in every
changed .cpp.

## Match counts per file (before -> after, of total)

| file | before | after | total | new matches |
|---|---|---|---|---|
| game/new_inventory.cpp | 58 | 61 | 69 | 0x4155f0 update_equip_slots, 0x417950 draw_list_rows, 0x4181d0 preview_view_begin |
| vsdk/unnamed_ix_0x53ac20.cpp | 25 | 27 | 35 | 0x53bf60 ix_sphere_polygon, 0x53d2d0 ix_ray_boundingbox |
| vsdk/unnamed_vif_0x5329f0.cpp | 8 | 8 | 15 | |
| game/ai.cpp | 81 | 82 | 88 | 0x474950 ai_do_frame |
| vsdk/unnamed_gr_poly_0x5450c0.cpp | 19 | 21 | 24 | 0x545340 set_texture, 0x545610 gr_poly_rgb |
| vsdk/unnamed_sndstream_0x566560.cpp | 22 | 22 | 26 | |
| vsdk/unnamed_checkbox_0x553ba0.cpp | 9 | 10 | 12 | 0x553d00 ui_checkbox::create |
| game/new_status.cpp | 48 | 50 | 51 | 0x42c0e0 status_page_up, 0x42d640 status_draw_skills |
| game/unnamed_editor_dialogs_0x430bc0.cpp | 3 | 3 | 5 | |
| game/new_int_multi_update.cpp | 3 | 4 | 5 | 0x4110b0 version_read_nfo_file |
| game/unnamed_mlayout_0x43f9d0.cpp | 8 | 9 | 9 | 0x43fe00 monster_layout_load |
| game/unnamed_item_info_popup_0x418580.cpp | 22 | 23 | 23 | 0x418ba0 item_info_popup_page_up |

All 14 marked `Confidence: exact` with the reason in the comment.

## What worked
- Duplicated tails written twice (update_equip_slots: the binary's function was 84 bytes longer).
- Branch layout from the condition's grouping: `type >= 4 && (type <= 5 || type == 9)` (monster_layout_load),
  `while (1) { x = f(); if (!x) break; }`.
- Dead argument slots: 8966 puts a local in a dead argument slot by itself once its live range allows it:
  a loop-body local (ix_sphere_polygon te), a width local computed first (ui_checkbox::create), w / h in their
  block (gr_poly_set_texture).
- `#pragma inline_depth(0)` (D9810) around gr_poly_set_texture / gr_poly_rgb: the binary calls float_max /
  float_min out of line there (D11600). Then the D6005 "call argument into a local first" and statement-order
  searches with vc6-try (144 orders for gr_poly_rgb).
- Declaration / initialisation before an object with a constructor (ix_ray_boundingbox `inside` before the
  profile_label).
- The coordinator's row-clamp tip: `if (count < page) rows = count; else rows = page;` from the globals plus the
  locals at the top: status_draw_skills and inventory_draw_list_rows.
- D10600 clamp on two more page_up functions (item_info_popup, status). Other workers own the remaining
  page_up functions with the same max-AND difference (assess, saveload, pxochat, gamelist, pickup, journal):
  worth trying `x -= page; x &= (x <= 0) - 1;` there.
- Global updated in place instead of x / y locals (preview_view_begin); `t1 - t0` written in each sum (ai_do_frame);
  513-byte buffers from the frame size (version_read_nfo_file).
- Tool: a byte-level diff of one function (scratch w7v/cdiff.mjs) shows register / operand differences that
  vc6-side.mjs (mnemonics only) hides; it treats addend bytes of relocations as relocations.

## Still differs (details in D11603)
- inventory_destroy_selected_item / identify_item / equip_item: only the clamp after min (AND into EDX, the
  binary into EAX); 25 more forms tried.
- Register choice / rotation: inventory_popup_result (now right size and table), ai_state::load,
  cast_effect_track_add, call_for_help (now the binary's `sete / sete / test`), ui_radio_button::create,
  wav_open (nBlockAlign operand order), inventory_handle_click, drag_to_equip, load_bitmaps.
- Tail merging chosen differently by 8966: inventory_use_selected_item, version_check_run, ai_update_perception,
  editor_dialog_handle_event (now right size), editor_dialog_show.
- FPU scheduling / stack: pick_attack_slot / claim_attack_slot (get_center x), ix_point_in_polygon (alpha kept on
  the stack), ix_linesegment_boundingbox, ui_checkbox::draw, ix_box_oriented_box_oriented, seg2d_intersect /
  ix_linesegment_sphere (locals not in the binary's argument slots), vif_* (segment search, see D10311),
  snd_stream_fill_* (frame slots), gr_poly_clip_* / buffer_flush (not attempted in depth).

## Behaviour errors / game bugs
- D11602 ix_point_in_polygon (0x53b791-0x53b7af, 0x53b8c2): the binary tests the first triangle before comparing
  the count (do / while); the source tested `i < n` first. Fixed (only differs for n <= 2, which callers do not
  pass). Not a game bug.
- No other behaviour difference found; all other changes keep the behaviour (NaN-sensitive compare forms were not
  used).

## Files changed (in w7-src)
- game/ai.cpp, game/new_int_multi_update.cpp, game/new_inventory.cpp, game/new_status.cpp,
  game/unnamed_editor_dialogs_0x430bc0.cpp, game/unnamed_item_info_popup_0x418580.cpp,
  game/unnamed_mlayout_0x43f9d0.cpp, vsdk/unnamed_checkbox_0x553ba0.cpp, vsdk/unnamed_gr_poly_0x5450c0.cpp,
  vsdk/unnamed_ix_0x53ac20.cpp, include/DECISIONS.md (D11600-D11603).

---

## Worker 8

### m-1020 worker 8 notes

Strict VC6 sweep (cl 8966) of the 11 files, before / after: **403 -> 416 match** (42 differ, 1 not found),
0 lost (`vc6-sweep --check w8-sweep-before.txt`). Lists: `w8-sweep-before.txt`, `w8-sweep.txt`.
mingw `-fsyntax-only -std=c++98`: 0 errors in the 7 changed files. No header changed (vector.h and
characterinfo.h were only changed temporarily for tests and restored; `diff -rq base w8-src` lists only the
.cpp files below and DECISIONS.md). Decisions: D11700-D11704 in w8-src/include/DECISIONS.md.

## Per file (match before -> after, of total)

| File | Before | After | New exact |
|---|---|---|---|
| game/living_entity.cpp | 111 | 115 / 132 | get_resistance 0x4626b0, on_death 0x4537f0, monster_drop_loot 0x450e30, find_ground 0x455fb0 |
| vsdk/unnamed_snd_0x5204c0.cpp | 51 | 54 / 58 | snd_calc_volume_3d 0x520fa0, sound_emitter_calc_volume 0x5210b0, snd_play_3d 0x521480 |
| game/boss_ai.cpp | 51 | 53 / 58 | boss_mount_pick_flee_point 0x4a1ef0, rider_unlink_mount 0x4a33b0 |
| vsdk/character_instance.cpp | 48 | 50 / 53 | alloc_buffers 0x519570, skeleton_get_bone_blend_weight 0x51a130 |
| vsdk/unnamed_vfx_0x536950.cpp | 24 | 25 / 28 | vfx_base::init_vis_flags 0x536ed0 |
| game/new_journal.cpp | 25 | 26 / 27 | journal_desc_page_up 0x41b570 |
| vsdk/matrix.cpp | 43 | 43 / 46 | |
| game/summoner_cd.cpp | 3 | 3 / 6 | (the no-CD patch, D2600) |
| game/summoner.cpp | 13 | 13 / 15 | (WinMain "not found": the tool does not pair `_WinMain@16`; its listing equals the binary apart from relocations) |
| game/unnamed_machival_0x4c0c40.cpp | 25 | 25 / 26 | |
| game/unnamed_level_complete_0x4b0960.cpp | 9 | 9 / 10 | |

Pending on another worker: **Character::init 0x44e1c0** is a byte match once worker 9's header change
(charinfo_create_character_instance(char_info *, int summon)) is merged; tested here with the header
switched temporarily. Marked likely in this copy. The stage sweep should show it as a new match.

## What worked

- **/Od frames read off the binary (D11703).** A scratch helper (`w8-tools/opdiff.mjs --frame`) aligns our
  listing with Sum.exe's by LCS over the instructions (ebp offsets masked) and pairs the ebp offsets of
  aligned instructions: every local's binary slot in one run. Then names chosen by a DP over suffixes so the
  D6101 buckets are non-decreasing down the frame (`dpnames.mjs`), declarations in reverse frame order
  (`frame.mjs` renames and rewrites the block). Three large /Od functions matched this way; unnamed gaps in
  the binary frame are real locals (find_ground's 0x2c-byte gap is a lobj_hit, init's ?: temp a local).
- Store-order brute force (rider_unlink_mount: 120 orders, two match).
- Inline members that order loads: `out->set(x, 0, z)` (flee point); the inline
  attached_light_create_from_vfx (init's argument temporaries).
- Reusing an argument as the local (`time -= s->start`), D6205 element pointer for a record
  (`vfx_base_tag *t`), the 16-bit union view for byte/word flag operations.
- snd_play's `int rval = -1; switch (snd_backend) { case 1: ... }` shape for snd_play_3d.
- An address-taken local stays in memory: `int zero = 0; v = *(float *)&zero;` reproduces the frame-slot
  zero of the two volume functions (a device; volatile also keeps it in memory but pins the store).
- D10600's `x &= (x <= 0) - 1` on the global for journal_desc_page_up; D10702's self copy for on_death's
  clamp; combined `||` test for monster_drop_loot's difficulty filter; a switch for init's hero range test.

## Closer, still differing (kept)

- summoner_cfg_load: size clamp now as the binary; register rotation in the tail.
- vfx_instance::process: block order fixed (146 -> 28 differing lines); register rotation of call args.
- journal_handle_click: frame size now right (dx/dy/dz at function scope); the constant 2 in EDI vs EBP.

## Still differs (tried; D11704)

matrix::determinant (term/factor sort), matrix43::transpose_in_place, matrix43_interpolate_quat,
boss_threat_add, boss_ai_unload (callees probably ai_state members), boss_move_in_range_dispatch (needs
D7603 (3) bool headers of other units), boss_ai_parse_table / boss_target_list_build (register allocation),
vfx init_nodes / get_node_world_transform, character_instance build_bone_matrices / increment_tick /
sum_root_velocity, snd_set_volume family, mp_handle_combat_sequence, machival_ai_select_action, the /Od
len() temporaries (get_speed, anim_speed_at_blend, stopping_distance, distance_to_reach_*),
ai_travel_offset, scale_stats_to_level (fidiv), save_state / load_state, update_head_look,
clip_move_to_navmesh, ai_move_frame, move_frame, get_hit_by_spell (code differs, not only the frame),
Character::Character (combat union, D1702).

## Behaviour errors / game-bug candidates

- **Fixed (D11703): Character::on_death** returned when the current move's trigger 9 time left was
  *above* 0.0001; the binary (0x4538b0-0x4538bd: `fcomp 0.0001; test ah, 0x41; je 0x4538c2`, else `jmp
  0x454060`) returns when it is at most 0.0001, as play_move (D6104). Now `<= 0.0001f`; the function is a
  byte match.
- Evidence, not applied: boss_ai_unload's cases tail-jump with ECX = this unchanged, so the per-boss
  unload functions (luminar_free_bitmap, tiger_unload, ...) are probably ai_state members.

## Files changed (in w8-src)

game/living_entity.cpp, game/boss_ai.cpp, game/new_journal.cpp, game/summoner.cpp,
vsdk/character_instance.cpp, vsdk/unnamed_snd_0x5204c0.cpp, vsdk/unnamed_vfx_0x536950.cpp,
include/DECISIONS.md (D11700-D11704).

---

## Worker 9

### m-1020 worker 9 notes

Tree: `m-1020/w9-src`. VC6 SP5 (cl 12.00.8966), strict. Before: `w9-before.txt`, after: `w9-after.txt`
(extracted from the full strict sweep `w9-full.txt`).
**My 14 files: 384 -> 399 of 449 (+15); none lost.** Full tree: 5092 match (repo list 5077), `--check
src/decomp/vc6-matches.txt`: 0 lost. mingw over all 259 .cpp: only unnamed_decals_0x4bbe80.cpp (9 errors, as before).
Decisions D11800-D11804 appended to `w9-src/include/DECISIONS.md`.

## Per file (matches before -> after / total)

| file | before | after | won |
|---|---|---|---|
| vsdk/unnamed_listbox_0x52ca60.cpp | 58 | 60 / 69 | 0x52d650 create (store order), 0x52e590 scroll_page_down (tail written in both branches) |
| game/characterinfo.cpp | 60 | 64 / 70 (+1 `target`) | 0x447850 remap_town_moves, 0x447e80 str_hash_nocase_rol6, 0x4482a0 load_script_anim, 0x448de0 create_character_instance (HEADER D11800) |
| vsdk/unnamed_net_0x517590.cpp | 23 | 23 / 31 | (NewSendReliable, ConnectToServer closer) |
| game/unnamed_tentacle_beast_0x4c6480.cpp | 19 | 21 / 25 | 0x4c7cc0 cmd_tentacles, 0x4c7e90 cmd_tentacle_test (boss_load, ai_frame much closer) |
| game/new_charselect.cpp | 26 | 29 / 32 | 0x406590 init, 0x406bb0 update_create_button, 0x4076d0 mp_char_create |
| game/new_levelselect.cpp | 21 | 23 / 25 | 0x41d1d0 draw_levels, 0x41d5e0 add_chat_line |
| vsdk/character.cpp | 31 | 32 / 34 | 0x541790 ca_load_preload_index (HEADER D11801) |
| vsdk/unnamed_movement_0x531d20.cpp | 3 | 4 / 4 | 0x531f50 movement_read_controls (unit flags /Ob0) |
| vsdk/unnamed_gadget_0x52bfe0.cpp | 46 | 46 / 48 | |
| game/unnamed_pyrul_0x4b87c0.cpp | 21 | 21 / 23 | |
| vsdk/unnamed_ui_manager_0x52f570.cpp | 11 | 11 / 12 | |
| vsdk/file_packfile.cpp | 19 | 19 / 20 | |
| game/objects.cpp | 39 | 39 / 40 | |
| game/item_paging.cpp | 7 | 7 / 8 | |

Confidence raised to exact for the 15 (comment `exact (VC6 byte match, m-1020 w9; was likely)`).

## What worked (D11802)

- max(x, 0) into a global (extends D10600): min as a ternary straight into the global, then `g &= (g < 0) - 1`
  (setl / sets) or `(g <= 0) - 1` (setle), matching the binary's set instruction. 3 wins.
- Written-out duplicate tails: 8966 keeps / cross-jumps them as the binary does where one shared statement does
  not (update_create_button's two `|= 4`, scroll_page_down's final vscroll in both branches, charselect_init's
  update_scrollbar(0) / (i) per branch).
- Statement shapes: store-order search (listbox create, 24 orders), `while` instead of `for` (remap_town_moves),
  `rows = a < b ? a : b` (draw_levels), local walking pointer (hash), read through the object the code reads
  (`ci->character`), `c = living_from_handle(); c->flags |= 8` (keeps the constant in a register),
  `pos -= vector3(2, 2, 2)`, `end = pool->offset` read before the test.
- Flags: unnamed_movement_0x531d20.cpp is /Ox /Ob0 /GX /MT /Ow (out-of-line vector3 members in the binary; the other
  three functions match either way). vc6-flags.txt line changed in my copy.
- Two header changes (below).
- Tooling (scratch, `m-1020/w9v`): `td.mjs` (variants with an operand-aware distance, from m-1019 w9, repne lines
  fixed), `fd.mjs` (distance of every differing function of a file), `apply.mjs`.

## Header changes

- D11800 include/engine/characterinfo/characterinfo.h: charinfo_create_character_instance(char_info *, int summon)
  (was bool; the binary uses the argument as a full dword). Callers: characterinfo.cpp, living_entity.cpp (111
  matches unchanged).
- D11801 include/vsdk/ca/character.h: ca_preload_offsets / ca_preload_sizes are one struct `ca_preload_tbl`
  (0x2ee20c4); old names kept as macros. Users: vsdk/character.cpp only.

## Still differing (D11803 closer, D11804 tried)

- Closer, kept: nw_NewSendReliable (one push late), nw_ConnectToServer (indexed form; port load early in the binary),
  tentacle_beast_boss_load (2 instructions: last find_move push order), tentacle_beast_ai_frame (code identical; the
  binary places the function's exit block after the stance block -> 16 bytes of long jumps), add_skeleton (same exit
  placement issue).
- Register choice only: page_down, both drag_scrollbar, item_page_out_all, send_event, set_parent, accept_reliable,
  report_memory (sum order canonicalised), get_vscroll_fraction, set_vscroll_fraction, scroll_page_up, ui_process.
- alloc_level_buffers: bytes match, strict `target`: the four anim-size loads come in a fixed order independent of
  every source order and declaration order tried (48 + 5 forms).
- Not attempted beyond a first look: characterinfo_parse_table, load_party (the cast-spell loop uses a biased
  pointer into char_info), can_use_summon, learn_chain_skill, setup_head, both handle_click (frames 8 vs 0x14 and
  0x10c vs 0x110), nw_WorkReliable, init_networking, NewCloseSocket (socket pointer register), listbox process / draw /
  add_entry / wrap_text / snap_scroll_bottom / constructor (the head's inline list_init would need inlining under
  /Ob0; __forceinline does not), pyrul choose_attack / fire_attack, tentacle select_action / frame_hook,
  item_obj_spawn, packfile_read_directory.

## Behaviour errors / game bugs

None found; every change is behaviour-identical (`i < first ? i : first` = the old if; the start_grab test after
the switch only runs for result 3 as before; `ci->character == ca` inside the test).

## Files changed (in w9-src)

game/characterinfo.cpp, game/new_charselect.cpp, game/new_levelselect.cpp, game/unnamed_tentacle_beast_0x4c6480.cpp,
vsdk/unnamed_listbox_0x52ca60.cpp, vsdk/unnamed_net_0x517590.cpp, vsdk/character.cpp,
vsdk/unnamed_movement_0x531d20.cpp, include/engine/characterinfo/characterinfo.h, include/vsdk/ca/character.h,
vc6-flags.txt (movement unit /Ob0), include/DECISIONS.md.

Coordinator tips received late: the row-count clamp shape was tried on both handle_click functions (worse; their
frames differ first); no place found for the volatile store.

---

## Worker 10

### Worker 10 notes (match round m-1020)

Strict VC6 sweep (cl 12.00.8966, `VC6_STRICT=1`) of my 14 units: **350 -> 366 matches of 407, 0 lost**
(`w10-before.txt` / `w10-sweep.txt`, `vc6-sweep --check w10-before.txt`). Whole-tree strict sweep of w10-src
(`w10-full.txt`): 5093 match, `--check src/decomp/vc6-matches.txt`: 5077 there, 0 lost. mingw
`-fsyntax-only -std=c++98 -w -Iinclude` over all 259 .cpp: errors only in game/unnamed_decals_0x4bbe80.cpp (the
known 9). decomp-sigcheck: only the accepted charinfo_load_move_anim. Decisions D11900-D11910 appended to
w10-src/include/DECISIONS.md.

## Per file (matches before -> after, of total)

| File | Before | After | Won |
|---|---|---|---|
| game/unnamed_interface_0x411270.cpp | 58 | 62 / 69 | 0x413200 party_find_next_member, 0x413290 party_find_prev_member, 0x4134b0 ui_draw_panel_frame, 0x414750 ui_tooltip_set |
| game/new_hud.cpp | 50 | 52 / 60 | 0x40eb30 hud_draw_tooltips, 0x40d640 hud_confirm_level_exit |
| vsdk/unnamed_ds_0x55bf90.cpp | 52 | 56 / 60 | 0x55cae0 ds_fill_buffer, 0x55ca10 ds_create_buffer, 0x55c890 ds_load_buffer, 0x55d530 ds_stream_read_chunk |
| game/unnamed_tiger_0x499e50.cpp | 42 | 43 / 48 | 0x49ba70 tiger_wander_update |
| game/new_automap.cpp | 26 | 27 / 32 | 0x403000 automap_drag_scroll |
| game/new_multichat.cpp | 8 | 8 / 12 | |
| game/unnamed_states_0x431870.cpp | 11 | 13 / 14 | 0x431e80 state_loadgame_enter, 0x432200 loadgame_level_list_cb |
| vsdk/unnamed_poly_clip_0x5519c0.cpp | 6 | 6 / 8 | |
| game/unnamed_projectors_0x4c0250.cpp | 9 | 9 / 11 | |
| vsdk/unnamed_ui_static_0x527e20.cpp | 9 | 9 / 10 | |
| vsdk/unnamed_inetgetfile_0x51b760.cpp | 9 | 10 / 10 | 0x51b780 InetGetFile::InetGetFile |
| vsdk/memory.cpp | 18 | 18 / 19 | |
| game/new_skill.cpp | 38 | 38 / 39 | |
| game/level_script_catacombs.cpp | 14 | 15 / 15 | 0x4fe5e0 ls_catacombs_use |

All 16 are `Confidence: exact` with the shape named in the comment.

## What worked

- A global read through itself in the NULL test (`if (!status_character) return NULL; ... status_character->
  handle`) with the local copied first (party_find_next / prev; D11900).
- Block-local temporaries: the border step `d` (panel frame), kind / id in the hot-button block (the frame slot
  order: declaration order has no effect at /O2, block scope does; D11901), `got` in the success block
  (ds_fill_buffer: 8966 then reuses a dead argument slot).
- Success branch first and the failure as an else that sets the result (ds_fill_buffer, ds_stream_read_chunk).
- Store-order brute force: ds_create_buffer's four descriptor stores (1 of 24 matches).
- D10600's `x &= (x <= 0) - 1` on the stored global (ui_tooltip_set, two clamps).
- Strict-mode `target` reports to fix operand order: tooltip pads, drag-scroll sums. Inline operator+ operand
  order (`centre_offset + pos`).
- Buffer sizes from the frame: InetGetFile's directory buffer 256 (not MAX_PATH; the compare's instruction text at
  the first difference was a wrong look-up, the byte difference was the `sub esp` immediate). hud_load_bitmaps'
  filename 128 (frame now right, still differs).
- Format string into a local before a call argument (hud_confirm_level_exit, D6205).
- An inline helper that takes the object rather than a field (loadgame_fill_save_list: two functions).
- Separate locals for the min result and if-forms for the two max clamps (automap_drag_scroll).
- A shared block placed before the final return via `if (0) { label: ...; }` (ls_catacombs_use; a plain label
  after the return makes 8966 merge more blocks than the binary).
- HEADER CHANGE D11903: ds_load_buffer's and snd_load's `streamed` are bool (passed on unconverted to the bool of
  ds_load_buffer_data). The snd_load definition in vsdk/unnamed_snd_0x5204c0.cpp (not my unit) changes its
  parameter type only; that unit's matches are unchanged and no match is lost in the tree.

## Still differs (tried; D11910)

state_title_enter (flag merge register choice), ui_choice_popup_show / _hit_test / _draw, ui_message_box_show
(shared `mov al, 1` test constant) / _draw, interface_init_global (base_w kept in memory by the binary), ui_3d_begin
(improved: loop shape now right, one load scheduled differently), hud_load_bitmaps (push scheduling),
hud_update_portraits / hud_interact_nearest / hud_handle_keys / hud_hot_buttons_drag / mp_draw_loading_status /
hud_portrait_get_color (register choice, cross-jumped colour tails), hud_drag_icon_begin, ds_is_3d_buffer,
ds_load_buffer_acm, ds_play, ds_stream_open, the tiger functions (tiger_path_to_target calls vector3 constructors
out of line; remap_in shares serpent's register pattern), automap_load_bitmaps / center_on_player / handle_click /
label_text_input / draw_map (frame 0x30 vs 0x34), the four multichat functions, polygon_clip_2d / _to_plane,
both projector functions (out-of-line vector3 / plane::operator= calls; the face collector's frame is 8 bytes
larger in ours), ui_static::draw, mem_report_line, character_use_skill. Not attempted in depth: hud_handle_keys,
automap_handle_click / draw_map, character_use_skill, ui_static::draw (large).

## Behaviour errors / game-bug candidates

- No behaviour error found; every change keeps the behaviour.
- Game-bug candidate (latent): InetGetFile::InetGetFile copies the local file name into a 256-byte stack buffer
  (frame 0x104 at 0x51b791, buffer at +4) with strcpy before cutting it at the last backslash: a name of 256
  characters or more overflows it. The callers pass short fixed names, so it is not reachable in practice.
- ds_load_buffer / snd_load: the type change (int -> bool) changes no behaviour (all callers pass 0 or 1).

## Files changed (in w10-src)

- game/unnamed_interface_0x411270.cpp
- game/new_hud.cpp
- game/unnamed_tiger_0x499e50.cpp
- game/new_automap.cpp
- game/unnamed_states_0x431870.cpp
- game/level_script_catacombs.cpp
- vsdk/unnamed_ds_0x55bf90.cpp
- vsdk/unnamed_inetgetfile_0x51b760.cpp
- vsdk/unnamed_snd_0x5204c0.cpp (not my unit: snd_load's parameter type only, D11903)
- include/vsdk/snd/ds.h (HEADER, D11903)
- include/vsdk/snd/snd.h (HEADER, D11903)
- include/DECISIONS.md (D11900-D11910)

---

## Worker 11

### m-1020 worker 11 notes

Strict sweep of the 14 units: 304 -> 312 match (57 -> 49 differ), no match lost (address by address against
w11-sweep-before.txt). Sweep lists: m-1020/w11-sweep-before.txt, m-1020/w11-sweep.txt. Decisions D12000-D12009 in
w11-src/include/DECISIONS.md. No header changed. mingw `-fsyntax-only`: 0 errors in every changed file.

## Per file (match before -> after, of total)

| File | Before | After | Total |
|---|---|---|---|
| game/spells.cpp | 28 | 30 | 39 |
| game/unnamed_ai_move_0x4b1410.cpp | 9 | 9 | 19 |
| game/unnamed_dialogue_box_0x4d6660.cpp | 24 | 26 | 32 |
| vsdk/effect_mem.cpp | 33 | 33 | 39 |
| game/levelrender.cpp | 6 | 7 | 12 |
| game/unnamed_lobj_raycast_0x4ae890.cpp | 3 | 3 | 7 |
| game/unnamed_phoenix_0x497ac0.cpp | 33 | 33 | 36 |
| vsdk/unnamed_ui_button_0x526940.cpp | 22 | 22 | 24 |
| game/unnamed_party_inventory_0x46b460.cpp | 28 | 28 | 30 |
| vsdk/unnamed_vfile_rw_0x525720.cpp | 50 | 50 | 51 |
| vsdk/unnamed_ftp_0x559ea0.cpp | 15 | 16 | 16 |
| vsdk/plainstring.cpp | 19 | 20 | 20 |
| game/new_gamelist.cpp | 28 | 29 | 29 |
| game/level_script_ikaemosu.cpp | 6 | 6 | 7 |

## New matches (Confidence raised to exact)

- 0x5152a0 int_to_string_with_commas: `first` is the loop index, incremented before pos (D12000).
- 0x559ed0 ftp_get::ftp_get: if / else of two strcpy calls for the defaults; last / first both from p + i + 1
  before the filename copy, last cleared first (D12001).
- 0x40b120 gamelist_page_up: `g -= rows; g &= (g <= 0) - 1;` on the global (D12002).
- 0x48fbb0 levelrender_sum_point_lights: `sqrt(d.y*d.y + (d.x*d.x + d.z*d.z))` for the binary's z, x, y sum (D12003).
- 0x47e100 spell_place_ground_effect: the avoidance record in the binary's store order (D12004).
- 0x4d7800 dialogue_topic_list::add: if / else with one shared `layout(); return e;` (D12005).
- 0x47bd30 spell_spawn_creature: counts cleared after their list heads; `n_others--` after the append (D12006).
- 0x4d74f0 dialogue_box::scroll_next_page: the max bound computed once into a local (D12007).

## What worked

- Ternary call argument -> if / else of two calls (ftp ctor's strcpy; the colour calls in the dialogue draws).
- The max(x,0) global clamp `x &= (x <= 0) - 1`.
- Store-order search (avoidance record; list heads and counts).
- A macro argument with a call evaluated twice where the binary calls once -> a local.
- if / else with a common tail (VC6 then duplicates it as the binary does).
- vector3::set(...) to get the right-to-left z, y, x evaluation of a centre (closer only).
- A scratch tool (m-1020/w11t/od.mjs, tr2.mjs) that disassembles our .obj with llvm-objdump and LCS-aligns it
  against Sum.exe with operands, relocations masked and jump targets relative; it shows slot and operand
  differences vc6-side does not, and scores variants by the number of differing instructions.

## Closer, still differing (kept; D12008)

dialogue_line::draw 0x4d6b80 (frame and dispatch now as the binary; integer sums associate differently),
dialogue_topic_entry::draw 0x4d6e70, File::read 0x525fe0 (entry copy of len), ai_state::wall_avoid_steer 0x4b2890.

## Still differing (D12009)

Register choice / shrink-wrap / cross-jump choice in: effects_parse_arr_line, ls_ikaemostopint_init,
last_visible_line, dialogue_box::draw, dialogue_topic_list::draw, character_use_inventory_item, spell_roll_amount,
steer_jitter, spell_fire_missile, effect_base_read, rfx_preload, vfx_load (bit-field copy), ui_button::process,
lobj_ray_test_faces (inline dot operand order), lobj_raycast, spell_target_value, cast_spell_update,
spells_parse_table. Not attempted at length: the large levelrender draw lists, summon_create, spell_prepare_cast,
spell_cast_execute, spell_cast_launch_effects, follow_path, avoid_characters, travel_steer_update, phoenix AI,
lobj BSP / sphere tests, ui_button::draw_arrow, vfx_instance_create, effects_load_bases.
Observation: 8966 canonicalises the order of 3-term integer sums and of the hand-written dot (all 48 source orders
give one form), so those differences need a different shape, not an operand order.

The coordinator's two later tips (volatile global store, row-count clamp): no matching pattern in these files.

## Behaviour errors / game bugs

None found. All changes are behaviour-identical (scroll_next_page now calls gr_get_font_height twice like the binary
instead of three times; same value).

## Files changed (in w11-src)

vsdk/plainstring.cpp, vsdk/unnamed_ftp_0x559ea0.cpp, vsdk/unnamed_vfile_rw_0x525720.cpp, game/new_gamelist.cpp,
game/levelrender.cpp, game/spells.cpp, game/unnamed_dialogue_box_0x4d6660.cpp, game/unnamed_ai_move_0x4b1410.cpp,
include/DECISIONS.md (D12000-D12009).

---

## Worker 12

### m-1020 worker 12 notes

Strict VC6 sweep (cl 12.00.8966, VC6_STRICT=1) of my 14 files: **373 -> 384 matches, 0 lost** (address by address:
`w12-before.txt` / `w12-sweep.txt`). Whole tree of the copy, strict, `--check src/decomp/vc6-matches.txt`: **5077 -> 5088,
0 lost** (`w12-full.txt`). mingw `-fsyntax-only -std=c++98 -w -Iinclude` on all 259 .cpp (a header changed): errors
only in game/unnamed_decals_0x4bbe80.cpp (9, as before). Decisions D12100-D12109 in w12-src/include/DECISIONS.md.

## Per file (matches before -> after / functions)

| file | before | after | won |
|---|---|---|---|
| game/script_internal.cpp | 17 | 20 / 28 | 0x4dca90 script_action_wander, 0x4db420 script_parse_navpoint, 0x4db700 script_parse_navpoints |
| vsdk/gr_direct3d.cpp | 40 | 42 / 50 | 0x538850 d3d_enumerate_ddraw_devices, 0x5394e0 gr_d3d_init |
| game/level_scripts_common.cpp | 70 | 72 / 78 | 0x4cb060 level_script_restart, 0x4ce6f0 script_guard_sees_player |
| vsdk/gr.cpp | 55 | 56 / 61 | 0x5077d0 gr_rect |
| game/level_script_khosani_lab.cpp | 39 | 41 / 45 | 0x4f7fe0 corner_nn, 0x4f8070 corner_np |
| vsdk/registry.cpp | 13 | 14 / 14 | 0x511e50 os_registry_resolve_key |
| game/unnamed_worldmap_automap_0x42f1e0.cpp | 31 | 31 / 35 | |
| game/unnamed_netsession_0x440b40.cpp | 18 | 18 / 21 | |
| vsdk/unnamed_ui_slider_0x55e040.cpp | 24 | 24 / 26 | |
| game/unnamed_netpackets_0x441820.cpp | 10 | 10 / 12 | |
| vsdk/unnamed_vfx_material_0x564d70.cpp | 14 | 14 / 15 | |
| vsdk/unnamed_eax_0x521e40.cpp | 13 | 13 / 14 | |
| game/new_gamecreate.cpp | 19 | 19 / 20 | |
| game/level_script_ionaext.cpp | 10 | 10 / 11 | |

## What worked

- **HEADER CHANGE D12108**: lobj_ray's constructor clears +0x30 as the float `min_radius = 0.0f` (same bytes). The
  binary stores that 0 as an immediate although a zero register is live, which 8966 does only for a float constant.
  Won script_guard_sees_player; with D12109 the two navpoint parsers. 17 user units, whole tree rechecked, none lost.
- **FLAGS CHANGE D12103**: vsdk/gr.cpp is /Ox /Ob0 /GX /MT /Ow. The binary builds gr_rect's and gr_bitmap_float's
  vertex arrays with `vector constructor iterator` + gr__Vertex__Vertex, which itself calls vector3() out of line:
  no inline expansion in that unit. gr_set_gamma needed `(float)pow(...)` instead of the math.h inline powf to keep
  its match. Then gr_rect: array declared after the clip tests, corners as vlist/sx/sy/sw/flag (1 of 24 orders).
- goto-fail: one shared `return NULL` after the success return (d3d_enumerate_ddraw_devices).
- One return of a register variable instead of three returns that fold a constant; pointer-walking copies whose
  induction variables 8966 merges (os_registry_resolve_key).
- Element pointer for a record read several times in a loop (`door_info *d = &doors[i]`, level_script_restart).
- Call arguments read into a local first (script_action_wander's layer).
- Vector copies for the ray set-up, memset just before the raycast, the level object in a local before the member
  call (navpoint parsers, D12109). Buffer size from the frame: navpoints' name is 80 bytes.
- `vector3 dir = a - b;` (built in place) and `dir.dot(v)` (guard).
- A two-argument setter evaluates y first as a call does (khosani corners, D12100); `s = 0.3f; if (!inner) s = 0.7f;`.
- The coordinator's volatile-store device (D11305) for gr_d3d_init's d3d_init_done store (D12106).

## Closer, still differing (kept; behaviour identical)

- party_set_level: one `mov dl, [eax + ecx + 0x93]` base / index order left (block-scope skill index, if / else adds).
- corner_pp: only the first y sum's operand order.
- gr_d3d_draw_primitive: `else if (pos != 0) nooverwrite; else discard;` (8966 preloads one value).
- cmd_disp_rate: table indexed directly, ON / OFF as if / else of two sprintf calls; EBP prologue push and the merged
  stack pop after sprintf left.
- script_action_combat_mode: `skip` declared before `on`, `? true : false` return; register allocation left.
- level_check_triggers: path pointer local (fewer register differences; frame still 4 bytes larger).
- gr_bitmap_float: array declared after gr_set_texture; FPU schedule left.
- create_scrollbar: `visible = f(); view = visible;` (this / listbox register swap left).

## Still differing, no form found (D12107)

ls_ionaext_frame's sete, level_trigger_point_left_of_line's 8 extra frame bytes, worldmap_frame's fstp st(3),
clamp_to_edge's load above fcos, get_frame_bitmap's this / n swap, eax_init's block order, gr_d3d_set_window's lea
order, gr_line's two integer loads, mp_player_alloc's `&p->prev`, multi_process_client_status's stores,
automap_labels_parse_table's early epilogue, gr_tmapper_multipass's +8 pass-pointer bias, mp_reset_net_stats, and the
large functions (parse_objects, entry_start, check_done, set_state, init_device, bitmap, screenshot, trace_beam,
emit_segment, choose_turn, labels_save, ...) beyond a first look.

## Behaviour errors / game-bug candidates

None found. All changes keep behaviour (the navpoint parsers' memset moved after the ray fields: the hit record is
still cleared before the raycast; the lobj_ray constructor writes the same bits).

## Files changed (in w12-src)

- game/level_script_khosani_lab.cpp, game/script_internal.cpp, game/level_scripts_common.cpp,
  game/unnamed_netpackets_0x441820.cpp
- vsdk/gr.cpp, vsdk/gr_direct3d.cpp, vsdk/registry.cpp, vsdk/unnamed_ui_slider_0x55e040.cpp
- vc6-flags.txt (gr.cpp line, D12103)
- include/engine/rendering/levelrender.h (HEADER, D12108)
- include/DECISIONS.md (D12100-D12109)

Tip for other workers: the D12109 ray set-up and D12108 apply to every lobj_raycast_all caller (gametarget,
level_scripts, living_entity, objects, spells, camera, ghost rider, goals / doors, luminar, render scene, script
editor, shadows, tentacle beast units).

---

## Worker 13

### Worker 13 notes (match round m-1020)

Strict VC6 sweep (cl 12.00.8966, VC6_STRICT=1) of my 13 units: **327 -> 337 matches (+10), 0 lost**
(`w13-before.txt` / `w13-after.txt`, compared address by address). Full-tree strict sweep of w13-src with
`--check src/decomp/vc6-matches.txt`: 5077 -> 5087, 0 lost (`w13-full.txt`). Decisions D12200-D12203 appended to
`w13-src/include/DECISIONS.md`. No header changed. mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in
all 7 changed .cpp.

## Per file (matches before -> after, of total)

| file | before | after | new matches |
|---|---|---|---|
| vsdk/unnamed_vfile_0x5128b0.cpp | 73 | 79 / 82 | file_add_path 0x512e60, file_find_files 0x5133b0, FileReader::read_string 0x513e00, File::find 0x5141c0, File::seek 0x514910, File::calc_crc32 0x514d60 |
| game/conversation.cpp | 24 | 25 / 27 | conversation::farewell 0x4d8760 |
| game/unnamed_goals_doors_0x475180.cpp | 62 | 63 / 67 | ai_state::goal_find 0x475eb0 |
| game/unnamed_mp_objects_0x47f860.cpp | 25 | 26 / 31 | mp_handle_obj_update 0x4806a0 |
| game/new_pickup.cpp | 18 | 19 / 20 | pickup_page_up 0x420d20 |
| game/combat.cpp | 49 | 49 / 63 | |
| game/pathfinding_visibility.cpp | 4 | 4 / 11 | (bsp_contains closer) |
| vsdk/unnamed_ca_mesh_0x50af80.cpp | 8 | 8 / 12 | (morph_vertices closer) |
| game/unnamed_evil_urath_0x4c4170.cpp | 15 | 15 / 18 | |
| vsdk/unnamed_crc32_0x5520d0.cpp | 3 | 3 / 4 | |
| vsdk/stringpool.cpp | 15 | 15 / 16 | |
| game/multitrade.cpp | 21 | 21 / 22 | |
| game/level_script_jadetemple.cpp | 10 | 10 / 11 | |

All new matches have `Confidence: exact` with the shape in the comment.

## What worked (D12200, D12201)

- **Flag change (D12200):** vfile unit to `/Ox /Ob0 /GX /MT /Ow` (vc6-flags.txt). calc_crc32 calls Crc32's
  constructor COMDAT 0x514eb0 out of line; all 77 old matches stay (superset, D7502), calc_crc32 then matched
  after a branch-polarity fix.
- Signed index loops over global tables instead of walking pointers (VC6 keeps `jl`).
- `while (1)` + `if (...) break;` where the binary's loop is not rotated (read_string, goal_find).
- List splice through a saved neighbour pointer; sorted-insert loops restarting from `list->next`.
- A chained field argument through a local (farewell); the origin added to the argument itself (seek).
- File::find: path_root flag as if/else of two calls, `? true : false` on find_at, loop fall-through to
  `path_id = 0` written with gotos.
- Block-scoped cp/cb/ch to share dead slots (obj_update); coordinator's `x &= (x <= 0) - 1` on the global
  (page_up).

## Closer, still differing (kept, behaviour identical, Confidence left likely/unsure)

- file_list_files: indexed loop (no mnemonic difference left); ext vs i/p frame-slot order (24 declaration orders
  and block scopes tried).
- mp_handle_status_add: the handle conversion hoisted into `source` (ndiff 7 -> 2); value spill / c reload order.
- mp_handle_new_path: `vector3 v` at function scope gives the binary's frame 0x30; the vector2 argument still
  goes through the stack.
- pathvis_bsp_contains: `vector2 n(...)` gives the binary's FPU sequence (ndiff 59 -> 26); the shared recursive
  tail is kept in the other case.
- ca_render_morph_vertices: anim as if/else (ndiff 132 -> 77); the instance pointer is cached in EBP by us.

## Still differing (D12203)

Crc32::update, StringPool::Add, set_cipher_key (binary looks like a bitfield flag: needs a header change),
get_async_status, mp_handle_ai_disable, mp_send_new_lep, mp_send_obj_update, pickup_draw_item_list, jadetemple
action, conversation clear_speaker (thiscall member, D10904) / restore_state, mp_handle_it_okay,
ai_choose_special, the pathfinding EH functions, matrix3 pack/unpack, door_find_floor_point, ai_goal_evaluate,
evil_urath leap_rise / leap_travel / select_action, and the large combat and ca_mesh renderers (not attempted
beyond a first look).

## Behaviour errors / game-bug candidates

- **Fixed (D12202), mp_handle_obj_update 0x4806a0:** two float constants were 1 ulp off the binary's: the heading
  factor (binary 0x38c910a4 at 0x581a7c, fmul at 0x480748; source had 0x38c910a3) and the 10-degree threshold
  (binary 0x3e32b8c3 at 0x581a44, fcomp at 0x480814; source had 0x3e32b8c2). Now `9.5875264e-5f` and
  `0.17453294f`. Found by the strict data-target check.
- No game-bug candidates found.

## Files changed (in w13-src)

- vsdk/unnamed_vfile_0x5128b0.cpp
- vc6-flags.txt (vfile unit `/Ox /Ob0 /GX /MT /Ow`, D12200)
- game/conversation.cpp
- game/unnamed_goals_doors_0x475180.cpp
- game/unnamed_mp_objects_0x47f860.cpp
- game/new_pickup.cpp
- game/pathfinding_visibility.cpp
- vsdk/unnamed_ca_mesh_0x50af80.cpp
- include/DECISIONS.md (D12200-D12203)

---

## Worker 14

## Worker 14

### Worker 14 notes (match round 2026-10-20)

Tree: `m-1020/w14-src`. Compiler VC6 SP5 (cl 12.00.8966), strict sweep (VC6_STRICT=1). Before: `w14-before.txt`,
after: `w14-sweep.txt`. **202 -> 210 of 259 functions in my 10 files; none lost** (`vc6-sweep --check
w14-before.txt` exits 0). No header and no flags change. mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in
the five changed files. Decisions D12300-D12303 appended to `w14-src/include/DECISIONS.md`.

## Per file (matches before -> after / total)

| file | before | after | won |
|---|---|---|---|
| game/level_scripts.cpp | 87 | 90 / 124 | 0x4cfbd0 data_offset, 0x4cfd30 set_flag, 0x4cff80 set_bit |
| game/new_saveload.cpp | 32 | 33 / 37 | 0x426260 saveload_page_up |
| game/multisave.cpp | 4 | 4 / 8 | |
| game/unnamed_cutscene_camera_0x493e50.cpp | 9 | 10 / 12 | 0x494310 cutscene_camera_sequence::sample |
| vsdk/unnamed_clip_0x5590b0.cpp | 15 | 15 / 17 | |
| game/new_playmenu.cpp | 14 | 16 / 16 | 0x4217c0 playmenu_frame, 0x421b00 playmenu_open_save |
| vsdk/unnamed_build_stamp_0x52f450.cpp | 2 | 3 / 3 | 0x52f450 build_stamp_check |
| vsdk/ui_container.cpp | 9 | 9 / 10 | |
| game/multicontainer.cpp | 12 | 12 / 13 | |
| game/level_script_sewers.cpp | 18 | 18 / 19 | |

All eight raised to `Confidence: exact`; nothing that differs is marked exact.

## What worked (D12300-D12302)

- One exit / one result variable: build_stamp_check (`result = 3` before the memcmp test, every outcome
  assigned, one return: the binary's `mov edx, 3` before the branch), data_offset (second switch assigns and
  breaks instead of returning in each case).
- Early returns instead of a guarded block: playmenu_frame (`if (input) return key; if (box) return key;`), which
  keeps the binary's store of the click result into the key slot and its shared return.
- Common subexpressions as locals before the branch: set_flag (`byte`, `mask` before `if (value)`), set_bit (byte
  then mask before the size check).
- `x &= (x <= 0) - 1` on the global (saveload_page_up), as listed.
- Case-local blocks: playmenu_open_save's locals per case share offset 0; `save` is `bool[4]` like
  open_saveload_screen (D7200).
- New: a parenthesised member operand in a float compare, `if (t <= (start))`, gives `fld m; fld st(1); fcompp`
  instead of `fcom m` (sample, D12301).
- tools: a small variant runner (`w14work/tryx.mjs`) that reports the first difference per variant and a
  register-sequence diff; useful because vc6-try's `--side` output piped into Select-String killed the run once
  and left a variant in the file (restored and checked against base).

## Closer but still differing (kept; behaviour unchanged)

- write_header 0x4d1df0: strings copied as `memcpy(dst, src, strlen(src))` (the binary's `not ecx; dec ecx`
  count), level_time through `float t = (level_time)` (the binary's dead local). Now only two registers differ
  (off + 0x100 in ESI vs EDI; the final compare's result register).
- hot_button_set 0x4d16a0: `bool has_kind = kind != 0;` before the loop (the binary's setne into a frame byte).
  Same size and instructions; only `[esi+eax]` vs `[eax+esi]` in the three stores.
- write_living 0x4d2150: living_from_handle hoisted out of the write_character argument list (now same order);
  remaining: 8966 caches the constant 4 in EDI for the first three `off += 4`.
- camera_blend_sequence::advance 0x494400: `pos = target_pos * w + cam->eye_pos * u` (binary's evaluation order);
  remaining: the six angle locals' slots (cam/target pairs swapped).
- saveload_draw_info 0x426d10: `hour = t->tm_hour` before the am/pm strcpy (same instruction count now);
  remaining: frame 0x258 vs 0x25c and the small locals' order.
- saveload_scan_saves 0x4258c0: the list head is a whole `file_find_node` (the binary's frame has 0x38 bytes for
  it, as new_charselect.cpp); frame 484 vs 492, the binary keeps saveload_num_saves in EBX and the node pointer in
  memory.

## Still differs (tried, not reproduced; D12303)

- Base / index order of `[reg+reg]` (hot_button_set, read_track's key stores): `p[k]`, `k[p]`, `*(p + k)`,
  post-increment all give the same order.
- Pure register swaps: get_int, has_conversation_data, level_item_has, level_format_display_name, savegame::read,
  mp_player_file_read_summary (only `mov ecx, eax` vs `mov edx, eax` in the version-8 flag remap; the reported
  first difference there points at the EH funclet, which vc6-compare prints by mistake), hot_button_get.
- Spill-slot order: write_item_objects (count vs the float temporary; function-scope f, f first, count in a block
  all unchanged), layout_conversation (the info / sec pointer spills; declaration and computation order
  unchanged), advance (declaration order and names do not change 8966's /O2 layout; float[2] arrays shrink the
  frame by 16).
- gr_classify_sphere_view: nested positive tests reproduce the shared `return -1` tail, but 8966 keeps nz in memory
  where the binary keeps it on the FPU stack (z *= c, block locals tried). Not kept.
- read_items / read_inventory: the remap helper's four hashes; inline code in the loop (block or function locals)
  compiles the same. The binary has a dead `lea eax, [eax + item_types+0x5c]` after each flag store (an inline
  returning the record, not identified) and keeps i in EBP.
- mp_handle_item_drop: `vector3 pos; pos = mp_dequantize_position(...)` gives the binary's temporary copy but a
  frame 12 bytes too big; not kept.
- ls_sewer_create_avoid_spline: the binary has no `vector constructor iterator` call for avoid_spline_def's
  vector2 arrays (also not in script_build_avoid_spline 0x4ddc60): in the original the point type has no
  constructor. Fixing it means a header change to avoid_spline_def (include/engine/ai/unnamed_avoidance_0x4a6710.h)
  that touches other workers' files (unnamed_avoidance, script_internal, level_script_khosani_lab); not made.
- Not attempted in depth: events_parse_table, script_filenames_parse_table, conversation_execute_action,
  savegame_location_allowed, level_add_navpoint, the speaker / topic bit functions, two_stage_set/get,
  read_two_stage, read_automap_labels, saveload_handle_input, ui_container::draw, bbox_silhouette_corners,
  mp_player_file_create / write / load.

## Behaviour errors / game-bug candidates

- No behaviour error found; no behaviour change made. The write_header change follows the binary exactly
  (memcpy of strlen bytes into zeroed fields): in the original a level file name of exactly 0x80 characters or a
  script name of exactly 0x20 characters is written without a terminating NUL (copies at 0x4d1e3c..0x4d1e5e and
  0x4d1e7a..0x4d1ea1: `dec ecx` at 0x4d1e4f / 0x4d1e8d drops the NUL from the count). Practically unreachable;
  noted only.

## Files changed (in w14-src)

- game/level_scripts.cpp
- game/new_saveload.cpp
- game/new_playmenu.cpp
- game/unnamed_cutscene_camera_0x493e50.cpp
- vsdk/unnamed_build_stamp_0x52f450.cpp
- include/DECISIONS.md (D12300-D12303)

---

## Worker 15

### m-1020 worker 15 notes

I ran a strict VC6 8966 sweep (VC6_STRICT=1) of the 12 units in w15-src. Before: 281 match and 57 differ. After: 291 match and 47 differ, so +10 and no match lost. I checked this address by address: w15-before.txt against w15-after.txt in this folder.

mingw `-fsyntax-only -std=c++98 -w -Iinclude` over all 259 .cpp: 0 errors, except game/unnamed_decals_0x4bbe80.cpp, which has 9 (the same as before). I checked every unit because a header changed (camera.h, D12407). Decisions are D12400-D12410 in w15-src/include/DECISIONS.md.

## Per file (matches before -> after, of total)

| file | before | after | total |
|---|---|---|---|
| game/unnamed_script_editor_0x4de0b0.cpp | 96 | 98 | 120 |
| game/gametarget.cpp | 15 | 15 | 22 |
| game/unnamed_camera_0x4778b0.cpp | 41 | 43 | 47 |
| game/unnamed_ai_groups_0x492b00.cpp | 13 | 13 | 18 |
| game/multi_chattracker.cpp | 16 | 17 | 20 |
| game/unnamed_evil_joseph_0x4c3470.cpp | 16 | 18 | 19 |
| vsdk/skeleton.cpp | 26 | 26 | 28 |
| game/player.cpp | 21 | 23 | 23 |
| vsdk/unnamed_bezier_0x558a30.cpp | 14 | 14 | 15 |
| vsdk/unnamed_anim_track_0x550660.cpp | 12 | 12 | 13 |
| game/multi_door.cpp | 5 | 5 | 6 |
| game/level_script_slums.cpp | 6 | 7 | 7 |

## New matches (Confidence raised to exact)

- 0x4339e0 Player::is_leader. The if-form bool (D12400).
- 0x433790 Player::create. The group test is a nested if with ai_group_create in both elses (D12400).
- 0x4fad90 ls_lenele1b_init. One if / else-if / else chain, with the v31-v33 calls in both lower arms (D12400).
- 0x478130 camera::update_eye_position. `float h = zoom * elevation`, fabsf, and the constant 2.2573366f (D12401; see the data fix below).
- 0x478990 camera::frame. cam_vec gets operator=(const vector3&) and a default constructor, p is declared first, and the min is written as an if (D12408).
- 0x4eb9e0 script_editor_door_sound_radio_cb. Each branch has its own found test (D12402).
- 0x4e01f0 script_editor_frame. size and p are function-scope vectors assigned through temporaries, the box size is computed once per block, and each time branch has its own gr_string call (D12405).
- 0x4c3860 evil_joseph_ai_frame and 0x4c39b0 evil_joseph_ai_select_action. Branch shapes (D12406).
- 0x4ac100 irc_read_line. The received-byte branch comes first and the disconnect is its else (D12410).

## What worked

- Success first with the failure as the else (select_action, irc_read_line), and nested ifs whose tails the compiler merges (Player::create, radio_cb).
- Vectors at function scope assigned with `v = a op b`, or block scope placed so that temporaries share the binary's slots (script_editor_frame, pick_test_character, pick_helper_summon, ray_hit_object). The frame size is a reliable guide.
- A class-local `operator=(const vector3&)` to avoid a converted temporary that has a destructor (camera::frame).
- A float product moved into a local to fix the operand load order (update_eye_position).

## Improved, still differs (kept, same behaviour)

- script_create 0x4e88a0 (D12403). It now writes through script_editor_cur_script each time: the size went from 969 to 1089 bytes, against 1097 in the binary. What is left is register choice.
- pick_test_character 0x43c620 (D12404). 97 -> 40 differing lines. The frame is right; one FPU liveness difference remains.
- pick_helper_summon 0x4c3cd0 (D12406). Vector operators and a block-scoped dir.
- find_occluders 0x478520 (D12407). It returns bool now. The remaining difference is the constructor store order.
- pick_hover 0x4e2bb0 (frame now right) and ray_hit_object 0x4e2f30 (75 -> 36 differing lines), D12410.

## Still differs, tried without a match (D12409)

update_orientation (the `dec / test / jge` wrap), auto_view_update, set_mode, spline::eval (the -0.5 fold; /Op loses matches), character_can_talk_to, irc_process_input, door_sound_inputbox_cb, focus_selection, world_pick_under_mouse (the partly expanded inline members, D10808), script_write_objects (three buffers from the frame; not applied), select_prev. I did not get to these: ai_groups, skeleton, anim_track, door_pick_by_mesh, chat_process_irc_message, script_editor_create_ui / handle_input / the write_* functions, world_mouse_*, and level_item_ok.

I did not find a place to use the coordinator's two tips (the volatile global store and the rows clamp) in the functions I worked on.

## Behaviour errors / game-bug candidates

- DATA FIX (D12401): camera::update_eye_position's steep-view factor. The source had 2.25734f (0x40107842). The binary's 0x581a4c holds 0x40107834, which is 2.2573366 = 1 / (1.3 - 0.857). This is a tiny numeric change in the rebuilt exe only; the original binary is not affected.
- HEADER (D12407): camera::find_occluders returns bool. The early return writes only AL (`xor al, al` at 0x478546). Its only caller, camera::frame, ignores the result, so behaviour does not change.
- I found no game bugs.

## Files changed (in w15-src)

game/player.cpp, game/level_script_slums.cpp, game/unnamed_camera_0x4778b0.cpp, game/unnamed_evil_joseph_0x4c3470.cpp, game/unnamed_script_editor_0x4de0b0.cpp, game/gametarget.cpp, game/multi_chattracker.cpp, include/engine/camera/camera.h (D12407), include/DECISIONS.md (D12400-D12410).
