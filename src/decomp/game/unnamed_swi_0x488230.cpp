// unnamed_swi_0x488230.cpp - Sum.exe v1.40, code 0x488230-0x488bf9 (the "SWI" packets of a network game:
// a character's AI starts walking, attacks, travels to a point, finds a path, a boss move, sneaks, hides).
// THE FILE NAME IS A PLACEHOLDER: no path string, no strings.tbl block, no data besides two printf
// literals (0x59c1d0, 0x59c200, after multichat.cpp's .data). It sits between multichat.cpp (ends
// 0x488223) and multicontainer.cpp (0x488c00). Written from the Sum.exe disassembly (tools/asm.mjs) by
// decomp76 (D5809). Target: VC++ 6.0 (cl 12.00.8966), /O2. Confidence per function: exact (control flow,
// calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape is a guess).
//
// A client sends its own characters' moves to the host with the characters' net handles; the host
// broadcasts them with its local handles. The handlers map the handle the same way (the host: its own
// handle; a client: the net handle map), let the AI do the move and, on the host, forward the packet.

#include <stdio.h>
#include <string.h>
#include "vsdk/math/vector.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"
#include "summoner/multi/unnamed_netpackets_0x441820.h"
#include "summoner/multi/unnamed_netsession_0x440b40.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/ai/ai.h"
#include "engine/ai/boss_ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/characterinfo/characterinfo.h"

// Packet types (mp_packet_table entries 0x1b-0x21; names from the table, "SWI!DIR" written NDIR).
#define MP_SWI_PTH 0x1b
#define MP_SWI_ATK 0x1c
#define MP_SWI_2DIR 0x1d
#define MP_SWI_NDIR 0x1e
#define MP_SWI_BSS 0x1f
#define MP_SWI_SNK 0x20
#define MP_SWI_HID 0x21

#define MP_MACHIVAL_TELEPORT_MOVE 0x33   // the sub-mode of his teleport (machival_ai_special_move)

// 0x488230  Confidence: exact (VC6 byte match). SWI_PTH {handle, walk}: start traveling along the current path.
void mp_send_character_walk(Character *c, bool a)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    if (MP_IS_CLIENT()) {
        MP_BUILD_HEADER(MP_SWI_PTH);
        MP_ADD_DATA(c->net_handle);
        data[packet_size++] = (ubyte)a;
        MP_END_PACKET();
        multi_send_packet(data, packet_size, NULL, true);
    } else {
        MP_BUILD_HEADER(MP_SWI_PTH);
        MP_ADD_DATA(c->handle);
        data[packet_size++] = (ubyte)a;
        MP_END_PACKET();
        mp_broadcast_packet(data, packet_size, true);
    }
}

// 0x4882f0  Confidence: likely (VC6 keeps the walk flag in a register; the original passes it through the
// dead data argument slot). A packet about the local leader is a protocol error (printed).
void mp_handle_swi_path(ubyte *data, void *from)
{
    int h = *(int *)data;
    bool walk;
    Character *c;

    if (MP_IS_HOST())
        c = living_from_handle(h);
    else
        c = (Character *)mp_obj_from_net_handle(h);
    if (!c)
        return;
    if (c == cur_character) {
        printf("What the heehaw -- someone's walking for me\n");
        return;
    }
    walk = data[4] != 0;
    c->ai.start_travel(walk);
    if (MP_IS_HOST())
        mp_forward_cur_packet(from);
}

// 0x488390  Confidence: exact (VC6 byte match). SWI_ATK {handle, target handle} (the combat target, +0x55c).
void mp_send_character_attack(Character *c)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    if (MP_IS_CLIENT()) {
        int target = mp_handle_to_net(c->combat.target);
        MP_BUILD_HEADER(MP_SWI_ATK);
        MP_ADD_DATA(c->net_handle);
        MP_ADD_DATA(target);
        MP_END_PACKET();
        multi_send_packet(data, packet_size, NULL, true);
    } else {
        MP_BUILD_HEADER(MP_SWI_ATK);
        MP_ADD_DATA(c->handle);
        MP_ADD_DATA(c->combat.target);
        MP_END_PACKET();
        mp_broadcast_packet(data, packet_size, true);
    }
}

