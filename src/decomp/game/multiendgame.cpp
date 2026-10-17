// multiendgame.cpp - Sum.exe v1.40, code 0x4811a0-0x481673 (leaving a network game)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Unit: sum.symbols module column (strings.tbl ids 225-235 of the `// multiendgame.cpp` block, read by
// mp_show_disconnect_popup and mp_handle_leave_game). It starts with its static initialiser (entry 238,
// 0x4811a0) after the respawn / FORCEPR handlers (0x481050-0x481196, module "-") and ends before the three
// empty initialisers 0x481680-0x4816d0 (entries 239-241) that open the lod_dist unit (0x4816e0); those
// are not included (no data ties them to this file).
// Not included either: mp_remove_player / mp_send_removed_notice / mp_handle_kick_pl (0x471df0-0x471fef),
// which read ids 233-234 of this block but sit 0xf000 bytes away, after multitrade.cpp (D2604).
// Data: .data 0x59bca8-0x59bd6f (multi_path_type = 1, then the console strings); .bss ccmd_Multi_path_type
// (0x22e94e8).
//
// Leaving: the host's departure ends the game for everyone (GM_OVER); a client tells the host (LV_GAME
// {reason, id}), shows the "Game Over" box for the reasons that have a text and leaves when it is closed.
// mp_leave_game saves the .plr, drops every connection and goes back to the game list (state 11 for a PXO
// game, 12 for a LAN one) or quits (state 3).

#include <stdio.h>
#include <string.h>
#include "vsdk/os/console.h"
#include "vsdk/localize/localize.h"
#include "vsdk/net/net.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/interface/interface.h"
#include "summoner/multi/multi.h"

#define GS_QUIT          3            // gameseq states (summoner.cpp's state table)
#define GS_PXO_GAMELIST  11
#define GS_LAN_GAMELIST  12

// ---- .data / .bss -------------------------------------------------------------------------------------
int multi_path_type = 1;                                     // 0x59bca8
// 0x4811a0 init_ccmd_Multi_path_type_thunk (jmp, static initialiser entry 238). Confidence: exact.
// 0x4811b0 init_ccmd_Multi_path_type (console_command constructor). Confidence: exact.
console_command ccmd_Multi_path_type("Multi_path_type",
    "Sets the method used to generate paths on multiplayer machiens", cmd_Multi_path_type);   // 0x22e94e8

// 0x4811d0  Confidence: exact (VC6 byte match, match24; was likely) (the name switch: case bodies laid out default, 1, 0). A valid value
// turns on the status print, an invalid one the usage; 2 is accepted but prints "Unknown".
void cmd_Multi_path_type()
{
    const char *name;

    if (calling_a_command_function) {
        process_command(ARG_INT);
        if (command_argument_int >= 0 && command_argument_int <= 2) {
            multi_path_type = command_argument_int;
            checking_status_for_command = 1;
        } else {
            doing_help_for_comand = 1;
        }
    }
    if (checking_status_for_command) {
        switch (multi_path_type) {
        case 0:
            name = "full path";
            break;
        case 1:
            name = "generate path";
            break;
        default:
            name = "Unknown";
            break;
        }
        print_to_console(name, 0);
    }
    if (doing_help_for_comand)
        print_to_console("Usage: multi_path_type <num> where num is 0: use full path; 1: generate paths\n", 0);
}

// 0x481260  Confidence: exact. Broadcast only (the host's call).
void mp_send_player_left(int reason)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    MP_BUILD_HEADER(MP_GM_OVER);
    MP_ADD_DATA(reason);
    MP_END_PACKET();
    mp_broadcast_packet(data, packet_size, 1);
}

// 0x4812a0  Confidence: exact (VC6 byte match, match24; was likely) (the player list walk keeps the next pointer before the call, written as
// a for loop with a saved next; the party test is the list-empty test of party_list).
void mp_leave_game(bool quit)
{
    mp_player *p, *next;

    mp_player_save_on_leave();
    if (MP_IS_HOST() && (mp_local_player->flags & MP_PF_TRACKED))
        tracker_end_game();
    for (p = mp_player_list.next, next = p->next; p != &mp_player_list; p = next, next = next->next)
        mp_player_disconnect(p);
    mp_local_player = NULL;
    if (party_list.next != &party_list)
        party_remove_all();
    chat_history_clear_thunk2();
    if (quit)
        gameseq_set_state(GS_QUIT);
    else if (pxo_cfg_flags & 1)
        gameseq_set_state(GS_PXO_GAMELIST);
    else
        gameseq_set_state(GS_LAN_GAMELIST);
}

