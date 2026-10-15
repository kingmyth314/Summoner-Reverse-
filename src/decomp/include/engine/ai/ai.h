// engine/ai/ai.h - the AI sub-object of a character and AI groups (ai.cpp in sum.symbols, 0x471f40-0x4742b0
// by string ids; directory from the engine\ai\pathfinding.cpp path). Not original source.
// Layout: symbols/types.h ai_state / ai_group, with the GameTimer fields typed as GameTimer and +0x1a8
// named `group` (player.cpp reads the character's group there).
#ifndef ENGINE_AI_AI_H
#define ENGINE_AI_AI_H

#include "vsdk/vsdk.h"
#include "vsdk/os/timestamp.h"

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

// The AI sub-object at Character +0x324 (ai_state__ctor 0x4722f0). thiscall members on this+0x324.
class ai_state {
public:
    void reset();                     // 0x472450 ai_reset
    bool is_combat_e_or_sneak_attack(); // 0x473120 ai_is_combat_e_or_sneak_attack
    bool is_mode(int mode, int sub);  // 0x473200 ai_is_mode (sub -1 = any)
    int  get_mode();                  // 0x473600 ai_get_mode
    void set_18(int style);           // 0x473610 ai_set_18 (combat style, +0x18)
    int  get_combat_style();          // 0x473620 ai_get_combat_style
    bool is_casting();                // 0x4731d0 ai_is_casting
    void go_idle();                   // 0x4738c0 ai_go_idle
    bool is_disabled_any();           // 0x475000 ai_is_disabled_any
    void goals_remove_type1();        // 0x475e00 ai_goals_remove_type1
    // decomp20 (conversationctl.cpp):
    bool is_traveling_sub0();         // 0x4730c0 ai_is_traveling_sub0
    // (mask, type, ...): goal slot index or -1. The conversation calls pass (1, type, 0, 0, -1, 0, 0, 0x6c);
    // the meaning of the last six is not known.
    int  goal_find(int mask, int type, int a, int b, int c, int d, int e, int f);  // 0x475eb0 ai_goal_find
    void goals_clear();               // 0x476380 ai_goals_clear
    void add_cast_spell(int spell, int priority);   // 0x4740c0 ai_goal_add_cast_spell (return unused)
    bool is_attacking();              // 0x473150 ai_is_attacking
    void add_cast_at_target(int spell, Character *target, int priority);   // 0x4740e0 ai_goal_add_cast_at_target
    void add_cast_at_point(int spell, vector3 *pos, int layer, int priority); // 0x474130 ai_goal_add_cast_at_point
    void add_skill_from_behind(Character *target, int priority);           // 0x4741d0 ai_goal_add_skill_from_behind
    void add_hide();                  // 0x474440 ai_goal_add_hide (toggles)
    void add_sneak();                 // 0x474480 ai_goal_add_sneak
    // decomp23 (combat.cpp, spells.cpp):
    bool is_combat_sub10();           // 0x473170 ai_is_combat_sub10 (ranged attack)
    bool is_combat_sub11();           // 0x4731e0 ai_is_combat_sub11 (special attack)
    void class_die();                 // 0x4739e0 ai_class_die
    void knock_down(int ms);          // 0x474350 ai_knock_down
    void disable_2f();                // 0x4743e0 ai_disable_2f
    void retaliate_on_hit(int attacker);  // 0x4b5790 ai_retaliate_on_hit (unit not known)
    // decomp29 (boss_ai.cpp). (pos, layer, 30.0f, 0, 5000) at boss_release_mount; return unused.
    void add_move_to_timed(vector3 *pos, int layer, float f, int walk, int ms);   // 0x474190 ai_goal_add_move_to_timed

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
    float travel_dest[3];    // +0x54
    int target;              // +0x60 target handle
    int prev_target;         // +0x64
    ubyte _unk68[0x14];
    int _unk7c;              // +0x7C
    int handle_slot;         // +0x80
    int mode_target;         // +0x84
    int _unk88;
    ubyte goals[5][0x34];    // +0x8C goal slots
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
    int _unk1d8[8];          // +0x1D8
    int path_started;        // +0x1F8
    int _unk1fc;
    int path_step;           // +0x200
    int _unk204;             // +0x204
    GameTimer perception_timer; // +0x208
    int _unk20c;
    void *path;              // +0x210
    float path_dest[3];      // +0x214
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

extern char **ai_combat_style_names[6];               // 0x22e49a0 localize_strings slots (init_strptrs_22e49a0)

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

#endif
