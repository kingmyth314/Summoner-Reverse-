// new_assess.h - screen 20, the assess window (new_assess.cpp). Prototypes moved from the pilot stub header.
#ifndef NEW_ASSESS_H
#define NEW_ASSESS_H

class Character;

void assess_load_bitmaps();
void assess_init(Character *target);
void assess_format_info(Character *target);
void assess_update_scrollbar();
void assess_leave();
int  assess_frame(int key);
int  assess_handle_click(int key);
void assess_page_up();
void assess_page_down();
void assess_button_action(int id);
void assess_scroll_up();
void assess_scroll_down();
void assess_update_buttons();
void assess_drag_scrollbar();
void assess_draw();
void assess_draw_text();
void assess_draw_scrollbar();
bool assess_mouse_in_panel(int x, int y);

#endif
