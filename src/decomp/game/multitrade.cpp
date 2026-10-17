// multitrade.cpp - Sum.exe v1.40, code 0x470e30-0x471def (the player-to-player item trade of a network game)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Unit: sum.symbols module column (strings.tbl ids 247-249 in mp_handle_it_deny). It follows the game list
// code (mp_join_clear_result 0x470e20) and ends before mp_remove_player (0x471df0, multiendgame).
// Data: no .data of its own ("Gold" 0x58d4a8 is a pooled copy); .bss 0x22e4630-0x22e47cb.
//
// The protocol: IT_RQST asks the target (state 1, 15 s timeout); the target answers IT_ACPT (both open
// screen 19, state 2) or IT_DENY (0 unable: dead / hidden, 1 busy). Offers go out as IT_ADDI / IT_RMVI and
// reset an accepted trade to state 2. IT_OKAY accepts (3 we did, 4 the partner did; the second accept
// completes on both sides). IT_QURY every 5 s while open, answered by IT_STAT, keeps the 15 s timeout alive.
// Every handler first checks that the packet is for cur_character (its net handle); the host forwards the
// others. All senders send to the server, or broadcast when they are the host.

#include <string.h>
#include "vsdk/os/timestamp.h"
#include "vsdk/localize/localize.h"
#include "engine/objects/items.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"

#define MP_TRADE_TIMEOUT      15000
#define MP_TRADE_QUERY_PERIOD 5000
#define MP_TRADE_SCREEN       19      // interface screen 0x13 (new_itemtrade.cpp)

// Character +0x320 flags that make a trade impossible (CF_SUMMONLIKE | CF_HIDDEN | CF_DEAD).
#define MP_TRADE_UNAVAILABLE_FLAGS 0x8a

// ---- .bss (address order; see the timer note) -------------------------------------------------------
mp_trade_item mp_trade_remote_items[16];                      // 0x22e4630
int mp_trade_state;                                           // 0x22e46f0
// 0x22e46f4: 4 bytes nothing references.
mp_trade_item mp_trade_local_items[16];                       // 0x22e46f8
int mp_trade_local_count;                                     // 0x22e47b8
Player *mp_trade_partner;                                     // 0x22e47bc
// The two timers' dynamic initialiser pairs, static_initializers entries 216 (0x22e47c4) and 217
// (0x22e47c0). Initialisers run in definition order, so the timeout timer is defined first although its
// address is the higher one (unsure for the definition order against the addresses; one marker per line,
// decomp76):
// 0x470e30 init_realtimetimer_22e47c4_thunk (jmp 0x470e40). Confidence: exact.
// 0x470e40 init_realtimetimer_22e47c4 (mov ecx, 0x22e47c4 / jmp RealtimeTimer__RealtimeTimer). Confidence: exact.
// 0x470e50 init_realtimetimer_22e47c0_thunk (jmp 0x470e60). Confidence: exact.
// 0x470e60 init_realtimetimer_22e47c0 (mov ecx, 0x22e47c0 / jmp RealtimeTimer__RealtimeTimer). Confidence: exact.
RealtimeTimer mp_trade_timeout_timer;                         // 0x22e47c4
RealtimeTimer mp_trade_request_timer;                         // 0x22e47c0
int mp_trade_remote_count;                                    // 0x22e47c8

// 0x470e70  Confidence: exact (mp_level_start).
void mp_trade_reset()
{
    mp_trade_set_state(MP_TRADE_NONE, NULL);
    mp_trade_timeout_timer.invalidate();
    mp_trade_request_timer.invalidate();
}

// 0x470e90  Confidence: exact (VC6 byte match, match11; was likely) (the case bodies are laid out 2, 1, 0 after the decrement chain). State 2
// keeps the partner when none is given.
void mp_trade_set_state(int state, Player *partner)
{
    mp_trade_state = state;
    switch (state) {
    case MP_TRADE_NONE:
        mp_trade_partner = NULL;
        mp_trade_local_count = 0;
        mp_trade_remote_count = 0;
        break;
    case MP_TRADE_REQUESTED:
        mp_trade_partner = partner;
        break;
    case MP_TRADE_OPEN:
        if (partner)
            mp_trade_partner = partner;
        break;
    }
}

