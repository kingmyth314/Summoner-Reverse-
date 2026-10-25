// engine/ai/unnamed_tiger_0x499e50.h - the Tiger Rider boss (code 0x499e50-0x49c83a; source
// game/unnamed_tiger_0x499e50.cpp, decomp54). The unit's file name is not known (no assert path, no
// strings.tbl section); this header's name and directory are placeholders. Not original source. Names
// from sum.symbols; the ones marked "ours" are new.
// The fight state is one object (D3403): one static initialiser (init_tiger_timers, entry 283)
// constructs its GameTimers; its extent, 0x2452a90 up to the console object 0x2452c98, follows the
// serpent's pattern (stage first, the console objects outside). The sum.symbols names of its fields
// stay as macros.
#ifndef ENGINE_AI_UNNAMED_TIGER_0X499E50_H
#define ENGINE_AI_UNNAMED_TIGER_0X499E50_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/console.h"
#include "engine/ai/boss_ai.h"

class Character;
struct move_info;
struct item_info;

#define TIGER_STAGES     3
#define TIGER_NUM_MOVES  6            // tiger_move_table (variations 0xb6, 0xb8, 0xb9, 3, 2, 1)

// The fight state at 0x2452a90 (0x208 bytes). Type, object and field names ours unless a macro below
// carries the sum.symbols name.
struct tiger_boss {
    int stage;                                        // +0x000 0x2452a90
    int target_stage;                                 // +0x004 0x2452a94
    bool dismount_stage_reached;                      // +0x008 0x2452a98
    bool stage_trigger;                               // +0x009 0x2452a99
    bool combat;                                      // +0x00a 0x2452a9a
    bool has_rider;                                   // +0x00b 0x2452a9b
    int anim_remap_count;                             // +0x00c 0x2452a9c
    int remap_slot[2];                                // +0x010
    float remap_weight[2];                            // +0x018
    item_info *weapon;                                // +0x020 0x2452ab0 the TigerSpear
    item_info *axe;                                   // +0x024 0x2452ab4
    int spear_uses_left;                              // +0x028 0x2452ab8
    bool weapon_ready;                                // +0x02c 0x2452abc the stage move put the weapon in hand
    ubyte _pad02d[3];
    boss_threat threat[BOSS_MAX_THREATS];             // +0x030 0x2452ac0
    vector3 wander_point;                             // +0x0a8 0x2452b38
    ubyte _unk0b4[0xc];                               // +0x0b4
    int wander_state;                                 // +0x0c0 0x2452b50 sub-mode 0x21 steps (-1 .. 6)
    int state_b54;                                    // +0x0c4 0x2452b54 -1 from tiger_reset_b54, no reader
    int reposition_state;                             // +0x0c8 0x2452b58 sub-mode 0x20 steps (-1 .. 2)
    GameTimer reposition_timer;                       // +0x0cc 0x2452b5c
    int wander_failures;                              // +0x0d0 0x2452b60
    int reposition_failures;                          // +0x0d4 0x2452b64
    int charge_state;                                 // +0x0d8 0x2452b68 -1, 2, 4
    float charge_trigger_time;                        // +0x0dc 0x2452b6c trigger 7 of move 0xb8 (0.4 default)
    bool charging;                                    // +0x0e0 0x2452b70
    bool charge_fired;                                // +0x0e1 0x2452b71
    bool move_trigger_pending;                        // +0x0e2 0x2452b72
    ubyte _pad0e3;
    move_info *cached_move;                           // +0x0e4 0x2452b74 move 0xb8
    move_info *move_b78;                              // +0x0e8 0x2452b78 charinfo_find_move(0xb, 0x20), no reader
    int pending_submode;                              // +0x0ec 0x2452b7c 0x21 or 0xe (no writer in this unit)
    boss_attack moves[TIGER_NUM_MOVES];               // +0x0f0 0x2452b80
    boss_attack *cur_move;                            // +0x1b0 0x2452c40
    GameTimer target_timer;                           // +0x1b4 0x2452c44
    int target;                                       // +0x1b8 0x2452c48
    move_info *flinch_status[TIGER_STAGES];           // +0x1bc 0x2452c4c
    move_info *flinch_area[TIGER_STAGES];             // +0x1c8 0x2452c58
    int anim_ride_stand;                              // +0x1d4 0x2452c64
    int anim_ride_ready;                              // +0x1d8
    int anim_ride_walk;                               // +0x1dc
    int anim_stage1_walk;                             // +0x1e0
    int anim_ride_run;                                // +0x1e4
    int anim_stage1_run;                              // +0x1e8
    int anim_ride_ready_axe;                          // +0x1ec 0x2452c7c loaded, never applied
    int anim_stage2_walk;                             // +0x1f0
    int anim_stage2_run;                              // +0x1f4
    int anim_ready;                                   // +0x1f8
    int anim_ready_hurt;                              // +0x1fc
    int anim_walk;                                    // +0x200
    int anim_run;                                     // +0x204
};
LAYOUT_CHECK_32(tiger_boss, sizeof(tiger_boss) == 0x208);
extern tiger_boss tiger;                              // 0x2452a90 (name ours)

