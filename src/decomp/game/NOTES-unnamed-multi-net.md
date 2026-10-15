# decomp53: the unnamed units 0x43eb00-0x4425a6 (multiplayer menus, PXO lobby, state 6 / monster layouts, effect streaming, net session, packet layer)

Written from Sum.exe v1.40's disassembly (`tools/asm.mjs`); Summoner 2 / Dash Faction not used (names only, none
taken here). Target VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 (every unit is optimised: `nop` padding, no `ebp` frames).
"exact" = control flow, calls, constants and globals match; not compiled with VC6. Header copy rebased on repo
commit ca313cb (the latest merge when the work ended); decisions D3300-D3312 are at the end of
`include/DECISIONS.md`.

| File | Unit (code) | Functions | Exact / likely / unsure |
|---|---|---|---|
| `game/unnamed_mpmenus_0x43eb00.cpp` | multiplayer menu states 0x43eb00-0x43ef24 | 16 | 13 / 3 / 0 |
| `game/unnamed_lobby_0x43ef30.cpp` | PXO lobby (state 11), state 10 enter 0x43ef30-0x43f9cc | 27 | 20 / 7 / 0 |
| `game/unnamed_mlayout_0x43f9d0.cpp` | state 6 and monster layouts 0x43f9d0-0x4403f2 | 25 | 20 / 5 / 0 |
| `game/unnamed_effectstream_0x440400.cpp` | effect streamer 0x440400-0x440b3a | 18 | 14 / 4 / 0 |
| `game/unnamed_netsession_0x440b40.cpp` | net session 0x440b40-0x441817 | 29 | 21 / 8 / 0 |
| `game/unnamed_netpackets_0x441820.cpp` | packet layer 0x441820-0x4425a6 | 14 | 6 / 8 / 0 |
| total | the two source-map rows 0x43ec90-0x43f9d0 and 0x43f9e0-0x4425c0, plus 0x43eb00-0x43ec80 of the same hand-cut unit | 129 | 94 / 35 / 0 |

Every function of sum.symbols from 0x43eb00 to 0x442380 is written (129, including 34 compiler-generated
initialiser / atexit helpers, each with its own `// 0xADDR` line). Nothing in the two rows is left.

## Units and evidence

No unit has a file name: no assert path, no `localize_strings` read anywhere in 0x43eb00-0x4425a6 (so no strings.tbl
block), sum.symbols module "-". All six are placeholders (D3312); modules.txt therefore names no file.

Rule used for the boundaries: each VC6 object's `.data` contribution is its initialised variables, then its string
literals (interface manager, effect streamer and lobby all show it), so a variable that follows another unit's
literals starts a new object. `.bss` blocks: which functions use which block. Static initialisers are emitted where
the object is defined (inside a unit as often as at its start: ccmd_all_multi_levels 0x43ec90, ccmd_disp_rate
0x442350), so an initialiser alone does not cut.

1. **0x43eb00-0x43ef24 multiplayer menus.** After summoner_cd.cpp (last strings.tbl id 937 at 0x43ea20). .data
   0x594f88-0x594fcb: only this unit's literals ("all_multi_levels", "Toggles All_multi_levels", "All_multi_levels").
   .bss 0x60ae58 (ccmd_all_multi_levels), 0x60ae64 (All_multi_levels). mp_host_game writes the netgame block, but that
   lies in the session unit's .bss (below).
2. **0x43ef30-0x43f9cc lobby.** .data starts with two variables at 0x594fcc (1, no reference) and 0x594fd0
   (lobby_join_channel_index, -1) directly after unit 1's literals: a new object (this upgrades the source map's
   "inferred" cut, D3309). Literals "%s %s", "/list", "!EMOTD$#!", "* ", "$" to 0x594ff7. .bss 0x60ae68-0x60f1c7
   (lobby_list_timer is static_initializers entry 147). state_mp_charselect_enter (0x43f9c0) is the last function
   before the next initialiser run; it touches no data, its place is by address.
3. **0x43f9d0-0x4403f2 state 6 / monster layouts.** Initialiser run 148-150 (the three UI objects) at 0x43f9d0, the
   two ccmds 151 / 152 before their commands; .bss 0x60f1d0-0x60fe27 (after an 8-byte gap 0x60f1c8-0x60f1cf), .data
   0x594ff8-0x595139 literals only. No variable marks the lobby / state 6 cut in .data, so it rests on the
   initialiser run and the .bss gap.
4. **0x440400-0x440b3a effect streamer.** .data 0x59513c (effect_stream_base = -1, a variable after unit 3's
   literals) then ".vim", ".rfx", ".peg"; .bss 0x60fe30-0x6513a7 (File effect_stream_file is initialiser 153).