// 0x488450  Confidence: exact (VC6 byte match).
void mp_handle_swi_attack(ubyte *data, void *from)
{
    int h = ((int *)data)[0];
    int th = ((int *)data)[1];
    Character *c, *t;

    if (MP_IS_HOST()) {
        c = living_from_handle(h);
        t = living_from_handle(th);
    } else {
        c = (Character *)mp_obj_from_net_handle(h);
        t = (Character *)mp_obj_from_net_handle(th);
    }
    if (!c || !t)
        return;
    if (c == cur_character) {
        printf("What the heehaw -- someone's attacking for me\n");
        return;
    }
    c->ai.charge_target(t);
    if (MP_IS_HOST())
        mp_forward_cur_packet(from);
}

// 0x4884f0  Confidence: exact (VC6 byte match). SWI2DIR {handle, the quantized travel destination} of the leader, sent
// buffered.
void mp_send_swi_2dir()
{
    ushort x, y, z;
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    int handle;

    mp_quantize_position((short *)&x, (short *)&y, (short *)&z, cur_character->ai.travel_dest);
    MP_BUILD_HEADER(MP_SWI_2DIR);
    if (MP_IS_HOST())
        handle = cur_character->handle;
    else
        handle = mp_handle_to_net(cur_character->handle);
    MP_ADD_DATA(handle);
    MP_ADD_DATA(x);
    MP_ADD_DATA(y);
    MP_ADD_DATA(z);
    MP_END_PACKET();
    if (MP_IS_CLIENT())
        multi_send_packet_buffered(data, packet_size, NULL, false);
    else
        mp_broadcast_packet_buffered(data, packet_size, false);
}

// 0x4885f0  Confidence: exact (VC6 byte match). Only for a player's character: its player's SWI timer restarts and the
// position is kept, then the character travels to the sent destination.
void mp_handle_swi_2dir(ubyte *data, void *from)
{
    int h = *(int *)data;
    Character *c;
    Player *pl;

    if (MP_IS_HOST())
        c = living_from_handle(h);
    else
        c = (Character *)mp_obj_from_net_handle(h);
    if (!c)
        return;
    pl = player_from_handle(c->handle);
    if (!pl || !pl->mp_owner)
        return;
    pl->mp_owner->swi_timer.SetTimeout(0);
    *(vector3 *)pl->mp_owner->swi_pos = c->pos;
    pl->mp_owner->swi_num_samples = 0;
    ushort x = ((ushort *)(data + 4))[0];
    ushort y = ((ushort *)(data + 4))[1];
    ushort z = ((ushort *)(data + 4))[2];
    c->ai.travel_dest = mp_dequantize_position(x, y, z);
    c->ai.travel_to_current_dest();
    if (MP_IS_HOST())
        mp_forward_cur_packet_buffered(from);
}

// 0x488700  Confidence: exact (VC6 byte match). SWI!DIR {handle, position, layer} of the leader.
void mp_send_swi_ndir()
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    int handle;
    float f;

    MP_BUILD_HEADER(MP_SWI_NDIR);
    if (MP_IS_HOST())
        handle = cur_character->handle;
    else
        handle = mp_handle_to_net(cur_character->handle);
    MP_ADD_DATA(handle);
    f = cur_character->pos.x;
    MP_ADD_DATA(f);
    f = cur_character->pos.y;
    MP_ADD_DATA(f);
    f = cur_character->pos.z;
    MP_ADD_DATA(f);
    MP_ADD_DATA(cur_character->layer);
    MP_END_PACKET();
    if (MP_IS_CLIENT())
        multi_send_packet(data, packet_size, NULL, true);
    else
        mp_broadcast_packet(data, packet_size, true);
}

// 0x4887e0  Confidence: exact (VC6 byte match). A path from the character to the sent point; without one the character
// is put there and goes idle.
void mp_handle_swi_ndir(ubyte *data, void *from)
{
    int h = *(int *)data;
    Character *c;
    vector3 dest;
    int layer;
    ai_path *path;

    if (MP_IS_HOST())
        c = living_from_handle(h);
    else
        c = (Character *)mp_obj_from_net_handle(h);
    if (!c)
        return;
    dest.x = ((float *)data)[1];
    dest.y = ((float *)data)[2];
    dest.z = ((float *)data)[3];
    layer = ((int *)data)[4];
    path = ai_path_find(&c->pos, c->layer, &dest, layer, c->info->size, 0.0f);
    if (path) {
        c->ai.set_path(&c->pos, path);
        c->ai.start_travel(false);
    } else {
        c->pos = dest;
        c->ai.go_idle();
    }
    if (MP_IS_HOST())
        mp_forward_cur_packet(from);
}

