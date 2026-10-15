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

// An AI group (10 x 0x54 bytes at 0x243e040; Character +0x4cc points at one). Saved in the LENT chunk.
struct ai_group {
    int count;               // +0x00
    int members[6];          // +0x04 handles
    ubyte _unk1c[0x2c];
    int leader;              // +0x48 leader's object handle
    unsigned int flags;      // +0x4C
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
    void goal_remove(int slot);       // 0x476300 ai_goal_remove
    bool has_goal_without_flag20();   // 0x476670 ai_has_goal_without_flag20 (decomp41: script_character_has_pending_goal)

    ~ai_state();                      // 0x4f13e0 (folded)
    bool is_traveling_sub1();         // 0x4730e0 ai_is_traveling_sub1
    void formation_slot_position(vector3 *out, const vector3 *base, const vector3 *dir, int slot);   // 0x4b52e0 (unit not known)
    void reset();                     // 0x472450 ai_reset
    void process_goals();             // 0x4763a0 ai_process_goals (combat_mode_set; decomp42)
    bool is_combat_e_or_sneak_attack(); // 0x473120 ai_is_combat_e_or_sneak_attack
    bool is_mode(int mode, int sub);  // 0x473200 ai_is_mode (sub -1 = any)
    int  get_mode();                  // 0x473600 ai_get_mode
    void set_18(int style);           // 0x473610 ai_set_18 (combat style, +0x18)
    int  get_combat_style();          // 0x473620 ai_get_combat_style
    bool is_casting();                // 0x4731d0 ai_is_casting
    void go_idle();                   // 0x4738c0 ai_go_idle
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
    int  goal_find(int mask, int type, int a, int b, int c, int d, int e, int f);  // 0x475eb0 ai_goal_find
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
    void summon_emerge_update();      // 0x4b4660 summon_emerge_update (unit not known)
    void retaliate_on_hit(int attacker);  // 0x4b5790 ai_retaliate_on_hit (unit not known)
    // decomp29 (boss_ai.cpp). (pos, layer, 30.0f, 0, 5000) at boss_release_mount; return unused.
    // decomp39 (D1903): f is goal_add's float argument, walk a byte (`cmp dl, 1`), the last the priority.
    int  add_move_to_timed(vector3 *pos, int layer, float f, bool walk, int ms);   // 0x474190 ai_goal_add_move_to_timed

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
    void tiger_seat_rider();          // 0x49aeb0
    void giant_salamanka_check_stage(); // 0x4c8560
    void serpent_check_stage();       // 0x495b90
    void serpent_seat_rider();        // 0x495e10
    void serpent_part_check_timeout();// 0x497610
    void phoenix_check_stage();       // 0x499690
    void phoenix_seat_rider();        // 0x4982a0
    void ghost_update_stage();        // 0x4bb4c0
    void ghost_seat_rider();          // 0x4ba8e0
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

