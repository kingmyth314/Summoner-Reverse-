// unnamed_http_0x55a8e0.cpp - Sum.exe v1.40, code 0x55a8e0-0x55b431: http_get, the HTTP file getter of
// InetGetFile (version check, PXO chat banners), and its asynchronous host name lookup. The file name is
// a placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp66 NOTES.md): after the FTP getter, before the vfx force code (0x55b440). .data
// 0x5adc80-0x5add23 ("Aborting....\n" .. "http"; the two "Content-Length:" copies show no pooling), .bss
// 0x2f54708-0x2f5471b (the last lookup record, the pending lookup, the "" literal 0x2f54718); the
// DirectSound buffers follow at 0x2f54720.

#include <winsock.h>
#include <process.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "vsdk/inetfile/unnamed_http_0x55a8e0.h"

// ---- .bss -------------------------------------------------------------------------------------------
http_dns_lookup http_last_lookup;                 // 0x2f54708
http_dns_lookup *http_async_lookup;               // 0x2f54714

// 0x55a8e0  Confidence: exact.
void http_obj_thread(void *p)
{
    http_get *h = (http_get *)p;

    h->send_get();
    h->thread_done = true;
}

// 0x55a900  Confidence: exact.
void http_get::abort_get()
{
    OutputDebugString("Aborting....\n");
    aborting = true;
    while (!thread_done)
        Sleep(50);
    OutputDebugString("Aborted....\n");
}

// 0x55a940  Confidence: exact.
http_get::http_get(const char *url, const char *local_file, const char *proxy, unsigned short port)
{
    proxy_host = proxy;
    proxy_port = port;
    use_proxy = true;
    init(url, local_file);
}

// 0x55a980  Confidence: exact.
http_get::http_get(const char *url, const char *local_file)
{
    use_proxy = false;
    init(url, local_file);
}

// 0x55a9a0  Confidence: likely (the slash scan's form). "http://host/path/file"; a ':' (a port) is a parse
// error. On success the state stays STARTUP until the worker connects.
void http_get::init(const char *u, const char *local_file)
{
    unsigned long nonblocking;
    const char *p;
    const char *first;
    const char *last;
    int i;

    sock = INVALID_SOCKET;
    bytes_in = 0;
    total_bytes = 0;
    state = HTTP_STATE_STARTUP;
    aborting = false;
    thread_done = false;
    strncpy(url, u, sizeof(url) - 1);
    url[sizeof(url) - 1] = '\0';
    file = fopen(local_file, "wb");
    if (file == NULL) {
        state = HTTP_STATE_CANT_WRITE_FILE;
        return;
    }
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        state = HTTP_STATE_SOCKET_ERROR;
        return;
    }
    nonblocking = 1;
    ioctlsocket(sock, FIONBIO, &nonblocking);
    p = u;
    if (_strnicmp(p, "http:", 5) == 0) {
        p += 5;
        while (*p == '/')
            p++;
    }
    if (strchr(p, ':') != NULL) {
        state = HTTP_STATE_URL_PARSING_ERROR;
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
        state = HTTP_STATE_URL_PARSING_ERROR;
        return;
    }
    strcpy(path, first);
    strncpy(host, p, first - p);
    host[first - p - 1] = '\0';
    if (_beginthread(http_obj_thread, 0, this) == 0)
        state = HTTP_STATE_INTERNAL_ERROR;
}

// 0x55ab70  Confidence: exact.
http_get::~http_get()
{
    if (sock != INVALID_SOCKET) {
        shutdown(sock, 2);
        closesocket(sock);
    }
}

// 0x55aba0  Confidence: exact.
int http_get::get_status()
{
    return state;
}

// 0x55abb0  Confidence: exact.
int http_get::get_bytes_in()
{
    return bytes_in;
}

// 0x55abc0  Confidence: exact.
int http_get::get_total_bytes()
{
    return total_bytes;
}

