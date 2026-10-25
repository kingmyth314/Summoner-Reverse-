// vsdk/inetfile/unnamed_ftp_0x559ea0.h - the FTP file getter InetGetFile uses for "ftp:" URLs (0x559ea0-
// 0x55a8d5; source at src/decomp/vsdk/unnamed_ftp_0x559ea0.cpp, decomp66; file and directory names
// placeholders). Class name from sum.symbols (ftp_get__scalar_deleting_dtor); the members carry the
// sum.symbols ftp_* names. The state numbers are the ones InetGetFile tests (3 connecting, 0xb
// receiving, 0xc received). Not original source.
#ifndef VSDK_INETFILE_UNNAMED_FTP_0X559EA0_H
#define VSDK_INETFILE_UNNAMED_FTP_0X559EA0_H

#include <stdio.h>
#include "vsdk/vsdk.h"

// ftp_get::state (names ours, after what sets them).
#define FTP_STATE_INTERNAL_ERROR     0
#define FTP_STATE_SOCKET_ERROR       1
#define FTP_STATE_URL_PARSING_ERROR  2
#define FTP_STATE_CONNECTING         3
#define FTP_STATE_HOST_NOT_FOUND     4
#define FTP_STATE_CANT_CONNECT       5
#define FTP_STATE_LOGGING_IN         6
#define FTP_STATE_LOGIN_ERROR        7
#define FTP_STATE_LOGGED_IN          8
#define FTP_STATE_DIRECTORY_INVALID  9
#define FTP_STATE_FILE_NOT_FOUND     10
#define FTP_STATE_RECEIVING          11
#define FTP_STATE_FILE_RECEIVED      12
#define FTP_STATE_UNKNOWN_ERROR      13
#define FTP_STATE_RECV_FAILED        14
#define FTP_STATE_CANT_WRITE_FILE    15
#define FTP_STATE_STARTUP            16

// 0x6c4 bytes (InetGetFile: operator new(0x6c4)). Active-mode transfer: a listening socket for the data
// connection, PORT, RETR. A worker thread does the transfer.
class ftp_get {
public:
    // (url, local file, user, password): NULL user / password = "anonymous" / "pxouser@pxo.net".
    ftp_get(const char *url, const char *local_file, const char *user, const char *password);   // 0x559ed0 ftp_get_create
    ~ftp_get();                                   // 0x55a150 ftp_get_destroy (the sockets; not the file)
    void abort_get();                             // 0x559eb0 ftp_abort_get
    int get_status();                             // 0x55a1c0 ftp_get_status
    int get_bytes_in();                           // 0x55a1d0 ftp_get_bytes_in
    int get_total_bytes();                        // 0x55a1e0 ftp_get_total_bytes
    void worker_thread();                         // 0x55a1f0 ftp_worker_thread
    int get_file();                               // 0x55a220 ftp_get_file
    int send_port();                              // 0x55a3f0 ftp_send_port
    int connect_control_socket();                 // 0x55a4f0 ftp_connect_control_socket
    int login_host();                             // 0x55a5b0 ftp_login_host
    int send_command(const char *cmd);            // 0x55a650 ftp_send_command: the reply code, 999 on error
    int read_server_reply();                      // 0x55a6a0 ftp_read_server_reply
    int read_data_channel();                      // 0x55a780 ftp_read_data_channel
    void read_control_channel();                  // 0x55a850 ftp_read_control_channel (drains it)

    int bytes_in;                                 // +0x000
    int total_bytes;                              // +0x004 from the RETR reply's "(n bytes)"
    int state;                                    // +0x008 FTP_STATE_*
    bool aborting;                                // +0x00c
    bool done;                                    // +0x00d the worker finished the transfer
    char user[100];                               // +0x00e
    char password[100];                           // +0x072
    char host[200];                               // +0x0d6
    char dir[200];                                // +0x19e
    char filename[100];                           // +0x266
    char recv_buffer[1000];                       // +0x2ca
    ubyte _pad6b2[2];
    unsigned int listen_socket;                   // +0x6b4 SOCKET
    unsigned int data_socket;                     // +0x6b8
    unsigned int control_socket;                  // +0x6bc
    FILE *file;                                   // +0x6c0
};
LAYOUT_CHECK_32(ftp_get, sizeof(ftp_get) == 0x6c4);

void ftp_worker_thread_entry(void *p);            // 0x559ea0 the _beginthread entry

#endif
