// unnamed_ftp_0x559ea0.cpp - Sum.exe v1.40, code 0x559ea0-0x55a8d5: ftp_get, the FTP file getter of
// InetGetFile. The file name is a placeholder: no path string, no strings.tbl block. The class and its
// HTTP sibling have the structure of the FreeSpace 2 inetfile code (CFtpGet / ChttpGet); that is a lead
// for the names only.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp66 NOTES.md): after lfsr_cipher (0x559e10), before the HTTP getter (0x55a8e0). .data
// 0x5adbfc-0x5adc7f ("wb" .. "PASS %s\r\n"), no .bss. The HTTP getter's .data starts at 0x5adc80 with its
// own "wb" (0x5adca0); whether the two are one file cannot be told from the layout (no pooling in either),
// so they are kept as two, like their FreeSpace 2 counterparts.

#include <winsock.h>
#include <process.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vsdk/inetfile/unnamed_ftp_0x559ea0.h"

// 0x559ea0  Confidence: exact (tail jump).
void ftp_worker_thread_entry(void *p)
{
    ((ftp_get *)p)->worker_thread();
}

// 0x559eb0  Confidence: exact. The wait reads done once (not volatile): with the worker still running,
// or after a failed connect / login (which never set done), it spins forever (game bug, NOTES.md).
void ftp_get::abort_get()
{
    aborting = true;
    while (!done)
        ;
    fclose(file);
}

// 0x559ed0  Confidence: likely (the slash scan's form). The URL is "ftp://host/dir/file"; a ':' anywhere
// (a port, "user:pass@") is a parse error. _beginthread's -1 is not caught (state 3 either way).
ftp_get::ftp_get(const char *url, const char *local_file, const char *u, const char *pw)
{
    sockaddr_in addr;
    const char *p;
    const char *first;
    const char *last;
    int i;

    state = FTP_STATE_STARTUP;
    listen_socket = INVALID_SOCKET;
    data_socket = INVALID_SOCKET;
    control_socket = INVALID_SOCKET;
    bytes_in = 0;
    total_bytes = 0;
    aborting = false;
    done = false;
    file = fopen(local_file, "wb");
    if (file == NULL) {
        state = FTP_STATE_CANT_WRITE_FILE;
        return;
    }
    if (u == NULL)
        u = "anonymous";
    strcpy(user, u);
    if (pw == NULL)
        pw = "pxouser@pxo.net";
    strcpy(password, pw);
    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == INVALID_SOCKET) {
        state = FTP_STATE_SOCKET_ERROR;
        return;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listen_socket, (sockaddr *)&addr, sizeof(addr)) != 0) {
        state = FTP_STATE_SOCKET_ERROR;
        return;
    }
    if (listen(listen_socket, 1) != 0) {
        state = FTP_STATE_SOCKET_ERROR;
        return;
    }
    control_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (control_socket == INVALID_SOCKET) {
        state = FTP_STATE_SOCKET_ERROR;
        return;
    }
    p = url;
    if (_strnicmp(p, "ftp:", 4) == 0) {
        p += 4;
        while (*p == '/')
            p++;
    }
    if (strchr(p, ':') != NULL) {
        state = FTP_STATE_URL_PARSING_ERROR;
        return;
    }
    first = NULL;
    last = NULL;
    for (i = strlen(p); i >= 0; i--) {
        if (p[i] == '/') {
            if (last == NULL) {
                last = p + i + 1;
                strcpy(filename, last);
                first = last;
            } else {
                first = p + i + 1;
            }
        }
    }
    if (first == NULL || last == NULL) {
        state = FTP_STATE_URL_PARSING_ERROR;
        return;
    }
    strncpy(dir, first, last - first);
    dir[last - first] = '\0';
    strncpy(host, p, first - p);
    host[first - p - 1] = '\0';
    state = _beginthread(ftp_worker_thread_entry, 0, this) != 0 ? FTP_STATE_CONNECTING : FTP_STATE_INTERNAL_ERROR;
}

