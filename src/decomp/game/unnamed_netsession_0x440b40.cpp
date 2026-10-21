// unnamed_netsession_0x440b40.cpp - Sum.exe v1.40, code 0x440b40-0x441817: the multiplayer session. The
// netgame block, the four player records with their used / free lists, the host and client frames
// (connection checks, accepting the reliable connections, the 500 ms status reports), the session state
// machine (GM_STAT from the host, CT_STAT from every player), the per-frame multiplayer pumps, the session
// reset and the local player record; also the respawn XP penalty.
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl block, sum.symbols module "-"; docs/sum-exe/source-map.md "Hand cuts inside the unnamed
// units", which keeps it together with the packet layer after it, D3309). Rename the file when the name
// turns up.
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .data 0x595158-0x5951b7 (literals only: "Changing netgame state to %d\n", "Unable to process client
// status packet -- unknown netid %d\n"), .bss 0x6513a8-0x65188f. The packet layer's object starts with
// mp_packet_table (a variable) at 0x5951b8, right after these literals.
//
// The session states (mp_netgame_state): 0 none, 1 level select (the host announces each listed player
// with NEW_INF), 2 loading (every player status 5: the host sends its PLY_DAT), 3 linking (status 7: the
// host sends the monster list), 4 objects (status 9: unpause), 5 playing. A client follows the host's
// GM_STAT only by one step from its current state (mp_handle_gm_stat).

#include <string.h>
#include <stdio.h>
#include "vsdk/os/timestamp.h"
#include "vsdk/os/console.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/os/debug.h"
#include "vsdk/net/net.h"
#include "vsdk/math/vector.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/spells/spells.h"
#include "summoner/summoner.h"
#include "summoner/gameplay.h"
#include "summoner/player/player.h"
#include "summoner/interface/new_charselect.h"
#include "summoner/multi/multi.h"

#define MP_UPDATE_PERIOD     500      // ms between the status reports
#define MP_DISCONNECT_LOST   6        // mp_player_disconnected reason: the server's socket failed
#define MP_NAME_WIDTH        50       // pixels of mp_player::display_name
#define MP_XP_PENALTY        0.1f     // 0x580214

// ---- .bss (0x6513a8) -----------------------------------------------------------------------------------
// In the order of the static initialisers (entries 154-157); the .bss addresses are not in that order.
mp_netgame_info mp_netgame;                                   // 0x6513a8
// 0x440b40  init_gametimers_65143c_thunk (static_initializers entry 154, jmp). Confidence: exact
// (compiler-generated).
// 0x440b50  init_gametimers_65143c: the implicit mp_player constructor inline, GameTimer::GameTimer on
// +0x24, +0x64, +0x68. Confidence: exact (compiler-generated).
mp_player mp_player_list;                                     // 0x651418
// 0x440b70  init_realtimetimer_651414_thunk (entry 155, jmp). Confidence: exact (compiler-generated).
// 0x440b80  init_realtimetimer_651414 (mov ecx, 0x651414; jmp RealtimeTimer::RealtimeTimer). Confidence:
// exact (compiler-generated).
RealtimeTimer mp_update_timer;                                // 0x651414
int mp_unk_6514d4;                                            // 0x6514d4 (unsure: only the gap is known)
// 0x440b90  init_gametimer_array_651604_thunk (entry 156, jmp). Confidence: exact (compiler-generated).
// 0x440ba0  init_gametimer_array_651604: the four records' constructors (a counted loop over 0xbc-byte
// steps, the GameTimers at +0x24 / +0x64 / +0x68 of each). Confidence: exact (compiler-generated).
mp_player mp_players[MP_MAX_PLAYERS];                         // 0x6515a0
// 0x440bd0  init_gametimers_6514fc_thunk (entry 157, jmp). Confidence: exact (compiler-generated).
// 0x440be0  init_gametimers_6514fc (the constructor of the free list head). Confidence: exact
// (compiler-generated).
mp_player mp_player_free_list;                                // 0x6514d8
unsigned short mp_net_port;                                   // 0x651594 (cmdline_read_params)
mp_player *mp_local_player;                                   // 0x651598
int mp_unk_65159c;                                            // 0x65159c (unsure: only the gap is known)

// 0x440c00  Confidence: exact. A client whose link to the server closed or failed leaves the game.
void mp_client_check_connection()
{
    int status;

    if (mp_local_player->socket == -1)
        return;
    status = net_get_reliable_socket_status(mp_local_player->socket);
    if (status == RNF_UNUSED || (status > RNF_CONNECTED && status <= 3))
        mp_player_disconnected(MP_DISCONNECT_LOST, false);
}

// 0x440c40  Confidence: exact (VC6 byte match, match23; was likely; the next pointer is read before the end test: a "safe" list walk). A
// player without a socket is dropped, except the pending join until its timer runs out; a player whose
// socket closed or failed is removed (with the leave message).
void mp_server_check_clients()
{
    mp_player *p;
    mp_player *next;
    int status;

    if (!MP_IS_HOST())
        return;
    for (p = mp_player_list.next, next = p->next; p != &mp_player_list; p = next, next = p->next) {
        if (p == mp_local_player)
            continue;
        if (p->socket == -1) {
            if ((mp_netgame_flags & MP_NETGAME_JOIN_PENDING) && p == mp_join_accept_player) {
                if (mp_join_accept_timer.elapsed()) {
                    mp_player_disconnect(p);
                    mp_join_accept_timer.invalidate();
                    mp_join_accept_player = NULL;
                    mp_netgame_flags &= ~MP_NETGAME_JOIN_PENDING;
                }
            } else {
                mp_player_disconnect(p);
            }
        } else {
            status = net_get_reliable_socket_status(p->socket);
            if (status == RNF_UNUSED || (status > RNF_CONNECTED && status <= 3))
                mp_remove_player(p, 0);
        }
    }
}

// 0x440d20  Confidence: exact. A connection from an address that is not a listed player keeps its
// socket open (no close; see NOTES).
void mp_server_accept_connections()
{
    net_addr addr;
    int socket;
    mp_player *p;

    socket = nw_accept_reliable(&addr);
    if (socket == -1)
        return;
    p = mp_player_find_by_addr(&addr, false);
    if (p) {
        p->socket = socket;
        mp_send_net_info(p);
        mp_netgame_num_players++;
    }
}

// 0x440d70  Confidence: exact (jump table on state - 1). The host's session steps.
void mp_host_update_session()
{
    mp_player *p;

    if (!MP_IS_HOST())
        return;
    switch (mp_netgame_state) {
    case 1:
        for (p = mp_player_list.next; p != &mp_player_list; p = p->next) {
            if (p != mp_local_player && p->type == MP_STATUS_LISTED && !(p->flags & MP_PF_ANNOUNCED)) {
                mp_broadcast_new_info(p);
                p->flags |= MP_PF_ANNOUNCED;
            }
        }
        break;
    case 2:
        if (mp_all_players_have_status(MP_STATUS_ENTERED)) {
            mp_send_player_data(mp_local_player);
            mp_set_local_status(MP_STATUS_LINKING);
            mp_set_session_state(3);
        }
        break;
    case 3:
        if (mp_all_players_have_status(MP_STATUS_LINKED)) {
            mp_send_monster_list();
            mp_set_local_status(MP_STATUS_OBJECTS);
            mp_set_session_state(4);
        }
        break;
    case 4:
        if (mp_all_players_have_status(MP_STATUS_READY)) {
            mp_unpause();
            mp_set_local_status(MP_STATUS_PLAYING);
            mp_set_session_state(5);
        }
        break;
    }
}

// 0x440e90  Confidence: exact.
void mp_server_frame()
{
    mp_server_check_clients();
    if (mp_netgame_state == 1)
        mp_server_accept_connections();
    mp_host_update_session();
    if (mp_local_player->type >= MP_STATUS_LOADING && mp_update_timer.elapsed()) {
        mp_update_timer.SetTimeout(MP_UPDATE_PERIOD);
        multi_set_netgame_state(false);
    }
}

// 0x440ee0  Confidence: exact.
void mp_client_frame()
{
    mp_client_check_connection();
    if (mp_local_player->type >= MP_STATUS_LOADING && mp_update_timer.elapsed()) {
        mp_update_timer.SetTimeout(MP_UPDATE_PERIOD);
        multi_process_client_status(mp_local_player, false);
    }
}

// 0x440f20  Confidence: exact (VC6 byte match, cl 8966; was likely: the packet offsets are constants: the size
// counter is folded; the pause
// flag is read first). GM_STAT: state, players, then (states 1 and 2) level, entry point and host level,
// or (state 5) the pause byte.
void multi_set_netgame_state(bool send_now)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    bool paused;

    if (!MP_IS_HOST())
        return;
    paused = game_is_paused();
    MP_BUILD_HEADER(MP_GM_STAT);
    MP_ADD_DATA(mp_netgame_state);
    MP_ADD_DATA(mp_netgame_num_players);
    if (mp_netgame_state > 0) {
        if (mp_netgame_state > 2) {
            if (mp_netgame_state == 5)
                MP_ADD_DATA(paused);
        } else {
            MP_ADD_DATA(mp_netgame_level);
            MP_ADD_DATA(level_entry_point);
            MP_ADD_DATA(mp_netgame_host_level);
        }
    }
    MP_END_PACKET();
    if (send_now)
        mp_broadcast_packet(data, packet_size, true);
    else
        mp_broadcast_packet_buffered(data, packet_size, true);
}

