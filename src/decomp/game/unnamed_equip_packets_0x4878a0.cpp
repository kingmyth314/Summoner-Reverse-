// unnamed_equip_packets_0x4878a0.cpp - Sum.exe v1.40, code 0x4878a0-0x487b8f: the INV_EQP / INV_UQP
// packets (a party member's equipment change in a network game).
// THE FILE NAME IS A PLACEHOLDER, AND THE UNIT ITSELF IS AN ASSUMPTION: the four functions have no .data
// or .bss of their own (the send counters are mp_packet_table's, 0x595758 / 0x5958e0), so nothing marks
// where they belong. They follow place_ground_decal (shadow-table unit, last RET 0x487898) and precede
// multichat.cpp (chat_cmd_kick 0x487b90, whose .data starts with its own command table at 0x59c180). They
// may be the tail of the shadow-table unit or the head of multichat.cpp; a file of their own is the
// assumption with the least to undo (NOTES.md).
// Written from the Sum.exe disassembly (tools/asm.mjs, llvm-objdump). Target: VC++ 6.0 (cl 12.00.8966),
// /O2 /Ob1. Confidence per function: exact (control flow, calls, constants and globals match), likely (same,
// but one detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// mp_handle_unequip_item (0x487b10-0x487b83), outside decomp63's range, was added at the end of the file
// by decomp76.

#include <string.h>
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/item_paging.h"
#include "engine/objects/living_entity.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"

// 0x4878a0  Confidence: exact. INV_EQP (player id, slot, item type) for a party member: the host
// broadcasts it reliably, a client sends it to the host.
void mp_send_equip_item(Character *c, int slot, int type)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    if (c->type != 7)
        return;
    mp_player *p = mp_player_find_by_name(c->name);
    if (!p)
        return;
    MP_BUILD_HEADER(MP_INV_EQP);
    MP_ADD_DATA(p->id);
    MP_ADD_DATA(slot);
    MP_ADD_DATA(type);
    MP_END_PACKET();
    if (MP_IS_HOST())
        mp_broadcast_packet(data, packet_size, 1);
    else
        multi_send_packet(data, packet_size, NULL, 0);
}

// 0x487950  Confidence: likely (the first old-item pointer lives in the dead data argument's slot). The
// item is created from its type and equipped on the player's party member; the items it replaces are
// freed; the weapon and shield meshes are paged in for the player's slots; the host forwards the packet.
void mp_handle_equip_item(ubyte *data, void *from)
{
    int id = ((int *)data)[0];
    int type = ((int *)data)[2];
    int slot = ((int *)data)[1];
    item_info *old1, *old2;
    int slot1, slot2;

    mp_player *p = mp_player_find_by_id(id);
    if (!p || !p->object)
        return;
    Character *c = living_from_handle(p->object->handle);
    if (!c || c->type != 7)
        return;
    character_equip_item(&old1, &old2, c, slot, item_create(type));
    if (old1)
        item_free(old1);
    if (old2)
        item_free(old2);
    item_paging_find_player_slot(c, &slot1, &slot2);
    if (c->equipment[5])
        item_page_in(c->equipment[5]->type - item_types, slot1, 0);
    if (c->equipment[6])
        item_page_in(c->equipment[6]->type - item_types, slot2, 0);
    if (MP_IS_HOST())
        mp_forward_cur_packet(from);
}

// 0x487a70  Confidence: exact. INV_UQP (player id, slot).
void mp_send_unequip_item(Character *c, int slot)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    if (c->type != 7)
        return;
    mp_player *p = mp_player_find_by_name(c->name);
    if (!p)
        return;
    MP_BUILD_HEADER(MP_INV_UQP);
    MP_ADD_DATA(p->id);
    MP_ADD_DATA(slot);
    MP_END_PACKET();
    if (MP_IS_HOST())
        mp_broadcast_packet(data, packet_size, 1);
    else
        multi_send_packet(data, packet_size, NULL, 0);
}

// 0x487b10  Confidence: exact (VC6 byte match; decomp76). INV_UQP (player id, slot): the slot of the player's
// party member is emptied (the model updated) and the removed item freed; the host forwards the packet.
void mp_handle_unequip_item(ubyte *data, void *from)
{
    int id = ((int *)data)[0];
    int slot = ((int *)data)[1];
    item_info *old;

    mp_player *p = mp_player_find_by_id(id);
    if (!p || !p->object)
        return;
    Character *c = living_from_handle(p->object->handle);
    if (!c || c->type != 7)
        return;
    old = character_unequip_slot(c, slot, true);
    if (old)
        item_free(old);
    if (MP_IS_HOST())
        mp_forward_cur_packet(from);
}
