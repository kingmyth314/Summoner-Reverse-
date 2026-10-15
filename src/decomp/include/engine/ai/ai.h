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

    int mode;                // +0x00 enum ai_state_mode
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
    int _unk34;
    int priority;            // +0x38 999999 for direct orders
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

void ai_group_create(Character *c);                   // 0x492ba0 (return value unused; type guessed)
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
