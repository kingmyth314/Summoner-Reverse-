// summoner/interface/new_spell.h - the spells panel (new_spell.cpp in sum.symbols; 0x42ade0-0x42af30..).
// Not a pilot unit yet: only the functions and data other screens use. Not original source.
#ifndef SUMMONER_INTERFACE_NEW_SPELL_H
#define SUMMONER_INTERFACE_NEW_SPELL_H

#include "vsdk/vsdk.h"

class Character;

bool character_can_cast_now(Character *c);           // 0x42ade0 (0 = cur_character)
bool spell_is_usable(int spell, int a, int b);       // 0x42ae20 (0, 0 from the HUD; types guessed)
void character_cast_spell(int spell, int a);         // 0x42af30 (a = 0; return unused)

extern int spells_icon_bms[38][4];                   // 0x5f5820 4 bitmaps per spell icon

#endif
