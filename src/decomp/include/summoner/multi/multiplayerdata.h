// summoner/multi/multiplayerdata.h - multiplayerdata.cpp (0x4af340-0x4afc9f: the player records of a network
// game, packets 5 NET_INF, 6 NEW_INF and 7 PLY_DAT; strings.tbl id 236). File name from the module column of
// sum.symbols, directory ours. Included by summoner/multi/multi.h. Not original source (decomp47).
#ifndef SUMMONER_MULTI_MULTIPLAYERDATA_H
#define SUMMONER_MULTI_MULTIPLAYERDATA_H

#include "vsdk/vsdk.h"

struct mp_player;
class Character;

// NET_INF's last byte: another packet follows (1) or this was the last player (2). Names ours.
#define MP_NET_INF_MORE 1
#define MP_NET_INF_LAST 2

// PLY_DAT carries the first 0x54 skill bytes (Character +0x614) and nine equipment slots (names ours).
#define MP_PLY_DAT_SKILLS 0x54
#define MP_PLY_DAT_EQUIP  9

void mp_broadcast_new_info(mp_player *p);                         // 0x4af340 NEW_INF (host, a new player)
void mp_announce_player_joined(mp_player *p);                     // 0x4af3c0 "%s has entered the game."
// {id, char_info, profile_flags, type, name\0} at *offset, which is advanced.
void mp_pack_player_info(mp_player *p, ubyte *data, ushort *offset);   // 0x4af400
void mp_handle_new_info(ubyte *data, void *from);                 // 0x4af4b0 packet 6
// The record of the packet at *offset: NULL for ourselves or when no record is free, the existing record
// when the id is known.
mp_player *mp_add_new_player(ubyte *data, ushort *offset);        // 0x4af4e0
void mp_send_net_info(mp_player *to);                             // 0x4af670 NET_INF, one packet per player
void mp_handle_net_info(ubyte *data, void *from);                 // 0x4af760 packet 5
void mp_send_player_data(mp_player *p);                           // 0x4af7d0 PLY_DAT of p's character
void mp_handle_player_data(ubyte *data, void *from);              // 0x4af990 packet 7
void mp_page_in_player_items(Character *c);                       // 0x4afbe0

#endif
