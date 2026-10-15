# Phase 3: multisave.cpp, multitrade.cpp, multichat.cpp, multicontainer.cpp (decomp24)

Written from build/sum.asm (`tools/asm.mjs fn`) and the raw .data bytes of Sum.exe only. No Summoner 2 or Dash
Faction code was read. All four files and the edited `game/item_paging.cpp` copy pass
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include` with no
warnings. Every header of `include/` compiles on its own and all together, and all 44 existing
`src/decomp/{game,vsdk}/*.cpp` (repo HEAD d45c43b, item_paging.cpp replaced by this copy) compile against
this tree with exactly the diagnostics they give against the repo's own tree (none new).

**Base:** the include tree here is the repo's tree at **d45c43b** (two commits, 0d523db and d45c43b, landed while
this batch was being written; the first copy was from 86d1792 and the changes were re-applied onto d45c43b).
Header changes: CHANGED-HEADERS.txt; decisions D400-D407 at the end of include/DECISIONS.md.

All 62 functions of the four units (sum.symbols module column) are written: 38 exact, 23 likely, 1 unsure
(counting each initialiser thunk and initialiser as a function, as sum.symbols lists them).

## item_paging.cpp (moved definition)

`RealtimeTimer mp_player_autosave_timer` (0xa59a84) and its initialiser comment (pair 0x46cc60 / 0x46cc70,
static_initializers entry 206) moved from `game/item_paging.cpp` to the top of `multisave.cpp` (D405). In the
item_paging.cpp copy the definition is replaced by a note, the unit range in its first comment (and in
`engine/objects/item_paging.h`) is now 0x46c5d0-0x46cc5f, and the "may be the next unit" remark is updated.
Nothing else in item_paging.cpp changed.

## multisave.cpp (0x46cc60-0x46dd2f; .data 0x59a978-0x59aa53; .bss 0xa59a84-0xa5a29b)

| Address | Function | Confidence |
|---|---|---|
| 0x46cc60 / 0x46cc70 | init_realtimetimer_a59a84_thunk / init_realtimetimer_a59a84 (mp_player_autosave_timer) | exact |
| 0x46cc80 | mp_player_file_create | likely (species switch order 0, 2, 3, 1) |
| 0x46d0b0 | mp_player_file_write | likely |
| 0x46d450 | mp_player_file_read_summary | likely (void* parameter kept) |
| 0x46d610 | mp_player_file_load | likely |
| 0x46da20 | mp_player_autosave_start | exact |
| 0x46da30 | mp_player_autosave_frame | exact |
| 0x46da60 | mp_player_save_on_leave | exact |
| 0x46da80 | pxo_read_config | likely |

10 functions: 5 exact, 5 likely.

Facts:
- .plr format (version 9, magic "PLYR"): name, char_info, flags, description, level (int16), xp, max HP
  (float: base + HP lent to the summon), max AP (int16), skill points, 9 equipment type indexes (-1 empty),
  inventory {type, count, charges}, gold, the 36 base skill bytes, chain skills {skill, points, slot},
  6 hot buttons {kind, id} (id -1 none). Version 8 files load too: flags = ((flags >> 1) & ~3) | (flags & 1).
- New characters: level 1, char_info's max HP / AP, default weapon in slot 5, 3 Health Tonic + 1 Book of
  Novices, 30 gold, skills from the species' skill_progression at level <= 1 (Speed always 10), the four chain
  directions (up slot 0, right 3, down 1, left 2) at 0 points.
- Loading refills HP and AP to the maximum; an empty weapon slot (5) keeps the current weapon.
- Autosave every 300 s and on leaving the game (mp_player_save_on_leave), always of the local player.
- pxo_read_config: defaults ut.pxo.net / gt.pxo.com / chat.pxo.net, port 4200, the three web URLs; multi.cfg
  (any path) may override the three servers (UserTracker: / GameTracker: / ChatTracker:, quoted). Bit 1 of
  the flags is cleared only when multi.cfg exists.
- 0xa5a098 / 0xa5a09c (inet_proxy_port / inet_proxy_server, names ours) are read by InetGetFile__InetGetFile
  and written nowhere: the HTTP proxy path is dead.

## multitrade.cpp (0x470e30-0x471def; no .data; .bss 0x22e4630-0x22e47cb)

| Address | Function | Confidence |
|---|---|---|
| 0x470e30 / 0x470e40 | init_realtimetimer_22e47c4 pair (mp_trade_timeout_timer, entry 216) | exact |
| 0x470e50 / 0x470e60 | init_realtimetimer_22e47c0 pair (mp_trade_request_timer, entry 217) | exact |
| 0x470e70 | mp_trade_reset | exact |
| 0x470e90 | mp_trade_set_state | likely (case layout 2, 1, 0) |
| 0x470ed0 | mp_trade_frame | likely (if / else for 1 and 2..4) |
| 0x470f80 | mp_send_it_rqst | exact |
| 0x471040 | mp_handle_it_rqst | likely (interface_set_screen(19, 0) pushes the zero screen register) |
| 0x471130 | mp_send_it_deny | exact |
| 0x4711c0 | mp_handle_it_deny | likely (switch) |
| 0x471250 | mp_send_it_acpt | exact |
| 0x4712f0 | mp_handle_it_acpt | exact |
| 0x471370 | mp_send_it_addi | likely (merge loop shape) |
| 0x4714a0 | mp_handle_it_addi | likely |
| 0x4715a0 | mp_send_it_rmvi | likely |
| 0x4716f0 | mp_handle_it_rmvi | likely |
| 0x471820 | mp_send_it_okay | exact |
| 0x4718d0 | mp_trade_complete | likely |
| 0x4719e0 | mp_handle_it_okay | likely (nested state test) |
| 0x471a70 | mp_trade_cancel | exact |
| 0x471b30 | mp_handle_it_cncl | exact |
| 0x471bb0 | mp_send_it_qury | exact |
| 0x471c50 | mp_handle_it_qury | exact |
| 0x471cd0 | mp_send_it_stat | exact |
| 0x471d70 | mp_handle_it_stat | exact |

26 functions: 16 exact, 10 likely.

Facts: states 0 none, 1 requested, 2 open, 3 we accepted, 4 partner accepted; a changed offer drops 3 / 4 back
to 2. 15 s timeout, 5 s query; trade is cancelled when cur_character is dead / hidden (flags 0x8a) or, while
open, not in AI mode 1. Requests are refused (IT_DENY 1, "busy") unless idle with no screen open; 0 ("unable")
when dead / hidden. Removing gold from the offer removes all of it, any other item one at a time. IT_STAT
carries the sender's state, but the handler ignores it. Clients send IT_RQST / DENY / ACPT reliably, the rest
unreliably.

## multichat.cpp (0x487b90-0x488223; .data 0x59c180-0x59c1cc; .bss 0x22fedd8-0x2302fdf)

| Address | Function | Confidence |
|---|---|---|
| 0x487b90 | chat_cmd_kick | exact |
| 0x487c10 | chat_cmd_ping | likely (host / else-if client form) |
| 0x487d00 | chat_cmd_list | exact |
| 0x487db0 | chat_history_clear_thunk | likely (a one-call function: jmp) |
| 0x487dc0 | chat_history_clear | exact |
| 0x487dd0 | chat_history_clear_thunk2 | likely |
| 0x487de0 | chat_history_pop | exact |
| 0x487e20 | chat_format_entry | exact |
| 0x487ea0 | chat_process_command | likely |
| 0x488030 | chat_history_push | exact |
| 0x488070 | chat_send_line | exact |
| 0x488130 | chat_build_packet | exact |
| 0x4881a0 | mp_handle_chat_line | exact |

13 functions: 9 exact, 4 likely.

Facts: commands /kick (host only), /ping, /list, /msg name text (shown as `<name> text` only to the target and
the sender; a /msg to an unknown name is swallowed). Game lines (sender -1) are `** text **`, players'
`[name] text`. The ring holds 63 lines (64 slots, one kept free); a full ring drops new lines silently.

## multicontainer.cpp (0x488c00-0x48972a; no .data, no .bss)

| Address | Function | Confidence |
|---|---|---|
| 0x488c00 | mp_broadcast_container_gold | exact |
| 0x488c50 | mp_handle_item_get | exact |
| 0x488ce0 | mp_send_container_open | exact |
| 0x488d50 | mp_handle_container_open | likely (address of a local in the argument slot) |
| 0x488da0 | mp_send_container_close | exact |
| 0x488eb0 | mp_handle_container_close | likely |
| 0x488fc0 | container_spawn_loot | likely |
| 0x489120 | mp_handle_item_drop | unsure (inline vector / matrix copies, running item pointer) |
| 0x489430 | container_take_gold_server | exact |
| 0x489480 | container_request_open | exact |
| 0x4894b0 | container_lockpick | exact |
| 0x4895e0 | mp_send_lockpick_result | exact |
| 0x489660 | mp_handle_container_lockpick | likely (if-chain) |

13 functions: 8 exact, 4 likely, 1 unsure.

Facts: lock picking has no random roll: success when Pick Lock skill + 0.05 >= the container's lock value.
Drops: one gold item stays an item; anything else becomes a container 0.2 above the first item; sound 13 if a
weapon is among the items, else 12. The handlers print "CONTAINER POS ..." and "Pick success!" with printf.

## Argument orders corrected against sum.symbols comments

- container_take_gold_server is (player id, handle) (0x489432 reads the handle from the second argument);
  the comment says "(handle, player)".
- mp_player_file_read_summary is (out, filename) (already noted in multi.h).
- mp_broadcast_container_gold sends ITM_GET (0x24), (player id, item handle); its name is historical.

## Game bugs noticed

1. **mp_handle_container_close (0x488f00):** every "taken item" is looked up from the container's net handle
   (data + 4) instead of the item handles after the count, so container_remove_item gets the container
   itself: on the other machines the taken items stay in the container and an emptied container is not
   deleted. (Unchecked in play.)
2. **mp_trade_complete (0x471914):** party_inventory_find_type_index's -1 is not checked; if an offered item
   left the inventory during the trade (used, dropped, sold), the count of party_inventory[-1] (0x970818,
   just before the array) is decremented and party_inventory_remove(-1, 1, 1) may run.
3. **mp_handle_container_lockpick (0x4896a2):** looks the container up with obj_from_handle on the host's
   handle, where mp_handle_container_open / item_get use mp_obj_from_net_handle; on a client this can miss or
   hit another object, so the lock is not cleared there. (Depends on client and host handles differing.)
4. **container_spawn_loot (0x4890c3):** for a single gold drop it reads container::mesh_item_type (+0xe0) of
   the gold item_obj (0xac bytes): a read past the object, sent but ignored by the receiver.
5. **mp_handle_it_addi:** no check against 16 entries in mp_trade_remote_items (the sender limits its own
   list, so only a modified peer could overflow it). mp_handle_item_drop likewise trusts the count (16 slots)
   and leaves slots unset for out-of-range types while the later loops index by count.

## Open questions

- multitrade .bss: the initialiser entries (216 for 0x22e47c4, 217 for 0x22e47c0) put the timeout timer
  first, the addresses the request timer; defined in initialiser order. 0x22e46f4 (4 bytes after
  mp_trade_state) is never referenced.
- inet_proxy_server's size: 0xa5a09c-0xa5a29b is 0x200 bytes plus 4 to s3d.cpp's 0xa5a2a0; 0x200 assumed.
- chat_history_clear_thunk / _thunk2: written as two functions that call chat_history_clear (each compiles
  to a jmp); they could also be one function folded by the linker, but two addresses are called
  (mp_session_reset, mp_leave_game).
- mp_handle_item_drop: the vectors (1,0,0) / (0,1,0) / (0,0,1) are stored inline and the matrix temporary is
  copied member by member, although vector.h declares the vector3 constructors out of line (0x43b580): either
  the original vector3 / matrix copy members are inline (and the 0x4xxxxx copies are COMDATs of the
  unoptimised units), or the code is written differently. Same for `c->pos = first->pos` and the dequantized
  position.
- container_spawn_loot / mp_handle_item_drop: the fourth argument (monster_drop_loot's [ebp-8]) is passed
  through to container_obj_create's second argument; its meaning is not known.
- interface_set_screen(4, &c) in mp_handle_container_open passes a container** (the container screen's
  argument type is not checked).
