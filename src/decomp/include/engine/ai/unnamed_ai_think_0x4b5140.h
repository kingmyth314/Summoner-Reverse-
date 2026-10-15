// engine/ai/unnamed_ai_think_0x4b5140.h - the AI think unit: party formation, target scoring and choice,
// following, the goal handlers, the attack-slot spreading of the party's enemies and ai_think (code
// 0x4b5140-0x4b77ef; source game/unnamed_ai_think_0x4b5140.cpp). File and directory names are
// placeholders (no path string, no strings.tbl block). The ai_state members are declared in
// engine/ai/ai.h (which includes this header); ai_target_query and ai_score_target are there too.
// Not original source.
#ifndef ENGINE_AI_UNNAMED_AI_THINK_0X4B5140_H
#define ENGINE_AI_UNNAMED_AI_THINK_0X4B5140_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"

class Character;

#define AI_FORMATION_SLOTS 5

// ---- .bss 0x247e8c8-0x247e963 (the formation of the last group leader that thought; nothing reads it
// back in Sum.exe, a debug display left over; names ours) -----------------------------------------------
extern int ai_formation_slot[AI_FORMATION_SLOTS];         // 0x247e8c8 member i's slot - 1
extern vector3 ai_formation_member_pos[AI_FORMATION_SLOTS];   // 0x247e8e0
extern vector3 ai_formation_slot_pos[AI_FORMATION_SLOTS];     // 0x247e920
extern int ai_formation_count;                            // 0x247e960
// (0x247e91c / 0x247e95c are ai_party_spread_attackers' function-static timer and its guard byte.)

// .rdata 0x581bb0: the attack goal priority / 10 by [attacker team][target team] (ai_pick_attack_target).
// File-static in the unit (defined const there).

// (attacker, target): the attack slot position around the target (0..5, fractional) that faces the
// attacker's side.
float ai_choose_attack_slot(Character *attacker, Character *target);   // 0x4b6a90

#endif
