# Byte-match round 2026-10-21, fifth: the thirty workers' notes

The 255 functions at distance 13-300 on the corrected --score, in thirty batches of whole files (at most twenty
workers ran at once: the app's subagent limit). Result: 53 new matches (strict 5,549 -> 5,602, none lost), about
60 closer. Not merged: worker 29's game/gameplay.cpp (state_game_frame, an unused pointer local for a non-match).
Kept: worker 18's three unused vector3 locals in camera_blend (they make VC6 emit the binary's out-of-line calls,
which decide the x87 rounding of D9807). The closer changes were read for behaviour by four review agents before
the commit. The decisions are D22200-D25199 in include/DECISIONS.md.

## Worker 1

### Worker W1, round m-1021e (decisions D22200 to D22299)

Unit: game/unnamed_script_editor_0x4de0b0.cpp (13 functions). Unit result: 107 -> 109 match.

## New matches

| file | address | function | D | what did it |
|---|---|---|---|---|
| game/unnamed_script_editor_0x4de0b0.cpp | 0x4ecbe0 | script_editor_level_item_ok | D22201 | name search as `do { found = false; for (i = 0; i < level_item_count; i++) if (strcmp(...) == 0) { sprintf(...); found = true; break; } } while (found);`, and the new record's `li->type` stored before `li->count` |
| game/unnamed_script_editor_0x4de0b0.cpp | 0x4e84b0 | script_editor_apply_action_params | D22203 | the type list's data / text read into function-scope `int d` / `const char *t` locals before the action address is formed (kinds 2, 3, 4, 5, 8), and kind 6's object stored before its kind in the source |

**D22201 (game/unnamed_script_editor_0x4de0b0.cpp). script_editor_level_item_ok 0x4ecbe0 is exact.** Two changes.
(1) The name search as a do / while over a `bool found` with an ordinary `for (i = 0; i < level_item_count;
i++)` that breaks on a clash. 8966 threads the `found = true` edge straight to the count load (the binary's
`jmp` back to `mov eax, [level_item_count]`) and the `found = false` exit out of the loop, and it puts the
table pointer after the entry test (`test / jle / mov edi, level_items`) as the binary does. The `goto again`
form (D20905) set the pointer before the entry test. Tried without effect before it: decl orders, `again: i
= 0;`, an unsigned index (143 stars), a while loop, a name pointer local; a do / while without the break
(worse), the plain `for` with `goto again` (worse), `for (;;)` with an inner search and a break (worse).
(2) `li->type = &item_types[e->data];` before `li->count = count;`: 8966 then loads the count after the
first `rep movsd` of the strcpy, as the binary. (A count pointer, an entry copy, a data local, a type local,
a name pointer, a record pointer, memcpy did nothing.) Same behaviour: same passes and restarts; two
independent stores swapped.

**D22203 (game/unnamed_script_editor_0x4de0b0.cpp). script_editor_apply_action_params 0x4e84b0 is exact**
(174 -> 0). The binary calls the type list's get_selected before it scales the index or pushes the other
arguments; with the call inside the store or argument list 8966 scales the index (or pushes `1`) first.
Steps: kind 2 `d = ...->data; actions[index].arg0c = d;` (174 -> 151); kind 3 `d = ...; m =
charinfo_find_move(info, 8, d, true);` (-> 128, it now pushes and jumps into kind 9's find_move call as the
binary); kind 4 `t = ...->text; script_editor_store_string(a, t);` (-> 100); kind 5 the same (-> 88); kind 8
`d = ...; character_equip_item(..., item_create_weapon(d));` (-> 28); kind 6 `sel.object = ...; sel.kind =
SE_PICK_CHARACTER;` in that source order (8966 stores the kind first, after the call, as the binary; -> 0).
d / t as block locals or at function scope (first / last) give the same code; function scope kept. Not
useful: an unsigned index, an index pointer, `break` for `return`, an `an = m->anim` local, a block copy of
`a->text`, a weapon local or a character local in kind 8 (36), kind 6 through `d` (162). Same behaviour:
the same calls in the same order, the same stores.

## Closer, kept

- game/unnamed_script_editor_0x4de0b0.cpp 0x4e30d0 script_editor_draw_navpoints: 83 -> 38, **D22202**.
- game/unnamed_script_editor_0x4de0b0.cpp 0x4e3a20 script_editor_handle_input: 292 -> 292, **D22204**
  (distance unchanged, but the Change / navpoint block now matches; see below).

