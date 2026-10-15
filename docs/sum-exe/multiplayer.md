---
title: "Multiplayer packets"
parent: Sum.exe
nav_order: 6.8
---

# Multiplayer packets

**Summary:** `mp_packet_table` (`0x5951b8`) lists all 66 multiplayer packet types. Each 0x1c-byte record
(`mp_packet_info` in `symbols/types.h`) holds the type, the receive handler, a flag byte, a send counter that the
senders increment, an 8-byte name (at most seven letters and a NUL) and four net-rate bytes. Ghidra had merged 48 of
the handlers into the functions before them. The table gave their real starts. All handlers are named in
`symbols/sum.symbols` (2026-10-14, agent naming pass, checked against the table; re-checked 2026-10-15). Lobby, chat
and tracker traffic (PXO: IRC, game and user trackers) uses separate code (`irc_*`, `gtrack_*`, `utrack_*`). The
decompiled units are `src/decomp/game/unnamed_netsession_0x440b40.cpp` (session states, player records) and
`src/decomp/game/unnamed_netpackets_0x441820.cpp` (receive, send and forward; placeholder names, D3309/D3312);
the NEW_MON and SRV_REQ handlers, the player lookups, the net-handle map helpers (`mp_net_handle_map`
`0x22e94f4`), the position quantising and the NEW_LEP orientation packing are in
`src/decomp/game/unnamed_goals_doors_0x475180.cpp` (D3800). Only a multiplayer client sends SRV_REQ
(`ai_service_request` `0x4766f0`), and the only NEW_LEP sender is the `load_mlayout` console command
(`monster_layout_load` → `mp_send_new_lep`), whose orientation packing is wrong (game bug K230).

- **Flag column:** `multi_process_packet` (`0x441820`) always calls the handler when the sender's address is in the
  player list; flag 1 lets it run for an unlisted sender as well (`cmp byte ptr [ebp + 0x5951c0], 1` at `0x441959`).
  That is why the join and query packets have it.
- **Net-rate bytes** (`+0x18`, `+0x19`): `[0]` marks the types with net rate info, `[1]` whether its display is on
  (toggled by the `disp_rate` console command). Both are set for SWI_PTH to SWI_BSS, only `[0]` for SWI_SNK and
  SWI_HID. The display itself is compiled out (`debug_return_zero`).
- **Range check off by one:** the type test is `cmp cl, 0x42; ja` (`0x441869`), so type 0x42 passes and reads
  record 66, which is the string "multi_process_packet: Invalid packet type..." after the table. DMY_PCK (0x40) has
  no handler and is called anyway. A single forged datagram of either type crashes the receiver
  ([Bugs](game-bugs.md), C201). The packet's own size word is not checked against the datagram length.
- **Transport:** only `multi_send_packet` (`0x442060`) uses the engine's UDP "reliable sockets" (`nw_NewSendReliable`;
  received with `nw_NewReceiveReliable`; see [vsdk: OS layer](vsdk-os.md#networking-0x517590-0x5194ab)).
  `multi_send_packet_buffered` (`0x441e90`) sends plain datagrams with `nw_send`: the 500 ms GM_STAT / CT_STAT status
  reports, OBJ_UPD, SWI2DIR, COM_SEQ, ACPT_JN and the host's buffered forwards (OBJ_UPD, ADJ_SKL, DISABLD, SWI2DIR,
  SWI_SNK, SWI_HID, SMN_QCK, COM_SEQ). `mp_send_tracked` (`0x441da0`) also uses `nw_send`, for JN_RQST, DENY_JN and
  the game info reply. Both kinds go through one 0x204-byte buffer per player and kind, flushed when a packet does not
  fit and at the end of every `mp_game_frame`.
- **Forwarding:** on the host, `mp_forward_cur_packet` (`0x4422c0`, reliable) and `mp_forward_cur_packet_buffered`
  (`0x442230`) resend the packet being handled to every other connected player except its sender.

