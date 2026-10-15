// vsdk/net/net.h - the UDP network layer with "reliable sockets" (sum.symbols module vsdk\net; one object,
// code 0x517590-0x5194ab, .data 0x5a7dcc-0x5a8483, .bss 0x26b63f0-0x2710ebb; file name not known: no path
// string, no strings.tbl block, no Summoner 2 counterpart; source src/decomp/vsdk/unnamed_net_0x517590.cpp,
// decomp50). Not original source. docs/sum-exe/vsdk-os.md "Networking". The messages name psnet
// ("psnet_send") and nw_WorkReliable / nw_SendReliable / nw_ReceiveReliable / nw_ConnectToServer.
#ifndef VSDK_NET_NET_H
#define VSDK_NET_NET_H

#include "vsdk/vsdk.h"

// Host-order address (8 bytes: nw_buffer_push / nw_accept_reliable copy two dwords). Name ours.
struct net_addr {
    uint ip;                              // +0 host order (nw_send_packet htonl's it)
    ushort port;                          // +4 host order
    ushort _pad6;
};
LAYOUT_CHECK(net_addr, sizeof(net_addr) == 8);

// Per-type receive buffers: nw_receive_all sorts each datagram by its first byte (0..4) into one of them.
#define NW_NUM_BUFFERS     5
#define NW_BUFFER_PACKETS  0x60
#define NW_MAX_PACKET      0x2a8              // recvfrom size
#define NW_TYPE_UNRELIABLE 0                  // nw_send / nw_receive
#define NW_TYPE_RELIABLE   1                  // reliable_header packets

struct nw_packet_slot {                   // 0x2b8 bytes; names ours
    int seq;                              // +0x00 -1 = free
    int size;                             // +0x04
    net_addr addr;                        // +0x08
    ubyte data[NW_MAX_PACKET];            // +0x10
};
LAYOUT_CHECK(nw_packet_slot, sizeof(nw_packet_slot) == 0x2b8);

struct nw_packet_buffer {                 // 0x1050c bytes; names ours
    nw_packet_slot slots[NW_BUFFER_PACKETS];   // +0x00000
    int next_seq;                         // +0x10500
    int first_seq;                        // +0x10504 oldest packet, -1 = empty
    int last_seq;                         // +0x10508 newest packet
};
LAYOUT_CHECK(nw_packet_buffer, sizeof(nw_packet_buffer) == 0x1050c);
extern nw_packet_buffer nw_recv_buffers[NW_NUM_BUFFERS];   // 0x26b63f0 (sum.symbols: uint8[0x10000])

// Reliable packet header (packed: send_time at +6). Field names ours.
#define NW_MAX_RELIABLE_DATA 600
#pragma pack(push, 1)
struct reliable_header {                  // 0x262 bytes (nw_buffer_receive size, the 0x262-byte memset)
    ubyte type;                           // +0x00 RNT_*
    ubyte compressed;                     // +0x01 never set
    ushort seq;                           // +0x02
    ushort data_len;                      // +0x04
    int send_time;                        // +0x06 timer_get(1000)
    ubyte data[NW_MAX_RELIABLE_DATA];     // +0x0a
};
#pragma pack(pop)
LAYOUT_CHECK(reliable_header, sizeof(reliable_header) == 0x262);
#define NW_RELIABLE_HEADER_SIZE 10

// Reliable packet types (the first byte; names ours).
#define RNT_ACK      0                    // data = the acknowledged seq as an int
#define RNT_DATA     1
#define RNT_DATA2    2                    // handled as RNT_DATA
#define RNT_REQ_CONN 3                    // seq = NW_CONNECT_SEQ
#define RNT_HEARTBEAT 5
#define RNT_I_AM_HERE 6                   // seq = NW_IAMHERE_SEQ
#define RNT_IGNORE   7                    // set on a duplicate connection request
#define NW_CONNECT_SEQ 0x142
#define NW_IAMHERE_SEQ 0xfebd

// Reliable socket states (reliable_socket::status; names ours).
#define RNF_UNUSED    0
#define RNF_CONNECTED 1
#define RNF_BROKEN    2
#define RNF_CONNECTING 4                  // accepted by the server, waiting for nw_accept_reliable
#define RNF_LIMBO     5

#define NW_MAX_RELIABLE_SOCKETS 10
#define NW_MAX_NET_BUFFERS      150
#define NW_PING_HISTORY         10

// A send / receive buffer (operator new(0x258) under a NULL current mempool). Name ours.
struct reliable_net_buffer {
    ubyte buffer[NW_MAX_RELIABLE_DATA];
};

