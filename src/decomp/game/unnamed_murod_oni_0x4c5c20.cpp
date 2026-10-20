// unnamed_murod_oni_0x4c5c20.cpp - Sum.exe v1.40, code 0x4c5c20-0x4c6470: the Murod Oni boss (species 0xb8),
// a melee boss. File name NOT known: a placeholder (src/decomp/README.md). No assert path, no strings.tbl
// block; cut by its static initialiser (entry 328, 0x588524) and its data.
// Written from the Sum.exe disassembly (tools/asm.mjs; jump tables decoded from the raw bytes).
// Target: VC++ 6.0 (cl 12.00.8966), /O2 like the other boss units. Confidence per function: exact =
// expect identical code, likely = same semantics, small layout risk, unsure = the source shape is a guess.
//
// Unit range: static_init_murod_oni_timers (0x4c5c20) to murod_oni_move_is_kick (0x4c6460-0x4c6470). The
// tentacle beast unit opens with its initialiser 0x4c6480 (entry 329).
// Data: .data 0x59ddd4-0x59dde6 ("murod_oni_boss.tbl"); .bss 0x248bff0-0x248c31b (murod_oni_move_0 first,
// murod_oni_move_2 last; the tentacle unit starts at 0x248c320). 0x248bff8-0x248bfff and the timer
// 0x248c2f0 are never used.
// Return codes of murod_oni_ai_select_action: 0 no target, 3 a new move, 5 keep the current move.

#include "engine/ai/boss_ai.h"
#include "engine/ai/ai.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/ca/character.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"

#define AIS_CLOSE_COMBAT  0x0f        // as in ai.cpp (sub-mode names 0x59ad94)
#define AIS_GET_CLOSE     0x16
#define AIS_GET_FAR       0x17

// ---- .bss (address order) -----------------------------------------------------------------------------
boss_attack *murod_oni_move_0;        // 0x248bff0 &murod_oni_moves[0][0]
boss_attack *murod_oni_move_3;        // 0x248bff4 [0][3]
int  murod_oni_248bff8[2];            // 0x248bff8 never accessed
int  murod_oni_248c000;               // 0x248c000 zeroed when the target is lost, never read
murod_oni_boss murod_oni;             // 0x248c004-0x248c30f: engaged .. area_hits (D10503; fields in boss_ai.h)
boss_attack *murod_oni_move_1;        // 0x248c310 [0][1]
boss_attack *murod_oni_move_4;        // 0x248c314 [0][4] the crowd move
boss_attack *murod_oni_move_2;        // 0x248c318 [0][2]

// 0x4c5c20  Confidence: exact. static_init_murod_oni_timers: "jmp 0x4c5c30" (initialiser entry 328).
// 0x4c5c30  Confidence: likely. init_murod_oni_timers: the GameTimer constructor for the five threat records,
// then the attack, crowd, unused and second timers.

// 0x4c5c80  Confidence: exact. boss_ai_idle_dispatch case 0xb8 (murod_oni_engaged is set at load).
void ai_state::murod_oni_ai_idle()
{
    if (owner->get_current_move() == NULL && owner->combat.state != COMBAT_IDLE) {
        owner->combat.state = COMBAT_IDLE;
        return;
    }
    if (murod_oni_engaged == true) {
        murod_oni_current_move = NULL;
        enter_combat();
    }
}

// 0x4c5cc0  Confidence: exact (VC6 byte match, w6 pass; was likely) (the offense-point packing loop as in the other loaders). One stage.
void ai_state::murod_oni_boss_load()
{
    int i;
    int tag;

    murod_oni_engaged = true;
    murod_oni_init_timers(owner);
    murod_oni_target_handle = -1;
    murod_oni_saved_target_handle = -1;
    for (i = 0; i < BOSS_MAX_THREATS; i++)
        murod_oni_threat_table[i].handle = -1;
    murod_oni_offense_point_count = 0;
    for (i = 0; i < 10; i++) {
        tag = owner->instance->character->find_mesh_tag(offense_point_name(i));
        if (tag != -1) {
            murod_oni_offense_points[murod_oni_offense_point_count].tag = tag;
            murod_oni_offense_point_count++;
        }
    }
    boss_ai_parse_table("murod_oni_boss.tbl", 1, murod_oni_moves, murod_oni_offense_point_count);
    murod_oni_init_move_tables();
}