// 0x55a150  Confidence: exact.
ftp_get::~ftp_get()
{
    if (listen_socket != INVALID_SOCKET) {
        shutdown(listen_socket, 2);
        closesocket(listen_socket);
    }
    if (data_socket != INVALID_SOCKET) {
        shutdown(data_socket, 2);
        closesocket(data_socket);
    }
    if (control_socket != INVALID_SOCKET) {
        shutdown(control_socket, 2);
        closesocket(control_socket);
    }
}

// 0x55a1c0  Confidence: exact.
int ftp_get::get_status()
{
    return state;
}

// 0x55a1d0  Confidence: exact.
int ftp_get::get_bytes_in()
{
    return bytes_in;
}

// 0x55a1e0  Confidence: exact.
int ftp_get::get_total_bytes()
{
    return total_bytes;
}

// 0x55a1f0  Confidence: exact.
void ftp_get::worker_thread()
{
    connect_control_socket();
    if (state == FTP_STATE_LOGGING_IN) {
        login_host();
        if (state == FTP_STATE_LOGGED_IN) {
            get_file();
            done = true;
        }
    }
}

// 0x55a220  Confidence: likely. The size parse tests strchr's result after adding 1 (never NULL), so a RETR
// reply without "(" makes it read and write at address 1 (a crash; NOTES.md).
int ftp_get::get_file()
{
    char cmd[200];
    char *p;

    sprintf(cmd, "TYPE I\r\n");
    if (send_command(cmd) >= 400) {
        state = FTP_STATE_UNKNOWN_ERROR;
        return 0;
    }
    if (aborting)
        return 0;
    if (dir[0] != '\0') {
        sprintf(cmd, "CWD %s\r\n", dir);
        if (send_command(cmd) >= 400) {
            state = FTP_STATE_DIRECTORY_INVALID;
            return 0;
        }
    }
    if (aborting)
        return 0;
    if (!send_port()) {
        state = FTP_STATE_UNKNOWN_ERROR;
        return 0;
    }
    if (aborting)
        return 0;
    sprintf(cmd, "RETR %s\r\n", filename);
    if (send_command(cmd) >= 400) {
        state = FTP_STATE_FILE_NOT_FOUND;
        return 0;
    }
    if (aborting)
        return 0;
    p = strchr(recv_buffer, '(');
    p++;
    if (p != NULL) {
        *strchr(p, ' ') = '\0';
        total_bytes = atoi(p);
    }
    if (aborting)
        return 0;
    data_socket = accept(listen_socket, NULL, NULL);
    closesocket(listen_socket);
    if (data_socket == INVALID_SOCKET) {
        state = FTP_STATE_SOCKET_ERROR;
        return 0;
    }
    if (aborting)
        return 0;
    read_data_channel();
    state = FTP_STATE_FILE_RECEIVED;
    return 1;
}

// 0x55a3f0  Confidence: likely (the byte order of the PORT arguments: the address bytes, then the port
// word's low and high byte).
int ftp_get::send_port()
{
    char cmd[200];
    sockaddr_in addr;
    int len;
    unsigned int port;

    len = sizeof(addr);
    if (getsockname(listen_socket, (sockaddr *)&addr, &len) == SOCKET_ERROR) {
        state = FTP_STATE_SOCKET_ERROR;
        return 0;
    }
    port = addr.sin_port;
    if (getsockname(control_socket, (sockaddr *)&addr, &len) == SOCKET_ERROR) {
        state = FTP_STATE_SOCKET_ERROR;
        return 0;
    }
    sprintf(cmd, "PORT %d,%d,%d,%d,%d,%d\r\n", addr.sin_addr.S_un.S_un_b.s_b1, addr.sin_addr.S_un.S_un_b.s_b2,
            addr.sin_addr.S_un.S_un_b.s_b3, addr.sin_addr.S_un.S_un_b.s_b4, port & 0xff, port >> 8);
    if (send_command(cmd) != 200) {
        state = FTP_STATE_SOCKET_ERROR;
        return 0;
    }
    return 1;
}

