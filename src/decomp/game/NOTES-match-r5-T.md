# match30: second / third VC6 pass on 14 units (cl 12.00.8966, VS6 SP5)

Tree: copy of snapU/src/decomp. Flags: the tree's vc6-flags.txt (game /O2, vsdk /Ox /Ob0 /Ow, unnamed_net /O2).
Total over the 14 files: **468 -> 480 of 602**, no address lost (checked address by address, before.txt /
after.txt). No header changed. Decisions D8900-D8904 in include/DECISIONS.md.

| file | before | after | of |
|---|---|---|---|
| vsdk/unnamed_quaternion_0x5225b0.cpp | 21 | 21 | 32 |
| game/ai.cpp | 78 | 79 | 88 |
| vsdk/unnamed_snd_0x5204c0.cpp | 48 | 48 | 58 |
| game/unnamed_fx_0x489c40.cpp | 47 | 47 | 57 |
| vsdk/unnamed_gr_3d_0x50d9b0.cpp | 29 | 33 | 39 |
| game/unnamed_ai_think_0x4b5140.cpp | 19 | 20 | 29 |
| game/gametarget.cpp | 12 | 14 | 22 |
| game/boss_ai.cpp | 49 | 51 | 58 |
| vsdk/unnamed_vector_0x523f30.cpp | 42 | 42 | 51 |
| game/unnamed_tiger_0x499e50.cpp | 39 | 39 | 48 |
| game/unnamed_camera_0x4778b0.cpp | 38 | 39 | 47 |
| vsdk/unnamed_net_0x517590.cpp | 22 | 23 | 31 |
| game/unnamed_ai_combat_0x4bce60.cpp | 19 | 19 | 28 |
| vsdk/unnamed_vfx_emitter_0x55ff00.cpp | 5 | 5 | 14 |

## Won (12), markers raised to exact
- boss_dismount_dispatch 0x4a2420: the three rider cases call serpent_dismount / tiger_dismount /
  phoenix_dismount (declared in boss_ai.cpp, names ours). In Sum.exe they are one folded function
  (boss_dismount 0x49c610); with one callee 8966 merges the case bodies (as D7903). A link needs the two extra
  definitions in the tiger unit (owner's call).
- boss_threat_find 0x4a2de0: `table[i].handle` (index form) instead of `table++`.
- ai_state::save 0x4746d0: `goals[i].x` directly (the element pointer biased the base by 4), `f = (g->time);`
  with parentheses for the binary's `fld / fstp / mov` copies (D8301's effect, wanted here), the name hash as
  an if / else of two name_hash_nocase calls.
- gr_rotate_vertex 0x50e910 / gr_rotate_vertices 0x50e990: `vector3 d = *p - gr_view_pos; ... d.rotated(..)`
  (in this /Ob0 unit the named difference is the original; markers already said exact).
- gr_screen_to_world_ray 0x50ebe0: x computed into dir.x and scaled there.
- gr_light_desc::gr_light_desc 0x50f3e0: explicit vector3() on +0x0c under _MSC_VER (D8900).
- ai_state::think 0x4b7610: the travel sub-mode as an if-chain with `||` (the binary compares 0, 3/5/4,
  6/7, 0x2b; any switch gives a byte table); `int t_party = 0;` declared before `t_goals`.
- object_open_pickup 0x43a6e0: `{ obj *open = o; interface_set_screen(4, &open); }` (D7004).
- world_pick_ground_point 0x43bba0: by-value inline float min / max (pick_min / pick_max, file-local),
  `(eye_pos - *pos).len()` unnamed, `int index = l - lobjs;` before lobj_face_get_surface.
- camera::apply_input_deltas 0x477af0: by-value inline float min / max (cam_min / cam_max) for all clamps.
- nw_can_write 0x5175e0: `if (ret == SOCKET_ERROR) return ret; return ret;` (the binary's `or eax, eax`
  is VC6's code for returning the compared -1, as mp_local_handle_from_net) and the timeout stored first.

## Closer (kept, still likely)
- ai_state::load 0x4747e0: indexed goals and `Character *o = owner;` give the right frame / bias and the
  owner load after the combat_style store; differs at +0x35 (rotation of the load registers eax / ecx).
- shadow_create_blob_bitmap 0x489f10: the palette as `palette[i * 3 + k]` with unsigned i (the +1 bias of
  the binary), x unsigned (the `cmp esi, 0x40 / jb` and p in EBP); differs at the row loop (y kept in
  memory, entered by a jmp).
- world_mouse_frame 0x43a960: the target distance unnamed (`(cur - target).len()`), to +0x3c; the binary
  places the object_use_container block at the end (`jne` far), not reproduced by goto / if-else wraps.
- character_can_talk_to 0x43a460: D8901 logic fix; the d.z / d.x square sum keeps both components on the
  FPU in the binary (named locals, inline helper, vector2, `d.y = 0; len()` tried).

## Logic error fixed
- D8901: character_can_talk_to passed the info-name fallback to level_shopkeeper_find; the binary passes
  npc->name (0x43a5d2 `push ecx`, ECX = npc + 0x2f4). In multiplayer an NPC with an empty name field is
  therefore never found as a shopkeeper (latent; shopkeepers have names). Behaviour of Sum.exe noted, not a
  decomp-introduced change of the game.

## Still differing, forms tried (D8903)
- snd volume clamps (set_volume / set_pan / set_volume_in_group / set_group_volume): nested, empty-then,
  goto, ternary self-assignment, comma expressions, do-while-break; all give the same canonical layout. The
  binary's `je store1; jmp end` layout looks like an unoptimised jump chain.
- nw_init_networking: layout of the slow / 56k / fast chain (slow's continuation first); goto into the slow
  block does not change 8966's placement.
- camera::set_mode (mode compared in memory, reloaded after flag stores), update_orientation (`dec / test /
  jge`; separate decrement, `== -1`, `<= -1`, `& 3` forms tried; by-value min / max did not help).
- rider_unlink_mount (flags temp register), boss_target_list_build (members pointer in the dead `list`
  argument slot), consider_life_drain (unshrink-wrapped push edi; element-pointer forms tried),
  ai_state::pick_attack_target (t / -1 registers), vector2_dist_sq_to_line (results in argument slots),
  cquat::log / exp family (fy / fz spilled as floats), gr_cull_bounding_box_transformed (plane local without
  a constructor call), tiger_path_to_target (out-of-line vector3 calls in an /O2 unit: not attempted).

## Flags retest (D8904)
unnamed_net: /O2 23, /Ox /Ow 23, /Ox /Ob0 /Ow 23, /O2 /Ow 23 of 31 (each set loses one: 0x517780 needs
/O2 because the binary reloads first_seq after the stores; 0x518ce0 needs /Ow). Kept /O2.

## Coordinator's note (snd_play_looping, D8400)
vsdk/unnamed_snd_0x5204c0.cpp was not edited (no form matched), so snd_play_looping and snd_play_3d's call
are as in snapU; nothing to merge there.

## Checks
- VC6 8966 before / after for all 14 files (before.txt, after.txt; tools/delta.mjs: 12 won, 0 lost).
- mingw `i686-w64-mingw32-g++ -fsyntax-only -std=c++98 -w -Iinclude`: 0 errors in each changed .cpp.
- Headers: none changed.
