// unnamed_lobby_0x43ef30.cpp - Sum.exe v1.40, code 0x43ef30-0x43f9cc: the PXO lobby, game state 11 (the
// IRC chat of screen 11: connecting, the channel list with its periodic "/list", joining a channel, the
// user list, routing the chat lines to the panes) and the state 10 enter.
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl block, sum.symbols module "-"; docs/sum-exe/source-map.md "Hand cuts inside the unnamed
// units"). Rename the file when the name turns up.
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .data 0x594fcc-0x594ff7 (two initialised ints, then the literals "%s %s", "/list", "!EMOTD$#!",
// "* ", "$"; "!MOTD$#!", "**" and " " are pooled copies of other units, /Gf), .bss 0x60ae68-0x60f1c7.
// The .data starts with a variable after the menu unit's literals, which is where a new object begins
// (each object's .data is its variables, then its literals); see NOTES.md.

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "vsdk/os/timestamp.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/gr/gr.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "summoner/interface/new_charselect.h"
#include "summoner/interface/new_pxochat.h"
#include "summoner/multi/multi.h"

#define MOTD_PREFIX    "!MOTD$#!"
#define EMOTD_PREFIX   "!EMOTD$#!"
#define LOBBY_LINE_BUF 0x800

// chat pane kinds of pxochat_add_chat_line (names ours)
#define CHAT_KIND_TEXT   1
#define CHAT_KIND_SYSTEM 3
#define CHAT_KIND_ACTION 4
#define CHAT_KIND_SERVER 6

// ---- .data (0x594fcc) ----------------------------------------------------------------------------------
int lobby_unk_594fcc = 1;                                     // 0x594fcc no reference (name ours; unsure)
int lobby_join_channel_index = -1;                            // 0x594fd0

// ---- .bss (0x60ae68) -----------------------------------------------------------------------------------
int lobby_num_users;                                          // 0x60ae68
int lobby_num_channels;                                       // 0x60ae6c
unsigned int lobby_flags;                                     // 0x60ae70
int lobby_cur_channel;                                        // 0x60ae74
// 0x43ef30  init_realtimetimer_60ae78_thunk (static_initializers entry 147, jmp). Confidence: exact
// (compiler-generated).
// 0x43ef40  init_realtimetimer_60ae78 (mov ecx, 0x60ae78; jmp RealtimeTimer::RealtimeTimer). Confidence:
// exact (compiler-generated).
RealtimeTimer lobby_list_timer;                               // 0x60ae78
int lobby_connect_state;                                      // 0x60ae7c
char lobby_users[LOBBY_MAX_USERS][33];                        // 0x60ae80
lobby_channel lobby_channels[LOBBY_MAX_CHANNELS];             // 0x60eef8

// 0x43ef50  Confidence: exact. The index of the new record; the second count starts at 0.
int lobby_add_channel(lobby_channel *ch)
{
    if (lobby_num_channels < LOBBY_MAX_CHANNELS) {
        strcpy(lobby_channels[lobby_num_channels].name, ch->name);
        strcpy(lobby_channels[lobby_num_channels].topic, ch->topic);
        lobby_channels[lobby_num_channels].num_users = ch->num_users;
        lobby_channels[lobby_num_channels].unk8c = 0;
        return lobby_num_channels++;
    }
    return -1;
}

// 0x43eff0  Confidence: exact. Back from the game list (state 12) only the list refresh is due; else a
// fresh lobby: local player, empty lists, IRC reset.
void state_mp_lobby_enter()
{
    if (gameseq_get_prev_state() == 12) {
        lobby_list_timer.SetTimeout(0);
        interface_set_screen(11, 0);
        return;
    }
    pxochat_reset_lists();
    mp_local_player_create();
    lobby_flags = 0;
    lobby_connect_state = 0;
    lobby_num_channels = 0;
    lobby_cur_channel = -1;
    pxochat_sync_channel_list();
    lobby_num_users = 0;
    pxochat_sync_user_list();
    lobby_list_timer.invalidate();
    irc_chat_reset();
    interface_set_screen(11, 0);
}

// Part of state_mp_lobby_frame (its own `sub esp, 0x100` block at 0x43f110 re-reads lobby_flags before the
// test): written as an inline function. Name ours. irc_connect: -1 failed, 0 in progress, 1 connected;
// -2 (no such return in multi_chattracker.cpp's irc_connect, D3306) counts as connected too.
// Game bug: without a selected profile the error is recorded, but prof->name is still read (NULL + 0x28).
static inline void lobby_try_connect()
{
    char user_info[0x100];
    mp_char_profile *prof;
    int result;

    if (lobby_flags & LOBBY_CONNECTED)
        return;
    prof = mp_charselect_get_selected_profile(0);
    if (prof == NULL) {
        lobby_connect_state = 1;
        lobby_flags |= LOBBY_ERROR;
    }
    sprintf(user_info, "%s %s", pxo_login_reply, prof->name);
    result = irc_connect(pxo_chat_server, prof->name, user_info);
    if (result == -2 || result == 1) {
        lobby_flags |= LOBBY_CONNECTED;
    } else if (result != 0) {
        lobby_connect_state = 1;
        lobby_flags |= LOBBY_ERROR;
    }
}

// 0x43f080  Confidence: exact (VC6 byte match, match24; was likely) (the connect block is the inline function above; the last two cases end in
// jumps to lobby_request_channel_list / lobby_query_all_channels). Once logged in: connect, ask for the
// channel list, finish a pending join, join the first channel, then refresh the list every 10 s.
void state_mp_lobby_frame()
{
    cursor_visible = true;
    interface_set_view_clip();
    gr_clear();
    lobby_poll_irc();
    interface_frame(PollInputEvents());
    interface_draw();
    if (!pxochat_logged_in)
        return;
    if (!(lobby_flags & LOBBY_CONNECTED))
        lobby_try_connect();
    else if (!(lobby_flags & LOBBY_LIST_SENT))
        lobby_request_channel_list();
    else if (lobby_flags & LOBBY_JOINING)
        lobby_join_channel(NULL);
    else if (lobby_cur_channel < 0 && lobby_num_channels > 0)
        lobby_join_channel(lobby_channels[0].name);
    else if (lobby_list_timer.elapsed()) {
        lobby_request_channel_list();
        lobby_query_all_channels();
    }
}

// 0x43f1c0  Confidence: exact. ("/list" is sent with quiet = 0.)
void lobby_request_channel_list()
{
    lobby_list_timer.SetTimeout(LOBBY_LIST_PERIOD);
    irc_process_input("/list", 0);
    lobby_flags |= LOBBY_LIST_SENT;
}

// 0x43f1f0  Confidence: exact.
void lobby_query_all_channels()
{
    int i;

    for (i = 0; i < lobby_num_channels; i++)
        lobby_query_channel(i);
}

// 0x43f220  Confidence: exact (VC6 byte match, match24; was likely) (the zero store is scheduled inside the strcpy; source order guessed). The
// game tracker's user count of the channel.
void lobby_query_channel(int index)
{
    gtrack_channel_query q;

    q.unk00 = 0;
    strcpy(q.channel, lobby_channels[index].name);
    gtrack_send_channel_query(&q);
}

// 0x43f280  Confidence: exact. Chat lines, then the channel list reply, then the IRC events.
void lobby_poll_irc()
{
    char *text;
    irc_command *cmd;

    while ((text = irc_get_chat_text()) != NULL)
        lobby_handle_chat_line(text);
    if ((text = irc_get_channel_list()) != NULL)
        lobby_parse_channel_list(text);
    while ((cmd = irc_get_chat_command()) != NULL)
        lobby_handle_irc_command(cmd);
}

// 0x43f2d0  Confidence: exact.
void lobby_handle_chat_line(char *line)
{
    if (chat_is_motd(line))
        pxochat_append_motd(line);
    else if (chat_is_emotd(line))
        pxochat_show_motd_thunk();
    else if (lobby_line_is_system(line))
        pxochat_add_chat_line(line, CHAT_KIND_SYSTEM);
    else if (lobby_line_is_action(line))
        pxochat_add_chat_line(line, CHAT_KIND_ACTION);
    else if (chat_is_server_line(line))
        pxochat_add_chat_line(line, CHAT_KIND_SERVER);
    else
        pxochat_add_chat_line(line, CHAT_KIND_TEXT);
}

// 0x43f360  Confidence: exact (VC6 byte match, match24; was likely) (the prefix length is computed first: a local or a strlen of the literal).
bool chat_is_motd(const char *line)
{
    int len = strlen(MOTD_PREFIX);

    if ((int)strlen(line) >= len && strncmp(line, MOTD_PREFIX, len) == 0)
        return true;
    return false;
}

// 0x43f3b0  Confidence: exact (VC6 byte match, match24; was likely) (as chat_is_motd).
bool chat_is_emotd(const char *line)
{
    int len = strlen(EMOTD_PREFIX);

    if ((int)strlen(line) >= len && strncmp(line, EMOTD_PREFIX, len) == 0)
        return true;
    return false;
}

// 0x43f400  Confidence: exact (neg / sbb / inc: the int result of the compare).
bool lobby_line_is_system(const char *line)
{
    return strncmp(line, "**", 2) == 0;
}

// 0x43f420  Confidence: exact.
bool lobby_line_is_action(const char *line)
{
    return strncmp(line, "* ", 2) == 0;
}

// 0x43f440  Confidence: exact.
bool chat_is_server_line(const char *line)
{
    return line[0] == '<';
}

// 0x43f450  Confidence: exact (VC6 byte match, match24; was likely) (only the first name skips its '$': each topic's strtok(.., "$") eats the
// next record's '$'). Each "$name users topic " record updates a known channel's user count or adds the
// channel; a record cut short returns without the list refresh.
void lobby_parse_channel_list(const char *text)
{
    lobby_channel ch;
    char buf[LOBBY_LINE_BUF];
    char *name;
    char *users;
    char *topic;
    int num_users;
    int index;

    strcpy(buf, text);
    name = strtok(buf, " ");
    if (name == NULL)
        return;
    for (name++; name != NULL; name = strtok(NULL, " ")) {
        users = strtok(NULL, " ");
        if (users == NULL)
            return;
        num_users = atoi(users);
        topic = strtok(NULL, "$");
        if (topic == NULL)
            return;
        trim_whitespace(ch.name, name, -1);
        ch.num_users = num_users;
        trim_whitespace(ch.topic, topic, -1);
        index = lobby_find_channel_index(ch.name);
        if (index >= 0)
            lobby_channels[index].num_users = num_users;
        else
            lobby_add_channel(&ch);
    }
    pxochat_sync_channel_list();
}

// 0x43f560  Confidence: exact (inline strcmp).
int lobby_find_channel_index(const char *name)
{
    int i;

    for (i = 0; i < lobby_num_channels; i++) {
        if (strcmp(name, lobby_channels[i].name) == 0)
            return i;
    }
    return -1;
}

// 0x43f5d0  Confidence: exact. The joined channel's user count follows the joins and parts.
void lobby_handle_irc_command(irc_command *cmd)
{
    int index;

    switch (cmd->command) {
    case IRC_CMD_USER_JOINED:
        if (lobby_find_user(cmd->data) < 0)
            lobby_add_user(cmd->data);
        if (lobby_cur_channel >= 0)
            lobby_channels[lobby_cur_channel].num_users++;
        pxochat_sync_user_list();
        break;
    case IRC_CMD_USER_LEFT:
        index = lobby_find_user(cmd->data);
        if (index >= 0)
            lobby_remove_user(index);
        if (lobby_cur_channel >= 0)
            lobby_channels[lobby_cur_channel].num_users--;
        pxochat_sync_user_list();
        break;
    case IRC_CMD_DISCONNECTED:
        lobby_disconnect();
        break;
    }
}

// 0x43f660  Confidence: exact.
int lobby_find_user(const char *name)
{
    int i;

    for (i = 0; i < lobby_num_users; i++) {
        if (_stricmp(name, lobby_users[i]) == 0)
            return i;
    }
    return -1;
}

// 0x43f6a0  Confidence: exact.
int lobby_add_user(const char *name)
{
    if (lobby_num_users < LOBBY_MAX_USERS) {
        strcpy(lobby_users[lobby_num_users], name);
        return lobby_num_users++;
    }
    return -1;
}

// 0x43f6f0  Confidence: exact (VC6 byte match, match24; was likely) (the size is the distance of two row addresses: both products are formed).
void lobby_remove_user(int index)
{
    if (index >= 0 && index < lobby_num_users) {
        memmove(lobby_users[index], lobby_users[index + 1],
                (char *)lobby_users[lobby_num_users] - (char *)lobby_users[index + 1]);
        lobby_num_users--;
    }
}

// 0x43f740  Confidence: exact. To the game list the joined channel becomes the tracker channel and the
// list filter; any other way out leaves multiplayer.
void state_mp_lobby_leave()
{
    if (gameseq_get_next_state() == 12) {
        strcpy(tracker_game_channel, lobby_channels[lobby_cur_channel].name);
        strcpy(tracker_filter_channel, lobby_channels[lobby_cur_channel].name);
        return;
    }
    lobby_disconnect();
    game_flags = GF_SINGLE_PLAYER;
    mp_player_disconnect(mp_local_player);
    mp_local_player = NULL;
}

// 0x43f7d0  Confidence: exact (VC6 byte match, match24; was likely) (the global is the loop index: its stores follow each step). With a name:
// find it, clear the user list and start the join; with NULL (state_mp_lobby_frame while joining): poll the
// pending join. irc_join_channel: 1 joined, 0 still joining, else failed (error 2).
void lobby_join_channel(const char *name)
{
    if (name != NULL) {
        for (lobby_join_channel_index = 0; lobby_join_channel_index < lobby_num_channels;
             lobby_join_channel_index++) {
            if (strcmp(name, lobby_channels[lobby_join_channel_index].name) == 0)
                break;
        }
        if (lobby_join_channel_index >= lobby_num_channels) {
            lobby_join_channel_index = -1;
            return;
        }
        lobby_num_users = 0;
        lobby_cur_channel = -1;
        pxochat_sync_user_list();
    }
    if (lobby_join_channel_index < 0)
        return;
    lobby_flags |= LOBBY_JOINING;
    switch (irc_join_channel(lobby_channels[lobby_join_channel_index].name)) {
    case 0:
        break;
    case 1:
        lobby_cur_channel = lobby_join_channel_index;
        lobby_join_channel_index = -1;
        lobby_request_channel_list();
        lobby_query_channel(lobby_cur_channel);
        lobby_flags &= ~(LOBBY_JOINING | LOBBY_TYPED);
        break;
    default:
        lobby_join_channel_index = -1;
        lobby_flags = (lobby_flags & ~(LOBBY_JOINING | LOBBY_TYPED)) | LOBBY_ERROR;
        lobby_connect_state = 2;
        break;
    }
}

// 0x43f910  Confidence: exact. The echo of the typed line (or the command's output) goes to the panes.
void lobby_send_chat_input(const char *text)
{
    char *line = irc_process_input((char *)text, 1);

    if (line)
        lobby_handle_chat_line(line);
}

// 0x43f930  Confidence: exact.
void lobby_disconnect()
{
    irc_disconnect();
    lobby_num_channels = 0;
    lobby_cur_channel = -1;
    lobby_num_users = 0;
    lobby_list_timer.invalidate();
    lobby_flags &= ~(LOBBY_CONNECTED | LOBBY_LIST_SENT);
}

// 0x43f970  Confidence: exact.
lobby_channel *lobby_find_channel(const char *name)
{
    int i;

    for (i = 0; i < lobby_num_channels; i++) {
        if (_stricmp(name, lobby_channels[i].name) == 0)
            return &lobby_channels[i];
    }
    return NULL;
}

// 0x43f9c0  Confidence: exact. State 10: the multiplayer character select screen.
void state_mp_charselect_enter()
{
    interface_set_screen(16, 0);
}
