// new_status.h - screen 2, the Character screen (new_status.cpp, 0x42b1c0-0x42e62e).
#ifndef NEW_STATUS_H
#define NEW_STATUS_H

void status_cameras_parse_table();                  // 0x42b240
void status_load_bitmaps();                         // 0x42b260
void status_init();                                 // 0x42b480
void status_update_species_tab();                   // 0x42b570
void status_update_member_buttons();                // 0x42b610
void status_build_skill_list();                     // 0x42b680
void status_update_scrollbar();                     // 0x42b800
void status_leave();                                // 0x42b910
void status_apply_skill_points();                   // 0x42b970
void status_cancel_skill_points();                  // 0x42b9f0
int  status_frame(int key);                         // 0x42ba50
int  status_handle_click(int key);                  // 0x42bb00
bool status_get_skill_points(int row);              // 0x42bfc0 (true: the row shows the +/- buttons)
void status_confirm_skills(int screen);             // 0x42c020
void status_goto_pending_screen();                  // 0x42c080
void status_confirm_skills_cb(int button);          // 0x42c0c0
void status_page_up();                              // 0x42c0e0
void status_page_down();                            // 0x42c110
void status_button_action(int button);              // 0x42c140
void status_confirm_inventory();                    // 0x42c260
void status_goto_shop_or_inventory();               // 0x42c270
void status_confirm_skills_switch_member(bool next);// 0x42c2d0
void status_show_prev_member();                     // 0x42c380
void status_confirm_skills_b_cb(int button);        // 0x42c3a0
void status_show_next_member();                     // 0x42c3c0
void status_confirm_skills_b_cb2(int button);       // 0x42c3e0
void status_scroll_up();                            // 0x42c400
void status_scroll_down();                          // 0x42c420
void status_ai_popup_show();                        // 0x42c450
void status_confirm_close();                        // 0x42c4c0
void status_show_skill_tooltip(int row);            // 0x42c4d0 (row on the page)
void status_update_buttons();                       // 0x42c540
void status_drag_scrollbar();                       // 0x42c630
int  status_ai_popup_frame(int key);                // 0x42c6c0
void status_handle_skill_buttons();                 // 0x42c710
void status_skill_increment(int row);               // 0x42c820 (row in status_skill_entries)
void status_skill_decrement(int row);               // 0x42c8d0
int  status_chain_popup_frame(int key);             // 0x42c940
void status_draw();                                 // 0x42c9c0
void status_draw_model();                           // 0x42ca50
void status_model_view_begin();                     // 0x42cce0
void status_model_view_end();                       // 0x42cd60
void status_draw_level();                           // 0x42cd80
void status_draw_stats();                           // 0x42cff0
void status_draw_skills();                          // 0x42d640
void status_draw_scrollbar();                       // 0x42da10
void status_draw_skill_points();                    // 0x42dac0
void status_draw_resistances();                     // 0x42dcd0
void status_draw_status_effects();                  // 0x42e410
void status_draw_combat_style();                    // 0x42e5c0
void status_refresh_for_character();                // 0x42e620

#endif
