// summoner/unnamed_mlayout_0x43f9d0.h - game state 6 (a character picker window: a button that opens a
// list box of every char_infos name) and the monster layout files (.mlo; the load_mlayout / save_mlayout
// console commands), 0x43f9d0-0x4403f2. Source: game/unnamed_mlayout_0x43f9d0.cpp. THE FILE NAME IS A
// PLACEHOLDER (no assert path, no strings.tbl block; decomp53). Not original source.
#ifndef SUMMONER_UNNAMED_MLAYOUT_0X43F9D0_H
#define SUMMONER_UNNAMED_MLAYOUT_0X43F9D0_H

#include "vsdk/vsdk.h"
#include "vsdk/ui/window.h"
#include "vsdk/ui/ui_gadgets.h"
#include "vsdk/os/console.h"

// .bss
extern ui_listbox state6_listbox;                       // 0x60f1d0
extern console_command ccmd_save_mlayout;               // 0x60f4f0
extern ui_button state6_button;                         // 0x60f500
extern console_command ccmd_load_mlayout;               // 0x60f928
extern ui_window state6_window;                         // 0x60f938
extern int state6_selection;                            // 0x60fe24 char_infos index

bool state6_button_cb(int event, int value);            // 0x43fa90
bool state6_listbox_cb(int event, int value);           // 0x43faf0
void state6_enter();                                    // 0x43fb60
void state6_leave();                                    // 0x43fc70
void state6_frame();                                    // 0x43fc80
// 0 success (File::close's result), -1 when the file cannot be opened.
int  monster_layout_save(const char *filename);         // 0x43fc90
// 0 success, -1 when the file cannot be opened, is obsolete or is for another map.
int  monster_layout_load(const char *filename);         // 0x43fe00
void cmd_load_mlayout();                                // 0x4401a0
void cmd_save_mlayout();                                // 0x4402f0

#endif
