// summoner/interface/unnamed_item_info_popup_0x418580.h - the item info popup (game/unnamed_item_info_popup_0x418580.cpp,
// 0x418580-0x419166; file name not known, decomp71). The six functions the other screens call
// (load_bitmaps, open_item, open_type, frame, render, is_active) stay declared in interface.h.
// Not original source.
#ifndef UNNAMED_ITEM_INFO_POPUP_0X418580_H
#define UNNAMED_ITEM_INFO_POPUP_0X418580_H

#include "summoner/interface/interface.h"
#include "vsdk/os/timestamp.h"

struct item_info;

extern const int item_info_popup_title_pos[3][2];          // 0x57fdbc
extern const int item_info_popup_text_pos[3][2];           // 0x57fdd4
extern const int item_info_popup_text_width[3];            // 0x57fdec
extern const int item_info_popup_page_size[3];             // 0x57fdf8
extern const int item_info_popup_scrollbar_track[3][2];    // 0x57fe04
extern const int item_info_popup_scrollbar_track_h[3];     // 0x57fe1c
extern const int item_info_popup_model_offset[3][2];       // 0x57fe28
extern ui_button_def item_info_popup_button_defs[7];       // 0x58cbd8
extern const char *item_info_popup_scrollbar_names[2];           // 0x58cd98

extern ui_button_state item_info_popup_buttons[7];         // 0x5c1470
extern bool item_info_popup_visible;                       // 0x5c1518
extern int  item_info_popup_drag_start_thumb;              // 0x5c151c
extern int  item_info_popup_line_count;                    // 0x5c1520
extern int  item_info_popup_scrollbar_x;                   // 0x5c1524
extern int  item_info_popup_scrollbar_y;                   // 0x5c1528
extern int  item_info_popup_drag_start_y;                  // 0x5c152c
extern int  item_info_popup_mouse_mode;                    // 0x5c1530
extern item_info *item_info_popup_item;                    // 0x5c1534
extern int  item_info_popup_source;                        // 0x5c1538 1 created by open_type, 2 party item
extern char *item_info_popup_text;                         // 0x5c153c
extern int  item_info_popup_pressed_button;                // 0x5c1540
extern RealtimeTimer item_info_popup_repeat_timer;         // 0x5c1544
extern int  item_info_popup_saved_view_x;                  // 0x5c1548
extern int  item_info_popup_saved_view_y;                  // 0x5c154c
extern int  item_info_popup_scrollbar_bm_w;                // 0x5c1550
extern int  item_info_popup_scrollbar_bm_h;                // 0x5c1554
extern int  item_info_popup_scrollbar_bm;                  // 0x5c1558
extern int  item_info_popup_scrollbar_w;                   // 0x5c155c
extern int  item_info_popup_scrollbar_h;                   // 0x5c1560
extern bool item_info_popup_scrollbar_visible;             // 0x5c1564
extern int  item_info_popup_scroll_pos;                    // 0x5c1568

// Defined in new_buysell.cpp (no header declared it; decomp71).
extern unsigned char buysell_cameras[0x3628];              // 0x5b2dc0 bs_cameras.tbl

void item_info_popup_setup();                              // 0x4187a0
void item_info_popup_update_scrollbar();                   // 0x418810
int  item_info_popup_handle_click(int key);                // 0x4189c0
void item_info_popup_page_up();                            // 0x418ba0
void item_info_popup_page_down();                          // 0x418bd0
void item_info_popup_close();                              // 0x418c00
void item_info_popup_button_action(int id);                // 0x418c40
void item_info_popup_scroll_up();                          // 0x418c90
void item_info_popup_scroll_down();                        // 0x418cb0
void item_info_popup_update_buttons();                     // 0x418ce0
void item_info_popup_drag_scrollbar();                     // 0x418db0
void item_info_popup_draw_title();                         // 0x418ee0
void item_info_popup_draw_text();                          // 0x418f30
void item_info_popup_draw_scrollbar();                     // 0x418ff0
void item_info_popup_draw_model();                         // 0x4190a0
void item_info_popup_view_begin();                         // 0x419100
void item_info_popup_view_end();                           // 0x419140

#endif
