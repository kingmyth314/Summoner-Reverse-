// summoner/multi/unnamed_gamelist_0x4705b0.h - the client's game list and join request unit 0x4705b0-0x470e2f
// (source game/unnamed_gamelist_0x4705b0.cpp, decomp76; THE FILE NAME IS A PLACEHOLDER: no path string, no
// strings.tbl block). The functions multi.h and unnamed_mp_join_0x4ab0f0.h declared already stay there
// (mp_gamelist_net_frame, mp_gamelist_init, mp_gamelist_free, mp_handle_actv_gm, mp_handle_acpt_jn,
// mp_handle_deny_jn, mp_send_gm_qury, mp_gamelist_request_refresh, mp_send_jn_rqst, mp_join_get_result,
// mp_join_clear_result, mp_gamelist_used, mp_gamelist_count). Not original source.
#ifndef SUMMONER_MULTI_UNNAMED_GAMELIST_0X4705B0_H
#define SUMMONER_MULTI_UNNAMED_GAMELIST_0X4705B0_H

#include "vsdk/vsdk.h"
#include "vsdk/os/timestamp.h"
#include "summoner/multi/multi.h"
#include "summoner/multi/multisave.h"

#define MP_GM_QURY 0x00               // mp_packet_table entry 0 (the game query; names from the table)
#define MP_JN_RQST 0x02
#define MP_GAMELIST_MAX_GAMES 100     // the node block of mp_gamelist_init (0x1f40 bytes)

// .bss 0x22e4428-0x22e462f (sum.symbols names; the character and password copies are ours).
extern int mp_join_result;                              // 0x22e4428 (see multi.h)
extern RealtimeTimer mp_join_timer;                     // 0x22e442c 7 s between join requests
extern mp_char_summary mp_join_character;               // 0x22e4430 the character the request sends (name ours)
extern bool mp_gamelist_refresh_pending;                // 0x22e455c
extern mp_gamelist_entry *mp_join_target;               // 0x22e4560 the game being joined
extern int mp_gamelist_request_time;                    // 0x22e4564 the queries' send time (ping base)
extern mp_gamelist_entry *mp_gamelist_entries;          // 0x22e4568 the node block
extern RealtimeTimer mp_join_retry_timer;               // 0x22e45c0 28 s: give up (mp_join_result 0xd)
extern mp_gamelist_entry mp_gamelist_free_list;         // 0x22e45c8 head
extern bool mp_gamelist_active;                         // 0x22e461c ACTV_GM is ignored while false
extern char mp_join_password[0x10];                     // 0x22e4620 (name ours)
// The int the join request sends in its +0x40 field (no sum.symbols entry; characterinfo.cpp's .bss by address).
extern int mp_join_unk_68e1bc;                          // 0x68e1bc (name ours)

void mp_gamelist_refresh();                             // 0x470600 (the tail of mp_gamelist_net_frame)
void mp_join_reset();                                   // 0x470700
void mp_gamelist_free_entry(mp_gamelist_entry *e);      // 0x470740

#endif
