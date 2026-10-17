// unnamed_mp_join_0x4ab0f0.cpp - Sum.exe v1.40, code 0x4ab0f0-0x4abb73 (network game: the host's game info
// and join handling, the level change, the wait state, and the glue to the PXO game tracker).
// FILE NAME NOT KNOWN (placeholder). No assert path, no strings.tbl block, no reference name; module "-".
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit: starts with its own dynamic initialiser (0x4ab0f0, static_initializers entry 302) after the
// alt_textures code; multi_chattracker.cpp starts at irc_send_raw (0x4abb80). D5206.
// Data: .data 0x59d3c4-0x59d41b (the accepted checksums, the warning, "%s_m1"; "**" 0x58e0c8 is pooled),
// .bss 0x24794bc-0x2479727 (multi_chattracker.cpp's starts at 0x2479728).

#include <stdio.h>
#include <string.h>
#include <winsock.h>
#include "vsdk/os/console.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/net/net.h"
#include "engine/objects/items.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/conversation.h"
#include "summoner/summoner.h"
#include "summoner/gameplay.h"
#include "summoner/interface/new_levelselect.h"
#include "summoner/multi/multi.h"
#include "summoner/multi/unnamed_mp_join_0x4ab0f0.h"

#define MP_NAME_WIDTH 50                    // pixels of mp_player::display_name (push 0x32)

// ---- .data (0x59d3c4-0x59d3cf) -----------------------------------------------------------------------
unsigned int mp_join_accepted_checksums[2] = { 0xf174baca, 0x6e4c08c4 };   // 0x59d3c4
int mp_join_num_accepted_checksums = 2;                                       // 0x59d3cc

// ---- .bss (0x24794bc-0x2479727), address order ---------------------------------------------------------
mp_player *mp_join_accept_player;           // 0x24794bc
RealtimeTimer mp_join_accept_timer;         // 0x24794c0 (0x24794c4 is never referenced)
gtrack_game_info tracker_game;              // 0x24794c8
char tracker_game_channel[0x100];           // 0x2479520
char tracker_filter_channel[0x100];         // 0x2479620
int tracker_initialized;                    // 0x2479720 (0x2479724 is never referenced)

// The compiler's dynamic initialiser pair of mp_join_accept_timer (static_initializers entry 302; no source
// of their own; one marker per line, decomp76):
// 0x4ab0f0 static_init_mp_join_timer (jmp). Confidence: exact.
// 0x4ab100 init_mp_join_timer (mov ecx, 0x24794c0 / jmp RealtimeTimer__RealtimeTimer). Confidence: exact.

// 0x4ab110  Confidence: exact. state14_enter.
void mp_join_timer_reset()
{
    mp_join_accept_timer.invalidate();
    mp_join_accept_player = NULL;
}

// 0x4ab130  Confidence: exact. state14_frame: the chat lines into the level select chat box; "**" lines
// (system messages) as type 4, "<name> text" as 3, the rest as 1.
void chat_history_pump()
{
    char text[0x200];
    chat_entry *e;

    while ((e = chat_history_pop()) != NULL) {
        chat_format_entry(text, sizeof(text), e);
        if (strlen(text) == 0)
            continue;
        if (strncmp(text, "**", 2) == 0)
            mp_levelselect_add_chat_line(text, 4);
        else if (text[0] == '<')
            mp_levelselect_add_chat_line(text, 3);
        else
            mp_levelselect_add_chat_line(text, 1);
    }
}

// 0x4ab1c0  Confidence: exact. GM_QURY: the host answers with its game info.
void mp_handle_game_query(ubyte *data, void *from)
{
    if (MP_IS_HOST())
        mp_build_game_info((net_addr *)from);
}

// 0x4ab1f0  Confidence: exact (VC6 byte match, match29; was likely) (the two version bytes are separate stores; the order of the trailing
// fields is the binary's). ACTV_GM {version 9, 9, name\0, level, players, max players, difficulty, lowest
// and highest character level, session state (byte), nw_network_status (byte)}.
void mp_build_game_info(net_addr *to)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    ushort len;
    int lo, hi;
    ubyte state = (ubyte)mp_netgame_state;
    ubyte status = (ubyte)nw_network_status;

    MP_BUILD_HEADER(MP_ACTV_GM);
    data[packet_size++] = MP_GAME_INFO_VERSION;
    data[packet_size++] = MP_GAME_INFO_VERSION;
    len = (ushort)strlen(mp_netgame_name);
    memcpy(data + packet_size, mp_netgame_name, len);
    packet_size += len;
    data[packet_size++] = 0;
    MP_ADD_DATA(mp_netgame_level);
    MP_ADD_DATA(mp_netgame_num_players);
    MP_ADD_DATA(mp_netgame_max_players);
    MP_ADD_DATA(mp_game_difficulty);
    lo = max(mp_netgame_host_level - mp_netgame_level_below, 1);
    MP_ADD_DATA(lo);
    hi = mp_netgame_host_level + mp_netgame_level_above;
    MP_ADD_DATA(hi);
    data[packet_size++] = state;
    data[packet_size++] = status;
    MP_END_PACKET();
    mp_send_tracked(data, packet_size, *to);
}

