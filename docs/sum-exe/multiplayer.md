---
title: "Multiplayer packets"
parent: Sum.exe
nav_order: 6.8
---

# Multiplayer packets

**Summary:** `mp_packet_table` (`0x5951b8`) lists all 66 multiplayer packet types. Each 0x1c-byte record
(`mp_packet_info` in `symbols/types.h`) holds the type, the receive handler, a flag, a send counter that the senders
increment, and an eight-letter name. Ghidra had merged 48 of the handlers into the functions before them. The table
gave their real starts, and they are named in `symbols/sum.symbols` (2026-10-14, agent naming pass, checked against the
table). Lobby, chat and tracker traffic (PXO: IRC, game and user trackers) uses separate code (`irc_*`, `gtrack_*`,
`utrack_*`).

| Type | Name | Handler | Flag |
|---|---|---|---|
| `0x0` | GM_QURY | `0x4ab1c0` `mp_handle_game_query` | 1 |
| `0x1` | ACTV_GM | `0x470860`  | 1 |
| `0x2` | JN_RQST | `0x4ab350` `mp_handle_join_request` | 1 |
| `0x3` | ACPT_JN | `0x470a80`  | 1 |
| `0x4` | DENY_JN | `0x470b60`  | 1 |
| `0x5` | NET_INF | `0x4af760` `mp_handle_net_info` | 1 |
| `0x6` | NEW_INF | `0x4af4b0` `mp_handle_new_info` | 0 |
| `0x7` | PLY_DAT | `0x4af990` `mp_handle_player_data` | 0 |
| `0x8` | GM_OVER | `0x481590` `mp_handle_game_over` | 0 |
| `0x9` | LV_GAME | `0x4815c0` `mp_handle_leave_game` | 0 |
| `0xa` | GM_STAT | `0x441000`  | 0 |
| `0xb` | CT_STAT | `0x441210`  | 0 |
| `0xc` | CHAT_LN | `0x4881a0` `mp_handle_chat_line` | 1 |
| `0xd` | OBJ_HND | `0x47fcc0`  | 0 |
| `0xe` | NEW_LEP | `0x47ff10`  | 0 |
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
| `0x2a` | IT_RQST | `0x471040`  | 0 |
| `0x2b` | IT_DENY | `0x4711c0`  | 0 |
| `0x2c` | IT_ACPT | `0x4712f0`  | 0 |
| `0x2d` | IT_ADDI | `0x4714a0`  | 0 |
| `0x2e` | IT_RMVI | `0x4716f0`  | 0 |
| `0x2f` | IT_OKAY | `0x4719e0`  | 0 |
| `0x30` | IT_CNCL | `0x471b30`  | 0 |
| `0x31` | IT_QURY | `0x471c50`  | 0 |
| `0x32` | IT_STAT | `0x471d70`  | 0 |
| `0x33` | INV_EQP | `0x487950` `mp_handle_equip_item` | 0 |
| `0x34` | CST_ANM | `0x4b0210` `mp_handle_cast_anim` | 0 |
| `0x35` | RLS_SPL | `0x4b04a0` `mp_handle_release_spell` | 0 |
| `0x36` | SMN_RQS | `0x4b0700` `mp_handle_summon_request` | 0 |
| `0x37` | SMN_DNY | `0x4b0640` `mp_handle_summon_deny` | 0 |
| `0x38` | SMN_ALD | `0x4b06a0` `mp_handle_summon_allowed` | 0 |
| `0x39` | SMN_HND | `0x4b0820` `mp_handle_summon_handle` | 0 |
| `0x3a` | SMN_QCK | `0x4b08f0` `mp_handle_summon_quick` | 0 |
| `0x3b` | KICK_PL | `0x471f90`  | 0 |
| `0x3c` | L_CMPLT | `0x4b09c0` `mp_handle_level_complete` | 0 |
| `0x3d` | D_CMPLT | `0x4b0ab0` `mp_handle_difficulty_complete` | 0 |
| `0x3e` | NEW_MON | `0x475610` `mp_handle_new_mon` | 0 |
| `0x3f` | SRV_REQ | `0x4767a0`  | 0 |
| `0x40` | DMY_PCK | `0x0`  | 0 |
| `0x41` | INV_UQP | `0x487b10` `mp_handle_unequip_item` | 0 |
