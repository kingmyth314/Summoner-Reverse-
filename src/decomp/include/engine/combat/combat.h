// engine/combat/combat.h - melee combat (combat.cpp in sum.symbols / strings.tbl, code 0x463540-0x468c77
// between living_entity.cpp and gamesound.cpp). Directory and file name guessed (the unit has no path
// string; the module name comes from the strings.tbl "combat.cpp" ids 215-217). Not original source.
// The living_combat record used to be declared in living_entity.h (DECISIONS.md D1); it moves here with
// its fields named (D300).
#ifndef ENGINE_COMBAT_COMBAT_H
#define ENGINE_COMBAT_COMBAT_H

#include "vsdk/vsdk.h"
#include "vsdk/math/matrix.h"

class Character;
struct move_info;

// living_combat.state (+0x14). The names are ours, from the writers: play_attack 1, roll_defense 2,
// pick_parry / pick_dodge 3, play_defense / play_flinch 4, play_special 5.
#define COMBAT_IDLE        0
#define COMBAT_ATTACKING   1
#define COMBAT_HIT         2          // the defence roll failed: the attack lands (roll_defense)
#define COMBAT_DEFENCE     3          // a parry or dodge move is chosen (+0x30)
#define COMBAT_REACTING    4          // playing the defence or flinch move
#define COMBAT_SPECIAL     5          // ranged or special attack move

// The melee record at Character +0x558 (0x54 bytes; constructor 0x4681b0). The attack and defence
// calls are thiscall on it; +0x08 points at the other side's record during an exchange.
class living_combat {
public:
    // The constructor 0x4681b0 (GameTimer at +0xc, then reset; D300, a real constructor since D19000).
    living_combat();                                // 0x4681b0 living_combat__ctor
    void  reset();                                  // 0x4635e0 living_combat_reset
    void  set_owner(Character *c);                  // 0x4681d0
    Character *get_target();                        // 0x4681e0
    void  set_target(int unused, Character *c);     // 0x4681f0 (first argument not read)
    bool  update(int move_id);                      // 0x468210 (the move id goes to the boss branch; D3404)
    void  update_boss_attack(int move_id);          // 0x468360 (compares it with +0x18)
    void  apply_net_exchange(int hit, int move, int defence, int skill, bool special);   // 0x4685c0
    bool  is_busy();                                // 0x468740
    void  set_38(int skill);                        // 0x468750 living_combat_set_38 (chain skill)
    void  set_special_skill_species89(int skill);   // 0x468c40
    int   get_special_skill();                      // 0x468c60
    void  set_40(int hits);                         // 0x468c70

    void  send_net(bool state_arg, bool special);   // 0x463540 living_combat_send_net (D22505: bool)
    void  play_attack(move_info *move);             // 0x463620
    void  play_special(move_info *move);            // 0x463660
    void  play_flinch_secondary(int variant);       // 0x4636a0
    void  play_flinch(int variant);                 // 0x463730
    void  pick_parry();                             // 0x4637d0
    void  pick_dodge();                             // 0x463830
    void  queue_counter_attack();                   // 0x463860 combat_queue_counter_attack
    void  play_defense();                           // 0x4638c0
    bool  fire_weapon_missile();                    // 0x463910 combat_fire_weapon_missile
    float attack_rating();                          // 0x463b40 combat_attack_rating
    float weapon_skill();                           // 0x463b90 combat_weapon_skill
    bool  has_weapon_skill();                       // 0x463be0 combat_has_weapon_skill
    float speed_share(living_combat *other);        // 0x463c00
    void  ai_choose_special();                      // 0x463d00
    move_info *pick_attack_move(int allow_any);     // 0x463e20
    void  begin_attack();                           // 0x463f30
    void  begin_attack_thunk();                     // 0x463f70 (jmp begin_attack)
    void  ranged_attack();                          // 0x463f80
    void  roll_defense(living_combat *attacker, bool simulate);   // 0x464100
    void  play_skill_attack();                      // 0x4645f0
    void  resolve_exchange(bool one_sided);         // 0x464700 combat_resolve_exchange
    bool  defense_in_time();                        // 0x464cb0
    float get_protection();                         // 0x464de0 character_get_protection
    float melee_damage(int unused);                 // 0x464f50 combat_melee_damage (callers pass 0)
    void  apply_melee_hit(bool flinch);             // 0x465df0 combat_apply_melee_hit
    float chain_check_window(int skill);            // 0x466ae0 chain_attack_check_window
    void  update_exchange();                        // 0x466d20
    void  update_attack();                          // 0x466f00
    void  update_ranged();                          // 0x467020
    void  trigger_special_ability(float damage);    // 0x4670a0 combat_trigger_special_ability
    void  trigger_reflex_ability(float damage);     // 0x467670 combat_trigger_reflex_ability
    void  update_special();                         // 0x467cb0
    bool  resolve_hit();                            // 0x467df0 living_combat_resolve_hit
    void  flinch_if_idle(int variant);              // 0x468160
    void  blood_splot_place(int size, float grey);  // 0x468760
    void  spawn_blood(float damage);                // 0x468ac0 combat_spawn_blood

