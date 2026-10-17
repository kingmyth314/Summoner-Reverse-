// unnamed_gtrack_0x4ae330.cpp - Sum.exe v1.40, code 0x4ae330-0x4ae882 (the PXO game tracker client: the
// hosted game's heartbeat and game over, game list requests, the replies, channel game counts).
// FILE NAME NOT KNOWN (placeholder). No assert path, no strings.tbl block, no reference name; module "-".
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit: multi_chattracker.cpp ends with irc_get_channel_list (0x4ae120, code to 0x4ae32d); the level-object
// ray tests start at 0x4ae890 (D5213: a separate object; this one could also be the tail of
// multi_chattracker.cpp, nothing in the binary decides it). Data: .data 0x59d618-0x59d647 (the two
// printf formats, after multi_chattracker.cpp's 0x59d41c-0x59d617), .bss 0x247b0a0-0x247e818 (after
// multi_chattracker.cpp's, up to the last byte of the heartbeat packet; the ray tests' .bss is 0x247e81c).
//
// The tracker is pxo_gtracker_server (pxo.cfg), UDP port 10512, sent and received through the net layer's
// buffer type 3. A hosted game sends its record every 10 s; when an ack is missing after 2 s it is sent
// again. All packets the server sends are acknowledged with their signature.

#include <stdio.h>
#include <string.h>
#include <winsock.h>
#include "vsdk/os/timer.h"
#include "vsdk/net/net.h"
#include "summoner/multi/multi.h"
#include "summoner/multi/unnamed_gtrack_0x4ae330.h"

#define GTRACK_UPDATE_INTERVAL 10           // s (timer_get(1))
#define GTRACK_RESEND_TIME     2000         // ms

// ---- .bss (0x247b0a0-0x247e818), address order. Names ours except gtrack_last_heartbeat_time. ----------
int  gtrack_game_running;                   // 0x247b0a0
int  gtrack_acked;                          // 0x247b0a4
int  gtrack_last_update;                    // 0x247b0a8
unsigned int gtrack_game_over_time;         // 0x247b0ac
gtrack_game_list gtrack_list_copy;          // 0x247b0b0
int  gtrack_game_type;                      // 0x247b230
unsigned int gtrack_game_over_first;        // 0x247b234
gtrack_packet gtrack_ack_packet;            // 0x247b238
int  gtrack_last_heartbeat_time;            // 0x247b82c
gtrack_packet gtrack_list_packet;           // 0x247b830
int  gtrack_game_over_state;                // 0x247be24
gtrack_packet gtrack_game_over_packet;      // 0x247be28
ubyte *gtrack_game_data;                    // 0x247c41c
net_addr gtrack_addr;                       // 0x247c420
gtrack_game_list gtrack_lists[GTRACK_NUM_LISTS];   // 0x247c428
gtrack_packet gtrack_heartbeat_packet;      // 0x247e228

// 0x4ae330  Confidence: exact (the byte stores of the game type use the constant the compare left in al;
// the source may store game_type). Builds the four outgoing packet headers and resolves the server
// (dotted address or host name). 0 for another game type or an unknown host.
int gtrack_init(int game_type)
{
    gtrack_game_type = game_type;
    gtrack_last_update = 0;
    gtrack_game_running = 0;
    if (game_type != GTRACK_GAME_TYPE)
        return 0;

    gtrack_heartbeat_packet.len = GTRACK_HEADER_SIZE + sizeof(gtrack_game_info);
    gtrack_heartbeat_packet.game_type = GTRACK_GAME_TYPE;
    gtrack_heartbeat_packet.type = GTRACK_HEARTBEAT;
    gtrack_game_data = gtrack_heartbeat_packet.data;
    gtrack_list_packet.game_type = GTRACK_GAME_TYPE;
    gtrack_list_packet.type = GTRACK_LIST_REQUEST;
    gtrack_list_packet.len = GTRACK_HEADER_SIZE;
    gtrack_ack_packet.game_type = GTRACK_GAME_TYPE;
    gtrack_ack_packet.type = GTRACK_ACK;
    gtrack_ack_packet.len = GTRACK_HEADER_SIZE;
    gtrack_game_over_packet.game_type = GTRACK_GAME_TYPE;
    gtrack_game_over_packet.type = GTRACK_GAME_OVER;
    gtrack_game_over_packet.len = GTRACK_HEADER_SIZE;

    unsigned long ip = inet_addr(pxo_gtracker_server);
    if (ip == INADDR_NONE) {
        hostent *he = gethostbyname(pxo_gtracker_server);
        if (!he)
            return 0;
        ip = *(unsigned long *)he->h_addr_list[0];
    }
    gtrack_addr.ip = ntohl(ip);
    gtrack_addr.port = GTRACK_PORT;
    gtrack_game_over_state = 0;
    return 1;
}

