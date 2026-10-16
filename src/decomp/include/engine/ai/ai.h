// engine/ai/ai.h - the AI sub-object of a character and AI groups (ai.cpp in sum.symbols, code
// 0x471ff0-0x475178: the unit opens with its static initialiser thunks, decomp39; directory from the
// engine\ai\pathfinding.cpp path). Not original source.
// Layout: symbols/types.h ai_state / ai_group, with the GameTimer fields typed as GameTimer and +0x1a8
// named `group` (player.cpp reads the character's group there).
#ifndef ENGINE_AI_AI_H
#define ENGINE_AI_AI_H

#include "vsdk/vsdk.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "engine/ai/pathfinding.h"      // decomp32: rect2d, pf_edge, pf_vis_node for the avoidance records

class Character;
class vector3;
struct boss_target;                    // engine/ai/boss_ai.h (the AI spell code uses the same records)

// One candidate spell of ai_choose_offensive_spell (8 bytes on its stack): the score is the running
// total when appended, normalised before the pick. Type and field names ours (decomp56).
struct ai_spell_choice {
    int spell;
    float score;
};

// An AI group (10 x 0x54 bytes at 0x243e040; Character +0x4cc points at one). Saved in the LENT chunk.
struct ai_group {
    int count;               // +0x00
    int members[6];          // +0x04 handles
    // decomp54 (D3400): ai_group_collect_enemies fills +0x1c / +0x20 with the enemies the members see
    // (was _unk1c[0x2c]).
    int num_enemies;         // +0x1C
    int enemies[10];         // +0x20 handles
    int leader;              // +0x48 leader's object handle
    unsigned int flags;      // +0x4C AI_GROUP_* (decomp54)
    GameTimer timer;         // +0x50
};
LAYOUT_CHECK(ai_group, sizeof(ai_group) == 0x54);

// A goal slot (ai_state +0x8c, five of 0x34 bytes). Fields from ai_goal_alloc's stores (0x47609e-
// 0x47611e), ai_state_init and ai_state_save / _load (decomp39, D1902). Type and field names ours.
struct ai_path;
struct ai_goal {
    int type;                         // +0x00 AI_GOAL_* (0 free)
    unsigned int flags;               // +0x04 bit 0 kept by ai_goal_alloc, bit 1 set by it, 0x10 walk, 0x200 run
    int id;                           // +0x08 from ai_goal_next_id; ai_state::cur_goal holds the running one
    float time;                       // +0x0C level_session_time when added
    int priority;                     // +0x10 -1 in a cleared slot
    vector3 pos;                      // +0x14 point argument
    int layer;                        // +0x20
    int spell;                        // +0x24 AI_NO_SPELL (0x6c) unless a cast goal
    int target;                       // +0x28 target handle, -1 none (saved as a name hash)
    float arg;                        // +0x2C goal_add's last argument (30.0f at boss_release_mount)
    ai_path *path;                    // +0x30 freed by ai_goal_remove (not saved)
};
LAYOUT_CHECK(ai_goal, sizeof(ai_goal) == 0x34);
#define AI_NO_SPELL 0x6c              // the spell argument of the non-cast goals (name ours)

// Goal types (ai_state::goal_add's first argument; names from the goal-name table 0x59ae98, decomp39).
#define AI_GOAL_ATTACK            1
#define AI_GOAL_FOLLOW            2
#define AI_GOAL_CAST_SPELL        3
#define AI_GOAL_CAST_TARGETED     4
#define AI_GOAL_CAST_LOCATION     5
#define AI_GOAL_SWITCH_COMBAT     6
#define AI_GOAL_SWITCH_NORMAL     7
#define AI_GOAL_PROTECT           8
#define AI_GOAL_PATHFIND          9
#define AI_GOAL_TURN_TO           10
#define AI_GOAL_RUN_AWAY          11
#define AI_GOAL_GOTO_NAVPOINT     12   // ai_goal_add_move_to
#define AI_GOAL_WANDER            13   // ai_goal_add_move_to_timed
#define AI_GOAL_BACKSTAB          14
#define AI_GOAL_SNEAK             15
#define AI_GOAL_HIDE              16
#define AI_GOAL_PICKUP_ITEM       17

