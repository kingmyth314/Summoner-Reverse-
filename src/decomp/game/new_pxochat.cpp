// new_pxochat.cpp - Sum.exe v1.40, code 0x421e70-0x42540a (screen 11, the PXO chat lobby)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .rdata 0x580478-0x580603 (layout tables), .data 0x58f720-0x590067 (banner strings, button table,
// channel list columns, scrollbar names, "+", "!MOTD$#!"), .bss 0x5c3c60-0x5c595b.
// Source order (from the data placement): the banner globals and functions come first (their literals
// precede pxochat_join_pending in .data, their .bss precedes the screen's), then the screen.
// Pooled copies in other units: "" (0x5bd0d0), "\n" (0x588ba4), "|" (0x588c10), "%d" (0x589820),
// "#End" (0x58bd08), the ScrollBarMid names (0x588b8c / 0x588b74).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/key.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/registry.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/vfile/file.h"
#include "vsdk/parse/parse.h"
#include "vsdk/inetfile/inetgetfile.h"
#include "vsdk/localize/localize.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/multi/multi.h"
#include "summoner/interface/new_pxo_banner.h"
#include "summoner/interface/new_int_multi_update.h"
#include "summoner/interface/new_pxochat.h"

#define PXOCHAT_NUM_BUTTONS  34
#define PXOCHAT_MAX_LINES    64
#define PXO_BANNER_MAX       10
#define PXO_BANNER_CFG       "new_chatbanners.cfg"
#define PXO_BANNER_BASE_URL  "http://www.pxo.net/files/summoner/northamerica/banners"
#define PXO_MOTD_PREFIX      "!MOTD$#!"

// ==== chat banner =====================================================================================

// ---- .bss (0x5c3c60-0x5c3e8f) ----------------------------------------------------------------------
char pxo_chat_banner_file[33];                // 0x5c3c60 banner .pcx name
char pxo_chat_banner_download_url[257];       // 0x5c3c81 base URL + name
char pxo_chat_banner_link_url[258];           // 0x5c3d82 opened when the banner is clicked
int pxo_chat_banner_bm;                       // 0x5c3e84 -1 none
// 0 fetch the cfg, 1 cfg downloading, 2 parse it, 3 banner downloading, 4 load the bitmap, 5 idle,
// 6 parse the cfg and use a banner already on disk (registry ChatBanners 0)
int pxo_chat_banner_state;                    // 0x5c3e88
InetGetFile *pxo_chat_banner_inet;            // 0x5c3e8c

// 0x421e70  Confidence: likely (the null test of the array address: VC6 did not fold it, so the source
// tested a pointer; written with a local).
void pxochat_open_banner_url()
{
    char *url = pxo_chat_banner_link_url;

    if (url && strlen(url) > 1)
        open_url(pxo_chat_banner_link_url);
}

// 0x421ea0  Confidence: likely (the order of the clears). With ChatBanners set, the small (failed)
// downloads are deleted and the cfg is fetched again (state 0); otherwise the banners on disk are used.
void pxo_find_chat_banners()
{
    DWORD fetch;

    pxo_chat_banner_bm = -1;
    os_registry_read_dword(0, "ChatBanners", &fetch, 1);
    if (fetch) {
        file_delete_small_files("*.cfg", path_multicache);
        file_delete_small_files("*.pcx", path_multicache);
        if (pxo_chat_banner_state == 5) {
            pxo_chat_banner_state = 6;
            return;
        }
        pxo_chat_banner_state = 0;
    } else {
        pxo_chat_banner_state = 5;
    }

    pxo_chat_banner_inet = 0;
    pxo_chat_banner_bm = -1;
    strcpy(pxo_chat_banner_file, "");
    strcpy(pxo_chat_banner_download_url, "");
    strcpy(pxo_chat_banner_link_url, "");
}

// 0x421fa0  Confidence: likely (jump table 0x422354 over the state, 0..6; the EH frame is the one of the
// two "new InetGetFile"; the two 512-byte buffers are "" initialised, their declaration order is a guess).
void pxo_load_chat_banners()
{
    char url[512] = "";
    char path[512] = "";

    switch (pxo_chat_banner_state) {
    case 0:
        sprintf(url, "%s/%s", PXO_BANNER_BASE_URL, PXO_BANNER_CFG);
        file_make_path(path_multicache, PXO_BANNER_CFG, path);
        pxo_chat_banner_inet = new InetGetFile(url, path);
        pxo_chat_banner_state = 1;
        break;

    case 1:
        if (pxo_chat_banner_inet->IsFileError()) {
            delete pxo_chat_banner_inet;
            pxo_chat_banner_inet = 0;
            pxo_chat_banner_state = 5;
            break;
        }
        if (pxo_chat_banner_inet->IsConnecting() || pxo_chat_banner_inet->IsReceiving()
            || !pxo_chat_banner_inet->IsFileReceived())
            break;
        delete pxo_chat_banner_inet;
        pxo_chat_banner_inet = 0;
        pxo_chat_banner_state = 2;
        break;

    case 2:
        pxo_parse_chat_banners_cfg(0);
        if (!strlen(pxo_chat_banner_file) || !strlen(pxo_chat_banner_download_url)) {
            pxo_chat_banner_state = 5;
            break;
        }
        if (file_exists_on_disk(pxo_chat_banner_file, path_multicache)) {
            pxo_chat_banner_state = 4;
            break;
        }
        file_make_path(path_multicache, pxo_chat_banner_file, path);
        pxo_chat_banner_inet = new InetGetFile(pxo_chat_banner_download_url, path);
        pxo_chat_banner_state = 3;
        break;

    case 3:
        if (pxo_chat_banner_inet->IsFileError()) {
            delete pxo_chat_banner_inet;
            pxo_chat_banner_inet = 0;
            pxo_chat_banner_state = 5;
            break;
        }
        if (pxo_chat_banner_inet->IsConnecting() || pxo_chat_banner_inet->IsReceiving()
            || !pxo_chat_banner_inet->IsFileReceived())
            break;
        delete pxo_chat_banner_inet;
        pxo_chat_banner_inet = 0;
        pxo_chat_banner_state = 4;
        break;

    case 4:
        if (strlen(pxo_chat_banner_file))
            pxo_chat_banner_bm = bm_load(pxo_chat_banner_file, path_multicache, -1);
        pxo_chat_banner_state = 5;
        break;

    case 5:
        break;

    case 6:
        pxo_parse_chat_banners_cfg(1);
        pxo_chat_banner_state = 4;
        break;
    }
}

// 0x422370  Confidence: exact. Screen 11's leave: stops the download and frees the banner.
void pxochat_leave()
{
    if (pxo_chat_banner_inet) {
        pxo_chat_banner_inet->AbortGet();
        delete pxo_chat_banner_inet;
        pxo_chat_banner_inet = 0;
    }
    if (pxo_chat_banner_bm != -1) {
        bm_release(pxo_chat_banner_bm);
        pxo_chat_banner_bm = -1;
    }
}

// 0x4223d0  Confidence: likely (the "found" fill is rep stosd of -1, read as memset; local order). The
// cfg: "$Base_url:" "url", then up to 10 of "$Pcx_name:" "name" [+Link: "url"], "#End". One banner is
// picked at random (only among those on disk with downloaded_only). Note: the +Link: string is parsed
// into one temporary buffer (esp+0x28c) and never copied into links[], so the link URL is always empty
// and the banner is never clickable; kept as in the binary.
void pxo_parse_chat_banners_cfg(int downloaded_only)
{
    char base_url[512] = "";
    TextFile tf;
    int found[PXO_BANNER_MAX];
    char link[256];
    char pcx_names[PXO_BANNER_MAX][512];
    char links[PXO_BANNER_MAX][512];
    int count = 0;
    int i;

    if (!file_exists_on_disk(PXO_BANNER_CFG, path_multicache))
        return;

    tf.Open(PXO_BANNER_CFG, path_multicache);
    pxo_chat_banner_bm = -1;
    strcpy(pxo_chat_banner_file, "");
    strcpy(pxo_chat_banner_download_url, "");
    strcpy(pxo_chat_banner_link_url, "");
    for (i = 0; i < PXO_BANNER_MAX; i++) {
        strcpy(pcx_names[i], "");
        strcpy(links[i], "");
    }

    if (!tf.Accept("$Base_url:")) {
        tf.Close();
        file_delete(PXO_BANNER_CFG, path_multicache);
        return;
    }

    tf.ParseString(base_url, 512, '"', '"');
    while (!tf.Accept("#End") && count < PXO_BANNER_MAX) {
        tf.Expect("$Pcx_name:");
        tf.ParseString(pcx_names[count], 254, '"', '"');
        if (tf.Accept("+Link:"))
            tf.ParseString(link, 254, '"', '"');
        count++;
    }
    tf.Close();

    if (count <= 0)
        return;

    if (downloaded_only) {
        int num_found = 0;

        memset(found, -1, sizeof(found));
        for (i = 0; i < count; i++) {
            if (file_exists_on_disk(pcx_names[i], path_multicache))
                found[num_found++] = i;
        }
        if (num_found > 0) {
            int pick = found[rand() % num_found];
            if (pick >= 0) {
                strncpy(pxo_chat_banner_file, pcx_names[pick], 32);
                strncpy(pxo_chat_banner_download_url, base_url, 256);
                strncat(pxo_chat_banner_download_url, pcx_names[pick], 256);
                strncpy(pxo_chat_banner_link_url, links[pick], 256);
            }
        }
    } else {
        int pick = rand() % count;
        strncpy(pxo_chat_banner_file, pcx_names[pick], 32);
        strncpy(pxo_chat_banner_download_url, base_url, 256);
        strncat(pxo_chat_banner_download_url, pcx_names[pick], 256);
        strncpy(pxo_chat_banner_link_url, links[pick], 256);
    }
}

