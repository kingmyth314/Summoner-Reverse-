// new_gamelist.h - screen 15, the multiplayer game list (new_gamelist.cpp, 0x40a660-0x40bf2b).
#ifndef NEW_GAMELIST_H
#define NEW_GAMELIST_H

#include "summoner/summoner.h"
#include "summoner/multi/multi.h"   // mp_gamelist_entry

void gamelist_load_bitmaps();                       // 0x40a6a0
void gamelist_init();                               // 0x40a850
int  gamelist_frame(int key);                       // 0x40a880
void gamelist_update_join_button();                 // 0x40a970
void gamelist_show_join_error(int code);            // 0x40a9b0 (mp_join_result 3..13)
void gamelist_update_scrollbar();                   // 0x40ab80
void gamelist_password_input_begin();               // 0x40ac90
// 0x40acc0 empty_frame_hook_40acc0 (mouse mode 3) is defined in new_gamelist.cpp but declared in
// summoner/summoner.h: identical-code folding made it the one bare RET that 12 callers in other units
// reach (DECISIONS.md D57).
int  gamelist_handle_click(int key);                // 0x40acd0
mp_gamelist_entry *mp_server_list_get(int index);   // 0x40b0b0 NULL past the end
void gamelist_refresh_tooltip(int button);          // 0x40b0e0
void gamelist_page_up();                            // 0x40b120
void gamelist_page_down();                          // 0x40b150
void gamelist_join_selected();                      // 0x40b180
void gamelist_back();                               // 0x40b1b0
void gamelist_password_input_cancel();              // 0x40b1d0
void gamelist_button_action(int button);            // 0x40b1e0
void gamelist_scroll_up();                          // 0x40b340
void gamelist_scroll_down();                        // 0x40b360
void gamelist_refresh();                            // 0x40b390
void gamelist_create_game();                        // 0x40b3b0
void gamelist_create_game_cb(int button);           // 0x40b3c0 message box callback (argument unused)
void gamelist_update_buttons();                     // 0x40b3d0
void gamelist_drag_scrollbar();                     // 0x40b4a0
void gamelist_draw();                               // 0x40b530
void mp_server_list_draw();                         // 0x40b5c0
void gamelist_draw_scrollbar();                     // 0x40bca0
void gamelist_draw_password_input();                // 0x40bd50
void mp_draw_join_help();                           // 0x40be80

#endif
