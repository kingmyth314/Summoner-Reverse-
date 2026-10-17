// unnamed_utrack_0x48d7c0.cpp - Sum.exe v1.40, code 0x48d7c0-0x48db2f: the PXO user tracker login (the
// request, its acknowledgement, the reply poll with resend and timeout). THE FILE NAME IS A PLACEHOLDER:
// no assert path, no strings.tbl block, module "-" in sum.symbols.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Unit: its .bss 0x2318e48-0x231906f lies between the effect objects' fx_count (0x2318e40) and
// levelrender.cpp's .bss (0x2319070); no .data, no initialiser; levelrender.cpp's code follows at 0x48db30.
// Callers: pxo_read_login / pxo_login_step (the interface's PXO login, 0x42f230 / 0x42f290) and
// tracker_init (0x4ab930).

#include <string.h>
#include <winsock.h>
#include "vsdk/os/timer.h"
#include "vsdk/net/net.h"
#include "summoner/multi/multi.h"

utrack_login_request *pxo_login_data;                 // 0x2318e48
utrack_packet pxo_login_packet;                       // 0x2318e50
net_addr utrack_addr;                                 // 0x2319058
int utrack_state;                                     // 0x2319060
char *utrack_reply_buffer;                            // 0x2319064
int utrack_last_send_time;                            // 0x2319068
int utrack_request_time;                              // 0x231906c

// 0x48d7c0  Confidence: exact. pxo_login_poll: acknowledges a reply (type 0, code 5, its signature).
void pxo_login_send(uint sig)
{
    utrack_packet p;

    p.type = 0;
    p.sig = sig;
    p.code = 5;
    p.len = UTRACK_HEADER_SIZE;
    nw_send_packet(&p, UTRACK_HEADER_SIZE, 0, &utrack_addr, 2);
}

// 0x48d810  Confidence: exact. tracker_init: the user tracker host (pxo_cfg, "ut.pxo.net" by default) by
// address or name, port 10556.
int utrack_init()
{
    unsigned long ip;
    hostent *he;

    utrack_request_time = 0;
    utrack_last_send_time = 0;
    utrack_state = 1;
    ip = inet_addr(pxo_utracker_server);
    if (ip == INADDR_NONE) {
        he = gethostbyname(pxo_utracker_server);
        if (!he)
            return 0;
        ip = *(unsigned long *)he->h_addr_list[0];
    }
    utrack_addr.ip = ntohl(ip);
    utrack_addr.port = 0x293c;
    return 1;
}

// 0x48d870  Confidence: likely (the switch over the state: jump table 0x48d9e0 for 1..5). With a request
// (only when idle): drains one waiting reply, sends the login (type 0xd, 0x4d bytes) and starts the
// timers. Without: polls; a final state (accepted, refused, timed out) is returned once and reset to idle.
int pxo_login_request(utrack_login_request *req, char *reply)
{
    ubyte buf[0x204];
    net_addr from;

    pxo_login_poll();
    if (req == NULL) {
        switch (utrack_state) {
        case 3:
            utrack_state = 1;
            return 1;
        case 4:
            utrack_state = 1;
            return -1;
        case 5:
            utrack_state = 1;
            return -2;
        case 1:
        case 2:
        default:
            return 0;
        }
    }
    if (utrack_state != 1)
        return -3;
    if (nw_reliable_slot_status(2))
        nw_buffer_receive(buf, sizeof(buf), 0, &from, 2);
    utrack_reply_buffer = reply;
    pxo_login_packet.type = 0xd;
    pxo_login_packet.len = UTRACK_HEADER_SIZE + sizeof(utrack_login_request);
    pxo_login_data = (utrack_login_request *)pxo_login_packet.data;
    strcpy(pxo_login_data->login, req->login);
    strcpy(pxo_login_data->password, req->password);
    nw_send_packet(&pxo_login_packet, UTRACK_HEADER_SIZE + sizeof(utrack_login_request), 0, &utrack_addr, 2);
    utrack_state = 2;
    utrack_request_time = timer_get(1000);
    utrack_last_send_time = timer_get(1000);
    return 0;
}

// 0x48da00  Confidence: exact (the length test: the packet's length is decremented in place, then
// compared). A reply of the right length is acknowledged whatever its type; 0xf (accepted, the 10-byte
// reply copied out) and 0xe (refused) end the wait. While waiting: give up after 30 s, resend every 3.5 s.
void pxo_login_poll()
{
    net_addr from;
    utrack_packet p;
    int n;
    int t;

    nw_receive_all();
    if (nw_reliable_slot_status(2)) {
        n = nw_buffer_receive((ubyte *)&p, sizeof(p), 0, &from, 2);
        p.len--;
        if (n == p.len) {
            switch (p.type) {
            case 0xe:
                if (utrack_state == 2)
                    utrack_state = 4;
                break;
            case 0xf:
                if (utrack_state == 2) {
                    utrack_state = 3;
                    strncpy(utrack_reply_buffer, (char *)p.data, 10);
                }
                break;
            }
            pxo_login_send(p.sig);
        }
    }
    if (utrack_state == 2) {
        t = timer_get(1000);
        if (t < utrack_request_time)
            utrack_request_time = t;
        if (t - utrack_request_time >= 30000) {
            utrack_state = 5;
            return;
        }
        if (t - utrack_last_send_time >= 3500) {
            nw_send_packet(&pxo_login_packet, pxo_login_packet.len, 0, &utrack_addr, 2);
            utrack_last_send_time = t;
        }
    }
}
