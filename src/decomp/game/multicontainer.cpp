// multicontainer.cpp - Sum.exe v1.40, code 0x488c00-0x48972a (loot drops, containers and lock picking in a
// network game)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Unit: sum.symbols module column (strings.tbl ids 223-224). It follows mp_handle_swi_hide (0x488ba0) and
// ends before sinit_containers (0x489730).
// Data: none of its own: "Gold" (0x58d4a8), "CONTAINER POS ..." (0x599874), "Pick success!\n" (0x59182c)
// are pooled copies in earlier units; 0.2f (0x57ef10), 0.05f (0x58021c) and 0.0f (0x57eb84) are shared
// float constants. No .bss.
//
// The host owns the containers: a client's open / pick requests reach it as service requests
// (mp_handle_srv_req: container_request_open, container_lockpick, container_take_gold_server) and it
// broadcasts the results. Handles in CNT_OPN, ITM_GET and CNT_PCK are the host's handles (= net handles).

#include <stdio.h>
#include <string.h>
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/localize/localize.h"
#include "engine/objects/items.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"

#define CONTAINER_SCREEN 4            // interface screen of an open container (its argument: container**)
#define MP_MAX_DROP_ITEMS 16

// 0x488c00  Confidence: exact. ITM_GET (player id, item handle), host only.
void mp_broadcast_container_gold(int player_id, int handle)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    MP_BUILD_HEADER(MP_ITM_GET);
    MP_ADD_DATA(player_id);
    MP_ADD_DATA(handle);
    MP_END_PACKET();
    mp_broadcast_packet(data, packet_size, 1);
}

// 0x488c50  Confidence: exact. The taker picks the item up ("Picked up %d Gold" for gold); everybody else
// frees its item record and deletes the object (vtable slot 0). Not forwarded.
void mp_handle_item_get(ubyte *data, void *from)
{
    char buf[32];
    int player_id = ((int *)data)[0];
    item_obj *it = (item_obj *)mp_obj_from_net_handle(((int *)data)[1]);

    if (!it)
        return;

    if (player_id == mp_local_player->id) {
        if (it->is_gold()) {
            sprintf(buf, XSTR("Picked up %d Gold", 223), it->count);
            hud_show_message(buf);
        }
        item_obj_pickup(it, 1);
    } else {
        item_free(it->item);
        it->item = NULL;
        it->deinit();
    }
}

// 0x488ce0  Confidence: exact. CNT_OPN (container handle, user id), broadcast.
void mp_send_container_open(int player_id, int handle)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    container *c = (container *)obj_from_handle(handle);
    if (c && c->type == OBJ_TYPE_CONTAINER) {
        c->user = player_id;
        MP_BUILD_HEADER(MP_CNT_OPN);
        MP_ADD_DATA(handle);
        MP_ADD_DATA(player_id);
        MP_END_PACKET();
        mp_broadcast_packet(data, packet_size, 1);
    }
}

// 0x488d50  Confidence: likely (c is a local whose address is passed; the code keeps it in the dead
// argument slot). The user opens the container screen.
void mp_handle_container_open(ubyte *data, void *from)
{
    container *c = (container *)mp_obj_from_net_handle(((int *)data)[0]);
    int user = ((int *)data)[1];

    if (c && c->type == OBJ_TYPE_CONTAINER) {
        c->user = user;
        if (user == mp_local_player->id)
            interface_set_screen(CONTAINER_SCREEN, &c);
    }
}

// 0x488da0  Confidence: exact. CNT_CLS (our id, container net handle, items taken, the net handle of each
// selected item); a client sends it to the server (unreliably), the host broadcasts it. Frees the container.
void mp_send_container_close(int handle, int num_taken, int num_items, item_obj **items, bool *selected)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    int i;

    int net = mp_handle_to_net(handle);
    MP_BUILD_HEADER(MP_CNT_CLS);
    MP_ADD_DATA(mp_local_player->id);
    MP_ADD_DATA(net);
    MP_ADD_DATA(num_taken);
    for (i = 0; i < num_items; i++) {
        if (selected[i]) {
            int item_net = mp_handle_to_net(items[i]->handle);
            MP_ADD_DATA(item_net);
        }
    }
    MP_END_PACKET();
    if (MP_IS_CLIENT())
        multi_send_packet(data, packet_size, NULL, 0);
    else
        mp_broadcast_packet(data, packet_size, 1);

    container *c = (container *)obj_from_handle(handle);
    if (c)
        c->user = -1;
}

