// new_inventory.h - screen 1, the inventory (new_inventory.cpp, 0x414c80-0x418572).
#ifndef NEW_INVENTORY_H
#define NEW_INVENTORY_H

#include "vsdk/math/matrix.h"   // vector3, matrix3

class Character;
struct ItemType;

struct item_info;
struct party_inventory_entry;

void cmd_show_ui();                                 // 0x414cb0
void inv_cameras_parse_table();                     // 0x414dc0
void inventory_load_bitmaps();                      // 0x414de0
void inventory_init(bool open_mode);                // 0x414fc0 (interface_set_screen passes a byte)
void inventory_page_in_equipped();                  // 0x4150c0
void inventory_update_species_tab();                // 0x415180
void inventory_update_category_buttons();           // 0x415220
int  inventory_category_first_index(int category);  // 0x4153d0
void inventory_update_scrollbar();                  // 0x415470
void inventory_update_trade_buttons();              // 0x415580
void inventory_update_equip_slots();                // 0x4155f0
void inventory_count_rows();                        // 0x415850
void inventory_leave();                             // 0x4158b0
int  inventory_frame(int key);                      // 0x4158f0
int  inventory_handle_click(int key);               // 0x4159a0
party_inventory_entry *inventory_get_list_item(int row);   // 0x416020
int  inventory_equip_index_for_button(int button);  // 0x416090 (paper-doll button -> equipment index)
void inventory_update_item_buttons(item_info *item, bool from_list);   // 0x416110
bool inventory_can_identify();                      // 0x416200
void inventory_hover_tooltips(int button);          // 0x416290
void inventory_show_item_info();                    // 0x416560
void inventory_page_up();                           // 0x4165c0
void inventory_page_down();                         // 0x4165f0
void inventory_button_action(int id);               // 0x416620
void inventory_goto_other_screen();                 // 0x4167e0
void inventory_goto_status();                       // 0x416840
void inventory_prev_member();                       // 0x416860
void inventory_next_member();                       // 0x416870
void inventory_scroll_up();                         // 0x416880
void inventory_scroll_down();                       // 0x4168a0
void inventory_goto_category0();                    // 0x4168d0
void inventory_goto_category1();                    // 0x416910
void inventory_goto_category2();                    // 0x416950
void inventory_goto_category3();                    // 0x416990
void inventory_goto_category4();                    // 0x4169d0
// interface_close_screen (0x416a10) is declared in summoner/interface/interface.h.
void inventory_update_buttons();                    // 0x416a20
void inventory_drag_scrollbar();                    // 0x416af0
int  inventory_popup_result(int key);               // 0x416b80
void inventory_use_selected_item();                 // 0x416c10
void inventory_use_list_item_begin(party_inventory_entry *entry);   // 0x416dc0
void inventory_use_equipped_item_begin(int slot);   // 0x416eb0
void inventory_identify_item();                     // 0x416fb0
void inventory_confirm_destroy();                   // 0x417110
void inventory_confirm_destroy_cb(int button);      // 0x417160
void inventory_destroy_selected_item();             // 0x417170
void inventory_drag_to_equip();                     // 0x417220
void inventory_equip_item(int slot, party_inventory_entry *entry);  // 0x4174a0
void inventory_drag_from_equip();                   // 0x417630
void inventory_unequip_item(int slot);              // 0x417740
void inventory_swap_rings();                        // 0x417790 (swaps the two rings)
void inventory_draw();                              // 0x417860
void inventory_draw_list_rows();                    // 0x417950 (draws all visible rows)
void inventory_set_row_color(int row);              // 0x417a60
void inventory_draw_scrollbar();                    // 0x417b10
void inventory_draw_item_model();                   // 0x417bc0
void ui_render_item_mesh(void *mesh, vector3 *pos, matrix3 *orient);   // 0x417c50
void inventory_item_view_offset(ItemType *type);    // 0x417cd0
void inventory_model_view_begin(int slot);          // 0x417da0
void inventory_model_view_end();                    // 0x417df0
void inventory_draw_character();                    // 0x417e10
void inventory_render_character(Character *c, vector3 *pos, matrix3 *orient);   // 0x417fe0
void inventory_draw_drag_item();                    // 0x418090
void inventory_draw_item_preview(item_info *item);  // 0x418160
void inventory_preview_view_begin(int item_slot);   // 0x4181d0
void inventory_draw_equipped_hover();               // 0x418430
void inventory_refresh_for_character();             // 0x4184b0
void world_use_item_on_object(party_inventory_entry *entry, Character *obj);   // 0x4184c0
void world_use_equipped_item_on_object(int slot, Character *obj);              // 0x418500
void world_use_item_at_point(party_inventory_entry *entry, void *point, int arg);   // 0x418540

#endif
