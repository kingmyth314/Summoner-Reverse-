// summoner/interface/new_skill.h - the skills panel (new_skill.cpp in sum.symbols; 0x428757-0x429410..).
// Not a pilot unit yet: only the functions and data other screens use. Not original source.
#ifndef SUMMONER_INTERFACE_NEW_SKILL_H
#define SUMMONER_INTERFACE_NEW_SKILL_H

#include "vsdk/vsdk.h"

class Character;

// The character argument is a thiscall `this` inside (0 = cur_character: 0x4291dc, 0x42941b).
bool skill_is_usable(int skill, Character *c);       // 0x4290a0
bool skill_is_available(int skill, Character *c);    // 0x4291d0
// (skill, target, user): user is arg 3 (0x429412 -> ESI, 0 = cur_character); the target is an item for
// Identify (inventory_identify_item) and the summon for Banish (hud_use_hot_button). DECISIONS.md D48.
bool character_use_skill(int skill, void *target, Character *user);   // 0x429410

extern int skills_icon_bms[56][4];                   // 0x5f4fe8 4 bitmaps per skill icon

#endif
