// new_levelselect.h - screen 18, multiplayer lobby / level select (new_levelselect.cpp, 0x41bea0-0x41d72b).
#ifndef NEW_LEVELSELECT_H
#define NEW_LEVELSELECT_H

void mp_levelselect_load_bitmaps();                 // 0x41bee0
void mp_levelselect_init();                         // 0x41c080
void mp_levelselect_update_scrollbar(int list);     // 0x41c110 (0 chat, 1 levels, -1 none)
void mp_levelselect_build_list();                   // 0x41c250
int  mp_levelselect_frame(int key);                 // 0x41c2a0
void mp_levelselect_sync_selection();               // 0x41c330
void mp_levelselect_update_kick_button();           // 0x41c380
int  mp_levelselect_handle_click(int key);          // 0x41c450
void mp_levelselect_back();                         // 0x41cad0
void mp_levelselect_page_up(int list);              // 0x41cb20
void mp_levelselect_page_down(int list);            // 0x41cb70
void mp_levelselect_start_thunk();                  // 0x41cbc0
void mp_levelselect_button_action(int button);      // 0x41cbd0
void mp_levelselect_scroll_up(int list);            // 0x41cc90
void mp_levelselect_scroll_down(int list);          // 0x41ccc0
void mp_levelselect_kick_selected_player();         // 0x41cd10
void mp_levelselect_update_buttons();               // 0x41cd80
void mp_levelselect_drag_scrollbar();               // 0x41ce50
void mp_levelselect_draw();                         // 0x41cf00
void mp_levelselect_draw_chat();                    // 0x41cf80
void mp_levelselect_draw_levels();                  // 0x41d1d0
void mp_levelselect_draw_scrollbar(int list);       // 0x41d2d0
void mp_levelselect_draw_chat_input();              // 0x41d390
void mp_levelselect_draw_players();                 // 0x41d4e0
void mp_levelselect_add_chat_line(const char *text, int type);   // 0x41d5e0 (type 0..4, see the .cpp)

#endif
