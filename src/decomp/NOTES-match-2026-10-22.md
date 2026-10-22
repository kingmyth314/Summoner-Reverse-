# Byte-match round 2026-10-22 (sixth targeted): the ten workers' notes

The 138 functions within distance 30 on the corrected --score (EH funclets no longer counted), plus every differing
function of the unoptimised units, in ten batches of whole files. Result: 11 new matches (strict 5,602 -> 5,613,
none lost), 10 closer. Four of the matches are unoptimised functions in game/living_entity.cpp, found with the
corrected name hash (D25200). Every closer change and the two header changes were read for behaviour by two review
agents before the commit: all equivalent. The decisions are D25300-D26299 in include/DECISIONS.md.

## Worker 1

### m-1022 worker 1 (D25300-D25399)

Batch: 14 functions in 7 units (render scene, lights, vif, fx, conversationctl, d3d tcache, vfx mesh).
Helper: `v/tryd.mjs` (vc6-try with the --score distance; `--align` / `--full` print the aligned listing of each
variant). Variant files are in `v/`.

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/unnamed_render_scene_0x481680.cpp | 0x484ae0 | levelrender_draw_speech_text | D25300 | the range tests as three separate `return`s inside `if (c->type != 7 && _stricmp(...))`, the type-7 dead test in the `else if` |

**D25300 (game/unnamed_render_scene_0x481680.cpp). levelrender_draw_speech_text 0x484ae0 is exact.**
```
if (c->type != 7 && _stricmp(c->name, "Ulsadana")) {
    if (fabsf(cur_character->pos.y - c->pos.y) > 4.0f) return;
    if (distance_squared(&cur_character->pos, &c->pos) > 2500.0f) return;
    if (!(c->obj_flags & 2)) return;
} else if (c->type == 7 && (c->flags & CF_DEAD))
    return;
```
replaces D22601's `bool always` form. The binary lays out the range tests first and the dead test after them,
with `je` to the shared return and a `jmp` over the dead test. The same branch with the three tests joined by
`||` puts the shared tail inline instead (14), and so do a bool for the joined test, an empty then-block and a
positive `else if (!(pass))` (14 each). Folding everything into one condition,
`if (!(type == 7 || !_stricmp || (pass))) return; if (type == 7 && dead) return;`, gives 1: the last `jmp`
over the dead test is missing. Same behaviour: a speaker who is neither type 7 nor Ulsadana must pass the three
range tests, and a type-7 speaker must not be dead.

## Closer, kept

| file | address | name | old -> new | D |
|---|---|---|---|---|
| game/unnamed_render_scene_0x481680.cpp | 0x483570 | levelrender_add_items | 6 -> 2 | D25301 |
| game/unnamed_render_scene_0x481680.cpp | 0x4836f0 | levelrender_add_containers | 6 -> 2 | D25301 |