// 0x4ab350  Confidence: exact (VC6 byte match, match29; was likely) (the reason is chosen by an if / else chain and sent by one call, D8800; the string offsets are
// 16-bit, the integer reads are not masked). JN_RQST on the host: version, open, full, difficulty unlocked,
// level available, character level range, a join already pending, password; then accepted.
void mp_handle_join_request(ubyte *data, void *from)
{
    mp_join_request req;
    int len;
    int off;
    int reason;
    int i;

    strcpy(req.name, (char *)data);
    len = (ushort)(strlen((char *)data) + 1);
    strcpy(req.password, (char *)data + len);
    off = (ushort)(strlen((char *)data + len) + len + 1);
    memcpy(&req.char_info, data + off, 4);
    off += 4;
    memcpy(&req.profile_flags, data + off, 4);
    off += 4;
    memcpy(&req.level, data + off, 4);
    off += 4;
    memcpy(&req._unk3c, data + off, 4);
    off += 4;
    memcpy(&req._unk40, data + off, 4);
    off += 4;
    memcpy(&req.checksum, data + off, 4);

    reason = 0;
    if (req.checksum != items_table_checksum) {
        for (i = 0; i < mp_join_num_accepted_checksums; i++) {
            if (req.checksum == mp_join_accepted_checksums[i])
                break;
        }
        if (i == mp_join_num_accepted_checksums)
            reason = MP_DENY_VERSION;
    }
    if (reason == 0) {
        if (mp_netgame_state != 1)
            reason = MP_DENY_NOT_OPEN;
        else if (mp_netgame_num_players == mp_netgame_max_players)
            reason = MP_DENY_FULL;
        else if (mp_game_difficulty > 0 && !(req.profile_flags & (1 << (mp_game_difficulty + 25))))
            reason = MP_DENY_DIFFICULTY;
        else if (!mp_player_level_available(req.profile_flags, mp_netgame_level, mp_game_difficulty))
            reason = MP_DENY_LEVEL_LOCKED;
        else if (req.level < max(mp_netgame_host_level - mp_netgame_level_below, 1))
            reason = MP_DENY_LEVEL_LOW;
        else if (req.level > mp_netgame_host_level + mp_netgame_level_above)
            reason = MP_DENY_LEVEL_HIGH;
        else if (mp_netgame_flags & MP_NETGAME_JOIN_PENDING)
            reason = MP_DENY_JOIN_PENDING;
        else if (strcmp(req.password, mp_netgame_password) != 0)
            reason = MP_DENY_PASSWORD;
        else {
            mp_accept_join_request(req, *(net_addr *)from);
            return;
        }
    }
    mp_send_deny_join(reason, *(net_addr *)from);
}

// 0x4ab580  Confidence: exact (the constant size 7 and data size 4 are the macros folded).
void mp_send_deny_join(int reason, net_addr to)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    MP_BUILD_HEADER(MP_DENY_JN);
    MP_ADD_DATA(reason);
    MP_END_PACKET();
    mp_send_tracked(data, packet_size, to);
}

// 0x4ab5d0  Confidence: exact (VC6 byte match, match11; was likely) (the free id search is a separate loop over the mask). A request from an
// address already in the list is ignored (the warning goes to string_buffer only). The new record gets
// the lowest free id; the host marks a join pending for 15 s (mp_server_check_clients).
void mp_accept_join_request(mp_join_request req, net_addr from)
{
    mp_player *p;
    int used = 0;
    int id;
    int i;

    for (p = mp_player_list.next; p != &mp_player_list; p = p->next) {
        if (nw_addr_equal(p->addr, &from, true) == 0) {
            sprintf(string_buffer, "WARNING -- ignoring redundant join request packet from net ID%d\n", p->id);
            return;
        }
        used |= 1 << p->id;
    }
    id = -1;
    for (i = 0; i < MP_MAX_PLAYERS; i++) {
        if (!(used & (1 << i))) {
            id = i;
            break;
        }
    }

    p = mp_player_alloc();
    if (!p) {
        mp_send_deny_join(MP_DENY_NO_RECORD, from);
        return;
    }
    mp_netgame_flags |= MP_NETGAME_JOIN_PENDING;
    mp_join_accept_timer.SetTimeout(15000);
    mp_join_accept_player = p;
    p->id = id;
    *(net_addr *)p->addr = from;
    p->socket = -1;
    p->type = 2;
    p->join_timer.SetTimeout(0);
    p->flags = 0;
    p->char_info = req.char_info;
    p->profile_flags = req.profile_flags;
    strcpy(p->name, req.name);
    gr_string_truncate_ellipsis(p->display_name, req.name, MP_NAME_WIDTH, -1);
    mp_send_accept_join(p);
}

