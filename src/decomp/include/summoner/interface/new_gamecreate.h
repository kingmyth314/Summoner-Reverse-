// new_gamecreate.h - screen 17, multiplayer game creation (new_gamecreate.cpp, 0x4099b0-0x40a65f).
#ifndef NEW_GAMECREATE_H
#define NEW_GAMECREATE_H

void gamecreate_load_bitmaps();                     // 0x4099d0
void gamecreate_init();                             // 0x409b10
void gamecreate_set_difficulty(int difficulty);     // 0x409c60
int  gamecreate_frame(int key);                     // 0x409cd0
void gamecreate_update_create_button();             // 0x409db0
int  gamecreate_handle_click(int key);              // 0x409de0
void gamecreate_cancel();                           // 0x40a020
void gamecreate_update_buttons();                   // 0x40a030
void gamecreate_button_action(int button);          // 0x40a0c0
void gamecreate_players_dec();                      // 0x40a230
void gamecreate_players_inc();                      // 0x40a250
void gamecreate_levels_above_dec();                 // 0x40a270
void gamecreate_levels_above_inc();                 // 0x40a280
void gamecreate_levels_below_dec();                 // 0x40a2a0
void gamecreate_levels_below_inc();                 // 0x40a2b0
void gamecreate_create_game();                      // 0x40a2d0
void gamecreate_draw();                             // 0x40a310
void gamecreate_draw_text_field(int field);         // 0x40a380
void gamecreate_draw_player_count();                // 0x40a4f0
void gamecreate_draw_limits();                      // 0x40a570

#endif
