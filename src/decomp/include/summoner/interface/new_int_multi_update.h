// new_int_multi_update.h - the online version check (new_int_multi_update.cpp, version_check_* 0x410cf0-
// 0x4110b0 by strings.tbl ids 932 / 933). Not original source; only what the PXO chat uses.
#ifndef NEW_INT_MULTI_UPDATE_H
#define NEW_INT_MULTI_UPDATE_H

// 1 up to date (or skipped), 2 a newer version exists, 3 error (text in version_check_status_text),
// 4 still checking. The result is kept once the check is done (version_check_done 0x5bd2d8).
int version_check_run();                            // 0x410f40
extern char version_check_status_text[512];         // 0x5bd0d4

#endif