// One reliable connection (0xd2d bytes, packed; names ours after the fields' use).
#pragma pack(push, 1)
struct reliable_socket {
    reliable_net_buffer *sbuffers[NW_MAX_NET_BUFFERS];  // +0x000 sent, not yet acknowledged
    ushort ssequence[NW_MAX_NET_BUFFERS]; // +0x258
    int timesent[NW_MAX_NET_BUFFERS];     // +0x384
    ushort send_len[NW_MAX_NET_BUFFERS];  // +0x5dc
    reliable_net_buffer *rbuffers[NW_MAX_NET_BUFFERS];  // +0x708 received, not yet read
    int recv_len[NW_MAX_NET_BUFFERS];     // +0x960
    ushort rsequence[NW_MAX_NET_BUFFERS]; // +0xbb8
    int last_packet_received;             // +0xce4
    int last_packet_sent;                 // +0xce8
    int status;                           // +0xcec RNF_*
    ushort theirsequence;                 // +0xcf0
    ushort oursequence;                   // +0xcf2
    net_addr addr;                        // +0xcf4
    int pings[NW_PING_HISTORY];           // +0xcfc
    ubyte ping_pos;                       // +0xd24
    uint num_ping_samples;                // +0xd25 (unsigned: jb 10)
    int mean_ping;                        // +0xd29 (net_get_reliable_socket_ping)
};
#pragma pack(pop)
LAYOUT_CHECK(reliable_socket, sizeof(reliable_socket) == 0xd2d);
extern reliable_socket nw_reliable_sockets[NW_MAX_RELIABLE_SOCKETS];   // 0x2708ad0

extern int nw_server_socket;                                        // 0x5a7dcc -1, the client's link
extern unsigned int nw_udp_socket;                                  // 0x5a7dd0 SOCKET
extern char nw_connection_speed[0x400];                             // 0x2707d2c
extern int nw_network_status;                                       // 0x270812c
extern int nw_reliable_resend_time;                                 // 0x2710e94
extern int nw_last_heartbeat_time;                                  // 0x2710e98
extern int nw_networking_ok;                                        // 0x2710e9c
extern int nw_socket;                                               // 0x2710ea0 1 once the socket is up
extern int nw_winsock_started;                                      // 0x2710ea4

// The unit's functions in address order (decomp50 unless noted).
int  nw_buffer_receive(ubyte *data, int max_len, int flags, net_addr *from, int type);   // 0x517590
int  nw_can_write();                                                // 0x5175e0
int  nw_reliable_slot_status(int type);                             // 0x517630 1 = a packet is waiting
int  nw_send_packet(void *data, int len, int flags, net_addr *to, int type);   // 0x517680
void nw_buffer_init(nw_packet_buffer *buf);                         // 0x517730
void nw_buffer_push(nw_packet_buffer *buf, ubyte *data, int size, net_addr *from);  // 0x517780
int  nw_buffer_pop(nw_packet_buffer *buf, ubyte *data, int *size, net_addr *from);  // 0x517820
unsigned long nw_ras_init();                                        // 0x5178c0 dial-up IP, 0 if none
void nw_set_socket_options(unsigned int sock);                      // 0x517ae0
int  nw_reliable_reset();                                           // 0x517bf0 returns 1
void nw_init_socket(unsigned short port);                           // 0x517c10
int  nw_send_reliable_reply(net_addr *to, int seq, int send_time);  // 0x517d20 RNT_ACK
int  nw_send_heartbeat(net_addr *to);                               // 0x517d70 RNT_I_AM_HERE
int  float_compare(const void *a, const void *b);                   // 0x517dc0 qsort callback (module -)
void nw_WorkReliable();                                            // 0x517e00
void nw_receive_all();                                              // 0x518770
void nw_close();                                                    // 0x518880 atexit handler
void nw_init_networking(unsigned short port);                        // 0x5188d0 (LoadSettings: mp_net_port)
void nw_addr_to_string(char *out, int size, net_addr *addr);        // 0x518a30 "IP: %s %d"
int  nw_send_broadcast(unsigned short port, void *data, int len);   // 0x518a90
int  nw_send(net_addr *to, void *data, int len);                    // 0x518ac0 1 = sent
int  nw_receive(ubyte *data, net_addr *from);                       // 0x518b80 size or 0
int  nw_accept_reliable(net_addr *from);                            // 0x518ce0 socket or -1
void nw_ConnectToServer(int *socket, net_addr *server);             // 0x518f30
void nw_flush_receive();                                            // 0x519440
// decomp53 (the net packet / session units): the reliable socket calls of the same object (module -).
// (socket, buffer, size): bytes read, <= 0 when nothing is waiting.
int  nw_NewReceiveReliable(int socket, void *buf, int size);         // 0x518bb0
int  nw_NewSendReliable(int socket, const void *data, int len);      // 0x518d70 (result unused)
// The socket's state: RNF_UNUSED 0, RNF_CONNECTED 1, RNF_BROKEN 2, 3, ...; the session drops a player
// on 0, 2 and 3.
int  net_get_reliable_socket_status(int socket);                     // 0x5193a0

// The ping field of a reliable socket record (index below 10; own assert). decomp24 (chat_cmd_ping).
int net_get_reliable_socket_ping(int socket);                       // 0x5193f0

// decomp47 (multiendgame.cpp, multiplayerdata.cpp): closes the reliable socket the pointer holds (the callers
// pass &mp_player::socket and set it to -1 afterwards); 0 when the IP dwords (and, with check_port, the
// ports) match, else -1.
void nw_NewCloseSocket(int *socket);                                // 0x5191c0
int  nw_addr_equal(const void *a, const void *b, bool check_port);  // 0x519370

#endif
