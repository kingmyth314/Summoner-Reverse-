// unnamed_inetgetfile_0x51b760.cpp - Sum.exe v1.40, code 0x51b760-0x51bb6b: InetGetFile, the HTTP / FTP file
// getter (module "-" in sum.symbols). Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0
// (cl 12.00.8966), /O2 /GX. Confidence per function: exact = expect identical code, likely = same semantics,
// small layout risk, unsure = the source shape is a guess.
//
// A unit of its own (D5611): it starts after character_instance.cpp's last COMDAT (imax 0x51b750) and ends
// with its own two COMDATs, the scalar deleting destructors of http_get and ftp_get (0x51bb30, 0x51bb50),
// before effect_mem.cpp (0x51bb70). Its literals "\\", "http:", "ftp:" (0x5a866c-0x5a867c) follow
// character_instance.cpp's .data. File name not known (FreeSpace 2 calls its counterpart inetgetfile.cpp,
// but this one is not proven), so a placeholder; the header is vsdk/inetfile/inetgetfile.h. Callers: the
// version check (new_int_multi_update.cpp) and the PXO chat banners (new_pxochat.cpp), multiplayer only.

#include <string.h>
#include <direct.h>
#include <windows.h>
#include "vsdk/inetfile/inetgetfile.h"
#include "vsdk/inetfile/unnamed_ftp_0x559ea0.h"
#include "vsdk/inetfile/unnamed_http_0x55a8e0.h"
#include "summoner/multi/multisave.h"

// 0x51b760  Confidence: exact (two tail calls).
void InetGetFile::AbortGet()
{
    if (use_http)
        http->abort_get();
    else
        ftp->abort_get();
}

// 0x51b780  Confidence: exact (VC6 8966 byte match, m-1020: dir is 256 bytes, D11904) (EH states 0-2 for the
// three news). Makes the local file's directory, then
// starts the getter for the URL's scheme ("http:" through the proxy when one is set), and gives the
// connection a second. A NULL URL or file name sets INET_ERROR_BADPARMS but is still passed to strstr
// (latent: no caller passes NULL).
InetGetFile::InetGetFile(const char *url, const char *local_file)
{
    char dir[256];       // 0x100 in the frame (sub esp, 0x104 with the new temporary), not MAX_PATH
    char *p;

    error = INET_ERROR_NO_ERROR;
    http = NULL;
    ftp = NULL;
    if (url == NULL || local_file == NULL)
        error = INET_ERROR_BADPARMS;
    if (strstr(local_file, "\\")) {
        strcpy(dir, local_file);
        p = dir + strlen(local_file);
        while (*p != '\\')
            p--;
        *p = 0;
        _mkdir(dir);
    }
    if (strstr(url, "http:")) {
        use_http = 1;
        if (strlen(inet_proxy_server))
            http = new http_get(url, local_file, inet_proxy_server, inet_proxy_port);
        else
            http = new http_get(url, local_file);
        if (http == NULL)
            error = INET_ERROR_NO_MEMORY;
    } else if (strstr(url, "ftp:")) {
        use_http = 0;
        ftp = new ftp_get(url, local_file, NULL, NULL);
        if (ftp == NULL)
            error = INET_ERROR_NO_MEMORY;
    } else {
        error = INET_ERROR_CANT_PARSE_URL;
    }
    Sleep(1000);
}

// 0x51b960  Confidence: exact (unit /Ob0, D8402; the original calls the scalar deleting destructors 0x51bb30 / 0x51bb50 with
// 1; VC6 here expands each delete into the destructor and operator delete, D5611). Neither getter deletes
// the downloaded file.
InetGetFile::~InetGetFile()
{
    if (http)
        delete http;
    if (ftp)
        delete ftp;
}

// 0x51b980  Confidence: exact (VC6 byte match with the state local).
int InetGetFile::IsConnecting()
{
    int state;

    if (use_http)
        state = http->get_status();
    else
        state = ftp->get_status();
    return state == FTP_STATE_CONNECTING;
}

