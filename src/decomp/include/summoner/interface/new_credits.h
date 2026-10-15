// new_credits.h - screen 21, the credits (new_credits.cpp, 0x408590-0x408d72). Prototypes moved from the
// pilot stub header (the binary's function order has callers before callees).
#ifndef NEW_CREDITS_H
#define NEW_CREDITS_H

void credits_init();
void credits_parse_table();
void credits_exit();
void credits_get_line_size(int *w, int *h, char *s);
void credits_leave();
int  credits_frame(int key);
void credits_finish();
void credits_draw();
int  credits_draw_lines(int w, int y, char *s);

#endif
