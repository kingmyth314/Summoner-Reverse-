// multi_chattracker.cpp - Sum.exe v1.40, code 0x4abb80-0x4ae32d (the IRC client of the PXO lobby: connect,
// the line reader, the server message parser, the command queue the lobby polls, users and channels).
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Unit: sum.symbols module column (strings.tbl ids 238-245). It follows tracker_frame (0x4abb50, module "-")
// and ends before gtrack_init (0x4ae330).
// Data: .data 0x59d41c-0x59d617 (the literals; the string order is that of first use in the file, which
// puts "/NICK %s" before "/USER ..."). Pooled copies from earlier units (/Gf): "" (0x5bd0d0), " " (0x58e0cc),
// "!" (0x594438), "%s %s" (0x594fd4), "!EMOTD$#!" (0x594fe4), "$" (0x594ff4), "!MOTD$#!" (0x59005c),
// "[%s] %s" (0x59c1ac) and "<%s> %s" (0x59c1b4, multichat.cpp).
// .bss 0x2479728-0x247b09b. All of it is used only here (no header declares it); sum.symbols labels
// irc_bind_addr, irc_get_word_line, irc_word_buf and chat_irc_format_buf, the other names are ours. Defined in
// address order; whether they were static (definition order) or common (linker order) is open.
//
// The server is pxo_chat_server, port 10555 (0x293b). The socket is non-blocking: irc_connect is called
// every frame; the first call starts the connect, later ones poll it with select() and send USER / NICK
// once writable; it returns 1 after the welcome (end of the MOTD, 376). Lines from the server go through
// chat_process_irc_message, which returns the text for the chat box (or NULL) and queues irc_command
// records (users joining / leaving, kicks, nick changes) that the lobby pops with irc_get_chat_command.

#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vsdk/localize/localize.h"
#include "summoner/multi/multi.h"

#define IRC_PORT      10555
#define IRC_MAX_LINE  600

// ---- .bss (address order) --------------------------------------------------------------------------
sockaddr_in irc_bind_addr;                    // 0x2479728 bind address, then the server address
irc_command *irc_cmd_list;                    // 0x2479738
irc_command *irc_cmd_cur;                     // 0x247973c
SOCKET irc_socket;                            // 0x2479740
char irc_get_word_line[IRC_MAX_LINE];         // 0x2479744
char irc_word_buf[IRC_MAX_LINE];              // 0x247999c
char irc_orig_nick[0x24];                     // 0x2479bf4 the nick asked for (433 appends a number)
char irc_nick[0x24];                          // 0x2479c18 the nick in use
irc_channel *irc_channel_list;                // 0x2479c3c
irc_user *irc_user_cur;                       // 0x2479c40
char irc_user_info[0x24];                     // 0x2479c44 USER real name
irc_user *irc_user_list;                      // 0x2479c68
irc_channel *irc_channel_cur;                 // 0x2479c6c
irc_command irc_cmd_result;                   // 0x2479c70 irc_pop_command's copy
char irc_message[0x800];                      // 0x2479cdc the complete line given to the parser
int  irc_connecting;                          // 0x247a4dc connect started
int  irc_nick_suffix;                         // 0x247a4e0 433: number appended to irc_orig_nick
char irc_cur_channel[0x24];                   // 0x247a4e4 joined channel, "" none
char irc_line_buf[0x800];                     // 0x247a508 the line being received
char irc_unused_ad08[0x24];                   // 0x247ad08 only cleared (irc_chat_reset)
char irc_unused_ad2c[0x24];                   // 0x247ad2c only cleared
int  irc_whois_not_found;                     // 0x247ad50 set by 401
int  irc_whois_done;                          // 0x247ad54 set by 401 too (never read)
char irc_whois_realname[0x64];                // 0x247ad58 311
char irc_whois_channels[0x64];                // 0x247adbc 319, "*" when 318 comes without one
char *irc_unused_ae20;                        // 0x247ae20 freed by irc_disconnect, never allocated
char *irc_channel_text;                       // 0x247ae24 irc_get_channel_list result
int  irc_connected;                           // 0x247ae28
int  irc_logged_in;                           // 0x247ae2c end of MOTD seen
int  irc_joining;                             // 0x247ae30 a JOIN is outstanding
int  irc_join_result;                         // 0x247ae34 1 when our JOIN came back
int  irc_list_state;                          // 0x247ae38 1 receiving the LIST, 2 complete
int  irc_unused_ae3c;                         // 0x247ae3c only cleared
int  irc_unused_ae40;                         // 0x247ae40 only cleared
char chat_irc_format_buf[IRC_MAX_LINE];       // 0x247ae44 the text chat_process_irc_message returns