// 0x4c5d80  Confidence: exact (store order as in the binary).
void murod_oni_init_move_tables()
{
    murod_oni_move_0 = &murod_oni_moves[0][0];
    murod_oni_move_1 = &murod_oni_moves[0][1];
    murod_oni_move_2 = &murod_oni_moves[0][2];
    murod_oni_move_3 = &murod_oni_moves[0][3];
    murod_oni_move_4 = &murod_oni_moves[0][4];
}

// 0x4c5dc0  Confidence: exact (the owner is pushed but not read).
void murod_oni_init_timers(Character *c)
{
    murod_oni_crowd_timer.SetTimeout(20000);
    murod_oni_timer_b.SetTimeout(5000);
}

// 0x4c5de0  Confidence: exact (D11400: the pick test positive, `else return 0`, puts the failure last). With more than one attacker, move 4 every 20 s; once the 5 s timer has run
// out (it is never rearmed), at a flag-4 candidate (in move 2's cone) with chance 0.3, move 2 at it and the
// old target is kept to come back to; otherwise move 1 / 3 / 0 at 30 / 30 / 40 %.
int ai_state::murod_oni_ai_select_action()
{
    boss_target list[BOSS_MAX_THREATS];
    Character *target;
    bool busy;
    bool timer_up;
    int count;
    int attackers;
    int handle;
    float r;

    target = living_from_handle(murod_oni_target_handle);
    if (target && (target->flags & CF_HIDDEN))
        target = NULL;
    busy = owner->combat.is_busy();
    if (target == NULL) {
        murod_oni_current_move = NULL;
        murod_oni_248c000 = 0;
    }
    if (busy == true)
        return murod_oni_current_move ? 5 : 0;
    if (murod_oni_current_move)
        return 5;
    timer_up = murod_oni_attack_timer.elapsed();
    murod_oni_update_offense_points(owner);
    count = boss_target_list_build(owner, list, murod_oni_threat_table);
    attackers = count_attackers();
    if (murod_oni_crowd_timer.elapsed() && attackers > 1) {
        murod_oni_current_move = murod_oni_move_4;
        murod_oni_crowd_timer.SetTimeout(20000);
        return 3;
    }
    if (murod_oni_timer_b.elapsed()) {
        handle = boss_target_list_pick(list, count, murod_oni_threat_table, murod_oni_target_handle, false, 4);
        if (handle != -1 && frand() < 0.3f) {
            murod_oni_saved_target_handle = murod_oni_target_handle;
            murod_oni_target_handle = handle;
            murod_oni_current_move = murod_oni_move_2;
            return 3;
        }
    }
    if (!timer_up && target) {
        if (murod_oni_saved_target_handle != -1) {
            murod_oni_target_handle = murod_oni_saved_target_handle;
            murod_oni_saved_target_handle = -1;
        }
    } else {
        murod_oni_target_handle = boss_target_list_pick(list, count, murod_oni_threat_table,
                                                        murod_oni_target_handle, false, 0);
        if (murod_oni_target_handle != -1)
            murod_oni_set_attack_timer(10000, 20000);
        else
            return 0;
    }
    r = frand();
    if (r < 0.3f)
        murod_oni_current_move = murod_oni_move_1;
    else if (r < 0.6f)
        murod_oni_current_move = murod_oni_move_3;
    else
        murod_oni_current_move = murod_oni_move_0;
    return 3;
}

// 0x4c6010  Confidence: exact (VC6 byte match, w6 pass; was likely), as pyrul_update_attack_points.
void murod_oni_update_offense_points(Character *c)
{
    int i;
    matrix orient;
    vector3 pos;
    boss_offense_point *pt;

    for (i = 0; i < murod_oni_offense_point_count; i++) {
        pt = &murod_oni_offense_points[i];
        c->instance->get_bone_world_transform(pt->tag, &c->orient, &c->pos, &orient, &pos);
        pt->dir = orient.uvec;
        pt->pos = pos + c->centre_offset;
    }
}

// 0x4c60b0  Confidence: exact.
void murod_oni_set_attack_timer(int min_ms, int max_ms)
{
    murod_oni_attack_timer.SetTimeout(min_ms + (int)(frand() * (max_ms - min_ms)));
}

// 0x4c60e0  Confidence: exact (the sub-mode switch: case -1 has its own slot, byte table 0x4c6174).
void ai_state::murod_oni_ai_frame()
{
    int result;
    Character *target;

    result = murod_oni_ai_select_action();
    if (result == 0) {
        go_idle();
        return;
    }
    target = living_from_handle(murod_oni_target_handle);
    owner->combat.target = murod_oni_target_handle;
    if (target == NULL || (target->flags & CF_HIDDEN)) {
        go_idle();
        return;
    }
    if (result == 3)
        combat_approach(target);
    switch (submode) {
    case -1:
        return;
    case AIS_CLOSE_COMBAT:
        combat_melee_attack(target);
        break;
    case AIS_GET_CLOSE:
    case AIS_GET_FAR:
        combat_close_in(target);
        break;
    }
}

