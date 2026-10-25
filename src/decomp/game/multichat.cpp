// multichat.cpp - Sum.exe v1.40, code 0x487b90-0x488223 (the in-game chat of a network game)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Unit: sum.symbols module column (strings.tbl ids 218-222). It follows mp_handle_unequip_item (0x487b10)
// and ends before mp_send_character_walk (0x488230).
// Data: .data 0x59c180-0x59c1cc (the command table, its count, "/ping", "/kick", the three line formats,
// "/msg"); "/list" (0x594fdc), " " (0x58e0cc) and "" (0x5bd0d0) are pooled copies in other units.
// .bss 0x22fedd8-0x2302fdf (the ring of received lines, read by chat_update_lines and chat_history_pump).
//
// A line typed by the player goes through chat_process_command with the local player's id: a slash
// command runs here and is not sent; anything else is sent as CHAT_LN and, on every machine (the sender
// too, through the host's forward), queued in the ring by chat_process_command. "/msg name text" is queued
// only on the target's and the sender's machine.

#include <stdio.h>
#include <string.h>
#include "vsdk/localize/localize.h"
#include "vsdk/net/net.h"
#include "summoner/summoner.h"
#include "summoner/multi/multi.h"

// ---- .data ---------------------------------------------------------------------------------------
chat_command chat_commands[3] = {                             // 0x59c180
    { "/kick", chat_cmd_kick },
    { "/ping", chat_cmd_ping },
    { "/list", chat_cmd_list },
};
int chat_num_commands = 3;                                    // 0x59c198
// then "/ping" (0x59c19c), "/kick" (0x59c1a4), "[%s] %s" (0x59c1ac), "<%s> %s" (0x59c1b4),
// "** %s **" (0x59c1bc), "/msg" (0x59c1c8).

// ---- .bss ----------------------------------------------------------------------------------------
int chat_history_read;                                        // 0x22fedd8
int chat_history_write;                                       // 0x22feddc
chat_entry chat_history[CHAT_HISTORY_SIZE];                   // 0x22fede0

// 0x487b90  Confidence: exact. A network client cannot kick; the host (or a single-player game, where
// mp_player_find_by_name finds nobody) looks the name up.
void chat_cmd_kick(const char *name)
{
    char buf[256];

    if (MP_IS_CLIENT())
        return;
    if (!name)
        return;

    mp_player *p = mp_player_find_by_name(name);
    if (p) {
        mp_remove_player(p, 1);
        return;
    }
    sprintf(buf, XSTR("Can't find player \"%s\"", 218), name);
    chat_process_command(-1, buf);
}

// 0x487c10  Confidence: exact (VC6 byte match, cl 8966; was likely: the host / client split as MP_IS_HOST() with
// else-if: a multiplayer game
// without a local player prints nothing, a client pings the server).
void chat_cmd_ping(const char *name)
{
    char buf[256] = "";

    if (!name)
        return;

    if (MP_IS_HOST()) {
        mp_player *p = mp_player_find_by_name(name);
        if (p) {
            if (p == mp_local_player)
                sprintf(buf, XSTR("Ping to %s: 0 ms", 219), p->name);
            else
                sprintf(buf, XSTR("Ping to %s: %d ms", 220), p->name, net_get_reliable_socket_ping(p->socket));
        } else {
            sprintf(buf, XSTR("Can't find player \"%s\"", 218), name);
        }
    } else if (mp_local_player) {
        sprintf(buf, XSTR("Ping to server: %d ms", 221), net_get_reliable_socket_ping(mp_local_player->socket));
    }

    if (strlen(buf))
        chat_process_command(-1, buf);
}

// 0x487d00  Confidence: exact. "Players: a b c" (the format has no arguments).
void chat_cmd_list(const char *unused)
{
    char buf[512];
    mp_player *p;

#ifdef MODERN_BUILD
    sprintf(buf, "%s", XSTR("Players:", 222));         // translated text is not a format
#else
    sprintf(buf, XSTR("Players:", 222));
#endif
    for (p = mp_player_list.next; p != &mp_player_list; p = p->next) {
        strcat(buf, " ");
        strcat(buf, p->name);
    }
    chat_process_command(-1, buf);
}

// 0x487db0  Confidence: exact (VC6 byte match, match24; was likely) ("jmp chat_history_clear": a function that only calls it, mp_session_reset's
// entry; name from sum.symbols).
void chat_history_clear_thunk()
{
    chat_history_clear();
}

// 0x487dc0  Confidence: exact.
void chat_history_clear()
{
    chat_history_read = 0;
    chat_history_write = 0;
}