**D25301 (game/unnamed_render_scene_0x481680.cpp; closer only). levelrender_add_items 0x483570 and
levelrender_add_containers 0x4836f0: the depth as `fvec.y * d.y + (fvec.x * d.x + fvec.z * d.z)`.** That form
gives the binary's z, x, y product order: fvec+32, then +24, then +28, all relocation targets checked. The old
`z*z + x*x + y*y` compiled as z, y, x. Of all 24 term orders (with and without the parentheses, either operand
order), only y + (x + z) and y + (z + x) give 2. `d.dot(fvec)` and `fvec.dot(d)` stay at 6. The instructions
are now the binary's, so the x87 rounding is the binary's too. Left (2): the 400 compare uses a QWORD constant
(D19402's double copies make the sum double); the binary uses the float constant 0x58192c, the shared
`__real@4@4007c8...` that seven other functions also use. Tried for the constant: `400`, `400.0`,
`20.0f * 20.0f`, a `float lim` local (all 2), a `static const float` (13: fld + fcompp), `(float)` on each half
(8), float copies with `(double)` casts (4).

## Still differs (tried; do not repeat)

- **levelrender_push_attachment 0x483430 (2)**, adding to the 21 forms of earlier rounds. With the radius read
  directly in the dist expression (no local): 17. With `e->depth = d.dot(...)` and `e->dist = e->depth - radius`:
  9. With the dot written out in any of the 12 term orders, with or without parentheses: 72-73, because the
  inline budget is spent differently and the matrix rows expand. The radius store is scheduled early; I stopped.
- **levelrender_add_effect_entry 0x4841e0 (4)**: only the `pop esi` placement is left (the align is otherwise
  identical). Not retried; 49 store orders were tried earlier.
- **level_vis_reset 0x487030 (12)**: 27 new forms, none closer. `v->count = level_num_vis_sets` in the pointer
  loop gives 12. The same in the index loop gives 17: 1 goes into EDX and the global is stored from it. Int and
  unsigned index loops with the three record stores in 3 orders and `level_num_vis_sets = 1` in 4 places
  (before the memset, after it, after the list store, after the loop) give 12-19. The unsigned index has the
  binary's jb and bias, but it CSEs the constant 1. D7807 (an 8168 match) stands.
- **gr_env_map_uv 0x51f960 (2), lights_calc_point_specular_color 0x51f560 (4)**: not retried. D17501 and
  D19605 list more than 30 forms for the scheduling of the `2 * normal` temporary's lea.
- **vif_emitter_update 0x532f40 (8)**: not retried. The x87 `interval - remaining` (binary `fsubr` from memory)
  is 8966 keeping `remaining` as an FPU register variable; 21e lists the forms.
- **vif_record_get_bounds 0x533aa0 (16)**: not retried (D19103 and 21d are exhaustive).
- **fx_create_spell_effect_facing 0x48c8a0 (4)**: not retried. D17303 and D20002 list 35 forms; it is the same
  difference as blood_splot_place (D9301).
- **conversation_controller::camera_blend 0x4d9ad0 (8)**: not retried. D23900 lists the forms for the eased
  t's fstp scheduling.
- **gr_d3d_create_texture_section 0x54bca0 (15)**: 24 new forms: each of w / tex_w and h / tex_h as the loaded
  variable with the other copied from it, in every statement order. The tex_w / tex_h-loaded forms stay at 15;
  the others give 47-140. The frame slots of w and h stay swapped against the binary's.
- **fx_rec98::update_frame 0x563a40 (22)**: not retried. Its frame slot packing (u against a0 / b0, and the
  uv product temporary) is covered by D23600's list.

## Behaviour errors / bug candidates

None found. D25301 makes the depth's x87 operation order the binary's. The old source summed the terms in a
different order from the binary.

## Header changes

None.

## Changed files

- src/game/unnamed_render_scene_0x481680.cpp (levelrender_draw_speech_text D25300; levelrender_add_items,
  levelrender_add_containers D25301)

## --check

- unnamed_render_scene_0x481680: `VC6_STRICT=1 vc6-sweep --only unnamed_render_scene_0x481680 --check
  src/decomp/vc6-matches.txt`: 65 matched there, 66 now, 0 lost.

## Worker 2

### Round 6, worker W2 (D25400-D25499)

Batch: 14 functions (level_scripts.cpp six, summoner_cd.cpp two, goals_doors two, pfgeom, bezier, gr_opengl,
cutscene script). Distances are vc6-compare --score at the start of the round, all as in batch.txt (cutscene
script load 25).

Variants were tried with a helper (`vt.mjs`, `va.mjs` in W) that writes each variant into `try/<unit>` in W and
runs vc6-compare --score / --align on it; src/ was never edited by the helpers.

## New matches

None.

## Closer, kept

None. (One closer form was found and not kept; see D25400.)

## Still differs (tried; do not repeat)

**D25400 (notes only). matrix3_pack_orientation 0x475380 (20).** `a` alone in a block of its own
(`{ vector2 a = ...; out->r0 = a.x; out->r1 = a.y; }` and then b at function scope) is the first form that puts
`a` in the dead out / m argument slots: the frame becomes the binary's 8 bytes (distance 14). But 8966 then stores
a into out before copying b and folds b's reload away (50 instructions against the binary's 52). That is the same
shape problem as the earlier "b declared before a" 14, so it was not kept. Also tried (all 20 or worse): a and b
both in one block (20), a block holding a with b nested inside (20), b in a block after a function-scope a (20),
a block for a and b with the u stores in an inner block (20), a function-scope b assigned inside a's block with
the u stores inside or after the block (25), a copied through a block-scope temporary into a function-scope a
(25), `*(vector2 *)&out->r0 = a;` for one or both pairs (20), `const vector2 &a = ...` (10, but the copy is gone:
not the binary's shape). Finding for others: a block-scope local whose block ends before the next copy can
take dead argument slots under 8966 (as D7004's block-scope copy did).

**D25401 (notes only). level_add_navpoint 0x4d5720 (4).** The z fadd operand order. New forms, none changes it:
ray.start.z stored first (12), start.z before start.y (4), `ray.start = vector3(...)` (157), ray.dir stored before
start (6), pos stored before layer (14), hit_pos in the block (4), the sum stored straight into navpoints[n].pos
(46), and file-local inline helpers `ray_point(ray, t)` by reference, by pointer, with (start, dir, t), and with
explicit components (48-71). Exhausted with D17405 / D20003.

**D25402 (notes only). door_find_floor_point 0x476ab0 (16).** Besides the mid fmul placement (D23402), the
final `ray.start + ray.dir * hit.t` loads hit.t first in each product, where the binary loads dir first (the
binary's order is what level_add_navpoint gets from the same expression). Tried, no change (16-18): hit declared
before ray, o declared after ray / hit, ray / hit at function scope in both orders (18), a vector3 local for the
sum, a `vector3 *` to floor_point, a `float t = hit.t` local, `ray.dir * hit.t + ray.start`, a product local; the
sum through `point` (93).

**D25403 (notes only). savegame::hot_button_set 0x4d16a0 (6).** The store SIB order. New forms, all 6:
`data[off++] = ...` for the stores, for stores and loads, `= true` for the used byte, `*(data + off) = ...` for
the byte stores. Exhausted with D17406 / D20004 / D12303 / D14706.

**D25404 (notes only). savegame::read_two_stage 0x4d34e0 (28).** Looked at the down-counter shape again
(`for (i = count - 1; i >= 0; i--)`): 8966 fuses `dec / js` where the binary has `dec; mov [i]; test; jl` at the
top, and moves `this` to EBX / hash to EDI, as D22301 says. No new form tried beyond that one; register choice.

**Not retried** (read the earlier notes; their lists cover the plausible forms and the round 5 devices do not
apply to what is left):
- seg2d_point_near_middle 0x4a6530 (4): the perp product's operand order (D17500, D19902: 50+ forms).
- spline::eval 0x558f40 (8): only the pop placement of EDI / ESI; the frame slots are the same (the binary's
  later [esp+4] / [esp+8] are b1 / b0 after the pops). Scheduling.
- gr_opengl_bitmap 0x54fa30 (16): the first corner's store schedule (21b / 21d lists).
- level_format_display_name 0x4d63b0 (16), savegame::get_speaker_bit 0x4d05f0 (18), savegame::get_int 0x4d0030
  (22): register choice (D18303, D18305, 21b / 21d lists). get_int's twin set_int matches with the same source.
- cutscene_script_load 0x4ed0f0 (25): the shared failure tail (21d / 21e lists cover if / else, goto, do-while,
  for, bool and int result forms).
- cd_find_drive 0x43e660 (3), cd_set_root 0x43e8e0 (3): not source differences; the shipped no-CD patch bytes
  (D2600, D17900, D19901, D19905).

## Behaviour errors / bug candidates

None found. (The known game bug in matrix3_pack_orientation, the up pair copied from rvec, is unchanged.)

## Header changes

None.

## Changed files

None. W/src is identical to the repository's src/decomp for every unit of the batch.

## --check results

No unit was edited. For the one unit edited and reverted during the round:
`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "unnamed_goals_doors_0x475180" --check
src/decomp/vc6-matches.txt`: 63 matched there, 63 now, 0 lost (run with the D25400 form in place; the file was
then restored to the repository's text, checked with cmp).

## Worker 3

### m-1022 worker 3 (D25500-D25599)

Batch: 14 functions (6 in game/living_entity.cpp, the unoptimised `#pragma optimize("", off)` part, and 8 short
optimised ones in other units). Result: 4 new matches, 1 closer, 9 unchanged. All four matches are /Od functions
that were at distance 504 to 2335.

## New matches

| File | Address | Name | D-number | What did it |
|---|---|---|---|---|
| game/living_entity.cpp | 0x4530d0 | Character::update_head_look | D25500 | locals named for the hash order; four unused locals for the binary's empty slots; `ray.dir.set(...)`; `vector3 v = foot - head;` with `v.dot(orient.fvec)`; the ray's +0x28 not written (behaviour fix, below) |
| game/living_entity.cpp | 0x45cd00 | Character::move_frame | D25501 | locals in the blocks that use them; one unused vector3; `pos + move`; the length tested without a local |
| game/living_entity.cpp | 0x45c340 | Character::ai_move_frame | D25502 | names for the hash order; `quaternion q = ...take_turn_delta()`; `vector3 t = move + centre_offset;` in its block; `t.dot(*plane_normal)`; `pos + delta_v` |
| game/living_entity.cpp | 0x45dea0 | Character::get_hit_by_spell | D25504 | names for the function scope and every case block; `preloaded` an int (header); eight source corrections listed under D25504 |

**D25500 (game/living_entity.cpp). Character::update_head_look 0x4530d0, 504 -> match.**
- The binary's frame holds 20 locals, in this order from ebp-0x4c down: the ray, a 12-byte gap, foot, angle, dy,
  d, fwd, ch, a 12-byte gap, hit, a 48-byte gap, head_orient, len, head_pos, foot_orient, foot_pos, alt, sign, a
  12-byte gap, head. Nothing in the code reads the four gaps. They are unused locals: three vector3 (dir, pivot,
  norm) and one matrix43 (frame_m).
- The order is VC6's name hash (D25200). All 20 locals were renamed so that their buckets rise from the top of the
  frame to the bottom, and they are declared bottom first, so that the later declaration is higher within a
  bucket. The names were found with a small dynamic-programming search over candidate names, then checked with
  `vc6-localhash --probe`.
- `ray.dir.x = 0; ray.dir.y = -10; ray.dir.z = 0;` became `ray.dir.set(0.0f, -10.0f, 0.0f)` (twice). The code is
  the same (/Od expands set with direct stores). The two extra expansions change VC6's inline decisions elsewhere
  in the function. `last_orient = orient` then matches the binary: rows 0 and 1 expanded with an out-of-line copy
  constructor, and row 2's operator= out of line.
- `d` is constructed from the difference (`vector3 v = foot_point - head_point;`, no operator= and no copy), and
  `fwd = v.dot(orient.fvec)`, which sums the z term first as the binary does.
- Matching aid: the four unused locals are part of the match. Removing them, or renaming any local, loses it.

**D25501 (game/living_entity.cpp). Character::move_frame 0x45cd00, 525 -> match.**
- frac / m / step (and m2 / step2) are declared in the two lean blocks, and fx in the `move_fx != -1` block. VC6
  places block locals after the function scope's locals, block by block in source order. That gives the binary's
  slots, and it also gives the binary's inline expansions of the two `orient = m * orient` (C C A / C A: the copy
  constructor out of line, then row 2's operator=).
- An unused `vector3 delta` fills the binary's empty slot at -0x18.
- `pf_trace_edge_crossings(&pos, layer, pos + move)`: with this operand order the argument pointer is stored
  first, as in the binary. `move + pos` stores it through EAX.
- `if (move.len() > 0.1f)` has no len local. The value's temporary is the binary's -0x160, and the code is
  `fst` / `fst` as the binary has it.

**D25502 (game/living_entity.cpp). Character::ai_move_frame 0x45c340, 832 -> match.**
- The function-scope names were chosen for the hash order: p, mdist, q, delta_v, speed, b, reground, profile,
  ret, h, velo, from, seen_v. Two are declared mid-function: q and from.
- `quaternion q = instance->take_turn_delta(frametime);` (the binary has no copy of the turn).
- `vector3 t = move + centre_offset;` declared inside the plane-normal block, so that t's slot comes after the
  function scope's.
- `t.dot(*plane_normal)` in place of the written-out x + y + z (the binary sums z first).
- `pos + delta_v`, as in D25501.

**D25503 (game/living_entity.cpp; closer only). Character::clip_move_to_navmesh 0x45ab00, 992 -> 526.**
bone_target, step, bone_slid and dest are declared in the `if (bone_ok)` block. With this change, the copy
constructors of `target = ...` and `ahead = ...` go out of line, as in the binary. The remaining difference is
inline decisions only:
- the binary calls the vector3(x, y, z) constructor for fix, fixed, `check = fixed + pos` and `to = pos + *move`,
  and calls the last operator+; ours expands them;
- the binary expands ~profile_label; ours calls it.

The frame cannot be fixed while these differ.

**D25504 (game/living_entity.cpp, include/engine/objects/living_entity.h). Character::get_hit_by_spell 0x45dea0,
2335 -> match.** The changes, in source order:
1. The function-scope names were chosen for the hash order: die, wince, hit_chance, dist, count1, src_n, player,
   mag, amount, retaliate, millis, play_fx, status, use_resist, lvl, resistance, who.
2. `int preloaded` (the binary tests `cmp dword ptr [ebp+0x24], 0`). This is a header change; see below.
3. Source type 6: the chain factor `lvl = (int)(lvl * (chain * 0.15 + 1))` is written in each of the three
   branches. The binary repeats it in each branch.
4. Case 0x67: sub_spells[] and statuses[] are declared, with their initialisers, after `play_fx = false; any =
   false;`. The binary fills the arrays after those two stores.
5. Cases 0x4e: `int stun = random_stun_duration(); ai.knock_down(stun);` in both branches. The binary stores the
   result in a block local before the push.
6. The `spell == 0x21` block has its own `int duration` (the binary's -0x130). Before, it wrote the function-scope
   ms, which nothing reads afterwards.
7. Case 3/5/0x69: two file-local inline helpers `ghs_assign_max/min(float &x, const float &a, float b)` replace
   the cur / max_hp locals. Their by-value parameter b is the inline expansion's temporary, which is the binary's
   -0x198 / -0x19c, below the other temporaries. get_max_stat is called once.
8. Case 0x11: `if ((gain = (float)drained) < 20.0f) gain = gain; else gain = 20.0f;` (the binary has `fst gain`
   in the test).
9. The jade case's range test is `distance(&src_n->pos, &this->pos)`. distance is symmetric, so this changes
   nothing.
10. Case 0x18: a `break;` at the end of the first branch. The binary jumps straight to the switch end.
11. The block locals of the case blocks were renamed or reordered for the hash order:
    - roll, duration, mod (declaration order);
    - mat for m;
    - case 0x67: idx, some, r, factor, subs, sub, length, status_list;
    - case 0x11: roll, leech, gain.

All 11 keep the behaviour; the reason for each is given in its item.

**Device (new, Observed in all four):**
- **Block-scope locals are also placed by the name hash.** The hash is of the plain name; the `$NNN` scope
  suffix in the listing is not part of it. Each block's locals come after the function scope's, block by block
  in source order.
- Locals declared mid-block count with that block. The later declaration is higher within a bucket.
- Inline-expansion parameters taken by value become temporaries, placed after all named locals.

The test was a /Od probe with a block of eight locals: the plain-name buckets were 0 1 2 2 8 9 10 10, top to
bottom.

**Inline decisions (Observed, not understood):** in the /Od code, the following change which inline functions
VC6 8966 expands elsewhere in the same function, even far from the change:
- extra inline expansions that produce the same code (`set()`);
- declaring locals in inner blocks;
- adding unused vector3 locals (this gave *more* expansion);
- removing a float local.

The changes are not monotonic. It is not a simple budget in source order. Comparing the sequence of
out-of-line vector3 calls (copy constructor / operator= / constructor / operator+ / operator-) with the
binary's, and trying such forms, found the match for three functions. It did not converge for
clip_move_to_navmesh.

## Closer, kept

- game/living_entity.cpp 0x45ab00 Character::clip_move_to_navmesh: 992 -> 526 (D25503).

## Still differs (tried; do not repeat)

**D25505 (notes only).**
- **ai_state::serpent_illusion_update 0x496d60 (4).** This is the -1000 divide. A standalone test file with 16
  forms shows that 8966 turns every `b + x / -1000` into `b - x / 1000`, including:
  - split statements and `a += b`;
  - `x * -b / 1000` and `-x * b / 1000`;
  - `b - -(...)` and `x / -1000 - -b`;
  - `(300 - 1300)`, `(int)-1000.0f`, `-(int)1000u`;
  - an inline divide helper, `(q = ...)`, an array element.

  A negative magic number appears only when the quotient is not added to anything (stored or returned). This
  confirms D20501; it is exhausted.
- **mp_handle_ai_disable 0x480f60 (4).** Tried, with no change: the switch as if / else (17), `(&c->ai)->`, a
  copy of `from` at the top, `if (!MP_IS_HOST()) return;`, and a `default: break;`. Register choice.
- **mp_handle_status_add 0x480d70 (10).** Not retried: the value's register (EAX, then ECX) and c's reload, as
  in D18001 and NOTES-match-2026-10-21c.
- **conversation::restore_state 0x4d84d0 (18).** Tried on top of D23200:
  - a `const char *tn = t->name` (82);
  - the plain calls with tn (82);
  - a `const char *sn = s->name` (103);
  - both (109).
- **ui_listbox::scroll_page_up 0x52e380 (20).** Tried:
  - a ternary chain (42);
  - an e->y local with `(line + 1) * fh + y` (32);
  - `if (vscroll != top) { ... } vscroll = top;` with `(line + 1) * fh + e->y` (42) and with `top += fh` (32);
  - the line-test cases swapped (38).
- **Not retried; the earlier notes list them as exhausted:**
  - Character::say 0x461dc0 (9; D21601, the strncat loop tail);
  - ls_ikaemostopint_init 0x4f9ec0 (11; the constant 1 in EBX, NOTES-match-2026-10-21c and earlier);
  - ui_listbox::set_vscroll_fraction 0x52ed70 (16; NOTES-match-2026-10-21e);
  - mp_handle_actv_gm 0x470860 (27; D25104).

## Behaviour errors / bug candidates

- **update_head_look (0x4530d0) wrote `ray._unk28 = 0` for both raycasts. The binary never writes ray +0x28.**
  The store is removed. The field stays as the ray's constructor left it. That makes no difference to the game:
  lobj_raycast_all reads +0x28 (stop_t) only with ray flag bit 2, and the flags are 0. NOTES-review-2026-10-20
  had noted this. It is fixed here as part of the match.
- No new game-bug candidates.

## Header changes

- include/engine/objects/living_entity.h: `Character::get_hit_by_spell(..., float power, int preloaded)` (was
  `bool preloaded`). This is essential for the match: the binary tests the argument as a dword (`cmp dword ptr
  [ebp+0x24], 0`) in 12 places, and a bool gives `and eax, 0xff; test`. The layout is unchanged.
- Every caller found (about 50 calls in 8 units) passes the constants true / false / 0, which push the same 0 / 1
  as before. The strict check of those 8 units shows 0 lost (306 = 306).
- The mangled name changes (`..._N@Z` -> `...H@Z`). A merger who also edits callers should keep int.

## Matching aids (for the fixed build and the merge)

These are needed for the matches, but they are not plausible original source. The fixed build may replace them
with plain C++, which would lose the match:
- the unused locals dir / frame_m / pivot / norm (D25500) and delta (D25501);
- `gain = gain;` (D25504 item 8);
- the ghs_assign_max / ghs_assign_min helpers (D25504 item 7).

Every chosen local name in the four functions is also part of its match: a rename or reordering loses it.

living_entity.cpp still compiles with /DFIXED_BUILD and with /DFIXED_BUILD /DEXPERIMENTAL_BUILD. The
get_hit_by_spell renames also apply inside its FIXED_BUILD G100 block.

## Changed files

- game/living_entity.cpp: update_head_look (D25500), clip_move_to_navmesh (D25503), ai_move_frame (D25502),
  move_frame (D25501), get_hit_by_spell (D25504), and the two ghs_assign_* helpers above 0x45dea0. Every
  function comment is updated.
- include/engine/objects/living_entity.h: the preloaded parameter.

## --check results

- living_entity: `VC6_STRICT=1 vc6-sweep --only living_entity --check`: 1 unit: 130 match, 0 wrong target,
  2 differ; 126 matched there, 130 now, **0 lost**.
- The units that call get_hit_by_spell (header change): boss_ai, combat, level_script_liangshan,
  level_script_wolong_caverns, spells, unnamed_fx_0x489c40, unnamed_phoenix_0x497ac0, unnamed_tiger_0x499e50.
  8 units: 306 match, 0 wrong target; 306 matched there, 306 now, **0 lost**.
- No other unit was changed. Every vc6-try / helper run restored its file; a cmp against REPO confirms it.

Helper scripts (in W/v): tryd.mjs (variants with distance), names*.mjs (search for names with rising hash
buckets), vote.mjs (maps our frame symbols to the binary's slots through a normalised LCS), ndiff / ndiffr.mjs
(diff with offsets / registers normalised), probe*.sh (out-of-line call sequence of a function).

## Worker 4

### W4 notes (round 6, block D25600-D25699)

Batch: 14 functions (vfx emitter x5, targautils x2, script editor x2, cmdline, packfile, luminar, ds, assess).
Distances below are from `vc6-compare --score` at the start of this round (funclets excluded).

## New matches

None.

## Closer, kept

- game/unnamed_script_editor_0x4de0b0.cpp 0x4e8cf0 script_editor_default_action_navpoint: 26 -> 17 (D25600).

**D25600 (game/unnamed_script_editor_0x4de0b0.cpp; closer only, 26 -> 17).
script_editor_default_action_navpoint 0x4e8cf0:** `found` is declared without an initialiser and set to NULL
inside the cutscene branch, after the cutscene-count return (`found = NULL;` before `if (navpoint_count > 0)`).
The behaviour is the same: found is only read in that branch, after this store. 8966 now gives the loop's found
its own register, EAX, as the binary does: the loop, its type test, the in-loop return and the loop-end return
are now byte-equal. What is left: the binary sets ESI to 0 at entry (push esi / xor esi, esi before is_latched)
and returns it with `mov eax, esi` on the two empty paths (count <= 0, loop not entered). Ours has no ESI and
does `xor eax, eax` before the count test. Forms that give the same 17: `found = 0`, keeping the `= NULL`
initialiser as well, `found = np` (at that point or as the initialiser), `found = np = NULL` with both left
uninitialised, `int i` declared in the for, found declared before np, `break` instead of the in-loop return.
Worse: `found = np` inside the count test (19), a `return found` inside the count test with `return NULL` or
`return np` after it (19), `found = NULL` inside the count test (19), `if (found) return found; return np;` (33),
found uninitialised with no store (44, reads the uninitialised slot). Forms from before this change, all 28: a
`return found` / `return np` inside the count test with found initialised at its declaration, `break` + `return np`.

## Still differs (tried; do not repeat)

**D25601 (notes). fx_recb0::spawn 0x55ff30 (2).** prev stored before the fstp of the ceil argument (binary
after). New: prev = last_t between the ceil and floor statements (4: the store sinks below the ceil call),
after both (72), floor before ceil (12), a `double lt` for the ceil argument (185), `float prev = last_t;`
declared at the point of use and `int first = (int)ceil(...)` declared there (both 2). The store cannot be put
between the fstp and the call from the source: a statement before the call is scheduled above the fstp, one
after it stays below the call. Exhausted.

**D25602 (notes). fx_recb0::init_particle 0x5601f0 (3).** `fiadd [ebx+0x20]` against the binary's `fild
[ebx+0x20]; faddp st(1), st` after the frand_range call (both shape tails). New forms:
- a float local `r` for the frand result (`(desc->frames + r)` or `(r + desc->frames)`), also block-scoped,
  `r += desc->frames`, the assignment inside the sum `(desc->frames + (r = frand_range(..)))`, and frand's
  result stored back into spread: all 5. These do give `fild [ebx+0x20]`, but 8966 keeps r on the FPU stack
  (`fadd st, st(1)` and a `fstp st(0)` after the store) instead of the binary's faddp;
- `r = desc->frames + frand_range(..)` (3), `desc->rate * desc->frames` (3), `-1.0f * spread` (7);
- `(double)desc->frames +`, `+ (double)frand_range(..)`, the whole sum in `(float)(...)` with a double term,
  `(float)(frames + frand)`: all keep fiadd;
- a float local for desc->frames (91), `life = frames; life += frand` (91), an int frames local (111).
Exhausted.

**D25603 (notes). fx_recb0::move_particles 0x560a00 (6).** Two places: spawn's prev store (D25601) and, in the
second force block, `test eax, ebx` (ours, EBX still holding 0x7fffffff from the num_links test) where the
binary uses the immediate, plus the fstp / `mov ecx, [eax]` order in that block. New: the first block as an
if / else around the rest (6), the test inverted with blocks 2-4 first (6), the first block's `return` as
`goto done` to the end (6), the frame as an `int f0` local (157). Exhausted.

**D25604 (notes). fx_recb0::get_fade 0x5626b0 (8).** `sub esp, 8` after the first fld (binary before). New:
without `inline` (8, just to check: the code is the same as a non-COMDAT function), `float s` for the size (8).
Exhausted.

**D25605 (notes). fx_recb0::render 0x561ec0 (18).** Not retried beyond the earlier lists (age / alpha slot swap,
this / mat reload order after gr_poly; 120 declaration orders and renames already tried).

**D25606 (notes). pcx_read_bitmap 0x555f60 (2).** `xor esi, esi` (pos) between `lea ecx, f` and the call in the
binary, after the call in ours. New: `int pos = 0` before `count = 0` and after it, both before the read (6:
ESI pushed for the zeros), pos set before the seek (6), the read called with `count, count` as its zeros (2).
Exhausted.

**D25607 (notes). m2v_skip_extension_and_user_data 0x5549c0 (11).** New: next_start_code at the top of the loop
body instead of at the end of each branch (60). With D20900's lists, exhausted.

**D25608 (notes). script_write_level_script 0x4e9310 (8).** Not retried: the two door-sound `sub esp, 8` /
fstp orders, with D14905's seven casts / locals and D22200's ten forms.

**D25609 (notes). cmdline_take_argument 0x53e250 (2).** Not retried: is_done stored inside the inline strcpy
(after scasb, before movsd), which no source order reaches (D17902-era lists and D19903's).

**D25610 (notes). packfile_read_directory 0x533f90 (4).** p in sector's dead argument slot, the binary in pf's.
New: renaming p (q, ptr, src, dir, s, cur, entry, d, rec, data, bp, sp), sector (buf, data, dir, s, b) and pf
(pack, p2, f, packfile): all 4. The slot choice does not depend on the names at /Ox. Exhausted.

**D25611 (notes). ai_state::luminar_boss_load 0x4c8dc0 (11).** Not retried. The second length's FPU order: the
binary squares the preloaded swing.z after swing.x * swing.x (`fld st(2); fmul st, st(3)`, then `fstp st(2)`),
8966 right after the first fsqrt. D19906 and earlier rounds tried 44 forms.

**D25612 (notes). ds_stream_open 0x55d400 (18).** Not retried (three separate `or eax, -1` epilogues in the
binary, cross-jumped by 8966; D21903 / D24302 lists).

**D25613 (notes). assess_format_info 0x4013f0 (28).** The binary has protection in EBP and damage in the target
argument slot (loaded into EAX in each branch, stored at the join); ours the reverse. New, on top of D24203:
a `resistance_set *rs` for the copy (the binary's `mov ebx, [ebx+0x71c]; add ebx, 0xf8; mov esi, ebx` looks
like one, but 8966 still uses ESI directly), `int *pp` for the protection store, `int *pd` for the damage store
(all 28); renaming damage (8 names) and protection (6 names): all 28. Without D24203: plain if / else (43, both
polarities), a ushort local (44), `goto` to the join (43), `int *pd` stores (43), the ternary (worse: the xor is
hoisted above the test). Register priority; exhausted.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/game/unnamed_script_editor_0x4de0b0.cpp (script_editor_default_action_navpoint, D25600)

## --check results

- `VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "unnamed_script_editor_0x4de0b0" --check
  src/decomp/vc6-matches.txt`: 1 unit, 109 match, 11 differ; 109 matched there, 109 now, 0 lost.

The other batch units were only tried through temporary variants (each restored; byte-identical to REPO).

## Worker 5

### Worker 5, round 6 (2026-10-22), block D25700-D25799

Batch: 14 functions (combat.cpp x4, floatmath x3, gr.cpp x2, anim_track, gadget, ai_move, gr_glide, new_dialogue).
Result: 2 new matches, 1 closer, 11 unchanged.

## New matches

| File | Address | Function | Decision | What did it |
|---|---|---|---|---|
| vsdk/unnamed_gadget_0x52bfe0.cpp | 0x52c570 | ui_gadget::set_parent | D25700 | `next_sibling = head;` straight from the old head in both branches, no temporary |
| vsdk/gr_glide.cpp | 0x549a20 | glide_record::load_section | D25702 | the record's scales copied back from the out pointers, no u / v locals |

**D25700 (vsdk/unnamed_gadget_0x52bfe0.cpp). ui_gadget::set_parent 0x52c570 is exact.** Both branches are written
`next_sibling = parent->first_child; parent->first_child = this;` (and the same with `win->first_gadget`), with no
`first` temporary. 8966 still loads the old head, stores the new head first and then next_sibling (as the binary),
and the else branch now takes EDX for the head as the binary. The temporary-free form in only one branch (either)
gives the old code (4). Same behaviour: the two stores are to different fields (0x26c of parent / 0x4c4 of the
window, 0x270 of this), so their order does not change the result. Device: **a field assigned straight from
another field, instead of through a temporary, changes 8966's register choice even where the code order stays.**

**D25702 (vsdk/gr_glide.cpp). glide_record::load_section 0x549a20 is exact.** The tail is
`this->section = section; *u_scale = sec->u_scale; *v_scale = sec->v_scale; this->u_scale = *u_scale; bitmap =
entry - glide_tcache_entries; this->v_scale = *v_scale;` (the old source had `float u`, `float v` locals). The unit
is /Ow (no aliasing assumed within the function), so 8966 forwards the stored values instead of reading through
the pointers: v goes through the dead section argument slot (ECX, then ECX is reused for glide_tcache_entries)
and is reloaded for the last store, which is what the binary does and what 720 store orders and every local form
of earlier rounds could not give. Same behaviour as the binary (identical bytes). In plain C the copy-back would
differ only if u_scale / v_scale pointed into this record, which no caller does. Variants: only v read back
(`this->v_scale = *v_scale`, u kept as a local) also matches; u read back alone is 20; `this->v_scale =
sec->v_scale` without the v local is 12; the v local kept with `this->v_scale = *v_scale` is 21.

## Closer, kept

- vsdk/gr.cpp 0x507100 gr_bitmap_float: 12 -> 8 (D25701).

**D25701 (vsdk/gr.cpp; closer only). gr_bitmap_float 0x507100: corner 2 written u, v, _unk1a, sx, sy, sw.** I
compiled all 720 orders of corner 2's six statements: 36 give 8, none exact (84 give 10, the rest 12-20). On top
of the chosen order, all 720 orders of corner 3's six statements (best 8, the current order) and 1260
interleavings of corner 2's groups (constants, sx, sy, sw) with corner 3's (constants, sx, sy), sx before sy in
each (best 8). Left: the binary stores corner 3's four constants before corner 2's sx `fadd x` / store; 8966
keeps them after. Same values (independent stores).

## Still differs (tried; do not repeat)

- **game/combat.cpp 0x4665f0 combat_get_weapon_tag_position (2)**: the `add esp, 16` after the bone call is
  placed before the products (binary: after the second fmul). New: `if ((dist = ...) < best_d)` (7), `dz*dz + (dy*dy
  + dx*dx)` (12), best / best_d store order, n read in the loop test (33), dz / dy / dx statement order reversed
  (10), bo declared before bp, a `vector3 *pp = &bp` argument (49), a while loop. All 2 or worse. Scheduling.
- **game/combat.cpp 0x464d00 combat_position_modifier (4)**: facing's slot (ours defender's dead argument slot,
  the binary attacker's). New: 50 renamings of the two parameters and of facing (attacker / defender, a / d2,
  att / def, src / dst, c1 / c2, me / him, atk / dfn, from / to, self / other, each with facing, dot, f,
  facing_dot, cosang): all 4, so at /O2 the names do not steer the argument-slot packing (unlike the /Od hash
  of D25200). Also: facing declared with its initialiser, m declared last (4), the sum written in the test with
  no facing local (50).
- **game/combat.cpp 0x468760 blood_splot_place (4)**, **0x463d00 ai_choose_special (26)**: not retried (D9301 /
  D17303 / D20101 list the `0 - u.z` forms; D21805 and earlier the push-block layout, an 8168 layout per D7807).
- **vsdk/unnamed_floatmath_0x542500.cpp fast_inv_sqrt (22), fast_sqrt_refined (8), fast_sqrt (8)**: new: the
  three compiled as C (a .c file, cl's C front end, same flags): the same EAX / EDX / ECX code as the C++ build.
  The rotation does not come from the C++ front end. With 60+ earlier forms: exhausted.
- **vsdk/gr.cpp 0x507310 gr_line (8)**: not retried (D18001, D20200's notes, round 2026-10-21c's list).
- **vsdk/unnamed_anim_track_0x550660.cpp 0x5506e0 calc_rot_tangents (2)**: new: the terms of tan_a.y and of
  tan_b.x swapped (2), tan_b.y before tan_a.y (14), all tan_a then all tan_b (381). Scheduling.
- **game/unnamed_ai_move_0x4b1410.cpp 0x4b3c80 travel_steer_update (12)**: new: a `Character *me = owner` for the
  offset and the dot in the party loop (131), the dot in a float local before the test (17).
- **game/new_dialogue.cpp 0x409000 dialogue_frame (28)**: not retried; D16803 / D19907 list the tail forms.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/vsdk/unnamed_gadget_0x52bfe0.cpp (ui_gadget::set_parent and its comment; D25700)
- src/vsdk/gr_glide.cpp (glide_record::load_section and its comment; D25702)
- src/vsdk/gr.cpp (gr_bitmap_float's corner 2 store order and its comment; D25701)

All other batch files are byte-identical to REPO's copies.

## --check results

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "<unit>" --check src/decomp/vc6-matches.txt`:
- vsdk/unnamed_gadget_0x52bfe0\.cpp: 1 unit, 48 match, 0 differ; 47 matched there, 48 now, 0 lost.
- vsdk/gr_glide\.cpp: 1 unit, 58 match, 3 differ; 57 matched there, 58 now, 0 lost.
- vsdk/gr\.cpp: 1 unit, 57 match, 4 differ; 57 matched there, 57 now, 0 lost.

Helper scripts (not for the repo): t/try.mjs (vc6-try with distances), t/perm.mjs and t/perm2.mjs (statement
permutations).

## Worker 6

### Round 6, worker W6 (D25800-D25899)

Batch: vector3_angle_and_axis, vector3::random_unit_vector / random_direction / random_direction_in_cone
(vsdk/unnamed_vector_0x523f30.cpp), cast_effect_track_add, ai_state::load (game/ai.cpp), bm_set_reduced,
bm_lock_tga (vsdk/bmpman.cpp), ai_state::choose_support_spell, ai_state::combat_toggle_ready_stance
(game/unnamed_ai_combat_0x4bce60.cpp), ix_ray_quad (vsdk/unnamed_ix_0x53ac20.cpp), decals_draw_for_lobj
(game/unnamed_decals_0x4bbe80.cpp), obstacles_debug_render (game/unnamed_shadows_0x4a37d0.cpp),
ca_character::add_skeleton (vsdk/character.cpp).

Distances below are the 2026-10-22 scoring (exception funclets left out). Helper: `tryd.mjs` in W (distance per
variant, the file restored after each run); the variant files are in `v/`.

## New matches

None.

## Closer, kept

| file | address | name | D | old -> new |
|---|---|---|---|---|
| vsdk/character.cpp | 0x540970 | ca_character::add_skeleton | D25800 | 28 -> 6 |

**D25800 (vsdk/character.cpp; closer). ca_character::add_skeleton 0x540970: 28 -> 6.** Two changes:
- the slot store through `int n = num_anims; skeletons[n] = s; num_anims++;` (alone 28 -> 18: the binary's
  `mov eax, [esi+0x48]` / `[esi + 4*eax + 0x54]` index register and the early `lea ecx, f`);
- the dup hit leaves through `goto found;` and the creation part (new, reader, File, the read) is a block (the goto
  may not skip its initialisations, C2362), ending in `i = num_anims - 1;`, with one `found: return i;` after it.
  The main path then falls into the epilogue and the dup path jumps to it, as in the binary (18 -> 6).
Left: 8966 computes the main result into EDI (the dup index i) and moves it to EAX at the shared return; the binary
keeps it in ESI (`mov esi, eax; dec esi; call ~File; mov eax, esi`) and has a separate `mov eax, edi; jmp` on the dup
path. Same behaviour (same stores, same values returned on both paths).

## Still differs (tried; do not repeat)

- **ca_character::add_skeleton (6, after D25800)**: a separate result local `r` (`r = i; goto found;`, `r = num_anims
  - 1`, `return r`; declared before or after i): 6, coalesced with i; `i = ++num_anims - 1` 6; the dup loop on a
  block index j copied to i before the goto 6; `i = num_anims++` 50; `i = num_anims - 1` after the block 48; `return
  num_anims - 1` inside the block with `found: return i` after it 18; plain returns with the block 18 (28 without
  n); `i = n` 54; no n local with the goto 16. Without the goto (on the n form): `return num_anims++` 18, `i =
  num_anims - 1; return i` 18, `return n` 32, `num_anims = num_anims + 1` 18, `n` at function scope / in a block 18,
  the dup loop with break + `if (i < num_anims) return i` 28; without n: `return num_anims++` 22, with a
  `!_stricmp` loop 22, with the break form 26.
- **cast_effect_track_add 0x4750b0 (2)**: the binary's loop test is `cmp ecx, end; jl` with the pointer in ECX and
  the index in EAX. The `jl` (a signed compare of a pointer) shows that this is 8966's linear-function test
  replacement of a plain indexed loop, not a pointer-end loop in the source (a pointer-end source gives `jb`, 18).
  The plain `for (i...) if (cast_effect_tracks[i].spell == -1) break;` gives that test but with index / pointer in
  ECX / EAX (16). Tried on the plain loop (all 16-39): i declared first, unsigned i (18), no `in` local (28), the
  spell test after the loop / after the `i == 5` test / joined with `||` (28), before the fx tests (39), handle store
  first (18), `spell < 0`, `i != 5` (25), `i >= 5` (25), `if (i != 5) { ... }` (16), the return inside the loop
  (20), `while (i < 5 && ...)` and the test in the for condition (16). On the e-walk (current form, 2): `i++, e++`
  (4), `e = ..., i = 0` (4), e declared with its initialiser and incremented in the for or the body (4-6), pointer
  end with `<`, `!=` or `&tracks[5]` (18), the e test after the loop (15), the spell test after the loop (14).
- **vector3_angle_and_axis 0x5249e0 (2)**: `(double)` argument to acos, a sign local `s` with `angle = -angle`,
  `>= 0` with the returns swapped (6), angle declared first, a `float d` dot local: all 2. Scheduling; exhausted
  with D17302 / D19702 / D15905.
- **random_unit_vector / random_direction / random_direction_in_cone (5 each)**: not retried; D15905 / D17301 /
  D19701 list 64 forms.
- **bm_set_reduced 0x502240 (4)**: not retried; D20001 compiled all 576 statement orders.
- **bm_lock_tga 0x503710 (10)**: frame slots of h / loaded. New: renaming loaded (first, done, mip, flag, got, ok,
  was_loaded, have, lvl, again) and h (height, hh, ht, y, lh, mh): all 10. Local names do not order this /O2
  frame (unlike the /Od hash of D25200).
- **ix_ray_quad 0x53b130 (4)**: not retried; D20301 / D15904 lists are exhaustive (scheduling).
- **obstacles_debug_render 0x4a3890 (20)**: ESI / EDI for cell / edge. New: the edge index as j or k instead of i
  (the wall loop keeps i), the wall loop on j or k: all 20.
- **decals_draw_for_lobj 0x4bc6b0 (12)**: not retried (the tri[2].z store, `lea ebp, clipped` and one fmul's
  operand order: scheduling; 21b / 21d / 21e lists).
- **ai_state::load 0x4747e0 (18)**: new: `c = living_list.live_next` before the hash read (for and while forms),
  the search in the for condition, `hash` and `n += 4` on separate lines, the hash read in the for-init (both
  comma orders): all 18; the arg read before the target store (24), `target = -1` then the if (23), `c ==
  &living_list` first (35), the arg copied as an int (18). The binary's hash read restarts the EAX / ECX / EDX
  rotation; ours continues it.
- **ai_state::choose_support_spell 0x4bea80 (18)**: new: `result = K; break;` in the pick loop (47), the dead `i ==
  -1` test removed (18), `r = frand()` moved next to the loop (18). EBX / EBP swap.
- **ai_state::combat_toggle_ready_stance 0x4bdce0 (24)**: not retried (21b / 21d / 21e lists).

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/vsdk/character.cpp (ca_character::add_skeleton, D25800)

## --check

- vsdk/character.cpp: `check against src/decomp/vc6-matches.txt: 33 matched there, 33 now, 0 lost`.

## Worker 7

### m-1022 worker 7 (D25900-D25999)

Batch: pathvis_bsp_contains, pathvis_add_edge_node, determine_point_visibility, do_single_vis_check
(game/pathfinding_visibility.cpp), pf_read_edges, pf_find_cell_crossing (game/pathfinding.cpp), wav_open,
snd_stream_fill_acm (vsdk/unnamed_sndstream_0x566560.cpp), pick_test_character (game/gametarget.cpp),
http_async_gethostbyname (vsdk/unnamed_http_0x55a8e0.cpp), spell_roll_amount (game/spells.cpp),
TextFile::ParseUInt (vsdk/parse.cpp), gr_d3d_init_device (vsdk/gr_direct3d.cpp), item_page_out_all
(game/item_paging.cpp).

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| vsdk/unnamed_sndstream_0x566560.cpp | 0x566630 | snd_stream_fill_acm | D25901 | the code after wav_read inside `if (wav_read(..) == 0) { ... goto done; }`, falling into `fail:` |

**D25901 (vsdk/unnamed_sndstream_0x566560.cpp). snd_stream_fill_acm 0x566630 is exact (12 -> 0).** The only
difference was where the single `rc = -1` block went. Sum.exe puts it just before `done:`, so the success path
jumps over it (`jmp done / or esi, -1 / done:`). With four `goto fail`s in a row, 8966 put the fail block right
after the last one (the acm_convert test), and the other gotos jumped into it there. The fix: the wav_read test
becomes `if (wav_read(...) == 0) { got / acm / copy ...; s->write_pos = ...; goto done; }`, and the end of that
block falls into `fail: rc = -1; done:`. The acm_convert `goto fail` stays as it was. Same behaviour.
**Device (new): 8966 places a shared `fail:` block after the source position of the last `goto` that reaches it.
To get the block at the end of the function, the last failure should fall into the label, not jump to it.** An
intermediate form also helps (4): the acm_convert test inverted, `if (acm(..) != -1) { body } else goto fail;`,
which moves the block after that if. snd_stream_fill_silence (same unit, not in my batch, 51) has the same
Lock / goto shape and is worth trying with this device.

## Closer, kept

- game/spells.cpp 0x47f4d0 spell_roll_amount: 6 -> 3 (**D25900**). The plain `for (i = 0; i < count; i++)` in
  place of D19503's `for (i = count - 1; i >= 0; i--)`. Under the current scorer the plain loop has the binary's
  entry test (`test eax, eax / jle` on count) and only the trip count differs (the binary has `dec eax` ...
  `lea esi, [eax+1]`; 8966 has `mov esi, eax`). Same behaviour, plainer source.
- vsdk/gr_direct3d.cpp 0x5395c0 gr_d3d_init_device: 20 -> 18 (**D25902**). A search of all 120 orders of the
  five z-buffer desc stores, with the pixel-format copy kept last. `dwSize` after `ddsCaps.dwCaps` (height,
  width, flags, caps, size) gives the binary's zdesc stores (one of five orders at 18). What is left is only the
  texture-caps `& 0xff` block, which round 5 already exhausted.

## Still differs (tried; do not repeat)

- spell_roll_amount (3 after D25900): `for (i = count; --i >= 0;)` (6), `while (--count >= 0)` (7),
  `while (count-- > 0)` (17), `for (i = count; i-- > 0;)` (3), `++i` (3), `i != -1` (9), `i <= count - 1` (6),
  `i >= 1` (7), `i > 0; --i` (3), `count > i` (3), `i = 1; i <= count` (7), `i = 1; i < count + 1` (7), an
  `if (count > 0)` guard with the down-for (9), the up-for `i <= count - 1` (9), do / whiles `++i <= count - 1`
  (56), `--i >= 0` (5), `i-- > 0` (20), `++i < count` (69), `i++ < count` (79), an n = count - 1 local (9),
  `i < count - 1 + 1` (3), `i - count < 0` (82), `i = count; i != 0` (5), `i != count` (5),
  `for (; count > 0; count--)` (3), `i = -count; i < 0` (8), `i = 1 - count; i <= 0` (29). None gives
  `dec / lea +1` without its own `count - 1 >= 0` test.
- pathvis_bsp_contains 0x4b86e0 (12): the binary has two inline copies of the near-split recursion that are
  cross-jumped. Case 2 pushes `node->front` and jumps into case 1's `push edi / call`. Writing the recursion
  inline in both cases (no goto) gives exactly that shape but with the roles swapped: case 1 jumps into case 2's
  copy (28). Tried on that inline form, all 28: the five other source orders of the cases (8966 sorts them),
  `continue` after either near block, `if (contains) node = back; else return false;` in either case, a
  `break` in case 1's block, a `default: break;`. One case inline and the other `goto both`, either way round
  (28). The D20601 goto form (12) is kept.
- determine_point_visibility 0x4b8350 (26): start_ms / t2 slots (4 rows), and the cleanup loop's head lea plus
  the list_remove register choice. New for the slots, all 26: unsigned t2, unsigned start_ms, `t2 = read_tsc()
  - t2` and the same for t (extra uses), `start_tsc = read_tsc() - start_tsc; cycles = start_tsc;`, t2 declared
  separately first, the reverse declaration order.
- pf_read_edges 0x49c970 (2, `lea ebp, [eax]`): an up-counter do / while `++i < n` (66), `for (i = 0; i < n; i++)`
  (23), a `char *` counter (2), `*count - ++i` (68), n set before first (10), n read straight from read_int and
  stored to *count (31, both test forms), `--n > 0` (7).
- snd_stream_fill_acm, while searching (for reference): an `rc = acm_convert(..)` local (89), returns at both
  exits (120), `rc = -1; goto done` at the acm test (23), `got != 0` (14), the got branches swapped (12).
- item_page_out_all 0x46ca50 (30, EBX / EBP): `if (slot != 17) { ... }` block, the popup `return` as `goto done`,
  the popup block in an if / else, a type local for the search, the popup's mesh store before paged: all 30.
- TextFile::ParseUInt 0x516380 (12, EBP / EBX): parenthesised digit terms (decimal, hex), `value << 4`, the digit
  first in the sum, unsigned digits, `*cursor` for cursor[0]: all 12. Register choice.
- gr_d3d_init_device (18): the rest of the 120 zdesc orders are 18-27. The texture-caps block is round 5's list.
- Not retried (exhausted by earlier rounds; looked at with --align only): pathvis_add_edge_node (22),
  do_single_vis_check (34, the scheduling of the a / b copies), pf_find_cell_crossing (16, the count / edges / e
  register permutation and an e spill), wav_open (4, imul operand load order), pick_test_character (2, `add esp,
  8` after fcomp), http_async_gethostbyname (2, the `push offset http_resolve_host` scheduled before the ip
  store).

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/vsdk/unnamed_sndstream_0x566560.cpp (snd_stream_fill_acm, D25901)
- src/game/spells.cpp (spell_roll_amount, D25900)
- src/vsdk/gr_direct3d.cpp (gr_d3d_init_device, D25902)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src --only "game/spells\.cpp|vsdk/unnamed_sndstream_0x566560\.cpp|vsdk/gr_direct3d\.cpp" --check src/decomp/vc6-matches.txt`:
3 units: 101 match, 14 differ, 0 not found, 0 errors; 100 matched in vc6-matches.txt, 101 now, **0 lost**.

Helpers in W: try.mjs (applies each variant, prints the distance), apply.mjs, sub.mjs, mk.mjs, gen_*.mjs, al.sh.

## Worker 8

### W8 notes (m-1022, decisions D26000-D26099)

Batch: party_set_level, level_parse_table, level_parse_shopkeeper (game/level_scripts_common.cpp); gr_setup_3d,
gr_project_vertex, gr_get_frustum_planes (vsdk/unnamed_gr_3d_0x50d9b0.cpp); dialogue_box::draw,
dialogue_topic_list::draw (game/unnamed_dialogue_box_0x4d6660.cpp); xp_level_up_gains
(game/unnamed_experience_0x489930.cpp); gr_poly_split_edge_vertex (vsdk/unnamed_poly_split_0x55f110.cpp);
ai_state::goal_do_travel (game/unnamed_ai_think_0x4b5140.cpp); localize_load_names
(vsdk/unnamed_localize_0x503f60.cpp); mp_player_alloc (game/unnamed_netsession_0x440b40.cpp); cutscene_parse_table
(game/unnamed_cutscene_0x434900.cpp).

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| vsdk/unnamed_gr_3d_0x50d9b0.cpp | 0x50ed60 | gr_get_frustum_planes | D26000 | the function returns the plane count (`int`, `n = gr_num_frustum_planes;` before the flags test, `return n;`) |

**D26000 (vsdk/unnamed_gr_3d_0x50d9b0.cpp, include/vsdk/gr/unnamed_gr_3d_0x50d9b0.h). gr_get_frustum_planes
0x50ed60 exact: it returns the plane count.** Rounds 1-5 called the EAX / ECX swap (count / second index)
register choice. In the binary the count is in EAX at every `ret` (both loops reload it into EAX, the flags loop
keeps it there), which is what a returned count looks like. The function has no caller in Sum.exe, so nothing
reads the value, and declaring it `int` changes no behaviour. `return gr_num_frustum_planes;` at the end with
the old body also matches. A new device for the list: **a void function whose count / result register looks
"wrongly" chosen may return a value nobody reads** (check what is in EAX at each `ret`, and whether the function
has callers that ignore it).

## Closer, kept

None.

## Still differs (tried; do not repeat)

**D26001 (notes only). gr_setup_3d 0x50dec0 (2).** The binary does not thread the second Glide test (`cmp ecx,
0x67; jne` appears again after the 65536 store); 8966 threads ours. Finding: a dead `float old_zfar =
gr_view_zfar;` placed before the second test (or at the top of the depth-scale block) stops the threading, so the
binary's two `cmp / jne` appear, but its `fld gr_view_zfar` / `fstp st(0)` stay (5). New forms, all 2 unless noted:
`old_zfar` declared before the if and assigned in the else; the 65536 store in braces with an empty statement
after; `s` declared before the tests; the mode in an int local; `GR_GLIDE == mode`; `(gr_mode)` (unsigned) casts
on either test or on the constant; the first test as `if (!= GR_GLIDE) ; else ...`. Worse: a float `scale` local
stored once (6), the 0.98 / 65536 as if / else (4) or `!=` if / else (12).

**D26002 (notes only). level_parse_table 0x4cb7b0 (14).** The code is the binary's; only filename (binary -544,
ours -800) and pc_bitmap (binary -800, ours -416) differ in the frame (relative to the EH record; the other
buffers now sit where the binary has them). Findings about 8966's slot choice at /O2 here: the names do not
matter (pc_bitmap renamed five ways and filename four ways, buckets 0-15: all 14). Reference weights do: a probe
with one filename use fewer (`tf.Open(level_filename, ..)`) or `strcat(strcpy(filename, level_filename),
".tbl")` puts pc_bitmap at the binary's -800 but filename and eax_path at -672 (binary -544 / -800);
the strcat form is distance 13 but drops the binary's second `lea edx, filename` (not kept: it trades a real
instruction for the slot). Probes with more filename or eax_path uses changed nothing; more pc_bitmap uses moved
it lower (-1184 / -1312). filename at function scope: 14 (eax_path then shares fog_layer's slot). fxname moved into
the if block (38) or the effect loop (42), the eax buffers at function scope (34). The slots seem to be groups of
scope-disjoint buffers ordered by use weight; no source form found that gives the binary's grouping (filename
alone, eax_path with pc_bitmap) with the same code.

**D26003 (notes only). cutscene_parse_table 0x4355e0 (/Od, 64).** The register rotation is one step behind from
`fog->shot = tbl.ParseInt()`: in the binary EDX is used up after `mov ecx, fog` with no instruction. New forms on
that store, all 64: `fog[0].shot`, `(&*fog)->shot`, `((cutscene_fog *)fog)->shot`, `*&fog->shot`,
`(&fog->shot)[0]`, `*(short *)&fog->shot`, `(short)(ParseInt() + 0)`, `(fog + 0)->shot`, `(unsigned short)`,
`(wchar_t)`, `(short)(unsigned)`, `(int)`; no-code statements after it (`fog;`, `(void)fog;`, `ent;`,
`fog->shot;`, `0;`, `(void)sizeof(*fog)`). `(short)(tshot = ParseInt())` gives 23 (an extra store / reload);
`(short)(signed char)` gives 15 (the tail aligns, one `movsx cx, al` left) but truncates the value, so not kept.

**D26004 (notes only). Other functions.**
- gr_poly_split_edge_vertex 0x55f900 (2): b's red byte load after the `fild`. All 2: `(float)` on b's red (and on
  all three), `t * (b - r) + r`, `(int)` on b's red, `(-r + b)`, `(float)` on the r load, the r / g / bl loads as
  one comma expression. A pass pointer for a's colours does not compile (no such type name). Scheduling.
- level_parse_shopkeeper 0x4cc4d0 (17): a `shop_item *si = &level_shop_items[count]` pointer for the two stores
  (the binary's dead `lea` before the count reload suggests one), at function scope or in a block: 62; declared
  at the store (also with `count++` in the index) does not compile (the `goto next` crosses it).
- Looked at, earlier lists stand, no new form found (all exhausted by rounds 1-5): party_set_level (0: the SIB
  base / index order of the skill copy), xp_level_up_gains (2: EDI load scheduling; EAX holds nothing at `ret`,
  so no hidden return value), dialogue_box::draw (5) and dialogue_topic_list::draw (19) (first loaded into its
  own register), ai_state::goal_do_travel (6: push 0 / info load order), localize_load_names (12: case 1's block
  after the epilogue; no return value in EAX), gr_project_vertex (10: the final flags reload), mp_player_alloc
  (21: the `lea ecx, [eax+4]`).

## Behaviour errors / bug candidates

None found.

## Header changes

- include/vsdk/gr/unnamed_gr_3d_0x50d9b0.h: `void gr_get_frustum_planes(...)` -> `int gr_get_frustum_planes(...)`
  (D26000). Essential for the match; the function has no caller anywhere in src/decomp or Sum.exe, so no other
  unit's code changes. No struct layout touched.

## Changed files

- src/vsdk/unnamed_gr_3d_0x50d9b0.cpp (gr_get_frustum_planes, D26000)
- src/include/vsdk/gr/unnamed_gr_3d_0x50d9b0.h (its declaration, D26000)

## --check

- `VC6_STRICT=1 vc6-sweep --only "unnamed_gr_3d_0x50d9b0" --check src/decomp/vc6-matches.txt`: 1 unit, 37 match,
  0 wrong target, 2 differ; 36 matched there, 37 now, **0 lost**.

## Worker 9

### Worker 9, round 2026-10-22 (D26100-D26199)

Batch: 13 functions (nw_ras_init, nw_buffer_pop, nw_WorkReliable, quaternion::multiply, cquat_squad,
cquat::multiply, rfx_preload, effects_parse_arr_line, snow_start, mp_handle_it_okay,
gr_bitmap_3d_oriented_quad, state_title_enter, Crc32::update). Distances measured with the 2026-10-22 tools.

## New matches

- game/multitrade.cpp 0x4719e0 mp_handle_it_okay (3 -> exact). **D26101.**

**D26101 (game/multitrade.cpp). mp_handle_it_okay 0x4719e0 exact.** The trade state is copied into a local after
the target test, `int state = mp_trade_state;`, and the outer test reads the global for its first compare and the
local for its second: `if (mp_trade_state == MP_TRADE_OPEN || state == MP_TRADE_ACCEPTED) { if (state ==
MP_TRADE_ACCEPTED) ...`. 8966 then threads only the state-3 path into the inner test's mp_trade_complete tail
jump and re-tests 3 on the state-2 path (`cmp 3; jne exit; jmp complete` / `cmp 3; jne else`), which is the
binary's partial threading that 29+ earlier forms did not give. The mixture decides it: all-global threads both
paths (3); local / global splits g-s-g, s-g-g give 3; s-s-g, g-g-s, s-g-s and an inline helper with a local
(`int s = mp_trade_state; return s == 2 || s == 3;`) thread neither (1); inline bool / int helpers without a
local give 4 (the exit `ret` duplicated); the local at the top of the function gives 21. Same behaviour (nothing
writes mp_trade_state between the reads). New device: **which reads of a global go through a local copy decides
VC6's jump threading** (here a read of the global and a local copy of the same value are different values to the
threader, so only the compares on the local are threaded into each other).

## Closer, kept

- vsdk/unnamed_net_0x517590.cpp 0x517e00 nw_WorkReliable: 19 -> 17. **D26100.**

**D26100 (vsdk/unnamed_net_0x517590.cpp). nw_WorkReliable 0x517e00, 19 -> 17.** The heartbeat header's `seq = 0`
before its `data_len = 0` (D24504 had data_len first): the binary stores the word at +2 before the word at +4.
Same behaviour. Left (unchanged from D24504): the IAMHERE status store, the ping's load order, the dead `cmp
0x258` and the seq register at the rsequence store.

## Still differs (tried; do not repeat)

**D26102 (notes only). Tried without a match.**

- nw_WorkReliable 0x517e00 (17): the ping (`pings[pos] = last_packet_received - send_time`; the binary loads
  send_time before the num_ping_samples increment): `-send_time + last_packet_received`, `+= 1`, `++x`,
  `(&nw_reliable_recv_buf)->send_time`, an `int *slot` for the ping entry (all 17); a `sent` / `rtt` local, or the
  store before the increment (246-270: any new local in this function moves EBX / EBP). IAMHERE: a `net_addr *to`
  local and an `int *st` status pointer (17). recv_len: the identical-branches `if (data_len > 600) recv_len =
  data_len; else recv_len = data_len;` without a len local (all of `>`, `>=`, `<`, braced, int cast: 45, the
  branches are not merged); with the len local (D24504's 50 form) plus a `ushort seq` local or an `(int)` cast on
  the reply's seq: 48.
- nw_ras_init 0x5178c0 (0, the loop failures' cross-jump target): `!ptr` tests, an extra scope around the
  GetProcAddress checks, the GetProcAddress results assigned inside the tests, the loop body nested on success with
  one failure tail and `continue` (all 0, same target). With D17704: exhausted.
- Crc32::update 0x552130 (21): `(value >> 8) ^ table[...]` with the index (21); a `*p++` pointer walk with the
  pointer declared before / after the init check, `value >> 8` first or last, a byte local `c`, an index local
  `idx`, a shift local, `value >>= 8; value ^= table[idx];` in index and pointer forms, `buf` itself stepped (21-39).
  Register choice; with D12203 and the 21e list: exhausted.
- rfx_preload 0x51bfd0 (6): total declared in the `pool == &effects_mempool` block (6), no total (the sum written at
  its three uses: 44), `b->data = NULL` before the sum, total / offset declared first, a separate declaration (6),
  `b->level = 0` after the queue call (14). The ECX / EDX roles of the two loads; exhausted.
- effects_parse_arr_line 0x51ca60 (6, the inner loop's entry exit target): an `unsigned n = *num` local for the
  entry test (both placements), `if (n < MAX) do { } while`, an `unsigned *n = num` pointer (78), `break` instead
  of the inner `return -1`, a result local with break (19), the strtok in the test (6). Exhausted.
- quaternion::multiply 0x522930 (2), snow_start 0x494ab0 (2), gr_bitmap_3d_oriented_quad 0x53edd0 (6),
  nw_buffer_pop 0x517820 (8), cquat_squad 0x523b10 (12), state_title_enter 0x431e20 (14), cquat::multiply
  0x522a40 (20): read the alignments and the earlier lists (D17801 / D19805, D17404 and 21c, D15003 and 21 / 21c,
  D17705 / D19804, 21b / 21d, D11910 / D12707 / D19202 / D21306, D15801 and 21e); every new-round device that
  applies is listed there as tried. Not retried. (The D26101 threading device does not apply to any of them: none
  differs in jump threading.)

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/game/multitrade.cpp (mp_handle_it_okay, D26101)
- src/vsdk/unnamed_net_0x517590.cpp (nw_WorkReliable heartbeat store order and its comment, D26100)

## --check results

- `--only "multitrade"`: 1 unit, 22 match, 0 differ; check: 21 matched there, 22 now, 0 lost.
- `--only "unnamed_net_0x517590"`: 1 unit, 27 match, 4 differ; check: 27 matched there, 27 now, 0 lost.

## Worker 10

### Worker W10, round 6 (m-1022), block D26200-D26299

## New matches

| File | Address | Function | Decision | What did it |
|---|---|---|---|---|
| game/unnamed_tentacle_beast_0x4c6480.cpp | 0x4c6520 | ai_state::tentacle_beast_ai_select_action | D26200 | `b.y = 0` before `a.y = 0` in the squeeze loop |

**D26200 (game/unnamed_tentacle_beast_0x4c6480.cpp). tentacle_beast_ai_select_action 0x4c6520 is exact.** With the
funclet-free scoring the remaining difference was no longer D21804's `test bl, cl` but the order of the two y
zero stores before the `distance(&a, &b)` call (binary: b.y, then a.y). Writing `b.y = 0.0f; a.y = 0.0f;` gives
the binary's bytes (store-order device, D23100 / D24201). Same behaviour: two independent stores to locals.

## Closer, kept

- game/unnamed_interface_0x411270.cpp 0x4136a0 ui_choice_popup_show: 20 -> 18, **D26201**.

**D26201 (game/unnamed_interface_0x411270.cpp; closer only). ui_choice_popup_show 0x4136a0: the enabled-array
store after the w / h zeros** (`values` (if flag 4), `w = 0`, `h = 0`, `enabled = c`). All 24 orders of the four
stores were tried; v-w-e-h, v-w-h-e, v-h-w-e and the chained `w = h = 0` forms give 18, the rest 20-31. Left: the
second loop keeps i in ESI (zeroed before the strlen) and the pointer in EDI; the binary loads the count into
ESI, zeroes EDI for i and uses ESI for the pointer. Tried on top (all 18 or worse): a `char **pp` walk (for,
for-init, `*pp++` 22), a separate `j` index, `count > i` (20), i = 0 before the strlen (34), `(int)` count, a
while loop, `i++` in the call (31), `*items++` in the copy loop (37). Same behaviour: independent global stores
with no read between them.

## Still differs (tried; do not repeat)

**D26202 (notes only). Tried, still differing.**
- ai_state::tentacle_beast_boss_load 0x4c6d10 (2): the last find_move's `mov ecx, [esi+0x4c]` after
  `push 0x87`. New forms, all 2 unless noted: the moves and slot loop in a block, `i = 0; while (i < 5) ...[i++]`,
  `find_move(tag = 0, ...)`, `t = tentacle_beast_active_tentacle` before the slot loop, a block-scope `int k`
  loop, `0x86 + 1`; `Character *o = owner` for all four calls (15). Scheduling; with D19704 exhausted.
- mp_handle_swi_path 0x4882f0 (2): the `this` lea before the walk load. New forms, all 2: `data[4] ? true :
  false`, `*wp = ...` through the pointer, `ai_state *ai = &c->ai` / `ai_state &ai` with `*wp`, `(bool)data[4]`,
  a `Character *cc` copy. Exhausted with D18801 / D19403.
- options_init 0x41f0f0 (4 now; the `test al, cl` difference is gone, only the forwarded first dword is left):
  not retried; D17304 / D14408 / the 2026-10-21c list cover 36 forms.
- vfx_channel::eval 0x55bc00 (6): not retried; D22900 lists 25 forms for the base_value / p[2] register pair.
- interface_init_global 0x4116c0 (8): new forms: default first with both stores per case (26), `case 0: case 1:
  default:` merged after case 2 (23), an int hires local stored once (21), both stores in every case without the
  preset (26). Exhausted with D16503 / D19104 / D21402.
- avoid_poly_add_search_nodes 0x4aa400 (8): the binary stores a float 0 at frame +0x10 (shared with i and the
  len temporaries) before the overlap call and its overlap-failed exit (laid out last) reloads it. Finding: a
  local whose address escapes to a call does produce the store and reload: `vector2 d; d.x = 0.0f; if
  (!overlap) return d.x;` with d later used as `d = hits[...] - ...; return d.len();` (76-88: d gets its own
  slot, frame 0x48, and the exit stays inline). So the original likely had a zero-initialised, address-taken
  value whose slot is block-scoped (sharing i's slot). Not found. Also tried: `u = 0; return u` (42), `float
  *lp = &len` (8), `hits[0].x = 0` (9: the store appears, the return is still constant), an unused `vector2 z`
  (8), a block-scope len (8), len assigned on the nhits == 2 path (8).
- ui_choice_popup_draw 0x413c70 (11): new: a `char *s = *pp` local (45), `count > i` (15), `++i, ++pp` (11),
  `pp++, i++` (13), `y = y + h` (11), a hover local (11), the x ternary as if / else (11). Note: the title block
  now differs in popup_x's register (binary ECX, ours EBX), a consequence of the choices pointer being hoisted
  into EBX above the title. Exhausted with D24300.
- StringPool::Add 0x544c40 (22): new: the new_length assignment inside the if (24), `len + length + 1` (22),
  `count++; return strcpy(...)` (36), a strcpy result local (36), `this->` forms (22), unsigned len (22),
  `strlen(s) + 1` with dest before new_length (24). Exhausted with D19201 / D19202 / D21402.
- avoid_prim_overlap 0x4a6e00 (13), LineFile::detect_eol 0x558430 (14), tentacle_beast_frame_hook 0x4c7960
  (24): not retried; the earlier notes (D18503 + 43 forms; D19602 + the 2026-10-21e list; D23401 + 33 orders)
  cover the devices that apply.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/game/unnamed_tentacle_beast_0x4c6480.cpp (tentacle_beast_ai_select_action, D26200; comment marked exact)
- src/game/unnamed_interface_0x411270.cpp (ui_choice_popup_show, D26201)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --only "unnamed_tentacle_beast_0x4c6480|unnamed_interface_0x411270"
--check src/decomp/vc6-matches.txt`: 2 units, 88 match, 6 differ; 87 matched there, 88 now, **0 lost**.
(vc6-matches.txt itself is not updated; the new match is 0x4c6520.)
