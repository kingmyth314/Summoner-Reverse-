// new_options.h - screen 12, the options screen (new_options.cpp, 0x41ee90-0x4205ae).
#ifndef NEW_OPTIONS_H
#define NEW_OPTIONS_H

void options_load_bitmaps();                        // 0x41ef30
void options_init(bool in_game);                    // 0x41f0f0
void options_set_sfx_volume(float volume);          // 0x41f250
void options_set_music_volume(float volume);        // 0x41f2b0
void options_set_camera_speed(float speed);         // 0x41f300
void options_apply_gamma(float gamma);              // 0x41f380
void options_set_eax(bool on);                      // 0x41f410
void options_set_solo_confirm(bool on);             // 0x41f490
void options_set_invert_camera(bool on);            // 0x41f4d0
void options_set_dialogue_camera(int mode);         // 0x41f510
void options_set_cfg_3c(bool on);                   // 0x41f5c0
void options_set_combat_flag(bool on);              // 0x41f600
void options_set_party_model_detail(int level);     // 0x41f640
void options_set_model_detail(int level);           // 0x41f6f0
void options_set_texture_quality(int quality);      // 0x41f7a0
void options_set_lightmaps_off(bool off);           // 0x41f7f0
void options_set_cfg_21(bool on);                   // 0x41f840
void options_set_detail_24(int level);              // 0x41f870
void options_set_fog_quality(int quality);          // 0x41f920
void options_set_antialias(bool on);                // 0x41f9d0
void options_set_cfg_34(bool on);                   // 0x41fa20
void options_leave();                               // 0x41fa60
void options_apply_and_save();                      // 0x41fa90
void options_restore_volumes();                     // 0x41fae0
int  options_frame(int key);                        // 0x41fb00
int  options_handle_click(int key);                 // 0x41fb50
void options_close();                               // 0x41fc50
void options_update_buttons();                      // 0x41fc80
void options_button_action(int button);             // 0x41fd20
void options_drag_sliders();                        // 0x41fff0
void options_draw();                                // 0x420320
void options_draw_sfx_slider();                     // 0x420430
void options_draw_music_slider();                   // 0x420490
void options_draw_camera_speed_slider();            // 0x4204f0
void options_draw_gamma_slider();                   // 0x420550

#endif
