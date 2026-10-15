// new_mainmenu.h - screen 14, the main menu (new_mainmenu.cpp, 0x41d740-0x41ddde).
#ifndef NEW_MAINMENU_H
#define NEW_MAINMENU_H

void mainmenu_load_bitmaps();                       // 0x41d740
void mainmenu_init();                               // 0x41d8b0
int  mainmenu_frame(int key);                       // 0x41d910
int  mainmenu_handle_click(int key);                // 0x41d960
void mainmenu_start_new_game();                     // 0x41dac0 (also the play menu's "New game" callback)
void mainmenu_check_network();                      // 0x41dae0
void mainmenu_check_network_cb(int button);         // 0x41dbc0
void mainmenu_check_network_cb2(int button);        // 0x41dbd0
void ui_open_options();                             // 0x41dbf0 (also the play menu's Options button)
void mainmenu_quit();                               // 0x41dc00
void mainmenu_update_buttons();                     // 0x41dc10
void mainmenu_button_action(int button);            // 0x41dca0
void mainmenu_draw();                               // 0x41dd90

#endif