// The AI sub-object at Character +0x324 (ai_state__ctor 0x4722f0). thiscall members on this+0x324.
class ai_state {
public:
    // ai.cpp (decomp39), in address order; the older declarations below keep their places.
    ai_state();                       // 0x4722f0 ai_state__ctor (constructs the GameTimer members)
    void call_for_help();             // 0x472200 ai_call_for_help
    void init();                      // 0x472380 ai_state_init (living_entity_init)
    void shutdown();                  // 0x472420 ai_state_shutdown
    void reset_party_member();        // 0x4725e0 ai_reset_party_member
    int  count_attack_slots_used();   // 0x4726a0 ai_count_attack_slots_used
    void attack_slot_direction(vector3 *out, int slot);   // 0x4726d0 ai_attack_slot_direction (this unused)
    int  pick_attack_slot(Character *attacker);           // 0x472780 ai_pick_attack_slot
    // (self, attacker, ms, slot -1 = best): self is the character the slots belong to.
    void claim_attack_slot(Character *self, Character *attacker, int ms, int slot);   // 0x472840
    void offset_point_sideways(const vector3 *pos, const vector3 *offset);   // 0x4729b0 -> travel_dest
    bool is_following_leader();       // 0x473100 ai_is_following_leader
    bool is_combat_sub16_17();        // 0x473190 ai_is_combat_sub16_17
    bool is_combat_sub16();           // 0x4731b0 ai_is_combat_sub16
    int  set_mode(int mode, int sub); // 0x473320 ai_set_mode: 0 when refused or unchanged, else 1
    bool path_get_destination(vector3 *out);              // 0x473660 ai_path_get_destination
    int  count_engaged_attackers(bool ignore_goals);      // 0x4736a0 ai_count_engaged_attackers
    int  count_attackers();           // 0x473750 ai_count_attackers
    void go_inactive();               // 0x473880 ai_go_inactive
    void go_mount_idle();             // 0x473940 ai_go_mount_idle
    void travel_to_xyz(vector3 dest);                      // 0x473a40 ai_travel_to_xyz
    void travel_to_point(vector3 *pos, int priority);     // 0x473a90 ai_travel_to_point
    void path_begin();                // 0x473ac0 ai_path_begin
    ai_path *set_path(vector3 *start, ai_path *p);        // 0x473bc0 ai_set_path
    ai_path *path_find_new(vector3 *from, int from_layer, vector3 *to, int to_layer, int size,
                           float cost_bias);              // 0x473c40 ai_path_find_new
    ai_path *path_find_append(vector3 *from, int from_layer, vector3 *to, int to_layer, int size,
                              float cost_bias);           // 0x473d80 ai_path_find_append
    // 0 with a path, -1 without (from_layer < 0 frees the path). The fifth argument is not read.
    int  path_to(vector3 *from, int from_layer, vector3 *to, int to_layer, int unused, int size,
                 bool append, bool keep_flag);            // 0x473e50 ai_path_to
    int  add_travel(vector3 *pos, int layer, int priority, bool walk);   // 0x473f40 ai_goal_add_travel
    int  set_combat_mode_goal(bool on, int priority);     // 0x473fd0 ai_set_combat_mode_goal
    int  add_move_to(vector3 *pos, int layer, bool walk, int priority);  // 0x474160 ai_goal_add_move_to
    int  knock_down_local(int ms);    // 0x4742b0 ai_knock_down_local: 0 when refused
    int  disable_2f_local();          // 0x474390 ai_disable_2f_local (pushed): 0 when refused
    void disable_31();                // 0x474410 ai_disable_31 (stunned)
    void attack_target(Character *t); // 0x4744a0 ai_attack_target
    void attack_target_sub10(Character *t);   // 0x4744e0 ai_attack_target_sub10
    void net_attack_target(Character *t);     // 0x474520 ai_net_attack_target
    void start_travel(bool walk);     // 0x474580 ai_start_travel
    void charge_target(Character *t); // 0x4745d0 ai_charge_target
    void travel_to_current_dest();    // 0x474620 ai_travel_to_current_dest
    void enter_sneaking();            // 0x474650 ai_enter_sneaking
    void enter_hiding(int enter);     // 0x474690 ai_enter_hiding (0 = leave)
    int  save(ubyte *out);            // 0x4746d0 ai_state_save, bytes written
    int  load(const ubyte *in);       // 0x4747e0 ai_state_load, bytes read
    void enter_combat();              // 0x474fc0 ai_enter_combat
    bool is_disabled_2c();            // 0x474fd0 ai_is_disabled_2c
    bool is_knocked_down();           // 0x474fe0 ai_is_knocked_down
    bool is_disabled_2e();            // 0x474ff0 ai_is_disabled_2e
    bool can_be_knocked_down();       // 0x475030 ai_can_be_knocked_down
    // The units after ai.cpp (module "-" in sum.symbols; decomp39 calls).
    void think();                     // 0x4b7610 ai_think
    void mode_traveling_frame();      // 0x4b4100 ai_mode_traveling_frame
    void mode_cast_spell_frame();     // 0x4b4590 ai_mode_cast_spell_frame
    void mode_disabled_frame();       // 0x4b4ad0 ai_mode_disabled_frame
    void mode_hiding_frame();         // 0x4b4d80 ai_mode_hiding_frame
    void mode_sneaking_frame();       // 0x4b4eb0 ai_mode_sneaking_frame
    void mode_combat_frame();         // 0x4bff60 ai_mode_combat_frame
    // (type, priority, flags, target handle, pos, layer, spell, arg): the slot index or -1.
    int  goal_add(int type, int priority, unsigned int flags, int target, vector3 *pos, int layer,
                  int spell, float arg);                  // 0x476190 ai_goal_add (ret 0x20)
    int  goal_alloc(int type, int priority, unsigned int flags, int target, vector3 *pos, int layer,
                    int spell);                           // 0x476020 ai_goal_alloc
    // decomp58: returns AL (false for a slot outside 0..4; was void).
    bool goal_remove(int slot);       // 0x476300 ai_goal_remove
    bool has_goal_without_flag20();   // 0x476670 ai_has_goal_without_flag20 (decomp41: script_character_has_pending_goal)
    // decomp58 (the goal unit 0x475180, game/unnamed_goals_doors_0x475180.cpp):
    void goals_remove_type9();        // 0x475dd0 ai_goals_remove_type9
    void goals_remove_types_3_5();    // 0x475e30 ai_goals_remove_types_3_5
    // The goal handlers (0x4b6000-0x4b68c0; argument lists from ai_process_goals). decomp68 (D4803): the
    // ones that set EAX return int (-1 no target, 0 started, 1 nothing to do; ai_process_goals ignores it).
    // Unit game/unnamed_ai_think_0x4b5140.cpp (file name a placeholder).
    int  goal_do_attack(int target, unsigned int flags);                  // 0x4b6000 ai_goal_do_attack
    int  goal_do_follow(int target);                                      // 0x4b6240 ai_goal_do_follow
    int  goal_do_travel(vector3 *pos, int layer, int priority, unsigned int flags);   // 0x4b62a0 ai_goal_do_travel (path_to's result)
    int  goal_do_nothing();                                               // 0x4b63e0 ai_goal_do_nothing (-1)
    int  goal_do_approach_party(int target);                              // 0x4b63f0 ai_goal_do_approach_party
    int  goal_do_combat_mode();                                           // 0x4b6430 ai_goal_do_combat_mode
    int  goal_do_cast_spell(int spell);                                   // 0x4b64c0 ai_goal_do_cast_spell (0)
    int  goal_do_cast_at_point(int spell, vector3 *pos, int layer);       // 0x4b6560 ai_goal_do_cast_at_point (0)
    int  goal_do_move_to(vector3 *pos, int layer, bool walk, ai_path *path);    // 0x4b65c0 ai_goal_do_move_to (0)
    void goal_do_move_to_timed(vector3 *pos, int layer, float arg, bool walk);  // 0x4b6690 ai_goal_do_move_to_timed
    void goal_do_hide();                                                  // 0x4b6710 ai_goal_do_hide
    void goal_do_sneak();                                                 // 0x4b6790 ai_goal_do_sneak
    void goal_do_special_attack(int target);                              // 0x4b6810 ai_goal_do_special_attack
    void goal_do_pickup(int target);                                      // 0x4b68c0 ai_goal_do_pickup