// 0x470ed0  Confidence: exact (VC6 byte match, match11; was likely) (if / else form of the 1 and 2..4 tests; the three conditions are bools,
// busy starts false and is only computed while trading). Cancels on the timeout, when cur_character is
// dead or hidden, or (trading) when it is not in AI mode 1; else sends the keep-alive query.
void mp_trade_frame()
{
    bool timed_out;
    bool unavailable;
    bool busy = false;

    if (mp_trade_state != MP_TRADE_REQUESTED) {
        if (mp_trade_state > MP_TRADE_REQUESTED && mp_trade_state <= MP_TRADE_PARTNER_ACCEPTED) {
            timed_out = mp_trade_timeout_timer.elapsed();
            unavailable = (cur_character->flags & MP_TRADE_UNAVAILABLE_FLAGS) != 0;
            busy = !cur_character->ai.is_mode(1, -1);
        } else
            return;
    } else {
        timed_out = mp_trade_timeout_timer.elapsed();
        unavailable = (cur_character->flags & MP_TRADE_UNAVAILABLE_FLAGS) != 0;
    }

    if (timed_out || unavailable || busy)
        mp_trade_cancel();
    else if (mp_trade_request_timer.elapsed())
        mp_send_it_qury();
}

// 0x470f80  Confidence: exact. (target net handle, our net handle)
void mp_send_it_rqst(Player *partner)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    if (mp_trade_state)
        return;

    int net = mp_handle_to_net(partner->handle);
    MP_BUILD_HEADER(MP_IT_RQST);
    MP_ADD_DATA(net);
    MP_ADD_DATA(cur_character->net_handle);
    MP_END_PACKET();
    if (MP_IS_HOST())
        mp_broadcast_packet(data, packet_size, 1);
    else
        multi_send_packet(data, packet_size, NULL, 1);

    mp_trade_set_state(MP_TRADE_REQUESTED, partner);
    mp_trade_timeout_timer.SetTimeout(MP_TRADE_TIMEOUT);
    mp_trade_request_timer.invalidate();
}

// 0x471040  Confidence: exact (VC6 byte match, match29; was likely) (the packet fields read with memcpy, D8302 / D8800; interface_set_screen's second
// argument is a literal 0). Accepts only while not trading, not in AI mode 3
// and with no screen open.
void mp_handle_it_rqst(ubyte *data, void *from)
{
    int target, sender;
    memcpy(&target, data, 4);
    memcpy(&sender, data + 4, 4);

    if (cur_character->net_handle != target) {
        if (MP_IS_HOST())
            mp_forward_cur_packet(from);
        return;
    }

    Player *p = player_from_handle(mp_local_handle_from_net(sender));
    int screen = interface_get_screen();
    if (cur_character->flags & MP_TRADE_UNAVAILABLE_FLAGS) {
        mp_send_it_deny(p, 0);
        return;
    }
    if (mp_trade_state == MP_TRADE_NONE && !cur_character->ai.is_mode(3, -1) && screen == 0) {
        mp_send_it_acpt(p);
        mp_trade_timeout_timer.SetTimeout(MP_TRADE_TIMEOUT);
        mp_trade_request_timer.SetTimeout(MP_TRADE_QUERY_PERIOD);
        interface_set_screen(MP_TRADE_SCREEN, 0);
        return;
    }
    mp_send_it_deny(p, 1);
}

// 0x471130  Confidence: exact. (requester net handle, reason)
void mp_send_it_deny(Player *partner, int reason)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    int net = mp_handle_to_net(partner->handle);
    MP_BUILD_HEADER(MP_IT_DENY);
    MP_ADD_DATA(net);
    MP_ADD_DATA(reason);
    MP_END_PACKET();
    if (MP_IS_HOST())
        mp_broadcast_packet(data, packet_size, 1);
    else
        multi_send_packet(data, packet_size, NULL, 1);
}