    Character *owner;            // +0x00 set_owner
    int target;                  // +0x04 target handle (-1 none)
    living_combat *opponent;     // +0x08 the other side (living_combat_update: &target->combat)
    int timer;                   // +0x0C GameTimer storage (the constructor runs GameTimer::GameTimer on it; D300)
    int _unk10;                  // +0x10 not touched in this unit
    int state;                   // +0x14 COMBAT_*
    int move_variant;            // +0x18 id (+0x14) of the chosen attack move
    bool chain_learn;            // +0x1C a chain attack landed: learn it on the next exchange
    ubyte _pad1d[3];
    int chain_count;             // +0x20 chain attacks in a row (also the damage bonus)
    int chain_index;             // +0x24 zeroed when the chain starts
    int last_chain_skill;        // +0x28 SKILL_NONE when no chain
    bool attack_turn;            // +0x2C set for the side that attacks next (combat_resolve_exchange)
    ubyte _pad2d[3];
    move_info *defense_move;     // +0x30 parry / dodge move (pick_parry, pick_dodge)
    int anim;                    // +0x34 anim of the playing move (-1 failed)
    int chain_skill;             // +0x38 chosen combat skill (SKILL_NONE)
    int skill;                   // +0x3C skill of the current attack (SKILL_NONE)
    int hits;                    // +0x40 hits that got through the defence roll
    int flinch_anim;             // +0x44 anim of play_flinch_secondary
    float pos_mod;               // +0x48 position modifier of the last hit (combat_position_modifier)
    float cooldown;              // +0x4C seconds until the next one-sided attack (update_attack)
    bool backstab_full;          // +0x50 the backstab target was not in combat mode (update_special)
    ubyte _pad51[3];
};
LAYOUT_CHECK_32(living_combat, sizeof(living_combat) == 0x54);

class console_command;
extern console_command ccmd_Super_power;            // 0x941720 (initialiser 0x464010 / 0x464020)
void  cmd_Super_power();                            // 0x464040
extern bool Super_power;                            // 0x94173d (party members never miss and hit for 200)

// (record, opponent): the opponent record +0x08 is set for the call and restored.
bool  living_combat_test_defense(living_combat *c, living_combat *opp);     // 0x463fc0 opp rolls in simulation, true when a hit gets through
bool  living_combat_resolve_hit_vs(living_combat *c, living_combat *opp);   // 0x463ff0
float combat_position_modifier(Character *attacker, Character *defender);   // 0x464d00
int   combat_get_weapon_tag_position(Character *attacker, Character *defender);   // 0x4665f0 bone index or -1
void  character_get_bone_world_matrix(Character *c, int bone, vector3 *out_pos, matrix *out_orient);  // 0x466920
void  combat_spawn_hit_vfx(Character *attacker, Character *defender);      // 0x4669c0
void  combat_play_weapon_hit_sound(Character *attacker, Character *defender);   // 0x466a80
void  combat_spawn_spark_vfx(Character *attacker, Character *defender);    // 0x466e90
void  character_break_paralysis_on_hit(Character *c);                      // 0x467db0

#endif