// 0x422790  Confidence: exact.
bool pxochat_banner_has_url()
{
    return pxo_chat_banner_bm != -1 && strlen(pxo_chat_banner_link_url) > 1;
}

// ==== the screen ======================================================================================

// ---- .rdata (0x580478-0x580603; per resolution_index 640 / 800 / 1024; names ours) ---------------------
const int pxochat_banner_pos[3][2]   = { { 111, 1 }, { 139, 1 }, { 353, 29 } };       // 0x580478
const int pxochat_banner_size[3][2]  = { { 476, 76 }, { 595, 95 }, { 478, 77 } };     // 0x580490
// [res][list]: list 0 channels, 1 users, 2 chat lines
const int pxochat_list_pos[3][3][2]  = { { { 359, 116 }, { 16, 125 }, { 190, 200 } },  // 0x5804a8
                                         { { 449, 145 }, { 20, 150 }, { 238, 258 } },
                                         { { 576, 186 }, { 26, 195 }, { 304, 322 } } };
const int pxochat_list_width[3][3]   = { { 257, 136, 420 }, { 321, 170, 525 }, { 411, 218, 672 } };   // 0x5804f0
const int pxochat_list_page[3][3]    = { { 4, 16, 11 }, { 4, 16, 10 }, { 4, 16, 11 } };               // 0x580514
const int pxochat_track_pos[3][3][2] = { { { 622, 143 }, { 163, 154 }, { 622, 241 } },  // 0x580538
                                         { { 778, 179 }, { 204, 193 }, { 778, 301 } },
                                         { { 995, 229 }, { 261, 246 }, { 997, 386 } } };
const int pxochat_track_h[3][3]      = { { 6, 171, 85 }, { 8, 214, 106 }, { 10, 274, 136 } };         // 0x580580
const int pxochat_chat_input_pos[3][2]  = { { 190, 380 }, { 238, 475 }, { 304, 608 } };  // 0x5805a4 clip rect
const int pxochat_chat_input_size[3][2] = { { 433, 14 }, { 541, 18 }, { 693, 22 } };     // 0x5805bc
const int pxochat_caption_pos[3][2]     = { { 189, 448 }, { 236, 560 }, { 302, 717 } };  // 0x5805d4 centre x, y
const int pxochat_channel_input_pos[3][2] = { { 285, 253 }, { 230, 316 }, { 294, 407 } };  // 0x5805ec

// ---- .data -----------------------------------------------------------------------------------------
bool pxochat_join_pending = true;             // 0x58f7b0 open the chat input once the channel is joined
// 0x58f7b8  34 x 0x40 (sum.symbols pxochat_button_def0_flags = 0x58f7f4, the flags of entry 0). flags:
// 1 HiLite that acts through the entry after it on release, 2 acts on press and auto-repeats, 4 belongs
// to the channel-name popup (reacts and is drawn on top only while it is open). 23 / 24 (close) exist only
// at 1024; 25 / 26 are the banner (no bitmap of their own); 0, 25 and 27 are always drawn.
ui_button_def pxochat_button_defs[PXOCHAT_NUM_BUTTONS] = {
    { "PXOChat-Bkgrnd",           { { 0, 0 },     { 0, 0 },     { 0, 0 } },     0 },
    { "Icon-ArrwUPHilite",        { { 619, 113 }, { 774, 141 }, { 989, 180 } }, 2 },   // channels up
    { "Icon-ArrwUPPress",         { { 619, 113 }, { 774, 141 }, { 989, 180 } }, 0 },
    { "Icon-ArrwDNHilite",        { { 619, 156 }, { 774, 195 }, { 989, 243 } }, 2 },   // channels down
    { "Icon-ArrwDNPress",         { { 619, 156 }, { 774, 195 }, { 989, 243 } }, 0 },
    { "Icon-ArrwUPHilite",        { { 161, 122 }, { 201, 153 }, { 256, 193 } }, 2 },   // users up
    { "Icon-ArrwUPPress",         { { 161, 122 }, { 201, 153 }, { 256, 193 } }, 0 },
    { "Icon-ArrwDNHilite",        { { 161, 337 }, { 201, 421 }, { 256, 534 } }, 2 },   // users down
    { "Icon-ArrwDNPress",         { { 161, 337 }, { 201, 421 }, { 256, 534 } }, 0 },
    { "Icon-ArrwUPHilite",        { { 619, 209 }, { 774, 261 }, { 989, 332 } }, 2 },   // chat up
    { "Icon-ArrwUPPress",         { { 619, 209 }, { 774, 261 }, { 989, 332 } }, 0 },
    { "Icon-ArrwDNHilite",        { { 619, 338 }, { 774, 423 }, { 989, 537 } }, 2 },   // chat down
    { "Icon-ArrwDNPress",         { { 619, 338 }, { 774, 423 }, { 989, 537 } }, 0 },
    { "PXOChat-JnChnlHilite",     { { 199, 113 }, { 249, 141 }, { 311, 181 } }, 1 },   // join channel
    { "PXOChat-JnChnlPress",      { { 199, 113 }, { 249, 141 }, { 311, 181 } }, 0 },
    { "PXOChat-JnPrvtHilite",     { { 210, 152 }, { 263, 190 }, { 311, 243 } }, 1 },   // join private
    { "PXOChat-JnPrvtPress",      { { 210, 152 }, { 263, 190 }, { 311, 243 } }, 0 },
    { "PXOChat-GmLstHilite",      { { 438, 400 }, { 548, 501 }, { 694, 641 } }, 1 },   // game list
    { "PXOChat-GmLstPress",       { { 438, 400 }, { 548, 501 }, { 694, 641 } }, 0 },
    { "PXOChat-MotDHilite",       { { 386, 443 }, { 483, 554 }, { 618, 710 } }, 1 },   // MOTD
    { "PXOChat-MotDPress",        { { 386, 443 }, { 483, 554 }, { 618, 710 } }, 0 },
    { "PXOChat-ExitHilite",       { { 566, 443 }, { 708, 554 }, { 905, 710 } }, 1 },   // exit
    { "PXOChat-ExitPress",        { { 566, 443 }, { 708, 554 }, { 905, 710 } }, 0 },
    { "Icon-GrnClseHilite",       { { 0, 0 },     { 0, 0 },     { 959, 32 } },  1 },   // close (1024 only)
    { "Icon-GrnClsePress",        { { 0, 0 },     { 0, 0 },     { 959, 32 } },  0 },
    { "",                         { { 112, 2 },   { 140, 2 },   { 355, 31 } },  1 },   // banner
    { "",                         { { 112, 2 },   { 140, 2 },   { 355, 31 } },  0 },
    { "PXO-JPBkgrnd",             { { 170, 122 }, { 213, 153 }, { 272, 195 } }, 4 },   // channel-name popup
    { "LevelSelectProfileHiLite", { { 204, 289 }, { 255, 361 }, { 326, 462 } }, 5 },   // popup: join
    { "LevelSelectProfilePress",  { { 204, 289 }, { 255, 361 }, { 326, 462 } }, 4 },
    { "LevelSelectProfileHiLite", { { 337, 289 }, { 421, 361 }, { 539, 462 } }, 5 },   // popup: cancel
    { "LevelSelectProfilePress",  { { 337, 289 }, { 421, 361 }, { 539, 462 } }, 4 },
    { "Icon-GrnClseHilite",       { { 427, 143 }, { 534, 179 }, { 683, 229 } }, 5 },   // popup: close
    { "Icon-GrnClsePress",        { { 427, 143 }, { 534, 179 }, { 683, 229 } }, 4 },
};
// Channel list columns (*new* names): right edge of the first number, x of the second. Not const, so
// .data; the 800 values are smaller than the 640 ones, kept as in the binary.
int pxochat_channel_users_right[3] = { 211, 108, 152 };      // 0x590038
int pxochat_channel_count_x[3]     = { 223, 117, 165 };      // 0x590044
// 0x590050. The strings are pooled copies at 0x588b8c / 0x588b74.
char *pxochat_scrollbar_names[2] = { "ScrollBarMid640.tga", "ScrollBarMid1024.tga" };
// then "+" (0x590058) and "!MOTD$#!" (0x59005c).