// 0x4abb80  Confidence: exact.
int irc_send_raw(char *text, int len)
{
    return send(irc_socket, text, len, 0);
}

// 0x4abba0  Confidence: exact (the stores follow the addresses).
void irc_chat_reset()
{
    irc_connecting = 0;
    irc_nick_suffix = 0;
    irc_cur_channel[0] = 0;
    irc_line_buf[0] = 0;
    irc_unused_ad08[0] = 0;
    irc_unused_ad2c[0] = 0;
    irc_whois_not_found = 0;
    irc_whois_done = 0;
    irc_whois_realname[0] = 0;
    irc_whois_channels[0] = 0;
    irc_unused_ae20 = NULL;
    irc_channel_text = NULL;
    irc_connected = 0;
    irc_logged_in = 0;
    irc_joining = 0;
    irc_join_result = 0;
    irc_list_state = 0;
    irc_unused_ae3c = 0;
    irc_unused_ae40 = 0;
}

// 0x4abc10  Confidence: likely (the local sizes: host 0x34, buf 0x64 bytes from the frame; the -1 of a
// failed socket() is returned through the register, "or eax, eax"). A connect() error other than
// WSAEWOULDBLOCK returns 0 without setting irc_connecting, so the next call starts over.
int irc_connect(char *server, char *nick, char *user_info)
{
    unsigned long nonblocking = 1;
    timeval timeout;
    char host[50];
    char buf[100];
    fd_set write_fds;
    fd_set error_fds;

    if (!irc_connecting) {
        strcpy(irc_nick, nick);
        strcpy(irc_orig_nick, nick);
        strcpy(irc_user_info, user_info);
        irc_user_list = NULL;
        irc_cmd_list = NULL;
        irc_logged_in = 0;
        irc_flush_command_queue();
        strcpy(host, server);

        irc_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (irc_socket == INVALID_SOCKET)
            return -1;
        memset(&irc_bind_addr, 0, sizeof(irc_bind_addr));
        irc_bind_addr.sin_family = AF_INET;
        irc_bind_addr.sin_addr.s_addr = INADDR_ANY;
        irc_bind_addr.sin_port = 0;
        if (bind(irc_socket, (sockaddr *)&irc_bind_addr, sizeof(irc_bind_addr)) == SOCKET_ERROR)
            return -1;
        ioctlsocket(irc_socket, FIONBIO, &nonblocking);

        unsigned long addr = inet_addr(host);
        if (addr == INADDR_NONE) {
            hostent *he = gethostbyname(host);
            if (!he)
                return -1;
            addr = *(unsigned long *)he->h_addr_list[0];
        }
        irc_bind_addr.sin_addr.s_addr = addr;
        irc_bind_addr.sin_port = htons(IRC_PORT);
        if (connect(irc_socket, (sockaddr *)&irc_bind_addr, sizeof(irc_bind_addr)) == SOCKET_ERROR) {
            if (WSAGetLastError() == WSAEWOULDBLOCK)
                irc_connecting = 1;
            return 0;
        }
        irc_connecting = 1;
        irc_connected = 1;
        return 1;
    }

    if (irc_logged_in)
        return 1;
    if (irc_connected)
        return 0;

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    FD_ZERO(&write_fds);
    FD_SET(irc_socket, &write_fds);
    if (select(0, NULL, &write_fds, NULL, &timeout)) {
        irc_connected = 1;
        sprintf(buf, "/USER %s %s %s :%s", "user", "user", "user", irc_user_info);
        irc_process_input(buf, 1);
        sprintf(buf, "/NICK %s", irc_nick);
        irc_process_input(buf, 1);
        return 0;
    }
    FD_ZERO(&error_fds);
    FD_SET(irc_socket, &error_fds);
    if (select(0, NULL, NULL, &error_fds, &timeout))
        return -1;
    return 0;
}

