# Byte-match round 2026-11-10, worker 1 (D26400-D26499)

Batch: 39 differing functions in 14 units (level_scripts, the tiger unit, ix, ca_mesh, gameplay, d3d tcache,
sndstream, new_assess, new_charselect, the cutscene script unit, experience, packfile, skeleton, vfx_channel).
Tools: `vc6-compare.mjs --score` / `--align` of the main checkout run on this tree, and a small variant runner
(`v/tryd.mjs`, not committed: applies each variant alone and prints the function's --score line).

Result: 1 new match, 1 closer. Strict sweep over the 14 units with `--check` (after the vc6-matches.txt line for
0x49b520 was set to match): 465 matched there, 465 now, 0 lost. `link-build --config fixed`: 259 units, unresolved 0, multiply
defined 0 (Observed, 2026-11-10).

## New matches

| file | address | name | D | what did it | distance before / after |
|---|---|---|---|---|---|
| game/unnamed_tiger_0x499e50.cpp | 0x49b520 | tiger_path_to_target | D26400 | the vectors declared first and assigned with operator=, the side offset built in its own statement, so that VC6's inline budget runs out where the binary's does | 139 -> match |

## Closer

| file | address | name | D | what did it | distance before / after |
|---|---|---|---|---|---|
| game/gameplay.cpp | 0x438130 | state_game_enter | D26401 | living_from_handle's result in a local before place_at_party_start, slot++ as its own statement | 45 -> 29 |

## Decisions

**D26400 (game/unnamed_tiger_0x499e50.cpp). tiger_path_to_target 0x49b520 is exact.** The binary expands some
vector3 members inline and calls the COMDAT copies of others: the (x, y, z) constructor 0x43b580 three times, the
copy constructor 0x410cd0 twice (each time for the copy that an inlined `operator=` returns, `return *this`, and
the caller throws away) and `operator=` 0x413190 once (for goal). That is the per-function inline budget of
D15802 / D15803 / D24204: once it is spent, VC6 calls the remaining candidates out of line. Two findings decided
the source:
- The discarded copies show that the vectors are assigned, not initialised: `vector3 d, side, dir, goal;` at the
  top, then `d = t->pos - self->pos; d.y = 0;`, `side = ...`, `dir = side - self->pos;`, `goal = dir * 3.5f +
  side;`. With initialisations (the old source) every member was expanded (distance 139).
- side's y is t->pos.y itself and its x / z are `-d.z * 0.75 + t.x` / `d.x * 0.75 + t.z` computed from a stored
  temporary: the perpendicular `vector3(-d.z, 0, d.x)` scaled by 0.75 and added to t->pos (VC6 folds the y
  component `0 * 0.75 + t.y` to `t.y`). Written in one statement, `side = vector3(-d.z, 0, d.x) * 0.75f +
  t->pos;`, the budget ran out one candidate late (distance 77: dir's statement fully expanded). In two
  statements, `side = vector3(-d.z, 0, d.x); side = side * 0.75f + t->pos;`, the extra operator= spends the
  budget the binary spent and every out-of-line call, the frame (0x54) and the instructions are the binary's.

Forms on the way (distance): all-assign with side as the old per-component constructor (141), with d as
`vector3(x, 0, z)` (141), `#pragma inline_depth(1..8)` around the old or the assigned form (145, no effect),
`inline_depth(0)` (150: everything out of line), side as one statement (77), `t->pos + perp * 0.75f` (77), a named
perp local (77), `goal = side + dir * 3.5f` (77), `dir *= 3.5f; goal = dir + side` (103), the declarations
spread to first use in any of the 8 combinations (77 / 86: declarations cost no budget), `float len` for the
length (77), a `vector3 pos = self->pos` copy (97), d as the difference of two `vector3(x, 0, z)` (65), d copied
through a local (67), `d = t->pos; d -= self->pos` (104).

Behaviour: the same values. d's y is computed (t.y - self.y) and then overwritten with 0, as before; side.y is
`0 * 0.75f + t.y`, which equals t.y for every t.y except -0 (it becomes +0), and side.y only feeds dir.y (cleared)
and goal.y (the path goal's height, where the sign of a zero does not matter). Under VC6 the folded code reads
t.y directly, as the binary does. (Inferred for clang: it may keep the add.)

**D26401 (game/gameplay.cpp; closer, 45 -> 29). state_game_enter 0x438130:** in both party placement loops the
binary calls living_from_handle before it pushes place_at_party_start's argument (`push [edx+0xc]; call
living_from_handle; mov ecx, [esi+0x1c]; ...; push ecx`). With the call in the object expression
(`living_from_handle(h)->place_at_party_start(x)`) 8966 pushes x first. `Character *c = living_from_handle(...);
c->place_at_party_start(...);` gives the binary's order in the multiplayer loop (45 -> 35); in the single-player
loop also `c->place_at_party_start(slot); slot++;` (the binary pushes EDI and increments it after the call; with
`slot++` in the argument 8966 copies it to EAX first): 35 -> 29. `for (...; p = p->next, slot++)` or the
increment without the local: 31. Left: only the default case's `new_game = false` block, which the binary places
after the multiplayer failure branch (see 2026-10-21e); default first, cases 5 / 6 first, and no break after the
default: all 29. Same behaviour: the same calls with the same arguments in the same order of side effects.

**D26402 (notes only). Tried, still differing (do not repeat).**
- conversation_execute_action 0x4d5990 (51): a finding for the next try. In the binary kind 7's false call is
  tail-merged with its own true call (`push 0; jmp` into the code after `push 1`), and kind 9's false call is
  merged with kind 11's false call and then into kind 11's true tail; 8966 merges kind 7's false call with kind
  9's instead. Kinds 10 and 12 stay whole calls in both. `set(..., count ? true : false)` and `count != 0 ? true :
  false` as one call give setne (288).
- state_game_frame 0x438f30 (92): not retried (D25002's forms; not merged).
- tiger_ai_frame 0x49bed0 (104), tiger_combat_update 0x49b6e0 (71), tiger_anim_remap_in 0x49c130 (36): looked at
  only; register choice (EBP zero; the fire flag; the loop register rotation) as the earlier notes say.
- seg2d_intersect 0x53d8f0 (62): the binary keeps every local in a dead argument slot (dx1 in a's, den in b's,
  dy1 in d.y's, fy / fx in c.y / c.x) and has no frame; D15306's header change (the vector2 parameters as plain
  floats) is still the proposal.
- savegame::read_items 0x4d2b70 (81): the binary loads the hash (`mov esi, [eax + 4*ebp + 8]`) before the helper's
  four name_hash_nocase calls; register / strength reduction, as 2026-10-21d says.
- savegame::hot_button_set (6), level_add_navpoint (4), wav_open (4), packfile_read_directory (4),
  xp_level_up_gains (2), assess_format_info (28), snd_stream_fill_silence (51), cutscene_script_load (25),
  ix_box_oriented_box_oriented (32), mp_charselect_handle_click (281), two_stage_set (178): looked at the aligned
  listings; the differences are the ones the earlier notes list as exhausted. Not retried.
- Not looked at: the three gr_*_render_character_mesh (700-900), gr_d3d_copy_bitmap_to_surface (883),
  gr_d3d_create_texture_section (15), ix_sphere_polygon_edge (126), ix_ray_quad (4), skeleton::read (50),
  vfx_channel::eval (6), the remaining level_scripts savegame functions (register choice per earlier notes).

Hint for the lead: the inline budget (D15802) may explain more of the functions that call 0x43b580 / 0x410cd0 /
0x413190 out of line. `node tools/asm.mjs callers 0x43b580` lists them; conversation_camera_compute_shot (32
calls), blood_splot_place, pyrul_fire_attack, ai_avoid_characters, world_pick_under_mouse and
character_clip_move_to_navmesh are game functions. The test that found this one: count the out-of-line calls per
statement in the binary, then add or remove one inline candidate (an extra `operator=` statement) before the
statement where ours expands one too many.

## Header changes

None.