// ---- .bss (0x5c3e90-0x5c595b) ----------------------------------------------------------------------
int pxochat_pressed_button;                                   // 0x5c3e90
bool pxochat_channel_cursor_visible;                          // 0x5c3e94
bool pxochat_saved_chat_input;                                // 0x5c3e95
bool pxochat_channel_input_active;                            // 0x5c3e96 channel-name popup open
bool pxochat_channel_input_unk;                               // 0x5c3e97 (*new*) set by input_begin, never read
bool pxochat_chat_cursor_visible;                             // 0x5c3e98
char pxochat_status_text[64];                                 // 0x5c3ea0
int pxochat_thumb_size[3][2];                                 // 0x5c3ee0
bool pxochat_chat_input_active;                               // 0x5c3ef8
ui_button_state pxochat_buttons[PXOCHAT_NUM_BUTTONS];         // 0x5c3f00
int pxochat_scroll_pos[3];                                    // 0x5c4230
RealtimeTimer pxochat_chat_cursor_timer;                      // 0x5c423c
RealtimeTimer pxochat_channel_cursor_timer;                   // 0x5c57d0
RealtimeTimer pxochat_button_timer;                           // 0x5c426c
char pxochat_channel_input[36];                               // 0x5c4240
int pxochat_thumb_bm_w;                                       // 0x5c4264
int pxochat_thumb_bm_h;                                       // 0x5c4268
bool pxochat_scrollbar_visible[3];                            // 0x5c4270
char pxochat_motd[1024];                                      // 0x5c4274
int pxochat_hover_row;                                        // 0x5c4674
char pxochat_selected_user[36];                               // 0x5c4678
char pxochat_typed_channel[36];                               // 0x5c469c
pxochat_line pxochat_chat_lines[PXOCHAT_MAX_LINES];           // 0x5c46c0
int pxochat_list_count[3];                                    // 0x5c57c0
int pxochat_drag_start_mouse_y;                               // 0x5c57d4
char pxochat_chat_input[256];                                 // 0x5c57d8
int pxochat_drag_start_thumb_y;                               // 0x5c58d8
// 0 idle, 1 button held, 2 dragging, 3 checking version, 4 version error, 5 new version, 6 validating,
// 7 / 8 / 9 login errors (popups 3..9 are ui_choice_popup boxes)
int pxochat_state;                                            // 0x5c58dc
char pxochat_selected_channel[36];                            // 0x5c58e0
int pxochat_scrollbar_bm;                                     // 0x5c5904
int pxochat_thumb_pos[3][2];                                  // 0x5c5908
int pxochat_active_list;                                      // 0x5c5920 list under the mouse / dragged, -1
int pxochat_logged_in;                                        // 0x5c5924
int pxochat_version_checked;                                  // 0x5c5928
int pxochat_login_checked;                                    // 0x5c592c
int pxochat_login_unk_5c5930;                                 // 0x5c5930 (*new*) only cleared here
int pxo_run_updater_on_exit;                                  // 0x5c5934

// Popup texts: references to localize_strings slots, which makes VC6 emit the init_strptrs_* dynamic
// initialisers (as new_help.cpp's tables). Definition order = static_initializers order.
char *&pxochat_str_new_version      = XSTR("A new version of Summoner is available.\n", 757);   // 0x5c5938
char *&pxochat_str_update_now       = XSTR("You must update to the new version to play on online\n\nAuto update now?", 758);   // 0x5c593c
char *&pxochat_str_checking_version = XSTR("Checking Version\n", 759);                          // 0x5c5940
char *&pxochat_str_blank_version    = XSTR("", 34);                                             // 0x5c5944
char *&pxochat_str_version_error    = XSTR("Error\n", 760);                                     // 0x5c5948
char *&pxochat_str_validating_user  = XSTR("Validating User\n", 761);                           // 0x5c57cc
char *&pxochat_str_validation_error = XSTR("Validation Error\n", 762);                          // 0x5c3e9c
char *&pxochat_str_blank_login      = XSTR("", 34);                                             // 0x5c594c
char *&pxochat_str_need_account     = XSTR("In order to play online, you must sign up for an account at "
                                           "www.thqmultiplay.net. ...\n", 763);                  // 0x5c5950
char *&pxochat_str_invalid_user     = XSTR("Your user name appears to be invalid. ...", 764);    // 0x5c5954
char *&pxochat_str_cannot_validate  = XSTR("Unable to contact THQMultiplay.net to validate your account.  "
                                           "Please try back later\n", 765);                     // 0x5c5958

// Compiler-generated dynamic initialiser pairs (thunk "jmp", initialiser), static_initializers entries
// 63-76, confidence exact (no source of their own):
//   0x4227c0 / 0x4227d0 pxochat_chat_cursor_timer, 0x4227e0 / 0x4227f0 pxochat_channel_cursor_timer,
//   0x422800 / 0x422810 pxochat_button_timer (RealtimeTimer__RealtimeTimer);
//   0x422820 / 0x422830 ... 0x422960 / 0x422970 init_strptrs_5c5938, 5c593c, 5c5940, 5c5944, 5c5948,
//   5c57cc, 5c3e9c, 5c594c, 5c5950, 5c5954, 5c5958 (one "mov [ref], &localize_strings[id]" each).

// 0x422980  Confidence: likely (the second NULL test is missing: with text1 NULL and text2 set,
// strlen(NULL) runs, as in the binary; the widths are a local array). Wraps both texts at 400 / 500 / 640
// pixels (at most 10 lines each), each block followed by an empty line.
void pxochat_build_popup_text(const char *text1, const char *text2, char *out)
{
    int widths[3] = { 400, 500, 640 };
    int lengths[10];
    int starts[10];
    int len = 0;
    int n, i;

    if (!text1 && !text2) {
        strcpy(out, "");
        return;
    }

    if (strlen(text1)) {
        n = gr_split_str(lengths, starts, text1, widths[resolution_index], 10, 0, -1);
        strcpy(out, "");
        for (i = 0; i < n; i++) {
            strncat(out, text1 + starts[i], lengths[i]);
            len += lengths[i];
            out[len++] = '\n';
            out[len] = 0;
        }
        strcat(out, "\n");
        len++;
    }

    if (strlen(text2)) {
        n = gr_split_str(lengths, starts, text2, widths[resolution_index], 10, 0, -1);
        for (i = 0; i < n; i++) {
            strncat(out, text2 + starts[i], lengths[i]);
            len += lengths[i];
            out[len++] = '\n';
            out[len] = 0;
        }
        strcat(out, "\n");
    }
}

// 0x422b40  Confidence: likely (field-wise copy into the reused slot; the two count stores are in
// +0x8c, +0x88 order). The typed name becomes "+name"; a channel slot that already holds a '+' channel is
// reused, else the record is appended.
int pxochat_add_typed_channel()
{
    lobby_channel ch;
    int i;

    if (!strlen(pxochat_channel_input))
        return -1;

    strcpy(pxochat_typed_channel, "+");
    strcat(pxochat_typed_channel, pxochat_channel_input);
    strcpy(ch.name, pxochat_typed_channel);
    strcpy(ch.topic, pxochat_channel_input);
    ch.unk8c = 0;
    ch.num_users = 0;

    for (i = 0; i < lobby_num_channels; i++) {
        if (lobby_channels[i].name[0] == '+')
            break;
    }
    if (i < lobby_num_channels) {
        strcpy(lobby_channels[i].name, ch.name);
        strcpy(lobby_channels[i].topic, ch.topic);
        lobby_channels[i].unk8c = 0;
        lobby_channels[i].num_users = 0;
        lobby_flags |= 8;
        return 0;
    }

    if (lobby_add_channel(&ch) < 0)
        return -1;
    lobby_flags |= 8;
    return 0;
}

// 0x422cf0  Confidence: exact.
void pxochat_join_typed_channel()
{
    if (pxochat_add_typed_channel() >= 0)
        pxochat_join_selected_channel();
    pxochat_input_cancel();
}

// 0x422d10  Confidence: exact. Closes the channel-name popup.
void pxochat_input_cancel()
{
    pxochat_channel_input_active = false;
    pxochat_channel_input[0] = 0;
    pxochat_chat_input_active = pxochat_saved_chat_input;
}

// 0x422d30  Confidence: exact.
void pxochat_join_selected_channel()
{
    lobby_join_channel((lobby_flags & 8) ? pxochat_typed_channel : pxochat_selected_channel);
    pxochat_scroll_pos[1] = 0;
    pxochat_selected_user[0] = 0;
    pxochat_chat_input_active = false;
    pxochat_join_pending = true;
}

