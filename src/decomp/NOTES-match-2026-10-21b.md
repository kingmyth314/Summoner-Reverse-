# Byte-match round 2026-10-21, second: the twelve workers' notes

The 92 functions that `tools/vc6-nearmiss.mjs` ranked at distance 13-40 after the first round (plus
`write_item_objects`), in twelve disjoint batches of whole files, with the first round's devices listed for the workers.
Result: 37 new matches, then two more at the merge (`inventory_destroy_selected_item` with D18500, and
`Character::Character` with W9's D19000 patch, which drops the D1 union): strict 5,462 -> 5,501, none lost. 16 closer.
The decisions are D18200-D19303 in include/DECISIONS.md.

## Worker 1

### W1 notes (targeted round 2026-10-21b, decisions D18200-D18206)

Batch: the quaternion unit (vsdk/unnamed_quaternion_0x5225b0.cpp), eight functions. The unit goes from 21 / 32 to
28 / 32 strict matches (7 new: 6 of the batch plus to_matrix_b, which was not in the batch).

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| vsdk/unnamed_quaternion_0x5225b0.cpp | 0x523370 | cquat::log_in_place | D18200 | the raw components copied (`float tx = fx, ty = fy, tz = fz;`) after the atan2 and used in the products |
| vsdk/unnamed_quaternion_0x5225b0.cpp | 0x523120 | cquat::log | D18201 | the same tx / ty / tz copies before the return |
| vsdk/unnamed_quaternion_0x5225b0.cpp | 0x522ea0 | quaternion::exp | D18202 | the length into s, then `float a = s;` (s is then reassigned in the branches) |
| vsdk/unnamed_quaternion_0x5225b0.cpp | 0x5231e0 | cquat::exp | D18203 | D18202 plus the D18200 copies |
| vsdk/unnamed_quaternion_0x5225b0.cpp | 0x523430 | cquat::exp_in_place | D18204 | D18202 plus the D18200 copies |
| vsdk/unnamed_quaternion_0x5225b0.cpp | 0x522bf0 | quaternion::to_matrix | D18205 | `const float *pw = &w;` with `*pw * z` / `*pw * x`, and the position zeros through `volatile float *p = m->m[3]` |
| vsdk/unnamed_quaternion_0x5225b0.cpp | 0x522cc0 | quaternion::to_matrix_b (not in the batch) | D18206 | the pointer to w of D18205 |

**D18200 (cquat::log_in_place 0x523370).** The binary keeps s on the FPU stack to the end (popped after the w
store), multiplies into fx's register (`fxch st(1); fmul st, st(1)`) and reloads fy / fz (`fld [fy]; fmul st,
st(1)`); ours used s as the destination and fy / fz as memory operands. The 8 factor orders, pointer locals to fy /
fz, a copy of s, a reference to s, product locals, a this pointer, a comma chain and the w store first change
nothing; a copy of fx alone gives the fxch (15 -> 12), copies of fx and fz 4, fx and fy 8, all three: match. Copies
declared together or one before each statement: both match (the together form kept).

**D18201 (cquat::log 0x523120).** D18200's three copies before the return: match (fx only 8, fy / fz only 9).
Before this, 15 declaration orders of fx / fy / fz / sx / sy / sz / s (separate from the initialisations) all gave 17.

**D18202 (quaternion::exp 0x522ea0).** The binary keeps the square root on the stack for the compare (`fstp st(3)`,
`fst [esp]`, `fcomp`) and reads a from memory afterwards. Of the 16 forms {sqrt into s or a} x {compare on s or a}
x {sin of s or a} x {divide by s or a}, only `float s = sqrt(...); float a = s; if (a > eps) s = sin(a) / a;`
matches; with s defined first the others give 15, with a defined first 25. Also tried (all 21-29): the assignment
in the if, s declared first, s initialised to 1, a cos local, a ternary, a result quaternion local (46), a
`(float)(sin / a)`, a copy of a after the sqrt / after the if / in the branch, a bool / int compare local,
`register`, a pointer to a, a one-float array, the branches inverted (29), a this pointer, a length-squared local.
Compiler flags (no /Ow, /O2, /Oa, /Oy-, /Op, /G5, /G6, /Os, /Og /Oi /Ot) do not give it either.

**D18203 / D18204 (cquat::exp 0x5231e0, cquat::exp_in_place 0x523430).** D18202's s / a form gives 17 / 19, the
D18200 copies alone 39 / 35; both together: match.

**D18205 (quaternion::to_matrix 0x522bf0).** Two differences: w loaded after z / x in `w * z` and `w * x` (the
factor order of D17805), and the three integer zero stores of the position, which 8966 scheduled into the FP code
and the binary leaves at the end. A pointer local to w (`*pw * z`, either factor order) fixes the first (14 -> 6;
a this pointer, pointers to z / x and a w copy do not: 14 / 14 / 50). For the zeros: chained, reversed, through a
float pointer, through a vector3 pointer, as ints, through a zero local, before m[2][2], through a matrix43 copy,
a loop (37), `*p++` (44), zeros first (20): all 6 or worse; through a `volatile float *`: match. The volatile is a
codegen device only (the stores are the same three dword stores of 0).

**D18206 (quaternion::to_matrix_b 0x522cc0).** The pointer to w of D18205: match.

## Closer, kept

None besides the matches.

## Still differs (tried; do not repeat)

- cquat_squad 0x523b10 (18): 8966 loads r.y's movsx one instruction earlier than the binary in the float
  conversion of r. Tried (all 18 unless noted): a pointer to r, an out pointer local, a w local (before the if and
  at the top), int locals, a float local for x, a pointer to r's components, a copy of r (78), a ternary for w,
  a pointer to out->y, a volatile out (28), a volatile x store (20), (float) / (int) casts, the constant first,
  short locals, the if as `!= 0` (32), an early return, a bool for the test, a constant local, a comma chain.
  Scheduling; stop.
- cquat::multiply 0x522a40 (28): the same conversion scheduling (the next movsx hoisted above the fild) plus the
  w1 * w2 temp's fstp and w1's load scheduled before the pushes. Tried: a pointer to q, int locals in both orders,
  (float) casts, a pointer to v1, the w conversions first (120), a copy of q (171), v1 / v2 interleaved (40).
  Scheduling; stop.
- quaternion::multiply 0x522930 (2, not in the batch): a volatile store of r.w (4) and r.w stored last through a
  local and a volatile (12) are worse; D17801's list stands.
- quaternion::quaternion(const matrix *) 0x522790 (69): not attempted.

## Behaviour errors / bug candidates

None found. All changes keep the computation (copies of locals, a pointer to w, a volatile pointer for three zero
stores).

## Header changes

None.

## Changed files

- src/vsdk/unnamed_quaternion_0x5225b0.cpp (cquat::log, log_in_place, exp, exp_in_place, quaternion::exp,
  to_matrix, to_matrix_b; comments carry D18200-D18206)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "unnamed_quaternion" --check src/decomp/vc6-matches.txt`:
1 units: 28 match, 0 wrong target, 4 differ, 0 not found, 0 errors; 21 matched there, 28 now, 0 lost.

## Method note

New device worth trying elsewhere: when 8966 uses a register variable as the destination of a multiply where the
binary keeps it and consumes the other operand (or uses a memory operand where the binary reloads with fld), a
plain copy of the other operand into a fresh local right before the use changes the FP allocation (D18200). And
when the binary compares a freshly computed value in a register but reads the variable from memory afterwards,
the value computed into the variable that is reassigned later and copied into the one that is kept (D18202).

## Worker 2

### m-1021b worker 2 (D18300-D18399)

Batch: 8 functions (level_scripts.cpp x6, new_hud.cpp, summoner_cd.cpp). Result: 4 new matches, 3 closer, 1 unchanged.
Helper scripts (variant runners with a register-aware side-by-side) are in `try/` in this folder; nothing there is for
the merge.

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/summoner_cd.cpp | 0x43ea20 | cd_prompt_disc2 | D18300 | `do { ...; if (drive < 0) { body } } while (drive < 0);` keeps the loop test at the top (a `for (;;)` with a break gets rotated) |
| game/level_scripts.cpp | 0x4d18e0 | savegame::layout_conversation | D18301 | the row of sections through `savegame_section *row = conversations[level]` with `row[slot].data / .size` for the first two stores, and the "_m1" test written out with `info->name` re-read |
| game/level_scripts.cpp | 0x4d25b0 | savegame::read | D18302 | the thumbnail size in an int local `n` for the second File::read |
| game/new_hud.cpp | 0x410570 | hud_drag_icon_begin | D18304 | `active` set first through a pointer `d`, then the rectangle stores in x, y, w, h order |

**D18300 (game/summoner_cd.cpp). cd_prompt_disc2 0x43ea20 matches.** The binary does not rotate its loop. It sets
`mov ebp, 6` before the loop top, tests the drive at the top, and ends the body with a `jmp` back. `for (;;)` with
`if (drive >= 0) break;` (the old form) and also `while (1)`, `goto` loops, `continue` forms and a comma condition
all rotate the loop, because 8966 copies the head below the body. Three forms match: `do { text = ...; drive = ...;
if (drive < 0) { body } } while (drive < 0);`, the same with `drive = -1; while (drive < 0)`, and a done or found
flag. The do / while is kept. It may also help other search loops where the binary re-tests at the top.

**D18301 (game/level_scripts.cpp). savegame::layout_conversation 0x4d18e0 matches.** (1) Frame slots: with
`savegame_section *row = conversations[level];` in the outer loop and `sec = &row[slot]`, the two strength-reduced
pointers take the binary's slots (distance 16 -> 4). (2) Store order: `row[slot].data = buffer + used; row[slot].size
= 0;` gives the binary's data-then-size order (sec->data / sec->size, a helper, a data local, size-first and chained
forms do not; distance 4 -> 2). (3) The binary has `lea edx, [name + len - 3]`. The shared inline
conversation_slot_is_mp gives [len + name - 3] in every spelling: `&name[len-3]`, `len + name - 3`, `name + (len -
3)`, an if form, and a name local. Writing it out as `int len = strlen(info->name); if (len >= 3 &&
_stricmp(info->name + len - 3, "_m1") == 0) continue;` matches. An unsigned len also matches. The helper stays for
set_speaker_bit and set_topic_bit, and its comment is updated.

**D18302 (game/level_scripts.cpp). savegame::read 0x4d25b0 matches** with `int n = thumbnail.size;` passed to the
second File::read (and `start += thumbnail.size` left as it was). That sets the binary's EAX / EDX choice in both
later reads. A pointer local for `buffer + start` gets to distance 13, and a `rest` local for the third size to 21.
A buffer copy, `&buffer[start]`, `start + buffer`, two locals and member reads are worse.

**D18304 (game/new_hud.cpp). hud_drag_icon_begin 0x410570 matches.** Writing `hud_drag_state *d = &hud_drag_flags;
d->active = 1;` keeps the binary's read of the old word (`or al, 1 ... and eax, 1`) inside the one merged store,
where the plain field stores fold to the constant 13. With d->active first, 8966 swaps each pair of rectangle
stores, so the source order x, y, w, h gives the binary's y, x, h, w (y, x, h, w in the source gives x, y, w, h).
d->active after the rectangle, all four fields through d, d only for slot or is_skill, byte / short / char ORs on
the word, a struct copy and bool casts do not match.

## Closer, kept

| file | address | name | old -> new | D |
|---|---|---|---|---|
| game/level_scripts.cpp | 0x4d05f0 | savegame::get_speaker_bit | 23 -> 18 | D18305 |
| game/level_scripts.cpp | 0x4d2370 | savegame::write_item_objects | 56 -> 36 | D18306 |
| game/level_scripts.cpp | 0x4d63b0 | level_format_display_name | 30 -> 16 | D18303 |

**D18303 (game/level_scripts.cpp; closer). level_format_display_name 0x4d63b0.** Written as early returns: `if
(level == -1) { strcpy Unknown; return; }`, then the loop, then `if (i >= 51 || strlen(...) == 0) { strcpy Unknown;
return; }`, then the copy. 8966 merges the two "Unknown" blocks into the binary's single block, and the search loop
now uses the binary's ECX / EDX. Still differs: the four `level_display_names[i].text` loads use EAX, ECX, EDX, EAX
where the binary uses ECX, EDX, EAX, ECX (one register further on). Tried without effect: `i == 51` (worse),
`!strlen`, a len local or split tests (worse), a bool, a strcspn string local (37), a goto to one Unknown (30), nested
ifs (30), a return from inside the loop (54), an else-first copy (56), a text local, a level_script_info or
display-name pointer, and unsigned i / n.

**D18305 (game/level_scripts.cpp; closer). savegame::get_speaker_bit 0x4d05f0.** The hashes are read through
`unsigned int *hp = (unsigned int *)(data + 4); ... h = *hp; hp++;` in place of the byte offset `off`. data and
bits_off now get the binary's EDI / EBP. Left: 8966 creates hp (and h's register) before the count test, where the
binary creates it after the test, and at the end ours adds bits_off before data. Tried without effect: seven spellings
of the final byte index (`data[bits_off + ...]`, `(data + bits_off)[...]`, a p local, `*(... + bits_off)`, and
others), `register` data / bits_off, a data copy, a count local, a section pointer, data assigned after the count
test (33), bits_off after the hash (41), hp in the for init, `*hp++`, `hp[i]` (29), if + do / while (56), if + for
(28), and a while.

**D18306 (game/level_scripts.cpp; closer). savegame::write_item_objects 0x4d2370.** The loop now sets `c =
&containers[ci]` first, tests `!c->type || c->num_items <= 0`, and calls `containers[ci].get_item(0)`. The
strength-reduced container pointer then sits at &c->pos (+0x10), as in the binary (56 -> 36). The nested-if, type
local and bool forms give 36 too. Left: count / piles_off (the binary's esp+0x10) and the float temporary (esp+0x14)
are in swapped frame slots. Tried without effect: `float f[1]`, a function-scope f before or after the other locals,
an unsigned count, `cnt[1]`, a reversed declaration order, a chained or moved count store, a memcpy of f, a pointer
loop over containers (83), pos through a `vector3 *` or float pointer (134), and a put_float inline helper (100, frame
0x10).

## Still differs (tried; do not repeat)

- **savegame::get_int 0x4d0030** (distance 22, unchanged): only ECX / EDX are swapped (the level * 64 offset and the
  loop pointer in EDX, the index in ECX in the binary; set_int has ours). Tried: the loop written out (30), a
  level_script_info pointer (23), a hashes local (36), an n local, an off split, an inline pointer loop (38),
  variants of the find helper (unsigned index with a signed test, while, pointer (26), hash first, a result local,
  count first), the final read as a pointer (28) / value local / memcpy / if-else / bit-offset split / array index,
  declaration order, an int hash, no locals, `register` index, split range tests, a return from the loop (28), nested
  ifs, and a data pointer. This looks like 8966's register choice.

## Behaviour errors / bug candidates

None found. Every change keeps the behaviour: the do / while in cd_prompt_disc2 runs the same passes, and in
write_item_objects `containers[ci].get_item(0)` is `c->get_item(0)`.

## Header changes

None.

## Changed files

- src/game/summoner_cd.cpp (cd_prompt_disc2)
- src/game/level_scripts.cpp (layout_conversation, read, get_speaker_bit, write_item_objects,
  level_format_display_name; the comment on conversation_slot_is_mp)
- src/game/new_hud.cpp (hud_drag_icon_begin)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "game/(level_scripts|new_hud|summoner_cd)\.cpp" --check
src/decomp/vc6-matches.txt`:

    3 units: 169 match, 0 wrong target, 21 differ, 0 not found, 0 errors
    check against src/decomp/vc6-matches.txt: 165 matched there, 169 now, 0 lost

The summoner_cd.cpp relocation note is cd_find_drive's string, which was already there (D2600 no-CD patch).

## Worker 3

### W3 notes (targeted round m-1021b, decisions D18400-D18499)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| vsdk/unnamed_snd_0x5204c0.cpp | 0x521680 | snd_set_volume | D18400 | the clamp as `do { if (v <= lo) { v = lo; break; } if (v >= hi) { v = hi; break; } } while (0);` |
| vsdk/unnamed_snd_0x5204c0.cpp | 0x5216e0 | snd_set_volume_in_group | D18400 | same |
| vsdk/unnamed_snd_0x5204c0.cpp | 0x521760 | snd_set_pan | D18400 | same (lo -1.0f) |
| vsdk/unnamed_snd_0x5204c0.cpp | 0x5218e0 | snd_set_group_volume | D18400 | same |
| game/s3d.cpp | 0x46ffe0 | s3d_read_mesh_ref | D18401 | the two corner copies as `memcpy(&box->mins, &mins, sizeof(vector3))` / `memcpy(&box->maxs, ...)` |

**D18400 (vsdk/unnamed_snd_0x5204c0.cpp). The volume / pan clamp is a do / while (0) with a break after each
store.** The binary tests both bounds first and jumps out of line to the two stores (`jne lo; ...; je hi; jmp done;
lo: ...; jmp done; hi: ...`). Every if / else-if, nested-if, ternary, goto and switch form of earlier rounds
(D10101, D13002, D15103) gives the fall-through layout; `do { if (v <= lo) { v = lo; break; } if (v >= hi) { v = hi;
break; } } while (0);` gives the binary's exactly, in all four functions. Same behaviour. Also tried this round, no
change: pointer and reference locals for the argument, a bool local for the low test, an empty else-if, a
`for (;;)` with breaks, ternaries through a local / an int alias, a comma expression, a `while (out of range)` form.
Device worth trying elsewhere: a clamp or if / else-if chain whose stores the binary places out of line after a
`jmp done`.

**D18401 (game/s3d.cpp). s3d_read_mesh_ref copies the lightmap box corners with memcpy.** The binary forms
&box->maxs (`mov edx, eax; add eax, 0xc`) in a second register before the first corner copy, as the memcpy
intrinsic does for two 12-byte copies; vector3's operator=, float-by-float stores, pointer / reference locals for
the maxs corner and a box pointer copy all give one base register. `memcpy(&box->mins, &mins, sizeof(vector3))`
and the same for maxs match (12 or sizeof both do). Same behaviour (three floats copied each).

## Closer, kept

None.

## Still differs (tried; do not repeat)

**D18402 (notes only). Tried this round, still differing.**

- mp_reset_net_stats 0x4419d0 (distance 17): the binary stores net_stats_timer from EAX (the memset zero) before the
  third rep stosd and the seven counters from a separate `xor edx, edx`. Tried: the arrays as loops (each, the
  third, the first two), the timer before / after every memset, `= 0` / `= 0.0` for the timer, the timer through a
  float pointer, `*(int *)&timer = 0` (gives the separate EDX zero but uses it for the timer too and stores it after
  the rep stosd), the same through unsigned, `*(int *)&` / `*&` / pointer-local / unsigned / long stores for the first
  counter, `0U` / `0L` / `false` / `(short)0` / `(unsigned char)0` literals for the counters, a chained assignment of
  all seven, a zero local, a counter loaded from the cleared array. None moves the second zero register.
- Crc32::update 0x552130 (distance 21): register choice (binary keeps the value in EAX and buf in EDX, XORs the
  table word into the shifted value). Tried: the index split into a local (int and unsigned), `(value >> 8)` first,
  a value local, a table-word local, the value through a pointer, `(value ^ b) & 0xff`, `value >>= 8; value ^= ...`
  with an old-value local, a byte local, pointer walks (while / for, `*p++`, `p[i]`), a shifted-value local. All
  give EDX for the result or change the prologue.
- tiger_pick_target 0x49b0a0 (distance 32): the frame and slots already match; the binary keeps its start-up zero in
  EDX (`xor edx, edx`), ours in EBX, which 8966 then reuses in the loop (`cmp esi, ebx`, `q.result = ebx`, an
  `xor ebx, ebx` at the loop top); also the base-threat sum's operand order (threat[1] loaded first in the binary,
  masked by the distance) and the scheduling of the third loop's base product. Tried: the candidate block as
  `if (c) { ... }` with block-scoped q / seen (as boss_target_list_build), `!c`, braces, the query stores through a
  pointer (result only, all), a candidate pointer, the ok store through a pointer, a boss local, c / q / seen at
  function scope (each, all, c and q), seen as an if, count initialised after the memset / after the g test, the g
  test before the memset, declaration orders, `cand[5] = { 0 }`, a `5 * sizeof` memset, `!g`; for the sum:
  `threat[0] + threat[1]`, a threat pointer (worse), a threat[1] local, `-a - b` (worse). Nothing moves the zero
  register; looks like 8966 register choice.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/vsdk/unnamed_snd_0x5204c0.cpp (snd_set_volume, snd_set_volume_in_group, snd_set_pan, snd_set_group_volume;
  the four comments now say exact, D18400)
