// engine/ai/unnamed_serpent_0x495290.h - the Serpent Rider boss (code 0x495290-0x497aba; source
// game/unnamed_serpent_0x495290.cpp, decomp54). The unit's file name is not known (no assert path, no
// strings.tbl section); this header's name and directory are placeholders. Not original source. Names
// from sum.symbols; the ones marked "ours" are new.
// The fight state is one object (D3403): one static initialiser (init_serpent_timers, entry 273)
// constructs all its GameTimers, and serpent_boss_load clears it with one rep stosd of 0x190 dwords
// from 0x2451d98, which ends exactly at the first console object (0x24523d8). The sum.symbols names of
// its fields stay as macros (as D1900, D3200).
#ifndef ENGINE_AI_UNNAMED_SERPENT_0X495290_H
#define ENGINE_AI_UNNAMED_SERPENT_0X495290_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/console.h"
#include "engine/ai/boss_ai.h"

class Character;
struct move_info;

#define SERPENT_STAGES     3
#define SERPENT_MAX_PARTS  3          // the serpent and its two illusion copies (serpent_part_index)

// serpent_offense_point (the "$OffenseNN" mesh tag record) is in engine/ai/boss_ai.h (the phoenix uses it too).

// The boss_target flags serpent_evaluate_target sets (boss_target_filter_dispatch). Names ours.
#define SERPENT_TF_ASLEEP      0x002  // status 9 time left
#define SERPENT_TF_PARALYZED   0x004  // status 10
#define SERPENT_TF_POISONED    0x008  // status 11
#define SERPENT_TF_SILENCED    0x010  // status 1
#define SERPENT_TF_BLIND       0x020  // status 0
#define SERPENT_TF_DISABLED    0x040  // stage 3: in AI mode (8, 0x2d)
#define SERPENT_TF_BEHIND      0x080  // stage 3: the target faces away and stands in front of the serpent
#define SERPENT_TF_WHIP        0x100  // stage 3: inside the whip's reach and angle (attacks[2][0])

// The fight state at 0x2451d98 (0x640 bytes). Type, object and field names ours unless a macro below
// carries the sum.symbols name.
struct serpent_boss {
    int stage;                                        // +0x000 0x2451d98
    int target_stage;                                 // +0x004 0x2451d9c
    bool dismount_stage_reached;                      // +0x008 0x2451da0
    bool stage_trigger;                               // +0x009 0x2451da1
    bool combat;                                      // +0x00a 0x2451da2
    bool has_rider;                                   // +0x00b 0x2451da3
    int _unk00c;                                      // +0x00c
    int anim_remap_count;                             // +0x010 0x2451da8
    int remap_slot[2];                                // +0x014 loco_anims index per blend entry
    float remap_weight[2];                            // +0x01c the blend weights, kept for remap_out
    int num_offense_points;                           // +0x024 0x2451dbc
    serpent_offense_point offense_points[10];         // +0x028 0x2451dc0
    boss_attack attacks[SERPENT_STAGES][BOSS_MAX_OFFENSE_POINTS];   // +0x140 0x2451ed8 serp_boss.tbl
    GameTimer timer_attack;                           // +0x500 0x2452298
    GameTimer place_timer;                            // +0x504 0x245229c
    GameTimer timer_508;                              // +0x508 0x24522a0 constructed only
    GameTimer illusion_timer;                         // +0x50c 0x24522a4
    int num_extra_parts;                              // +0x510 0x24522a8
    int handles[SERPENT_MAX_PARTS];                   // +0x514 0x24522ac [0] the serpent, then the parts
    vector3 part_pos[SERPENT_MAX_PARTS];              // +0x520 0x24522b8 serpent_place_parts
    GameTimer illusion_active_timer;                  // +0x544 0x24522dc
    bool illusion_fx_on;                              // +0x548 0x24522e0
    ubyte _pad549[3];
    int _unk54c;                                      // +0x54c
    boss_threat threat[BOSS_MAX_THREATS];             // +0x550 0x24522e8
    int whip_hit_count;                               // +0x5c8 0x2452360
    int whip_hits[5];                                 // +0x5cc 0x2452364
    int part_target[SERPENT_MAX_PARTS];               // +0x5e0 0x2452378
    boss_attack *part_busy[SERPENT_MAX_PARTS];        // +0x5ec 0x2452384
    int part_move[SERPENT_MAX_PARTS];                 // +0x5f8 0x2452390
    int part_attack_count[SERPENT_MAX_PARTS];         // +0x604 0x245239c
    int pending_submode;                              // +0x610 0x24523a8 0x20 wander, 0x38 place parts
    GameTimer timer_a;                                // +0x614 0x24523ac
    GameTimer timer_b;                                // +0x618 0x24523b0
    GameTimer wander_timer;                           // +0x61c 0x24523b4
    bool first_attack;                                // +0x620 0x24523b8
    ubyte _pad621[3];
    int _unk624;                                      // +0x624
    move_info *flinch_status[SERPENT_STAGES];         // +0x628 0x24523c0 "stageN_flinch_status"
    move_info *flinch_area[SERPENT_STAGES];           // +0x634 0x24523cc "stageN_flinch_area"
};
LAYOUT_CHECK_32(serpent_boss, sizeof(serpent_boss) == 0x640);
extern serpent_boss serpent;                          // 0x2451d98 (name ours)