// 0x422d70  Confidence: exact. The link cursor over a clickable banner (entry 25).
void pxochat_update_banner_cursor()
{
    if (point_in_rect(game_mouse_x, game_mouse_y, pxochat_buttons[25].x, pxochat_buttons[25].y,
                      pxochat_buttons[25].w, pxochat_buttons[25].h)
        && pxochat_banner_has_url())
        cursor_set_type(cursor_url_bm);
    else
        cursor_set_type(cursor_walk_bm);
}

// 0x422dd0  Confidence: likely (flags = 4 written as a store, not an or; the order of the x / y stores
// before the bitmap load). The close button (23 / 24) exists only at 1024; the banner entry (25) gets its
// position here and its bitmap and size from pxochat_update_banner.
void pxochat_load_bitmaps()
{
    float scale = ui_get_scale();
    int i;

    for (i = 0; i < PXOCHAT_NUM_BUTTONS; i++) {
        pxochat_buttons[i].flags = 0;
        pxochat_buttons[i].bitmap = -1;
        pxochat_buttons[i].x = 0;
        pxochat_buttons[i].y = 0;
        pxochat_buttons[i].w = 0;
        pxochat_buttons[i].h = 0;

        if ((i == 23 || i == 24) && resolution_index != 2) {
            pxochat_buttons[i].flags = 4;
            continue;
        }

        if (strlen(pxochat_button_defs[i].name) || i == 25) {
            pxochat_buttons[i].x = pxochat_button_defs[i].pos[resolution_index][0];
            pxochat_buttons[i].y = pxochat_button_defs[i].pos[resolution_index][1];
            if (i != 25) {
                char filename[128];

                strcpy(filename, pxochat_button_defs[i].name);
                strcat(filename, ui_bitmap_suffix_tga[ui_hires]);
                pxochat_buttons[i].bitmap = bm_load(filename, path_interface, -1);
                bm_get_dimensions(pxochat_buttons[i].bitmap, &pxochat_buttons[i].w, &pxochat_buttons[i].h);
                pxochat_buttons[i].w = (int)(pxochat_buttons[i].w * scale + 0.5f);
                pxochat_buttons[i].h = (int)(pxochat_buttons[i].h * scale + 0.5f);
            }
            if (i == 0 || i == 25 || i == 27)
                pxochat_buttons[i].flags |= 1;
        }
    }

    pxochat_scrollbar_bm = bm_load(pxochat_scrollbar_names[ui_hires], path_interface, -1);
    bm_get_dimensions(pxochat_scrollbar_bm, &pxochat_thumb_bm_w, &pxochat_thumb_bm_h);
    pxochat_thumb_bm_w = (int)(pxochat_thumb_bm_w * scale);
    pxochat_thumb_bm_h = (int)(pxochat_thumb_bm_h * scale);
    pxochat_clear_motd();
    pxo_login_reset();
}

// 0x422fc0  Confidence: likely (the clamp spelling of the chat list's last page). The chat list keeps its
// lines and shows its last page; the other lists scroll to the top.
void pxochat_init()
{
    int i;

    for (i = 0; i < 3; i++) {
        if (i == 2) {
            int pos = pxochat_list_count[2] - pxochat_list_page[resolution_index][2];
            pxochat_scroll_pos[2] = pos > 0 ? pos : 0;
        } else {
            pxochat_scroll_pos[i] = 0;
        }
        pxochat_update_scrollbar(i);
    }

    pxochat_selected_channel[0] = 0;
    pxochat_selected_user[0] = 0;
    pxochat_chat_input[0] = 0;
    pxochat_chat_input_active = false;
    pxochat_join_pending = true;
    pxochat_status_text[0] = 0;
    pxochat_state = 0;
    pxochat_active_list = -1;
    pxo_find_chat_banners();
}

// 0x423050  Confidence: likely (as mp_charselect_update_scrollbar; count and page in spare stack slots).
// Buttons 1 + 4 * list / 3 + 4 * list are the list's arrows.
void pxochat_update_scrollbar(int list)
{
    if (list == -1)
        return;

    int count = pxochat_list_count[list];
    int page = pxochat_list_page[resolution_index][list];

    if (count <= page) {
        pxochat_scrollbar_visible[list] = false;
        pxochat_buttons[1 + list * 4].flags |= 4;
        pxochat_buttons[3 + list * 4].flags |= 4;
        return;
    }

    pxochat_scrollbar_visible[list] = true;
    pxochat_buttons[1 + list * 4].flags &= ~4;
    pxochat_buttons[3 + list * 4].flags &= ~4;

    if (count > 0) {
        pxochat_thumb_pos[list][0] = pxochat_track_pos[resolution_index][list][0];
        pxochat_thumb_pos[list][1] = (int)((float)pxochat_scroll_pos[list] / count
                                           * pxochat_track_h[resolution_index][list] + 0.5f)
                                     + pxochat_track_pos[resolution_index][list][1];
        pxochat_thumb_size[list][0] = pxochat_thumb_bm_w;
        pxochat_thumb_size[list][1] = (int)((float)page / count * pxochat_track_h[resolution_index][list] + 0.5f);
    } else {
        pxochat_thumb_pos[list][0] = pxochat_track_pos[resolution_index][list][0];
        pxochat_thumb_pos[list][1] = pxochat_track_pos[resolution_index][list][1];
        pxochat_thumb_size[list][0] = pxochat_thumb_bm_w;
        pxochat_thumb_size[list][1] = pxochat_track_h[resolution_index][list];
    }
}

// 0x4231a0  Confidence: likely (a one-call function, tail jump; the name of the source function is not
// known). Reached from interface_set_screen as screen 11's leave.
void pxochat_leave_thunk()
{
    pxochat_leave();
}

// 0x4231b0  Confidence: exact. Steps the banner download and shows the banner once it is loaded.
void pxochat_update_banner()
{
    pxo_load_chat_banners();
    pxochat_update_banner_cursor();

    if (pxochat_channel_cursor_timer.elapsed()) {
        pxochat_channel_cursor_timer.SetTimeout(500);
        pxochat_channel_cursor_visible = !pxochat_channel_cursor_visible;
    }

    if (pxochat_buttons[25].bitmap != pxo_chat_banner_bm && pxo_chat_banner_bm != -1) {
        pxochat_buttons[25].bitmap = pxo_chat_banner_bm;
        pxochat_buttons[25].w = pxochat_banner_size[resolution_index][0];
        pxochat_buttons[25].h = pxochat_banner_size[resolution_index][1];
    }
}

// 0x423220  Confidence: likely (jump table 0x42334c over state - 3, 0..6). Version check first, then the
// login; while a popup is up its frame function owns the keys.
int pxochat_frame(int key)
{
    int i;

    if (!pxochat_version_checked)
        pxochat_version_check_frame();
    else if (!pxochat_login_checked)
        pxochat_login_frame();

    switch (pxochat_state) {
    case 3:
        return pxochat_checking_version_popup_frame(key);
    case 4:
        return pxochat_version_error_popup_frame(key);
    case 5:
        return pxochat_new_version_popup_frame(key);
    case 6:
        return pxochat_validating_popup_frame(key);
    case 7:
    case 8:
    case 9:
        return pxochat_login_error_popup_frame(key);
    }

    if (pxochat_escape_leaves(key)) {
        gameseq_set_state(1);
        return key;
    }

    pxochat_update_join_buttons();
    empty_frame_hook_40acc0();
    pxochat_update_status_text();
    pxochat_update_banner();
    pxochat_on_channel_joined();

    if (pxochat_chat_cursor_timer.elapsed()) {
        pxochat_chat_cursor_timer.SetTimeout(500);
        pxochat_chat_cursor_visible = !pxochat_chat_cursor_visible;
    }

    for (i = 0; i < PXOCHAT_NUM_BUTTONS; i++)
        pxochat_buttons[i].flags &= ~2;

    switch (pxochat_state) {
    case 0:
        key = pxochat_handle_click(key);
        break;
    case 1:
        pxochat_update_buttons();
        break;
    case 2:
        pxochat_drag_scrollbar();
        break;
    }
    return key;
}

// 0x423370  Confidence: likely. Any choice ends the check; a button (the Cancel) leaves to state 1.
int pxochat_checking_version_popup_frame(int key)
{
    int choice = ui_choice_popup_frame(key);

    if (choice == -1)
        return key;
    if (choice == -2) {
        pxochat_version_checked = 1;
        pxochat_state = 0;
        return 0;
    }
    pxochat_version_checked = 1;
    pxochat_state = 0;
    if (choice >= 0)
        gameseq_set_state(1);
    return key;
}

// 0x4233d0  Confidence: likely (a compare of the choice with 0 is left without a branch: an empty
// "if (choice >= 0)" or a removed statement in the source).
int pxochat_version_error_popup_frame(int key)
{
    int choice = ui_choice_popup_frame(key);

    if (choice == -1)
        return key;
    if (choice == -2) {
        pxochat_version_checked = 1;
        pxochat_state = 0;
        return 0;
    }
    pxochat_version_checked = 1;
    pxochat_state = 0;
    return key;
}

