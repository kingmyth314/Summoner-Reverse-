// summoner/multi/unnamed_mp_join_0x4ab0f0.h - the join / tracker unit 0x4ab0f0-0x4abb73 (the host's game
// info and join handling, the level change of a network game, the PXO game tracker glue). Source:
// game/unnamed_mp_join_0x4ab0f0.cpp. THE FILE NAME IS A PLACEHOLDER (decomp71). The functions of this unit
// that multi.h already declared stay there (mp_join_timer_reset, chat_history_pump, mp_unpause, tracker_init,
// tracker_start_game, tracker_end_game, tracker_frame, mp_change_level, mp_handle_game_query,
// mp_handle_join_request, mp_join_accept_player / _timer, tracker_game_channel / tracker_filter_channel).
// Not original source.
#ifndef SUMMONER_MULTI_UNNAMED_MP_JOIN_0X4AB0F0_H
#define SUMMONER_MULTI_UNNAMED_MP_JOIN_0X4AB0F0_H

#include "vsdk/vsdk.h"
#include "vsdk/net/net.h"
#include "summoner/multi/multi.h"
#include "summoner/multi/unnamed_gtrack_0x4ae330.h"

// Packet types of mp_packet_table entries 1, 3 and 4 (names from the table).
#define MP_ACTV_GM 0x01               // the game info reply (mp_build_game_info)
#define MP_ACPT_JN 0x03
#define MP_DENY_JN 0x04

// The ACTV_GM version bytes (two bytes of 9; mp_handle_actv_gm reads the trailing state bytes only from
// version 7 on). Name ours.
#define MP_GAME_INFO_VERSION 9

// JN_RQST deny reasons (mp_join_result 3..13 on the client; names ours).
#define MP_DENY_FULL          3
#define MP_DENY_PASSWORD      4
#define MP_DENY_NOT_OPEN      5       // mp_netgame_state != 1
#define MP_DENY_NO_RECORD     6       // mp_player_alloc failed
#define MP_DENY_JOIN_PENDING  7
#define MP_DENY_VERSION       8       // checksum not accepted
#define MP_DENY_LEVEL_LOW     9
#define MP_DENY_LEVEL_HIGH    10
#define MP_DENY_DIFFICULTY    11      // the difficulty is not unlocked for the character
#define MP_DENY_LEVEL_LOCKED  12      // mp_player_level_available

// A JN_RQST as mp_handle_join_request unpacks it (0x48 bytes, passed by value to mp_accept_join_request;
// the sender is mp_send_jn_rqst 0x470bc0). Type and field names ours.
struct mp_join_request {
    char name[0x20];                  // +0x00
    char password[0x10];              // +0x20
    int  char_info;                   // +0x30 the sender's mp_player +0x2c
    int  profile_flags;               // +0x34 ... +0x30
    int  level;                       // +0x38 character level (0x22e4558, a short on the client)
    int  _unk3c;                      // +0x3c ... mp_player +0x20
    int  _unk40;                      // +0x40 the client's 0x68e1bc
    unsigned int checksum;            // +0x44 the client sends the constant 0xf174baca (D5210)
};
LAYOUT_CHECK(mp_join_request, sizeof(mp_join_request) == 0x48);

// .data 0x59d3c4: the checksums a JN_RQST may carry besides the host's items_table_checksum. Names ours.
extern unsigned int mp_join_accepted_checksums[2];       // 0x59d3c4 {0xf174baca, 0x6e4c08c4}
extern int mp_join_num_accepted_checksums;               // 0x59d3cc 2

extern gtrack_game_info tracker_game;                    // 0x24794c8 the hosted game's tracker record
extern int tracker_initialized;                          // 0x2479720 tracker_init succeeded

void mp_build_game_info(net_addr *to);                   // 0x4ab1f0 ACTV_GM to one address
void mp_send_deny_join(int reason, net_addr to);         // 0x4ab580 DENY_JN
void mp_accept_join_request(mp_join_request req, net_addr from);   // 0x4ab5d0
void mp_send_accept_join(mp_player *p);                  // 0x4ab710 ACPT_JN
void mp_goto_wait_state();                               // 0x4ab8e0
void tracker_request_games();                            // 0x4aba70 (mp_gamelist_net_frame's caller 0x4705f0)
void tracker_query_listed_games(gtrack_game_list *list); // 0x4abaf0

// Other units (decomp71: no header declared it).
void mp_send_gm_qury(net_addr *to);                      // 0x470b70 GM_QURY (game list unit; return unused)

#endif
