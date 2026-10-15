# decomp47: new_multi.cpp, summoner_cd.cpp, multiendgame.cpp, multiplayerdata.cpp, pathfinding_visibility.cpp

Written from Sum.exe v1.40's disassembly (`tools/asm.mjs`); Summoner 2 / Dash Faction not used. Target VC++ 6.0
(cl 12.00.8966). "exact" = control flow, calls, constants and globals match; not compiled with VC6.

| File | Original unit | Functions | Exact / likely / unsure |
|---|---|---|---|
| `game/new_multi.cpp` | `new_multi.cpp` (0x41dde0-0x41e3cf) | 10 | 2 / 8 / 0 |
| `game/summoner_cd.cpp` | `summoner_cd.cpp` (0x43e660-0x43eaf8) | 6 | 3 / 2 / 1 |
| `game/multiendgame.cpp` | `multiendgame.cpp` (0x4811a0-0x481673) | 9 + 1 initialiser pair | 6 / 5 / 0 |
| `game/multiplayerdata.cpp` | `multiplayerdata.cpp` (0x4af340-0x4afc99) | 10 | 4 / 6 / 0 |
| `game/pathfinding_visibility.cpp` | `engine\ai\pathfinding_visibility.cpp` (0x4b77f0-0x4b87bd) | 11 | 2 / 8 / 1 |
| total | | 48 | 17 / 29 / 2 |