5. **0x440b40-0x441817 net session.** Initialisers 154-157 (the list heads' and records' GameTimers, mp_update_timer);
   .bss 0x6513a8-0x65188f (netgame block, timer, list heads, mp_net_port, mp_local_player, the four records); .data
   0x595158-0x5951b7 only the literals of mp_handle_gm_stat and mp_handle_ct_stat.
6. **0x441820-0x4425a6 packet layer.** .data starts with the variable mp_packet_table (0x5951b8) after unit 5's
   literals, then its own literals from 0x5958f0; .bss 0x651890-0x65e967 (characterinfo.cpp's starts at 0x65e968 with
   ccmd_charinfo). The source map has 5 and 6 as one unit; the .data and .bss split them at 0x441820 (D3309).
   0x442350-0x442380 (disp_rate) are the end of this unit, as the source map's "Tags to check" says.

## Headers (see CHANGED-HEADERS.txt)

New placeholder unit headers `summoner/multi/unnamed_mpmenus_0x43eb00.h`, `unnamed_lobby_0x43ef30.h`,
`unnamed_netsession_0x440b40.h`, `unnamed_netpackets_0x441820.h` (all included at the end of multi.h) and
`summoner/unnamed_mlayout_0x43f9d0.h`; `engine/effects/effect_stream.h` completed. The declarations of these units
that were in multi.h moved to them (D3311). Every existing src/decomp .cpp still compiles against the copy (checked
against ca313cb, all 120 files).

## Functions

#### unnamed_mpmenus_0x43eb00.cpp (16)
| Address | Function | Confidence |
|---|---|---|
| 0x43eb00 | state14_enter | exact |
| 0x43eb30 | state14_frame | exact |
| 0x43eb60 | state14_leave | likely |
| 0x43eb70 | mp_player_level_available | likely |
| 0x43ebd0 | mp_level_available_to_all | exact |
| 0x43ec20 | mp_levelselect_set_level | exact |
| 0x43ec40 | mp_levelselect_start_if_ready | exact |
| 0x43ec90 | init_ccmd_all_multi_levels_thunk | exact |
| 0x43eca0 | init_ccmd_all_multi_levels | exact |
| 0x43ecc0 | cmd_all_multi_levels | exact |
| 0x43ed80 | state_mp_gamecreate_enter | exact |
| 0x43ed90 | state_mp_menu_frame | exact |
| 0x43edc0 | mp_host_game | likely |
| 0x43eed0 | state_mp_gamelist_enter | exact |
| 0x43eef0 | state_mp_gamelist_frame | exact |
| 0x43ef20 | state_mp_gamelist_leave | exact |

#### unnamed_lobby_0x43ef30.cpp (27)
| Address | Function | Confidence |
|---|---|---|
| 0x43ef30 | init_realtimetimer_60ae78_thunk | exact |
| 0x43ef40 | init_realtimetimer_60ae78 | exact |
| 0x43ef50 | lobby_add_channel | exact |
| 0x43eff0 | state_mp_lobby_enter | exact |
| 0x43f080 | state_mp_lobby_frame | likely |
| 0x43f1c0 | lobby_request_channel_list | exact |
| 0x43f1f0 | lobby_query_all_channels | exact |
| 0x43f220 | lobby_query_channel | likely |
| 0x43f280 | lobby_poll_irc | exact |
| 0x43f2d0 | lobby_handle_chat_line | exact |
| 0x43f360 | chat_is_motd | likely |
| 0x43f3b0 | chat_is_emotd | likely |
| 0x43f400 | lobby_line_is_system | exact |
| 0x43f420 | lobby_line_is_action | exact |
| 0x43f440 | chat_is_server_line | exact |
| 0x43f450 | lobby_parse_channel_list | likely |
| 0x43f560 | lobby_find_channel_index | exact |
| 0x43f5d0 | lobby_handle_irc_command | exact |
| 0x43f660 | lobby_find_user | exact |
| 0x43f6a0 | lobby_add_user | exact |
| 0x43f6f0 | lobby_remove_user | likely |
| 0x43f740 | state_mp_lobby_leave | exact |
| 0x43f7d0 | lobby_join_channel | likely |
| 0x43f910 | lobby_send_chat_input | exact |
| 0x43f930 | lobby_disconnect | exact |
| 0x43f970 | lobby_find_channel | exact |
| 0x43f9c0 | state_mp_charselect_enter | exact |