// 0x488eb0  Confidence: likely (the copy of the remaining items into a local array that nothing reads is
// kept). Game bug: the taken items are looked up from the container's net handle (data + 4) every time,
// not from the handles after the count (data + 12 ...), so remove_item gets the container itself and the
// other machines never remove the taken items (an emptied container is not deleted either).
void mp_handle_container_close(ubyte *data, void *from)
{
    item_obj *taken[MP_MAX_DROP_ITEMS];
    item_obj *items[MAX_CONTAINER_ITEMS];
    int player_id = ((int *)data)[0];
    int net = ((int *)data)[1];
    int num_taken = ((int *)data)[2];
    int i;

    if (player_id == mp_local_player->id)
        return;
    int handle = mp_local_handle_from_net(net);
    if (handle == -1)
        return;

    container *c = (container *)obj_from_handle(handle);
    for (i = 0; i < num_taken; i++)
        taken[i] = (item_obj *)obj_from_handle(mp_local_handle_from_net(net));
    for (i = 0; i < c->num_items; i++)
        items[i] = c->get_item(i);
    for (i = 0; i < num_taken; i++)
        if (taken[i])
            c->remove_item(taken[i]);

    c->user = -1;
    if (c->num_items <= 0)
        obj_mark_for_delete(c);
    if (MP_IS_HOST())
        mp_forward_cur_packet(from);
}

// 0x488fc0  Confidence: likely (MP_ADD_DATA of the argument: the code also copies it to an unused local).
// ITM_DRP (count, container_obj_create argument, host handle of the drop, quantized position, then per
// item its type and, for gold, the amount, then the mesh type of the container when the handle is set).
// The type is looked up by name again. For a single gold drop the handle is the gold item, whose +0xe0
// (container::mesh_item_type) is past the end of an item_obj; the receiver does not read it then.
void container_spawn_loot(int count, item_obj **items, int handle, float lock, const vector3 *pos)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    short x, y, z;
    int i;

    mp_quantize_position(&x, &y, &z, *pos);
    MP_BUILD_HEADER(MP_ITM_DRP);
    MP_ADD_DATA(count);
    MP_ADD_DATA(lock);
    MP_ADD_DATA(handle);
    MP_ADD_DATA(x);
    MP_ADD_DATA(y);
    MP_ADD_DATA(z);

    int gold = item_type_find("Gold");
    for (i = 0; i < count; i++) {
        int type = item_type_find(items[i]->item->type->name);
        MP_ADD_DATA(type);
        if (type == gold)
            MP_ADD_DATA(items[i]->count);
    }
    if (handle != -1) {
        container *c = (container *)obj_from_handle(handle);
        int mesh = c ? c->mesh_item_type : -1;
        MP_ADD_DATA(mesh);
    }
    MP_END_PACKET();
    mp_broadcast_packet(data, packet_size, 1);
}

// 0x489120  Confidence: unsure (the three vectors are plain stores, i.e. inline vector3 constructors, and
// the matrix goes through a temporary copied member by member; the spawned items are stored through a
// running pointer, so the skipped invalid types leave the later slots unset). A single gold drop becomes
// the gold item itself, anything else a container at the first item (0.2 higher) holding the items.
// Sound 13 when one of the items is a weapon (kind 0), else 12.
void mp_handle_item_drop(ubyte *data, void *from)
{
    item_obj *items[MP_MAX_DROP_ITEMS];
    vector3 rvec(1.0f, 0.0f, 0.0f);
    vector3 uvec(0.0f, 1.0f, 0.0f);
    vector3 fvec(0.0f, 0.0f, 1.0f);
    matrix orient = matrix(rvec, uvec, fvec);
    int gold = item_type_find("Gold");
    int count = ((int *)data)[0];
    int arg = ((int *)data)[1];
    int net = ((int *)data)[2];
    vector3 pos = mp_dequantize_position(*(short *)(data + 0xc), *(short *)(data + 0xe),
                                         *(short *)(data + 0x10));
    int offset = 0x12;
    item_obj **next = items;
    int i;

    for (i = count; i > 0; i--) {
        int type = *(int *)(data + offset);
        int n;
        offset += 4;
        if (type == gold) {
            n = *(int *)(data + offset);
            offset += 4;
        } else {
            n = 1;
        }
        if (type >= 0 && type < MAX_ITEM_TYPES)
            *next++ = item_obj_spawn(item_create(type), &pos, &orient, n, -1, -1);
    }

    if (count <= 1 && items[0]->is_gold()) {
        if (count == 1)
            mp_map_net_handle(net, items[0]->handle);
    } else {
        container *c = container_obj_create(&pos, arg);
        if (c) {
            for (i = 0; i < count; i++)
                c->add_item(items[i]);
            item_obj *first = c->get_item(0);
            if (first) {
                c->pos = first->pos;
                c->pos.y += 0.2f;
            }
            mp_map_net_handle(net, c->handle);
            printf("CONTAINER POS : (%f %f %f) (%f %f %f) (%f %f %f)\n",
                   c->pos.x, c->pos.y, c->pos.z,
                   c->get_item(0)->pos.x, c->get_item(0)->pos.y, c->get_item(0)->pos.z,
                   cur_character->pos.x, cur_character->pos.y, cur_character->pos.z);
            int mesh = *(int *)(data + offset);
            if (mesh != -1)
                c->mesh_item_type = mesh;
        }
    }

    for (i = 0; i < count; i++) {
        if (items[i]->item->type->kind == 0) {
            sound_play_3d_at(13, &pos);
            break;
        }
    }
    if (i >= count)
        sound_play_3d_at(12, &pos);
}

