# Byte-match round 2026-10-21: the ten workers' notes

A targeted round: the 67 functions that `tools/vc6-nearmiss.mjs` ranked within distance 12 of a match, in ten
disjoint batches of whole files (no shared header edits but one). Result: 17 new matches (strict 5,445 -> 5,462,
none lost), 3 closer. The decisions are D17200-D18101 in include/DECISIONS.md.

## Worker 1

### m-1021 worker 1 (D17200-D17203)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/unnamed_avoidance_0x4a6710.cpp | 0x4a77d0 | avoid_spline_point_get | D17200 | range test through a bool local |
| game/unnamed_avoidance_0x4a6710.cpp | 0x4a7830 | avoid_spline_get | D17200 | same form |
| game/unnamed_avoidance_0x4a6710.cpp | 0x4a7900 | avoid_spline_point_create | D17201 | both points copied as avoid_spline_end (POD) |
| game/unnamed_avoidance_0x4a6710.cpp | 0x4a6bf0 | avoid_circle_contains_point | D17202 | centre through a pointer local |
| game/unnamed_avoidance_0x4a6710.cpp | 0x4a6f90 | avoid_box_box_overlap | D17203 | pointer copies bo = b (b's half sizes in tests 1-2) and ao = a (k_aa's receiver) |

**D17200 (game/unnamed_avoidance_0x4a6710.cpp). avoid_spline_point_get 0x4a77d0 and avoid_spline_get 0x4a7830
are exact** with `bool ok = i >= 0 && i < N; if (!ok) return NULL; return (flags & 1) ? &x[i] : NULL;`. 8966 then
places the NULL return right after the two tests (`jl null; cmp; jl body; null: xor eax, eax; ret`), as in the
binary. Every if / early-return / ternary / loop / switch / retval form tried (about 20 here, plus 15 before)
put the body first. Same behaviour.

**D17201 (same file). avoid_spline_point_create 0x4a7900 is exact** with the two points copied as the plain
8-byte struct avoid_spline_end (D13300): `*(avoid_spline_end *)&pt->p0 = *(const avoid_spline_end *)a;` and the
same for p1. Through vector2's inline operator= the p1.y store stays ahead of the `wall.a` store. Copying only p1
this way also matches. p0 alone, component copies, the constructor, reference locals, pointer locals and all 24
orders of wall.a / wall.b / size / layer do not match. Same behaviour, since both copy the same 8 bytes.

**D17202 (same file). avoid_circle_contains_point 0x4a6bf0 is exact** with
`const vector2 *c = &ob->centre;` and `d.x = p->x - c->x; d.y = p->y - c->y;`. r then goes into ob's dead
argument slot (binary [esp+0x14]) instead of p's. Without it: operator- for d, an initialising constructor,
r declared first, r assigned inside r2's expression, d.y first and lsq after r all compile the same or worse.
Same behaviour.

**D17203 (same file). avoid_box_box_overlap 0x4a6f90 is exact.** On top of D15502 there are now two pointer
copies, `const avoid_obstacle *bo = b; const avoid_obstacle *ao = a;`. b's half sizes in the first two tests
are read as `bo->half_b` / `bo->half_a`, and k_aa is `fabs(ao->axis_a.dot(b->axis_a))`. In 8966 the operand order
of commutative float operations does not follow the source. It changes with which symbol an operand reads
through: all orders of terms and factors compile the same. bo alone fixes the term order of tests 1 and 2 and the
x product of k_ab (distance 12 -> 4). ao then fixes k_aa's x product. Declaring ao without using it also matches,
so it is the extra symbol that counts. The names do not matter (o/q, bo/ao, b2/a2 and other/self all match).
These do not help: the 24 declaration orders, the 24 assignment orders x 4 k_ab spellings, 100 k_aa x k_ba
spellings, every three-term order and parenthesisation of test 1, the explicit and dot forms of k_aa with bo,
and an alias for the half pointer. Same values: the same products and sums are added in the binary's order.

## Closer, kept

None (each improvement above is a full match).

## Still differs (tried; do not repeat)

- **game/combat.cpp 0x464d00 combat_position_modifier** (distance 6, unchanged). The binary schedules
  `lea ecx, [esi+0x324]` (this for ai.is_mode) after the third product of the facing dot. Tried with no change:
  an ai_state pointer local (before and after facing), a reference and a pointer for orient.fvec,
  `fvec.dot(d)` and `d.dot(fvec)`, facing split into two statements, a Character alias for the ai call, for
  facing, for single products and for pos, unused aliases, a bool for is_mode, all five other declaration orders
  of m / d / facing, and `!(facing < 0)`. Parenthesising the sum, `d = defender->pos - attacker->pos` and a
  double facing made it worse. Add the 48 dot forms from earlier rounds. This looks like 8966 scheduling (D7807:
  the function matches under 8168).
- **vsdk/unnamed_gr_line_0x53e520.cpp 0x53edd0 gr_bitmap_3d_oriented_quad** (distance 8, unchanged). The binary
  stores v[3].x after the integer stores of v[3] and the width push. Tried with no change: a vertex pointer for
  v[3], `u = v = 0` chained, x through a local stored after y, `-(w*ca + h*sa)`, `pnt->x - (...)`, pnt first in
  the sums, and all 29 other placements of the x / y stores among v[3]'s six. `width = width * 0.5f` and
  `/= 2.0f` compile the same. Worse: codes_and = 0xff in seven other places, halved width / height in new locals
  (frame grows), height halved first, a width local for the far-clip call. Add the 8 store orders from earlier.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/game/unnamed_avoidance_0x4a6710.cpp: avoid_circle_contains_point, avoid_box_box_overlap,
  avoid_spline_point_get, avoid_spline_get and avoid_spline_point_create, with comments and confidence set to
  exact.