    ~ai_state();                      // 0x4f13e0 (folded)
    bool is_traveling_sub1();         // 0x4730e0 ai_is_traveling_sub1
    // (out, base, dir, slot 1..5, -1 = this AI's formation_slot): base + the slot offset turned to dir.
    void formation_slot_position(vector3 *out, const vector3 *base, const vector3 *dir, int slot);   // 0x4b52e0 (unnamed_ai_think_0x4b5140.cpp)
    void reset();                     // 0x472450 ai_reset
    void process_goals();             // 0x4763a0 ai_process_goals (combat_mode_set; decomp42)
    bool is_combat_e_or_sneak_attack(); // 0x473120 ai_is_combat_e_or_sneak_attack
    bool is_mode(int mode, int sub);  // 0x473200 ai_is_mode (sub -1 = any)
    int  get_mode();                  // 0x473600 ai_get_mode
    void set_18(int style);           // 0x473610 ai_set_18 (combat style, +0x18)
    int  get_combat_style();          // 0x473620 ai_get_combat_style
    bool is_casting();                // 0x4731d0 ai_is_casting
    void go_idle();                   // 0x4738c0 ai_go_idle
    void goals_mark_keep();           // 0x4766a0 ai_goals_mark_keep (flag 0x40 on every used goal; decomp52)
    // decomp43 (level_script_lpalaceint.cpp): (out, seconds) the owner's position + direction * speed *
    // seconds (pushes &out, 0.5f at 0x4f3447-0x4f345f).
    void predict_owner_position(vector3 *out, float seconds);   // 0x4ca650 ai_predict_owner_position
    // decomp35 (gametarget.cpp):
    bool can_backstab_target(Character *target);   // 0x4737a0 ai_can_backstab_target
    int  add_attack(Character *target, int priority);   // 0x473f70 ai_goal_add_attack (-1 when refused)
    bool is_disabled_any();           // 0x475000 ai_is_disabled_any
    void goals_remove_type1();        // 0x475e00 ai_goals_remove_type1
    // decomp20 (conversationctl.cpp):
    bool is_traveling_sub0();         // 0x4730c0 ai_is_traveling_sub0
    // (mask, type, ...): goal slot index or -1. The conversation calls pass (1, type, 0, 0, -1, 0, 0, 0x6c).
    // decomp39: the arguments after the mask are goal_add's first seven (type, priority, flags, target,
    // pos, layer, spell); ai.cpp passes masks 1, 3, 4 and 0x41, ai_goal_add 0x1f. Parameter names kept.
    // decomp44 (D2306): d is the position pointer (script_entry_check_done passes &navpoint->pos).
    int  goal_find(int mask, int type, int a, int b, int c, vector3 *d, int e, int f);  // 0x475eb0 ai_goal_find
    void goals_clear();               // 0x476380 ai_goals_clear
    // decomp39: the goal_add wrappers return its slot index (or -1); the callers above ignore it.
    int  add_cast_spell(int spell, int priority);   // 0x4740c0 ai_goal_add_cast_spell
    bool is_attacking();              // 0x473150 ai_is_attacking
    int  add_cast_at_target(int spell, Character *target, int priority);   // 0x4740e0 ai_goal_add_cast_at_target
    int  add_cast_at_point(int spell, vector3 *pos, int layer, int priority); // 0x474130 ai_goal_add_cast_at_point
    int  add_skill_from_behind(Character *target, int priority);           // 0x4741d0 ai_goal_add_skill_from_behind
    int  add_hide();                  // 0x474440 ai_goal_add_hide (toggles; 0 when it left hiding)
    int  add_sneak();                 // 0x474480 ai_goal_add_sneak
    // decomp23 (combat.cpp, spells.cpp):
    bool is_combat_sub10();           // 0x473170 ai_is_combat_sub10 (ranged attack)
    bool is_combat_sub11();           // 0x4731e0 ai_is_combat_sub11 (special attack)
    void class_die();                 // 0x4739e0 ai_class_die
    void knock_down(int ms);          // 0x474350 ai_knock_down
    void disable_2f();                // 0x4743e0 ai_disable_2f
    // decomp38 (characterinfo.cpp, summon_stash_for_cutscene):
    void begin_emerge();              // 0x473970 ai_begin_emerge
    void summon_emerge_update();      // 0x4b4660 summon_emerge_update (unnamed_ai_move_0x4b1410.cpp)
    void retaliate_on_hit(int attacker);  // 0x4b5790 ai_retaliate_on_hit (unnamed_ai_think_0x4b5140.cpp)
    // decomp54 (ai_group_think_caster): (candidates, count, out chosen candidate or NULL) -> spell or -1.
    // The candidates are boss_target records (engine/ai/boss_ai.h). Unit not known.
    int  choose_support_spell(struct boss_target *list, int count, struct boss_target **out);   // 0x4bea80 ai_choose_support_spell
    // decomp29 (boss_ai.cpp). (pos, layer, 30.0f, 0, 5000) at boss_release_mount; return unused.
    // decomp39 (D1903): f is goal_add's float argument, walk a byte (`cmp dl, 1`), the last the priority.
    int  add_move_to_timed(vector3 *pos, int layer, float f, bool walk, int ms);   // 0x474190 ai_goal_add_move_to_timed
    // decomp44 (script_internal.cpp, script_entry_jump_to_sequence):
    void goals_remove_unmarked();     // 0x4766c0 ai_goals_remove_unmarked (every used goal without flag 0x40)

    // boss_ai.cpp (decomp29): the species dispatchers that are thiscall on the AI record.
    void boss_ai_unload();            // 0x4a04d0 (ai_state_shutdown)
    void boss_ai_load();              // 0x4a0600 (ai_reset)
    void boss_ai_check_stage();       // 0x4a0730
    void boss_ai_idle_dispatch();     // 0x4a08c0
    void boss_ai_frame();             // 0x4a09f0
    void boss_threat_decay();         // 0x4a1310
    float boss_turn_rate_override();  // 0x4a1c50 (0 = no override)
    int  boss_get_attack_move_id();   // 0x4a2070
    void boss_get_move_range(float *min, float *max);   // 0x4a26f0 (ret 8; also sets attack_range)