#### unnamed_mlayout_0x43f9d0.cpp (25)
| Address | Function | Confidence |
|---|---|---|
| 0x43f9d0 | sinit_ui_window_60f938 | exact |
| 0x43f9e0 | init_ui_window_60f938 | exact |
| 0x43f9f0 | register_dtor_ui_window_60f938 | exact |
| 0x43fa00 | dtor_ui_window_60f938 | exact |
| 0x43fa10 | sinit_ui_button_60f500 | exact |
| 0x43fa20 | init_ui_button_60f500 | exact |
| 0x43fa30 | register_dtor_ui_button_60f500 | exact |
| 0x43fa40 | dtor_ui_button_60f500 | exact |
| 0x43fa50 | sinit_ui_listbox_60f1d0 | exact |
| 0x43fa60 | init_ui_listbox_60f1d0 | exact |
| 0x43fa70 | register_dtor_ui_listbox_60f1d0 | exact |
| 0x43fa80 | dtor_ui_listbox_60f1d0 | exact |
| 0x43fa90 | state6_button_cb | exact |
| 0x43faf0 | state6_listbox_cb | exact |
| 0x43fb60 | state6_enter | likely |
| 0x43fc70 | state6_leave | exact |
| 0x43fc80 | state6_frame | exact |
| 0x43fc90 | monster_layout_save | likely |
| 0x43fe00 | monster_layout_load | likely |
| 0x440170 | init_ccmd_load_mlayout_thunk | exact |
| 0x440180 | init_ccmd_load_mlayout | exact |
| 0x4401a0 | cmd_load_mlayout | likely |
| 0x4402c0 | init_ccmd_save_mlayout_thunk | exact |
| 0x4402d0 | init_ccmd_save_mlayout | exact |
| 0x4402f0 | cmd_save_mlayout | likely |

#### unnamed_effectstream_0x440400.cpp (18)
| Address | Function | Confidence |
|---|---|---|
| 0x440400 | sinit_file_651288 | exact |
| 0x440410 | init_file_651288 | exact |
| 0x440420 | register_dtor_file_651288 | exact |
| 0x440430 | dtor_file_651288 | exact |
| 0x440440 | effects_read_all_bases | likely |
| 0x440480 | effect_stream_init | exact |
| 0x4404a0 | effect_stream_queue_add | likely |
| 0x440510 | effect_stream_queue_contains | exact |
| 0x440560 | effect_stream_start | exact |
| 0x4405e0 | effect_stream_next | exact |
| 0x440650 | file_type_from_ext | exact |
| 0x4406d0 | effect_stream_open_current | likely |
| 0x440860 | effect_stream_push_mempool | exact |
| 0x440880 | effect_stream_pop_mempool | exact |
| 0x4408a0 | effect_stream_update | exact |
| 0x440930 | effect_stream_finish_entry | likely |
| 0x440b20 | effect_stream_reset | exact |
| 0x440b30 | effect_stream_flush | exact |

#### unnamed_netsession_0x440b40.cpp (29)
| Address | Function | Confidence |
|---|---|---|
| 0x440b40 | init_gametimers_65143c_thunk | exact |
| 0x440b50 | init_gametimers_65143c | exact |
| 0x440b70 | init_realtimetimer_651414_thunk | exact |
| 0x440b80 | init_realtimetimer_651414 | exact |
| 0x440b90 | init_gametimer_array_651604_thunk | exact |
| 0x440ba0 | init_gametimer_array_651604 | exact |
| 0x440bd0 | init_gametimers_6514fc_thunk | exact |
| 0x440be0 | init_gametimers_6514fc | exact |
| 0x440c00 | mp_client_check_connection | exact |
| 0x440c40 | mp_server_check_clients | likely |
| 0x440d20 | mp_server_accept_connections | exact |
| 0x440d70 | mp_host_update_session | exact |
| 0x440e90 | mp_server_frame | exact |
| 0x440ee0 | mp_client_frame | exact |
| 0x440f20 | multi_set_netgame_state | likely |
| 0x441000 | mp_handle_gm_stat | likely |
| 0x441150 | multi_process_client_status | likely |
| 0x441210 | mp_handle_ct_stat | exact |
| 0x4412a0 | mp_game_frame | likely |
| 0x4413d0 | multi_do_frame | likely |
| 0x441490 | mp_session_reset | likely |
| 0x441530 | mp_level_start | exact |
| 0x441560 | pxo_read_config_thunk | exact |
| 0x441570 | mp_set_session_state | exact |
| 0x4415d0 | mp_set_local_status | exact |
| 0x441610 | mp_player_alloc | exact |
| 0x441660 | mp_player_disconnect | likely |
| 0x441710 | character_xp_death_penalty | exact |
| 0x441760 | mp_local_player_create | exact |

