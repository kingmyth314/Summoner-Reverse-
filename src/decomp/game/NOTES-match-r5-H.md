# match18: VC6 SP5 (c2 12.00.8966) byte matching, 12 game units

Base: scratchpad\snapG\src\decomp, copied to match18\src. Compiler: scratchpad\vc6sp5\VC98 (cl 12.00.8966).
All units are default /O2 /GX /MT (none is in vc6-flags.txt). /O2 /Ob0 was measured for each unit and loses
in all 12 (for example script_internal 16 -> 11, machival 22 -> 17), so no vc6-flags.txt line is proposed.

## Results (matches under 8966, before -> after)

| unit | before | after | newly exact |
|---|---|---|---|
| game/new_skill.cpp | 29 / 39 | 34 / 39 | 0x427570 0x4276a0 0x427950 0x427b50 0x428710 |
| game/s3d.cpp | 24 / 34 | 26 / 34 | 0x46f2c0 0x470410 |
| game/script_internal.cpp | 16 / 28 | 17 / 28 | 0x4dc860 |
| game/unnamed_dialogue_box_0x4d6660.cpp | 23 / 32 | 24 / 32 | 0x4d7410 |
| game/unnamed_fx_0x489c40.cpp | 45 / 57 | 47 / 57 | 0x48c3e0 0x48cb40 |
| game/unnamed_ghost_rider_0x4b9e70.cpp | 31 / 38 | 33 / 38 | 0x4baae0 0x4bb380 |
| game/unnamed_machival_0x4c0c40.cpp | 22 / 26 | 24 / 26 | 0x4c0f60 0x4c1070 |
| game/unnamed_murod_0x4c5140.cpp | 8 / 12 | 9 / 12 | 0x4c5a60 |
| game/unnamed_phoenix_0x497ac0.cpp | 30 / 36 | 32 / 36 | 0x499410 0x499b80 |
| game/unnamed_pyrul_0x4b87c0.cpp | 19 / 23 | 20 / 23 | 0x4b8ce0 |
| game/unnamed_serpent_0x495290.cpp | 31 / 43 | 35 / 43 | 0x495e10 0x4970a0 0x497230 0x4973e0 |
| game/unnamed_tiger_0x499e50.cpp | 37 / 48 | 39 / 48 | 0x49aeb0 0x49bce0 |
| total | 315 / 416 | 340 / 416 | +25 |

Checked address by address: no match lost in any unit. No header changed. mingw `-fsyntax-only -std=c++98`:
0 errors in all 12 files. Every new match has its marker raised to exact; three markers that said exact but
differ under 8966 are lowered to likely (script_action_wander 0x4dca90, dialogue_box::last_visible_line
0x4d7480, dialogue_topic_list::add 0x4d7800).

## What worked (D7700)