// 0x4888d0  Confidence: exact (VC6 byte match). SWI_BSS {handle, mode, sub-mode} (the host's boss moves); Machival's
// teleport adds the facing position and the index of the teleport navpoint (0 when it is not one of the
// six).
void mp_send_swi_boss(Character *boss, int mode, int submode)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    ubyte i;

    MP_BUILD_HEADER(MP_SWI_BSS);
    MP_ADD_DATA(boss->handle);
    MP_ADD_DATA(mode);
    MP_ADD_DATA(submode);
    if (boss->info->species == SPECIES_MACHIVAL && submode == MP_MACHIVAL_TELEPORT_MOVE) {
        i = 0;
        while (machival_teleport_navpoint != machival_teleports[i]) {
            i++;
            if (i >= 6)
                break;
        }
        if (i == 6)
            i = 0;
        MP_ADD_FLOAT(machival_teleport_pos.x);
        MP_ADD_FLOAT(machival_teleport_pos.y);
        MP_ADD_FLOAT(machival_teleport_pos.z);
        data[packet_size++] = i;
    }
    MP_END_PACKET();
    mp_broadcast_packet(data, packet_size, true);
}

// 0x4889c0  Confidence: exact (VC6 byte match). Only Machival's teleport is handled (the other boss moves are sent but
// ignored).
void mp_handle_swi_boss(ubyte *data, void *from)
{
    int submode = ((int *)data)[2];
    Character *c = (Character *)mp_obj_from_net_handle(((int *)data)[0]);

    if (!c)
        return;
    if (c->info->species != SPECIES_MACHIVAL || submode != MP_MACHIVAL_TELEPORT_MOVE)
        return;
    machival_teleport_pos.x = ((float *)data)[3];
    machival_teleport_pos.y = ((float *)data)[4];
    machival_teleport_pos.z = ((float *)data)[5];
    c->ai.machival_teleport(data[0x18]);
}

// 0x488a20  Confidence: exact (VC6 byte match). SWI_SNK {net handle}: the net handle in both directions.
void mp_send_swi_sneak(Character *c)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    int handle = mp_handle_to_net(c->handle);

    MP_BUILD_HEADER(MP_SWI_SNK);
    MP_ADD_DATA(handle);
    MP_END_PACKET();
    if (MP_IS_CLIENT())
        multi_send_packet(data, packet_size, NULL, false);
    else
        mp_broadcast_packet(data, packet_size, true);
}

// 0x488ab0  Confidence: likely (VC6 loads the handle into ecx, the original into eax).
void mp_handle_swi_sneak(ubyte *data, void *from)
{
    Character *c = living_from_handle(mp_local_handle_from_net(*(int *)data));

    if (c) {
        c->ai.enter_sneaking();
        if (MP_IS_HOST())
            mp_forward_cur_packet_buffered(from);
    }
}

// 0x488b00  Confidence: exact (VC6 byte match). SWI_HID {net handle, hiding (AI mode 9 / 9)}.
void mp_send_swi_hide(Character *c)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    int handle = mp_handle_to_net(c->handle);
    bool hiding = c->ai.is_mode(9, 9);

    MP_BUILD_HEADER(MP_SWI_HID);
    MP_ADD_DATA(handle);
    MP_ADD_DATA(hiding);
    MP_END_PACKET();
    if (MP_IS_CLIENT())
        multi_send_packet(data, packet_size, NULL, false);
    else
        mp_broadcast_packet(data, packet_size, true);
}

// 0x488ba0  Confidence: exact (VC6 byte match).
void mp_handle_swi_hide(ubyte *data, void *from)
{
    int h = *(int *)data;
    ubyte enter = data[4];
    Character *c = living_from_handle(mp_local_handle_from_net(h));

    if (c) {
        c->ai.enter_hiding(enter);
        if (MP_IS_HOST())
            mp_forward_cur_packet_buffered(from);
    }
}
