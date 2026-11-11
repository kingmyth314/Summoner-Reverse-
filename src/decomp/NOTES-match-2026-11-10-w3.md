# Byte-match round 2026-11-10, worker 3 (D26600-D26699)

Batch: 39 differing functions in 15 units (script editor 11, conversationctl 4, shadows 4, net 4, targautils 3,
living_entity 2, character_instance 2, vfx 2, and one each in multispell, new_dialogue, cutscene, gamelist,
cmdline, stringpool, ui_slider). Distances are `vc6-compare --score` (strict). Helper: `v/t.mjs` (not committed)
writes each variant into the file, runs vc6-compare --score and restores the file.

Triage (Observed): every function at distance 30 or less has a latest DECISIONS entry that says exhausted / stop /
register choice (D17704, D17705, D25606, D25607, D25608, D25609, D21601, D25600, D20906, D22205, D24504, D26100,
D25003, D25104, D19201, D22206, D22207, D20908, D25105, D25106, D26003, D24506). Most of the time went to the
two least-worked large functions of conversationctl.cpp.

## New matches

None.

## Closer, kept

| file | address | name | D | what did it | before -> after |
|---|---|---|---|---|---|
| game/conversationctl.cpp | 0x4da010 | conversation_controller::frame | D26600 | the slide vector as negated position plus target; the step through `float * vector2` | 365 -> 318 |

**D26600 (game/conversationctl.cpp; closer only). conversation_controller::frame 0x4da010, 365 -> 318.** In the
approach-1 slide branch (Observed in Sum.exe 0x4da23e-0x4da2e1):
1. `d = vector2(-cur_character->pos.x, -cur_character->pos.z); d.x += slide_x; d.y += slide_z;` instead of
   `d.x = slide_x - pos.x; d.y = slide_z - pos.z;`. The binary loads pos.z and pos.x, negates each (`fchs`) and
   adds slide_x / slide_z (`fadd [esi+0x18]`, `fadd [esi+0x1c]`); the subtraction form compiles to `fld; fsub`.
   365 -> 326. Same values: -a + b is b - a exactly in IEEE arithmetic. Left: 8966 stores -z to a temporary and
   negates x first (binary: z first, no temporary). Tried (all worse or equal): the vector2 constructor with the
   two sums (357 as an initialiser, 332 assigned), `vector2(-x, -z) + vector2(slide_x, slide_z)` (337 assigned, 353
   as an initialiser), `vector2(slide_x, slide_z) - vector2(x, z)` (336), `vector2(slide_x, slide_z) +
   vector2(-x, -z)` (334), field stores `d.y = -z; d.x = -x;` then the adds (354), `d.x = -x; d.y = -z;` then the
   adds (358), the adds y first (350), `d.x = d.x + slide_x` / `slide_x + d.x` (326), `d = d + vector2(...)` (334),
   a `vector3 n = -cur_character->pos` (330), d declared with the negated constructor (344).
2. `vector2 m = (reported_frametime / len) * d; mx = m.x; mz = m.y;` instead of `float s = reported_frametime / len;
   mx = s * d.x; mz = d.y * s;`. The binary keeps the quotient on the FPU (`fld st(0)`, `fmul`) and spills the
   y product through one frame slot (`fstp [esp+0x14]; mov edx, [esp+0x14]`), which this form reproduces; the
   float local was stored (`fst s`) and reloaded. 326 -> 318. Same behaviour up to the x87 precision of the
   quotient: the old form multiplied y by the stored float s, the binary (and the new form) multiplies both
   components by the unrounded quotient, so the new form is the binary's arithmetic. Left: the binary computes the
   y product as `fld d.y; fmul st, st(2)` (ours `fxch; fmul d.y`). Tried: `vector2 m(s * d.x, d.y * s)`, the same
   assigned, `m(d.x * s, d.y * s)`, `m = s * d` with a local s, field stores, a constructor without s (all 317-318),
   no s with the division written twice (327), s at the block's top (326), mz before mx (321).
