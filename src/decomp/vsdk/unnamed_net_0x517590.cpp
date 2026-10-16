// unnamed_net_0x517590.cpp - Sum.exe v1.40, code 0x517590-0x5194ab (sum.symbols module vsdk\net): the UDP
// layer. One socket; incoming datagrams are sorted by their first byte into five packet buffers; type 1
// carries the "reliable socket" protocol (connection request, IAMHERE / ACK, sequence numbers, resends,
// pings, time-outs) for up to 10 links.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 (VC6 reuses
// dead argument slots for address-taken locals: nw_buffer_receive's and nw_receive's size).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// File name not known (no path string, no strings.tbl block, no Summoner 2 counterpart; the messages name
// psnet_send and nw_WorkReliable, the names of the Volition / Outrage network code), so a placeholder.
// Data: .data 0x5a7dcc-0x5a8483 (the two sockets, then the literals in order of use), .bss
// 0x26b63f0-0x2710ebb (address order below).
// Every error path formats a message into a local buffer that is never printed (as registry.cpp).
//
// Not written here (sum.symbols module -, same object): float_compare 0x517dc0, nw_NewReceiveReliable
// 0x518bb0, nw_NewSendReliable 0x518d70, nw_NewCloseSocket 0x5191c0. (net_get_reliable_socket_status 0x5193a0 and
// net_get_reliable_socket_ping 0x5193f0, module - as well, are written below: decomp74, D5610.)

#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vsdk/net/net.h"
#include "vsdk/os/memory.h"
#include "vsdk/os/timer.h"
#include "vsdk/os/registry.h"
#include "vsdk/os/console.h"

// The RAS (dial-up) calls, loaded from rasapi32.dll by nw_ras_init. The structures are the Windows 95 ones
// (RASCONN 0x19c, RASCONNSTATUS 0xa0, RASPPPIP 0x28 bytes); written out here because the SDK sizes depend
// on WINVER. Names ours, after <ras.h>.
struct nw_rasconn {
    DWORD dwSize;
    HANDLE hrasconn;
    char szEntryName[257];
    char szDeviceType[17];
    char szDeviceName[129];
};
LAYOUT_CHECK(nw_rasconn, sizeof(nw_rasconn) == 0x19c);
struct nw_rasconnstatus {
    DWORD dwSize;
    int rasconnstate;
    DWORD dwError;
    char szDeviceType[17];
    char szDeviceName[129];
};
LAYOUT_CHECK(nw_rasconnstatus, sizeof(nw_rasconnstatus) == 0xa0);
struct nw_rasppp_ip {
    DWORD dwSize;
    DWORD dwError;
    char szIpAddress[16];
    char szServerIpAddress[16];
};
LAYOUT_CHECK(nw_rasppp_ip, sizeof(nw_rasppp_ip) == 0x28);
#define NW_RASP_PPPIP 0x8021
typedef DWORD (WINAPI *nw_RasEnumConnections_t)(nw_rasconn *, DWORD *, DWORD *);
typedef DWORD (WINAPI *nw_RasGetConnectStatus_t)(HANDLE, nw_rasconnstatus *);
typedef DWORD (WINAPI *nw_RasGetProjectionInfo_t)(HANDLE, int, void *, DWORD *);

// A compare left in the code with no use of its result (nw_can_write `or eax, eax`, nw_WorkReliable
// `cmp eax, 0x258`): an expression-only debug macro. Name ours.
#define NW_DEBUG_CHECK(expr) ((void)(expr))

// ---- .data (0x5a7dcc-0x5a7dd3) ----------------------------------------------------------------------
int nw_server_socket = -1;                               // 0x5a7dcc
unsigned int nw_udp_socket = INVALID_SOCKET;             // 0x5a7dd0

// ---- .bss (0x26b63f0-0x2710ebb, address order) ------------------------------------------------------
nw_packet_buffer nw_recv_buffers[NW_NUM_BUFFERS];        // 0x26b63f0
char nw_connection_speed[0x400];                         // 0x2707d2c "ConnectionSpeed"
int nw_network_status;                                   // 0x270812c 0 none, 1 slow, 2 56k, 3 fast (module -)
static reliable_header nw_flush_buf;                     // 0x2708130 (name ours) nw_flush_receive's sink
static reliable_header nw_reliable_recv_buf;             // 0x2708398 nw_WorkReliable
static reliable_header nw_connect_reply_buf;             // 0x2708600 nw_ConnectToServer
static reliable_header nw_connect_request;               // 0x2708868 nw_ConnectToServer (10 bytes sent)
reliable_socket nw_reliable_sockets[NW_MAX_RELIABLE_SOCKETS];   // 0x2708ad0
int nw_reliable_resend_time;                             // 0x2710e94 client: the connect time (timeout base)
int nw_last_heartbeat_time;                              // 0x2710e98
int nw_networking_ok;                                    // 0x2710e9c
int nw_socket;                                           // 0x2710ea0
int nw_winsock_started;                                  // 0x2710ea4
// 0x2710ea8-0x2710eaf: no reference.
static nw_RasEnumConnections_t nw_RasEnumConnections;    // 0x2710eb0 (names ours)
static nw_RasGetConnectStatus_t nw_RasGetConnectStatus;  // 0x2710eb4
static nw_RasGetProjectionInfo_t nw_RasGetProjectionInfo;   // 0x2710eb8