// 0x4abee0  Confidence: exact.
void irc_flush_command_queue()
{
    irc_command *next;

    irc_cmd_cur = irc_cmd_list;
    while (irc_cmd_cur) {
        next = irc_cmd_cur->next;
        free(irc_cmd_cur);
        irc_cmd_cur = next;
    }
    irc_cmd_list = NULL;
}

// 0x4abf20  Confidence: exact.
void irc_disconnect()
{
    if (!irc_connected)
        return;

    irc_process_input("/QUIT", 1);
    shutdown(irc_socket, 2);
    closesocket(irc_socket);
    irc_connecting = 0;
    irc_connected = 0;
    irc_line_buf[0] = 0;
    if (irc_unused_ae20) {
        free(irc_unused_ae20);
        irc_unused_ae20 = NULL;
    }
    if (irc_channel_text) {
        free(irc_channel_text);
        irc_channel_text = NULL;
    }
    irc_logged_in = 0;
    irc_joining = 0;
    irc_join_result = 0;
    irc_remove_all_users();
    irc_flush_command_queue();
}

// 0x4abfc0  Confidence: exact.
void irc_remove_all_users()
{
    irc_user *next;

    irc_user_cur = irc_user_list;
    while (irc_user_cur) {
        next = irc_user_cur->next;
        irc_queue_command(IRC_CMD_USER_LEFT, irc_user_cur->nick, strlen(irc_user_cur->nick) + 1);
        free(irc_user_cur);
        irc_user_cur = next;
    }
    irc_user_list = NULL;
}

// 0x4ac020  Confidence: likely (command as int, stored as a short; the else-branch tails are the
// compiler's duplicates of the final next = NULL).
void irc_queue_command(int command, void *data, int len)
{
    irc_cmd_cur = irc_cmd_list;
    if (!irc_cmd_cur) {
        irc_cmd_list = (irc_command *)malloc(sizeof(irc_command));
        irc_cmd_list->next = NULL;
        irc_cmd_cur = irc_cmd_list;
    } else {
        while (irc_cmd_cur->next)
            irc_cmd_cur = irc_cmd_cur->next;
        irc_cmd_cur->next = (irc_command *)malloc(sizeof(irc_command));
        irc_cmd_cur = irc_cmd_cur->next;
    }
    irc_cmd_cur->command = (short)command;
    if (len && data)
        memcpy(irc_cmd_cur->data, data, len);
    irc_cmd_cur->next = NULL;
}

// 0x4ac0e0  Confidence: exact (tail jump).
char *irc_get_chat_text()
{
    if (!irc_connected)
        return NULL;
    return irc_read_line();
}

// 0x4ac100  Confidence: likely (loop form). Reads what the socket has, one byte at a time, into
// irc_line_buf; at CR or LF a non-empty line is parsed and its text returned. A line longer than 0x7ff
// bytes is cut. recv() failing (connection closed) queues IRC_CMD_DISCONNECTED.
char *irc_read_line()
{
    char ch[2];
    timeval timeout;
    fd_set read_fds;

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    FD_ZERO(&read_fds);
    FD_SET(irc_socket, &read_fds);
    if (select(0, &read_fds, NULL, NULL, &timeout)) {
        do {
            if (recv(irc_socket, ch, 1, 0) <= 0) {
                irc_queue_command(IRC_CMD_DISCONNECTED, NULL, 0);
                return NULL;
            }
            ch[1] = 0;
            if (ch[0] == '\n' || ch[0] == '\r') {
                if (!irc_line_buf[0])
                    return NULL;
                strcpy(irc_message, irc_line_buf);
                irc_line_buf[0] = 0;
                return chat_process_irc_message(irc_message, 0);
            }
            if (strlen(irc_line_buf) < sizeof(irc_line_buf) - 1)
                strcat(irc_line_buf, ch);
            else
                irc_line_buf[sizeof(irc_line_buf) - 1] = 0;
        } while (select(0, &read_fds, NULL, NULL, &timeout));
    }
    return NULL;
}

