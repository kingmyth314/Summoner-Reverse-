# match4: VC6 byte-match pass over ten game units

Base: `scratchpad/snapC/src/decomp` (the repo's src/decomp at 0e15720), copied to `match4/src`. Compiler: VC6
(cl 12.00.8168) through `tools/vc6-compare.mjs` with the per-unit flags (all ten units use the default
`/O2 /GX /MT`). Decisions D6200-D6207 are at the end of `src/include/DECISIONS.md`. Every function that matches now
says `Confidence: exact` (81 markers raised, earlier matches included). Helper scripts (not for the repo) are in
`match4/tools` (match1's, retargeted, plus try.mjs: several edit sets per run, keep the best that loses nothing).

## Totals

**256 -> 329 of 438 functions byte-identical (58.4% -> 75.1%, +73).** No function that matched before lost its
match (checked address by address against the base run: `base/*.txt`, `after/*.txt`).

| File | Before | After | Functions |
|---|---|---|---|
| game/unnamed_tiger_0x499e50.cpp | 28 | 37 | 48 |
| game/unnamed_camera_0x4778b0.cpp | 27 | 34 | 47 |
| game/unnamed_ai_combat_0x4bce60.cpp | 8 | 19 | 28 |
| game/new_status.cpp | 31 | 41 | 51 |
| game/level_scripts_common.cpp | 59 | 64 | 78 |
| game/script_internal.cpp | 10 | 15 | 28 |
| game/new_automap.cpp | 17 | 26 | 32 |
| game/boss_ai.cpp | 40 | 45 | 58 |
| game/unnamed_ai_think_0x4b5140.cpp | 14 | 19 | 29 |
| game/new_skill.cpp | 22 | 29 | 39 |

Header changes (D6200-D6203), each with every caller compiling: mingw `-fsyntax-only -std=c++98` gives 0 `error:`
lines for all 259 .cpp files. Units outside the ten that the changes touch, VC6 before / after: combat.cpp 35 / 35
of 63 (signature of has_weapon_skill), unnamed_swi_0x488230.cpp 12 / 12 of 14 (signature of
mp_send_character_walk), unnamed_ai_move_0x4b1410.cpp 4 / 4 of 19 (a caller).

## What worked (each verified by a byte match; the full list is D6205)

- **Arguments through locals.** The most productive shape of this pass: where the original computes the complex
  arguments of a call (rectangles for point_in_rect / ui_tooltip_set, cell indices, a scaled float) before the
  simple ones, `int x = ...; int y = ...; f(..., x, y, ...)` gives its code. 8 functions (status tooltip, three
  automap hover / reveal functions, both skills row hit tests, follow_character, acquire_item's format string).
- **One load, several stores.** `int walk = tiger.anim_stage2_walk;` then the stores: the original never re-reads
  the global after a store through the character pointer. The four tiger anim setters and equip / setup.
- **Float locals that live in memory.** A float assigned in several places (a result variable, a default) is kept
  in its stack slot by the original: write it as `rate = x; break;` and one return (boss_turn_rate_override,
  tiger_cache_move_b8, rand_weighted_index).
- **The search loops** of choose_offensive_spell and choose_support_spell: the `i == -1` test sits inside the found
  branch, with one `return -1` after the loop.
- **int / int division in float:** `(float)((float)rem / (double)(time * 1000))` gives `fild / fild / fdivp` and a
  float `fsubr` (all six consider_* functions).
- **Term order of sums of products:** 8168 reverses them, so `x*x + y*y + z*z` in natural order gives the original;
  vector forms (`vector3 d = a - b; d.x*d.x + ...`, `d.len()`) where the original subtracts the vectors.
- Element pointers for button / action / threat records, `vector3(...)` temporaries, bound-first clamps with short
  operands, branch polarity and nested ifs with a shared exit, bool return shapes, int flags, store order.

## What still differs (D6206)

Of the 109 functions that still differ, most differ by what no source form changed: stack pops merged across
sprintf (status_draw_stats / _level / acquire_item: the rest matches), jump tables whose cases share the default
target (three boss dispatchers), copy elision of a vector3 result (camera update_orientation, snap_to_target), spill
slots and frame sizes, shrink-wrapped pushes of two registers, ebx / ebp and esi / edi swaps (choose_support_spell,
consider_life_drain, combat_approach, script_action_wander), FPU values kept across a ternary (camera
apply_input_deltas). The long functions (approach_attack_slot, mode_combat_frame, script_parse_objects,
level_parse_table, automap_handle_click, character_use_skill) differ by these and by block layout.

## Logic errors and game bugs

- **Logic error fixed (D6204):** tiger_anim_remap_out copied the mount's loco anims; the binary copies the
  character's own (`[ebx + 4*slot + 0x5ac]`, EBX = the argument; the mount is only tested for NULL).
  **serpent_anim_remap_out (0x495db0, unnamed_serpent_0x495290.cpp, not in this pass) has the same error** in the
  source (its marker says exact, the comment says "puts the mount's current anims back"); it should read
  c->loco_anims too. The phoenix version may be worth the same check.
- **Declarations corrected (no behaviour change):** level_trigger_point_left_of_line takes the s3d_named_path (the
  caller passes the path, D6201); ai_score_support_targets' heal / cure are int (D6200); mp_send_character_walk's
  flag is bool (D6202); has_weapon_skill returns bool (D6203).
- No new game bug. The existing camera smooth_height note (overshoot replaced by -difference) still holds:
  `fabsf(step) > fabsf(d)` is the same test.
- For the owners of the other two boss units (not compiled here): unnamed_phoenix_0x497ac0.cpp (0x498720) and
  unnamed_serpent_0x495290.cpp (0x4954c0) declare the flinch message buffer as 0x70 bytes; both functions open with
  `sub esp, 0x84` in sum.asm like the tiger's, whose buffer is 0x80 (matched), so theirs are 0x80 too.
- 17 functions in these units are marked exact (by reading, before the VC6 tool) but differ under 8168 (D6207).
