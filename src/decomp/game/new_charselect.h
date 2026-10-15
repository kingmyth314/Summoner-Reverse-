// new_charselect.h - screen 16, multiplayer character select / create (new_charselect.cpp, 0x4063b0-0x408582).
#ifndef NEW_CHARSELECT_H
#define NEW_CHARSELECT_H

struct mp_char_profile;   // stubs_gamecreate_journal.h

void mp_charcreate_load_bitmaps();                  // 0x4063f0
void mp_charselect_init();                          // 0x406590
void mp_char_scan_files();                          // 0x406710
void mp_charselect_update_scrollbar(int list);      // 0x406820 (0 characters, 1 description input, 2 info, -1 none)
void mp_charselect_select_species(int species);     // 0x406970
int  mp_charselect_frame(int key);                  // 0x4069e0
void mp_charselect_update_button_states();          // 0x406b40
void mp_charselect_update_create_button();          // 0x406bb0
int  mp_charselect_handle_click(int key);           // 0x406bf0
void mp_charselect_wrap_text_input();               // 0x4072e0
void mp_charselect_page_up(int list);               // 0x4073c0
void mp_charselect_page_down(int list);             // 0x407410
void mp_charselect_save_choice();                   // 0x407460
void mp_charselect_back();                          // 0x4074b0
void mp_charselect_clear_text_input();              // 0x4074d0
void mp_charselect_button_action(int button);       // 0x4074e0
void mp_charselect_scroll_up(int list);             // 0x407650
void mp_charselect_scroll_down(int list);           // 0x407680
void mp_char_create();                              // 0x4076d0
void mp_char_confirm_delete();                      // 0x4078b0
void mp_char_confirm_delete_cb(int button);         // 0x4078f0
void mp_char_delete_file();                         // 0x407900
void mp_char_format_info();                         // 0x4079c0
void mp_charselect_update_buttons();                // 0x407d30
void mp_charselect_drag_scrollbar();                // 0x407e10
void mp_charselect_draw();                          // 0x407ed0
void mp_charselect_draw_char_list();                // 0x407fd0
void mp_charselect_draw_scrollbar(int list);        // 0x4080c0
void mp_charselect_draw_name_input();               // 0x408180
void mp_charselect_draw_text_input();               // 0x4082d0
void mp_charselect_draw_char_info();                // 0x408490
// mp_charselect_get_selected_profile (0x408540) is declared in stubs_gamecreate_journal.h as
// (int validate); the binary tests the low byte only (bool), see NOTES.

#endif
