// summoner/multi/multiendgame.h - multiendgame.cpp (0x4811a0-0x481673: leaving a network game, packets 8
// GM_OVER and 9 LV_GAME, the "Game Over" message box, the Multi_path_type console command; strings.tbl ids
// 225-235). File name from the module column of sum.symbols, directory ours. Included by
// summoner/multi/multi.h. Not original source (decomp47).
#ifndef SUMMONER_MULTI_MULTIENDGAME_H
#define SUMMONER_MULTI_MULTIENDGAME_H

#include "vsdk/vsdk.h"
#include "vsdk/os/console.h"

// Disconnect reasons (mp_show_disconnect_popup's switch, strings.tbl 225-230; LV_GAME's first word; names
// ours). 0 is a plain leave (no message box), 3 is mp_add_new_player's out-of-records case.
#define MP_LEAVE_NORMAL        0
#define MP_LEAVE_KICKED        1      // "You were kicked out of the game."
#define MP_LEAVE_SERVER_ENDED  2      // "The server has ended the game." (GM_OVER)
#define MP_LEAVE_NO_MEMORY     3      // "Insufficient memory. ..."
#define MP_LEAVE_LOAD_FAILED   4      // "Level failed to load properly."
#define MP_LEAVE_LEVEL_STARTED 5      // "Server started the level without you. ..."
#define MP_LEAVE_DROPPED       6      // "The server dropped you from the game."

// .data / .bss
extern int multi_path_type;                             // 0x59bca8 = 1 (mp_handle_new_path; 0 full path, 1 generate)
extern console_command ccmd_Multi_path_type;            // 0x22e94e8

void cmd_Multi_path_type();                             // 0x4811d0
// GM_OVER to every player (host side of mp_player_disconnected); the word is the reason, which
// mp_handle_game_over ignores (D2601).
void mp_send_player_left(int reason);                   // 0x481260
// Saves the .plr, ends the tracked game, closes every connection, then state 3 (quit = true), else 11
// (PXO game) or 12.
void mp_leave_game(bool quit);                          // 0x4812a0
bool mp_show_disconnect_popup(int reason);              // 0x481340 false: no text for the reason
void mp_disconnect_popup_cb(int button);                // 0x481450
// LV_GAME {reason, id}: broadcast on the host, sent to the server by a client (D2601).
void mp_send_player_removed(int reason, int id);        // 0x481460
// The local player leaves: (reason, quit). The level select and the HUD pass (2, 0) / (0, 0), gameplay.cpp
// (4, 0), mp_add_new_player (3, 0). The second argument goes on to mp_leave_game unchanged (D2601).
void mp_player_disconnected(int reason, bool quit);     // 0x4814e0
void mp_handle_game_over(ubyte *data, void *from);      // 0x481590 packet 8
void mp_handle_leave_game(ubyte *data, void *from);     // 0x4815c0 packet 9

#endif