- Boss AI frames. `case -1: return;` alone fixed the function sizes but no frame matched; each also needed
  its go_idle layout:
  - ghost (0x4bb380), phoenix (0x499410), serpent (0x4973e0), machival (0x4c1070): the result of
    choose_attack / select_action is a `switch (r) { case 0: go_idle(); return; case 1: ...; }` (the
    binary's `sub eax, 0 / dec eax` chain). machival's client branch is unchanged.
  - pyrul (0x4b8ce0): `if (result != 0) { ... switch ... } else go_idle();` (the go_idle call is last and
    falls into the shared epilogue; the case bodies `return`).
  - murod (0x4c5a60): both go_idle paths share one call at the end: `if (result != 0) { ...; if (leap ||
    arrive || target) { ...switch...; return; } } go_idle();`.
  - ghost / phoenix: the new target handle through a local `int h = X_target; if (h != target) { target =
    h; owner->combat.target = h; flags |= 0x10; t = living_from_handle(target); }`. The binary stores
    EAX twice and ORs the flags through AL; every other form reloaded the global or used EDI for the flags.
- Horizontal distance squares: 8966 emits a hand-written `dx * dx + dz * dz` as z*z first whatever the
  source order (all orders, named / unnamed temporaries, vector2-style forms tried). `vector3 d = p - q;
  d.y = 0.0f; d.len_sq()` gives the binary's x-then-z order: serpent_wander_start 0x497230,
  ghost_wander_start 0x4baae0 (with `while (tries--)` from 8 and the vector p declared in the loop), and
  serpent_place_parts 0x4970a0 (with the inner index `for (j = i - 1; ...)` and `if (ok) {success}` first).
- Unrolled loops: 8966 does not unroll `for (i = 1; i <= 16; i++) skills_buttons[i].flags &= ~1;`; the
  binary has 16 and / store pairs, so the statements are written out (skills_update_species_tab 0x4276a0,
  which was marked exact from the 8168 survey; skills_set_page 0x427b50, six clears). skills_count_page
  0x427950 has the same body six times with constant indices (one per switch case): a local macro.
- Branch layout: serpent / tiger seat_rider (0x495e10, 0x49aeb0) nest the success path so that the shared
  `return false` is last; skills_count_in_group 0x427570 is one if / else with a single `return count`;
  tiger_reposition_update 0x49bce0 has `if (s == -1) state = 0;` before `if (s == -1 || s == 0)`, and a
  found path that leaves the retry loop with `goto picked` past the unconditional failure count (the binary
  has no `i == 3` test); s3d_read_textured_record 0x470410: an if / else of two write_string calls instead
  of a ternary argument (D6900's form).
- Locals and stores: machival_boss_load 0x4c0f60 (`const char *name = offense_point_name(i); ca_character
  *ch = owner->instance->character; tag = ch->find_mesh_tag(name);`; the same form does not help ghost /
  phoenix / serpent boss_load, which differ elsewhere); s3d_read_navpoint 0x46f2c0 (`quaternion t(&orient);
  q = t;`); script_action_wait_at 0x4dc860 (`pos = orient.fvec + c->pos; c->walk_to_point(pos, 100);`: the
  binary's frame is pos + orient, the sum is stored into pos); skills_handle_mouse 0x428710 (the
  interface_set_screen argument is a dword slot with byte 0 set: a lone bool sits at [esp + 7], the binary
  uses [esp + 4]; written `int identify; *(bool *)&identify = true;`, an array `bool identify[4]` matches
  too); dialogue_box::first_visible_line 0x4d7410 (`(first + i) % 64` with `line[idx].text` /
  `line[idx].height + line[idx].top`, as last_visible_line); fx_new 0x48c3e0 (spell_effect stored last);
  summon_spawn_effect 0x48cb40 (`d.extent_a = d.extent_b = radius;`, the binary stores extent_b first).

## Logic error fixed (D7701)

- phoenix_anim_remap_out 0x499b80 copied mount->loco_anims; the binary reads [ebx + 4*edi + 0x5ac] at
  0x499bb9, EBX = the argument c (the character's own anims), exactly as serpent_anim_remap_out (D6400). The
  mount is only tested for NULL. Fixed; now exact. (ghost / tiger remap_out already had the fix.)

No new game bug found.

## Still different (D7702), with what was tried

- tiger_ai_frame 0x49bed0: the binary keeps 0 in EBP from the start (`cmp edi, ebp`, `push ebp`, `sub eax,
  ebp` in the result switch). Rewritten closer (switch on the result with case 3 before case 1, the two
  reset_b58 branches separate, `target = tiger_target; owner->combat.target = target;`), the zero register
  never appears. Marker stays likely.
- serpent / phoenix / tiger / ghost anim_remap_in (0x495d00, 0x499ad0, 0x49c130, 0x4bb610, one source
  shape): EBP = 0 doubles as the loop index, `ready` lives in the frame and the stand slot is a branch, not
  neg / sbb. Tried: ready as if / else, ternary, separate stores with `continue`, `r != NULL` forms.
- dialogue_box::last_visible_line 0x4d7480: count and first + i swap EBX / EBP. Tried j counters, (i +
  first), a count local, a line pointer, else forms, declaration orders.
- script_action_wander 0x4dca90: `this` (&c->ai) is computed before &pos in the binary. Tried an ai_state
  pointer, a vector pointer, a goal pointer, reloading a->nav. (combat_mode 0x4dcbc0 not reached.)
- serpent_part_is_busy_or_asleep 0x4960f0: the binary pushes EBX around the flags byte test and computes
  &list[i] in ECX unused; inlining serpent_part_is_busy (inline keyword, both argument forms) gives the
  same code as now.
- s3d_read_color_record 0x470300: the binary keeps v in the f argument slot (`mov ecx, [esi]; mov [esp +
  0x18], ecx; fld [esp + 0x18]`) and reloads it; inline clamp helpers (by value, three-way), clamp_float
  and the ternaries all keep v on the FPU stack.
- s3d_read_door 0x470220: only `mov ecx, edi` (for f->error()) is scheduled earlier; four store orders give
  the same code.
- tiger_choose_attack 0x49b350: the binary tests 0x1f / 0x1e after the s == 0x21 path too, and returns 3
  through ECX after the idiv in the rand() % 3 switch. A wrapped `if (...) { stages } return 5;` with
  `break` in case 2 gets the block order (dist 74 -> 71) but not these two; not kept.
- pyrul_cast_special 0x4b94a0: 8966 moves the wall of fire / pushback `return true` behind the other cases;
  the binary has it first. Tried `case -1: break`, case order, if / else bodies.
- fx_create_at_camera 0x48c140: the three negated rows are computed in a different FPU order and through a
  12-byte temporary (operator- in vector.h); not tried further (fx_frame has the same lines).
- ghost_find_floor_below, ghost_rider_boss_load, tiger_path_to_target (out-of-line vector3 constructors),
  murod_ai_select_action (shared return blocks, D7305), skills_load_bitmaps (frame 0x10c), dialogue
  draw functions, fx_update_missile / fx_find_spawn_point / pyrul_fire_attack (size differences, not
  investigated): not reached.

## Tools (match18)

c.sh / st.sh / sd.sh (compile, status, side by side), try2.mjs (variants with an LCS distance of the
normalised instruction lists as a closeness score), distall.mjs (that distance for every differing
function of a unit), ap.mjs / mk.mjs (apply / build variants from text files), base.sh (all 12 units).
before\ and after\ hold the per-unit `-v` outputs.
