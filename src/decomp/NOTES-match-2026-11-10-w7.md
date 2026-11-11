# Byte-match round 2026-11-10, worker 7 (D27000-D27099)

Batch: 39 differing functions in 16 units (game/spells.cpp, vsdk/unnamed_listbox_0x52ca60.cpp,
game/unnamed_goals_doors_0x475180.cpp, vsdk/gr.cpp, game/unnamed_mp_objects_0x47f860.cpp,
vsdk/unnamed_quaternion_0x5225b0.cpp, game/unnamed_dialogue_box_0x4d6660.cpp, vsdk/unnamed_lights_0x51e070.cpp,
game/conversation.cpp, game/level_script_ikaemosu.cpp, game/new_levelselect.cpp, game/new_pxochat.cpp,
game/unnamed_luminar_0x4c8d10.cpp, game/unnamed_states_0x431870.cpp, vsdk/unnamed_clip_0x5590b0.cpp,
vsdk/unnamed_http_0x55a8e0.cpp). Distances are `vc6-compare --score` under VC6_STRICT=1 (**Observed**).

Result: no new match; four functions closer, all in game/spells.cpp. Most of the batch had been worked by several
earlier rounds and their lists stand.

## New matches

None. src/decomp/vc6-matches.txt is unchanged.

## Closer, kept

| file | address | name | D | what did it | distance before -> after |
|---|---|---|---|---|---|
| game/spells.cpp | 0x47e900 | summon_create | D27001 | `orient.set_from_forward(caster->pos - *pos)`: the facing as a temporary, not a named `dir` | 255 -> 125 |
| game/spells.cpp | 0x47c5d0 | spell_cast_execute | D27002 | the missile case's bone_pos / bone_orient declared in each branch; the 0x68 case declares hit_orient before hit_pos | 809 -> 717 |
| game/spells.cpp | 0x47f2a0 | spell_target_value | D27000 | heal, resurrect, blind and power leech as `return cond ? a : b;` | 164 -> 150 |
| game/spells.cpp | 0x47d420 | spell_cast_launch_effects | D27003 | the missile case in a block with bone_pos / bone_orient at its top; the item missile's start as `pos + centre_offset` | 673 -> 663 |

**D27000 (game/spells.cpp; closer). spell_target_value 0x47f2a0: 164 -> 150.** Four of the cases are written as
one `return cond ? x : y;` in place of `if (cond) return x; return y;`: heal (`max > hp ? 1 : 0`), resurrect
(`dead ? 0 : 1`), blind (`flags & 0x80 ? 0 : 1`), power leech (`ap <= 0 ? 0 : 1`). With the ternary 8966 keeps
heal's 1.0 inline after the compare and jumps to the 0.0 block, as the binary does (`jne 0x47f38c`), and it
cross-jumps resurrect and blind into one test tail, as the binary cross-jumps both into not_boss's `jne`. Same
values. About 80 of the remaining 150 are the jump table's DD / DB lines, which the scorer compares as
instructions in every function that has a table (**Observed** in the alignment). What is left in the code: the
binary's not_boss block falls into its own 1.0 return and jumps to cure's 0.0; ours falls into 0.0 and jumps to the
default 1.0, and 8966 lays life drain out before not_boss (the binary after it, jumping back with `jg`).
Tried, worse or equal: a `float value` result with breaks (228, also with `default: break`), `default: return
1.0f;` with no final return (164), each ternary alone (155-169), not_boss as a ternary as well (155), cure as a
ternary as well (`return (... has_status(STATUS_SLOW)) ? 1.0f : 0.0f;`: 162),
every ternary written with 1.0 as the first value (`!dead ? 1 : 0` ...: 166-168), life drain as one test with
not_boss's condition and no goto (`level > 1 && type != 9 && !(flags & 0x40)`, either polarity, if or ternary,
nested if: 155-173).

