// summoner/interface/new_tutorial.h - the tutorial hints (new_tutorial.cpp, 0x42e630-0x42f1df).
// Not original source.
#ifndef SUMMONER_INTERFACE_NEW_TUTORIAL_H
#define SUMMONER_INTERFACE_NEW_TUTORIAL_H

#include "vsdk/vsdk.h"

extern bool Ignore_tutorial;                         // 0x5f9625 console command "Ignore_tutorial"

void tutorial_reset();                               // 0x42ecf0 from interface_load_all
void tutorial_show(int entry);                       // 0x42ed00 (0..4) pauses the game
void tutorial_close();                               // 0x42ed90
bool tutorial_frame(int key);                        // 0x42ef00 (true = the key was used)
void tutorial_draw();                                // 0x42f020
bool tutorial_is_active();                           // 0x42f1c0
bool tutorial_mouse_in_panel(int x, int y);          // 0x42f1d0 (x, y ignored)

#endif
