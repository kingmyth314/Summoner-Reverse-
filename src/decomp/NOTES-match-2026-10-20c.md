# Byte-match round 2026-10-20 (third): the fifteen workers' notes

Merged into src/decomp (5,317 -> 5,404 strict matches; DECISIONS D14000-D15499). One section per worker, as written.

---

## Worker 1

### m-1020c worker 1 notes

Strict sweep (VC6 SP5 8966, VC6_STRICT=1), my 11 files: 257 -> 262 matches, none lost
(w1-sweep-before.txt / w1-sweep-after.txt, `--check` passed).

| File | Before | After |
|---|---|---|
| game/items.cpp | 29/31 | 30/31 (ring_format_info 0x44b7a0) |
| vsdk/unnamed_key_0x5093a0.cpp | 23/26 | 26/26 (key_mark, key_flush, key_clear) |
| vsdk/skeleton.cpp | 26/28 | 27/28 (vim_read 0x543430) |
| vsdk/unnamed_ds_0x55bf90.cpp | 56/60 | 56/60 (ds_play closer) |
| game/conversationctl.cpp | 20/27 | 20/27 |
| game/pathfinding.cpp | 23/36 | 23/36 |
| game/unnamed_serpent_0x495290.cpp | 38/43 | 38/43 |
| vsdk/unnamed_vfx_channel_0x55b440.cpp | 17/19 | 17/19 |
| game/unnamed_states_0x431870.cpp | 13/14 | 13/14 |
| game/item_paging.cpp | 7/8 | 7/8 |
| game/unnamed_cutscene_script_0x4ed050.cpp | 5/6 | 5/6 |

## What worked (DECISIONS D14000-D14005)
- D14000 ring_format_info (the corrected listing): `speed != 0` with the 1.00 text in the else; the MP skill
  remap as a switch; resist line through n / fmt locals; Charges / Effect format strings hoisted into
  `const char *fmt`; two flags (comma, has) in the weapon requirement and an if / else of two strcat;
  block-scoped `int v` in the stat lines.
- D14001 key_tables: the key queue, the five 256-entry tables and the queue head / tail are ONE object
  (struct key_tables key_tab, 0x25d94e0, 0x1468 bytes, old names as macros, only this unit uses them).
  Explains key_flush's one-pointer loop and key_mark's reload / store order; key_down_state cleared inside the
  table loop. New technique worth trying elsewhere: when 8966 splits a multi-array loop into rep stosd, or a
  global is re-read after a store to a neighbouring array, the globals are probably one struct.
- D14003 vim_read: sizes stored / re-read through `mesh->submeshes[i]`, blocks through `sm`; the bone loop
  re-reads mesh->skin.num_ext.
- D14002 ds_play: `int flags = 0; if (looping) flags = DSBPLAY_LOOPING;` (closer: 40 -> 18 lines).
- D14004 skeleton::read: if / else of two seek calls (467 -> 17 lines), block-scoped morph counter,
  `(float)fabs(x) < 0.0001f` (D6300).

## Still differs
- skeleton::read: SIB operand order flips depending on the count of float compares modulo 4 (see D14004);
  else-branch trigger loop pointer (.name vs .time).
- ds_play: channel addressed as offset + ds_channels in the binary (pointer forms tried, D14002).
- fx_rec4c::update: frame packing of temporaries only (21 operand lines).
- state_title_enter: one scheduling pair with a volatile read (not kept).
- items_parse_table, item_page_out_all, serpent_illusion_start, pf_read_region, ds_stream_open,
  cutscene_script_load, camera_capture: register choice / tail placement / inline budget (D14002, D14005).
- Not attempted in depth: the large pathfinding and conversation camera functions, serpent_choose_attack,
  vfx_channel::eval, ds_load_buffer_acm.

