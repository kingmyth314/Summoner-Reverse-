// summoner/multi/unnamed_netpackets_0x441820.h - the multiplayer packet layer (0x441820-0x4425a6):
// mp_packet_table, the receive loop and dispatch, the per-player send buffers, the broadcasts and
// forwards, the net statistics and the disp_rate console command. Source: game/unnamed_netpackets_0x441820.cpp.
// THE FILE NAME IS A PLACEHOLDER (no assert path, no strings.tbl block; decomp53). Included by
// summoner/multi/multi.h; the send / forward declarations moved here from multi.h. Not original source.
#ifndef SUMMONER_MULTI_UNNAMED_NETPACKETS_0X441820_H
#define SUMMONER_MULTI_UNNAMED_NETPACKETS_0X441820_H

#include "vsdk/vsdk.h"
#include "vsdk/net/net.h"
// mp_player / mp_packet_info must be complete; multi.h includes this header at its end (guarded).
#include "summoner/multi/multi.h"

class console_command;

#define MP_RECV_BUFFER_SIZE 0x4000
#define MP_NUM_RECV_BUFFERS 3         // multi_recv_buffer_index selects one per nesting level
#define MP_NET_STATS_HISTORY 10

// One per-player send buffer (0x204 bytes; sum.symbols multi_reliable_send_bufs / multi_unreliable_send_bufs,
// indexed by mp_player::id). Type name ours.
struct mp_send_buffer {
    int len;                          // +0x000
    ubyte data[MP_MAX_PACKET];        // +0x004
};
LAYOUT_CHECK(mp_send_buffer, sizeof(mp_send_buffer) == 0x204);

// .data
extern mp_packet_info mp_packet_table[MP_NUM_PACKET_TYPES];   // 0x5951b8
// .bss
extern int net_stats_recv_bytes;                        // 0x651890
extern ubyte multi_recv_buffers[MP_NUM_RECV_BUFFERS][MP_RECV_BUFFER_SIZE];   // 0x651894 (name ours)
extern int net_stats_history_index;                     // 0x65d894
extern ubyte multi_recv_buffer_index;                   // 0x65d898
extern ubyte *mp_cur_packet;                            // 0x65d8a0 the packet being handled (type, size, data)
extern ushort mp_cur_packet_size;                       // 0x65d8a4 its data size
extern float net_stats_timer;                           // 0x65d8a8
extern mp_send_buffer multi_reliable_send_bufs[MP_MAX_PLAYERS];     // 0x65d8b0
extern console_command ccmd_disp_rate;                  // 0x65e0c0
extern int net_stats_recv_history[MP_NET_STATS_HISTORY];   // 0x65e0cc
extern int net_stats_sent_packets;                      // 0x65e0f4
extern int net_stats_avg_sent_packets;                  // 0x65e0f8
extern int net_stats_sent_bytes;                        // 0x65e0fc
extern mp_send_buffer multi_unreliable_send_bufs[MP_MAX_PLAYERS];   // 0x65e100
extern int net_stats_sent_packets_history[MP_NET_STATS_HISTORY];   // 0x65e910
// The two byte averages are filled crosswise (D3307): avg_sent_bytes gets the received bytes' average,
// avg_recv_bytes the sent bytes'. Nothing reads either.
extern int net_stats_avg_sent_bytes;                    // 0x65e938
extern int net_stats_sent_bytes_history[MP_NET_STATS_HISTORY];   // 0x65e93c
extern int net_stats_avg_recv_bytes;                    // 0x65e964

// (data, total length, sender address, reliable): every packet of the datagram / stream chunk.
void multi_process_packet(ubyte *data, int len, void *from, bool reliable);   // 0x441820
void mp_reset_net_stats();                              // 0x4419d0
// Host: multi_send_packet_buffered to every other connected player. (data, total length, flush)
void mp_broadcast_packet_buffered(ubyte *data, int len, bool flush);          // 0x441a30
// Host: multi_send_packet to every other connected player; with skip_flagged a player with
// MP_PF_INFO_SENT only has the flag cleared. (data, total length, skip_flagged) (D3301)
void mp_broadcast_packet(ubyte *data, int len, bool skip_flagged);            // 0x441a90
void multi_process_incoming();                          // 0x441b10
void mp_net_stats_update();                             // 0x441ca0
// An unbuffered datagram to an address (the game list and join replies). (data, total length, address)
void mp_send_tracked(ubyte *data, int len, net_addr to);                      // 0x441da0
// (data, total length, to (ignored on a client: the server), flush): appended to the player's
// unreliable buffer, which goes out with nw_send when full or when flush is set. data may be NULL.
void multi_send_packet_buffered(ubyte *data, int len, mp_player *to, bool flush);   // 0x441e90
// The same with the reliable buffer and nw_NewSendReliable (always reliable; D3301: the fourth
// argument is the flush, not "reliable").
void multi_send_packet(ubyte *data, int len, mp_player *to, bool flush);      // 0x442060
// Host: resends mp_cur_packet to every other player except the sender (the handler's second argument).
void mp_forward_cur_packet_buffered(void *from);                              // 0x442230
void mp_forward_cur_packet(void *from);                                       // 0x4422c0
void cmd_disp_rate();                                   // 0x442380

#endif