**D27001 (game/spells.cpp; closer). summon_create 0x47e900: 255 -> 125.** The binary's frame is 0x6c, ours was
0x7c. Mapping our named slots to the binary's (helper `v/slots.mjs`, below) showed why: the binary puts the
`p -= orient.fvec * value` temporary in the slot of the facing vector, which is dead after `set_from_forward`,
and keeps `placed` in the dead `pos` argument slot. 8966 does neither while the facing is a named function-scope
`vector3 dir`. Written as `orient.set_from_forward(caster->pos - *pos);` (a temporary bound to the `const
vector3 &` parameter), the frame becomes 0x6c and every slot of the function the binary's; `dir` declared in a
block of its own around the call gives the same 125. Same behaviour: dir had no other use. What is left (about
110 of 125) is the ring xp chain: 8966 cross-jumps the identical `push 4 / mov ecx, savegame_state / call get_int
/ mov esi, eax` tails, the binary keeps every branch whole (and pushes EAX for the constant 4 in the species-4
branch). Tried on top, all 125: the chain as one nested ternary, as separate ifs each ending in `goto have_xp`,
and with braces on every branch. Before D27001 (all 255): the block locals (species, p, placed) at function
scope, p alone or placed alone at function scope, p / placed swapped, the dragon block's locals at function scope
(257), dir declared after orient.
**Device (new for this unit, the same family as D25400): a named local that is dead after its first call keeps
its slot for the whole function under 8966; where the binary reuses that slot for a later temporary, the local
was a temporary (or block-scoped) in the original.** Check the frame size first: when ours is larger, map the
slots (`v/slots.mjs`) and look for a binary temporary sitting in a named local's slot.

**D27002 (game/spells.cpp; closer). spell_cast_execute 0x47c5d0: 809 -> 717.** The frame was 0x13c against the
binary's 0x12c. (1) The missile case (0x18 ...) declares `vector3 bone_pos; matrix bone_orient;` inside each of
the two branches that use them (weapon / armour trigger, item) instead of at the top of the case. (2) The 0x68
case declares `matrix hit_orient = c->orient;` before `vector3 hit_pos = c->pos;`. With both the frame is 0x12c
and every mapped slot is the binary's (req at -156, the prepare temporaries at -64, where / st / i / found / bone /
orient / created / m: all the binary's offsets) except the item missile's dir (ours -284, the binary -272). (1)
alone gives 763 (frame 0x12c), (2) alone 804; the trigger branch's `matrix hit_orient = t->orient;` initialised
at its declaration gives 787 alone, 789 with (1), 780 with (1) and (2). Same behaviour: declarations only. What is left is code: the 0x04 / 0x15 / 0x16 party loop's
where store and tail (the binary duplicates the hit-sound epilogue into this case), the item missile's
start / dir arithmetic order (as in D27003) and several push orders.

