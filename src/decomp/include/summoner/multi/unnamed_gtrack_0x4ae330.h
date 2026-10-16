// summoner/multi/unnamed_gtrack_0x4ae330.h - the PXO game tracker client 0x4ae330-0x4ae882 (heartbeats of
// a hosted game, game over, game list requests and replies, channel counts). Source:
// game/unnamed_gtrack_0x4ae330.cpp. THE FILE NAME IS A PLACEHOLDER (decomp71). gtrack_channel_query and
// gtrack_send_channel_query stay in multi.h. Type and field names ours. Not original source.
#ifndef SUMMONER_MULTI_UNNAMED_GTRACK_0X4AE330_H
#define SUMMONER_MULTI_UNNAMED_GTRACK_0X4AE330_H

#include "vsdk/vsdk.h"
#include "vsdk/net/net.h"

struct gtrack_channel_query;

#define GTRACK_GAME_TYPE     5        // Summoner (gtrack_init accepts only this)
#define GTRACK_PORT          10512    // 0x2910
#define GTRACK_HEADER_SIZE   0x15
#define GTRACK_MAX_DATA      1500     // 0x5f1 - 0x15
#define GTRACK_NUM_LISTS     20
// Packet types (+0x0d).
#define GTRACK_SERVER_ACK    0        // in: the server acknowledged our last packet
#define GTRACK_ACK           1        // out: we acknowledge a server packet (its sig)
#define GTRACK_GAME_OVER     3
#define GTRACK_HEARTBEAT     4        // out: the hosted game's record
#define GTRACK_LIST_REQUEST  5        // out: game list (header only) or filtered by a channel (0x28 bytes)
#define GTRACK_GAME_LIST     6        // in: a gtrack_game_list
#define GTRACK_CHANNEL_QUERY 7        // out: count of games in a channel
#define GTRACK_CHANNEL_COUNT 8        // in: {int count, char channel[]}

#pragma pack(push, 1)
// One tracker packet (0x5f1 bytes, byte packed: gtrack_send_ack stores the signature at +0x11).
struct gtrack_packet {
    unsigned int len;                 // +0x00 bytes sent
    ubyte game_type;                  // +0x04 GTRACK_GAME_TYPE
    ubyte addr[8];                    // +0x05 never written by the client
    int type;                         // +0x0d GTRACK_*
    unsigned int sig;                 // +0x11 the server's packet id (acknowledged)
    ubyte data[GTRACK_MAX_DATA];      // +0x15
};
#pragma pack(pop)

// A received game list (0x180 bytes; 20 slots, a slot is free while game_type is 0).
struct gtrack_game_list {
    ubyte game_type;                  // +0x000
    char names[10][0x20];             // +0x001
    unsigned int servers[10];         // +0x144 network order, 0 empty
    ushort ports[10];                 // +0x16c network order
};
LAYOUT_CHECK(gtrack_game_list, sizeof(gtrack_game_list) == 0x180);

// The hosted game's record (0x58 bytes; tracker_start_game fills it, gtrack_start_game copies it into the
// heartbeat).
struct gtrack_game_info {
    char name[0x20];                  // +0x00 mp_netgame_name
    int _unk20;                       // +0x20 0
    int _unk24;                       // +0x24 0
    int _unk28;                       // +0x28 0
    int _unk2c;                       // +0x2c 4 (maximum players?)
    int _unk30;                       // +0x30 1 (players?)
    char channel[0x24];               // +0x34 tracker_game_channel (copied without a length check)
};
LAYOUT_CHECK(gtrack_game_info, sizeof(gtrack_game_info) == 0x58);

extern int  gtrack_game_running;                         // 0x247b0a0 heartbeats on
extern int  gtrack_acked;                                // 0x247b0a4 the last heartbeat was acknowledged
extern int  gtrack_last_update;                          // 0x247b0a8 timer_get(1) of the last heartbeat
extern unsigned int gtrack_game_over_time;               // 0x247b0ac timer_get(1000) of the last game over
extern gtrack_game_list gtrack_list_copy;                // 0x247b0b0 gtrack_get_game_list's result
extern int  gtrack_game_type;                            // 0x247b230
extern unsigned int gtrack_game_over_first;              // 0x247b234 written, never read
extern gtrack_packet gtrack_ack_packet;                  // 0x247b238
extern int  gtrack_last_heartbeat_time;                  // 0x247b82c timer_get(1000) (sum.symbols name)
extern gtrack_packet gtrack_list_packet;                 // 0x247b830
extern int  gtrack_game_over_state;                      // 0x247be24 0 idle, 1 sending (2 never set)
extern gtrack_packet gtrack_game_over_packet;            // 0x247be28
extern ubyte *gtrack_game_data;                          // 0x247c41c the heartbeat's data
extern net_addr gtrack_addr;                             // 0x247c420 pxo_gtracker_server:10512
extern gtrack_game_list gtrack_lists[GTRACK_NUM_LISTS];  // 0x247c428
extern gtrack_packet gtrack_heartbeat_packet;            // 0x247e228

int  gtrack_init(int game_type);                         // 0x4ae330 1 = ready
void gtrack_send_heartbeat();                            // 0x4ae410 also receives the tracker's packets
gtrack_game_list *gtrack_get_game_list();                // 0x4ae660 NULL none
void gtrack_request_game_list();                         // 0x4ae6b0
void gtrack_request_filtered_list(gtrack_channel_query *filter);   // 0x4ae6e0
int  gtrack_send_game_over();                            // 0x4ae720 (return unused)
void gtrack_send_ack(unsigned int sig);                  // 0x4ae7a0
void gtrack_start_game(gtrack_game_info *game);          // 0x4ae7d0

#endif
