// new_dialogue.h - screen 13, the conversation panel (new_dialogue.cpp, 0x408d80-0x4099ac).
#ifndef NEW_DIALOGUE_H
#define NEW_DIALOGUE_H

void dialogue_load_bitmaps();                       // 0x408d80
void dialogue_init(const char *name);               // 0x408f90 (interface_set_screen argument: the speaker's name)
void dialogue_leave();                              // 0x408fe0
int  dialogue_frame(int key);                       // 0x409000
void dialogue_draw();                               // 0x409650
bool dialogue_mouse_in_panel(int x, int y);         // 0x409980

#endif
