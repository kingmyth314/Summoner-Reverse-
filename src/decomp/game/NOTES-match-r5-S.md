# match29: D8302 memcpy pass over the multiplayer units, second pass over five units (cl 12.00.8966)

Tree: snapT/src/decomp copied to match29/src. Compiler: scratchpad/vc6sp5/VC98 (SP5, c2 12.00.8966), flags from
match29/src/vc6-flags.txt (game /O2, vsdk /Ox /Ob0 /Ow). No header was changed. Decisions D8800 (shapes), D8801 (logic
fix), D8802 (still different) appended to match29/src/include/DECISIONS.md.

## Per file (VC6 matches, before -> after; no match lost, checked address by address)

| file | before | after | of |
|---|---|---|---|
| game/unnamed_mp_objects_0x47f860.cpp | 18 | 25 | 31 |
| game/multiplayerdata.cpp | 10 | 10 | 10 |
| game/unnamed_mp_join_0x4ab0f0.cpp | 14 | 17 | 17 |
| game/multicontainer.cpp | 11 | 12 | 13 |
| game/multichat.cpp | 13 | 13 | 13 |
| game/unnamed_level_complete_0x4b0960.cpp | 8 | 8 | 10 |
| game/unnamed_gamelist_0x4705b0.cpp | 13 | 13 | 14 |
| game/multitrade.cpp | 13 | 21 | 22 |
| vsdk/unnamed_listbox_0x52ca60.cpp | 57 | 58 | 69 |
| game/unnamed_goals_doors_0x475180.cpp | 55 | 62 | 67 |
| vsdk/unnamed_ix_0x53ac20.cpp | 24 | 25 | 35 |
| game/script_internal.cpp | 17 | 17 | 28 |
| game/unnamed_ai_move_0x4b1410.cpp | 8 | 8 | 19 |
| **total** | **261** | **289** | 348 |

Markers: every matched function says exact (24 raised, "VC6 byte match, match29; was ..."); no exact marker is on a
differing function (none had to be lowered). Stale parentheticals that explained the old difference were rewritten
(multitrade 0x471040, mp_objects 0x481050 / 0x481150, multicontainer 0x488fc0, mp_join 0x4ab350). mingw
(`-fsyntax-only -std=c++98 -w -Iinclude`): 0 errors in all 13 files.

## What worked

- **memcpy reads (D8302)**: all eight multitrade handlers at once (mp_handle_it_addi also needed `t ==
  items[i].type` for the binary's `cmp esi, [edx]`); mp_handle_respawn, _forcepr, _adjust_hp, _status_cure (status read
  after the lookup), mp_handle_srv_req; mp_handle_new_lep's packed orientation is one `memcpy(&packed, data, 17)`.
- **MP_ADD_FLOAT for a float argument**: container_spawn_loot (the binary's extra local is the macro's temporary).
- **Locals that make the binary's early loads**: mp_build_game_info (state / status bytes read first),
  ai_service_request (`int type = req->type` before the header), mp_send_new_path (`ubyte layer` first in the loop).
- **Branch layout through a reason chain**: mp_handle_join_request (`reason = 0; if (checksum) reason = VERSION; if
  (reason == 0) { if .. else if .. else { accept; return; } } send_deny(reason)`), plus `int len`.
- **Jump-table case order read from Sum.exe**: mp_handle_srv_req (case 6 before 4/5), process_goals (HIDE, SNEAK,
  BACKSTAB). process_goals also needed `bool walk = false; if (flags & 0x10) walk = true;`.
- **Condition / expression shapes**: character_is_locally_controlled (`type >= 4 && (type <= 5 || type == 9)`),
  door_start_move (`d->opening = 1 - d->opening;`, `elapsed = 0` first, opening stored before moving), goal_alloc
  (`goals[best].field` instead of a pointer), door_update (`closed * e + open * (1 - e)`: 8966 evaluates the right
  operand of + first), mp_change_level (sprintf from `level_script_table[level].name`).
- **vsdk loops / returns**: snap_scroll_top (unrotated `while (1)` with two breaks and `int n`),
  ix_linesegment_sphere_chord_length (single `return result` with nested ifs and `result -= lo`).

## Closer but still different

