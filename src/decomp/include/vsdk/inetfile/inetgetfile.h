// vsdk/inetfile/inetgetfile.h - the HTTP / FTP file getter (InetGetFile__*, 0x51b760-0x51bb6b, between
// character_instance.cpp and effect_mem.cpp; class and member names as in sum.symbols, which follow the
// FreeSpace 2 inetfile code; directory and file name guessed). Not original source.
#ifndef VSDK_INETFILE_INETGETFILE_H
#define VSDK_INETFILE_INETGETFILE_H

#include "vsdk/vsdk.h"

class ftp_get;
class http_get;

// InetGetFile::error and GetErrorCode's results (values from the code; names ours, decomp74).
#define INET_ERROR_NO_ERROR          0
#define INET_ERROR_BADPARMS          1   // a NULL URL or file name
#define INET_ERROR_CANT_WRITE_FILE   2
#define INET_ERROR_CANT_PARSE_URL    3   // neither "http:" nor "ftp:"
#define INET_ERROR_BAD_FILE_OR_DIR   4
#define INET_ERROR_HOST_NOT_FOUND    5
#define INET_ERROR_UNKNOWN_ERROR     6
#define INET_ERROR_NO_MEMORY         7   // the getter's new failed

// 0x18 bytes (operator new(0x18) in pxo_load_chat_banners). Non-virtual destructor: "delete p" calls
// InetGetFile__dtor and then operator delete.
class InetGetFile {
public:
    InetGetFile(const char *url, const char *local_file);   // 0x51b780 "http:" / "ftp:" getter
    ~InetGetFile();                                         // 0x51b960
    void AbortGet();                                        // 0x51b760
    int  IsConnecting();                                    // 0x51b980 getter state 3
    int  IsReceiving();                                     // 0x51b9b0 getter state 0xb
    int  IsFileReceived();                                  // 0x51b9e0 getter state 0xc
    int  IsFileError();                                     // 0x51ba10
    // decomp74: the function at 0x51ba70 (no sum.symbols entry; name ours, FreeSpace 2 style).
    int  GetErrorCode();                                    // 0x51ba70 INET_ERROR_*
    int  GetTotalBytes();                                   // 0x51baf0
    int  GetBytesIn();                                      // 0x51bb10

    // decomp74: typed and renamed (were void *ftp_get / *http_get, which hid the class names; D5611).
    ftp_get *ftp;                     // +0x00
    http_get *http;                   // +0x04
    int  use_http;                    // +0x08
    int  _unk0c;                      // +0x0c
    int  _unk10;                      // +0x10
    int  error;                       // +0x14 INET_ERROR_*, set by the constructor
};
LAYOUT_CHECK_32(InetGetFile, sizeof(InetGetFile) == 0x18);

#endif