#### unnamed_netpackets_0x441820.cpp (14)
| Address | Function | Confidence |
|---|---|---|
| 0x441820 | multi_process_packet | likely |
| 0x4419d0 | mp_reset_net_stats | likely |
| 0x441a30 | mp_broadcast_packet_buffered | exact |
| 0x441a90 | mp_broadcast_packet | exact |
| 0x441b10 | multi_process_incoming | likely |
| 0x441ca0 | mp_net_stats_update | likely |
| 0x441da0 | mp_send_tracked | likely |
| 0x441e90 | multi_send_packet_buffered | likely |
| 0x442060 | multi_send_packet | likely |
| 0x442230 | mp_forward_cur_packet_buffered | exact |
| 0x4422c0 | mp_forward_cur_packet | exact |
| 0x442350 | init_ccmd_disp_rate_thunk | exact |
| 0x442360 | init_ccmd_disp_rate | exact |
| 0x442380 | cmd_disp_rate | likely |

## What the units do (short)

- **Session states** (mp_netgame_state): 1 level select (the host broadcasts NEW_INF once per listed player,
  MP_PF_ANNOUNCED 0x2), 2 loading (all status 5: host PLY_DAT, status 6), 3 linking (all 7: monster list, 8),
  4 objects (all 9: unpause, 10), 5 playing. The host sends GM_STAT every 500 ms once its status is 4 or more, and on
  every state change; a client accepts only the next state from its current one and, while playing, follows the
  host's pause byte and a new level (state 1 / 2 again). A client's status 6 -> 7 is "every player has
  MP_PF_DATA_RECEIVED", 8 -> 9 "every living entity has a net handle" (multi_do_frame).
- **Send buffers:** one 0x204-byte {len, data[0x200]} per player id for each of reliable / unreliable; a packet that
  does not fit flushes the buffer first; mp_game_frame flushes everything at the end of each frame
  (`(NULL, 0, ..., flush)` calls). On a client the reliable buffer is index 0, the unreliable one the server player
  id's.
- **Forwarding:** the host relays the packet being handled (mp_cur_packet) to every other connected player except the
  sender; mp_handle_ct_stat and the item / trade / summon handlers use it.
- **Lobby:** the IRC connection is opened once pxochat_logged_in; the channel list is asked for at once and every 10 s
  ("/list"), plus a game tracker query per channel; the first channel is joined automatically; users come from the
  IRC join / part events.
- **State 6** is a char_infos picker (a 100 x 15 window with a button; the button opens a 400-pixel list box; a
  double click picks); game-bugs.md calls it the monster layout editor. No gameseq_set_state passes 6.
- **Monster layouts (.mlo):** "" header, version 1, map file name, records {type, character name, position,
  orientation} for living types 4, 5, 7, 9, ended by type 0. Loading deletes types 4, 5, 9, moves the party (type 7)
  by name and creates the rest (NEW_LEP to the clients).
- **Effect streamer:** queue of 100 {name, target, mempool}; one asynchronous read at a time; .rfx bases are read
  into their own buffer behind the 0x14-byte header and relocated by effect_base_finish_stream; .mvf / .vim go
  through effect_stream_buffer (0x40000 bytes). Only .rfx names are ever queued (effect_base_queue_stream), so the
  skeleton / vim paths are dead in v1.40, and a .peg starts a zero-byte read without opening a file.

## Open questions

