// multiplayerdata.cpp - Sum.exe v1.40, code 0x4af340-0x4afc99 (the player records of a network game)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Unit: sum.symbols module column (strings.tbl id 236, the whole `// multiplayerdata.cpp` block, in
// mp_announce_player_joined). It follows the level-object ray tests (lighting_sample_stub 0x4af320, module
// "-") and ends before multispell.cpp (0x4afca0).
// Data: .data 0x59d648-0x59d697 (the "already connected" line); no .bss (the send counters are
// mp_packet_table's).
//
// The packets: NEW_INF (6) announces one new player to everybody (host), NET_INF (5) gives a joining player
// the records of the others, one packet per player, the last one flagged; PLY_DAT (7) carries a party
// member's equipment, skills and stats. A record is {id, char_info, profile_flags, type, name\0}.

#include <stdio.h>
#include <string.h>
#include "vsdk/os/console.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/localize/localize.h"
#include "vsdk/net/net.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/item_paging.h"
#include "engine/objects/living_entity.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"

#define MP_NAME_WIDTH 50              // pixels of mp_player::display_name (push 0x32)

// 0x4af340  Confidence: likely (the flag store is scheduled before the size store; source order guessed).
void mp_broadcast_new_info(mp_player *p)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    MP_BUILD_HEADER(MP_NEW_INF);
    mp_pack_player_info(p, data, &packet_size);
    p->flags |= MP_PF_INFO_SENT;
    MP_END_PACKET();
    mp_broadcast_packet(data, packet_size, 1);
    mp_announce_player_joined(p);
}

// 0x4af3c0  Confidence: exact.
void mp_announce_player_joined(mp_player *p)
{
    char text[256];

    sprintf(text, XSTR("%s has entered the game.", 236), p->name);
    chat_process_command(-1, text);
}

// 0x4af400  Confidence: exact. The name length is cut to 16 bits (`and ecx, 0xffff` before the copy).
void mp_pack_player_info(mp_player *p, ubyte *data, ushort *offset)
{
    ushort len;

    *(int *)(data + *offset) = p->id;
    *offset += 4;
    *(int *)(data + *offset) = p->char_info;
    *offset += 4;
    *(int *)(data + *offset) = p->profile_flags;
    *offset += 4;
    *(int *)(data + *offset) = p->type;
    *offset += 4;
    len = (ushort)strlen(p->name);
    memcpy(data + *offset, p->name, len);
    *offset += len;
    data[(*offset)++] = 0;
}

// 0x4af4b0  Confidence: exact.
void mp_handle_new_info(ubyte *data, void *from)
{
    ushort offset = 0;
    mp_player *p;

    p = mp_add_new_player(data, &offset);
    if (p)
        mp_announce_player_joined(p);
}

// 0x4af4e0  Confidence: likely (the local name buffer's size: the frame has 0x40 bytes after the four
// ints). A known id only writes the message to string_buffer (the print is compiled out) and returns the
// existing record, so its "has entered" line is shown again. Out of records: the local player leaves with
// "Insufficient memory" (reason 3; the second argument is the NULL in eax).
mp_player *mp_add_new_player(ubyte *data, ushort *offset)
{
    char name[64];
    int id, char_info, profile_flags, type;
    int len;
    mp_player *p;

    id = *(int *)(data + *offset);
    *offset += 4;
    char_info = *(int *)(data + *offset);
    *offset += 4;
    profile_flags = *(int *)(data + *offset);
    *offset += 4;
    type = *(int *)(data + *offset);
    *offset += 4;
    len = strlen((char *)data + *offset);
    strcpy(name, (char *)data + *offset);
    *offset += len + 1;

    if (id == mp_local_player->id)
        return NULL;
    p = mp_player_find_by_id(id);
    if (p) {
        sprintf(string_buffer, "Received new player packet but player %s is already connected with this id\n",
                p->name);
        return p;
    }
    p = mp_player_alloc();
    if (!p) {
        mp_player_disconnected(MP_LEAVE_NO_MEMORY, false);
        return NULL;
    }
    p->type = type;
    p->char_info = char_info;
    p->id = id;
    p->socket = -1;
    p->profile_flags = profile_flags;
    strcpy(p->name, name);
    gr_string_truncate_ellipsis(p->display_name, name, MP_NAME_WIDTH, -1);
    mp_netgame_num_players++;
    return p;
}

// 0x4af670  Confidence: likely (the `> 0` test of the pending packet; nw_addr_equal is 0 for a match, so the
// test keeps the players at another address than `to`). Each packet is closed with MP_NET_INF_MORE when
// the next one is started, the last with MP_NET_INF_LAST; all go reliably to `to`.
void mp_send_net_info(mp_player *to)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size = 0;
    mp_player *p;

    for (p = mp_player_list.next; p != &mp_player_list; p = p->next) {
        if (nw_addr_equal(p->addr, to->addr, true)) {
            if (packet_size > 0) {
                data[packet_size++] = MP_NET_INF_MORE;
                MP_END_PACKET();
                multi_send_packet(data, packet_size, to, 1);
            }
            MP_BUILD_HEADER(MP_NET_INF);
            mp_pack_player_info(p, data, &packet_size);
        }
    }
    data[packet_size++] = MP_NET_INF_LAST;
    MP_END_PACKET();
    multi_send_packet(data, packet_size, to, 1);
}

