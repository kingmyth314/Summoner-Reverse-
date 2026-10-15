// summoner/multi/unnamed_utrack_0x48d7c0.h - the PXO user tracker login (0x48d7c0-0x48db2f: pxo_login_send,
// utrack_init, pxo_login_request, pxo_login_poll; .bss 0x2318e48-0x231906f, no .data). Source:
// game/unnamed_utrack_0x48d7c0.cpp. THE FILE NAME IS A PLACEHOLDER (no assert path, no strings.tbl block;
// decomp64). Included by summoner/multi/multi.h. Not original source.
#ifndef SUMMONER_MULTI_UNNAMED_UTRACK_0X48D7C0_H
#define SUMMONER_MULTI_UNNAMED_UTRACK_0X48D7C0_H

#include "vsdk/vsdk.h"
#include "vsdk/net/net.h"

// A tracker datagram (0x204 bytes: pxo_login_poll's receive buffer and pxo_login_send's local are that
// size; the global copy at 0x2318e50 is followed by utrack_addr at 0x2319058). Packed: the 17-byte header
// has unaligned fields (len +1, code +3, sig +9). Type and field names ours.
#pragma pack(push, 1)
struct utrack_packet {
    ubyte type;                       // +0x00 0 ack, 0xd login request, 0xe login refused, 0xf login accepted
    ushort len;                       // +0x01 header + data
    uint code;                        // +0x03 5 in an ack
    ushort xcode;                     // +0x07
    uint sig;                         // +0x09 echoed in the ack
    uint security;                    // +0x0d
    ubyte data[0x1f3];                // +0x11
};
#pragma pack(pop)
LAYOUT_CHECK(utrack_packet, sizeof(utrack_packet) == 0x204);
#define UTRACK_HEADER_SIZE 0x11

// The login request in the packet data (0x3c bytes: packet length 0x4d). pxo_read_login passes
// pxo_login_name (0x5f9670, char[33]) with pxo_login_password (0x5f9691, char[27]) right after it.
struct utrack_login_request {
    char login[0x21];                 // +0x00
    char password[0x1b];              // +0x21
};
LAYOUT_CHECK(utrack_login_request, sizeof(utrack_login_request) == 0x3c);

// utrack_state: 1 idle, 2 waiting for the reply, 3 accepted, 4 refused, 5 timed out (30 s).
extern utrack_login_request *pxo_login_data;            // 0x2318e48 = pxo_login_packet.data (name ours)
extern utrack_packet pxo_login_packet;                  // 0x2318e50
extern net_addr utrack_addr;                            // 0x2319058
extern int utrack_state;                                // 0x2319060
extern char *utrack_reply_buffer;                       // 0x2319064
extern int utrack_last_send_time;                       // 0x2319068 timer_get(1000)
extern int utrack_request_time;                         // 0x231906c timer_get(1000)

void pxo_login_send(uint sig);                          // 0x48d7c0 the ack (type 0, code 5)
int  utrack_init();                                     // 0x48d810 0 when the host is not found, else 1
// (request or NULL, reply buffer): with a request (state 1 only, else -3) sends it and returns 0; with
// NULL polls: 0 busy / idle, 1 accepted, -1 refused, -2 timed out (the last three reset the state to 1).
int  pxo_login_request(utrack_login_request *req, char *reply);   // 0x48d870
void pxo_login_poll();                                  // 0x48da00

#endif
