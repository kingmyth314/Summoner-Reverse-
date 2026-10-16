// new_pxo_banner.cpp - Sum.exe v1.40, code 0x421de0-0x421e67 (open_url and its jump table). Written from
// the Sum.exe disassembly (tools/asm.mjs) by decomp76. Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (one detail of
// statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit: strings.tbl id 834 is the only string of the block "// new_pxo_banner.cpp", and open_url reads it;
// sum.symbols tags open_url with the file. The function sits between new_playmenu.cpp (game_abandon_to_menu
// 0x421d70) and new_pxochat.cpp (pxochat_open_banner_url 0x421e70, its caller). .data: the two rectangles
// at 0x58f6f8 (nothing references them by address; they precede the unit's literal "open" 0x58f718, where
// VC6 puts a unit's variables, so they are probably this unit's: unsure) and "open".

#include <windows.h>
#include <shellapi.h>
#include "vsdk/localize/localize.h"
#include "summoner/interface/interface.h"
#include "summoner/interface/new_pxo_banner.h"

// ---- .data (0x58f6f8) -------------------------------------------------------------------------------
// {x, y, w, h}: two banner-sized rectangles side by side (the banners of the PXO chat are 475 x 75; name
// and meaning ours, no reference).
int pxo_banner_rects[2][4] = { { 149, 3, 475, 75 }, { 524, 3, 475, 75 } };   // 0x58f6f8

// 0x421de0  Confidence: exact (VC6 byte match; the switch includes 32, which the < 32 test excludes).
// ShellExecute's error results (0 and SE_ERR_* / ERROR_* 2-31) show the warning box with an Ok button; the
// button list lives in the dead url argument slot.
void open_url(const char *url)
{
    int r = (int)ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOW);

    if (r < 32) {
        switch (r) {
        case 0:
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        case SE_ERR_ACCESSDENIED:
        case SE_ERR_OOM:
        case ERROR_BAD_FORMAT:
        case SE_ERR_SHARE:
        case SE_ERR_ASSOCINCOMPLETE:
        case SE_ERR_DDETIMEOUT:
        case SE_ERR_DDEFAIL:
        case SE_ERR_DDEBUSY:
        case SE_ERR_NOASSOC:
        case SE_ERR_DLLNOTFOUND: {
            char *ok = XSTR("Ok", 213);
            ui_message_box_show(XSTR("Warning", 645), XSTR("Could not locate/launch default Internet Browser", 834),
                                -1, -1, 1, &ok, NULL, 0);
            break;
        }
        }
    }
}