// 0x423420  Confidence: unsure (the last case: "cmp eax, 1" is followed by an unconditional
// gameseq_set_state(1), so every choice above 0 leaves to state 1). Yes (0) runs the updater on exit
// (state 3), Update Later (1) leaves.
int pxochat_new_version_popup_frame(int key)
{
    int choice = ui_choice_popup_frame(key);

    if (choice == -1)
        return key;
    if (choice == -2) {
        pxochat_version_checked = 1;
        pxochat_state = 0;
        return 0;
    }
    if (choice < 0) {
        pxochat_version_checked = 1;
        pxochat_state = 0;
        return key;
    }
    if (choice == 0) {
        gameseq_set_state(3);
        pxo_run_updater_on_exit = 1;
    } else {
        gameseq_set_state(1);
    }
    return key;
}

// 0x4234a0  Confidence: exact. Cancel or Esc gives up the login.
int pxochat_validating_popup_frame(int key)
{
    int choice = ui_choice_popup_frame(key);

    if (choice == -1)
        return key;
    if (choice == -2 || key == KEY_ESC) {
        pxochat_logged_in = 0;
        pxochat_login_unk_5c5930 = 0;
        pxochat_state = 0;
        gameseq_set_state(1);
        return 0;
    }
    return key;
}

// 0x4234f0  Confidence: exact.
int pxochat_login_error_popup_frame(int key)
{
    if (ui_choice_popup_frame(key) != -1) {
        pxochat_logged_in = 0;
        pxochat_login_unk_5c5930 = 0;
        pxochat_state = 0;
        gameseq_set_state(1);
    }
    return key;
}

// 0x423530  Confidence: likely (local order of the button names; the tail test after the result 1 path
// returns early). version_check_run: 1 done, 2 new version (Yes / Update Later), 3 error (Ok), 4 still
// checking (Cancel).
void pxochat_version_check_frame()
{
    char *cancel = XSTR("Cancel", 696);
    char *ok = XSTR("Ok", 213);
    char *yes_later[2] = { XSTR("Yes", 654), XSTR("Update Later", 766) };
    char text[1024];

    if (!pxochat_version_checked) {
        int result = version_check_run();

        if (result == 1) {
            pxochat_version_checked = 1;
            pxochat_state = 0;
            return;
        }
        if (result == 2) {
            if (pxochat_state != 5) {
                pxochat_state = 5;
                pxochat_build_popup_text(localize_string_or_invalid(&pxochat_str_new_version),
                                         localize_string_or_invalid(&pxochat_str_update_now), text);
                ui_choice_popup_show(text, -1, -1, 2, yes_later, 0xb, 0, 0);
            }
        } else if (result == 3) {
            if (pxochat_state != 4) {
                pxochat_state = 4;
                pxochat_build_popup_text(localize_string_or_invalid(&pxochat_str_version_error),
                                         version_check_status_text, text);
                ui_choice_popup_show(text, -1, -1, 1, &ok, 0xb, 0, 0);
            }
        } else if (result == 4) {
            if (pxochat_state == 0) {
                pxochat_state = 3;
                pxochat_build_popup_text(localize_string_or_invalid(&pxochat_str_checking_version),
                                         localize_string_or_invalid(&pxochat_str_blank_version), text);
                ui_choice_popup_show(text, -1, -1, 1, &cancel, 0xb, 0, 0);
            }
        }
    }

    if (pxochat_version_checked && !pxochat_logged_in)
        gameseq_set_state(1);
}

// 0x4236c0  Confidence: likely (jump table 0x4238ac over pxo_login_step's result 0..4; outside states 0
// and 6 the step is skipped and the "validating" case runs, which then does nothing).
void pxochat_login_frame()
{
    char *ok = XSTR("Ok", 213);
    char *cancel = XSTR("Cancel", 696);
    char text[1024];

    if (!pxochat_login_checked) {
        int result = 3;

        if (pxochat_state == 6 || pxochat_state == 0)
            result = pxo_login_step();

        switch (result) {
        case 0:
            pxochat_logged_in = 1;
            pxochat_login_checked = 1;
            pxochat_state = 0;
            return;
        case 1:
            if (pxochat_state != 7) {
                pxochat_state = 7;
                pxochat_build_popup_text(localize_string_or_invalid(&pxochat_str_validation_error),
                                         localize_string_or_invalid(&pxochat_str_need_account), text);
                ui_choice_popup_show(text, -1, -1, 1, &ok, 0xb, 0, 0);
            }
            break;
        case 2:
            if (pxochat_state != 9) {
                pxochat_state = 9;
                pxochat_build_popup_text(localize_string_or_invalid(&pxochat_str_validation_error),
                                         localize_string_or_invalid(&pxochat_str_cannot_validate), text);
                ui_choice_popup_show(text, -1, -1, 1, &ok, 0xb, 0, 0);
            }
            break;
        case 3:
            if (pxochat_state == 0) {
                pxochat_state = 6;
                pxochat_build_popup_text(localize_string_or_invalid(&pxochat_str_validating_user),
                                         localize_string_or_invalid(&pxochat_str_blank_login), text);
                ui_choice_popup_show(text, -1, -1, 1, &cancel, 3, 0, 0);
            }
            break;
        case 4:
            if (pxochat_state != 8) {
                pxochat_state = 8;
                pxochat_build_popup_text(localize_string_or_invalid(&pxochat_str_validation_error),
                                         localize_string_or_invalid(&pxochat_str_invalid_user), text);
                ui_choice_popup_show(text, -1, -1, 1, &ok, 0xb, 0, 0);
            }
            break;
        }
    }

    if (pxochat_login_checked && !pxochat_logged_in)
        gameseq_set_state(1);
}

// 0x4238c0  Confidence: exact. Join channel (13) needs a selected channel, Game list (17) a joined one.
void pxochat_update_join_buttons()
{
    if (pxochat_list_count[0] > 0 && strlen(pxochat_selected_channel))
        pxochat_buttons[13].flags &= ~4;
    else
        pxochat_buttons[13].flags |= 4;

    if (lobby_cur_channel < 0)
        pxochat_buttons[17].flags |= 4;
    else
        pxochat_buttons[17].flags &= ~4;
}

// 0x423920  Confidence: likely. Esc leaves at once while the lobby is not connected with channels (or
// is joining one).
int pxochat_escape_leaves(int key)
{
    if (!((lobby_flags & 1) && lobby_num_channels > 0 && !(lobby_flags & 4)) && key == KEY_ESC)
        return 1;
    return 0;
}

// 0x423950  Confidence: likely (three of the strcpy calls share one tail in the binary).
void pxochat_update_status_text()
{
    if (lobby_flags & 0x80000000) {
        sprintf(pxochat_status_text, XSTR("Error: %d", 767), lobby_connect_state);
        return;
    }
    if (!(lobby_flags & 1))
        strcpy(pxochat_status_text, XSTR("Connecting to server...", 768));
    else if (lobby_num_channels <= 0)
        strcpy(pxochat_status_text, XSTR("Requesting channels...", 769));
    else if (lobby_flags & 4)
        strcpy(pxochat_status_text, XSTR("Joining channel...", 770));
    else if (lobby_cur_channel >= 0)
        sprintf(pxochat_status_text, XSTR("\"%s\" channel", 771), lobby_channels[lobby_cur_channel].topic);
    else
        strcpy(pxochat_status_text, XSTR("Not connected to channel", 772));
}