// 0x55a4f0  Confidence: exact.
int ftp_get::connect_control_socket()
{
    hostent *he;
    servent *se;
    sockaddr_in addr;

    he = gethostbyname(host);
    if (he == NULL) {
        state = FTP_STATE_HOST_NOT_FOUND;
        return 0;
    }
    if (aborting)
        return 0;
    se = getservbyname("ftp", NULL);
    if (se == NULL)
        addr.sin_port = htons(21);
    else
        addr.sin_port = se->s_port;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = *(unsigned int *)he->h_addr_list[0];
    if (aborting)
        return 0;
    if (connect(control_socket, (sockaddr *)&addr, sizeof(addr)) != 0) {
        state = FTP_STATE_CANT_CONNECT;
        return 0;
    }
    state = FTP_STATE_LOGGING_IN;
    return 1;
}

// 0x55a5b0  Confidence: exact.
int ftp_get::login_host()
{
    char cmd[200];

    sprintf(cmd, "USER %s\r\n", user);
    if (send_command(cmd) >= 400) {
        state = FTP_STATE_LOGIN_ERROR;
        return 0;
    }
    sprintf(cmd, "PASS %s\r\n", password);
    if (send_command(cmd) >= 400) {
        state = FTP_STATE_LOGIN_ERROR;
        return 0;
    }
    state = FTP_STATE_LOGGED_IN;
    return 1;
}

// 0x55a650  Confidence: exact.
int ftp_get::send_command(const char *cmd)
{
    read_control_channel();
    if (send(control_socket, cmd, strlen(cmd), 0) == SOCKET_ERROR)
        return 999;
    return read_server_reply();
}

// 0x55a6a0  Confidence: exact (memcpy of the three digits; the code buffer is 5 to 8 bytes by the frame, 8 taken). Restarts on a "nnn-" continuation line.
int ftp_get::read_server_reply()
{
    char ch[2];
    char code[8];
    int got_line;

    for (;;) {
        got_line = 0;
        memset(recv_buffer, 0, sizeof(recv_buffer));
        do {
            ch[0] = '\0';
            if (recv(control_socket, ch, 1, 0) == SOCKET_ERROR)
                return 999;
            if (ch[0] == '\n' || ch[0] == '\r') {
                if (recv_buffer[0] != '\0')
                    got_line = 1;
            } else {
                ch[1] = '\0';
                strcat(recv_buffer, ch);
            }
            Sleep(1);
        } while (!got_line);
        if (recv_buffer[3] != '-')
            break;
    }
    if (recv_buffer[3] != ' ')
        return 999;
    memcpy(code, recv_buffer, 3);
    code[3] = '\0';
    return atoi(code);
}

// 0x55a780  Confidence: exact (a do / while (true) loop). bytes_in also takes the -1 of a failed recv.
int ftp_get::read_data_channel()
{
    char buf[4096];
    int n;

    state = FTP_STATE_RECEIVING;
    if (aborting)
        return 0;
    do {
        n = recv(data_socket, buf, sizeof(buf), 0);
        bytes_in += n;
        if (n > 0)
            fwrite(buf, n, 1, file);
        Sleep(1);
        if (n <= 0)
            break;
        if (aborting)
            return 0;
    } while (true);
    fclose(file);
    if (n == SOCKET_ERROR) {
        state = FTP_STATE_RECV_FAILED;
        return 0;
    }
    state = FTP_STATE_FILE_RECEIVED;
    return 1;
}

// 0x55a850  Confidence: exact (the fd_set is filled by hand: the control socket, then count 1).
void ftp_get::read_control_channel()
{
    fd_set fds;
    timeval tv;
    char c[4];

    tv.tv_sec = 0;
    tv.tv_usec = 0;
    fds.fd_array[0] = control_socket;
    fds.fd_count = 1;
    while (select(0, &fds, NULL, NULL, &tv) != 0) {
        recv(control_socket, c, 1, 0);
        Sleep(1);
    }
}
