// summoner/multi/multichat.h - multichat.cpp (0x487b90-0x488223: the in-game chat, its slash commands and
// the received-lines ring; packet 0x0c CHAT_LN; strings.tbl ids 218-222). File name from the module column
// of sum.symbols, directory ours. Included by summoner/multi/multi.h. Not original source.
#ifndef SUMMONER_MULTI_MULTICHAT_H
#define SUMMONER_MULTI_MULTICHAT_H

#include "vsdk/vsdk.h"

#define CHAT_HISTORY_SIZE 64

// chat_entry flags
#define CHAT_FLAG_WHISPER 0x1         // a /msg line: shown as "<name> text"
#define CHAT_FLAG_SYSTEM  0x2         // sender -1: shown as "** text **"

// One received line (0x108 bytes, chat_history 0x22fede0). Type and field names ours.
struct chat_entry {
    int sender;                       // +0x000 player id, -1 for the game's own lines
    unsigned int flags;               // +0x004 CHAT_FLAG_*
    char text[0x100];                 // +0x008
};
LAYOUT_CHECK(chat_entry, sizeof(chat_entry) == 0x108);

// A slash command (chat_commands 0x59c180: "/kick", "/ping", "/list"). Type name ours.
struct chat_command {
    const char *name;
    void (*func)(const char *arg);    // the next word of the line, or NULL
};
extern chat_command chat_commands[3];                   // 0x59c180
extern int chat_num_commands;                           // 0x59c198 (3; name ours)
extern int chat_history_read;                           // 0x22fedd8
extern int chat_history_write;                          // 0x22feddc
extern chat_entry chat_history[CHAT_HISTORY_SIZE];      // 0x22fede0

void chat_cmd_kick(const char *name);                   // 0x487b90 host only
void chat_cmd_ping(const char *name);                   // 0x487c10
void chat_cmd_list(const char *unused);                 // 0x487d00
void chat_history_clear_thunk();                        // 0x487db0 (mp_session_reset)
void chat_history_clear();                              // 0x487dc0
void chat_history_clear_thunk2();                       // 0x487dd0 (mp_leave_game)
chat_entry *chat_history_pop();                         // 0x487de0 NULL when empty
char *chat_format_entry(char *out, chat_entry *e);      // 0x487e20 returns out
// (sender id, text): runs a slash command (true) or queues the line; false = also show / forward it.
bool chat_process_command(int sender, const char *text);   // 0x487ea0
chat_entry *chat_history_push();                        // 0x488030 NULL when full
void chat_send_line(const char *text);                  // 0x488070
void chat_build_packet(ubyte *data, ushort *size, int sender, const char *text);   // 0x488130
void mp_handle_chat_line(ubyte *data, void *from);      // 0x4881a0

#endif