    int mode;               // +0x00 enum ai_state_mode
    int submode;             // +0x04
    int prev_mode;           // +0x08
    int prev_submode;        // +0x0C
    int pending_mode;        // +0x10
    int pending_submode;     // +0x14
    int combat_style;        // +0x18 ai_get_combat_style / ai_set_18
    GameTimer timer;         // +0x1C restarted by ai_set_mode
    int _unk20;
    ubyte _unk24[0x0c];
    GameTimer timer30;       // +0x30
    float attack_range;      // +0x34 reach of the current boss move (boss_get_move_range; was _unk34, D900)
    int priority;           // +0x38 999999 for direct orders
    GameTimer timer3c;       // +0x3C
    GameTimer timer40;       // +0x40
    GameTimer timer44;       // +0x44
    int _unk48;
    Character *owner;        // +0x4C
    unsigned int flags;      // +0x50 AI_FLAG_*; 0x10000000 path started (ai_path_begin)
    vector3 travel_dest;     // +0x54 (was float[3]; decomp39)
    int target;              // +0x60 target handle
    int prev_target;         // +0x64
    ubyte _unk68[0x14];
    int _unk7c;              // +0x7C attack point index at the target (its attack_point_users), -1 none
    int handle_slot;         // +0x80 released by ai_set_mode (handle_slot_release)
    int mode_target;         // +0x84 -1 when a cast mode ends
    int _unk88;
    ai_goal goals[5];        // +0x8C goal slots (was ubyte[5][0x34]; decomp39)
    int cur_goal;            // +0x190
    int emerge_move;         // +0x194
    int _unk198;
    GameTimer timer19c;      // +0x19C
    GameTimer disable_timer; // +0x1A0
    int disable_ms;          // +0x1A4
    ai_group *group;         // +0x1A8 (Character +0x4cc; types.h _unk1a8, zeroed by ai_state_init)
    int _unk1ac;             // +0x1AC
    struct {                 // +0x1B0 attacker slots
        int handle;          //   -1 free
        GameTimer timer;
    } attackers[5];
    int attack_point_users[8]; // +0x1D8 per char_info attack point, -1 free (was _unk1d8; decomp39)
    int path_started;        // +0x1F8
    int _unk1fc;
    int path_step;           // +0x200
    int _unk204;             // +0x204
    GameTimer perception_timer; // +0x208
    int _unk20c;
    ai_path *path;           // +0x210 (was void *; decomp39)
    vector3 path_start;      // +0x214 the from point of the search (was path_dest float[3]; D1904)
    int _unk220;             // +0x220
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
// ai_mode_traveling_frame increments [2], ai_mode_combat_frame [3], ai_mode_cast_spell_frame [4]). Name ours.
extern int ai_mode_frame_counts[NUM_AI_STATES];       // 0x247e840
void ai_groups_frame();                               // 0x492d30 (unit not known)

// A service request (0xc bytes on the caller's stack; hud_key_pause sets only the type). Type name ours.
struct ai_service_req {
    int type;                         // 0 pause, 1 unpause
    int _unk04;
    int _unk08;
};
void ai_service_request(ai_service_req *req);         // 0x4766f0 (handled locally or sent with multi_send_packet)
void ai_global_init();                                // 0x474900 pathfinding_init, direction table (GameInit)

// The query record of ai_score_target (0x14 bytes on the caller's stack; boss_target_list_build fills
// it). Type and field names ours; ai_score_target lowers `best` (compared and replaced at 0x4b5761) and
// writes `result`; +0x08 is 10 at boss_target_list_build. Unit not known.
struct ai_target_query {
    Character *self;                  // +0x00
    Character *candidate;             // +0x04
    int _unk08;                       // +0x08 10 at boss_target_list_build
    int result;                       // +0x0C
    float best;                       // +0x10 starts at the char_info fov range (+0x220)
};
LAYOUT_CHECK(ai_target_query, sizeof(ai_target_query) == 0x14);
void ai_score_target(ai_target_query *q, int a);      // 0x4b53c0 (boss_target_list_build passes 1; return unused)

// An avoidance-area primitive ("AA"; 0x30 bytes on the caller's stack, avoid_area_add copies it).
// Layout from the three spells.cpp builders (decomp23); names ours, unit not known.
struct avoid_area_desc {
    int _unk00;                       // +0x00 0 at every caller
    int shape;                        // +0x04 0 (box), 1 (spell_spawn_hit_fx)
    float cx, cz;                     // +0x08 centre (x, z)
    float ax, az;                     // +0x10 first axis
    float bx, bz;                     // +0x18 second axis
    float extent_a, extent_b;         // +0x20 half sizes along the axes
    float _unk28;                     // +0x28 0.5 at every caller
    int owner;                        // +0x2C caller's argument (handle or team)
};
LAYOUT_CHECK(avoid_area_desc, sizeof(avoid_area_desc) == 0x30);
int avoid_area_add(avoid_area_desc *d);               // 0x4a9b40 index ("AA - primative not added!!")

// Avoidance splines (unit not known, 0x4a7800-0x4a7a27; decomp34, D1402). The description avoid_spline_create
// reads: a count and per point the two cell bytes and two 2D end points (avoid_spline_point_create(&p0[i],
// &p1[i], cell_x[i], cell_y[i]) at 0x4a79c1-0x4a79cd). Type and field names ours.
struct avoid_spline_def {
    int count;                        // +0x00
    int cell_x[8];                    // +0x04
    int cell_y[8];                    // +0x24
    vector2 p0[8];                    // +0x44
    vector2 p1[8];                    // +0x84
};
LAYOUT_CHECK(avoid_spline_def, sizeof(avoid_spline_def) == 0xc4);
int  avoid_spline_create(avoid_spline_def *def);      // 0x4a7990 spline index (< 20) or -1
void avoid_spline_free(int index);                    // 0x4a78b0
// Console toggle "pause_ai" (unit not known; decomp34).

// ---- Obstacle avoidance tables (0x4a6710-0x4aa760, file not known; decomp32, pathfinding.cpp) ----
// Only the fields pathfinding.cpp reads; names ours. A segment-list corner starts like a pf_vis_node
// (point, flags, links, A* parent at +0x1c), so pf_graph_search links the two kinds.
struct avoid_corner {                 // 0x44, segment list +0x288
    pf_vis_node node;                 // +0x00
    ubyte _unk28[0x1c];
};
LAYOUT_CHECK(avoid_corner, sizeof(avoid_corner) == 0x44);
#define AVOID_SEGLIST_USED 0x1
#define AVOID_SEGLIST_DOOR 0x2        // blocks instead of adding search nodes (pf_add_avoidance_nodes)
#define AVOID_SEGLIST_OFF  0x8        // skipped by pf_mark_nodes_visible_from_point
struct avoid_seglist {                // 0xa78, 14 at 0x2460db0 (sum.symbols avoid_seglists)
    int _unk00;
    uint flags;                       // +0x04 AVOID_SEGLIST_*
    ubyte _unk08[6];
    short num_corners;                // +0x0e
    int size;                         // +0x10
    int layer;                        // +0x14
    ubyte _unk18[0x270];
    avoid_corner corners[29];         // +0x288
    ubyte _unka3c[0x3c];
};
LAYOUT_CHECK(avoid_seglist, sizeof(avoid_seglist) == 0xa78);
#define AVOID_NUM_SEGLISTS 14
extern avoid_seglist avoid_seglists[AVOID_NUM_SEGLISTS];   // 0x2460db0
// A box / circle primitive (avoid_prim_from_segment builds one on the caller's stack: 0x78 bytes fit in
// pf_add_avoidance_nodes' frame).
struct avoid_prim {
    ubyte _unk00[0x78];
};
struct avoid_obstacle {               // 0x88, 14 at 0x246a080 (sum.symbols avoid_obstacles)
    ushort flags;                     // +0x00 bit 0 used, bit 1 on the obstacle grid
    short num_points;                 // +0x02
    short size;                       // +0x04
    short layer;                      // +0x06
    ubyte _unk08[0x30];
    vector2 points[10];               // +0x38 closed outline
};
LAYOUT_CHECK(avoid_obstacle, sizeof(avoid_obstacle) == 0x88);
#define AVOID_NUM_OBSTACLES 14
extern avoid_obstacle avoid_obstacles[AVOID_NUM_OBSTACLES];   // 0x246a080
extern char avoid_obstacle_counts[PF_NUM_SIZES * PF_MAX_LAYERS];   // 0x246a040 per size * 20 + layer
// An avoidance polygon (0x114, table 0x246a7f0; avoid_poly_get).
struct avoid_poly {
    short _unk00;
    short count;                      // +0x02 obstacles
    ubyte _unk04[4];
    char obstacles[0xfc];             // +0x08 avoid_obstacles indices
    rect2d bounds;                    // +0x104
};
LAYOUT_CHECK(avoid_poly, sizeof(avoid_poly) == 0x114);
// An avoidance spline (0xc, 20 at 0x2460cc0) and its points (0x24, 100 at 0x246b998). A point carries a
// wall record at +4 that pf_sweep_segment tests like a pf_edge.
struct avoid_spline {
    short flags;                      // +0x00
    short count;                      // +0x02
    char points[8];                   // +0x04 avoid_spline_point_get indices
};
LAYOUT_CHECK(avoid_spline, sizeof(avoid_spline) == 0xc);
struct avoid_spline_point {
    ubyte flags;                      // +0x00
    ubyte _unk01;
    char size;                        // +0x02
    char layer;                       // +0x03
    pf_edge wall;                     // +0x04
    ubyte _unk14[0x10];
};
LAYOUT_CHECK(avoid_spline_point, sizeof(avoid_spline_point) == 0x24);
extern char avoid_spline_point_counts[PF_NUM_SIZES * PF_MAX_LAYERS];   // 0x246b8dc per size * 20 + layer
void avoid_reset();                                    // 0x4a9d60
void avoid_seglists_clear_refs();                      // 0x4a7b90
avoid_poly *avoid_poly_get(avoid_seglist *s);          // 0x4aa3e0
bool rect2d_overlap(rect2d a, rect2d b);               // 0x4a7160 (eight floats on the stack)
void avoid_prim_from_segment(avoid_prim *out, const vector2 *a, const vector2 *b);   // 0x4a6d60
bool avoid_prim_overlap(const avoid_obstacle *a, const avoid_prim *b, float margin);   // 0x4a6e00
// Returns the extra path cost of going round the obstacle (pf_add_avoidance_nodes adds it).
float avoid_poly_add_search_nodes(const avoid_prim *seg, const avoid_obstacle *ob);   // 0x4aa400
bool avoid_segment_hits_prim(const avoid_obstacle *ob, const vector2 *a, const vector2 *b, float margin);   // 0x4a6f50
avoid_spline *avoid_spline_get(int i);                 // 0x4a7830 NULL unless used
avoid_spline_point *avoid_spline_point_get(int i);     // 0x4a77d0 NULL unless used

void ai_groups_restart_timers();                      // 0x492b60 (state_game_enter; decomp42)

#endif
