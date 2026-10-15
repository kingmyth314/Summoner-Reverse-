// summoner/multi/unnamed_lobby_0x43ef30.h - the PXO lobby state 11 (0x43ef30-0x43f9cc: the IRC chat behind
// screen 11, its channel and user lists) and the state 10 enter. Source: game/unnamed_lobby_0x43ef30.cpp.
// THE FILE NAME IS A PLACEHOLDER (no assert path, no strings.tbl block; decomp53). The IRC client itself is
// multi_chattracker.cpp. Included by summoner/multi/multi.h. Not original source.
// The lobby_channel record and the lobby globals moved here from multi.h (decomp53).
#ifndef SUMMONER_MULTI_UNNAMED_LOBBY_0X43EF30_H
#define SUMMONER_MULTI_UNNAMED_LOBBY_0X43EF30_H

#include "vsdk/vsdk.h"
#include "vsdk/os/timestamp.h"

struct irc_command;

// One channel (0x90 bytes, lobby_channels 0x60eef8, at most 5). Type and field names ours.
struct lobby_channel {
    char name[0x21];                  // +0x00 IRC channel name ('+' prefix for one typed by the player)
    char topic[0x67];                 // +0x21 shown in the channel list
    int  num_users;                   // +0x88 first number of the list row
    int  unk8c;                       // +0x8c second number of the list row (lobby_add_channel zeroes it)
};
LAYOUT_CHECK(lobby_channel, sizeof(lobby_channel) == 0x90);

#define LOBBY_MAX_CHANNELS 5
#define LOBBY_MAX_USERS    500
#define LOBBY_LIST_PERIOD  10000      // ms between "/list" requests

// lobby_flags (names ours): 1 connected, 2 channel list requested, 4 joining a channel, 8 the typed channel
// is the one to join, 0x80000000 error (lobby_connect_state says which step).
#define LOBBY_CONNECTED    0x1
#define LOBBY_LIST_SENT    0x2
#define LOBBY_JOINING      0x4
#define LOBBY_TYPED        0x8
#define LOBBY_ERROR        0x80000000

// .data
extern int lobby_unk_594fcc;                            // 0x594fcc 1, no reference (name ours)
extern int lobby_join_channel_index;                    // 0x594fd0 -1 (also lobby_join_channel's loop index)
// .bss
extern int lobby_num_users;                             // 0x60ae68
extern int lobby_num_channels;                          // 0x60ae6c
extern unsigned int lobby_flags;                        // 0x60ae70
extern int lobby_cur_channel;                           // 0x60ae74 joined channel, -1 none
extern RealtimeTimer lobby_list_timer;                  // 0x60ae78
extern int lobby_connect_state;                         // 0x60ae7c printed by the "Error: %d" status
extern char lobby_users[LOBBY_MAX_USERS][33];           // 0x60ae80
extern lobby_channel lobby_channels[LOBBY_MAX_CHANNELS];   // 0x60eef8

int  lobby_add_channel(lobby_channel *ch);              // 0x43ef50 index, < 0 when the list is full
void state_mp_lobby_enter();                            // 0x43eff0 state 11
void state_mp_lobby_frame();                            // 0x43f080
void lobby_request_channel_list();                      // 0x43f1c0
void lobby_query_all_channels();                        // 0x43f1f0
void lobby_query_channel(int index);                    // 0x43f220
void lobby_poll_irc();                                  // 0x43f280
void lobby_handle_chat_line(char *line);                // 0x43f2d0
bool chat_is_motd(const char *line);                    // 0x43f360 "!MOTD$#!"
bool chat_is_emotd(const char *line);                   // 0x43f3b0 "!EMOTD$#!"
bool lobby_line_is_system(const char *line);            // 0x43f400 "**"
bool lobby_line_is_action(const char *line);            // 0x43f420 "* "
bool chat_is_server_line(const char *line);             // 0x43f440 '<'
void lobby_parse_channel_list(const char *text);        // 0x43f450 irc_get_channel_list's "$name users topic "
int  lobby_find_channel_index(const char *name);        // 0x43f560 -1 (strcmp)
void lobby_handle_irc_command(irc_command *cmd);        // 0x43f5d0
int  lobby_find_user(const char *name);                 // 0x43f660 -1 (_stricmp)
int  lobby_add_user(const char *name);                  // 0x43f6a0 index, -1 when full
void lobby_remove_user(int index);                      // 0x43f6f0
void state_mp_lobby_leave();                            // 0x43f740
void lobby_join_channel(const char *name);              // 0x43f7d0 NULL: poll the pending join
void lobby_send_chat_input(const char *text);           // 0x43f910
void lobby_disconnect();                                // 0x43f930
lobby_channel *lobby_find_channel(const char *name);    // 0x43f970 NULL (_stricmp)
void state_mp_charselect_enter();                       // 0x43f9c0 state 10 (screen 16)

#endif
