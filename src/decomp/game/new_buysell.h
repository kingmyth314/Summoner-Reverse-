// new_buysell.h - screen 7, the shop (new_buysell.cpp, 0x4048b0-0x4063a2).
#ifndef NEW_BUYSELL_H
#define NEW_BUYSELL_H

#include "types.h"   // shop_item, ItemType, vector3, matrix3

void buysell_update_button_states();                // 0x4048d0
shop_item *buysell_get_list_item(int row);          // 0x404960
int  shop_find_item_type(ItemType *type);           // 0x4049c0
void bs_cameras_parse_table();                      // 0x404a00
void buysell_load_bitmaps();                        // 0x404a20
void buysell_init();                                // 0x404bd0
void buysell_set_category(int mode);                // 0x404cc0 (0 buy, 1 sell)
void buysell_update_category_buttons();             // 0x404dc0
int  buysell_category_first_index(int category);    // 0x404f70
void buysell_update_scrollbar();                    // 0x404fe0
void buysell_count_party_items();                   // 0x4050f0
void buysell_leave();                               // 0x4051a0
int  buysell_frame(int key);                        // 0x4051e0
int  buysell_handle_click(int key);                 // 0x405260
void buysell_category_tooltip(int button);          // 0x4056b0
void buysell_page_up();                             // 0x405790
void buysell_page_down();                           // 0x4057c0
void buysell_buy_item(int row);                     // 0x4057f0
void buysell_sell_item(int row);                    // 0x405900
void buysell_button_action(int id);                 // 0x4059a0
void buysell_goto_status();                         // 0x405ba0
void buysell_goto_inventory();                      // 0x405bc0
void buysell_scroll_up();                           // 0x405c00
void buysell_scroll_down();                         // 0x405c20
void buysell_goto_weapons();                        // 0x405c50
void buysell_goto_armor();                          // 0x405c90
void buysell_goto_magic_items();                    // 0x405cd0
void buysell_goto_jewelry();                        // 0x405d10
void buysell_goto_misc();                           // 0x405d50
void buysell_show_item_info();                      // 0x405d90
void buysell_update_buttons();                      // 0x405db0
void buysell_drag_scrollbar();                      // 0x405e80
void buysell_draw();                                // 0x405f10
void buysell_draw_item_list();                      // 0x405fd0
void buysell_draw_scrollbar();                      // 0x406150
void buysell_draw_item_model();                     // 0x406200
void buysell_model_view_begin();                    // 0x406280
void buysell_model_view_end();                      // 0x4062c0
void buysell_render_item_mesh(void *mesh, vector3 *pos, matrix3 *orient);   // 0x4062e0
// shop_set_shopkeeper_from_char (0x406360) and ui_has_owner_character (0x406390) are declared in
// stubs_gamecreate_journal.h (the parameter there is void*, see NOTES.md).
bool ui_return_true(int x, int y);                  // 0x4063a0

#endif