// 0x489430  Confidence: exact. Host side of a gold pickup: tells everybody, then deletes its own object.
void container_take_gold_server(int player_id, int handle)
{
    item_obj *it = (item_obj *)obj_from_handle(handle);

    if (it) {
        mp_broadcast_container_gold(player_id, handle);
        item_free(it->item);
        it->item = NULL;
        it->deinit();
    }
}

// 0x489480  Confidence: exact. No type check here (mp_send_container_open makes it).
void container_request_open(int player_id, int handle)
{
    container *c = (container *)obj_from_handle(handle);

    if (c && c->user == -1)
        mp_send_container_open(player_id, handle);
}

// 0x4894b0  Confidence: exact. No random roll: the pick succeeds when the Pick Lock skill + 0.05 reaches
// the lock value. Sound 6 / feedback 8 on success, sound 5 / feedback 9 on failure.
void container_lockpick(int player_id, int handle)
{
    container *c = (container *)obj_from_handle(handle);
    if (!c || c->type != OBJ_TYPE_CONTAINER)
        return;
    mp_player *p = mp_player_find_by_id(player_id);
    if (!p || !p->object)
        return;
    Character *ch = living_from_handle(p->object->handle);
    if (!ch)
        return;

    if (c->lock == 0.0f) {
        mp_send_lockpick_result(p, handle, 2);
        return;
    }
    if (ch->get_skill(SKILL_PICK_LOCK) + 0.05f >= c->lock) {
        printf("Pick success!\n");
        c->lock = 0.0f;
        game_play_sound(6, 1.0f, 0.0f, 0, 0);
        feedback_play(ch->info->species, 8, &ch->pos);
        mp_send_lockpick_result(p, handle, 0);
    } else {
        game_play_sound(5, 1.0f, 0.0f, 0, 0);
        feedback_play(ch->info->species, 9, &ch->pos);
        mp_send_lockpick_result(p, handle, 1);
    }
}

// 0x4895e0  Confidence: exact. CNT_PCK (player id, container handle, result): a success goes to
// everybody, the rest only to the picking player (unreliably).
void mp_send_lockpick_result(mp_player *p, int handle, int result)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    MP_BUILD_HEADER(MP_CNT_PCK);
    MP_ADD_DATA(p->id);
    MP_ADD_DATA(handle);
    MP_ADD_DATA(result);
    MP_END_PACKET();
    if (result == 0)
        mp_broadcast_packet(data, packet_size, 1);
    else
        multi_send_packet(data, packet_size, p, 0);
}

// 0x489660  Confidence: likely (if-chain over the result). The container is looked up with obj_from_handle,
// not mp_obj_from_net_handle, although the handle is the host's (see NOTES). A failure plays the sound
// without the feedback line the host played.
void mp_handle_container_lockpick(ubyte *data, void *from)
{
    int player_id = ((int *)data)[0];
    int handle = ((int *)data)[1];
    int result = ((int *)data)[2];

    mp_player *p = mp_player_find_by_id(player_id);
    if (!p || !p->object)
        return;
    Character *ch = living_from_handle(p->object->handle);
    if (!ch)
        return;
    container *c = (container *)obj_from_handle(handle);
    if (!c || c->type != OBJ_TYPE_CONTAINER)
        return;

    if (result == 0) {
        printf("Pick success!\n");
        c->lock = 0.0f;
        game_play_sound(6, 1.0f, 0.0f, 0, 0);
        feedback_play(ch->info->species, 8, &ch->pos);
    } else if (result == 1) {
        game_play_sound(5, 1.0f, 0.0f, 0, 0);
    } else if (result == 2) {
        hud_show_message(XSTR("This container already unlocked.", 224));
    }
}
