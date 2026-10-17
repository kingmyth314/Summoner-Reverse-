// unnamed_netpackets_0x441820.cpp - Sum.exe v1.40, code 0x441820-0x4425a6: the multiplayer packet layer.
// mp_packet_table (66 types), the dispatch of a received datagram or stream chunk, the receive loop with
// its three nested buffers, the per-player reliable and unreliable send buffers (0x200 bytes, flushed when
// full or on request), the host's broadcasts and forwards, the per-second net statistics and the disp_rate
// console command of the compiled-out net-rate display.
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl block, sum.symbols module "-"). docs/sum-exe/source-map.md "Hand cuts inside the unnamed
// units" keeps 0x440b40-0x442380 as one unit; the .data shows two objects (D3309). Rename the file when
// the name turns up.
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .data 0x5951b8-0x595b23 (mp_packet_table, then the literals), .bss 0x651890-0x65e967 (characterinfo.cpp's
// .bss starts at 0x65e968).
//
// The rate display: each sender and multi_process_packet build "OUT U " / "OUT R " / "INC U " / "INC R "
// plus the type name and pass it to debug_return_zero, a compiled-out hook that returns 0, so the "Unable
// to add net rate info" sprintf runs for every packet of a type with rate info (the SWI_* types).

#include <string.h>
#include <stdio.h>
#include "vsdk/os/console.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/frametime.h"
#include "vsdk/net/net.h"
#include "summoner/summoner.h"
#include "summoner/multi/multi.h"

#define MP_RATE_NAME_LEN    16        // "INC R " + the 7-letter type name
#define MP_RATE_PREFIX_LEN  6

// ---- .data (0x5951b8) ----------------------------------------------------------------------------------
// {type, receive handler, handled from an unknown sender, send counter, name, net-rate flags}.
mp_packet_info mp_packet_table[MP_NUM_PACKET_TYPES] = {   // 0x5951b8
    { 0x00, mp_handle_game_query,            1, { 0 }, 0, "GM_QURY", { 0, 0 } },
    { 0x01, mp_handle_actv_gm,               1, { 0 }, 0, "ACTV_GM", { 0, 0 } },
    { 0x02, mp_handle_join_request,          1, { 0 }, 0, "JN_RQST", { 0, 0 } },
    { 0x03, mp_handle_acpt_jn,               1, { 0 }, 0, "ACPT_JN", { 0, 0 } },
    { 0x04, mp_handle_deny_jn,               1, { 0 }, 0, "DENY_JN", { 0, 0 } },
    { 0x05, mp_handle_net_info,              1, { 0 }, 0, "NET_INF", { 0, 0 } },
    { 0x06, mp_handle_new_info,              0, { 0 }, 0, "NEW_INF", { 0, 0 } },
    { 0x07, mp_handle_player_data,           0, { 0 }, 0, "PLY_DAT", { 0, 0 } },
    { 0x08, mp_handle_game_over,             0, { 0 }, 0, "GM_OVER", { 0, 0 } },
    { 0x09, mp_handle_leave_game,            0, { 0 }, 0, "LV_GAME", { 0, 0 } },
    { 0x0a, mp_handle_gm_stat,               0, { 0 }, 0, "GM_STAT", { 0, 0 } },
    { 0x0b, mp_handle_ct_stat,               0, { 0 }, 0, "CT_STAT", { 0, 0 } },
    { 0x0c, mp_handle_chat_line,             1, { 0 }, 0, "CHAT_LN", { 0, 0 } },
    { 0x0d, mp_handle_obj_hnd,               0, { 0 }, 0, "OBJ_HND", { 0, 0 } },
    { 0x0e, mp_handle_new_lep,               0, { 0 }, 0, "NEW_LEP", { 0, 0 } },
    { 0x0f, mp_handle_delete_entity,         0, { 0 }, 0, "DEL_LEP", { 0, 0 } },
    { 0x10, mp_handle_new_path,              0, { 0 }, 0, "NEW_PTH", { 0, 0 } },
    { 0x11, mp_handle_obj_update,            0, { 0 }, 0, "OBJ_UPD", { 0, 0 } },
    { 0x12, mp_handle_adjust_hp,             0, { 0 }, 0, "ADJ_HPS", { 0, 0 } },
    { 0x13, mp_handle_adjust_ap,             0, { 0 }, 0, "ADJ_APS", { 0, 0 } },
    { 0x14, mp_handle_adjust_xp,             0, { 0 }, 0, "ADJ_EXP", { 0, 0 } },
    { 0x15, mp_handle_character_skills,      0, { 0 }, 0, "ADJ_SKL", { 0, 0 } },
    { 0x16, mp_handle_status_add,            0, { 0 }, 0, "SET_STS", { 0, 0 } },
    { 0x17, mp_handle_status_cure,           0, { 0 }, 0, "CLR_STA", { 0, 0 } },
    { 0x18, mp_handle_ai_disable,            0, { 0 }, 0, "DISABLD", { 0, 0 } },
    { 0x19, mp_handle_respawn,               0, { 0 }, 0, "RESTART", { 0, 0 } },
    { 0x1a, mp_handle_forcepr,               0, { 0 }, 0, "FORCEPR", { 0, 0 } },
    { 0x1b, mp_handle_swi_path,              0, { 0 }, 0, "SWI_PTH", { 1, 1 } },
    { 0x1c, mp_handle_swi_attack,            0, { 0 }, 0, "SWI_ATK", { 1, 1 } },
    { 0x1d, mp_handle_swi_2dir,              0, { 0 }, 0, "SWI2DIR", { 1, 1 } },
    { 0x1e, mp_handle_swi_ndir,              0, { 0 }, 0, "SWI!DIR", { 1, 1 } },
    { 0x1f, mp_handle_swi_boss,              0, { 0 }, 0, "SWI_BSS", { 1, 1 } },
    { 0x20, mp_handle_swi_sneak,             0, { 0 }, 0, "SWI_SNK", { 1, 0 } },
    { 0x21, mp_handle_swi_hide,              0, { 0 }, 0, "SWI_HID", { 1, 0 } },
    { 0x22, mp_handle_combat_sequence,       0, { 0 }, 0, "COM_SEQ", { 0, 0 } },
    { 0x23, mp_handle_item_drop,             0, { 0 }, 0, "ITM_DRP", { 0, 0 } },
    { 0x24, mp_handle_item_get,              0, { 0 }, 0, "ITM_GET", { 0, 0 } },
    { 0x25, mp_handle_container_open,        0, { 0 }, 0, "CNT_OPN", { 0, 0 } },
    { 0x26, mp_handle_container_close,       0, { 0 }, 0, "CNT_CLS", { 0, 0 } },
    { 0x27, mp_handle_container_lockpick,    0, { 0 }, 0, "CNT_PCK", { 0, 0 } },
    { 0x28, mp_handle_door_toggle,           0, { 0 }, 0, "DOOR_TG", { 0, 0 } },
    { 0x29, mp_handle_door_lockpick,         0, { 0 }, 0, "DOR_PCK", { 0, 0 } },
    { 0x2a, mp_handle_it_rqst,               0, { 0 }, 0, "IT_RQST", { 0, 0 } },
    { 0x2b, mp_handle_it_deny,               0, { 0 }, 0, "IT_DENY", { 0, 0 } },
    { 0x2c, mp_handle_it_acpt,               0, { 0 }, 0, "IT_ACPT", { 0, 0 } },
    { 0x2d, mp_handle_it_addi,               0, { 0 }, 0, "IT_ADDI", { 0, 0 } },
    { 0x2e, mp_handle_it_rmvi,               0, { 0 }, 0, "IT_RMVI", { 0, 0 } },
    { 0x2f, mp_handle_it_okay,               0, { 0 }, 0, "IT_OKAY", { 0, 0 } },
    { 0x30, mp_handle_it_cncl,               0, { 0 }, 0, "IT_CNCL", { 0, 0 } },
    { 0x31, mp_handle_it_qury,               0, { 0 }, 0, "IT_QURY", { 0, 0 } },
    { 0x32, mp_handle_it_stat,               0, { 0 }, 0, "IT_STAT", { 0, 0 } },
    { 0x33, mp_handle_equip_item,            0, { 0 }, 0, "INV_EQP", { 0, 0 } },
    { 0x34, mp_handle_cast_anim,             0, { 0 }, 0, "CST_ANM", { 0, 0 } },
    { 0x35, mp_handle_release_spell,         0, { 0 }, 0, "RLS_SPL", { 0, 0 } },
    { 0x36, mp_handle_summon_request,        0, { 0 }, 0, "SMN_RQS", { 0, 0 } },
    { 0x37, mp_handle_summon_deny,           0, { 0 }, 0, "SMN_DNY", { 0, 0 } },
    { 0x38, mp_handle_summon_allowed,        0, { 0 }, 0, "SMN_ALD", { 0, 0 } },
    { 0x39, mp_handle_summon_handle,         0, { 0 }, 0, "SMN_HND", { 0, 0 } },
    { 0x3a, mp_handle_summon_quick,          0, { 0 }, 0, "SMN_QCK", { 0, 0 } },
    { 0x3b, mp_handle_kick_pl,               0, { 0 }, 0, "KICK_PL", { 0, 0 } },
    { 0x3c, mp_handle_level_complete,        0, { 0 }, 0, "L_CMPLT", { 0, 0 } },
    { 0x3d, mp_handle_difficulty_complete,   0, { 0 }, 0, "D_CMPLT", { 0, 0 } },
    { 0x3e, mp_handle_new_mon,               0, { 0 }, 0, "NEW_MON", { 0, 0 } },
    { 0x3f, mp_handle_srv_req,               0, { 0 }, 0, "SRV_REQ", { 0, 0 } },
    { 0x40, NULL,                            0, { 0 }, 0, "DMY_PCK", { 0, 0 } },
    { 0x41, mp_handle_unequip_item,          0, { 0 }, 0, "INV_UQP", { 0, 0 } },
};

// ---- .bss (0x651890) -----------------------------------------------------------------------------------
int net_stats_recv_bytes;                                     // 0x651890
ubyte multi_recv_buffers[MP_NUM_RECV_BUFFERS][MP_RECV_BUFFER_SIZE];   // 0x651894
int net_stats_history_index;                                  // 0x65d894
ubyte multi_recv_buffer_index;                                // 0x65d898
ubyte *mp_cur_packet;                                         // 0x65d8a0
ushort mp_cur_packet_size;                                    // 0x65d8a4
float net_stats_timer;                                        // 0x65d8a8
mp_send_buffer multi_reliable_send_bufs[MP_MAX_PLAYERS];      // 0x65d8b0
// ccmd_disp_rate (0x65e0c0) is defined at its initialiser, before cmd_disp_rate (below).
int net_stats_recv_history[MP_NET_STATS_HISTORY];             // 0x65e0cc
int net_stats_sent_packets;                                   // 0x65e0f4
int net_stats_avg_sent_packets;                               // 0x65e0f8
int net_stats_sent_bytes;                                     // 0x65e0fc
mp_send_buffer multi_unreliable_send_bufs[MP_MAX_PLAYERS];    // 0x65e100
int net_stats_sent_packets_history[MP_NET_STATS_HISTORY];     // 0x65e910
int net_stats_avg_recv_bytes;                                 // 0x65e938
int net_stats_sent_bytes_history[MP_NET_STATS_HISTORY];       // 0x65e93c
int net_stats_avg_sent_bytes;                                 // 0x65e964

// 0x441820  Confidence: likely (the loop variable is the offset; the invalid-type exit returns without
// clearing mp_cur_packet). Each packet's handler runs when the sender is a listed player or the type's
// flag allows strangers. Game bugs (D3310): the type check is `> 66`, so type 0x42 reads the record after
// the table (the literal "multi_process_packet: ...": flag 'o', handler "i_pr" = 0x72705f69) and calls that
// address for a listed sender; DMY_PCK (0x40) has a NULL handler, also called for a listed sender. The
// packet sizes are not checked against len.
void multi_process_packet(ubyte *data, int len, void *from, bool reliable)
{
    char rate_name[MP_RATE_NAME_LEN];
    mp_player *sender;
    ubyte type;
    int offset;

    mp_cur_packet = NULL;
    mp_cur_packet_size = 0;
    sender = mp_player_find_by_addr(from, true);
    for (offset = 0; offset < len; offset += mp_cur_packet_size + MP_HEADER_SIZE) {
        mp_cur_packet = data + offset;
        type = mp_cur_packet[0];
        mp_cur_packet_size = *(ushort *)(mp_cur_packet + 1);
        if (type > MP_NUM_PACKET_TYPES) {
            sprintf(string_buffer, "multi_process_packet: Invalid packet type: %d\n", type);
            return;
        }
        sprintf(string_buffer, "Processing packet type: %d\n", type);
        if (mp_packet_table[type].rate_flags[0]) {
            strcpy(rate_name, reliable ? "INC R " : "INC U ");
            strcpy(rate_name + MP_RATE_PREFIX_LEN, mp_packet_table[type].name);
            if (!debug_return_zero(0, rate_name, mp_cur_packet_size + MP_HEADER_SIZE,
                                   mp_packet_table[type].rate_flags[1]))
                sprintf(string_buffer, "Unable to add net rate info for type; %s\n", rate_name);
        }
        if (sender || mp_packet_table[type].flag == 1)
            mp_packet_table[type].handler(mp_cur_packet + MP_HEADER_SIZE, from);
    }
    mp_cur_packet = NULL;
    mp_cur_packet_size = 0;
}

// 0x4419d0  Confidence: likely (the three memsets and the scalar stores; the timer's store sits between
// the second and third memset).
void mp_reset_net_stats()
{
    memset(net_stats_recv_history, 0, sizeof(net_stats_recv_history));
    memset(net_stats_sent_bytes_history, 0, sizeof(net_stats_sent_bytes_history));
    net_stats_timer = 0.0f;
    memset(net_stats_sent_packets_history, 0, sizeof(net_stats_sent_packets_history));
    net_stats_avg_sent_packets = 0;
    net_stats_avg_sent_bytes = 0;
    net_stats_avg_recv_bytes = 0;
    net_stats_history_index = 0;
    net_stats_sent_packets = 0;
    net_stats_sent_bytes = 0;
    net_stats_recv_bytes = 0;
}

// 0x441a30  Confidence: exact.
void mp_broadcast_packet_buffered(ubyte *data, int len, bool flush)
{
    mp_player *p;

    if (!MP_IS_HOST())
        return;
    for (p = mp_player_list.next; p != &mp_player_list; p = p->next) {
        if (p != mp_local_player && p->socket != -1)
            multi_send_packet_buffered(data, len, p, flush);
    }
}

// 0x441a90  Confidence: exact. The player a NEW_INF just announced (MP_PF_INFO_SENT) misses the next
// broadcast with skip_flagged, which clears the flag (also the flush broadcast of mp_game_frame).
void mp_broadcast_packet(ubyte *data, int len, bool skip_flagged)
{
    mp_player *p;

    if (!MP_IS_HOST())
        return;
    for (p = mp_player_list.next; p != &mp_player_list; p = p->next) {
        if (p != mp_local_player && p->socket != -1) {
            if (skip_flagged && (p->flags & MP_PF_INFO_SENT))
                p->flags &= ~MP_PF_INFO_SENT;
            else
                multi_send_packet(data, len, p, true);
        }
    }
}

// 0x441b10  Confidence: exact (VC6 byte match, match23; was likely; the "safe" walk of the player list reads next before the end test; the
// reliable loops are rotated `while (socket != -1 && receive > 0)`). The buffer index nests the calls (a
// handler may receive again); the per-second statistics roll at the outermost call.
void multi_process_incoming()
{
    net_addr from;
    ubyte *buf = multi_recv_buffers[multi_recv_buffer_index];
    mp_player *p;
    mp_player *next;
    int len;

    if (multi_recv_buffer_index == 0)
        mp_net_stats_update();
    multi_recv_buffer_index++;
    while ((len = nw_receive(buf, &from)) > 0) {
        multi_process_packet(buf, len, &from, false);
        net_stats_recv_bytes += len;
    }
    if (MP_IS_HOST()) {
        for (p = mp_player_list.next, next = p->next; p != &mp_player_list; p = next, next = p->next) {
            if (p == mp_local_player)
                continue;
            while (p->socket != -1 && (len = nw_NewReceiveReliable(p->socket, buf, MP_RECV_BUFFER_SIZE)) > 0) {
                multi_process_packet(buf, len, p->addr, true);
                net_stats_recv_bytes += len;
            }
        }
    } else {
        while (mp_local_player && mp_local_player->socket != -1
               && (len = nw_NewReceiveReliable(mp_local_player->socket, buf, MP_RECV_BUFFER_SIZE)) > 0) {
            multi_process_packet(buf, len, mp_server_addr, true);
            net_stats_recv_bytes += len;
        }
    }
    multi_recv_buffer_index--;
}

// 0x441ca0  Confidence: exact (VC6 byte match, match23; was likely; the three sums share one loop; the divisions by 10 are magic multiplies).
// Once a second: this second's counts into the 10-slot histories, the averages, and the counters reset.
// The byte averages are stored crosswise (D3307).
void mp_net_stats_update()
{
    int sent_packets;
    int sent_bytes;
    int recv_bytes;
    int i;

    net_stats_timer += reported_frametime;
    if (net_stats_timer < 1.0f)
        return;
    net_stats_sent_packets_history[net_stats_history_index] = net_stats_sent_packets;
    net_stats_sent_bytes_history[net_stats_history_index] = net_stats_sent_bytes;
    net_stats_recv_history[net_stats_history_index] = net_stats_recv_bytes;
    net_stats_history_index = (net_stats_history_index + 1) % MP_NET_STATS_HISTORY;
    sent_packets = 0;
    sent_bytes = 0;
    recv_bytes = 0;
    for (i = 0; i < MP_NET_STATS_HISTORY; i++) {
        sent_packets += net_stats_sent_packets_history[i];
        sent_bytes += net_stats_sent_bytes_history[i];
        recv_bytes += net_stats_recv_history[i];
    }
    net_stats_sent_packets = 0;
    net_stats_avg_sent_packets = sent_packets / MP_NET_STATS_HISTORY;
    net_stats_sent_bytes = 0;
    net_stats_recv_bytes = 0;
    net_stats_avg_sent_bytes = sent_bytes / MP_NET_STATS_HISTORY;
    net_stats_timer = 0.0f;
    net_stats_avg_recv_bytes = recv_bytes / MP_NET_STATS_HISTORY;
}

// 0x441da0  Confidence: likely (the type byte is kept in the dead first-argument slot). An immediate
// datagram to an address; the rate name is built with "OUT U " but no counter changes.
void mp_send_tracked(ubyte *data, int len, net_addr to)
{
    char rate_name[MP_RATE_NAME_LEN];
    ubyte type;

    if (data) {
        type = data[0];
        if (mp_packet_table[type].rate_flags[0]) {
            strcpy(rate_name, "OUT U ");
            strcpy(rate_name + MP_RATE_PREFIX_LEN, mp_packet_table[type].name);
            if (!debug_return_zero(0, rate_name, len, mp_packet_table[type].rate_flags[1]))
                sprintf(string_buffer, "Unable to add net rate info for type; %s\n", rate_name);
        }
    }
    nw_send(&to, data, len);
}

// 0x441e90  Confidence: likely (the address is copied into a local: from the server's on a client, else
// from the record). A client's buffer is the server player id's.
void multi_send_packet_buffered(ubyte *data, int len, mp_player *to, bool flush)
{
    char rate_name[MP_RATE_NAME_LEN];
    net_addr addr;
    mp_send_buffer *buf;
    ubyte type;
    int id;

    if (data) {
        type = data[0];
        if (mp_packet_table[type].rate_flags[0]) {
            strcpy(rate_name, "OUT U ");
            strcpy(rate_name + MP_RATE_PREFIX_LEN, mp_packet_table[type].name);
            if (!debug_return_zero(0, rate_name, len, mp_packet_table[type].rate_flags[1]))
                sprintf(string_buffer, "Unable to add net rate info for type; %s\n", rate_name);
        }
        net_stats_sent_packets++;
    }
    if (MP_IS_CLIENT()) {
        addr = *(net_addr *)mp_server_addr;
        id = mp_server_player_id;
    } else {
        addr = *(net_addr *)to->addr;
        id = to->id;
    }
    buf = &multi_unreliable_send_bufs[id];
    if (buf->len + len > MP_MAX_PACKET) {
        nw_send(&addr, buf->data, buf->len);
        net_stats_sent_bytes += buf->len;
        buf->len = 0;
    }
    if (len > 0) {
        memcpy(buf->data + buf->len, data, len);
        buf->len += len;
    }
    if (flush && buf->len > 0) {
        nw_send(&addr, buf->data, buf->len);
        net_stats_sent_bytes += buf->len;
        buf->len = 0;
    }
}

// 0x442060  Confidence: likely (the packet counter is bumped on both paths of the rate test). Always over
// the reliable socket; a client uses the local record's socket (the link to the server) and buffer 0.
void multi_send_packet(ubyte *data, int len, mp_player *to, bool flush)
{
    char rate_name[MP_RATE_NAME_LEN];
    mp_send_buffer *buf;
    ubyte type;
    int socket;
    int id;

    if (data) {
        type = data[0];
        if (mp_packet_table[type].rate_flags[0]) {
            strcpy(rate_name, "OUT R ");
            strcpy(rate_name + MP_RATE_PREFIX_LEN, mp_packet_table[type].name);
            if (!debug_return_zero(0, rate_name, len, mp_packet_table[type].rate_flags[1]))
                sprintf(string_buffer, "Unable to add net rate info for type; %s\n", rate_name);
        }
        net_stats_sent_packets++;
    }
    if (MP_IS_CLIENT()) {
        socket = mp_local_player->socket;
        id = 0;
    } else {
        socket = to->socket;
        id = to->id;
    }
    buf = &multi_reliable_send_bufs[id];
    if (buf->len + len > MP_MAX_PACKET) {
        nw_NewSendReliable(socket, buf->data, buf->len);
        net_stats_sent_bytes += buf->len;
        buf->len = 0;
    }
    if (len > 0) {
        memcpy(buf->data + buf->len, data, len);
        buf->len += len;
    }
    if (flush && buf->len > 0) {
        nw_NewSendReliable(socket, buf->data, buf->len);
        net_stats_sent_bytes += buf->len;
        buf->len = 0;
    }
}

// 0x442230  Confidence: exact. The host relays the packet being handled to everybody but its sender.
void mp_forward_cur_packet_buffered(void *from)
{
    mp_player *sender;
    mp_player *p;

    if (!MP_IS_HOST() || !mp_cur_packet)
        return;
    sender = mp_player_find_by_addr(from, true);
    if (!sender)
        return;
    for (p = mp_player_list.next; p != &mp_player_list; p = p->next) {
        if (p != mp_local_player && p->socket != -1 && p != sender)
            multi_send_packet_buffered(mp_cur_packet, mp_cur_packet_size + MP_HEADER_SIZE, p, true);
    }
}

// 0x4422c0  Confidence: exact.
void mp_forward_cur_packet(void *from)
{
    mp_player *sender;
    mp_player *p;

    if (!MP_IS_HOST() || !mp_cur_packet)
        return;
    sender = mp_player_find_by_addr(from, true);
    if (!sender)
        return;
    for (p = mp_player_list.next; p != &mp_player_list; p = p->next) {
        if (p != mp_local_player && p->socket != -1 && p != sender)
            multi_send_packet(mp_cur_packet, mp_cur_packet_size + MP_HEADER_SIZE, p, true);
    }
}

// 0x442350  init_ccmd_disp_rate_thunk (static_initializers entry 158, jmp). Confidence: exact
// (compiler-generated).
// 0x442360  init_ccmd_disp_rate: console_command::console_command("disp_rate", ...). Confidence: exact
// (compiler-generated).
console_command ccmd_disp_rate("disp_rate", "Toggle whether net rate info for a packet is displayed", cmd_disp_rate);   // 0x65e0c0

// 0x442380  Confidence: likely (the help branch is entered by a jump past its test after
// doing_help_for_comand = 1). Toggles a type's rate display (the four hooks are told); refuses everything,
// help and status included, in single player.
void cmd_disp_rate()
{
    char rate_name[MP_RATE_NAME_LEN];
    mp_packet_info *t;
    int i;

    if (!(game_flags & GF_MULTIPLAYER)) {
        print_to_console("This function cannot be used in single player mode.", 0);
        return;
    }
    if (calling_a_command_function) {
        process_command(ARG_NONE | ARG_STRING);
        if (arg_type & ARG_STRING) {
            for (i = 0; i < MP_NUM_PACKET_TYPES; i++) {
                if (_stricmp(mp_packet_table[i].name, next_arg) == 0)
                    break;
            }
            if (i == MP_NUM_PACKET_TYPES) {
                print_to_console("Invalid packet abbreviation\n\n", 0);
                doing_help_for_comand = 1;
            } else {
                t = &mp_packet_table[i];
                t->rate_flags[1] = !t->rate_flags[1];
                strcpy(rate_name + MP_RATE_PREFIX_LEN, t->name);
                strncpy(rate_name, "OUT U ", MP_RATE_PREFIX_LEN);
                debug_noop_disp_rate(0, rate_name, t->rate_flags[1]);
                strncpy(rate_name, "OUT R ", MP_RATE_PREFIX_LEN);
                debug_noop_disp_rate(0, rate_name, t->rate_flags[1]);
                strncpy(rate_name, "INC U ", MP_RATE_PREFIX_LEN);
                debug_noop_disp_rate(0, rate_name, t->rate_flags[1]);
                strncpy(rate_name, "INC R ", MP_RATE_PREFIX_LEN);
                debug_noop_disp_rate(0, rate_name, t->rate_flags[1]);
                sprintf(string_buffer, t->rate_flags[1] ? "Net rate info for %s is now ON."
                                                        : "Net rate info for %s is now OFF.", t->name);
                print_to_console(string_buffer, 0);
            }
        } else {
            doing_help_for_comand = 1;
        }
    } else if (checking_status_for_command) {
        print_to_console("Net rate info for the following packets are being displayed:\n", 0);
        for (i = 0; i < MP_NUM_PACKET_TYPES; i++) {
            if (mp_packet_table[i].rate_flags[1])
                print_to_console(mp_packet_table[i].name, 0);
        }
    }
    if (doing_help_for_comand) {
        print_to_console("Usage: disp_rate <packet>, where <packet> is the abbreviation of the appropriate packet.\n", 0);
        print_to_console("Allowable packet abbreviations:\n", 0);
        for (i = 0; i < MP_NUM_PACKET_TYPES; i++) {
            if (mp_packet_table[i].rate_flags[0])
                print_to_console(mp_packet_table[i].name, 0);
        }
    }
}