The frame is 0x3c now (binary 0x48, 0x40 before). Still differs, Observed: the binary calls the vector3 (x, y, z)
constructor (0x43b580) out of line for the three `pos + centre_offset` sums of the approach-0 case (inline budget,
D16104 / D23900); ours expands them. Unused vector3 locals as budget spenders (after the approach-0 goal call):
up to 28 change nothing (318; three in a `{ }` block 320), 30 gives 310 (a copy constructor goes out of line, not
the 3-float constructor), 32-40 are worse; after the direction normalise or the dir sum only 40 changes it (341);
at the top of case 0 or of the function, 40 changes nothing. Not kept. Also tried for the
not-arrived tail (the binary places `if (!conversation_dest_found) approach = 0; return 0;` after the give-up
block): else-block, inverted condition, nested ifs; all 318.

## Still differs (tried; do not repeat)

**D26601 (notes only). conversation_controller::camera_compute_shot 0x4d8f50 (403).** Observed: the out-of-line
call profile is the binary's except for one swap. The binary calls vector3::operator+ (0x45b680) out of line for
the two top sums `speaker->pos + speaker->centre_offset` and `cur_character->pos + cur_character->centre_offset`
and expands the default branch's `side * 0.5f`; 8966 expands the two sums (calling the 3-float constructor) and
calls the member `vector3::operator*(float)` out of line for `side * 0.5f`. All other counts match (10 copy
constructors, 6 operator=, 32 3-float constructors, 6 operator-, one `float * vector3`, 12 size_height).
Unused vector3 locals change the distance by count only, not by position (top, middle, before the side choice,
at the end: identical): 2-3 give 300, 5-6 give 285, 7 gives 311, 8-10 give 409, more is worse; none makes the top
sums out-of-line calls. The constructor-initialiser form of the two sums gives 403, assigning them after a
declaration 661. Not kept (dummies are not evidence). Inferred: the original spends the budget differently around
the top of the function; the inline order is not source order.

**D26602 (notes only). Retried without result (new forms only).**
- nw_ras_init 0x5178c0 (0): `goto fail` with the label in the first GetProcAddress block and the two loop
  failures as `goto fail` (10: the block moves out of line, as D17704 found for other label places).
- nw_WorkReliable 0x517e00 (17): the dead `cmp eax, 0x258` (D24504): NW_DEBUG_CHECK as `if (!(x)) f()` /
  `if (x) f()` / `if (x) f(); else f();` with an empty static inline f, `__assume(x)`, an inline function taking
  the int, an inline returning it, the check on data_len instead of recv_len: all 17. The merged-branches form
  (D24504) again 48, with a `ushort s` seq local 48, without the len local 45, with the test inverted 48. The
  ping load order: a send_time local before the sample increment (270), after it (17), `-(int)send_time + last`
  (17), a pointer to the ping slot (17), `num_ping_samples = num_ping_samples + 1` (17), the ping store before the
  increment (246). The IAMHERE status store as a comma in the first argument (17). The status store after the
  reply call scores 15, but only because the aligner moves the store; it is not the binary's order. Not kept.
- Character::say 0x461dc0 (9): `&quoted[offsets[i]]` (9), the test as `total + lens[i] + 1` (9), `>= 0x100`
  (11), the total after strncat with either test (21), the total before strcat (36), a `char *t = r->text` (9).
- script_editor_default_action_navpoint 0x4e8cf0 (17): `found = np` (17), found initialised NULL with or without
  the branch store (17 / 26), found initialised from np (26), early `return np` / `return NULL` for the empty table
  (21-44), `break` with one return (17), a while loop (17).
- Not retried, verdicts stand: pcx_read_bitmap, cmdline_take_argument, m2v_skip_extension_and_user_data,
  nw_buffer_pop, script_write_level_script, camera_blend, StringPool::Add, mp_handle_actv_gm, obstacles_debug_render,
  script_editor_door_sound_inputbox_cb, ui_slider::create_scrollbar, vfx_instance::process / init_nodes,
  script_editor_action_type_allowed, script_create, level_items_write_table, cutscene_parse_table,
  tga_rle_encode_row, mp_read_spell_targets, nw_NewCloseSocket. Looked at only: script_editor_draw_navpoints
  (cutscene loop's induction pointer, D22202), shadow_record_add (inline budget, D16704).

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## --check

- `VC6_STRICT=1 vc6-sweep --only conversationctl --check src/decomp/vc6-matches.txt`: 23 matched there, 23 now,
  0 lost (the only unit changed).
- `node tools/link-build.mjs --config fixed` in the worktree: 259 units, unresolved 0; multiply defined 0.