// 0x4711c0  Confidence: exact (VC6 byte match, match29; was likely) (switch: the default body directly follows the two tests).
void mp_handle_it_deny(ubyte *data, void *from)
{
    int target, reason;
    memcpy(&target, data, 4);
    memcpy(&reason, data + 4, 4);

    if (cur_character->net_handle != target) {
        if (MP_IS_HOST())
            mp_forward_cur_packet(from);
        return;
    }

    switch (reason) {
    case 0:
        hud_show_message(XSTR("Player is unable to trade", 247));
        break;
    case 1:
        hud_show_message(XSTR("Player is busy", 248));
        break;
    default:
        hud_show_message(XSTR("Player can't trade right now", 249));
        break;
    }
    mp_trade_set_state(MP_TRADE_NONE, NULL);
    mp_trade_timeout_timer.invalidate();
    mp_trade_request_timer.invalidate();
}

// 0x471250  Confidence: exact. (requester net handle)
void mp_send_it_acpt(Player *partner)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    int net = mp_handle_to_net(partner->handle);
    MP_BUILD_HEADER(MP_IT_ACPT);
    MP_ADD_DATA(net);
    MP_END_PACKET();
    if (MP_IS_HOST())
        mp_broadcast_packet(data, packet_size, 1);
    else
        multi_send_packet(data, packet_size, NULL, 1);
    mp_trade_set_state(MP_TRADE_OPEN, partner);
}

// 0x4712f0  Confidence: exact. The partner was set by mp_send_it_rqst.
void mp_handle_it_acpt(ubyte *data, void *from)
{
    int target;
    memcpy(&target, data, 4);

    if (cur_character->net_handle != target) {
        if (MP_IS_HOST())
            mp_forward_cur_packet(from);
        return;
    }

    mp_trade_set_state(MP_TRADE_OPEN, NULL);
    mp_trade_timeout_timer.SetTimeout(MP_TRADE_TIMEOUT);
    mp_trade_request_timer.SetTimeout(MP_TRADE_QUERY_PERIOD);
    interface_set_screen(MP_TRADE_SCREEN, 0);
}

// 0x471370  Confidence: exact (VC6 byte match, match11; was likely) (the merge loop: break after adding, append when the loop ran out). (partner
// net handle, item type, count, charges). Sent unreliably by a client. The local list keeps type and
// count only.
void mp_send_it_addi(item_info *item, int count)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    int i;

    if (count <= 0 || mp_trade_local_count >= 16)
        return;

    int type = item->type - item_types;
    int net = mp_handle_to_net(mp_trade_partner->handle);
    MP_BUILD_HEADER(MP_IT_ADDI);
    MP_ADD_DATA(net);
    MP_ADD_DATA(type);
    MP_ADD_DATA(count);
    MP_ADD_DATA(item->charges);
    MP_END_PACKET();
    if (MP_IS_HOST())
        mp_broadcast_packet(data, packet_size, 1);
    else
        multi_send_packet(data, packet_size, NULL, 0);

    for (i = 0; i < mp_trade_local_count; i++) {
        if (mp_trade_local_items[i].type == item->type) {
            mp_trade_local_items[i].count += count;
            break;
        }
    }
    if (i >= mp_trade_local_count) {
        mp_trade_local_items[mp_trade_local_count].type = item->type;
        mp_trade_local_items[mp_trade_local_count].count = count;
        mp_trade_local_count++;
    }

    if (mp_trade_state == MP_TRADE_ACCEPTED || mp_trade_state == MP_TRADE_PARTNER_ACCEPTED)
        mp_trade_set_state(MP_TRADE_OPEN, NULL);
}