## --check result

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "unnamed_avoidance_0x4a6710" --check src/decomp/vc6-matches.txt`:
`1 units: 56 match, 0 wrong target, 11 differ` and `51 matched there, 56 now, 0 lost`.

## Devices worth reusing

- A bool local for a range test moves the failure return in front of the body (D17200).
- A POD struct copy in place of an inline member operator= changes how stores are scheduled (D17201).
- A pointer copy of a parameter (`T *o = b;`), used for some of the reads, changes 8966's operand order in
  commutative float operations and which dead argument slot a spill takes (D17202, D17203). Try it where earlier
  rounds wrote "8966 emits one order whatever the source".

## Worker 2

### m-1021 worker 2 notes (D17300-D17399)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/s3d.cpp | 0x470220 | s3d_read_door | D17300 | store order lock / _unk5c / state, then the floor point cleared through a new file-local `static inline void s3d_door_clear3(float *p)` (three `p[i] = 0`); the sound fields stay written out. The inline helper's block boundary puts `mov ecx, edi` (this for f->error()) after the floor stores, as in the binary. |

**D17300 (game/s3d.cpp; m-1021 worker 2). s3d_read_door 0x470220 is exact.** The run-time reset is written
`lock = 0; _unk5c = 0; state &= ~0xb; s3d_door_clear3(door->_unk48);` and then the ten sound-field stores as
before. The helper is ours: an inline over the three floats. Its expansion closes a block, and 8966 then
schedules the `mov ecx, edi` for `f->error()` after the floor stores (0x4702ad), where the binary has it. Earlier
rounds wrote the floor point out (360 orders, and a sound_ref helper with two calls); that left the mov one block
early. A floor helper with a sound_ref helper, or the floor written out with a sound_ref helper, still differs.
Same stores and values, same behaviour. Device: **a small inline helper around a group of stores can move a
scheduled instruction (here the `this` load for the next call) past that group.**

## Closer, kept

None.

## Still differs (tried; do not repeat)

**D17301 (notes only). vector3::random_unit_vector 0x524650 / random_direction 0x5246a0 / random_direction_in_cone
0x5246f0 (distance 5 each).** 8966 squares z with `fld st(0); fmul st, st(1)` and keeps the loaded z until after
the r store (`fstp st(0)`); the binary does `fmulp`. This round added 26 forms, all with the same code:
- unit flags (/Ox, /O2, /Ox without /Ow, /Oa, /Ob1, /Og /Oi /Ot /Oy). None fixes it; /Op and /Ob1 make it much
  worse.
- r: `-(z*z) + 1`, `r * cos(a)`, double sqrt / cos casts, `1.0 - z*z`, `(double)z * z`, r computed in place
  (`r = 1 - z*z; r = sqrt(r)`), r assigned inside the x statement, r declared at use or in a block.
- this / z access: `this->z`, a `vector3 *v = this` local, `float &zr = z`, a local `one`, a `zz` local
  assigned to z, `__forceinline sq(z)`.
- declaration order `r, a`; a as double.
- blocks around the r statement, around z / a / r, and around the whole tail.
- `float t = z` after a (in a block or not): the z copy is still kept.
- `float t = z` before frand(): t goes to memory (`fld t; fmul t`). The tail then matches but z is read from t,
  so it is worse.

**D17302 (notes only). vector3_angle_and_axis 0x5249e0 (distance 2).** The acos result is stored into a's
argument slot. The binary schedules the fstp after `push ecx; push edi`; ours stores it right after the call.
15 more forms, all the same: `angle = -angle` / `*= -1` then one return, a ternary return, else-return,
angle declared at use or first, `!(x >= 0)`, `0 > x`, `na.dot(cross)` (worse), a double acos, a dot local, an
`s` local for the sign, `(*a).cross`. `vector3 c = a->cross(*b)` and a const-ref temporary are worse: the frame
grows.

**D17303 (notes only). fx_create_spell_effect_facing 0x48c8a0 (distance 4).** right.x is `0 - dir.z`. The binary
has `fld 0; fld dir.z; fsubr st, st(1)`; ours has `fld 0; fld st(0); fsub dir.z`. This is the same difference as
blood_splot_place (D9301). 23 forms tried:
- a `vector3(0,1,0)` temporary in the cross, and `right = dir.cross(up)` as an assignment (worse).
- all 16 operand orders of the written-out products `dir.y * up.z - dir.z * up.y` and so on: they compile like
  the cross.
- written-out components with literal constants (`0.0f - dir.z`, `-(dir.z - 0)`, `dir.z * 1.0f`, a `zero`
  local), and right.z first. 8966 folds these to `fchs`, which is worse.

**D17304 (notes only). options_init 0x41f0f0 (distance 10).** 8966 loads Summoner_options' first dword before
the rep movsd and pushes that value. The binary reads options_edit after the copy, and it stores
options_mouse_mode inside the copy's register setup. 17 more forms, all with the same forwarding:
- the copy as an inline `options_copy(&edit, &src)` helper, through a `SummonerOptions &` local, through a
  `const SummonerOptions *src`, in its own block, cast through `void *`, or as a 16-int or 64-char block struct
  (also through a pointer local).
- `*(volatile int *)&` on the first argument read.
- the mouse_mode store after the copy, plain or through `*(volatile int *)&`. The volatile forms move the
  mouse_mode store but not the read.
- for the `test al, cl` order: nested ifs, `!= 0`, `1u`, a `(ubyte)` cast, `options_flags = options_flags | 1`
  and `1 | options_flags`.

## Behaviour errors / bug candidates

None found. D17300 keeps every store and value.

## Header changes

None.

## Changed files

- src/game/s3d.cpp (s3d_read_door and the new s3d_door_clear3 helper above it). The other batch files are
  unchanged and still the same as REPO.

## --check

- game/s3d.cpp: `VC6_STRICT=1 vc6-sweep --only "game/s3d\.cpp" --check src/decomp/vc6-matches.txt`:
  1 unit, 32 match, 0 wrong target, 2 differ. 31 matched there, 32 now, **0 lost**.

## Worker 3

### m-1021 worker W3 (decisions D17400-D17406)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/level_scripts.cpp | 0x4d1390 | savegame::automap_label_remove | D17401 | x and z read into a two-float array `float rxz[2]` (not two float scalars), which puts `start` below the pair in the frame, as in the binary |
| game/level_scripts.cpp | 0x4d3740 | savegame::read_automap_labels | D17402 | the same device: `float xz[2]` for x / z puts count in the binary's frame slot |
| game/unnamed_interface_0x411270.cpp | 0x412400 | ui_3d_begin | D17403 | `vector3 p;` declared at function scope, not inside the do-loop body: 8966 then loads base->light_tracks after p's stores, as the binary does |

**D17401 (game/level_scripts.cpp). savegame::automap_label_remove 0x4d1390 matches** when x / z are read into
`float rxz[2]` (still with the `*(int *)&rxz[k] = ...` integer copies). With two float scalars 8966 puts rx / rz
below `start` (start at -36, rx -44, rz -40); the binary has start at -44 and the pair above it. An anonymous
`struct { float x, z; }` local matches too. Declaration order, int-typed rx / rz, memcpy reads and the compare
order do not move the slot.

**D17402 (game/level_scripts.cpp). savegame::read_automap_labels 0x4d3740 matches** with the same device
(`float xz[2]`; the struct form matches too). It also looks worth trying on the other frame-slot cases in this
unit (layout_conversation, write_item_objects). I have not tried it on them because they are not in my batch.

**D17403 (game/unnamed_interface_0x411270.cpp). ui_3d_begin 0x412400 matches** with `vector3 p;` at function
scope. With p block-scoped inside the do-loop, 8966 moves the `mov eax, [ebp+0x24]` (base->light_tracks) above
p's fstp stores. Indexed `vfx_light_track` forms, a track pointer local, `track + base->light_tracks`, a float t
local, `*pos + light->pos` and copy-initialisation (`vector3 p = ...`, frame 0xc) do not do it.

## Closer, kept

**D17400 (game/unnamed_snow_0x4946a0.cpp). snow_start 0x494ab0: 4 -> 2.** The outer test is now
`while ((int)cp < (int)snow_cells[SNOW_GRID])`, which gives the binary's signed `jl`. Behaviour is the same:
both addresses lie in one array. What is left is one swapped pair: `add ebp, 0xb4` comes before `mov ebx, 15`
in ours and after it in the binary.

## Still differs (tried; do not repeat)

**D17404 (notes only). snow_start 0x494ab0 (distance 2 now).** I tried 28 forms. The plain double index
`snow_cells[i][j] = &snow_cell_records[i][j]` folds into single linear pointers. Forms with an int i loop
(`rec = snow_cell_records[i]`, a cp pointer, `cp[j] = &rec[j]`, `snow_cells[i][j] = rec++`, a row pointer with
i, `row += SNOW_GRID` in the for increment) put the row step at the bottom, or make the test on ebp. These do
not move the swapped pair: the row step in the inner for's init or at the end of the body, `rec = (row += 15) - 15`,
`row = rec + 15`, a do / while inner loop, `cp - end < 0`, `(*cp++)->reset()`, separate `rec++`, cp[0] / cp[j]
indexing. Most likely it is 8966 scheduling.

**D17405 (notes only). level_add_navpoint 0x4d5720 (distance 4).** Only the z component of
`ray.start + ray.dir * hit.t` differs: the binary loads start.z first, 8966 loads the product's temporary first.
I tried 15 forms and none changes it: a t local, a copy constructor, parentheses, a start copy, an initialised
local, explicit per-component sums in both operand orders (8966 reorders them), a vector3(...) of the products,
fully expanded components (frame changes), `+=` in both orders, `dir * t + start`, a `lobj_ray *` local and a
named product local (frame changes).

**D17406 (notes only). worldmap_frame 0x42f500 (distance 6) and hot_button_set 0x4d16a0 (distance 6).**
- worldmap_frame: the binary pops the three components with `fstp st(3)` and then narrows the sqrt result
  through a stack temporary. Our vector3::len() narrows before the pops (its `(double)(float)` device). I tried
  inline helpers that return a double sqrt, with `(float)`, `(double)(float)`, a float local, a parenthesised
  local and a volatile local. They do reproduce `fstp st(3)` and the two pops, but they then narrow straight into
  a `push ecx` argument slot instead of temp / mov / push, which is a bigger difference (distance 67). Also
  tried: float-returning helpers with (float), (double) or a double local; nested inlines; and an inline sqrt of
  the sum. The only form that would match is probably a different len() in the header, which would affect its
  other callers. Not attempted.
- hot_button_set: the store SIB order `[esi+eax]` (binary `[eax+esi]`). Inline store helpers (by value and by
  reference off), `*(off + data)`, memcpy for the id, `(char *)` casts and `this->` all give the same order.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/game/level_scripts.cpp (automap_label_remove, read_automap_labels)
- src/game/unnamed_interface_0x411270.cpp (ui_3d_begin)
- src/game/unnamed_snow_0x4946a0.cpp (snow_start)

## --check results (VC6_STRICT=1, against src/decomp/vc6-matches.txt)

- level_scripts (the regex also takes in a second unit): 178 matched there, 180 now, 0 lost
- unnamed_snow_0x4946a0: 10 matched there, 10 now, 0 lost
- unnamed_interface_0x411270: 64 matched there, 65 now, 0 lost

## Worker 4

### m-1021 worker 4 (D17500-D17599)

## New matches
None.

## Closer, kept
- game/unnamed_pfgeom_0x4a4ee0.cpp 0x4a6530 seg2d_point_near_middle: distance 10 -> 4 (D17500).

**D17500 (game/unnamed_pfgeom_0x4a4ee0.cpp; closer, kept).** seg2d_point_near_middle: `const vector2 *a = s->a;`
and `mid = 0.5f * (*a + *s->b);` with `dir` still built from `s->b->x / s->b->y`. This gives the binary's start
(s in EAX, a in ECX, b in EAX) and both midpoint sums loading b first. Two other forms are worse: a and b both
in locals is 8 (the y sum loads a first), and b in a local with s->a direct is 18. The only remaining difference
is the second dot product's x term: the binary loads d.x and multiplies by perp.x, 8966 loads perp.x first. Same
value, the behaviour does not change.

## Still differs (tried; do not repeat)

**D17501 (notes only). Tried without a match:**
- gr_env_map_uv 0x51f960 (distance 2): the `lea` of the `2.0f * *normal` result temporary is scheduled before
  the dot product's `fstp [esp]` (the binary loads it after `push esi`). Forms tried, all compile the same:
  a `float two` local, `const vector3 &n`, `e.dot(*normal)` (worse), a `(float)` cast of the dot product,
  a `vector3(...)` copy of the product, no parentheses, `2` and `2.0` as the factor, an explicit
  `operator*(2.0f, *normal)` call, `.operator*(dot)`, `normal[0]`, a pointer local, e declared before r (worse),
  e assigned after its declaration (worse). Unit flags /O2 /Ob0, /O2 /Ob0 /Ow, /Ox /Ob0 and /Oa leave this
  lea where it is, and they lose matches elsewhere in the unit. This is scheduling.
- lights_calc_point_specular_color 0x51f560 (distance 4): the same lea, twice (both reflections). Not retried
  separately, since the gr_env_map_uv forms cover it.
- light_create_spot 0x51e3f0 (distance 2): the clamped intensity's fstp. Looked at only. Earlier rounds tried
  the clamp order and a local k. The flag variants above do not move it either.
- ai_state::tentacle_beast_boss_load 0x4c6d10 (distance 2): the last find_move's `mov ecx, [esi+0x4c]` comes
  before `push 0x87`, where the binary has it after. Forms tried, all compile the same: the slots loop through
  t, `this->owner`, 135 written in decimal, a do/while loop, a `Character *o` block, a `j` loop counter,
  `i = 0; for (; ...)`, a slot pointer expression, a braced loop body.
- packfile_read_directory 0x533f90 (distance 4): the code is otherwise identical. The binary keeps p in pf's
  dead argument slot ([esp+0x50]), and 8966 puts it in sector's slot. Forms tried, no change: `&sector[0]`,
  `const ubyte *`, `char *`, p set in the for-init, the size read from p + 60 (worse), an int offset in
  place of p (worse). The earlier rounds' forms (declaration, scope, walking sector, a pf copy) were not
  repeated.
- cmdline_take_argument 0x53e250 (distance 6): the binary stores is_done inside the inline strcpy. Forms tried:
  a `char *dst` local, `strcpy(param->arg = new ...)`, a `char *src` local, `= 1` (all compile the same), is_done
  stored between new and strcpy (55), `cmdline_args[index + 1]` (39).
- seg2d_point_near_middle (after D17500): the perp factor order. Forms tried: `d.dot(dir)` / `dir.dot(d)` /
  `d.dot(perp)` / `perp.dot(d)` in all four pairings, three term and factor orders, a named
  `vector2 perp2(...)`, perp's fields stored separately in both orders (worse), d computed before perp (worse),
  `-1.0f * dir.y` (worse), `d = mid - *pt` and `d = vector2(...)` (these flip the y term instead), a named
  d2, mixed perp / dir forms (these drop perp.x's store), and all 24 declaration orders of mid / d / perp /
  half (no change). The operand order is set by 8966's canonical sort.

## Behaviour errors / bug candidates
None found.

## Header changes
None.

## Changed files
- src/game/unnamed_pfgeom_0x4a4ee0.cpp (seg2d_point_near_middle, D17500)

## --check
- unnamed_pfgeom_0x4a4ee0: `VC6_STRICT=1 vc6-sweep --only unnamed_pfgeom_0x4a4ee0 --check`: 20 matched there,
  20 now, 0 lost.

## Worker 5

### W5 notes (m-1021, decisions D17600-D17699)

## New matches
- vsdk/gr_direct3d.cpp 0x538b70 gr_d3d_set_window, D17600: copying the client corner into locals
  (`int x = rect.left; int y = rect.top;`) before the sizes and computing `w = abs(rect.right - x + 1)` etc.
  gives the binary's `lea edx, [ecx + esi - 1]` / `[eax + edi - 1]` base / index order. Same behaviour.

**D17600 (vsdk/gr_direct3d.cpp; m-1021 worker 5). gr_d3d_set_window 0x538b70 is exact** with rect.left /
rect.top copied into locals x / y before the width / height and used both in the size and in the stored
rectangle. Every operand order of `rect.left + w - 1` (w first, -1 first, parenthesised, the stored global as
operand) compiles the same; locals x / y declared after the sizes do not help; the copy must feed the abs()
expression. (Also exact: keeping the min results in separate locals `cw` / `ch` and assigning them back.)
A device for lea base / index order: a local copy of the struct member that is used in an earlier expression.

## Closer, kept
- game/new_inventory.cpp 0x417170 inventory_destroy_selected_item, D17601: distance 11 -> 11 (the distance
  masks relocations), but the first two loads were swapped: the binary loads inventory_scroll_pos (0x5bd970)
  into EAX first and inventory_selected_row (0x5c0ff8) into ECX. `index = inventory_scroll_pos +
  inventory_selected_row` now loads as the binary does. Only the max(pos, 0) AND direction remains.

**D17601 (game/new_inventory.cpp; m-1021 worker 5; closer only). inventory_destroy_selected_item 0x417170:** the
index is `inventory_scroll_pos + inventory_selected_row` (the binary's load order; the other order loads the
two globals into swapped registers, which the distance score does not show). Same value. Possibly the same in
inventory_identify_item (its row is written `inventory_selected_row + inventory_scroll_pos`; not in this
batch, not checked).

## Still differs (tried; do not repeat)
**D17602 (notes only, m-1021 worker 5). Tried without a match:**
- level_vis_reset 0x487030 (22 forms): unsigned index, index with chained sets_high = sets, sets-first and
  sets_high-first store orders, a `s3d_vis_set ***` pointer to .sets with p[-1] / p[0] / p[1], a pointer
  compared via `&v->sets`, a pointer plus an unsigned counter, a pointer declared inside an index loop, a
  reference local, do/while index, `!=` bound, count = level_num_vis_sets, count / num as 1u / (short) /
  (char) / true, unsigned char index, a local copy of the lists address, num store after the lists store.
  Every form that gives the binary's pointer biased to .sets (`[edx - 4]`) also CSEs the constant 1 into a
  register; every unbiased pointer form keeps the immediate. Not reproduced.
- gr_d3d_set_window: matched (D17600).
- rfx_preload 0x51bfd0 (13 forms): `lea edi, [ecx + edx]` (binary `[edx + ecx]`): local copies of offset /
  buf_size in all orders (copy-propagated away), `total = a; total += b` both orders, `total = total + b`,
  casts, `offset - -buf_size`. The order does not follow the source.
- inventory_destroy_selected_item (22 forms beyond the prior 25): nested max(min()), global `&=` after min
  (AND into the right direction but the value in ECX, the binary's in EAX), `if (scroll > lim)` / `>=` /
  ternaries on the global, pos from an if (`pos = lim; if (scroll < pos) pos = scroll;`) then the global or
  max, min with swapped arguments, mask local, `pos & -(pos > 0)`, a saved `cur` copy. The binary keeps the
  limit's register (EAX) as the result and ANDs into it; no form gives both.
- anim_track::calc_rot_tangents 0x5506e0 (4 forms): explicit float casts, int / float locals for the y
  components (worse, 12), a short temporary for tan_b.x. The movsx of log_prev.y is scheduled before the
  tan_b.x store by 8966: scheduling.
- levelrender_push_attachment 0x483430 (4 forms): the radius copy placed after d, after depth (worse, 14),
  after the depth store (worse, 33), first (worse, 22). The radius store is two instructions early:
  scheduling.
- levelrender_add_effect_entry 0x4841e0: not attempted (49 store orders already tried for the pop esi
  placement; scheduling).

## Behaviour errors / bug candidates
None found.

## Header changes
None.

## Changed files
- src/vsdk/gr_direct3d.cpp (gr_d3d_set_window, D17600)
- src/game/new_inventory.cpp (inventory_destroy_selected_item, D17601)

## --check
`VC6_STRICT=1 vc6-sweep --only "gr_direct3d|new_inventory" --check src/decomp/vc6-matches.txt`:
2 units: 108 match, 11 differ, 0 errors; 107 matched there, 108 now, 0 lost.

## Worker 6

### m-1021 worker 6 (D17700-D17799)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/ai.cpp | 0x472780 | ai_state::pick_attack_slot | D17700 | the attacker's centre written out, x as `him.x = attacker->pos.x; him.x += attacker->centre_offset.x;` (y / z plain sums) |
| game/ai.cpp | 0x472840 | ai_state::claim_attack_slot | D17701 | the own centre written the same way (`me.x = self->pos.x; me.x += self->centre_offset.x;`), plus `dir = him - me` and `dot = s.dot(dir)` (all three needed) |
| vsdk/unnamed_gr_poly_0x5450c0.cpp | 0x545ac0 | gr_poly_clip_edge | D17702 | the pass index `unsigned i` with a signed loop test `(int)i < gr_poly_num_textures` |
| game/unnamed_dialogue_box_0x4d6660.cpp | 0x4d7480 | dialogue_box::last_visible_line | D17703 | `int page = gr_get_font_height(-1) * lines; int bottom = page - (int)scroll;` |

**D17700 (game/ai.cpp). ai_state::pick_attack_slot 0x472780 is a byte match.** The one difference was the
operand order of the attacker centre's x add (the binary loads pos.x first, fadd centre_offset.x; get_center()
and every written-out sum gave centre_offset first). Writing x as a store then `+=` (`him.x = attacker->pos.x;
him.x += attacker->centre_offset.x;`) gives the binary's order; y / z stay plain sums. Same behaviour (the sum is
kept on the FPU stack in both). A reproduction device: 8966 commutes a plain `a + b` by its own rule.

**D17701 (game/ai.cpp). ai_state::claim_attack_slot 0x472840 is a byte match.** Here the own (self) centre is
the one whose x is pos-first in the binary: `me.x = self->pos.x; me.x += self->centre_offset.x;` alone fixes it
but flips the last dot product's operands (distance 4); with pick_attack_slot's `dir = him - me` and
`dot = s.dot(dir)` as well, everything matches. Tried before reaching it (distance 4 each): six declaration
orders of me / him / dir (with s in the loop; s at the top gives frame 0x30), dot written s.x * dir.x,
dir.x * s.x first, all-dir-first, z / y swapped, `dot +=` for x, `dir.dot(s)`; `me = self->pos; me += offset`
(worse, 74), offset-first and plain sums for me (no change), x stored last (worse).

**D17702 (vsdk/unnamed_gr_poly_0x5450c0.cpp). gr_poly_clip_edge 0x545ac0 is a byte match.** The base / index
order of the three uv addresses (`[eax + esi]` in the binary) follows the signedness of the pass index: `unsigned
i` gives it, and `(int)i < gr_poly_num_textures` keeps the signed loop test (jle); plain `unsigned i` with an
unsigned compare gives jbe (distance 2). `[(unsigned)i]` on every index with int i also matches; the loop-test
cast is the smaller change. Same behaviour (i runs 0 .. count - 1 either way). The same device may help
gr_clip_edge (noted in D13200 as having the same address order) - not in this batch, not tried.

**D17703 (game/unnamed_dialogue_box_0x4d6660.cpp). dialogue_box::last_visible_line 0x4d7480 is a byte match.**
The EBX / EBP swap (count / ring index) went away when the bottom is computed in two statements: `int page =
gr_get_font_height(-1) * lines; int bottom = page - (int)scroll;`. Other splits did not: `h = font height` then
`h * lines - scroll` (or lines * h, or a scroll local) moves the imul (distance 6); `bottom = ...; bottom -= ...`
moves this into ESI (24). The two dialogue draw functions with the same ring-index register issue (D9239) might
answer to the same split; not in this batch, not tried.

## Closer, kept

None.

## Still differs (tried; do not repeat)

**D17704 (notes only). vsdk/unnamed_net_0x517590.cpp nw_ras_init 0x5178c0 (distance 0, only the cross-jump target of
the two loop failures).** Finding: 8966 points the loop failures at the nearest preceding identical FreeLibrary /
return 0 block (the count < 1 block; with count's block removed or made different it picks the enumeration block,
then the third GetProcAddress block). The binary points them at the first GetProcAddress block, so in the original
the four blocks in between are not identical candidates, or the first is created later. Tried: `||` of enumeration
and count, `||` of the two loop calls, `count <= 0`, `!= 0`, else-if, a braced success tail, goto fail with the
label at the end (target moves to the end), the label inside the gp1 / gp3 / enum / count blocks (each block then
goes out of line, distance 60-120), all failures goto one block (registers change, 150).

**D17705 (notes only). nw_buffer_pop 0x517820 (distance 8): scheduling of the tail** (binary: addr.ip loaded before
the `*size` store, `pop edi` early, the second address dword stored after the first_seq increment). Tried: a net_addr
local (before / after memcpy), field copies (and ip first), seq / first_seq order, `*size = slot->size` direct,
a source pointer, `*size` after the address, chained size, memcpy of the address (before / after), `first_seq =
first_seq + 1`, `++`, a from local, unsigned i, len / slot at the top, seq declared after i, the increment through a
local. None exact; 8966 scheduling.

**D17706 (notes only). vsdk/gr_glide.cpp glide_record::alloc 0x549b50 (distance 4): the final store order** (the
binary stores next, addr, size, texture, section in source order; 8966 stores addr first and section before
texture). Tried: addr / next swapped, section / texture swapped, both, stores through blocks[index] (worse, 43), a
return local, a second block pointer for the tail, next / addr / index declared at the top in three orders. None.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/game/ai.cpp (pick_attack_slot, claim_attack_slot; comments with D17700 / D17701)
- src/vsdk/unnamed_gr_poly_0x5450c0.cpp (gr_poly_clip_edge; comment with D17702)
- src/game/unnamed_dialogue_box_0x4d6660.cpp (last_visible_line; comment with D17703)

## --check results (VC6_STRICT=1 vc6-sweep --check src/decomp/vc6-matches.txt)

- game/ai.cpp: 84 matched there, 86 now, 0 lost
- unnamed_gr_poly_0x5450c0: 22 matched there, 23 now, 0 lost
- unnamed_dialogue_box_0x4d6660: 27 matched there, 28 now, 0 lost

## Worker 7

### m-1021 worker 7 notes (D17800-D17899)

Batch: quaternion::multiply, quaternion::to_matrix_b, fast_sqrt_refined, fast_sqrt, ui_gadget::set_parent,
buysell_count_party_items, TextFile::ParseUInt. Variants were tried with a small in-place runner (try/run.mjs,
which restores the unit after every run); all five units are byte-identical to REPO's copies afterwards.

## New matches

None.

## Closer, kept

None.

## Still differs (tried; do not repeat)

**D17800 (notes only). fast_sqrt 0x542590 / fast_sqrt_refined 0x542540 (distance 8).** The binary loads x into
ECX, copies it to EAX for the exponent and ORs the table word into EAX; 8966 loads EAX, copies to ECX and ORs into
EDX. Ten more forms (on top of the 30+ of D8405 / D14606 and every #pragma optimize letter): index local first;
a bits local plus index local with `*(float *)&b` return; exponent local; table word local then `|=`; exponent
store then `|=` from a copy; pointer into the table; signed int bits (gives sar, worse); shift-before-mask exponent
(worse); result local; `(x & 0xffc000) >> 14` index. All leave the same registers. Register choice; stop.

**D17801 (notes only). quaternion::multiply 0x522930 (distance 2).** Only r.w's fstp, which the binary schedules
after `push edx` (&r); ours stores it before r.z. Nine more forms (beyond all 24 store orders and 6 forms earlier):
a w local stored after x / z; the w local before `quaternion r` (worse, 38: the product moves above the default
constructor); `ww = w1 * w2` local; `-(dot - w1 * w2)` (fchs, worse); `*(vector3 *)&r.x = v` (calls operator=,
worse); a comma-chained statement; a const reference to r for the copy; a pointer to r; `return r`. Scheduling;
stop.

**D17802 (notes only). ui_gadget::set_parent 0x52c570 (distance 4).** The else branch's old head is in ECX, the
binary's in EDX. Twelve more forms: a static inline / __forceinline push-front helper taking (win, g) or (g, win)
for the else branch; one helper `(ui_gadget **head, g)` for both branches (worse, `add eax, 0x4c4`); separate
head locals with an early return; the cast written twice instead of a win local; the branches inverted (worse);
a second `wfirst` local; testing `this->parent`; win declared at the top (both declaration orders); a comma chain;
the head read through the gadgets sentinel's end. Register choice; stop.

**D17803 (notes only). buysell_count_party_items 0x4050f0 (distance 7).** The max-0 AND (`and eax, ecx` in the
binary, `and ecx, eax` in ours). Seventeen more forms. D10600 / D11802's in-place clamp on the global
(`buysell_scroll_pos &= (buysell_scroll_pos <= 0) - 1`) does give the binary's `and eax, ecx`, but with the min
written into the global first (as min(), as four ternary orientations, as an if) 8966 then loads scroll_pos into
EAX before the page-size load and compares the other way round (distance 13-17). Through a local (`pos = limit;
if (scroll < limit) pos = scroll;`, min / ternary into pos, then `scroll = pos; scroll &= ...`) copy propagation
gives back the base code (7). Also tried: `pos &= mask` on the local, the mask written first, nested max(min()),
no limit local (worse), `-(g > 0)` (gives sets path, worse). Not found; stop.

**D17804 (notes only). TextFile::ParseUInt 0x516380 (distance 12).** The only difference is digits in EBP, the
binary's in EBX (the earlier notes' EDI / ESI `this` swap is ParseInt / ParseString). Eleven forms: both
declaration orders, unsigned digits, `register` on digits, value or both, int value (worse), declarations
separate from the initialisations (both orders), no `char c` local (reads of *cursor), int c (worse), c through a
pointer copy. Register choice; stop.

**D17805 (notes only). quaternion::to_matrix_b 0x522cc0 (distance 8).** The binary loads w first in `w * z` and
`w * x`; 8966 emits z then w and x then w whatever the source order. Four more factor swaps (wz, wx, xy, yz) give
identical code, on top of the 25 orders of D15800-era notes and the 24 local-copy orders. Note: the ordering is
not by member offset or first use (w * y and z * x come out higher offset first, x * y and z * w lower first), so
it looks like 8966's own operand canonicalisation. Stop.

## Behaviour errors / bug candidates

None found. (ParseUInt's strchr-on-NUL defect is already documented in its comment.)

## Header changes

None.

## Changed files

None. No unit was edited, so no --check run was needed (all five units compare byte-identical to REPO's
src/decomp copies).

## Worker 8

### W8 notes (m-1021, decisions D17900-D17999)

## New matches

None.

## Closer, kept

None. No source file was changed.

## Still differs (tried; do not repeat)

**D17900 (game/summoner_cd.cpp; notes). cd_set_root (0x43e8e0), distance 3: not a source difference.** The only
difference is the shipped no-CD patch (D2600): the binary's `mov al, '.'` / nop sits over `add eax, 0x41`. No
compiler emits it, so a byte match needs the patch applied again after the build. Nothing tried. Do not retry.

**D17901 (game/level_scripts_common.cpp; notes). party_set_level (0x4cec00), distance 2.** Only the skill copy's
`mov dl, [eax + ecx + 0x93]` is left: the binary puts the counter first, ours puts the char_info pointer first.
19 more forms, none changes the order: `i[c->info->skill]`, `*(i + c->info->skill)`, byte-pointer and int casts
(`(ubyte *)c->info + i + 0x94`, `(int)c->info + i + 0x94`, `(int)c->info->skill + i`, `(ubyte *)i + (int)...`),
`i[c->skill]` on the left and on both sides, a `char_info *ci = c->info` in the loop body (with `i[ci->skill]`
too), `j` as the counter, a block-scope `k`, a `while` loop, `skill[i++]`, a destination pointer, a source
pointer, a `ubyte` temporary. `do/while` and an unsigned counter make it worse (distance 4). 8966 seems to fix
this operand order itself. Exhausted.

**D17902 (vsdk/unnamed_vfx_emitter_0x55ff00.cpp; notes). fx_recb0::spawn (0x55ff30), distance 2.** The prev
store comes after the fstp of the ceil argument in the binary. Seven more forms: ceil before the prev copy (and
the same with an int copy) gives distance 8. The comma form, `*(int *)&prev = *(int *)&last_t`, no `(int)` cast
on ceil, and `this->last_t` all stay at 2. A float local for last_t gives 102. Instruction scheduling. Exhausted.

**D17903 (same file; notes). fx_recb0::get_fade (0x5626b0), distance 8.** Only the prologue differs. The binary
runs `sub esp, 8` before `fld age / fdiv life`. The 8 bytes are the qword temporary of the unsigned alpha
conversion. Six more forms, all unchanged or worse: a named `__int64` for alpha, an `unsigned int` local, a
`(double)` cast, a `t0 = age / life` local, `/=`, and get_fixed12_90 called before the division (worse, 12).
Prologue scheduling. Exhausted.

**D17904 (vsdk/unnamed_http_0x55a8e0.cpp; notes). http_async_gethostbyname (0x55b310), distance 4.** The binary
stores `l->ip` (ebx) before `push offset http_resolve_host`. Ours pushes first. Ten more forms, none moves it:
`&http_resolve_host`, a function-pointer local, an explicit function-pointer cast, `(void *)l`, the global as
the argument, `0u` and `(unsigned)0` for the stack size, `l->ip = 0u`, host stored before ip, and the call inside
an `if`. Scheduling. Exhausted.

**D17905 (game/boss_ai.cpp; notes). boss_threat_add (0x4a13b0), distance 8.** Both fill blocks differ only in
where `test edi, edi` goes (the binary puts it after the threat[1] store) and in where one register copy goes.
Earlier rounds tried 24 store orders, chained assignment, an inline fill helper, `== 0`, and a float local.
Nothing new was tried here. Exhausted.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

None. All variants were applied by a script and the files were restored afterwards. The five units are the same
as REPO's src/decomp.

## --check result

No unit was edited, so there is nothing to check. Nothing was lost.

## Worker 9

### m-1021 worker 9 (D18000 to D18099)

## New matches

- game/conversation.cpp 0x4d7e10 conversation::clear_speaker, **D18000**: the call before the two stores is
  `speaker->release();`, a new empty non-virtual member of conversation_speaker (thiscall, so ECX = speaker, as the
  binary loads it), defined empty in conversation.cpp after clear_speaker; the linker folded it into empty_function
  (0x4f13e0). This is the declaration D10904 (1) asked for. Same behaviour (an empty call).

**D18000 (game/conversation.cpp, include/levelscripts/conversation.h). conversation::clear_speaker 0x4d7e10 is
exact.** It loads `speaker` into ECX before `call empty_function`: a thiscall member of the speaker with an empty
body that identical-code folding merged into empty_function (as for the ai_state think hooks, D4411 and similar).
Declared as `void release();` in conversation_speaker (the name is ours), defined `void conversation_speaker::release() {}`
in conversation.cpp. The call target has no address map entry, so it is counted as an unchecked relocation.

**D18001 (notes only). Tried without a match this round** (add these to the lists of earlier rounds; do not repeat):
- pf_read_edges 0x49c970 (distance 2, `lea ebp, [eax]` against `mov ebp, eax`): 16 more loop forms. `for (unsigned i
  = 0; i < (unsigned)*count; i++)`, `++i != n` and `++i < n` do/while, `for (n = 0; n != *count; n++)`, `for (n =
  *count; n; n--)` / `n != 0; --n`, `for (i = 0; i != n; i++)`, `for (i = n; i != 0; i--)`, `for (i = 0; i < n;
  i++)`: all either drop the zero register or add an entry test. `while (n--)` / `while (n-- != 0)` copy and
  decrement up front. `while (--n > 0)`, `while (n-- != 1)`, an unsigned counter, an unused end pointer and a split
  `pf_edge_count++` keep `mov ebp, eax`.
- mp_handle_ai_disable 0x480f60 (distance 4, the forward's `from` in ECX instead of EAX): `if (c) { ... }` block, the
  ushort at function scope, an obj * local tested, a copy of `from`, `goto done`, a bool host local: all
  identical; an `ai_state *ai` local is worse (27).
- mp_handle_status_add 0x480d70 (distance 10, value in ECX/EDX and c's reload placement): three declaration orders,
  a Character * copy at the call, a float pointer to the value: identical; the conversion in the argument list (12),
  `source = mp_local_handle_from_net(data[5])` (14) and memcpy of the value (18) are worse.
- gr_line 0x507310 (distance 8, the integer copy of clip_top / clip_bottom scheduled one instruction earlier): a
  `t` copy of clip_top in the block, a dy local, a numerator local, a comma expression, the clip locals declared then
  assigned: identical; storing y1 before the x1 computation (old y1 saved) is worse (26). Scheduling; stop.
- wav_open 0x5670a0 (distance 4, the imul operand load order of nBlockAlign): not retried; earlier rounds list the
  operand swap, int casts, pointer locals, parenthesised forms (D12503 and later). Exhausted.

## Closer, kept

None.

## Still differs (tried; do not repeat)

See D18001 above: pf_read_edges, mp_handle_ai_disable, mp_handle_status_add, gr_line (register choice or
scheduling of 8966), wav_open (not retried, exhausted by earlier rounds).

## Behaviour errors / bug candidates

None found.

## Header changes

- include/levelscripts/conversation.h: one member function declaration added to conversation_speaker,
  `void release();` (empty; folded into empty_function 0x4f13e0; name ours, D18000). Non-virtual, so no layout
  change (LAYOUT_CHECK 0x1c still holds); needed because the binary passes the speaker in ECX (thiscall).

## Changed files

- src/game/conversation.cpp (clear_speaker calls speaker->release(); the empty definition after it)
- src/include/levelscripts/conversation.h (the declaration)

## --check result

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "game/conversation" --check src/decomp/vc6-matches.txt`:
2 units (conversation, conversationctl): 47 match, 7 differ, 0 errors; 46 matched there, 47 now, **0 lost**.
The other four units of the batch (unnamed_mp_objects_0x47f860, pathfinding, gr, unnamed_sndstream_0x566560) are
unchanged (all variants were applied and restored by a script).

