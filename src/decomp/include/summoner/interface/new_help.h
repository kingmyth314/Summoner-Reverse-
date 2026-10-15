// new_help.h - screen 22, the help pages (new_help.cpp, 0x40bf30-0x40cb55). Prototypes moved from the
// pilot stub header.
#ifndef NEW_HELP_H
#define NEW_HELP_H

void help_reset_page();
void help_init(bool in_game);
void help_leave();
int  help_frame(int key);
void help_prev_page();
void help_next_page();
void help_draw();

#endif