// 0x423a20  Confidence: likely (the chat-input part: VC6 threads the store of chat_input_active = false
// straight to the "joined channel" test of the else branch; the "if (!active) return 0" after the key
// handling cannot be reached, as in gamecreate_handle_click; the list loop has no break after a list hit).
// The channel-name popup takes the keys while it is open (16 characters); the chat line takes them once
// a channel is joined (255 characters; Enter sends it, Esc clears it). A double click on a channel joins
// it; a click selects a channel or a user.
int pxochat_handle_click(int key)
{
    int i;

    pxochat_pressed_button = -1;
    pxochat_active_list = -1;
    pxochat_hover_row = -1;

    for (i = 0; i < PXOCHAT_NUM_BUTTONS; i++) {
        if (((pxochat_button_defs[i].flags >> 2) & 1) == pxochat_channel_input_active
            && !(pxochat_buttons[i].flags & 4)
            && (pxochat_button_defs[i].flags & 3)
            && point_in_rect(game_mouse_x, game_mouse_y, pxochat_buttons[i].x, pxochat_buttons[i].y,
                             pxochat_buttons[i].w, pxochat_buttons[i].h)) {
            pxochat_buttons[i].flags |= 2;
            pxochat_pressed_button = i;
            if (pxochat_button_defs[i].flags & 1) {
                if (mouse_was_pressed(0)) {
                    pxochat_state = 1;
                    pxochat_button_timer.invalidate();
                    return key;
                }
            } else if (pxochat_button_defs[i].flags & 2) {
                if (mouse_was_pressed(0)) {
                    pxochat_state = 1;
                    pxochat_button_action(pxochat_pressed_button + 1);
                    pxochat_button_timer.SetTimeout(500);
                    return key;
                }
            }
            break;
        }
    }

    if (pxochat_channel_input_active) {
        if (key) {
            int c = key_to_ascii(key);
            if (c != 0xff) {
                int len = strlen(pxochat_channel_input);
                if (len < 16) {
                    pxochat_channel_input[len] = (char)c;
                    pxochat_channel_input[len + 1] = 0;
                    pxochat_channel_cursor_timer.SetTimeout(500);
                    pxochat_channel_cursor_visible = true;
                    return 0;
                }
            } else {
                switch (key) {
                case KEY_ESC:
                    pxochat_input_cancel();
                    return 0;
                case KEY_BACKSPACE: {
                    int len = strlen(pxochat_channel_input);
                    if (len > 0) {
                        pxochat_channel_input[len - 1] = 0;
                        pxochat_channel_cursor_timer.SetTimeout(500);
                        pxochat_channel_cursor_visible = true;
                        return 0;
                    }
                    break;
                }
                case KEY_ENTER:
                    pxochat_join_typed_channel();
                    return 0;
                }
            }
        }
        return key;
    }

    for (i = 0; i < 3; i++) {
        if (point_in_rect(game_mouse_x, game_mouse_y, pxochat_thumb_pos[i][0], pxochat_thumb_pos[i][1],
                          pxochat_thumb_size[i][0], pxochat_thumb_size[i][1])
            && mouse_was_pressed(0)) {
            pxochat_state = 2;
            pxochat_active_list = i;
            pxochat_drag_start_mouse_y = game_mouse_y;
            pxochat_drag_start_thumb_y = pxochat_thumb_pos[i][1];
            return key;
        }
    }

    {
        int font_h = gr_get_font_height(-1);
        int list;

        for (list = 0; list < 3; list++) {
            int rows = pxochat_list_count[list];

            if (rows >= pxochat_list_page[resolution_index][list])
                rows = pxochat_list_page[resolution_index][list];

            if (point_in_rect(game_mouse_x, game_mouse_y, pxochat_list_pos[resolution_index][list][0],
                              pxochat_list_pos[resolution_index][list][1], pxochat_list_width[resolution_index][list],
                              rows * font_h)) {
                pxochat_active_list = list;
                if (list != 2) {
                    int row, y;

                    for (row = 0, y = 0; row < rows; row++, y += font_h) {
                        if (!point_in_rect(game_mouse_x, game_mouse_y, pxochat_list_pos[resolution_index][list][0],
                                           pxochat_list_pos[resolution_index][list][1] + y,
                                           pxochat_list_width[resolution_index][list], font_h))
                            continue;
                        pxochat_hover_row = row;
                        if (mouse_was_double_clicked(0)) {
                            if (list == 0) {
                                strcpy(pxochat_selected_channel, lobby_channels[pxochat_scroll_pos[0] + row].name);
                                pxochat_join_selected_channel();
                                game_play_sound(0x23, 1.0f, 0, 0, 0);
                                return key;
                            }
                        } else if (mouse_was_pressed(0)) {
                            switch (list) {
                            case 0:
                                strcpy(pxochat_selected_channel, lobby_channels[pxochat_scroll_pos[0] + row].name);
                                break;
                            case 1:
                                strcpy(pxochat_selected_user, lobby_users[pxochat_scroll_pos[1] + row]);
                                break;
                            }
                            return key;
                        }
                    }
                }
            }
        }
    }

    if (lobby_cur_channel < 0)
        pxochat_chat_input_active = false;

    if (pxochat_chat_input_active) {
        if (key) {
            int c = key_to_ascii(key);
            if (c != 0xff) {
                int len = strlen(pxochat_chat_input);
                if (len < 255) {
                    pxochat_chat_input[len] = (char)c;
                    pxochat_chat_input[len + 1] = 0;
                    pxochat_chat_cursor_timer.SetTimeout(500);
                    pxochat_chat_cursor_visible = true;
                    return 0;
                }
            } else {
                switch (key) {
                case KEY_ESC:
                    pxochat_chat_input[0] = 0;
                    pxochat_chat_input_active = false;
                    return 0;
                case KEY_BACKSPACE: {
                    int len = strlen(pxochat_chat_input);
                    if (len > 0) {
                        pxochat_chat_input[len - 1] = 0;
                        pxochat_chat_cursor_timer.SetTimeout(500);
                        pxochat_chat_cursor_visible = true;
                        return 0;
                    }
                    break;
                }
                case KEY_ENTER: {
                    char line[256];

                    trim_trailing_whitespace(line, pxochat_chat_input);
                    if (strlen(line))
                        lobby_send_chat_input(line);
                    pxochat_chat_input[0] = 0;
                    return 0;
                }
                }
            }
        }
        if (!pxochat_chat_input_active)
            return 0;
    } else if (lobby_cur_channel >= 0 && mouse_was_pressed(0)
               && point_in_rect(game_mouse_x, game_mouse_y, pxochat_chat_input_pos[resolution_index][0],
                                pxochat_chat_input_pos[resolution_index][1],
                                pxochat_chat_input_size[resolution_index][0],
                                pxochat_chat_input_size[resolution_index][1])) {
        pxochat_chat_input[0] = 0;
        pxochat_chat_cursor_timer.SetTimeout(500);
        pxochat_chat_cursor_visible = true;
        pxochat_chat_input_active = true;
        return key;
    }

    if (key == KEY_ESC) {
        pxochat_goto_charselect();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        return 0;
    }

    {
        int dx, dy, dz;
        mouse_get_delta(&dx, &dy, &dz);
        if (dz > 0)
            pxochat_page_up(pxochat_active_list);
        else if (dz < 0)
            pxochat_page_down(pxochat_active_list);
    }
    return key;
}

// 0x424120  Confidence: likely (two stores of the element, as mp_charselect_page_up).
void pxochat_page_up(int list)
{
    if (list == -1)
        return;
    if (pxochat_scroll_pos[list] > 0) {
        pxochat_scroll_pos[list] -= pxochat_list_page[resolution_index][list];
        pxochat_scroll_pos[list] = pxochat_scroll_pos[list] > 0 ? pxochat_scroll_pos[list] : 0;
        pxochat_update_scrollbar(list);
    }
}

// 0x424170  Confidence: likely (as mp_charselect_page_down).
void pxochat_page_down(int list)
{
    if (list == -1)
        return;
    int last = pxochat_list_count[list] - pxochat_list_page[resolution_index][list];
    if (pxochat_scroll_pos[list] < last) {
        pxochat_scroll_pos[list] += pxochat_list_page[resolution_index][list];
        pxochat_scroll_pos[list] = pxochat_scroll_pos[list] < last ? pxochat_scroll_pos[list] : last;
        pxochat_update_scrollbar(list);
    }
}

// 0x4241c0  Confidence: exact. Back to the character select (state 10).
void pxochat_goto_charselect()
{
    gameseq_set_state(10);
}

// 0x4241d0  Confidence: likely (byte table 0x424300 over button - 2, 0..31, into the jump table 0x4242c4;
// the scroll cases share one sound call; 22 and 24 share the exit case, 31 and 33 the close case; 29 / 31 /
// 33 play the sound before the tail-called action). The banner (26) acts only when it has a link.
void pxochat_button_action(int button)
{
    switch (button) {
    case 2:
        pxochat_scroll_up(0);
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 4:
        pxochat_scroll_down(0);
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 6:
        pxochat_scroll_up(1);
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 8:
        pxochat_scroll_down(1);
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 10:
        pxochat_scroll_up(2);
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 12:
        pxochat_scroll_down(2);
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 14:
        pxochat_join_selected_channel();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 16:
        pxochat_input_begin();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 18:
        pxochat_goto_gamelist();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 20:
        pxochat_show_motd();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 22:
    case 24:
        pxochat_goto_charselect();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    case 26:
        if (pxochat_banner_has_url()) {
            pxochat_open_banner_url();
            game_play_sound(0x23, 1.0f, 0, 0, 0);
        }
        break;
    case 29:
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        pxochat_join_typed_channel();
        break;
    case 31:
    case 33:
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        pxochat_input_cancel();
        break;
    }
}

// 0x424320  Confidence: likely (store order around the timer call). Opens the channel-name popup; the
// chat line's focus is saved and given back by pxochat_input_cancel.
void pxochat_input_begin()
{
    pxochat_channel_input_active = true;
    pxochat_channel_input[0] = 0;
    pxochat_channel_cursor_timer.SetTimeout(500);
    pxochat_channel_cursor_visible = true;
    pxochat_channel_input_unk = true;
    pxochat_saved_chat_input = pxochat_chat_input_active;
    pxochat_chat_input_active = false;
}

// 0x424360  Confidence: exact.
void pxochat_scroll_up(int list)
{
    if (list == -1)
        return;
    if (pxochat_scroll_pos[list] > 0) {
        pxochat_scroll_pos[list]--;
        pxochat_update_scrollbar(list);
    }
}

