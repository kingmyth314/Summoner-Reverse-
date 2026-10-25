// summoner/multi/multi_chattracker.h - multi_chattracker.cpp (0x4abb80-0x4ae32d: the IRC client behind the
// PXO lobby, screen 11; strings.tbl ids 238-245 for its own lines). File name from the module column of
// sum.symbols, directory ours. Included by summoner/multi/multi.h. The lobby side (lobby_poll_irc
// 0x43f280, state_mp_lobby_frame, lobby_join_channel, ...) is in another unit. Not original source.
#ifndef SUMMONER_MULTI_MULTI_CHATTRACKER_H
#define SUMMONER_MULTI_MULTI_CHATTRACKER_H

#include "vsdk/vsdk.h"

// irc_command.command: what the lobby gets from irc_get_chat_command (lobby_handle_irc_command 0x43f5d0).
#define IRC_CMD_USER_JOINED   1       // data = nick (irc_add_user)
#define IRC_CMD_USER_LEFT     2       // data = nick (irc_remove_user, irc_remove_all_users)
#define IRC_CMD_DISCONNECTED  3       // no data: recv failed (irc_read_line) or numeric 432
#define IRC_CMD_KICKED        4       // no data: we were kicked from the channel
#define IRC_CMD_NICK_CHANGED  5       // data = "old new"
#define IRC_CMD_CHANNEL_SET   6       // data = channel: "#autoselect" resolved to the joined channel

// A queued command for the lobby (0x6c bytes, malloc'ed; list irc_cmd_list). Type and field names ours.
struct irc_command {
    short command;                    // +0x00 IRC_CMD_*
    char  data[0x66];                 // +0x02
    irc_command *next;                // +0x68
};
LAYOUT_CHECK_32(irc_command, sizeof(irc_command) == 0x6c);

// A user of the joined channel (0x28 bytes; list irc_user_list). Type and field names ours.
struct irc_user {
    char nick[0x24];                  // +0x00
    irc_user *next;                   // +0x24
};
LAYOUT_CHECK_32(irc_user, sizeof(irc_user) == 0x28);

// One RPL_LIST (322) entry (0x8c bytes; list irc_channel_list, rebuilt per LIST). Type and field names ours.
struct irc_channel {
    char  name[0x22];                 // +0x00
    unsigned short users;             // +0x22 (unsigned: D9802)
    char  topic[0x64];                // +0x24
    irc_channel *next;                // +0x88
};
LAYOUT_CHECK_32(irc_channel, sizeof(irc_channel) == 0x8c);

int   irc_send_raw(char *text, int len);                 // 0x4abb80 send() result
void  irc_chat_reset();                                  // 0x4abba0 (state_mp_lobby_enter)
// (server, nick, user info): -1 error, 0 in progress, 1 connected and welcomed (state_mp_lobby_frame).
int   irc_connect(char *server, char *nick, char *user_info);   // 0x4abc10
void  irc_flush_command_queue();                         // 0x4abee0
void  irc_disconnect();                                  // 0x4abf20 (lobby_disconnect)
void  irc_remove_all_users();                            // 0x4abfc0
void  irc_queue_command(int command, void *data, int len);      // 0x4ac020
char *irc_get_chat_text();                               // 0x4ac0e0 a line for the chat box, or NULL
char *irc_read_line();                                   // 0x4ac100
// (line, mode): mode 0 a line from the server, 1 a line we sent (no prefix). The formatted text or NULL.
char *chat_process_irc_message(char *line, int mode);    // 0x4ac240
char *irc_get_word(int n, const char *text);                   // 0x4ad8b0 word n, or the rest of the line after a ':'
void  irc_flush_channel_list();                          // 0x4ad9a0
void  irc_add_channel(char *name, unsigned short users, char *topic);    // 0x4ad9e0
int   irc_add_user(char *nick);                          // 0x4adb40 0 when already listed
int   irc_remove_user(char *nick);                       // 0x4adc40 0 when not listed
// (typed line, quiet): a slash command or a channel message. quiet passes unknown commands to the server.
char *irc_process_input(const char *line, int quiet);          // 0x4adcd0
irc_command *irc_get_chat_command();                     // 0x4adfb0
irc_command *irc_pop_command();                          // 0x4adfd0
// -1 not connected / join failed, 0 joining, 1 joined.
int   irc_join_channel(char *name);                      // 0x4ae010 (lobby_join_channel)
// "$name users topic " per channel, after RPL_LISTEND (323); NULL otherwise.
char *irc_get_channel_list();                            // 0x4ae120

#endif
