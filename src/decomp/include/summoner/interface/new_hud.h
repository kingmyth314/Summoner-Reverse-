// new_hud.h - screen 0, the in-game HUD (new_hud.cpp, 0x40cb60-0x41075a).
#ifndef NEW_HUD_H
#define NEW_HUD_H

#include "vsdk/gr/gr.h"   // Color (returned by value)

void  hud_load_bitmaps();                           // 0x40cc10
void  hud_init();                                   // 0x40cef0
int   hud_frame(int key);                           // 0x40cf40
void  hud_update_portraits();                       // 0x40d160
void  hud_portrait_set_state(int slot, int state);  // 0x40d4d0
void  hud_update_anim_time();                       // 0x40d540
void  hud_update_combat_buttons();                  // 0x40d5d0
void  hud_confirm_level_exit();                     // 0x40d640
void  hud_level_exit_step_back_thunk(int button);   // 0x40d840 (message box callback)
void  hud_level_exit_step_back();                   // 0x40d850
void  hud_confirm_level_exit_cb(int button);        // 0x40d910
int   hud_handle_keys(int key);                     // 0x40d930
void  hud_key_save();                               // 0x40e0d0
void  hud_select_party_member(int n);               // 0x40e1f0
void  hud_key_playmenu();                           // 0x40e250
void  hud_key_help();                               // 0x40e260
void  hud_key_pause();                              // 0x40e270
void  hud_toggle_solo_mode();                       // 0x40e300
void  hud_key_h_cb(int button);                     // 0x40e380
void  hud_key_automap();                            // 0x40e390
void  hud_toggle_combat_mode();                     // 0x40e3c0
void  hud_key_status();                             // 0x40e3e0
void  hud_key_spells();                             // 0x40e3f0
void  hud_key_journal();                            // 0x40e420
void  hud_key_inventory();                          // 0x40e430
void  hud_key_skills();                             // 0x40e450
void  hud_interact_nearest();                       // 0x40e480
void  quickload();                                  // 0x40e740
void  quicksave();                                  // 0x40e7c0
void  hud_key_camera();                             // 0x40e980
void  hud_use_hot_button(int slot);                 // 0x40e9d0
void  hud_draw_tooltips(int n, bool hot_button);    // 0x40eb30
void  hud_update_buttons();                         // 0x40ee20
void  hud_button_action(int button);               // 0x40eeb0
void  hud_portraits_handle_click();                 // 0x40f120
void  hud_portrait_action(int button);             // 0x40f1b0
void  hud_hot_buttons_handle_click();               // 0x40f230
void  hud_hot_buttons_drag();                       // 0x40f340
int   hud_death_timer_frame(int key);               // 0x40f570
int   hud_confirm_quit(int key);                    // 0x40f5d0
void  hud_confirm_quit_cb(int button);              // 0x40f650
void  death_popup();                                // 0x40f690
void  hud_draw();                                   // 0x40f6e0
Color hud_portrait_get_color(int n);                // 0x40f880
void  hud_draw_camera_mode();                       // 0x40fa90
void  hud_draw_camera_forced_icon();                // 0x40fc20
void  hud_draw_health_bars();                       // 0x40fd40
void  hud_draw_animated_icons();                    // 0x40fed0
void  hud_draw_hot_buttons();                       // 0x410020
void  draw_game_over();                             // 0x4101e0
void  mp_draw_loading_status();                     // 0x4102a0
int   hud_get_panel_height();                       // 0x410510
bool  hud_mouse_in_panel(int x, int y);             // 0x410520
void  hud_drag_icon_begin(int x, int y, int w, int h, int kind, int id);  // 0x410570
void  hud_drag_icon_end();                          // 0x4105d0
void  hud_drag_icon_draw();                         // 0x4105e0
void  hud_mode_set_5();                             // 0x4106d0
void  hud_party_dead();                             // 0x4106e0
void  hud_dead_clear_popups();                      // 0x410730
void  hud_mode_reset();                             // 0x410750

#endif