// 0x517590  Confidence: likely (max_len and flags are not read; the size lives in type's argument slot).
int nw_buffer_receive(ubyte *data, int max_len, int flags, net_addr *from, int type)
{
    int size;

    if (type < 0 || type >= NW_NUM_BUFFERS)
        return -1;
    if (!nw_buffer_pop(&nw_recv_buffers[type], data, &size, from))
        return -1;
    return size;
}

// 0x5175e0  Confidence: likely (the empty SOCKET_ERROR branch; FD_ZERO / FD_SET of <winsock.h> fold to two
// stores).
int nw_can_write()
{
    fd_set write_fds;
    struct timeval timeout;

    FD_ZERO(&write_fds);
    FD_SET(nw_udp_socket, &write_fds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    int ret = select(-1, NULL, &write_fds, NULL, &timeout);
    if (ret == SOCKET_ERROR)
        NW_DEBUG_CHECK(ret);
    return ret;
}

// 0x517630  Confidence: exact. sum.symbols reads this as "connected"; it tests that the buffer of the type
// holds a packet (first_seq valid and not past last_seq).
int nw_reliable_slot_status(int type)
{
    if (type < 0 || type >= NW_NUM_BUFFERS)
        return -1;
    nw_packet_buffer *buf = &nw_recv_buffers[type];
    if (buf->first_seq != -1 && buf->first_seq <= buf->last_seq)
        return 1;
    return 0;
}

// 0x517680  Confidence: likely (local order). -2 when the send would block, -1 on other errors, else the
// byte count of sendto (len + 1 with the type byte).
int nw_send_packet(void *data, int len, int flags, net_addr *to, int type)
{
    struct sockaddr_in addr;
    ubyte packet[0x340];

    packet[0] = (ubyte)type;
    memcpy(packet + 1, data, len);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(to->port);
    addr.sin_addr.s_addr = htonl(to->ip);
    int ret = sendto(nw_udp_socket, (char *)packet, len + 1, flags, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == SOCKET_ERROR)
        return WSAGetLastError() == WSAEWOULDBLOCK ? -2 : -1;
    return ret;
}

// 0x517730  Confidence: exact.
void nw_buffer_init(nw_packet_buffer *buf)
{
    memset(buf->slots, 0, sizeof(buf->slots));
    for (int i = 0; i < NW_BUFFER_PACKETS; i++)
        buf->slots[i].seq = -1;
    buf->next_seq = 0;
    buf->first_seq = -1;
    buf->last_seq = -1;
}

// 0x517780  Confidence: likely (the free-slot search as a loop with break). A full buffer drops the packet.
void nw_buffer_push(nw_packet_buffer *buf, ubyte *data, int size, net_addr *from)
{
    int i;

    for (i = 0; i < NW_BUFFER_PACKETS; i++) {
        if (buf->slots[i].seq == -1)
            break;
    }
    if (i == NW_BUFFER_PACKETS)
        return;
    nw_packet_slot *slot = &buf->slots[i];
    memcpy(slot->data, data, size);
    slot->size = size;
    slot->addr = *from;
    slot->seq = buf->next_seq;
    buf->last_seq = buf->next_seq++;
    if (buf->first_seq == -1)
        buf->first_seq = buf->last_seq;
}

// 0x517820  Confidence: likely (the search as for / break).
int nw_buffer_pop(nw_packet_buffer *buf, ubyte *data, int *size, net_addr *from)
{
    int seq = buf->first_seq;
    int i;

    if (seq == -1 || seq > buf->last_seq)
        return 0;
    for (i = 0; i < NW_BUFFER_PACKETS; i++) {
        if (buf->slots[i].seq == seq)
            break;
    }
    if (i == NW_BUFFER_PACKETS)
        return 0;
    nw_packet_slot *slot = &buf->slots[i];
    int len = slot->size;
    memcpy(data, slot->data, len);
    *size = len;
    *from = slot->addr;
    slot->seq = -1;
    buf->first_seq++;
    return 1;
}

// 0x5178c0  Confidence: likely (locals: count +0xc, size +0x10, projection size +0x14, projection +0x18,
// message +0x40, status +0x140, connections +0x1e0). Returns the IP of the last connection's PPP
// projection, 0 when RAS is missing, has no connection or fails (every failure frees the library).
unsigned long nw_ras_init()
{
    DWORD count;
    DWORD size;
    DWORD projection_size;
    nw_rasppp_ip projection;
    char msg[256];
    nw_rasconnstatus status;
    nw_rasconn connections[25];

    HMODULE lib = LoadLibraryA("rasapi32.dll");
    if (lib == NULL)
        return 0;
    nw_RasEnumConnections = (nw_RasEnumConnections_t)GetProcAddress(lib, "RasEnumConnectionsA");
    if (nw_RasEnumConnections == NULL) {
        FreeLibrary(lib);
        return 0;
    }
    nw_RasGetConnectStatus = (nw_RasGetConnectStatus_t)GetProcAddress(lib, "RasGetConnectStatusA");
    if (nw_RasGetConnectStatus == NULL) {
        FreeLibrary(lib);
        return 0;
    }
    nw_RasGetProjectionInfo = (nw_RasGetProjectionInfo_t)GetProcAddress(lib, "RasGetProjectionInfoA");
    if (nw_RasGetProjectionInfo == NULL) {
        FreeLibrary(lib);
        return 0;
    }

    size = sizeof(connections);
    connections[0].dwSize = sizeof(nw_rasconn);
    if (nw_RasEnumConnections(connections, &size, &count)) {
        FreeLibrary(lib);
        return 0;
    }
    if (count < 1) {
        FreeLibrary(lib);
        return 0;
    }
    sprintf(msg, "Found %d connections\n", (int)count);
    for (DWORD i = 0; i < count; i++) {
        sprintf(msg, "Connection %d:\n", (int)i);
        sprintf(msg, "Entry Name: %s\n", connections[i].szEntryName);
        sprintf(msg, "Device Type: %s\n", connections[i].szDeviceType);
        sprintf(msg, "Device Name: %s\n", connections[i].szDeviceName);
        status.dwSize = sizeof(status);
        if (nw_RasGetConnectStatus(connections[i].hrasconn, &status)) {
            FreeLibrary(lib);
            return 0;
        }
        projection.dwSize = sizeof(projection);
        projection_size = sizeof(projection);
        if (nw_RasGetProjectionInfo(connections[i].hrasconn, NW_RASP_PPPIP, &projection, &projection_size)) {
            FreeLibrary(lib);
            return 0;
        }
        sprintf(msg, "\tIP Address: %s\n", projection.szIpAddress);
    }
    FreeLibrary(lib);
    unsigned long ip = inet_addr(projection.szIpAddress);
    return ip == INADDR_NONE ? 0 : ip;
}

// 0x517ae0  Confidence: likely (the halving loop's form). Broadcast on, non-blocking, then the largest
// receive buffer from 32 KB down that the stack accepts, but never below the current size.
void nw_set_socket_options(unsigned int sock)
{
    int current;
    int wanted;
    int len;
    unsigned long arg;
    int broadcast;
    char msg[256];

    broadcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&broadcast, sizeof(broadcast));
    arg = 1;
    if (ioctlsocket(sock, FIONBIO, &arg) == SOCKET_ERROR)
        sprintf(msg, "Unable to make socket non-blocking -- %d\n", WSAGetLastError());
    len = sizeof(int);
    getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&current, &len);
    for (wanted = 0x8000; wanted >= current; wanted >>= 1) {
        if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&wanted, sizeof(wanted)) != SOCKET_ERROR)
            break;
        int err = WSAGetLastError();
        if (err == WSAENOPROTOOPT || err == WSAEINVAL)
            break;
    }
    getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&current, &len);
    sprintf(msg, "Receive buffer set to %d\n", current);
}