## Worker 10

### m-1021 worker 10 (D18100-D18101)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/unnamed_camera_0x4778b0.cpp | 0x479930 | camera::set_mode | D18100 | `high_view_requested = 0` written through a pointer local (`ubyte *hv = &high_view_requested; *hv = 0;`) |
| vsdk/unnamed_ds_0x55bf90.cpp | 0x55bfc0 | ds_is_3d_buffer | D18101 | the if-form `if (caps.dwFlags & DSBCAPS_CTRL3D) return true; return false;` inside the GetCaps test |

**D18100 (game/unnamed_camera_0x4778b0.cpp; m-1021 worker 10). camera::set_mode 0x479930 is exact** with the
clearing store of high_view_requested made through a pointer local: `ubyte *hv = &high_view_requested; *hv = 0;`.
8966 then cannot keep mode in a register across that store (a store through a ubyte pointer may alias it), so the
first test is `cmp [esi], edx` and mode is reloaded at the join (`mov ecx, [esi]`), as in the binary. The direct
member store, volatile reads, operand orders, a `*(char *)&` cast on both stores (D13105 and this round) all keep mode in ECX. Same behaviour. The device (a store through a pointer local to kill a CSE the binary
does not have) may help other "binary reloads, ours keeps it in a register" cases.

**D18101 (vsdk/unnamed_ds_0x55bf90.cpp; m-1021 worker 10). ds_is_3d_buffer 0x55bfc0 is exact** in the if-form
of game_is_paused (D14500) and Player::is_leader (D12400): `if (GetCaps(&caps) >= 0) { if (caps.dwFlags &
DSBCAPS_CTRL3D) return true; return false; } return false;` gives the dword load and `shr al, 4 / and al, 1`.
Every expression form (`(f >> 4) & 1`, casts, `!!`, `!= 0`, ternaries, char locals, `#pragma optimize` variants)
gives the 32-bit shift. The if-form with `< 0` first, or one `&&` condition, does not match. Same behaviour.

