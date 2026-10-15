// new_journal.h - screen 10, the quest journal (new_journal.cpp, 0x41a940-0x41be94).
#ifndef NEW_JOURNAL_H
#define NEW_JOURNAL_H

#include "types.h"   // quest_info

void journal_load_bitmaps();                        // 0x41a960
void journal_init();                                // 0x41ab10
void journal_update_list_scrollbar(int list);       // 0x41abf0 (0 main quests, 1 side quests, -1 none)
void journal_select_quest(quest_info *quest);       // 0x41ad40
void journal_update_desc_scrollbar();               // 0x41ade0
void journal_leave();                               // 0x41aef0
int  journal_frame(int key);                        // 0x41af30
int  journal_handle_click(int key);                 // 0x41af80
quest_info *journal_get_quest(int list, int n);     // 0x41b470
void journal_list_page_up(int list);                // 0x41b4d0
void journal_list_page_down(int list);              // 0x41b520
void journal_desc_page_up();                        // 0x41b570
void journal_desc_page_down();                      // 0x41b5a0
void journal_button_action(int button);             // 0x41b5d0
void journal_goto_shop_or_status();                 // 0x41b6f0
void journal_goto_inventory();                      // 0x41b720
void journal_list_scroll_up(int list);              // 0x41b750
void journal_list_scroll_down(int list);            // 0x41b780
void journal_desc_scroll_up();                      // 0x41b7c0
void journal_desc_scroll_down();                    // 0x41b7e0
void journal_update_buttons();                      // 0x41b810
void journal_drag_scrollbar();                      // 0x41b8e0
void journal_draw();                                // 0x41b9f0
void journal_draw_quest_list();                     // 0x41ba60
void journal_draw_description();                    // 0x41bbf0
void journal_draw_list_scrollbar(int list);         // 0x41bd40
void journal_draw_desc_scrollbar();                 // 0x41be00

#endif