// 0x51b9b0  Confidence: exact.
int InetGetFile::IsReceiving()
{
    int state;

    if (use_http)
        state = http->get_status();
    else
        state = ftp->get_status();
    return state == FTP_STATE_RECEIVING;
}

// 0x51b9e0  Confidence: exact.
int InetGetFile::IsFileReceived()
{
    int state;

    if (use_http)
        state = http->get_status();
    else
        state = ftp->get_status();
    return state == FTP_STATE_FILE_RECEIVED;
}

// 0x51ba10  Confidence: exact (VC6 byte match; the switch: byte-indexed table 0x51ba58 with three
// targets, so CONNECTING is a case of its own next to the default, both returning 0). True for the error states.
int InetGetFile::IsFileError()
{
    int state;

    if (error)
        return 1;
    if (use_http)
        state = http->get_status();
    else
        state = ftp->get_status();
    switch (state) {
    case FTP_STATE_INTERNAL_ERROR:
    case FTP_STATE_SOCKET_ERROR:
    case FTP_STATE_URL_PARSING_ERROR:
    case FTP_STATE_HOST_NOT_FOUND:
    case FTP_STATE_CANT_CONNECT:
    case FTP_STATE_LOGIN_ERROR:
    case FTP_STATE_DIRECTORY_INVALID:
    case FTP_STATE_FILE_NOT_FOUND:
    case FTP_STATE_UNKNOWN_ERROR:
    case FTP_STATE_RECV_FAILED:
    case FTP_STATE_CANT_WRITE_FILE:
        return 1;
    case FTP_STATE_CONNECTING:
        return 0;
    default:
        return 0;
    }
}

// 0x51ba70  Confidence: exact (VC6 byte match; no sum.symbols entry, no caller found; name ours). The
// constructor's error, else the getter's state as an INET_ERROR_* code (table 0x51badc).
int InetGetFile::GetErrorCode()
{
    int state;

    if (error)
        return error;
    if (use_http)
        state = http->get_status();
    else
        state = ftp->get_status();
    switch (state) {
    case FTP_STATE_URL_PARSING_ERROR:
        return INET_ERROR_CANT_PARSE_URL;
    case FTP_STATE_HOST_NOT_FOUND:
        return INET_ERROR_HOST_NOT_FOUND;
    case FTP_STATE_DIRECTORY_INVALID:
    case FTP_STATE_FILE_NOT_FOUND:
        return INET_ERROR_BAD_FILE_OR_DIR;
    case FTP_STATE_INTERNAL_ERROR:
    case FTP_STATE_SOCKET_ERROR:
    case FTP_STATE_CANT_CONNECT:
    case FTP_STATE_LOGIN_ERROR:
    case FTP_STATE_UNKNOWN_ERROR:
    case FTP_STATE_RECV_FAILED:
        return INET_ERROR_UNKNOWN_ERROR;
    case FTP_STATE_CANT_WRITE_FILE:
        return INET_ERROR_CANT_WRITE_FILE;
    default:
        return INET_ERROR_NO_ERROR;
    }
}

// 0x51baf0  Confidence: exact (two tail calls).
int InetGetFile::GetTotalBytes()
{
    if (use_http)
        return http->get_total_bytes();
    else
        return ftp->get_total_bytes();
}

// 0x51bb10  Confidence: exact (two tail calls).
int InetGetFile::GetBytesIn()
{
    if (use_http)
        return http->get_bytes_in();
    else
        return ftp->get_bytes_in();
}

// 0x51bb30 http_get__scalar_deleting_dtor: `delete http`'s helper (~http_get, then operator delete when
// bit 0 of the flag is set). Compiler-generated. Confidence: exact.
// 0x51bb50 ftp_get__scalar_deleting_dtor: the same for ftp_get. Compiler-generated. Confidence: exact.