// 0x441000  Confidence: exact (VC6 byte match, match23; was likely; jump table on the current state - 1; the change-level and report tails are
// shared). A client takes the host's state only one step on from its own: 1 -> 2 changes the level, 2 -> 3
// sends its PLY_DAT, 3 -> 4 and 4 -> 5 advance its status; while playing it follows the pause and a level
// change (state 1 or 2 again).
void mp_handle_gm_stat(ubyte *data, void *from)
{
    int state;
    int level;
    int entry;
    ubyte paused;

    if (!MP_IS_CLIENT())
        return;
    paused = 0;
    level = -1;
    entry = -1;
    state = ((int *)data)[0];
    mp_netgame_num_players = ((int *)data)[1];
    if (state > 0) {
        if (state > 2) {
            if (state == 5)
                paused = data[8];
        } else {
            level = ((int *)data)[2];
            entry = ((int *)data)[3];
            mp_netgame_host_level = ((int *)data)[4];
        }
    }
    switch (mp_netgame_state) {
    case 1:
        if (mp_local_player->type == MP_STATUS_LISTED)
            mp_local_player->flags |= MP_PF_ANNOUNCED;
        mp_netgame_level = level;
        if (state != 2)
            return;
        mp_change_level(-1);
        break;
    case 2:
        if (state != 3)
            return;
        mp_send_player_data(mp_local_player);
        mp_set_local_status(MP_STATUS_LINKING);
        break;
    case 3:
        if (state != 4)
            return;
        mp_set_local_status(MP_STATUS_OBJECTS);
        break;
    case 4:
        if (state != 5)
            return;
        mp_unpause();
        mp_set_local_status(MP_STATUS_PLAYING);
        break;
    case 5:
        if (paused) {
            if (!game_is_paused())
                game_pause(0, 0);
        } else if (game_is_paused()) {
            game_unpause();
        }
        if (state != 2 && state != 1)
            return;
        mp_netgame_level = level;
        mp_change_level(entry);
        break;
    default:
        return;
    }
    sprintf(string_buffer, "Changing netgame state to %d\n", state);
    mp_set_session_state(state);
}