- 0x594fcc (int 1) and the .bss dwords 0x6514d4 / 0x65159c have no reference; defined as placeholders where the
  layout puts them (names ours, unsure). The .bss order of the units' objects does not follow the definition
  (initialiser) order (listbox, ccmd_save_mlayout, button, ccmd_load_mlayout, window; timer 0x651414 before the list
  head 0x651418 although the head's initialiser runs first), so the definition order is the initialisers'.
- state14_leave and two calls in mp_game_frame / mp_session_reset call empty_function: functions whose bodies the
  linker folded; their own names are not known.
- lobby_try_connect (inline, name ours): the connect block re-reads lobby_flags after the frame's test, which suggests
  an inlined function rather than code written in place.
- 0x6513dc (+0x34) and 0x651400 (+0x58) of the netgame block have no direct reference (perhaps copied as a block by
  mp_handle_acpt_jn / tracker_start_game; not checked).
- mp_player +0x20 and +0x6c..+0xbb are not traced.
- The lobby / state 6 cut (0x43f9d0) has no .data evidence (see Units, item 3).

## Doc and symbol disagreements (for the docs pass)

- **docs/sum-exe/source-map.md** "Hand cuts", row 0x440b40-0x442380: two objects, cut at 0x441820 (D3309). Row
  0x43ef30: the cut is supported by .data (the variable at 0x594fcc), not only "inferred". Row 0x43eb00-0x43ef20 ends
  with the 0x43ef20 jmp (0x43ef24).
- **docs/sum-exe/multiplayer.md:** the "Flag" column is the byte that lets a handler run for a sender that is not in
  the player list (multi_process_packet); type 0x42 is accepted by the range check and runs a garbage handler, DMY_PCK
  (0x40) has handler 0 and is called too (D3310); the two bytes after the name are the net-rate flags (set for
  SWI_PTH..SWI_BSS both, SWI_SNK / SWI_HID the first only). "an eight-letter name": the field is 8 bytes, 7 letters
  and a NUL. The new sentence "The packets themselves travel over the engine's UDP reliable sockets" holds for
  multi_send_packet only: multi_send_packet_buffered (OBJ_UPD, the swi_* forwards, GM_STAT / CT_STAT status reports)
  and mp_send_tracked (game list, join replies) send plain datagrams with nw_send.
- **sum.symbols** mp_packet_table: "name[12]" is name[8] plus the 4 rate-flag bytes. net_stats_avg_recv_bytes /
  net_stats_avg_sent_bytes comments are crosswise to the code (D3307). lobby_handle_irc_command: "2 user joined,
  1 user left" is the other way round (1 joined, 2 left; multi_chattracker.h's IRC_CMD_* are right).
  mp_player_level_available: "(mask >> 26) is beyond the table" is the completed-difficulty test (`>= 1 <<
  difficulty`). mp_forward_cur_packet(_buffered) "with debug flag 2": the test is game_flags bit 1 (multiplayer)
  and the host bit (MP_IS_HOST). mp_broadcast_packet "(data, len, skip_new)": the flag skips (and clears)
  MP_PF_INFO_SENT players. effect_stream_next "returns the cursor or 1 at the end": right; so the `< 0` tests of its
  callers are dead.
- **multi.h** (before decomp53): multi_send_packet's fourth argument "reliable" is a flush flag, mp_broadcast_packet's
  third "reliable" is skip_flagged (D3301); the comment "multi_send_packet is always reliable" (commit 11f4aef) is
  right.
- **game-bugs.md** "States 5 to 8 never entered (... monster layout editor ...)": state 6 is a character picker
  window; the layout files themselves are reachable through the load_mlayout / save_mlayout console commands.

## Game bugs (candidates for game-bugs.md)

1. **Packet type 0x42 runs a garbage address** (multi_process_packet 0x441869: `ja` after `cmp cl, 0x42`, so 66 passes;
   mp_packet_table[66] is the literal at 0x5958f0, handler bytes "i_pr" = 0x72705f69, called at 0x441970 for a listed
   sender). A connected player (or a datagram with a listed player's address) crashes the receiver with one 3-byte
   packet. Same for **DMY_PCK 0x40** (handler 0). No packet size is checked against the received length either.
   (D3310)
2. **load_mlayout creates monsters with the wrong char_info** (monster_layout_load 0x43ff07 / 0x440111): the map name's
   charinfo_find_by_name result (normally -1) for every record. (D3308)
3. **load_mlayout / save_mlayout with no argument clear filename[-1]** (0x440250 / 0x4403a0: strlen 0 - 1), a stack byte
   below the buffer.
4. **NULL use after a failed setup:** mp_local_player_create (0x441772-0x4417c5) only requests state 10 when no record
   is free or no character is selected and writes through the NULL pointer; the lobby's connect step (0x43f137-0x43f150)
   reads prof->name after recording the "no profile" error.
5. **mp_server_accept_connections leaks a socket** (0x440d47): an accepted reliable connection from an address that is
   not in the player list is neither stored nor closed.
6. **lobby_parse_channel_list** returns without pxochat_sync_channel_list when a record is cut short (0x43f4bb /
   0x43f4dc), so the counts updated so far are not shown until the next list. Cosmetic.
7. **disp_rate refuses its own help and status in single player** (0x442390), and its status lists the names without a
   separator. Cosmetic, debug command.
8. **The NEW_INF skip also eats a flush:** mp_game_frame's final mp_broadcast_packet(NULL, 0, true) clears
   MP_PF_INFO_SENT instead of flushing that player's reliable buffer, so the new player's buffered reliable data waits
   one more frame. Invisible.