// 0x4ac240  Confidence: likely (the order of the seven 600-byte buffers in the frame and their "" / {0}
// initialisers; the sums of the string lengths are reassociated by the compiler). The tests are separate
// if statements, not an else-if chain: 318 has no return and goes on to the following tests, and the
// numerics 372 / 375 are tested twice (the second tests are dead). Lines that only change state return
// NULL; JOIN and plain NOTICEs format a text and still return NULL.
//   offset: how far the command starts in the line (the prefix length; -2 for our own lines, which have
//   neither the ':' nor the space).
char *chat_process_irc_message(char *line, int mode)
{
    char rest[IRC_MAX_LINE] = "";          // the parameters still to parse
    char prefix[IRC_MAX_LINE] = {0};       // nick!user@host
    char prefix_copy[IRC_MAX_LINE] = "";   // strtok'ed for the nick; KICK: the kicked nick
    char target[IRC_MAX_LINE] = "";        // first parameter (channel or nick)
    char nick[IRC_MAX_LINE] = {0};         // the sender
    char command[IRC_MAX_LINE] = "";
    char msg[IRC_MAX_LINE] = "";           // PRIVMSG / NOTICE text (CTCP: starts with 1)
    char tmp[33];
    char nick_change[70];
    char channel_topic[200];
    int offset = 0;
    char *word;
    char *tok;

    if (strlen(line) >= IRC_MAX_LINE)
        return NULL;

    if (mode == 0) {
        strcpy(rest, line);
        if (line[0] == ':') {
            word = irc_get_word(0, line + 1);
            strcpy(prefix, word);
            strcpy(prefix_copy, word);
            strcpy(rest, line + strlen(prefix) + 1);
        }
        tok = strtok(prefix_copy, "!");
        if (tok) {
            strcpy(nick, tok);
        } else {
            strncpy(nick, prefix, 31);
            nick[31] = 0;
        }
        offset = strlen(prefix);
    } else if (mode == 1) {
        strcpy(rest, line);
        offset = -2;
        strcpy(nick, irc_nick);
        strcpy(prefix, irc_nick);
    }

    word = irc_get_word(0, rest);
    if (!word[0])
        return NULL;
    strcpy(command, word);
    strcpy(rest, line + offset + strlen(command) + 2);

    if (!stricmp(command, "PRIVMSG")) {
        strcpy(target, irc_get_word(0, rest));
        strcpy(rest, line + offset + strlen(command) + strlen(target) + 4);
        if (rest[0] == ':')
            strcpy(msg, irc_get_word(0, rest + 1));
        else
            strcpy(msg, irc_get_word(0, rest));
        if (msg[strlen(msg) - 1] == 1)
            msg[strlen(msg) - 1] = 0;
        if (msg[0] == 1) {
            strcpy(rest, line + offset + strlen(command) + strlen(msg) + strlen(target) + 6);
            rest[strlen(rest) - 1] = 0;
            if (!stricmp(msg + 1, "ACTION")) {
                sprintf(chat_irc_format_buf, "* %s %s", nick, rest);
                return chat_irc_format_buf;
            }
            if (mode == 1) {
                strcpy(prefix_copy, line + offset + strlen(command) + strlen(target) + 4);
                rest[strlen(rest) - 1] = 0;
                sprintf(chat_irc_format_buf, "** CTCP %s %s %s", target, msg + 1, rest);
                return chat_irc_format_buf;
            }
            if (!stricmp(msg + 1, "PING")) {
                sprintf(chat_irc_format_buf, "/NOTICE %s :\001PING %s\001", nick, rest);
                return irc_process_input(chat_irc_format_buf, 1);
            }
            if (!stricmp(msg + 1, "VERSION"))
                return NULL;
            strcpy(rest, irc_get_word(0, line + offset + strlen(command) + strlen(target) + 4) + 1);
            rest[strlen(rest) - 1] = 0;
            sprintf(chat_irc_format_buf, "** CTCP Message from %s (%s)", nick, rest);
            return chat_irc_format_buf;
        }
        if (target[0] == '#') {
            sprintf(chat_irc_format_buf, "[%s] %s", nick, irc_get_word(0, rest));
            return chat_irc_format_buf;
        }
        sprintf(chat_irc_format_buf, "<%s> %s", nick, irc_get_word(0, rest));
        return chat_irc_format_buf;
    }

    if (mode == 1)
        return NULL;

    if (!stricmp(command, "NOTICE")) {
        strcpy(target, irc_get_word(0, rest));
        strcpy(rest, line + offset + strlen(command) + strlen(target) + 4);
        if (rest[0] == ':')
            strcpy(msg, irc_get_word(0, rest + 1));
        else
            strcpy(msg, irc_get_word(0, rest));
        if (msg[strlen(msg) - 1] == 1)
            msg[strlen(msg) - 1] = 0;
        if (msg[0] == 1) {
            strcpy(rest, line + offset + strlen(command) + strlen(msg) + strlen(target) + 6);
            rest[strlen(rest) - 1] = 0;
            if (!stricmp(msg + 1, "PING"))
                return NULL;
            strcpy(rest, irc_get_word(0, line + offset + strlen(command) + strlen(target) + 4) + 1);
            rest[strlen(rest) - 1] = 0;
            sprintf(chat_irc_format_buf, XSTR("** CTCP Message from %s (%s)", 239), nick, rest);
            return chat_irc_format_buf;
        }
        sprintf(chat_irc_format_buf, "%s", rest);
        return NULL;
    }

    if (!stricmp(command, "JOIN")) {
        if (!stricmp(irc_nick, nick)) {
            irc_join_result = 1;
            if (!stricmp(irc_cur_channel, "#autoselect")) {
                strcpy(irc_cur_channel, irc_get_word(0, rest));
                irc_queue_command(IRC_CMD_CHANNEL_SET, irc_cur_channel, strlen(irc_cur_channel) + 1);
            }
        }
        irc_add_user(nick);
        strcpy(target, irc_get_word(0, rest));
        irc_add_user(nick);
        sprintf(chat_irc_format_buf, XSTR("** %s has joined %s", 240), nick, target);
        return NULL;
    }

    if (!stricmp(command, "PART")) {
        strcpy(target, irc_get_word(0, rest));
        strcpy(rest, line + offset + strlen(command) + strlen(target) + 3);
        if (!stricmp(irc_nick, nick))
            irc_remove_all_users();
        irc_remove_user(nick);
        return NULL;
    }

    if (!stricmp(command, "KICK")) {
        strcpy(target, irc_get_word(0, rest));
        strcpy(prefix_copy, irc_get_word(1, rest));
        word = irc_get_word(2, rest);       // the reason, in irc_word_buf (overwritten just below)
        if (!stricmp(irc_nick, irc_get_word(1, rest))) {
            irc_cur_channel[0] = 0;
            irc_queue_command(IRC_CMD_KICKED, NULL, 0);
            irc_remove_all_users();
        }
        sprintf(chat_irc_format_buf, XSTR("*** %s has kicked %s from channel %s (%s)", 241), nick,
                prefix_copy, target, word);
        irc_remove_user(prefix_copy);
        return chat_irc_format_buf;
    }

    if (!stricmp(command, "NICK")) {
        if (!stricmp(irc_nick, nick))
            strcpy(irc_nick, irc_get_word(0, rest));
        sprintf(nick_change, "%s %s", nick, irc_get_word(0, rest));
        irc_queue_command(IRC_CMD_NICK_CHANGED, nick_change, strlen(nick_change) + 1);
        irc_remove_user(nick);
        irc_add_user(irc_get_word(0, rest));
        sprintf(chat_irc_format_buf, XSTR("*** %s is now known as %s", 242), nick, irc_get_word(0, rest));
        return chat_irc_format_buf;
    }

    if (!stricmp(command, "PING")) {
        sprintf(chat_irc_format_buf, "/PONG :%s", irc_get_word(0, rest));
        irc_process_input(chat_irc_format_buf, 1);
        return NULL;
    }

    if (!stricmp(command, "MODE"))
        return NULL;

    // ERR_NOSUCHNICK
    if (!stricmp(command, "401")) {
        strcpy(tmp, irc_get_word(1, rest));
        irc_whois_done = 1;
        irc_whois_not_found = 1;
        sprintf(chat_irc_format_buf, XSTR("**Error: %s is not online!", 243), tmp);
        return chat_irc_format_buf;
    }
    // RPL_WHOISUSER
    if (!stricmp(command, "311")) {
        strcpy(tmp, irc_get_word(1, rest));
        strcpy(irc_whois_realname, irc_get_word(5, rest));
        return NULL;
    }
    // RPL_WHOISCHANNELS
    if (!stricmp(command, "319")) {
        strcpy(tmp, irc_get_word(1, rest));
        strcpy(irc_whois_channels, irc_get_word(2, rest));
        return NULL;
    }
    // RPL_ENDOFWHOIS (no return)
    if (!stricmp(command, "318")) {
        if (!irc_whois_channels[0])
            irc_whois_channels[0] = '*';
    }
    // RPL_LISTSTART
    if (!stricmp(command, "321")) {
        irc_flush_channel_list();
        irc_list_state = 1;
        return NULL;
    }
    // RPL_LIST
    if (!stricmp(command, "322")) {
        if (irc_list_state != 1)
            return NULL;
        strcpy(channel_topic, irc_get_word(3, rest));
        strcpy(tmp, irc_get_word(1, rest));
        irc_add_channel(tmp, atoi(irc_get_word(2, rest)), channel_topic);
        return NULL;
    }
    // RPL_LISTEND
    if (!stricmp(command, "323")) {
        irc_list_state = 2;
        return NULL;
    }
    if (!stricmp(command, "324"))
        return NULL;
    // RPL_TOPIC and TOPIC: the comparison with the joined channel is all that is left.
    if (!stricmp(command, "332")) {
        if (!stricmp(irc_cur_channel, target)) {
        }
        return NULL;
    }
    if (!stricmp(command, "TOPIC")) {
        if (!stricmp(irc_cur_channel, target)) {
        }
        return NULL;
    }
    if (!stricmp(command, "QUIT")) {
        irc_remove_user(nick);
        return NULL;
    }
    // RPL_ENDOFMOTD: the welcome is complete.
    if (!stricmp(command, "376")) {
        if (!irc_logged_in)
            irc_logged_in = 1;
        strcpy(chat_irc_format_buf, "!EMOTD$#!");
        return chat_irc_format_buf;
    }
    if (!stricmp(command, "377") || !stricmp(command, "372") || !stricmp(command, "372")) {
        strcpy(chat_irc_format_buf, "!MOTD$#!");
        strcat(chat_irc_format_buf, irc_get_word(3, line));
        return chat_irc_format_buf;
    }
    if (!stricmp(command, "366"))
        return NULL;
    if (!stricmp(command, "333"))
        return NULL;
    if (!stricmp(command, "329"))
        return NULL;
    // RPL_NAMREPLY: the users of the channel, without the @ / + of operators and voiced users.
    if (!stricmp(command, "353")) {
        strcpy(rest, irc_get_word(3, line + offset + strlen(command) + 2));
        tok = strtok(rest, " ");
        if (!tok)
            return NULL;
        do {
            if (*tok == '@' || *tok == '+')
                tok++;
            irc_add_user(tok);
            tok = strtok(NULL, " ");
        } while (tok);
        return NULL;
    }
    if (!stricmp(command, "001"))
        return NULL;
    if (!stricmp(command, "002"))
        return NULL;
    if (!stricmp(command, "003"))
        return NULL;
    if (!stricmp(command, "004"))
        return NULL;
    if (!stricmp(command, "251"))
        return NULL;
    if (!stricmp(command, "254"))
        return NULL;
    if (!stricmp(command, "255"))
        return NULL;
    if (!stricmp(command, "265"))
        return NULL;
    if (!stricmp(command, "375"))
        return NULL;
    if (!stricmp(command, "372"))
        return NULL;
    if (!stricmp(command, "375"))
        return NULL;
    // ERR_ERRONEUSNICKNAME
    if (!stricmp(command, "432")) {
        sprintf(chat_irc_format_buf, XSTR("Your nickname contains invalid characters", 244));
        irc_queue_command(IRC_CMD_DISCONNECTED, NULL, 0);
        return chat_irc_format_buf;
    }
    // ERR_NICKNAMEINUSE: try the asked-for nick with a number.
    if (!stricmp(command, "433")) {
        sprintf(tmp, "%s%d", irc_orig_nick, irc_nick_suffix);
        strcpy(irc_nick, tmp);
        irc_nick_suffix++;
        sprintf(chat_irc_format_buf, "/NICK %s", tmp);
        irc_process_input(chat_irc_format_buf, 1);
        return NULL;
    }
    strcpy(chat_irc_format_buf, line);
    return NULL;
}

