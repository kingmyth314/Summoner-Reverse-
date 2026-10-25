// vsdk/inetfile/unnamed_http_0x55a8e0.h - the HTTP file getter InetGetFile uses for "http:" URLs (0x55a8e0-
// 0x55b431; source at src/decomp/vsdk/unnamed_http_0x55a8e0.cpp, decomp66; file and directory names
// placeholders). Class name from sum.symbols (http_get__scalar_deleting_dtor); the members carry the
// sum.symbols http_* names. Not original source.
#ifndef VSDK_INETFILE_UNNAMED_HTTP_0X55A8E0_H
#define VSDK_INETFILE_UNNAMED_HTTP_0X55A8E0_H

#include <stdio.h>
#include "vsdk/vsdk.h"

// http_get::state: the FTP numbers (unnamed_ftp_0x559ea0.h) are shared; 6 = connected.
#define HTTP_STATE_INTERNAL_ERROR    0
#define HTTP_STATE_SOCKET_ERROR      1
#define HTTP_STATE_URL_PARSING_ERROR 2
#define HTTP_STATE_CONNECTING        3
#define HTTP_STATE_HOST_NOT_FOUND    4
#define HTTP_STATE_CANT_CONNECT      5
#define HTTP_STATE_CONNECTED         6
#define HTTP_STATE_FILE_NOT_FOUND    10
#define HTTP_STATE_RECEIVING         11
#define HTTP_STATE_FILE_RECEIVED     12
#define HTTP_STATE_UNKNOWN_ERROR     13
#define HTTP_STATE_RECV_FAILED       14
#define HTTP_STATE_CANT_WRITE_FILE   15
#define HTTP_STATE_STARTUP           16

// 0x7f4 bytes (InetGetFile: operator new(0x7f4)). A non-blocking socket; a worker thread sends the GET and
// receives the file.
class http_get {
public:
    http_get(const char *url, const char *local_file);   // 0x55a980 http_get_create
    // (url, local file, proxy host, proxy port 0 = 80): the request carries the full URL.
    http_get(const char *url, const char *local_file, const char *proxy, unsigned short port);   // 0x55a940 http_get_create_proxy
    ~http_get();                                  // 0x55ab70 http_get_destroy (the socket; not the file)
    void init(const char *url, const char *local_file);   // 0x55a9a0 http_get_init
    void abort_get();                             // 0x55a900 http_abort_get (waits for the thread)
    int get_status();                             // 0x55aba0 http_get_status
    int get_bytes_in();                           // 0x55abb0 http_get_bytes_in
    int get_total_bytes();                        // 0x55abc0 http_get_total_bytes
    void send_get();                              // 0x55abd0 http_send_get (the worker's body)
    int connect_socket();                         // 0x55ae20 http_connect_socket
    char *get_line();                             // 0x55b0d0 http_get_line: NULL on a socket error
    int read_data_channel();                      // 0x55b1c0 http_read_data_channel

    bool thread_done;                             // +0x000 set when the worker returns
    ubyte _pad001[3];
    int bytes_in;                                 // +0x004
    int total_bytes;                              // +0x008 Content-Length
    int state;                                    // +0x00c HTTP_STATE_*
    bool use_proxy;                               // +0x010
    ubyte _pad011[3];
    const char *proxy_host;                       // +0x014
    char url[300];                                // +0x018
    unsigned short proxy_port;                    // +0x144
    ubyte _unk146[0xc8];                          // +0x146 not used
    char host[200];                               // +0x20e
    char path[200];                               // +0x2d6
    char filename[100];                           // +0x39e
    bool aborting;                                // +0x402
    ubyte _pad403;
    net_socket sock;                              // +0x404 SOCKET
    FILE *file;                                   // +0x408
    char recv_buffer[1000];                       // +0x40c
};
LAYOUT_CHECK_32(http_get, sizeof(http_get) == 0x7f4);

// The host name lookup thread (records of 12 bytes, malloc / free).
struct http_dns_lookup {
    unsigned int ip;                              // +0x0
    const char *host;                             // +0x4
    bool done;                                    // +0x8
    bool error;                                   // +0x9
    bool abort;                                   // +0xa
};
LAYOUT_CHECK_32(http_dns_lookup, sizeof(http_dns_lookup) == 0xc);
#define HTTP_DNS_CANCEL 1
#define HTTP_DNS_START  2
#define HTTP_DNS_POLL   3
extern http_dns_lookup http_last_lookup;          // 0x2f54708 (sum.symbols http_host_addr = its ip)
extern http_dns_lookup *http_async_lookup;        // 0x2f54714
void http_obj_thread(void *p);                    // 0x55a8e0 the _beginthread entry
// (&ip, command, host): START 1, CANCEL -2, POLL 1 done / 0 pending / -1 failed or no lookup.
int http_async_gethostbyname(unsigned int *ip, int command, const char *host);   // 0x55b310
void http_resolve_host(void *p);                  // 0x55b3e0 the lookup thread

#endif
