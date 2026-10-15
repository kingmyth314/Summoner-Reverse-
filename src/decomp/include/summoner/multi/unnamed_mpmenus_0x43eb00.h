// summoner/multi/unnamed_mpmenus_0x43eb00.h - the multiplayer menu states (0x43eb00-0x43ef20: states 10
// frame, 12 game list, 13 game creation, 14 level select), the level availability checks, mp_host_game and
// the all_multi_levels console command. Source: game/unnamed_mpmenus_0x43eb00.cpp. THE FILE NAME IS A
// PLACEHOLDER (no assert path, no strings.tbl block; decomp53). Included by summoner/multi/multi.h.
// Not original source.
#ifndef SUMMONER_MULTI_UNNAMED_MPMENUS_0X43EB00_H
#define SUMMONER_MULTI_UNNAMED_MPMENUS_0X43EB00_H

#include "vsdk/vsdk.h"

class console_command;

// gameseq states (gameseq_state_table): enter / frame / leave.
void state14_enter();                                   // 0x43eb00 level select (screen 18)
void state14_frame();                                   // 0x43eb30
void state14_leave();                                   // 0x43eb60
void state_mp_gamecreate_enter();                       // 0x43ed80 state 13 (screen 17)
void state_mp_menu_frame();                             // 0x43ed90 frame of states 10 and 13
void state_mp_gamelist_enter();                         // 0x43eed0 state 12 (screen 15)
void state_mp_gamelist_frame();                         // 0x43eef0
void state_mp_gamelist_leave();                         // 0x43ef20

// (completed-level mask, level, difficulty): the mask's top bits (>> 26) are the completed difficulties
// as a number; one at or above 1 << difficulty opens every level, else every prerequisite level of the
// level (level_table[level].unk0c bits) must be completed. decomp53 (D3304): the mask is signed (sar).
bool mp_player_level_available(int mask, int level, int difficulty);   // 0x43eb70
bool mp_level_available_to_all(int level, int difficulty);   // 0x43ebd0
void mp_levelselect_set_level(int level);               // 0x43ec20
void mp_levelselect_start_if_ready();                   // 0x43ec40
extern console_command ccmd_all_multi_levels;           // 0x60ae58
extern bool All_multi_levels;                           // 0x60ae64
void cmd_all_multi_levels();                            // 0x43ecc0
// (name, password, max players, difficulty, levels below, levels above): new_gamecreate.cpp's call.
void mp_host_game(const char *name, const char *password, int max_players, int difficulty,
                  int levels_below, int levels_above);   // 0x43edc0

#endif
