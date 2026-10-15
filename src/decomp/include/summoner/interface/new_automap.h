// new_automap.h - screen 3, the map screen (new_automap.cpp, 0x401e80-0x404871).
#ifndef NEW_AUTOMAP_H
#define NEW_AUTOMAP_H

void automap_load_bitmaps();                        // 0x401ef0
void automap_init();                                // 0x402250
void automap_select_label(int index);               // 0x402280 (-1 none)
void automap_center_on_player();                    // 0x4023b0
void automap_labels_update_positions();             // 0x402490
void automap_show_player_layer();                   // 0x402560
void automap_set_layer(int layer);                  // 0x4025a0
void automap_update_layer_buttons();                // 0x4025f0
int  automap_frame(int key);                        // 0x402680
int  automap_handle_click(int key);                 // 0x402760
bool automap_map_point_revealed(int x, int y, bool scaled);   // 0x402d80
void automap_update_buttons();                      // 0x402e90
void automap_button_action(int id);                 // 0x402f10
void automap_layer_down();                          // 0x402fb0
void automap_layer_up();                            // 0x402fd0
void automap_drag_scroll();                         // 0x403000
void automap_add_label_at_mouse();                  // 0x403100
bool automap_mouse_to_map_cell(int *out_x, int *out_y, int mx, int my);   // 0x403240
void automap_map_to_world(float *out_x, float *out_z, int mx, int my);    // 0x403300
int  automap_label_text_input(int key);             // 0x403350
void automap_label_box_hover();                     // 0x403510
void automap_labels_hover();                        // 0x4035c0
void automap_draw();                                // 0x403690
void automap_draw_map();                            // 0x403750
void automap_draw_label(int index);                 // 0x403ff0
void automap_draw_title();                          // 0x404310
int  automap_get_panel_width();                     // 0x404460
bool automap_mouse_in_panel(int x, int y);          // 0x404470
void cmd_map_mask();                                // 0x4044d0
void cmd_map_grid();                                // 0x4045c0
void cmd_show_coordinates();                        // 0x4046b0
void cmd_reset_map_labels();                        // 0x4047a0

extern bool Map_mask;                               // 0x588bc4
extern bool Show_coordinates;                       // 0x588bd0
extern bool Map_grid;                               // 0x5b2db4

#endif