    // The per-boss AI functions the dispatchers call (thiscall on the AI record: the dispatchers load
    // ECX with it or tail-jump with it in ECX). Their units are not known (module "-" in sum.symbols;
    // bosses.md gives the code ranges). The ones marked "folded" are empty bodies that identical-code
    // folding merged into empty_function (summoner.h); their names are ours.
    void luminar_boss_load();         // 0x4c8dc0
    void tiger_boss_load();           // 0x49ad20
    void giant_salamanka_boss_load(); // 0x4c83a0
    void serpent_boss_load();         // 0x495960
    void phoenix_boss_load();         // 0x498420
    void ghost_rider_boss_load();     // 0x4ba260
    void tentacle_beast_boss_load();  // 0x4c6d10
    void titus_boss_load();           // 0x4c2380
    void pyrul_boss_load();           // 0x4b8a10
    void murod_oni_boss_load();       // 0x4c5cc0
    void murod_boss_load();           // 0x4c56c0
    void evil_urath_boss_load();      // 0x4c45b0
    void evil_joseph_boss_load();     // 0x4c35e0
    void machival_boss_load();        // 0x4c0f60
    void tiger_check_stage();         // 0x49a6c0
    bool tiger_seat_rider();          // 0x49aeb0 (decomp54: returns al; was void)
    void giant_salamanka_check_stage(); // 0x4c8560
    void serpent_check_stage();       // 0x495b90
    bool serpent_seat_rider();        // 0x495e10 (decomp54: returns al, true when seated; was void)
    void serpent_part_check_timeout();// 0x497610
    void phoenix_check_stage();       // 0x499690
    bool phoenix_seat_rider();        // 0x4982a0 (decomp54: returns al; was void)
    void ghost_update_stage();        // 0x4bb4c0
    bool ghost_seat_rider();          // 0x4ba8e0 (decomp56: returns true when the rider was placed; was void, D3600)
    void tentacle_beast_check_stage();// 0x4c7360
    void titus_check_stage();         // 0x4c24c0
    void pyrul_check_stage();         // 0x4b8b90
    void murod_oni_check_stage();     // folded (boss_ai_check_stage case 0xb8)
    void murod_check_stage();         // 0x4c5750
    void evil_urath_check_stage();    // 0x4c47e0
    void evil_urath_hold_height();    // 0x4c46f0
    void evil_joseph_check_stage();   // 0x4c3f20
    void evil_joseph_check_stage_b(); // folded (the second call of boss_ai_check_stage case 0xc9)
    void machival_check_stage();      // 0x4c19c0
    void luminar_ai_idle();           // 0x4c95d0
    void tiger_ai_idle();             // 0x49b040
    void giant_salamanka_ai_idle();   // folded (boss_ai_idle_dispatch case 0xac)
    void serpent_ai_idle();           // 0x495fa0
    void phoenix_ai_idle();           // 0x4989e0
    void ghost_ai_idle();             // 0x4baa60
    void tentacle_beast_ai_idle();    // 0x4c7060
    void titus_ai_idle();             // 0x4c2e60
    void pyrul_ai_idle();             // 0x4b89d0
    void murod_oni_ai_idle();         // 0x4c5c80
    void murod_ai_idle();             // 0x4c5680
    void evil_urath_ai_idle();        // 0x4c4710
    void evil_joseph_ai_idle();       // 0x4c3800
    void machival_ai_idle();          // 0x4c0f20
    void luminar_ai_frame();          // 0x4c95f0
    void tiger_ai_frame();            // 0x49bed0
    void giant_salamanka_ai_frame();  // 0x4c86b0
    void serpent_ai_frame();          // 0x4973e0
    void phoenix_ai_frame();          // 0x499410
    void ghost_ai_frame();            // 0x4bb380
    void tentacle_beast_ai_frame();   // 0x4c7070
    void titus_ai_frame();            // 0x4c2630
    void pyrul_ai_frame();            // 0x4b8ce0
    void murod_oni_ai_frame();        // 0x4c60e0
    void murod_ai_frame();            // 0x4c5a60
    void evil_urath_ai_frame();       // 0x4c4760
    void evil_joseph_ai_frame();      // 0x4c3860
    void machival_ai_frame();         // 0x4c1070
    // decomp54: the serpent unit's thiscall functions (game/unnamed_serpent_0x495290.cpp).
    int  serpent_choose_attack();     // 0x496330 0 no fight, 1 wander / place, 3 attack, 5 busy
    void serpent_wander_update();     // 0x496b00
    void serpent_illusion_start();    // 0x496ba0
    void serpent_illusion_update();   // 0x496d60
    bool serpent_place_parts();       // 0x4970a0
    bool serpent_wander_start();      // 0x497230
    // decomp54: the phoenix unit's thiscall functions (game/unnamed_phoenix_0x497ac0.cpp).
    void phoenix_set_dive_point();    // 0x497d30
    bool phoenix_reposition_update(); // 0x497da0
    void phoenix_wander_update();     // 0x4981a0
    void phoenix_end_move_3a();       // 0x498210
    void phoenix_end_move_3b();       // 0x498250
    int  phoenix_choose_attack();     // 0x498a20 0 no fight, 1 pending sub-mode, 3 attack, 5 busy
    void phoenix_stage_transition_move(); // 0x4995b0
    // The AI mode units' combat helpers the bosses call (units not known; decomp54).
    // decomp68 (D4802): the argument is the wanted speed, a float (0 = the stopping distance decides);
    // was int a.
    bool follow_path(float speed);    // 0x4b3010 ai_follow_path (the bosses pass 0)
    bool combat_approach(Character *t);       // 0x4bda50 ai_combat_approach (tiger_wander_update tests al)
    // decomp68 (D4802): the second argument is a bool (`mov al, [esp+0xb8]`): use boss_turn_rate_override.
    bool turn_towards(vector3 *point, bool boss_rate); // 0x4b2af0 ai_turn_towards (the tiger passes 1)
    void alert_guards_of_target(Character *attacker);   // 0x4bfed0 ai_alert_guards_of_target
    // decomp56 (the Pyrul and Ghost Rider units): their think helpers.
    int  pyrul_choose_attack();       // 0x4b8da0 0 none, 3 approach with a new move, 5 keep the current move
    int  ghost_choose_attack();       // 0x4bac20 0 none, 1 wander (ghost_pending_submode), 3 new move, 5 keep
    void ghost_wander_update();       // 0x4baaa0 (sub-mode AIS_SAFETY)
    bool ghost_wander_start();        // 0x4baae0
    int  goal_do_cast_at_target(int spell, int target);   // 0x4b6500 ai_goal_do_cast_at_target: 0, or -1 without the target