## Behaviour errors / bug candidates
None found; every change is behaviour-identical (ring_format_info's speed test sends NaN to "1.00" in both forms).

## Changed files (in w1-src)
- game/items.cpp, vsdk/unnamed_key_0x5093a0.cpp, vsdk/skeleton.cpp, vsdk/unnamed_ds_0x55bf90.cpp,
  include/DECISIONS.md (D14000-D14005). No header changed. mingw -fsyntax-only: 0 errors on the four .cpp.
- Note for merge: key_tab is a new global name at 0x25d94e0 (sum.symbols calls 0x25d94e0 key_queue); hybrid /
  link builds that map globals by name may need the alias.

## Tools (scratch, w1-work/)
opdiff.mjs (operand-level diff with frame names resolved), try.mjs (vc6-try with multi-replace and operand
counts), lens.mjs (instruction length diff), side.mjs (vc6-side without the 400-instruction cap: the repo's
vc6-side.mjs and vc6-try --side cut functions at 400 instructions).

---

## Worker 2

### Worker 2 notes (match round m-1020c)

Tree: `m-1020c/w2-src`. Compiler VC6 SP5 (cl 12.00.8966), strict sweep (VC6_STRICT=1). Before:
`w2-sweep-before.txt`, after: `w2-sweep.txt`. **248 -> 256 of 288 functions in my 10 files; none lost**
(`vc6-sweep --check w2-sweep-before.txt`: 0 lost). No header or flags change. mingw `-fsyntax-only -std=c++98 -w
-Iinclude`: 0 errors in the five changed files. Decisions D14100-D14108 appended to `w2-src/include/DECISIONS.md`.

## Per file (matches before -> after / total)

| File | Before | After / total | Won |
|---|---|---|---|
| vsdk/gr_opengl.cpp | 43 | 48 / 54 | 0x54d5c0 a8, 0x54d8d0 4444, 0x54dc20 1555 scaled, 0x54f9a0 apply_tint, 0x550080 tmapper |
| vsdk/gr_font.cpp | 15 | 16 / 17 | 0x509130 gr_split_str |
| vsdk/unnamed_inputbox_0x5521a0.cpp | 9 | 10 / 13 | 0x5530b0 ui_inputbox::draw |
| game/level_script_khosani_lab.cpp | 43 | 44 / 45 | 0x4f8100 corner_pp |
| game/unnamed_snow_0x4946a0.cpp | 10 | 10 / 11 | (closer) |
| game/pathfinding_visibility.cpp | 4 | 4 / 11 | |
| game/unnamed_tiger_0x499e50.cpp | 43 | 43 / 48 | |
| game/unnamed_ai_think_0x4b5140.cpp | 25 | 25 / 29 | |
| game/gameplay.cpp | 36 | 36 / 39 | |
| game/unnamed_dialogue_table_0x4feb50.cpp | 20 | 20 / 21 | |

## What worked (details in D14100-D14106)

- **Index forms instead of walking pointers** (8966 strength-reduces them itself and then chooses the binary's
  registers and SIB base / index order): `src[y * bm_w + x]` (a8), `&buf[len - n]` (inputbox draw). This answers
  some of D12303's "base / index order" cases: try the indexed form of the loop.
- **Element pointers per iteration**: `vector3 *p = &pts[i]; float *t = tc[i]; ubyte *c = colors[i];` (tmapper),
  `ushort *sp = &s[idx]` read as `*sp` in each channel (the 4444 / 565 converters' `lea` + `[eax]` load, D11200).
- **An int local for a value converted to float once** (apply_tint): the float lands in the dead argument slot.
- **Duplicated line-end blocks** that 8966 merges into the binary's shared block (gr_split_str), with the
  overflow test written `>=` and the then-branch the binary's fall-through.
- **A float local for one operand of a sum** gives that operand's load first (corner_pp, `float ty = t.y;
  ty + out->y`).
- `switch ((unsigned)x)` gives an unsigned-ascending compare chain (gr_font_create).
- Assignment inside a product, `(t[3] = v->sw) * v->u * scale`, keeps the value on the FPU with `fst`.

## Closer, kept (behaviour identical, Confidence stays likely)

gr_font_create (159 -> 43 differing lines), gl_convert_1555_to_rgba8888 (103 -> 26), trace_beam (272 -> 100,
frame now 0x48), snow_start (9 -> 3). See D14107.

## Still differing (D14108)

read_backbuffer (store scheduling), bitmap first corner (scheduling), set_mode (EBX / EBP swap), inputbox
set_valid_chars / set_invalid_chars (8966 cross-jumps the range blocks), inputbox process (cross-jumped
duplicates, as before), gl_build_mipmaps / create_texture (large), state_game_frame (integer-move float copies),
state_game_enter / game_render_frame (known), the pathfinding_visibility EH functions (register allocation),
tiger functions and tiger_anim_remap_in (register choice; the serpent / phoenix / ghost copies have the same
difference), ai_think pick_attack_target (-1 in EBP), goal_do_travel (keep flag spilled into goal_flags' slot),
choose_attack_slot / party_spread_attackers (frames), conversation::parse_table (large, not attempted).

The coordinator's tip (reuse the `w, h` locals of gr_get_string_size) had no remaining target in my files: the
only differing function calling it, ui_inputbox::process, differs by cross-jumping.

## Behaviour errors / bug candidates

None found. No behaviour was changed.

## Changed files

- w2-src/vsdk/gr_opengl.cpp
- w2-src/vsdk/gr_font.cpp
- w2-src/vsdk/unnamed_inputbox_0x5521a0.cpp
- w2-src/game/level_script_khosani_lab.cpp
- w2-src/game/unnamed_snow_0x4946a0.cpp
- w2-src/include/DECISIONS.md (D14100-D14108)

Scratch helpers (mine, outside the tree): w2-tryx.mjs / w2-tryx.ps1 (vc6-try with an operand-level differing-line
score), w2-norm.mjs, w2-apply.mjs, w2-mkvar.mjs, w2-gen-*.mjs, w2-v-*.json; kept compile folders in w2-tmp.

---

## Worker 3

## Worker 3 (m-1020c)

Strict VC6 sweep (cl 8966) of the 10 files, before / after: **315 -> 322 match** (33 differ, 0 not found),
0 lost (`vc6-sweep --check w3-sweep-before.txt`). Lists: `w3-sweep-before.txt`, `w3-sweep.txt`.
mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in the 5 changed files. No header changed
(`diff -rq base w3-src` lists only the five .cpp files and include/DECISIONS.md). Decisions: D14200-D14206 in
w3-src/include/DECISIONS.md.

## Per file (match before -> after, of total)

| File | Before | After | New exact |
|---|---|---|---|
| vsdk/unnamed_net_0x517590.cpp | 23 | 26 / 31 | nw_accept_reliable 0x518ce0, nw_NewSendReliable 0x518d70, nw_ConnectToServer 0x518f30 (D14204) |
| game/boss_ai.cpp | 53 | 55 / 58 | boss_ai_parse_table 0x4a1790 (D14200), boss_target_list_build 0x4a2f60 (D14201) |
| game/multi_chattracker.cpp | 17 | 19 / 20 | irc_add_user 0x4adb40 (D14202), irc_process_input 0x4adcd0 (D14203) |
| game/script_internal.cpp | 20 | 20 / 28 | (walk_to, replace_weapon closer) |
| game/level_scripts_common.cpp | 72 | 72 / 78 | (level_parse_shopkeeper closer) |
| game/s3d.cpp | 30 | 30 / 34 | |
| vsdk/character.cpp | 32 | 32 / 34 | |
| game/level_script_worldmap.cpp | 21 | 21 / 23 | |
| game/unnamed_party_inventory_0x46b460.cpp | 29 | 29 / 30 | |
| game/unnamed_evil_joseph_0x4c3470.cpp | 18 | 18 / 19 | |

## What worked

- **The corrected asm.mjs listing** (boss_ai): both flagged functions fell once the real prologue was visible.
  boss_ai_parse_table: the record pointer and the next one as two variables, `next = table[--stage]; do { a =
  next++; ...`. boss_target_list_build: `n = 0` per branch and `list[n].field` instead of `list++` (frame 0x28
  -> 0x24; the induction pointer then takes the dead `list` slot as in the binary).
- **Repeated tails (D14202).** A deferred `add esp` that merges a call's pops across a `jmp` into shared code
  means the source has the tail on both paths and the compiler cross-jumped them (irc_add_user).
- **Fall-through decides the cross-jump direction (D14203).** irc_process_input: with `return NULL` in each case
  8966 keeps the later copy; when list / quiet fall through to the final `return NULL`, the earlier (xyz) copy
  survives, as in the binary.
- **`#pragma optimize("w", on)` for the end of the net object (D14204).** Three consecutive functions match only
  with /Ow (or /Oa); a region from 0x518ce0 to the end changes nothing else. The pragma can start anywhere
  after nw_buffer_push. Tried per function on every other differing function of my files: no other gain. Worth
  a try on other vsdk units whose differences are store scheduling around calls.
- Direct global indexing `arr[count].f = ...` instead of a record pointer (replace_weapon: the binary keeps the
  count in a register and old1 in EAX).
- `goto next` out of a duplicate scan instead of `break` + `if (i < n) continue` (the binary jumps from the match
  straight to the loop test).

## Closer, still differing (kept, same behaviour; D14205)

- script_action_walk_to 0x4dc6e0: 90 -> 38 differing instructions, same size; the binary keeps `so` in EBX all
  through, `np` in the frame and `a` in the dead `so` slot.
- script_action_replace_weapon 0x4dccb0: 65 -> 3 differing lines; only the c (ESI) / count-and-`a` (EDI) pair
  is swapped.
- nw_NewCloseSocket 0x5191c0: the binary's block layout (if (valid) { ... } else sprintf); the socket pointer
  should be in EBX from the entry.
- level_parse_shopkeeper 0x4cc4d0: 88 -> 65; the unknown-item block sits after the sell scan in the binary.

## Still differs (tried; D14206)

boss_threat_add, boss_ai_unload / boss_move_in_range_dispatch (other units' declarations, D7603), level_trigger_
point_left_of_line, ls_worldmap1_pick_encounter_stage9 / stage15, evil_joseph_pick_helper_summon,
script_action_combat_mode (24 declaration orders), script_action_replace_armor, ca_character::add_skeleton
(the epilogue belongs to the main path in the binary), report_memory, nw_ras_init (cross-jump target),
nw_buffer_pop, nw_init_networking, nw_WorkReliable (307 lines), chat_process_irc_message (1,600 lines),
script_parse_objects / script_entry_start / script_entry_check_done / script_build_avoid_spline, s3d_read_meshes /
mesh_ref / door / color_record, character_equip_item, party_set_level, level_check_triggers / parse_table /
script_parse_table (not attempted in depth: large or tried at length before).

## Behaviour errors / game-bug candidates

None found. Every change is a source-form change with the same behaviour (checked against the listing:
`table[--stage]` with stage reassigned each pass; `list[n]` is the old `list` + n; the duplicated irc tail; the
fall-through paths of irc_process_input return NULL as before; `price <= 1` vs `< 1` gives the same result).

## Files changed (in w3-src)

game/boss_ai.cpp, game/multi_chattracker.cpp, vsdk/unnamed_net_0x517590.cpp, game/script_internal.cpp,
game/level_scripts_common.cpp, include/DECISIONS.md (D14200-D14206).

Scratch tools (w3-tools/): try.mjs (multi-edit variants, per-variant address, mnemonic-diff count), apply.mjs,
survey.mjs (differing-line count per function), pragma.mjs / region.mjs (wrap functions or ranges in
`#pragma optimize`).

---

## Worker 4

### m-1020c worker 4

Strict sweeps (VC6_STRICT=1): before `w4-sweep-before.txt` (my 10 files), after `w4-sweep-after.txt` (full tree:
259 units, 5,323 match, `--check src/decomp/vc6-matches.txt`: 5,317 there, 5,323 now, 0 lost).

| File | Before | After | of |
|---|---|---|---|
| vsdk/unnamed_listbox_0x52ca60.cpp | 61 | 62 | 69 |
| vsdk/gr_glide.cpp | 53 | 55 | 61 |
| game/ai.cpp | 82 | 83 | 88 (sweep lines; 0x472200 is listed twice) |
| game/characterinfo.cpp | 65 (+1 target) | 67 (+1 target) | 70 |
| game/new_pxochat.cpp | 53 | 53 | 57 |
| game/multisave.cpp | 5 | 5 | 8 |
| game/unnamed_pyrul_0x4b87c0.cpp | 21 | 21 | 23 |
| game/multispell.cpp | 20 | 20 | 22 |
| game/unnamed_machival_0x4c0c40.cpp | 25 | 25 | 26 |
| game/unnamed_evil_urath_0x4c4170.cpp | 17 | 17 | 18 |
| total | 402 | 408 | |

New matches (Confidence raised to exact): 0x52d5c0 ui_listbox::ui_listbox, 0x5492f0 gr_glide_create_texture,
0x547cc0 gr_glide_create_mipmapped_texture, 0x446e90 character_can_use_summon, 0x4452f0 characterinfo_parse_table,
0x472e80 ai_update_perception.

## What worked (DECISIONS D14300-D14306)
- /Ob0 unit: an inline member constructor is called out of line; the binary's member-init stores came from the
  constructor body (list_init) scheduled before the vtable store. Head subclass removed (HEADER, D14300).
- Statement order inside a store group (x += before the h store), if/else of two stores instead of min().
- `*d++` running pointers instead of indexed stores; ushort nibble locals (no AND folding); zeroing order picks
  register roles; switch cases in the binary's block order; initialisers moved before a call.
- Stores through `entry->sections->` each time instead of a local pointer.
- Shared tails: write the duplicated code out (no goto / no early `return false` per block) and let 8966
  cross-jump: else-if chain with one final `return false` (can_use_summon); visible tail written twice
  (ai_update_perception); leap code per HP band (evil_urath, closer).
- Two separate global arrays where the binary indexes two absolute bases (HEADER characterinfo.h, D14305);
  a for loop of NULL stores for a rep stosd whose lea comes first; a float literal one ulp below 1/30.
- Tooling (scratch, not committed): an LCS diff of normalised instructions (ours from vc6-side's left column,
  orig from `asm.mjs fn <addr> 20000`). vc6-side's own right column stops early for long functions (399 lines
  for parse_table) and ignores register names; the LCS diff found most of these wins.

## Still differs
- alloc_level_buffers: `target` only, sum load order (not name-dependent: renames tried).
- pick_attack_slot / claim_attack_slot: get_center x operand order (known).
- call_for_help, ai_state::load, cast_effect_track_add, setup_head, learn_chain_skill, read_summary, file_write,
  file_load, multispell x2, pxochat_page_down / drag_scrollbar, listbox process / draw / scroll_page_up /
  set_vscroll_fraction / wrap_text / snap_scroll_bottom, tcache_set: register allocation / frame slot choice.
- add_entry: unrotated content_height loop (D14301). load_section: tail store schedule. alloc: 5 store-order
  hunks left (D14303). gr_glide_bitmap: vertex store scheduling (frame fixed). evil_urath select_action: which
  `return 0` copy is kept. pxochat_handle_click: constant -1 in EBX, shared blink tail.
- gr_glide_create_texture_section, tmapper_multitexture (separate per-field induction variables in Sum.exe),
  pyrul choose_attack / fire_attack, machival select_action, pxo_parse_chat_banners_cfg: not solved.
- Coordinator tip (reuse gr_get_string_size w/h locals) does not fit wrap_text: Sum.exe has one slot more there.

## Behaviour errors / bug candidates
None found; every change keeps the behaviour (checked against the listing at the addresses in D14300-D14306).

## Changed files (in w4-src)
- vsdk/unnamed_listbox_0x52ca60.cpp, vsdk/gr_glide.cpp
- game/characterinfo.cpp, game/ai.cpp, game/unnamed_evil_urath_0x4c4170.cpp, game/new_pxochat.cpp
- include/vsdk/ui/ui_gadgets.h (HEADER D14300: ui_listbox_entry_head removed, `entries` is ui_listbox_entry)
- include/engine/characterinfo/characterinfo.h (HEADER D14305: default_footstep_sounds_left / _right)
- include/DECISIONS.md (D14300-D14307)

Checks: mingw `-fsyntax-only -std=c++98 -w -Iinclude` over all 259 units: only unnamed_decals' known 9 errors.
Full strict sweep after the header changes: 0 lost.

---

## Worker 5

### m-1020c worker 5 notes

Strict sweep of my 10 units: 276 -> 283 matches (of 316), 0 lost (`vc6-sweep --check src/decomp/vc6-matches.txt`).
Lists: m-1020c/w5-sweep-before.txt, m-1020c/w5-sweep.txt. Decisions D14400-D14408 in w5-src/include/DECISIONS.md.
No header changed. No recursive delete was run; scratch tools live in m-1020c/w5-work.

## Counts (strict, before -> after / total)

| file | before | after | won |
|---|---|---|---|
| game/living_entity.cpp | 122 | 125 / 132 | 0x44ddb0 scale_stats_to_level, 0x457150 save_state, 0x458800 load_state |
| game/new_multichat.cpp | 8 | 11 / 12 | 0x41e900 chat_draw_lines, 0x41ebd0 chat_draw_input, 0x41e6b0 chat_input_add_char_wrap |
| vsdk/unnamed_vif_0x5329f0.cpp | 10 | 11 / 15 | 0x5337a0 vif_instantiate_level |
| game/spells.cpp | 32 | 32 / 39 | (spells_parse_table closer) |
| game/levelrender.cpp | 7 | 7 / 12 | (levelrender_face_vertex_colors closer) |
| game/new_options.cpp | 32 | 32 / 35 | |
| game/new_buysell.cpp | 40 | 40 / 42 | |
| game/unnamed_projectors_0x4c0250.cpp | 9 | 9 / 11 | |
| game/unnamed_level_complete_0x4b0960.cpp | 9 | 9 / 10 | |
| game/unnamed_experience_0x489930.cpp | 7 | 7 / 8 | |

All seven wins are `Confidence: exact` with the shape in the comment.

## What worked

- **Probe compiles** (w5-work/micro/*.cpp, cl /O2 /FAs with `#pragma optimize("", off)` for /Od): quickest way to
  find which source form gives an instruction. Found: `a / ((float)i)` -> fild / fdivp (bare cast -> fidiv);
  `x == 1 ? true : false` -> cmp / sete, `b != 0 ? true : false` -> test / setne in /Od (D14403, D14407).
- One call per switch case so 8966 cross-jumps them into `push arg; jmp call` (D14400).
- Reusing an address-taken local (x after chat_get_origin) for a later value: the binary stores it to that slot
  (D14401). `x &= (x > 0) - 1` for `if (x > 0) x = 0`.
- Nested if / else with the normal path first instead of early returns, when the binary has the short blocks at
  the end (D14402, D14405).
- do-while(0) macro shapes in /Od: the loop-back targets show which statements had their own do-while (D14407);
  after changing them the block locals needed re-ordering / renaming by the D6101 hash.
- Array indexing at each use instead of a pointer local, to stop store hoisting / pick the binary's induction
  (D14405, D14404).
- Tools (scratch): w5-work/fd.cjs (full normalised diff with frame symbols resolved from the .cod),
  tryfd.cjs (variant scoring on it, for /Od and long functions where vc6-side cuts at 400 lines), offs.cjs (first
  byte-offset divergence: finds short/near jump differences when the instruction streams are equal), frame.cjs
  (our frame symbols -> the binary's offsets).

## Still differs (tried; D14408)

options_init (forwarding of the first dword), spell_roll_amount (trip count), spell_target_value (shared return
blocks), Character::say, options_load_bitmaps, buysell_count_party_items / handle_click (register choice),
mp_handle_combat_sequence, to_lobj_space (plane::operator= expanded where the binary calls it), handle_key (block
order), vif_record_get_bounds / vif_draw_particles (slot choice), options_drag_sliders, summon_create (identical
get_int branches merged by 8966), Character::Character (needs the combat member as a real member, D1702), the
large /Od living_entity functions and the levelrender draw lists, spell_prepare_cast / cast_execute /
launch_effects, vif_compact_particles / vif_emitter_update, projector_collect_lobj_faces, xp_level_up_gains.

## Behaviour errors / bug candidates

None found. All changes keep behaviour: the reordered unlink in vif_instantiate_level loads prev / next before any
store (the binary does the same); `total` reads the same global value as before; the bool ternaries and casts
give the same values.

## Changed files (in w5-src)

- game/new_multichat.cpp
- game/living_entity.cpp
- vsdk/unnamed_vif_0x5329f0.cpp
- game/spells.cpp (closer only)
- game/levelrender.cpp (closer only)
- include/DECISIONS.md (D14400-D14408 appended)

mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in the five changed .cpp files.

---

## Worker 6

### m-1020c worker 6 notes

Strict sweeps (VC6_STRICT=1) of the nine assigned units in `w6-src`: before 235 match / 40 differ, after
242 match / 33 differ, `--check` against the before list: 0 lost. Lists: `w6-sweep-before.txt`,
`w6-sweep-after.txt`.

## Counts per file (match / functions compared)

| File | Before | After | New matches |
|---|---|---|---|
| game/unnamed_avoidance_0x4a6710.cpp | 50 / 67 | 50 / 67 | |
| vsdk/unnamed_localize_0x503f60.cpp | 23 / 28 | 23 / 28 | |
| game/unnamed_ai_groups_0x492b00.cpp | 13 / 18 | **15** / 18 | 0x492db0 ai_group_combat_think, 0x493ac0 ai_group_collect_enemies |
| game/new_automap.cpp | 28 / 32 | **29** / 32 | 0x403350 automap_label_text_input |
| game/new_saveload.cpp | 34 / 37 | **36** / 37 | 0x4258c0 saveload_scan_saves, 0x426d10 saveload_draw_info |
| game/unnamed_netsession_0x440b40.cpp | 19 / 21 | 19 / 21 | |
| game/new_charselect.cpp | 30 / 32 | 30 / 32 | |
| game/unnamed_giant_salamanka_0x4c8030.cpp | 18 / 19 | **19** / 19 | 0x4c87e0 giant_salamanka_ai_select_action |
| game/unnamed_frame_0x4323e0.cpp | 20 / 21 | **21** / 21 | 0x432da0 game_is_paused |

Confidence raised to exact on the seven new matches only. Decisions D14500-D14507 appended to
`w6-src/include/DECISIONS.md`.

## What worked

- game_is_paused (D14500): the if-form `if (game_flags & 4) return true; return false;` gives `shr al, 2 / and al, 1`.
- giant_salamanka_ai_select_action (D14501): candidates written through an element pointer (`cand_t *e =
  &cand[count++]`, handle first), `goto next` out of the duplicate test, the pick as `-1` / found / `if (pick == -1)
  pick = 0`, vector3 sums (`pt->pos = pos + owner->centre_offset`, `vector3 d = t->pos + t->centre_offset -
  gsal.offense_points[...].pos`), `count = 0` after the busy test, `if (pick >= 0) { ... }` before `none:`.
- saveload_draw_info (D14502): `char ampm[3]`, `char buf[260]` give the frame (0x25c) and slot order. The array
  sizes decide frame slots; a small script listing the `.cod` frame offsets per variant (w6-try/frame.js) found it.
- saveload_scan_saves (D14503): direct `saveload_entries[saveload_num_saves].field` indexing (no entry pointer) and
  `list.prev = list.next = &list`.
- automap_label_text_input (D14504): one `if (changed) automap_select_label(...)` after the key block; VC6 threads it.
- ai_group_collect_enemies (D14505) and ai_group_combat_think (D14506): **the member read as `ai_think_members[i]`
  at every use** (no `Character *m` local; the binary reloads `[esi]` after each call). Plus `vector3 dir; dir = a -
  b;`, the dot through a reference in x / z / y order, `goto found`, fabsf, and the AP ratio as two float locals.
  The same reload pattern is likely in other ai_think_members loops of other units.

## Still differing (tried; details in D14507)

- netsession mp_session_reset (pointer loop: right registers but `jb`), mp_player_alloc (`lea ecx, [eax+4]`).
- charselect mp_charselect_page_down (list in EDX), mp_charselect_handle_click (frame 0x14 vs 8, large; a
  `ui_button_state *b` pointer plus `(unsigned)flags >> 3` fixes the first loop's shr but not its registers; reverted).
- automap center_on_player (8966 sinks x / y into the branch), handle_click (button pointer `b` kept: first
  difference +0x20 -> +0x52), draw_map (frame 0x30 vs 0x34, many slots).
- saveload_handle_input (row-hit block placed inside the description-box code in the binary).
- ai_groups think_style1 / style2 (register colouring), think_caster (frame 0x6c vs 0x80).
- localize: set_string (id in EBX, dest in the frame in the binary); the others not attempted again.
- avoidance: box_box_overlap reached +0x84 (`vector2 d; d = a->centre - b->centre;` gives a in EDX, `const vector2
  &u = b->axis_a;` for k_ab / k_aa; left: term order of the first two tests) but not kept; the progress copy is
  `w6-try/avoid.boxbox-progress.cpp`. area_rebuild (inline return), spline_point_get (ten more shapes),
  merge_obstacle_polys (frame +32, different loop shape). The large clip / create / add functions not attempted.

## Behaviour errors / bug candidates

None found. Every change has the same behaviour (buffers in draw_info only grow).

## Changed files (in w6-src)

- game/unnamed_frame_0x4323e0.cpp
- game/unnamed_giant_salamanka_0x4c8030.cpp
- game/new_saveload.cpp
- game/new_automap.cpp (label_text_input match; handle_click's first loop through `b`, still differs)
- game/unnamed_ai_groups_0x492b00.cpp
- include/DECISIONS.md (D14500-D14507 appended)

No header declaration changed. mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors on each changed .cpp.
new_charselect.cpp and unnamed_avoidance_0x4a6710.cpp were tried and restored byte-identical to the base.

---

## Worker 7

### m-1020c worker 7 notes

Strict sweeps (VC6_STRICT=1) of the ten units, before (w7-sweep-before.txt) and after (w7-sweep-after.txt):
239 -> 242 matches, 0 lost (`vc6-sweep.mjs --check` against the before list).

| File | Before | After | Won |
|---|---|---|---|
| game/combat.cpp | 51 / 63 | 52 / 63 | 0x467df0 living_combat::resolve_hit |
| game/gametarget.cpp | 15 / 22 | 16 / 22 | 0x43a770 world_pick_lobj |
| game/new_credits.cpp | 7 / 9 | 8 / 9 | 0x408ba0 credits_draw_lines |
| game/unnamed_shadows_0x4a37d0.cpp | 8 / 13 | 8 / 13 | (two closer) |
| vsdk/unnamed_floatmath_0x542500.cpp | 16 / 20 | 16 / 20 | (one closer) |
| vsdk/unnamed_vfile_0x5128b0.cpp | 79 / 82 | 79 / 82 | |
| game/summoner_cd.cpp | 3 / 6 | 3 / 6 | (the no-CD patch, not compilable) |
| game/unnamed_netpackets_0x441820.cpp | 10 / 12 | 10 / 12 | |
| game/unnamed_ghost_rider_0x4b9e70.cpp | 37 / 38 | 37 / 38 | |
| game/unnamed_gamelist_0x4705b0.cpp | 13 / 14 | 13 / 14 | |

All three wins are `Confidence: exact` now. DECISIONS.md D14600-D14606 (appended to the copy's include/DECISIONS.md).

## What worked

- **Initialiser order = declaration order** (D14600): world_pick_lobj's `iter, index, flag, best` found by a
  permutation search of the four initialised declarations.
- **Two changes that only match together** (D14601): resolve_hit needed `int no_flinch` (EDI) *and* no cached
  `Character *t` (read `opponent->owner` in place). Each alone made the code worse, so test combinations.
- **In-memory clamp = `float_min(float_max(x, 0.0f), 1.0f)`** (D14602, D14603): vector.h's inline helpers by
  value keep the value in memory (here in a dead argument slot), give the `fst` / two reloads and the
  out-of-line `fld 0.0; jmp` zero case. credits_draw_lines matched; shadow_project_face got its frame and clamp.
  Worth trying on other "in-memory clamp" functions (D7702's s3d_read_color_record, snd volume clamps).
- **Block scope for a dead argument slot** (D14604): a block-scoped `bool first` starting *after* the last use of
  `hit` in the source got hit's argument slot.
- Vector temporaries built in place (`vector3 pr = d.rotated(...)`, `vector3 world = a.unrotated(m) + pos`)
  instead of assignment to function-scope vectors (D14603).

## Closer, still differing (kept; behaviour identical)

- lobj_raycast_draw_lists 0x4a4bb0: frame now right; only ESI / EDI swapped (hit vs the zero / counter web).
- shadow_project_face 0x4a46a0: frame and clamp right; the vertex pointer is biased to &v->a in EBP (binary: v in
  EDI, clipped pointer in EBP).
- float_format_buf 0x542830: first difference moved from +0x1c to +0x3f (n declared first).

## Still differing (see D14606 for the forms tried)

fast_inv_sqrt / fast_sqrt / fast_sqrt_refined (register choice), mp_reset_net_stats (separate EDX zero),
File::set_cipher_key (a local bit-field cast gives the zero branch exactly, not kept), get_async_status,
file_list_files (frame order), mp_handle_actv_gm, character_can_talk_to, pick_test_character, ai_choose_special,
get_protection, ghost_anim_remap_in (D7702, not retried), cmd_disp_rate, combat_position_modifier (48 forms
earlier), the large ones not attempted beyond a first look: fire_weapon_missile, resolve_exchange,
melee_damage, combat_get_weapon_tag_position (matrix inline expansion), chain_check_window,
trigger_special_ability / trigger_reflex_ability (the binary holds -1 in EDI and reloads owner),
blood_splot_place, world_mouse_frame, world_pick_under_mouse (inline / COMDAT mix), world_mouse_hover_target,
object_try_unlock_or_open (113 bytes shorter: tail merging), obstacles_debug_render (359 bytes shorter),
shadow_record_add, shadow_transform_to_lobj, credits_parse_table. summoner_cd's three are the no-CD patch.

## Behaviour errors / bug candidates

None found. The changes are all behaviour-identical (the clamp rewrite differs from the old ternary only for
a NaN, which the integer quotients in credits_draw_lines and the depth fade in shadow_project_face cannot give;
the binary's code is the float_min / float_max form).

## Changed files (in w7-src)

- game/gametarget.cpp (world_pick_lobj)
- game/combat.cpp (resolve_hit)
- game/new_credits.cpp (credits_draw_lines)
- game/unnamed_shadows_0x4a37d0.cpp (shadow_project_face, lobj_raycast_draw_lists; closer only)
- vsdk/unnamed_floatmath_0x542500.cpp (float_format_buf; closer only)
- include/DECISIONS.md (D14600-D14606 appended)

No header changed. mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in each changed .cpp.

## Housekeeping

Compiler temp folders (vc6cmp-*) went to m-1020c\w7-tmp (TEMP pointed there), not to the user's Temp. Scratch
scripts and variant files are m-1020c\w7-*.mjs / .json / .ps1 and the w7-bak-*.cpp backups; nothing deleted.

---

## Worker 8

### m-1020c worker 8 notes

Tree: `m-1020c/w8-src`. VC6 SP5 (cl 12.00.8966), strict (`VC6_STRICT=1`). Sweeps of my 7 units:
`w8-before.txt` (197 match / 41 differ) and `w8-sweep.txt` (**206 match / 32 differ**),
`vc6-sweep --check w8-before.txt`: 0 lost. mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in both
changed files. No header and no flags change. Decisions D14700-D14706 appended to `w8-src/include/DECISIONS.md`.

## Per file (before -> after / total)

| file | before | after | won |
|---|---|---|---|
| game/level_scripts.cpp | 92 | 100 / 124 | 0x4d0590 has_conversation_data, 0x4d1770 hot_button_get, 0x4d1df0 write_header, 0x4d2150 write_living, 0x4d2c90 read_inventory, 0x4d31e0 read_item_objects, 0x4d39c0 events_parse_table, 0x4d6100 script_filenames_parse_table |
| vsdk/unnamed_checkbox_0x553ba0.cpp | 10 | 11 / 12 | 0x5548a0 ui_radio_button::create |
| vsdk/unnamed_vfx_0x536950.cpp | 26 | 26 / 28 | |
| game/conversation.cpp | 25 | 25 / 27 | |
| game/unnamed_swi_0x488230.cpp | 13 | 13 / 14 | |
| game/unnamed_cutscene_0x434900.cpp | 25 | 25 / 26 | |
| game/level_script_ikaemosu.cpp | 6 | 6 / 7 | |

All 9 won functions are `Confidence: exact` with the shape in the comment.

## What worked

- **memcpy reads of save-buffer fields (D14700).** The binary's "load into a register, then test" and "byte in a
  frame slot read back as a dword with `and 0xff`" both come from `memcpy(&v, p, sizeof v)`: has_conversation_data,
  hot_button_get, read_inventory's closing size, read_item_objects' lock byte. Worth trying wherever the binary
  keeps a just-read byte in a stack slot (mp_handle_swi_path got one step closer with it).
- **Top-tested `while` loops in table parsers (D14702)**: `while (!Accept("#End")) {...}` instead of
  `if (!Accept) do {...} while (!Accept)`; the do-while duplicated the loop head. Plus every field through the
  CONVERSATION_SAVE_INFO macro (script_filenames_parse_table) and an index loop for the tolower pass
  (events_parse_table): 8966 strength-reduces an index loop and creates the pointer after the guard, the binary's
  placement; a pointer loop sets it before the guard.
- **`off += 4; return off <= size;` (D14705)** at the end of writers instead of `return off + 4 <= size`:
  fixed the compare registers of write_header and the whole-function constant-4-in-EDI of write_living.
- **Constructor temporaries + block-scoped pos (D14704)** for read_item_objects' frame (slot sharing).
- **Inline helper written out (D14703)**: savegame_remap_item_hash expanded in read_inventory.
- **Store order search (D14701)**: radio_button::create as checkbox::create plus the font / label_x / label_y order.
- Helper: `w8-t/mytry.mjs` (vc6-try with replace-all variants, the first differing instruction with operands, and
  a count of differing mnemonic lines with `--side`).

## Closer, kept (behaviour unchanged)

- read_automap_labels 0x4d3740: the label through `automap_label *l`; every instruction matches; only count's
  frame slot differs (it comes after x / z in ours).

## Still differs (D14706)

SIB base / index order (hot_button_set, level_item_add); the search loops whose hash pointer the binary creates
after the count test with h in a register (level_item_has / _remove, read_level_items, get_speaker_bit); register
choice (get_int, level_format_display_name, read, conversation::restore_state, vfx init_nodes); frame slot order
(write_item_objects, read_automap_labels, layout_conversation, automap_label_remove); read_items (strength
reduction of the index); two_stage_get / set, read_two_stage, the topic bit functions (spills, down-counting loop);
level_add_navpoint (fadd operand order); cutscene_parse_table (/Od register of `num_fogs++`); mp_handle_swi_path
(the `this` lea before the walk load); ui_checkbox::draw (gr_line scheduling); ls_ikaemostopint_init (constant 1
in EBX); conversation::clear_speaker (needs a conversation_speaker member: a header change, D10904 (1), not made);
conversation_execute_action, init_conversation_slot, write_party_state, vfx process: not attempted in depth.

## Behaviour errors / bug candidates

None found; no behaviour change made. Every change is an equivalent form (memcpy of the same bytes, equivalent
loop shapes, reordered independent stores, a local increment after its last use).

## Files changed (in w8-src)

- game/level_scripts.cpp
- vsdk/unnamed_checkbox_0x553ba0.cpp
- include/DECISIONS.md (D14700-D14706 appended)

---

## Worker 9

### m-1020c worker 9

Strict sweep of my 12 units: 335 -> 337 matches (of 376), none lost (`--check src/decomp/vc6-matches.txt`:
335 matched there, 337 now, 0 lost). Lists: `w9-sweep-before.txt`, `w9-sweep.txt`.

| File | Before | After | Of | New matches |
|---|---|---|---|---|
| vsdk/unnamed_quaternion_0x5225b0.cpp | 21 | 21 | 32 | |
| game/new_inventory.cpp | 62 | 63 | 69 | 0x416c10 inventory_use_selected_item |
| vsdk/gr.cpp | 56 | 56 | 61 | |
| vsdk/unnamed_gr_3d_0x50d9b0.cpp | 35 | 35 | 39 | |
| game/unnamed_camera_0x4778b0.cpp | 43 | 44 | 47 | 0x478520 camera::find_occluders |
| vsdk/unnamed_vfx_mesh_0x562a30.cpp | 7 | 7 | 9 | |
| vsdk/unnamed_ui_slider_0x55e040.cpp | 24 | 24 | 26 | |
| game/new_itemtrade.cpp | 34 | 34 | 36 | |
| vsdk/unnamed_vfx_material_0x564d70.cpp | 14 | 14 | 15 | |
| game/unnamed_titus_0x4c20e0.cpp | 26 | 26 | 27 | |
| game/unnamed_alt_textures_0x4aac10.cpp | 3 | 3 | 4 | |
| game/level_script_ionaext.cpp | 10 | 10 | 11 | |

Both new matches are `Confidence: exact` with the form in the comment (D14800, D14802).

## What worked

- **D14800, inventory_use_selected_item** (one of the three functions asm.mjs used to misread): with the fixed
  listing the list branch is the equipped branch's shape. Row index as a local, `self_only` first with its own
  return, then `if (flags & 4) {..} else if (flags & 2) {..} else {..}` without returns in the chain.
  `(flags & 4) || (flags & 2)` is folded by 8966 into `test [mem], 6`; the chain is not, and the whole-function
  change also stopped the equipped branch's two begin blocks being cross-jumped.
- **D14802, camera::find_occluders**: the second ray resets index / blocked before start / dir, and
  `return a > 0 || b > 0;` for a bool function gives the binary's `xor eax, eax` / `mov eax, 1` (an
  `if (..) return 1; return 0;` gives `xor al, al` / `mov al, 1`). Worth trying on other bool functions whose
  binary returns in EAX.
- **D14801 (closer only), inventory_handle_click**: `d` / `b` pointer locals at the top of the button loop
  (skills_handle_click's shape) give the binary's loop registers; LCS distance 277 -> 220.

## Still differs (tried; LCS distance against the full listing, TRY_FULL=1)

- new_inventory: identify_item (6), destroy_selected_item, equip_item: only `and edx, eax` vs `and eax, edx` in
  the max-0 clamp; 34 more forms (D14803). handle_click (220): -1 in EBX not EBP, return tails / shared break
  target 0x415a32. popup_result (22), drag_to_equip (65): not tried further (w15 had tried both).
- quaternion: exp (5) keeps sqrt on the FPU stack (`fstp st(3)` then `fst [esp]`); 15 forms. Rest closed by
  earlier notes (to_matrix 8 / to_matrix_b, multiply 12, cquat ones 24-28). from_matrix_b 0x522790 is 157.
- gr_3d: gr_setup_3d (6) Glide test threaded (8 + 3 volatile forms); gr_cull_bounding_box_transformed (2) is
  the plane() call under /Ob0 (D4502, header-level); gr_get_frustum_planes (18) count in ECX not EAX (6 forms);
  gr_project_vertex (24 -> 20 with a flags local, not kept) the binary reloads flags for the last return and
  keeps the early return in line (5 forms).
- camera: auto_view_update (8) `push ebx` shrink-wrapped after the early exits (3 forms); set_mode 0x479930 (6)
  per D13105; update_orientation 0x477f60 (39) per D12409.
- gr.cpp: gr_line (8) integer copies of clip_top / clip_bottom scheduled earlier (compound assignment forms);
  gr_bitmap_float 0x507100 (37) float schedule; 0x505e00 (116), 0x5061f0 (223), 0x507a80 (33) not tried.
- itemtrade_count_party_rows (7): `inc dword ptr [rows]` after the store (volatile, pointer, inline helper,
  local forms). itemtrade 0x419710 (193) not tried.
- vfx_tex_layer::get_frame_bitmap (32): this / n register swap (D12107). ui_slider 0x55ed30 (38) per D12107,
  0x55e040 (162). level_script_ionaext 0x4f7270 (4): the sete (D12107). alt_textures (73): shrink-wrapped pushes
  (D11006). titus 0x4c2790 (90), vfx_mesh 0x562f60 (422) / 0x563a40 (207): too large, not tried.

## Behaviour errors / bug candidates

None found. All changes keep the behaviour (an independent store moved, an if / else-if chain equal to the `||`
test, the same bool result written as an expression, pointer locals for the same array elements).

## Changed files (in w9-src)

- game/new_inventory.cpp (0x416c10 rewritten, 0x4159a0 pointer locals, two marker comments)
- game/unnamed_camera_0x4778b0.cpp (0x478520 and its marker)
- include/DECISIONS.md (D14800-D14803 appended). No header changed.
- mingw `-fsyntax-only -std=c++98`: 0 errors in both changed .cpp files.

Tooling (scratch, `w9-work/`): `try.mjs` is worker 15's scorer (m-1020b) with the root changed to w9-src;
`side.mjs` writes one function's vc6-side output. Note: piping vc6-try / try.mjs output into
`Select-Object -First` kills it before it restores the file; I lost no work (restored from base at once), but
write the output to a file instead.

---

## Worker 10

### Worker 10, match round 2026-10-20c

Strict sweep of my 10 units: before 316 / 357 (w10-sweep-before.txt), after 320 / 357 (w10-sweep-after.txt),
none lost. `--check src/decomp/vc6-matches.txt` on the two changed units: 121 -> 125, 0 lost.

| File | before | after | of |
|---|---|---|---|
| game/unnamed_script_editor_0x4de0b0.cpp | 98 | 102 | 120 |
| game/new_levelselect.cpp | 23 | 23 | 25 |
| vsdk/unnamed_lights_0x51e070.cpp | 37 | 37 | 41 |
| vsdk/unnamed_ca_mesh_0x50af80.cpp | 8 | 8 | 12 |
| game/unnamed_decals_0x4bbe80.cpp | 12 | 12 | 15 |
| vsdk/unnamed_ui_button_0x526940.cpp | 22 | 22 | 24 |
| vsdk/unnamed_vfile_rw_0x525720.cpp | 50 | 50 | 51 |
| vsdk/cmdline.cpp | 15 | 15 | 16 |
| game/objects.cpp | 39 | 39 | 40 |
| game/multicontainer.cpp | 12 | 12 | 13 |

## New matches (Confidence raised to exact)
- 0x4e9ce0 script_write_objects (D14900): one header condition plus `else if` for "+No create"; msg[0x100].
- 0x4e58a0 script_editor_place_creature (D14901): no navpoint local, script_editor_hover.object at each use;
  signed length test.
- 0x4e0690 script_editor_create_ui (D14902): radius via the inline vector3::len of the size row, height copied
  after; parameter rows indexed by i alone; one callback / font store order.
- 0x4e2bb0 script_editor_pick_hover (D14903): size row copied as a vector3; `np = &navpoints[i]` in the loop body.

## What worked (reusable)
- Reading a global (selection / hover object) at each use instead of a local: the binary reloads after calls.
- Element pointer taken inside the loop body (binary walks a member address and derives the element).
- An inline member (vector3::len) for a sqrt of squares that the binary keeps on the x87 stack.
- Loops over i with derived values computed from i (8966 builds the induction variables itself).
- A small tool: w10-try/cdiff.mjs normalises both listings (stack offsets, labels) so `diff` shows only real
  differences; w10-try/try2.mjs is vc6-try with `--fn` printing that diff count per variant.

## Closer, still differs (D14904)
script_editor_handle_input (frame right; register rotation, one inline-budget call), add_navpoint,
edit_button_cb (same size), action_type_allowed (only three je targets differ), level_item_ok (62 -> 22 lines),
mp_levelselect_handle_click (frame right).

## Tried without result (D14905)
write_level_table, default_action_navpoint, cmdline_take_argument, mp_levelselect_drag_scrollbar,
ui_button::draw_arrow, select_prev, focus_selection, write_level_script (two `sub esp, 8` only), lights (4, one
fstp / lea each; flag sets tried), decals_draw_for_lobj (sorted fmul). Not attempted: ca_mesh renderers,
fill_action_lists, script_create, script_action_format, level_items_write_table, File::read, ui_button::process,
item_obj_spawn (/Od copy temporaries, D6107), mp_handle_item_drop.

## Behaviour errors / bug candidates
None found. All edits are behaviour-neutral (checked by reading: the dispatch conditions agree for bool levels,
the level-item name search restarts from 0 as before, the edit window tail runs only for n > 0 which every case
gives).

## Changed files (in w10-src)
- game/unnamed_script_editor_0x4de0b0.cpp
- game/new_levelselect.cpp
- include/DECISIONS.md (D14900-D14905)
No header change. mingw -fsyntax-only: 0 errors on both changed .cpp.

---

## Worker 11

### m-1020c worker 11 (D15000-D15010)

Strict sweep of the 12 units: 270 -> 279 match, 0 lost (`--check base/vc6-matches.txt`: "270 matched there,
279 now, 0 lost"). mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in the five changed files. No header
change (DECISIONS.md only). No behaviour change.

| File | Before | After | Of | New matches |
|---|---|---|---|---|
| game/unnamed_worldmap_automap_0x42f1e0.cpp | 31 | 33 | 35 | 0x430580 automap_labels_save, 0x42fd40 automap_labels_parse_table |
| vsdk/unnamed_gr_line_0x53e520.cpp | 13 | 17 | 19 | 0x53e520 gr_line_3d, 0x53e770 gr_poly, 0x53eae0 gr_bitmap_3d_projected, 0x53f060 gr_bitmap_3d_oriented_quad_facing |
| game/unnamed_fx_0x489c40.cpp | 48 | 49 | 57 | 0x48c630 fx_create_spell_area |
| game/unnamed_ai_combat_0x4bce60.cpp | 21 | 22 | 28 | 0x4bff60 ai_state::mode_combat_frame |
| vsdk/unnamed_poly_clip_0x5519c0.cpp | 6 | 7 | 8 | 0x551ca0 polygon_clip_to_plane |
| game/unnamed_phoenix_0x497ac0.cpp | 33 | 33 | 36 | |
| vsdk/unnamed_poly_split_0x55f110.cpp | 1 | 1 | 5 | |
| game/summoner.cpp | 13 | 13 | 15 | (WinMain not found, as before) |
| vsdk/unnamed_http_0x55a8e0.cpp | 14 | 14 | 15 | |
| vsdk/file_packfile.cpp | 19 | 19 | 20 | |
| game/new_status.cpp | 50 | 50 | 51 | |
| game/multitrade.cpp | 21 | 21 | 22 | |

## What worked (details in DECISIONS.md D15000-D15008)

- `while (1)` instead of `for (;;)`: 8966 rotates a `for (;;)` loop and duplicates its head, not a `while (1)`
  (automap_labels_save; D5904 noted it for one loop shape).
- One-exit regions (D8701 extended): when the binary saves a register after an early return, rewrite the later
  exits so the region has one exit: an if / else reaching one return (fx_create_spell_area), the failure path
  as `goto done` to the common tail (gr_line_3d), a block of its own with the condition repeated after it so
  8966 threads the second test (gr_poly). The same idea the other way: every `go_idle(); return;` as one
  `idle:` block placed where the binary has it, reached by goto (mode_combat_frame). Placement of `if (t) {
  ...; return; } goto idle;` matters (the `if (!t) goto idle;` form duplicates the block).
- Locals assigned after the early return instead of initialised at the declaration (gr_line_3d's clipped /
  ret; gr_bitmap_3d_projected's codes_and before the call).
- Block-scoped locals whose slots the binary reuses for temporaries (bw / bh in the bitmap functions).
- `x / (n * 1.0f)` for the binary's `fild / fdivp` where `x / n` gives `fidiv` (casts are folded back).
- Branch-local assignments instead of `a = b = c` before an if, so the values stay on the FPU stack.
- An array indexed by a counter (`out[count] = ..`) where the binary strength-reduces into the argument slot;
  `i++` written at the loop end to place the pointer step.
- Named local for a call result compared with 0 (`cmp eax, edi`, D11204) and split `||` early returns in EH
  functions.

Tooling: scratch `w11-try.mjs` (variants as vc6-try, plus an LCS diff of mnemonic + registers between
our listing and the binary, which shows register swaps and moved instructions that vc6-side's line-by-line
mnemonic compare hides). Worth adding to tools/ (vc6-side `--lcs`).

## Still differs (closer, kept: D15003, D15009)

- gr_bitmap_3d_oriented_quad 0x53edd0: one fstp of v[3].x scheduled before the integer stores.
- fx_missile_hit 0x48bac0: EBP / ESI saved in the prologue (binary: around the stop / hit / process region).
- shadow_create_blob_bitmap 0x489f10: y in EBP and the pixel pointer in the frame (binary: the reverse).
- combat_cast_frame 0x4bfaa0: zero register EBP vs EBX, 4-byte frame difference, merged give-up block placement.

## Tried without a match (D15010)

packfile_read_directory, choose_support_spell, worldmap_frame, worldmap_marker_clamp_to_edge (not retried),
summoner_cfg_load, status_handle_click (reverted), consider_life_drain, combat_approach, phoenix_anim_remap_in,
approach_attack_slot; register allocation / FPU scheduling throughout in polygon_clip_2d, the poly_split
functions, gr_draw_sphere, the phoenix AI, combat_toggle_ready_stance, fx_frame, fx_update_missile,
fx_find_spawn_point, fx_create_instance, fx_create_at_camera, mp_handle_it_okay, http_async_gethostbyname.

## Behaviour errors / bug candidates

None found. All changes keep behaviour (the gr_poly repeated global test reads a global no code between the
two tests writes; `bw * 1.0f` is exact).

## Changed files (in w11-src)

- game/unnamed_worldmap_automap_0x42f1e0.cpp
- vsdk/unnamed_gr_line_0x53e520.cpp
- game/unnamed_fx_0x489c40.cpp
- vsdk/unnamed_poly_clip_0x5519c0.cpp
- game/unnamed_ai_combat_0x4bce60.cpp
- include/DECISIONS.md (D15000-D15010 appended)

---

## Worker 12

### m-1020c worker 12 notes

Strict sweep of my 12 units: 351 -> 355 match of 392, 0 lost (`--check` against w12-before.txt).
Lists: w12-before.txt, w12-after.txt.

| File | Before | After | Of | New matches |
|---|---|---|---|---|
| game/unnamed_ai_move_0x4b1410.cpp | 10 | 12 | 19 | 0x4b2510 steer_jitter, 0x4b2890 wall_avoid_steer |
| vsdk/bmpman.cpp | 49 | 50 | 56 | 0x502f70 bm_prefer_vbm |
| vsdk/effect_mem.cpp | 33 | 34 | 39 | 0x51c8b0 effect_base_read |
| vsdk/unnamed_snd_0x5204c0.cpp | 54 | 54 | 58 | |
| game/unnamed_tentacle_beast_0x4c6480.cpp | 21 | 21 | 25 | (select_action closer) |
| vsdk/character_instance.cpp | 50 | 50 | 53 | |
| vsdk/unnamed_linefile_0x558340.cpp | 12 | 12 | 14 | |
| game/unnamed_cutscene_camera_0x493e50.cpp | 10 | 10 | 12 | |
| vsdk/unnamed_eax_0x521e40.cpp | 13 | 13 | 14 | |
| vsdk/matrix.cpp | 45 | 45 | 46 | |
| game/new_spell.cpp | 37 | 37 | 38 | |
| game/new_assess.cpp | 17 | 17 | 18 | |

New matches are `Confidence: exact` with the shape in the comment.

## What worked (D15100-D15102)

- **Centre through vector3::operator+ (D15100).** `owner->pos + owner->centre_offset + ...` instead of
  `get_center()`: the inline's local c makes 8966 order the y / z adds the other way. steer_jitter matched;
  path_closest_point and turn_towards closer.
- **No pointer local for a member (D15101).** wall_avoid_steer's `Character *o = owner` kept EDI live from the
  entry; reading `owner->` each time lets 8966 shrink-wrap EDI around the probe as the binary. Plus: centre and side
  vector through operators, if / else of two immediate stores, one combined test, one block to the end (temporary
  slot reuse), single exit. Same pointer-local removal helped tentacle_beast_ai_select_action (`t`).
- **Two exits inside the region push at the entry (D15102, inverse of D8701).** effect_base_read and
  bm_prefer_vbm: a `return` from inside the File block makes 8966 push ESI / EDI at the entry as the binary.
- Tool: w12-try/vt.mjs (variant runner reporting byte-difference count `nd` and first difference, with a local copy
  of vc6-compare that counts differing bytes). nd was a much better guide than first-difference offset.

## Closer, kept (behaviour identical)

- path_closest_point (nd 304 -> 98; frame 0x58 vs 0x60 left), turn_towards (978 -> 374),
  mode_traveling_frame (842 -> 301: cases reordered with TURN last, PATH / TURN through a `done` flag, follow speed
  as `float v = s < scale ? s : scale`), tentacle_beast_ai_select_action (523 -> 386: no `t` local, `goto next`,
  squeeze points as copies with y cleared).

## Still differs (tried, D15103)

snd_set_volume family, eax_init, character_use_skill_on, LineFile::detect_eol / read_line, rfx_preload,
read_track (SIB order), camera_blend_sequence::advance (slot order of the six angles), tentacle_beast_boss_load,
tentacle_beast_ai_frame, tentacle_beast_frame_hook (binary calls vector3 ctors out of line), parse_arr_line,
bm_set_reduced (closer form found, not applied), bm_lock_vbm, bm_lock_pcx, bm_load_internal, sum_root_velocity.
Not attempted in depth: the large ai_move functions (avoid_characters, follow_path, travel_steer_update,
ai_find_nearest_wall_hit), build_bone_matrices, increment_tick, vfx_instance_create, vfx_load,
effects_load_bases, bm_read_header, bm_lock_tga, assess_format_info, matrix43::transpose_in_place.
The coordinator's gr_get_string_size tip did not apply to my remaining functions.

## Behaviour errors / bug candidates

None found. All changes are same-value reshapings.

## Changed files (my copy)

- game/unnamed_ai_move_0x4b1410.cpp, game/unnamed_tentacle_beast_0x4c6480.cpp, vsdk/effect_mem.cpp, vsdk/bmpman.cpp
- include/DECISIONS.md (D15100-D15103 appended). No header changed.
- mingw `-fsyntax-only -std=c++98`: 0 errors in each changed .cpp.

---

## Worker 13

### m-1020c worker 13 notes

Strict sweep (VC6_STRICT=1) of my 12 units in my copy: 344 -> 353 matches, 0 lost
(`vc6-sweep.mjs --check src/decomp/vc6-matches.txt`: 344 matched there, 353 now, 0 lost).
Lists: w13-sweep-before.txt, w13-sweep-after.txt (scratchpad m-1020c folder).

## Counts per file (strict)

| File | Before | After / total | New matches |
|---|---|---|---|
| vsdk/unnamed_vector_0x523f30.cpp | 43 | 47 / 51 | 0x5243d0 vector3::unrotated, 0x5240e0 vector3::length_quick, 0x5253a0 vector2_dist_sq_to_line, 0x525410 vector2_closest_point_on_segment |
| game/new_hud.cpp | 52 | 55 / 60 | 0x40cc10 hud_load_bitmaps, 0x40d160 hud_update_portraits, 0x4102a0 mp_draw_loading_status |
| game/unnamed_interface_0x411270.cpp | 63 | 64 / 69 | 0x413970 ui_choice_popup_hit_test |
| vsdk/memory.cpp | 18 | 19 / 19 | 0x512640 mem_report_line |
| vsdk/unnamed_sndstream_0x566560.cpp | 22 | 22 / 26 | |
| game/unnamed_mp_objects_0x47f860.cpp | 27 | 27 / 31 | |
| vsdk/parse.cpp | 48 | 48 / 51 | |
| vsdk/unnamed_gr_poly_0x5450c0.cpp | 22 | 22 / 24 | |
| game/unnamed_editor_dialogs_0x430bc0.cpp | 3 | 3 / 5 | |
| vsdk/unnamed_crc32_0x5520d0.cpp | 3 | 3 / 4 | |
| game/new_skill.cpp | 38 | 38 / 39 | (closer) |
| game/new_dialogue.cpp | 5 | 5 / 6 | |

## What worked (DECISIONS.md D15200-D15205, appended in my copy's include/DECISIONS.md)

- Reusing the address-taken w / h locals for other values (mp_draw_loading_status's panel size,
  ui_choice_popup_hit_test's row height / width): the binary keeps those values in the w / h slots. A pattern
  worth checking in every function with `int w, h` filled by gr_get_string_size.
- Pointer locals for array elements in loops (hud_load_bitmaps: b and d; hud_update_portraits: h and b).
- Parenthesised sub-expressions: `x * r + (y * u + z * f)` (unrotated), `(t + (t * 0.5f))` stops 8966
  folding t + t*0.5f into t*1.5f (length_quick).
- Block-scoped swap temporaries move the frame slots (length_quick).
- A dot product accumulated in a variable holding the first product (`d = ap.x * ab.x; ... (d + ap.y * ab.y)`)
  reproduces the binary's x87 leftover `fadd st, st(1)` + `fstp st(0)` (dist_sq_to_line, closest_point).
- Two clamp calls as two statements (closest_point).
- The bound written in the loop test over a length local (mem_report_line: D13204 had it as impossible).
- Dropping a variable can be what gives a dead-slot store: 8966 put the appraise sqrt in target's slot by itself.
- Switch case order: 8966 emits jump-table case bodies in source order and the compare-chain cases last;
  reordering hud_handle_keys' cases to the binary's body order moved the first difference from 252 to 441.

## Still differing (tried, see D15206)

hud_handle_keys (shared tail kept in the wrong case), hud_interact_nearest (binary pushes EDI / EBP only around
the second half), hud_hot_buttons_drag, hud_portrait_get_color, hud_drag_icon_begin, dialogue_frame (tail),
character_use_skill (pick-lock fcompp), interface_init_global (which float is on the x87 stack), ui_3d_begin,
ui_choice_popup_show / _draw, ui_message_box_show (flags byte reread; a volatile read gets most of it, not kept),
mp_handle_ai_disable, mp_handle_status_add, mp_send_new_lep, mp_handle_new_path, wav_open, snd_stream_fill_*,
ParseInt / ParseUInt / ParseString, gr_poly_clip_edge / clip_and_draw, Crc32::update, editor_dialog_show /
handle_event, the three random-direction functions, vector3_angle_and_axis. Not attempted beyond a look:
editor_dialog_show, mp_handle_new_path, mp_send_new_lep (D13204).

## Behaviour errors / bug candidates

None found. All changes keep behaviour (reassociated float sums now follow the binary's order, so they are
closer to its rounding, not further).

## Changed files (in w13-src)

- vsdk/unnamed_vector_0x523f30.cpp
- game/new_hud.cpp
- game/unnamed_interface_0x411270.cpp
- vsdk/memory.cpp
- game/new_skill.cpp
- include/DECISIONS.md (D15200-D15206 appended)

No header changes. mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in each changed file.
Helper scripts (scratchpad, not for the repo): w13-try.mjs, w13-side.mjs, w13-side-long.mjs (vc6-side with a
1200-instruction listing; the tool's 400 cuts large functions), w13-survey.mjs, w13-opdiff.mjs, w13-apply.mjs.

---

## Worker 14

### m-1020c worker 14 notes

Tree: `m-1020c/w14-src`. VC6 SP5 (cl 12.00.8966), strict (`VC6_STRICT=1`). Before: `w14-before.txt` (my 12
units, 251 match / 41 differ, equal to the repo's vc6-matches.txt). After: `w14-sweep.txt` (my units, 254 / 38)
and `w14-sweep-full.txt` (whole copy: 5320 match, `--check src/decomp/vc6-matches.txt`: 5317 -> 5320, 0 lost;
the one `target` and one `not_found` are the repo's own). **My 12 units: 251 -> 254 (+3), none lost.**
Decisions D15300-D15306 appended to `w14-src/include/DECISIONS.md`.

## Per file (strict matches before -> after / compared)

| file | before | after | won |
|---|---|---|---|
| vsdk/unnamed_vfx_emitter_0x55ff00.cpp | 6 | 6 / 14 | |
| vsdk/gr_direct3d.cpp | 43 | 44 / 50 | 0x538760 gr_d3d_draw_primitive (header change, D15304) |
| vsdk/unnamed_ix_0x53ac20.cpp | 29 | 29 / 35 | |
| game/unnamed_dialogue_box_0x4d6660.cpp | 27 | 27 / 32 | |
| game/unnamed_lobj_raycast_0x4ae890.cpp | 3 | 5 / 7 | 0x4ae890 lobj_raycast (D15300), 0x4aea50 lobj_ray_test_faces (D15302) |
| vsdk/targautils.cpp | 18 | 18 / 21 | (pcx_read_bitmap closer) |
| vsdk/unnamed_gadget_0x52bfe0.cpp | 46 | 46 / 48 | |
| game/unnamed_effectstream_0x440400.cpp | 12 | 12 / 14 | |
| vsdk/unnamed_bezier_0x558a30.cpp | 14 | 14 / 15 | (spline::eval closer) |
| vsdk/stringpool.cpp | 15 | 15 / 16 | |
| game/new_pickup.cpp | 19 | 19 / 20 | |
| game/new_gamecreate.cpp | 19 | 19 / 20 | |

The three winners are `Confidence: exact`, with the shape in the comment.

## What worked

- **Named locals for vector temporaries (D15300).** lobj_raycast: one `vector3 d` reused for both rotated
  inputs (the second as an assignment: temporary + copy), and the second rotated result in a named local
  (`vector3 r = d.rotated(...)`): the binary reads it from its frame slot, not through EAX. Frame 0x30 exact.
- **Component stores instead of the 3-float constructor (D15302).** `vector3 c(x, y, z)` evaluates z first and
  stores at the end; the binary computes and stores x, y, z in turn, so write `c.x = ...; c.y = ...; c.z = ...;`.
  Gave lobj_ray_test_faces. Likely useful in other units with centroids / constructed vectors.
- **A void function that keeps EAX returns something (D15304).** gr_d3d_draw_primitive returns the draw call's
  HRESULT: the binary updates the counter in ECX to spare EAX, and the other branch is `call; ret`. Plus the
  lock flags as `reset; if (pos == 0) ... else ...` (an else-if chain lets 8966 preload one flag).
- **Parenthesised negative-constant products (D15303):** `(-1.5f * u3) + ...` keeps 8966 from folding `-c * x`
  into a subtraction and CSE-ing `c * x` across expressions (spline::eval now the same size, only two pops
  scheduled differently).
- **Init order for the zero register (D15301):** `count = 0` after `pos = 0` lets every pre-loop read push EBP.
- Tools (scratch, `m-1020c/w14-tools`): `try.mjs` (multi-replacement variants, prints differing-line counts and
  the first differing lines; restores the file), `cmp.mjs`, `survey.mjs`, `apply.mjs`. Note: give vc6-side.mjs a
  qualified name (`draw@dialogue_line`), else a substring like `draw` or `eval` picks another method's listing.

## Closer, still differing (kept, behaviour identical)

- pcx_read_bitmap 0x555f60: first difference 0xa6 -> 0x1a6; only where pos is cleared (D15301).
- lobj_sphere_test_faces 0x4aec20: 111 -> 91 differing lines; the dead second `t > 1` compare and frame left.
- spline::eval 0x558f40: 49 -> 12 differing lines, size now equal; the pop placement left.

## Still differs (tried, D15305)

gamecreate_handle_click, pickup_draw_item_list, gr_d3d_set_window, ui_gadget::set_parent / send_event,
fx_recb0::get_fade, m2v_skip_extension_and_user_data, dialogue_line::draw, ix_point_in_polygon,
tga_rle_encode_row, effect_stream_start. Not attempted in depth: the large vfx_emitter functions (move_particles,
render_d3d, get_transform, apply_forces), gr_d3d_set_state / bitmap / tmapper_multipass / init_device,
d3d_compute_pixel_format_shifts, the ix ray / box functions, lobj_raycast_bsp, dialogue topic / box draws
(register choice per earlier notes), StringPool::Add, effect_stream_finish_entry, fx_recb0::render (register
choice only: EBX / EBP swapped, D13606 tried it).

Finding (D15306, not applied): seg2d_intersect's frame comes from its two by-value vector2 parameters; with four
float parameters 8966 reuses the argument slots as the binary does (frame 0). The signature was not changed.

## Behaviour errors / bug candidates

None found. Every change keeps behaviour: `dir + start` = `start + dir` (float addition commutes exactly),
component stores compute the same values, `a + (-c * x)` = `a - c * x` (negation exact), the lock flag reset
form takes the same branch for every position, and the new HRESULT return is unused by every caller.

## Header change

- include/vsdk/gr/directx/gr_direct3d.h: `HRESULT gr_d3d_draw_primitive(...)` (was void), D15304. Only
  gr_direct3d.cpp calls it (six statements). mingw `-fsyntax-only -std=c++98 -w -Iinclude` over every .cpp:
  only unnamed_decals_0x4bbe80.cpp fails, with its 9 errors as in the repo. decomp-sigcheck: only the known
  charinfo_load_move_anim. Full strict sweep: 0 lost.

## Files changed (in w14-src)

- game/unnamed_lobj_raycast_0x4ae890.cpp
- vsdk/gr_direct3d.cpp
- include/vsdk/gr/directx/gr_direct3d.h
- vsdk/targautils.cpp
- vsdk/unnamed_bezier_0x558a30.cpp
- include/DECISIONS.md (D15300-D15306)

---

## Worker 15

### m-1020c worker 15: notes

Strict sweeps (VC6_STRICT=1) of the 11 units before and after: 276 -> 280 matches, none lost
(w15-sweep-before.txt, w15-sweep-after.txt; `--check src/decomp/vc6-matches.txt` on the five changed units: 0 lost).

| File | Before | After | Total |
|---|---|---|---|
| game/unnamed_render_scene_0x481680.cpp | 61 | 62 | 77 |
| vsdk/window.cpp | 35 | 36 | 40 |
| game/unnamed_pfgeom_0x4a4ee0.cpp | 19 | 20 | 24 |
| game/unnamed_goals_doors_0x475180.cpp | 63 | 63 | 67 |
| vsdk/unnamed_d3d_tcache_0x54bb30.cpp | 11 | 11 | 14 |
| vsdk/unnamed_clip_0x5590b0.cpp | 15 | 16 | 17 |
| game/unnamed_luminar_0x4c8d10.cpp | 21 | 21 | 23 |
| vsdk/unnamed_anim_track_0x550660.cpp | 12 | 12 | 13 |
| vsdk/ui_container.cpp | 9 | 9 | 10 |
| game/new_journal.cpp | 26 | 26 | 27 |
| game/new_int_multi_update.cpp | 4 | 4 | 5 |

New matches (Confidence raised to exact):
- 0x52aff0 ui_window::send_event: no return after `if (callback) return callback(...)` (C4715; EAX = the NULL
  pointer, AL = 0). D15400.
- 0x4a53d0 pf_first_edge_crossing: `dy = seg->a->x - pt.x` (copy of the x line) and `dx*dx + dy*dy`. D15401.
- 0x559890 bbox_silhouette_corners: table row indexed in place, `&out[n]`. D15402.
- 0x483f90 levelrender_effect_apply_vis: `int nt = base->num_tags;` (a separate local) puts the tag offset in
  the dead fx slot. D15405.

Closer, kept (no match change): version_check_run (switch on the download status, 75 -> 43 differing
instructions, D15403), levelrender_add_effect_entry (fvec.dot(d), only `pop esi` placement left, D15404),
level_vis_parse (unsigned index loop gives the biased pointer, D15406), ui_window::handle_arrow_key (one sw / sh
pair for both branches gives the frame slots, D15407).

## Techniques that worked
- A missing return after a conditional `return f()` reproduces "returns the tested pointer when NULL".
- Duplicated subexpressions in their own locals stop 8966 folding `a*a + a*a` and give the binary's memory spill.
- Splitting a reused variable (n for several loops) into a new later local changes which values get dead
  argument slots (8966 slot assignment), see D15405; sharing one variable between two branches can do the
  reverse (D15407).
- A compare chain plus inline default is a small `switch` (D15403).
- A byte-level side-by-side tool (w15-tools/bside.mjs: compares the .cod bytes with Sum.exe per instruction,
  relocations masked) shows operand and register differences that vc6-side.mjs (mnemonics only) hides; note
  that its `[sym+4]` displacement lines are false positives.

## Still differs (tried, not reproduced)
- seg2d_point_near_middle 0x4a6530: load order of s->a / s->b and one product's operand order; every term order of
  both dot products compiles the same (canonicalised); vector2 / float / pointer-local forms tried.
- level_vis_reset 0x487030: pointer form gives no bias; the index form gives the bias but CSEs the constant 1
  into a register (7 forms).
- ui_window::init 0x52b100: this in ESI and both registers saved up front in the binary; if/else, declaration
  placement and a `self` copy tried.
- gr_classify_sphere_view 0x5594f0: nested positive tests give the shared -1 tail but nz is kept in memory (5
  forms).
- matrix3_pack_orientation 0x475380: `a` in the dead out / m argument slots; 9 more declaration / block forms.
- matrix3_unpack_orientation 0x475430: the binary copies the four inputs as integers into memory slots (in's
  slot reused); vector2, array and in-place-square forms tried.
- anim_track::calc_rot_tangents 0x5506e0: 5 instructions, the schedule of one movsx before the tan_b.x store; term
  and factor orders are canonicalised.
- ui_container::draw 0x553800: only frame slots (sy after text_x / text_y in the binary); scope, declaration
  order, names, cached subexpressions tried: the slots do not move.
- levelrender_add_items: the binary keeps d.z / d.y on the FPU stack for the distance test (as in prior notes).
- levelrender_push_attachment: the binary expands matrix::operator= with two inline row copies (each calling the
  vector3 copy constructor for the discarded result) and calls vector3::operator= for the third, as if the inline
  budget ran out; `#pragma inline_depth(0..4)` does not reproduce it.
- ai_state::luminar_boss_load: the second length's order (x from memory, z from the stack); 25 len / sqrt forms.
- Not attempted (large, many prior attempts): render_scene's draw_list / build_draw_lists / render_scene /
  fade_occluders / add_effects / draw_speech_text / overhead_label_place / draw_name_labels / add_lobj /
  add_containers, pfgeom's segment_hits_walls / segment_intersect / grid_cells_along_segment, goals_doors'
  ai_goal_evaluate / door_find_floor_point, d3d_tcache's three, window's parse_radio_button / parse_slider,
  luminar_frame_hook, journal_handle_click.

## Behaviour errors / bug candidates
None found or changed. pf_first_edge_crossing's distance (x twice) is the known game bug; the source now writes it
as a copied line (`dy = seg->a->x - pt.x`), same behaviour.

## Changed files (in w15-src)
- vsdk/window.cpp (send_event, handle_arrow_key)
- game/unnamed_pfgeom_0x4a4ee0.cpp (pf_first_edge_crossing)
- vsdk/unnamed_clip_0x5590b0.cpp (bbox_silhouette_corners)
- game/unnamed_render_scene_0x481680.cpp (effect_apply_vis, add_effect_entry, level_vis_parse)
- game/new_int_multi_update.cpp (version_check_run)
- include/DECISIONS.md (D15400-D15407 appended)

No header changed. mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in each changed file.