// 0x517bf0  Confidence: exact.
int nw_reliable_reset()
{
    memset(nw_reliable_sockets, 0, sizeof(nw_reliable_sockets));
    return 1;
}

// 0x517c10  Confidence: likely (the sockaddr clearing). Ports 0-1023 and 65535 become 4200. The messages
// say TCP; the socket is UDP. A second SO_BROADCAST is set and its error code kept in a dead local.
void nw_init_socket(unsigned short port)
{
    struct sockaddr_in addr;
    int broadcast;
    int error;
    char msg[256];

    unsigned short use_port = 4200;
    if (port > 0x3ff && port < 0xffff)
        use_port = port;
    nw_udp_socket = INVALID_SOCKET;
    nw_udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (nw_udp_socket == INVALID_SOCKET) {
        sprintf(msg, "Cannot create TCP socket (%d)!\n", WSAGetLastError());
        return;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(use_port);
    if (bind(nw_udp_socket, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        sprintf(msg, "Couldn't bind TCP socket (%d)! Invalidating TCP\n", WSAGetLastError());
        return;
    }
    nw_set_socket_options(nw_udp_socket);
    broadcast = 1;
    if (setsockopt(nw_udp_socket, SOL_SOCKET, SO_BROADCAST, (char *)&broadcast, sizeof(broadcast)) == SOCKET_ERROR)
        error = WSAGetLastError();
    nw_reliable_reset();
    nw_socket = 1;
}

// 0x517d20  Confidence: exact. 14 bytes: header (type ACK, data_len 4, the peer's send_time) and the seq.
int nw_send_reliable_reply(net_addr *to, int seq, int send_time)
{
    reliable_header ack;

    ack.type = RNT_ACK;
    ack.data_len = sizeof(int);
    ack.send_time = send_time;
    *(int *)ack.data = seq;
    return nw_send_packet(&ack, NW_RELIABLE_HEADER_SIZE + sizeof(int), 0, to, NW_TYPE_RELIABLE);
}

// 0x517d70  Confidence: exact. The IAMHERE packet (sum.symbols "heartbeat"); send_time is left unset.
int nw_send_heartbeat(net_addr *to)
{
    reliable_header packet;

    packet.type = RNT_I_AM_HERE;
    packet.seq = NW_IAMHERE_SEQ;
    packet.data_len = 0;
    nw_last_heartbeat_time = timer_get(1000);
    return nw_send_packet(&packet, NW_RELIABLE_HEADER_SIZE, 0, to, NW_TYPE_RELIABLE);
}

// 0x517dc0 float_compare: module -, not written here.

// 0x517e00  Confidence: likely (long function: the do / while (rcode > 0) form is from the rotated loop; the
// order of the header stores; the dead `cmp 0x258`).
// Defects: rsocket is not reset per packet, so a packet from an unknown address after one from a known peer
// is handled as the earlier peer's; a duplicate connection request still opens a second LIMBO link; the
// pings are int milliseconds sorted with float_compare (fine for non-negative values); the sequence wrap
// test adds theirsequence where the distance needs 0x10000 - theirsequence.
void nw_WorkReliable()
{
    char msg[256];
    char addr_text[256];
    reliable_header send_header;
    reliable_header heartbeat;
    int sort_ping[NW_PING_HISTORY];
    net_addr from;
    reliable_socket *rsocket = NULL;
    int rcode;
    int i;
    int j;

    nw_receive_all();
    if (nw_server_socket != -1 && nw_reliable_sockets[nw_server_socket].status == RNF_LIMBO) {
        int now = timer_get(1000);
        if (now < nw_last_heartbeat_time)
            nw_last_heartbeat_time = now;
        if (now - nw_last_heartbeat_time > 750) {
            if (nw_send_heartbeat(&nw_reliable_sockets[nw_server_socket].addr) == -1)
                nw_reliable_sockets[nw_server_socket].last_packet_sent = timer_get(1000) - 750;
            else
                nw_reliable_sockets[nw_server_socket].last_packet_sent = timer_get(1000);
        }
    }

    do {
        rcode = nw_reliable_slot_status(NW_TYPE_RELIABLE);
        if (!rcode)
            continue;
        int bytesin = nw_buffer_receive((ubyte *)&nw_reliable_recv_buf, sizeof(reliable_header), 0, &from,
                                        NW_TYPE_RELIABLE);
        if (bytesin < 0) {
            sprintf(msg, "recvfrom returned an error! -- %d\n", bytesin);
            return;
        }
        if (bytesin == 0)
            continue;

        if (nw_reliable_recv_buf.type == RNT_REQ_CONN) {
            for (i = 1; i < NW_MAX_RELIABLE_SOCKETS; i++) {
                if ((nw_reliable_sockets[i].status == RNF_CONNECTED || nw_reliable_sockets[i].status == RNF_LIMBO)
                    && nw_addr_equal(&from, &nw_reliable_sockets[i].addr, true) == 0) {
                    sprintf(msg, "Received duplicate connection request: %d\n", i);
                    nw_send_reliable_reply(&nw_reliable_sockets[i].addr, nw_reliable_recv_buf.seq,
                                           nw_reliable_recv_buf.send_time);
                    nw_reliable_recv_buf.type = RNT_IGNORE;
                }
            }
            for (i = 1; i < NW_MAX_RELIABLE_SOCKETS; i++) {
                if (nw_reliable_sockets[i].status == RNF_UNUSED) {
                    nw_reliable_sockets[i].addr = from;
                    nw_reliable_sockets[i].ping_pos = 0;
                    nw_reliable_sockets[i].num_ping_samples = 0;
                    nw_reliable_sockets[i].status = RNF_LIMBO;
                    nw_reliable_sockets[i].last_packet_received = timer_get(1000);
                    rsocket = &nw_reliable_sockets[i];
                    break;
                }
            }
            if (i == NW_MAX_RELIABLE_SOCKETS)
                continue;
            nw_send_reliable_reply(&rsocket->addr, nw_reliable_recv_buf.seq, nw_reliable_recv_buf.send_time);
            continue;
        }

        for (i = 1; i < NW_MAX_RELIABLE_SOCKETS; i++) {
            if (nw_addr_equal(&from, &nw_reliable_sockets[i].addr, true) == 0) {
                rsocket = &nw_reliable_sockets[i];
                break;
            }
        }
        if (rsocket == NULL) {
            nw_addr_to_string(addr_text, sizeof(addr_text), &from);
            sprintf(msg, "Received from %s\n", addr_text);
            continue;
        }

        rsocket->last_packet_received = timer_get(1000);
        if (rsocket->status != RNF_CONNECTED) {
            if (rsocket->status == RNF_LIMBO) {
                if (nw_server_socket != -1) {
                    if (nw_reliable_recv_buf.type == RNT_ACK) {
                        if (*(int *)nw_reliable_recv_buf.data == NW_IAMHERE_SEQ) {
                            rsocket->status = RNF_CONNECTED;
                            printf("Got ACK for IAMHERE!\n");
                        }
                        continue;
                    }
                } else if (nw_reliable_recv_buf.type == RNT_I_AM_HERE) {
                    rsocket->status = RNF_CONNECTING;
                    nw_send_reliable_reply(&rsocket->addr, nw_reliable_recv_buf.seq, nw_reliable_recv_buf.send_time);
                    printf("Got IAMHERE!\n");
                    continue;
                }
            }
            if (nw_reliable_recv_buf.type == RNT_DATA && nw_server_socket != -1) {
                rsocket->status = RNF_CONNECTED;
            } else {
                sprintf(msg, "Packet from nonconnected socket -- seq: %d status: %d\n", nw_reliable_recv_buf.seq,
                        rsocket->status);
                rsocket->last_packet_received = timer_get(1000);
                continue;
            }
        }

        rsocket->last_packet_received = timer_get(1000);
        if (nw_reliable_recv_buf.type == RNT_HEARTBEAT)
            continue;
        if (nw_reliable_recv_buf.type == RNT_ACK) {
            rsocket->num_ping_samples++;
            rsocket->pings[rsocket->ping_pos] = rsocket->last_packet_received - nw_reliable_recv_buf.send_time;
            if (rsocket->num_ping_samples >= NW_PING_HISTORY) {
                memcpy(sort_ping, rsocket->pings, sizeof(sort_ping));
                qsort(sort_ping, NW_PING_HISTORY, sizeof(int), float_compare);
                rsocket->mean_ping = (sort_ping[NW_PING_HISTORY / 2] + sort_ping[NW_PING_HISTORY / 2 + 1]) / 2;
            }
            rsocket->ping_pos++;
            if (rsocket->ping_pos >= NW_PING_HISTORY)
                rsocket->ping_pos = 0;
            int acked = *(int *)nw_reliable_recv_buf.data;
            for (j = 0; j < NW_MAX_NET_BUFFERS; j++) {
                if (rsocket->sbuffers[j] && rsocket->ssequence[j] == acked) {
                    Mempool *old_pool = get_current_mempool();
                    set_current_mempool(NULL);
                    delete rsocket->sbuffers[j];
                    set_current_mempool(old_pool);
                    rsocket->sbuffers[j] = NULL;
                    rsocket->ssequence[j] = 0;
                }
            }
            rsocket->last_packet_received = timer_get(1000);
            continue;
        }
        if (nw_reliable_recv_buf.type == RNT_DATA2)
            nw_reliable_recv_buf.type = RNT_DATA;
        else if (nw_reliable_recv_buf.type != RNT_DATA)
            continue;

        int distance = (int)nw_reliable_recv_buf.seq - (int)rsocket->theirsequence;
        if (distance < 0)
            distance = -distance;
        if (distance >= NW_MAX_NET_BUFFERS - 1)
            continue;
        int savepacket = 1;
        if (rsocket->theirsequence <= 0xffff - NW_MAX_NET_BUFFERS) {
            if (rsocket->theirsequence > nw_reliable_recv_buf.seq)
                savepacket = 0;
        } else if (nw_reliable_recv_buf.seq < rsocket->theirsequence
                   && (ushort)(rsocket->theirsequence + nw_reliable_recv_buf.seq + NW_MAX_NET_BUFFERS)
                          > NW_MAX_NET_BUFFERS - 1) {
            savepacket = 0;
        }
        for (j = 0; j < NW_MAX_NET_BUFFERS; j++) {
            if (rsocket->rbuffers[j] && rsocket->rsequence[j] == nw_reliable_recv_buf.seq)
                savepacket = 0;
        }
        if (savepacket) {
            for (j = 0; j < NW_MAX_NET_BUFFERS; j++) {
                if (rsocket->rbuffers[j] == NULL) {
                    rsocket->recv_len[j] = nw_reliable_recv_buf.data_len;
                    NW_DEBUG_CHECK(rsocket->recv_len[j] <= NW_MAX_RELIABLE_DATA);
                    Mempool *old_pool = get_current_mempool();
                    set_current_mempool(NULL);
                    rsocket->rbuffers[j] = new reliable_net_buffer;
                    set_current_mempool(old_pool);
                    memcpy(rsocket->rbuffers[j], nw_reliable_recv_buf.data, rsocket->recv_len[j]);
                    rsocket->rsequence[j] = nw_reliable_recv_buf.seq;
                    break;
                }
            }
        }
        nw_send_reliable_reply(&rsocket->addr, nw_reliable_recv_buf.seq, nw_reliable_recv_buf.send_time);
    } while (rcode > 0);

    for (i = 0; i < NW_MAX_RELIABLE_SOCKETS; i++) {
        rsocket = &nw_reliable_sockets[i];
        if (nw_server_socket == -1) {
            if (rsocket->status == RNF_LIMBO) {
                int now = timer_get(1000);
                if (now < rsocket->last_packet_received)
                    rsocket->last_packet_received = now;
                if (now - rsocket->last_packet_received > 100000) {
                    sprintf(msg, "Reliable (but in limbo) socket (%d) timed out in nw_WorkReliable().\n", i);
                    memset(rsocket, 0, sizeof(reliable_socket));
                    rsocket->status = RNF_UNUSED;
                }
            }
        } else {
            int now = timer_get(1000);
            if (now < nw_reliable_resend_time)
                nw_reliable_resend_time = now;
            if (rsocket->status == RNF_LIMBO && now - nw_reliable_resend_time > 100000) {
                rsocket->status = RNF_BROKEN;
                sprintf(msg, "Reliable socket (%d) timed out in nw_WorkReliable().\n", i);
            }
        }

        if (rsocket->status == RNF_CONNECTED) {
            float retry_cutoff;
            int mean_ping = rsocket->mean_ping;
            if (mean_ping != 0 && mean_ping <= 3000) {
                if (mean_ping < 100)
                    retry_cutoff = 100.0f;
                else
                    retry_cutoff = mean_ping * 1.25f;
            } else {
                retry_cutoff = 750.0f;
            }
            for (j = 0; j < NW_MAX_NET_BUFFERS; j++) {
                if (rsocket->sbuffers[j] == NULL)
                    continue;
                int now = timer_get(1000);
                if (now < rsocket->timesent[j])
                    rsocket->timesent[j] = now;
                int elapsed = now - rsocket->timesent[j];
                if ((float)elapsed < retry_cutoff)
                    continue;
                send_header.type = RNT_DATA;
                send_header.seq = rsocket->ssequence[j];
                send_header.data_len = rsocket->send_len[j];
                send_header.send_time = timer_get(1000);
                memcpy(send_header.data, rsocket->sbuffers[j], rsocket->send_len[j]);
                if (nw_send_packet(&send_header, rsocket->send_len[j] + NW_RELIABLE_HEADER_SIZE, 0, &rsocket->addr,
                                   NW_TYPE_RELIABLE) == -2) {
                    rsocket->timesent[j] = timer_get(1000) - 3000;
                } else {
                    rsocket->last_packet_sent = timer_get(1000);
                    rsocket->timesent[j] = timer_get(1000);
                }
            }
        }

        if (rsocket->status == RNF_CONNECTED) {
            int now = timer_get(1000);
            if (now < rsocket->last_packet_sent)
                rsocket->last_packet_sent = now;
            if (now - rsocket->last_packet_sent > 10000) {
                heartbeat.type = RNT_HEARTBEAT;
                heartbeat.seq = 0;
                heartbeat.data_len = 0;
                heartbeat.send_time = timer_get(1000);
                if (nw_send_packet(&heartbeat, NW_RELIABLE_HEADER_SIZE, 0, &rsocket->addr, NW_TYPE_RELIABLE) >= 0)
                    rsocket->last_packet_sent = timer_get(1000);
            }
            if (now < rsocket->last_packet_received)
                rsocket->last_packet_received = now;
            if (now - rsocket->last_packet_received > 100000) {
                sprintf(msg, "Reliable Socket (%d) timed out in nw_WorkReliable().\n", i);
                rsocket->status = RNF_BROKEN;
            }
        }
    }
}

// 0x518770  Confidence: likely (from_len is cleared before the select and set to 16 before recvfrom; the
// type test keeps its `< 0` half). Drains the socket; packets with a first byte above 4 are dropped.
void nw_receive_all()
{
    for (;;) {
        int from_len = 0;
        net_addr addr;
        struct timeval timeout;
        struct sockaddr_in from;
        fd_set read_fds;
        ubyte packet[NW_MAX_PACKET];

        FD_ZERO(&read_fds);
        FD_SET(nw_udp_socket, &read_fds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        if (select(0, &read_fds, NULL, NULL, &timeout) == SOCKET_ERROR)
            return;
        if (!FD_ISSET(nw_udp_socket, &read_fds))
            return;
        from_len = sizeof(from);
        int bytes = recvfrom(nw_udp_socket, (char *)packet, sizeof(packet), 0, (struct sockaddr *)&from, &from_len);
        addr.port = ntohs(from.sin_port);
        addr.ip = ntohl(from.sin_addr.s_addr);
        if (bytes == SOCKET_ERROR)
            return;
        int type = packet[0];
        if (type < 0 || type >= NW_NUM_BUFFERS)
            continue;
        nw_buffer_push(&nw_recv_buffers[type], packet + 1, bytes - 1, &addr);
    }
}

// 0x518880  Confidence: exact.
void nw_close()
{
    if (nw_winsock_started) {
        WSACancelBlockingCall();
        if (nw_udp_socket != INVALID_SOCKET) {
            shutdown(nw_udp_socket, 1);
            closesocket(nw_udp_socket);
        }
        WSACleanup();
        nw_winsock_started = 0;
        nw_socket = 0;
    }
}

// 0x5188d0  Confidence: likely (VC6 shares the `status = 0; return` of "none" with an unknown speed).
// "slow" and "56k" probe RAS (nw_networking_ok = a dial-up IP was found); "fast" skips it; anything else
// (and "none") stops before WinSock.
void nw_init_networking(unsigned short port)
{
    char msg[256];
    WSADATA wsa_data;

    strcpy(nw_connection_speed, "None");
    os_registry_read_string_nodefault(NULL, "ConnectionSpeed", nw_connection_speed);
    if (_stricmp(nw_connection_speed, "none") == 0) {
        nw_network_status = 0;
        return;
    }
    if (_stricmp(nw_connection_speed, "slow") == 0) {
        nw_network_status = 1;
    } else if (_stricmp(nw_connection_speed, "56k") == 0) {
        nw_network_status = 2;
    } else if (_stricmp(nw_connection_speed, "fast") == 0) {
        nw_network_status = 3;
        goto start_winsock;
    } else {
        nw_network_status = 0;
        return;
    }
    nw_networking_ok = nw_ras_init() != 0;
start_winsock:
    int error = WSAStartup(0x101, &wsa_data);
    if (error != 0) {
        sprintf(msg, "There was an error initializing networking! Error=%d\n", error);
        return;
    }
    nw_winsock_started = 1;
    atexit(nw_close);
    nw_init_socket(port);
    for (int i = 0; i < NW_NUM_BUFFERS; i++)
        nw_buffer_init(&nw_recv_buffers[i]);
}

// 0x518a30  Confidence: exact (size is not read).
void nw_addr_to_string(char *out, int size, net_addr *addr)
{
    char text[64];
    struct in_addr ip;

    ip.s_addr = htonl(addr->ip);
    sprintf(text, "IP: %s %d", inet_ntoa(ip), addr->port);
    strcpy(out, text);
}

// 0x518a90  Confidence: exact.
int nw_send_broadcast(unsigned short port, void *data, int len)
{
    net_addr to;

    to.port = port;
    to.ip = 0xffffffff;
    return nw_send(&to, data, len);
}

// 0x518ac0  Confidence: likely (the port-0 message pushes the constant 0).
int nw_send(net_addr *to, void *data, int len)
{
    char msg[256];

    if (!nw_socket)
        return 0;
    if (to->port == 0) {
        sprintf(msg, "Network ==> destination port %d invalid in psnet_send\n", to->port);
        return 0;
    }
    int ret = nw_can_write();
    if (ret < 0) {
        sprintf(msg, "Error on blocking select for write %d\n", ret);
        return 0;
    }
    if (ret == 0)
        return 0;
    ret = nw_send_packet(data, len, 0, to, NW_TYPE_UNRELIABLE);
    if (ret == -2)
        return 0;
    if (ret == -1) {
        sprintf(msg, "Couldn't send data! (%d)\n", ret);
        return 0;
    }
    return 1;
}

// 0x518b80  Confidence: exact (size in from's argument slot).
int nw_receive(ubyte *data, net_addr *from)
{
    int size;

    if (nw_buffer_pop(&nw_recv_buffers[NW_TYPE_UNRELIABLE], data, &size, from))
        return size;
    return 0;
}

// 0x518bb0 nw_NewReceiveReliable: module -, not written here.

// 0x518ce0  Confidence: likely (the found branch after the loop).
int nw_accept_reliable(net_addr *from)
{
    char addr_text[64];
    int i;

    nw_WorkReliable();
    for (i = 1; i < NW_MAX_RELIABLE_SOCKETS; i++) {
        if (nw_reliable_sockets[i].status == RNF_CONNECTING)
            break;
    }
    if (i == NW_MAX_RELIABLE_SOCKETS)
        return -1;
    nw_reliable_sockets[i].status = RNF_CONNECTED;
    *from = nw_reliable_sockets[i].addr;
    nw_addr_to_string(addr_text, sizeof(addr_text), from);
    sprintf(string_buffer, "Got address from: %s\n", addr_text);
    return i;
}

// 0x518d70 nw_NewSendReliable: module -, not written here.

// 0x518f30  Confidence: likely (the loop: the reply path skips the start-time wrap update).
// Sends the connection request every 2 s until an ACK with NW_CONNECT_SEQ arrives or 100 s pass, then
// opens a LIMBO link, sends IAMHERE and runs nw_WorkReliable for up to 2 s waiting for the ACK.
// Defect: when the IAMHERE send fails the slot is cleared but nw_server_socket and *socket keep its index.
void nw_ConnectToServer(int *socket, net_addr *server)
{
    net_addr from;
    char msg[256];
    int i;

    if (!nw_socket)
        return;
    nw_connect_request.type = RNT_REQ_CONN;
    nw_connect_request.seq = NW_CONNECT_SEQ;
    nw_connect_request.data_len = 0;
    nw_flush_receive();
    memset(&nw_connect_reply_buf, 0, sizeof(nw_connect_reply_buf));
    int ret = nw_send_packet(&nw_connect_request, NW_RELIABLE_HEADER_SIZE, 0, server, NW_TYPE_RELIABLE);
    if (ret < 0) {
        sprintf(msg, "Unable to send UDP packet in nw_ConnectToServer()! -- %d\n", ret);
        return;
    }

    int start = timer_get(1000);
    int last_sent = start;
    int now = start;
    for (;;) {
        nw_receive_all();
        Sleep(10);
        if (nw_reliable_slot_status(NW_TYPE_RELIABLE)) {
            ret = nw_buffer_receive((ubyte *)&nw_connect_reply_buf, sizeof(reliable_header), 0, &from,
                                    NW_TYPE_RELIABLE);
            if (ret < 0) {
                sprintf(msg, "recvfrom returned an error! -- %d\n", ret);
                return;
            }
            if (ret != 0 && nw_connect_reply_buf.type == RNT_ACK
                && *(short *)nw_connect_reply_buf.data == NW_CONNECT_SEQ)
                break;
        } else {
            now = timer_get(1000);
            if (now < last_sent)
                last_sent = now;
            if (now - last_sent > 2000) {
                ret = nw_send_packet(&nw_connect_request, NW_RELIABLE_HEADER_SIZE, 0, server, NW_TYPE_RELIABLE);
                if (ret > 0)
                    last_sent = timer_get(1000);
                else
                    sprintf(msg, "Error sending connection request! -- %d\n", ret);
            }
            if (now < start)
                start = now;
        }
        if (now - start >= 100000)
            return;
    }

    for (i = 1; i < NW_MAX_RELIABLE_SOCKETS; i++) {
        if (nw_reliable_sockets[i].status == RNF_UNUSED)
            break;
    }
    if (i == NW_MAX_RELIABLE_SOCKETS)
        return;
    reliable_socket *rsocket = &nw_reliable_sockets[i];
    rsocket->addr = *server;
    rsocket->last_packet_received = timer_get(1000);
    rsocket->status = RNF_LIMBO;
    nw_server_socket = i;
    *socket = i;
    if (nw_send_heartbeat(&rsocket->addr) == -1) {
        rsocket->status = RNF_UNUSED;
        memset(rsocket, 0, sizeof(reliable_socket));
        return;
    }
    nw_reliable_resend_time = timer_get(1000);
    rsocket->last_packet_sent = timer_get(1000);
    int wait_start = timer_get(1000);
    do {
        nw_WorkReliable();
        now = timer_get(1000);
        if (now < wait_start)
            wait_start = now;
        if (now - wait_start >= 2000)
            return;
    } while (rsocket->status != RNF_CONNECTED);
}

// 0x5191c0 nw_NewCloseSocket: module -, not written here.

// 0x519370  Confidence: exact.
int nw_addr_equal(const void *a, const void *b, bool check_port)
{
    const net_addr *x = (const net_addr *)a;
    const net_addr *y = (const net_addr *)b;

    if (x->ip == y->ip && (!check_port || x->port == y->port))
        return 0;
    return -1;
}

// 0x5193a0  Confidence: exact (VC6 byte match; `jb`: the id compared as unsigned). The status of a
// reliable socket, -1 for a bad id (the message goes to a local buffer, never printed). Module - in
// sum.symbols; between nw_addr_equal and nw_flush_receive, so this object's (D5610).
int net_get_reliable_socket_status(int socket)
{
    char msg[256];

    if ((unsigned int)socket >= NW_MAX_RELIABLE_SOCKETS) {
        sprintf(msg, "Invalid socket id passed to net_get_reliable_socket_status() -- %d\n", socket);
        return -1;
    }
    return nw_reliable_sockets[socket].status;
}

// 0x5193f0  Confidence: exact (VC6 byte match). The mean ping of a reliable socket.
int net_get_reliable_socket_ping(int socket)
{
    char msg[256];

    if ((unsigned int)socket >= NW_MAX_RELIABLE_SOCKETS) {
        sprintf(msg, "Invalid socket id passed to net_get_reliable_socket_ping() -- %d\n", socket);
        return -1;
    }
    return nw_reliable_sockets[socket].mean_ping;
}

// 0x519440  Confidence: exact.
void nw_flush_receive()
{
    net_addr from;
    ubyte data[NW_MAX_PACKET];

    while (nw_receive(data, &from) > 0)
        ;
    for (int i = 0; i < NW_NUM_BUFFERS; i++) {
        while (nw_reliable_slot_status(i))
            nw_buffer_receive((ubyte *)&nw_flush_buf, sizeof(reliable_header), 0, &from, i);
    }
}
