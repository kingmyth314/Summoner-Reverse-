# Byte-match round 2026-10-21, third: the fourteen workers' notes

The 50 functions still within distance 12 (tried twice before, now with the second round's devices) and the 63 at
distance 41-80, in fourteen batches of whole files. Result: 19 new matches (strict 5,501 -> 5,520, none lost), about
25 closer. Not merged: worker 14's `phoenix_anim_remap_in` change (a `volatile` local for a non-match, 78 -> 70).
Worker 6 ran some commands from the repository folder and left scratch listings there; it moved them out itself.
The decisions are D19400-D20707 in include/DECISIONS.md.

## Worker 1

### m-1021c worker 1 (D19400-D19499)

Batch: 9 functions. 1 new match, 3 closer (kept), 5 unchanged.

## New matches

- vsdk/unnamed_ca_mesh_0x50af80.cpp 0x50b150 ca_render_morph_vertices: **D19400** (distance 62 -> match).

**D19400 (vsdk/unnamed_ca_mesh_0x50af80.cpp). ca_render_morph_vertices 0x50b150 is exact** with the instance
read through a local `character_instance *ci = ca_render_instance;`, used both for the anim choice and for
`ci->times[anim]` in the loop, and the anim written as a store then an if (`anim = ci->action_anim; if (anim
== -1) anim = ci->state_anim;`). 8966 then propagates ci back to the global (the binary reloads
ca_render_instance in the loop instead of holding it in EBP), stores the anim before the test, and pushes
EBX / ESI / EDI / EBP late, as the binary does. The pointer local alone gives 51; the ternary or an if / else
on `ci->action_anim != -1` stays at 51; the if form without ci (87). Same behaviour (no call in the loop can
change the global). DEVICE note: a local copy of a global pointer can make 8966 *not* cache the global in a
callee-saved register (the copy is propagated, the hoisted global load is not formed).

## Closer, kept

- game/unnamed_render_scene_0x481680.cpp 0x483570 levelrender_add_items: 52 -> 6, **D19402**.
- game/unnamed_render_scene_0x481680.cpp 0x4836f0 levelrender_add_containers: 54 -> 6, **D19402**.
- game/unnamed_tiger_0x499e50.cpp 0x49c130 tiger_anim_remap_in: 78 -> 36, **D19401**.

**D19402 (game/unnamed_render_scene_0x481680.cpp; closer only). levelrender_add_items 0x483570 and
levelrender_add_containers 0x4836f0: the distance test as `double dz = d.z, dy = d.y; if (d.x * d.x + (dy * dy
+ dz * dz) > 400.0f)`.** The double copies are FPU-register candidates (a float copy is propagated back to the
struct member and never enregistered), which gives the binary's "keep d.y / d.z on the FPU stack and square
copies" (`fld st(0); fmul st, st(1); fld st(2); fmul st, st(3)`, x from memory, `fstp st(0)` twice after the
compare) - the pattern earlier rounds called not reproducible. The parenthesised y / z pair puts x's product
last (without the parentheses 16 / 18). Left in both: 8966 compares the double sum with a QWORD 400.0
(`__real@8@...`), the binary with the float constant 0x58192c (dword) - so in the original the sum was float
typed. Not found: `(float)` casts on the pair or the whole sum (12: the cast ends the copies' lifetime and they
are popped before x's product), a float sum local (22), a bool inline helper taking a float (12), float-returning
inline helpers with double / vector3 params (12-47), float copies in any spelling (`register`, two statements,
via a pointer local, `* 1.0f`, unary plus, assignment inside the expression, a float[2]: all 52), vector3
temporaries / len_sq (52-86). Same values: the binary's x87 code never rounds the products or sum to float
either, and 400 is exact in both. Also worth trying on levelrender_add_lobj (0x4838f0, distance 178, same
pattern per earlier notes) and build_draw_lists' 0x482f46 test (three components kept on the stack).

**D19401 (game/unnamed_tiger_0x499e50.cpp; closer only). tiger_anim_remap_in 0x49c130: the weight store
written in every branch, including both arms of the ready test** (`if (ready) { slot = 3; weight = ...; } else {
slot = 0; weight = ...; }`, no `continue`). This gives the binary's zero in EBP shared by the mount test, ready
and i, ready kept in c's dead argument slot and re-read in the loop, the 3 / 0 stores unfolded (one shared
weight store folds them into neg / sbb / and), and both arrays addressed from the one induction pointer on the
set (`[edx + eax]` / `[esi + eax]`). Left: the three loop registers rotated (base of the slots, base of the
weights and the temporary are ECX / EDX / ESI in ours, EDX / ESI / ECX in the binary), so push esi comes three
instructions earlier. On the way: `volatile int ready` (71; with the nested form 43; plain int is better, 36),
weight store per branch but one in the stand case (57), `tiger.remap_slot[i] = 0; if (ready) ... = 3;` (55).
Not changing the rotation: slot / weight pointer locals in both orders or either alone, `register int a`,
operand order of the compares, a float local for the weight, unsigned index, no `a` local, `*(set->anim + i)`,
a continue / early-continue in the arms, `!ready` first (38), weight before slot (47), a `slot` local with one
store (83), `int i` / `int a` at function scope, set declared late, the rider test as `(r = ...) != NULL`.
The serpent / phoenix / ghost copies (serpent_anim_remap_in etc., not in this batch) were said to differ "the
same way": the per-branch weight store is worth trying there.

## Still differs (tried; do not repeat)

**D19403 (notes only). Tried, still differing.**
- mp_handle_swi_path 0x4882f0 (distance 2; 17 forms): file-local inline helpers around start_travel (bool by
  value, `const bool &`, `bool *`, `ai_state *` + bool, a helper taking its own address of the parameter) -
  by-value forms lose the slot (walk in a register, worse), pointer forms stay at 2; `*(volatile bool *)&walk`
  (worse), a bool[1], a one-member struct (worse), `ai_state *` first then the pointer, the pointer created
  first and stored through, with `start_travel(walk)` or `(*wp)`. The `this` lea before the walk load is
  scheduling; stop.
- levelrender_push_attachment 0x483430 (distance 4; 17 forms): the radius through a pointer local (store or
  load), a const float pointer to mesh->radius, memcpy of the radius (all 4); a vim pointer local (4); the radius
  assigned after d (4); `e->dist = (e->depth = depth) - radius` (4); a pointer to e->depth (4); a radius copy
  before the subtraction (12); a fvec pointer for the dot (14); `fvec.dot(d)` (9); dist before depth (11);
  `float dist = depth; dist -= radius` (4); a float[2] for depth / radius (34); a local {vector3, float} sphere
  struct copied from the mesh (55); memcpy of the centre (47); a float[4] copy (65). Scheduling; stop.
- levelrender_add_effect_entry 0x4841e0 (distance 8): not attempted beyond reading (49 store orders in earlier
  rounds; the `pop esi` placement is scheduling).