// 0x4ae410  Confidence: exact (the received length is compared with len - 1, which is also stored back
// into the packet: D5214). Sends the heartbeat when due (or resends it), resends a pending game over every
// 2 s, then handles one packet from the tracker: an ack ends the game over and marks the heartbeat seen,
// a game list goes into the first free slot, a channel count updates the lobby's channel row.
void gtrack_send_heartbeat()
{
    net_addr from;
    char channel[0x200];
    gtrack_packet in;

    nw_receive_all();
    int now_s = timer_get(1);
    int now_ms = timer_get(1000);

    if (gtrack_game_running) {
        if (now_s - gtrack_last_update > GTRACK_UPDATE_INTERVAL && gtrack_game_over_state == 0) {
            nw_send_packet(&gtrack_heartbeat_packet, gtrack_heartbeat_packet.len, 0, &gtrack_addr, 3);
            gtrack_acked = 0;
            gtrack_last_update = now_s;
            gtrack_last_heartbeat_time = now_ms;
            printf("Sending update\n");
        } else if (!gtrack_acked && now_ms - gtrack_last_heartbeat_time > GTRACK_RESEND_TIME) {
            int len = gtrack_heartbeat_packet.len;
            printf("Sending update because no ack\n");
            nw_send_packet(&gtrack_heartbeat_packet, len, 0, &gtrack_addr, 3);
            gtrack_acked = 0;
            gtrack_last_update = now_s;
            gtrack_last_heartbeat_time = now_ms;
        }
    }

    if (gtrack_game_over_state && (unsigned int)now_ms - gtrack_game_over_time > GTRACK_RESEND_TIME) {
        gtrack_game_over_time = now_ms;
        nw_send_packet(&gtrack_game_over_packet, gtrack_game_over_packet.len, 0, &gtrack_addr, 3);
    }

    if (!nw_reliable_slot_status(3))
        return;
    int size = nw_buffer_receive((ubyte *)&in, sizeof(in), 0, &from, 3);
    if (size == -1)
        return;
    if (size != (int)--in.len)
        return;

    switch (in.type) {
    case GTRACK_SERVER_ACK:
        gtrack_acked = 1;
        gtrack_game_over_state = 0;
        break;
    case GTRACK_GAME_LIST: {
        for (int i = 0; i < GTRACK_NUM_LISTS; i++) {
            if (!gtrack_lists[i].game_type) {
                memcpy(&gtrack_lists[i], in.data, sizeof(gtrack_game_list));
                i = GTRACK_NUM_LISTS + 1;
            }
        }
        break;
    }
    case GTRACK_CHANNEL_COUNT: {
        memset(channel, 0, sizeof(channel));
        int count = *(int *)in.data;
        strcpy(channel, (char *)in.data + 4);
        lobby_channel *c = lobby_find_channel(channel);
        if (c)
            c->unk8c = count;
        break;
    }
    }
    gtrack_send_ack(in.sig);
}

// 0x4ae660  Confidence: exact. The first received list, copied out; its slot is freed.
gtrack_game_list *gtrack_get_game_list()
{
    for (int i = 0; i < GTRACK_NUM_LISTS; i++) {
        if (gtrack_lists[i].game_type) {
            memcpy(&gtrack_list_copy, &gtrack_lists[i], sizeof(gtrack_game_list));
            gtrack_lists[i].game_type = 0;
            return &gtrack_list_copy;
        }
    }
    return NULL;
}

// 0x4ae6b0  Confidence: exact.
void gtrack_request_game_list()
{
    gtrack_list_packet.len = GTRACK_HEADER_SIZE;
    nw_send_packet(&gtrack_list_packet, GTRACK_HEADER_SIZE, 0, &gtrack_addr, 3);
}

// 0x4ae6e0  Confidence: exact (the 0x28-byte filter is a whole gtrack_channel_query).
void gtrack_request_filtered_list(gtrack_channel_query *filter)
{
    memcpy(gtrack_list_packet.data, filter, sizeof(gtrack_channel_query));
    gtrack_list_packet.len = GTRACK_HEADER_SIZE + sizeof(gtrack_channel_query);
    nw_send_packet(&gtrack_list_packet, GTRACK_HEADER_SIZE + sizeof(gtrack_channel_query), 0, &gtrack_addr, 3);
}

// 0x4ae720  Confidence: exact. Called repeatedly by the leaving host; returns 1 only in state 2, which
// nothing sets (the server ack sets 0), so tracker_end_game's single call only starts the sending.
int gtrack_send_game_over()
{
    if (gtrack_game_over_state == 2) {
        gtrack_game_over_state = 0;
        return 1;
    }
    if (gtrack_game_over_state == 1) {
        gtrack_send_heartbeat();
        return 0;
    }
    if (gtrack_game_over_state == 0) {
        gtrack_game_over_time = timer_get(1000);
        gtrack_game_over_first = timer_get(1000);
        gtrack_game_over_state = 1;
        gtrack_game_running = 0;
        nw_send_packet(&gtrack_game_over_packet, gtrack_game_over_packet.len, 0, &gtrack_addr, 3);
    }
    return 0;
}

// 0x4ae7a0  Confidence: exact.
void gtrack_send_ack(unsigned int sig)
{
    gtrack_ack_packet.sig = sig;
    nw_send_packet(&gtrack_ack_packet, gtrack_ack_packet.len, 0, &gtrack_addr, 3);
}

// 0x4ae7d0  Confidence: exact (VC6 duplicated the two stores after the copy into both paths). The first
// heartbeat goes out on the next gtrack_send_heartbeat (last update -10 s).
void gtrack_start_game(gtrack_game_info *game)
{
    gtrack_game_over_state = 0;
    if (gtrack_game_type == GTRACK_GAME_TYPE)
        memcpy(gtrack_game_data, game, sizeof(gtrack_game_info));
    gtrack_game_running = 1;
    gtrack_last_update = -GTRACK_UPDATE_INTERVAL;
}

// 0x4ae830  Confidence: exact. The count of games listed under a channel (answered with type 8).
void gtrack_send_channel_query(gtrack_channel_query *q)
{
    gtrack_packet p;

    p.game_type = GTRACK_GAME_TYPE;
    p.type = GTRACK_CHANNEL_QUERY;
    p.len = GTRACK_HEADER_SIZE + sizeof(gtrack_channel_query);
    memcpy(p.data, q->channel, sizeof(q->channel));
    nw_send_packet(&p, GTRACK_HEADER_SIZE + sizeof(gtrack_channel_query), 0, &gtrack_addr, 3);
}