    // decomp68: the AI movement unit 0x4b1410-0x4b513f (game/unnamed_ai_move_0x4b1410.cpp) and the AI think
    // unit 0x4b5140-0x4b77ef (game/unnamed_ai_think_0x4b5140.cpp); file names placeholders. Names from
    // sum.symbols; the four empty think hooks are ours.
    bool avoid_characters();          // 0x4b1aa0 ai_avoid_characters: true = steering around someone
    void steer_jitter(float dt);      // 0x4b2510 ai_steer_jitter (travel_dest = ahead + the random offset)
    void path_closest_point(float max_dist);  // 0x4b2670 ai_path_closest_point
    void wall_avoid_steer();          // 0x4b2890 ai_wall_avoid_steer
    void travel_steer_update();       // 0x4b3c80 ai_travel_steer_update (sub-mode 1)
    void cast_spell_update();         // 0x4b4920 ai_cast_spell_update
    void group_assign_formation();    // 0x4b5160 ai_group_assign_formation (the leader)
    void pick_attack_target(int max_attackers);   // 0x4b5900 ai_pick_attack_target
    // (self = the owner, target): 0 re-pathed or still on the way, -1 no path.
    int  follow_repath(Character *self, Character *target);   // 0x4b5a80 ai_follow_repath
    // -1 no target, 0 moving / in place, 1 close enough or no path.
    int  follow_character(Character *target);  // 0x4b5bd0 ai_follow_character
    void party_follow_think();        // 0x4b6940 ai_party_follow_think
    void clear_target_and_idle();     // 0x4b6a30 ai_clear_target_and_idle (priority -1, then go_idle)
    void party_spread_attackers();    // 0x4b6c10 ai_party_spread_attackers (this not read)
    void think_party_update();        // 0x4b71c0 ai_think_party_update
    // ai_think's per-mode hooks: empty bodies, folded into empty_function 0x4f13e0 (names ours).
    void think_idle();                // folded (AI_STATE_IDLE)
    void think_travel_wait();         // folded (AI_STATE_TRAVELING sub-mode 0)
    void think_travel_escort();       // folded (AI_STATE_TRAVELING sub-modes 6 / 7)
    void think_combat();              // folded (AI_STATE_COMBAT)
    // decomp56 (the AI combat unit 0x4bce60-0x4c0245, game/unnamed_ai_combat_0x4bce60.cpp; file not known).
    // combat_approach, alert_guards_of_target, combat_close_in / _melee_attack / _ranged_attack and
    // choose_support_spell are declared with the decomp54 declarations.
    bool within_attack_radius(float dist);   // 0x4bce80 ai_state__within_attack_radius: dist < attack_range + 0.2 (D3603)
    bool approach_random_offset(Character *target);   // 0x4bcea0 ai_approach_random_offset
    bool approach_target_point(Character *target);    // 0x4bcff0 ai_approach_target_point
    bool approach_attack_slot(Character *target);     // 0x4bd2a0 ai_approach_attack_slot
    bool combat_start_approach(Character *target, bool random_offset);   // 0x4bd9c0 ai_combat_start_approach
    bool combat_toggle_ready_stance();                // 0x4bdce0 ai_combat_toggle_ready_stance (true: done)
    int  chase_target_until_in_range(Character *target, float range);   // 0x4be0a0 1 in range, 0 chasing, -1 no path / target
    bool combat_face_target(Character *target);       // 0x4be470 ai_combat_face_target (false: still turning)
    void combat_follow_and_strike(Character *target); // 0x4be840 ai_combat_follow_and_strike
    void combat_close_strike(Character *target);      // 0x4be8a0 ai_combat_close_strike
    // (spell, record, candidates, candidate count, running total): appends {spell, running total}.
    void rate_spell_targets(int spell, boss_target *t, ai_spell_choice *out, int *count, float *total);   // 0x4beba0
    int  consider_blind_or_silence(boss_target *list, int count, boss_target **out);   // 0x4bee40 7, 8 or -1
    int  consider_area_spell(int spell, boss_target *list, int count);          // 0x4befe0 spell or -1
    // The next five take the current target record (pushed by value; not read): their spell or -1.
    int  consider_power_leech(boss_target *list, int count, boss_target *cur);  // 0x4bf0c0 0x11
    int  consider_death(boss_target *list, int count, boss_target *cur);        // 0x4bf1d0 0xc
    int  consider_poison(boss_target *list, int count, boss_target *cur);       // 0x4bf320 6
    int  consider_jade_beam(boss_target *list, int count, boss_target *cur);    // 0x4bf460 0x18
    int  consider_life_drain(boss_target *list, int count, boss_target *cur);   // 0x4bf5a0 0xd
    int  choose_offensive_spell(boss_target *list, int count, boss_target **out);   // 0x4bf700 spell or -1
    void combat_cast_frame(Character *target);        // 0x4bfaa0 ai_combat_cast_frame (AIS_CHOOSE_SPELL)
    // decomp54: the tiger unit's thiscall functions (game/unnamed_tiger_0x499e50.cpp).
    bool tiger_wander_start(int target);      // 0x49a9b0 (ret 4)
    int  tiger_choose_attack();               // 0x49b350 0 none, 3 move chosen, 5 busy
    void tiger_combat_update(Character *t);   // 0x49b6e0 sub-mode 0x1f (ret 4)
    void tiger_approach_target(Character *t); // 0x49b9e0 sub-mode 0x1e (ret 4)
    void tiger_wander_update();               // 0x49ba70 sub-mode 0x21
    void tiger_reposition_update(Character *t); // 0x49bce0 sub-mode 0x20 (ret 4)
    bool tiger_stage_move_update();           // 0x49c240 sub-mode 0xe: true when the weapon move ended
    void combat_close_in(Character *t);       // 0x4be280 ai_combat_close_in (sub-modes 0x16 / 0x17)
    void combat_melee_attack(Character *t);   // 0x4be5f0 ai_combat_melee_attack (sub-mode 0xf)
    void combat_ranged_attack(Character *t);  // 0x4be760 ai_combat_ranged_attack (sub-mode 0x10)

    // decomp59: the boss units 0x4c0c40-0x4ca6ff (game/unnamed_<boss>_<address>.cpp; files not known).
    // Machival:
    int  machival_ai_select_action(); // 0x4c13a0 0 none, 1 cast, 3 new move, 4 nothing, 5 busy
    void machival_cast_spell_at(int spell, vector3 *pos, int layer);   // 0x4c18c0 (ret 0xc)
    void machival_cast_spell(int spell, Character *target);           // 0x4c1940 (ret 8)
    void machival_ai_melee(Character *target);  // 0x4c19f0 sub-mode 0xf
    bool machival_teleport(int index);          // 0x4c1e20 (mp_handle_swi_boss passes the packet index)
    void machival_ai_special_move();            // 0x4c1f90 sub-mode 0x33
    void machival_ai_wait_move_end();           // 0x4c2090 sub-mode 0x34
    // Titus:
    int  titus_ai_select_action();    // 0x4c2790 0 none, 1 eruption pending, 3 new move, 5 keep
    void titus_ai_rock_erupt();       // 0x4c3240 sub-mode 0x3c
    void titus_start_rock_erupt();    // 0x4c32a0 sub-mode 0x3d
    void titus_ai_wait_rock_erupt_fx(); // 0x4c32b0 sub-mode 0x3e
    // Evil Joseph:
    int  evil_joseph_ai_select_action(); // 0x4c39b0 0 none, 1 walk to a teleport point, 3 new move, 5 keep
    // Evil Urath:
    void evil_urath_cast_spell(int spell, Character *target);   // 0x4c43f0 (ret 8)
    void evil_urath_ai_leap_start();  // 0x4c4810 sub-mode 0x23
    void evil_urath_ai_leap_rise();   // 0x4c4870 sub-mode 0x24
    void evil_urath_ai_leap_travel(); // 0x4c4ad0 sub-mode 0x25
    void evil_urath_ai_leap_land();   // 0x4c4ca0 sub-mode 0x26
    int  evil_urath_ai_select_action(); // 0x4c4db0 0 none, 1 spell or leap, 4 nothing, 5 busy
    // Murod:
    bool murod_start_leap();          // 0x4c51a0 false: no navigable point (retry in 5 s)
    void murod_ai_leap();             // 0x4c53c0 sub-mode 0x33
    void murod_end_move();            // 0x4c5550 sub-mode 0x34
    bool murod_try_self_heal();       // 0x4c5570
    void murod_cast_spell(int spell, Character *target);   // 0x4c5600 (ret 8)
    int  murod_ai_select_action();    // 0x4c57b0 0 none, 1 cast / leap, 5 busy
    // Murod Oni:
    int  murod_oni_ai_select_action(); // 0x4c5de0 0 none, 3 new move, 5 keep
    // Tentacle beast (thiscall on a tentacle's AI record):
    int  tentacle_beast_ai_select_action(); // 0x4c6520 3 move chosen, 4 none in reach, 5 busy
    void tentacle_beast_start_grab();       // 0x4c6800
    void tentacle_beast_ai_grab(Character *victim);   // 0x4c6850 sub-mode 0x35 (ret 4)
    void tentacle_beast_start_release();    // 0x4c69e0 (tentacle_begin_sink calls it on the tentacle)
    void tentacle_beast_ai_release();       // 0x4c6a80 sub-mode 0x37
    // Giant Salamanka:
    int  giant_salamanka_ai_select_action(); // 0x4c87e0 2 spell move, 3 melee move, 4 none, 5 busy
    // Luminar (luminar_boss_load, _ai_idle, _ai_frame are declared above; predict_owner_position 0x4ca650
    // is defined in the Luminar unit):
    void luminar_ai_select_action(Character *t);   // 0x4c9410 (ret 4; no result)
    int  luminar_choose_melee_move(Character *t);  // 0x4c97b0 move id 0x83 / 0x84 / 0x85, 0x88 carrying
    bool luminar_ai_melee(Character *t);           // 0x4c98a0 sub-mode 0xf (true: choose afterwards)
    bool luminar_try_start_grab(Character *t);     // 0x4c9990
    bool luminar_ai_grab_approach(Character *t);   // 0x4c9a40 sub-mode 0x36
    void luminar_ai_grab(Character *t);            // 0x4c9b00 sub-mode 0x35
    void luminar_start_release();                  // 0x4c9c40
    void luminar_ai_release();                     // 0x4c9cd0 sub-mode 0x37
    bool luminar_teleport_out();                   // 0x4c9da0
    void luminar_teleport_in();                    // 0x4ca000 sub-mode 0x33
    void luminar_ai_beam_move_end();               // 0x4ca1d0 sub-mode 0x34
    bool luminar_beam_start(Character *t);         // 0x4ca230
    void luminar_beam_hit(Character *t);           // 0x4ca3a0 sub-mode 0x32

