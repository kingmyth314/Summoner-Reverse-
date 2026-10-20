// unnamed_gamelist_0x4705b0.cpp - Sum.exe v1.40, code 0x4705b0-0x470e2f (the client side of the network
// game list: the game queries, the ACTV_GM replies collected into a list, the join request and its
// answers). THE FILE NAME IS A PLACEHOLDER: no path string, no strings.tbl block (source-map.md row
// 0x4705b0-0x470e20). Written from the Sum.exe disassembly (tools/asm.mjs) by decomp76. Target: VC++ 6.0
// (cl 12.00.8966), /O2. Confidence per function: exact (control flow, calls, constants and globals match),
// likely (one detail of statement order or expression form is a guess), unsure (the shape is a guess).
//
// Unit: the static initialiser run 0x4705b0-0x4705ef (entries 214-215, the two RealtimeTimers) opens it;
// .bss 0x22e4428-0x22e462f (multitrade.cpp's starts at 0x22e4630); no .data of its own (the packet
// counters are mp_packet_table's). The game list screen (new_gamelist.cpp) reads the list and the join
// result; the host side of the protocol is unnamed_mp_join_0x4ab0f0.cpp.

#include <string.h>
#include "vsdk/os/timer.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/net/net.h"
#include "summoner/summoner.h"
#include "summoner/multi/multi.h"
#include "summoner/multi/multisave.h"
#include "summoner/multi/unnamed_netpackets_0x441820.h"
#include "summoner/multi/unnamed_netsession_0x440b40.h"
#include "summoner/multi/unnamed_mp_join_0x4ab0f0.h"
#include "summoner/multi/unnamed_gamelist_0x4705b0.h"

// ---- .bss (0x22e4428-0x22e462f, address order) --------------------------------------------------------
int mp_join_result;                           // 0x22e4428
// 0x4705b0 init_realtimetimer_22e442c_thunk (jmp, static_initializers entry 214). Confidence: exact.
// 0x4705c0 init_realtimetimer_22e442c (mov ecx, 0x22e442c / jmp RealtimeTimer__RealtimeTimer). Confidence: exact.
RealtimeTimer mp_join_timer;                  // 0x22e442c
mp_char_summary mp_join_character;            // 0x22e4430
bool mp_gamelist_refresh_pending;             // 0x22e455c
mp_gamelist_entry *mp_join_target;            // 0x22e4560
int mp_gamelist_request_time;                 // 0x22e4564
mp_gamelist_entry *mp_gamelist_entries;       // 0x22e4568
mp_gamelist_entry mp_gamelist_used;           // 0x22e4570
// 0x4705d0 init_realtimetimer_22e45c0_thunk (jmp, entry 215). Confidence: exact.
// 0x4705e0 init_realtimetimer_22e45c0 (mov ecx, 0x22e45c0 / jmp RealtimeTimer__RealtimeTimer). Confidence: exact.
RealtimeTimer mp_join_retry_timer;            // 0x22e45c0
mp_gamelist_entry mp_gamelist_free_list;      // 0x22e45c8
int mp_gamelist_count;                        // 0x22e4618
bool mp_gamelist_active;                      // 0x22e461c
char mp_join_password[0x10];                  // 0x22e4620

// 0x4705f0  Confidence: exact. State 12's network frame.
void mp_gamelist_net_frame()
{
    nw_receive_all();
    mp_gamelist_refresh();
}

// 0x470600  Confidence: exact (VC6 byte match; the free loop reads the next node before the end test). A pending
// refresh
// empties the list and queries the LAN (a broadcast GM_QURY, pxo_cfg bit 1) and / or the PXO tracker
// (bit 0). While a join is under way (local status 1) the request is resent every 7 s until it is
// answered or 28 s pass (result 0xd).
void mp_gamelist_refresh()
{
    ubyte data[MP_MAX_PACKET];
    mp_gamelist_entry *e, *next;

    if (mp_gamelist_refresh_pending) {
        e = mp_gamelist_used.next;
        next = e->next;
        while (e != &mp_gamelist_used) {
            mp_gamelist_free_entry(e);
            e = next;
            next = e->next;
        }
        if (pxo_cfg_flags & 2) {
            data[0] = MP_GM_QURY;
            *(ushort *)(data + 1) = 0;
            mp_packet_table[MP_GM_QURY].sent++;
            nw_send_broadcast(mp_net_port, data, MP_HEADER_SIZE);
        }
        if (pxo_cfg_flags & 1)
            tracker_request_games();
        mp_gamelist_refresh_pending = false;
        mp_gamelist_request_time = timer_get(1000);
    }
    if (mp_local_player->type == 1) {
        if (mp_join_retry_timer.elapsed()) {
            mp_join_result = 0xd;
            mp_join_reset();
            return;
        }
        if (mp_join_timer.elapsed()) {
            mp_send_jn_rqst(mp_join_target, NULL, NULL, false);
            mp_join_timer.SetTimeout(7000);
        }
    }
}