// 0x441150  Confidence: exact (VC6 byte match, w4 m-1020b D12800: the status read into a local before the header;
// constant packet offsets as in multi_set_netgame_state). CT_STAT (id,
// status): a client sends it to the server (reliable or buffered, both flushed), the host broadcasts it.
void multi_process_client_status(mp_player *p, bool reliable)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    int status = p->type;

    MP_BUILD_HEADER(MP_CT_STAT);
    MP_ADD_DATA(p->id);
    MP_ADD_DATA(status);
    MP_END_PACKET();
    if (MP_IS_CLIENT()) {
        if (reliable)
            multi_send_packet(data, packet_size, NULL, true);
        else
            multi_send_packet_buffered(data, packet_size, NULL, true);
    } else if (reliable) {
        mp_broadcast_packet(data, packet_size, true);
    } else {
        mp_broadcast_packet_buffered(data, packet_size, false);
    }
}

// 0x441210  Confidence: exact. Status 3 from the pending join ends the join wait; the host forwards it.
void mp_handle_ct_stat(ubyte *data, void *from)
{
    int id = ((int *)data)[0];
    int status = ((int *)data)[1];
    mp_player *p = mp_player_find_by_id(id);

    if (!p) {
        sprintf(string_buffer, "Unable to process client status packet -- unknown netid %d\n", id);
        return;
    }
    if (p == mp_join_accept_player && status == MP_STATUS_LISTED) {
        mp_join_accept_timer.invalidate();
        mp_join_accept_player = NULL;
        mp_netgame_flags &= ~MP_NETGAME_JOIN_PENDING;
    }
    p->type = status;
    if (MP_IS_HOST())
        mp_forward_cur_packet(from);
}