// 0x4ad8b0  Confidence: likely (the word loop counts n down; the separator array is 10 bytes, 5 of them
// from the literal). A word starting with ':' is the rest of the line.
char *irc_get_word(int n, char *text)
{
    char seps[10] = " \n\r\t";
    char *tok;

    strcpy(irc_get_word_line, text);
    tok = strtok(irc_get_word_line, seps);
    for (; n; n--)
        tok = strtok(NULL, seps);
    if (!tok)
        return "";
    strcpy(irc_word_buf, tok);
    if (*tok == ':')
        strcpy(irc_word_buf, text + (tok - irc_get_word_line) + 1);
    return irc_word_buf;
}

// 0x4ad9a0  Confidence: exact.
void irc_flush_channel_list()
{
    irc_channel *next;

    irc_channel_cur = irc_channel_list;
    while (irc_channel_cur) {
        next = irc_channel_cur->next;
        free(irc_channel_cur);
        irc_channel_cur = next;
    }
    irc_channel_list = NULL;
}

// 0x4ad9e0  Confidence: likely (the first-record branch stores next twice: its own store, then the common
// one after the if).
void irc_add_channel(char *name, short users, char *topic)
{
    irc_channel_cur = irc_channel_list;
    if (!irc_channel_cur) {
        irc_channel_list = (irc_channel *)malloc(sizeof(irc_channel));
        strcpy(irc_channel_list->name, name);
        strcpy(irc_channel_list->topic, topic);
        irc_channel_list->users = users;
        irc_channel_list->next = NULL;
        irc_channel_cur = irc_channel_list;
    } else {
        while (irc_channel_cur->next)
            irc_channel_cur = irc_channel_cur->next;
        irc_channel_cur->next = (irc_channel *)malloc(sizeof(irc_channel));
        irc_channel_cur = irc_channel_cur->next;
        strcpy(irc_channel_cur->name, name);
        strcpy(irc_channel_cur->topic, topic);
        irc_channel_cur->users = users;
    }
    irc_channel_cur->next = NULL;
}

