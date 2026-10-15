// summoner/interface/new_tutorial.h - the tutorial hints (new_tutorial.cpp in sum.symbols; 0x42ef00-0x42f020).
// Not a pilot unit yet: only the functions the HUD calls. Not original source.
#ifndef SUMMONER_INTERFACE_NEW_TUTORIAL_H
#define SUMMONER_INTERFACE_NEW_TUTORIAL_H

#include "vsdk/vsdk.h"

bool tutorial_frame(int key);                        // 0x42ef00 (true = the key was used)
void tutorial_draw();                                // 0x42f020

#endif
