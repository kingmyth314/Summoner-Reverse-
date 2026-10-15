// vsdk/net/net.h - network layer state (0x2708000-0x2710fff .bss; docs/sum-exe/vsdk-os.md "Networking";
// directory and file name guessed). Not original source.
#ifndef VSDK_NET_NET_H
#define VSDK_NET_NET_H

extern int nw_network_status;                                       // 0x270812c
extern int nw_networking_ok;                                        // 0x2710e9c
extern int nw_winsock_started;                                      // 0x2710ea4

// The ping field of a reliable socket record (index below 10; own assert). decomp24 (chat_cmd_ping).
int net_get_reliable_socket_ping(int socket);                       // 0x5193f0

#endif
