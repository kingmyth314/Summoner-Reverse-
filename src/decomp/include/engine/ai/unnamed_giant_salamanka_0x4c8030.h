// engine/ai/unnamed_giant_salamanka_0x4c8030.h - the Giant Salamanka boss (code 0x4c8030-0x4c8d03; source
// game/unnamed_giant_salamanka_0x4c8030.cpp, decomp59). The unit's file name is not known (no assert path,
// no strings.tbl section); this header's name and directory are placeholders. Not original source. Names
// from sum.symbols; the ones marked "ours" are new.
// The fight state is one object (D3904): giant_salamanka_boss_load clears 0x178 dwords from 0x248c7d0
// (rep stosd, 0x4c83b6), which ends exactly where the Luminar unit's .bss starts (0x248cdb0), and one static
// initialiser (init_giant_salamanka_timers, entry 333) constructs its GameTimers. The two console objects
// before it (0x248c7b0, 0x248c7c0) are outside. The sum.symbols names of its fields stay as macros.
#ifndef ENGINE_AI_UNNAMED_GIANT_SALAMANKA_0X4C8030_H
#define ENGINE_AI_UNNAMED_GIANT_SALAMANKA_0X4C8030_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/console.h"
#include "engine/ai/boss_ai.h"

class Character;
class matrix;
struct move_info;

#define GSAL_STAGES 3

// The fight state at 0x248c7d0 (0x5e0 bytes). Type, object and field names ours unless a macro below
// carries the sum.symbols name.
struct giant_salamanka_boss {
    bool combat_enabled;                              // +0x000 0x248c7d0
    ubyte _pad001[3];
    int stage;                                        // +0x004 0x248c7d4 1..3
    int forced_stage;                                 // +0x008 0x248c7d8 cmd_gs_stage (1..3, never cleared)
    bool stage2_reached;                              // +0x00c 0x248c7dc
    bool stage2_ack;                                  // +0x00d 0x248c7dd
    bool stage3_reached;                              // +0x00e 0x248c7de
    bool stage3_ack;                                  // +0x00f 0x248c7df
    int _unk010;                                      // +0x010 0x248c7e0 not accessed
    int missile_tag;                                  // +0x014 0x248c7e4 "$missile"
    int _unk018;                                      // +0x018 0x248c7e8 not accessed
    boss_threat threat[BOSS_MAX_THREATS];             // +0x01c 0x248c7ec
    GameTimer attack_timer;                           // +0x094 0x248c864 spell 0x38 waits 5 s
    int num_offense_points;                           // +0x098 0x248c868
    boss_offense_point offense_points[10];            // +0x09c 0x248c86c
    boss_attack moves[GSAL_STAGES][BOSS_MAX_OFFENSE_POINTS];   // +0x1b4 0x248c984 "gsal_boss.tbl"
    int stage_move_id[GSAL_STAGES];                   // +0x574 0x248cd44 0xa7, 0xac, 0xaf (the frame's default)
    boss_attack *current_move;                        // +0x580 0x248cd50
    int target_handle;                                // +0x584 0x248cd54
    int _unk588;                                      // +0x588 0x248cd58 not accessed
    move_info *flinch_status[GSAL_STAGES];            // +0x58c 0x248cd5c "stageN_flinch_status"
    move_info *flinch_area[GSAL_STAGES];              // +0x598 0x248cd68
    move_info *flinch_right[GSAL_STAGES];             // +0x5a4 0x248cd74
    move_info *flinch_left[GSAL_STAGES];              // +0x5b0 0x248cd80
    move_info *flinch_front[GSAL_STAGES];             // +0x5bc 0x248cd8c
    bool stage1_attacks[8];                           // +0x5c8 0x248cd98
    bool stage2_attacks[8];                           // +0x5d0 0x248cda0
    bool stage3_attacks[8];                           // +0x5d8 0x248cda8
};
LAYOUT_CHECK_32(giant_salamanka_boss, sizeof(giant_salamanka_boss) == 0x5e0);
extern giant_salamanka_boss gsal;                     // 0x248c7d0 (name ours)

#define giant_salamanka_combat_enabled      (gsal.combat_enabled)
#define giant_salamanka_stage               (gsal.stage)
#define giant_salamanka_forced_stage        (gsal.forced_stage)
#define giant_salamanka_stage2_reached      (gsal.stage2_reached)
#define giant_salamanka_stage2_ack          (gsal.stage2_ack)
#define giant_salamanka_stage3_reached      (gsal.stage3_reached)
#define giant_salamanka_stage3_ack          (gsal.stage3_ack)
#define giant_salamanka_missile_tag         (gsal.missile_tag)
#define giant_salamanka_threat_table        (gsal.threat)
#define giant_salamanka_attack_timer        (gsal.attack_timer)
#define giant_salamanka_offense_point_count (gsal.num_offense_points)
#define giant_salamanka_current_move        (gsal.current_move)
#define giant_salamanka_target_handle       (gsal.target_handle)
#define giant_salamanka_stage1_attacks      (gsal.stage1_attacks)
#define giant_salamanka_stage2_attacks      (gsal.stage2_attacks)
#define giant_salamanka_stage3_attacks      (gsal.stage3_attacks)

extern console_command ccmd_gsal_combat;              // 0x248c7b0
extern console_command ccmd_gs_stage;                 // 0x248c7c0
extern int giant_salamanka_bitmap;                    // 0x59debc -1, released by giant_salamanka_free_bitmap, never loaded (name ours)

// Declared in engine/ai/boss_ai.h or engine/ai/ai.h (not repeated): giant_salamanka_free_bitmap,
// _move_has_spell, _fire_attack, _target_in_reach, _move_pushes, _move_knocks_down, _stage_attack_enabled;
// ai_state::giant_salamanka_boss_load, _check_stage, _ai_idle (folded), _ai_frame, _ai_select_action.
void  cmd_gs_stage();                                 // 0x4c80a0
void  cmd_gsal_combat();                              // 0x4c8100
// (character, anim): stand, walk, ready, ... (the eight non-run locomotion slots) all get one anim.
void  character_set_locomotion_anims(Character *c, int anim);   // 0x4c8120
void  giant_salamanka_place_boss1(Character *c);      // 0x4c8160 "walk", navpoint "$boss1"
void  giant_salamanka_place_boss2(Character *c);      // 0x4c81c0 "walk2", "$boss2"
void  giant_salamanka_place_boss3(Character *c);      // 0x4c8220 "walk3", "$boss3"
void  giant_salamanka_set_flinch(move_info *moves);   // 0x4c8280 the char_info move list of type 4
void  giant_salamanka_get_missile_bone(Character *c, matrix *orient, vector3 *pos);   // 0x4c8b30

#endif
