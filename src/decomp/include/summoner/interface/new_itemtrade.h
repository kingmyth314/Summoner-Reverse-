// new_itemtrade.h - screen 19, the multiplayer item trade (new_itemtrade.cpp, 0x419170-0x41a932).
#ifndef NEW_ITEMTRADE_H
#define NEW_ITEMTRADE_H

#include "engine/objects/objects.h"   // party_inventory_entry

void itemtrade_load_bitmaps();                      // 0x4191b0
void itemtrade_init();                              // 0x419380
void itemtrade_update_scrollbar();                  // 0x419420
void itemtrade_count_party_rows();                  // 0x419530
void itemtrade_leave();                             // 0x419550
int  itemtrade_frame(int key);                      // 0x419560
void itemtrade_update_trade_buttons();              // 0x419610
void itemtrade_update_scroll_buttons();             // 0x419670
int  itemtrade_handle_click(int key);               // 0x419710
void itemtrade_party_page_up();                     // 0x419bb0
void itemtrade_party_page_down();                   // 0x419be0
void itemtrade_cancel();                            // 0x419c10
void itemtrade_submit_gold();                       // 0x419c20
void itemtrade_clear_gold_input();                  // 0x419c40
void itemtrade_button_action(int id);               // 0x419c50
void itemtrade_list1_scroll_up();                   // 0x419dc0
void itemtrade_list1_scroll_down();                 // 0x419de0
void itemtrade_list2_scroll_up();                   // 0x419e10
void itemtrade_list2_scroll_down();                 // 0x419e30
void itemtrade_party_scroll_up();                   // 0x419e60
void itemtrade_party_scroll_down();                 // 0x419e80
void itemtrade_accept();                            // 0x419eb0
void itemtrade_update_buttons();                    // 0x419ec0
void itemtrade_drag_scrollbar();                    // 0x419f90
void itemtrade_list1_handle_mouse();                // 0x41a020
void itemtrade_party_handle_mouse();                // 0x41a0c0
party_inventory_entry *itemtrade_get_party_row(int row);   // 0x41a1d0
void itemtrade_begin_gold_input();                  // 0x41a200
void itemtrade_draw();                              // 0x41a230
void itemtrade_draw_list1();                        // 0x41a360
void itemtrade_draw_list2();                        // 0x41a490
void itemtrade_draw_party_list();                   // 0x41a5a0
void itemtrade_draw_scrollbar();                    // 0x41a700
void itemtrade_draw_gold_input();                   // 0x41a7b0
int  itemtrade_get_panel_width();                   // 0x41a8b0
bool itemtrade_mouse_in_panel(int x, int y);        // 0x41a8c0
// The gold amount input line is open (interface_frame then keeps the keys from the chat line; decomp48).
extern bool itemtrade_gold_input_active;            // 0x5c1590

#endif