// 0x470700  Confidence: exact. Also the DENY_JN handler's tail.
void mp_join_reset()
{
    mp_join_timer.reset();
    mp_join_retry_timer.reset();
    mp_set_local_status(0);
    mp_local_player->flags = 0;
    mp_join_target = NULL;
}

// 0x470740  Confidence: exact.
void mp_gamelist_free_entry(mp_gamelist_entry *e)
{
    if (e == mp_join_target)
        mp_join_target = NULL;
    list_remove(e);
    list_append(&mp_gamelist_free_list, e);
    mp_gamelist_count--;
}

// 0x470790  Confidence: exact (VC6 byte match; the node pointer is a local). State 12 enter.
void mp_gamelist_init()
{
    int i;
    mp_gamelist_entry *e;

    mp_join_reset();
    list_init(&mp_gamelist_free_list);
    list_init(&mp_gamelist_used);
    mp_gamelist_entries = new mp_gamelist_entry[MP_GAMELIST_MAX_GAMES];
    mp_gamelist_count = 0;
    if (mp_gamelist_entries) {
        for (i = 0; i < MP_GAMELIST_MAX_GAMES; i++) {
            e = &mp_gamelist_entries[i];
            list_append(&mp_gamelist_free_list, e);
        }
    }
    mp_gamelist_request_refresh();
    mp_gamelist_active = true;
}

// 0x470820  Confidence: exact. State 12 leave (the free list head is not reset).
void mp_gamelist_free()
{
    if (mp_gamelist_entries) {
        delete[] mp_gamelist_entries;
        mp_gamelist_entries = NULL;
    }
    mp_gamelist_active = false;
    list_init(&mp_gamelist_used);
}

// 0x470860  Confidence: likely (the ping clamp as a ternary gives the binary's two stores, D15703; the list
// unlink still differs: the binary keeps &e->prev in EAX; VC6 keeps the sender pointer in ebx and spills the version byte; the reads
// go through memcpy, which keeps the six ints in memory as the original does). ACTV_GM {version, min
// version, name\0, level, players, max players, difficulty, lowest and highest character level, from
// version 7 on the session state and the connection speed bytes} from a server not yet listed goes into a
// free node at the end of the list, with the ping since the query. Packets with version and min version
// both off (below 9 and above 9) are dropped. The name goes through a local copy.
void mp_handle_actv_gm(ubyte *data, void *from)
{
    ubyte version, min_version;
    ubyte state, speed;
    int level, num_players, max_players, difficulty, min_level, max_level;
    char name[0x20];
    int off;
    mp_gamelist_entry *e;

    if (!mp_gamelist_active)
        return;
    state = 0;
    version = data[0];
    min_version = data[1];
    strcpy(name, (char *)data + 2);
    speed = 0;
    off = (ushort)(strlen((char *)data + 2) + 3);
    memcpy(&level, data + off, 4);
    off += 4;
    memcpy(&num_players, data + off, 4);
    off += 4;
    memcpy(&max_players, data + off, 4);
    off += 4;
    memcpy(&difficulty, data + off, 4);
    off += 4;
    memcpy(&min_level, data + off, 4);
    off += 4;
    memcpy(&max_level, data + off, 4);
    off += 4;
    if (version >= 7) {
        memcpy(&state, data + off, 1);
        memcpy(&speed, data + off + 1, 1);
    }
    if (version < MP_GAME_INFO_VERSION && min_version > MP_GAME_INFO_VERSION)
        return;
    for (e = mp_gamelist_used.next; e != &mp_gamelist_used; e = e->next) {
        if (nw_addr_equal(e->addr, from, true) == 0)
            break;
    }
    if (e != &mp_gamelist_used)
        return;
    e = mp_gamelist_free_list.next;
    if (e == &mp_gamelist_free_list)
        return;
    list_remove(e);
    list_append(&mp_gamelist_used, e);
    mp_gamelist_count++;
    strcpy(e->name, name);
    e->max_players = (ubyte)max_players;
    e->level = level;
    e->num_players = (ubyte)num_players;
    e->max_level = (ubyte)max_level;
    e->difficulty = difficulty;
    e->min_level = (ubyte)min_level;
    memcpy(e->addr, from, 8);
    e->ping = timer_get(1000) - mp_gamelist_request_time;
    e->ping = e->ping > 0 ? e->ping : 0;
    e->last_seen = timer_get(1000);
    if (version >= 7) {
        e->status = state;
        e->connection = speed;
    }
}