// 0x55abd0  Confidence: likely (the header loop's form). Through a proxy the request names the full URL.
void http_get::send_get()
{
    char req[1000];
    char *line;
    char *p;
    int code;

    connect_socket();
    if (aborting) {
        fclose(file);
        return;
    }
    if (state != HTTP_STATE_CONNECTED) {
        fclose(file);
        return;
    }
    sprintf(req, "GET %s%s HTTP/1.1\nAccept: */*\nAccept-Encoding: deflate\nHost: %s\n\n\n",
            use_proxy ? "" : "/", use_proxy ? url : path, host);
    send(sock, req, strlen(req), 0);
    line = get_line();
    if (_strnicmp("HTTP/", line, 5) != 0) {
        state = HTTP_STATE_UNKNOWN_ERROR;
        fclose(file);
        return;
    }
    p = strchr(line, ' ') + 1;
    if (p == NULL) {
        state = HTTP_STATE_UNKNOWN_ERROR;
        fclose(file);
        return;
    }
    p[3] = '\0';
    code = atoi(p);
    if (code == 0) {
        state = HTTP_STATE_UNKNOWN_ERROR;
        fclose(file);
        return;
    }
    if (code != 200) {
        state = HTTP_STATE_FILE_NOT_FOUND;
        fclose(file);
        return;
    }
    line = get_line();
    while (line != NULL) {
        if (line[0] == '\0') {
            read_data_channel();
            return;
        }
        if (_strnicmp(line, "Content-Length:", strlen("Content-Length:")) == 0) {
            p = strchr(line, ' ') + 1;
            if (p != NULL) {
                for (line = p; *line != '\0'; line++) {
                    if (!isdigit(*line))
                        *line = '\0';
                }
                total_bytes = atoi(p);
            }
        }
        Sleep(1);
        line = get_line();
    }
    state = HTTP_STATE_UNKNOWN_ERROR;
    fclose(file);
}

// 0x55ae20  Confidence: likely (the lookup and connect loops' forms). A failed lookup is never seen:
// http_resolve_host marks its own record, POLL reads the global copy (http_async_gethostbyname), so an
// unknown host keeps this thread polling until the getter is aborted (game bug, NOTES.md).
int http_get::connect_socket()
{
    sockaddr_in addr;
    fd_set fds;
    timeval tv;
    unsigned int ip;
    servent *se;
    int r;
    int err;

    if (aborting)
        return 0;
    ip = inet_addr(host);
    if (ip == INADDR_NONE) {
        http_async_gethostbyname(&ip, HTTP_DNS_START, host);
        do {
            if (aborting) {
                http_async_gethostbyname(&ip, HTTP_DNS_CANCEL, host);
                return 0;
            }
            r = http_async_gethostbyname(&ip, HTTP_DNS_POLL, host);
            Sleep(1);
        } while (r == 0);
        if (r == -1) {
            state = HTTP_STATE_HOST_NOT_FOUND;
            return 0;
        }
    }
    if (aborting)
        return 0;
    se = getservbyname("http", NULL);
    if (aborting)
        return 0;
    if (se == NULL)
        addr.sin_port = htons(80);
    else
        addr.sin_port = se->s_port;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;
    if (use_proxy) {
        ip = inet_addr(proxy_host);
        if (ip == INADDR_NONE) {
            http_async_gethostbyname(&ip, HTTP_DNS_START, proxy_host);
            do {
                if (aborting) {
                    http_async_gethostbyname(&ip, HTTP_DNS_CANCEL, proxy_host);
                    return 0;
                }
                r = http_async_gethostbyname(&ip, HTTP_DNS_POLL, proxy_host);
                Sleep(1);
            } while (r == 0);
            if (r == -1) {
                state = HTTP_STATE_HOST_NOT_FOUND;
                return 0;
            }
        }
        addr.sin_port = htons(proxy_port != 0 ? proxy_port : 80);
        addr.sin_addr.s_addr = ip;
    }
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    r = connect(sock, (sockaddr *)&addr, sizeof(addr));
    err = WSAGetLastError();
    if (r != 0) {
        while (err == WSAEALREADY || err == WSAEINVAL || err == WSAEWOULDBLOCK) {
            fds.fd_count = 1;
            fds.fd_array[0] = sock;
            if (select(0, NULL, &fds, NULL, &tv) != 0)
                goto connected;
            if (aborting)
                return 0;
            r = connect(sock, (sockaddr *)&addr, sizeof(addr));
            if (r == 0)
                goto connected;
            err = WSAGetLastError();
            if (err == WSAEISCONN)
                goto connected;
            Sleep(1);
        }
        if (r != 0) {
            state = HTTP_STATE_CANT_CONNECT;
            return 0;
        }
    }
connected:
    state = HTTP_STATE_CONNECTED;
    return 1;
}

