// summoner/multi/multisave.h - multisave.cpp (0x46cc60-0x46dd2f: the multiplayer character files (.plr),
// the autosave timer, multi.cfg; strings.tbl id 237). File name from the module column of sum.symbols,
// directory ours. Included by summoner/multi/multi.h. Not original source.
#ifndef SUMMONER_MULTI_MULTISAVE_H
#define SUMMONER_MULTI_MULTISAVE_H

#include "vsdk/vsdk.h"
#include "vsdk/os/timestamp.h"

class Player;
class Character;

#define MP_PLAYER_FILE_MAGIC   0x52594c50     // "PLYR"
#define MP_PLAYER_FILE_VERSION 9              // version 8 files are read too (flags converted)

// The .plr summary: the 0x12c bytes from the name on, read by mp_player_file_read_summary and kept by
// mp_player_file_write in a local (description at +0x28 of it: 0x46d146). mp_char_record (multi.h) is a
// file name followed by this; type name ours (D400).
struct mp_char_summary {
    char name[0x20];                  // +0x000
    int  char_info;                   // +0x020 char_infos index
    int  flags;                       // +0x024 int: the version 8 conversion shifts it with sar
    char description[0x100];          // +0x028
    short level;                      // +0x128
    short _pad12a;
};
LAYOUT_CHECK(mp_char_summary, sizeof(mp_char_summary) == 0x12c);

// The PXO settings reset and read by pxo_read_config (0x610 bytes at 0xa59a88, cleared as one block).
// sum.symbols names the flags word and the three server names; the type and the other names are ours (D404).
struct pxo_config {
    unsigned int flags;               // +0x000 bit 0: back to state 11, else 12; bit 1 cleared after multi.cfg
    ushort port;                      // +0x004 4200 (not read anywhere else)
    ushort _pad006;
    int  _unk008;                     // +0x008 0 (not read anywhere else)
    int  _unk00c;                     // +0x00c
    char utracker_server[0x100];      // +0x010 0xa59a98 pxo_utracker_server, "ut.pxo.net"
    char gtracker_server[0x100];      // +0x110 0xa59b98 pxo_gtracker_server, "gt.pxo.com"
    char chat_server[0x100];          // +0x210 0xa59c98 pxo_chat_server, "chat.pxo.net"
    char newaccount_url[0x100];       // +0x310 "http://www.pxo.net/newaccount.cfm"
    char verify_url[0x100];           // +0x410 "http://www.pxo.net/verify.cfm"
    char banners_url[0x100];          // +0x510 "http://www.pxo.net/files/banners"
};
LAYOUT_CHECK(pxo_config, sizeof(pxo_config) == 0x610);
extern pxo_config pxo_cfg;                              // 0xa59a88
// The names the other units use (sum.symbols labels of the fields).
#define pxo_cfg_flags       (pxo_cfg.flags)               // 0xa59a88
#define pxo_utracker_server (pxo_cfg.utracker_server)     // 0xa59a98
#define pxo_gtracker_server (pxo_cfg.gtracker_server)     // 0xa59b98
#define pxo_chat_server     (pxo_cfg.chat_server)         // 0xa59c98

// The HTTP proxy InetGetFile__InetGetFile passes to http_get_create_proxy when the name is not empty.
// Nothing writes them (only the three reads at 0x51b84f-0x51b891), so it is always a direct connection.
// They sit in this unit's .bss (between pxo_cfg and s3d.cpp's level_s3d 0xa5a2a0); names ours (D404).
extern ushort inet_proxy_port;                          // 0xa5a098
extern char inet_proxy_server[0x200];                   // 0xa5a09c

extern RealtimeTimer mp_player_autosave_timer;          // 0xa59a84 (was item_paging.cpp's; D405)

// 0 created, 1 player exists, 2 no player directory, 3 cannot write (jump table 0x4078a0).
int  mp_player_file_create(const char *name, int char_info, const char *description);   // 0x46cc80
// The party member's .plr file under its player name: 0 written, 2 no directory, 3 cannot write.
int  mp_player_file_write(Player *p);                  // 0x46d0b0
// (out, filename): out is an mp_char_summary (the callers pass mp_char_record + 0x28); 0 read, 2 cannot
// open, 3 not a .plr file, 4 version below 8. sum.symbols' "(path, out)" has the order reversed.
int  mp_player_file_read_summary(void *out, const char *filename);    // 0x46d450
// (the party character, file name): 0 loaded, 1 no Player record, 2..4 as read_summary. Return unused.
int  mp_player_file_load(Character *c, const char *filename);        // 0x46d610
void mp_player_autosave_start();                        // 0x46da20 300 s
void mp_player_autosave_frame();                        // 0x46da30
void mp_player_save_on_leave();                         // 0x46da60
void pxo_read_config();                                 // 0x46da80 (via pxo_read_config_thunk 0x441560)

#endif
