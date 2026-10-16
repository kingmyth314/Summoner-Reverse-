// unnamed_kick_0x471df0.cpp - Sum.exe v1.40, code 0x471df0-0x471fe5 (the host removing a player: KICK_PL).
// THE FILE NAME IS A PLACEHOLDER: no path string, no data of its own; it reads strings.tbl ids 233-234 of the
// "// multiendgame.cpp" block, but multiendgame.cpp is the contiguous block 0x4811a0-0x481673, so the ids
// are shared (D2604). Whether these three functions end multitrade.cpp (0x470e30-0x471def) or are a file
// of their own is open (source-map.md); they are written as their own unit (decomp76, D5808). Written from
// the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2. Confidence per function:
// exact (control flow, calls, constants and globals match), likely (one detail of statement order or
// expression form is a guess), unsure (the shape is a guess).

#include <stdio.h>
#include <string.h>
#include "vsdk/localize/localize.h"
#include "summoner/summoner.h"
#include "summoner/multi/multi.h"
#include "summoner/multi/multichat.h"
#include "summoner/multi/multiendgame.h"
#include "summoner/multi/unnamed_netpackets_0x441820.h"
#include "summoner/multi/unnamed_netsession_0x440b40.h"

// 0x471df0  Confidence: likely (VC6 pops the arguments of the three calls of a case at once; the original
// pops after the first). The host drops another player: the player is told
// (KICK_PL), unlinked and counted out, the others get GM_OVER with the matching reason (0 -> 6, 1 kicked
// -> 1, 2 -> 5) and the chat shows "%s was kicked / dropped from the game.".
void mp_remove_player(mp_player *p, int reason)
{
    char name[0x20];
    char msg[0x100];
    int id;

    if (!MP_IS_HOST())
        return;
    if (p == mp_local_player)
        return;
    id = p->id;
    strcpy(name, p->name);
    mp_send_removed_notice(p, reason);
    mp_player_disconnect(p);
    mp_netgame_num_players--;
    switch (reason) {
    case 0:
        mp_send_player_removed(6, id);
        sprintf(msg, XSTR("%s was dropped from the game.", 234), name);
        chat_process_command(-1, msg);
        break;
    case 1:
        mp_send_player_removed(1, id);
        sprintf(msg, XSTR("%s was kicked from the game.", 233), name);
        chat_process_command(-1, msg);
        break;
    case 2:
        mp_send_player_removed(5, id);
        sprintf(msg, XSTR("%s was dropped from the game.", 234), name);
        chat_process_command(-1, msg);
        break;
    }
}

// 0x471f40  Confidence: exact. KICK_PL {player id, reason} reliably to the removed player.
void mp_send_removed_notice(mp_player *p, int reason)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    MP_BUILD_HEADER(MP_KICK_PL);
    MP_ADD_DATA(p->id);
    MP_ADD_DATA(reason);
    MP_END_PACKET();
    multi_send_packet(data, packet_size, p, true);
}

// 0x471f90  Confidence: exact (VC6 byte match). KICK_PL on the removed client: it leaves the game with
// the reason the host gave (0 -> 6, 1 kicked -> 1, 2 -> 5).
void mp_handle_kick_pl(ubyte *data, void *from)
{
    int id = ((int *)data)[0];
    int reason = ((int *)data)[1];
    mp_player *p = mp_player_find_by_id(id);

    if (!p || p != mp_local_player)
        return;
    switch (reason) {
    case 0:
        mp_player_disconnected(6, false);
        break;
    case 1:
        mp_player_disconnected(1, false);
        break;
    case 2:
        mp_player_disconnected(5, false);
        break;
    }
}
