// new_saveload.h - screen 8, the save / load screen (new_saveload.cpp, 0x425410-0x42723b).
#ifndef NEW_SAVELOAD_H
#define NEW_SAVELOAD_H

void saveload_load_bitmaps();                       // 0x4254e0
void saveload_init(bool save_mode, bool in_game);   // 0x4256a0
void saveload_update_save_button();                 // 0x425730
void saveload_refresh(bool save_mode);              // 0x425790
void saveload_update_mode_buttons();                // 0x4257f0
void saveload_scan_saves();                         // 0x4258c0
void saveload_update_scrollbar();                   // 0x425b00
void saveload_load_thumbnail(int row);              // 0x425c10
void saveload_leave();                              // 0x425c80
int  saveload_frame(int key);                       // 0x425c90
void saveload_update_select_button();               // 0x425d60
void saveload_update_delete_button();               // 0x425d80
int  saveload_handle_input(int key);                // 0x425de0
void saveload_page_up();                            // 0x426260
void saveload_page_down();                          // 0x426290
void saveload_close();                              // 0x4262c0
void saveload_save_or_load();                       // 0x4262f0
int  saveload_try_save();                           // 0x426320
int  saveload_save_to_slot();                       // 0x426370
int  saveload_load_selected();                      // 0x426480
void saveload_confirm_overwrite();                  // 0x426500
void saveload_confirm_overwrite_cb(int button);     // 0x426540
void saveload_button_action(int button);            // 0x426570
void saveload_scroll_up();                          // 0x426670
void saveload_scroll_down();                        // 0x426690
void saveload_confirm_delete();                     // 0x4266c0
void saveload_delete_save(int button);              // 0x426700
void saveload_update_buttons();                     // 0x4267e0
void saveload_hover_thumbnail();                    // 0x4268b0
void saveload_drag_scrollbar();                     // 0x426930
void saveload_draw();                               // 0x4269c0
void saveload_draw_list();                          // 0x426a20
void saveload_format_level_name(char *out, int level); // 0x426be0
void saveload_draw_info();                          // 0x426d10
void saveload_draw_thumbnail();                     // 0x426fd0
void saveload_draw_scrollbar();                     // 0x427050
void saveload_capture_thumbnail();                  // 0x427100 (also quicksave, cmd_savegame)

#endif