**D27003 (game/spells.cpp; closer). spell_cast_launch_effects 0x47d420: 673 -> 663.** (1) The item missile's
start as `req->caster->pos + req->caster->centre_offset` (as spell_cast_execute writes it): the binary computes
the start into a temporary and copies x / z, adding the bone height to y, which this order gives (665; the
binary's slot for the temporary is still 12 bytes off). (2) The missile case in a block of its own, with
`vector3 bone_pos; matrix bone_orient;` at its top instead of in the two branches (663). Note the opposite
placement from D27002: here the binary has bone_pos at the case's top (one slot, -60, for both branches).
Not kept: `matrix m = t->orient;` initialised at declaration gives 654, but 8966 then drops the copy of the
unused matrix (2833 bytes against the binary's 2965; the binary makes the copy), so the assignment form stays.
Tried: the 20 groupings and operand orders of the dir expression (`a + b - c - d`, `(a + b) - (c + d)`,
`(a - c) + (b - d)` ... with either operand order in each sum): 665-693; spell_hit h declared last, `played` /
`where` / `i` declaration orders (the binary has where at the bottom of the frame and ours i; no declaration order
moves it: 8966 orders by use).

## Still differs (tried; do not repeat)

- **http_async_gethostbyname 0x55b310 (2)**: new forms, all 2: `l` declared in the block at its malloc, `new
  http_dns_lookup`, the ip store moved after abort, the `_beginthread` call in an `if`, a `(void)` cast,
  `INADDR_ANY` for 0. The binary stores `l->ip` before `push offset http_resolve_host`; scheduling (D17904, D20403).
- **spell_cast_execute 0x47c5d0 (717 after D27002)**: `matrix orient(dir.cross(up), up, dir)` with no right
  local (822).
- **spell_prepare_cast 0x47abe0 (525)**: the curse case's two hit paths: the binary duplicates the `p.mode = 5;
  p.kind = 2;` bit-field tail into both `p.value |= 1` paths (`or al, 0x80 / store / and 0x1fffff81 / or
  0x4000000a / store`), ours jumps to one shared block. Tried: mode / kind set in both branches of the source-type
  test (532), kind before mode (525), mode / kind set before the target loop (601). Also two spell_roll_amount
  calls whose tails 8966 cross-jumps (one `add esp, 48`).
- **summon_create 0x47e900 (125 after D27001)**: the ring chain's cross-jumping, see D27001.
- **gr_screenshot 0x5061f0 (199)**: the binary keeps pitch in EBX (computed per case in place of w: `shl ebx, 2`
  / `lea ebx, [ebx+ebx*2]` / `add ebx, ebx`) and w, h, buf in stack slots; ours keeps w in EBP and pitch in a slot.
  Tried, all 199: every order of `w * h * 4`, the allocation as a separate statement, h declared first, w / h
  assigned after the declarations.
- **gr_init 0x505e00 (54)**: looked only. The binary's store order of gr_screen is window_mode, max_h, max_w,
  clip_width, clip_right, clip_height, mode, aspect, offset_x, offset_y, clip_left, clip_top, clip_bottom, far,
  bitmaps, fog, vsync, poly_arg_a4 (D23503's list has the orders tried).
- **mp_handle_ai_disable 0x480f60 (4)**, **ls_ikaemostopint_init 0x4f9ec0 (11)**, **dialogue_box::draw 0x4d7680
  (5)**, **state_title_enter 0x431e20 (14)**, **quaternion::multiply 0x522930 (2)**, **cquat_squad 0x523b10 (12)**,
  **gr_line 0x507310 (8)**, **gr_classify_sphere_view 0x5594f0 (36)**, **matrix3_pack_orientation 0x475380 (20)**,
  **ui_listbox::snap_scroll_bottom 0x52ebe0 (90)**, **pxochat_handle_click 0x423a20 (288)**: read the alignment
  only; the earlier rounds' lists stand (register choice or scheduling). pxochat_handle_click: the binary's frame is
  4 bytes larger (0x118) and it keeps -1 in EBX for the whole function.
- Not looked at (time): spells_parse_table, spell_roll_amount, the other listbox functions, matrix3_unpack_orientation,
  ai_goal_evaluate, door_find_floor_point, gr_bitmap_float, mp_send_new_lep, mp_handle_status_add,
  cquat::multiply, dialogue_topic_list::draw, the two lights functions, conversation::restore_state,
  mp_levelselect_handle_click, ai_state::luminar_boss_load.

## Tool (not committed)

`v/slots.mjs <align.txt> <listing.cod> <PROC substring>` maps each named stack slot of our listing to the binary's
offset by reading the aligned `_name$[esp+K]` / `[esp + 0xV]` pairs of `vc6-compare --align` (binary offset =
our offset + (V - K - offset) + (our frame - the binary's frame)). It is how D27001 and D27002 were found: it shows
which named locals sit where the binary has a temporary. Worth adding to tools/ if the next round wants it.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/decomp/game/spells.cpp (D27000-D27003)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src/decomp --only "<the 16 units>" --check src/decomp/vc6-matches.txt`:
16 units: 517 match, 1 wrong target, 39 differ, 0 not found, 0 errors; 517 matched there, 517 now, **0 lost**.
(The wrong target is in game/unnamed_luminar_0x4c8d10.cpp, untouched; it was there at the start of the round.)

`node tools/link-build.mjs --config fixed` in the worktree: 259 units (269 compiled, 0 failed); unresolved 0;
multiply defined 0.
