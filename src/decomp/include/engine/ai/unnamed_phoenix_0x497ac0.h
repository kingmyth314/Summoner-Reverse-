// engine/ai/unnamed_phoenix_0x497ac0.h - the Phoenix Rider boss (code 0x497ac0-0x499e41; source
// game/unnamed_phoenix_0x497ac0.cpp, decomp54). The unit's file name is not known (no assert path, no
// strings.tbl section); this header's name and directory are placeholders. Not original source. Names
// from sum.symbols; the ones marked "ours" are new.
// The fight state is one object (D3403): one static initialiser (init_phoenix_timers, entry 282)
// constructs its GameTimers. Its extent, 0x2452420 up to the first console object 0x24529f8, is taken
// from the serpent's (the first 0x500 bytes have the same layout); nothing clears it in one go. The
// sum.symbols names of its fields stay as macros.
#ifndef ENGINE_AI_UNNAMED_PHOENIX_0X497AC0_H
#define ENGINE_AI_UNNAMED_PHOENIX_0X497AC0_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/console.h"
#include "engine/ai/boss_ai.h"
// serpent_offense_point: engine/ai/boss_ai.h

class Character;
struct move_info;

#define PHOENIX_STAGES 3

// phoenix_flags (names ours).
#define PHOENIX_F_REPOSITION   0x01   // the next choice is a reposition (pending sub-mode 0x20)
#define PHOENIX_F_DIVE         0x02   // the dive attack is next
#define PHOENIX_F_REPOS_PATH   0x04   // the reposition found a path to the dive point
#define PHOENIX_F_REBIRTH      0x08   // the rebirth move plays (HP reached 0 before stage 3)
#define PHOENIX_F_REBORN       0x10   // stage 3 after the rebirth
// boss_target flag of phoenix_evaluate_target.
#define PHOENIX_TF_FAR         0x01   // edge distance above 2.5

// The fight state at 0x2452420 (0x5d8 bytes). Type, object and field names ours unless a macro below
// carries the sum.symbols name.
struct phoenix_boss {
    int stage;                                        // +0x000 0x2452420
    int target_stage;                                 // +0x004 0x2452424
    bool dismount_stage_reached;                      // +0x008 0x2452428
    bool stage_trigger;                               // +0x009 0x2452429
    bool combat;                                      // +0x00a 0x245242a
    bool has_rider;                                   // +0x00b 0x245242b
    int _unk00c;
    int anim_remap_count;                             // +0x010 0x2452430
    int remap_slot[2];                                // +0x014
    float remap_weight[2];                            // +0x01c
    int num_offense_points;                           // +0x024 0x2452444
    serpent_offense_point offense_points[10];         // +0x028 0x2452448
    boss_attack attacks[PHOENIX_STAGES][BOSS_MAX_OFFENSE_POINTS];   // +0x140 0x2452560 phoenix_boss.tbl
    boss_attack *cur_attack;                          // +0x500 0x2452920
    int pending_submode;                              // +0x504 0x2452924 0x20, 0x3a, 0x3b
    vector3 dive_point;                               // +0x508 0x2452928
    unsigned int flags;                               // +0x514 0x2452934 PHOENIX_F_*
    boss_threat threat[BOSS_MAX_THREATS];             // +0x518 0x2452938
    int target_handle;                                // +0x590 0x24529b0
    int ranged_count;                                 // +0x594 0x24529b4 repeats of the stage 2/3 ranged move
    int _unk598;                                      // +0x598 0x24529b8 -1 from boss_load and the end moves, no reader
    int ground_effect;                                // +0x59c 0x24529bc
    GameTimer timer_c0;                               // +0x5a0 0x24529c0
    GameTimer timer_c4;                               // +0x5a4
    GameTimer wander_timer;                           // +0x5a8
    GameTimer timer_cc;                               // +0x5ac
    GameTimer timer_d0;                               // +0x5b0
    GameTimer timer_d4;                               // +0x5b4
    int saved_move;                                   // +0x5b8 0x24529d8
    move_info *flinch_status[PHOENIX_STAGES];         // +0x5bc 0x24529dc
    move_info *flinch_area[PHOENIX_STAGES];           // +0x5c8 0x24529e8
    int _unk5d4;                                      // +0x5d4
};
LAYOUT_CHECK_32(phoenix_boss, sizeof(phoenix_boss) == 0x5d8);
extern phoenix_boss phoenix;                          // 0x2452420 (name ours)