| Type | Name | Handler | Flag |
|---|---|---|---|
| `0x0` | GM_QURY | `0x4ab1c0` `mp_handle_game_query` | 1 |
| `0x1` | ACTV_GM | `0x470860` `mp_handle_actv_gm` | 1 |
| `0x2` | JN_RQST | `0x4ab350` `mp_handle_join_request` | 1 |
| `0x3` | ACPT_JN | `0x470a80` `mp_handle_acpt_jn` | 1 |
| `0x4` | DENY_JN | `0x470b60` `mp_handle_deny_jn` | 1 |
| `0x5` | NET_INF | `0x4af760` `mp_handle_net_info` | 1 |
| `0x6` | NEW_INF | `0x4af4b0` `mp_handle_new_info` | 0 |
| `0x7` | PLY_DAT | `0x4af990` `mp_handle_player_data` | 0 |
| `0x8` | GM_OVER | `0x481590` `mp_handle_game_over` | 0 |
| `0x9` | LV_GAME | `0x4815c0` `mp_handle_leave_game` | 0 |
| `0xa` | GM_STAT | `0x441000` `mp_handle_gm_stat` | 0 |
| `0xb` | CT_STAT | `0x441210` `mp_handle_ct_stat` | 0 |
| `0xc` | CHAT_LN | `0x4881a0` `mp_handle_chat_line` | 1 |
| `0xd` | OBJ_HND | `0x47fcc0` `mp_handle_obj_hnd` | 0 |
| `0xe` | NEW_LEP | `0x47ff10` `mp_handle_new_lep` | 0 |
| `0xf` | DEL_LEP | `0x480000` `mp_handle_delete_entity` | 0 |
| `0x10` | NEW_PTH | `0x4801f0` `mp_handle_new_path` | 0 |
| `0x11` | OBJ_UPD | `0x4806a0` `mp_handle_obj_update` | 0 |
| `0x12` | ADJ_HPS | `0x4809b0` `mp_handle_adjust_hp` | 0 |
| `0x13` | ADJ_APS | `0x480a90` `mp_handle_adjust_ap` | 0 |
| `0x14` | ADJ_EXP | `0x480b60` `mp_handle_adjust_xp` | 0 |
| `0x15` | ADJ_SKL | `0x480c60` `mp_handle_character_skills` | 0 |
| `0x16` | SET_STS | `0x480d70` `mp_handle_status_add` | 0 |
| `0x17` | CLR_STA | `0x480e60` `mp_handle_status_cure` | 0 |
| `0x18` | DISABLD | `0x480f60` `mp_handle_ai_disable` | 0 |
| `0x19` | RESTART | `0x481050` `mp_handle_respawn` | 0 |
| `0x1a` | FORCEPR | `0x481150` `mp_handle_forcepr` | 0 |
| `0x1b` | SWI_PTH | `0x4882f0` `mp_handle_swi_path` | 0 |
| `0x1c` | SWI_ATK | `0x488450` `mp_handle_swi_attack` | 0 |
| `0x1d` | SWI2DIR | `0x4885f0` `mp_handle_swi_2dir` | 0 |
| `0x1e` | SWI!DIR | `0x4887e0` `mp_handle_swi_ndir` | 0 |
| `0x1f` | SWI_BSS | `0x4889c0` `mp_handle_swi_boss` | 0 |
| `0x20` | SWI_SNK | `0x488ab0` `mp_handle_swi_sneak` | 0 |
| `0x21` | SWI_HID | `0x488ba0` `mp_handle_swi_hide` | 0 |
| `0x22` | COM_SEQ | `0x4b1280` `mp_handle_combat_sequence` | 0 |
| `0x23` | ITM_DRP | `0x489120` `mp_handle_item_drop` | 0 |
| `0x24` | ITM_GET | `0x488c50` `mp_handle_item_get` | 0 |
| `0x25` | CNT_OPN | `0x488d50` `mp_handle_container_open` | 0 |
| `0x26` | CNT_CLS | `0x488eb0` `mp_handle_container_close` | 0 |
| `0x27` | CNT_PCK | `0x489660` `mp_handle_container_lockpick` | 0 |
| `0x28` | DOOR_TG | `0x4aa780` `mp_handle_door_toggle` | 0 |
| `0x29` | DOR_PCK | `0x4aa7d0` `mp_handle_door_lockpick` | 0 |
| `0x2a` | IT_RQST | `0x471040` `mp_handle_it_rqst` | 0 |
| `0x2b` | IT_DENY | `0x4711c0` `mp_handle_it_deny` | 0 |
| `0x2c` | IT_ACPT | `0x4712f0` `mp_handle_it_acpt` | 0 |
| `0x2d` | IT_ADDI | `0x4714a0` `mp_handle_it_addi` | 0 |
| `0x2e` | IT_RMVI | `0x4716f0` `mp_handle_it_rmvi` | 0 |
| `0x2f` | IT_OKAY | `0x4719e0` `mp_handle_it_okay` | 0 |
| `0x30` | IT_CNCL | `0x471b30` `mp_handle_it_cncl` | 0 |
| `0x31` | IT_QURY | `0x471c50` `mp_handle_it_qury` | 0 |
| `0x32` | IT_STAT | `0x471d70` `mp_handle_it_stat` | 0 |
| `0x33` | INV_EQP | `0x487950` `mp_handle_equip_item` | 0 |
| `0x34` | CST_ANM | `0x4b0210` `mp_handle_cast_anim` | 0 |
| `0x35` | RLS_SPL | `0x4b04a0` `mp_handle_release_spell` | 0 |
| `0x36` | SMN_RQS | `0x4b0700` `mp_handle_summon_request` | 0 |
| `0x37` | SMN_DNY | `0x4b0640` `mp_handle_summon_deny` | 0 |
| `0x38` | SMN_ALD | `0x4b06a0` `mp_handle_summon_allowed` | 0 |
| `0x39` | SMN_HND | `0x4b0820` `mp_handle_summon_handle` | 0 |
| `0x3a` | SMN_QCK | `0x4b08f0` `mp_handle_summon_quick` | 0 |
| `0x3b` | KICK_PL | `0x471f90` `mp_handle_kick_pl` | 0 |
| `0x3c` | L_CMPLT | `0x4b09c0` `mp_handle_level_complete` | 0 |
| `0x3d` | D_CMPLT | `0x4b0ab0` `mp_handle_difficulty_complete` | 0 |
| `0x3e` | NEW_MON | `0x475610` `mp_handle_new_mon` | 0 |
| `0x3f` | SRV_REQ | `0x4767a0` `mp_handle_srv_req` | 0 |
| `0x40` | DMY_PCK | `0x0` (none; `multi_process_packet` calls it anyway, C201) | 0 |
| `0x41` | INV_UQP | `0x487b10` `mp_handle_unequip_item` | 0 |