// 0x487dd0  Confidence: exact (VC6 byte match, match24; was likely) (the same shape, mp_leave_game's entry).
void chat_history_clear_thunk2()
{
    chat_history_clear();
}

// 0x487de0  Confidence: exact (signed % 64: the and 0x8000003f / jns fix-up).
chat_entry *chat_history_pop()
{
    chat_entry *e = NULL;

    if (chat_history_read != chat_history_write) {
        e = &chat_history[chat_history_read];
        chat_history_read = (chat_history_read + 1) % CHAT_HISTORY_SIZE;
    }
    return e;
}

// 0x487e20  Confidence: exact. A line of a player that has left stays unformatted (out unchanged).
char *chat_format_entry(char *out, int size, chat_entry *e)
{
    if (e->sender == -1) {
        sprintf(out, "** %s **", e->text);
        return out;
    }

    mp_player *p = mp_player_find_by_id(e->sender);
    if (p) {
        if (e->flags & CHAT_FLAG_WHISPER) {
            sprintf(out, "<%s> %s", p->name, e->text);
            return out;
        }
        sprintf(out, "[%s] %s", p->name, e->text);
        return out;
    }
    return out;
}

// 0x487ea0  Confidence: exact (VC6 byte match, cl 8966; was likely: the whisper flag is a bool cleared at the top;
// "/msg" to a missing player
// returns true, the line is dropped). The game's own lines (sender -1) skip the command parse and are
// flagged CHAT_FLAG_SYSTEM. Returns true when the line was a command.
bool chat_process_command(int sender, const char *text)
{
    char buf[256];
    bool whisper = false;
    const char *line;
    int i;

    if (sender != -1) {
        strcpy(buf, text);
        char *word = strtok(buf, " ");
        for (i = 0; i < chat_num_commands; i++) {
            if (!_stricmp(word, chat_commands[i].name)) {
                chat_commands[i].func(strtok(NULL, " "));
                return true;
            }
        }
        if (!_stricmp(word, "/msg")) {
            mp_player *p = mp_player_find_by_name(strtok(NULL, " "));
            if (p) {
                if (p != mp_local_player && sender != mp_local_player->id)
                    return false;
                whisper = true;
                line = strtok(NULL, "");
            } else {
                return true;
            }
        } else {
            line = text;
        }
    } else {
        line = text;
    }

    if (line) {
        chat_entry *e = chat_history_push();
        if (e) {
            e->sender = sender;
            e->flags = 0;
            strcpy(e->text, line);
            if (sender == -1) {
                e->flags |= CHAT_FLAG_SYSTEM;
                return false;
            }
            if (whisper)
                e->flags |= CHAT_FLAG_WHISPER;
        }
    }
    return false;
}

// 0x488030  Confidence: exact. NULL when the ring is full (one slot stays free).
chat_entry *chat_history_push()
{
    chat_entry *e = NULL;
    int next = (chat_history_write + 1) % CHAT_HISTORY_SIZE;

    if (next != chat_history_read) {
        e = &chat_history[chat_history_write];
        chat_history_write = next;
    }
    return e;
}

// 0x488070  Confidence: exact. (sender id, text) as CHAT_LN, reliable.
void chat_send_line(const char *text)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    if (chat_process_command(mp_local_player->id, text))
        return;

    MP_BUILD_HEADER(MP_CHAT_LN);
    chat_build_packet(data, &packet_size, mp_local_player->id, text);
    if (MP_IS_HOST())
        mp_broadcast_packet(data, packet_size, 1);
    else
        multi_send_packet(data, packet_size, NULL, 1);
}

// 0x488130  Confidence: exact. Appends the id and the text with its terminator and closes the header.
void chat_build_packet(ubyte *data, ushort *size, int sender, const char *text)
{
    *(int *)(data + *size) = sender;
    *size += 4;
    ushort len = (ushort)strlen(text);
    memcpy(data + *size, text, len);
    *size += len;
    data[*size] = 0;
    (*size)++;
    *(ushort *)(data + 1) = *size - MP_HEADER_SIZE;
}

// 0x4881a0  Confidence: exact. The host forwards a line that is not a command (a whisper included).
void mp_handle_chat_line(ubyte *data, void *from)
{
    char text[256];
    int sender = *(int *)data;

    strcpy(text, (char *)data + 4);
    if (mp_player_find_by_id(sender) && !chat_process_command(sender, text) && MP_IS_HOST())
        mp_forward_cur_packet(from);
}
