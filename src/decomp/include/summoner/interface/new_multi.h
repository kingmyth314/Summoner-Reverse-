// summoner/interface/new_multi.h - new_multi.cpp (0x41dde0-0x41e3cf: the multiplayer elevators of IonaExt and
// eleh, strings.tbl ids 821-827, and ui_screen_stop_player). File name from the module column of
// sum.symbols, directory as the other new_*.cpp. ui_screen_stop_player (0x41e380) stays declared in
// summoner/interface/interface.h. Not original source (decomp47).
#ifndef SUMMONER_INTERFACE_NEW_MULTI_H
#define SUMMONER_INTERFACE_NEW_MULTI_H

#include "vsdk/vsdk.h"

// Levels with an elevator (level_script_table index; names ours).
#define ELEVATOR_LEVEL_IONAEXT 15     // "IonaExt": $npc117 goes up, $npc118 goes down
#define ELEVATOR_LEVEL_ELEH    29     // "eleh": $npc106, $npclg2, $npclg3 offer the three floors

void elevator_popup_frame();          // 0x41dde0 (interface_frame, multiplayer only)
void elevator_offer_ride();           // 0x41de10
void elevator_go_up_cb(int button);   // 0x41dec0 message box callback: 0 Yes -> start 4, 1 No -> start 3
void elevator_move_party(int player_id);   // 0x41df70 cur_summon to party start (id + 1) % 4
void elevator_go_down_cb(int button); // 0x41dfb0 0 Yes -> start 3, 1 No -> start 4
void elevator_choose_floor();         // 0x41e060
void elevator_floor_cb_a(int button); // 0x41e140 Upper 5, Lower 4, Cancel 3
void elevator_floor_cb_b(int button); // 0x41e200 Upper 5, Ground 3, Cancel 4
void elevator_floor_cb_c(int button); // 0x41e2c0 Ground 3, Lower 4, Cancel 5

#endif