#define phoenix_stage                   (phoenix.stage)
#define phoenix_target_stage            (phoenix.target_stage)
#define phoenix_dismount_stage_reached  (phoenix.dismount_stage_reached)
#define phoenix_stage_trigger           (phoenix.stage_trigger)
#define phoenix_combat                  (phoenix.combat)
#define phoenix_has_rider               (phoenix.has_rider)
#define phoenix_anim_remap_count        (phoenix.anim_remap_count)
#define phoenix_num_offense_points      (phoenix.num_offense_points)
#define phoenix_cur_attack              (phoenix.cur_attack)
#define phoenix_dive_point              (phoenix.dive_point)
#define phoenix_flags                   (phoenix.flags)
#define phoenix_threat_table            (phoenix.threat)
#define phoenix_target_handle           (phoenix.target_handle)
#define phoenix_ground_effect           (phoenix.ground_effect)
#define phoenix_timer_c0                (phoenix.timer_c0)
#define phoenix_timer_c4                (phoenix.timer_c4)
#define phoenix_wander_timer            (phoenix.wander_timer)
#define phoenix_timer_cc                (phoenix.timer_cc)
#define phoenix_timer_d0                (phoenix.timer_d0)
#define phoenix_timer_d4                (phoenix.timer_d4)
#define phoenix_saved_move              (phoenix.saved_move)

// The attack records phoenix_boss_load names (attacks[stage - 1][index]); names ours.
extern boss_attack *phoenix_move_s1_0;                // 0x2452a10 stage 1 ranged
extern boss_attack *phoenix_move_s1_1;                // 0x245241c the reposition attack
extern boss_attack *phoenix_move_s1_2;                // 0x2452a1c the dive
extern boss_attack *phoenix_move_s2_0;                // 0x2452a14 stage 2 ranged
extern boss_attack *phoenix_move_s2_1;                // 0x2452a24
extern boss_attack *phoenix_move_s2_2;                // 0x2452a30 the ranged repeat
extern boss_attack *phoenix_move_s2_3;                // 0x2452a08 (sub-modes 0x3a / 0x3b)
extern boss_attack *phoenix_move_s2_4;                // 0x2452a64
extern boss_attack *phoenix_move_s3_0;                // 0x2452a18 stage 3 ranged
extern boss_attack *phoenix_move_s3_1;                // 0x2452a28
extern boss_attack *phoenix_move_s3_2;                // 0x2452a2c the ranged repeat
extern boss_attack *phoenix_move_s3_3;                // 0x2452a0c (sub-modes 0x3a / 0x3b)
extern boss_attack *phoenix_move_s3_4;                // 0x2452a04
extern boss_attack *phoenix_move_s3_5;                // 0x2452418 the reposition attack
extern boss_attack *phoenix_move_s3_6;                // 0x2452a20 the dive
extern Character *phoenix_mount;                      // 0x2452a54
extern bool phoenix_loaded;                           // 0x2452a68
extern console_command ccmd_phoenix_stage;            // 0x24529f8
extern console_command ccmd_phoenix_solo;             // 0x2452a38
extern console_command ccmd_phoenix_combat;           // 0x2452a48
extern console_command ccmd_phoenix_unlink;           // 0x2452a58

// Declared in engine/ai/boss_ai.h or engine/ai/ai.h (not repeated): phoenix_unload, phoenix_get_attack_range,
// phoenix_cast_special, phoenix_special_strike, phoenix_select_ranged_move, phoenix_anim_remap_in / _out,
// phoenix_set_foot_anims, phoenix_set_ride_anims, phoenix_set_solo, phoenix_evaluate_target;
// ai_state::phoenix_boss_load, phoenix_seat_rider, phoenix_ai_idle, phoenix_ai_frame, phoenix_check_stage
// and the thiscall ones of this unit.
void  cmd_phoenix_stage();                            // 0x497af0
void  cmd_phoenix_unlink();                           // 0x497b50
Character *phoenix_find_rider();                      // 0x497be0 the last living entity of species 0x58
void  cmd_phoenix_solo();                             // 0x497c50
void  cmd_phoenix_combat();                           // 0x497ca0
void  phoenix_timer_c4_set_random(int lo, int hi);    // 0x4981e0
void  phoenix_rider_set_flinch(move_info *moves);     // 0x498720
void  phoenix_set_stage1_anims(Character *c);         // 0x498840 mounted (stage 1)
void  phoenix_setup_anims_wrapper(Character *c);      // 0x498900
void  phoenix_setup_anims(Character *c);              // 0x498910 on foot (boss_release_mount)
void  phoenix_timer_c0_set_random(int lo, int hi);    // 0x499220
// (list, count, current target, bool exclude current, bool far only): a weighted random handle or -1.
int   phoenix_pick_weighted_move(boss_target *list, int count, int cur_target, bool exclude_cur,
                                 bool far_only);      // 0x499250
int   phoenix_threat_find(int handle);                // 0x4993f0 index or -1

#endif