// 0x4ab710  Confidence: exact (VC6 byte match, match11; was likely) (the float goes through the MP_ADD_FLOAT temporary). The game info, then
// ACPT_JN {the new id, the host's id, the level scale}, buffered and flushed.
void mp_send_accept_join(mp_player *p)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    mp_build_game_info((net_addr *)p->addr);
    MP_BUILD_HEADER(MP_ACPT_JN);
    MP_ADD_DATA(p->id);
    MP_ADD_DATA(mp_local_player->id);
    MP_ADD_FLOAT(mp_netgame_level_scale);
    MP_END_PACKET();
    multi_send_packet_buffered(data, packet_size, p, true);
}

// 0x4ab780  Confidence: exact (VC6 byte match, match29; was likely) (the slot search's result is never used, D5211). Saves the characters, then
// loads the level of mp_netgame_level with the "<level>_m1" script: through game_change_level, or (game_flags
// 0x200) straight into state 4. No level: the wait state.
void mp_change_level(int entry_point)
{
    char script[0x40];
    int level, slot;
    level_script_info *ls;

    mp_player_save_on_leave();
    if (mp_netgame_level == -1) {
        mp_goto_wait_state();
        return;
    }
    level = level_table[mp_netgame_level].id;
    sprintf(script, "%s_m1", level_script_table[level].name);
    ls = &level_script_table[level];
    for (slot = 0; slot < 7; slot++) {
        conversation_save_info *c = CONVERSATION_SAVE_INFO(level, slot);
        if (!c->name || _stricmp(c->name, script) == 0)
            break;
    }

    if (!(game_flags & 0x200)) {
        game_change_level(ls->name, script);
    } else {
        strcpy(level_prev_filename, level_filename);
        strcpy(level_filename, ls->name);
        strcpy(level_script_name, script);
        level_entry_point = entry_point;
        gameseq_set_state(4);
    }
    mp_net_handle_map_reset();
    game_pause(0, 0);
    mp_set_local_status(MP_STATUS_LOADING);
    if (MP_IS_HOST())
        mp_set_session_state(2);
}

// 0x4ab8e0  Confidence: exact.
void mp_goto_wait_state()
{
    mp_set_local_status(MP_STATUS_LISTED);
    gameseq_set_state(14);
    if (MP_IS_HOST())
        mp_set_session_state(1);
}

// 0x4ab920  Confidence: exact (a tail jump).
void mp_unpause()
{
    game_unpause();
}

// 0x4ab930  Confidence: exact. Retried by every mp_session_reset until both trackers resolve.
void tracker_init()
{
    if (tracker_initialized == 1)
        return;
    if (gtrack_init(GTRACK_GAME_TYPE) && utrack_init())
        tracker_initialized = 1;
}

// 0x4ab960  Confidence: exact (VC6 byte match, match11; was likely) (the three zero stores after the memset are in the binary). The channel is
// copied into a 0x24-byte field from a 0x100-byte buffer (D5212).
void tracker_start_game()
{
    if (!tracker_initialized)
        return;
    memset(&tracker_game, 0, sizeof(tracker_game));
    strcpy(tracker_game.name, mp_netgame_name);
    tracker_game._unk20 = 0;
    tracker_game._unk24 = 0;
    tracker_game._unk28 = 0;
    tracker_game._unk2c = 4;
    tracker_game._unk30 = 1;
    if (strlen(tracker_game_channel))
        strcpy(tracker_game.channel, tracker_game_channel);
    gtrack_start_game(&tracker_game);
    mp_local_player->flags |= MP_PF_TRACKED;
}

// 0x4aba20  Confidence: exact.
void tracker_end_game()
{
    if (tracker_initialized && MP_IS_HOST() && (mp_local_player->flags & MP_PF_TRACKED)) {
        gtrack_send_game_over();
        memset(&tracker_game, 0, sizeof(tracker_game));
        mp_local_player->flags &= ~MP_PF_TRACKED;
    }
}

// 0x4aba70  Confidence: exact. With a filter channel of 1..31 characters only that channel's games.
void tracker_request_games()
{
    if (!tracker_initialized)
        return;
    int len = strlen(tracker_filter_channel);
    if (len > 0 && len < 0x20) {
        gtrack_channel_query q;
        memset(&q, 0, sizeof(q));
        strcpy(q.channel, tracker_filter_channel);
        gtrack_request_filtered_list(&q);
        return;
    }
    gtrack_request_game_list();
}

// 0x4abaf0  Confidence: exact. A GM_QURY to every server of a received list.
void tracker_query_listed_games(gtrack_game_list *list)
{
    for (int i = 0; i < 10; i++) {
        if (list->servers[i]) {
            net_addr a;
            a.ip = ntohl(list->servers[i]);
            a.port = ntohs(list->ports[i]);
            mp_send_gm_qury(&a);
        }
    }
}

// 0x4abb50  Confidence: exact. multi_do_frame.
void tracker_frame()
{
    nw_receive_all();
    if (tracker_initialized) {
        gtrack_send_heartbeat();
        gtrack_game_list *list = gtrack_get_game_list();
        if (list)
            tracker_query_listed_games(list);
    }
}