#define serpent_stage                   (serpent.stage)
#define serpent_target_stage            (serpent.target_stage)
#define serpent_dismount_stage_reached  (serpent.dismount_stage_reached)
#define serpent_stage_trigger           (serpent.stage_trigger)
#define serpent_combat                  (serpent.combat)
#define serpent_has_rider               (serpent.has_rider)
#define serpent_anim_remap_count        (serpent.anim_remap_count)
#define serpent_num_offense_points      (serpent.num_offense_points)
#define serpent_attack_table            (serpent.attacks)
#define serpent_whip_reach              (serpent.attacks[2][0].range_max)       // 0x2452160
#define serpent_timer_attack            (serpent.timer_attack)
#define serpent_place_timer             (serpent.place_timer)
#define serpent_illusion_timer          (serpent.illusion_timer)
#define serpent_num_extra_parts         (serpent.num_extra_parts)
#define serpent_handle                  (serpent.handles[0])
#define serpent_part_handles            (&serpent.handles[1])
#define serpent_illusion_active_timer   (serpent.illusion_active_timer)
#define serpent_threat_table            (serpent.threat)
#define serpent_whip_hit_count          (serpent.whip_hit_count)
#define serpent_whip_hits               (serpent.whip_hits)
#define serpent_part_target             (serpent.part_target)
#define serpent_part_busy               (serpent.part_busy)
#define serpent_part_move               (serpent.part_move)
#define serpent_part_attack_count       (serpent.part_attack_count)
#define serpent_timer_a                 (serpent.timer_a)
#define serpent_timer_b                 (serpent.timer_b)
#define serpent_wander_timer            (serpent.wander_timer)
#define serpent_first_attack            (serpent.first_attack)

// The move ids the attacks play (set by serpent_boss_load; the last two names are ours).
extern int serpent_attack_anim_a;                     // 0x2451d80 1
extern int serpent_attack_anim_b;                     // 0x2451d84 3
extern int serpent_attack_anim_c;                     // 0x2451d88 6
extern int serpent_illusion_anim;                     // 0x2451d8c 7
extern int serpent_attack_anim_d;                     // 0x2451d90 5
extern int serpent_attack_anim_e;                     // 0x24523e4 2
extern int serpent_attack_anim_f;                     // 0x24523f4 4
extern Character *serpent_mount;                      // 0x2452414
extern bool serpent_boss_needs_setup;                 // 0x59c65c (init 1)
extern console_command ccmd_serpent_solo;             // 0x24523d8
extern console_command ccmd_serpent_stage;            // 0x24523e8
extern console_command ccmd_serpent_unlink;           // 0x24523f8
extern console_command ccmd_serpent_combat;           // 0x2452408

// Declared in engine/ai/boss_ai.h or engine/ai/ai.h (not repeated): serpent_unload, serpent_anim_remap_in /
// _out, serpent_evaluate_target, serpent_part_index, boss_part_deactivate, serpent_set_foot_anims,
// serpent_set_ride_anims, serpent_set_cutscene_walk_anim, serpent_whip_test, serpent_whip_resolve_hit,
// serpent_move_in_range, serpent_set_solo; ai_state::serpent_boss_load, serpent_check_stage,
// serpent_seat_rider, serpent_ai_idle, serpent_ai_frame, serpent_part_check_timeout.
void  cmd_serpent_stage();                            // 0x495340
void  cmd_serpent_unlink();                           // 0x4953a0
Character *serpent_get_rider();                       // 0x495410 the serpent rider itself (serpent_handle)
void  cmd_serpent_solo();                             // 0x495450
void  cmd_serpent_combat();                           // 0x4954a0
void  serpent_rider_set_flinch(move_info *moves);     // 0x4954c0 (the char_info flinch list, +0x5c)
void  serpent_spawn_rider(Character *c);              // 0x4955e0 mounted, with the two illusion parts
void  serpent_setup_anims(Character *c);              // 0x495830 on foot (boss_release_mount)
void  serpent_deactivate_parts(Character *c);         // 0x495910
float serpent_target_weight(int species);             // 0x495fc0 the pick weight of a hero species
int   serpent_pick_target(boss_target *list, int count);   // 0x496000 index or -1
bool  serpent_part_is_asleep(boss_target *list, int i);    // 0x496060
bool  serpent_part_is_busy(boss_target *list, bool asleep, int i);   // 0x4960a0
bool  serpent_part_is_busy_or_asleep(boss_target *list, int i);      // 0x4960f0
void  serpent_timer_a_set_random(int lo, int hi);     // 0x496150
void  serpent_timer_b_set_random(int lo, int hi);     // 0x496180
void  serpent_illusion_freeze_party();                // 0x496b40
void  serpent_part_update_alpha(Character *c);        // 0x497370

#endif