// 0x55b0d0  Confidence: likely. A line ends at '\r' (the byte after it is read and dropped); a lone '\n'
// stays in the line.
char *http_get::get_line()
{
    char ch[2];
    int done = 0;

    memset(recv_buffer, 0, sizeof(recv_buffer));
    do {
        ch[0] = '\0';
        while (recv(sock, ch, 1, 0) == SOCKET_ERROR) {
            if (WSAGetLastError() != WSAEWOULDBLOCK)
                return NULL;
        }
        Sleep(1);
        if (ch[0] == '\r') {
            while (recv(sock, ch, 1, 0) == SOCKET_ERROR) {
                if (WSAGetLastError() != WSAEWOULDBLOCK)
                    return NULL;
            }
            Sleep(1);
            done = 1;
        } else {
            ch[1] = '\0';
            strcat(recv_buffer, ch);
        }
        Sleep(1);
    } while (!done);
    return recv_buffer;
}

// 0x55b1c0  Confidence: likely (the would-block retry jumps back to the loop head with n = 1).
int http_get::read_data_channel()
{
    char buf[4096];
    fd_set fds;
    timeval tv;
    int n = 0;

    tv.tv_sec = 0;
    tv.tv_usec = 500;
    state = HTTP_STATE_RECEIVING;
    do {
        fds.fd_count = 1;
        fds.fd_array[0] = sock;
        if (total_bytes != 0 && bytes_in == total_bytes)
            break;
        select(0, &fds, NULL, NULL, &tv);
        if (aborting) {
            fclose(file);
            return 0;
        }
        n = recv(sock, buf, sizeof(buf), 0);
        if (aborting) {
            fclose(file);
            return 0;
        }
        if (n == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {
            n = 1;
            continue;
        }
        bytes_in += n;
        if (n > 0)
            fwrite(buf, n, 1, file);
        Sleep(1);
    } while (n > 0);
    fclose(file);
    if (n == SOCKET_ERROR) {
        state = HTTP_STATE_RECV_FAILED;
        return 0;
    }
    state = HTTP_STATE_FILE_RECEIVED;
    return 1;
}

// 0x55b310  Confidence: likely (CANCEL falls out to the -2 of an unknown command).
int http_async_gethostbyname(unsigned int *ip, int command, const char *host)
{
    http_dns_lookup *l;

    if (command == HTTP_DNS_START) {
        if (http_async_lookup != NULL)
            http_async_lookup->abort = true;
        l = (http_dns_lookup *)malloc(sizeof(http_dns_lookup));
        l->ip = 0;
        l->host = host;
        l->done = false;
        l->error = false;
        l->abort = false;
        http_async_lookup = l;
        http_last_lookup.done = false;
        _beginthread(http_resolve_host, 0, l);
        return 1;
    } else if (command == HTTP_DNS_CANCEL) {
        if (http_async_lookup != NULL)
            http_async_lookup->abort = true;
        http_async_lookup = NULL;
    } else if (command == HTTP_DNS_POLL) {
        if (http_async_lookup == NULL)
            return -1;
        if (http_last_lookup.done) {
            http_async_lookup = NULL;
            *ip = http_last_lookup.ip;
            return 1;
        }
        if (http_last_lookup.error) {
            free(http_async_lookup);
            http_async_lookup = NULL;
            return -1;
        }
        return 0;
    }
    return -2;
}

// 0x55b3e0  Confidence: exact. A failed lookup marks the record and leaks it.
void http_resolve_host(void *p)
{
    http_dns_lookup *l = (http_dns_lookup *)p;
    hostent *he;

    he = gethostbyname(l->host);
    if (he == NULL) {
        l->error = true;
        return;
    }
    if (!l->abort) {
        l->done = true;
        l->ip = *(unsigned int *)he->h_addr_list[0];
        http_last_lookup = *l;
    }
    free(l);
}