// 0x481340  Confidence: exact (VC6 byte match, match24; was likely) (jump table 0x481430 over reasons 1-6; the bodies are laid out 1, 2, 3, 4,
// 6, 5, so case 6 is written before case 5). Reason 0 (and anything above 6) leaves the text empty: no box.
bool mp_show_disconnect_popup(int reason)
{
    char *buttons[1];
    char text[256] = "";

    switch (reason) {
    case MP_LEAVE_KICKED:
        strcpy(text, XSTR("You were kicked out of the game.", 225));
        break;
    case MP_LEAVE_SERVER_ENDED:
        strcpy(text, XSTR("The server has ended the game.", 226));
        break;
    case MP_LEAVE_NO_MEMORY:
        strcpy(text, XSTR("Insufficient memory.  Please exit other applications and try again.", 227));
        break;
    case MP_LEAVE_LOAD_FAILED:
        strcpy(text, XSTR("Level failed to load properly.", 228));
        break;
    case MP_LEAVE_DROPPED:
        strcpy(text, XSTR("The server dropped you from the game.", 229));
        break;
    case MP_LEAVE_LEVEL_STARTED:
        strcpy(text, XSTR("Server started the level without you.  You have been dropped from the game.", 230));
        break;
    }
    if (strlen(text) == 0)
        return false;
    buttons[0] = XSTR("OK", 231);
    ui_message_box_show(XSTR("Game Over", 232), text, -1, -1, 1, buttons, mp_disconnect_popup_cb, 2);
    return true;
}

// 0x481450  Confidence: exact.
void mp_disconnect_popup_cb(int button)
{
    mp_leave_game(false);
}

// 0x481460  Confidence: exact. {reason, id}
void mp_send_player_removed(int reason, int id)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    MP_BUILD_HEADER(MP_LV_GAME);
    MP_ADD_DATA(reason);
    MP_ADD_DATA(id);
    MP_END_PACKET();
    if (MP_IS_HOST())
        mp_broadcast_packet(data, packet_size, 1);
    else
        multi_send_packet(data, packet_size, NULL, 1);
}

// 0x4814e0  Confidence: exact (VC6 byte match, match24; was likely) (the reason test `== 1 || (> 4 && <= 6)`, as in mp_handle_leave_game). Runs
// once (MP_PF_LEAVING). A client sends LV_GAME except when it was kicked or dropped by the server (1, 5,
// 6: the host knows), closes its socket and leaves at once when no box was shown.
void mp_player_disconnected(int reason, bool quit)
{
    bool shown;

    if (mp_local_player->flags & MP_PF_LEAVING)
        return;
    mp_local_player->flags |= MP_PF_LEAVING;
    if (MP_IS_HOST()) {
        mp_send_player_left(reason);
    } else {
        shown = mp_show_disconnect_popup(reason);
        if (!(reason == MP_LEAVE_KICKED || (reason > 4 && reason <= 6)))
            mp_send_player_removed(reason, mp_local_player->id);
        nw_NewCloseSocket(&mp_local_player->socket);
        mp_local_player->socket = -1;
        if (shown)
            return;
    }
    mp_leave_game(quit);
}

// 0x481590  Confidence: exact. The packet's reason word is not read.
void mp_handle_game_over(ubyte *data, void *from)
{
    mp_show_disconnect_popup(MP_LEAVE_SERVER_ENDED);
    nw_NewCloseSocket(&mp_local_player->socket);
    mp_local_player->socket = -1;
}

// 0x4815c0  Confidence: exact (VC6 byte match, match24; was likely) (three sprintf calls whose common tail is merged; bodies laid out dropped,
// left, kicked). The host forwards the packet to the others before the chat line.
void mp_handle_leave_game(ubyte *data, void *from)
{
    char text[256];
    int reason, id;
    memcpy(&reason, data, 4);
    memcpy(&id, data + 4, 4);
    mp_player *p = mp_player_find_by_id(id);

    if (!p)
        return;
    if (MP_IS_HOST())
        mp_forward_cur_packet(from);
    if (reason != MP_LEAVE_KICKED) {
        if (reason > 4 && reason <= 6)
            sprintf(text, XSTR("%s was dropped from the game.", 234), p->name);
        else
            sprintf(text, XSTR("%s has left the game.", 235), p->name);
    } else {
        sprintf(text, XSTR("%s was kicked from the game.", 233), p->name);
    }
    chat_process_command(-1, text);
    mp_player_disconnect(p);
    mp_netgame_num_players--;
}