// 0x470a80  Confidence: exact (VC6 byte match). ACPT_JN {our player id, the server's player id, level scale} while
// joining:
// the reliable link to the server, then the netgame block from the joined list entry, session state 1 and
// game state 14.
void mp_handle_acpt_jn(ubyte *data, void *from)
{
    int id, server_id;

    if (mp_local_player->type != 1)
        return;
    id = ((int *)data)[0];
    server_id = ((int *)data)[1];
    mp_netgame_level_scale = *(float *)(data + 8);
    nw_ConnectToServer(&mp_local_player->socket, (net_addr *)from);
    if (mp_local_player->socket == -1)
        return;
    mp_join_result = 2;
    mp_local_player->id = id;
    mp_set_local_status(2);
    strcpy(mp_netgame_name, mp_join_target->name);
    mp_netgame_level = mp_join_target->level;
    mp_netgame_num_players = 1;
    mp_netgame_max_players = mp_join_target->max_players;
    mp_game_difficulty = mp_join_target->difficulty;
    mp_netgame_state = 0;
    mp_server_player_id = server_id;
    memcpy(mp_server_addr, mp_join_target->addr, 8);
    mp_set_session_state(1);
    gameseq_set_state(14);
}

// 0x470b60  Confidence: exact (VC6 byte match, match24; was likely) (the reason is read with memcpy into a local, D8302). DENY_JN {reason}.
void mp_handle_deny_jn(ubyte *data, void *from)
{
    int reason;
    memcpy(&reason, data, 4);
    mp_join_result = reason;
    mp_join_reset();
}

// 0x470b70  Confidence: exact (VC6 byte match, cl 8966; was likely: the order of the header stores and the
// counter). GM_QURY (no data) to one address
// (the tracker's listed games).
void mp_send_gm_qury(net_addr *to)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    MP_BUILD_HEADER(MP_GM_QURY);
    MP_END_PACKET();
    nw_send(to, data, packet_size);
}

// 0x470bb0  Confidence: exact.
void mp_gamelist_request_refresh()
{
    mp_gamelist_refresh_pending = true;
}

// 0x470bc0  Confidence: exact (VC6 byte match, cl 8966; was likely: VC6 loads game after the memset setup; same
// size). JN_RQST {name\0, password\0,
// char_info, profile
// flags, character level, the player's +0x20, the int at 0x68e1bc, the checksum 0xf174baca} to the game's
// server. The character and the password are kept for the resends (mp_gamelist_refresh passes NULL);
// retry arms the 28 s give-up timer (SIGNATURE: bool, was int; the byte is tested, D5807).
void mp_send_jn_rqst(mp_gamelist_entry *game, const void *character, const char *password, bool retry)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    ushort len;
    mp_join_request req;

    if (character)
        mp_join_character = *(const mp_char_summary *)character;
    if (password) {
        strncpy(mp_join_password, password, 0xf);
        mp_join_password[0xf] = 0;
    }
    mp_join_result = 1;
    mp_join_target = game;
    memset(&req, 0, sizeof(req));
    strcpy(req.name, mp_local_player->name);
    strcpy(req.password, mp_join_password);
    req.char_info = mp_local_player->char_info;
    req.profile_flags = mp_local_player->profile_flags;
    req.level = mp_join_character.level;
    req._unk3c = mp_local_player->_unk20;
    req._unk40 = mp_join_unk_68e1bc;
    req.checksum = 0xf174baca;
    mp_set_local_status(1);
    MP_BUILD_HEADER(MP_JN_RQST);
    len = (ushort)strlen(req.name);
    memcpy(data + packet_size, req.name, len);
    packet_size += len;
    data[packet_size++] = 0;
    len = (ushort)strlen(req.password);
    memcpy(data + packet_size, req.password, len);
    packet_size += len;
    data[packet_size++] = 0;
    MP_ADD_DATA(req.char_info);
    MP_ADD_DATA(req.profile_flags);
    MP_ADD_DATA(req.level);
    MP_ADD_DATA(req._unk3c);
    MP_ADD_DATA(req._unk40);
    MP_ADD_DATA(req.checksum);
    MP_END_PACKET();
    mp_send_tracked(data, packet_size, *(net_addr *)game->addr);
    mp_join_timer.SetTimeout(7000);
    if (retry)
        mp_join_retry_timer.SetTimeout(28000);
}

// 0x470e10  Confidence: exact.
int mp_join_get_result()
{
    return mp_join_result;
}

// 0x470e20  Confidence: exact.
void mp_join_clear_result()
{
    mp_join_result = 0;
}