// 0x4412a0  Confidence: exact (VC6 byte match, match23; was likely; the owner search is an inline loop; empty_function is a folded call of an
// unknown empty function). The in-game multiplayer frame: autosave, receive, the local OBJ_UPDs, the trade
// timeouts; the host dismisses summons whose owner left; then everything buffered is flushed.
void mp_game_frame()
{
    Player *pl;
    Character *c;
    mp_player *p;

    if (!(game_flags & GF_MULTIPLAYER) || !mp_local_player)
        return;
    mp_player_autosave_frame();
    debug_noop_mp_frame();
    nw_receive_all();
    if (mp_netgame_state == 5)
        mp_send_local_updates();
    mp_trade_frame();
    if (MP_IS_HOST()) {
        mp_level_complete_check();
        for (pl = party_list.next; pl != &party_list; pl = pl->next) {
            c = living_from_handle(pl->handle);
            if (c && (c->flags & CF_SUMMON)) {
                for (p = mp_player_list.next; p != &mp_player_list; p = p->next) {
                    if (pl->mp_owner == p)
                        break;
                }
                if (p == &mp_player_list) {
                    mp_send_summon_quick(c->handle);
                    summon_dismiss(c->handle);
                }
            }
        }
    }
    empty_function();
    if (MP_IS_CLIENT()) {
        multi_send_packet_buffered(NULL, 0, NULL, true);
        multi_send_packet(NULL, 0, NULL, true);
    } else {
        mp_broadcast_packet_buffered(NULL, 0, true);
        mp_broadcast_packet(NULL, 0, true);
    }
}

// 0x4413d0  Confidence: exact (VC6 byte match, match23; was likely; the two status checks are a switch on 6 / 8 with inline list walks). The
// frame pump of every multiplayer state: tracker, receive, the loading steps 6 -> 7 (every player's data is
// in) and 8 -> 9 (every character has a net handle), then the host or client frame.
void multi_do_frame()
{
    mp_player *p;
    Character *c;

    if (!mp_local_player)
        return;
    if (pxo_cfg_flags & 1)
        tracker_frame();
    multi_process_incoming();
    switch (mp_local_player->type) {
    case MP_STATUS_LINKING:
        for (p = mp_player_list.next; p != &mp_player_list; p = p->next) {
            if (!(p->flags & MP_PF_DATA_RECEIVED))
                break;
        }
        if (p == &mp_player_list)
            mp_set_local_status(MP_STATUS_LINKED);
        break;
    case MP_STATUS_OBJECTS:
        for (c = living_list.live_next; c != &living_list; c = c->live_next) {
            if (c->net_handle == -1)
                break;
        }
        if (c == &living_list)
            mp_set_local_status(MP_STATUS_READY);
        break;
    }
    if (MP_IS_HOST())
        mp_server_frame();
    else
        mp_client_frame();
}

// 0x441490  Confidence: exact (VC6 byte match, D18800: mp_local_player cleared, then the free list's
// list_init, before the memset; the free list's prev word is stored only after the loop: list_init's
// store is merged into the appends). Empty lists, the four records on the free list with no socket, and
// the other multiplayer tables reset.
void mp_session_reset()
{
    int i;

    tracker_init();
    mp_update_timer.SetTimeout(0);
    list_init(&mp_player_list);
    mp_local_player = NULL;
    list_init(&mp_player_free_list);
    memset((void *)mp_players, 0, sizeof(mp_players));
    for (i = 0; i < MP_MAX_PLAYERS; i++) {
        mp_players[i].socket = -1;
        list_append(&mp_player_free_list, &mp_players[i]);
    }
    mp_net_handle_map_reset();
    empty_function();
    chat_history_clear_thunk();
    mp_reset_net_stats();
    objects_reset();
    multi_charinfo_slots_clear();
}