// 0x4af760  Confidence: exact. The server's record takes the address the join was accepted from. The
// last-packet byte is read only when a record was made (mp_add_new_player NULL: never read; NOTES.md).
void mp_handle_net_info(ubyte *data, void *from)
{
    ushort offset = 0;
    mp_player *p;

    p = mp_add_new_player(data, &offset);
    if (!p)
        return;
    mp_announce_player_joined(p);
    if (p->id == mp_server_player_id)
        memcpy(p->addr, mp_server_addr, sizeof(mp_server_addr));
    if (data[offset++] == MP_NET_INF_LAST)
        mp_set_local_status(3);
}

// 0x4af7d0  Confidence: likely (the equipment ternary; the two floats go through a stack temporary,
// MP_ADD_FLOAT). {player id, character handle, 9 equipment type indices (-1 empty), 0x54 skill bytes,
// skill points, level (short), xp, base max HP, HP, base max AP (short), AP (short)}. A client sends it
// unreliably.
void mp_send_player_data(mp_player *p)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    Player *pl;
    Character *c;
    int i;
    int type;

    if (!p)
        return;
    pl = p->object;
    if (!pl)
        return;
    c = living_from_handle(pl->handle);
    if (!c)
        return;

    MP_BUILD_HEADER(MP_PLY_DAT);
    MP_ADD_DATA(p->id);
    MP_ADD_DATA(c->handle);
    for (i = 0; i < MP_PLY_DAT_EQUIP; i++) {
        type = c->equipment[i] ? c->equipment[i]->type - item_types : -1;
        MP_ADD_DATA(type);
    }
    for (i = 0; i < MP_PLY_DAT_SKILLS; i++)
        data[packet_size++] = c->skill[i];
    MP_ADD_DATA(pl->skill_points);
    MP_ADD_DATA(c->level);
    MP_ADD_DATA(c->xp);
    MP_ADD_FLOAT(c->base_max_hp);
    MP_ADD_FLOAT(c->hp);
    MP_ADD_DATA(c->base_max_ap);
    MP_ADD_DATA(c->ap);
    MP_END_PACKET();
    if (MP_IS_HOST())
        mp_broadcast_packet(data, packet_size, 1);
    else
        multi_send_packet(data, packet_size, NULL, 0);
}

// 0x4af990  Confidence: likely (the two out slots of character_equip_item: the first is the dead `data`
// argument slot; the clamps are max() expansions after the plain stores). For our own character only the
// handle mapping is kept. The host answers with the PLY_DAT of its copy (to every player).
void mp_handle_player_data(ubyte *data, void *from)
{
    int id = ((int *)data)[0];
    int net_handle = ((int *)data)[1];
    ushort offset = 8;
    mp_player *mp;
    Player *pl;
    Character *c;
    item_info *removed;
    item_info *removed2;
    int i;
    int type;

    mp = mp_player_find_by_id(id);
    if (!mp)
        return;
    pl = mp->object;
    if (!pl)
        return;
    c = living_from_handle(pl->handle);
    if (!c)
        return;
    if (c == cur_character) {
        if (MP_IS_CLIENT())
            mp_map_net_handle(net_handle, c->handle);
        return;
    }

    for (i = 0; i < MP_PLY_DAT_EQUIP; i++) {
        removed = NULL;
        removed2 = NULL;
        type = *(int *)(data + offset);
        offset += 4;
        if (type >= 0)
            character_equip_item(&removed, &removed2, c, i, item_create(type));
        else
            removed = character_unequip_slot(c, i, 1);
        if (removed)
            item_free(removed);
        if (removed2)
            item_free(removed2);
    }
    mp_page_in_player_items(c);
    for (i = 0; i < MP_PLY_DAT_SKILLS; i++)
        c->skill[i] = data[offset++];
    pl->skill_points = *(int *)(data + offset);
    offset += 4;
    c->level = *(short *)(data + offset);
    offset += 2;
    c->xp = *(int *)(data + offset);
    offset += 4;
    c->base_max_hp = *(float *)(data + offset);
    offset += 4;
    c->hp = *(float *)(data + offset);
    offset += 4;
    c->hp = max(c->hp, 0.0f);
    c->base_max_ap = *(short *)(data + offset);
    offset += 2;
    c->ap = *(short *)(data + offset);
    c->ap = max(c->ap, 0);

    if (MP_IS_CLIENT()) {
        mp_map_net_handle(net_handle, c->handle);
        pl->mp_owner->flags |= MP_PF_DATA_RECEIVED;
    } else {
        mp_send_player_data(pl->mp_owner);
        pl->mp_owner->flags |= MP_PF_DATA_RECEIVED;
    }
}

// 0x4afbe0  Confidence: likely (`slot1` lives in the dead argument slot; the i != 5 / 6 tests are pointer
// compares against equip_page_slots + 5 / + 6). The weapon (5) and shield (6) take the player's two
// paging slots, the rest the fixed slot of their equipment index.
void mp_page_in_player_items(Character *c)
{
    int slot1, slot2;
    int i;

    if (!c)
        return;
    item_paging_find_player_slot(c, &slot1, &slot2);
    if (c->equipment[5])
        item_page_in(c->equipment[5]->type - item_types, slot1, 0);
    if (c->equipment[6])
        item_page_in(c->equipment[6]->type - item_types, slot2, 0);
    for (i = 0; i < 9; i++) {
        if (i != 5 && i != 6 && c->equipment[i])
            item_page_in(c->equipment[i]->type - item_types, equip_page_slots[i], 0);
    }
}