// 0x4c6190  Confidence: exact. boss_target_filter_dispatch case 0xb8 (as pyrul_target_filter's first
// flags): 1 an attacker whose char_info lacks flag 0x10; 2 attacking (or sub-mode 0x16) the boss; 4 in move
// 2's cone.
void murod_oni_evaluate_target(Character *boss, boss_target *t)
{
    if (t->character == NULL)
        return;
    if (!(t->character->info->flags & 0x10) && t->character->ai.is_attacking() == true)
        t->flags |= 1;
    if (t->character->ai.is_attacking() == true || t->character->ai.is_combat_sub16()) {
        if (t->character->ai.target == boss->handle)
            t->flags |= 2;
    }
    if (murod_oni_offense_point_reaches(t->character, murod_oni_move_2))
        t->flags |= 4;
}

// 0x4c6230  Confidence: exact (as machival_offense_point_reaches).
bool murod_oni_offense_point_reaches(Character *c, boss_attack *move)
{
    boss_offense_point *pt = &murod_oni_offense_points[move->offense_point];
    vector3 d(c->pos.x - pt->pos.x, 0.0f, c->pos.z - pt->pos.z);

    if (d.normalize_safe_length() > move->range_max)
        return false;
    if (d.dot(pt->dir) < move->cos_max_angle)
        return false;
    return true;
}

// 0x4c62d0  Confidence: exact. The move index 0..4, -1 none.
int murod_oni_get_attack_kind(Character *c)
{
    if (murod_oni_current_move == NULL)
        return -1;
    if (murod_oni_current_move == murod_oni_move_0)
        return 0;
    if (murod_oni_current_move == murod_oni_move_1)
        return 1;
    if (murod_oni_current_move == murod_oni_move_2)
        return 2;
    if (murod_oni_current_move == murod_oni_move_3)
        return 3;
    return murod_oni_current_move == murod_oni_move_4 ? 4 : -1;
}

// 0x4c6330  Confidence: exact (VC6 byte match, w6 pass; was likely) (the normalised direction is not used). The members of cur_character's group
// within the current move's range (horizontally) whose defence fails.
bool murod_oni_collect_area_hits(Character *c)
{
    ai_group *group;
    Character *t;
    int i;

    murod_oni_area_hit_count = 0;
    group = cur_character->ai.group;
    if (group) {
        for (i = 0; i < group->count; i++) {
            t = living_from_handle(group->members[i]);
            if (t == NULL)
                continue;
            vector3 d(t->pos.x - c->pos.x, 0.0f, t->pos.z - c->pos.z);
            if (d.normalize_safe_length() <= murod_oni_current_move->range_max
                && living_combat_test_defense(&c->combat, &t->combat))
                murod_oni_area_hits[murod_oni_area_hit_count++] = group->members[i];
        }
    }
    return murod_oni_area_hit_count > 0;
}

// 0x4c6410  Confidence: exact.
void murod_oni_apply_area_hits(Character *c)
{
    int i;
    Character *t;

    for (i = 0; i < murod_oni_area_hit_count; i++) {
        t = living_from_handle(murod_oni_area_hits[i]);
        if (t)
            living_combat_resolve_hit_vs(&c->combat, &t->combat);
    }
}

// 0x4c6460  Confidence: exact. boss_attack_stuns_dispatch case 0xb8: move 3 stuns.
bool murod_oni_move_is_kick()
{
    return murod_oni_current_move == murod_oni_move_3 ? true : false;
}

// folded (0x4f13e0, empty_function)  Confidence: likely. boss_ai_unload case 0xb8 (boss_ai.h: folded).
void murod_oni_unload()
{
}

// folded (0x4f13e0, empty_function)  Confidence: likely. boss_ai_check_stage case 0xb8 (ai.h: folded).
void ai_state::murod_oni_check_stage()
{
}

// folded (empty_function)  Confidence: likely. boss_attack_trigger_dispatch case 0xb8 (boss_ai.h: folded).
void murod_oni_fire_attack(Character *boss, Character *target)
{
}

// folded (ls_test_use: xor al, al)  Confidence: likely. boss_cast_special_dispatch case 0xb8.
bool murod_oni_cast_special(Character *c, int anim)
{
    return false;
}