All five units were already tagged in `sum.symbols`; no untagged neighbour was added (modules.txt is empty).
Checked and left out: 0x471df0-0x471fef (kick code, reads ids 233-234 of the multiendgame block but is not
contiguous with it, D2604); 0x481050-0x481196 (respawn / FORCEPR handlers before multiendgame's initialiser);
0x481680-0x4816d0 (three empty initialisers, entries 239-241, nothing ties them to multiendgame rather than to
the lod_dist unit after them); 0x43eb00-0x43eb60 (state 14, after summoner_cd.cpp: no string, data or call of
the CD code); 0x4af2c0 / 0x4af320 (lobj ray unit before multiplayerdata).

## Functions

### new_multi.cpp (.data 0x58e0a0-0x58e0c7 navpoint names; strings.tbl 821-827)
| Address | Function | Confidence |
|---|---|---|
| 0x41dde0 | elevator_popup_frame | likely |
| 0x41de10 | elevator_offer_ride | exact |
| 0x41dec0 | elevator_go_up_cb | likely |
| 0x41df70 | elevator_move_party | exact |
| 0x41dfb0 | elevator_go_down_cb | likely |
| 0x41e060 | elevator_choose_floor | likely |
| 0x41e140 | elevator_floor_cb_a | likely |
| 0x41e200 | elevator_floor_cb_b | likely |
| 0x41e2c0 | elevator_floor_cb_c | likely |
| 0x41e380 | ui_screen_stop_player | exact |

Levels: 15 = IonaExt ($npc117 up, $npc118 down; start groups 3 below, 4 above), 29 = eleh ($npc106 ground =
group 3, $npclg2 lower = 4, $npclg3 upper = 5). "No" / "Cancel" re-place the party at its own floor's start,
which also takes it out of the 2-unit range so the box is not offered again at once.

### summoner_cd.cpp (.data 0x594f2c-0x594f87, .bss cd_root 0x60ad7c)
| Address | Function | Confidence |
|---|---|---|
| 0x43e660 | cd_find_drive | unsure (the patched branches, D2600) |
| 0x43e8a0 | cd_select_drive | exact |
| 0x43e8e0 | cd_set_root | exact (unpatched format) |
| 0x43e940 | cd_has_level_data | exact |
| 0x43e9b0 | CdCheck | likely |
| 0x43ea20 | cd_prompt_disc2 | likely |

### multiendgame.cpp (.data 0x59bca8-0x59bd6f, .bss ccmd_Multi_path_type 0x22e94e8; strings.tbl 225-235)
| Address | Function | Confidence |
|---|---|---|
| 0x4811a0 | init_ccmd_Multi_path_type_thunk | exact |
| 0x4811b0 | init_ccmd_Multi_path_type | exact |
| 0x4811d0 | cmd_Multi_path_type | likely |
| 0x481260 | mp_send_player_left | exact |
| 0x4812a0 | mp_leave_game | likely |
| 0x481340 | mp_show_disconnect_popup | likely |
| 0x481450 | mp_disconnect_popup_cb | exact |
| 0x481460 | mp_send_player_removed | exact |
| 0x4814e0 | mp_player_disconnected | likely |
| 0x481590 | mp_handle_game_over | exact |
| 0x4815c0 | mp_handle_leave_game | likely |

### multiplayerdata.cpp (.data 0x59d648-0x59d697; strings.tbl 236)
| Address | Function | Confidence |
|---|---|---|
| 0x4af340 | mp_broadcast_new_info | likely |
| 0x4af3c0 | mp_announce_player_joined | exact |
| 0x4af400 | mp_pack_player_info | exact |
| 0x4af4b0 | mp_handle_new_info | exact |
| 0x4af4e0 | mp_add_new_player | likely |
| 0x4af670 | mp_send_net_info | likely |
| 0x4af760 | mp_handle_net_info | exact |
| 0x4af7d0 | mp_send_player_data | likely |
| 0x4af990 | mp_handle_player_data | likely |
| 0x4afbe0 | mp_page_in_player_items | likely |

### pathfinding_visibility.cpp (.data 0x59d7d8-0x59d933, .bss 0x247e968-0x247fcef)
| Address | Function | Confidence |
|---|---|---|
| 0x4b77f0 | sort_points_clockwise ("sort_points_clockwise()") | likely |
| 0x4b7af0 | make_initial_edge_tree ("make_initial_edge_tree()") | unsure (the never-taken alt-mode loop) |
| 0x4b7d30 | pathvis_add_edge_node | exact |
| 0x4b7da0 | pathvis_edge_pool_init | likely |
| 0x4b7df0 | pathvis_insert_sorted | likely |
| 0x4b7e80 | add_or_remove_edge ("add_or_remove_edge()", unoptimised) | likely |
| 0x4b8150 | pathvis_free_edge_node | exact |
| 0x4b8190 | pathvis_insert_by_side | likely |
| 0x4b8230 | do_single_vis_check ("do_single_vis_check()") | likely |
| 0x4b8350 | determine_point_visibility | likely |
| 0x4b86e0 | pathvis_bsp_contains | likely |

The quoted names are in the binary: determine_point_visibility's timing report prints them for its four phase
counters (0x59d870-0x59d8e0). They are likely the original function names; renaming in sum.symbols is a
suggestion for the main session (not done: names come from sum.symbols).

## Open questions
- cd_find_drive: the targets of the three zero-displacement jumps the no-CD edit left (D2600). The reading
  (skip non-CD drives, skip unreadable volumes, return -3 when levelm.vpp is on a non-CD drive) rests on the
  `found` store pattern only.
- make_initial_edge_tree' alternative mode (0x247fce8 set): cell edges walked through a link at +0x14 of a
  pf_edge (a 0x10-byte record); the flag is never written, so the layout behind it is unknown.
- cd_root size: sum.symbols char[64]; the .bss gap is 0xdc bytes.
- profile_label texts (all five pathvis functions) are ours, as in ai.cpp / gameplay.cpp.
- add_or_remove_edge's local declaration order (unoptimised frame) is inferred from the "first declared =
  highest address" rule; the label sits second (ebp-0x14).
- The packet's PLY_DAT skill count 0x54 against Character's 0x5c skill bytes: the last 8 are not sent.

## Doc / symbol disagreements
- sum.symbols 0x41e380 ui_screen_stop_player "(single player)": it acts only in a network game (`test
  game_flags, 2 / je ret`) and not while leaving (mp_player flags 0x80000000).
- sum.symbols 0x41df70 elevator_move_party "(floor) places cur_summon / party": the argument is the local
  player's id and only cur_summon moves (to start (id + 1) % 4, start navpoint cleared).
- sum.symbols 0x4814e0 mp_player_disconnected "(id, reason) ... chat line": it is (reason, quit); the chat
  lines are mp_handle_leave_game's. 0x481460 mp_send_player_removed "(id, reason)": (reason, id). 0x481260
  mp_send_player_left "(id)": the word is the reason (D2601). 0x4812a0 mp_leave_game "state 3 or 0xB": 3, 0xB
  (PXO game, pxo_cfg_flags bit 0) or 0xC.
- sum.symbols 0x4af340 mentions "mp_process_new_player" (stale name; the function calls mp_pack_player_info).
- sum.symbols 0x43e8e0 cd_set_root `sprintf ".\\" path`: that is the patched format (D2600).
- sum.symbols 0x4b77f0 sort_points_clockwise "collects the edges facing the point": it sorts the graph points
  clockwise and returns them as one list. 0x4b7df0 / 0x4b8190 "(list index, node)": (node, layer). 0x4b86e0
  "(node, point)": (point, node). 0x4b7e80 "(node, list index)": (edge, layer, point). 0x4b8230 "1 unless
  0x247ecdc is set ... else": 1 = blocked, returned at once while 0x247ecdc is clear (D2602, D2603).
- engine/ai/pathfinding.h pf_bsp "Type 1 leaf": type 1 splits at a point's x (pf_bsp_node_find too); the
  comment is corrected in this header copy (D2602).
- docs/sum-exe/source-map.md (0x471df0 row): "multiendgame.cpp is at 0x481340-0x4815c0" - the unit is
  0x4811a0-0x481673 (its initialiser, entry 238, is at the front).
- docs/sum-exe/game-bugs.md C110 "the elevator callbacks (2 to 5)": they use 3, 4 and 5 only.
- docs/patches/timeslip.md / docs/index.md treat the CD check as present in Sum.exe and removed by Timeslip's
  jge -> jmp; the Sum.exe in the repo already carries a file patch in cd_find_drive / cd_set_root (every
  drive finds the files under ".\"), D2600. Worth a line in the Sum.exe docs (which build / distribution).

## Game bugs (new)
- determine_point_visibility: the pending array holds 50 points of one sweep ray. At 50 the point's edges are
  inserted at once and the point is still appended (`pending[50]`, then 51, ...), overwriting the EH
  registration and the saved registers above the array (0x4b84ac-0x4b84f0); the later flush inserts those
  edges a second time, which removes them again (add_or_remove_edge toggles). Needs more than 50 graph
  points exactly in line with the query point.
- pathvis_bsp_contains: a node type other than 1, 2, 3 loops for ever (the default jumps back to the loop
  test with the same node, 0x4b8701 -> 0x4b8795).
- determine_point_visibility: when the last point of the list belongs to a group, the pending edges are never
  inserted (break before the flush); harmless, the sweep is over.
- mp_add_new_player: a NEW_INF / NET_INF for an id that is already connected returns the existing record, and
  both handlers print "%s has entered the game." again.
- mp_handle_net_info: the "last packet" byte is read only when a record was made; if mp_add_new_player fails
  (no free record: it also starts the "Insufficient memory" leave) mp_set_local_status(3) is never reached.
- mp_send_player_data: a client sends PLY_DAT unreliably (`multi_send_packet(..., 0)`, 0x4af96d) from
  mp_handle_gm_stat; a lost packet leaves the host without the client's equipment and stats until the next
  one (inferred: whether it is resent was not checked).
- mp_handle_player_data clamps the received HP and AP at 0 but never at the maximum.
- cmd_Multi_path_type accepts 2 (prints "Unknown"); the help text says "machiens".
- cd_prompt_disc2 centres the box horizontally only (its top edge is at height / 2).
- cd_has_level_data: sprintf with the destination as its own %s argument (works with the CRT's
  left-to-right copy).

## Header changes
See CHANGED-HEADERS.txt and DECISIONS.md D2600-D2605.