    int mode;               // +0x00 enum ai_state_mode
    int submode;             // +0x04
    int prev_mode;           // +0x08
    int prev_submode;        // +0x0C
    int pending_mode;        // +0x10
    int pending_submode;     // +0x14
    int combat_style;        // +0x18 ai_get_combat_style / ai_set_18
    GameTimer timer;         // +0x1C restarted by ai_set_mode
    vector3 approach_pos;    // +0x20 the target position when an approach starts (decomp56, D3602; was _unk20 / _unk24)
    int approach_layer;      // +0x2C
    GameTimer timer30;       // +0x30
    float attack_range;      // +0x34 reach of the current boss move (boss_get_move_range; was _unk34, D900)
    int priority;           // +0x38 999999 for direct orders
    GameTimer timer3c;       // +0x3C
    GameTimer timer40;       // +0x40
    GameTimer timer44;       // +0x44
    int _unk48;              // +0x48 the stun time of sub-mode 0x30 (status 10 when the move ends; decomp68)
    Character *owner;        // +0x4C
    unsigned int flags;      // +0x50 AI_FLAG_*; 0x10000000 path started (ai_path_begin)
    vector3 travel_dest;     // +0x54 (was float[3]; decomp39)
    int target;              // +0x60 target handle
    int prev_target;         // +0x64
    vector3 cast_point;      // +0x68 point of a cast at a location (decomp54: phoenix_cast_special sets it,
                             //       phoenix_special_strike places the ground effect there; was _unk68[0x14])
    int cast_arg;            // +0x74 passed with it to spell_place_ground_effect (its owner argument)
    float wander_radius;     // +0x78 sub-mode 8 wander distance (goal_do_move_to_timed's arg; decomp68, was ubyte[4])
    int _unk7c;              // +0x7C attack point index at the target (its attack_point_users), -1 none
    int handle_slot;         // +0x80 released by ai_set_mode (handle_slot_release)
    int mode_target;         // +0x84 -1 when a cast mode ends
    int _unk88;
    ai_goal goals[5];        // +0x8C goal slots (was ubyte[5][0x34]; decomp39)
    int cur_goal;            // +0x190
    int emerge_move;         // +0x194
    union {
        int _unk198;
        int turn_move;       // +0x198 the turn-in-place move (ai_turn_towards; decomp68)
    };
    GameTimer timer19c;      // +0x19C
    GameTimer disable_timer; // +0x1A0
    int disable_ms;          // +0x1A4
    ai_group *group;         // +0x1A8 (Character +0x4cc; types.h _unk1a8, zeroed by ai_state_init)
    union {
        int _unk1ac;         // +0x1AC
        int formation_slot;  //   1..5, 0 none (ai_group_assign_formation; decomp68)
    };
    struct ai_attacker {     // +0x1B0 attacker slots (named for VC6, D4901)
        int handle;          //   -1 free
        GameTimer timer;
    } attackers[5];
    int attack_point_users[8]; // +0x1D8 per char_info attack point, -1 free (was _unk1d8; decomp39)
    int path_started;        // +0x1F8
    union {
        int _unk1fc;
        int prev_path_started;   // +0x1FC path_started of the last ai_follow_path (decomp68)
    };
    int path_step;           // +0x200
    union {
        int _unk204;         // +0x204
        float steer_offset;  //   ai_steer_jitter's random side offset (decomp68)
    };
    GameTimer perception_timer; // +0x208
    union {
        int _unk20c;
        float steer_side;    // +0x20C turn rate of the avoidance steering (path_started 2; decomp68)
    };
    ai_path *path;           // +0x210 (was void *; decomp39)
    vector3 path_start;      // +0x214 the from point of the search (was path_dest float[3]; D1904)
    union {
        int _unk220;         // +0x220
        int avoid_handle;    //   the character ai_avoid_characters steers around (decomp68)
    };
    GameTimer timer224;      // +0x224
    GameTimer path_timer_a;  // +0x228
    GameTimer path_timer_b;  // +0x22C
    ubyte path_flags;        // +0x230
    ubyte _pad231[3];
};
LAYOUT_CHECK(ai_state, sizeof(ai_state) == 0x234);

#define AI_FLAG_PLAYER_CONTROL 0x1    // ai_state::flags bit 0: player controlled, AI off

// ai_state::mode; names from the game's own string table at 0x59ad60.
enum ai_state_mode {
    AI_STATE_INACTIVE = 0, AI_STATE_IDLE = 1, AI_STATE_TRAVELING = 2, AI_STATE_COMBAT = 3, AI_STATE_CAST_SPELL = 4,
    AI_STATE_PATROL = 5, AI_STATE_SUMMONING = 6, AI_STATE_BANISHING = 7, AI_STATE_DISABLED = 8,
    AI_STATE_HIDING = 9, AI_STATE_SNEAKING = 10, AI_STATE_MOUNT_TRAVEL = 11, AI_STATE_MOUNT_IDLE = 12,
};