// 0x4adb40  Confidence: likely (the two branches end in the same next = NULL store, merged).
int irc_add_user(char *nick)
{
    irc_user_cur = irc_user_list;
    while (irc_user_cur) {
        if (!stricmp(nick, irc_user_cur->nick))
            return 0;
        irc_user_cur = irc_user_cur->next;
    }

    irc_user_cur = irc_user_list;
    if (!irc_user_cur) {
        irc_user_list = (irc_user *)malloc(sizeof(irc_user));
        strcpy(irc_user_list->nick, nick);
        irc_user_list->next = NULL;
    } else {
        while (irc_user_cur->next)
            irc_user_cur = irc_user_cur->next;
        irc_user_cur->next = (irc_user *)malloc(sizeof(irc_user));
        irc_user_cur = irc_user_cur->next;
        strcpy(irc_user_cur->nick, nick);
        irc_user_cur->next = NULL;
    }
    irc_queue_command(IRC_CMD_USER_JOINED, nick, strlen(nick) + 1);
    return 1;
}

// 0x4adc40  Confidence: exact.
int irc_remove_user(char *nick)
{
    irc_user *prev = NULL;

    irc_user_cur = irc_user_list;
    while (irc_user_cur) {
        if (!stricmp(nick, irc_user_cur->nick)) {
            if (prev)
                prev->next = irc_user_cur->next;
            else
                irc_user_list = irc_user_cur->next;
            irc_queue_command(IRC_CMD_USER_LEFT, irc_user_cur->nick, strlen(irc_user_cur->nick) + 1);
            free(irc_user_cur);
            return 1;
        }
        prev = irc_user_cur;
        irc_user_cur = irc_user_cur->next;
    }
    return 0;
}