// 0x424390  Confidence: exact.
void pxochat_scroll_down(int list)
{
    if (list == -1)
        return;
    if (pxochat_scroll_pos[list] < pxochat_list_count[list] - pxochat_list_page[resolution_index][list]) {
        pxochat_scroll_pos[list]++;
        pxochat_update_scrollbar(list);
    }
}

// 0x4243e0  Confidence: exact. On to the game list (state 12) once a channel is joined.
void pxochat_goto_gamelist()
{
    if (lobby_cur_channel >= 0)
        gameseq_set_state(12);
}

// 0x424400  Confidence: exact.
void pxochat_show_motd()
{
    pxochat_add_chat_line(pxochat_motd, 2);
}

// 0x424410  Confidence: likely (the banner entry 25 is its own hit area, so the state entry is the pressed
// one for 25 and the next one otherwise; the action always gets pressed + 1). As gamelist_update_buttons.
void pxochat_update_buttons()
{
    int pressed = pxochat_pressed_button;
    int button = pressed + 1;

    if (pressed >= 0 && pressed < PXOCHAT_NUM_BUTTONS) {
        ui_button_state *b = &pxochat_buttons[button + (pressed == 25 ? -1 : 0)];

        if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
            b->flags |= 2;

        if (mouse_is_down(0)) {
            if (pxochat_button_timer.valid() && pxochat_button_timer.elapsed() && (b->flags & 2)) {
                pxochat_button_action(button);
                pxochat_button_timer.SetTimeout(100);
            }
            return;
        }

        if ((b->flags & 2) && !pxochat_button_timer.valid())
            pxochat_button_action(button);
    }
    pxochat_state = 0;
}

// 0x4244f0  Confidence: likely (as mp_charselect_drag_scrollbar, three stores of the element).
void pxochat_drag_scrollbar()
{
    int list = pxochat_active_list;
    int count = pxochat_list_count[list];

    pxochat_scroll_pos[list] = (int)((float)(game_mouse_y - pxochat_track_pos[resolution_index][list][1]
                                             - pxochat_drag_start_mouse_y + pxochat_drag_start_thumb_y)
                                     / pxochat_track_h[resolution_index][list] * count + 0.5f);
    pxochat_scroll_pos[list] = pxochat_scroll_pos[list] > 0 ? pxochat_scroll_pos[list] : 0;
    count -= pxochat_list_page[resolution_index][list];
    pxochat_scroll_pos[list] = pxochat_scroll_pos[list] < count ? pxochat_scroll_pos[list] : count;
    pxochat_update_scrollbar(list);

    if (!mouse_is_down(0))
        pxochat_state = 0;
}

// 0x4245b0  Confidence: exact. Opens the chat line once the requested channel is joined.
void pxochat_on_channel_joined()
{
    if (pxochat_join_pending && (lobby_flags & 1) && lobby_cur_channel >= 0) {
        pxochat_chat_input[0] = 0;
        pxochat_chat_cursor_timer.SetTimeout(500);
        pxochat_chat_cursor_visible = true;
        pxochat_chat_input_active = true;
        pxochat_join_pending = false;
    }
}

// 0x424600  Confidence: likely (the cursor blink chooses the string in an if / else, as
// gamecreate_draw_text_field; 36-byte buffer).
void pxochat_draw_channel_input()
{
    int x = pxochat_channel_input_pos[resolution_index][0];
    int y = pxochat_channel_input_pos[resolution_index][1];
    char buf[36];

    strcpy(buf, pxochat_channel_input);
    strcat(buf, "|");
    gr_set_color_ptr(&ui_color_text);
    if (pxochat_channel_cursor_visible)
        gr_string(x, y, buf, -1, gr_string_mode);
    else
        gr_string(x, y, pxochat_channel_input, -1, gr_string_mode);
}

// 0x4246d0  Confidence: likely (the second channel_input_active test is written as its own if). The popup
// entries are drawn again on top of the lists while the popup is open; the choice popup over everything
// in states 3..9.
void pxochat_draw()
{
    int i;

    gr_set_color(255, 255, 255, 255);
    for (i = 0; i < PXOCHAT_NUM_BUTTONS; i++) {
        if ((pxochat_channel_input_active || !(pxochat_button_defs[i].flags & 4))
            && (pxochat_buttons[i].flags & 3))
            ui_bitmap_scaled(pxochat_buttons[i].bitmap, pxochat_buttons[i].x, pxochat_buttons[i].y,
                             gr_bitmap_wrap_mode);
    }
    pxochat_draw_banner_highlight();
    pxochat_draw_channel_list();
    pxochat_draw_user_list();
    pxochat_draw_chat_lines();
    for (i = 0; i < 3; i++)
        pxochat_draw_scrollbar(i);
    pxochat_draw_chat_input();
    pxochat_draw_caption();

    if (pxochat_channel_input_active) {
        for (i = 0; i < PXOCHAT_NUM_BUTTONS; i++) {
            if ((pxochat_button_defs[i].flags & 4) && (pxochat_buttons[i].flags & 3))
                ui_bitmap_scaled(pxochat_buttons[i].bitmap, pxochat_buttons[i].x, pxochat_buttons[i].y,
                                 gr_bitmap_wrap_mode);
        }
    }
    if (pxochat_channel_input_active)
        pxochat_draw_channel_input();

    if (pxochat_state >= 3 && pxochat_state <= 9)
        ui_choice_popup_draw();
}

// 0x4247d0  Confidence: exact. A one-pixel frame round a clickable banner under the mouse.
void pxochat_draw_banner_highlight()
{
    gr_set_color(255, 255, 255, 255);
    if (point_in_rect(game_mouse_x, game_mouse_y, pxochat_buttons[25].x, pxochat_buttons[25].y,
                      pxochat_buttons[25].w, pxochat_buttons[25].h)
        && pxochat_banner_has_url()) {
        int x = pxochat_banner_pos[resolution_index][0];
        int y = pxochat_banner_pos[resolution_index][1];
        int w = pxochat_banner_size[resolution_index][0];
        int h = pxochat_banner_size[resolution_index][1];

        gr_rect(x, y, w, 1, gr_rect_mode, 1.0f);
        gr_rect(x, y + h, w, 1, gr_rect_mode, 1.0f);
        gr_rect(x, y, 1, h, gr_rect_mode, 1.0f);
        gr_rect(x + w, y, 1, h, gr_rect_mode, 1.0f);
    }
}

// 0x4248c0  Confidence: likely (local order; strcmp inlined). Topic, then the two counts.
void pxochat_draw_channel_list()
{
    int font_h = gr_get_font_height(-1);
    int rows = pxochat_list_count[0];
    int x, y, row;
    char buf[16];
    int w, h;

    if (rows >= pxochat_list_page[resolution_index][0])
        rows = pxochat_list_page[resolution_index][0];
    x = pxochat_list_pos[resolution_index][0][0];
    y = pxochat_list_pos[resolution_index][0][1];

    for (row = 0; row < rows; row++) {
        lobby_channel *ch = &lobby_channels[pxochat_scroll_pos[0] + row];

        if ((pxochat_active_list == 0 && row == pxochat_hover_row) || !strcmp(ch->name, pxochat_selected_channel))
            gr_set_color_ptr(&ui_color_highlight);
        else
            gr_set_color_ptr(&ui_color_text);
        gr_string(x, y, ch->topic, -1, gr_string_mode);

        sprintf(buf, "%d", ch->num_users);
        gr_get_string_size(&w, &h, buf, -1, -1);
        gr_string(pxochat_channel_users_right[resolution_index] - w + x, y, buf, -1, gr_string_mode);

        sprintf(buf, "%d", ch->unk8c);
        gr_string(x + pxochat_channel_count_x[resolution_index], y, buf, -1, gr_string_mode);

        y += font_h;
    }
}

// 0x424a60  Confidence: likely (local order; strcmp inlined).
void pxochat_draw_user_list()
{
    int font_h = gr_get_font_height(-1);
    int rows = pxochat_list_count[1];
    int x, y, row;
    char buf[64];

    if (rows >= pxochat_list_page[resolution_index][1])
        rows = pxochat_list_page[resolution_index][1];
    x = pxochat_list_pos[resolution_index][1][0];
    y = pxochat_list_pos[resolution_index][1][1];

    for (row = 0; row < rows; row++) {
        char *name = lobby_users[pxochat_scroll_pos[1] + row];

        if ((pxochat_active_list == 1 && row == pxochat_hover_row) || !strcmp(name, pxochat_selected_user))
            gr_set_color_ptr(&ui_color_highlight);
        else
            gr_set_color_ptr(&ui_color_text);
        gr_string_truncate_ellipsis(buf, name, pxochat_list_width[resolution_index][1], -1);
        gr_string(x, y, buf, -1, gr_string_mode);
        y += font_h;
    }
}