ai_group *ai_group_create(Character *c);              // 0x492ba0 the new group (savegame_read_groups; D707)
void ai_groups_reset();                               // 0x492b30 (state_game_enter, savegame_read_living)
extern ai_group ai_groups[10];                        // 0x243e040
extern int ai_goal_next_id;                           // 0x59b7c8 (init 1; saved in the LENT chunk; decomp27)
void ai_group_add_member(ai_group *group, Character *c); // 0x492c50
void ai_group_remove_member(Character *c);            // 0x492ca0

void ai_do_frame();                                   // 0x474950 (objects_do_frame)
void cast_effect_tracks_reset();                      // 0x475090 (unit not known; follows ai_do_frame)
void cast_effect_track_add(int spell, int fx);        // 0x4750b0 (decomp37: character_perform_spell_cast)

extern bool Ai_paused;                                // 0x22e499c
extern char **ai_combat_style_names[6];               // 0x22e49a0 localize_strings slots (init_strptrs_22e49a0)

// ---- ai.cpp (decomp39) ---------------------------------------------------------------------------
// ai_state::mode names and the AIS_* sub-mode names (cmd_ai_state lists the first table; nothing reads
// the second, which has 55 names and 10 empty slots and lacks five values: D1906).
#define NUM_AI_STATES 13
extern char *ai_state_names[NUM_AI_STATES];           // 0x59ad60 (name ours)
extern char *ai_substate_names[65];                   // 0x59ad94 (name ours)
extern char *ai_goal_type_names[18];                  // 0x59ae98 "unused", "attack", "follow", ... (name ours)
// The default combat style (ai_combat_style_names index) per species, read by ai_reset. Type name ours.
struct ai_species_style {
    int species;
    int style;
};
extern ai_species_style ai_species_styles[14];        // 0x59aee0 (name ours)
extern int num_ai_species_styles;                     // 0x59af50 14 (name ours)
extern float slowdown_scaler;                         // 0x59af54 4.0, ai_frame scales Character +0x6b0 by it
extern bool Ai_path_wandering_enabled;                // 0x59af58 true ("ai_wander"; ai_follow_path)
extern bool ai_path_flag_59af59;                      // 0x59af59 true, read by ai_follow_path only (name ours)
extern int ai_frame_update_rate;                      // 0x59af5c 8, 1..30 ("ai_frame_update_rate"; no reader)
extern int Ai_attack_point;                           // 0x59af60 -1 ("attack_point"; no reader; name ours)
extern int Ai_offense_point;                          // 0x59af64 -1 ("offense_point"; no reader; name ours)

extern vector3 ai_directions[16];                     // 0x22e47f0 {cos, 0, sin} in 22.5 degree steps (name ours)
extern unsigned int ai_move_flags;                    // 0x22e48cc passed to character_ai_move_frame
extern vector3 ai_move_dir;                           // 0x22e4940 first path step (ai_path_begin)
extern bool ai_do_frame_active;                       // 0x22e499d
// The AI profiler (0x22e48e0, 0x50 bytes): one static initialiser constructs both timers, so it is one
// object (D1900). The averages are recomputed every 500 ms by ai_do_frame from the accumulators.
// sum.symbols names the accumulators and the second timer; the macros keep those names.
struct ai_prof_counters {
    int path_us;                      // the total: ai_do_frame's time and ai_path_to's outside it
    int do_frame_us;
    int frame_us;
    int frame_b_us;
    int frame_c_us;
    int think_a_us;
    int think_b_us;
    int think_c_us;
    int think_d_us;
};
struct ai_profile {
    RealtimeTimer avg_timer;          // +0x00 (0x22e48e0) never used
    ai_prof_counters avg;             // +0x04 averages (microseconds per 1000 ms of play)
    RealtimeTimer timer;              // +0x28 (0x22e4908) ai_prof_timer, 500 ms
    ai_prof_counters acc;             // +0x2C (0x22e490c) accumulators
};
LAYOUT_CHECK(ai_profile, sizeof(ai_profile) == 0x50);
extern ai_profile ai_prof;                            // 0x22e48e0 (name ours)
#define ai_prof_timer       (ai_prof.timer)
#define ai_prof_path_us     (ai_prof.acc.path_us)
#define ai_prof_do_frame_us (ai_prof.acc.do_frame_us)
#define ai_prof_frame_us    (ai_prof.acc.frame_us)
#define ai_prof_frame_b_us  (ai_prof.acc.frame_b_us)
#define ai_prof_frame_c_us  (ai_prof.acc.frame_c_us)
#define ai_prof_think_a_us  (ai_prof.acc.think_a_us)
#define ai_prof_think_b_us  (ai_prof.acc.think_b_us)
#define ai_prof_think_c_us  (ai_prof.acc.think_c_us)
#define ai_prof_think_d_us  (ai_prof.acc.think_d_us)

// Tracked cast effects (5 x 0xc at 0x22e4950; the three functions after ai_can_be_knocked_down, D1905).
struct cast_effect_track {
    int spell;                        // +0x0 -1 free
    int handle;                       // +0x4 effect object
    float time;                       // +0x8 seconds left (counted down by ai_do_frame)
};
LAYOUT_CHECK(cast_effect_track, sizeof(cast_effect_track) == 0xc);
#define MAX_CAST_EFFECT_TRACKS 5
extern cast_effect_track cast_effect_tracks[MAX_CAST_EFFECT_TRACKS];   // 0x22e4950
int  cast_effect_track_count();                       // 0x475160

// decomp56: the AI combat unit (0x4bce60-0x4c0245, game/unnamed_ai_combat_0x4bce60.cpp, file not known).
// (records, count, heal wanted, cure wanted): per record flag 1 hurt below half (score 50 - 100 * HP
// fraction), flag 2 a curable status (+10); returns the sum of the scores.
float ai_score_support_targets(boss_target *list, int count, int heal, int cure);   // 0x4be940
extern vector3 ai_combat_aim_point;                   // 0x2483558 the target's attack point or position (name ours)
// The AI movement unit (0x4b1410-0x4b513f, game/unnamed_ai_move_0x4b1410.cpp; decomp68). Names ours.
extern Character *ai_follow_target;                   // 0x247e878 set before ai_follow_path by the combat code (never read)
// decomp68 (D4805): 0x247e84c is ai_mode_frame_counts[AI_STATE_COMBAT] (the array 0x247e840 has 13 ints);
// the name stays as a macro for the AI combat unit.
#define ai_combat_frames_watched (ai_mode_frame_counts[AI_STATE_COMBAT])

