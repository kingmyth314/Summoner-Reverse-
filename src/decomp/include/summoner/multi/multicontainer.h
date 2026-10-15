// summoner/multi/multicontainer.h - multicontainer.cpp (0x488c00-0x48972a: loot drops, containers and lock
// picking in a network game, packets 0x23 ITM_DRP .. 0x27 CNT_PCK; strings.tbl ids 223-224). File name from
// the module column of sum.symbols, directory ours. Included by summoner/multi/multi.h. Not original source.
#ifndef SUMMONER_MULTI_MULTICONTAINER_H
#define SUMMONER_MULTI_MULTICONTAINER_H

#include "vsdk/vsdk.h"

struct mp_player;
class item_obj;
class vector3;

// ITM_GET (0x24, player id, item handle): the player picked the item up. Host only (broadcast).
void mp_broadcast_container_gold(int player_id, int handle);   // 0x488c00
void mp_handle_item_get(ubyte *data, void *from);       // 0x488c50
// (player id, container handle): marks the container used and broadcasts CNT_OPN.
void mp_send_container_open(int player_id, int handle); // 0x488ce0
void mp_handle_container_open(ubyte *data, void *from); // 0x488d50
// (container handle, items taken, items shown, item objects, selected flags of each).
void mp_send_container_close(int handle, int num_taken, int num_items,
                             item_obj **items, bool *selected);   // 0x488da0
void mp_handle_container_close(ubyte *data, void *from);   // 0x488eb0
// Host (monster_drop_loot): broadcasts ITM_DRP for the dropped items; handle is the gold item or the
// loot container the host made, arg is passed on to container_obj_create.
void container_spawn_loot(int count, item_obj **items, int handle, float lock, const vector3 *pos);   // 0x488fc0 (decomp37: the lock, a float)
void mp_handle_item_drop(ubyte *data, void *from);      // 0x489120
// (player id, item handle): mp_handle_srv_req. sum.symbols' "(handle, player)" has the order reversed.
void container_take_gold_server(int player_id, int handle);   // 0x489430
void container_request_open(int player_id, int handle); // 0x489480 (mp_handle_srv_req)
void container_lockpick(int player_id, int handle);     // 0x4894b0 (mp_handle_srv_req)
// result: 0 picked (broadcast), 1 failed, 2 already unlocked (both to the player only).
void mp_send_lockpick_result(mp_player *p, int handle, int result);   // 0x4895e0
void mp_handle_container_lockpick(ubyte *data, void *from);   // 0x489660

#endif