- mp_handle_obj_update (unsure -> likely): `vector3 pos; pos = mp_dequantize_position(..)`, target speed via memcpy,
  and the decision tree as `if (!traveling) { if (remote) { if (d > 10) goto set; goto done; } if (!(d > thr)) {
  extract; if (!(fabs > ang)) goto done; } } set:`: instruction-identical (197/197); only frame offsets differ (the
  binary keeps ch / traveling in the dead `data` slot, frame 0x28 against 0x30).
- mp_handle_new_path (unsure -> likely): `ubyte n` (the binary's byte store / dword reload with `and 0xff`), each node
  read with memcpy into a `vector2 p` copied into v, the direction loop without the redundant `&& c->ai.path` test.
  Remaining: the vector2 is stored to the stack before the call (the binary pushes the two dwords from registers).
  `*(vector2 *)&v.x` would pass (x, y), not (x, z): rejected.
- mp_handle_item_drop: matrix temporary + operator= gives the binary's nine dword copies; the three vectors' store
  order (binary f, u, r) is not the declaration order under 8966 and does not change with it.
- mp_handle_ai_disable: the knock-down time read as `ushort t; memcpy(&t, data + 5, 2)` gives `mov cx / and ecx,
  0xffff`; the forward's `from` is loaded into ECX (binary EAX).
- ui_listbox::set_vscroll_fraction: first difference moved from +0x5f to +0x1c9 (fh local, if / else rounding);
  remaining: num_lines copied to the dead `f` slot and vscroll re-read in the line loop.
- ix_sphere_polygon: instruction-identical with the i + 1 edge pair; te has its own slot (binary: r's slot).
- door_find_floor_point: `mid = vector2(..)` evaluates y first as the binary (+1 instruction); register choice in the
  edge loop differs. Not kept.

## Still different (tried)

- mp_handle_it_okay: the binary re-tests `state == 3` on the state-2 path and jumps to a `jmp mp_trade_complete`; 8966
  threads both. Tried switch, early return, inner switch, `!=` order, local copies (outer / inner), `return` after
  complete.
- mp_handle_status_add, mp_handle_combat_sequence, seg2d_intersect, matrix3_pack/unpack_orientation, ix functions:
  locals placed in dead argument slots (D8203). 30 declaration orders of seg2d_intersect change nothing.
- mp_send_obj_update: frame order of the short / float locals; 60 random renames of the seven locals change nothing
  (under 8966 the layout does not follow the names here).
- mp_send_new_lep: the packet counter held in ESI (info computed after the header, MP_ADD_FLOAT, one 17-byte copy,
  the if order: no change).
- mp_handle_actv_gm: version stays in BL in the binary (speed = 0 earlier, struct-copy of the address: no change).
- mp_handle_join_request: block order fixed by the reason chain (now exact).
- goal_find: the strength-reduced pointer's base field (binary: pos.z) and `jge exit / jmp top` loop end (continue /
  break / found forms tried).
- ai_move: the binary pushes all four registers in the prologue (no shrink-wrap); script_action_wander: `lea ecx,
  this` scheduled before the last pushes (ai pointer, goal pointer forms tried).
- listbox draw / process / wrap_text / scroll_page_*: register allocation (wrap_text keeps split in EBP and the indent
  in memory).

## Logic errors / game bugs

- **Decomp logic error fixed (D8801)**: mp_handle_item_drop read the container lock (a float written by
  container_spawn_loot with MP_ADD_FLOAT) as an int and converted it (`fild`) before container_obj_create; Sum.exe
  pushes the dword unchanged (0x4892da). Now a float.
- No new game bug. Behaviour-preserving rewrites only otherwise (`1 - opening` on a 1-bit field = `!opening`; the
  join-request reason chain gives the same reason order; `type >= 4 && (<= 5 || == 9)` = the old test).

## Tools (match29/)

c.mjs (compare one tree file), vo.mjs (variant tester with per-function changes and an LCS score of the side-by-side
when SIDE / SADDR are set; `apply`, `side`, `all` modes), sd.mjs (compile + side.mjs), ap.mjs (apply variants),
rn.mjs (random local renames), memc.mjs (`T x = ((T *)data)[N];` runs to memcpy), markers.mjs (Confidence markers
from a compare).