- level_vis_reset 0x487030 (distance 12; 7 new forms): an index loop with an int pointer to .count, a record
  pointer local inside an index loop, a lists local, a file-local inline helper for the three stores (index
  form 23; pointer form 12, unchanged), a `one` local read through a pointer, an unsigned index with a signed
  test (all index forms 23: the constant 1 CSE'd into a register). Not reproduced.
- spline::eval 0x558f40 (distance 8; 8 new forms): an `out` pointer local, the z or y sum through a float
  local, a `const vector3 *q = p` for the y or z line, a `const float *` to b0 in the z line (all 8), a
  `const float *` to b1 declared at the top (24), the x sum through a vector3 local (189). The pops of EDI / ESI
  are scheduled; stop.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/vsdk/unnamed_ca_mesh_0x50af80.cpp (ca_render_morph_vertices, D19400)
- src/game/unnamed_render_scene_0x481680.cpp (levelrender_add_items, levelrender_add_containers, D19402)
- src/game/unnamed_tiger_0x499e50.cpp (tiger_anim_remap_in, D19401)

## --check result (VC6_STRICT=1, against src/decomp/vc6-matches.txt)

`--only "unnamed_render_scene_0x481680|unnamed_ca_mesh_0x50af80|unnamed_tiger_0x499e50|unnamed_swi_0x488230|unnamed_bezier_0x558a30"`:
5 units: 141 match, 0 wrong target, 25 differ, 0 not found, 0 errors; 140 matched there, 141 now, **0 lost**.

## Worker 2

### m-1021c worker 2 notes (D19500-D19599)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/pathfinding.cpp | 0x49db20 | seg2d_closest_point | D19500 | the line local as a `pf_edge` (D19301's device) and the three end-point copies as `memcpy` |
| vsdk/unnamed_d3d_tcache_0x54bb30.cpp | 0x54d120 | gr_d3d_create_texture | D19501 | the Glide twin's shape (x = 0 before the row height, both clamps as if / else) and the x store through `e->sections[section].x` before `s` is taken |

**D19500 (game/pathfinding.cpp). seg2d_closest_point 0x49db20 is exact** with `pf_edge line;` in place of `seg2d
line;` and `memcpy(out, s->a / s->b, sizeof(vector2))` for the three copies. The binary's frame is 0x18: perp at
the bottom, the line's two pointers above it and 8 bytes nobody touches at the top; those are the pf_edge's own
fields (flags / door / pad / search_id), as in D19301. With pf_edge alone the frame is right (58 -> 44) but the
copies still differ: `*out = *s->a` lets 8966 reuse ESI (`mov esi, [esi]`) where the binary loads the end point
into a scratch register and keeps s; memcpy gives the binary's form in all three places. A pointer local for the
end point, a pointer copy of s, per-member copies and `a` / `b` pointer locals for the perpendicular did not.
Same behaviour (the line is still passed as its seg2d base; only a / b are written).

**D19501 (vsdk/unnamed_d3d_tcache_0x54bb30.cpp). gr_d3d_create_texture 0x54d120 is exact** with the section loop
as in gr_glide_create_texture: `x = 0;` before the row height, `if (h - y > gr_screen.max_bitmap_h) sh =
max_bitmap_h; else sh = h - y;`, the width as `if (w - x > max_bitmap_w) s->w = max_bitmap_w; else s->w = w - x;`
(the binary re-reads s->w for `x += s->w`), and the first store written `e->sections[section].x = x;` before
`s = &e->sections[section];`. The if / else clamps took it from 79 to 19 (and put e in EDI, ny in ESI as in the
binary); the indexed x store then loads the sections pointer before y, as the binary does (`s = e->sections +
section`, `section + e->sections`, a base pointer local, `s->y` first, a y copy or pointer did not). The min()
forms in both operand orders, with and without x first, stay at 71-81. Same values (min with the same ties).

## Closer, kept

- game/pathfinding.cpp 0x49ecb0 pf_find_cell_crossing: 65 -> 21 (**D19502**). `pf_edge seg;` gives the binary's
  frame 0x28 (the 8 untouched bytes below the EH record are the pf_edge's spare fields), and `bool found = false;
  float best = 1e9f;` declared before the `profile_label` puts their stores first in the prologue, before the EH
  state store, as in the binary. Left: in the edge loop the binary loads the count into EDX and the edges into
  EAX and stores e (to `from`'s dead argument slot) after the count test; 8966 stores e before the test and the
  register choices in the loop differ (e / hit / seg in EDX / ECX / EAX in the binary).
- game/spells.cpp 0x47f4d0 spell_roll_amount: 55 -> 6 (**D19503**). `for (i = count - 1; i >= 0; i--)` (noted as
  closest in D13504; kept now that the distance shows it). Left only: the binary tests `count > 0` (`test eax, eax;
  jle`) and then does `dec eax; lea esi, [eax + 1]`; 8966 does the dec first and tests the result (`jl`).

## Still differs (tried; do not repeat)

**D19504 (notes only). Tried without a match:**
- pf_read_edges 0x49c970 (distance 2, `lea ebp, [eax]`; this is the only `lea reg, [other reg]` with no offset in
  the whole of Sum.exe's code; the others are padding or data): `for (unsigned i = 1; i <= (unsigned)n; i++)`
  (adds `cmp eax, 1; jb` and still `mov`), `unsigned i = 0; i <= (unsigned)n - 1` and `(unsigned)(n - 1)` (same
  as base), `for (i = 1; i <= n; i++)`, `i <= n - 1`, `i != n + 1`, `i < count + 1`, unsigned down-counters
  `>= 1` / `> 0`, an unsigned copy of the count, do / while up-counters against `*count` (re-read each pass: 82-95),
  a pointer end (`first + ++n != end`), `while (--n != 0)`, `(n -= 1) != 0`, chained `n = *count = read_int()` /
  `*count = n = ...` (worse), n set before `first`, `int *np = &n; *np = *count;`, `n = (int)&((char *)0)[*count]`.
- pf_read_region 0x49cc50 (77): the binary keeps i in EDI and indexes `[reg + 8*edi]`, with EBP the zero / k
  offset reset at each row; 8966 hoists i * 8 into EDI and spills i. Unsigned i with `(int)i <` (D17702), unsigned
  i and k, k = 0 at the row top, k / n / row declared in the blocks, `row = grid->rows + i`, `_pad02` through a
  pointer, `row->cells = ...`, `grid->rows[i].cells + k`, row taken before the num_cells store: all 77 or worse.
- gr_bitmap_3d_oriented_quad 0x53edd0 (8, the fstp of v[3].x after the integer stores and the width push): codes_and
  between the x and y stores in 11 placements of x / y / codes_and among v[3]'s stores, a width copy for the
  far-clip call, x / y through float pointer locals, a gr_vertex pointer for x / y, codes_and through a ubyte
  pointer, the far-clip argument through `const float *`, `off` through a pointer, a pnt->x copy, the scale.x
  factor through a pointer, codes_and / far-clip in a `for (;;) { ... break; }`. All 8 (or worse). This file is
  /Ob0, so the inline-helper device (D17300) does not apply. Looks like 8966 scheduling.
- pf_find_cell_crossing 0x49ecb0 (21 after D19502), the edge loop: `e = c->edges;` before the for, `e` first in the
  for init, `e++, i++`, an n local, the edges through `pf_edge *const *`, the count through `const int *`, e / i
  declared in the loop block (either order), e declared first / last, i declared first, `hit` before `seg`, a while
  loop, a seg2d pointer for the call, the result in an int local: all 21. Index forms (`&c->edges[i]`, e from the
  index, `e++` in the call) 78-111; `if (num_edges > 0) do {} while` 27; an `if` guard around the for 67.
- spells_parse_table 0x47a190 (41, EBX / EDI swapped for cls and the record, and the binary's EDX for every `&tf`):
  `spells[i].spell_class = cls = ...`, a `spell_info *rec` for the class store (and for `first`), the class store
  through an `int *`, sound declared before cls, cls at function scope (after prev_class, or first): all 41.
- spell_roll_amount 0x47f4d0 (6 after D19503): `i = 1; i < count + 1`, `i = 1; i <= count` (53), `i = -1; i <
  count - 1`, `i = count; i > 0` (i-- and --i), `i < count - 1 + 1`, unsigned `(int)u < count`, `count > i`,
  `i - 1 < count - 1`, `i = count - 1; i > -1`, unsigned `count - 1` down to `(int)u >= 0` (6, the same), an `int
  last = count - 1` with `i <= last` (6), and with an explicit `if (count > 0)` guard: the for (121), do / while
  `--i >= 0` (51), `i-- > 0`, up-counting do / whiles, a while: all worse.
- mp_player_file_write 0x46d0b0 (63): not tried further; the side-by-side shows only the scratch-register rotation
  of 8966 (EAX / ECX / EDX for the `&f` lea and the pushed values, e.g. the binary loads the value before `lea ecx,
  f`), the kind of difference the earlier rounds could not steer.

## Behaviour errors / bug candidates

None found. D19500-D19503 keep every value and store.

## Header changes

None.

## Changed files

- src/game/pathfinding.cpp (seg2d_closest_point D19500; pf_find_cell_crossing D19502)
- src/vsdk/unnamed_d3d_tcache_0x54bb30.cpp (gr_d3d_create_texture D19501)
- src/game/spells.cpp (spell_roll_amount D19503)

## --check (VC6_STRICT=1 vc6-sweep, --only "game/pathfinding|game/spells\.cpp|unnamed_d3d_tcache")

`4 units: 72 match, 0 wrong target, 28 differ, 0 not found, 0 errors`
`check against src/decomp/vc6-matches.txt: 70 matched there, 72 now, 0 lost`

## Worker 3

### Worker W3 notes (targeted round m-1021c, decisions D19600-D19699)

Batch: 9 functions. Result: 2 new matches, 3 closer (kept), 4 unchanged.

## New matches

- vsdk/unnamed_lights_0x51e070.cpp 0x51e3f0 light_create_spot, **D19600**. Distance 2 -> match.
- vsdk/unnamed_lights_0x51e070.cpp 0x51ec70 lights_shade_point, **D19601**. Distance 44 -> match.

**D19600 (vsdk/unnamed_lights_0x51e070.cpp). light_create_spot: the clamped intensity goes through a one-float
array.** `float in[1]; in[0] = float_min(float_max(intensity, 0.0f), 1.0f); intensity = in[0];` moves the clamp's
`fstp` past the type bit-field store and the pos argument push, where Sum.exe has it. This is the D17401 device
(a small float array). Forms that compile the same as before: the type store first (worse), a `float *pi =
&intensity` store (with either read), a `const vector3 *p = pos` copy, a `light *lt = l` copy, a local k for
float_max.

**D19601 (vsdk/unnamed_lights_0x51e070.cpp). lights_shade_point: three changes in the spot case.**
1. The spot case declares its own `float dot;` inside its block. The binary gives the point case's dot and the
   spot case's dot different stack slots, so they are two variables (44 -> 14). Declaration-order permutations of
   the function-scope floats (17 orders) change nothing. Only the block-scope dot matters.
2. `atten = float_min(atten, f); r = atten * l->r; ...` keeps the minimum in atten, not f. That swaps the slots of
   atten and f into the binary's pair: atten shares b's slot, f shares dot's (14 -> 2). Forms that do not do it:
   f and/or atten declared in the spot block or in the inner block, atten as the outer dot or f, a ternary atten,
   pointer stores into atten / f, an `af[2]` array (worse), dot used as f (worse).
3. `float au[1]; au[0] = (float)acos(down.dot(h)); float ang_u = au[0];`: through the one-float array, ang_u's
   fstp comes after the next dot product's two leas, as in Sum.exe (2 -> match). Forms with no effect: a split
   declaration, a pointer store, `ang_u, ang_v` declared first in either order, a local for the dot product.
   An array for both angles is worse (69).
The behaviour is the same in all three.

## Closer, kept (behaviour identical)

- vsdk/unnamed_linefile_0x558340.cpp 0x558430 LineFile::detect_eol: 48 -> 14 (**D19602**).
- vsdk/unnamed_linefile_0x558340.cpp 0x558600 LineFile::read_line: 60 -> 44 (**D19603**).
- game/new_hud.cpp 0x40f880 hud_portrait_get_color: 80 -> 78 (**D19604**).

**D19602 (vsdk/unnamed_linefile_0x558340.cpp). LineFile::detect_eol returns fill_buffer's own result, and the CRLF
test is kept in a bool.** In the binary the fill_buffer failure jumps to the shared `pop esi; ret` with EAX still
holding fill_buffer's value, so the source returned that value (`int err = fill_buffer(); if (err != 0) return
err;`). That gets the late `push edi` and the epilogue (48 -> 36, with the other forms 32). fill_buffer always
returns 0, so the behaviour is the same. Then `crlf = cr != NULL && cr - buffer < count - 1 && cr[1] == '\n';
if (!crlf) { lf / cr chain } else { next = cr + 1; if (cr > next || (lf != NULL && (lf > next || lf < cr))) ANY;
else CLEAR; }` puts the CRLF block last, as in the binary, and keeps the binary's `cr > cr + 1` test (36 -> 14).
What still differs: 8966 cross-jumps the "lf and cr" ANY store into the final ANY tail. The binary keeps it as its
own block (its `pop edi` is scheduled differently) and merges the CRLF ANY with the "neither" ANY instead.
Tried, no further change: an int flag, a flag set by an if, `return 0` after any subset of the six stores (63
combinations: all 20), the if / else polarity of both inner tests (`cr == NULL` first, `!(...)`, the De Morgan
form), `lf && cr` / `lf && !cr` chains (14, 38), a switch on (lf, cr) (34), `goto crlf` (36), next computed before
the test (33), success-nested `if (err == 0)` bodies (54-78).

**D19603 (vsdk/unnamed_linefile_0x558340.cpp). LineFile::read_line: the CRLF search length through an int local,
`k = n; k -= eol - cur;`.** The CRLF search's own arithmetic still differs (the binary computes `n - eol + cur`,
8966 computes `cur - eol + n`). The register choice of the three other cases' memchr arguments
(edx / eax / ecx) now matches (60 -> 44). Still different: the prologue (`mov esi, [size]; ... dec esi` in the
binary, `lea esi, [ecx-1]` in ours), the loop rotation (the binary jumps back to the `left != 0` test, 8966
duplicates it at the bottom), and where `this` is reloaded after the switch.
Tried, no change: `size--` / `size -= 1` as left (worse, 79-81), `left = size; left--` / `--left`, eol / left
init order, `for (;;) { if (left == 0) break; ...}`, `do { if (!left) break; ... } while (eol == NULL)`, `if
(left) do ... while (eol == NULL && left)`, a `goto top` loop, `while (left && eol == NULL)` (worse), the compound
`while (left != 0 && ((n = count) != 0 || !(flags & EOF)))` and a for with it, `continue` / `break` endings.
The length spelled as `cur - eol + n`, `n + cur - eol`, `(int)(cur - eol) + n`, `n - (int)eol + (int)cur`,
`(unsigned)n - ...`, `(cur + n) - eol`, `k = n - (int)eol; ... k + (int)cur`, `int k = eol - cur; n - k` (worse).
8966 reassociates all of them to the same order.

**D19604 (game/new_hud.cpp). hud_portrait_get_color: the hit-flash clamp as `if (v > 0) { if (v >= 255) v = 255;
} else v = 0;`.** This gives the binary's outer `jbe` (80 -> 78). The rest of the function lines up instruction
for instruction. Only the clamp tail differs: the binary goes through CL (`xor cl, cl; mov al, cl` for 0, a dead
`mov cl, al` before the `jae`), and its 255 block is placed last. Tried in this round (all 78, or worse):
a second ubyte g taking the result in 9 if / else / ternary shapes (`g = v` before or inside, `v < 255` and
`v >= 255` polarities, the else-if chain); v-only ternaries; `ubyte *pv` / `ubyte *pg` stores; a do / while (0)
clamp with breaks (86); `int g` (130); the clamp into c.g with `c.b = c.g` (126). Also, r / g / b at function
scope for cases 3 and 4 (worse, 133-135), and case 4 declared `b, g, r` (78, not kept; the case 3 / 4 slots
already match once the clamp is aligned).

## Still differs (tried; do not repeat)

**D19605 (notes only). Tried without a match:**
- gr_env_map_uv 0x51f960 (distance 2). The `lea` of the `2.0f * *normal` result temporary is still scheduled before
  the dot product's `fstp [esp]`. 16 more forms on top of D17501's, all the same or worse:
  - the dot product through a float local or a `d[1]` array (worse: the dot is stored and reloaded);
  - `float two[1]`;
  - `const vector3 *n = normal` used for the product or for the dot;
  - `const vector3 *pe = &e` used in either place;
  - `vector3 *pr = &r; *pr = ...`;
  - `const vector3 e`;
  - `vector3 r, e = ...`;
  - `normal[0]`;
  - `const vector3 &n`;
  - __forceinline helpers for the whole reflection (by reference or returned) or for the dot. These shrink the
    frame (0x18 / 0x30), so they are worse.
- lights_calc_point_specular_color 0x51f560 (distance 4). The same lea, twice. Forms tried:
  - a pointer copy of normal or of dn (same);
  - an `s` through a one-float array (same);
  - the reflection in a vector3 local, `eye.dot(...)`, or the dot through a float array (all worse, 133-221).
- options_init 0x41f0f0 (distance 10). 8966 forwards Summoner_options' first dword past the rep movsd. 9 more
  forms, all 10:
  - a destination pointer `SummonerOptions *e` (copy and/or read through it);
  - `*(float *)&options_edit` and `((float *)&options_edit)[0]`;
  - an `int *mm` store of mouse_mode;
  - a `const float *pv` read;
  - memcpy with a destination pointer.
  - A `float v[1]` copy of the volume is worse (119).
- gr_classify_sphere_view 0x5594f0 (distance 63). The binary keeps nz (and xs, ys) on the FPU stack across the
  early returns. It also cross-jumps the -1 returns at four pop depths, the shared block placed after the d[0] test.
  - `goto reject` to one shared `return -1` gives the merged tail at the end of the function (45, not kept). The
    block is in the wrong place, and nz is still spilled.
  - A result variable is worse (66).
  - Written-out products with no nz / xs / ys locals (z local or p->z, early or nested returns) are worse (107-135).
  - These do not change it: an nz local with xs / ys written out, `d[0] = (nz = ...) - (xs = ...)` (67),
    `d[2] = (ys = ...) + nz`, `float nz[1]`, `float xy[2]`, d[1] stored and reloaded through xs.

## Behaviour errors / bug candidates

None found. In detect_eol, the binary's `cr > cr + 1` test (kept through `next`) can never be true. It is an
odd source form, not a behaviour difference.

## Header changes

None.

## Changed files (under src)

- vsdk/unnamed_lights_0x51e070.cpp (light_create_spot D19600, lights_shade_point D19601)
- vsdk/unnamed_linefile_0x558340.cpp (detect_eol D19602, read_line D19603)
- game/new_hud.cpp (hud_portrait_get_color D19604)

## --check results (VC6_STRICT=1 vc6-sweep --check src/decomp/vc6-matches.txt)

- unnamed_lights_0x51e070: 37 matched there, 39 now, 0 lost
- unnamed_linefile_0x558340: 12 matched there, 12 now, 0 lost
- new_hud: 56 matched there, 56 now, 0 lost

## Worker 4

### m-1021c worker 4 (D19700-D19799)

## New matches

| file | address | name | decision | what did it |
|---|---|---|---|---|
| game/unnamed_cutscene_camera_0x493e50.cpp | 0x494400 | camera_blend_sequence::advance | D19700 | the target angles' addresses through pointer locals |

**D19700 (game/unnamed_cutscene_camera_0x493e50.cpp). camera_blend_sequence::advance 0x494400 is exact** (distance
64 -> match). The code was otherwise identical. The six angle scalars had the binary's slots except that each pair
was swapped: ours put the target angle (p1 / b1 / h1) below the camera angle, the binary has h0, h1, b0, b1, p0, p1
going up. The swap also reversed the operand order of the blend products (`h1 * w` first). Earlier rounds found that
declaration order and names do not move them, and that arrays (`float p[2]...`) fix the order but shrink the frame
(D16603). What fixed it: `float *tp = &p1, *tb = &b1, *th = &h1;` declared before the two extract_angles calls,
and `target_orient.extract_angles(tp, tb, th)`. The pointers are folded away. Taking those addresses through
locals changes how 8966 orders the slots, and each camera angle then sits below its target angle. The behaviour
is the same.
Device: **a pointer local for a scalar's address (passed to a call in place of `&x`) moves that scalar in 8966's
slot order** (related to D17202 / D18205, which order operands).
Tried on the way (no match): an inline unwrap helper by reference or by pointer (64), pointer locals for the camera
angles instead of the target's (46), a pointer local in each `else` body (64), pair arrays (80), one `float ang[6 /
7 / 8]` array (132).

## Closer, kept

None.

## Still differs (tried; do not repeat)

**D19701 (notes only). vector3::random_unit_vector 0x524650 / random_direction 0x5246a0 / random_direction_in_cone
0x5246f0 (distance 5).** This is still the `fmul st, st(1)` with z kept on the stack, where the binary has
`fmulp`. The 17 new forms (on top of D15905 / D17301) all give the same code except where noted:
- `const float *pz = &z` read twice.
- `z * *pz`, which gives `fmul [esi+8]` with no copy (worse).
- `float *rp = &r; *rp = ...`, `float *pz = &z` for both the frand_range store and the read, and `const vector3 *v = this`.
- `float s = z * z` and `zz = z * z; zz = 1 - zz; r = sqrt(zz)`.
- `float zl = z` after a, a `float ta = a` copy, a `float c = cos(a)` local, and the r statement in a `do { } while (0)`.
- `(*this).z`, and `(&x)[2]`.
- An inline helper for the sqrt by value or by const ref. 8966 did not inline either of them at the unit's flags,
  so both are worse.

From the binary, `fld st(0); fmulp` comes from a value that 8966 does not keep as a CSE. In quaternion::to_matrix
(`float yy2 = y * y * 2.0f`) the same pair appears, but the square is the leftmost operand there. Here it is the
right operand of `1 - z*z`, and none of the forms above changed it.

**D19702 (notes only). vector3_angle_and_axis 0x5249e0 (distance 2).** In the binary the acos fstp comes after
`push ecx; push edi`. The 14 new forms all stay at distance 2:
- a `float *pa = &angle` store (in place, and declared at the top with `*pa` returns);
- `const vector3 *bb = b` (at the cross only, and for the normalize and the cross);
- `const vector3 *pn = &na` (before the acos, and at the top), and `vector3 *pn` used for the na assignment as well;
- `const vector3 *pc = &nc`;
- a bool local for the sign test;
- `angle = -angle; return angle`;
- `float d = nb.dot(nc)`;
- the acos statement in a `do { } while (0)`.

This is scheduling.

**D19703 (notes only). xp_level_up_gains 0x489ac0 (distance 2).** In the binary EDI (species) is loaded before
`mov al, [esi+5]`. New forms:
- The hp_random byte read into an int (71) or a ubyte (32) before the cap store. Both are worse.
- `xp_level *l2 = lv` for the cap: 2.
- `int *cp = cap` set before the points store: 2.
- `(unsigned)species` as the index: 2.

This is scheduling. Stop.

**D19704 (notes only). ai_state::tentacle_beast_boss_load 0x4c6d10 (distance 2).** The last find_move's
`mov ecx, [esi+0x4c]` still comes before `push 0x87`. New forms:
- At distance 2: a `move_info *m` local, a `move_info **pm` store, an `int mv = 0x87` argument, a do/while slot
  loop, `Character *o` for the last call only, `this->owner`, `(*owner)`, and a file-local inline helper for the
  last call.
- Worse: the store after the loop (16), the slots through `t` (22), and an inline helper for all four calls (35).

Stop.

**D19705 (notes only). summoner_cfg_load 0x43e250 (distance 43).** All that is left is the register rotation from
`loaded:` on (ours ECX / EDX / EAX / ECX, the binary EAX / ECX / EDX / EAX; the byte `and al` follows the register
that holds EAX). New forms:
- At distance 43: a `SummonerOptions *o` for the volumes, a block around the sfx call, `*&` on the sfx argument,
  `!(mb > 8)`, a pointer store for texture_quality, n and size at function scope (both, or size alone, or n first),
  and size unsigned.
- Worse: float locals for the volumes (66 and 83).
- They do not compile: a video-memory int or bool local (the goto skips its initialisation).

**D19706 (notes only). cutscene_parse_table 0x4355e0 (/Od, distance 80).** The /Od round-robin register runs one
step behind the binary from `ent->num_fogs++` (line 537 of the listing) to the end. All the code up to it is
identical, and the fog loop's register sequence matches up to `mov ecx, fog; mov [ecx+8], ax`. In the binary one
register (EDX) is used up there with no instruction to show for it.

New forms:
- At distance 80: `(short)`, `(int)`, `(ushort)`, `(short)(ushort)` and `(short)(long)` casts of ParseInt, a
  `*(ushort *)&fog->shot` store, `(*fog).shot`, `(*ent).num_fogs++`, and a comma expression.
- Worse: `num_fogs = num_fogs + 1` (110), a `short *ps` store (102), and `ent->fogs[ent->num_fogs]->shot` (84).

A statement that uses up a register without emitting any code is still to be found.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/game/unnamed_cutscene_camera_0x493e50.cpp (camera_blend_sequence::advance, D19700)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "unnamed_cutscene_camera_0x493e50" --check
src/decomp/vc6-matches.txt`: 1 unit, 11 match, 1 differ; 10 matched there, 11 now, **0 lost**.
(The other units in the batch are unchanged; every variant was applied by script and the file restored. A diff
against the repository's copies shows no change in them.)

## Worker 5

### m-1021c worker 5 (D19800-D19899)

Batch: nw_ras_init, nw_buffer_pop, nw_NewCloseSocket (vsdk/unnamed_net_0x517590.cpp), quaternion::multiply,
quaternion::quaternion(const matrix *) (vsdk/unnamed_quaternion_0x5225b0.cpp), anim_track::calc_rot_tangents
(vsdk/unnamed_anim_track_0x550660.cpp), ai_state::goal_do_travel (game/unnamed_ai_think_0x4b5140.cpp),
projector_record::to_lobj_space (game/unnamed_projectors_0x4c0250.cpp).

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| vsdk/unnamed_quaternion_0x5225b0.cpp | 0x522790 | quaternion::quaternion(const matrix *) | D19800 | the second largest-diagonal test written in both arms |

**D19800 (vsdk/unnamed_quaternion_0x5225b0.cpp). quaternion::quaternion(const matrix *) 0x522790 is exact** (was
distance 69, never attempted before) with
`i = 0; if (m->uvec.y > m->rvec.x) { i = 1; if (m->fvec.z > m->uvec.y) i = 2; } else if (m->fvec.z > m->uvec.y) i = 2;`
in place of the two sequential ifs. The binary repeats the `fvec.z > uvec.y` compare after `mov edx, 1` and in
the else path (then `jmp` to the shared `mov edx, 2`); the switch then works on EDX directly. Same behaviour.
Not it: a ternary `i = (a) ? 1 : 0;`, an if / else for i = 1 / 0, `i = a > b;`, if / else each with its own second
test and `i = 0` inside the else (block order differs).

## Closer, kept

| file | address | name | old -> new | D |
|---|---|---|---|---|
| game/unnamed_ai_think_0x4b5140.cpp | 0x4b62a0 | ai_state::goal_do_travel | 44 -> 6 | D19802 |

**D19802 (game/unnamed_ai_think_0x4b5140.cpp). ai_state::goal_do_travel 0x4b62a0: 44 -> 6** with the keep flag
written through a pointer to a block-scope bool:
`{ bool keep; bool *kp = &keep; *kp = (bool)(~(goal_flags >> 9) & 1); r = path_to(..., keep); }`. The flag now
lives in the dead goal_flags argument slot (`mov byte [esp+0x1c], dl`, reloaded as a dword for the push), as in the
binary. Left: 8966 reloads the flag before `owner->info` (the binary loads info first) and puts `push 0` before the
layer load. Same distance 6: the pointer declared at function top, `bool &kr = keep`, `*kp` as the argument, a
`Character *o` local, `!((goal_flags >> 9) & 1)`, an inline helper whose bool parameter goes through a pointer.
Worse: a size local first (54) / after (24), the flag stored before the type-7 test (35), `char_info *ci` before
(47) / after (8), int (24) / unsigned char (18) through a pointer, plain bool / block bool / array / int / uchar
without a pointer (44), an inline helper with a plain bool parameter (44).

## Still differs (tried; do not repeat)

**D19801 (notes only). projector_record::to_lobj_space 0x4c0740 (64; source unchanged).** Finding: the binary's
loop is `local_planes[i] = planes[i].transform(&lo->pos, &lo->orient);` (the header plane::transform, by value,
D16401) with `local_pos = (pos - lo->pos).rotated(lo->orient);` (an operator- temporary, so pt reuses rel's
slot and the frame is 0x44): that form is distance 6, and both the out-of-line plane copy (0x4c0a20) and the
vector3 copy-constructor call for operator='s returned copy appear exactly. The rest: 8966 sums the inlined
`normal.dot(pt)` as (y + z) + x where the binary sums (x + y) + z. **Not kept**: the summation order changes the
x87 rounding (D9804 is about exactly this sum). With a `vector3 rel(...)` local (tr1-tr3) the frame is 0x50 (74).
A file-local inline helper with any explicit (x + y) + z dot gives the right sum but 8966 then strength-reduces
the plane pointer to &normal.y (`lea esi, [ebx+24]`, binary +0x1c) and, without an inline dot call inside the
helper, expands the plane copy constructor (distance 22-24); tried: helper by reference / pointer, a `const
vector3 &n` alias (22), a `float sd` copy, lobj_dot(a, b) by reference / pointer in both argument orders, a
derived-class member (`class lobj_plane_src : public plane`) with lobj_dot, with explicit sums (42-49) or with a
derived vector3 dot_xyz (24), `pt.dot(normal)` (z first), pointer locals for pt / normal (17), copies of pt /
normal (130-144), `return plane(p)`, static non-inline helper, `#pragma inline_depth(0-8)` around the function
(no effect), `plane(lp)` / `(plane)lp` / a block-scope copy in the old form (copies folded away). A match needs
vector3::dot or plane::transform to sum x first: a header change (vector.h dot is shared by many matches; not
attempted).

**D19803 (notes only). nw_NewCloseSocket 0x5191c0 (53).** Register roles swapped: the binary keeps the socket
pointer in EBX (pushed first, loaded before the other pushes) and the i * 2 counter in EDI; 8966 puts the pointer
in EDI and the counter in EBX. Thirteen forms, none moved it: i declared first / between msg and the packet,
unsigned i (55), do / while loop, the packet fields set at the top (60), goto to the bad-id tail (151), the
rsequence store through a `ushort *` (53), old_pool declared at function top, the NULL / rsequence store orders
(55, 59), `nw_socket != 0`, the server-socket test before the packet fields (66). Register choice; stop.

**D19804 (notes only). nw_buffer_pop 0x517820 (8).** Six new-device forms on top of D17705: `int *sz = size`
store, `const net_addr *sa` source, `net_addr *f = from`, first_seq through an int pointer, slot->seq through a
pointer (all 8), `first_seq = seq + 1` (59). Scheduling; stop.

**D19805 (notes only). quaternion::multiply 0x522930 (2).** Five new-device forms on top of D17801: r.w through a
float pointer, a `const float *` to w1, the w result in a `float[1]` stored last, memcpy of v into r, a const
pointer to r for the returned copy. All 2. Scheduling; stop.

- nw_ras_init 0x5178c0 (0): the cross-jump target (D17704, D14206); no new device applies to block merging. Not
  retried.
- anim_track::calc_rot_tangents 0x5506e0 (2): one movsx scheduled before the tan_b.x store (D17x / prior rounds,
  4 + forms). Not retried.

## Behaviour errors / bug candidates

None new. (quaternion::quaternion(const matrix *)'s case-2 m.rvec.y read and the fvec.z vs uvec.y test stay as
the binary has them, already noted as a game bug.)

## Header changes

None.

## Changed files

- src/vsdk/unnamed_quaternion_0x5225b0.cpp (D19800; the 0x522790 comment now says exact)
- src/game/unnamed_ai_think_0x4b5140.cpp (D19802)

## --check results (VC6_STRICT=1 vc6-sweep --check src/decomp/vc6-matches.txt)

- unnamed_quaternion_0x5225b0: 28 matched there, 29 now, 0 lost
- unnamed_ai_think_0x4b5140: 25 matched there, 25 now, 0 lost

## Worker 6

### W6 notes (m-1021c, decisions D19900-D19999)

## New matches
None.

## Closer, kept
- vsdk/unnamed_vfx_emitter_0x55ff00.cpp 0x561ec0 fx_recb0::render: distance 56 -> 18 (D19900).
- game/summoner_cd.cpp 0x43e660 cd_find_drive: distance 41 -> 3 (D19901). The 3 left are the shipped no-CD
  patch bytes, as in cd_set_root (D17900): with the patch reapplied after the build this is a byte match.

**D19900 (vsdk/unnamed_vfx_emitter_0x55ff00.cpp; closer, kept). fx_recb0::render (0x561ec0), 56 -> 18.** The
particle ring walk written as `for (p = particles; p != NULL; p = p->next) { ... if (p->next == particles)
break; }` (it was `for (p = particles; p != NULL; ) { ... p = p->next; if (p == particles) break; }`). The behaviour
is the same: p->next is read again, and nothing changes it between the two reads. This gives the binary's register
choice (desc in EBX, p in EBP) and its loop tail (`mov ebp, [ebp+0x20]; cmp ebp, [esi+0x9c]`). Two differences
remain. (1) age and alpha have each other's stack slots: the binary has age at [esp+0x20] and alpha at [esp+0x1c].
(2) After gr_poly the binary reloads `this` (ESI) before mat (EDI); ours reloads mat first. Forms tried for the
slots, none moves them: all 120 declaration orders of age / life / size / fade / alpha; age, age + life, alpha,
and size + fade + alpha declared in the blocks that use them; `float alpha[1]`; a two-float array for alpha and age
(back to 56); `!(...)` forms of the age and alpha tests; `this->particles` in the break test; a `vt` declared
then assigned; `i` declared in the for; a `const vfx_material *m` copy. A `first = particles` local makes the
frame 4 bytes bigger (182 / 216). Before the loop change these forms left the registers as they were: a desc
copy (function scope, block scope, `*const`), p declared first / last / in the block / in the for, `while`, a
pointer to particles, a `self = this` copy, mat declared late or in the block, a `next` local, `register` on p
and desc, and a flat local. A do/while ring walk is worse (58).

**D19901 (game/summoner_cd.cpp; closer, kept). cd_find_drive (0x43e660), 41 -> 3.** `char path[512] = "";` is now
declared after the two drive tests (GetDriveTypeA, GetVolumeInformationA), not at the top of the loop body. The
binary clears path there (`mov al, [""]; mov [esp+0x20], al; rep stosd`), after the `jne` tests. The behaviour is
the same: path is not read before it is set. The SHIPPED_NOCD branch lost its own copy of the declaration, so both
builds use the one after `#endif`. That build still compiles (checked with `--flags "/O2 /GX /MT /DSHIPPED_NOCD"`).
The only difference left is the patched `mov dl, '.'` / nop where 8966 emits `add dl, 'A'`.

## Still differs (tried; do not repeat)

**D19902 (notes). seg2d_point_near_middle (0x4a6530), distance 4.** The perp dot product's x term: the binary
loads d.x, then multiplies by perp.x. 30 new forms, none changes it: a `const vector2 *` to perp, d, dir, mid
and pt (each alone, and to d in both dot products); a pointer store for perp and for d; `float dx = d.x`;
`float px = perp.x`; d and perp as float arrays (perp[2] is worse, 17); perp built by its constructor; perp
copied from a named vector2, also with memcpy; perp's fields stored separately (17). With `d = mid - *pt` the
y term flips instead. Then 4 term orders, `d.dot(perp)` / `perp.dot(d)`, perp built by its constructor and a
pointer to perp give no change, and a pointer to d is worse (8). Together with D17500's list this is exhausted:
8966 sorts the operands itself.

**D19903 (notes). fx_recb0::spawn (0x55ff30), distance 2.** The prev store after the fstp of the ceil
argument. 11 new forms: a store through `float *pv` and through `int *`, a `const float *lt = &last_t` for both
reads and for the ceil only, prev declared in the block and declared last (all 2), a ceil before the prev
copy (8), `ceil((double)last_t)` (2), `ceil(prev = last_t)` and a float local (102), a double local (239). This
unit is built /Ob0, so the inline-helper device does not apply. Exhausted.

**D19904 (notes). fx_recb0::get_fade (0x5626b0), distance 8.** The binary runs `sub esp, 8` before `fld age`.
7 new forms, none moves it: `float *pa = &age; *pa = age / life` and `*pa /= life`, `const float *pl = &life`,
`float t = age / life; age = t;`, a double local for the alpha conversion, `__int64 av[1]`, and `do { } while (0)`
around the division. Exhausted.

**D19905 (notes). cd_set_root (0x43e8e0), distance 3.** Not a source difference (D17900, the no-CD patch). Not
tried.

**D19906 (notes). ai_state::luminar_boss_load (0x4c8dc0), distance 45.** The binary loads swing.z once up front
and squares it from the stack after swing.x * swing.x from memory. 8966 squares z first. 19 new forms. A pointer
local to back or swing, or a copy of swing (plain, or by memcpy), makes the frame 4 bytes smaller and computes swing first
(31-40). An explicit sqrt in any term order, also through the existing x / z locals, does the same (31-35). A
`float bl = back.len()` local is worse (53). Pointer stores for the y zeroing, and `back.y = swing.y = 0`, give no
change or are worse (45 / 53). FPU register choice. Exhausted.

**D19907 (notes). dialogue_frame (0x409000), distance 66.** The binary loads `key` into EDI before the tail's
`mode != 0` test, so the mode == 0 path and the `count != 0` return share one exit, and stores mode = 1 before
that exit. Ours loads key in each path and schedules the mode = 1 store between the pops. 13 new forms. A
do/while(0) with breaks is worse (80) and an early `if (mode == 0) return key` is worse (73). These give no
change: goto to a shared label (two placements), `goto out` on `!dialogue_mode`, a pointer store for mode = 3,
an `int k = key` (also `register`), a `const int *kp = &key`, a mode local, an if/else that stores 1 for
count == 0, a `for (;;)` around the built test, and a bool local for built. Exhausted.

## Behaviour errors / bug candidates
None found.

## Header changes
None.

## Changed files
- src/vsdk/unnamed_vfx_emitter_0x55ff00.cpp (fx_recb0::render, D19900)
- src/game/summoner_cd.cpp (cd_find_drive, D19901)

## --check results
`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "unnamed_vfx_emitter_0x55ff00|summoner_cd" --check src/decomp/vc6-matches.txt`:
2 units, 10 match, 10 differ; 10 matched there, 10 now, 0 lost.

## Process note
Early in the session three side-by-side listings and two scratch files were written into the repository's
working folder by mistake (a `cd` in a helper script). git status showed them as untracked. They were moved
to W/stray right away, and git status in the repository is clean again. No tracked file was touched.

## Worker 7

### W7 notes (targeted round m-1021c, D20000-D20099)

## New matches

None.

## Closer, kept

| file | address | function | D | old -> new distance |
|---|---|---|---|---|
| game/unnamed_gamelist_0x4705b0.cpp | 0x470860 | mp_handle_actv_gm | D20000 | 67 -> 37 |

**D20000 (game/unnamed_gamelist_0x4705b0.cpp). mp_handle_actv_gm 0x470860: the six entry field stores in the
order level, num_players, max_players, difficulty, min_level, max_level.** I compiled all 720 orders of the
six independent stores (the old source order was max_players, level, num_players, max_level, difficulty,
min_level). Only this order gives 8966's schedule of the binary's loads and stores: num_players in DL, max_players
in AL, level in ECX, the max_players store first. The next best order is at distance 45. The behaviour is the
same, because these are stores to separate fields of values already read. Still different: the list unlink. The
binary keeps `&e->prev` in EAX (`lea eax, [ebp+4]`, 1 byte longer), and the used-list / count stores follow
from that. On top of D20000 I tried: a `mp_gamelist_entry **pp = &e->prev` unlink, the same for the append, one
block for both, `mp_gamelist_count++` before the append (43), and a copy `f = e` for list_remove. Except for
the count order, all gave 37.

## Still differs (tried; do not repeat)

**D20001 (notes only). bm_set_reduced 0x502240 (distance 4, on top of D18702).** I tried the level store moved
to the shared tail (`e->num_levels = n;` after the if / else), inline helpers for the store, for store plus
pixel count, and for the pixel count alone (int and ubyte parameter), `int *pn = &n` at the call, and a
`bitmap_entry *be = e` copy for width / height. I also compiled every order of the four statements in each
branch (24 x 24 = 576 combinations): the D18702 order is the only one at distance 4, and all the others are
6 or more. 8966 always schedules the `and eax, 0xff` and the height load above the level store.

**D20002 (notes only). fx_create_spell_effect_facing 0x48c8a0 (distance 4).** Tried with right.x
`fld 0; fld st(0); fsub dir.z` (binary `fld 0; fld dir.z; fsubr st, st(1)`): `const vector3 *d = &dir`
(d->cross), `const vector3 *u = &up`, `const vector3 &u = up`, `vector3 *dp = &dir`, `right` declared and then
assigned (75), and `up` declared at the top / before normalize_safe / before tp (59-69). No change.

**D20003 (notes only). level_add_navpoint 0x4d5720 (distance 4).** The z fadd operand order did not change with
any of these: `const vector3 *s = &ray.start`, `const float *pt = &hit.t` (34), `const vector3 *d = &ray.dir`,
both pointers, `vector3 *hp = &hit_pos; *hp = ...`, `lobj_ray *rp = &ray` for start, the layer call before
hit_pos (34), the pos / layer stores swapped (14), and `ray.start = src->pos; ray.start.y += 1` (86).

**D20004 (notes only). savegame::hot_button_set 0x4d16a0 (distance 6).** For the store SIB order I tried
`unsigned off` (declared first and declared last), a `ubyte *p = data + off; *p = ...` store per field, and
`ubyte *p = &data[off]`. All still give `[esi+eax]`.

**D20005 (notes only). conversation_execute_action 0x4d5990 (distance 63).** The whole distance comes from
kind 7's else call. 8966 merges it with kind 10's identical false call; the binary instead puts `push 0; jmp`
back into kind 7's own call, right after its `push 1`. Kind 9 / kind 11 are already as in the binary. I tried
16 forms:
- for kind 7: a ternary bool argument (`? true : false`, `? 1 : 0`, `!= 0`: setne, 430), a bool local (486 /
  493), the condition negated with the false call first (324), true call plus `break` then the false call,
  `!= 0` in the if, the count in an int local, `(bool)0`, a `conversation *db` pointer in the else (325), an
  inline `topic_set(a, v)` helper for both calls, a ternary expression statement with two calls, and a goto to
  the true call.
- for kind 10: early `break`, `(*conversation_db.speaker).name`, `(*a).name`, `(bool)0`, `0 != 0`, and a
  `conversation *db` pointer (325).
None of these stops the merge with kind 10. (A distance in the hundreds here only means the code size changed
and the jump table moved.)

**D20006 (notes only). shadow_create_blob_bitmap 0x489f10 (distance 65).** The binary keeps p in EBP and y in
the frame (it reloads y and squares it in place). 8966 does the reverse. I tried: `int yy = y * y` per row (67),
p declared before y, `*(p + x)`, `y = 32 - row` in the body (100), y declared in the do block and in the row
block, `y = 32` before the for, `y--` at the end of the row body, `register` p (in place and declared first), and
`ubyte *q = p; q[x]`. None moves the registers.

**D20007 (notes only). matrix43::transpose_in_place 0x511bb0 (distance 46).** A swap chain
(`t = a; a = b; b = t` x 8, float t), four-statement rotations
(`t = m11; m11 = m01; m01 = m10; m10 = t; t = m30; ...`, float and `*(int *)&` int), and an inline
`fswap(float &, float &)` helper all give no stack temporary. This unit is /Ob0, so the helper is called and
not expanded (80). Analysis: per cycle the binary stores t twice (old m11, then old m30) and uses each value
once (m12 = old m11, m10 = old m30). It loads old m30 before m30 = m12 is stored, while the first value is
still live. One variable t cannot do that in source order, so the slot is probably a dead spill or a second
temporary that shares t's slot. No source form found.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/game/unnamed_gamelist_0x4705b0.cpp (mp_handle_actv_gm: the store order, D20000 in the function comment)

## --check

- unnamed_gamelist_0x4705b0: `1 units: 13 match, 0 wrong target, 1 differ, 0 not found, 0 errors`;
  `check against src/decomp/vc6-matches.txt: 13 matched there, 13 now, 0 lost`.
- No other unit was edited: level_scripts.cpp, unnamed_fx_0x489c40.cpp, bmpman.cpp and matrix.cpp are the same
  as in REPO.

## Worker 8

### m-1021c worker 8 (D20100-D20199)

## New matches

None.

## Closer, kept

| file | address | name | D | old -> new distance |
|---|---|---|---|---|
| game/combat.cpp | 0x464d00 | combat_position_modifier | D20100 | 6 -> 4 |
| game/combat.cpp | 0x468760 | living_combat::blood_splot_place | D20101 | 58 -> 4 |

**D20100 (game/combat.cpp). combat_position_modifier 0x464d00, closer (6 -> 4).** The facing dot is written with one
float local per product (`float fz = d.z * fvec.z; float fy = ...; float fx = ...; facing = fz + fy + fx;`). 8966
then schedules the `lea ecx, [esi+0x324]` for ai.is_mode after the third product, as the binary. The instruction
stream is now the binary's (64/64, no extra stores, so the same x87 values); what is left is facing's slot: 8966 puts
it in defender's dead argument slot (`_facing$ = 12`), the binary in attacker's ([esp+0x14] at the end). Tried for
the slot, no change: facing declared at use, first, between m and d, in a block with the products, the product
locals declared at the top (before / after facing), a Character copy of attacker for all its reads, a Character copy
of defender, a `const vector3 *` to fvec, `facing = fz + fy; facing += fx;`, a `float *` store, reusing fz as
facing, `float facing[1]`, m declared after d, one if / else exit. Worse: a bool for `facing >= 0` (80), a vector2
for facing (68), a `float f[3]` for the products (56).

**D20101 (game/combat.cpp). blood_splot_place 0x468760, closer (58 -> 4).** In the small and medium cases `v = v *
radius;` becomes `{ const float *pr = &radius; v = v * *pr; }` (the pointer-local device). 8966 then loads the
radius once (`fld [esp]`, then `fld v.c; fmul st, st(1)` per component, `fstp st(0)` at the end) as the original,
which closes D9301 (b). Same value (the stored radius, as before). The block is needed (a declaration in a case
otherwise is skipped by the next case label); one `pr` at function scope gives the old code (58). A `float r =
radius` copy, a `(const float &)` cast, a `vector3 *` to v and `v.operator*(radius)` give the old code; a vector3
copy of v is worse (132). Left: D9301 (a), `fld st(0); fsub u.z` against the binary's `fld u.z; fsubr st, st(1)` for
the cross product's `0 - u.z`. Tried for it this round: `const vector3 *` / `vector3 *` to u with `->cross` and
`(*up).cross`, a `const vector3 &` to u (all the same); a vector3 copy of u is worse (106).

## Still differs (tried; do not repeat)

**D20102 (notes only). Tried without a match this round** (add to the earlier rounds' lists):
- vsdk/file_packfile.cpp 0x533f90 packfile_read_directory (distance 4, p in sector's dead slot, the binary pf's):
  i in the if block, size in the loop body, name in the if block, all locals in the if block, `ubyte **pp = &p;
  *pp += 4`, an `int *ix = index` local, a Packfile copy for the first store, p from `*(&sector)` through a pointer,
  `register` p, p declared first at function scope, an `int h` for the hash, a `Packfile **` local (all 4); p set
  before the `if` (12), the size read through an `unsigned int *` walking pointer (23).
- game/unnamed_mp_objects_0x47f860.cpp 0x480f60 mp_handle_ai_disable (distance 4, from in ECX, binary EAX): `void
  **pf = &from` for the call, a bool host local, the host test with an `mp_player *lp` local, a `ubyte *` to the
  kind byte, an int kind local (all 4). Register choice; stop.
- 0x480d70 mp_handle_status_add (distance 10): value read through a `float *pv`, a Character copy at the call, a
  bool local for `false`, all reads through an `int *d`, reads through a packet struct pointer, value declared after
  the c test, an `if (c) { }` block, a `float &` to value (all 10); value read last (14), after the conversion (27),
  a float copy of value (16).
- 0x47fdf0 mp_send_new_lep (distance 80, the counter in ESI and the store schedule): the counter bumped first / mid /
  last / through an `int *` (top and after info), the counter as `int n = sent + 1` stored later, the header first,
  info after c->type, a `char_info *` for info, an int type local (80 to 84). Scheduling of 8966; stop.
- game/level_script_ikaemosu.cpp 0x4f9ec0 ls_ikaemostopint_init (distance 47, the constant 1 in EBX): a `register
  bool t = true` for the four uses, a `bool *` store for one of the stores, an int local for the first flag test
  (all 47).
- vsdk/skeleton.cpp 0x544090 skeleton::read (distance 68 = the rotation-key loop's [edi+ecx] against [ecx+edi], 25
  instructions, plus 17 instructions of EH funclets after the function): an unsigned k with `(int)k <` test, k in
  the for, `k[t->rot_keys]`, `(t->rot_keys + k)->`, `(k + t->rot_keys)->`, a byte-offset cast address (all 68); an
  `anim_rot_key *const *` to t->rot_keys (439), an anim_track copy of t (435), the epsilon tests in an inline helper
  (673), one epsilon test through a float local (68).
- game/combat.cpp combat_position_modifier and blood_splot_place: see D20100 / D20101.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/game/combat.cpp (combat_position_modifier, blood_splot_place and their comments)

## --check result

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "game/combat\.cpp" --check src/decomp/vc6-matches.txt`:
1 unit, 52 match, 11 differ; 52 matched there, 52 now, 0 lost.
No other unit was edited (packfile, mp_objects, ikaemosu and skeleton are byte-identical to REPO's copies).

## Worker 9

### m-1021c worker 9 notes (D20200-D20299)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| vsdk/gr_glide.cpp | 0x549b50 | glide_record::alloc | D20200 | stores of addr, texture and section through pointer locals |
| game/unnamed_avoidance_0x4a6710.cpp | 0x4a7cb0 | avoid_area_rebuild | D20201 | polygon search as `for (;;)` with `if (++p >= N) return;` at the bottom |

**D20200 (vsdk/gr_glide.cpp; m-1021c worker 9). glide_record::alloc 0x549b50 is exact.** The tail is written
`b->next = next; FxU32 *pa = &b->addr; *pa = addr;` in the else branch, then `b->size = size; int *pt =
&b->texture; *pt = texture; int *ps = &b->section; *ps = section; return b->addr;`. The pointer store to addr
alone gives the binary's next-then-addr order (8966 otherwise stores addr first); the texture and section
pointer stores together give texture before section (one of them alone does not). Same stores, same values.
Device (D18100's, extended): **a store through a pointer local to a struct field keeps the source order of
stores that 8966 otherwise schedules differently.**

**D20201 (game/unnamed_avoidance_0x4a6710.cpp; m-1021c worker 9). avoid_area_rebuild 0x4a7cb0 is exact.** The
search loop is `p = 0; for (;;) { poly = &avoid_polys[p]; if (used && size && layer match) { for k ... goto
found; } if (++p >= AVOID_NUM_POLYS) return; }`. The binary's loop ends with `jge` to the single shared
epilogue and an unconditional `jmp` back to the top; the plain `for` with `return` after it gave an inline
second epilogue and a `jl` back. A `do { } while (++p < N); goto done;` form and a `goto next` form also match;
`for (...) ...; goto done;` does not. AVOID_NUM_POLYS > 0, so the first pass always runs: same behaviour.
Device: **a loop whose exit test is an `if (...) return;` at the bottom of a `for (;;)` gives `jcc exit; jmp
top` with the return sharing the final epilogue.**

## Closer, kept

- vsdk/gr_opengl.cpp 0x5502c0 gr_opengl_read_backbuffer: 72 -> 56 (D20202). Two row pointers (`top_row = buf`,
  `bottom_row = buf + (h - 1) * 4 * w`), each swap reading the bottom byte first (`t = bottom[k]; bottom[k] =
  top[k]; top[k] = t;`), and the bottom row stepped before the top row. Same bytes swapped, same result.
- vsdk/unnamed_sndstream_0x566560.cpp 0x566630 snd_stream_fill_acm: 56 -> 50 (D20203). `UINT got = 0;`
  declared before p1 / p2 / n1 / n2: the binary stores got's zero first (the frame slots were already the
  same; only the store order differed).

**D20202 (notes; closer).** gr_opengl_read_backbuffer: the binary keeps both row pointers biased by +2 (they
point at byte 2) in the h / buf argument slots, -4w in EBP and the h/2 counter in w's slot. Forms tried
(about 30): the bottom-first swap alone in the index form (60, with `i*w*4`, `i*4*w`, `4*i*w`, `w*4*i`,
`(h - i - 1)`, `&buf[..]`, `last - i*4*w`: all 60), a `stride` local (75 / 87), index addressing `top[j*4+k]`
(87), row pointers biased by +2 with [-2..1] indexing (69 / 61 / 67), a `-4 * w` int local for the step (67 /
70), a do / while with a count (83), the top row stepped first (64). 56 is the best.

**D20203 (notes; closer).** snd_stream_fill_acm: left is the block placement of the one `rc = -1` (Sum.exe puts
it just before done, the success path jumping over it; 8966 puts it after the acm_convert test). Tried: the
acm_convert result in an int local, a bool local, `rc = acm_convert(..)` (63), and the whole body in a
`do { ... break; } while (0)` with `rc = -1` after it (50, same). Not kept beyond got-first.

## Still differs (tried; do not repeat)

- vsdk/unnamed_sndstream_0x566560.cpp 0x5670a0 wav_open (4): nBlockAlign's imul operand order. Tried (all
  identical, 4): `const WORD *` pointer locals to wBitsPerSample and / or nChannels in both orders, an int local
  split (`ba = bits; ba *= ch;` both orders), a product local, WORD / unsigned / int locals of the two fields in
  both declaration orders, a `WAVEFORMATEX *` copy (30), an `int bc[2]` array (18). Exhausted.
- vsdk/effect_mem.cpp 0x51bfd0 rfx_preload (6): `lea edi, [ecx + edx]`: in the binary ECX holds offset (the
  index argument slot) and EDX buf_size; 8966 swaps them. Tried (all 6): `const unsigned int *` locals to offset
  / buf_size / both in both operand orders, a pointer to total, an int local for offset, `buf_size - -offset`,
  `b->data = NULL` before the sum (both orders). Exhausted.
- vsdk/unnamed_sndstream_0x566560.cpp 0x566970 snd_stream_fill_silence (51): the frame. Binary: p1 -4, p2 -8,
  n1 -12, n2 -16 (declaration order), fill in stream's dead slot, extra in bytes' slot. 8966: p1 -8, n1 -12,
  p2 -16, fill -4, n2 in stream's slot. Tried: fill declared first, reversed declarations, `int fill` (96),
  fill assigned after its declaration, `void *p[2]` / `DWORD n[2]` arrays (51, same slots), a `ubyte *` to fill
  for the 16-bit store, extra in an inner block, everything after s in a block, fill outside and the rest in a
  block (59), p / n outside and fill in a block, `memset` before `extra = 0` (104). None moves n2 out of stream's
  slot.
- vsdk/effect_mem.cpp 0x51dd00 vfx_load (43): the copy of the base's flag bits 1 and 2. The binary reads flags
  as a word, masks each bit separately, sign-extends each from 13 bits (`shl cx, 3; sar cx, 3`), then `and edx,
  -4` / `and al, 0xfa` before the OR and `<< 6`: a signed 13-bit bit-field read twice (vfx_base::clear clears
  "the low 13 flag bits"). A file-local `struct { short low : 13; short high : 3; }` cast over base->flags
  gives the shl / sar but 8966 merges the two masks into one `and 6` (50): ternaries, plain masks, a pointer
  local, int / short locals per term, explicit `& ~3` / `& ~5`, a two-statement `|=` form all give 50; the
  unsigned field gives 43. Not kept. Probably wants a bit-field declaration of vfx_base::flags (and maybe of
  id_bits) in the header, which I did not change.

## Behaviour errors / bug candidates

None found. All kept changes keep the stores, values and loop counts.

## Header changes

None.

## Changed files

- src/vsdk/gr_glide.cpp (glide_record::alloc, D20200)
- src/game/unnamed_avoidance_0x4a6710.cpp (avoid_area_rebuild, D20201)
- src/vsdk/gr_opengl.cpp (gr_opengl_read_backbuffer, D20202)
- src/vsdk/unnamed_sndstream_0x566560.cpp (snd_stream_fill_acm, D20203)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root W/src --only
"gr_glide|unnamed_avoidance_0x4a6710|gr_opengl|unnamed_sndstream_0x566560|effect_mem" --check
src/decomp/vc6-matches.txt`: 5 units: 220 match, 27 differ, 0 errors; 218 matched there, 220 now, **0 lost**.

## For the coordinator

snd_stream_fill_pcm 0x566820 (not in my batch, distance 134) has the same declarations as fill_acm; D20203's
got-first order may help it too (not tried).

## Worker 10

### W10 notes (m-1021c, decisions D20300-D20399)

Batch: gr_line, gr_bitmap_float, gr_tmapper_multipass (vsdk/gr.cpp); ix_point_in_polygon, ix_ray_quad
(vsdk/unnamed_ix_0x53ac20.cpp); ui_gadget::set_parent (vsdk/unnamed_gadget_0x52bfe0.cpp); items_parse_table
(game/items.cpp); ui_listbox_entry::wrap_text (vsdk/unnamed_listbox_0x52ca60.cpp).

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| vsdk/unnamed_ix_0x53ac20.cpp | 0x53b6f0 | ix_point_in_polygon | D20300 | beta copied into a block local for the last test |
| vsdk/gr.cpp | 0x507a80 | gr_tmapper_multipass | D20302 | a `gr_multipass_vertex *mv = verts[i]` local in each per-vertex loop |

**D20300 (vsdk/unnamed_ix_0x53ac20.cpp). ix_point_in_polygon 0x53b6f0 is exact** with
`float b = beta; inside = alpha >= 0.0f && b + alpha <= 1.0f;`. With the copy, 8966 reloads beta
(`fld beta; fadd st, st(1)`) and keeps alpha on the FPU stack, popping it at both exits (`fstp st(0)`), as the
binary does. Before, it did `fadd beta` onto alpha. Same value. Forms that gave identical code (do not repeat):
`alpha + beta`, a `const float *` to beta or to alpha, a copy of alpha, the sum in an assigned local, a bool
local for `alpha >= 0`, an if form that returns true, nested ifs. This is the D18200 / D18202 device ("a copy of
a float into a local before its use").

**D20302 (vsdk/gr.cpp). gr_tmapper_multipass 0x507a80 is exact** when each of the three per-vertex loops (uv,
alpha, colour) reads through `gr_multipass_vertex *mv = verts[i];` and then `mv->pass[p].x`. 8966 then uses the
binary's +8-biased pass offset (`mov esi, 8`, `[esi + ecx - 4]` / `[esi + ecx]` / `[ecx + esi + 7]`) and does
not add a `verts[i] + 12p` common subexpression. Same behaviour. Did not work: a `gr_pass_vertex_data *pd =
&verts[i]->pass[p]` local (98), storing v before u (53).

## Closer, kept

- vsdk/unnamed_ix_0x53ac20.cpp 0x53b130 ix_ray_quad: distance 10 -> 4 (**D20301**). Copies `float vv = v;
  float uu = u;` and `*w = (vv + uu) * 0.5f;` (same value). With only the v copy the distance is 6. Left: the
  float_min / float_max constant pushes and the `mov esi, w` come before `fadd u` in the binary, and `mov al, 1`
  comes before the last `fstp [esi]`.

## Still differs (tried; do not repeat)

- **ix_ray_quad 0x53b130 (4, after D20301)**: on top of vv / uu: a `float *pw = w` first, a bool return local, a
  `const float *` to uu (6), a `float m[2]` (10), a third copy, the float_max result in a local (25). Before
  D20301: the assignment inside the float_max argument, a pw pointer, 0 / 1 constants as locals, a bool return
  local, `u + v`, a `const float *pu`, a comma return, a do / while (0), u / v declared in blocks (10), a
  `float uv[2]` (50), `u /=` split division (16), `*w = v; *w = (*w + u) * 0.5f` (10), a copy of *w after the mean
  (21). Scheduling; stop.
- **ui_gadget::set_parent 0x52c570 (4)**: old head in ECX, the binary's in EDX (else branch). On top of D17802's
  twelve forms: `ui_gadget **hp = &win->first_gadget` (worse, `add eax, 0x4c4`), a pointer to next_sibling, a
  `self = this` copy, window as `ui_element *`, an int copy of the head, a block-local second head with a `me`
  copy, `ns[0] =`, reusing the `parent` argument as the temporary (two forms), a comma chain, a `ui_window **` to
  window, a goto to a shared `next_sibling = first`, a ternary load and an if / else store, two separate ifs, the
  store after the if / else, a head pointer in the parent branch only, `if (!parent) {...; return;}` (14). All
  give the same code. Register choice; stop.
- **gr_line 0x507310 (8)**: the integer copy of clip_top / clip_bottom is scheduled one instruction later than in
  the binary (four places). Tried in the first block: a `float *py = &y1` (two forms), a `const float *` to
  clip_top, an int copy through `*(int *)`, a `float *px = &x1`, a copy of y1, a local for `clip_top - y1`, a
  clip_top local with that, a copy of x2 (10); the clip values as a `float clip[4]` in two orders (8 and 58); all
  four declared first and then assigned. Plus D18001's forms. Scheduling; stop.
- **gr_bitmap_float 0x507100 (12)**: the binary pushes gr_tmapper's arguments between corner 2's `fsub hs` and
  `fadd x`, and stores corner 2's sx after the constant stores. Tried: copies of x / y (`float tx = x`), a
  `const float *` to x, a `vl = vlist` copy, a mode copy, a `gr_vertex *v2 = &verts[2]`, corner 2's sx through a
  local stored after its constants (18), eight orders of the corner 2 / corner 3 statements (12 to 18). 8966
  orders them back; scheduling; stop.
- **items_parse_table 0x44a090 (50)**: almost all the distance comes from the jump tables after the code
  (build/sum.asm disassembles them as code). The only real code difference is register rotation in three of the
  four armour-texture case blocks (binary ecx / edx,eax / ecx,edx; ours edx / eax,ecx / edx,eax). Identical code
  from: a default case, a block-local hero copy, flat `(&...[0][0][0])[(heavy * 4 + hero) * 4 + k]` indexing, an
  `ItemType *tt = t` copy, a copy of part, hero / heavy / part / ok / tex declared at function scope (three
  splits), two other declaration orders. Plus D11104 / D9559's forms. Register choice; stop.
- **ui_listbox_entry::wrap_text 0x52ca60 (73)**: the binary keeps the value of split in EBP across the length
  store and the gr_get_string_size call, so cur_indent goes to a stack slot (frame 0x18 against our 0x14). 8966
  merges every copy of split back into the memory load: `int n = split` (block, function scope, `register`), a
  temporary whose address is passed, then copied to split, a ternary length store (69), a pointer to the length
  slot (69), strlen into a local. cur_indent through a pointer local, or as `int[1]` / `int[2]` / a shared
  `int[2]`: 8966 keeps it in a register anyway. Declarations moved: split / lw / lh at function scope (two
  orders), next at function scope, max_w in the loop, cur_indent at its first assignment, a `k = num_lines` index,
  a `tw` width local (90). Register choice; stop.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/vsdk/unnamed_ix_0x53ac20.cpp (ix_point_in_polygon D20300, ix_ray_quad D20301)
- src/vsdk/gr.cpp (gr_tmapper_multipass D20302)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "vsdk/gr\.cpp|vsdk/unnamed_ix_0x53ac20\.cpp"
--check src/decomp/vc6-matches.txt`:
`2 units: 88 match, 0 wrong target, 8 differ, 0 not found, 0 errors`;
`check against src/decomp/vc6-matches.txt: 86 matched there, 88 now, 0 lost`.

Helper scripts and variant files are in try/ (t.mjs applies one variant at a time, prints the distance and
restores the file).

## Worker 11

### Worker 11 (D20400-D20499): parse.cpp, boss_ai.cpp, http, new_multichat, interface

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/boss_ai.cpp | 0x4a13b0 | boss_threat_add | D20400 | threat[1] stored through a pointer local in both fill blocks |
| vsdk/parse.cpp | 0x516290 | TextFile::ParseInt | D20401 | the sign applied into a separate result local |
| game/new_multichat.cpp | 0x41e5d0 | chat_input_handle_key | D20402 | `if (key != 0) { ... } return key;`, Esc case first, Enter's non-empty test first |

**D20400 (game/boss_ai.cpp). boss_threat_add 0x4a13b0 matches** with `float *p1 = &t->threat[1]; *p1 = amount;`
in both fill blocks (the -1 slot and the stale slot). This is the D18100 device (a store through a pointer local).
8966 then puts `test edi, edi` after the threat[1] store and the eax copy after the threat[0] store, as the
binary does. A pointer for threat[3] alone, threat[0] alone, threat[3] plus threat[2], or the handle plus threat[3]
gets to distance 4. A pointer for both threat[1] and threat[3] also matches; the threat[1] one alone was kept.
Behaviour is the same.

**D20401 (vsdk/parse.cpp). TextFile::ParseInt 0x516290 matches** with `int result = value; if (neg) result =
-value; ... return result;` in place of negating value itself. The extra web moves `this` to EDI and value to
ESI, as in the binary. `int result = neg ? -value : value;` gives the old code (44), and so does moving the
negation after the Error test (52). Behaviour is the same.

**D20402 (game/new_multichat.cpp). chat_input_handle_key 0x41e5d0 matches** with three changes together:
1. The early `if (key == 0) return key;` becomes `if (key != 0) { ... }` around the body, with one final
   `return key;`.
2. The Esc case comes first in the switch.
3. Enter tests `strlen(chat_input_text) != 0` first (the send path returns 0) and then calls
   `chat_input_cancel(); return 0;`.

The cancel block then follows Enter's return as in the binary. The `if (key != 0)` form alone gives 28 (the
cancel block lands inline). With the inverted Enter test alone, or if / else forms, the result is 45. Behaviour
is the same: key 0 still returns 0, and the case order does not change the dispatch.

## Closer, kept

None.

## Still differs (tried; do not repeat)

**D20403 (notes only). http_async_gethostbyname 0x55b310 (distance 4).** The binary stores `l->ip` between
`push ebx` and `push offset http_resolve_host`. Ours stores it after the push. 15 new forms, none moves it:
- `unsigned int *pip = &l->ip; *pip = 0;`
- a second pointer local for the malloc result (`n`)
- a file-local inline fill helper (all five stores, or four with the ip store outside)
- `*(unsigned int *)l = 0`
- a block-scoped pointer copy for all stores
- a `void *arg` local for the thread argument
- a host local
- a zero local
- a pointer store for http_last_lookup.done
- `abort = error = done = false` chained
- a struct temporary copied in (worse)
- the ip store moved after the global stores, plain or through a pointer, or through http_async_lookup (worse)

Scheduling. Exhausted.

**D20404 (notes only). TextFile::ParseUInt 0x516380 (distance 12).** digits is in EBP, the binary's in EBX. All
of these give 12 or worse:
- a `TextFile *tf = this` copy
- `int *pd = &digits`
- `!digits`
- a dead `bool neg`
- `++digits`
- digits++ moved to the end of the hex loop body (12); in the decimal loop it gives 25
- `unsigned *pv = &value`
- a do/while for the decimal loop (69)
- a bool for the Error test
- `char c` at function scope (three declaration positions, with and without `register`)
- unsigned char c (71), signed char c, register c
- a result local in the ParseInt style (unsigned, int, function scope)
- an int copy of digits
- a hex string local
- `strchr(...)` without `!= NULL`
- `int d = *cursor - '0'`
- a shared `value * 16` local (49)

Register choice. Exhausted.

**D20405 (notes only). TextFile::ParseString 0x5166b0 (distance 50).** The binary pushes EBX in the prologue and
uses it for the incremented cursor (`mov ebx, [edi+4]; inc ebx; mov ecx, ebx; mov [edi+4], ebx`) and later for
dest. Because EBX is saved at entry, both size-check paths begin with `push esi` (len), and 8966 hoists that push
above the `cmp esi, eax`. Ours pushes EBX late, in the strncpy path only, so the push is not shared. Forms tried:
- a cursor pointer local: `cursor + 1`, `s++`, `++cursor`, assigned after `cursor++`, chained, at function scope,
  with the loop on the local or on cursor (54-64)
- one pointer variable reused for the cursor and then dest
- a dest local (block and function scope)
- a result local
- `size <= len`
- `int *pl = &len`
- len declared at the top (two positions)
- the terminator store before the cursor update
- a do/while scan (64)

Not found. 8966 copy-propagates every pointer local away.

**D20406 (notes only). ai_state::boss_ai_unload 0x4a04d0 (distance 64).** As D7603 says, every case tail-jumps
with ECX unchanged, and the dispatcher loads the info pointer into EDX. The per-boss unload functions must be
ai_state (thiscall) members. They are declared in include/engine/ai/boss_ai.h and defined in about 14 other
units, which are outside this batch. Not changed. No source form in boss_ai.cpp alone can keep ECX live for free
functions.

**D20407 (notes only). ui_message_box_show 0x413e40 (distance 75).** The binary stores the flags word, re-reads
its low byte (`mov cl, byte [flags]`), and tests it with a shared `mov al, 1` that the game_flags test also uses.
- A volatile byte read, `(*(volatile unsigned char *)&ui_message_box_flags & 1)`, with the callback store
  written before the flags store gives **39**. That fixes the `interface_cur_screen` switch registers too.
- With the source's store order the same read gives 72 (flags in EDX).
- The residual at 39 is the schedule: the binary has the flags store, the byte re-read, the callback store, the
  test, then the mouse_mode store. Ours sinks the re-read below all three stores.

Not kept, as in the earlier rounds: it is a volatile cast with no source meaning. The merger may choose to take
it, since it keeps behaviour. Other forms tried, at 39 or worse:
- `1 & byte`
- mouse_mode stored first, or callback / mouse_mode / flags
- `game_flags & 1`
- a pointer store for the callback (flags first, or callback first)
- non-volatile char casts (75)
- a char local, a char pointer local
- a bool local (96)
- a `one` local
- volatile stores for flags, callback and mouse_mode in combinations (72-74)
- a byte local read straight after the flags store (58-70)

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/game/boss_ai.cpp (boss_threat_add, D20400)
- src/vsdk/parse.cpp (TextFile::ParseInt, D20401)
- src/game/new_multichat.cpp (chat_input_handle_key, D20402)

## --check results (VC6_STRICT=1 vc6-sweep against src/decomp/vc6-matches.txt)

- boss_ai | vsdk/parse | new_multichat (3 units): 114 matched there, 117 now, **0 lost**
- new_multichat (re-run after the comment rewrap): 11 there, 12 now, 0 lost

## Worker 12

### W12 notes (m-1021c, decisions D20500-D20599)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/unnamed_worldmap_automap_0x42f1e0.cpp | 0x42f500 | worldmap_frame | D20500 | the distance stored through a float pointer local |
| game/unnamed_evil_joseph_0x4c3470.cpp | 0x4c3cd0 | evil_joseph_pick_helper_summon | D20502 | boss->pos through a pointer local, and `dx * dx` in a local |
| game/conversationctl.cpp | 0x4d8940 | conversation_controller::line_talker | D20503 | the line text through a local before strchr |

**D20500 (game/unnamed_worldmap_automap_0x42f1e0.cpp). worldmap_frame 0x42f500 is exact.** The marker scale's
argument is `float dist; float *pdist = &dist; *pdist = d.len(); loc->scale = worldmap_marker_scale_for_distance(dist);`.
With the store through the pointer, 8966 keeps the sqrt result on the FPU past the three component pops
(`fstp st(3)`, two pops) and only then narrows it through the stack temporary (fstp / mov ecx / push ecx), as
the binary does. vector3::len() is unchanged. Tried without a match: a plain `float dist = d.len()` (12), a copy
`float s = d.len(); float dist = s;` (6), `const vector3 *pd = &d; pd->len()` (6), `(float)sqrt(d.len_sq())` (73), a
float[1] array (12), dist declared before d (12), a double sqrt local (73). Same behaviour.

**D20502 (game/unnamed_evil_joseph_0x4c3470.cpp). evil_joseph_pick_helper_summon 0x4c3cd0 is exact.** Two changes
together: `const vector3 *bp = &boss->pos; evil_joseph_summon_pos = (*bp + t->pos) * 0.5f;` (the binary adds x as
boss + t but y and z as t + boss; the pointer gives that order), and `float dd = dx * dx; if ((float)sqrt(dd + dz * dz)
< 4.0f)` (the binary squares dx in its own FPU slot and keeps dz to the end). Alone: dd gives 8, bp gives 45; a
`const vector3 *tp` for t->pos instead is worse (12 with dd); bp and tp together also match (kept bp only). Also
tried: `dx = dx * dx + dz * dz` (53), `dx *= dx` (8), `dz * dz + dx * dx` (53), an inline 2D length (53), pointer
locals for dx or dz (53). Same behaviour (float addition commutes).

**D20503 (game/conversationctl.cpp). conversation_controller::line_talker 0x4d8940 is exact.** `const char *text =
line->text; if (!strchr(text, '"')) return NULL;`. The case blocks then use the binary's registers (EAX for
cur_character, ECX for speaker, EDX / ECX in the species loops). Tried: a `self = this` copy (78), `c = cur_character`
/ `c = speaker` before the calls (65), a kind local for the switch (78). Same behaviour.

## Closer, kept

**D20501 (game/unnamed_serpent_0x495290.cpp). ai_state::serpent_illusion_update 0x496d60, distance 68 -> 4.**
`if (rem > 0 && 300 >= rem)` instead of `rem > 0 && rem <= 300`: 8966 no longer threads away the binary's re-test
of rem <= 300 (after the earlier `rem > 300` return); `test eax, eax / jle / cmp eax, 300 / jg` as the binary.
(`rem >= 1 && rem < 301` gives the re-test with other compares, 63; `rem > 0 && rem < 301` 8; `!(rem >= 301)` 8;
`0 < rem && rem <= 300` 68; a bool local, a pointer to rem, nested ifs, `!(rem <= 0 || rem > 300)` and a copy of
rem 68.) Left: the alpha divide. The binary divides by -1000 directly (magic 0xef9db22d, then `add ecx, edx`);
8966 divides by 1000 and subtracts. Tried (all 4): split into `a /= -1000; a += base` and `a = x / -1000; a +=
base`, `base + ...`, `base * (rem - 300)`, a local `int d = -1000`, `const int k`, a static const, an enum
constant, `~999`, `(0 - 1000)`, `(1000 * -1)`, `(int)0xfffffc18`, `-1000L`, `(short)-1000`, a temp for rem - 300 or
for the product, `-(x) / 1000`, inline helpers for the divide and for the whole fade. `(300 - rem) * base / 1000`
is worse (225). This is the only divide by 0xef9db22d in Sum.exe. Same behaviour.

## Still differs (tried; do not repeat)

**D20504 (notes only). fast_sqrt 0x542590 / fast_sqrt_refined 0x542540 (distance 8).** The EAX / ECX swap again. Six
more forms on fast_sqrt, all 8 (split `e |=` 24): a pointer local to x (`unsigned *p = (unsigned *)&x; *p = ...`),
a pointer local plus a bits copy, a pointer into the table, `unsigned long` casts, an inline helper on the bits.
Register choice; stop.

**D20505 (notes only). ai_state::serpent_illusion_start 0x496ba0 (distance 42).** The scratch-register rotation
from the divide's sign fix (binary `mov ecx, edx`, ours `mov eax, edx`) to the end. Tried (all 42 unless noted): a
`ubyte *hv` store for the alpha and for the zero, a character_instance local (late and early), `a /= 1000` split,
a clamp through `int *pp`, an `r = rem - 1000` local, `int d = 1000`, a do/while clamp (44), a ternary clamp, a
ubyte result local (47), `0 < rem`, a bool for `rem > 0`, `p != NULL` in the last loop, illusion_fx_on stored
before invalidate (44), owner / handle / timer pointer locals.

**D20506 (notes only). serpent_anim_remap_in 0x495d00 (distance 78).** The binary keeps 0 in EBP (then i), ready
in c's argument slot (re-read and tested per pass) and one induction pointer with two offsets. Tried (78 unless
noted): ready from a ternary, declared at the top, after r, as unsigned, as bool (80); a ternary store; `ready != 0`;
anims indexed directly; no `a` local; a weight local (80); a do/while loop; a pointer to the boss struct (74, not
kept); the loop bound from serpent_anim_remap_count (72, but the binary reads set->count each pass: not kept);
count local; set declared after the rider test; a flags local; an anim pointer; a slot pointer local. `volatile int
ready` gives the binary's frame slot and prologue (71) but 8966 folds the 3 / 0 store into neg / sbb / and in
every form tried (ternary, inverted test, slot pointer, switch, `mount == 0`, `r != NULL`); not kept.

## Behaviour errors / bug candidates

None found. Every change has the same behaviour.

## Header changes

None.

## Changed files (in w12/src)

- game/unnamed_worldmap_automap_0x42f1e0.cpp (D20500)
- game/unnamed_serpent_0x495290.cpp (D20501)
- game/unnamed_evil_joseph_0x4c3470.cpp (D20502)
- game/conversationctl.cpp (D20503)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <w12/src> --only "unnamed_worldmap_automap_0x42f1e0|unnamed_serpent_0x495290|unnamed_evil_joseph_0x4c3470|conversationctl" --check src/decomp/vc6-matches.txt`:
4 units: 115 match, 0 wrong target, 9 differ, 0 not found, 0 errors; 112 matched there, 115 now, 0 lost.

## Worker 13

### W13 notes (D20600-D20699)

Batch: pathvis_bsp_contains, make_initial_edge_tree, do_single_vis_check (game/pathfinding_visibility.cpp),
pcx_read_bitmap, tga_rle_encode_row (vsdk/targautils.cpp), cmdline_take_argument (vsdk/cmdline.cpp),
ui_container::draw (vsdk/ui_container.cpp), ghost_anim_remap_in (game/unnamed_ghost_rider_0x4b9e70.cpp).

## New matches

| file | address | function | D | what did it |
|---|---|---|---|---|
| game/pathfinding_visibility.cpp | 0x4b7af0 | make_initial_edge_tree | D20603 | see below (66 -> match) |

**D20603 (game/pathfinding_visibility.cpp). make_initial_edge_tree 0x4b7af0: byte match.** Four changes, all needed:
1. `pf_edge *e = NULL; int i = 0;` declared before `profile_label label(...)` (the binary stores i = 0 before the EH
   state; with them after the label: distance 7 at the end).
2. `a_left = false;` before `a_right = false;` (the binary clears a_left with an immediate, a_right in BL).
3. The straddle test as `if (e->b->x <= pt->x) { if (!a_right) goto next; } else if (!a_left) goto next;` with
   `next:` before the alt-mode / `i++` step (the ternary `(b <= x) ? a_right : a_left`, its inversion, and a bool
   local all lay the a_left test first; the binary tests a_right first and jumps to the body).
4. `n->tag = pathvis_edge_tag;` before `n->dist = -d;` (8966 then loads the tag before the fchs as the binary).
Same behaviour (same tests, same stores; the tag / dist stores are to different fields).

## Closer, kept

| file | address | function | D | old -> new distance |
|---|---|---|---|---|
| game/pathfinding_visibility.cpp | 0x4b86e0 | pathvis_bsp_contains | D20601 | 50 -> 30 |
| game/pathfinding_visibility.cpp | 0x4b8230 | do_single_vis_check | D20602 | 76 -> 43 |
| vsdk/targautils.cpp | 0x554ea0 | tga_rle_encode_row | D20600 | 47 -> 41 |

**D20600 (vsdk/targautils.cpp). tga_rle_encode_row 0x554ea0: `int count = 1;` declared first.** Gives the binary's
prologue (row_bytes in ECX, EBP the only zero register, `lea edx, [edi + ecx]` for end). Left: 8966 keeps dst_bpp in
ECX across the loop's back edge (and reloads it at the loop head), the binary reloads it at each use. (count at
positions 0-3: 41; 4-5: 47 before the for test; 6: 41.)

**D20601 (game/pathfinding_visibility.cpp). pathvis_bsp_contains 0x4b86e0: the near-split recursion as one block
after the switch**: both cases `goto both;`, after the switch `continue;` then `both: if (!pathvis_bsp_contains(pt,
node->front)) return false; node = node->back;`. 50 -> 30. Left: the binary has the recursion inline in both cases
and cross-jumps case 2 (after its own `push front`) into case 1's copy at `push edi`; 8966 with the inline form
cross-jumps the other way (case 1 into case 2), and with the goto form keeps one shared block (case 2 tests with
`jne both`, the binary `je else`).

**D20602 (game/pathfinding_visibility.cpp). do_single_vis_check 0x4b8230: the two later `return 1`s as `goto
blocked;` with `blocked: return 1;` at the end** (8966 then shares the early `!pathvis_ray_clear` return's epilogue
as the binary does: 76 -> 47) **and `seg.a = &a;` right after `a = *from->pos;`, before `b = *to->pos;`** (47 -> 43;
all 120 orders of the five set-up statements tried; this one and `layer, a, seg.b, b, seg.a` give 43, the rest
45 or more). Left:
the binary has b (the to point) at the lower frame slot (-0x34) and a at -0x2c, so `fld b.x; fadd a.x`; 8966 puts
a lowest whatever the declaration order.

## Still differs (tried; do not repeat)

**D20604 (notes only). Tried without a match:**
- cmdline_take_argument 0x53e250 (distance 6, unchanged): a `bool *d = &cmdline_args[next].is_done` store (before
  the strlen or before strcpy), a `CmdArg *na` pointer local (store after, or source read then store before
  strcpy), the store as a comma expression with strcpy, strcpy's return in a local, a `char *dst` local with the
  store between new and strcpy (both orders), a `char *src` local read before the store. The after-strcpy forms all
  compile identically; every before-strcpy form moves the store to the top of the function (whole plan changes).
  Exhausted.
- pcx_read_bitmap 0x555f60 (distance 9, unchanged): a file-local inline helper for the 0x400 read (returning 0 into
  pos, void, pos by reference set before / after the read, pos by pointer, count by reference). Before-the-read
  forms push ESI for the zeros, after-the-read forms keep the xor after the call. Exhausted.
- tga_rle_encode_row (on top of D20600): a `ubyte *s = src` copy at each declaration position (copy-propagated),
  start / prev not initialised (worse), `prev = start`, the count == 1 branch's statement orders and chained
  `start = prev = src` / `prev = start = src`, min_run as if / `+ 1` (worse) / `!= 1`, a bool local for
  bytes_equal, a dst_bpp local for bytes_equal, inline helpers for the two calls (the function is then not found
  by vc6-compare), a while / goto-to-test loop. NOTE: `for (;;) { ... if (src >= end) break; }` reads 39 but changes
  behaviour (the `continue` then skips the end test); not kept.
- pathvis_bsp_contains (on top of D20601 / its base): source case order (c1 first, c3 first, default label: no
  change, 8966 sorts), bool / front-pointer locals for the recursion, `continue` in place of `break`, inverted
  near-split tests, `if (contains) node = back; else return false;`, tail-recursive forms (66, 108), `for (;;)` with
  the null test (66), do / while (47), do / while with the shared block (37), the shared label inside case 1 or
  case 2 (with the other case's locals assigned, not initialised: 30, the same code).
- do_single_vis_check (on top of D20602): declaration orders `b, a, mid` and four others and separate declarations
  (no frame change), `(a.x + b.x)` operand order, pointer locals for a / b in the mid sums, memcpy copies (worse),
  per-field copies, `0.5f * (b + a)` / `(a + b)` via the vector2 operators (110+), mid as a constructed local,
  copy-constructed a / b, a `vector2 p[2]` / `p[3]` array (53).
- ui_container::draw 0x553800 (distance 56, unchanged): text_x / text_y as `float txy[2]`, sx / sy as an array,
  text_w / text_h as an array, right / bottom as an array (all worse), a `const float *psy`, a copy of sy made after
  the string-size call, a `half` int local. The binary's sy slot (after text_x / text_y) does not move.
- ghost_anim_remap_in 0x4bb610 (distance 78, unchanged): ready through an `int *pr` (test, stores, both), ready
  initialised in its declaration, i declared first, the ready arms swapped, ready as bool (80), `ready = 0` before
  get_mount (82), i = 0 early with `ready = i`, `mount == NULL`. The binary keeps ready in c's dead slot, i in EBP
  (the zero register) and branches on ready; 8966 makes the kind store branchless (neg / sbb / and). The serpent /
  phoenix / tiger copies differ the same way.

## Behaviour errors / bug candidates
None found.

## Header changes
None.

## Changed files
- src/game/pathfinding_visibility.cpp (make_initial_edge_tree D20603, pathvis_bsp_contains D20601,
  do_single_vis_check D20602)
- src/vsdk/targautils.cpp (tga_rle_encode_row D20600)

## --check
`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "pathfinding_visibility|targautils" --check
src/decomp/vc6-matches.txt`: 2 units: 23 match, 9 differ; 22 matched there, 23 now, 0 lost.

## Worker 14

### m-1021c worker 14 (D20700-D20799)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/new_buysell.cpp | 0x4050f0 | buysell_count_party_items | D20700 | the clamp at 0 through a pointer to the local: `int *pp = &pos; *pp = max(*pp, 0); buysell_scroll_pos = pos;` gives the binary's `and eax, ecx` (D18500's device, with max() instead of the `&=` form; both match) |

## Closer, kept

- game/level_scripts_common.cpp 0x4cb7b0 level_parse_table: 57 -> 51 (**D20701**). The code (instructions) is now
  the binary's; only stack slots of the buffers differ.
- game/unnamed_phoenix_0x497ac0.cpp 0x499ad0 phoenix_anim_remap_in: 78 -> 70 (**D20702**, a device).

**D20700 (game/new_buysell.cpp). buysell_count_party_items 0x4050f0: match.** `int pos = min(buysell_scroll_pos,
limit); int *pp = &pos; *pp = max(*pp, 0); buysell_scroll_pos = pos;`. The `*pp &= (*pp <= 0) - 1` form matches
too. Without the pointer (a mask local `m & pos` / `pos & m`, a pointer to the global, a do / while (0) min)
it stays at 7 or gets worse (17). The same device may help the other max-0 AND near misses listed in D7104 /
D7305 / D14803 (status_page_up etc.); not tried, they are not in this batch.

**D20701 (game/level_scripts_common.cpp). level_parse_table 0x4cb7b0: 57 -> 51.** Two changes:
- `int layer = atoi(psx_layer + 6); if (layer == 0)` in the $AutomapPSX branch gives the binary's `cmp eax, ebp`
  (zero register) for `atoi(...) == 0` (was `test eax, eax`). `!layer`, a layer declared at the block top, all the
  same; `0 == atoi(...)` does nothing.
- `char filename[256]` declared in the `if (!level_script_restarting)` block. Evidence: in the binary filename
  (frame -544) overlaps eax_path (-800..-416), so their scopes are disjoint.
With these the instruction stream is identical (vc6-side shows no mnemonic difference except the EH funclets of
ours, which vc6-side lists at the end); what is left is the frame slots of the buffers. Binary frame offsets
(EBP-less frame, relative to the EH record frame as in our .cod): tf -1424, fxname -1344, script -1312, pc_layer
-1312, psx_bitmap -1184, fog_layer -1184, eax_name -1184, psx_layer -928, pc_bitmap -800, eax_path -800, filename
-544, f -288. Ours now: eax_name / psx_layer -1312, pc_layer / script -1184, fog_layer / psx_bitmap -1056,
eax_path / filename -800, pc_bitmap -416. Findings about 8966's slot choice here: declaration order within a
block (psx / pc pairs swapped, 4 combinations), moving the four automap buffers to the while-body scope, and
renaming a buffer change nothing; a buffer's size does (pc_bitmap[0x7f] gives a different layout, distance
41, not kept: sizes are not ours to change). So the layout is driven by sizes and use, not order or names;
helper scripts in w14/try (layout.mjs, slots.mjs) print our layout and the per-line slot differences.

**D20702 (game/unnamed_phoenix_0x497ac0.cpp; a device). phoenix_anim_remap_in 0x499ad0: 78 -> 70.**
`volatile int ready` (as D15010 noted: the binary keeps ready in c's dead argument slot and reloads it for the
test) plus `do { if (ready) { slot = 3; break; } slot = 0; } while (0);` (D18400's device) keeps the 3 / 0
stores as two branches as the binary (volatile alone folds them into neg / sbb / and, 71). The behaviour is the
same. Left: i is in EDI (binary EBP, also its zero register), set in ESI (binary EDI), the late push is EBP
(binary ESI), and the slot stores go through a separate pointer (ECX) where the binary addresses both slot and
weight as an offset from the blend pointer (`[edx + eax]`, `[esi + eax]`, edx / esi = global - set).
ghost_anim_remap_in 0x4bb610 and serpent_anim_remap_in 0x495d00 are the same function at the same distance
(78); the same two changes should apply to them (not done: not in this batch).

## Still differs (tried; do not repeat)

- **pick_test_character 0x43c620 (2)**: the `add esp, 8` / `fcomp best_r` order after distance(). Tried, all
  still 2: inline helpers (half distance as float, a bool far_from, an int far_from with if / return, a
  distance-only obj_dist, a by-reference vector3 distance wrapper), a comma expression through a sep local,
  parenthesised argument forms, best_r through a `float *` local. With D19001's list, treat as exhausted
  (scheduling).
- **party_set_level 0x4cec00 (2)**: the SIB order of the skill copy. Tried, all still 2: unsigned index with a
  signed test (D17702's device), an inline copy_skill helper, an inline info_skill getter, `(&skill[0])[i]`,
  `(*c->info).skill[i]`, `*(i + (ubyte *)skill)`, an int-sum address form, `long i`, a store through a pointer
  local, a one-byte memcpy; `i++` first with `[i - 1]` is worse (6). Exhausted with D17901's list.
- **snow_start 0x494ab0 (2)**: `add ebp, 0xb4` before `mov ebx, 15`. Tried, all still 2: the inner count set
  before the row step (`j = SNOW_GRID; row += SNOW_GRID; for (; j; j--)`), the same as do / while (--j), a
  snow_next_row(&row) helper returning the old row, a void step helper by reference, the step through a
  `snow_cell **rp` pointer, rec / j / row step in one for-init with a comma. Exhausted with D17404's list.
- **character_can_talk_to 0x43a460 (50; the real difference is two FPU instructions)**: the binary reloads d.z
  once and squares it as `fld st(0) / fmul st, st(1)` (and keeps d.x unconsumed: `fld st(2) / fmul st, st(3)`),
  popping both after the fsqrt; ours multiplies d.z from memory. The distance number is inflated by LCS
  alignment. Tried: inline helpers (x, z) and (z, x) by value, by const vector3 &, by value vector3 (worse),
  `(double)(float)sqrt` in place and in helpers, a `const float *pz`, `dz` copies, an `xz[2]` array, a `float *pd`
  index form, a squared-sum local, a `sq()` helper on both or one term, `double` sqrt local, scalar dx / dy / dz
  locals (all three declaration orders, separate or initialised: 8966 then computes dz before dy and keeps all
  three on the FPU, no spill; worse), a vector3 d with scalar dx / dy, and `d.y = 0` followed by d.len(),
  sqrt(d.len_sq()), sqrt(d.dot(d)) or explicit three-term sums in all six orders. Only the `d.y = 0` plus a
  vector3 member call (len / len_sq) gives a `fld st(0)` dup of d.z, but with the x term first and fxch pairs
  (45, not the binary's code). Not found.
- **version_check_run 0x410f40 (60)**: the only real difference: ours schedules `pop edi` right after
  `mov esi, 1` in the SkipVerify block, so its tail differs from cases 1 / 2 and is not cross-jumped; in the
  binary the SkipVerify stores start a basic block (cases 1 / 2 of the inner switch jump to 0x410f90, after
  `mov esi, 1`). Tried: a goto from cases 1 / 2 to a label in the SkipVerify block (143: the zero register is
  lost, 1 goes to EBX), an inline vc_finish(r) helper for both, chained stores, `return version_check_result`,
  `if (!skip) goto fetch;` with the tail unindented. All 60 except the goto.
- **level_parse_table**: see D20701 for the slot analysis; the forms that did not change the layout are listed
  there.
- **phoenix_anim_remap_in** (beyond D20702): a loco_blend_set pointer over phoenix.anim_remap_count (the phoenix
  remap block has the loco_blend_set layout: count, two ints, two floats) gives 69 with volatile + do / while
  (74 / 72 without volatile), not kept (a cast over the boss struct for one step); without `a`, i at function
  scope, a weight pointer local: all 70.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/game/new_buysell.cpp (buysell_count_party_items, D20700)
- src/game/level_scripts_common.cpp (level_parse_table, D20701)
- src/game/unnamed_phoenix_0x497ac0.cpp (phoenix_anim_remap_in, D20702)

## --check results (VC6_STRICT=1 vc6-sweep --check src/decomp/vc6-matches.txt)

- new_buysell: 40 matched there, 41 now, 0 lost
- level_scripts_common: 74 matched there, 74 now, 0 lost
- unnamed_phoenix_0x497ac0: 33 matched there, 33 now, 0 lost
