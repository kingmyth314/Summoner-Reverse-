// summoner/multi/unnamed_netsession_0x440b40.h - the multiplayer session (0x440b40-0x441817): the netgame
// block, the player records and lists, the host / client frames, the session state machine (GM_STAT /
// CT_STAT), the local player. Source: game/unnamed_netsession_0x440b40.cpp. THE FILE NAME IS A PLACEHOLDER
// (no assert path, no strings.tbl block; decomp53). Included by summoner/multi/multi.h. The declarations
// of this unit that were in multi.h moved here. Not original source.
#ifndef SUMMONER_MULTI_UNNAMED_NETSESSION_0X440B40_H
#define SUMMONER_MULTI_UNNAMED_NETSESSION_0X440B40_H

#include "vsdk/vsdk.h"
#include "vsdk/os/timestamp.h"
// mp_player must be complete for mp_players[]; multi.h includes this header at its end (guarded).
#include "summoner/multi/multi.h"

// The netgame block (0x6c bytes at 0x6513a8). mp_host_game clears it with one memset of 0x6c bytes
// (rep stosd, 0x1b dwords from mp_netgame_name), so it is one object (D3305); type and object names ours.
// The old global names stay as macros (as multisave.h does for pxo_cfg).
struct mp_netgame_info {
    char name[0x20];                  // +0x00 0x6513a8 game name
    char password[0x10];              // +0x20 0x6513c8
    int  level;                       // +0x30 0x6513d8 selected level index
    int  _unk34;                      // +0x34 0x6513dc (no direct reference)
    unsigned int flags;               // +0x38 0x6513e0 bit 2: a join is pending (mp_accept_join_request)
    int  num_players;                 // +0x3c 0x6513e4 (decomp37: the HP / gold / xp multiplier)
    int  max_players;                 // +0x40 0x6513e8 at most 4
    int  difficulty;                  // +0x44 0x6513ec 0..2
    int  host_level;                  // +0x48 0x6513f0 the host's character level
    int  level_below;                 // +0x4c 0x6513f4 allowed levels below the host's
    int  level_above;                 // +0x50 0x6513f8 ... above
    int  state;                       // +0x54 0x6513fc session state 0..5 (5 = playing)
    int  _unk58;                      // +0x58 0x651400 (no direct reference)
    uint server_addr[2];              // +0x5c 0x651404 (mp_handle_acpt_jn)
    int  server_player_id;            // +0x64 0x65140c
    float level_scale;                // +0x68 0x651410 frand_range(0.8, 1.2) of mp_host_game
};
LAYOUT_CHECK(mp_netgame_info, sizeof(mp_netgame_info) == 0x6c);
extern mp_netgame_info mp_netgame;                      // 0x6513a8
#define mp_netgame_name         (mp_netgame.name)
#define mp_netgame_password     (mp_netgame.password)
#define mp_netgame_level        (mp_netgame.level)
#define mp_netgame_flags        (mp_netgame.flags)
#define mp_netgame_num_players  (mp_netgame.num_players)
#define mp_netgame_max_players  (mp_netgame.max_players)
#define mp_game_difficulty      (mp_netgame.difficulty)
#define mp_netgame_host_level   (mp_netgame.host_level)
#define mp_netgame_level_below  (mp_netgame.level_below)
#define mp_netgame_level_above  (mp_netgame.level_above)
#define mp_netgame_state        (mp_netgame.state)
#define mp_server_addr          (mp_netgame.server_addr)
#define mp_server_player_id     (mp_netgame.server_player_id)
#define mp_netgame_level_scale  (mp_netgame.level_scale)
#define MP_NETGAME_JOIN_PENDING 0x4

// mp_player::type values the session code tests or sets (names ours).
#define MP_STATUS_LISTED        3     // on the level select screen
#define MP_STATUS_LOADING       4     // from here on the 500 ms status reports run
#define MP_STATUS_ENTERED       5     // state_game_enter
#define MP_STATUS_LINKING       6
#define MP_STATUS_LINKED        7     // every player's PLY_DAT is in
#define MP_STATUS_OBJECTS       8
#define MP_STATUS_READY         9     // every character has a net handle
#define MP_STATUS_PLAYING       10

extern RealtimeTimer mp_update_timer;                   // 0x651414 500 ms status reports
extern mp_player mp_player_list;                        // 0x651418 head
extern mp_player mp_player_free_list;                   // 0x6514d8 head
extern int mp_unk_6514d4;                              // 0x6514d4 between the list heads, no reference (name ours)
extern unsigned short mp_net_port;                      // 0x651594 UDP port, 10234 unless -port (cmdline_read_params)
extern mp_player *mp_local_player;                      // 0x651598
extern int mp_unk_65159c;                              // 0x65159c no reference (name ours)
extern mp_player mp_players[MP_MAX_PLAYERS];            // 0x6515a0

void mp_client_check_connection();                      // 0x440c00
void mp_server_check_clients();                         // 0x440c40
void mp_server_accept_connections();                    // 0x440d20
void mp_host_update_session();                          // 0x440d70
void mp_server_frame();                                 // 0x440e90
void mp_client_frame();                                 // 0x440ee0
// Host: GM_STAT (session state, players, level / entry point / host level, pause) to every player.
// decomp51 (game_pause / game_unpause) declared it in multi.h: send_now picks mp_broadcast_packet (reliable,
// sent at once) over mp_broadcast_packet_buffered (sum.symbols name kept). Moved here (decomp53).
void multi_set_netgame_state(bool send_now);            // 0x440f20
void mp_handle_gm_stat(ubyte *data, void *from);        // 0x441000 GM_STAT
// CT_STAT (id, status) of p: a client sends it to the server, the host broadcasts it.
void multi_process_client_status(mp_player *p, bool reliable);   // 0x441150 (state_game_enter: mp_local_player, 1)
void mp_handle_ct_stat(ubyte *data, void *from);        // 0x441210 CT_STAT
void mp_game_frame();                                   // 0x4412a0 (state_game_frame)
void multi_do_frame();                                  // 0x4413d0 (LoadingScreenDraw)
void mp_session_reset();                                // 0x441490
void mp_level_start();                                  // 0x441530 (state_game_enter)
void pxo_read_config_thunk();                           // 0x441560 jmp pxo_read_config (GameInit)
void mp_set_session_state(int state);                   // 0x441570
void mp_set_local_status(int status);                   // 0x4415d0 (state_game_enter: 5)
mp_player *mp_player_alloc();                           // 0x441610 NULL when the free list is empty
void mp_player_disconnect(mp_player *p);                // 0x441660 party_remove_player, closes +0x18, unlinks
// character_xp_death_penalty (0x441710) is also this unit's; it stays declared in living_entity.h.
void mp_local_player_create();                          // 0x441760

#endif