// 0x441530  Confidence: exact (state_game_enter).
void mp_level_start()
{
    mp_trade_reset();
    mp_player_autosave_start();
    mp_summon_claim_reset();
    if (MP_IS_HOST())
        mp_level_find_boss();
}

// 0x441560  Confidence: exact ("jmp pxo_read_config"; GameInit).
void pxo_read_config_thunk()
{
    pxo_read_config();
}

// 0x441570  Confidence: exact. The host reports every change at once; entering state 1 takes the host
// level from the selected character.
void mp_set_session_state(int state)
{
    mp_char_profile *prof;

    if (mp_netgame_state == state)
        return;
    mp_netgame_state = state;
    if (MP_IS_HOST()) {
        if (state == 1) {
            prof = mp_charselect_get_selected_profile(1);
            if (prof)
                mp_netgame_host_level = prof->level;
        }
        multi_set_netgame_state(true);
    }
}

// 0x4415d0  Confidence: exact. Statuses 0 and 1 are not reported.
void mp_set_local_status(int status)
{
    if (!mp_local_player || mp_local_player->type == status)
        return;
    mp_local_player->type = status;
    if (mp_local_player->socket != -1 && mp_local_player->type != 0 && mp_local_player->type != 1)
        multi_process_client_status(mp_local_player, true);
}

// 0x441610  Confidence: likely (differs under cl 12.00.8966, match23; was exact).
mp_player *mp_player_alloc()
{
    mp_player *p = mp_player_free_list.next;

    if (p == &mp_player_free_list)
        return NULL;
    list_remove(p);
    list_append(&mp_player_list, p);
    return p;
}

// 0x441660  Confidence: exact (VC6 byte match, w7: the append to the free list is list_append, prev
// stored first). The membership test is an inline loop; the unlink is guarded by both pointers.
void mp_player_disconnect(mp_player *p)
{
    mp_player *q;

    if (!p)
        return;
    for (q = mp_player_list.next; q != &mp_player_list; q = q->next) {
        if (q == p)
            break;
    }
    if (q == &mp_player_list)
        return;
    if (p->object) {
        party_remove_player(p->object, true);
        p->object = NULL;
    }
    if (p->socket != -1) {
        nw_NewCloseSocket(&p->socket);
        p->socket = -1;
    }
    if (p->next && p->prev)
        list_remove(p);
    list_append(&mp_player_free_list, p);
}

// 0x441710  Confidence: exact (fild / fmul 0.1f / _ftol). The respawn penalty (new_playmenu.cpp): 10% of
// the XP above the current level's threshold, never below it.
void character_xp_death_penalty(Character *c)
{
    int base = xp_for_level(c->level);
    int xp = c->xp;

    c->xp = xp - (int)((xp - base) * MP_XP_PENALTY);
    if (c->xp < base)
        c->xp = base;
}

// 0x441760  Confidence: exact. Game bug (NOTES): when no record is free or no character is selected, the
// state change to 10 is only requested and the NULL record / profile is used at once.
void mp_local_player_create()
{
    mp_char_profile *prof;

    game_flags = GF_MULTIPLAYER;
    mp_local_player = mp_player_alloc();
    if (mp_local_player == NULL)
        gameseq_set_state(10);
    prof = mp_charselect_get_selected_profile(1);
    if (prof == NULL)
        gameseq_set_state(10);
    mp_local_player->flags = 0;
    mp_local_player->type = 0;
    mp_local_player->object = NULL;
    mp_local_player->char_info = prof->char_info;
    mp_local_player->profile_flags = prof->flags;
    strcpy(mp_local_player->name, prof->name);
    gr_string_truncate_ellipsis(mp_local_player->display_name, prof->name, MP_NAME_WIDTH, -1);
}
