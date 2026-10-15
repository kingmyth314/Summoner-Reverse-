// summoner/multi/multitrade.h - multitrade.cpp (0x470e30-0x471def: the player-to-player item trade, packets
// 0x2a IT_RQST .. 0x32 IT_STAT; strings.tbl ids 247-249). File name from the module column of sum.symbols,
// directory ours. Included by summoner/multi/multi.h. Not original source.
#ifndef SUMMONER_MULTI_MULTITRADE_H
#define SUMMONER_MULTI_MULTITRADE_H

#include "vsdk/vsdk.h"
#include "vsdk/os/timestamp.h"
#include "summoner/player/player.h"

struct ItemType;
struct item_info;

// mp_trade_state: 0 none, 1 request sent (waiting for IT_ACPT), 2 trading, 3 we accepted (IT_OKAY sent),
// 4 the partner accepted. Names ours.
#define MP_TRADE_NONE      0
#define MP_TRADE_REQUESTED 1
#define MP_TRADE_OPEN      2
#define MP_TRADE_ACCEPTED  3
#define MP_TRADE_PARTNER_ACCEPTED 4

// One offered item (0xc bytes; mp_trade_remote_items 0x22e4630, mp_trade_local_items 0x22e46f8, 16 each).
// The local list never stores charges (mp_send_it_addi sends them in the packet only).
struct mp_trade_item {
    ItemType *type;                   // +0x0
    int count;                        // +0x4
    int charges;                      // +0x8
};
LAYOUT_CHECK(mp_trade_item, sizeof(mp_trade_item) == 0xc);
// The trade partner is the partner's Player record (+0xc handle, +0x14 mp_owner): mp_handle_it_rqst stores
// player_from_handle's result. new_itemtrade.cpp's mp_trade_peer / ->player spelling (D401).
typedef Player mp_trade_peer;

extern mp_trade_item mp_trade_remote_items[16];         // 0x22e4630
extern int mp_trade_state;                              // 0x22e46f0
extern mp_trade_item mp_trade_local_items[16];          // 0x22e46f8
extern int mp_trade_local_count;                        // 0x22e47b8
extern Player *mp_trade_partner;                        // 0x22e47bc
extern RealtimeTimer mp_trade_request_timer;            // 0x22e47c0 5 s: IT_QURY keep-alive
extern RealtimeTimer mp_trade_timeout_timer;            // 0x22e47c4 15 s: cancel when nothing arrives
extern int mp_trade_remote_count;                       // 0x22e47c8

void mp_trade_reset();                                  // 0x470e70 (mp_level_start)
void mp_trade_set_state(int state, Player *partner);    // 0x470e90
void mp_trade_frame();                                  // 0x470ed0 (mp_game_frame)
void mp_send_it_rqst(Player *partner);                  // 0x470f80 (world_mouse_frame)
void mp_handle_it_rqst(ubyte *data, void *from);        // 0x471040
void mp_send_it_deny(Player *partner, int reason);      // 0x471130 0 unable, 1 busy
void mp_handle_it_deny(ubyte *data, void *from);        // 0x4711c0
void mp_send_it_acpt(Player *partner);                  // 0x471250
void mp_handle_it_acpt(ubyte *data, void *from);        // 0x4712f0
void mp_send_it_addi(item_info *item, int count);       // 0x471370
void mp_handle_it_addi(ubyte *data, void *from);        // 0x4714a0
void mp_send_it_rmvi(ItemType *type);                   // 0x4715a0
void mp_handle_it_rmvi(ubyte *data, void *from);        // 0x4716f0
void mp_send_it_okay();                                 // 0x471820
void mp_trade_complete();                               // 0x4718d0
void mp_handle_it_okay(ubyte *data, void *from);        // 0x4719e0
void mp_trade_cancel();                                 // 0x471a70
void mp_handle_it_cncl(ubyte *data, void *from);        // 0x471b30
void mp_send_it_qury();                                 // 0x471bb0
void mp_handle_it_qury(ubyte *data, void *from);        // 0x471c50
void mp_send_it_stat();                                 // 0x471cd0
void mp_handle_it_stat(ubyte *data, void *from);        // 0x471d70

#endif
