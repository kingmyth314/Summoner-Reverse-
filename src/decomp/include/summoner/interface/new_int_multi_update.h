// new_int_multi_update.h - the online version check (new_int_multi_update.cpp, version_check_* 0x410cf0-
// 0x411260 by strings.tbl ids 932 / 933; written by decomp49). Not original source.
#ifndef NEW_INT_MULTI_UPDATE_H
#define NEW_INT_MULTI_UPDATE_H

#include "vsdk/vsdk.h"

class InetGetFile;

// The version this build compares with: major * 10000 + minor * 100 + build (1.40.00).
#define VERSION_CHECK_OWN_VERSION 14000

// 1 up to date (or skipped), 2 a newer version exists, 3 error (text in version_check_status_text),
// 4 still checking. The result is kept once the check is done (version_check_done 0x5bd2d8).
int version_check_run();                            // 0x410f40
extern char version_check_status_text[512];         // 0x5bd0d4

// 0 started, -1 failed (text set).
int  version_check_fetch();                         // 0x410cf0
// -1 failed (text set), 0 still running, 1 received; deletes the getter when done.
int  version_check_poll_download();                 // 0x410e20
void version_check_set_error_text();                // 0x410f10 strings.tbl 933
// (path): -1 no file or no "%i %i %i" line, 1 not newer than VERSION_CHECK_OWN_VERSION, 0 newer.
int  version_read_nfo_file(const char *path);       // 0x4110b0

extern int version_check_need_registry;             // 0x58bd34 (.data 1; name ours)
extern int version_check_need_fetch;                // 0x58bd38 (.data 1; name ours)
extern int version_check_result;                    // 0x58bd3c (.data 999)
extern InetGetFile *version_check_download;         // 0x5bd2d4
extern int version_check_done;                      // 0x5bd2d8

#endif
