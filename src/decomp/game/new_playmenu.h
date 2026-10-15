// new_playmenu.h - screen 9, the in-game play menu (new_playmenu.cpp, 0x421590-0x421dd3).
#ifndef NEW_PLAYMENU_H
#define NEW_PLAYMENU_H

void playmenu_load_bitmaps();                       // 0x421590
void playmenu_init();                               // 0x4216f0
void playmenu_leave();                              // 0x4217b0
int  playmenu_frame(int key);                       // 0x4217c0
int  playmenu_handle_click(int key);                // 0x421820
void playmenu_update_buttons();                     // 0x421900
void playmenu_button_action(int button);            // 0x4219c0
void playmenu_confirm_new_game();                   // 0x421aa0
void mainmenu_confirm_new_game_cb(int button);      // 0x421ae0
void playmenu_open_save();                          // 0x421b00
void open_saveload_screen();                        // 0x421c80 (also used by the main menu and the HUD)
void playmenu_confirm_quit();                       // 0x421ca0
void mainmenu_confirm_quit_cb(int button);          // 0x421ce0
void playmenu_draw();                               // 0x421cf0
bool playmenu_mouse_in_panel(int x, int y);         // 0x421d40
void game_abandon_to_menu();                        // 0x421d70

#endif
