// engine/ai/boss_ai.h - boss hooks (boss_ai.cpp in sum.symbols, 0x4a04b0-0x4a3580, path string 0x4a1812)
// and the per-boss units around it whose file names are not known (phoenix, tiger, ghost rider). The
// directory is guessed. Not original source. Declared for combat.cpp and spells.cpp (decomp23); the
// argument lists are from their call sites.
#ifndef ENGINE_AI_BOSS_AI_H
#define ENGINE_AI_BOSS_AI_H

#include "vsdk/vsdk.h"

class Character;
struct move_info;

// (attacker, defender) as living_combat_resolve_hit pushes them (sum.symbols: "(boss, attacker)").
void  boss_counter_hit_dispatch(Character *attacker, Character *defender);   // 0x4a2230
bool  boss_move_in_range_dispatch(Character *boss, Character *target);  // 0x4a0ea0
bool  boss_attack_stuns_dispatch(int species);         // 0x4a1090
bool  boss_attack_knocks_down_dispatch(int species);   // 0x4a1150
void  boss_clear_current_move(Character *c);           // 0x4a2590
bool  character_is_not_boss(Character *c);             // 0x4a2d40 type != 9
bool  serpent_part_on_hit(Character *c);               // 0x4a28b0 (true: the hit is taken by a serpent part)
int   rand_int_range(int lo, int hi);                  // 0x4a3410 lo + (int)(frand() * (hi - lo))

move_info *phoenix_select_ranged_move(Character *c);   // 0x499a90 (phoenix unit, file not known)
void  tiger_restore_ride_anims(Character *rider);      // 0x49c750 (tiger unit, file not known)

// The ghost rider's stage (ghost_update_stage writes 1..3; written by ghost_rider_boss_load too).
// spell_compute_resist gives spell 0x50 (Ghost Paralyzing Fear) a 0.5 hit chance in stage 3. Name ours;
// its .bss belongs to the ghost rider unit.
extern int ghost_stage;                                // 0x2480310

#endif