// 0x424b80  Confidence: likely (jump table 0x424de0 over type - 1, 0..5: case labels 1 and 6 exist, 5 is
// handled before; two separate name buffers). Types 0 / 5: the text up to and with the first space is the
// speaker, drawn blue (0) or highlighted (5).
void pxochat_draw_chat_lines()
{
    int font_h = gr_get_font_height(-1);
    int rows = pxochat_list_count[2];
    int x, y, row;
    int w, h;

    if (rows >= pxochat_list_page[resolution_index][2])
        rows = pxochat_list_page[resolution_index][2];
    x = pxochat_list_pos[resolution_index][2][0];
    y = pxochat_list_pos[resolution_index][2][1];

    for (row = 0; row < rows; row++) {
        pxochat_line *line = &pxochat_chat_lines[pxochat_scroll_pos[2] + row];

        if (line->type == 0) {
            char *space = strchr(line->text, ' ');
            if (space) {
                char name[72];
                int len = space - line->text + 1;

                strncpy(name, line->text, len);
                name[len] = 0;
                gr_get_string_size(&w, &h, name, -1, -1);
                gr_set_color_ptr(&ui_color_blue);
                gr_string(x, y, name, -1, gr_string_mode);
                gr_set_color_ptr(&ui_color_text);
                gr_string(x + w, y, line->text + len, -1, gr_string_mode);
            } else {
                gr_set_color_ptr(&ui_color_blue);
                gr_string(x, y, line->text, -1, gr_string_mode);
            }
        } else if (line->type == 5) {
            char *space = strchr(line->text, ' ');
            if (space) {
                char name[72];
                int len = space - line->text + 1;

                strncpy(name, line->text, len);
                name[len] = 0;
                gr_get_string_size(&w, &h, name, -1, -1);
                gr_set_color_ptr(&ui_color_highlight);
                gr_string(x, y, name, -1, gr_string_mode);
                gr_set_color_ptr(&ui_color_text);
                gr_string(x + w, y, line->text + len, -1, gr_string_mode);
            } else {
                gr_set_color_ptr(&ui_color_blue);
                gr_string(x, y, line->text, -1, gr_string_mode);
            }
        } else {
            switch (line->type) {
            case 2:
                gr_set_color_ptr(&ui_color_gold);
                break;
            case 3:
                gr_set_color_ptr(&ui_color_magenta);
                break;
            case 4:
                gr_set_color_ptr(&ui_color_green);
                break;
            case 1:
            case 6:
            default:
                gr_set_color_ptr(&ui_color_text);
                break;
            }
            gr_string(x, y, line->text, -1, gr_string_mode);
        }
        y += font_h;
    }
}

// 0x424e00  Confidence: likely (as mp_charselect_draw_scrollbar).
void pxochat_draw_scrollbar(int list)
{
    int tiles, i;

    if (list == -1 || !pxochat_scrollbar_visible[list])
        return;

    gr_set_color(255, 255, 255, 255);
    tiles = pxochat_thumb_size[list][1] / pxochat_thumb_bm_h;
    for (i = 0; i < tiles; i++)
        ui_bitmap_scaled(pxochat_scrollbar_bm, pxochat_thumb_pos[list][0],
                         pxochat_thumb_bm_h * i + pxochat_thumb_pos[list][1], gr_bitmap_wrap_mode);
    ui_bitmap_scaled(pxochat_scrollbar_bm, pxochat_thumb_pos[list][0],
                     pxochat_thumb_pos[list][1] - pxochat_thumb_bm_h + pxochat_thumb_size[list][1],
                     gr_bitmap_wrap_mode);
}

// 0x424ec0  Confidence: likely (as gamelist_draw_password_input; 260-byte buffer).
void pxochat_draw_chat_input()
{
    int clip_x, clip_y, clip_w, clip_h;
    char buf[260];
    int w, h;
    int x;

    if (!pxochat_chat_input_active)
        return;

    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    gr__set_clip(pxochat_chat_input_pos[resolution_index][0], pxochat_chat_input_pos[resolution_index][1],
                 pxochat_chat_input_size[resolution_index][0], pxochat_chat_input_size[resolution_index][1]);

    strcpy(buf, pxochat_chat_input);
    strcat(buf, "|");
    gr_get_string_size(&w, &h, buf, -1, -1);
    x = pxochat_chat_input_size[resolution_index][0] - w;
    x = x > 0 ? 0 : x;
    gr_set_color_ptr(&ui_color_text);
    gr_string(x, 0, pxochat_chat_cursor_visible ? buf : pxochat_chat_input, -1, gr_string_mode);

    gr__set_clip(clip_x, clip_y, clip_w, clip_h);
}

// 0x425010  Confidence: exact. The status text, centred on the caption position.
void pxochat_draw_caption()
{
    int w, h;

    gr_get_string_size(&w, &h, pxochat_status_text, -1, -1);
    gr_set_color_ptr(&ui_color_text);
    gr_string(pxochat_caption_pos[resolution_index][0] - w / 2, pxochat_caption_pos[resolution_index][1],
              pxochat_status_text, -1, gr_string_mode);
}

// 0x425080  Confidence: likely (VC6 duplicates the clear at the end). Drops the selection when its
// channel is gone.
void pxochat_sync_channel_list()
{
    int i;

    pxochat_list_count[0] = lobby_num_channels;
    pxochat_update_scrollbar(0);
    for (i = 0; i < lobby_num_channels; i++) {
        if (!strcmp(pxochat_selected_channel, lobby_channels[i].name))
            break;
    }
    if (i >= lobby_num_channels)
        pxochat_selected_channel[0] = 0;
}

// 0x425110  Confidence: likely (as pxochat_sync_channel_list).
void pxochat_sync_user_list()
{
    int i;

    pxochat_list_count[1] = lobby_num_users;
    pxochat_update_scrollbar(1);
    for (i = 0; i < lobby_num_users; i++) {
        if (!strcmp(pxochat_selected_user, lobby_users[i]))
            break;
    }
    if (i >= lobby_num_users)
        pxochat_selected_user[0] = 0;
}

// 0x4251a0  Confidence: likely (the bound spelling "< 1024"). Appends the text after "!MOTD$#!" and one
// separator character, with a newline, while it fits.
void pxochat_append_motd(const char *line)
{
    int prefix, len;

    if (!line)
        return;
    prefix = strlen(PXO_MOTD_PREFIX);
    len = strlen(line) - prefix - 1;
    if (len > 0 && strlen(pxochat_motd) + len < 1024) {
        strcat(pxochat_motd, line + prefix + 1);
        strcat(pxochat_motd, "\n");
    }
}

// 0x425250  Confidence: likely (a one-call function, tail jump; name of the source function not known).
void pxochat_show_motd_thunk()
{
    pxochat_show_motd();
}

// 0x425260  Confidence: exact.
void pxochat_clear_motd()
{
    pxochat_motd[0] = 0;
}

// 0x425270  Confidence: likely (jump table 0x4253c0 over the type 0..6 inside the line loop; an unknown
// type leaves without the scroll update). Wraps the text to the chat width (at most 16 lines, none longer
// than 63 characters), drops the oldest lines beyond 64 and shows the last page. Continuation lines of
// types 0 / 5 become 1 / 6.
void pxochat_add_chat_line(const char *text, int type)
{
    int lengths[16];
    int starts[16];
    int n, i, excess, pos;

    n = gr_split_str(lengths, starts, text, pxochat_list_width[resolution_index][2], 16, 0, -1);
    for (i = 0; i < n; i++) {
        if (lengths[i] > 63)
            return;
    }

    excess = pxochat_list_count[2] + n - PXOCHAT_MAX_LINES;
    if (excess > 0) {
        memmove(pxochat_chat_lines, &pxochat_chat_lines[excess],
                (pxochat_list_count[2] - excess) * sizeof(pxochat_line));
        pxochat_list_count[2] -= excess;
    }

    for (i = 0; i < n; i++) {
        pxochat_line *line = &pxochat_chat_lines[pxochat_list_count[2]];

        switch (type) {
        case 0:
        case 1:
            line->type = i != 0;
            break;
        case 5:
        case 6:
            line->type = (i != 0) + 5;
            break;
        case 2:
        case 3:
        case 4:
            line->type = type;
            break;
        default:
            return;
        }
        strncpy(line->text, text + starts[i], lengths[i]);
        line->text[lengths[i]] = 0;
        pxochat_list_count[2]++;
    }

    pos = pxochat_list_count[2] - pxochat_list_page[resolution_index][2];
    pxochat_scroll_pos[2] = pos < 0 ? 0 : pos;
    pxochat_update_scrollbar(2);
}

// 0x4253e0  Confidence: exact.
void pxochat_reset_lists()
{
    int i;

    for (i = 0; i < 3; i++) {
        pxochat_list_count[i] = 0;
        pxochat_scroll_pos[i] = 0;
        pxochat_update_scrollbar(i);
    }
    pxochat_clear_motd();
}