void ai_frame(Character *c);                          // 0x472a90 cdecl (sum.symbols: thiscall; D1901)
bool ai_update_perception(Character *c);              // 0x472e80 cdecl: true = skip this frame's AI
void cmd_ai_frame_update_rate();                      // 0x4720e0
void cmd_slowdown_scaler();                           // 0x472190
void cmd_attack_point();                              // 0x473260
void cmd_offense_point();                             // 0x473300
void cmd_ai_state();                                  // 0x474d10
void cmd_pause_ai();                                  // 0x474e10
void cmd_ai_wander();                                 // 0x474f00

// The AI mode units (0x4b4100-..): per-mode frame counters zeroed by ai_do_frame (13 ints at 0x247e840;
// ai_mode_traveling_frame increments [2], ai_mode_combat_frame [3], ai_mode_cast_spell_frame [4], each
// only for the object under the mouse). Name ours; defined in unnamed_ai_move_0x4b1410.cpp (decomp68).
extern int ai_mode_frame_counts[NUM_AI_STATES];       // 0x247e840
void ai_groups_frame();                               // 0x492d30 (unit not known)

// A service request (0xc bytes on the caller's stack; hud_key_pause sets only the type). Type name ours.
struct ai_service_req {
    int type;                         // 0 pause, 1 unpause
    int _unk04;
    int _unk08;
};
// decomp58: only a multiplayer client sends it (SRV_REQ to the host); everywhere else it does nothing.
void ai_service_request(ai_service_req *req);         // 0x4766f0

// ---- the goal unit (decomp58, game/unnamed_goals_doors_0x475180.cpp) --------------------------------
// ai_goal_evaluate's verdict (names ours): keep, drop (no target), drop (done / dead), suspend.
#define AI_GOAL_KEEP     0
#define AI_GOAL_LOST     1
#define AI_GOAL_DONE     2
#define AI_GOAL_SUSPEND  3
// The fields a goal type uses (ai_goal_type_fields; bits ours, D3803): goal_find compares and
// goal_add fills only these (with 0x1 type, 0x2 flags and 0x40 "not the running goal" from the mask).
#define AI_GF_TYPE    0x01
#define AI_GF_FLAGS   0x02
#define AI_GF_TARGET  0x04
#define AI_GF_POS     0x08            // pos and layer
#define AI_GF_SPELL   0x10
#define AI_GF_ARG     0x20
#define AI_GF_NOT_CUR 0x40
int  ai_goal_evaluate(ai_state *ai, ai_goal *goal);   // 0x475830 cdecl, AI_GOAL_*
int  ai_goal_compare(const void *a, const void *b);   // 0x475d20 qsort comparator
void ai_goals_sort(ai_goal *goals);                   // 0x475db0 qsort of the five slots
unsigned int ai_goal_type_fields(int type);           // 0x475fb0 AI_GF_* of the type
void ai_goals_validate(ai_state *ai);                 // 0x476610 cdecl
void ai_global_init();                                // 0x474900 pathfinding_init, direction table (GameInit)

// The query record of ai_score_target (boss_target_list_build fills it). Type and field names ours;
// ai_score_target lowers `best` (compared and replaced at 0x4b5761) and writes `result`. decomp68 (D4804):
// 0x18 bytes: ai_score_target also writes +0x14 (the chosen candidate's attack rating, read back to scale
// the next score); +0x08 is the attacker limit (ai_count_engaged_attackers must stay below it); `result`
// is the chosen Character (was int). Unit game/unnamed_ai_think_0x4b5140.cpp.
struct ai_target_query {
    Character *self;                  // +0x00
    Character *candidate;             // +0x04
    union {
        int _unk08;                   // +0x08 10 at boss_target_list_build, 2 at the tiger
        int max_attackers;
    };
    Character *result;                // +0x0C NULL until a candidate passes
    float best;                       // +0x10 starts at the char_info fov range (+0x220)
    float rating;                     // +0x14 combat_attack_rating of `result`
};
LAYOUT_CHECK(ai_target_query, sizeof(ai_target_query) == 0x18);
// (query, boss list): a boss scoring its own list skips the boss test of a type-9 self.
void ai_score_target(ai_target_query *q, bool boss_list);   // 0x4b53c0 (boss_target_list_build passes 1; return unused)

// The avoidance records and calls (avoid_area_desc, avoid_obstacle, avoid_poly, avoid_seglist, the splines,
// avoid_*) moved to engine/ai/unnamed_avoidance_0x4a6710.h (decomp57).
#include "engine/ai/unnamed_avoidance_0x4a6710.h"
// Console toggle "pause_ai" (unit not known; decomp34).

void ai_groups_restart_timers();                      // 0x492b60 (state_game_enter; decomp42)

// decomp68: the AI movement and think units (placeholder headers).
#include "engine/ai/unnamed_ai_move_0x4b1410.h"
#include "engine/ai/unnamed_ai_think_0x4b5140.h"

// ---- the AI group unit (0x492b00-0x493e41, file not known; game/unnamed_ai_groups_0x492b00.cpp, decomp54) ----
// ai_group::flags bits (names ours).
#define AI_GROUP_PARTY       0x01     // the leader was a party member (type 7) at ai_group_create
#define AI_GROUP_FIGHTING    0x04     // cleared by ai_group_combat_think when no enemy is seen; 1 s think rate
#define AI_GROUP_COMBAT_SET  0x08     // combat_mode_set(1) done this think (cleared by ai_groups_frame)
#define AI_GROUP_LOCAL       0x10     // multiplayer: this machine thinks for the group
// sum.symbols' names, but the contents are the other way round (D3400): ai_think_members holds the
// group's own members (ai_think_num_members of them), ai_think_enemies the enemies they see (the
// group's num_enemies of them). Names kept.
extern Character *ai_think_enemies[10];               // 0x243e394
extern int ai_think_num_members;                      // 0x243e3bc
extern Character *ai_think_members[6];                // 0x243e3c0 (sum.symbols [5]; 0x18 bytes to 0x243e3d8, D3400)
extern bool Party_controller_check;                   // 0x59c594 (init 1; no reader)
void ai_group_combat_think(ai_group *g);              // 0x492db0
int  ai_group_count_helpless(ai_group *g);            // 0x493000
int  character_is_helpless(Character *c);             // 0x493040 asleep, paralysed or in an ice coffin
bool ai_member_skips_group_orders(Character *c);      // 0x493080
void ai_group_think_style0(Character *c, ai_group *g);   // 0x4930d0
void ai_group_order_attack(Character *c, Character *target, ai_group *g, bool mark);   // 0x493220
void ai_group_think_style1(Character *c, ai_group *g);   // 0x493290
void ai_group_think_style2(Character *c, ai_group *g);   // 0x493570
void ai_group_think_caster(Character *c, ai_group *g, int first_pass);   // 0x4936c0
void ai_group_collect_enemies(ai_group *g);           // 0x493ac0
void ai_group_shuffle_enemies(ai_group *g);           // 0x493ce0
void cmd_party_controller_check();                    // 0x493d90

#endif