// 0x4adcd0  Confidence: likely (the word and the output share buf; target 0x34 bytes in the frame). Our
// own PRIVMSG lines are echoed through chat_process_irc_message (mode 1) without the CR LF.
char *irc_process_input(char *line, int quiet)
{
    char target[50];
    char buf[IRC_MAX_LINE];

    if (!irc_connected)
        return NULL;

    if (line[0] == '/') {
        strcpy(buf, irc_get_word(0, line + 1));
        if (!stricmp(buf, "msg")) {
            strcpy(target, irc_get_word(1, line + 1));
            sprintf(buf, "PRIVMSG %s :%s\n\r", target, line + strlen("/msg ") + strlen(target) + 1);
            irc_send_raw(buf, strlen(buf));
            buf[strlen(buf) - 2] = 0;
            return chat_process_irc_message(buf, 1);
        }
        if (!stricmp(buf, "me")) {
            sprintf(buf, "PRIVMSG %s :\001ACTION %s\001\n\r", irc_cur_channel, line + strlen("/me "));
            irc_send_raw(buf, strlen(buf));
            buf[strlen(buf) - 2] = 0;
            return chat_process_irc_message(buf, 1);
        }
        if (!stricmp(buf, "xyz")) {
            sprintf(buf, "%s\n\r", line + strlen("/xyz "));
            irc_send_raw(buf, strlen(buf));
            return NULL;
        }
        if (!stricmp(buf, "list")) {
            sprintf(buf, "%s\n\r", line + 1);
            irc_send_raw(buf, strlen(buf));
            return NULL;
        }
        if (quiet) {
            sprintf(buf, "%s\n\r", line + 1);
            irc_send_raw(buf, strlen(buf));
            return NULL;
        }
        return XSTR("Unrecognized command", 245);
    }

    if (irc_cur_channel[0]) {
        sprintf(buf, "PRIVMSG %s :%s\n\r", irc_cur_channel, line);
        irc_send_raw(buf, strlen(buf));
        if (strlen(buf) >= 2) {
            buf[strlen(buf) - 2] = 0;
            return chat_process_irc_message(buf, 1);
        }
    }
    return NULL;
}