- src/game/s3d.cpp (s3d_read_mesh_ref; its comment says exact, D18401)

## --check results

- `--only "unnamed_snd_0x5204c0"`: 58 match, 0 differ; against vc6-matches.txt 54 matched there, 58 now, 0 lost.
- `--only "game/s3d\.cpp"`: 33 match, 1 differ (s3d_read_meshes, untouched); 32 matched there, 33 now, 0 lost.

Unchanged and not edited: game/unnamed_netpackets_0x441820.cpp, vsdk/unnamed_crc32_0x5520d0.cpp,
game/unnamed_tiger_0x499e50.cpp.

Scratch: try/ (variant generators, tryscore.mjs: applies a variant, runs vc6-compare --score, restores the file).

## Worker 4

### W4 notes (m-1021b, decisions D18500-D18599)

Batch: 8 functions. 6 new matches, 1 closer (kept), 1 unchanged.

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/new_inventory.cpp | 0x4174a0 | inventory_equip_item | D18500 | clamp at 0 in place through a pointer to pos |
| game/new_inventory.cpp | 0x416fb0 | inventory_identify_item | D18500 | same |
| game/new_inventory.cpp | 0x416b80 | inventory_popup_result | D18501 | negative choice returned first, every case a break to one `return key`, case 4 and default separate |
| game/new_itemtrade.cpp | 0x419530 | itemtrade_count_party_rows | D18504 | the count read through a pointer local |
| vsdk/unnamed_vfile_rw_0x525720.cpp | 0x525fe0 | File::read | D18502 | `int pos = s->pos;` before `left = s->size - n - pos` |
| vsdk/unnamed_vfx_material_0x564d70.cpp | 0x564d90 | vfx_tex_layer::get_frame_bitmap | D18505 | the call's bitmap read through a pointer local into a local |

