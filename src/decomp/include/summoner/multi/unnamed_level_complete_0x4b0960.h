// summoner/multi/unnamed_level_complete_0x4b0960.h - the multiplayer level / difficulty completion and the
// combat exchange packet (code 0x4b0960-0x4b140f; game/unnamed_level_complete_0x4b0960.cpp, file name a
// placeholder; decomp68). Included by multi.h, which declared most of these before. Not original source.
#ifndef SUMMONER_MULTI_UNNAMED_LEVEL_COMPLETE_0X4B0960_H
#define SUMMONER_MULTI_UNNAMED_LEVEL_COMPLETE_0X4B0960_H

#include "vsdk/vsdk.h"

class Character;
struct move_info;

#define MP_COM_SEQ 0x22               // a living_combat exchange (mp_send_living_combat)
#define MP_L_CMPLT 0x3c               // a level completed (bit in mp_player::profile_flags)
#define MP_D_CMPLT 0x3d               // the game completed at a difficulty (bits 26..28)

#define MP_NUM_LEVELS     26          // level_table entries; bits 0..25 of profile_flags
#define MP_FINAL_LEVEL    25          // its completion also completes the difficulty
#define MP_DIFFICULTY_BIT 26

// .bss 0x247e830-0x247e83b (names ours).
extern bool mp_level_done;                              // 0x247e830 the level's completion was sent
extern int  mp_level_index;                             // 0x247e834 level_table index of cur_level_index, -1
extern Character *mp_level_boss;                        // 0x247e838 the level's boss, NULL none

void mp_send_level_complete(int level);                 // 0x4b0960 L_CMPLT (host)
void mp_handle_level_complete(ubyte *data, void *from); // 0x4b09c0 L_CMPLT (clients)
void mp_mark_level_complete(int level);                 // 0x4b09f0
void mp_send_difficulty_complete(int difficulty);       // 0x4b0a50 D_CMPLT (host)
void mp_handle_difficulty_complete(ubyte *data, void *from);   // 0x4b0ab0 D_CMPLT (clients)
void mp_mark_difficulty_complete(int difficulty);       // 0x4b0ae0
void mp_level_find_boss();                              // 0x4b0b50 (mp_level_start, host)
void mp_level_complete_check();                         // 0x4b1150 (mp_game_frame, host)
// Packet 0x22 of a living_combat exchange, the arguments as living_combat_send_net pushes them:
// (attacker net handle, defender net handle, move variant, defence move, hit flag (living_combat state
// argument), skill, special). decomp68 (D4801): the four flags are bytes in the packet and the defence
// is a move (its category byte is sent); were int / void *.
void mp_send_living_combat(int attacker, int defender, ubyte variant, move_info *defence, ubyte hit,
                           ubyte skill, bool special);  // 0x4b11a0
void mp_handle_combat_sequence(ubyte *data, void *from);   // 0x4b1280 COM_SEQ

#endif