#define tiger_stage                    (tiger.stage)
#define tiger_target_stage             (tiger.target_stage)
#define tiger_dismount_stage_reached   (tiger.dismount_stage_reached)
#define tiger_stage_trigger            (tiger.stage_trigger)
#define tiger_combat                   (tiger.combat)
#define tiger_has_rider                (tiger.has_rider)
#define tiger_anim_remap_count         (tiger.anim_remap_count)
#define tiger_weapon                   (tiger.weapon)
#define tiger_axe                      (tiger.axe)
#define tiger_spear_uses_left          (tiger.spear_uses_left)
#define tiger_threat_table             (tiger.threat)
#define tiger_wander_point             (tiger.wander_point)
#define tiger_reposition_timer         (tiger.reposition_timer)
#define tiger_move_trigger_pending     (tiger.move_trigger_pending)
#define tiger_cached_move              (tiger.cached_move)
#define tiger_move_table               (tiger.moves)
#define tiger_cur_move                 (tiger.cur_move)
#define tiger_target                   (tiger.target)
#define tiger_anim_ride_stand          (tiger.anim_ride_stand)
#define tiger_anim_ride_ready          (tiger.anim_ride_ready)
#define tiger_anim_ride_walk           (tiger.anim_ride_walk)
#define tiger_anim_stage1_walk         (tiger.anim_stage1_walk)
#define tiger_anim_ride_run            (tiger.anim_ride_run)
#define tiger_anim_stage1_run          (tiger.anim_stage1_run)
#define tiger_anim_stage2_walk         (tiger.anim_stage2_walk)
#define tiger_anim_stage2_run          (tiger.anim_stage2_run)
#define tiger_anim_ready               (tiger.anim_ready)
#define tiger_anim_ready_hurt          (tiger.anim_ready_hurt)
#define tiger_anim_walk                (tiger.anim_walk)
#define tiger_anim_run                 (tiger.anim_run)

extern Character *tiger_mount;                        // 0x2452ca4
extern int tiger_wander_misses;                       // 0x2452cb4 counted by tiger_wander_update, no reader (name ours)
extern console_command ccmd_tiger_solo;               // 0x2452a70
extern console_command ccmd_tiger_stage;              // 0x2452a80
extern console_command ccmd_tiger_combat;             // 0x2452c98
extern console_command ccmd_tiger_unlink;             // 0x2452ca8

// Declared in engine/ai/boss_ai.h or engine/ai/ai.h (not repeated): tiger_find_rider, tiger_set_ride_anims,
// tiger_unload, tiger_anim_remap_in / _out, tiger_set_foot_anims, boss_dismount, tiger_move_done_hook,
// tiger_move_in_range, tiger_get_attack_range, tiger_restore_ride_anims, tiger_move_trigger,
// tiger_set_solo; ai_state::tiger_boss_load, tiger_check_stage, tiger_seat_rider, tiger_ai_idle,
// tiger_ai_frame and the thiscall ones of this unit.
void  cmd_tiger_stage();                              // 0x499ec0
void  cmd_tiger_unlink();                             // 0x499f30
void  cmd_tiger_combat();                             // 0x499ff0
void  cmd_tiger_solo();                               // 0x49a080
int   tiger_current_move_index();                     // 0x49a0a0 index into tiger_move_table or -1
void  tiger_set_ride_stand_anims(Character *c);       // 0x49a0c0
void  tiger_set_stage1_anims(Character *c);           // 0x49a100
void  tiger_set_stage2_anims(Character *c);           // 0x49a140
void  tiger_rider_set_flinch(move_info *moves);       // 0x49a1a0
void  tiger_equip_weapon(Character *c, bool on_foot); // 0x49a2c0 the spear (four tags) and the axe
void  tiger_swap_weapon(Character *c);                // 0x49a420 the axe (stage 2)
void  tiger_setup_anims(Character *c);                // 0x49a4b0 on foot (stage 3)
// (self, target, tries, min radius, max radius, min dot, max dot, min distance from self) -> point in
// tiger_wander_point.
bool  tiger_pick_wander_point(Character *self, Character *target, int tries, float rmin, float rmax,
                              float dot_min, float dot_max, float min_dist);   // 0x49a830
void  tiger_init_move_table();                        // 0x49aad0
void  tiger_load_anims(Character *c);                 // 0x49ab10
void  tiger_reset_b58();                              // 0x49ac40
void  tiger_reset_b50();                              // 0x49ac60
void  tiger_reset_b54();                              // 0x49ac80
void  tiger_reset_move_state();                       // 0x49ac90
void  tiger_cache_move_b8(Character *c);              // 0x49acc0
int   tiger_threat_find(int handle);                  // 0x49b080 index or -1
int   tiger_pick_target(Character *boss);             // 0x49b0a0 handle or -1
struct ai_path *tiger_path_to_target(Character *self, Character *t);   // 0x49b520 NULL when within 3 m

#endif
