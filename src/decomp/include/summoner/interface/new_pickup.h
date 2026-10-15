// new_pickup.h - screen 4, the container pickup window (new_pickup.cpp). Prototypes moved from the pilot
// stub header.
#ifndef NEW_PICKUP_H
#define NEW_PICKUP_H

class container;

void pickup_load_bitmaps();
void pickup_init(container *c);
void pickup_update_scrollbar();
void pickup_leave();
int  pickup_frame(int key);
int  pickup_handle_click(int key);
void pickup_page_up();
void pickup_page_down();
void pickup_take_selected();
void pickup_close();
void pickup_button_action(int id);
void pickup_scroll_up();
void pickup_scroll_down();
void pickup_update_buttons();
void pickup_list_handle_click();
void pickup_drag_scrollbar();
void pickup_draw();
void pickup_draw_scrollbar();
void pickup_draw_item_list();
bool pickup_mouse_in_panel(int x, int y);

#endif