**D18500 (game/new_inventory.cpp). inventory_identify_item 0x416fb0 and inventory_equip_item 0x4174a0 are byte
matches.** In the clamp after the min, the binary ANDs into pos's register (`and eax, edx` / `and eax, ecx`), the
way inventory_page_up's in-place `&=` on the global does. Every max() / ternary / mask-local form ANDs into the
mask register. The form that matches is the in-place AND through a pointer to the local:
`int pos = min(...); int *pp = &pos; *pp &= (*pp <= 0) - 1; inventory_scroll_pos = pos;`. Also not it (21 forms
this round): a pointer to the global (store, or load and store), a mask local (int / unsigned, `&` or `&=`),
`pos & -(pos > 0)`, `neg - 1`, a copy of pos, the limit as a local with `if` forms (page_down's shape), min straight
into the global and then `&=` on it (8 operand / condition spellings: the AND is right, but the min ends in the
global's register, `jl` in place of `jge`), the same through a pointer to the global.
inventory_destroy_selected_item 0x417170 (not in my batch) has the same clamp
(`int pos = min(...); inventory_scroll_pos = max(pos, 0);` at the `party_inventory_remove(index, 1, 1)` path) and
differs only there. The same change will very likely match it. I did not edit it.

**D18501 (game/new_inventory.cpp). inventory_popup_result 0x416b80 is a byte match.** The binary keeps key in EDI and
zero in ESI. Cases 1 and 2 load the result before the mode store (`mov eax, edi; mov [mode], esi`), and case 0 and
the out-of-range path share the opposite order. The source that matches: `if (choice < 0) { mode = 0; return key; }`
after the -1 / -2 tests, then a switch where cases 0, 1, 2 do `...; inventory_mouse_mode = 0; break;`, case 3
breaks after confirm_destroy, case 4 and `default` each have their own `inventory_mouse_mode = 0; break;`, and one
`return key;` at the end. Behaviour unchanged. Not it: `break` everywhere inside `if (choice >= 0)` with the `else`
(54), the earlier return-per-case forms with a bool flag or a result local (53), a `k` copy, a zero local,
pointers to key / mode, case 4 first, `(unsigned)` switch (all 27).

**D18502 (vsdk/unnamed_vfile_rw_0x525720.cpp). File::read 0x525fe0 is a byte match.** The refill's
`int left = s->size - n - s->pos;` is computed `(size - pos) - n` by 8966. Loading pos into a local first
(`int pos = s->pos; int left = s->size - n - pos;`) gives the binary's `mov ecx, [pos]; sub esi, ebp;
sub esi, ecx`. This also fixes the later `buf_start = n + s->pos` difference D16202 left (side-by-side alignment
made it look separate). `left = s->size - n; left -= s->pos;` and `int *pp = &s->pos` also match. Not it:
`s->size - (n + s->pos)`, `s->size - s->pos - n`, explicit parentheses, `-n + ...`, and four buf_start spellings.

**D18504 (game/new_itemtrade.cpp). itemtrade_count_party_rows 0x419530 is a byte match.**
`int *c = &party_inventory_count; itemtrade_party_row_count = *c; if (party_gold > 0) itemtrade_party_row_count++;`.
8966 then re-reads party_gold after the row-count store and increments the global in memory (`inc dword ptr`), as
the binary does. `c[0]` and a pointer to party_gold beside it also match. A pointer to party_gold alone, a pointer
to the row count, `+= 1`, and `(&party_inventory_count)[0]` do not. I also found
`int *g = &party_gold; rows = g[1]; ...`, which matches but depends on the two globals' layout. Do not use that one.

**D18505 (vsdk/unnamed_vfx_material_0x564d70.cpp). vfx_tex_layer::get_frame_bitmap 0x564d90 is a byte match.**
Inside the `bitmap > 0` block: `int *bp = &bitmap; int bm = *bp; n = bm_get_num_frames(bm, &secs);`. The binary's
this goes to ESI and n to EDI, with the tested bitmap passed from EAX. `int *bp = &bitmap;` before the test with
`int bm = *bp` in the block also matches. Steps on the way: `*bp` straight into the call gives the right registers
but reloads bitmap (21). Testing `*bp > 0` too tail-merges the `n == 1` return into the final one (22). Not it:
`vfx_tex_layer *l = this` (any placement), `this->`, `register` n / f, a mode local, the return through f, a
`bm` copy without the pointer (32), a pointer to a `bm` copy (32), and the return forms through `*bp`.

## Closer, kept

- game/unnamed_avoidance_0x4a6710.cpp 0x4a6e00 avoid_prim_overlap: distance 27 -> 13 (**D18503**). ku is now
  `fabs(bo->axis_a.dot(*u))` with `const avoid_prim *bo = b;`. The third test's two k terms are inline dots
  (`b->axis_b.dot(*v)`, `b->axis_b.dot(*u)`), and the first is assigned to an otherwise unused `kw`. This gives
  the binary's operand order for ku's x product and both k terms, and its no-op `fxch st(1)` pair. Behaviour is the
  same. What is left: (1) the first two projections, where the binary multiplies d.x * u.x first and ours d.y * u.y
  first; (2) the binary's dead `fst dword ptr [esp+0x1c]` of the third test's u term into b's argument slot.

## Still differs (tried; do not repeat)

- game/pathfinding_visibility.cpp 0x4b7d30 pathvis_add_edge_node (22, unchanged). The binary keeps &e->prev /
  &e->next in EDX / ECX (two `lea`, ESI / EDI saved). 8966 folds every form into `[eax+0x20]` / `[eax+0x24]`
  operands. Tried (9): two pointer locals in both declaration orders, a pair in a block, `pathvis_edge *&`
  references, an inline helper with reference parameters, one with pointer parameters, `l[0]` / `l[1]` through
  `&e->prev`, char-offset casts, pointer locals plus value locals. The fix is probably in the list_remove macro
  in vsdk.h (D7006), a header question.
- avoid_prim_overlap, the remaining projection order (tried on top of D18503, 23 forms): every source order of the
  two products and factors, `d.dot(*u)` / `u->dot(d)`, `const vector2 *dp = &d` (26), d through operator- (21),
  `float d[2]` (21), dx / dy floats in either order (40), d declared last, u / v assignment and declaration order,
  `(*u).x`, a pointer copy of u, an `ao` copy. For the dead fst: kw assigned on the u term (31), volatile kw on the
  u term (33), both terms through locals (45).

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files (under W\src)

- game/new_inventory.cpp (inventory_identify_item, inventory_popup_result, inventory_equip_item)
- game/new_itemtrade.cpp (itemtrade_count_party_rows)
- game/unnamed_avoidance_0x4a6710.cpp (avoid_prim_overlap, closer)
- vsdk/unnamed_vfile_rw_0x525720.cpp (File::read)
- vsdk/unnamed_vfx_material_0x564d70.cpp (vfx_tex_layer::get_frame_bitmap)

## --check (VC6_STRICT=1 vc6-sweep --check src/decomp/vc6-matches.txt)

| unit | matched there | now | lost |
|---|---|---|---|
| new_inventory | 63 | 66 | 0 |
| new_itemtrade | 35 | 36 | 0 |
| unnamed_avoidance_0x4a6710 | 56 | 56 | 0 |
| unnamed_vfile_rw_0x525720 | 50 | 51 | 0 |
| unnamed_vfx_material_0x564d70 | 14 | 15 | 0 |

## Devices that worked (for later rounds)

- A pointer to a local or a global, read into a value (`int *p = &x; int v = *p;` or `rows = *c`), breaks 8966's
  CSE / alias knowledge. It reproduced a reload in memory (itemtrade), the AND destination register (inventory
  clamp), and a register assignment (get_frame_bitmap).
- A member loaded into a local before a subtraction chain sets the binary's association (File::read).
- A switch whose cases all `break` to one return, with the negative range returned first, sets the order of the
  duplicated tails (popup_result).

## Worker 5

### m-1021b worker 5 (D18600-D18699)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| vsdk/unnamed_vfile_0x5128b0.cpp | 0x5147c0 | File::get_async_status | D18600 | a slot pointer `file_slot *s = &file_slots[slot]` inside the if, with `s->owner->packentry` written out three times (test, flag test, argument); 8966 keeps the slot address as a common subexpression, the binary's unfolded `add eax, file_slots` and ECX / EAX choice |
| game/characterinfo.cpp | 0x4469c0 | living_entity_setup_head | D18601 | `const char *n = attack_point_names[i];` before `find_mesh_tag(n)` in the second loop: the name load goes to EAX and the later scratch registers (index, the transform call's leas) follow the binary |
| vsdk/unnamed_eax_0x521e40.cpp | 0x521f40 | eax_init | D18603 | the support test through a bool local: `bool ok = SUCCEEDED(QuerySupport(...)) && (support & 3) == 3; if (!ok) { fail }` puts the failure block first and the success block last, as the binary (the D17200 bool device) |
| vsdk/unnamed_vfile_0x5128b0.cpp | 0x513590 | file_list_files | D18605 | `char ext[5]` (was `ext[8]`) declared last, after `int i`: ext goes to the bottom of the frame, p and i above it, as the binary. Same behaviour (strncpy 4 + ext[4] = 0 needs 5 bytes) |

**D18600 (vsdk/unnamed_vfile_0x5128b0.cpp).** File::get_async_status is exact with a slot pointer and the
owner's packentry read three times (`if (s->owner->packentry && (s->owner->packentry->packfile->flags &
PACKFILE_ON_CD)) return packfile_fstatus(s->owner->packentry);`). The repeated reads are CSEd (no store between),
so the behaviour is unchanged; the common slot address is what keeps `add eax, file_slots` unfolded. A single
`e` local, an `int s = slot`, a `file_slot *base`, an owner reference / pointer-to-pointer, early returns, nested
ifs, a this copy and `s += slot` all compile to the folded form.

**D18601 (game/characterinfo.cpp).** living_entity_setup_head: the attack point name through a pointer local.
Pointer loop, int count local, post-increment store, target pointer local, block tag local and reloading
ci->character do not.

**D18602 (notes only).** ca_character::report_memory 0x541020: 10 more forms. An `int n[3]` array of the three
counts loads them in the binary's order (pos, rot, bones) but 8966 still folds skel into the bones * 45 chain
(distance 22 -> 28, not kept); 12 sum orders over the array (grouped, split, temporaries) give the same canonical
code. Locals per count, pointer copy, reference, `skeleton **`, unsigned sum and split statements change nothing.

**D18603 (vsdk/unnamed_eax_0x521e40.cpp).** eax_init: see the table. goto into / after the fail block, an
HRESULT local with the success block first, an early `== true` return, a switch on the condition and a `for (;;)`
with break all keep 8966's success-first layout.

**D18604 (vsdk/unnamed_vfile_0x5128b0.cpp; closer, kept).** File::set_cipher_key: `int i = slot;` and, in the
zero branch, `unsigned int *fl = &file_slots[i].flags; *fl &= ~1;` with `file_slots[i]` in the key branch:
distance 32 -> 28. The slot address is now computed in each branch and the key branch's tail (flag or, set_key
call) is exact. Left: 8966 loads key into EAX and keeps a zero register (`xor edx, edx; cmp eax, edx`, k[1] /
k[2] stored from EDX); the binary loads key into EDX, tests it and stores immediates.

**D18605 (vsdk/unnamed_vfile_0x5128b0.cpp).** file_list_files: see the table. Eight more declaration orders
with p and offset at function scope compile the old frame; only the smaller array declared last moves it.

**D18606 (notes only).** ls_ionaext_frame 0x4f7270 and effects_parse_arr_line 0x51ca60, see below.

## Closer, kept

- File::set_cipher_key 0x5143e0: 32 -> 28 (D18604).

## Still differs (tried; do not repeat)

- File::set_cipher_key 0x5143e0 (28): the key / zero register. Tried this round on top of the D14606 forms: pointer,
  bit-field (direct and through a pointer) for the zero branch, `|=` and bit-field for the set, s / i / no local,
  k stores separate, chained, k[0] last, `= { key }`, key copied into a local, `int` / `unsigned long` k with a cast,
  `(unsigned char)0` stores, `k[1] = k[2]`, stores through `int *`, `!key`, a bool `off` / `on` local, if / else
  with the key branch first (about 50 forms). The zero register is always kept.
- ls_ionaext_frame 0x4f7270 (18): the `sete al / test al, al`. Tried: bool locals reassigned, set false then true,
  set true then cleared, `b = !b`, ternary with `== true`, int / char locals, `(x == true) == false`, `^ true`, `!!`,
  bool cast, a while-once loop, and static inline helpers (two returns, bool / int parameter, `return !b`, a local
  result, ternary, a returning helper, __forceinline). 8966 folds every one into `test al, al; jne`.
- effects_parse_arr_line 0x51ca60 (16, only a branch target): the inner loop's first `>= 32` exit goes to the
  function-end epilogue in ours, to the loop's own one in the binary. Tried: goto to a second `return -1`, an
  if + do/while, a result local, if / else in the loop, `for (;;)` with the test inside, break to the outer return,
  `return ~0`, an `unsigned *n` local, an outer do/while with an entry test, outer `for (;;)`, a goto to the found
  block, a block scope, bool loop flags (room / full / done), a test after the loop.
- ca_character::report_memory 0x541020 (22): D18602.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/vsdk/unnamed_vfile_0x5128b0.cpp (get_async_status, file_list_files, set_cipher_key)
- src/game/characterinfo.cpp (living_entity_setup_head)
- src/vsdk/unnamed_eax_0x521e40.cpp (eax_init)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root W/src --only "unnamed_vfile_0x5128b0|game/characterinfo|unnamed_eax_0x521e40" --check src/decomp/vc6-matches.txt`:
3 units: 163 match, 1 wrong target, 2 differ; 159 matched there, 163 now, 0 lost. (The wrong target is the one
characterinfo already had before this round.)

## Worker 6

### m-1021b worker 6 (D18700-D18799)

## New matches

| file | address | function | D | what did it |
|---|---|---|---|---|
| game/new_spell.cpp | 0x42b050 | character_use_skill_on | D18704 | the backstab case ends `goto clear;` to a `clear:` label before the shared `world_pick_clear_filter();` |

**D18704 (game/new_spell.cpp). character_use_skill_on 0x42b050 is exact.** The backstab case (0x45) had its own
`world_pick_clear_filter(); return;`. Written as `goto clear;`, with `clear:` before the last
`world_pick_clear_filter();`, the region after the `!target || !user` test has a single exit, so 8966
shrink-wraps the EBX push into it (D8701), and it still writes out the epilogue per exit as the binary does.
Earlier rounds' single-exit forms (D8701 note in 20b) did not use a shared tail call; this one does. Same behaviour.

## Closer, kept

| file | address | function | D | old -> new distance |
|---|---|---|---|---|
| vsdk/targautils.cpp | 0x555f60 | pcx_read_bitmap | D18701 | 13 -> 9 |
| vsdk/bmpman.cpp | 0x502240 | bm_set_reduced | D18702 | 23 -> 4 |
| vsdk/bmpman.cpp | 0x503aa0 | bm_lock_pcx | D18703 | 34 -> 33 |
| vsdk/gr.cpp | 0x507100 | gr_bitmap_float | D18705 | 22 -> 12 |

**D18701 (vsdk/targautils.cpp). pcx_read_bitmap 0x555f60: `int pos = 0;` after the last read before the loop**
(count = 0 still before it). That read now pushes EBP (count) for its two zeros as the binary does; the only
difference left is where `xor esi, esi` sits: the binary clears pos between the `lea ecx, f` and the call, 8966 after
the call. The rest of the distance is the EH funclet lines.

**D18702 (vsdk/bmpman.cpp). bm_set_reduced 0x502240: the new level count through an int local of the byte result**:
`int n;` and in each branch `n = (ubyte)(e->num_levels -/+ 1); e->num_levels = n;`, then
`bm_calc_num_pixels(e->width, e->height, n)`. This gives the binary's `mov al, [esi+0x1c]; dec / inc al`, the
in-place `and eax, 0xff`, the early push of the count, eax / edx for width / height, and the binary's ecx in the
handle division of gr_mark_texture_dirty's argument. Left (distance 4): 8966 puts the `and` and `xor edx, edx; mov dx,
height` above the level store; the binary stores, ands, pushes, then loads. Same behaviour (the byte wraps the same).

**D18703 (vsdk/bmpman.cpp). bm_lock_pcx 0x503aa0: `int bpp = bm_bytes_per_pixel(...)` first and `new
ubyte[bpp * num_pixels]`** gives the binary's `mov ecx, [num_pixels]; imul ecx, eax`. Left: 8966 pre-shifts the
9 * index (`shl esi, 3`, `[esi + field]`), the binary uses `[8 * esi + field]` in every address.

**D18705 (vsdk/gr.cpp). gr_bitmap_float 0x507100: only hs = hh * sa and wc = hw * ca as locals; the hw * sa / hh * ca
products written into the sy sums** (`(-(hw * sa) - hh * ca) + y`, `(hw * sa - hh * ca) + y`,
`(hh * ca + hw * sa) + y`, `(hh * ca - hw * sa) + y`). 8966 then computes hs, wc, corner 0's sx, then the two products
kept on the FPU, as the binary does. And corner 2's four constant stores written after its sx / sy, which moves them
below the gr_tmapper pushes as in the binary (22 -> 16 -> 12). Same values (same products, same sum order). Left:
the mode / vlist pushes and corner 2's sx `fadd x` / store are scheduled differently. (All products inline, with
hs / wc too: 27; the four locals declared at the point of use: 27; locals hs, wc then ws, hc after corner 0's sx: 16.)

## Still differs (tried; do not repeat)

**D18700 (notes only). Tried without a match:**
- pcx_read_bitmap (on top of D15301): `count = pos = 0`, pos in the for initialiser, a pointer store `*pp = 0`, the
  read through a `File *` or a buffer pointer local, `(pos = 0, buffer)` / `buffer + (pos = 0)` as the first
  argument (with 0 or count as the zeros), `pos = count` before the read, `int pos = (f.read(...), 0)`,
  `int pos = sizeof(buffer); f.read(buffer, pos, 0, 0); pos = 0;`. Every form with pos set before the call uses ESI
  for the zeros; every form after it puts the xor after the call.
- bm_set_reduced (on top of D18702): `int n = e->num_levels` before the call (reloads), cast in the call, int w / h /
  n locals, ubyte n (function or block scope: 8966 spills a byte local), `n = --e->num_levels`, chained
  `n = e->num_levels = ...` (the byte op moves to DL), `e->num_levels--; n = e->num_levels;` (DL), `& 0xff`,
  `(ubyte)n` at the store, store first then `n = (ubyte)e->num_levels` (DL), unsigned n, `register`, n declared
  first, a ubyte pointer store, an int h local.
- bm_lock_pcx: index as unsigned / short / int local, `(bm_bitmaps + index)->`, `index[bm_bitmaps]`, a size local,
  `return pcx_read_bitmap(...) == 0`, `bm_bitmaps[i].num_pixels * bpp`.
- m2v_skip_extension_and_user_data: while-condition loop, case 2 as if / else with the 29-bit read in both arms,
  `continue` in case 1 or 2, `continue` after the user-data branch, a pointer local for bs, an id local for the
  switch, default first / after case 1, a result variable with goto, case 2 as an inline helper: 8966 always
  cross-jumps case 2's next_start_code tail into case 1's; the binary keeps three separate tails.
- read_track: `unsigned k` with `(int)k <` tests (D17702's device), plain unsigned k, `[(unsigned)k]`: the base /
  index order of `[keys + offset]` does not move.
- ai_choose_special, gr_bitmap_float further: not retried beyond the above (ai_choose_special's push block placed
  after the epilogue is an 8168 layout; earlier rounds' switch / separate-test / goto / else-if forms are listed
  in D13105 and the notes after it).

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/vsdk/targautils.cpp (pcx_read_bitmap)
- src/vsdk/bmpman.cpp (bm_set_reduced, bm_lock_pcx)
- src/game/new_spell.cpp (character_use_skill_on)
- src/vsdk/gr.cpp (gr_bitmap_float)

## --check (VC6_STRICT=1, against src/decomp/vc6-matches.txt)

`--only "vsdk/(targautils|bmpman|gr)\.cpp|game/new_spell\.cpp"`: 4 units: 162 match, 14 differ, 0 errors;
161 matched there, 162 now, 0 lost.

## Worker 7

### m-1021b worker 7 (D18800-D18899)

## New matches

- game/unnamed_netsession_0x440b40.cpp 0x441490 mp_session_reset (distance 14 -> match), **D18800**.

**D18800 (game/unnamed_netsession_0x440b40.cpp). mp_session_reset 0x441490 matches with the statement order
`list_init(&mp_player_list); mp_local_player = NULL; list_init(&mp_player_free_list); memset(mp_players, ...)`.**
Before, the free list's list_init came after the memset (ECX / EDX swapped: the -1 socket and the list tail).
The free list's init before the memset gives the binary's registers (distance 4, only the mp_local_player
store scheduled after the free list's next store); with mp_local_player cleared before that init it matches
(`mp_local_player = NULL` as the first statement after the timer also matches). The order of stores to three
different globals and a memset of a fourth: same behaviour. Tried on the way: the socket store after the
append, a record pointer local, an unsigned index, a head pointer, an expanded append with a tail local, a -1
local, a while loop, a `register` index (all 14 or 16); memset first (30), mp_local_player after the memset (18 / 28).

## Closer, kept

- game/unnamed_swi_0x488230.cpp 0x4882f0 mp_handle_swi_path: 18 -> 2, **D18801**.

**D18801 (game/unnamed_swi_0x488230.cpp). mp_handle_swi_path 0x4882f0: `bool *wp = &walk;
c->ai.start_travel(*wp);`.** The address taken puts walk in data's dead argument slot (byte store, dword load
and push), as in the binary. Left: the binary computes the `this` lea (`lea ecx, [eax+0x324]`) before the
walk load; 8966 places it after the push. Same distance 2 with memcpy into / out of walk, a block-scope bool,
an `ai_state *` or reference local before or after, `(&c->ai)->`, a Character copy, a `const bool &`, an int
pointer to the slot, the pointer created before the store. Worse: `*&walk` (18, folded), a ternary or `!= 0`
on the argument (24).

## Still differs (tried; do not repeat)

**D18802 (notes only). Tried, still differing.**
- mp_player_alloc 0x441610 (25): the binary's `lea ecx, [eax+4]` (&p->prev kept, reloaded through it). New
  forms, all exactly 25: a `mp_player **pp = &p->prev` through the remove and the append, the same for the
  remove only, `mp_player *&pr = p->prev`, both links through `**pp` / `**np`, a copy `q = p` used for the
  remove or for the append. 8966 folds every one back to `[eax+4]`. Exhausted with D7006 / D12803 / D14507.
- gr_project_vertex 0x50ea50 (24): cause found: the binary reloads `v->flags` for the final return
  (`mov al, [ecx+0x19]`), so its final return is not `mov al, dl; ret` and is not tail-merged with the
  `flags & 3` return; ours merges them (`jne` to the end instead of the binary's `je` over an inline return).
  Fixing the reload would fix both. Tried: a flags local in all exits (48), the same with a bool test, a
  chained `return v->flags = flags | 2` (24), nested if / else (24), a flags pointer (48 / 53),
  `v->flags = v->flags | 1`, sw stored first, sx / sy through a float pointer or a vertex pointer copy, the
  return value read between the sx and sy stores, y declared apart (all 24), the projection in an inline
  helper (75 / 77), an inline clamp by value / reference / pointer (99-103), the address of x taken (24),
  `|= 1` moved after the sx / sy stores or into the return (72).
- gr_get_frustum_planes 0x50ed60 (34): the plane count in ECX (binary EAX) and the second loop's index in EAX
  (binary ECX). Tried: an unsigned second index, no n local, a flags pointer, a plane pointer walk (32, not
  kept: a different shape), unsigned index for both, a separate j, an out pointer copy, n declared first,
  `gr_num_frustum_planes > i`, `*o++ =`, explicit `operator=`, a while loop, a const plane reference, a
  destination pointer. Register choice.
- ui_checkbox::draw 0x553f00 (22): three gr_line argument setups where the integer `lea` / `mov` and the
  `fild` / `fstp` of the int-to-float temporaries are scheduled in another order (lines 150, 169-175, 330-336,
  465-474 of the side-by-side). Scheduling only; not attempted further (as D10029 / 20c / 20d).
- script_editor_default_action_navpoint 0x4e8cf0 (28): the binary allocates the loop's address (found) in
  EAX and keeps a NULL in ESI only for the empty / entry-test exits (`mov eax, esi`); 8966 keeps found in ESI
  throughout. Tried: `found = np` before the branch (41), `return np` for the empty case with or without the
  in-if return / break (32), found declared first, a pointer copy in the loop, an unsigned index, a bool
  local for the type test, an int n local, the address returned as an expression, break, a `last` pointer
  before the loop (33), `found = np` at declaration, a temporary in the loop, the assignment after the test
  (41). All others 28.
- ui_slider::create_scrollbar 0x55ed30 (38): this / listbox register swap (binary this in ESI, listbox in
  EDI, the listbox load after `push edi`). Tried: window before lb, locals declared first, declaration split
  from the assignments, get_window through lb, view declared first, `this->create / set_parent`, the
  hscrollbar store through a pointer local. All 38.

## Behaviour errors / bug candidates

None new. (script_editor_default_action_navpoint's NULL navpoint read and &navpoints[-1] stay as described
in its comment, D8702.)

## Header changes

None.

## Changed files

- src/game/unnamed_netsession_0x440b40.cpp (mp_session_reset, D18800)
- src/game/unnamed_swi_0x488230.cpp (mp_handle_swi_path, D18801)

## --check result (VC6_STRICT=1, against src/decomp/vc6-matches.txt)

- unnamed_netsession_0x440b40: 19 matched there, 20 now, 0 lost
- unnamed_swi_0x488230: 13 matched there, 13 now, 0 lost

## Worker 8

### m-1021b worker 8 notes (D18900-D18999)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/multisave.cpp | 0x46d450 | mp_player_file_read_summary | D18900 | version-8 flag remap through an int local finished with `fl \|= out->flags & 1` |
| game/multisave.cpp | 0x46d610 | mp_player_file_load | D18901 | the same remap form for profile_flags (not in the batch; same file, same statement) |
| vsdk/gr_opengl.cpp | 0x54da80 | gl_convert_1555_to_rgba8888 | D18902 | alpha set by an if / else, every channel read through `*sp` (the `ushort p` copy dropped) |

**D18900 (game/multisave.cpp). mp_player_file_read_summary 0x46d450 is exact** with the version-8 flag remap
written as
`if (version < 9) { int fl = out->flags; fl = (fl >> 1) & ~3; fl |= out->flags & 1; out->flags = fl; }`.
The one-expression form (and the 20 earlier forms) put the shifted copy in EDX and the lea of the description
in EAX. The copy into fl with the shift in its own statement moves it to ECX (distance 19 -> 9, leaving `or eax, ecx`
where the binary has `or ecx, eax`). The compound `|=` then gives the binary's operand order. `out->flags = (out->flags & 1) | fl`,
`fl | (out->flags & 1)`, a separate lo local, `fl >>= 1; fl &= ~3`, an uninitialised fl, a bool for the version test,
a pointer to the flags and a pointer copy of out do not match. Same value, same behaviour.

**D18901 (same file). mp_player_file_load 0x46d610 (distance 130 before) is exact** with the same form for
`p->mp_owner->profile_flags`. Most of the 130 came from the remap's register choice shifting the rest of the function.
Same behaviour.

**D18902 (vsdk/gr_opengl.cpp). gl_convert_1555_to_rgba8888 0x54da80 is exact** with
`int a; if (*sp & 0x8000) a = 0; else a = 0xff;` and b, g and r also read through `*sp` (as the 4444 and scaled
converters do, D14105). The `ushort p` copy is no longer needed: the binary's load-then-dead-lea comes out without it,
and keeping it also matches. With the ?: form the alpha's neg / sbb / and / add was scheduled after the blue
channel. Tried before it, none of them matching: the 32 combinations of p / *sp per channel with `?:` and `!x ? 0xff : 0` (best 20), and,
with ?:, `== 0`, `!= 0`, ubyte, uint, a bool or int temporary, `>> 15`, `(short) < 0`, `a = 0xff; if (..) a = 0`.
Same behaviour.

## Closer, kept

- vsdk/unnamed_listbox_0x52ca60.cpp 0x52ed70 ui_listbox::set_vscroll_fraction: distance 26 -> 16 (**D18903**). The line
  search is written without the line_top / line_bottom locals:
  `for (i = 0; i < e->num_lines; i++) if (vscroll >= i * fh + top && vscroll < (i + 1) * fh + top) break;`.
  8966 strength-reduces it to line_top in EDX, as in the binary. The rest of the difference is register choice: the binary keeps
  top in EBP, spills num_lines to [esp+0x1c] and re-reads vscroll into EDI on each pass. Ours spills top and keeps vscroll.
  It is the same test, because `i * fh + top` is the running line_top. Reading vscroll through `ui_listbox *lb = this` gives 15
  but was not kept, since it is an unnatural alias for one step.

## Still differs (tried; do not repeat)

- **vsdk/unnamed_floatmath_0x542500.cpp 0x542500 fast_inv_sqrt** (22, unchanged). Pure register permutation: the
  binary has x in ECX, the constant in EAX and the mask in EDX, and ours has EAX / EDX / ECX. 21 more forms all give 22 (the union
  gives 27): a pointer to the bits (also returned through it), a table pointer, signed int casts, the index first in
  a local, the exponent through a local (`e = 0xbe000000; e -= ..`, and split into three statements), the result in a separate
  float or uint returned by pointer, the bits read once into a local through a pointer, an int exponent, the table term first
  through the pointer, `/ 2`, three `register` locals, the table value read first, `-(int)e + 0xbe000000`, a float out
  local, a pointer to the table element, unsigned long, an int index. Earlier rounds also tried every #pragma optimize letter.
  This looks like 8966's register choice. fast_sqrt / fast_sqrt_refined have the same swap.
- **vsdk/gr_opengl.cpp 0x54fa30 gr_opengl_bitmap** (16, unchanged). Only the first corner's store schedule
  differs. The binary writes pts[0].x / y / z, colors[0][0..1], tc[0][0], colors[0][2..3], tc[0][1..3] in near
  source order, and ours interleaves them. Tried with no change: a block, `for (;;) { .. break; }`, a vector3 pointer, a
  colors-row pointer, a tc-row pointer, flat `ubyte colors[16]` / `float tc[16]` / both, two other declaration orders
  of the arrays, `ubyte a, b, g, r`, a pointer for the z store, and tc[0][0] moved between the colour stores (22). Much worse:
  file-local inline helpers for the point / colour / tc stores, in any combination (160-342), declaring the arrays
  before or among the float locals (254-270), and a chained z store (93).
- **vsdk/unnamed_listbox_0x52ca60.cpp 0x52e380 ui_listbox::scroll_page_up** (32, unchanged). Two register swaps: in
  the line snap (line*fh in EDX and e->y in ECX in the binary) and in the BY_ENTRY / ALIGN_BOTTOM entry_at argument
  (vscroll in EDX, the lea in EAX). Tried with no change: an e->y local (and declared first, 61), `e->y + line*fh`,
  `fh * line` (both terms and each alone), an entry pointer copy, top split in two statements, a num_lines - 1 local,
  the `vscroll != top` nest in two shapes (50, 52), `int line; line = ..`, a pointer to e->y, `vscroll + view_height`,
  a block-local e2, `e->y + e->height`, a `first` local for entries.next, the fh / e declaration order, `vscroll = vscroll - ..`,
  and a this-pointer copy. Worse: a view-height local (45), a vscroll local (85), a y-argument local (77).
- **vsdk/unnamed_listbox_0x52ca60.cpp 0x52ed70 ui_listbox::set_vscroll_fraction** (now 16, see above). Also tried:
  `register int top`, a num_lines local (with both loop forms), a pointer to vscroll, `top + fh`, a while loop, a vscroll
  local in the loop, `e->y` in the result, an entry pointer copy, a nested if, top / i / line_top / line_bottom at function
  scope (all four combinations, no change), only a line_top (26) or only a line_bottom (27) induction variable, and
  locals inside the loop body (16). Worse: `vscroll = line_top` (50) and explicit (i + 1) / (i + 2) updates (55).
- **game/conversation.cpp 0x4d84d0 conversation::restore_state** (30, unchanged). The difference is the order in which
  level / slot / s->name / t->name are loaded around the pushes (the binary loads level before slot in the speaker call).
  Tried with no change: a `conversation *c = this` copy (for both calls, the speaker call only, the topic calls only,
  level only, slot only), the bits at function scope, `this->level`, and a savegame pointer. Worse: an s->name or t->name
  local (96, 94). save_topics, which passes the same arguments, matches, so this looks like 8966 scheduling.
- **vsdk/unnamed_ds_0x55bf90.cpp 0x55d400 ds_stream_open** (40, unchanged). The binary keeps three separate
  `or eax, -1` epilogues (acm failure, read_chunk failure shared with the CreateSoundBuffer failure, fill failure), and 8966
  cross-jumps them into one. Eight more return shapes: `== 0 return 0` with a final `return -1` (47), an outer `>= 0 &&`
  if (47, 49), a combined acm-and-read test (47), split CreateSoundBuffer / NULL tests (61), separate `return 0`s (51), and
  the if / else and else-if with a single `return 0` (40). This is the cross-jumping difference noted for
  ui_inputbox::set_valid_chars.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/game/multisave.cpp: mp_player_file_read_summary and mp_player_file_load (remap form, comments, confidence exact).
- src/vsdk/gr_opengl.cpp: gl_convert_1555_to_rgba8888 (alpha if / else, channels through *sp, comment, exact).
- src/vsdk/unnamed_listbox_0x52ca60.cpp: ui_listbox::set_vscroll_fraction (line search form, comment; still likely).

## --check results

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "<unit>" --check src/decomp/vc6-matches.txt`:
- multisave: `1 units: 7 match, 0 wrong target, 1 differ`. 5 matched before, 7 now, 0 lost.
- gr_opengl: `1 units: 50 match, 0 wrong target, 4 differ`. 49 matched before, 50 now, 0 lost.
- unnamed_listbox_0x52ca60: `1 units: 62 match, 0 wrong target, 7 differ`. 62 matched before, 62 now, 0 lost.

## Devices worth reusing

- A bit remap `x = ((x >> 1) & ~3) | (x & 1)` written as `fl = x; fl = (fl >> 1) & ~3; fl |= x & 1; x = fl;` changes
  8966's register choice and the operand order of the `or` (D18900, D18901).
- A `?:` whose two arms are constants, rewritten as an if / else assignment, moves the neg / sbb sequence to where the
  binary computes it (D18902).
- A search loop written with `i * fh + top` instead of hand-made running locals lets 8966 make the induction
  variables itself (D18903, closer).

## Worker 9

### W9 notes (targeted round m-1021b, decisions D19000-D19099)

Batch: Character::Character, Character::say (living_entity.cpp), ai_state::load, cast_effect_track_add (ai.cpp),
matrix3_pack_orientation (unnamed_goals_doors_0x475180.cpp), pick_test_character (gametarget.cpp),
item_page_out_all (item_paging.cpp).

## New matches

None applied in `src` (the one match below needs files outside my batch, so it is a separate patch).

- **Proposed, verified: game/living_entity.cpp 0x44e060 Character::Character, D19000.** It matches once the D1
  union around Character +0x558 is dropped. This is the "To do" of D1 / D300 / D1702. The patch is in
  `D19000-living-combat-ctor.diff`, and the patched files are in `alt/` (alt/include, alt/game). It changes five
  files, three of them outside my batch:
  - include/engine/objects/living_entity.h: `union { living_combat combat; living_combat protection; };` becomes
    `living_combat combat;` (same offset +0x558, same size 0x54; LAYOUT_CHECK unchanged).
  - include/engine/combat/combat.h: `living_combat *construct();` becomes the constructor `living_combat();`.
  - game/combat.cpp: `living_combat *living_combat::construct()` becomes `living_combat::living_combat()`, and
    `return this;` goes (a constructor returns this in EAX by itself). 0x4681b0 still matches (22/22 bytes).
  - game/new_assess.cpp: `target->protection.get_protection()` becomes `target->combat.get_protection()`. That
    was the only use of the alias. The unit stays at 17/18, the same as before.
  - game/living_entity.cpp: `combat.construct();` comes out of the constructor body. VC6 then builds combat
    between ai and timer6d4, as the binary does. The unit goes from 125 to 126 of 132.
  Checks: living_combat is not used by value anywhere else (no locals, no other unions), and nothing else calls
  construct(). The behaviour is the same: the same constructor runs once, and the vtable / type stores come after
  it, as in the binary. Merge note: vc6-matches.txt may list 0x4681b0 under the name living_combat::construct,
  which is now living_combat::living_combat.

## Closer, kept

- game/gametarget.cpp 0x43c620 pick_test_character: 24 -> 2 (**D19001**). Three changes:
  - The character branch puts the height in a local and stores `off.x = 0; off.z = 0; off.y = height;`. This
    gives the binary's x, z, y store order without the z store being cross-jumped into the container branch.
  - `d2 = d.y * d.y + dz2; d2 += dx2;` gives the binary's (dy2 + dz2) + dx2 order.
  - `float h2 = dz2; h2 += dx2; if (h2 < r ...)` keeps dx2 on the FPU stack as the binary does
    (`fld dz2; fadd st, st(1)`, popped after the test).
  What is left: `add esp, 8` after the distance() call is placed before the `fcomp best_r`, where the binary puts
  it after. That is one scheduling pair. The behaviour is the same (the same values and tests).
- game/ai.cpp 0x4747e0 ai_state::load: 36 -> 18 (**D19002**). Reading owner directly (no `o` local) gives the
  binary's EAX/ECX/EDX rotation through all the goal copies. What is left: 8966 loads owner into EDX before the
  combat_style store (the binary loads it into EAX after the store), and the arg copy after the target join uses
  EAX where the binary uses EDX. An earlier round recorded the no-`o` form as "hoists the owner load" and kept
  `o`. Under the new distance score, the no-`o` form is closer.

## Still differs (tried; do not repeat)

- **Character::say 0x461dc0 (25)**: this is loop-tail scheduling and the strncat argument load order (the binary
  loads and pushes lens[i] before it loads offsets[i]). Tried, and none changed the code: `total = lens[i] + total
  + 1`, an `int *lp = &lens[i]` pointer local, a len local, a src pointer local, `total += lens[i]; total++;`, a
  bool for the test, `total + lens[i] + 1` order, `1 + lens[i]`, `>= 0x100` (27), an `r->text` pointer local, a
  lens pointer walk (146), a `need` local, `&quoted[offsets[i]]`, `character_speech[slot].text`,
  `offsets[i] + quoted`, an `(unsigned)` length cast. Treat it as exhausted.
- **matrix3_pack_orientation 0x475380 (20)**: the instructions are identical. Only the frame differs: the binary
  puts the first copy `a` in the dead out / m argument slots (frame 8, against our 16). Tried:
  - local types and declaration order: a file-local POD `f2` struct (20), b declared before a (14: the frame is
    still 16), direct-init, a source pointer local;
  - store forms: struct stores into out (20) and through f2 (44), direct f2 copies (36), `float *d` stores;
  - an `ab[2]` array (26);
  - inline helpers: one returning vector2 by value (20), by-value vector2 parameters (one or two pairs: 14);
  - temporaries straight from the return value (18, frame 8, but nothing in the argument slots);
  - copying out and/or m into locals at the top or in the middle (all 20: the copies are propagated away).
  The binary's form was not found.
- **pick_test_character**, beyond D19001: the add esp / fcomp order. Tried, all still 2: nested if with continue,
  a sep local (raw and halved, `sep *= 0.5f`), a bool take flag, `>=`, `best_r >` and `best_r <=` forms,
  `0.5f * distance`, `/ 2.0f`, a best-pos pointer local, splitting `!best ||` into an if. A `best_r` local gives
  13. Also tried for the earlier differences: an `sq[2]` array (104), a pd pointer, dy2 locals, an
  `h = dz2 + dx2` local before d2 (24), dz2 / dx2 at function scope, `half` local (94), dx2 declared first
  (107), `vector3(0, h, 0)` (53), chained x/z stores.
- **item_page_out_all 0x46ca50 (30)**: slot and the zero register are swapped (EBX / EBP). Nothing else differs.
  Tried, all unchanged: `item_page_slots + slot`, `register` s, s / other / i declared at the top, mesh `= 0`, the
  final-store order, a chained -1 (32), `slot != i`, the popup test operands swapped (89), a copy of slot, a type
  local, a while loop. Treat it as register choice only.
- **cast_effect_track_add 0x4750b0 (40)**: the counter / pointer registers are swapped in the free-slot loop
  (EAX / ECX), and the rotation carries on after it. Tried: an unsigned i with `(int)i <` (40) and plain unsigned
  (42), an `e` pointer local for the stores (52), i declared first or block-scoped, a frames local, a vfx_base
  local, a `for (;;)` search (61). Treat it as register choice only.
- **ai_state::load**, beyond D19002: the combat_style store through a pointer local / an int local / a `self`
  pointer / an int-pointer read, n initialised later or through `n = 0; ... n += 4`, declaration orders, an int
  type local, a const / obj `o`, `o` assigned separately or in the condition (36), `NPC || MONSTER` (22), an
  early return (52); the arg copy as an int-pointer store, ternary target (20), `c == &living_list` first (35),
  an int local. A float local or parentheses give 16, but with fld / fstp where the binary uses an integer copy,
  so I did not keep them.

## Behaviour errors / bug candidates

None found. The game bug already noted for matrix3_pack_orientation (the up row's pair is copied from rvec
again) is unchanged.

## Header changes

None in `src/include`. The D19000 proposal (in `alt/` and the .diff only) changes include/engine/objects/
living_entity.h (the union becomes one member) and include/engine/combat/combat.h (construct() becomes the
constructor). Both are needed for the Character::Character match: a union member cannot have a constructor, and
the binary calls 0x4681b0 inside the member construction sequence.

## Changed files (in W/src)

- game/gametarget.cpp (pick_test_character, D19001)
- game/ai.cpp (ai_state::load, D19002)

Outside src: `D19000-living-combat-ctor.diff`, plus `alt/` (the patched copies of the five files and a copy of
include/). Helper scripts and variant files are in `v/`.

## --check results

`VC6_STRICT=1 vc6-sweep --only "game/(ai|gametarget)\.cpp" --check src/decomp/vc6-matches.txt`: 2 units, 102
match, 8 differ; 102 matched there, 102 now, **0 lost**.
For the D19000 patch (compiled from alt/): living_entity 126 match (was 125), combat 52 (was 52, 0x4681b0 still
matches), new_assess 17 (was 17).

## Decisions

**D19000 (include/engine/objects/living_entity.h, include/engine/combat/combat.h, game/combat.cpp,
game/new_assess.cpp, game/living_entity.cpp; proposed patch, not in src). The D1 union at Character +0x558 is
dropped.** combat is a plain `living_combat` member, and living_combat::construct() becomes the constructor
living_combat::living_combat() (0x4681b0, still exact). new_assess.cpp reads `target->combat`. VC6 then builds the
member between ai and timer6d4, and Character::Character 0x44e060 is exact. This completes the To do of D1 / D300 /
D1702.

**D19001 (game/gametarget.cpp). pick_test_character 0x43c620: 24 -> 2.** The character branch stores
`off.x = 0; off.z = 0; off.y = height` through a height local; d2 is summed as `d.y * d.y + dz2` then `+= dx2`; and
the horizontal test uses `h2 = dz2; h2 += dx2;`. Left: the `add esp, 8` / `fcomp best_r` order after distance().

**D19002 (game/ai.cpp). ai_state::load 0x4747e0: 36 -> 18.** owner is read without the `o` local, which gives
the binary's register rotation in the goal copies. Left: the owner load is hoisted above the combat_style store,
and the arg copy uses EAX instead of EDX.

## Worker 10

### m-1021b worker 10 (D19100-D19199)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/unnamed_serpent_0x495290.cpp | 0x4960f0 | serpent_part_is_busy_or_asleep | D19100 | `list[i].character` / `list[i].flags` indexed directly, no `boss_target *t` pointer local |
| game/unnamed_camera_0x4778b0.cpp | 0x4787b0 | camera::auto_view_update | D19101 | the 5 s re-arm and view_low as one if / else (no `return` inside the tail), so the region after enemy_near_leader has one exit |

**D19100 (game/unnamed_serpent_0x495290.cpp). serpent_part_is_busy_or_asleep 0x4960f0 is exact** with
`Character *c = list[i].character; if ((list[i].flags & SERPENT_TF_PARALYZED) || asleep) ...`. Without the
`t` pointer local 8966 addresses both fields as `[esi + edi]` (i * 16 + list), loads the flags byte into BL
with EBX saved only around that test, and computes the dead `lea ecx, [esi + edi]`, as the binary does. With
`t` it adds list into ESI once (`add esi, edi`) and needs no EBX. `Character *c = list[i].character;
boss_target *t = &list[i];` (c first, t used for the flags) matches too. Did not help: a ubyte / uint flags
local, a bool for the paralysed test, separate ifs, a reference `boss_target &t`, an `ai_state *` local, a
copy of list, the early `return false` for i == -1. Same behaviour (the same two fields are read).
serpent_part_is_busy 0x4960a0 keeps its `t` form (it matches as it is).

**D19101 (game/unnamed_camera_0x4778b0.cpp). camera::auto_view_update 0x4787b0 is exact** with
```
} else if (!near_enemy) {
    if (enemy_near == true)
        auto_view_timer.SetTimeout(5000);
    else
        view_low();
}
enemy_near = near_enemy;
```
in place of `if (enemy_near == true) { SetTimeout(5000); enemy_near = near_enemy; return; } view_low();`.
This is D8701 again: 8966 shrink-wraps a callee-saved push into a region only when that region has one exit.
The old tail had two (the `return` and the end), so EBX was pushed at the entry; now `push ebx` sits after the
follow-flag test, as in the binary, and 8966 duplicates the epilogue per path itself. A `goto done` to the final
store also matches; a `goto` past the final store (keeping the store in the branch) does not. Same behaviour:
both paths still set enemy_near = near_enemy, the 5 s path without calling view_low.
Device worth reusing: where the binary pushes a callee-saved register late and the source has an early
`return` in the tail, fold the return into an if / else.

## Closer, kept

None.

## Still differs (tried; do not repeat)

**D19102 (notes only). dialogue_box::draw 0x4d7680 (distance 19) and dialogue_topic_list::draw 0x4d7930 (29).**
The binary loads `first` into a register of its own each pass (`mov ebp, [esi + 0x20]; mov eax, ebx; add eax,
ebp`; EDI in the topic list, the register that then holds the entry), ours adds from memory (`mov eax, [esi +
0x20]; add eax, ebx`). The topic list also has its lea / final set_clip registers one name off, probably the
same cause. Tried on dialogue_box::draw this round, all with the same code: an idx local ((i + first) and
(first + i)), `int f = first` in the loop and at function scope (also `register`), `idx = first; idx += i;
idx %= 64`, `idx = i + first; idx = idx % 64; l = line + idx`, `n = i; n += first`, f assigned then reused as
the index, idx / l at function scope, the visible test split (`page = font height; visible = page * lines`),
visible computed before l, visible inlined into the test, nested ifs, a while loop, a `dialogue_box *b = this`
copy, a `dialogue_line *base = line` copy, file-local inline helpers returning the line (this + n; line + first +
n), `register int i`, i declared before cx..ch, unsigned i with `(int)i < count` and casts on the sum, and an
unsigned first. The D17703 split that matched last_visible_line does not apply here (the font-height product
is not the issue). Not retried on the topic list (same shape).

**D19103 (notes only). vif_record_get_bounds 0x533aa0 (distance 16).** Only the fild temporary of `1.0f /
count` differs: the binary uses radius's dead argument slot, ours rec's. Tried: a rec pointer copy (for the
divide, for the first loop, for the whole path block, for the spread reads, unused), a radius copy, a centre
copy, a float or int count local, an `inv` local, `c /= (float)count` (worse), `c = c * (...)` (worse), a ubyte
count local (worse), `register` on rec / radius, the tail stores swapped (worse), farthest / c / i declaration
orders, the path locals at function scope (worse), farthest declared at its assignment, the tail in a block,
the divide in a block, a dead early `farthest = 0`, farthest set in the for-init (`farthest = 0, i = 0` and
`i = 0, farthest = 0`), `farthest = 0` (int). Finding: assigning `farthest = 0.0f` before the divide (after
c.zero() or right before the divide) does put the temporary in radius's slot (`16+[esp+40]`, the binary's
0x38), but the farthest store then moves up with it (distance 20); the binary has the store after the divide's
loop guard. So the slot follows something that is live across the divide; not found.

**D19104 (notes only). interface_init_global 0x4116c0 (distance 20).** The binary stores 640 to base_w inside
the out-of-line 0 / 1 / default block and keeps base_w in memory and screen_w on the x87 stack. Tried this
round (all with the stores in the switch): base_w as `float[1]`, as `float w[2]` (also with screen_w in w[1]),
as a one-member struct, through a `float *` (stores and division, or stores only), h_fov declared first,
`case 0: case 1:` before case 2, separate case 0 / case 1 / default bodies (8966 then builds a dec chain),
screen_w as a switch, as a three-way if / else, in a block, `register` screen_w, screen_w declared first, and
both the pre-switch init and the in-case stores. With the stores in the cases either base_w goes to the FPU
(plain) or both floats go to memory (array / struct / pointer forms); merged default labels change the switch
lowering (`sub eax, 2; je`). Left as D16503.

**D19105 (notes only). pickup_draw_item_list 0x421370 (distance 32).** x is EBP and i EBX in ours, the reverse
in the binary; everything else is the same. Tried this round: unsigned i with `(int)i < end`, a pickup_point
pointer local for the origin, i at function scope (after buf and first of all), i declared before the for,
`register` x and `register` i, the body in `if (pickup_container) { }` (one exit), `item_obj *it` at function
scope, `x + button_x` operand orders, and a button-x local (worse, the four calls no longer cross-jump). x / y
read after the font height is worse. With the 23 earlier forms this looks like 8966 register choice.

## Behaviour errors / bug candidates

None found. Both changes keep the behaviour.

## Header changes

None.

## Changed files

- src/game/unnamed_serpent_0x495290.cpp: serpent_part_is_busy_or_asleep (body and comment, D19100, confidence
  exact).
- src/game/unnamed_camera_0x4778b0.cpp: camera::auto_view_update (tail and comment, D19101, confidence exact).

The other batch files (unnamed_dialogue_box_0x4d6660.cpp, unnamed_vif_0x5329f0.cpp,
unnamed_interface_0x411270.cpp, new_pickup.cpp) are unchanged.

## --check result

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "unnamed_serpent_0x495290|unnamed_camera_0x4778b0"
--check src/decomp/vc6-matches.txt`: `2 units: 86 match, 0 wrong target, 4 differ, 0 not found, 0 errors` and
`84 matched there, 86 now, 0 lost`.

## Worker 11

### m-1021b worker 11 (D19200-D19299)

Batch: choose_support_spell, combat_toggle_ready_stance, state_title_enter, travel_steer_update,
ui_gadget::send_event, StringPool::Add, pxochat_page_down.

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/new_pxochat.cpp | 0x424170 | pxochat_page_down | D19200 | mp_charselect_page_down's form (D15700): the page size in a local, `pxochat_list_count[list] - page` written out at each use (test, clamp test, clamp value), no `last` local |

**D19200 (game/new_pxochat.cpp; m-1021b worker 11). pxochat_page_down 0x424170 is exact** with
`int page = pxochat_list_page[resolution_index][list];` and `pxochat_list_count[list] - page` written at each of
its three uses (as mp_charselect_page_down, D15700). With a `last = count - page` local (and with `page` and
`last` both as locals) 8966 gives the opposite clamp branch and the list in ECX instead of EDX. Writing the whole
expression without a page local (`count - pxochat_list_page[resolution_index][list]` three times) also matches.
Same behaviour.

## Closer, kept

| file | address | name | old -> new |
|---|---|---|---|
| vsdk/stringpool.cpp | 0x544c40 | StringPool::Add | 24 -> 22 |

**D19201 (vsdk/stringpool.cpp; m-1021b worker 11; closer only). StringPool::Add 0x544c40:** `new_length`
computed before `dest` (statement order) moves the strlen ahead of the length load as in the binary; s is still in
EBP (binary ESI), new_length in EAX (binary EBP), dest in EBX. Same behaviour.

## Still differs (tried; do not repeat)

**D19202 (notes only; m-1021b worker 11). Tried without a match:**

- state_title_enter 0x431e20 (distance 14): the binary keeps game_flags in EAX and the ternary in ECX, with the
  `and eax, 0x1c` scheduled before the compare; 8966 gives the opposite roles for every form. Tried (19, all the
  same code unless noted): pointer local to game_flags (three placements), a pointer local plus mask local, a
  bool local for the compare, mask local first (`f = game_flags & 0x1c; f |= ...`), `f = game_flags; f &= 0x1c`,
  `(prev == 14) + 1` both operand orders, int and unsigned locals for it, two statements (`&=` then `|=`),
  `^` and `+` for the merge, an unsigned char local (worse), `~0xffffffe3`, an unsigned cast; if / else with two
  stores and an if-set default (both branch, worse). With earlier rounds (D11910, D12707, the volatile read) this
  is exhausted: register choice.
- ai_state::travel_steer_update 0x4b3c80 (distance 17): only operand orders differ: the facing dot product (ours
  y, z, x; binary z, y, x) and the side vector `fvec + rvec` (ours rvec first in all three components; binary
  rvec first in x only, fvec first in y and z). Tried: dot as the four other term orders (xyz / xzy: x, y, z
  rotated, worse; yxz / zxy: unchanged), `z + (y + x)`, fvec operand first, `d.dot(fvec)` / `fvec.dot(d)`, a
  `const vector3 *fv` pointer local, a `const vector3 *dp = &d` (all three worse, 72-80); side vector: fvec / rvec
  pointer locals (both: unchanged; one: worse), an owner local, an fvec copy, `rvec + fvec`, component-wise
  `vector3(...)` in two operand orders (dot then changes to z, x, y), `d.set(...)`, `d = fvec; d += rvec`.
  All worse or unchanged.
- ui_gadget::send_event 0x52c330 (distance 20): an ESI / EDI swap (this / value). Every form that returns the
  callback's result through a variable gives the swap: register value local, register self pointer, value
  pointer, a callback local, a `for (;;)` wrapper, `if (callback && (handled = callback(...))) return handled;`
  (bool, `!= false`, int, initialised), r = ... chains joining the window call (four shapes), a goto to a shared
  return. `if (callback && callback(event, value)) return true;` (and `if (!callback || !callback(...)) {...}
  return true;`) gives the binary's registers (distance 15) but a separate `mov al, 1` tail where the binary
  jumps to the window call's epilogue and returns the callback's own AL; not kept (it would return 1 for any
  nonzero AL).
- ai_state::choose_support_spell 0x4bea80 (distance 20): the EBX / EBP swap (binary: cure and then out in EBX,
  vitalize in EBP). Tried: `hurt > 2 && vitalize` (worse), `vitalize != 0`, heal / cure declaration swap,
  vitalize declared first, an `out` local for the store (`boss_target **o`), the vitalize test as a ternary,
  an unsigned loop test (worse). With D16205 / D10162 (5 declaration orders, default, nested, out local,
  `(*out)->flags`) and the earlier 36 forms: register choice.
- ai_state::combat_toggle_ready_stance 0x4bdce0 (distance 24): ready in AL where the binary keeps it in BL (the
  constant 1 register), owner in ECX where the binary uses EAX in the tail, and the prologue push order. Tried 19
  combinations of the ready test (ternary, true default with if, if / else, `||`, `? 1 :`) and the toggle
  (`ready = !ready; flip = ready`, `flip = ready = !ready`, `flip = !ready; ready = flip`,
  `ready = flip = !ready`), `bool nr = !ready` then both stores (two spellings), `ready ^= 1`, an owner local for
  the tail, a stance_blend pointer, a ternary store (worse). All unchanged or worse.
- StringPool::Add 0x544c40 (now 22): besides D19201: `&pool[length]`, `length + pool`, a length local (two orders,
  worse), size_t len, a self pointer, `strlen(s) + 1`, `new_length = strlen(s) + 1; new_length += length`,
  `new_length++` split, `dest = pool; dest += length`, separate declarations, an src local, five forms without
  new_length (`length += len + 1` after the test; worse).

## Behaviour errors / bug candidates

None found. Both changes keep behaviour.

## Header changes

None.

## Changed files

- src/game/new_pxochat.cpp (pxochat_page_down, D19200)
- src/vsdk/stringpool.cpp (StringPool::Add, D19201)

## --check result (VC6_STRICT=1, against src/decomp/vc6-matches.txt)

- game/new_pxochat: 54 matched there, 55 now, 0 lost
- vsdk/stringpool: 15 matched there, 15 now, 0 lost

Helper scripts (variant runner without deletions, operand diff) are in v/ in this folder; they are not for the
repository.

## Worker 12

### m-1021b worker 12 notes (D19300-D19399)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/unnamed_editor_dialogs_0x430bc0.cpp | 0x4315c0 | editor_dialog_handle_event | D19300 | each button block has its own press test and `break` |
| game/level_scripts_common.cpp | 0x4caed0 | level_trigger_point_left_of_line | D19301 | the segment local is a `pf_edge`, not a `seg2d` |
| vsdk/gr_glide.cpp | 0x549560 | gr_glide_tcache_set | D19302 | `data` / `palette` declared inside the dirty block |

**D19300 (game/unnamed_editor_dialogs_0x430bc0.cpp). editor_dialog_handle_event 0x4315c0 is exact** when each
button test is its own `if (g == d->buttons[k]) { if (cb) result = cb(...); if (event == UI_EVENT_PRESSED) {
d->closed = true; return true; } break; }`, followed by a final `break` for a gadget that is no button. 8966
cross-jumps the identical tails: button 1's block loads callbacks[1] and jumps back into button 0's call, and the
event load is no longer hoisted above the button compares. The earlier else-if chain with one shared press test,
a callback local with goto (D13702), a `g == b0 || g == b1` test with a ternary or index, a find-only loop, and
`return result` duplicated per branch (distance 11) all differ. Same behaviour: a gadget that is no button
returns result (still false) in both forms; the third button still tests callbacks[2] and calls callbacks[1].

**D19301 (game/level_scripts_common.cpp). level_trigger_point_left_of_line 0x4caed0 is exact** with
`pf_edge seg;` (engine/ai/pathfinding.h: seg2d plus flags / door / pad / search_id, 0x10 bytes) in place of
`seg2d seg;`. The binary's frame is 0x28 with 8 unused bytes above the segment's two pointers: those are the
pf_edge's own fields. Only seg.a / seg.b are written and the call takes it as the seg2d base. Same behaviour.

**D19302 (vsdk/gr_glide.cpp). gr_glide_tcache_set 0x549560 is exact** with `ubyte *data; ubyte *palette;`
declared at the top of the `if (entry->dirty[tmu])` block instead of at function scope. 8966 then gives data
the dead `tmu` argument slot (Sum.exe `lea eax, [esp + 0x20]`) and the loop index i the frame slot; at function
scope (or at the `if (bm >= 0)` block) i took the argument slot. Moving i (function top, bottom, the dirty block)
or swapping data / palette does not do it. Same behaviour.

## Closer, kept

None.

## Still differs (tried; do not repeat)

**D19303 (notes only). Tried without a match.**

- mp_handle_it_okay 0x4719e0 (distance 15): Sum.exe threads only the state-3 path (`cmp 3; jne exit; jmp
  <complete>`) and re-tests 3 on the state-2 path. Tried here, 20 more forms: `!=` with the bodies swapped,
  goto into the shared block, return after complete, do / while(0) and for(;;) with break, a volatile inner
  read, an inline helper for the inner if, switch with `case 2:` falling into `case 3:`, switch with the inner
  if in case 2 plus case 3, goto from case 2 into case 3, `if (2) {...} else if (3)` and its early-return form,
  duplicated `return`s, `case 2: case 3:` (either order, with default / return), `state == 3 || state == 2`.
  All thread both paths or neither.
- glide_record::load_section 0x549a20 (21): all 720 orders of the six tail stores (best 16, none closer in shape;
  the binary's order is the current one), `float uv[2]`, pointer copies of sec / this / u_scale / v_scale, `&v`
  through a pointer, the bitmap index as a local or a base pointer local, u / v declared at the top or in a
  block, v declared before u (20), `this->v_scale = *v_scale`, v in the section argument, size declared at the
  top, info declared first. A volatile read of v for the last store gives the binary's reload and is 4 bytes
  closer (20) but not kept (not a plausible source). Left: Sum.exe copies v (int) into the dead section slot with
  ECX and reuses ECX for glide_tcache_entries early, so the last store reloads v; 8966 keeps v in EAX and stores
  it early.
- decals_draw_for_lobj 0x4bc6b0 (17): two scheduling hunks (the tri[2].z store after the num_planes load; the
  `edi` / `ebp` induction pointer inits). Tried: a pointer local for verts[f->v[2]], component copies,
  a POD struct copy of tri[2], tri[0..1] or all three (D17201 device: 71-119), the vector3 copy constructor,
  a num_planes local, a projector pointer local, a face index pointer, a verts pointer copy, a clipped pointer
  local in the vertex loop, the u-plane statement before the flag store, a gr_vertex pointer local, vert_flags
  as int. Nothing closer.
- script_action_replace_weapon 0x4dccb0 (32): only c (ESI in Sum.exe) and count / `a` (EDI) are swapped. Tried:
  pointer copies of c (first, last, `register`), of a, of both in either order, a count local, an unsigned
  index with a signed test, the test reversed, a record pointer local, a continue form, an inline search
  helper (66), an unused arg0c local. None moves the registers.

## Behaviour errors / bug candidates

None new. (editor_dialog_handle_event keeps its known game bug: the third button tests callbacks[2] and calls
callbacks[1].)

## Header changes

None.

## Changed files

- src/game/unnamed_editor_dialogs_0x430bc0.cpp (editor_dialog_handle_event, comment)
- src/game/level_scripts_common.cpp (level_trigger_point_left_of_line, comment)
- src/vsdk/gr_glide.cpp (gr_glide_tcache_set, comment)

## --check results (VC6_STRICT=1)

- unnamed_editor_dialogs: 3 matched there, 4 now, 0 lost
- level_scripts_common: 73 matched there, 74 now, 0 lost
- gr_glide: 55 matched there, 56 now, 0 lost