// 0x4adfb0  Confidence: exact (tail jump).
irc_command *irc_get_chat_command()
{
    if (!irc_connected)
        return NULL;
    return irc_pop_command();
}

// 0x4adfd0  Confidence: exact.
irc_command *irc_pop_command()
{
    irc_command *next;

    if (!irc_cmd_list)
        return NULL;
    irc_cmd_cur = irc_cmd_list;
    irc_cmd_result = *irc_cmd_cur;
    next = irc_cmd_cur->next;
    free(irc_cmd_cur);
    irc_cmd_list = next;
    return &irc_cmd_result;
}

// 0x4ae010  Confidence: exact. Polled by the lobby until it returns non-zero.
int irc_join_channel(char *name)
{
    char buf[100];

    if (!irc_connected)
        return -1;
    if (irc_joining == 1) {
        if (irc_join_result == 1) {
            irc_joining = 0;
            return 1;
        }
        if (irc_join_result == -1) {
            irc_joining = 0;
            strcpy(irc_cur_channel, "");
            return -1;
        }
        return 0;
    }
    if (irc_cur_channel[0]) {
        sprintf(buf, "/PART %s", irc_cur_channel);
        irc_process_input(buf, 1);
    }
    strcpy(irc_cur_channel, name);
    sprintf(buf, "/JOIN %s", irc_cur_channel);
    irc_process_input(buf, 1);
    irc_joining = 1;
    irc_join_result = 0;
    return 0;
}

// 0x4ae120  Confidence: likely (the length sum's operand order). Once per LIST: the text, then the
// records are freed and the state goes back to 0.
char *irc_get_channel_list()
{
    int len = 0;
    char num[10];

    if (irc_list_state != 2)
        return NULL;
    if (!irc_connected)
        return NULL;
    if (irc_channel_text) {
        free(irc_channel_text);
        irc_channel_text = NULL;
    }

    irc_channel_cur = irc_channel_list;
    while (irc_channel_cur) {
        len += strlen(irc_channel_cur->name) + strlen(irc_channel_cur->topic) + 7;
        irc_channel_cur = irc_channel_cur->next;
    }
    irc_channel_cur = irc_channel_list;
    irc_channel_text = (char *)malloc(len + 1);
    irc_channel_text[0] = 0;
    while (irc_channel_cur) {
        strcat(irc_channel_text, "$");
        strcat(irc_channel_text, irc_channel_cur->name);
        strcat(irc_channel_text, " ");
        sprintf(num, "%d ", irc_channel_cur->users);
        strcat(irc_channel_text, num);
        strcat(irc_channel_text, irc_channel_cur->topic);
        strcat(irc_channel_text, " ");
        irc_channel_cur = irc_channel_cur->next;
    }
    irc_flush_channel_list();
    irc_list_state = 0;
    return irc_channel_text;
}