// 0x4714a0  Confidence: exact (VC6 byte match, match29; was likely) (as mp_send_it_addi). No bound check on the remote list (16 entries).
void mp_handle_it_addi(ubyte *data, void *from)
{
    int target, type, count, charges;
    memcpy(&target, data, 4);
    memcpy(&type, data + 4, 4);
    memcpy(&count, data + 8, 4);
    memcpy(&charges, data + 12, 4);
    int i;

    if (cur_character->net_handle != target) {
        if (MP_IS_HOST())
            mp_forward_cur_packet(from);
        return;
    }

    ItemType *t = &item_types[type];
    for (i = 0; i < mp_trade_remote_count; i++) {
        if (t == mp_trade_remote_items[i].type) {
            mp_trade_remote_items[i].count += count;
            break;
        }
    }
    if (i >= mp_trade_remote_count) {
        mp_trade_remote_items[mp_trade_remote_count].type = t;
        mp_trade_remote_items[mp_trade_remote_count].count = count;
        mp_trade_remote_items[mp_trade_remote_count].charges = charges;
        mp_trade_remote_count++;
    }

    if (mp_trade_state == MP_TRADE_ACCEPTED || mp_trade_state == MP_TRADE_PARTNER_ACCEPTED)
        mp_trade_set_state(MP_TRADE_OPEN, NULL);
    mp_trade_timeout_timer.SetTimeout(MP_TRADE_TIMEOUT);
    mp_trade_request_timer.SetTimeout(MP_TRADE_QUERY_PERIOD);
}

// 0x4715a0  Confidence: exact (VC6 byte match, match11; was likely) (the second i < count test after the search loop). Takes back one of the
// offered items, or all the offered gold. (partner net handle, item type)
void mp_send_it_rmvi(ItemType *type)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    int i;

    for (i = 0; i < mp_trade_local_count; i++)
        if (type == mp_trade_local_items[i].type)
            break;
    if (i >= mp_trade_local_count)
        return;

    int net = mp_handle_to_net(mp_trade_partner->handle);
    int index = type - item_types;
    if (index == item_type_find("Gold"))
        mp_trade_local_items[i].count = 0;
    else
        mp_trade_local_items[i].count--;
    if (mp_trade_local_items[i].count <= 0) {
        memmove(&mp_trade_local_items[i], &mp_trade_local_items[i + 1],
                (mp_trade_local_count - (i + 1)) * sizeof(mp_trade_item));
        mp_trade_local_count--;
    }

    MP_BUILD_HEADER(MP_IT_RMVI);
    MP_ADD_DATA(net);
    MP_ADD_DATA(index);
    MP_END_PACKET();
    if (MP_IS_HOST())
        mp_broadcast_packet(data, packet_size, 1);
    else
        multi_send_packet(data, packet_size, NULL, 0);

    if (mp_trade_state == MP_TRADE_ACCEPTED || mp_trade_state == MP_TRADE_PARTNER_ACCEPTED)
        mp_trade_set_state(MP_TRADE_OPEN, NULL);
}

// 0x4716f0  Confidence: exact (VC6 byte match, match29; was likely) (as mp_send_it_rmvi).
void mp_handle_it_rmvi(ubyte *data, void *from)
{
    int target, type;
    memcpy(&target, data, 4);
    memcpy(&type, data + 4, 4);
    int i;

    if (cur_character->net_handle != target) {
        if (MP_IS_HOST())
            mp_forward_cur_packet(from);
        return;
    }

    ItemType *t = &item_types[type];
    for (i = 0; i < mp_trade_remote_count; i++)
        if (t == mp_trade_remote_items[i].type)
            break;
    if (i >= mp_trade_remote_count)
        return;

    if (type == item_type_find("Gold"))
        mp_trade_remote_items[i].count = 0;
    else
        mp_trade_remote_items[i].count--;
    if (mp_trade_remote_items[i].count <= 0) {
        memmove(&mp_trade_remote_items[i], &mp_trade_remote_items[i + 1],
                (mp_trade_remote_count - (i + 1)) * sizeof(mp_trade_item));
        mp_trade_remote_count--;
    }

    if (mp_trade_state == MP_TRADE_ACCEPTED || mp_trade_state == MP_TRADE_PARTNER_ACCEPTED)
        mp_trade_set_state(MP_TRADE_OPEN, NULL);
    mp_trade_timeout_timer.SetTimeout(MP_TRADE_TIMEOUT);
    mp_trade_request_timer.SetTimeout(MP_TRADE_QUERY_PERIOD);
}