**D22202 (game/unnamed_script_editor_0x4de0b0.cpp; closer only). script_editor_draw_navpoints 0x4e30d0:** (1)
the type switch's case blocks in the binary's layout order ambient / camera / level (83 -> 65; 8966 lays
the blocks out in source order; level-ambient-camera and ambient-level-camera give the same, camera-first
orders 83); (2) the level case's second (gr_line_mode) marker call written in both branches of the
start-navpoint test, and the same in the cutscene loop (65 -> 43 -> 38): 8966 then tail-merges the
set_color + marker calls across branches and the camera case (`jmp` into the level case's call, one `add
esp, 0x48` after it) as the binary does. Left (38): the cutscene loop. The binary's induction pointer is
&np->ground_height (ESI) with `lea edi, [esi - 0x10]` for &np->pos before each call and `mov ebx, 1` loaded
inside the loop for the kind / mode compares; ours steps &np->pos in ESI and keeps the 1 in EDI. Tried (all
38): an index loop with `np = &cutscene_navpoints[i]`, a fully indexed body, a separate `cp` pointer
(function scope or block), a `vector3 *pos` local used for the marker calls and the label, `np` set first
in the for, the increments swapped (40), a ground-height float local (128), an end-pointer loop (125).

**D22204 (game/unnamed_script_editor_0x4de0b0.cpp; closer only, distance unchanged).
script_editor_handle_input 0x4e3a20:** in the Change-latched navpoint branch `np = hover.object;` before
`index = cur_action->data;` (the binary loads the hover object into ECX first; the cutscene test, the
switch on a->type in EAX and the `a->nav = np` store then match). Distance stays 292 (dominated by the
later blocks); the first difference moves from line 114 to line 549 of the side-by-side: the Y drag's
plane, where the binary calls vector3::operator= (0x413190) for set_normal_point's `normal = *n` and ours
expands it inline (the inline-budget difference already noted, D14904). Same behaviour (two loads swapped).

## Still differs (tried; do not repeat)

**D22200 (notes only). script_write_level_script 0x4e9310 (15).** Only the two door-sound sprintf argument
blocks: the binary does `sub esp, 8` for the second double before the `fstp` of the first (params[2]);
8966 stores first. Tried this round (all identical code): a `sound_ref *` local (much worse), a `const ubyte
*` params pointer, `(double)` casts, an int local and a `float t[1]` / float local for params[2], `(float)`
on the product's byte, the constant first, `static const float k`, `/ 255.0f` (VC6 emits the same fmul),
`(float)(int)` casts, a double constant. With D14905's seven casts / locals: scheduling; stop.

**D22205 (notes only). script_editor_default_action_navpoint 0x4e8cf0 (26).** Tried this round (no
better): the latched test inverted with the plain return first (worse, 38 stars), the same with `return
np` for the empty case, an else-chain with the navpoint return in the else (both with return / break in
the loop), break instead of the in-loop return, an `np` double pointer for the type read, the last path
through `found`. With D20906 / D12501: register choice (found in EAX in the binary); stop.

**D22206 (notes only). script_editor_door_sound_inputbox_cb 0x4ebb20 (46).** Tried this round (all 46
unless noted): `list = NULL` at declaration (also declared first), `list = (ui_listbox *)value`, the list
stores through `ui_listbox **lp`, a bool local for the opening radio, the radio test inverted with the
early return first (55), `break` for the no-radio return (51), the list as `value` reused through a cast.
The binary keeps the list in ESI from its definition (ESI pushed at entry) and in value's slot; 8966 never
splits it that way. Stop.

**D22207 (notes only). script_create 0x4e88a0 (179).** The binary keeps n in EBX (cutscene's register,
dead by then) and pushes ESI / EDI only after the `n == 54` test; ours holds n in EDI (spilled to a frame
slot, frame 0x54) so all four are pushed at entry. Tried (all 179 / 178): n, i, type declared first or
last, `register int n`, the insert / default tails as one if / else with one or two fill_action_list
calls. Register choice; stop.

**D22208 (notes only). level_items_write_table 0x4ea500 (256).** Ours keeps `temp` in EBP across the
function (an extra push); the binary uses EBX / ESI / EDI only and reloads temp from its argument slot.
Tried (all 256): the copy loop as do / while (identical code), `const char **tp = &temp` for the open or as
a declaration, `*&temp`, the close / backup chain as one condition, a separate err variable. Not solved.

**D22209 (notes only). script_editor_fill_action_lists 0x4e62e0 (210).** The binary pushes EBP / ESI /
EDI after the change-button test; 8966 at entry. Tried: the rest as the else of the latched test (210),
the whole body under `if (!latched)` (222). The cases keep their own set_focus + return in the binary, so
a single-exit form would change the layout. Stop.

Not attempted this round: script_editor_action_type_allowed (D20908, all code identical, three branch
targets), script_editor_edit_button_cb (D20904, entry pushes), script_write_level_table (D14905, the
aligned EBP frame).

## Behaviour errors / bug candidates

None new.

## Header changes

None.

## Changed files

- src/game/unnamed_script_editor_0x4de0b0.cpp: script_editor_level_item_ok (D22201, confidence exact),
  script_editor_apply_action_params (D22203, confidence exact), script_editor_draw_navpoints (D22202,
  comment), script_editor_handle_input (D22204, inline comment).

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root W/src --only "unnamed_script_editor_0x4de0b0" --check
src/decomp/vc6-matches.txt`: 1 unit, 109 match, 0 wrong target, 11 differ, 0 errors; 107 matched there,
109 now, **0 lost**.

## Worker 2

### Worker W2 notes (round m-1021e, decisions D22300-D22399)

Batch: thirteen functions of game/level_scripts.cpp (the savegame conversation / two-stage functions,
conversation_execute_action, level_format_display_name). Distances are vc6-compare --score after the 2026-10-21 fix.

## New matches

None.

## Closer, kept

| file | address | name | old -> new | D |
|---|---|---|---|---|
| game/level_scripts.cpp | 0x4d0e20 | savegame::two_stage_set | 297 -> 178 | D22300 |
| game/level_scripts.cpp | 0x4d34e0 | savegame::read_two_stage | 83 -> 28 | D22301 |
| game/level_scripts.cpp | 0x4d09e0 | savegame::set_topic_bit | 134 -> 67 | D22302 |
| game/level_scripts.cpp | 0x4d0870 | savegame::get_topic_bit | 76 -> 73 | D22303 |
| game/level_scripts.cpp | 0x4d02d0 | savegame::init_conversation_slot | 140 -> 112 | D22304 |
| game/level_scripts.cpp | 0x4d06e0 | savegame::set_speaker_bit | 73 -> 54 | D22305 |

**D22300 (game/level_scripts.cpp; closer). savegame::two_stage_set 0x4d0e20: 297 -> 178.** The prologue as the
binary computes it: `int coff = cur_level_index * 4; int level = cur_level_index; int hoff = SAVEGAME_NUM_LEVELS * 4
+ level * SAVEGAME_MAX_TWO_STAGE * 4; int base = ...; int count = *(int *)(two_stage.data + coff);` (coff from the
global, not from level, so level is not kept), the search loop indexing `((unsigned int *)(two_stage.data +
hoff))[i]` (8966 strength-reduces it into the binary's walking pointer and re-reads data afterwards, which also
gives the binary's `mov edx, [ecx+8]; test dh, 1` flags test), `char name[0x41]` (the binary's 0x50 frame: one
spare slot above name), the open / closed printfs as if / else (the binary's two pushes and one call), new
entries stored through `((unsigned int *)(two_stage.data + hoff))[count]` and `count++; *(int *)(two_stage.data +
coff) = count;`, and in the container path for an existing entry `int byte = bit / 8; int cshift = bit % 8;` as
block locals and `(open ? true : locked) << cshift`. Left: the binary keeps hash in EBX and i in the frame (cached
in EAX through the loop); 8966 gives i EBX and spills hash in every form tried; the binary also reloads locked from
its argument slot after the "stuffing flag" printf and keeps the shift in obj's dead argument slot, where 8966
folds locked = false into a setne.
Tried (all 178-297, do not repeat): a walking `hp` pointer in for / while / do-while / if + do-while / goto forms,
`*hp++`, an inline find helper (for and do-while bodies, merged into i by copy propagation), a separate loop
variable copied into i or into an address-taken `found`, `register` i / hash / hp, hash or i declared first or
last, `int hash`, hoff as a one-int array or via a const pointer, coff / hoff as members of a local struct (memory,
but level comes back), hoff from coff, `hp = hoff + data`, flags tests as `!= 0`, a local, `>> 8 & 1`, a bool,
shift as ubyte / char, the locked forms `open ? 1 : locked`, `open ? open : locked`, `open || locked`, a v local,
if / else into v, `bool *lp = &locked` stores before and after the printf.

**D22301 (game/level_scripts.cpp; closer). savegame::read_two_stage 0x4d34e0: 83 -> 28.** The hash row read at a
running byte offset of the data, re-read each pass: `int hoff = SAVEGAME_NUM_LEVELS * 4 + level *
SAVEGAME_MAX_TWO_STAGE * 4;` and in the loop `hash = *(unsigned int *)(two_stage.data + hoff); hoff += 4;` (was a
`hashes` row pointer read once). Everything but the loop control now matches: the binary runs one down-counter
(count - 1, `dec / jns`) where 8966 keeps i and count. Tried: `TWO_STAGE->hashes[level][i]` (89), the index forms
`((unsigned int *)(data + hoff))[i]`, `+ i * 4` in five orders, `<< 2`, `[hoff / 4 + i]` (39-80), `hoff += 4`
before the read with `- 4` / `[-1]` (28), explicit down-counters `for (i = count - 1; i >= 0; i--)`, `for (i =
count; --i >= 0;)`, `while (--count >= 0)`, `while (count-- > 0)`, `while (count--)`, `i > 0` / `i-- > 0` (56-71:
the binary's loop shape, but `this` moves to EBX and hash to EDI), `++i`, `count > i`, `i != count`, unsigned i, a
block-scope i, i declared last, a const count, and i counting up beside `--count >= 0` (71-87). With `char
name[0x41]` / `[0x44]` the down-counters get the binary's 0x60 frame back (53; `[0x48]` 59), also with `hoff += 4`
first (51), but `this` stays in EBX and hash in EDI, and `dec / js` replaces the binary's `test / jl`: register
choice, stopped.

**D22302 (game/level_scripts.cpp; closer). savegame::set_topic_bit 0x4d09e0: 134 -> 67.** The speaker search walks
one pointer as the binary: `int *counts = (int *)(data + counts_off);` and in the loop `int *cp = &counts[i];
unsigned int h = *(unsigned int *)((ubyte *)cp + (4 - counts_off)); count = *cp;` (the hash reached at the fixed
difference 4 - counts_off from its count, the binary's `mov eax, 4; sub eax, ebx; mov ebx, [eax+ecx]`), the
speaker count through CONVERSATION_SAVE_INFO in the loop condition and the re-test (D16806), and the bit written as
set_speaker_bit's D20801 (`int mask = 1 << (i & 7); int byte = (i >> 3) + bits_off;` before the value test). The
bit code and the propagate loop now match. Left: the binary keeps first in EBP and spills data and the hash; 8966
keeps data in EBP and first in the frame. Tried: the hash read first, operands swapped, index forms, `i * 4 +
counts_off`, the difference as `data + counts_off + i * 4 + (4 - counts_off)` (134), a `ubyte *cp` (127), an `hd`
local (78-140), `&counts[i]` without a cp local (135), count before the hash read (135), the count / first zeroing
in four orders and as `count = first = 0`, the bit through `ubyte *p = data + (i >> 3)` (89) or `(data +
byte)[bits_off]` (118).

**D22303 (game/level_scripts.cpp; closer). savegame::get_topic_bit 0x4d0870: 76 -> 73.** D22302's loop body
(cp and the fixed difference) and `count = 0;` before `first = 0;`. Left as in set_topic_bit: data / first / hash
registers. Tried: data declared last, first declared first, `register` first, the final test as `((1 << ...) &
(data + (i >> 3))[bits_off])`, data and / or the speaker hash declared at their first assignment (D19302's dead
argument slot device; all 73).

**D22304 (game/level_scripts.cpp; closer). savegame::init_conversation_slot 0x4d02d0: 140 -> 112.** The save info
read through CONVERSATION_SAVE_INFO at each use (no `info` local: the binary reads `[index + table]` each time) and
`n = 0;` before the speaker hash store (the binary zeroes it before the call). Left: register choice (data / level /
this), the counts_off / topics_off arithmetic order, and the binary keeps the found source slot in the dead
`speakers` argument slot. Tried: `n = num_speakers * 4` shared by counts_off / topics_off in two orders (115), the
topic hash address as `data + num_topics * 4 + topics_off` or an index (112), the source slot as a new local
declared after the hash loops with `s = speakers` set before the search, at function or block scope (151).

**D22305 (game/level_scripts.cpp; closer). savegame::set_speaker_bit 0x4d06e0: 73 -> 54.** The search as
get_speaker_bit's walk: `{ unsigned int *hp = (unsigned int *)(data + 4); for (i = 0; i < n; i++) { unsigned int
h = *hp; hp++; if (hash == h) break; } }` with `n` loaded first and the re-test through the macro (`if (i >=
CONVERSATION_SAVE_INFO(level, slot)->num_speakers)`), which keeps the binary's re-test. Left: level in EBX for the
whole function (the binary keeps this in EBX and reloads level later), and hp set before the count test. Without
the macro re-test (n, `!(i < n)`, `n <= i`, the test written twice, `i = 0` hoisted) the prologue is the binary's
but 8966 threads the loop exit past the re-test (67-71); an if + do-while is 112.

## Still differs (tried; do not repeat)

- **level_format_display_name 0x4d63b0 (16), savegame::get_speaker_bit 0x4d05f0 (18), savegame::get_int 0x4d0030
  (22), savegame::write_item_objects 0x4d2370 (36), savegame::read_items 0x4d2b70 (81)**: exhausted in earlier
  rounds (D18303, D18305, D18306, round d's list); not retried.
- **conversation_execute_action 0x4d5990 (51)**: the kind 7 / kind 10 false-call merge (D20005). Tried on top
  (D22306, notes only): kind 7 as two separate tests of an int local (`if (amount) ...true; if (!amount)
  ...false;`, D21803's redundant test: 8966 still merges, 51), `if (...) { true call; break; } if (1) false call`
  (51), and one call with a `have ? true : false` bool argument (291).
- two_stage_set, read_two_stage, set / get_topic_bit, init_conversation_slot, set_speaker_bit: see their D
  paragraphs above for what is left and the forms tried.

## Behaviour errors / bug candidates

None found. Equivalences of the changes: `open ? true : locked` right after `locked = false` is the same value as
the old `open ? 1 : (int)locked`; `name[0x41]` only enlarges a local buffer (the strncpy / strncat bounds are
unchanged); `count++` then the store equals storing `count + 1`; coff / hoff are the byte offsets of
counts[level] / hashes[level] in the two-stage block; `cp + (4 - counts_off)` is `data + 4 + i * 4`; the speaker
count read through the macro is the same field as `n`; init_conversation_slot's `info` was a pure alias of the
macro; mask / byte locals compute the same byte and bit.

## Header changes

None.

## Changed files

- src/game/level_scripts.cpp (two_stage_set D22300, read_two_stage D22301, set_topic_bit D22302, get_topic_bit
  D22303, init_conversation_slot D22304, set_speaker_bit D22305)

## --check

`VC6_STRICT=1 vc6-sweep --only "level_scripts\.cpp" --check src/decomp/vc6-matches.txt`: 1 unit: 109 match, 0 wrong
target, 15 differ, 0 not found, 0 errors; 109 matched there, 109 now, 0 lost.

Notes for the merge: get_topic_bit and set_topic_bit now share the `cp + (4 - counts_off)` speaker loop;
two_stage_set and read_two_stage now use two_stage_get's `hoff` byte offset (D20803).

## Worker 3

### Worker W3 notes (targeted round m-1021e, decisions D22400-D22499)

Batch: game/pathfinding.cpp, 9 functions. Result: 4 new matches, 3 closer (kept), 2 unchanged.

## New matches

- game/pathfinding.cpp 0x49cc50 pf_read_region (59 -> match), **D22400**. No `row` pointer local: every access
  indexed (`grid->rows[i].x`), and `n = grid->rows[i].num_cells;` read before `grid->rows[i]._pad02 = 0;`. With the
  pointer local 8966 hoisted i * 8 into EDI and spilled i; indexed, i stays in EDI and EBP is the zero / k-offset
  register as in the binary.
- game/pathfinding.cpp 0x49f340 pf_compute_endpoint_visibility (113 -> match), **D22401**. The seglist loops by an
  index (`for (j = 0; j < AVOID_NUM_SEGLISTS; j++) { sl = &avoid_seglists[j]; ...`; the binary's end test is a signed
  `jl` against the strength-reduced pointer, which a pointer loop (`jb`) cannot give), and the corner count read once
  into a `short n` (`for (k = 0, n = sl->num_corners; k < n; k++)`; the binary counts down a register copy). `int n`
  gives 9.
- game/pathfinding.cpp 0x49f810 pf_add_avoidance_nodes (118 -> match), **D22402**. The seglist loop by index j (as
  D22401); `have_rect` an int (the binary stores the zero register / 1 as dwords); the rectangle from pointer locals
  `const vector2 *pa = s->a->pos, *pb = s->b->pos;` with four separate stores `r.min_y = pa->y; r.max_y = pa->y; ...`
  (dword copies) and float_min / float_max (b's coordinate loaded once; the vsdk.h min / max macros reload it); the end
  `return s->cost != 0.0f;` (`mov eax, 1` / `xor eax, eax` tail).
- game/pathfinding.cpp 0x49e8c0 pf_find_nearest_navigable_point (157 -> match), **D22403**. found / best declared
  before the profile_label (D19502's device: their stores precede the EH state store); no `cells` local
  (`for (c = pf_graphs[size][layer].cells; ...)`, the address is recomputed and kept on the stack as in the binary);
  `nrm.set(bx - ay..., 0.0f, ...)` (both components computed before the stores); the to_vector3 result in a block local
  per branch (`{ vector3 t = cp.to_vector3(); *pos = t - nrm * 0.035f; }`, so it is called before nrm * 0.035f is
  evaluated); `p1 = *pos;` after `again = false; found = true; best = 1e9f;` in the restart. Plus a constant fix (see
  Behaviour errors).

## Closer, kept

- 0x49cdd0 pathfinding_load_graph: 113 -> 82, **D22404**. `pf_nodes = (vector2 *)operator new(pf_node_count *
  sizeof(vector2));` (the binary has no vector2 constructor-loop vestige after the call; same form as
  unnamed_alt_textures / gr_opengl), and the layer list's `list` store before its `count` store (84 -> 82).
- 0x49e110 pf_sweep_segment: 167 -> 42, **D22405**. Wall count once in `n = cell->num_walls`; obstacles by index j
  with `ob = &avoid_obstacles[j]` after the count/break test; the edge loop `for (k = 0; k < ob->num_points; k++)` with
  `prev = k;` in the body before `s->wall = &w;` (instead of `prev = k++` in the for); the initial stores in the order
  best_wall.flags, best_t, best_kind, best_corner, best_hit, size, layer (initialisers moved out of the declarations).
  All mnemonics now match; left: two frame slots swapped (best_kind at -56 vs the binary's -64, the cell countdown
  temp at -64 vs -56) and the obstacle pointer's spill slot (ours a frame temp, the binary the dead `s` argument slot).
- 0x49dc20 pf_sweep_test_wall: 285 -> 235, **D22406**. n through float locals (`float nx = dir.y; float ny = -dir.x;
  n.x = nx; n.y = ny;`, the x87 path; 260), and `fa = s->from - a;` in a vector2 local computed before `rel` (260;
  both 235). Left: frame 0x58 vs 0x48, kind in memory and corner in EBX (the binary: kind in EBX, corner in len's
  dead slot), the off / p / q temporaries.

## Still differs (tried; do not repeat)

**D22407 (notes only).**
- pf_find_cell_crossing 0x49ecb0 (21, unchanged): unsigned i with `(int)i <` (25), `vector2 *hp = &hit` for the call,
  `c->num_edges > i`, a bool for the intersect result, the count through `const int *np`, a `pf_cell *cc = c` copy,
  `register` e / i, `pf_edge **ep = &e; *ep = c->edges`, e assigned in the test (85), an edges local with `e = ed++`
  (54), the bounds test as `if (!(...)) continue;`, `&& c->num_edges > 0` in the bounds test (31), guarded do / while
  with e set inside the guard (28) or i set before it (21), `for (;;)` with a break (21). All 21 or worse: the count /
  edges / e / hit / seg register permutation and the e store after the count test look like 8966's choice.
- pathfinding_load_graph 0x49cdd0 (82 after D22404): the binary keeps `layer` on the stack (16-bit reload for
  `node->layer`) and holds 0x80 in EBP for `or word ptr [esi+4], bp` (PFN_NO_EDGES); ours keeps layer in EBP and does
  `or byte ptr`. Tried: `(short)layer` in a short local, no cast, the layer store after pos, the index store first
  (92), declaration orders of size / layer, the layer-list loop on i / size / size20 / found, the OR as
  `flags = flags | x`, `(ushort)` casts, through a `short *` / `ushort *`, an int / ushort local holding 0x80 (all
  propagated); `count` before `remaining -=` (84). Also tried for pf_nodes: `new char[]`, `new float[]` (= operator new).
- pf_sweep_segment 0x49e110 (42 after D22405): declaration orders (i first / last, best_kind first / last / after
  best_corner, ints first), renaming i (ci, idx, m, c, ii, cnt) or best_kind (kind, bk, hit_kind), the spline or cell
  loop on j, the obstacle loop on i, a `pf_grid_cell **cp` walk (count-down or with i), `*(results + i)`: no frame
  change (44). Pointer obstacle loop with the same edge-loop changes: 70. `w.b` first, flags first, wall first: no
  better. `found` compare swapped: no change.
- pf_sweep_test_wall 0x49dc20 (235 after D22406): `register int kind`, kind declared first, corner declared last,
  `unsigned kind`, corner initialised in its declaration (272), rel after the from test (267), n through a dir
  pointer (285), n.y first (285).
- pf_check_direct_walk 0x49e620 (201, unchanged): the clip as `vector3(...)` (201), `d * k + *from` (201), through a
  block temporary (201), `d *= k; end = *from + d;` (197, not kept), `#pragma inline_depth(N)` before the comment and
  `#pragma inline_depth()` after the function: 0 -> 234, 1..4 -> 201. The binary calls the vector3 copy constructor
  0x410cd0 out of line for operator='s returned copy and its frame is 0x38 (ours 0x30).

## Behaviour errors / bug candidates

- pf_find_nearest_navigable_point 0x49e8c0: the "within 0.025" threshold constant was `0.000625f` (0x3a23d70a); the
  binary's .rdata constant is 0x3a23d70b, which is `0.025f * 0.025f` rounded to float. Now written `0.025f * 0.025f`
  (vc6-compare -v no longer reports a target/contents difference). One ulp, so practically no gameplay change.
- No other behaviour changes. Every other edit is a statement order, a local's type (int have_rect, short n), an
  index instead of a pointer walk, or a different but equivalent expression (float_min / float_max have the same
  `a < b ? a : b` / `a > b ? a : b` as the min / max macros; `vector3::set` assigns x, y, z).
- Hint for whoever has pf_mark_nodes_visible_from_point (0x49ee80) and pf_graph_search (0x49fa70), which are not in
  my batch and which I did not edit: both still walk `avoid_seglists` with a pointer loop; D22401 / D22402 show that
  the binary's form is an index loop (signed `jl`), and the corner counts may want a `short n`.

## Header changes

None.

## Changed files

- src/game/pathfinding.cpp only (pf_read_region D22400, pf_compute_endpoint_visibility D22401,
  pf_add_avoidance_nodes D22402, pf_find_nearest_navigable_point D22403, pathfinding_load_graph D22404,
  pf_sweep_segment D22405, pf_sweep_test_wall D22406). Four functions marked "Confidence: exact".

## --check (VC6_STRICT=1 vc6-sweep --only "game/pathfinding\.cpp")

`1 units: 28 match, 0 wrong target, 8 differ, 0 not found, 0 errors`
`check against src/decomp/vc6-matches.txt: 24 matched there, 28 now, 0 lost`

(Scratch files in W: try.mjs / apply.mjs / side.sh / opdiff.js, variant JSONs, pathfinding.cpp.bak*, side_*.txt and a
copied pathfinding.cod; none are needed for the merge.)

## Worker 4

### m-1021e worker 4 (D22500-D22599)

Batch: game/combat.cpp (8 functions), game/new_status.cpp status_handle_click. Distances are the 2026-10-21
vc6-compare --score values.

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/combat.cpp | 0x463910 | living_combat::fire_weapon_missile | D22500 | `p += owner->pos; p += ofs;` and the spell chosen by if / else into an int local after the sums |
| game/combat.cpp | 0x4670a0 | living_combat::trigger_special_ability | D22503 | `(r = frand()) < ...chance` in both tests (float local r) |
| game/combat.cpp | 0x467670 | living_combat::trigger_reflex_ability | D22504 | the same `(r = frand())` form in both tests |
| game/combat.cpp | 0x464700 | living_combat::resolve_exchange | D22505, D22506 | role tests as inline helpers through a pointer local, both-sides case first, shares set before the learn calls, chain pick with one call per slot and a 0x50 buffer, net send through a bool local, send_net(bool, bool) |

**D22500 (game/combat.cpp). fire_weapon_missile 0x463910 is a byte match.** The binary's matrix assignment
`orient = owner->orient * rot` expands each row's vector3::operator= inline and calls the vector3 copy
constructor (0x410cd0) out of line for its returned copy. This happens because the inline budget runs out. The
two position sums written with vector3's `+=` operator (`p += owner->pos; p += ofs;`) use up the budget the same
way: the frame becomes 0x180 and the three copy constructor calls appear. The spell id as `int sp; if (species
== 0x58) sp = SPELL_FIRE_ARROW; else sp = SPELL_GENERIC_ARROW;` after the sums keeps the call's arguments after
the float code. A ternary in the call, or the local set before the sums, leaves the pushes scheduled into the
float adds. Same behaviour: `p.x + ofs.x` instead of `ofs.x + p.x` (exact float addition, commutative).

**D22501 (game/combat.cpp; closer only). get_protection 0x464de0: plain if / else with one shared bless tail,
45 -> 36.** Under the new distance the D21801 form (the tail written in both branches) scores 45. One tail
after the if / else no longer schedules the tail's `push 4` into the fild. Left: the join is placed after the
armour block (the binary puts the armour block after the epilogue and jumps back), and e6 is held in EDX.

**D22502 (game/combat.cpp; closer only). combat_get_weapon_tag_position 0x4665f0: 150 -> 2.** Three changes:
- bp / bo are block-scoped in the loop, so they share the orient temporaries' slots (frame 0x178).
- The tag position is copied as a vector3 (`p = *(vector3 *)world.m[3];` in both branches).
- `p += attacker->pos;`.

Together these use up the inline budget as in the binary: row 0 expanded with a copy constructor call, rows 1
and 2 through out-of-line operator=. Left: the call's `add esp, 16` comes before the products (the binary has it
after the second fmul). Not changing it: product statement forms (`dist += ...`, two product locals), declaration
orders and scopes of dx / dy / dz / dist, `vector3 d`, `best_d > dist` (11), pointer locals to bp or p (49),
`++i`, and `float d[3]` (100).

**D22503 (game/combat.cpp). trigger_special_ability 0x4670a0 is a byte match.** The test is written as
`... && (r = frand()) < owner->info->special_chance` (and `(r = frand()) <
w->type->weapon_triggered_effect->chance`), with `float r;`. 8966 then loads the chance's pointers after the
frand call instead of caching owner / info in callee-saved registers across it. That frees EDI for the binary's
-1 constant (`cmp word ptr [edx + 0x3c], di`, the request constructor's -1 fields, `or edi, -1` after
rep movsd). An `int none = -1` local changes nothing (constant-propagated), and `chance > frand()` gives 166.

**D22504 (game/combat.cpp). trigger_reflex_ability 0x467670 is a byte match.** The same device in both tests
(`(r = frand()) < opponent->owner->info->reflex_chance`, `(r = frand()) <
it->type->armour_triggered_effect->chance`). The FIXED_BUILD branch is untouched.

**D22505 (game/combat.cpp, include/engine/combat/combat.h). living_combat::send_net's first parameter is a
bool.** resolve_exchange passes its bool argument as a dword (`mov eax, [esp + 0x64]; push eax`, no
`and eax, 255`), and send_net 0x463540 passes it on unchanged to mp_send_living_combat's ubyte `hit`. send_net
still matches. The other callers pass 1.

**D22506 (game/combat.cpp). resolve_exchange 0x464700 is a byte match.** Changes, all same behaviour:
1. The multiplayer filter uses two file-local inline helpers, `combat_mp_client()` / `combat_mp_host()`, each
   `mp_player *p = mp_local_player; return p && (!)(p->flags & 1);`. With the helpers 8966 keeps the binary's
   second NULL / flag test. The plain `else if` drops it, and the helpers without the pointer local keep only
   the NULL test (5).
2. The client case is written `if (!one_sided) { both-sides test } else if (!locally(owner)) return;`. The
   owner push is then hoisted and the calls are cross-jumped as in the binary.
3. `share = 1.0f` / `0.0f` is set before the character_learn_chain_skill calls (the binary stores the
   constant into the slot before the call).
4. COMBAT_PICK_CHAIN calls character_get_chain_skill once per slot in an if / else-if chain (the tails are
   cross-jumped, `push edi` for slot 0). The buffer is `char buf_[0x50]`: the binary's frame is 0x58, with 16
   unused bytes above the 0x40 that the text needs.
5. The net tail is `bool special; if (...) special = ...; else if (...) special = ...; else return;
   send_net(one_sided, special);` (the binary's byte temporary at [esp + 0xc]).

Progress, in that order: 209, 187, 80, 48, 34, 19, 11, 5, match.

**D22507 (game/combat.cpp; closer only). chain_check_window 0x466ae0: 59 -> 47.** The first window branch keeps
the learn tail under a label (`learn:`). The second branch is `if (skill_id == SKILL_NONE) return 0.0f; if
(skill_id != last_chain_skill) goto learn; return 100.0f;`. Left: 8966 lays the labelled tail out after the
second branch's tests, and the binary keeps it in the first branch. Also tried:
- an inline helper `combat_chain_accept` for the whole accept part: in both branches 52, in the first only 50,
  in the second only 50, with a pointer local 52;
- a void learn helper: 59;
- the `goto learn` after `== last ... return 100.0f`: 96.

## Closer, kept

- game/combat.cpp 0x4665f0 combat_get_weapon_tag_position: 150 -> 2 (D22502).
- game/combat.cpp 0x464de0 living_combat::get_protection: 45 -> 36 (D22501).
- game/combat.cpp 0x466ae0 living_combat::chain_check_window: 59 -> 47 (D22507).

## Still differs (tried; do not repeat)

**D22508 (notes only).**
- living_combat::ai_choose_special 0x463d00 (26): the push test block after the epilogue (see D21805).
  New: an inline helper for the push flag test as `&&` in the else-if, and as a nested if (both 34); a
  Character pointer local for the push test (26). Not reproduced.
- living_combat::get_protection 0x464de0 (36), on top of D22501:
  - layout forms: the inverted test with the no-armour case in the else, `goto armour` with a shared tail
    label (both 36);
  - e6 forms: no e6 local, a pointer to equipment[6], e6 declared first, a bool for the "no armour" test (all
    36); e6 in p6 (48); `item_info *volatile` read (78);
  - prot forms: `float prot[1]`, a `float *pp` store, bless written as `prot = ... + prot; return prot;` (all
    36).
- status_handle_click 0x42bb00 (237): register permutation in the skill-row loop. Ours has i in ESI, rows in
  EDI and the running i * font_h in EBP; the binary has EDI / EBP / ESI. Tried: `int rows, i;`, rows / i
  declared around font_h, `register` on i, on rows and on both in both orders (all 237); unsigned rows (241); an
  unsigned index with a signed test, `++i`, `i * font_h + top` (237); an explicit running row-y variable and a
  `row_y` local (263). Not changed.

## Behaviour errors / bug candidates

None found. All changes keep behaviour. (The larger COMBAT_PICK_CHAIN buffer only adds stack space.)

## Header changes

- include/engine/combat/combat.h: `void send_net(bool state_arg, bool special);` (was `int state_arg`). This is
  needed for resolve_exchange's match (D22505). Only combat.cpp calls send_net. A strict sweep of the 23 other
  units that include combat.h loses nothing (756 matched there, 756 now).

## Changed files

- src/game/combat.cpp: fire_weapon_missile, send_net, COMBAT_PICK_CHAIN, the two new inline helpers
  combat_mp_host / combat_mp_client, resolve_exchange, get_protection, combat_get_weapon_tag_position,
  chain_check_window, trigger_special_ability and trigger_reflex_ability.
- src/include/engine/combat/combat.h (send_net declaration).

## --check results

- `VC6_STRICT=1 vc6-sweep --only "game/combat\.cpp" --check`: 1 unit, 56 match; 52 matched there, 56 now,
  0 lost.
- The units that include combat.h (ai, boss_ai, gametarget, living_entity and the 19 unnamed AI / boss / fx
  units): 23 units, 756 matched there, 756 now, 0 lost.
- game/new_status.cpp: not edited.

## Worker 5

### W5 notes (m-1021e, decisions D22600-D22606)

Helper scripts in this folder (not for the repo): `try.mjs` (variants with the vc6-compare --score distance),
`align.mjs` (our listing against the binary, LCS-aligned with operands and stack offsets normalised; much better
than vc6-side for operand-level differences), `apply.mjs`, `cod.mjs`. They compile into `t/obj` only.

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/unnamed_render_scene_0x481680.cpp | 0x482e20 | levelrender_build_draw_lists | D22600 | the living case (4..10) written first in the switch, items / containers after it (binary block order); the depth as `float dep = fvec.z * d.z + fvec.y * d.y; float dx = d.x; depth = dep + dx * fvec.x;` (the x product's operand order) |
| game/unnamed_render_scene_0x481680.cpp | 0x4829e0 | levelrender_fade_occluders | D22605 | each set's count and index list read once (`int n = set->count; int *ix = set->indices; index = *ix++`), the occluder count read once (`int nc`) and the occluder index in a local (`int oc`), the see-through count in a local, `alpha = 0xff` set right before the third loop, the switch as `case 3 / case 1 / case 2 / case 0` with the expired fades falling through to the constant alphas |

**D22600 (game/unnamed_render_scene_0x481680.cpp). levelrender_build_draw_lists 0x482e20:** the binary places
the living case right after the jump table and the item / container calls after it: source order of the cases.
The depth: the z and y products in one statement, then `float dx = d.x;` before `dep + dx * fvec.x` gives the
binary's `fld d.x / fmul fvec.x` for the last product (a pointer to fvec, to d, a float copy of fvec.x, a dot()
call or a reversed sum do not). Now exact.

**D22605 (same file). levelrender_fade_occluders 0x4829e0:** the binary's loops are count-down loops with the index
pointer advanced right after the load: `int n = set->count; int *ix = set->indices;` and `int index = *ix++;`
(loops 1 and 3); camera_num_occluders read once into a local (count-down); `int oc = camera_occluders[k];` gives
the shared `idx * 12` for the flag store and the seethru load; alpha initialised just before the third loop (the
binary's `mov byte [esp+0x10], 0xff` there). The switch: the binary lays out case 3, its expired tail sharing
case 1's `alpha = 0x80`, then case 2, its expired tail sharing case 0's `alpha = 0xff`: written as
`case 3: {... if (t < 270) { alpha = ...; break; } flags = ...;} case 1: alpha = 0x80; break; case 2: {...} case 0:
alpha = 0xff; break;` (fall-through). Same behaviour (alpha is only read in the third loop and every case sets it).
Now exact.

## Closer, kept

| function | old -> new distance | D |
|---|---|---|
| levelrender_draw_speech_text 0x484ae0 | 64 -> 12 | D22601 |
| overhead_label_place 0x484e30 | 148 -> 133 | D22602 |
| levelrender_render_scene 0x482200 | 166 -> 101 | D22603 |
| levelrender_add_effects 0x483ba0 | 179 -> 83 | D22604 |
| ai_state::pyrul_choose_attack 0x4b8da0 | 253 -> 63 | D22606 |

**D22601 (render_scene unit). levelrender_draw_speech_text:** the slot search with its own block-scoped loop
indexes `i, j` (side / row are only the placed slot: the binary does not store the loop variables, 64 -> 14);
the Ulsadana / type 7 test through `bool always = ...; if (always)` removes the duplicated epilogue (14 -> 12). Still
differs: with the bool the then-block (type 7 dead test) is laid out before the else-block; without it the
else-block's last test duplicates the epilogue. Tried (no gain): splitting the else-if, goto to a shared tail (with
the rest in a block), positive form with an empty then, redundant else, for(;;)/break, `>> 1 & 1`, `== 0`,
negated condition order (`type != 7 && _stricmp`), bools `hide` / `normal`, `always == true / false`.

**D22602 (render_scene unit). overhead_label_place:** `tt = screen_h * 2 / 3` and `d1 = y - third` assigned inside
the tests (function-scope locals) (148 -> 133). Still differs: register allocation (the binary keeps the row
pointer in EDI and *side in EBP; ours the reverse) and so no tail cross-jump between the two halves. Tried:
`register` on s / r / s2, declaration orders, a row-pointer local, separate d1 statement.

**D22603 (render_scene unit). levelrender_render_scene:** the vis-cache clearing loop with `lobj *lo = lobjs; int n
= set->count; int *ix = set->indices;` per set and `lobj *o = &lo[ix[j]]; o->flags &= ~0x6000;` (166 -> 103), the
speech loop by index `&character_speech[i]` (the binary's `jl`) (-> 101). Still differs: the sky loop's register
choice (binary: the fx_sky_list element pointer spilled, vi in ESI, n in EDI, node offset in EBX; ours the pointer
in EBX and the offset spilled) and `sets` kept in EBX by the binary; letterbox scratch registers. Tried: pointer
to fx->pos, n / k declaration orders, short k, node-desc pointer, k init inside the if, `!fx_sky_list[i]`,
`register sets`, the rest-height expression in three forms.

**D22604 (render_scene unit). levelrender_add_effects:** `vector3 c = vi->pos.unrotated(fx->orient) + fx->pos;`
(the binary's `fld pos / fadd temp` order), the emitter sum through operator+ with a reference to centre
(`const vector3 &cp = centre; vector3 c = cp.unrotated(fx->orient) + fx->pos;`, replacing the three component
statements), the hidden node's skip as `if (!(vis & 0x600)) { index++; n = n->next; continue; }` (the binary's
duplicated increments), `int index = 0;` declared before n (179 -> 83). Still differs: scratch register choice in
a few loads (eax / edx swapped), the node loop's `n = n->next` duplicated in both paths in ours, the emitter
desc address (`lea eax,[edx+ebp]` vs `mov eax,ebp; add eax,[ecx+44]`). Tried: `erec` as the vif_emitter_update
argument (closer but the binary recomputes the address: not kept), a goto to a shared `n = n->next`, `if / else`
increments, vi before base, a local for id_bits, a local for the owner handle, the components `u` local.

**D22606 (game/unnamed_pyrul_0x4b87c0.cpp). ai_state::pyrul_choose_attack:** in each stage the pick condition
through a bool (`bool pick = timer_up || !target; if (pick) ... else if (retarget) ...`) lays the retarget block
after the pick block as the binary does (all three stages: 191 -> 73); the stage 1 weight count as
`int nw = 3; if (!flagged) nw = 2;` (the binary's `mov eax, 3 / jne / mov eax, 2`, 253 -> 191) placed before the
weight stores (73 -> 63). Still differs: the stage 1 switch (the binary tests 0 explicitly, `jne default`, and
falls into case 2: a separate `default:` gives the explicit tests but shifts the scratch registers after it,
182), the stage 2 / 3 rand if/else block order, the shared `pyrul_cur_move` store tails and the final
SetTimeout / return 3 block placement. Tried: `!(!timer_up && target)`, `timer_up == true || target == NULL`,
`else { if (retarget) }`, `retarget == true`, a goto-shared pick tail, `!= 0` on the timer, `default:` before /
after / separate from `case 0` (4 orders), negated rand tests in stages 2 / 3, a goto to the final SetTimeout,
weight store orders.

## Still differs (tried; do not repeat)

- **level_vis_parse 0x486d40 (86, unchanged):** the binary keeps tf in EDI and 0 / the found set in EBP (frame 4
  bytes smaller: set never spilled). Tried: `set` at function scope / first / `register`, a `zero` int local for
  all the 0 uses, `print_to_console(line, (int)set)`, high at function scope, `!high`, an `n` local for the set
  count store, `else` instead of `continue`. Register allocation; stop.
- **levelrender_draw_name_labels 0x4850a0 (190, unchanged):** binary frame 0x154 (name buffer at frame +0x54, 44
  bytes of unused slots below it at +0x24..+0x2c and +0x38..+0x50); ours 0x128. Tried: name declared in the
  loop / first, x / y at loop scope, a vector3 copy of the label position (+4 only, as D16903). The binary's slot
  map (frame offsets): has_name +3, owner iter +4, position iter +8, used iter +0xc, name_h +0x10, row +0x14, h
  +0x18, name_w +0x1c, w +0x20, px / py +0x30 / +0x34, name +0x54. Needs a guess at the temporaries.

## Behaviour errors / bug candidates

None found. All changes keep behaviour (float sums only commuted: a + b == b + a exactly; loops read
loop-invariant counts once where nothing in the loop changes them; the fade switch's fall-throughs give the same
alpha for every state).

## Header changes

None.

## Changed files

- src/game/unnamed_render_scene_0x481680.cpp (build_draw_lists, fade_occluders, render_scene, add_effects,
  draw_speech_text, overhead_label_place)
- src/game/unnamed_pyrul_0x4b87c0.cpp (pyrul_choose_attack)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root W/src --only "unnamed_render_scene_0x481680|unnamed_pyrul_0x4b87c0"
--check src/decomp/vc6-matches.txt`: 2 units: 86 match, 0 wrong target, 14 differ, 0 not found, 0 errors; 84 matched
there, 86 now, **0 lost**.

## Tip for others

The fade_occluders / render_scene device should apply to other `for (j = 0; j < set->count; j++) ...
set->indices[j]` loops in this unit that compile to a count-down loop in the binary (level_vis_reset's neighbours,
levelrender_add_lobjs, draw_list): `int n = set->count; int *ix = set->indices;` and `*ix++`.

## Worker 6

### m-1021e worker 6 notes (D22700-D22799)

## New matches

| file | address | function | D | what did it |
|---|---|---|---|---|
| game/script_internal.cpp | 0x4dba30 | script_parse_objects | D22701 | navpoint_find into a block-local `np` before the spawn call; the trigger flag as `if (!get_flag) set_ai_active(0); else set_ai_active(1);`; Play action's move id read into `n` first and the anim as if / else |
| game/script_internal.cpp | 0x4dd780 | script_entry_check_done | D22702 | one exit for the switch, cases in the binary's code order, `done = false; break;` for the not-done cases (play action `goto`s into follow's copy), next action through a pointer, `done = true` before the flag store |
| game/script_internal.cpp | 0x4dccb0 | script_action_replace_weapon | D22703 | the new item in a block-local `it` before the equip call |
| game/script_internal.cpp | 0x4dd0b0 | script_entry_start | D22705 | `ok = true`, explicit `case -1: return;`, no default; cases in code order; wander flag passed as bool; next action through a pointer; the move found before the second _stricmp; the name copied by if / else of two strcpy |

**D22701 (game/script_internal.cpp). script_parse_objects 0x4dba30 is exact.** Three changes, each found from the
side listing:
- Sum.exe calls navpoint_find before it pushes any of the spawn call's arguments (`push eax; call navpoint_find;
  mov ecx, ci; push esi; push eax ...`). VC6 pushes the outer call's later arguments first when the call is
  nested; `navpoint *np = navpoint_find(navname);` in a block before the spawn call gives the binary's order.
- `c->set_ai_active(get_flag(...) ? 1 : 0)` gave `neg / sbb / neg`; the binary has `test eax, eax; jne; push eax;
  jmp; push 1` and one shared call, with c loaded into ESI on both paths before. Written as
  `if (!get_flag(...)) c->set_ai_active(0); else c->set_ai_active(1);` (the `if (get_flag) 1 else 0` order puts
  the blocks the other way round).
- Play action: `n = tf->ParseInt(); m = charinfo_find_move(c->info, 8, n, true);` (ParseInt before `push 1`, the
  same nested-call rule) and `if (m != NULL) a->arg0c = m->anim; else a->arg0c = -1;`.
Distance 115 -> 105 -> 97 -> 0.

**D22702 (game/script_internal.cpp). script_entry_check_done 0x4dd780 is exact.**
- The binary pushes EDI only after the early returns (shrink-wrapped, D8701 / D18704): the switch had three
  `return false;`; written as `done = false; break;` / `break` the switch region has one exit and the push moves.
- The byte table (0x4dda08) numbers the case bodies by their lowest case value, but the code order is the source
  order: walk, play, wait go, trigger, trigger nav, trigger visit, the "true" group, wander for, follow, synch,
  combat. The jump table showed wander / die with their own entry (0x4dd95f, follow's `xor bl, bl` block), so
  they are `done = false; break;`, placed before wander for; play action's fail jumps to the same block, written
  `goto not_done;` with `not_done:` inside follow's `if (!so->timer.valid()) { ... }`.
- `lea ecx, [esi + 8*edx + 0x1c]` (dead) before the load: the next action through `script_action *na =
  &so->actions[so->cur_action + 1]; if (na->type == ... || na->type == ...)` (a `next` int local does not give it).
- `mov bl, 1` before the flags store: `done = true;` above `c->flags &= ~0x1000;`.
Distance 165 -> 152 -> 93 -> 81 -> 0. Behaviour unchanged (done is false on every path that returned false).

**D22703 (game/script_internal.cpp). script_action_replace_weapon 0x4dccb0 is exact** with
`item_info *it = item_create(a->arg0c); character_equip_item(&old1, &old2, c, 5, it);` in a block. With the
nested call, c (pushed before item_create is called) was live across the call in a way that gave it EDI; the
local moves c to ESI and the count / `a` pair to EDI as in Sum.exe. Earlier rounds' 25+ register forms all
kept the nested call.

**D22705 (game/script_internal.cpp). script_entry_start 0x4dd0b0 is exact.**
- The jump table 0x4dd698 starts at -1 (`inc ecx; cmp ecx, 0x21`), so the source has `case -1:`. A `case -1:
  return;` next to `default: return;` is dropped by VC6; with no default and `bool ok = true;` (the unknown types,
  wait go, label and the triggers now leave the switch with ok set and skip the message, as the old `default:
  return;` did) the case stays and the table matches.
- The cases in the binary's code order (no block before say; follow between pass go and show / hide; replace
  armour before guard spline).
- `script_action_wander(c, a, *(bool *)&a->flag14)`: the binary pushes the byte without `test / setne`
  (flag14 is a ubyte in level_scripts.h; it holds 0 / 1 only, so the same value; a header change to bool would
  also do, not made).
- The next action through a pointer (as D22702); the move found before the `_stricmp` with a comma expression
  `(m = charinfo_find_move_any(...), _stricmp(m->action, "...") == 0)` (the string pushed after the call).
- The name: `if (o != NULL) strcpy(name, o->info->name); else strcpy(name, XSTR(...));` (two inline strcpy
  expansions, cross-jumped after the `lea edx, name`).
Distance 259 -> 188 -> 177 -> 93 -> 0.

Device worth reusing: **a call nested in another call's arguments** is evaluated after the outer call's later
arguments are pushed. Where Sum.exe calls the inner function first (its `add esp` merged into the outer call's),
the original had the inner result in a local (D22701, D22703, D22705).

## Closer, kept

| file | address | function | D | old -> new distance |
|---|---|---|---|---|
| game/script_internal.cpp | 0x4dc6e0 | script_action_walk_to | D22700 | 88 -> 59 |
| game/script_internal.cpp | 0x4dce60 | script_action_replace_armor | D22704 | 196 -> 120 |

**D22700. script_action_walk_to:** a `vector3 *goal = &np->pos;` local used in the three calls. so is now in EBP
and np in EBX for the whole function (Sum.exe: so in EBX, &np->pos in EBP, np in the frame). Same behaviour.

**D22704. script_action_replace_armor:** the saved records written as in replace_weapon,
`cutscene_saved_equipment[cutscene_saved_equipment_count].field` (the binary indexes with the count in ECX and
stores `count + 1` per branch); no record pointer. Left: Sum.exe keeps a zero register in EBX (old1 / old2,
`cmp byte ptr [save], bl`, the NULL record items, the `sete al; cmp al, bl` slot-8 test), `save` in the frame,
the constant 7 as immediates and EBP pushed only around the search loop; ours keeps save in BL and 7 in EBP.

## Still differs (tried; do not repeat)

- script_action_walk_to 0x4dc6e0 (59 with D22700): goal declared first / `register` / with a `register` so copy,
  np through `navpoint **`, `goal = &(np = a->nav)->pos`, goal assigned inside the call arguments (88), np read
  as a->nav each time (86), a / np declared late, np in a block, walk through `bool *`, the layer read from
  a->nav, the path store split, `void **` for so->path. Without goal (88): so never gets a register; with goal:
  so / np / goal take EBP / EBX / frame instead of EBX / frame / EBP.
- script_action_replace_armor 0x4dce60 (120): `bool *ps = &save` (store or all uses), `bool save[1]` (two
  forms), `save != false`, `old1 = old2 = NULL`, a `robe` bool local for the slot-8 test (as `if (robe)`,
  `== true`, `!= false`, function scope), `(slot == 8) == true`. All compile to the same code: save stays in BL.
- ls_khosanilab_puzzle_trace_beam 0x4f7b50 (63): nested if / else instead of `continue` (63), case c `goto` into
  a's `turn_dirs` (104), c to a's and b to a's `cur = next` (104), if / else with `turn:` / `step:` labels after
  it (104), `if (!open(a)) { b, c, dead end }` with a's code last and labels (274, 252). Labels in the loop
  change the loop head's scheduling; the cross-jump direction (8966 keeps the last copy, Sum.exe keeps case a's)
  is not solved.
- script_build_avoid_spline 0x4ddc60 (230): seg as `pf_edge` / `pf_wall` (216, frame still 0x16c of 0x180; not
  kept). Not attempted further: float scheduling plus a frame layout with 0x14 bytes more.
- pxochat_handle_click 0x423a20 (288): read only. Sum.exe keeps -1 in EBX for the whole function (the three
  stores, the strlen counts); ours keeps 2 (`flags |= 2`, `pxochat_state = 2`). Not attempted.

## Behaviour errors / bug candidates

None. (All changes keep every value, call and store; script_entry_start's unknown types still return without the
retry message.)

## Header changes

None.

## Changed files

- src/game/script_internal.cpp: script_parse_objects (D22701), script_action_walk_to (D22700),
  script_action_replace_weapon (D22703), script_action_replace_armor (D22704), script_entry_start (D22705),
  script_entry_check_done (D22702); comments updated (four now "exact").

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "script_internal" --check src/decomp/vc6-matches.txt`:
1 unit: 25 match, 0 wrong target, 3 differ, 0 not found, 0 errors; 21 matched there, 25 now, **0 lost**.
level_script_khosani_lab.cpp and new_pxochat.cpp are unchanged.

## Worker 7

### m-1021e worker 7 notes (D22800-D22899)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/unnamed_avoidance_0x4a6710.cpp | 0x4a8390 | avoid_do_poly_seg_list_links | D22800 | door add `p.x = p.x + d.x; float t = d.y; p.y = t + p.y;`, the corner's flags through `sl->corners[k].node`, an int index for the other lists, `break` for the out-of-memory exit, the graph point indexed in place |

**D22800 (game/unnamed_avoidance_0x4a6710.cpp). avoid_do_poly_seg_list_links 0x4a8390 is exact** (34 -> 0).
Five changes, each needed:
- The door offset: `p.x = p.x + d.x; float t = d.y; p.y = t + p.y;`. With this form 8966 keeps p.y on the FPU
  across the door test (`fst`, then `fld d.y; fadd st, st(1)` and the `fstp st(0)` at the join) and adds p.x
  from memory, as the binary does. Without the float local, `p.y = d.y + p.y` / `p.y += d.y` add d.y from
  memory and use up the register (worse).
- The node pointer's bias: with fewer `node->pos` reads, 8966 based ESI on `&node->flags` (+4). Reading the
  corner's flags as `sl->corners[k].node.flags` (test and clear) moves the base back to the node (`[esi]` = pos,
  `[esi+0xc]` = links). Local copies of the flags (`ushort` / `int` / pointer), a links local, a `c` corner
  pointer and an extra `(void)node->pos` do not do this.
- The other lists' loop as `for (int j = 0; j < AVOID_NUM_SEGLISTS; j++) { o = &avoid_seglists[j]; ...`: the
  binary's signed end test `cmp edi, end; jl` (a pointer loop gives `lea; cmp; jb`).
- The out-of-memory exit as `break` to the one `pf_cur_size = saved_size;` at the end (the binary's exits
  differ: saved_size in EDX / EAX, the store after the pops). A `goto` to a label there does not do it.
- The graph loop as `if (g->points[i].flags & PFN_VISIBLE) { node->links[n++] = &g->points[i]; ...`. This gives
  the binary's `mov edx, [eax+0x10]; add edx, ebp`. `other = &g->points[i]`, `g->points + i`, `i + g->points`,
  a points local, a byte offset and an unsigned index all give `mov edx, ebp; add edx, [eax+16]`.
Same behaviour: p.y equals node->pos->y when the offset is added; break leads to the same restore and return.

## Closer, kept

- game/unnamed_avoidance_0x4a6710.cpp 0x4aa400 avoid_poly_add_search_nodes: 167 -> 8 (**D22801**). The
  previous point through an index (`j = ob->num_points - 1; for (i...) { ... &ob->points[j] ...; j = i; }`, the
  binary's `lea ecx, [8*eax - 8]` / `lea ecx, [8*eax]`, without the `num_points > 0` guard), `hits[nhits++] =
  ...` (nhits in EBX), and each length as `(p - q).len()` on its own temporary. With one `d` local, 8966
  merges the three len calls into one tail. The binary keeps three exits, and its frame is now right (0x40).
  Left: the binary stores a float 0 into i's slot (`mov [esp+0x2c], 0`) between the overlap call's pushes,
  and its overlap-failed exit loads it (`fld [esp+0x20]`). The final `return 0.0f` is a constant. Tried for
  that (all give a constant or worse): `float len = 0.0f; ... return len;` (also with len assigned on the
  len paths, the body wrapped in `if (overlap) {...} return len;`, else branch), `d.x = 0` / `return d.x`, a
  one-float array, `t = 0; return t` (18), `volatile float` (20, own slot), `return *&len`. Unused d / prev
  locals removed (no change). Same behaviour.
- game/unnamed_avoidance_0x4a6710.cpp 0x4a8b80 avoid_merge_shared_segments: 191 -> 128 (**D22802**). The corner
  point copied as `refs[k].list->corners[cc].pos = p->pos;` (vector2 operator=) in both branches. 8966 then
  cross-jumps the two avoid_seglist_calc_shared_corner_offset calls into one, as the binary does (both branches
  push their arguments and `jmp` to a shared call). Left: the binary computes the third avoid_ref_entry call
  before pushing any argument (its pop is folded into the call's `add esp, 0x1c`), the x store through the
  full address before the `lea` for y, the register choice (w in EBP, &refs[k] in EBX), and frame 0x2a4
  against 0x29c. In the binary, c / cc / l use different slots in the two branches. Tried (no change or
  worse): a `vector2 *q` / `avoid_corner *q` / `avoid_seglist *ml` local (128), `float px` (149), memcpy
  (176), the POD `avoid_spline_end` copy (176), `e = avoid_ref_entry(...)` before the call (137 / 143),
  c / cc / l / p / sp declared in each branch block (128).
- game/unnamed_avoidance_0x4a6710.cpp 0x4a73f0 avoid_merge_obstacle_polys: 263 -> 233 (**D22803**). The point
  copies as vector2 assignments: `poly->points[i] = ob->points[i];` in the inside case, `out[count] = hit;`
  and `out[count] = cur->points[next];` in the walk. Left: frame 0x250 against 0x22c (other is at 0x38, out
  at 0x160, 20 unused bytes between them), the early `return false` placed inline in the binary, and the walk's
  registers (out pointers in EBX / EDI, count and start on the stack). Tried: a bool local or an if / else for
  the flags test (no change), `out[0] = poly->points[i]` (265), the final copy loop as an assignment (no
  change), `p = &out[count]; out[count++] = hit;` (252), do / while(!done) (no change), while(!done) (244),
  a `*(vector2 *)p = hit` store (230, not kept: it only moves the distance).
- game/unnamed_avoidance_0x4a6710.cpp 0x4a8680 avoid_seglist_add_segment: 264 -> 262 (**D22804**). `pf_edge ws;`
  instead of `seg2d ws;` (D19301's device: the binary's frame has 8 spare bytes above ws). Frame 0x18 against
  0x1c. In the binary, ib takes sl's dead argument slot (`[esp+0x40]`). Tried, with no change: ib declared at
  its first assignment, `int ib, ia` order, `avoid_seglist *s = sl`, while loops for the corner searches,
  ep / idx / ws / i declared in their blocks.
- game/new_charselect.cpp 0x406bf0 mp_charselect_handle_click: 299 -> 281 (**D22805**). The button loop through
  `ui_button_state *b = &mp_charselect_buttons[i]; ui_button_def *d = &mp_charselect_button_defs[i];` (b first)
  and `else if (d->flags & 2)`. 8966 then uses the loop's pointers after the break, as the binary does. Left:
  the binary keeps the constant 2 in EDI from the scrollbar loop on (`cmp esi, edi`, mode = 2, drag index =
  2), frame 0x14 against 8, the list loop's rows / font_h slots, and the shared store tails of the typing paths.
  Tried: d first (286), a flags int local (281, same), b only (290).

## Still differs (tried; do not repeat)

- game/unnamed_avoidance_0x4a6710.cpp 0x4a6e00 avoid_prim_overlap (13, unchanged). Not retried: earlier rounds
  list about 43 forms (NOTES-match-2026-10-21b / -21d). The byte side-by-side shows only the first two
  projections' product order (the binary's d.x * u.x first) and the dead `fst [esp+0x1c]`.
- game/new_dialogue.cpp 0x409000 dialogue_frame (28, unchanged). Not retried: D19907 / D16803 cover the tail. In
  the byte comparison, the only real difference is the tail (`key` loaded into EDI before the mode test, one
  shared exit). The other marks are relocations with an addend.
- game/unnamed_projectors_0x4c0250.cpp 0x4c0740 projector_record::to_lobj_space (64, unchanged). Not retried:
  D19801 found the distance-6 form. It is rejected because 8966 sums the inlined dot as (y + z) + x, which
  changes x87 rounding (D9804).

## Behaviour errors / bug candidates

None found. Every change keeps the values and the order of the calls.

## Header changes

None.

## Changed files

- src/game/unnamed_avoidance_0x4a6710.cpp: avoid_do_poly_seg_list_links (D22800, exact),
  avoid_poly_add_search_nodes (D22801), avoid_merge_shared_segments (D22802), avoid_merge_obstacle_polys
  (D22803), avoid_seglist_add_segment (D22804)
- src/game/new_charselect.cpp: mp_charselect_handle_click (D22805)

## --check results (VC6_STRICT=1, against src/decomp/vc6-matches.txt)

- unnamed_avoidance_0x4a6710: 59 match, 0 wrong target, 8 differ; 58 matched there, 59 now, 0 lost
- new_charselect: 31 match, 0 wrong target, 1 differ; 31 matched there, 31 now, 0 lost

## Tooling note

The byte-exact side-by-side I used is in W (bside.mjs: it compares the .cod bytes with Sum.exe's bytes per
instruction). vc6-side.mjs compares only mnemonics, so it hides register and operand differences. For example,
in avoid_do_poly_seg_list_links the node pointer's bias showed up only in the byte comparison.

## Worker 8

### m-1021e worker 8 (decisions D22900 to D22999)

Distances are vc6-compare --score (the 2026-10-21 scorer).

## New matches

None.

## Closer, kept

| file | address | name | old -> new | decision |
|---|---|---|---|---|
| vsdk/unnamed_vfx_channel_0x55b440.cpp | 0x55bc00 | vfx_channel::eval | 28 -> 6 | D22900 |
| vsdk/unnamed_listbox_0x52ca60.cpp | 0x52e380 | ui_listbox::scroll_page_up | 32 -> 20 | D22901 |
| vsdk/unnamed_listbox_0x52ca60.cpp | 0x52f080 | ui_listbox::draw | 179 -> 148 | D22902 |
| vsdk/unnamed_listbox_0x52ca60.cpp | 0x52d080 | ui_listbox::process | 124 -> 104 | D22903 |

**D22900 (vsdk/unnamed_vfx_channel_0x55b440.cpp; closer only). vfx_channel::eval 0x55bc00: 28 -> 6.** The
found-in-run path is written with a `ubyte *d = (ubyte *)data;` and indexes `d[cursor]`, `d[cursor + 2]`,
`d[cursor + 3]` (the binary's `mov eax, [ecx]; xor edx, edx; mov dx, [ecx+8]; lea esi, [edx+eax]`; a `p = data +
cursor` pointer, a fresh `ubyte *q`, `d + cursor` / `cursor + d` / `&d[cursor]` and a block all give `add`, 14),
the run byte is tested with `switch (d[cursor]) { case 1: break; default: return 0; }` (`xor eax, eax; mov al, [esi];
dec eax`), and the two-byte value is built in a short: `short v = d[cursor + 3] << 8; v |= d[cursor + 2]; return
base_value + v;` (`mov ah` / `mov dl` / `or`; `p[2] | (p[3] << 8)` gives `mov dh` / `mov dl` into one word, `+`
forms 23, `* 256` 21). Same values: the return is a short, so the 16-bit OR and the int OR give the same low word.
Left: the one-byte return loads base_value into EAX and p[2] into ECX in the binary, ours the other way round.
Tried for that return, all 6: `short v = p[2]`, `short r = base_value; r += p[2]`, `p[2] + base_value`,
`(short)` / `(int)` / `(ushort)` casts, `& 0xff`, an int v, an int b, a `ubyte k`, `const short *bp = &base_value`
(also with a short v), a `vfx_channel *c = this`, a `short v[1]`, `register short`, `base_value - -p[2]`, int / short
result locals, the if / else return, the returns inside the switch's `case 1:` (two shapes), and a shared
`short v` for both returns (23). `!bytes` first (20), `if (d[cursor] == 1) { ... } return 0;` (11). A static inline
helper for the found path is not inlined (144-147). `goto found` after the loop: same as in place (14 before the
other changes).

**D22901 (vsdk/unnamed_listbox_0x52ca60.cpp; closer only). ui_listbox::scroll_page_up 0x52e380: 32 -> 20.** The
last branch of the line snap is `vscroll = top + fh;` (top = line * fh + e->y, so the same value) instead of
`(line + 1) * fh + e->y`. With it the BY_ENTRY / ALIGN_BOTTOM entry_at argument comes out with the binary's
registers. Left: the line snap (binary: line * fh in EDX, e->y in ECX and reused by its `(line + 1) * fh + e->y`;
ours the other way round) and the binary's final `inc / imul / add`. Tried with no change on top of the old form:
`top == vscroll`, `e->y + (line + 1) * fh`, `fh * (line + 1) + e->y`, `line * fh + fh + e->y`, `vscroll - top == 0`,
`int top = line * fh; top += e->y;` (also declared first), `int lf = line * fh`, `const int *pf = &fh`, `int *lp =
&line`, an e->y local for both terms, a `last = num_lines - 1` local, an `nx = e->next` local (all 32), `int v =
vscroll; vscroll = top; if (v != top) ...` (39). On the new form, all 20: `top == vscroll`, `fh + top`, the split
top, an e->y local, `e->y + line * fh`, `fh * line`, `int *tp = &top`, `int *vp = &vscroll`, a bool for the test.

**D22902 (vsdk/unnamed_listbox_0x52ca60.cpp; closer only). ui_listbox::draw 0x52f080: 179 -> 148.** The sunken
frame is drawn with one call per branch (`else if (is_hidden()) draw_sunken(&colors[3], 1); else
draw_sunken(colors, 1);`): the binary pushes the 1 before the test and each branch loads colors and pushes it.
The ternary in other spellings (`colors + 3`, `&colors[0]`, `!is_hidden()`, a Color pointer local by ?: or if /
else) stays at 179. Left: register allocation in the entry loop (binary: e in EDI, y in EBP, x in EBX, the colour
pointer in a stack slot, EBP pushed only before the loop; ours: e in EBP, y in EDI, the colour pointer in EBX, x on
the stack), and the hover / set_clip argument registers. Tried, all 148: the colour pointer declared at function
scope (two places), as `Color *ca[1]` (copied to c, or used throughout), through `Color **cp`; x declared before the
inner loop or at function scope; `register` on x, y, e, hover (and combinations); hover / e declaration orders; e
declared in the for; `e = entries.next;` before the for; `e == hover`; the visible test as `if (is_entry_visible(e))
{ ... }`; lines declared before y.

**D22903 (vsdk/unnamed_listbox_0x52ca60.cpp; closer only). ui_listbox::process 0x52d080: 124 -> 104.** The hidden
case ends with `goto out;` to the final `return key;` (label `out`), not its own `return key;`. 8966 then gives the
zero constant EBX and the scroll-bar code matches. Left: the binary still has a separate return for the hidden case
(key in EDI), pushes EBP only after it and keeps key in EBP after ui_gadget::process (ours keeps key in its argument
slot). Tried: the disabled return also through `out` (104), each switch case `key = 0; goto out;` (126), the hidden
block's key through an `int k` (also `register`, also at the block top; 124), `return key = 0;`, `{ int *kp = &key;
*kp = 0; }` and `key = 0; return 0;` in the cases (124), the body after the hidden case in a block (124),
`vscrollbar != NULL` (124), `int k = ui_gadget::process(key); key = k;` (124).

## Still differs (tried; do not repeat)

**D22904 (notes only). Tried, still differing.**
- ui_listbox::set_vscroll_fraction 0x52ed70 (16): the binary keeps top in EBP, a line_bottom induction variable in
  EBX, re-reads vscroll from this on every pass, and spills num_lines into f's dead argument slot. Tried (16 unless
  noted): `vscroll <= ... ` reversed compares (20), `top + i * fh`, `i * fh + top + fh`, `this->vscroll`, an unsigned i
  with `(int)i <`, a nested if, `!(a || b)`, `continue` for the first test, an `int n` local, a do / while, `int *vs =
  &vscroll` for the final store only / everywhere / only in the loop (15), `ui_listbox *lb = this` (15, as D18903),
  `lb->vscroll` everywhere, `const int *tp = &top`, `int top[1]`, `top >= vscroll` (18), explicit line_top and
  line_bottom induction variables in four shapes (24-40), only one of them (24, 46), a `volatile` read (21).
- ui_listbox_entry::wrap_text 0x52ca60 (59): register choice (split in EBP and cur_indent on the stack in the
  binary). Tried (59 unless noted): `int *sp = &split` passed (and read through), `int *cp = &cur_indent` for the
  read, the init, the store, both; an `avail` local for the width; `int *wp = &lw; *wp += cur_indent;`; a pointer to
  the length slot (55, as before).
- ui_listbox::snap_scroll_bottom 0x52ebe0 (90): the binary keeps y in ESI and a copy in y's argument slot (written on
  each line pass), this in EDI. An `int last = y` (top, first, after the empty test, returned for the empty case),
  `int lastv[1]`, `*lp` through `int *lp = &last`: all 90 (8966 merges the copy).
- skeleton::read 0x544090 (68): not retried beyond reading D14004 / the 2026-10-21c notes (the SIB order flips with
  the count of float compares modulo 4; every listed form was tried).

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/vsdk/unnamed_vfx_channel_0x55b440.cpp: vfx_channel::eval (found-in-run path, comment; still likely).
- src/vsdk/unnamed_listbox_0x52ca60.cpp: ui_listbox::scroll_page_up (D22901), ui_listbox::draw (D22902),
  ui_listbox::process (D22903), with comments; all still likely.

## --check results

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "<unit>" --check src/decomp/vc6-matches.txt`:
- unnamed_listbox_0x52ca60: `1 units: 63 match, 0 wrong target, 6 differ`; 63 matched there, 63 now, 0 lost.
- unnamed_vfx_channel_0x55b440: `1 units: 18 match, 0 wrong target, 1 differ`; 18 matched there, 18 now, 0 lost.

## Devices worth reusing

- Indexing `d[cursor + k]` from a base local instead of a pointer `p = data + cursor` gives 8966's
  `lea esi, [edx + eax]` with the base loaded first (D22900).
- A 16-bit value from two bytes as `short v = hi << 8; v |= lo;` gives `mov ah` / `or` instead of the `mov dh` /
  `mov dl` word merge (D22900).
- A ternary argument whose two arms the binary loads separately (constant argument pushed before the test) is an if /
  else with one call per branch (D22902).
- An equivalent `top + fh` for `(line + 1) * fh + y` changed register choice elsewhere in the function (D22901).

Helper scripts in w8/try: tryd.mjs (distance per variant, restores the file, no deletes), diff.mjs (LCS side by
side; its compile folders are kept in w8/tmp, not the system Temp folder).

## Worker 9

### m-1021e worker 9 (decisions D23000 to D23099)

Batch: choose_support_spell, combat_toggle_ready_stance, combat_approach, consider_life_drain, combat_cast_frame
(game/unnamed_ai_combat_0x4bce60.cpp), set_valid_chars / set_invalid_chars (vsdk/unnamed_inputbox_0x5521a0.cpp),
cutscene_parse_table (game/unnamed_cutscene_0x434900.cpp).

## New matches

| file | address | name | decision | what did it |
|---|---|---|---|---|
| game/unnamed_ai_combat_0x4bce60.cpp | 0x4bf5a0 | ai_state::consider_life_drain | D23000 | no `Character *c` local: `list[i].character->` written out in each test |
| game/unnamed_ai_combat_0x4bce60.cpp | 0x4bfaa0 | ai_state::combat_cast_frame | D23001 | no `ai_group *g` local; `float d` for the distance; no `count > 0` test |

**D23000 (game/unnamed_ai_combat_0x4bce60.cpp). ai_state::consider_life_drain 0x4bf5a0 is exact** (was distance
54). The first loop reads `list[i].character->_unk690 / ->level / ->type` directly, without the `Character *c`
local. With the local, 8966 strength-reduced the loop on `&list[i].score` (list + 8) and shared that pointer with
the two later loops (EDI pushed late). Without the local it walks from list, as the binary does, and computes
list + 8 only after the loop. The prologue `push edi` and the count / list registers then match. Earlier rounds'
element pointers, walking pointer, `p = list` copy and inline helper did not get this. Same behaviour.

**D23001 (game/unnamed_ai_combat_0x4bce60.cpp). ai_state::combat_cast_frame 0x4bfaa0 is exact** (was distance 158).
Three changes:
1. The healer branch has no `ai_group *g` local: `target->ai.group->num_enemies` and `->enemies[i]`. The
   binary reloads the group from the target argument on each pass. This gave the 0xb0 frame and EBX as the
   zero register (158 -> 42).
2. `float d = owner->distance_between_edges(target, false, -1); if (d > spells[mode_target].distance)`. 8966
   then reloads mode_target from memory after the call, as the binary does. Before, it kept `spell` in EDI.
   This also gave the binary's give-up block layout: the 7 s block inline, the cur_character one at the end, and
   the get_attack_range and choose_offensive_spell copies kept apart (42 -> 10). Earlier rounds tried the
   give-up layout with gotos and labels.
3. The cur_character branch is `else if (cur_character && cur_character->ai.group)`, without
   `&& ->count > 0`. The loop's own entry test becomes the binary's `cmp dword ptr [eax], 0 / jle`, without the
   second test (10 -> 0). Same behaviour: with a count of 0 or less, the loop body never runs.

## Closer, kept

- ai_state::combat_approach 0x4bda50: 50 -> 45 (D23002). A file-local `static inline int
  cast_result_zero(int r) { if (r) return 0; return 1; }` and `return cast_result_zero(goal_do_cast_at_target(...))
  ? true : false;` give the binary's neg / sbb / inc, then test / setne. Earlier rounds tried more than 20
  forms for this. Left: register choice in the boss branch, where the binary loads info into ECX and keeps
  owner in EAX (it reloads info in the else branch); ours reuses EAX. The goal_do_cast_at_target argument
  pushes also use a different register (ours EDX, the binary ECX). Same behaviour.

**D23002 (game/unnamed_ai_combat_0x4bce60.cpp, closer).** The cast_result_zero helper, described above. Other
helper bodies tried: `return r == 0;` (50), `return !r;` (50), a bool `? true : false` helper (50), and an int
`ok` local with `!= 0` (57).

## Still differs (tried; do not repeat)

**D23003 (notes only). Tried without a match:**

- ai_state::choose_support_spell 0x4bea80 (18): the EBX / EBP swap only. Tried this round (all 18 unless noted):
  unsigned cure, unsigned vitalize, unsigned heal, `boss_target **op = out` for the NULL store,
  `int *vp = &vitalize`, `int vitalize[1]`, `int cure[1]`, a `c2 = cure` copy for the score call, an owner local
  (28), `register` on vitalize / cure / both, `out[0] =`, the vitalize if form `vitalize = 0; if (type != 5 ||
  elapsed()) ...` (19), a goto past the vitalize call, hurt or i declared first, the pick loop with
  `goto found` (27), a `boss_target *t` for the store and flag tests (27), and t for the store only (18). The
  no-local device of D23000 / D23001 does not apply here: the function has no pointer local.
- ai_state::combat_toggle_ready_stance 0x4bdce0 (24): the prologue `mov esi, ecx` before `push edi`, and ready
  in AL where the binary keeps it in BL (the constant-1 register). Tried this round: ready as a block-scope
  local, `bool *rp = &ready`, `float *bp = &owner->stance_blend`, an owner local for the tail (all 24), s
  computed before the ready test (30), cos split before the test (28), a non-volatile flip (53), flip declared
  first or assigned after the declaration (24), and `volatile char flip` (36).
- ai_state::combat_approach 0x4bda50 (now 45): see Closer. Boss-branch forms tried this round: a `char_info *ci`
  local in the tiger branch (50), the else branch nested with `ci` (50), `float *mx = &max_range` (50), the
  species compare reversed (50), an owner local in the else branch (50), `float *rp = &range` (50), the boss
  test first (44, needs an `else if (1)`, so not kept), `fov_range < dist` (58), and `if (ranged)` (54). Return
  forms tried: `(x == 0) == true`, `& 1` (48), `!!`, `^ 0`, `1 - (x != 0)` (48), `!= false`, if-set int
  locals (54, 57), a bool local, `(int)(...) != 0`, `+ 0 != 0`, and `| 0` (50 or 51 unless noted).
- ui_inputbox::set_valid_chars / set_invalid_chars (183 / 182): the binary keeps three identical range blocks
  (lo in the shared frame+0 byte slot, memset expanded in each) that 8966 cross-jumps. Tried through the macro:
  a function-scope lo, lo through a pointer, `char lo[1]`, and an `allowed + lo` form. All four give the 0x8c
  frame but merge all three blocks (852 bytes, 173). A lo per else-if branch block keeps 0x94 (183). Also tried:
  a hi-first order (221), `register char lo` (183), a do / while (0) macro (183), an unsigned char index local
  (183), no lo or hi locals (291 or 261), lo only (216), and static inline / __forceinline / inline helper
  functions (221).

  A small test file shows that 8966 at /O2 and /Ox does not share frame slots between block-scope locals, and
  that it cross-jumps the memset tails. Only `/Os` kept both copies, and the unit under /Ox /Os matches 2 of 13
  functions, so the flags are not the answer. ui_inputbox::process has the same cross-jump problem. Why the
  binary keeps the copies is unknown.
- cutscene_parse_table 0x4355e0 (/Od, 70): the round-robin register is one step behind the binary from
  `ent->num_fogs++` on. In the binary one register (EDX) is taken around the `fog->shot = tbl.ParseInt()` store
  with no instruction for it. A `(char)` cast on that store takes the register and aligns the whole tail
  (distance 21), but it truncates the value, so it was not kept. That points to a hidden conversion
  temporary at this store. Same-behaviour casts that leave the code unchanged (70): `(short)`, `(short)(int)`,
  `(unsigned short)`, `(unsigned)`, `(long)`, `(short)(unsigned short)`, `(unsigned short)(short)`,
  `(int)(short)`, `(short)(long)`, `(__int16)`, `(unsigned __int16)`, `(int)(unsigned short)`, and
  `(short)(int)(short)`. Also unchanged: `++`, `+= 1`, `= + 1` for the increment, a `(float)` far_dist,
  `(float)(double)` near_dist, and an `(int)` index.

## Behaviour errors / bug candidates

None found. All kept changes keep the behaviour.

## Header changes

None.

## Changed files

- src/game/unnamed_ai_combat_0x4bce60.cpp (consider_life_drain, combat_cast_frame, combat_approach, and the new
  file-local helper cast_result_zero just before combat_approach's comment)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "unnamed_ai_combat_0x4bce60" --check
src/decomp/vc6-matches.txt`: 1 unit, 24 match, 4 differ; 22 matched there, 24 now, **0 lost**.
(The inputbox and cutscene units were not changed. vc6-matches.txt needs the two new entries, 0x4bf5a0 and 0x4bfaa0.)

## Lesson for later rounds

A pointer local that holds a member (`c = list[i].character`, `g = target->ai.group`) can change 8966's
strength reduction and CSE: it changes the loop's base pointer, a register kept across calls, and the frame.
Writing the member access out at each use, as the binary's reloads suggest, matched two functions that earlier
rounds had called register choice. A float local for a call result (`float d = call(); if (d > x)`) stopped
8966 from keeping an earlier value in a callee-saved register across the call.

## Worker 10

### Worker W10, round m-1021e (decisions D23100-D23199)

## New matches

| File | Address | Function | D | Was |
|---|---|---|---|---|
| game/unnamed_tiger_0x499e50.cpp | 0x49b0a0 | tiger_pick_target | D23100 | 20 |
| game/new_automap.cpp | 0x402760 | automap_handle_click | D23101 | 218 |

**D23100 (game/unnamed_tiger_0x499e50.cpp). tiger_pick_target 0x49b0a0 is exact with the query's stores in the
order `_unk08, best, result, self, candidate`** (the order boss_target_list_build already uses). With `result = 0`
written second, 8966 CSEs that zero with the start-up zero and keeps it in EBX across the first loop (`cmp esi, ebx`,
`q.result = ebx`, an `xor ebx, ebx` at the loop top), which is the residue earlier rounds called register choice.
With `result` after `best`, the store is an immediate (`mov [q.result], 0`), the null test a `test esi, esi`, and the
start-up zero goes to EDX as in the binary. Found by trying all 120 orders of the five stores: only this one matches
(`_unk08, best, result, candidate, self` gives 2, most others 4-6). An aggregate initialiser also moves the zero to
EDX (18 / 19) but zero-fills `rating` too. Same behaviour (independent stores to a local).
Lesson worth trying elsewhere: when 8966 holds a zero (or other constant) in a callee-saved register that the
binary does not, the order of the constant's stores relative to other loads can decide whether it is CSE'd.

**D23101 (game/new_automap.cpp). automap_handle_click 0x402760 is exact** with four changes, each found from the
side-by-side:
- the two label-box rectangles as `int x / int y` locals inside the reveal test's block (as automap_label_box_hover
  and automap_labels_hover already have them): the binary computes x and y before pushing w / h (218 -> 122);
- the selected box as `view + automap_scroll_x + automap_label_box_x` (and the y twin): with box before scroll the
  instructions match but the two loads swap their globals (vc6-compare's relocation-target warning);
- the key switch's case bodies in the binary's order (Delete, ESC / M, C, I, R, S; 122 -> 74);
- the script index in a local before the remove call (`int idx = level_script_find_index(...);
  SAVEGAME_AUTOMAP.automap_label_remove(cur_level_index, idx, label);`): the binary calls find_index before it
  pushes the label (74 -> match).
Same behaviour.

## Closer, kept

None.

## Still differs (tried; do not repeat)

- **ai_state::tiger_ai_frame 0x49bed0 (104)**: the binary keeps a zero in EBP for the whole function (cmp edi, ebp;
  push ebp; sub eax, ebp ...); ours uses immediates (`sub eax, 0`) and saves no EBP. Tried this round: `int idx = 0`
  (in place and at the top), `Character *t = NULL; t = ...`, `bool done = false` at the top, all six orders of the
  wander block's three stores (failures, timer, state), the choose result in a local, `!idx`, `!t || ... != 0`, the
  timeout's 0 through an int local. All 104.
- **ai_state::tiger_combat_update 0x49b6e0 (71)**: the binary keeps `fire` in BL and tests it at the join (re-reading
  charge_fired); 8966 threads the constant in every form. Tried this round: fire as `char` / `unsigned char` with 1,
  the dist test nested in `if (!charge_fired)`, the store through a `bool *`, `fire == true && !charge_fired` order,
  `bool fire[1]`, the fire test and stores in a static inline helper (with and without a separate `fire = false`),
  the follow_path branches swapped (`if (follow_path(0)) { ... }`, with and without else: 109). All 71 otherwise.
  Also seen: the binary reads `charging` separately in each arm of the follow_path test (ours hoists one load).
- **tiger_anim_remap_in 0x49c130 (36)**: the loop registers rotated (see D19401). Tried this round: the weight copy as
  `*(int *)&... = *(int *)&...`, ready as a bool (54), a count local, a while loop, a do / while under an `if`, a
  `tiger_boss *` pointer for the stores, a `float *w` per element. All 36 unless noted.
- **mp_handle_combat_sequence 0x4b1280 (90)**: the binary converts variant / skill to ints (EBX / EBP) right after
  storing their bytes, keeps t and hit in memory. Tried this round (current scoring): int copies of variant / skill
  (98), ubyte special (95), both (91), int copies declared right after skill (94), with ubyte special (101), the int
  copies as `register` in both orders (91), plus hit read through a `ubyte *` (91). Ours gives t and hit the
  registers the binary gives the int copies.
- **automap_draw_map 0x403750 (200)**: two structural differences found, neither moved by source:
  - the clip rectangle: the binary computes view_x / origin_x / left first, then view_y / origin_y / top
    (statement order `view_x, origin_x, left = max; view_y, origin_y, top = max` gives this order, but the registers
    differ: binary RI / view_x / view_y in ESI / EDI / EBX and RI re-read for bottom; ours EBX / ESI / EDI) - 206,
    not kept; with an `ri` local, rx / by temporaries, a `const int *size` row pointer: all 206;
  - the mask-tile loop: the binary computes x (i's ftol first) and y completely before pushing any argument of
    gr_bitmap_stretched; 8966 sinks them into the arguments (y first) in every form tried: x / y reusing sx / sy,
    x / y at function scope (first or after sx / sy), `x += ...` split statements, `x -= tile_w / 2` split,
    `x = x - tile_w / 2 - left` before the call, a static inline helper taking (x, y, left, top) with x / y as
    locals or as the argument expressions. All 200 (identical code).
- **tiger_path_to_target 0x49b520 (139)**: not retried (out-of-line vector3 constructors; earlier rounds).

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/game/unnamed_tiger_0x499e50.cpp (tiger_pick_target, D23100)
- src/game/new_automap.cpp (automap_handle_click, D23101)

## --check

```
VC6_STRICT=1 node tools/vc6-sweep.mjs --root W/src --only "unnamed_tiger_0x499e50|new_automap" --check src/decomp/vc6-matches.txt
2 units: 75 match, 0 wrong target, 5 differ, 0 not found, 0 errors
check against src/decomp/vc6-matches.txt: 73 matched there, 75 now, 0 lost
```
unnamed_level_complete_0x4b0960.cpp was not changed.

Helper scripts (W/v): try.mjs (distance per variant, restores the file; flags relocation-target warnings),
side.mjs (whole-function side-by-side, removes vc6-compare's kept folder file by file), apply.mjs.

## Worker 11

### W11 notes (m-1021e, decisions D23200 to D23299)

Batch: travel_steer_update, turn_towards, mode_traveling_frame, ai_find_nearest_wall_hit (ai_move),
boss_ai_unload, boss_move_in_range_dispatch (boss_ai), conversation::restore_state, gamecreate_handle_click.

## New matches

None.

## Closer, kept

- game/conversation.cpp 0x4d84d0 conversation::restore_state, 30 -> 18, **D23200**.
- game/new_gamecreate.cpp 0x409de0 gamecreate_handle_click, 92 -> 62, **D23201**.
- game/unnamed_ai_move_0x4b1410.cpp 0x4b15a0 ai_find_nearest_wall_hit, 99 -> 95, **D23202**.

**D23200 (game/conversation.cpp; closer only). conversation::restore_state 0x4d84d0 (30 -> 18).** A file-local
`static inline int rs_topic_bit(int level, int slot, const char *sn, const char *tn, int k)` around
`savegame_state.get_topic_bit`, used for the first three topic-bit reads (bit0..bit2); the fourth (bit3) stays a
plain call. The helper is placed above the function's address comment, because vc6-compare takes the next function
after the comment. All 16 helper / plain combinations were measured (all four through the helper: 24; none: 30; the
best 0111 mask: 18). The rest is the order in which 8966 loads level / slot / s->name / t->name around the pushes;
it changes with the combination, but no form gave the binary's order for every call. Same behaviour.

**D23201 (game/new_gamecreate.cpp; closer only). gamecreate_handle_click 0x409de0 (92 -> 62).** After the
`if (!gamecreate_field_active[i]) return 0;` guard, a redundant `if (gamecreate_field_active[i]) break;` (the
redundant-test device of D21803). 8966 folds the second test (one test in the code, as in the binary). The default
case of the field switch now jumps forward to the field check, and the case bodies and the typing path are laid out
in the binary's order. Left: the c == 0xff key switch (Escape / Backspace / Enter) is laid out right after the typing
path. The binary has it after the mouse loop. So the shared timer tail belongs to the backspace path in ours and to
the typing path in the binary (the cross-jump goes the other way). Same behaviour.

**D23202 (game/unnamed_ai_move_0x4b1410.cpp; closer only). ai_find_nearest_wall_hit 0x4b15a0 (99 -> 95).** The
obstacle loop is an index loop (`for (k = 0; k < AVOID_NUM_OBSTACLES; k++) { ob = &avoid_obstacles[k]; ...`).
8966 strength-reduces it to the binary's signed `jl` pointer compare (the pointer loop gave `jb`). D16304 found
this too but did not keep it. Same behaviour.

## Still differs (tried; do not repeat)

- **D23203 (notes only). conversation::restore_state (18 kept).** Tried on top of the helper (no change from 18):
  a speaker-bit inline helper, `this->level` / `this->slot` in the speaker call and in each topic call, an int local
  for the speaker bit, int copies of level / slot, the bits declared first, and a second helper taking
  `savegame &` for one call. Worse: a helper taking the speaker / topic pointers (58, 46), one taking `this`
  (30), the reversed argument order (60), bool bits (98), and the `savegame &` helper on the fourth call (24).
  A `savegame *sg` local and an int array for bit1 / bit2 gave no change (30).
- **D23204 (notes only). travel_steer_update 0x4b3c80 (17, unchanged).** Tried: the side vector as
  `vector3(rvec.x + fvec.x, fvec.y + rvec.y, fvec.z + rvec.z)` (the binary's mixed order: 19, 8966 still puts rvec
  first in all three adds, and the dot becomes z, x, y), the same as member stores or `set()` (111),
  `fvec - -rvec` (170), the dot with fvec.z first, `0.5f <` on the left, `(z + y) + x`, `x + (z + y)` (17), a
  double compare (19). Unused locals in five places (17; a vector3 one 22). 8966 orders commutative operands
  its own way. A form elsewhere in the function can change the dot's order, so the order may depend on the whole
  function's temporaries. Not found.
- **D23205 (notes only). ai_state::turn_towards 0x4b2af0 (65, unchanged).** Tried the D19700 device: a
  `float *tp = &t` passed to both move_playing calls, to one of them, at function scope or in the block (65).
  A pointer for angle cannot cross the `goto apply`.
- **D23206 (notes only). ai_state::mode_traveling_frame 0x4b4100 (73, unchanged).** The only difference is
  EBP / EBX for &timer40 (the rest of the distance is the jump table read as code). Not retried beyond D21303's
  list. This is register choice.
- **D23207 (notes only). ai_find_nearest_wall_hit (95 kept).** The binary keeps `from` in EBP. It reloads `dir`
  from its argument slot for every edge. It computes both edge differences before loading dir (z product first,
  then fxch). It spills n and count into dir's / obstacles' argument slots. With dir read through
  `*(const vector3 * volatile *)&dir` the allocation becomes the binary's: 46, or 50 without the index loop.
  That is not an exact match, so it is not kept. The volatile shows that dir must not be a register candidate.
  No other form stops it: a `vector3 **dp = &dir` (8966 folds it), `const` / `register` on the parameters, an
  inline facing helper (two forms), an inline squared-distance helper, difference locals (ny / nx, both product
  orders), the d terms written out twice, and the bounds through move.a / move.b (95-101).
- **D23208 (notes only). boss_move_in_range_dispatch 0x4a0ea0 (241, unchanged).** Most of the distance is the
  jump table read as code. Findings:
  - The binary's callees return without any conversion. It returns `mov eax, 1` / `xor eax, eax`, which is the bool
    `return a < b;` form, as boss_move_target_in_range (0x4a0e60) does.
  - An int return for the dispatcher adds `and eax, 255` after every bool callee. That is wrong.
  - With D7603 (3) (tiger_move_in_range and giant_salamanka_target_in_reach declared bool, tested only in this W
    copy and then reverted) only two things are left. One is the Luminar / Murod case: the binary has
    `fld r; fcomp st(1); test ah, 0x41; fstp st(0); jne default; mov eax, 1`. The other is the default `xor eax, eax`,
    which that case's false path shares.
  - Luminar forms tried, with and without the header change: if / break, `>=` with `return false` or break,
    `!(d < r)`, `return r > d`, `return d < r`, `!(d >= r)`, `!(r <= d)`, a ternary, a float d local (both orders),
    a bool local (241-244 without the header; 230-231 with it).
  - 8966 always emits `fcomp [r]`. A `return` comparison gives its own `xor eax, eax` tail, and the default stays
    `xor al, al`.
  - Needs the D7603 (3) declarations, whose definitions are in the tiger and salamanka units (outside this batch).
- **D23209 (notes only). ai_state::boss_ai_unload 0x4a04d0 (66, unchanged).** One register differs: the info
  pointer is in ECX in ours and EDX in the binary. The rest is the jump table read as code. As D7603 / D20406
  say, the callees must be ai_state members, declared in a header and defined in other units. Not changed.
- **gamecreate_handle_click (62 kept).** Tried on top of D23201, all 60-62. The `c == 0xff` branch first
  (60, the switch is still inline). A shared timer tail through `goto` (len at loop scope or function scope). Two
  ifs (`c != 0xff`, then `c == 0xff`). `else if (c == 0xff)`. A `goto check` after typing. The special keys moved
  behind the check with `goto special` / `goto check` (60). A bool `special` flag.
  - Before D23201: a bool `typed` local (92). The typing block in `for (;;)` / `do { } while (0)` with break (97-99).
    `if (key == 0) { } else`.
  - The check also written in the default case, with a goto past it: 41, but it moves key into EDI and reorders
    the function. Not the binary's code, not kept.

## Behaviour errors / bug candidates

None found.

## Header changes

None. (The boss_ai.h edits in D23208 were trials in this W copy only and are reverted; boss_ai.h equals the
repository's.)

## Changed files (in W\src)

- game/conversation.cpp (restore_state: D23200, the rs_topic_bit helper before the function)
- game/new_gamecreate.cpp (gamecreate_handle_click: D23201, header comment updated)
- game/unnamed_ai_move_0x4b1410.cpp (ai_find_nearest_wall_hit: D23202)

## --check (VC6_STRICT=1 vc6-sweep --only "conversation|new_gamecreate|unnamed_ai_move_0x4b1410" --check src/decomp/vc6-matches.txt)

4 units (the regex also took a fourth unit): 80 matched there, 80 now, 0 lost.

## Worker 12

### W12 notes (m-1021e, decisions D23300-D23399)

## New matches

| file | address | function | D | what did it |
|---|---|---|---|---|
| vsdk/unnamed_ui_button_0x526940.cpp | 0x526a50 | ui_button::process | D23302 | single exit for the state machine (`goto out`), one `\|\|` press test, case 5 as `if (is_mouse_over()) { ... }` |
| game/new_saveload.cpp | 0x425de0 | saveload_handle_input | D23303 | the closing Escape test as a one-case switch; the Escape and Enter cases of the name box without fall-through |

**D23302 (vsdk/unnamed_ui_button_0x526940.cpp). ui_button::process 0x526a50 is exact** (79 -> 0). There are three changes.
The behaviour is the same.
- Every `return key;` inside the `for (;;) switch` is `goto out;`, with `out: return key;` after the loop. With one
  exit, 8966 pushes EBP late, after the disabled / hidden early returns, as the binary does (the shrink-wrap of
  D18704). It still writes an epilogue copy for each of cases 6-9 (79 -> 53).
- The two press tests are one condition: `if ((... && has_hotkey(key)) || (is_selected() && key == 0x1c)) goto
  pressed;`. The `pressed:` block then stays after case 0's body and is not pulled up behind the Enter test (53 -> 41).
- Case 5 is `if (is_mouse_over()) { if (!ui_left_released) goto out; ...; state = 0; break; } state = 0; break;`
  in place of an early `if (!is_mouse_over()) { state = 0; break; }`. 8966 then cross-jumps the not-over path into
  case 2's `state = 0` block, as the binary does (41 -> 0). These did not help: `goto reset` into case 2's block and
  an else-if after the early break both stay at 41.
The unit is now 23 / 24.

**D23303 (game/new_saveload.cpp). saveload_handle_input 0x425de0 is exact** (95 -> 0). The final
`if (key == KEY_ESC) { ...; return 0; }` is `switch (key) { case KEY_ESC: ...; return 0; }`. This gives the binary's
`mov eax, esi; dec eax; je` with the Escape block at the end of the function. Its `return 0` tail is shared with the
`saveload_selected_row < 0` return, and the row-hit block lands where the binary has it (95 -> 2). In the name box's
switch, `case KEY_ESC:` no longer falls through into `case KEY_ENTER:`. Each case has its own
`saveload_editing_name = false; break;`, and 8966 merges the two tails (`jne default; jmp`) as in the binary
(2 -> 0). Putting the Enter case first also matches. The behaviour is the same. With `if` and a `goto ret0` the
distance is 48. Reordering the cases or adding a default does not change the result (2). The unit is now 37 / 37.

## Closer, kept

- vsdk/unnamed_vfx_emitter_0x55ff00.cpp 0x5601f0 fx_recb0::init_particle: 33 -> 3 (D23300).
- vsdk/unnamed_vfx_emitter_0x55ff00.cpp 0x561900 fx_recb0::render_d3d: 41 -> 35 (D23301).

**D23300 (init_particle).** `case 1:` has no braces, so its `vector3 v` is at switch scope. Case 0's `v` and case 1's
`v` then get separate slots (-216 / -204). That makes the binary's 0xe8 frame and moves every temporary and `unused`
to the binary's slots. The behaviour is the same, and no case label is jumped past the initialisation. Putting
another brace level around case 0 or case 1 does not help (33). One difference is left: the binary computes
`desc->frames + frand_range(..)` as `fild [ebx+0x20]; faddp st(1), st`, and ours uses `fiadd`. Tried for that,
all 3 or worse (both shape tails changed):
- casts to double and long double (5);
- frand + frames (3);
- `(float)` casts (3);
- `/ 4800.0f` (3);
- a 1.0 double constant (5);
- `(double)spread` (3);
- `spread * -1.0f` (7);
- a float sum local (3);
- an int frames local (117);
- a named random value with a float frames copy in 7 forms (`+=`, `lt = fr + lt`, `fr + rr`, array, pointer: 5,
  because the named value stays on the FPU stack with `fadd st, st(1)` / `fstp st(0)`);
- a float frames local before the call (106: stored across the call);
- an assignment inside the expression (106);
- a `const int *` to frames (3);
- a volatile read (79).

**D23301 (render_d3d).** `float fw = w; hh = h * size / fw;` (and `float fh = h;` for hw) gives the binary's
`fild w; fdivp st(1), st` instead of `fidiv` (41 -> 35). The behaviour is the same: the int to float conversion
is exact, as before. These forms do not help: `(float)` casts on the divisor or both operands, double and long
double casts, `* (1.0f / w)` (57), and a `const int *` to w.
Left in render_d3d:
- the store of `end` comes before the loop's first `jge`;
- in the colour pack, 8966 schedules the g / b loads later;
- the vertex block: 8966 hoists the `v[2].tu / tv` stores above `v[0].sy`, and the zero register is EDX, not ESI.
Tried for the vertex block:
- sn / cs reused for ay / by with x / y locals (67, 198);
- a `d3d_tlvertex *v2` (223: the whole register plan changes);
- `v[3] = v[0]` before the v[2] stores (58).
Tried for `end`, all 35:
- `end` declared in the while block, also initialised there;
- `end += 6` and `*pe =` forms;
- `1000 > end` and `!(end >= 1000)`;
- `p` as a bare truth test.
Two forms are worse: `end <= 999` (41) and `end` set before the Lock (186).

## Still differs (tried; do not repeat)

**D23304 (notes). fx_recb0::render 0x561ec0 (18).** The difference is still the swap of the age / alpha slots and
the order of the this / mat reloads after gr_poly. New forms, all 18 with the slots unchanged:
- a pointer local for each of the alpha, size and fade arguments of get_fade, and `*ap` in the alpha test;
- `float age[1]`;
- `const float *pa = &age` in the range test;
- a bool local for the range test;
- renaming age / alpha (ag, zage, a0, alp, aalpha, z): slot order does not depend on the names.
`float ag = age;` as the get_fade argument is worse (159).

**D23305 (notes). item_page_out_all 0x46ca50 (30).** Only the EBX / EBP swap (slot vs. zero) is left. New forms, all
30 (type_first 34):
- `continue` for `i == slot`;
- nested ifs;
- the type test first;
- `s->paged` as a truth test;
- a nested outer if;
- the whole body inside `if (slot != 17) { }`;
- `!(type < 0)`.
This is register choice only.

**D23306 (notes). ui_button::draw_arrow 0x527440 (248 -> 244 / 240, not kept).** The binary's register plan differs
throughout. It has aw in ESI, ah in EDI, u in EBP and -2 * aw in EBX, and i, n, t and my in the frame. Its frame is
4 bytes bigger and it uses shr on the flags. Tried:
- an unsigned flags local or `(unsigned)flags` casts (244);
- `min(u / ah, t / ah)` and the equivalent ternary (244);
- `u += -2 * aw` and `u += -aw * 2` (244);
- a step local `s = -2 * aw` (240);
- `t += aw + aw` (244).
None of these changes the register plan; it needs a different approach.

**D23307 (notes). fx_recb0::get_transform 0x5612b0 (94).** The binary keeps `next` in EBP and evaluates each
channel's frame term first. It spills that term into frac's dead slot. Ours keeps the term in EBP and `next` in
memory. Tried:
- the operands swapped in each sum, also with `w * eval` (94: 8966 keeps its own order);
- `register` on next / frame (94);
- next declared first (94);
- w1 / w0 declared first (94);
- a block-scope int for the frame term (155);
- a `__forceinline` blend helper or term helper (287-289).

## Behaviour errors / bug candidates
None found.

## Header changes
None.

## Changed files
- src/vsdk/unnamed_ui_button_0x526940.cpp (ui_button::process D23302)
- src/game/new_saveload.cpp (saveload_handle_input D23303)
- src/vsdk/unnamed_vfx_emitter_0x55ff00.cpp (init_particle D23300, render_d3d D23301)

## --check results (VC6_STRICT=1 vc6-sweep --check src/decomp/vc6-matches.txt)
- unnamed_vfx_emitter_0x55ff00: 7 matched there, 7 now, 0 lost
- unnamed_ui_button_0x526940: 22 matched there, 23 now, 0 lost
- new_saveload: 36 matched there, 37 now, 0 lost
- all three together: 65 matched there, 67 now, 0 lost

## New observations (devices)
- `float fw = w;` used once as a divisor or addend gives `fild; fdivp` / `fild; faddp` where a direct int operand
  gives `fidiv` / `fiadd` (D23301). This works only when no call comes between the copy and its use.
- A case body without braces (switch-scope locals) gives a local its own slot instead of sharing one with a
  sibling case's local (D23300).
- Duplicated tails (no fall-through, or the work inside `if (cond) { ... }` with the short path after it) let 8966
  cross-jump into another case's identical block as the binary does (D23302, D23303).
- A one-case `switch` for an equality test gives `dec eax; je` and moves the block to the end (D23303).

## Worker 13

### Worker W13 notes (m-1021e, decisions D23400-D23499)

Batch: game/unnamed_goals_doors_0x475180.cpp (matrix3_pack_orientation, matrix3_unpack_orientation,
door_find_floor_point, ai_goal_evaluate), game/unnamed_tentacle_beast_0x4c6480.cpp
(ai_state::tentacle_beast_ai_frame, tentacle_beast_frame_hook), game/new_inventory.cpp
(inventory_drag_to_equip), game/characterinfo.cpp (character_learn_chain_skill).

## New matches

| file | address | function | D | what did it |
|---|---|---|---|---|
| game/unnamed_tentacle_beast_0x4c6480.cpp | 0x4c7070 | ai_state::tentacle_beast_ai_frame | D23400 | the sub-mode switch's `case AIS_GET_CLOSE:` written last |

**D23400 (game/unnamed_tentacle_beast_0x4c6480.cpp). ai_state::tentacle_beast_ai_frame 0x4c7070 is exact** with
the sub-mode switch's cases in the order CLOSE_COMBAT, TENTACLE_GRAB, TENTACLE_RELEASE, GET_CLOSE. The code was
already identical; only the exit block's place differed (earlier rounds: the exit block sits after the stance
block in the binary; early-return, else and goto forms tried). GET_CLOSE's `set_mode(COMBAT, CLOSE_COMBAT)` is cross-jumped with the stance block's
identical call; when the GET_CLOSE case is the last statement of the function its tail falls into the function's
exit block, and 8966 then keeps that exit right after the stance block's set_mode (as the binary) instead of
giving the stance block its own epilogue. GET_CLOSE first (before CLOSE_COMBAT) stays at 93. Same behaviour (case
order of a switch). General device: **the source order of switch cases sets the binary's block order** (also
used in D23403); look at the jump table's targets in address order and write the cases in that order.

## Closer, kept

| file | address | function | D | old -> new distance |
|---|---|---|---|---|
| game/unnamed_tentacle_beast_0x4c6480.cpp | 0x4c7960 | tentacle_beast_frame_hook | D23401 | 138 -> 24 |
| game/unnamed_goals_doors_0x475180.cpp | 0x476ab0 | door_find_floor_point | D23402 | 209 -> 16 |
| game/unnamed_goals_doors_0x475180.cpp | 0x475830 | ai_goal_evaluate | D23403 | 295 -> 68 |

**D23401 (tentacle_beast_frame_hook 0x4c7960; closer, 138 -> 24).** Inline budget (D15802 / D16901). The binary
calls out of line, after lobj_raycast_all: vector3(x, y, z) (the landing's operator+ result), the copy constructor
(operator='s discarded return of `r->pos = ...`), go_idle, the copy constructor again (`fwd = r->orient.fvec`),
set_from_forward_up; `up` is built inline. Three changes give exactly this call sequence:
- the victim offset as `vector3 down = v->orient.uvec * 1.5f; p -= down;` (was a 3-float constructor and three
  component subtractions);
- `vector3 up; ... up.set(0.0f, 1.0f, 0.0f);` in place of `vector3 up(0.0f, 1.0f, 0.0f)` (a later expansion
  changes which earlier ones are inlined: the budget is not spent strictly in source order);
- fwd / up in a `{ }` block after go_idle: they then share the frame slots with down / step and the landing's
  temporaries, frame 0xb4 -> 0xa8 (the binary's).
Left: the step product (`throw_velocity * reported_frametime`) against the ray.start copy: the binary loads
r->pos.x through ESI + 0x10 before `push ebp; lea ebp, [esi + 0x10]` and stores step.x straight into ray.dir.x.
Tried (all 24-40): every order of `landed = false`, the ray / hit declarations, ray.start, a named step or the
product straight into ray.dir (33 orders); step in a block; step assigned after declaration (114); a frame-time
local; ray / hit declared first or at function scope; hit before ray; `bool landed = false` (81); landed set after
the ray set-up (86) or in the miss branch (149); ray.start by components (124), memcpy (110) or set() (136); the
ray's flag stores moved (80 / 24); a ray pointer for the call; a `vector3 *rp = &r->pos` source.
Other forms of the down / up part: `p -= down` alone 117, `down = uvec * 1.5f` alone 117, `v->pos = p - down` 180,
up by components 119, up after `fwd.y = 0` 117.

**D23402 (door_find_floor_point 0x476ab0; closer, 209 -> 16).** Three changes:
- `pf_edge *w = cell->walls[k];` before the edges loop (the binary walks walls with a pointer and compares the
  hoisted wall; alone 107);
- `vector2 mid(...)` constructed inside the layer loop (was a function-scope vector2 with two component stores).
  The constructor's arguments are evaluated right to left, so the y sum comes first as in the binary.
- the ray's +0x28 and +0x30 zeroed as floats, `ray.stop_t = 0.0f; ... ray.min_radius = 0.0f;` (were `_unk28 = 0`
  / `_unk30 = 0`): the binary stores immediate zeros there and uses the zero register only for the int fields
  (flags, exclude_flags, _unk2c); same bits. 34 -> 16, and the size is now the binary's 836 bytes.
With these, the loops, the registers, the frame (0xb8) and all frame slots but one are the binary's. Left: mid's
slot (ours -156 from the frame top, the binary's -152; -156 is unused in the binary) and the second sum's fmul,
which the binary schedules before the argument pushes. Tried: mid as `= vector2(...)` (206), through operator+ /
operator*(float, vector2) (101-109), separate sum locals (32, not kept: same shape, extra locals), a sum vector2
(101), `0.5f *` first (34), y stored first (211), mid without a constructor (48), mid in an inner block (34), at
size-loop or function scope (143 / 144), an `e = edges[0]` local (36), n declared in the block (34), a vector3
mid (63), every declaration order of the function-scope locals (34), cell / w declared beside mid (34), a 0.5f
local (34).

**D23403 (ai_goal_evaluate 0x475830; closer, 295 -> 68).**
- Every `return X;` inside the switch is written `result = X; break;` (the binary returns through EBX: `mov ebx, X;
  pop edi; mov eax, ebx; ...`; 8966 copies the epilogue into each of these blocks). Alone: 235. The first
  `is_mode(DISABLED)` test keeps its plain `return AI_GOAL_SUSPEND` (mov eax, 3 before EBX / EDI are pushed).
- The cases in the binary's block order (from the jump table 0x475cd4): ATTACK, FOLLOW / PROTECT, CAST, SWITCH_COMBAT,
  HIDE, SNEAK, PATHFIND, GOTO_NAVPOINT, BACKSTAB, the KEEP group (D23400's device). 235 -> 113.
- Where the binary keeps a result block in place (not cross-jumped to a shared copy), the tests are if / else chains
  instead of `{ result = X; break; }`: attack's is_casting test (`if (casting) result = 3; else if (ready) {...}`,
  106), cast's current-goal test (`if (cur) { if (!is_mode) result = 2; } else if (targeted) {...}`), switch-combat
  (`if ((f & 1) && (f & 0x400)) result = 3; else if (!combat) result = 2;` and `else if (casting || sneaking)`),
  hide (`if (!hiding && !ready) result = 3; } else if (speed > 0 || casting)`). 113 -> 68.
Same behaviour throughout (each former return is still the last thing done on its path). Left: the sneak case's
`(flags & CF_COMBAT_READY) ? DONE : SUSPEND`: the binary does `mov edx, [ecx + 0x320]; not dl; movsx ebx, dl; and
ebx, 1; or ebx, 2`, ours `mov ebx, dword; not ebx; and; or` (and the next block then loads the owner into EAX, ours
EDX). Tried: if / else, `!x + 2`, `== 0`, `!x ?`, char / signed char casts on the flags or on the result, char /
bool / int locals, an unsigned / int flags local (also with `(char)~f & 1 | 2` and `((char)f & 1) ? :`), inline helpers returning char / bool / int (all `mov bl, byte` or
the dword form, no movsx). Most of the remaining 68 is the jump table and padding the distance counts.
Also tried (worse): `return result = X` (195, constant-propagated), `result = X; return result;` (195),
`result = X; goto done;` with `done:` before the return (174).

## Still differs (tried; do not repeat)

**D23404 (game/new_inventory.cpp; notes only). inventory_drag_to_equip 0x417220 (31)**: only the default case's
`n = 0` store remains (`xor esi, esi; mov [mode], esi` against `mov [mode], 0`; distance 31 is mostly the jump
table). Tried, beyond D20804's list:
  `mode = n` (79), no `n = 0` (79), a `const int *np = &n` for the loop bound (79), unsigned i with `(int)i < n`
  (31, loses a match), n declared after entry (35), an entry copy for the equip call (31), the mode store through
  a pointer (79), `mode = n; n = 0;` (31), `n = 0;` before the mode store (31), n initialised before the switch
  (33), `n > i` (35), the flag set after the loop (121), `n = 0; mode = n;` (31).
**D23405 (game/characterinfo.cpp; notes only). character_learn_chain_skill 0x449650 (97)**: register choice
(binary: species EDI, c EBX, both also spilled; slot in the frame slot it shares with cost; EBP pushed late for
new_points). Tried, all 97: cost in the
  if block, cost in the block plus points / new_points declared at use, slot declared first, c initialised at
  declaration, species set after c, a one-element slot array, the search loop through a `chain_skill *cs`, `(c =
  ...) != NULL`, new_points by if / else, the tail in an `if (points < 100)` block, one exit with `goto done` for all
  returns, a `slot_i` extra local; c re-fetched in the message loop 129. An inline helper for the AP part did not
  compile into the comparison (not found). Treat it as register choice.
**D23406 (game/unnamed_goals_doors_0x475180.cpp; notes only). matrix3_pack_orientation 0x475380 (20)**: frame only
(a in the dead argument slots). Tried, all 20 or worse: memcpy for a, for b, for both; a `const vector2 *src`
pointer copy; memcpy into out (22). **matrix3_unpack_orientation 0x475430 (73)**: not retried (21d's list is long; the frame / sign multiplies are
  the same problem as the packer's argument slots plus the double-literal shape).

## For the merge / other workers

- D23400 / D23403: the source order of switch cases sets the binary's block order. Earlier "exit block / tail
  placement" near misses (cutscene_script_load, camera_capture, script_action_combat_mode, parse_arr_line, ...)
  are worth retrying with the cases (or if-chains) reordered to the binary's block order.
- D23403: when every return of a switch-heavy function goes through a callee-saved register (`mov ebx, K; pop
  edi; mov eax, ebx`), write the returns as `result = K; break;`; where the binary keeps such a block in place
  rather than jumping to a shared copy, write the test as an if / else chain.
- D23402: a zero store through the zero register vs an immediate 0 tells an int field from a float field.

## Behaviour errors / bug candidates

None found. All changes are same-value reshapings.

## Header changes

None.

## Changed files

- src/game/unnamed_tentacle_beast_0x4c6480.cpp (tentacle_beast_ai_frame D23400, tentacle_beast_frame_hook D23401)
- src/game/unnamed_goals_doors_0x475180.cpp (door_find_floor_point D23402, ai_goal_evaluate D23403)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "unnamed_goals_doors_0x475180|unnamed_tentacle_beast_0x4c6480" --check src/decomp/vc6-matches.txt`:
2 units: 85 match, 0 wrong target, 7 differ; 84 matched there, 85 now, **0 lost**.

Helper scripts (in W, not for the repo): try.mjs (variants with distance and, with CALLS=, our call sequence),
cod.mjs (our listing of one function, compiled into W/out), sdiff.mjs (LCS diff of our listing against asm.mjs),
apply.mjs, slots.sh, jt.cjs (reads a jump table from Sum.exe).

## Worker 14

### m-1021e worker 14 (D23500-D23599)

Batch: pathvis_add_edge_node, determine_point_visibility, do_single_vis_check, add_or_remove_edge
(game/pathfinding_visibility.cpp), gr_init, gr_screenshot (vsdk/gr.cpp), version_check_run
(game/new_int_multi_update.cpp), character_use_skill (game/new_skill.cpp).

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/pathfinding_visibility.cpp | 0x4b7e80 | add_or_remove_edge | D23500 | locals renamed for the unoptimised frame's name hash; the body after the parallel test as an `if (!= 0.0f) { ... }` block; `e->dist = (len = (float)sqrt(...))` |
| game/new_skill.cpp | 0x429410 | character_use_skill | D23502 | pick-lock test as `user->get_skill(SKILL_PICK_LOCK) + 0.05f >= (double)door->lock` |

**D23500 (game/pathfinding_visibility.cpp). add_or_remove_edge 0x4b7e80 is exact. NEW DEVICE: in an
unoptimised function (#pragma optimize off, /Od) the frame slots of the locals follow a hash of their NAMES,
not their declaration order.** Measured with int locals in a /Od test function: 16 buckets; the bucket of a
name is `h & 15` with `h = h * 4 + (h >> 4) + c` over its characters (h starts at 0; checked against
every 1- to 5-character name probed; for 6 or more characters the formula no longer fits and the bucket has
to be probed); bucket 0 gets the highest address; within a bucket the later declaration gets the higher
address. Types do not matter (a profile_label, floats and a pointer all follow it). The old comment
("declared in frame order") was a coincidence of the names. The binary's slots (e -0x10, label -0x14, ray dx
-0x18, edge dx -0x1c, ray dy -0x20, edge dy -0x24, the two projections -0x28 / -0x2c, len -0x30) come out with
the names `prof` (label), `e`, `rdx`, `sdx`, `rdy`, `sdy`, `dotn`, `dotp`, `len`, declared `prof, e; sdx, rdx;
dotn, sdy, rdy; dotp; len` (ties: e / prof bucket 5, rdx / sdx 7, rdy / sdy / dotn 8). Two code changes were
also needed: the early `return` after the parallel test becomes an `if (... != 0.0f) { rest }` block (the
binary's `jne` to the end instead of `mov [ebp-4], -1 / jmp`), and `e->dist = (len = (float)sqrt(...))` with
the parentheses gives the binary's `fst len / fstp e->dist` (without them: fstp len and an integer copy).
Distance 199 -> 0. Same behaviour. Tools for the hash, in W: `hash.mjs` (compiles a /Od function with given
int locals and prints their slots), `bucket.sh` (the bucket of a name against 16 single-letter anchors),
`h.mjs` (the formula). This should apply to objects.cpp (/Od) and the other #pragma optimize("", off)
functions (living_entity.cpp, the cutscene file): a wrong slot order there is fixed by renaming, not by
reordering declarations.

**D23502 (game/new_skill.cpp). character_use_skill 0x429410 is exact.** The only difference left was the
pick-lock compare: the binary loads door->lock and uses `fld [ebx+0x58] / fcompp / test ah, 0x41 / je`.
`user->get_skill(SKILL_PICK_LOCK) + 0.05f >= (double)door->lock` gives exactly that: the cast keeps 8966 from
folding the float member into `fcomp [mem]`. The other forms with a double (cast on the left, cast of the sum,
`(double)get_skill + 0.05f`) add a `sub esp, 8` temporary. Same behaviour (the compare is done on the x87
register either way). Distance 99 -> 0.

## Closer, kept

- game/pathfinding_visibility.cpp 0x4b8230 do_single_vis_check: 41 -> 39 (**D23501**). b (the to point) read
  through `const vector2 *pb = &b;` in the mid sums puts b below a in the frame as in the binary (in /O2 code
  the frame order follows use, not names or declarations: renaming a / b and swapping declarations changed
  nothing; an extra read of one local moves it lower). With it the best of all 120 orders of the five set-up
  statements is `layer, a, seg.b, b, seg.a`. Left: the scheduling of the two copies (the binary loads
  from->pos before the layer and computes &b first).
- vsdk/gr.cpp 0x505e00 gr_init: 102 -> 54 (**D23503**). Case 201: the exstyle as an if / else of two
  SetWindowLongA calls (8966 cross-jumps them to the binary's `cmp edi, 0x67 / jne / push 0x20 / jmp / push
  ebx`; the ternary gives setne / dec / and or neg / sbb), and the aspect through two float locals
  `fw = (float)gr_screen.max_w; fh = (float)gr_screen.max_h; aspect = fw / fh;` (fild / fild / fdivp; the
  plain division gives fild / fidiv). Same behaviour.

## Still differs (tried; do not repeat)

- add_or_remove_edge: done (D23500).
- pathvis_add_edge_node 0x4b7d30 (22): exhausted in earlier rounds; not retried.
- do_single_vis_check (39, D23501): renames of a / b (swap, p / q, from_pt / to_pt, p0 / p1: no effect in /O2),
  per-field copy of b, a block pointer copy of to->pos, an extra `seg.b = &b` after mid, pointer locals for a
  only, `seg.a->x` / `seg.b->x` in the sums (seg.b also moves b lower, same 36 in my count), 120 statement orders
  with pb.
- determine_point_visibility 0x4b8350 (31): the real differences are start_ms and t2 in swapped frame slots
  (binary: start_ms at base+0x24, t2 at +0x28) and the cleanup loop's head (D21501). Tried for the slots: t2
  replaced by t (one variable), t2 declared in the while block, start_ms declared first (before num_pending),
  both, names swapped. None moves them (8966 orders by use; t2 is used in the loop).
- version_check_run 0x410f40 (32): the SkipVerify block's `pop edi` is scheduled after `mov esi, 1`, so it is
  not cross-jumped with cases 1 / 2. Tried: a store through `int *d = &version_check_done`, a bool local for the
  skip test, `return version_check_done` in the skip block, chained stores with that return, `return
  version_check_done` in cases 1 / 2. No change.
- gr_init (54, D23503): store orders of the gr_screen fields (window_mode first, the binary's store order,
  clip pairs together, window_mode last, clip_left / right / top / bottom before the sizes, 10 orders in all:
  66-116 in my count, none better than the current source order by much; one at 66 is the clip fields before
  the sizes, not kept), declarations of x / y / ww / wh at function scope or block top, rect / work at function
  scope, `work.top` passed directly instead of y (worse), renamed parameters (no effect on the argument-slot
  choice), the aspect division as `/=`, as double, with one cast.
- gr_screenshot 0x5061f0 (204): looked only. The differences start at the pitch computation (the binary keeps
  the width in EBX and computes the bpp pitches in a register; ours stores the pitch per case) and run through
  the row loops. Not attempted.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/game/pathfinding_visibility.cpp (add_or_remove_edge D23500, do_single_vis_check D23501)
- src/game/new_skill.cpp (character_use_skill D23502)
- src/vsdk/gr.cpp (gr_init D23503)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "game/pathfinding_visibility\.cpp|game/new_skill\.cpp|vsdk/gr\.cpp" --check src/decomp/vc6-matches.txt`:
3 units: 103 match, 0 wrong target, 8 differ, 0 not found, 0 errors; 101 matched there, 103 now, 0 lost.

## Tools (in W, not for the repository unless wanted)

- `cod.mjs <unit> <mangled-substring> <0xaddr> [all]`: compiles into W/out (no temp folders), aligns our
  instructions with the original's by an LCS of masked bytes and prints the differing lines; it stops at the
  EH funclet, so unlike vc6-compare --score it is not thrown off by unoptimised functions with an EH frame
  (vc6-compare reported add_or_remove_edge as "differs ... at +0x30: jmp ___CxxFrameHandler" with distance 132
  when only frame slots differed). `VARIANT=<json>:<name>` compiles a vc6-try variant from a copy.
- `hash.mjs`, `bucket.sh`, `h.mjs`: the /Od slot hash (D23500).

## Worker 15

### m-1021e worker 15 (D23600 to D23699)

Batch: 8 functions (gametarget x4, new_options x2, new_pickup x1, vfx_mesh x1). Result: 5 new matches, 1 closer
(kept), 2 unchanged. Helper scripts in W/try (v.mjs: distance per variant and the side listing per variant;
cmp.mjs + d.sh: a copy of vc6-compare whose VC6_DIFF=<addr> prints the LCS instruction diff of one function with
the instruction text, which shows where the distance comes from).

## New matches

| file | address | function | D | what did it |
|---|---|---|---|---|
| game/gametarget.cpp | 0x43a460 | character_can_talk_to | D23601 | `Character *cc = cur_character;`, both sums written `pos + centre_offset`, and `double dx = d.x; double dz = d.z; dist = (float)sqrt(dz * dz + dx * dx);` |
| game/new_options.cpp | 0x41ef30 | options_load_bitmaps | D23602 | `ui_button_def *d` declared before `ui_button_state *b` in the loop body |
| game/new_options.cpp | 0x41fff0 | options_drag_sliders | D23603 | the divisor written as the conversion again (`x / (float)size`), and `float w` declared in each case block |
| game/gametarget.cpp | 0x43b5e0 | world_mouse_hover_target | D23604 | case -1 as `if (picked) { ... } else if (door) { ... }`; case 7 with a `bool sel`; the 0x100 test around the trade path; case 0 with one exit |
| game/gametarget.cpp | 0x43c060 | object_try_unlock_or_open | D23605 | the door_toggle_state success tail last (`if (!door_toggle_state(..)) { messages } host tail`); the "Cannot be opened from here" (client) and "Cannot be closed" / "Cannot be opened" (server) messages join the shared `sound_play_2d(5)` |

**D23601 (game/gametarget.cpp). character_can_talk_to 0x43a460 is exact.** Two parts. (1) The binary loads
centre_offset first in both sums (`fld [eax+0x28]; fadd [eax+0x10]`). 8966 sorts the operands of the inline
operator+ per component, and the order depends on the form: with `cur_character->pos + cur_character->centre_offset`
it loaded pos first for x but centre_offset first for y / z. A `Character *cc = cur_character;` local and both
sums written `X->pos + X->centre_offset` give centre_offset first for all six (distance 26 -> 10). Tried for this:
both sums centre_offset + pos (30), pointer locals for centre_offset or pos (18 / 26 / 30 / 14), `a = pos; a +=
centre_offset` (55). (2) The length: the binary reloads d.z and squares it with `fld st(0); fmul st, st(1)` and
keeps d.x on the stack (`fld st(2); fmul st, st(3)`), the z term first. `double dx = d.x; double dz = d.z;` (dx
declared first) and `sqrt(dz * dz + dx * dx)` give exactly that. Tried: z term first with float d.x (9), a float
dz (9), `float zz[1]` (9), `const float *pz` (10), a double dz alone (13), double dz then dx (36), long double (13).
Same behaviour (the products were on the 64-bit FPU stack in both).

**D23602 (game/new_options.cpp). options_load_bitmaps 0x41ef30 is exact.** The loop's increment reloads i into
EDI in the binary (ours ESI). With `ui_button_def *d = &options_button_defs[i];` declared before
`ui_button_state *b = &options_buttons[i];` 8966 picks EDI. Tried this round: d as a running pointer (124), b and
d both running (134), `(unsigned)i < N` (122), a pointer to i (137), a switch on an int copy of i (120).

**D23603 (game/new_options.cpp). options_drag_sliders 0x41fff0 is exact.** The "width copied once more" of
earlier notes is the conversion `(float)size` spilled to a temporary and copied into w: the clamp uses w and the
division the temporary. Written `w = (float)size; if (x >= w) x = w; v = (int)((x / (float)size + 0.05f) ...`
case 1 is exact (8966 CSEs the second conversion). Cases 2 / 19 / 70 have the temporary and w in the other two
slots; `float w` declared inside each case block (`case 1: { ... }`) sets that per case. Tried: the compare on
the conversion (216), all of x / w / v in blocks (158), w and v in blocks (158), declaration orders of x / w / v
(170). Same behaviour.

**D23604 (game/gametarget.cpp). world_mouse_hover_target 0x43b5e0 is exact.** (1) The door case (picked NULL)
is placed after the type switch in the binary (0x43b833): `if (picked) { world_hover_object = picked; switch ...
} else if (door) { ... }` (184 -> 145). (2) Case 7: `bool sel = true; if (MP) { p = ...; if (!p || p->mp_owner
!= mp_local_player) sel = false; } if (sel) { tint; return SELECT; }` then the trade path written `if
(!(target->flags & 0x100)) { if (!(len_sq <= 25)) return NONE; tint; return TRADE; } return NONE;` (145 -> 108).
Forms of (2) that did not move the select block: `!MP || (p && owner == local)` as the if, a goto to a trade
label, `do { } while (0)` with break, `!(p && owner == local)`, an if / else, two select copies (147 / 135, not
merged). (3) Case 0 with one exit: the two AT_TARGET returns as one if / else and one `return` after it (a goto to
one return also matches); 8966 then pushes EBX inside case 0 (0x43b87c) instead of in the prologue (D18704). Same
behaviour.

**D23605 (game/gametarget.cpp). object_try_unlock_or_open 0x43c060 is exact.** The binary keeps separate
`add esp, 4` / `add esp, 8` around hud_show_message / sound_play_2d for "Cannot be opened from here", "Cannot be
closed" and "Cannot be opened": in the source they join the shared `sound_play_2d(5, 1.0f); return;` (8966
duplicates that return tail into each, but the stack adjustment stays in the block before the join). The server
part as `if (d->key) { 624 } else if (lobj_door_is_open) { 629 } else { 630 }` (160 -> 72), the door_toggle_state
success (host) tail last as `if (!door_toggle_state(l, 0)) { ...messages...; return; } if (MP_IS_HOST()) ...`
(72 -> 3), and the client's 625 message joining the client's shared sound (3 -> match). A `goto toggled` form
did not move the tail (72). Same behaviour.

## Closer, kept

| file | address | function | D | old -> new distance |
|---|---|---|---|---|
| vsdk/unnamed_vfx_mesh_0x562a30.cpp | 0x563a40 | fx_rec98::update_frame | D23600 | 103 -> 22 |

**D23600 (vsdk/unnamed_vfx_mesh_0x562a30.cpp). fx_rec98::update_frame 0x563a40, closer (103 -> 22).** Kept, all
the same behaviour:
- `float frame = (float)(desc->mesh_flags >> 6); frame *= tsec - desc->start_time;` gives the binary's `fild rate;
  fld tsec; fsub start; fmulp` (the int rate times the difference loads the difference first in every other
  form: `(float)rate * d`, a float rate local (with or without a pointer), a difference local, `rate * (...)`).
- the skinned ticks as `float tf = tsec * 30.0f; int ticks = (int)(tf * 160.0f);` (and the same for start): the
  binary multiplies by 30 and 160 (0x581adc / 0x581d5c); written in one expression VC6 folds them into 4800.
- `num_pos_keys > 0` / `num_rot_keys > 0` / `num_scale_keys > 0` (the binary's `jbe`; `!= 0` gives `je`).
- `float u = 1.0f - frac;` declared inside the morph loop and inside the uv-anim face loop: 8966 hoists it after
  the loop guard as the binary does.
- the rotation track: `quaternion tq = desc->track->get_rotation(...); tq.to_matrix_b(&rot);` (a named result;
  the temporary form pushes &rot before the call).
- the billboard width / height: `float a0 = get_a(f0) * u; width = a0 + get_a(f1) * frac;` (and b0) for the
  binary's f0-product-first order (writing the source terms in either order changes nothing).
Still differs (22): the billboard u is in tsec's slot (esp+0x14) in the binary and a0 / b0 in frame's (0x10),
ours the reverse; the uv-anim loop's product temporary is at esp+0x40 in the binary (ours 0x44). Tried: a0 / b0
declared first, one reused temporary, `float u[1]`, blocks around a0 / b0, a pointer to u (70), separate a1 / b1
locals (70), u at function scope (for the billboard only, billboard + morph, all four: 22 / 22 / 72 / 44),
declaration orders of tsec / frame / frac / i / j (all 22); for the uv loop: product locals (38), source order
swapped, factor order swapped, dst / src pointer locals (42 / 200), a frac pointer (36).

## Still differs (tried; do not repeat)

**D23606 (notes only). pickup_draw_item_list 0x421370 (32; x EBP / i EBX, the binary the reverse).** Tried this
round (all 32): `!= false` on the selected flag, `y = fh + y`, i declared before the for, x / y declaration
order swapped, `get_item(i + 0)`, `*(pickup_item_selected + i)`, `pickup_hover_item == i`, an int copy of x in
the loop body. With D19105 / D20907's lists: register choice.

**D23607 (notes only). world_pick_under_mouse 0x43b260 (215).** The binary calls plane::set_normal_point at both
sites, the vector3(x, y, z) constructor for `*dir * 10`, `+ pos` and `-*dir`, vector3::operator= for the plane
copy, and the copy constructor for the dead results of `ray.origin = *eye`, `ray.dir = *dir` and `wanted_dir =
*out`; but it expands `*out * speed` and `*out -= pos`. Found: a file-local `static inline void wpum_plane(plane
*, const vector3 *, const vector3 *) { pl->set_normal_point(n, p); }` used at both sites makes 8966 call
set_normal_point out of line (215 -> 177), so a member called from a file-local inline function is not expanded;
`#pragma inline_depth(1..3)` around the function changes nothing (0 gives 211 with everything called). Not kept:
the rest of the pattern (the copy constructor after an expanded operator=) has no source form found, and the
wrapper alone is not a match. A next try could wrap the other sites the same way.

## Behaviour errors / bug candidates

None found. All changes keep the behaviour.

## Header changes

None.

## Changed files

- src/vsdk/unnamed_vfx_mesh_0x562a30.cpp: fx_rec98::update_frame (D23600, closer).
- src/game/gametarget.cpp: character_can_talk_to (D23601), world_mouse_hover_target (D23604),
  object_try_unlock_or_open (D23605); confidence exact in their comments.
- src/game/new_options.cpp: options_load_bitmaps (D23602), options_drag_sliders (D23603); confidence exact.

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root W/src --only "gametarget|new_options|unnamed_vfx_mesh_0x562a30"
--check src/decomp/vc6-matches.txt`: 3 units: 60 match, 0 wrong target, 6 differ, 0 not found, 0 errors; 55
matched there, 60 now, 0 lost.

## For other workers

pick_test_character 0x43c620 (gametarget.cpp, distance 2, not in this batch) has a sum of squares too; D23601's
`double` locals for the squared terms (dx declared first, the term order of the binary) may be worth a try.

## Worker 16

### W16 notes (m-1021e, decisions D23700-D23706)

## New matches

None.

## Closer, kept

| file | address | function | old -> new | decisions |
|---|---|---|---|---|
| vsdk/gr_direct3d.cpp | 0x5395c0 | gr_d3d_init_device | 43 -> 20 | D23700, D23701, D23702 |
| vsdk/gr_direct3d.cpp | 0x536360 | gr_d3d_tmapper_multipass | 129 -> 117 | D23705 |
| game/unnamed_titus_0x4c20e0.cpp | 0x4c2790 | ai_state::titus_ai_select_action | 106 -> 34 | D23703, D23704 |

**D23700 (vsdk/gr_direct3d.cpp, gr_d3d_init_device).** The ALPHAREF render state as an if / else with two
calls (`if (d3d_use_vertex_buffer) set(ALPHAREF, 0x18); else set(ALPHAREF, 0);`): 8966 merges the common tail
and gives the binary's `test al, al / je / push 0x18 / jmp / push ebx / push 0x18 / call` instead of the ternary's
`neg / sbb / and`. 43 -> 32.

**D23701 (vsdk/gr_direct3d.cpp, gr_d3d_init_device).** The square-only flag assigned before the pow2 flag: 8966
then computes pow2 (bit 1) first, as the binary. 32 -> 30. The binary's two separate `and reg, 0xff` copies of
dwTextureCaps (`mov esi, eax / and eax, 0xff / shr eax / and eax, 1 / and esi, 0xff ... shr esi, 5`) are still not
reproduced (see Still differs).

**D23702 (vsdk/gr_direct3d.cpp, gr_d3d_init_device).** `missing = 0;` before the strcpy of the "missing
features" message: the binary's `xor ebx, ebx / xor edx, edx / lea ebp, msg` register use around the strcpy. 30 -> 20.
What is left: the two `& 0xff` copies (11 bytes) and the gr_screen.max_bitmap_w store placement next to them.

**D23703 (game/unnamed_titus_0x4c20e0.cpp, titus_ai_select_action).** The retarget test of each stage through a
function-scope `bool fresh` (`fresh = timer_up || !target; if (fresh) { pick 0 } else if (retarget) { pick
0x80000000 }`; stage 2 `fresh = !target || timer_up == true`): 8966 then lays out the pick-0 block first and the
retarget block after it, as the binary (with the condition in the if, 8966 puts the else-if block first in all
three stages). 106 -> 74 (stage 1 -> 88, stage 2 -> 74, stage 3 -> 76 alone; together with D23704 34). The
stage-2 move 12 / 13 choice through the same local (`fresh = index == -1 || index == 0; if (fresh)`) puts the
move-12 store first (36 -> 34). This is the D17200 device ("a range test through a bool local") applied to an
`||` test with an else-if.

**D23704 (game/unnamed_titus_0x4c20e0.cpp, titus_ai_select_action).** In stage 2's timer block, `if (!valid13)
SetTimeout(20000); else goto move12_13;` instead of `if (valid13) goto move12_13; SetTimeout(20000);`: the move-10
path stays inline and move12_13 goes after it, as the binary. 62 -> 36.

**D23705 (vsdk/gr_direct3d.cpp, gr_d3d_tmapper_multipass).** In the sectioned pass, one walking local
`gr_multipass_vertex **vv = verts;` passed to gr_poly_get_section and walked by the uv loop (`j++, vv++`,
`(*vv)->pass[i].u`): the binary loads verts into EDI before the call and walks it in the loop. 129 -> 117.
The same walking local in the other two uv loops is worse (119, or 267 when all three are walked: the frame breaks).

## Still differs (tried; do not repeat)

**D23706 (notes only).**

- gr_d3d_init_device 0x5395c0 (20 now): the texture-caps `& 0xff` copies. Tried (on top of D16204's list):
  `((BYTE)caps >> n) & 1`, `((BYTE)caps & bit) ? 1 : 0`, `!= 0`, `((BYTE)caps & bit) >> n`, `(int)(BYTE)(caps >>
  n) & 1`, `(unsigned char)` casts, `LOBYTE(caps)` in shift / ternary / `!= 0` forms (all of these narrow the load to
  a byte: `xor edx, edx / mov dl, [..]`), `((caps & 0xff) >> n) & 1` (folded), a DWORD local with BYTE casts, two
  DWORD locals (one copy), a BYTE local (block, function scope, register; goes to the frame), an int local of the
  BYTE, `(BYTE)(caps & bit)` ternary / shift, `(bool)` cast, `/ bit & 1`, a desc pointer local, `*(BYTE *)&caps`,
  `(char)` / `(WORD)` / `| 0` variants to break the CSE, a `const DWORD *` local, both statements after the dwSize
  stores, max_bitmap_w before the dwSize stores, the dwSize chain reversed. None gives two masks from one dword load.
- d3d_compute_pixel_format_shifts 0x53a370 (81): register choice (binary: desc in EBX, shifts in ECX, r_bits in
  desc's dead argument slot, g / b masks in EDI / ESI). Tried: `register` shifts, a `register` desc copy first, r_bits
  declared after the r_shift loop, the shift loop as `for (r_shift = 0; !(m & 1); r_shift++) m >>= 1`, the alpha
  flag test through m / a DWORD local / a BYTE local. All give the same code (81).
- cutscene_script_load 0x4ed0f0 (32): the shared failure tail (binary: after the level-name test, success falls
  into the epilogue; 8966: at the end). Tried (beyond D11204 / the 10-21d list): `else { ... }` around the success
  part, `return 0`, a bool `ok` tested with `return ok`, `goto fail` with the label inside the stricmp if (locals
  hoisted), `goto ok` with `fail: return false; ok:`, nested positive tests with `goto ok`, `if (!_stricmp) {...}
  else return false; return true;`, the parse loops as `for (;;)`. All keep the tail at the end.
- gr_d3d_tmapper_multipass 0x536360 (117): the binary addresses `[i*12 + *vp + 4]` / `[.. + 8]` with two base+index
  loads; 8966 adds once and uses [ecx+4] / [ecx+8] in every uv loop, and the pass->mode load is not hoisted above the
  sectioned test. Tried: a vv local without walking (129), walking in loop 2 or 3 or all (119 / 117 / 267),
  `gr::Mode m = pass->mode` before the sectioned test (129).
- titus_ai_select_action 0x4c2790 (34): left: the move-11 branch loads titus_flags before the move store (tried
  flags first, an unsigned local, a pointer local: no change), and the stage-2 `valid10 == true` / `else if
  (!valid10)` blocks (binary: the else-if block falls into the move-10 path; tried a `goto move10` from the then
  block, `else { if (!valid10) ... }`, `valid10 != true` first (63), `== false`, bool locals for the inner tests,
  the move-12 / 13 code nested in the else-if (62); none moved it). Also worse: the goto-into-tail and merged-tail
  forms of the stage-1 retarget (120), `timer_up == true || target == NULL` (111).
- ai_state::pick_attack_target 0x4b5900 (122): not changed. The binary holds -1 in EBP (`or ebp, -1`) and has a
  single exit; earlier rounds' forms cover it (D-notes 10-20b / 10-21d). Not retried.
- ai_choose_attack_slot 0x4b6a90 (142): not retried (D9600 / D9601 / D9810 / D17005: needs the out-of-line vector3
  constructor no source form gives).
- gr_d3d_bitmap 0x5356d0 (228): looked at only; x87 scheduling of the vertex setup and the section loop's register
  roles (bm / x in EBX / EBP loaded in another order). Not attempted further.

## Behaviour errors / bug candidates

None found. All kept changes keep behaviour (D23700 the same render state values; D23701 two independent stores
swapped; D23702 a local cleared earlier; D23703 / D23704 the same tests and branches).

## Header changes

None.

## Changed files

- src/vsdk/gr_direct3d.cpp (gr_d3d_init_device: D23700, D23701, D23702; gr_d3d_tmapper_multipass: D23705)
- src/game/unnamed_titus_0x4c20e0.cpp (titus_ai_select_action: D23703, D23704; a `bool fresh;` local added)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root W/src --only "gr_direct3d|unnamed_titus_0x4c20e0" --check
src/decomp/vc6-matches.txt`: 2 units, 71 match, 0 wrong target, 6 differ, 0 errors; 71 matched there, 71 now,
0 lost.

## Worker 17

### W17 notes (m-1021e, decisions D23800-D23899)

## New matches

| file | address | function | D | what did it |
|---|---|---|---|---|
| vsdk/window.cpp | 0x529d70 | ui_window::parse_radio_button | D23800 | gx / gy as one expression each, label_x declared before label_y |
| vsdk/window.cpp | 0x52a180 | ui_window::parse_slider | D23801 | `scroll = NULL` in the else branch, parent_x declared first |

**D23800 (vsdk/window.cpp). ui_window::parse_radio_button 0x529d70 is exact** with
`int gx = tf->ParseInt() - ((ui_gadget *)parent)->get_screen_x(0);` (and the same for gy). The px / py temporaries
were not needed: VC6 calls get_screen_x / _y before the ParseInt of the right-hand form, which is the binary's
order. Without the temporaries the parent pointer goes to EDI and the zero stays in EBP for the whole function
(61 -> 10). Declaring `int label_x = 0;` before `int label_y = 0;` then swaps their two frame slots (10 -> 0).
Same behaviour (same call order). Confidence -> exact.

**D23801 (vsdk/window.cpp). ui_window::parse_slider 0x52a180 is exact** with
`ui_slider *scroll; if (scrollbar) { s = scroll = new ui_slider; } else { scroll = NULL; s = new ui_slider_base; }`
(the NULL store in the else branch, not at the declaration): scrollbar then lives in BL, the frame is 0x304 as in the
binary, and the EH state stores use immediates (183 -> 55). `int parent_x = 0;` declared first (before
marker_bitmap) swaps ESI / EDI for the zero and the parent pointer (55 -> 0). Same behaviour. Confidence -> exact.

## Closer, kept

- game/levelrender.cpp 0x48f8c0 levelrender_face_vertex_colors: 174 -> 65 (D23802).

**D23802 (game/levelrender.cpp; closer, kept). levelrender_face_vertex_colors 0x48f8c0.** The view-space position
read through a pointer local, `vector3 *p = &verts[idx[i]]; gv->x = p->x; ...`, makes 8966 base the loop's induction
on lightmap_uv (kept in the face argument slot) with the idx and face_vert_list offsets in two frame temps, as the
binary does, and gives frame 0x20 (174 -> 88). `int v3 = v * 3;` at the top of the lit branch without a lightmap,
used for the three vertex_rgb reads, gives the binary's spilled index slot (88 -> 65). Left: register choices only:
the tint sum loads rgb[2] / rgb[0] into EDX and rgb[1] into ECX in the binary (ECX / EDX in ours); the lightmap
branch loads colors into ECX before the lea (ours lea EDX first); in the clamped branch the binary keeps the byte in
EBX and c in ECX (ours: c in EBX for the red component).

## Still differs (tried; do not repeat)

**D23803 (notes only). Tried without a match:**

- vsdk/targautils.cpp 0x554ea0 tga_rle_encode_row (33, unchanged). The only real difference: 8966 keeps dst_bpp in
  ECX at the loop head (loaded at entry, reloaded at the back edge, the entry jumps over the reload), so the binary's
  `mov eax, dst_bpp` / `mov ecx, row_bytes` / `lea edx, [edi + ecx]` prologue and its ECX for dup in the 0x81 block
  are lost. Tried: an address-taken dst_bpp (`int *pb = &dst_bpp;` unused, used for min_run, const), `*&dst_bpp` for
  bytes_equal, an `int bpp = dst_bpp` local for every loop use, `register` on count / dup / min_run, min_run assigned
  after dup (59), end as `row_bytes + src`, end before min_run (59), a `volatile` dst_bpp parameter (55). All others
  33.
- vsdk/window.cpp 0x52b100 ui_window::init (69, unchanged): `this` in ESI and both registers pushed at entry in the
  binary. Tried also: `register` win, a local for sentinel.prev in the modal branch, a sentinel pointer local, win
  declared at the top, a `self` copy for the modal stores, the modal test through `this->flags` or a flags local,
  `this->next = win` last, the link as `prev = win->prev; win->prev->next = this; ...` (75), a `me` copy for the
  flags store. All 69.
- vsdk/window.cpp 0x52abf0 ui_window::handle_arrow_key (83, unchanged; about 80 of it is the jump table): left case
  `int t = w - g->w - g->x; d = t + x;` / `d = x + t`, `t -= g->x` split, an x or w local before, a `this` copy (85),
  a `const int *` to g->w, `d = t; d += x;`, a comma form, `(w - g->w - g->x) + x` (85); up case `(h + y) - (g->h +
  g->y)`, a gadget pointer copy, a bottom / top local (85), `-(...) + (...)`, `d -= ...` splits (85), `y - g->h - g->y
  + h`; d declared in each loop, at function top, `register`. Register naming only (g->h / g->w go to EBP in the
  binary).
- game/unnamed_editor_dialogs_0x430bc0.cpp 0x430c80 editor_dialog_show (137, unchanged; most of it is the EH
  funclets and the jump table). Real differences: frame slots (num_lines at 0x18, y with by at 0x14, the window's
  $T at 0x10, the title static's $T at 0x14 in the binary), text_h loaded before the `if (title)` test for h, and s
  held in EDI with the zero for the title test (`xor edi, edi; cmp eax, edi`), no spill of s. Tried: h as
  `text_h + title_h + 65`, `text_h + 65 + title_h`, a ternary, `h = text_h; h += ...`, a text_h local, `text_h + (title
  ? ...)` (207); s = NULL in an else (135), `title != NULL`, s initialised at the top (417) / before d's stores (415)
  / before the window (417) / after init, a temporary for the new, a separate title pointer (block or function
  scope), a separate text pointer, `d->title = NULL` first and the store inside the if (116, but the store moves
  ahead of the branch), eight declaration orders of the int locals and s first. Not kept.
- game/levelrender.cpp 0x48de60 levelrender_wave_lobj (135, unchanged). Frame slots differ: the binary has t at
  0x64, n0 0x68, n1 0x74, n2 0x80, a spill at 0x10 and p / e1 at 0x14; ours n1 0x64, t 0x70, n2 0x74, n0 0x80, the
  spill at 0x1c and p / e1 at 0x10. Tried: six declaration orders of the function locals (n2 n1 n0, t last, n first,
  separate declarations), renaming the locals (no effect at /O2), n0..n2 declared in the loop (365) or at the loop
  top, face declared in the loop. All 135 otherwise.
- levelrender_face_vertex_colors (on top of D23802): ten declaration orders of the locals; for the clamped branch a
  two-step c, an int byte local, the ftol first, `(int)` on the byte, a negated sum, separate c locals, a ternary
  store (140), `int *cp = &c`, `if (c < 255) ... else` (74), a colors pointer (77 / 65), unsigned v3, v3 at
  function scope; for the lightmap branch a colors pointer, a colors copy, a v copy; for the tint sum a float local,
  three int locals, two int locals. All 65.

## Behaviour errors / bug candidates

None found. All changes keep the behaviour.

## Header changes

None.

## Changed files

- src/vsdk/window.cpp (parse_radio_button D23800, parse_slider D23801; both comments now say exact)
- src/game/levelrender.cpp (levelrender_face_vertex_colors D23802, comment updated)

## --check

```
VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "vsdk/window|game/levelrender" --check src/decomp/vc6-matches.txt
2 units: 45 match, 0 wrong target, 7 differ, 0 not found, 0 errors
check against src/decomp/vc6-matches.txt: 43 matched there, 45 now, 0 lost
```

## Tools left in W

try.mjs / gen.mjs / apply.js (variant runner with --score distances), vcd.mjs + diff.sh (a copy of vc6-compare that
prints the LCS diff of one function; set VC6_DIFF=<addr>), cod.sh (keeps the compile folder under W/tmp, never in
the system Temp folder). W/tmp holds kept compile folders (inside W only).

## Worker 18

### m-1021e worker 18 (decisions D23900-D23903)

Helper scripts are in W/try (tryd.mjs: distance per variant; gen2.mjs / one.mjs: whole-function variants;
sidev.sh / calls.sh: side-by-side and our call sequence per variant; search.mjs: coordinate descent over
unused-local counts at marked points; cod.sh: our listing, compiled under W/tmp).

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/conversationctl.cpp | 0x4da890 | conversation_controller::camera_begin | D23902 | `speaker_dir = matrix43(character->orient).transform(vector3(0, 0, 1))` assigned directly (no matrix43 / vector3 locals), `speaker_flags` stored before `this->name`, and the two bit-field clears (shot_cut, sliding) after the three int zeroes (approach, cam_mode, facing) |

## Closer, kept

| file | address | name | old -> new | D |
|---|---|---|---|---|
| game/conversationctl.cpp | 0x4d9ad0 | conversation_controller::camera_blend | 172 -> 8 | D23900 |
| vsdk/unnamed_d3d_tcache_0x54bb30.cpp | 0x54bca0 | gr_d3d_create_texture_section | 141 -> 15 | D23903 |
| game/conversationctl.cpp | 0x4d9d90 | conversation_controller::camera_update | 235 -> 219 | D23901 |

**D23900 (game/conversationctl.cpp; closer). camera_blend 0x4d9ad0: 172 -> 8.** Written with the vector
operators: `axis_a.dot(axis_b)` for the flip test, `vector3 axis = axis_a * t + axis_b * (1.0f - t);`, and
then `vector3 unused0, unused1, unused2;` (three unused locals) right after the axis. Their empty inline
default constructors spend VC6's inline budget, and with exactly three the binary's out-of-line calls appear:
the flip's operator= result copy (0x410cd0), the axis's vector3(x, y, z) constructor (0x43b580, which is why
the binary sums z, y, x: the arguments are pushed right to left), the transform result copy and the direction
assignment's result copy. The instructions are the binary's, so values and x87 rounding are unchanged (z sums
the two unrounded products, x and y the rounded ones, as D9807 found). Observations on the budget, useful
elsewhere: the dummies only count when placed after the axis statement (at the function's top, 1-6 of them
change nothing); 1 / 2 / 4 / 5 give 80 / 71 / 151 / 170; three `axis_a[0];` statements at the end work the
same way (8). `#pragma inline_depth(1..3)` changes nothing, (0) is far worse. Left (8): the eased t's
`fstp` is scheduled before `push edi` (binary: after `mov edi, a`). Tried for that, all 8 or worse: the
ease as a ternary, through `float *tp`, an `s` local (122), a `t2` local (24), the arms swapped (16), a
do / while (16), braces, `4.0f * t` forms, a `pa = a` copy (before / after the ease), `const vector3 *ad =
&a->dir` and a reference (both change the budget: 119, and 8 again only with three dummies), declaration
orders of m / mb / axes / angles, `register`. Note for the merge: the three unused locals are a device;
there is no evidence they are in the original (they could equally be other inline uses after the axis).

**D23901 (game/conversationctl.cpp; closer). camera_update 0x4d9d90: 235 -> 219.** The no-zoom condition
through `bool still = ...; if (still) { ... } else { ... }` (the binary places the no-zoom block first and
falls into it), and its mode test written `if (cam_mode != 1 && cam_mode != 0 && cam_mode == 3)`, which
gives the binary's three compares in line (the empty-case switch put the code at the end, 8966 dropping the
two empty cases). Same behaviour. Left: the vector copies. The binary expands `pos` assignments and calls
vector3::operator= (0x413190) for the first three `dir` copies, the copy constructor for the later ones'
results. A lead, not kept: with dialog_camera_pose::operator= written as `pos = p.pos; dir = p.dir; return
*this;` in levelscripts/conversation.h (or removed, implicit; 0x4d9aa0 still matches either way, and the
unit keeps its matches) and the source using `out = game;`, `out = shot;`, `blend = shot;`, `out = blend;`,
the 'ifs' form scores 136, and with nine unused vector3 locals at three points 49 (coordinate descent,
W/try/search.mjs). Not kept: it needs a header change and nine dummies.

**D23902 (game/conversationctl.cpp; match).** camera_begin 0x4da890, see the table. Confidence -> exact.

**D23903 (vsdk/unnamed_d3d_tcache_0x54bb30.cpp; closer). gr_d3d_create_texture_section 0x54bca0: 141 ->
15.** `tex_w = e->sections[section].w; tex_h = ...h; w = tex_w; h = tex_h;` (113 / 31 for the other orders),
the scales from float locals (`float fw = (float)w, ftw = (float)tex_w; u_scale = fw / ftw;`: the binary
converts both with fild and divides with fdivp, no fidiv), `ddsd.ddsCaps.dwCaps = ...` before `size =
tex_h * tex_w * 2`, and the section written through `e->sections[section].x` at the end with no pointer
local (the unused `s` removed). Same values (fild of an int is exact either way). Left (15): w and h have
each other's frame slots (binary: h at esp+0x14 stored first, shared with level; w at 0x1c, shared with
size). Tried, all 15: declaration orders (5), `int h` / `int w` declared at their use (3), h through a
pointer, h read before w (4 orders; reading h before tex_w is 55-57), mixed `h = sections.h; tex_h = h`
(97-109), renaming w / h (14 name pairs: slot order is not by name), `level = NULL` before v_scale, level
through a pointer, the float locals one per declaration, v_scale first (33).

## Still differs (tried; do not repeat)

- **vsdk/gr_opengl.cpp 0x54fa30 gr_opengl_bitmap (16)**: exhausted by earlier rounds (see 21b / 21d); not
  retried.
- **vsdk/gr_opengl.cpp 0x5502c0 gr_opengl_read_backbuffer (56)**: index forms `buf + i * w * 4` / `(h - i -
  1) * w * 4` (60), separate i / k indices (77), `(4 * h - 4) * w` (75), counting down (56), inner pointers
  `top_row + j * 4` (66), byte index `j += 4` (90), top / bottom declared at function scope (56), `int n =
  w` counter (56), `bottom_row = bottom - 8w` (85), a `-w * 4` step local (76), a `rows[2]` array (68), buf
  itself stepped as the top row (64). The binary keeps two IVs biased +2 (top from the buf slot each row,
  bottom in ECX across rows) and a separate inner counter; 8966 makes one IV plus a difference register.
- **vsdk/gr_opengl.cpp 0x54efe0 gr_opengl_set_mode (111)**: the first 76 instructions match; then 8966
  caches glDisable in ESI after the texture switch (the binary never caches it) and swaps EBX / EBP (glEnable
  / alpha_blend). Tried: fog's disable shared by fall-through (112), `register` alpha_blend, declarations
  first in three orders, glEnable through a function-pointer local (all 111). The colour switch as `case 0:
  ... case 1: ... case 2:` (separate case 0 and 2) gives 103 (not kept: the binary shares the 0 / 2 block);
  the alpha switch orders (a12 first, 3/0 then 2/1, 0 / 1+2 / 3) change nothing on top.
- **game/unnamed_ghost_rider_0x4b9e70.cpp 0x4bb610 ghost_anim_remap_in (36)**: same residue as tiger /
  phoenix / serpent (loop registers rotated). New forms, all 36: kind / weight base pointer locals, a weight
  pointer only, the anim value in a local, reversed declaration order, the count through a local, the weight
  through a float temporary, `*(ghost_remap_kind + i)` addressing.
- **camera_blend (8), camera_update (219), gr_d3d_create_texture_section (15)**: see D23900, D23901, D23903.

## Behaviour errors / bug candidates

None found. All changes keep the computed values (camera_blend's instructions are the binary's apart from
one store's position; create_texture_section's float conversions are exact).

## Header changes

None (the dialog_camera_pose::operator= lead in D23901 was tested in W only and reverted; the header in W is
identical to the repository's).

## Changed files

- src/game/conversationctl.cpp (camera_blend D23900, camera_update D23901, camera_begin D23902)
- src/vsdk/unnamed_d3d_tcache_0x54bb30.cpp (gr_d3d_create_texture_section D23903)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root W/src --only "conversationctl|unnamed_d3d_tcache_0x54bb30"
--check src/decomp/vc6-matches.txt`: 2 units: 35 match, 0 wrong target, 6 differ, 0 not found, 0 errors;
34 matched there, 35 now, **0 lost**.

## Worker 19

### m-1021e worker 19 (D24000-D24099)

Batch: vif_record_get_bounds, vif_emitter_update, vif_compact_particles (vsdk/unnamed_vif_0x5329f0.cpp),
cast_effect_track_add, ai_state::load (game/ai.cpp), state_title_enter (game/unnamed_states_0x431870.cpp),
ca_character::add_skeleton (vsdk/character.cpp), ai_state::machival_ai_select_action
(game/unnamed_machival_0x4c0c40.cpp).

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/unnamed_machival_0x4c0c40.cpp | 0x4c13a0 | ai_state::machival_ai_select_action | D24001 | `target` set from a `Character *t` copy (`t = living_from_handle(..); target = t; if (t && hidden) { target = NULL; t = *(Character * volatile *)&target; }`), `t == NULL` for the null test, and `handle = machival_target_handle;` read before the `timer_up \|\| !target` test, passed to the pick. 144 -> 0. The volatile read is accepted because it gives the exact match (the binary re-reads target from its frame slot). |

## Closer, kept

- game/ai.cpp 0x4750b0 cast_effect_track_add: 14 -> 2 (**D24000**). `vfx_instance *in = fx->instance;` read
  into a local for the time gives the binary's EAX / ECX / EDX in the time code. Left: the free-slot loop tests
  the counter (`cmp eax, 5`), the binary the pointer end (`cmp ecx, end`, i.e. linear-function test
  replacement of the plain indexed loop).
- vsdk/unnamed_vif_0x5329f0.cpp 0x532f40 vif_emitter_update: 143 -> 8 (**D24002**). Four changes:
  - `spawn = !(late > interval) ? 1 : n;` instead of `spawn = 1; if (late > interval) spawn = n;` (two
    separate tests as in the binary; spawn in memory, i in EBX);
  - `rec->speed_spread` tested and passed to frand_range directly (no `speed = rec->speed_spread` copy:
    8966 then keeps it as the binary's `fld / fst [slot] / fcomp` temporary; this was most of the distance);
  - the free-list relink in the order unlink, `p->next = free.next`, `free.next->prev = p`, `free.next = p`,
    `p->prev = &free`, flags, `num_live--`;
  - `em->spawn_timer = remaining = float_min(..) - reported_frametime;` chained (flag load before the store),
    and the late time kept in `remaining` (`remaining = (float)fabs(remaining);`; n then takes remaining's slot).
  Left: after `interval - late` 8966 keeps late on the x87 stack (`fld interval / fsub st, st(1) / fstp /
  fstp st(0)`), the binary has `fsubr interval`.
  Behaviour: the same (independent stores reordered, the same frand_range argument values, remaining not
  used after the spawn block).

## Still differs (tried; do not repeat)

- **cast_effect_track_add (2)**: with the `in` local: the plain indexed loop (16, the binary's pointer-end test
  but EAX / ECX swapped), i first / block-scoped / `register` / unsigned with `(int)i <`, `(cast_effect_tracks +
  i)->spell`, a table pointer `e[i]`, e incremented in the body / in the for (4 / 6), e set before the for, a
  pointer-end condition (18), `i != 5` (9), `i <= 4` (4), a do / while (35 / 48), a while with the test in the
  condition, an inline find helper (both returning forms and the e walk: same swap), `int *sp` walk (2), a slot
  pointer for the stores (36), the one-expression time (5 / 19), `if (i != 5) { ... }` one exit (2 / 16), a
  return inside the loop (20), `i >= 5` (25).
- **ai_state::load (18)**: a `const int *` for the combat_style read, the arg copy as an int store, through a
  `const ubyte *ap` pointer, `n += 4` first, an unsigned hash, a target pointer local, a target temp (19), a
  type pointer (41), a `self` pointer: all 18 (besides the 21b / 21d lists).
- **state_title_enter (14)**: not retried; D11910 / D12707 / D19202 / D21306 list every form.
- **vif_record_get_bounds (16)**: not retried; D19103 and the 21d list are exhaustive.
- **ca_character::add_skeleton (38)**: `return num_anims++;` gives 32 (the epilogue then follows the main path
  as in the binary, but the dup return is laid out after it with its own epilogue and the tail computes
  `old + 1` in EDX); `(num_anims++)`, `int n = num_anims++; return n;` 32; `skeletons[num_anims++] = s`,
  `i = num_anims - 1; return i;`, `num_anims += 1`, `!_stricmp`, `++num_anims - 1`, `(num_anims += 1) - 1`: 38;
  File before FileReader 44; the creation part in a block with `goto found` to a final `return i` (with the
  old tail, with `num_anims++`, or with `i = num_anims - 1` falling through): all 38 (the same layout).
- **vif_compact_particles (269)**: the binary computes the new half's base twice (new_base in EBP and
  vif_particle_top in ECX, no CSE). Tried `&vif_particles[h][0]` for either, top first, a cast,
  `vif_particles[0] + h * VIF_MAX_PARTICLES`: all 269 (8966 folds them into one). Also the record pointer bias
  (binary +0x70, ours +0xa0) is unchanged. Not pursued further.
- **vif_emitter_update (8)**: for the x87 late: a function-scope late, `float late[1]`, late in its own block,
  `late = interval - late; spawn_timer = late`, `-late + interval`, a ternary expression for the store, the
  store in both branches (22), a `remaining = late` copy (154), `remaining = remaining - n * interval`,
  `interval * n`, fabs without the cast; an inline helper (not inlined, /Ob0). Earlier steps: `if (interval <
  late)` (160), a bool for the test (164), `int spawn[1]` / `int *sp` / `int *np` / `float late[1]` (no
  change), `if ((speed = rec->speed_spread) == 0.0f)` (no change), the flag test as `!= 0` / nested / goto,
  em->interval stored first (117).

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files (under W/src)

- game/ai.cpp (cast_effect_track_add, D24000)
- game/unnamed_machival_0x4c0c40.cpp (ai_state::machival_ai_select_action, D24001; marker now exact)
- vsdk/unnamed_vif_0x5329f0.cpp (vif_emitter_update, D24002)

## --check (VC6_STRICT=1 vc6-sweep, the three units together)

`3 units: 124 match, 0 wrong target, 5 differ, 0 not found, 0 errors`
`check against src/decomp/vc6-matches.txt: 123 matched there, 124 now, 0 lost`

## Decisions

**D24000 (game/ai.cpp; closer only). cast_effect_track_add 0x4750b0: 14 -> 2.** fx->instance is read into a
`vfx_instance *in` local for the time, which gives the binary's EAX / ECX / EDX there. Left: the free-slot loop
tests the counter; the binary tests the pointer end.

**D24001 (game/unnamed_machival_0x4c0c40.cpp; match). ai_state::machival_ai_select_action 0x4c13a0.** The
target is taken through a `t` copy: `target = t`, and when hidden `target = NULL; t = *(Character * volatile
*)&target;` (the binary stores 0 to target's frame slot and reloads it); the first null test uses t. This also
frees EBP, so target lives in memory as in the binary (frame 0x78). machival_target_handle is read into handle
before the `timer_up || !target` test and passed to boss_target_list_pick, which gives the binary's early load
and register rotation in the rest of the function. Same behaviour.

**D24002 (vsdk/unnamed_vif_0x5329f0.cpp; closer only). vif_emitter_update 0x532f40: 143 -> 8.** spawn as
`!(late > interval) ? 1 : n`, the speed spread tested and passed from rec directly, the free-list relink in
the binary's store order, the spawn_timer store chained with remaining, and the late time kept in remaining.
Left: the x87 late after `interval - late`.

**D24003 (notes only).** No new form for ai_state::load, ca_character::add_skeleton (`return num_anims++` 32,
not kept: the tail computation differs), vif_compact_particles; state_title_enter and vif_record_get_bounds not
retried (exhausted in earlier rounds).

Helper scripts are in W/try (tryd.mjs: distance per variant; trys.mjs: the same with a side-by-side when SIDE is
set; sdiff.cjs: an LCS diff of a side-by-side by mnemonic; apply.cjs: applies a variant). Kept compile folders
are in W/tmp (not the system Temp folder).

## Worker 20

### m-1021e w20: byte-match notes (decisions D24100 to D24199)

Batch: decals_draw_for_lobj, decal_place, decal_build_clip_planes (game/unnamed_decals_0x4bbe80.cpp),
LineFile::detect_eol, LineFile::read_line (vsdk/unnamed_linefile_0x558340.cpp), localize_load_names
(vsdk/unnamed_localize_0x503f60.cpp), ui_slider::create_scrollbar (vsdk/unnamed_ui_slider_0x55e040.cpp),
character_equip_item (game/unnamed_party_inventory_0x46b460.cpp).

## New matches

- game/unnamed_decals_0x4bbe80.cpp 0x4bc320 decal_place (66 -> match), **D24100**.
- game/unnamed_party_inventory_0x46b460.cpp 0x46bdf0 character_equip_item (147 -> match), **D24101**.

**D24100 (decal_place).** Four changes on top of D22001, all needed (each one taken back: 24, 6, 41 or worse):
1. The recycle test reads the global: `d = decal_free; if (decal_free == NULL && decal_age != NULL) { ...; d =
   decal_free; }`. With `d == NULL` the zero register and the d / pointer registers come out rotated.
2. The removal goes through `decal_record **pn = &decal_free->next;` (d is decal_free there), `*pn` for every
   d->next of the removal; 8966 then keeps `&d->next` in EDI (the binary's `lea edi, [esi + 0x28]`) for the
   removal and for plist_append's next stores. `pn = &d->next` is folded back to [esi+0x28] (41).
3. The lifetime store through a record pointer copy, `decal_record *dd = d; dd->lifetime = ms;`: 8966 then emits
   the bit-field insert as the binary's separate `(old ^ ms) & mask ^ old` and clears in_use / lit with their own
   `and 0x3fffffff`, instead of folding the three stores' masks together.
4. In the age-ring loop the record is taken inside the lifetime test: `if (decal_records[n -
   decal_nodes].lifetime) { other = &decal_records[n - decal_nodes]; left = ...; } else left = 0x7fffffff;` (the
   binary's `shl eax, 3; test [eax + records + 0x24]; lea edi, [eax + records]`).
Behaviour unchanged (d == decal_free in the removal; the same reads and stores).

**D24101 (character_equip_item).** Two things:
1. The case order of all three switches follows the binary's block order (8966 lays case blocks out in source
   order here): the item-slot switch has HANDS and FEET before FINGER / WEAPON; the texture-pointer switch has
   TORSO / LEGS / HEAVY before HANDS / FEET; the set_underwear_textures switch is HEAVY (falls into TORSO),
   TORSO, LEGS, FEET, HANDS.
2. The status-effect modifier through a float local with the level test choosing between two calls: `float
   mod; if (level == 0) mod = status_effect_modifier(i, c->level); else mod = status_effect_modifier(i,
   level); c->status_effect_add_from_item(i, item, mod);` (the binary stores the result in the dead item argument
   slot, reloads it into EDX and keeps the level in EAX). The ternary as the argument or a mod local after an
   if gives 85-87.
Behaviour unchanged.

Tool note for the merge: tools/vc6-side.mjs marks only lines whose mnemonics differ. character_equip_item
looked identical up to its jump tables in --side output while vc6-compare still said "differs at +0x145: push
ebx | orig: push 0x3" (an operand difference, a case block in the wrong place). Look at vc6-compare's first
differing offset, not only the `*` lines.

## Closer, kept

None besides the two matches.

## Still differs (tried; do not repeat)

**D24102 (notes only).**
- decals_draw_for_lobj 0x4bc6b0 (17): no change from file-local inline helpers for the three tri copies, for the
  polygon_touches_planes call, for both, or per vector3 copy; a `const vector3 *vv = verts` copy; a `vector3 *t
  = tri` pointer for the copies; tri declared after clipped (all 17). Worse: copy orders 2-1-0 and 0-2-1 (31), a
  num_planes int local (39). Same two scheduling hunks as before; stop.
- LineFile::detect_eol 0x558430 (14): all 14 (or worse) with: the lf branch as a ternary (22), through an
  unsigned f local (27), through `unsigned int *fp` in the ANY store or both stores (14), a `return 0` in any
  single ANY branch (18), the lf&&cr ANY as `|= LF | CR`, two `|=` statements, `&= ~MASK; |= ANY`, `(flags &
  ~MASK) | ANY`, `(flags | LF) | CR`; the same four forms in the "neither" ANY, the CRLF ANY or both (12 forms,
  all 14); a `goto any` into the "neither" ANY (14) or to a shared ANY after the return (18); an `any` bool flag
  with one store at the end (14); a pointer store in the final ANYs (14, 26 with `fp` at function scope). The
  lf&&cr ANY is always cross-jumped into the final ANY at the IL level (the binary keeps it separate); no source
  form found that changes that.
- LineFile::read_line 0x558600 (36): in the binary `left` is the size argument itself (`mov esi, [size]; dec
  esi; ...; mov [size], esi; test esi, esi`); `size--` / `size -= 1` used as left gives exactly the binary's
  prologue but 49 overall (the loop top test is still merged into the dec's flags and the memchr argument
  registers move). On that base, no change from `for (;;) { if (!size) break; ... }`, `do { if (!size) break;
  ... } while (eol == NULL)`, `if (eol == NULL) continue; break;`, a for with a goto to the tail (49), `while
  (size && eol == NULL)` (51), and the CRLF length as `n - (eol - cur)`, `cur + n - eol` (51), `k = n - (eol -
  cur)`, `k = cur - eol; k += n` (49). With left: `left = --size`, `left = size; left--`, `int *lp = &left`,
  `this->cur` in `n = eol - cur` (36).
- localize_load_names 0x504520 (17): the binary puts case 1's block after the epilogue and jumps back into case
  2's strcpy (cross-jump the other way round). No change (17) from an if / else-if chain (both orders), if forms
  with the return inside, a goto into case 1 from a test (all `cmp eax, 1` instead of the binary's `dec eax`),
  the switch with case 2 first, default first, `switch (which - 1)`, an explicit `case 0:` with the default, the
  table store after the strcpy, a `const char *` name with one strcpy (different code), case 1 as `goto item`
  with its block after the function's end (VC6 moves it back), case 1 ending in `goto` to the shared part.
- ui_slider::create_scrollbar 0x55ed30 (38): this / listbox ESI / EDI swap. All 38: `register` lb, lb used
  for every listbox access, lb declared after window, a `ui_slider *self = this` copy for the stores, view
  assigned first, lb declared in each branch, the cast written at every use, lb declared after set_parent,
  `this->` calls, count as two statements, window through `ui_element **pw`, the scrollbar stores through
  `ui_slider **`. Worse: set_page_size(visible) (47), the scrollbar store after the count (42).
- decal_build_clip_planes 0x4bbf10 (279): not retried. The binary calls plane::plane, vector3::len, the vector3
  constructors and operators out of line throughout; our VC6 expands some of them (first plane, len, the
  cross-product copy), so the frame and everything after differ. An inline-budget question, not source form.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files (under src)

- game/unnamed_decals_0x4bbe80.cpp (decal_place, D24100)
- game/unnamed_party_inventory_0x46b460.cpp (character_equip_item, D24101)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only
"unnamed_decals_0x4bbe80|unnamed_party_inventory_0x46b460" --check src/decomp/vc6-matches.txt`:
2 units: 43 match, 0 wrong target, 2 differ, 0 not found, 0 errors; 41 matched there, 43 now, 0 lost.

## Worker 21

### W21 notes (m-1021e, decisions D24200-D24299)

## New matches

| file | address | function | decision | what did it |
|---|---|---|---|---|
| game/unnamed_netpackets_0x441820.cpp | 0x442380 | cmd_disp_rate | D24200 | `print_to_console(string_buffer, 0)` written in both branches of the ON / OFF if / else |
| game/unnamed_netpackets_0x441820.cpp | 0x4419d0 | mp_reset_net_stats | D24201 | `net_stats_avg_sent_packets = 0;` moved before the third memset (after the timer) |
| vsdk/bmpman.cpp | 0x503aa0 | bm_lock_pcx | D24202 | every field indexed through a copy of the array base, `bitmap_entry *bm = bm_bitmaps;` |
| game/objects.cpp | 0x46a1b0 | item_obj_spawn | D24204 | `cast.dir.set(0.0f, -10.0f, 0.0f)` in both ray blocks (inline budget) + the -999999 constant (behaviour fix) |

**D24200 (game/unnamed_netpackets_0x441820.cpp). cmd_disp_rate 0x442380 is exact** with the final
print_to_console written into both branches of the ON / OFF test (`if (on) { sprintf(ON); print; } else
{ sprintf(OFF); print; }`). 8966 merges the two tails into the binary's single sprintf / print sequence, but the
EBP region (the entry pointer) then no longer has the one exit that lets 8966 shrink-wrap it (D8701): EBP is pushed
at the entry, and the two stack pops after sprintf / print_to_console are merged (`add esp, 0x14`), as in the
binary. Same behaviour. Before: EBP pushed inside the found branch, `add esp, 12` / `add esp, 8` separate
(distance 16).

**D24201 (game/unnamed_netpackets_0x441820.cpp). mp_reset_net_stats 0x4419d0 is exact** with the first
counter's store before the third memset: `memset; memset; net_stats_timer = 0.0f; net_stats_avg_sent_packets =
0; memset; ...`. The int zero then has to live across the third rep stosd, so 8966 gives it its own register
(`xor edx, edx` before the rep stosd) and uses it for all seven counters; the float timer keeps memset's EAX zero,
and the scheduler puts both stores where the binary has them. Same behaviour (independent globals all set to 0).
On the way (not kept): `*(unsigned *)&net_stats_timer = 0u;` anywhere before the third memset gives distance 3
(EDX zero right, but the timer then also uses EDX after the rep stosd).

**D24202 (vsdk/bmpman.cpp). bm_lock_pcx 0x503aa0 is exact** with `bitmap_entry *bm = bm_bitmaps;` declared
first and every access written `bm[index].field`. With the global array indexed directly 8966 pre-shifts
9 * index (`shl esi, 3`, then `[esi + field]`); through the pointer copy it keeps `lea esi, [eax + 8 * eax]` and
`[8 * esi + field]` in each address, as the binary. Same behaviour. The same device may help bm_lock_tga /
bm_set_reduced (not in my batch, not tried). A late `bm = bm_bitmaps;` assignment (after the bpp call) does not
work; a `bitmap_entry &e` reference is worse (29).

**D24203 (game/new_assess.cpp; closer only, 43 -> 28). assess_format_info 0x4013f0:** the damage read through a
`ushort *dp` chosen in the two branches (`dp = &...->type->damage;` / `dp = &target->info->damage;` then
`damage = *dp;`, in a block). Same value (the ushort zero-extended once). Left: the binary keeps protection in
EBP and damage in the target argument slot (loaded into EAX in each branch, stored once at the join); ours still
has damage in EBP and protection in the slot. The source file is CRLF; kept CRLF.

**D24204 (game/objects.cpp). item_obj_spawn 0x46a1b0 is exact** with the ray direction set through the inline
`vector3::set` in both ray blocks (`cast.dir.set(0.0f, -10.0f, 0.0f);` in place of three member stores; same
stores in the same order). This is D16104's inline budget: the two extra inline expansions make 8966 expand
`io->orient = *orient` as the binary does (rows 0 and 1 assigned inline with their result copies through the
out-of-line copy constructor 0x410cd0, row 2 through vector3::operator= 0x413190); the frame shrinks from 0xf8 to
the binary's 0xf0. One set alone is worse (234), both as `= vector3(...)` worse (288). The last difference was a
constant: see Behaviour errors.

## Closer, kept

- game/new_assess.cpp assess_format_info 0x4013f0: 43 -> 28 (D24203).

## Still differs (tried; do not repeat)

**D24205 (notes only). Tried without a match:**

- dialogue_topic_list::draw 0x4d7930 (19): the binary loads `first` into EDI (later the entry pointer) and adds
  it to a copy of i (`mov edi, [esi + 0x20]; mov eax, ebx; add eax, edi`); ours adds from memory. Tried (all 19
  unless noted): `int f = first` with `(i + f)` / `(f + i)`, `idx = i; idx += first`, `entry + (...)`, visible
  computed first (29), `const int *fp = &first`, a `dialogue_topic_list *t = this`, an `entry` pointer copy
  (both operand orders), e / visible / idx at function scope, e declared first, i declared first, unsigned i with
  `(int)i < count` (25-29), the clip values as `int c[4]` (25) and in reverse order, no e pointer (42), visible
  inline, nested ifs, an idx local with `entry[idx]` (39), a reference, a file-local inline `topic_at(this, i)` in
  four forms, an inline `ring_index(first, i)` in both orders, `register` e, `*(volatile int *)&first` in both
  orders, `int f[1]`. Probably 8966's register choice (dialogue_box::draw, distance 5 now, has the same EBP load).
- bm_read_header 0x502a00 (193): the binary keeps `type` in a stack slot (between mip_bpp and mip_w) and the
  pointer parameters num_levels / num_frames / fps in EBP / EBX / ESI; ours keeps type in ESI. Tried: type
  declared first / last, a `int *tp = &type` store, a `register int *` copy of num_levels: all 193 (identical
  code). Not pursued further (whole-function register allocation).
- bm_load_internal 0x502720 (178): looked only (name / force_new in EBP / EBX swapped, frame 4 bytes larger,
  `reduced` in BL in the binary, D16505). Not retried.

## Behaviour errors / bug candidates

- **item_obj_spawn 0x46a1b0 (fixed, D24204):** the best floor height started at `-1000000.0f`; the binary stores
  0xc97423f0 = -999999.0f (`mov dword ptr [ebp - 0x8c], 0xc97423f0`). Changed to `-999999.0f`. In practice no
  difference (a hit must lie above it).

## Header changes

None. (A diagnostic `long` / `unsigned` type for the net-stats counters in
include/summoner/multi/unnamed_netpackets_0x441820.h was tried and restored byte for byte; no effect.)

## Changed files

- src/game/unnamed_netpackets_0x441820.cpp (cmd_disp_rate D24200, mp_reset_net_stats D24201)
- src/vsdk/bmpman.cpp (bm_lock_pcx D24202)
- src/game/new_assess.cpp (assess_format_info D24203, closer only)
- src/game/objects.cpp (item_obj_spawn D24204, includes the constant fix)

## --check result

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "unnamed_netpackets_0x441820|vsdk/bmpman|new_assess|game/objects\.cpp" --check src/decomp/vc6-matches.txt`:
4 units: 121 match, 0 wrong target, 5 differ, 0 not found, 0 errors; 117 matched there, 121 now, **0 lost**.

Per unit (vc6-compare --score): netpackets 12/12, bmpman 52/56, new_assess 17/18, objects 40/40.

## Method notes

- `try.mjs` in W (my helper): applies variants and prints the --score distance; it supports `reps` (several
  replacements) and `after` (replace the first occurrence after a marker string). Without `after`, a pattern can
  hit an earlier function with the same text (dialogue_topic_list::layout has the same entry expression).

## Worker 22

### W22 notes (m-1021e), decisions D24300-D24307

## New matches

None.

## Closer, kept (behaviour identical)

| file | address | name | distance | D |
|---|---|---|---|---|
| game/unnamed_interface_0x411270.cpp | 0x413c70 | ui_choice_popup_draw | 44 -> 11 | D24300 |

**D24300 (ui_choice_popup_draw 0x413c70, closer).** Two changes:
- title: `int *xp = &x; *xp += ui_choice_popup_x;` then `gr_string(x, ...)`. This gives the binary's `add eax, ecx`
  and the h load before `add esp`, so the title block is now byte-equal (44 -> 30 on its own). Plain `x += popup_x`,
  `popup_x + x` and a `const int *` to popup_x do not change it.
- loop: `char **pp = ui_choice_popup_choices;` walked as `for (i = 0; ...; i++, pp++)` with `*pp` in both calls.
  This gives the binary's EBX pointer / EBP x (44 -> 28 on its own).
Together: 11. Left: (1) 8966 hoists `mov ebx, offset choices` above the count test's `jle` (binary: after it); (2) in
the loop the binary loads popup_x into EAX, adds it, then loads hover into EAX, while ours loads popup_x into EDX
first. Tried on top of D24300, all 11 or worse: pp declared at the top (30), assigned apart, in the for init,
`&choices[0]`, in a block, a `while` loop, `if (count > 0)` guard (15), pp++ in the body (13); for (2): a pointer
store `*xq += popup_x` (block or function scope), `x = x + popup_x`, `popup_x + x`, an int local for popup_x, a
`const int *` to popup_x, the sum in one expression (15), an if / else for x, a flags local, hover through a local /
pointer / bool, `hover == i` (13). Index form `ui_choice_popup_choices[i]` with an `s` local: 57.

## Still differs (tried; do not repeat)

- **gr_get_frustum_planes 0x50ed60 (20)** (D24301). Pure EAX / ECX swap of count and second index. New forms, all
  20: unsigned index with `(int)j < n`, a `ubyte *f = flags` copy, a pointer to each flag (`ubyte *f = &flags[i]`),
  n through an int copy and through `int *np`, `int n; int i;` order, a block-local j, the first loop with an
  unsigned index. Register choice; exhausted.
- **ds_stream_open 0x55d400 (18)** (D24302). Cross-jumped `or eax, -1` epilogues (the binary keeps three). New: the
  success return written `== 0 return 0; return -1;` per branch (25: 8966 turns it into neg / sbb), `for (;;) { if
  (... == 0) break; return -1; }` per branch (18). Exhausted with the D21903 list.
- **ui_choice_popup_show 0x4136a0 (20)** (D24303). i zeroed early in ESI. New: memset of w / h (69), a bool for the
  title strlen, an n local for the count (55), i set through `int *ip`, a guarded do / while with a `char **` (19:
  same ESI / EDI pair). Exhausted.
- **matrix43::transpose_in_place 0x511bb0 (46)** (D24304). Analysis: the binary's two cycles are exactly
  `t = m11; m11 = m01; m01 = m10; m10 = t; t = m30; m30 = m12; m12 = m10; m10 = t;` (and the same with m22 / m02 /
  m20 / m21 / m31); the [esp] stores are t's, the first m10 store is dead. New forms with that order, none keeps
  the stack slot: float t (48), `struct { float v; } t` (48), a union float / int (48), `vector2 t` (66),
  `float t[1]` (48), `float *tp = &t` (48), a second float u carrying the first value (46), a row pointer (48).
  (Note: `--side transpose_in_place` matches matrix::transpose_in_place first; use `transpose_in_place@matrix43`.)
- **ui_choice_popup_draw**: see D24300 above.
- **ui_message_box_show 0x413e40 (49)** (D24305). The binary re-reads the flags byte after the store and tests it
  against `mov al, 1` shared with the game_flags test. New, all 49 or worse (8966 forwards the stored value every
  time): `*(ubyte *)&flags & 1`, `*(char *)`, `*(bool *)`, both flags as byte reads, a local one-bit bit-field
  struct (`unsigned` and `unsigned char`) for one or both, the flags store through `unsigned *fp` (at use / at top),
  `!= 0` tests; callback stored first with these (51). Only volatile re-reads it (D20407). Exhausted.
- **ds_load_buffer_acm 0x55cbe0 (114)** (D24306). New, all 114 (no change at all): src and rc both through
  pointers, src through `void **ps` (store only; store and test), `void *src[1]`. Exhausted.
- **mp_handle_item_drop 0x489120 (150)** (D24307). Frame analysis of Sum.exe (offsets from the frame base):
  0 loop counter, 4 count, 8 next (in memory), 0xc net, 0x10 pos, 0x1c lock, 0x20-0x44 the three vector3
  temporaries of the matrix, with the dequantize result temporary sharing 0x20, 0x44 orient, 0x68 items with the
  matrix result temporary sharing it; gold in EDI, data in ESI. Our `vector3 pos = f(...)` has no temporary
  (frame 168 = the binary's only because gold sits in memory). New forms giving the binary's temporary copy, all
  frame 180: `pos = f(...)` (224), memcpy from a block temporary (167), pos declared first / with orient (197),
  everything C-style (declarations first, then statements; three pos positions, 197), the orient or the pos
  statement in its own block (197), a block `vector3 t; pos = t` (167), named rvec / uvec / fvec in a block (199);
  a `ubyte *d` copy of data (150). 8966 does not share the dequantize temporary's slot with the vector
  temporaries. Not found.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/game/unnamed_interface_0x411270.cpp (ui_choice_popup_draw only, D24300)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "unnamed_interface_0x411270" --check src/decomp/vc6-matches.txt`:
1 unit: 65 match, 0 wrong target, 4 differ; 65 matched there, 65 now, **0 lost**.

## Worker 23

### m-1021e worker 23 (D24400 - D24499)

Batch: level_parse_table, level_parse_shopkeeper, level_script_parse_table (game/level_scripts_common.cpp),
glide_record::load_section, gr_glide_bitmap (vsdk/gr_glide.cpp), cquat::multiply
(vsdk/unnamed_quaternion_0x5225b0.cpp), snd_stream_fill_silence (vsdk/unnamed_sndstream_0x566560.cpp),
journal_handle_click (game/new_journal.cpp).

## New matches

None.

## Closer, kept

- game/level_scripts_common.cpp 0x4cc4d0 level_parse_shopkeeper: 23 -> 17 (D24400).
- game/level_scripts_common.cpp 0x4cd5f0 level_script_parse_table: 97 -> 92 (D24401).

**D24400 (game/level_scripts_common.cpp). level_parse_shopkeeper 0x4cc4d0: 23 -> 17.** The item check is
`if (type != -1) { if (!item_types[type].name) continue; price = item_types[type].value; }`: no `it`
pointer, the name test before the price read. 8966 then indexes item_types with EAX (`shl eax, 8`, the call
result) as the binary does; with the `it` pointer, or with price read first, it copies EDI to ECX. Behaviour is the
same (price is not used after the `continue`). Left: the binary reads name and then value before the branch and
stores price before it (`mov ecx, [eax + name]; mov eax, [eax + value]; test ecx, ecx; mov price, eax; je`); every
form that reads value before the test (into a name local, a bool, `== NULL`, `goto next`, value through an int
local, a reference, a const pointer, byte offsets, price stored in both paths) puts the name in EDX and copies
EDI again (21-23). Also left: the game_flags test after the fild and the dead lea before the count reload.

**D24401 (game/level_scripts_common.cpp). level_script_parse_table 0x4cd5f0: 97 -> 92.** The door's lock bit
as `(((locked ? 1 : 0) & 1) << 3)` gives the binary's `and edx, 1` (a bit-field merge). A function-local
one-bit bit-field over d->state (`struct { unsigned low : 3; unsigned locked : 1; }`) assigned `locked ? 1 : 0`
or `locked != 0` gives the same 92; assigned the bool directly 100; `(locked & 1) << 3` 100. Behaviour is
unchanged. Left (most of the distance): the binary keeps the zero constant in EBX (re-zeroed after the door
loop) and l (NULL per door, `xor ebp, ebp` in the door body, also used for open_a / open_b = 0) in EBP; ours
merges l's NULL and the zero constant into EBP for the whole function. Tried without change (92): l = NULL
before the for, after the ParseString, in the for-init, declared last, `register`, through a `lobj **`, a
one-element array, the loop index as a block local, `lobjs + i`, open_a / open_b / open_c as separate
declarations.

## Still differs (tried; do not repeat)

- **glide_record::load_section 0x549a20 (19)**: all 19 (same code): `float *vs = v_scale` for the store, the
  bitmap index as an int local before u / v and at its store, `float v` read after `this->section = section`, a
  `float *tv = &this->v_scale` store, `const float *pv = &sec->v_scale`, `int *sp = &section`, `*(int *)&section =
  *(int *)&v` (an explicit store into the dead section slot). With the earlier lists: exhausted (register choice).
- **cquat::multiply 0x522a40 (20)**: the unit is compiled /Ob0, so inline helpers are calls: a `cq_f(short)`
  helper (137), a `cq_v(vector3 &, const cquat &)` helper (117), quaternion::unpack_short4 into two quaternions
  (136); pointer locals for v1 / v2 (20), w1 / w2 converted between v1 and v2 (74). Scheduling of the movsx /
  fild pairs; stop.
- **level_parse_table 0x4cb7b0 (21)**: the instructions are the binary's; only buffer frame slots differ
  (D20701's table). Not retried beyond an analysis: neither bottom-up nor top-down first fit with scope or loop
  interference reproduces the binary's layout (filename overlaps the tail of eax_path, a 128-byte hole sits
  between psx_bitmap and psx_layer). w23/cod.mjs prints our layout of a function.
- **level_script_parse_table 0x4cd5f0 (92)**: see D24401.
- **snd_stream_fill_silence 0x566970 (51)**: pointer locals for the Lock out arguments (`DWORD *pn2 = &n2`, the
  same for n1, p2, and all four), a `ubyte *fp = &fill` for the 16-bit store, both together, a one-byte fill
  array: all 51. `snd_streams[stream].write_pos = ...` (stream kept live) 89. The binary also keeps n1 in EBP
  after the first memset. Exhausted with D19's / D20's lists.
- **journal_handle_click 0x41af80 (152)**: the binary loads the constant 2 into EBP in both the button hit path
  (used by `b->flags |= 2`) and the loop exit; ours loads it into EDI once at the join. The binary keeps font_h in
  EBP and spills the row counter; ours spills font_h. Tried (all 152): `register int font_h`, font_h declared
  first / last / at its assignment, `b->flags |= 2` before the pressed store, a separate index for the thumb
  loop; the row position as `row * font_h` (both orders, also first in the sum) instead of the running y (154),
  y initialised at its declaration (152). Register allocation; stop.
- **gr_glide_bitmap 0x54a800 (252)**: much of the distance is frame slots: the binary's one-section block has
  a / r / b / g at -304 / -300 / -296 / -292 (ours a -308, g -296, b -292), u0 -288, v0 -284, fx0 -280, u1 -276,
  fx1 -272, fy1 -268, fbh and v1 sharing -320, fy0 -316. All 24 declaration orders of r, g, b, a (declared
  together, assigned in r, g, b, a order) give 252: the slots are not set by declaration order. The rest is the
  known vertex-store scheduling.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/game/level_scripts_common.cpp (level_parse_shopkeeper D24400, level_script_parse_table D24401; comments)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <w23>/src --only "level_scripts_common" --check
src/decomp/vc6-matches.txt`: 1 unit, 74 match, 4 differ; 74 matched there, 74 now, 0 lost.

## Worker 24

### W24 notes (m-1021e, decisions D24500 to D24599)

Batch: nw_NewCloseSocket, nw_init_networking, nw_WorkReliable (vsdk/unnamed_net_0x517590.cpp),
shadow_create_blob_bitmap, fx_missile_hit (game/unnamed_fx_0x489c40.cpp), mp_player_alloc
(game/unnamed_netsession_0x440b40.cpp), credits_parse_table (game/new_credits.cpp), polygon_clip_2d
(vsdk/unnamed_poly_clip_0x5519c0.cpp).

## New matches

- game/unnamed_fx_0x489c40.cpp 0x489f10 shadow_create_blob_bitmap (21 -> exact). **D24500.**
- vsdk/unnamed_net_0x517590.cpp 0x5188d0 nw_init_networking (48 -> exact). **D24501.**
- game/new_credits.cpp 0x408620 credits_parse_table (53 -> exact). **D24502.**

**D24500 (game/unnamed_fx_0x489c40.cpp). shadow_create_blob_bitmap 0x489f10 exact.** The ramps are written
`shadow_blob_pixels[i][row * 64 + x]` inside `for (i = 0; i < NUM_SHADOW_BLOBS; i++)` instead of through a
running pixel pointer `p` with a do / while on p. 8966 then makes the row pointer its own induction variable
(EBP, `add ebp, 0x40` per row, the outer test `cmp ebp, end` replaces `i < 16`) and keeps y in the frame,
as the binary does; the earlier p-pointer forms all gave p the frame slot and y EBP (D20006). The for
increment must be `y--, row++` (with `row++, y--` the y register and the dec / add order differ, 6).
`(row << 6) + x` and `x + row * 64` give the same code. Same behaviour (64 rows of 64 per blob).

**D24501 (vsdk/unnamed_net_0x517590.cpp). nw_init_networking 0x5188d0 exact.** The speed chain only sets
nw_network_status (`else nw_network_status = 0;`), then `if (nw_network_status == 0) return;` and
`if (nw_network_status != 3) nw_networking_ok = nw_ras_init() != 0;`. 8966 threads both tests into the
chain's branches: the unknown speed joins the inline "none" return at the top, 56k jumps back to the RAS
probe and fast to WSAStartup, both placed after the body, which is the binary's layout that no goto shape
gave (D8903, D21xxx notes). `status < 3`, `<= 2` and `== 1 || == 2` for the probe test give 14 with the
`else { status = 0; return; }` form; the separate `== 0` return is what matches. Same behaviour.

**D24502 (game/new_credits.cpp). credits_parse_table 0x408620 exact.** In the wrap loop the piece length is
written out as `end - start` three times (strncpy count, terminator index, `out += end - start + 1`)
instead of an `int len = end - start;` local. With the local, 8966 coalesced start = 0 with the zero
register and moved out into EBX; without it, EBX stays the zero register through the read loop (re-zeroed
with `xor ebx, ebx` after the inner loop), out is EBP and start ESI, as in the binary. Same behaviour.

## Closer, kept

- game/unnamed_fx_0x489c40.cpp 0x48bac0 fx_missile_hit: 73 -> 39. **D24503.**
- vsdk/unnamed_net_0x517590.cpp 0x517e00 nw_WorkReliable: 158 -> 19. **D24504.**
- vsdk/unnamed_poly_clip_0x5519c0.cpp 0x5519f0 polygon_clip_2d: 185 -> 84. **D24505.**

**D24503 (game/unnamed_fx_0x489c40.cpp). fx_missile_hit 0x48bac0, 73 -> 39.** The spell-missile part moved
into a file-local `static inline bool fx_missile_spell_part(effect_object *fx, float frametime)` (defined
before the function; with the definition after it is not inlined), called as
`if (!(fx->fx_flags & 8)) return fx_missile_spell_part(fx, frametime);`. 8966 then pushes EBP / ESI only
around the spell part, as the binary does (41). The timeout written
`(int)(((distance(...) * 0.75f) * 1000.0f) * 0.05f)` gives the binary's three fmuls (0.75, 1000, 0.05); with
the helper this is now also lower (39) and the sizes are equal (637). Left: vi in EBP and c in EBX inside
the spell part (binary: vi EBX, c EBP). Tried on top of the helper, all 39 unless noted: declaration orders
of vi / n / c (4), c declared in the block, vi initialised at its declaration, vi passed as a parameter
(from the caller), frametime first in the parameters, `register` on vi / on c, `vi->is_stopped() == false`,
a copy `vfx_instance *v2 = vi` for stop, a `Character *cc = c` copy for the hit call, a `first = vi->emitters`
local (35, size 635), the loop with a goto to a false tail, a `bool done = true` in the helper, `done =
helper(); return done;` in the caller. Two forms put vi in EBX and c in EBP but change the loop tail: a
`bool r = true; ... { r = false; break; } return r;` result (33, size 643: r in AL, n in ECX) and the same
with an int r and `return r != 0` (28, size 650, setne). Not kept: they move away from the binary's loop
(n in EAX, `mov al, 1` inline, the false return shared with the weapon tail). Without the helper, a
`done = false; break; ... return done` spell loop and the three-fmul form alone stay at 57 / 71.

**D24504 (vsdk/unnamed_net_0x517590.cpp). nw_WorkReliable 0x517e00, 158 -> 19.** Six changes, all the same
behaviour: (1) `rsocket = NULL;` after nw_receive_all() (the binary's `xor ebx, ebx` follows the call);
(2) the not-connected block reads the status once, `int status = rsocket->status;`, for the LIMBO test and
the sprintf argument (the binary keeps it in EAX and pushes it; the type then lands in CL; 156 -> 91);
(3) `if (!rcode) break;` instead of `continue` in the do / while (rcode > 0): 8966 then rotates the loop
as the binary (the slot-status call repeated at the bottom) and the recvfrom error block moves after the
limbo-timeout block of the second loop (91 -> 60; `rcode = 1; while (rcode > 0)` and `for (rcode = 1; ...)`
give 68); (4) the wrap test `rsocket->theirsequence < 0x10000 - NW_MAX_NET_BUFFERS` (binary `cmp ax, 0xff6a;
jae`; was `<= 0xffff - 150`, `ja`); (5) the resend header written send_time first, then data_len, seq, type
(the binary calls timer_get before the field loads and loads send_len once for the store and the memcpy);
(6) the heartbeat header in the same order (19). Left: (a) the IAMHERE `rsocket->status = RNF_CONNECTING`
store is scheduled after the reply's argument loads in the binary (locals for the arguments: 286, frame
change); (b) the dead `cmp eax, 0x258` before the recv_len store, and with it the seq register at the
rsequence store (binary `mov di, seq` used for the store and the reply; ours a second load). The dead
compare appears with `int len = data_len; if (len > NW_MAX_RELIABLE_DATA) rsocket->recv_len[j] = len; else
rsocket->recv_len[j] = len;` (merged identical branches, D10300), but the seq store then loads into CX and
the score is 50, so not kept. Tried for the compare without result (all 19 or worse): empty if / if-else /
braces, a bool local, a ternary, `while (...) break;`, `goto` to the next line, a self-assignment (114), the
check before the store, an empty if with an int cast, `len = data_len` again in the if (46), a nested
redundant if (46), the store in the if and after it (58), the store twice through recv_len (41, 46); for
the seq: the rsequence store before the memcpy (267), through a `ushort *`. Header forms tried: the header
fields read back for memcpy / the packet length (89 to 93), time first with the other fields in source
order or type last (51).

**D24505 (vsdk/unnamed_poly_clip_0x5519c0.cpp). polygon_clip_2d 0x5519f0, 185 -> 84.** (1) `const vector2 *b
= edges; if (i + 1 < num_edges) b = a + 1;` instead of the conditional expression (the binary loads edges
into EDI each edge and overwrites it with a + 1; 135); (2) `vector2 **dst = geom_poly2d_clip_list2;` and,
after the dists[n] / list[n] wrap stores, `if (list != geom_poly2d_clip_list) dst = geom_poly2d_clip_list;`
(134); (3) the first list filled through `geom_poly2d_clip_list[k]` with `list = geom_poly2d_clip_list;`
after the loop (84: n now in EBX and verts in EDI as in the binary). Note: a form that declared list
uninitialised and set it only after a loop that writes `list[k]` scored 81 but reads an uninitialised
pointer; not used. Left: the binary loads n into ESI first and copies it to EBX (the init counter is ESI),
stores list before the vector2 constructor call; the dot loop keeps two pointers (dists in ESI, list in
EDI, `add edi, 4`) where 8966 uses `[list - dists + p]`; the split branch's block order. Tried and not
kept: `fabs(normal.x) + fabs(normal.y)` (no change; the binary's double `fxch` not reproduced), float
locals for the sum (137), a down-counting pointer init loop (137), `b` with if / else (185), `&edges[i + 1]`
(181), declaration order (187), the init through a `vv = verts` copy, `*result = verts` after the init
loop (100 on its own, 139 with (3)), flip set after the loop (129), newp set after the loop (141),
`register` n, a/b declaration order, `a = b + i`, the dot loop through `lp` / `dp` pointers (144) or an
`lp` copy, the split block entered by a goto from the inside branch (113 / 143).

## Still differs (tried; do not repeat)

**D24506 (notes only). nw_NewCloseSocket 0x5191c0 (33).** Socket pointer in EDI and the i * 2 counter in
EBX (binary: pointer EBX, counter EDI). New forms on top of D19803, all 33 unless noted: a separate `id`
local (function scope or block) for the memset tail, `int *sp = socket` (first, last, `register`, `int
*const`) used for every `*socket`, `reliable_socket *rs = &nw_reliable_sockets[*socket or i]` for the
memset / status store (39). Register choice; stop.

**D24507 (notes only). mp_player_alloc 0x441610 (21).** The binary's `lea ecx, [eax+4]` (&p->prev kept and
reloaded through). New forms: `mp_player *volatile *pp = &p->prev` for every prev access, a volatile
`next` pointer with a plain prev pointer, the links as `mp_player **l = (mp_player **)p; l[0] / l[1]`.
None keeps the lea. Exhausted with D7006 / D12803 / D14507 / D18802.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/game/unnamed_fx_0x489c40.cpp (shadow_create_blob_bitmap; fx_missile_hit and the new file-local inline
  helper fx_missile_spell_part just before it)
- src/vsdk/unnamed_net_0x517590.cpp (nw_init_networking, nw_WorkReliable)
- src/game/new_credits.cpp (credits_parse_table)
- src/vsdk/unnamed_poly_clip_0x5519c0.cpp (polygon_clip_2d)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only
"unnamed_fx_0x489c40|unnamed_net_0x517590|new_credits|unnamed_poly_clip_0x5519c0" --check
src/decomp/vc6-matches.txt`:
4 units: 95 match, 0 wrong target, 10 differ, 0 not found, 0 errors; 92 matched there, 95 now, 0 lost.

## Worker 25

### Worker 25 notes (m-1021e; decisions D24600-D24699)

## New matches

None.

## Closer, kept

| file | address | name | old -> new | D |
|---|---|---|---|---|
| vsdk/unnamed_clip_0x5590b0.cpp | 0x5594f0 | gr_classify_sphere_view | 53 -> 36 | D24600 |
| game/unnamed_phoenix_0x497ac0.cpp | 0x498a20 | ai_state::phoenix_choose_attack | 115 -> 109 | D24601 |
| game/unnamed_phoenix_0x497ac0.cpp | 0x497da0 | ai_state::phoenix_reposition_update | 238 -> 156 | D24602 |
| game/new_levelselect.cpp | 0x41c450 | mp_levelselect_handle_click | 190 -> 126 | D24603 |

**D24600 (vsdk/unnamed_clip_0x5590b0.cpp; closer). gr_classify_sphere_view 0x5594f0: 53 -> 36.** The z test and
the d[0] test stay separate early returns; d[1], d[2] and d[3] are one `||` chain with the assignments inside
(`(d[1] = xs + nz) >= r || (d[2] = (ys = p->y * K) + nz) >= r || (d[3] = nz - ys) >= r`). Inside one expression
8966 keeps xs and ys on the FPU stack across the tests and gives the binary's `fst / fcomp` (d[0], d[2]) and
`fstp / fstp st(0) / fld / fcomp` (d[1], d[3]) pairs. Left: 8966 still keeps nz in memory (the binary keeps two
copies of it on the FPU stack, `fld st(0)` after the multiply), and the shared `return -1` block sits at the end
(binary: after the d[0] test, entered at four pop depths). Tried (all no better): the whole test as one `||`
chain from `z + r` on (with p->z: 93, with a z local and written-out products: 102, with nz / xs / ys assigned in
the chain: 38 / 45), the same as one `&&` chain around the second pass (38), z assigned in the first test (38),
`z *= K` reused as nz (38), the d[0] test inside the chain (38), nz assigned inside the d[0] test (46), `register
float nz`, d[2] / d[3] as a separate chain (36), nz copied into two locals for the two pairs (`n2 = n1`, copy
propagated: 53; `n2 = z * K` recomputed: 102), nz through a pointer, `-K * z`, `nz + xs`, declaration orders,
inline helpers (a pair test, a subtraction, a store-and-test helper: 79-81).

**D24601 (game/unnamed_phoenix_0x497ac0.cpp; closer). ai_state::phoenix_choose_attack 0x498a20: 115 -> 109.**
Two changes: the stage-2 ranged roll through the r local (`r = frand(); if (r < 0.3f / (...))`, as stage 3
already does) gives the binary's `fxch st(1) / fcompp / test ah, 1` (110), and the busy test written as D16404's
`if (owner->combat.is_busy() == true) { if (phoenix_cur_attack) return 5; } else if (phoenix_cur_attack) return 5;`
gives the dead `cmp al, 1` (109 with both). Same behaviour. Left: register plan (`this` in EBX and spilled to the
frame, n in EBP; binary `this` in EBP unspilled, n in EBX, stage 3's hi in a frame slot and lo in EBX) and the
global load before `sub esp`. Tried for the registers (all 109): lo / hi declared in the stage blocks (one or
both), n declared apart, a reordered declaration list, `n` through a second local, `ai_state *self = this`,
attackers declared and assigned apart.

**D24602 (game/unnamed_phoenix_0x497ac0.cpp; closer). ai_state::phoenix_reposition_update 0x497da0: 238 -> 156.**
vector3's operator= returns a copy; in this function 8966 emits that copy as an out-of-line copy-constructor
call (`??0vector3@@QAE@ABV0@@Z`), which the binary does not have. Writing `p = centre + p` per member
(`p.x = centre.x + p.x; ...`, 159) and the else branch's `centre = owner->pos` as memcpy (158) remove both calls;
the mounted branch written with its own `SetTimeout(5000); return true;` (the binary keeps two copies; the
unmounted one shares the dive branch's block) gives 156. Left: the dive branch (the binary has no separate mid:
it builds `(a + b) * 0.5` in centre's slot, copies centre / down[0]->pos / owner->pos and the dive point with
integer moves where ours goes through the FPU, and keeps its two set_mode calls apart), the else branch's hi
kept on the FPU across the join, the loop's p.z product kept on the FPU (the binary drops the dead p.z store)
and the loop's back edge (binary `jne` to the top, return false inline). Tried (not better): centre built
directly as `centre = (a + b) * 0.5f` without mid (187-189), `vector3 mid = (a + b) * 0.5f` (156), memcpy for
centre = mid (215-222), memcpy for the n == 1 / else copies (163) or the dive point (161), `dest` negated per
member (156), `p += centre` (220), `centre + p` through a const pointer (162-165), through a `vector3 *pp` (158),
through float locals (162), `vector3 q = centre + p` plus memcpy (170), `p.x = centre.x + p.x * s` with an s
local (160), keeping `p = centre + p` (169, the copy call gone but temporaries), a `for (;;)` loop with `if
(tries-- == 0) break` (156), lo / hi store orders and declaration order (156-158), `phoenix_flags &
PHOENIX_F_DIVE && ...` without parentheses (158).

**D24603 (game/new_levelselect.cpp; closer). mp_levelselect_handle_click 0x41c450: 190 -> 126.** (1) The row
positions written `top + row * font_h` with no running y (D21003's device): 8966 strength-reduces them and
zeroes y after the count test, as the binary. (2) rows as `count < page ? count : page` (the binary loads the
count, copies it and compares the loaded value; (1) alone is 192, (1) + (2) 186). (3) The list rectangle read into `int lx, ly, lw`
before point_in_rect (the binary loads all three, then pushes; 162). (4) The player rectangle the same way (px /
py / pw) and the player loop written `if (MP_PLAYER_TYPE(p) != 3) continue;` followed by qx / qy / qw locals
(126; the `continue` runs the for's increment, so row still counts every player: same behaviour). The function is
now the binary's size (1669 bytes). Left: font_h in EDI and the row counters in ESI (binary ESI / EDI), the list
rows' rectangle (binary indexes `[res * 2 + 1]` with one lea), and the chat block's shared `SetTimeout(500);
blink = true; return 0` tail (binary: kept in the append case, the backspace case jumps to it; ours the
reverse). Tried (not better): the same locals in the list rows (assigned inside the call: 142, separate
statements with `continue`: 146), only the player rectangle locals (176), an h local for `rows * font_h`, a
`cnt` copy for the clamp (192), font_h declared at its call, first, or after list / i (126), `register int
font_h`, row / p declared in the for statements (126), a `goto blink` from the backspace case into the append
case's tail (126).

## Still differs (tried; do not repeat)

**D24604 (notes only).**
- **phoenix_anim_remap_in 0x499ad0 (36, unchanged).** The rest is the loop's register rotation (slot base / weight
  base / temporary in ECX / EDX / ESI, binary EDX / ESI / ECX, push esi earlier). Tried on D20800's form (all 36
  unless noted): no `a` local (error: a needed), `a` at function scope, i at function scope, a `const float *`
  to the set weight, `int *ps` / `float *pw` locals for the two phoenix arrays (both orders), the run test
  written `mount->anim_run == a`, the set cast after the mount / rider code, an unsigned index with a signed
  test, `register int a`, `register int i`, a count local; worse: `int *ap = set->anim` (65), a bool ready
  (54), ready as a ?: (41), the count store before the rider test (45), the rider read before `ready = 0` (38).
- **Crc32::update 0x552130 (21, unchanged).** Binary: value in EAX, buf walked as a pointer (`inc edx`), the
  result returned from EAX without a copy. Tried (combinations not in the older lists): a `*p++` walk with
  `(value >> 8) ^ table[...]` first, the same with p++ as its own statement, a value local inside the loop, a
  `unsigned *vp = &value` pointer, table-word first with `*p++`: all 33 (worse).
- **fast_inv_sqrt 0x542500 (22)**, **float_format_buf 0x542830 (80)**: not retried; the older lists (D21304,
  D14605 / D14606, D19202) cover the new devices that apply. float_format_buf's first difference is still the
  integer copy of a into rem before the `>= 1` test and the x loop's guard.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/vsdk/unnamed_clip_0x5590b0.cpp (gr_classify_sphere_view, D24600)
- src/game/unnamed_phoenix_0x497ac0.cpp (phoenix_choose_attack D24601, phoenix_reposition_update D24602)
- src/game/new_levelselect.cpp (mp_levelselect_handle_click, D24603)

## --check results (VC6_STRICT=1)

- unnamed_phoenix_0x497ac0: 33 matched there, 33 now, 0 lost
- unnamed_clip_0x5590b0: 16 matched there, 16 now, 0 lost
- new_levelselect: 24 matched there, 24 now, 0 lost

Helper scripts (W/t): try.mjs (vc6-try with --score distances), mk.mjs (whole-function variants and edit chains
from a text file), run.sh, sd.mjs (normalised diff of a --side listing), apply.mjs.

## Worker 26

### m-1021e worker 26 (D24700-D24799)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| vsdk/effect_mem.cpp | 0x51dd00 | vfx_load | D24700 | the copy of the base's flag bits 1 and 2 written as two shorts sign-extended from 13 bits, then masked with ~3 / ~5 |

**D24700 (vsdk/effect_mem.cpp). vfx_load 0x51dd00 is exact.** The binary masks the flags word with 4 and with 2,
sign-extends each from 13 bits (`shl cx, 3; sar cx, 3`), then ANDs them with -4 and 0xfa before the OR and `<< 6`.
This form gives it:

```
short t = (short)((base->flags & 4) << 3) >> 3, u = (short)((base->flags & 2) << 3) >> 3;
vi->id_bits = (vi->id_bits & ~0x190) | (((t & ~3) | (u & ~5)) << 6);
```

The values are the same (t is 4 or 0, u is 2 or 0), so the behaviour does not change. A second form also matches:
a block-local `struct { short low : 13; short high : 3; }` temporary per bit (`t.low = bf->low & 4;` and
`(t.low & ~3) | (u.low & ~5)`, with bf a cast of &base->flags). I kept the plain-short form because it needs no
cast of the header field. The masks probably come from a 13-bit bit-field macro in the original. Without the
`& ~3` / `& ~5` (plain bit-field temporaries, `>> 2 << 2`, short casts), 8966 merges both masks into one `and 6`.

## Closer, kept (same behaviour, no match lost)

| file | address | name | old -> new | D |
|---|---|---|---|---|
| vsdk/unnamed_poly_split_0x55f110.cpp | 0x55f900 | gr_poly_split_edge_vertex | 191 -> 2 | D24703 |
| vsdk/effect_mem.cpp | 0x51cf60 | effects_load_bases | 73 -> 58 | D24701 |
| game/new_hud.cpp | 0x40e480 | hud_interact_nearest | 155 -> 135 | D24702 |

**D24703 (vsdk/unnamed_poly_split_0x55f110.cpp). gr_poly_split_edge_vertex 0x55f900: the pass data indexed in
place** (`n->pass[i].u = (b->pass[i].u - a->pass[i].u) * t + a->pass[i].u;` and so on), with no pa / pb / pn
pointer locals. 8966 then builds the binary's induction pointers (a's pass as the base, b and n as offsets from
it, b's colour bytes through a pointer kept in a's argument slot) and frame (0x44, g / b in the npasses / passes
slots). Size and instruction count now equal the binary's (498 bytes, 163 instructions). Only one difference is
left: the binary loads b's red byte (`mov dl, [eax-1]`) before `fstp g`, and ours loads it after the `fild` of
b. Tried without a change (all distance 2): the declaration order of r / g / bl / al (6 orders), a comma chain,
an `int` or `float` temporary for b's red (in a block, declared first, or assigned after the three loads),
`const float *tp = &t`, `float rr[1]` for r, and `r + (b - r) * t`. These were worse: other orders of the
r / g / b loads (12-124), `float gb[2]` (114), the colour lerps without the r / g / bl locals (177), the colour
locals declared in their blocks, pointer locals declared in the loop, and a `gr_tmapper_pass *` walking pointer
(the last three no change at 191).

**D24701 (vsdk/effect_mem.cpp). effects_load_bases 0x51cf60: `category` is not reset per line.** The binary
stores only `num = 0` before each effects_parse_arr_line call. NOTES-review-2026-10-20 says so too: the parser
sets *category before every successful return. The behaviour does not change. 73 -> 58. What is left: the
binary's zero register is EBP (zeroed at entry and again at the loop head, where EBP then becomes the names loop
index), ours is EDI. The binary also keeps the effects_name_order loop indexed (`mov [4*edx+...], edx` with EDX
zeroed between the memsets), where ours uses a pointer. Tried without a change: 7 declaration orders and
splits of i / j / found / num, `i = 0` before the memsets, a do / while, a while, `*(arr + i)`, an `(int)i`
cast, and a found / block-local unsigned / int counter for the order loop (58-60). num / line / category as
the counter were worse (59-73).

**D24702 (game/new_hud.cpp). hud_interact_nearest 0x40e480:** the object distance as `(cur_character->pos -
obj->pos).len()`, the vector3 inline the old comment pointed to. The binary computes the height test's
`cur.y - obj.y` again rather than spilling dy (155 -> 149). The final choice is one `if (best_obj && (!best_door
|| best_obj_dist < best_door_dist)) ... else if (best_door) ...` instead of the nested form with an early
return, which gives the binary's shared door tail (149 -> 135). The behaviour is the same in every case. The
unused dx / dy / dz locals are gone. Still differs: best_door is in the frame in the binary and in EBP for the
whole function in ours, so the binary pushes EDI / EBP only around the door and container loops and has no
`mov bl, 2` constant register. Tried without a gain: `(...).len_sq()` under sqrt (149), dx / dy / dz in a
block (155), the length written out without locals (171), `int i` in the game_flags block, separate i / j
loop counters, the obj loop pointer declared at the loop (149), `if / return; if` (135, the same), and
best_obj / best_door pairs as a local `struct { float dist; void *obj; }` in 3 declaration orders (139, still
enregistered).

## Still differs (tried; do not repeat)

- **game/new_hud.cpp 0x40f880 hud_portrait_get_color (38).** New this round, all with no change at the clamp:
  file-local inline clamp helpers in 10 shapes (`ubyte f(ubyte)` with if / else, return chains, ternaries and
  a result local; `int` parameter; `(v, lo, hi)` versions), each called as `v = f(v); c.g = v; c.b = v;` and as
  `c.g = c.b = f(v)`; `ubyte g[1]` (two shapes); a bool `lo` local for the range test; `g = v` copied and then
  `v = g`; `c.g = c.b = g`; a do / while (0) with breaks; `c.g = v = 0` in the else branch; a `ubyte z = 0` copy.
  The binary's clamp goes through CL (`mov cl, al` before the `jae`, `xor cl, cl; mov al, cl` for 0, the 255
  block last). This is register choice; I stopped.
- **game/new_hud.cpp 0x40d930 hud_handle_keys (198).** A `goto` to a label on the Esc case's 0x23 sound tail,
  from all five other 0x23 cases (230: 8966 then also merges the 0x27 tails, which the binary keeps apart), from
  Ctrl+L / Ctrl+S only (203), from I / Q / R only (198), and from Ctrl+S only (198). The binary keeps the shared
  tail at Esc; 8966 keeps the last copy.
- **vsdk/stringpool.cpp 0x544c40 StringPool::Add (22)** and **game/items.cpp 0x44a090 items_parse_table (54):**
  not attempted. The notes list them as register choice, exhausted (D19201 / D19202 and the 2026-10-21d note;
  2026-10-21c for items_parse_table), and no new device fits.

## Behaviour errors / bug candidates

None. D24701 drops a per-line `category = 0` store that the binary does not have. This is not a behaviour
change, because the parser writes *category before every successful return.

## Header changes

None.

## Changed files

- src/vsdk/effect_mem.cpp (vfx_load D24700, effects_load_bases D24701)
- src/game/new_hud.cpp (hud_interact_nearest D24702)
- src/vsdk/unnamed_poly_split_0x55f110.cpp (gr_poly_split_edge_vertex D24703)

## --check (VC6_STRICT=1 vc6-sweep against src/decomp/vc6-matches.txt)

- effect_mem: 34 matched there, 35 now, 0 lost
- new_hud: 57 matched there, 57 now, 0 lost
- unnamed_poly_split_0x55f110: 3 matched there, 3 now, 0 lost

## Worker 27

### m-1021e worker 27 (D24800-D24899)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| vsdk/unnamed_checkbox_0x553ba0.cpp | 0x553f00 | ui_checkbox::draw | D24800 | the cross's two gr_line calls through a file-local `static inline void cb_line(float x0, float y0, float x1, float y1)` that passes gr_line_mode |
| game/unnamed_mp_objects_0x47f860.cpp | 0x4801f0 | mp_handle_new_path | D24801 | v.x / v.z copied from the vector2 with `memcpy(&v.x, &p.x, 4); memcpy(&v.z, &p.y, 4);` |
| game/unnamed_alt_textures_0x4aac10.cpp | 0x4aac10 | alt_textures_parse_table | D24802 | base loop as `for (;;)` with two exits, each `tf.Close(); break;`; `n = 0` before GetPosition; the new array through a local |

**D24800 (vsdk/unnamed_checkbox_0x553ba0.cpp). ui_checkbox::draw 0x553f00 is exact.** The six gr_line calls of
the cross (three states) go through `static inline void cb_line(float x0, float y0, float x1, float y1) {
gr_line(x0, y0, x1, y1, gr_line_mode); }` (ours). The helper's call boundary gives 8966 the binary's order of the
far-corner `lea` / `mov ebp` and of the int-to-float temporaries' fild / fstp. Same calls and values. Other helper
shapes were worse: a whole-cross helper over ints (360), over ints with ex / ey locals (192), over four floats
(210), cb_line with the Mode as a fifth parameter (274). Device: **an inline helper around a call with
by-value class argument (gr::Mode) reorders the argument set-up.**

**D24801 (game/unnamed_mp_objects_0x47f860.cpp). mp_handle_new_path 0x4801f0 is exact (32 -> 0).** The node point
is still read into `vector2 p` with two memcpy, and v.x / v.z are now set with `memcpy(&v.x, &p.x, 4);
memcpy(&v.z, &p.y, 4);` instead of float assignments. p then needs no frame slot and its two dwords are pushed
straight from EAX / ECX, as in Sum.exe. `*(int *)&v.x = *(int *)&p.x` (with int reads of the packet) also
matches; memcpy kept for the file's style. Not matching: memcpy into v then `p.x = v.x` (121), `p =
vector2(v.x, v.z)` (98), one 8-byte memcpy into p (129), float reads through `*(float *)` (120), a `vector2 *pp`
(32), `vector2 p(v.x, v.z)` after memcpy into v (136). Same bytes copied; same behaviour.

**D24802 (game/unnamed_alt_textures_0x4aac10.cpp). alt_textures_parse_table 0x4aac10 is exact (54 -> 0).** Three
changes, same behaviour: (1) `alt_texture_set *a = (alt_texture_set *)operator new(...); alt_texture_lists[num] = a;
alt_texture_counts[num] = n;` (the index is read after the call, 54 -> 21; also with `void *`, `new
alt_texture_set[n]`, an index local); (2) `n = 0;` before `tf.GetPosition(&pos)` (the binary's `xor esi, esi`
before the call, 21 -> 19); (3) the base loop as `for (;;) { if (tf.Accept("#End")) { tf.Close(); break; } if
(tf.AtEnd() || num_alt_texture_bases >= MAX_ALT_TEXTURE_BASES) { tf.Close(); break; } ... }`: the Open-success
region then has two exits and 8966 pushes EBX / EBP at the entry instead of shrink-wrapping them after the Open
test (19 -> 0). The tests run in the same order with the same short-circuits, and Close runs once on every exit
as before. Not working for the push: `goto out` past the block, one `for (;;)` with one `break` or a `goto done`
to the Close, an if / else with an empty then, `do { if (open fails) break; ... } while (0)`, a bool or int
local for the Open result, an early `return` (75-77), `tf.Close(); return;` inside the loop (75), a `register` n.
Device: **a second exit (a duplicated `Close(); break;`) in a region stops 8966's shrink-wrap (D8701 /
D21702 again, here without an early return).**

## Closer, kept

| file | address | name | old -> new |
|---|---|---|---|
| game/new_buysell.cpp | 0x405260 | buysell_handle_click | 193 -> 173 |

**D24803 (game/new_buysell.cpp; closer only). buysell_handle_click 0x405260:** in both list-row branches, `shop_item
*it = buysell_get_list_item(buysell_selected_item); item_page_in(it->item_type, BUYSELL_PAGE_SLOT, 0);`. The
item lookup now runs before the 0 / 0x12 pushes and the two calls' stack clean-ups are combined (`add esp, 0x14`)
as in Sum.exe. An `int type` local instead is worse (204). Same calls, same order. Left: register choice (Sum.exe
keeps -1 in EBP through the button loop up to `gr_get_font_height(-1)` and i in EBX; ours puts the constant 4 in
BL and i in EBP; in the row loop Sum.exe has fh in ESI, rows in EBX, i in EBP, i * fh in EDI).

## Still differs (tried; do not repeat)

**D24804 (notes only).**

- game/unnamed_mp_objects_0x47f860.cpp 0x47fdf0 mp_send_new_lep (56; Sum.exe keeps the sent counter in ESI and
  pushes ESI): the header before info, info after the type (66), `sent++` first with the header split, the header
  split with info between, `int sent` loaded at the top and stored after the type (with `sent++` separately and
  `pt->sent = ++sent`), a `register int sent`, `register int info`, an unused `int *` to the counter, an
  `mp_packet_info *pt` for the bump, MP_ADD_FLOAT for the four floats, a `Character *` copy, info / f and data /
  packet_size declaration orders: all 56 except the marked. Register allocation; stop.
- game/spells.cpp 0x47a190 spells_parse_table (39; EBX / EDI swapped for cls and the record pointer, and Sum.exe's
  EDX for every effect branch's `&tf`): `register int cls`, a `register spell_info *rec` for the class store and
  `first`, the class store through an `int *`, `spell_group *g`, prev_class stored before the count++, `prev_class
  != cls`, the class stored first and read back into cls, prev_class from the record (42), every group access
  through the record (69), an inline `spells_add_quoted(TextFile *)` for the effect names (with and without the
  name), a `TextFile *tfp` local for AddQuoted. All 39 except the marked.
- game/spells.cpp 0x47f2a0 spell_target_value (164): 8966 merges every `return 1.0f` into the final default
  block and falls through to `return 0.0f`; Sum.exe merges the 0.0 returns forward (heal's into cure's
  fall-through, cure's 1.0 into power leech's) and keeps the default's 1.0 separate. Tried: heal as `if (max <= hp)
  return 0; return 1;` and `if (!(max > hp))`, `default: return 1.0f;` inside the switch, power leech as `if (ap >
  0) return 1;`, cure with `else return 0`, and combinations (164-165). Source polarity does not move the blocks.
- game/spells.cpp 0x47e900 summon_create (255): the ring xp chain's repeated get_int tails are cross-jumped by
  8966 (ours pushes the string and jumps to one shared `push 4 / call / mov esi, eax`); Sum.exe keeps every
  branch whole. Tried: the chain in a `static inline int summon_ring_xp(int species)` with returns (else-if and
  plain ifs: not inlined, 352) and `static __forceinline` (255, merged the same).
- game/new_buysell.cpp 0x405260 buysell_handle_click (173 after D24803): an `int none = -1` (plain, const,
  declared first) for both stores and the font height, chained stores in both orders, the 4 tests split / as `== 0` /
  through `buysell_buttons[i]` / an int copy of b->flags, `const ui_button_def *`, `register int i`, a separate
  row-loop index, rows / fh declared at the top in two orders, fh declared then assigned: all 173 (8966 folds the -1
  everywhere).

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/vsdk/unnamed_checkbox_0x553ba0.cpp (cb_line helper, ui_checkbox::draw; D24800)
- src/game/unnamed_mp_objects_0x47f860.cpp (mp_handle_new_path; D24801)
- src/game/unnamed_alt_textures_0x4aac10.cpp (alt_textures_parse_table; D24802)
- src/game/new_buysell.cpp (buysell_handle_click, closer; D24803)

## --check results (VC6_STRICT=1 vc6-sweep --check src/decomp/vc6-matches.txt)

- unnamed_checkbox: 11 matched there, 12 now, 0 lost
- unnamed_alt_textures + unnamed_mp_objects: 30 matched there, 32 now, 0 lost
- all four units together (new_buysell, unnamed_checkbox, unnamed_alt_textures, unnamed_mp_objects): 82 matched
  there, 85 now, 0 lost

## Worker 28

### m-1021e worker 28 (D24900-D24999)

Batch: seg2d_intersect, ix_box_oriented_box_oriented, ix_sphere_polygon_edge, ai_state::serpent_illusion_start,
serpent_anim_remap_in, TextFile::ParseString, ui_container::draw, ai_group_think_caster.
Result: 2 new matches, 2 closer, 4 unchanged. No header changes.

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| src/game/unnamed_ai_groups_0x492b00.cpp | 0x4936c0 | ai_group_think_caster | D24900 | 194 -> 0: `int has_ap`, `count = 0` moved into the rating block, `foe` declared before `own`, the score computed before the flag / count updates |
| src/vsdk/parse.cpp | 0x5166b0 | TextFile::ParseString | D24901 | 24 -> 0: `int len = 0;` before `cursor++` |

**D24900 (game/unnamed_ai_groups_0x492b00.cpp). ai_group_think_caster 0x4936c0 is exact.** Four changes, each
measured:
- `bool has_ap` -> `int has_ap` (194 -> 164): the binary keeps the result as 0 / 1 in EDI (`mov edi, 1` /
  `xor edi, edi`, `cmp edi, ebx`), not in BL.
- `count = 0;` moved from before the first member loop to the top of the `foe / own > 3` block (164 -> 14). With
  the early store 8966 keeps count in memory and a separate zero in EBP; with the late store count lives in EBX
  from the start (the binary's `xor ebx, ebx` zero is count) and ai goes to EBP. count is not read before that
  block, so behaviour is unchanged. (A general device: an initialisation moved next to its first use gives a
  variable the register the binary uses as its zero constant.)
- `float foe = 0.0f, own = 0.0f;` (14 -> 12): the binary stores foe's 0 first; the slots stay the same.
- `float score = ...` computed right after `n` and before `list[i].flags |= 4; count++;` (12 -> 0): the binary
  loads n first (`fild n; fmul 10`) and keeps frac / n in the binary's slots. (`float frac[1]` also fixed the
  slots, 12 -> 8, but is not needed with the moved score.)
FIXED_BUILD: the C210 block is untouched (count is still reset at the top of the rating block in both builds).

**D24901 (vsdk/parse.cpp). TextFile::ParseString 0x5166b0 is exact** with `int len = 0;` written before
`cursor++` (and `for (; ...)`). len is then live in ESI across the increment, so the incremented cursor takes
EBX (pushed in the prologue, as the binary), and the shared `push esi` before the size test follows. The line
count of the function is unchanged (VSDK_ERROR line numbers of later functions depend on it).
Caution for later rounds: TextFile::ParseDelimitedString (0x516630), just above, has the same first twelve lines;
a vc6-try / variant whose `from` is that text edits ParseDelimitedString, not ParseString (my first 14 forms
did, and the file lost a match). Anchor variants on the ParseString signature.

## Closer, kept

| file | address | name | old -> new | D |
|---|---|---|---|---|
| src/vsdk/unnamed_ix_0x53ac20.cpp | 0x53c610 | ix_box_oriented_box_oriented | 128 -> 32 | D24902 |
| src/vsdk/unnamed_ix_0x53ac20.cpp | 0x53bb50 | ix_sphere_polygon_edge | 193 -> 126 | D24903 |

**D24902 (ix_box_oriented_box_oriented 0x53c610; closer only).** Unlike D21906 (not merged: products stored into
float locals can round), every added local here is an exact copy of a single float or pointer, never a product:
- Bf read through `const float (*F)[3] = Bf;` (D21906's first step, 128 -> 82);
- T copies: `const vector3 *T5 = T;` in test 5; `float tz6 = T->z` (and `tz6b = tz6`, `ty6 = T->y`) in test 6,
  `tz8`, `ty9`, `b20 = B[2][0]`, `tx15 = T->x` (82 -> 60, D21906's q copies);
- single-element copies of F found by a greedy search (try/bxsearch.mjs, SCALAR=1; then pairs, try/bxpairs.mjs):
  `bf00` (test 2), `bf02` (test 6), `bf20` (test 7), `bf21` (test 8), `bf22` (test 9), `bf12` (test 10), `bf22b`
  (test 13) (60 -> 32). 8966 propagates the copies (no stack slot), they only change its operand ranking.
Pointer copies of a / b / T / F per term (160 variants) gave nothing. Left (6 places): test 2's `T->z * B[2][0]`
first, test 5's `T->x * B[0][1]` operand order, test 6's `B[0][2] * T->x` operand order, tests 7 / 9's first two
terms, tests 11 / 12's `F * b->x` third. Pairs of scalar copies on those lines: one gain (34 -> 32), the rest 32-78.

**D24903 (ix_sphere_polygon_edge 0x53bb50; closer only).** The edge hit (`near_pt = *a + e * u`) and the vertex hit
(`near_pt = *a`) share one `found:` tail (`*t = t0; *hit = near_pt; return true;`): 193 -> 126, and a / d now take
the binary's EDI / ESI. The binary goes further: the vertex path jumps into the edge path's `near_pt` assignment
(the reverse of ours), the edge block lies after the vertex block, and the `t0 < 0` branch lies after the best test.
8966 lays the blocks out from the flow graph: every goto / label reordering with the same graph gave the identical
126 (edge after vertex with goto edge, three forms; `t0 >= best`; the early branch at a label after the best test,
three forms; split `||` tests; `t1 >= 0` goto). `else if (t0 >= 0.0f) {...} else {B}` gives the binary's branch
sense for the chain but drops 6 instructions (138, three forms). Not kept.

## Still differs (tried; do not repeat)

- **ai_state::serpent_illusion_start 0x496ba0 (34, D24904, notes only).** The rotation is EAX -> ECX -> EDX -> EAX
  in every block after the `rem` tests (ours EAX / ECX / EDX where the binary has ECX / EDX / EAX). Finding: writing
  the zero path first (`if (rem <= 0) { zero ... } else { alpha }`, also `!(rem > 0)`, `0 >= rem`, a bool, a
  do/while break, goto forms, nested in `rem <= 2000`) gives exactly the binary's registers in all three blocks but
  lays the zero block first (`jg` instead of `jle`, 42). Every form with the alpha block first keeps the rotation
  (34): rem / p / a / i declared at the top or per loop, `a /= 1000` split, a character_instance local, one exit
  (else instead of return), `rem <= 2000` wrapper, `rem > 0 ... else` with returns, goto to a fade label, `rem >= 1`
  (39), `1 + count`. Not found.
- **serpent_anim_remap_in 0x495d00 (36, D24905, notes only).** Binary: slot base in EDX, weight base in ESI, temp
  in ECX; ours ECX / EDX / ESI. Tried (36 unless noted): unsigned i with `(int)i < n`, `a` at the top / first /
  `register`, i declared at the top, `int i = 0` in five positions (before / after ready, before the count store,
  at the top 38, chained with ready), a slot pointer, a byte-offset read, do/while, `set->count > i` (38), the
  weight store through one `goto store` (74), a mount copy, a set copy.
- **TextFile::ParseString**: matched (D24901).
- **ui_container::draw 0x553800 (56, D24906, notes only).** Binary slots: sx -36, text_x -32, text_y -28, sy -24,
  text_w -20, text_h -8 (right shares it) and bottom -4; ours sx, sy, text_x, text_y with right / bottom on
  text_x / text_y. Tried (56 unless noted): text_h in an inner block (78), text_x / text_y in a block (also with
  text_h, 82), right / bottom in a block, text_x / text_y declared before sy, all four declared at the top in the
  binary's slot order, sy from an int after the size call (60), a `const float *` for text_y and for sx.
- **seg2d_intersect 0x53d8f0 (62, D24907, notes only).** Not tried: D15306 stands (the binary reuses the c / d
  argument slots as scalars, which needs float parameters, a header and signature change; the only caller,
  fx_find_spawn_point, is not matched).

## Behaviour errors / bug candidates

None found. All kept changes are behaviour-neutral: has_ap int vs bool (only tested), count's reset moved past
code that does not read it, declaration order, the score computed earlier (no side effects), `len = 0` earlier,
exact float / pointer copies, and a shared return tail.

## Header changes

None.

## Changed files

- src/game/unnamed_ai_groups_0x492b00.cpp (ai_group_think_caster, D24900)
- src/vsdk/parse.cpp (TextFile::ParseString, D24901)
- src/vsdk/unnamed_ix_0x53ac20.cpp (ix_box_oriented_box_oriented D24902, ix_sphere_polygon_edge D24903)

## --check results (VC6_STRICT=1 vc6-sweep --check src/decomp/vc6-matches.txt)

- unnamed_ai_groups_0x492b00: 17 matched there, 18 now, 0 lost
- vsdk/parse.cpp: 49 matched there, 50 now, 0 lost
- unnamed_ix_0x53ac20: 31 matched there, 31 now, 0 lost

Helper scripts (W/try): v.mjs (distance per variant), side.mjs (full-operand side-by-side, removes its compile
folder file by file), ndiff.mjs (LCS diff with the frame symbols resolved), bxsearch.mjs / bxpairs.mjs (copy
searches), gen_*.mjs (variants written with String.raw, since Git Bash heredocs drop backslashes).

## Worker 29

### W29 notes (m-1021e, decisions D25000-D25099)

Distances are vc6-compare --score (2026-10-21 scoring). Note: for functions with a jump table or EH funclets
after the body, part of the distance is the linear listing of the table / funclets, not real code.

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/summoner.cpp | 0x43e250 | summoner_cfg_load | D25000 | the sfx volume copied with `memcpy` into a one-float array (`float vv[1]; memcpy(vv, &Summoner_options.sfx_volume, sizeof(float)); snd_set_group_volume(0, vv[0]);`) |
| game/multisave.cpp | 0x46d0b0 | mp_player_file_write | D25001 | the inventory type index through an int local (`int t = party_inventory[i].item->type - item_types; f.write_int(t);`) |

**D25000 (game/summoner.cpp). summoner_cfg_load 0x43e250 is exact.** All that differed was the tail's register
rotation from `loaded:` on (ours ECX / EDX / EAX / ECX, the binary EAX / ECX / EDX / EAX; D19705, D16506). A
`float vv[1]` filled by `memcpy` from the sfx volume and passed as `vv[0]`: the copy is still the binary's
integer `mov eax, [sfx_volume]; push eax`, and it shifts every later scratch register by one, into the binary's
order. Behaviour unchanged (same value passed). Other forms of this round: a one-float array with a plain
assignment (`vv[0] = sfx_volume`, gives `fld / fstp`, 14), the array at function scope (14), a float scalar copy
(14), an `int vv[1]` copy through `*(int *)&` (also exact, not kept: the memcpy form says the same without a
cast), the array for music_volume (22), both volumes in a `float vv[2]` (39), a `const float *` to the volume
(27), a `SummonerOptions *` (27), file-local inline helpers for the pxo flags or the two volume calls (27), a
`unsigned int *` for pxo_cfg_flags (27), an `unsigned int fl` local for the flag update (26), a bool local for
snd_is_eax_enabled (29). NEW DEVICE: a `memcpy` into a one-element array burns one scratch register without
changing the code, to shift a register rotation that is one step out.

**D25001 (game/multisave.cpp). mp_player_file_write 0x46d0b0 is exact.** The inventory loop's scratch registers
(D16505's "register rotation in the inventory loop", NOTES-match-2026-10-21c) come right when the type index is
first stored in an `int t` and then written; the loop and the rest of the function (the chain loop's
`push ecx / lea ecx, f`, the hot-button loop) follow. `ItemType *ty = party_inventory[i].item->type; f.write_int(ty
- item_types);` is also exact (not kept). Closer only: an `item_info *it` local (18), a memcpy of the count (18), a
charges local (26). No change: an entry pointer, a `File *fp`, an unsigned index.

## Closer, kept (old -> new distance)

**D25002 (game/gameplay.cpp; closer). state_game_frame 0x438f30: 109 -> 97.** The binary keeps one frame float
(at esp+4) for the camera fov passed to gr_setup_3d and for the fx_frame frame times, stores every value into it
(dead stores included) and pushes a register copy (`mov eax, fov; push 1; mov ecx, eax; push 1; push ecx; ...;
mov [esp+0x18], eax`). Written as one `float frametime;` declared after frame_label, its address taken by an
unused `float *ftp = &frametime;` (without it 8966 uses `fld / fstp` and drops the stores), `frametime =
camera_fov;` before gr_setup_3d: the camera block, the dead stores and the normal branch's memory round trip are
now as in the binary. Also the render test is `if (gameplay_no_render_60a630 != 1) game_render_frame(); else
frame_sleep_when_inactive(0, 1);` (the binary tests `je` to the sleep call). Left: the paused / cutscene
branches' scratch registers (the binary copies into ECX every time: EDX->ECX, EAX->ECX, EDX->ECX, EAX->ECX;
ours rotates EDX->EAX, ECX->EDX, EAX->ECX, EDX->EAX), and the interface switch's index register (ECX vs EDX).
Tried for that: `fx_frame(*ftp, ...)`, `*ftp = ...` stores, `fx_frame(frametime = x, ...)`, an inline
`game_fx_frame(float, bool)` helper (with / without gr_setup_3d helper: fld/fstp forms, 71-110), helpers taking
`float &` / `float *` (105), a `float a = frametime;` copy (75, loses the stores), `float frametime[1]` (104),
memcpy into frametime (removes the dead stores, 71), the globals passed directly with the stores kept (87, loses
the copies), `*(int *)ftp = *(int *)&global` on any subset of the four branch stores (67-94, each removes that
branch's store and copy; not kept: not the binary's shape), the declarations at the top or the pointer at the
end (97), `float *const`, volatile (107).

**D25003 (game/unnamed_shadows_0x4a37d0.cpp; closer). obstacles_debug_render 0x4a3890: 188 -> 20.** The segment
loop indexes everything through sl: `ia = sl->segs[k].a; ib = sl->segs[k].b;`, `sl->corners[ia].pos.x`, ...,
`0.5f * sl->segs[k].normal`, with no seg or corner pointer locals (the binary keeps `edi = sl - 4` and addresses
`[edi + 4*ia*17 + 0x270]`, the corner base without the displacement folded in, and the seg pointer unbiased);
and the midpoint as `sl->corners[ia].pos + sl->corners[ib].pos` (the binary loads b's x first). The `s`, `ca`,
`cb` locals are gone. Left: ESI / EDI swapped for cell / edge in the first (cell) loop. Tried for that (no
change): unsigned edge index, a pf_graph pointer, do / while and while forms of the cell loop, the edge increment
in the body, a cell copy, a block-scoped cell or edge, `register`, `e = ..., i = 0` in the for, a pf_cell ** to
the list head, a block-scoped wall pointer, the wall pointer also assigned to e, the layer test as if / else,
declaration order cell / e; worse: an edge-count local (36), `cell->edges[i]` indexing (258), a pointer copy of
e (208). Steps on the way: ia / ib before the corner pointers (182), ib first (152), the seg walked by pointer
(22), `sl->segs + k` (188), `s->a` inside the corner index (315).

**D25004 (game/s3d.cpp; closer). s3d_read_meshes 0x46ebe0: 200 -> 178.** The orientation check reads
`mesh->orient.` directly (the `matrix *m` local is gone; 2026-10-20b listed this as closer and did not keep it).
Left: the binary keeps `untextured` in a frame byte (esp+0x13 / 0x17) and with it num_faces and then the flags
pointer in EBX; ours has untextured in BL. Tried: `volatile bool` (174 with the orient change, but the loop shape
then differs; volatile is accepted only for an exact match, not kept), an unused `bool *` to it, the pointer used
in the loop test / store, `bool untextured[1]`, an inline helper taking `bool *` (inlined and enregistered: no
change), an `unsigned int *fl` for the flag updates (no change), lightmap_boxes = NULL after faces (198).

## Still differs (tried; do not repeat)

**D25005 (notes only). Tried without a match:**
- state_game_enter 0x438130 (45): only the default case's block placement (the binary puts `new_game = false;
  jmp back` after the level_load failure block). 8966 ignores the source position: a `goto not_new` to a block
  written after the failure block is moved back too. Also tried (all 45): both failure tails through gotos into
  the second failure block (the binary's tail merge), a bool local for level_load, `== false`, `*(&new_game)`, a
  prev-state int local, `do switch ... while (0)`, the case 2 / 14 body ending in `goto entered`; `unsigned char
  new_game` is worse (52).
- lobj_raycast_draw_lists 0x4a4bb0 (98): ESI / EDI (hit vs the zero / counter web) and one `pop edi` placement.
  Tried (98): an inline helper for the three hit stores, `register lobj_hit *h`, a hit pointer copy for the
  stores, an `int *` zero store, unsigned index with `(int)i <`, `float fog_far[1]`, a separate `j` for the
  first loop, an `int zero` local, `== NULL` tests; worse: the stores before / after the index setup (120), the
  blocked setup before the index setup (115).
- shadow_transform_to_lobj 0x4a4890 (183): which inline vector3 members are called out of line (the binary
  calls operator= 0x413190 for the three local_orient rows, the (x, y, z) constructor 0x43b580 and the copy
  0x410cd0 in the non-door path). Padding the door pose with nested `vector3(vector3(...))` copies (1-24) or with
  1-20 unused `vector3 padK(door->open_pos)` locals only moves the NESTED copies out of line (the returned copy
  of operator= becomes a call to 0x410cd0), never the top-level operator= or (x, y, z) constructor: 184-317.
  Also `vector3 a = ..., b = ...; pos = a + b;` (237), copies in `s->pos - pos` (205), `open_pos * 1.0f` (213).

## Behaviour errors / bug candidates

None found. D25000-D25004 keep every value, store and call (D25003's midpoint sum a + b instead of b + a gives
the same float).

## Header changes

None.

## Changed files

- src/game/summoner.cpp (summoner_cfg_load, D25000)
- src/game/multisave.cpp (mp_player_file_write, D25001)
- src/game/gameplay.cpp (state_game_frame, D25002)
- src/game/unnamed_shadows_0x4a37d0.cpp (obstacles_debug_render, D25003)
- src/game/s3d.cpp (s3d_read_meshes, D25004)

## --check results (VC6_STRICT=1 vc6-sweep --check src/decomp/vc6-matches.txt)

- game/summoner.cpp: 14 match, 0 differ, 1 not found (WinMain); 13 matched there, 14 now, 0 lost
- game/multisave.cpp: 8 match; 7 matched there, 8 now, 0 lost
- game/gameplay.cpp: 37 match, 2 differ; 37 matched there, 37 now, 0 lost
- game/unnamed_shadows_0x4a37d0.cpp: 9 match, 4 differ; 9 matched there, 9 now, 0 lost
- game/s3d.cpp: 33 match, 1 differ; 33 matched there, 33 now, 0 lost

## Tools (in W, not for the repo)

try.mjs (variants with the distance), side.mjs (full side-by-side; runs vc6-compare --keep with TEMP pointed at
W/tmp, so the kept compile folders stay in W/tmp and nothing goes to the system Temp), regdiff.mjs (side lines
whose registers differ, which vc6-side does not star).

## Worker 30

### W30 notes (m-1021e, decisions D25100-D25199)

## New matches

| file | address | function | D | what did it |
|---|---|---|---|---|
| game/unnamed_pfgeom_0x4a4ee0.cpp | 0x4a4fb0 | pf_segment_hits_walls | D25101 | float_min / float_max for the bounds, block-scope `const vector2 *eb = e->b, *ea = e->a;` (b declared first), obstacle loop over an int index |

**D25101 (game/unnamed_pfgeom_0x4a4ee0.cpp; match). pf_segment_hits_walls 0x4a4fb0 (116 -> 0, exact).** Three
changes, all with the same behaviour:
- The segment bounds `min_x = float_min(a->x, b->x)` (and float_max, vsdk/math/vector.h) instead of the four
  ternaries. The binary's `fld b; fld a; fst tmp; fcomp st(1)` is the inline function with by-value arguments
  (evaluated right to left; the D6078 note on rect2d_add_point). 116 -> 89.
- The obstacle loop `for (i = 0; i < AVOID_NUM_OBSTACLES; i++) { ob = &avoid_obstacles[i]; ...`: the binary ends
  with `cmp esi, end; jl` (a signed test, so an int index strength-reduced to the pointer), not the pointer
  loop's `jb`. 89 -> 84.
- The edge bounds also through float_min / float_max, but on block-scope pointer locals
  `const vector2 *eb = e->b, *ea = e->a;`. With `e->a->x` directly 8966 keeps e->b->x in a stack temporary
  for the next min / max (131); the pointer locals stop that CSE (21); declaring eb before ea swaps ECX / EDX
  into the binary's order (0). Function-scope pointer locals give 21 in one order and 0 with `eb = e->b`
  assigned first; the block-scope declaration was kept.
  Tried on the way (worse): all 15 mixes of ternary / float_min for the four edge bounds (131-168), the edge
  bounds in a `float eb[4]` (163), the four edge-bound declarations reordered or moved into the loop (131).

## Closer, kept

| file | address | function | D | old -> new distance |
|---|---|---|---|---|
| game/unnamed_pfgeom_0x4a4ee0.cpp | 0x4a5750 | seg2d_segment_intersect | D25100 | 108 -> 36 |
| game/unnamed_pfgeom_0x4a4ee0.cpp | 0x4a6060 | pf_grid_cells_along_segment | D25102 | 199 -> 171 |
| game/multispell.cpp | 0x4affb0 | mp_read_spell_targets | D25103 | 59 -> 54 |

**D25100 (game/unnamed_pfgeom_0x4a4ee0.cpp; closer, kept). seg2d_segment_intersect 0x4a5750 (108 -> 36).**
- The vertical (dax == 0) collinear case ends in its own `if (...) { lo; return 2; } else { hi; return 2; }`.
  With one shared `*out = *hi; return 2;` after both cases 8966 cross-jumped the vertical case's distance test
  (call ftol ... jge) into the horizontal one's (`jmp $L`); the binary has both written out and the horizontal
  case falls into the shared hi tail. 108 -> 52. (Giving the horizontal case its own else too: 77; both: 52.)
- a and b are block copies of the arguments (`seg2d_segment_intersect(const seg2d *a_, const seg2d *b_, ..)
  { const seg2d *a = a_; const seg2d *b = b_;`): the vertical case's two ftol calls come in the binary's order
  (distance to lo first). 52 -> 46. Copying only a or only b: 52.
- The end-point order tests read b's end points (`if (b->a->y > b->b->y)`, same for x) instead of lo / hi
  (which hold exactly those at that point): the binary compares through ECX, the b->a it just loaded. 46 -> 42.
- The horizontal case's distance test goes through `t = a->b;` (t is dead there): the two ftol calls in the
  binary's order. 42 -> 36. (The same for the vertical case: no further change.)
- Left: a in EBX / b in EBP where the binary has b in EBX / a in EBP (all instructions otherwise equal).
  Tried for that (no change, 46 or worse): `register` on either / both arguments or copies, the copies in
  either order, other parameter names (s1 / s2, sa / sb, x1 / x2, seg / other, a0 / b0), `q = b->a` before
  `p = a->a` (126), the shared-endpoint tests written without p / q / t (118 before, 46 after), the
  comparisons written `q->x == p->x` (84), `const vector2 *` locals, local declaration orders (5),
  `abs(hi) > abs(lo)` and int locals for the two distances (no change), a `p` / `q` local for a->b in the
  distance tests (46 / 52), dax .. cy through p / q / t (36), dbx / dby computed first (52), the lo / hi swap
  through `t = lo; lo = hi; hi = t;` (97).

**D25102 (game/unnamed_pfgeom_0x4a4ee0.cpp; closer, kept). pf_grid_cells_along_segment 0x4a6060 (199 -> 171).**
- Case 1's first branch (`-sx < sy`) adds s before the cell steps (`px += s; fx += s; cy++; by++; fy = 0;
  py = cy;`). In the old order it was the same source as case 3's first branch and 8966 cross-jumped one into
  the other (`jmp $L4817`); the binary has both written out with identical code (the scheduler gives the new
  order the same instructions). 199 -> 176. The other orders of cy++ / by++ / fy = 0 there: 199-204;
  `fx += s` before `px += s`: 180.
- Case 0 tests `-sx < -sy` (was `-sy > -sx`, same result): the binary negates sx first. 176 -> 171.
- The switch is now instruction for instruction the binary's (sizes equal, 1100 bytes). Left: the frame
  (0x34 against 0x3c: the binary keeps n's two halves in separate slots, dir apart from ex, ay apart from by),
  grid / cx / cy in ESI / EDI / EBX against EBX / ESI / EDI, and the binary sets bx / by after the loop's zero
  test. Tried (all 171 or worse): n split into nx + ny (four forms, also a separate step counter), declaration
  orders of the floats (6) and ints, `register` on cx / cy (both orders), a grid copy, `register` grid,
  bx / by as a for-init (171), in an `if (n) { ...; do .. while (--n); }` (185), before the hash lookup or the
  dir computation (213), by before bx (171).

**D25103 (game/multispell.cpp; closer, kept). mp_read_spell_targets 0x4affb0 (59 -> 54): case 10's loop runs
`for (i = 5; i < 10; i++) { GET(t[i - 5]); GET(t[i]); }`.** 8966 then strength-reduces on the second half
(`lea ecx, [base + 0x14]; add ecx, 4; [ecx-0x18] / [ecx-4]`) as the binary does. Same behaviour (same reads in
the same order). The same form for case 4 (`i = 6 .. 11`, t[i] and t[i - 6]) is worse (88), as are
`(t - 6)[i]` (84) and a q walk (84); `(t + 6)[i]` and a handle local: no change (54). Also tried: unsigned i
with `(int)i < n` (59), no t local (req->params.target, 84), p->target without t (84). Left: Sum.exe keeps req
itself in EDI and uses it as case 4's loop pointer (`add edi, 0x44`).

## Still differs (tried; do not repeat)

**D25104 (notes only). mp_handle_actv_gm 0x470860 (27).** Not retried: the only difference left is the list
unlink's `lea eax, [ebp+4]` (&e->prev kept in a register), which earlier rounds (D7006, D15703, D20000,
D18802 for mp_player_alloc, pathvis_add_edge_node) found no source form for; every pointer / reference /
inline-helper form folds back to `[ebp+4]`.

**D25105 (notes only). vfx_instance::process 0x536fb0 (48).** The 28 register-differing lines are eax / ecx /
edx rotations of the call arguments (vif_emitter_update, update_light, fx_rec2c::update, get_transform).
Tried on top of the earlier rounds' list: a `const matrix *o = orient` copy (48), a `const vector3 *p = pos`
copy (48), an int copy of _unk0c (48), a `vfx_desc21c *` local for the desc (48); for update_light a copy of
orient or pos (47, the copy is not the binary's form, not kept); a `vfx_dummy_desc *` base pointer for the
dummies (49).

**D25106 (notes only). vfx_instance::init_nodes 0x536a70 (88).** Looked at: `this` in EDI / the walk pointers
in ESI against the binary's reverse (EDI pushed late in the binary). Earlier rounds' forms stand; no new form
tried beyond reading the listing.

**D25107 (notes only). character_instance::increment_tick 0x51a230 (212).** 280 of 357 lines differ in
registers (num in EBX against EBP, skels, the first loop's base / index); the binary also stores trigger_anim
before the three float zeros. Tried: the float initialisations moved after `trigger_anim = -1` in two orders
(218 / 210) and one zero after get_active_anims (218). Not kept (210 is not the binary's form for a gain of 2).
Large; not attempted further.

## Behaviour errors / bug candidates

None found. (seg2d_segment_intersect's b->b report for a->b == b->a stays documented as a game bug.)

## Header changes

None.

## Changed files

- src/game/unnamed_pfgeom_0x4a4ee0.cpp (pf_segment_hits_walls D25101, exact; seg2d_segment_intersect D25100;
  pf_grid_cells_along_segment D25102; D-numbers in the function comments)
- src/game/multispell.cpp (mp_read_spell_targets D25103)

## --check results

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "unnamed_pfgeom_0x4a4ee0|multispell" --check
src/decomp/vc6-matches.txt`: 2 units: 42 match, 0 wrong target, 4 differ, 0 not found, 0 errors; 41 matched
there, 42 now, 0 lost.