## Closer, kept

None.

## Still differs (tried; do not repeat)

- **xp_level_up_gains 0x489ac0 (distance 2)**: the binary loads species into EDI before `mov al, [esi+5]` (skill_cap).
  Tried this round (all distance 2 except one): group pointer before / after the points store, `register` group
  pointer, int species copy (early and late), cap via a local, cap via a pointer local, comma-chained stores,
  `(unsigned)` cast, the random flag in a local, `xp_level_table + i`, `(lv->groups + species)->`, `!= 0`, the
  stores from `xp_level_table[i]` without lv (worse, 102). Plus D11403 / D15804. Pure scheduling; stop.
- **ix_point_in_polygon 0x53b6f0 (distance 6)**: the binary keeps alpha on the FPU (`fld beta; fadd st, st(1)`,
  `fstp st(0)` at both exits). Tried: ternary, beta / alpha declaration swap, `register float alpha`, an if that sets
  inside and breaks, the test duplicated in both branches (tail-merged to the same code), `alpha = alpha` after,
  `inside = false; if (alpha >= 0) inside = ...`, `1.0f + 0.0f`, a float cast of the sum, a negated form (worse).
  Plus D11602 / D15306-era forms. Stop.
- **ix_ray_quad 0x53b130 (distance 10)**: the binary loads w and pushes float_min / float_max's constants before
  `v + u`, and `mov al, 1` before the last store. Tried: the assignment inside the float_max argument, the
  expression inside the argument (worse), a mean local m, the mean into v or into u (u: worse), `*w = u = ...`, a w
  pointer local, a comma form. Plus D15904's 13. Scheduling; stop.
- **spline::eval 0x558f40 (distance 8)**: the pops of EDI / ESI before the y sum's last add. Tried: z or y line with
  `points[i + k]`, `(p + k)->`, b0..b3 declared in reverse, the weights declared before u, an extra pointer local
  declared first. Plus D15303's forms. Scheduling; stop.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/game/unnamed_camera_0x4778b0.cpp (camera::set_mode, D18100)
- src/vsdk/unnamed_ds_0x55bf90.cpp (ds_is_3d_buffer, D18101)

## --check results (VC6_STRICT=1 vc6-sweep --check src/decomp/vc6-matches.txt)

- unnamed_camera_0x4778b0: 45 matched there, 46 now, 0 lost
- unnamed_ds_0x55bf90: 56 matched there, 57 now, 0 lost