// 0x471820  Confidence: exact. Accepts the trade (itemtrade_accept); completes when the partner had.
void mp_send_it_okay()
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    if (mp_trade_state == MP_TRADE_OPEN || mp_trade_state == MP_TRADE_PARTNER_ACCEPTED) {
        int net = mp_handle_to_net(mp_trade_partner->handle);
        MP_BUILD_HEADER(MP_IT_OKAY);
        MP_ADD_DATA(net);
        MP_END_PACKET();
        if (MP_IS_HOST())
            mp_broadcast_packet(data, packet_size, 1);
        else
            multi_send_packet(data, packet_size, NULL, 0);

        if (mp_trade_state == MP_TRADE_PARTNER_ACCEPTED)
            mp_trade_complete();
        else
            mp_trade_set_state(MP_TRADE_ACCEPTED, NULL);
    }
}

// 0x4718d0  Confidence: exact (VC6 byte match, match11; was likely) (party_inventory[slot].count is updated in place and re-read for the test).
// Gives the offered items away and takes the partner's; the partner's items are new items with the
// charges it sent. The local slot is not checked for -1 (see NOTES).
void mp_trade_complete()
{
    int gold = item_type_find("Gold");
    int i;

    for (i = 0; i < mp_trade_local_count; i++) {
        int type = mp_trade_local_items[i].type - item_types;
        if (type == gold) {
            party_gold -= mp_trade_local_items[i].count;
        } else {
            int slot = party_inventory_find_type_index(type);
            party_inventory[slot].count -= mp_trade_local_items[i].count;
            if (party_inventory[slot].count <= 0)
                party_inventory_remove(slot, 1, 1);
        }
    }

    for (i = 0; i < mp_trade_remote_count; i++) {
        int type = mp_trade_remote_items[i].type - item_types;
        if (type == gold) {
            party_gold += mp_trade_remote_items[i].count;
        } else {
            item_info *item = item_create(type);
            item->charges = mp_trade_remote_items[i].charges;
            party_inventory_add_item(item, mp_trade_remote_items[i].count, 1);
        }
    }

    mp_trade_set_state(MP_TRADE_NONE, NULL);
    mp_trade_timeout_timer.invalidate();
    mp_trade_request_timer.invalidate();
    interface_set_screen(0, 0);
}

// 0x4719e0  Confidence: likely (the nested state test: state 3 tail-jumps to mp_trade_complete).
void mp_handle_it_okay(ubyte *data, void *from)
{
    int target;
    memcpy(&target, data, 4);

    if (cur_character->net_handle != target) {
        if (MP_IS_HOST())
            mp_forward_cur_packet(from);
        return;
    }

    if (mp_trade_state == MP_TRADE_OPEN || mp_trade_state == MP_TRADE_ACCEPTED) {
        if (mp_trade_state == MP_TRADE_ACCEPTED) {
            mp_trade_complete();
        } else {
            mp_trade_set_state(MP_TRADE_PARTNER_ACCEPTED, NULL);
            mp_trade_timeout_timer.SetTimeout(MP_TRADE_TIMEOUT);
            mp_trade_request_timer.SetTimeout(MP_TRADE_QUERY_PERIOD);
        }
    }
}

