# match13: VC6 byte matches for 29 units (D7100-D7104)

Compiler: VC6 SP5 (c2 12.00.8966, c1xx 8964) from the coordinator's change; the "before" lists were re-taken with
it on the unmodified snapF tree (`before8966.txt`), the result is `after8966.txt` (same sweep, my vc6-flags.txt).
Every address compared: **0 lost, 46 won; 272 -> 318 of 399 functions under 8966** (269 under 8168 before).
The gr_vertex header change also wins unnamed_script_editor_0x4de0b0.cpp 0x4e3700 (not my file).
summoner_cd.cpp is kept as the unpatched code (D2600); its 3 differing functions were not chased.

| file | functions | before 8168 | before 8966 | after 8966 |
|---|---|---|---|---|
| game/level_script_iona_ext2.cpp | 5 | 4 | 4 | 5 |
| game/level_script_lpalaceint.cpp | 12 | 11 | 11 | 12 |
| game/level_script_masad.cpp | 13 | 11 | 11 | 12 |
| game/level_script_sewers.cpp | 19 | 15 | 15 | 17 |
| game/level_script_wolong2.cpp | 15 | 14 | 14 | 15 |
| game/level_script_wolong_caverns.cpp | 13 | 8 | 10 | 13 |
| game/multiendgame.cpp | 9 | 7 | 7 | 7 |
| game/new_credits.cpp | 9 | 6 | 6 | 7 |
| game/player.cpp | 23 | 15 | 15 | 21 |
| game/summoner_cd.cpp | 6 | 2 | 3 | 3 |
| game/unnamed_evil_joseph_0x4c3470.cpp | 19 | 14 | 14 | 14 |
| game/unnamed_gtrack_0x4ae330.cpp | 9 | 8 | 8 | 9 |
| game/unnamed_input_0x4343a0.cpp | 9 | 6 | 6 | 7 |
| game/unnamed_item_info_popup_0x418580.cpp | 23 | 14 | 14 | 22 |
| game/unnamed_mlayout_0x43f9d0.cpp | 9 | 5 | 5 | 7 |
| game/unnamed_mpmenus_0x43eb00.cpp | 14 | 12 | 12 | 13 |
| game/unnamed_murod_oni_0x4c5c20.cpp | 14 | 7 | 7 | 10 |
| game/unnamed_netpackets_0x441820.cpp | 12 | 6 | 6 | 6 |
| vsdk/debug.cpp | 2 | 1 | 1 | 1 |
| vsdk/unnamed_bitarray_0x55bd80.cpp | 10 | 8 | 8 | 9 |
| vsdk/unnamed_floatmath_0x542500.cpp | 20 | 16 | 16 | 16 |
| vsdk/unnamed_gr_line_0x53e520.cpp | 19 | 2 | 2 | 10 |
| vsdk/unnamed_http_0x55a8e0.cpp | 15 | 9 | 9 | 9 |
| vsdk/unnamed_inputbox_0x5521a0.cpp | 13 | 8 | 8 | 8 |
| vsdk/unnamed_key_0x5093a0.cpp | 26 | 18 | 18 | 22 |
| vsdk/unnamed_lfsr_0x559db0.cpp | 3 | 1 | 1 | 1 |
| vsdk/unnamed_mouse_0x500a30.cpp | 28 | 27 | 27 | 28 |
| vsdk/unnamed_vfx_material_0x564d70.cpp | 15 | 9 | 9 | 9 |
| vsdk/unnamed_vif_0x5329f0.cpp | 15 | 5 | 5 | 5 |

## Header and flag changes (others: check before merging)
- `include/vsdk/gr/gr.h`: `struct gr_vertex : vector3` (the x / y / z floats become the base). D7100. All 18 users
  rechecked under 8966 (and 8168): only change is +1 match (script editor 0x4e3700). mingw 259 files, 0 errors.
- `vc6-flags.txt`: `vsdk/unnamed_gr_line_0x53e520.cpp /O2 /Ob0 /GX /MT` (D7101).
- `include/DECISIONS.md`: D7100-D7104 appended.

## What worked (D7102 lists the forms)
- gr_line: /Ob0 plus the vertex constructor: 2 -> 10.
- player.cpp 15 -> 21: branch polarity (drop_leader), one `||` condition for the shared `return false` and the
  JOSEPH return moved last (skill_unlocked), list-macro store order (party_add_member, party_lists_init), a
  `do {} while (0)` around the live-list unlink (party_remove_member), `fmt` locals for the two sprintf calls,
  one obj pointer (party_remove_player).
- item_info_popup 14 -> 22: assess-template button loops with element pointers, index loops, clamps through a
  `last` local, the release/held update template, the draw_text call order.
- wolong caverns 10 -> 13 (named `vector3 d` + `len_sq` inside an index loop: note the coordinator's tip (1) is
  the opposite; here the named local is what matches, verified under 8966).
- murod_oni 7 -> 10: `case -1: return;` (sub-mode jump table), `vector3 d(x, 0, z)` + `d.dot`, `? true : false`.
- key 18 -> 22: `bool key_down_state[256]` (D7103), `(lparam >> 16) & 0xff`, numlock `? true : false`.
- level scripts: music / rand / packet length through locals (iona_ext2, lpalaceint, masad, wolong2, gtrack);
  sewers' tentacle loops as index loops.
- Single functions: mouse_di_read (SUCCEEDED first), BitArray::get (`(ubyte)data[..]`), mp_player_level_available
  (`need` local), player_read_mouse_input (memset of 12), credits_frame (case order + separate case 0, key switch),
  mlayout state6 callbacks (then-block is the success path).

## Still differs (why)
- gr_line (9): dead-argument slots reused for locals (gr_line_3d, gr_poly, project_to_screen; D6306 (4) still holds
  under 8966), register allocation in the big bitmap / sphere / arrow functions.
- vif (10), vfx_material (6), http (6), inputbox (5), netpackets (6): not reached in this pass (time); the
  side-by-sides are in `sbs/`.
- evil_joseph (5): spawn_helpers needs the original's 12-byte temporary that 8966 elides for every form tried;
  boss_load / murod_oni boss_load / collect_area_hits: the binary re-reads the global count after the array store
  (8966 keeps it in a register for every form); ai_frame / select_action: scheduling; pick_helper_summon closer
  (handle kept in the global, mid and dir one vector) but differs in the summon_pos copy order.
- player Player::create (cross-jumped ai_group calls), is_leader (`shr al, 3`: probably a one-bit field in the
  original flags; header not changed).
- item_info_popup_page_up, BitArray::all_equal, DebugError, lfsr (2), floatmath (4), mp_reset_net_stats, key_flush,
  key_mark, key_remap_layout, key_clear, mlayout state6_enter / monster_layout_load, mp_host_game,
  mp_player_disconnected, mp_handle_leave_game (layout now right, register choice left), credits_parse_table /
  draw_lines (closer), camera_process_input, cmd_slew, sewer_init (pops deferred to the end), masad leaving_cs_end,
  create_avoid_spline: D7104.

## Logic errors / game bugs
None found in this pass. (Known: gr_bitmap_3d_projected's `!pnt->flags & 1`, already in the file header.)

## Tip for the other boss units
`case -1: break;` -> `case -1: return;` made murod_oni_ai_frame exact under 8966; ghost rider, machival, murod,
phoenix, pyrul, serpent and titus use `case -1: break;` in their ai_frame switches.

Tools: sweep.mjs (flags from `<root>/vc6-flags.txt`), cmp.mjs (side by side), c.ps1 / w.ps1, conf.js.