// 0x471a70  Confidence: exact (itemtrade_leave, the cancel button, mp_trade_frame).
void mp_trade_cancel()
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    if (mp_trade_state > MP_TRADE_NONE && mp_trade_state <= MP_TRADE_PARTNER_ACCEPTED) {
        int net = mp_handle_to_net(mp_trade_partner->handle);
        MP_BUILD_HEADER(MP_IT_CNCL);
        MP_ADD_DATA(net);
        MP_END_PACKET();
        if (MP_IS_HOST())
            mp_broadcast_packet(data, packet_size, 1);
        else
            multi_send_packet(data, packet_size, NULL, 0);

        mp_trade_set_state(MP_TRADE_NONE, NULL);
        mp_trade_timeout_timer.invalidate();
        mp_trade_request_timer.invalidate();
        interface_set_screen(0, 0);
    }
}

// 0x471b30  Confidence: exact. A cancel during the request (state 1) is ignored.
void mp_handle_it_cncl(ubyte *data, void *from)
{
    int target;
    memcpy(&target, data, 4);

    if (cur_character->net_handle != target) {
        if (MP_IS_HOST())
            mp_forward_cur_packet(from);
        return;
    }

    if (mp_trade_state >= MP_TRADE_OPEN && mp_trade_state <= MP_TRADE_PARTNER_ACCEPTED) {
        mp_trade_set_state(MP_TRADE_NONE, NULL);
        mp_trade_timeout_timer.invalidate();
        mp_trade_request_timer.invalidate();
        interface_set_screen(0, 0);
    }
}

// 0x471bb0  Confidence: exact. (partner net handle, our net handle)
void mp_send_it_qury()
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    if (!mp_trade_partner)
        return;

    int net = mp_handle_to_net(mp_trade_partner->handle);
    MP_BUILD_HEADER(MP_IT_QURY);
    MP_ADD_DATA(net);
    MP_ADD_DATA(cur_character->net_handle);
    MP_END_PACKET();
    if (MP_IS_HOST())
        mp_broadcast_packet(data, packet_size, 1);
    else
        multi_send_packet(data, packet_size, NULL, 0);
}

// 0x471c50  Confidence: exact. Only the partner's query is answered.
void mp_handle_it_qury(ubyte *data, void *from)
{
    int target, sender;
    memcpy(&target, data, 4);
    memcpy(&sender, data + 4, 4);

    if (cur_character->net_handle != target) {
        if (MP_IS_HOST())
            mp_forward_cur_packet(from);
        return;
    }

    if (player_from_handle(mp_local_handle_from_net(sender)) == mp_trade_partner) {
        mp_send_it_stat();
        mp_trade_timeout_timer.SetTimeout(MP_TRADE_TIMEOUT);
        mp_trade_request_timer.SetTimeout(MP_TRADE_QUERY_PERIOD);
    }
}

// 0x471cd0  Confidence: exact. (partner net handle, our net handle, our trade state)
void mp_send_it_stat()
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    if (!mp_trade_partner)
        return;

    int net = mp_handle_to_net(mp_trade_partner->handle);
    MP_BUILD_HEADER(MP_IT_STAT);
    MP_ADD_DATA(net);
    MP_ADD_DATA(cur_character->net_handle);
    MP_ADD_DATA(mp_trade_state);
    MP_END_PACKET();
    if (MP_IS_HOST())
        mp_broadcast_packet(data, packet_size, 1);
    else
        multi_send_packet(data, packet_size, NULL, 0);
}

// 0x471d70  Confidence: exact. The partner's state (data + 8) is not read.
void mp_handle_it_stat(ubyte *data, void *from)
{
    int target, sender;
    memcpy(&target, data, 4);
    memcpy(&sender, data + 4, 4);

    if (cur_character->net_handle != target) {
        if (MP_IS_HOST())
            mp_forward_cur_packet(from);
        return;
    }

    if (player_from_handle(mp_local_handle_from_net(sender)) == mp_trade_partner) {
        mp_trade_timeout_timer.SetTimeout(MP_TRADE_TIMEOUT);
        mp_trade_request_timer.SetTimeout(MP_TRADE_QUERY_PERIOD);
    }
}
