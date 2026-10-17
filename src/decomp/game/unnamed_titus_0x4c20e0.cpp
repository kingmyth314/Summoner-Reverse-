// unnamed_titus_0x4c20e0.cpp - Sum.exe v1.40, code 0x4c20e0-0x4c3467: the Titus boss (species 0xc1).
// File name NOT known: a placeholder (src/decomp/README.md). No assert path, no strings.tbl block; cut by
// its static initialiser run (entries 320-322, 0x588504-0x58850c) and its data.
// Written from the Sum.exe disassembly (tools/asm.mjs; jump tables decoded from the raw bytes).
// Target: VC++ 6.0 (cl 12.00.8966), /O2 like the other boss units. Confidence per function: exact =
// expect identical code, likely = same semantics, small layout risk, unsure = the source shape is a guess.
//
// Unit range: static_init_titus_timers (0x4c20e0) to titus_rock_erupt_damage (0x4c3340-0x4c3467). The Evil
// Joseph unit opens with its initialiser 0x4c3470 (entry 323).
// Data: .data 0x59dc08-0x59dcb7 (the two command names / help texts, "Invalid stage for titus rider\n",
// the combat message, "titus_boss.tbl"); .bss 0x248b380-0x248b977 (titus_move_0 first, ccmd_titus_stage
// last; Evil Joseph's ccmd_darkjo_combat is at 0x248b980). 0x248b950 and 0x248b974 are never accessed.
// The moves are titus_moves[stage][index]; titus_move_<s><i> is [s][i] (titus_move_0 = [0][0], titus_move_21
// = [2][1]). Return codes of titus_ai_select_action: 0 no target, 1 the rock eruption (pending sub-mode
// 0x3c), 3 a new move, 5 keep the current move.

#include "engine/ai/boss_ai.h"
#include "engine/ai/ai.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "engine/spells/spells.h"
#include "engine/effects/fx.h"
#include "vsdk/os/console.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/ca/character.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"

#define AIS_CLOSE_COMBAT     0x0f     // as in ai.cpp (sub-mode names 0x59ad94)
#define AIS_GET_CLOSE        0x16
#define AIS_GET_FAR          0x17
#define AIS_TITUS_ROCK_ERUPT 0x3c     // play the eruption move and start the effect (names ours)
#define AIS_TITUS_ERUPT_NEXT 0x3d     // one frame, then 0x3e
#define AIS_TITUS_ERUPT_WAIT 0x3e     // until the effect ends

#define SPELL_KNOCKDOWN_MOVE 0x3b
#define SPELL_TITUS_CAST     0x4c

// ---- .bss (address order) -----------------------------------------------------------------------------
boss_attack *titus_move_0;            // 0x248b380 &titus_moves[0][0]
boss_attack *titus_move_12;           // 0x248b384 [1][2]
boss_attack *titus_move_21;           // 0x248b388 [2][1] the rock eruption
boss_attack *titus_move_2;            // 0x248b38c [0][2]
boss_attack *titus_move_1;            // 0x248b390 [0][1]
boss_attack *titus_move_13;           // 0x248b394 [1][3]
boss_attack *titus_move_11;           // 0x248b398 [1][1]
boss_attack *titus_move_23;           // 0x248b39c [2][3]
boss_attack *titus_move_10;           // 0x248b3a0 [1][0]
boss_attack *titus_move_22;           // 0x248b3a4 [2][2]
titus_boss titus;                     // 0x248b3a8-0x248b957: stage .. rock_erupt_fx_handle (D8100; fields in boss_ai.h)
// ccmd_titus_combat 0x248b958, titus_move_20 0x248b964 and ccmd_titus_stage 0x248b968: below.

// 0x4c20e0  Confidence: exact. static_init_titus_timers: "jmp 0x4c20f0" (initialiser entry 320).
// 0x4c20f0  Confidence: likely. init_titus_timers: the GameTimer constructor for the five threat records,
// then titus_action_timer, titus_attack_timer, the move 10 / 13 timers and the two stage timers.

// 0x4c2150  Confidence: exact. static_init_ccmd_titus_stage: "jmp 0x4c2160" (entry 321).
// 0x4c2160  Confidence: exact. init_ccmd_titus_stage (console_command constructor).
console_command ccmd_titus_stage("titus_stage", "Set Titus's current combat stage", cmd_titus_stage);   // 0x248b968

// 0x4c2180  Confidence: exact (as cmd_pyrul_stage; the message says "titus rider").
void cmd_titus_stage()
{
    process_command(ARG_INT);
    if (arg_type & ARG_INT) {
        if (command_argument_int >= 0 && command_argument_int <= 3)
            titus_forced_stage = command_argument_int;
        else
            print_to_console("Invalid stage for titus rider\n", 0);
    }
}

// 0x4c21c0  Confidence: exact. static_init_ccmd_titus_combat: "jmp 0x4c21d0" (entry 322).
// 0x4c21d0  Confidence: exact. init_ccmd_titus_combat (console_command constructor).
console_command ccmd_titus_combat("titus_combat", "Sets titus in combat state", cmd_titus_combat);   // 0x248b958
boss_attack *titus_move_20;           // 0x248b964 [2][0] (between the two command objects)

// 0x4c21f0  Confidence: exact.
void cmd_titus_combat()
{
    titus_combat_enabled = true;
    print_to_console("Setting titus' ai state to combat\n", 0);
}

// 0x4c2210  Confidence: exact. boss_target_filter_dispatch case 0xc1 (only the record is pushed). Flags:
// 1 = nearer than 10; stages 2 / 3: 2 = disabled by sub-mode 0x2c / 0x2d (knocked down), else 4; stage 3:
// 8 = not paralysed.
void titus_filter_target(boss_target *t)
{
    if (t->dist < 10.0f)
        t->flags |= 1;
    if (titus_stage == 2 || titus_stage == 3) {
        if (t->character->ai.is_mode(AI_STATE_DISABLED, 0x2c) || t->character->ai.is_mode(AI_STATE_DISABLED, 0x2d))
            t->flags |= 2;
        else
            t->flags |= 4;
        if (titus_stage == 3 && !t->character->has_status(STATUS_PARALYZE))
            t->flags |= 8;
    }
}

// 0x4c22a0  Confidence: exact. Any candidate nearer than range.
bool titus_target_list_any_within(boss_target *list, int count, float range)
{
    int i;

    for (i = 0; i < count; i++) {
        if (list[i].character && list[i].dist < range)
            return true;
    }
    return false;
}

// 0x4c22e0  Confidence: likely (the 1 is stored before the compare result is tested). While move 11 or 23
// (the grabs) plays a move with id 5 and its trigger 1 has passed, the boss does not turn
// (ai_combat_face_target, titus_get_attack_reach).
void titus_update_turn_lock(Character *c)
{
    if ((titus_current_move == titus_move_11 || titus_current_move == titus_move_23)
        && c->cur_move && c->cur_move->id == 5 && c->move_trigger_time_left(1) < 0.0001f)
        titus_turn_lock = true;
    else
        titus_turn_lock = false;
}

// 0x4c2330  Confidence: exact (the last call is a tail jump). Stage 2 entry (the owner is not read).
void titus_reset_attack_timers(Character *c)
{
    titus_attack_timer.SetTimeout(10000);
    titus_move10_timer.SetTimeout(20000);
    titus_move13_timer.invalidate();
}

// 0x4c2360  Confidence: exact. Stage 3 entry (the owner is not read).
void titus_reset_stage_timers(Character *c)
{
    titus_stage_timer_a.SetTimeout(0);
    titus_stage_timer_b.SetTimeout(25000);
}

// folded (0x4f13e0, empty_function)  Confidence: likely. Stage 1 entry (titus_boss_load, titus_check_stage).
void titus_stage1_reset(Character *c)
{
}

// folded (0x4f13e0, empty_function)  Confidence: likely. boss_ai_unload case 0xc1 (boss_ai.h: folded).
void titus_unload()
{
}

// 0x4c2380  Confidence: exact (VC6 byte match, match22; state object D8100; was likely) (the offense-point packing loop as in the other loaders).
void ai_state::titus_boss_load()
{
    int i;
    int tag;

    titus_stage = 1;
    titus_forced_stage = 0;
    titus_stage1_reset(owner);
    titus_target_handle = -1;
    for (i = 0; i < BOSS_MAX_THREATS; i++)
        titus_threat_table[i].handle = -1;
    titus_offense_point_count = 0;
    for (i = 0; i < 10; i++) {
        tag = owner->instance->character->find_mesh_tag(offense_point_name(i));
        if (tag != -1) {
            titus_offense_points[titus_offense_point_count].tag = tag;
            titus_offense_point_count++;
        }
    }
    boss_ai_parse_table("titus_boss.tbl", 3, titus_moves, titus_offense_point_count);
    titus_init_move_tables();
    titus_rock_erupt_move = owner->find_move(9, 1);
}

// 0x4c2450  Confidence: exact (store order as in the binary).
void titus_init_move_tables()
{
    titus_move_0 = &titus_moves[0][0];
    titus_move_1 = &titus_moves[0][1];
    titus_move_2 = &titus_moves[0][2];
    titus_move_10 = &titus_moves[1][0];
    titus_move_11 = &titus_moves[1][1];
    titus_move_12 = &titus_moves[1][2];
    titus_move_13 = &titus_moves[1][3];
    titus_move_20 = &titus_moves[2][0];
    titus_move_21 = &titus_moves[2][1];
    titus_move_22 = &titus_moves[2][2];
    titus_move_23 = &titus_moves[2][3];
}

// 0x4c24c0  Confidence: likely (as pyrul_check_stage: the forced-stage bodies laid out 3, 2, 1; the HP
// branches share the stage store). HP below 30 % is stage 3, below 66 % stage 2. At HP <= 0 while a
// cutscene runs, the boss goes idle and gets CF_HIDDEN.
void ai_state::titus_check_stage()
{
    float frac;

    if (titus_forced_stage != 0) {
        if (titus_stage != titus_forced_stage) {
            switch (titus_forced_stage) {
            default:
                titus_forced_stage = 0;
                break;
            case 3:
                titus_reset_stage_timers(owner);
                titus_stage = 3;
                break;
            case 2:
                titus_reset_attack_timers(owner);
                titus_stage = 2;
                break;
            case 1:
                titus_stage1_reset(owner);
                titus_stage = 1;
                break;
            }
        }
    } else {
        frac = owner->hp / (float)owner->get_max_stat(0);
        if (frac < 0.3f) {
            if (titus_stage != 3) {
                titus_reset_stage_timers(owner);
                titus_stage = 3;
            }
        } else if (frac < 0.66f) {
            if (titus_stage != 2) {
                titus_reset_attack_timers(owner);
                titus_stage = 2;
            }
        } else {
            if (titus_stage != 1) {
                titus_stage1_reset(owner);
                titus_stage = 1;
            }
        }
    }
    if (owner->hp <= 0.0f && (game_flags & GF_CUTSCENE) && !(owner->flags & CF_HIDDEN)) {
        go_idle();
        owner->flags |= CF_HIDDEN;
    }
}

// 0x4c2610  Confidence: exact (go_idle is a tail jump).
void titus_end_attack(Character *c)
{
    titus_current_move = NULL;
    c->ai.go_idle();
}

// 0x4c2630  Confidence: exact (VC6 byte match, match22; result switch, case -1: return; was likely) (the sub-mode switch: case -1 has its own slot, byte table 0x4c2744; the
// bodies are laid out 15, 22/23, 60, 61, 62). The eruption (move 21) goes on without a target.
void ai_state::titus_ai_frame()
{
    int result;
    Character *target;

    result = titus_ai_select_action();
    switch (result) {
    case 0:
        titus_end_attack(owner);
        go_idle();
        return;
    case 1:
        if (titus_pending_mode == AIS_TITUS_ROCK_ERUPT) {
            disable_timer.invalidate();
            set_mode(AI_STATE_COMBAT, AIS_TITUS_ROCK_ERUPT);
        }
        break;
    }
    target = living_from_handle(titus_target_handle);
    owner->combat.target = titus_target_handle;
    if (target && (target->flags & CF_HIDDEN))
        target = NULL;
    if (target == NULL && titus_current_move != titus_move_21) {
        go_idle();
        return;
    }
    if (result == 3)
        combat_approach(target);
    titus_update_turn_lock(owner);
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
    case AIS_TITUS_ROCK_ERUPT:
        titus_ai_rock_erupt();
        break;
    case AIS_TITUS_ERUPT_NEXT:
        titus_start_rock_erupt();
        break;
    case AIS_TITUS_ERUPT_WAIT:
        titus_ai_wait_rock_erupt_fx();
        break;
    }
}

// 0x4c2790  Confidence: unsure (the three stage blocks share tails; the order of the tests matches). Stage
// 1: moves 0 / 1 / 2 weighted 0.4 / 0.4 / 0.2. Stage 2: move 10, then (flag bit 0) move 11 at a standing
// target or move 12 / 13 at a disabled one; the move 10 / 13 timers alternate. Stage 3: move 20 every 50 s
// (15 s retry; at a paralysed target only a flag-8 candidate), the eruption (move 21) every 50 s when any
// candidate is within 5, else move 22 / 23 weighted 0.4 / 0.6 (23 not at a paralysed target). In stage 3
// the target handle is the one held at the start of the retarget code: when the paralysis pick fails it is
// -1 (NOTES, game bugs).
int ai_state::titus_ai_select_action()
{
    boss_target list[BOSS_MAX_THREATS];
    float weights[3];
    Character *target;
    Character *t;
    bool moving;
    bool busy;
    bool timer_up;
    bool retarget;
    bool valid10;
    bool valid13;
    int count;
    int handle;
    int index;

    target = living_from_handle(titus_target_handle);
    if (target && (target->flags & (CF_HIDDEN | CF_DEAD)))
        target = NULL;
    if (titus_current_move == titus_move_21)
        return 5;
    moving = owner->get_current_move() != NULL;
    busy = owner->combat.is_busy();
    if (!moving && busy) {
        owner->combat.state = COMBAT_IDLE;
        busy = false;
    }
    if (target == NULL) {
        titus_flags = 0;
        titus_current_move = NULL;
    }
    if (busy == true)
        return titus_current_move ? 5 : 0;
    retarget = false;
    if (titus_current_move) {
        if (titus_action_timer.GetElapsedTime() > 8000 && submode != AIS_CLOSE_COMBAT) {
            retarget = true;
            titus_current_move = NULL;
        }
    }
    if (titus_current_move)
        return 5;

    timer_up = titus_attack_timer.elapsed();
    titus_update_offense_points(owner);
    count = boss_target_list_build(owner, list, titus_threat_table);

    if (titus_stage == 1) {
        handle = titus_target_handle;
        if (timer_up || !target) {
            handle = boss_target_list_pick(list, count, titus_threat_table, handle, false, 0);
            if (handle == -1)
                return 0;
            titus_target_handle = handle;
            titus_set_attack_timer(20000, 30000);
        } else if (retarget) {
            handle = boss_target_list_pick(list, count, titus_threat_table, handle, false, 0x80000000);
            if (handle == -1)
                return 0;
            titus_target_handle = handle;
            titus_set_attack_timer(20000, 30000);
        }
        weights[0] = 0.4f;
        weights[1] = 0.4f;
        weights[2] = 0.2f;
        index = rand_weighted_index(weights, 3);
        if (index == -1 || index == 0)
            titus_current_move = titus_move_0;
        else if (index == 1)
            titus_current_move = titus_move_1;
        else
            titus_current_move = titus_move_2;
    } else if (titus_stage == 2) {
        if (titus_flags & 1)
            timer_up = false;
        if (!target || timer_up == true) {
            titus_flags = 0;
            handle = boss_target_list_pick(list, count, titus_threat_table, titus_target_handle, false, 0);
            if (handle == -1)
                return 0;
            titus_target_handle = handle;
            titus_set_attack_timer(20000, 30000);
        } else if (retarget) {
            handle = boss_target_list_pick(list, count, titus_threat_table, titus_target_handle, false, 0x80000000);
            if (handle == -1)
                return 0;
            titus_target_handle = handle;
            titus_set_attack_timer(20000, 30000);
        }
        t = living_from_handle(titus_target_handle);
        if (t == NULL)
            return 0;
        if (titus_flags & 1) {
            if (t->ai.is_disabled_any()) {
                if (frand() > 0.5f)
                    titus_current_move = titus_move_12;
                else
                    titus_current_move = titus_move_13;
            } else {
                titus_current_move = titus_move_11;
                titus_flags &= ~1;
                titus_attack_timer.SetTimeout(0);
            }
        } else {
            valid10 = titus_move10_timer.valid();
            valid13 = titus_move13_timer.valid();
            if (valid10 == true) {
                if (valid13 == valid10)
                    titus_move13_timer.invalidate();
            } else if (!valid10) {
                if (valid13)
                    goto move12_13;
                titus_move10_timer.SetTimeout(20000);
            }
            titus_current_move = titus_move_10;
            titus_flags |= 1;
            if (titus_move10_timer.elapsed() == true) {
                titus_move10_timer.invalidate();
                titus_move13_timer.SetTimeout(20000);
            }
            goto done;
move12_13:
            weights[0] = 0.5f;
            weights[1] = 0.5f;
            index = rand_weighted_index(weights, 2);
            if (index == -1 || index == 0)
                titus_current_move = titus_move_12;
            else
                titus_current_move = titus_move_13;
            if (titus_move13_timer.elapsed() == true) {
                titus_move13_timer.invalidate();
                titus_move10_timer.SetTimeout(20000);
            }
        }
    } else if (titus_stage == 3) {
        handle = titus_target_handle;
        if (timer_up || !target) {
            handle = boss_target_list_pick(list, count, titus_threat_table, titus_target_handle, false, 0);
            if (handle == -1)
                return 0;
            titus_target_handle = handle;
            titus_set_attack_timer(20000, 30000);
        } else if (retarget) {
            handle = boss_target_list_pick(list, count, titus_threat_table, titus_target_handle, false, 0x80000000);
            if (handle == -1)
                return 0;
            titus_target_handle = handle;
            titus_set_attack_timer(20000, 30000);
        }
        t = living_from_handle(titus_target_handle);
        if (t == NULL)
            return 0;
        if (titus_stage_timer_a.elapsed()) {
            handle = titus_target_handle;
            if (t->has_status(STATUS_PARALYZE))
                handle = boss_target_list_pick(list, count, titus_threat_table, titus_target_handle, false, 8);
            if (handle != -1) {
                titus_stage_timer_a.SetTimeout(50000);
                titus_target_handle = handle;
                titus_current_move = titus_move_20;
                goto done;
            }
            titus_stage_timer_a.SetTimeout(15000);
        }
        if (titus_stage_timer_b.elapsed() && titus_target_list_any_within(list, count, 5.0f)) {
            titus_stage_timer_b.SetTimeout(50000);
            titus_target_handle = handle;
            titus_current_move = titus_move_21;
            titus_pending_mode = AIS_TITUS_ROCK_ERUPT;
            return 1;
        }
        weights[0] = 0.4f;
        weights[1] = 0.6f;
        index = rand_weighted_index(weights, 2);
        if ((index == -1 || index == 1) && !t->has_status(STATUS_PARALYZE)) {
            titus_target_handle = handle;
            titus_current_move = titus_move_23;
        } else {
            titus_target_handle = handle;
            titus_current_move = titus_move_22;
        }
    } else {
        return 0;
    }
done:
    titus_action_timer.SetTimeout(0);
    return 3;
}

// 0x4c2d90  Confidence: exact.
void titus_set_attack_timer(int min_ms, int max_ms)
{
    titus_attack_timer.SetTimeout(min_ms + (int)(frand() * (max_ms - min_ms)));
}

// 0x4c2dc0  Confidence: exact (VC6 byte match, match22; state object D8100, vector sum; was likely), as pyrul_update_attack_points.
void titus_update_offense_points(Character *c)
{
    int i;
    matrix orient;
    vector3 pos;
    boss_offense_point *pt;

    for (i = 0; i < titus_offense_point_count; i++) {
        pt = &titus_offense_points[i];
        c->instance->get_bone_world_transform(pt->tag, &c->orient, &c->pos, &orient, &pos);
        pt->dir = orient.uvec;
        pt->pos = pos + c->centre_offset;
    }
}

// 0x4c2e60  Confidence: exact. boss_ai_idle_dispatch case 0xc1.
void ai_state::titus_ai_idle()
{
    if (owner->get_current_move() == NULL && owner->combat.state != COMBAT_IDLE) {
        owner->combat.state = COMBAT_IDLE;
        return;
    }
    if (titus_combat_enabled == true) {
        titus_current_move = NULL;
        enter_combat();
    }
}

// 0x4c2ea0  Confidence: exact. boss_get_move_range case 0xc1: 10 while turn-locked in an attack and for
// move 20, else the char_info attack radius.
float titus_get_attack_reach(Character *c)
{
    if (titus_turn_lock == true && c->combat.state == COMBAT_ATTACKING)
        return 10.0f;
    if (titus_current_move == titus_move_20)
        return 10.0f;
    return c->info->attack_radius;
}

// 0x4c2ef0  Confidence: exact (VC6 byte match, cl 8966). boss_attack_trigger_dispatch case 0xc1. Spell 0x4c casts
// at the target;
// spell 0x3b: moves 10 / 22 knock down the party members within 6 (chance 0.8, the target always) for 2-3
// s, moves 11 / 23 the target alone for 6-9 s.
void titus_knock_down_party(Character *boss, Character *target)
{
    Player *p;
    Character *c;

    if (titus_current_move == NULL || titus_current_move->spell == -1)
        return;
    if (titus_current_move->spell != SPELL_KNOCKDOWN_MOVE) {
        if (titus_current_move->spell == SPELL_TITUS_CAST)
            spell_cast_execute(boss, SPELL_TITUS_CAST, target->handle, NULL, -1, 0, 0, 0);
    } else if (titus_current_move != titus_move_10 && titus_current_move != titus_move_22) {
        if (titus_current_move == titus_move_11 || titus_current_move == titus_move_23) {
            if (target->ai.can_be_knocked_down() == true)
                target->ai.knock_down(rand_int_range(6000, 9000));
        }
    } else {
        for (p = party_list.next; p != &party_list; p = p->next) {
            c = living_from_handle(p->handle);
            if (c == NULL)
                continue;
            vector3 d(c->pos.x - boss->pos.x, c->pos.y - boss->pos.y, c->pos.z - boss->pos.z);
            if (d.y * d.y + d.x * d.x + d.z * d.z < 36.0f && (frand() < 0.8f || c == target) && c->ai.can_be_knocked_down() == true)
                c->ai.knock_down(rand_int_range(2000, 3000));
        }
    }
}

// 0x4c3060  Confidence: exact. 0 moves 0 / 12, 1 moves 1 / 13, 2 move 2, 3 moves 10 / 22, 4 moves 11 / 23
// (the grabs), 5 move 20, -1 none (the argument is not read).
int titus_get_attack_kind(Character *c)
{
    if (titus_current_move == NULL)
        return -1;
    if (titus_current_move == titus_move_0 || titus_current_move == titus_move_12)
        return 0;
    if (titus_current_move == titus_move_1 || titus_current_move == titus_move_13)
        return 1;
    if (titus_current_move == titus_move_2)
        return 2;
    if (titus_current_move == titus_move_10 || titus_current_move == titus_move_22)
        return 3;
    if (titus_current_move == titus_move_11 || titus_current_move == titus_move_23)
        return 4;
    return titus_current_move == titus_move_20 ? 5 : -1;
}

// 0x4c30e0  Confidence: exact. boss_hook_dispatch_51_c1 case 0xc1: hold while a move plays, or for a grab
// while the target is disabled or in its sub-mode 0xe / sneak attack.
bool titus_hold_attack(Character *boss, Character *target)
{
    if (boss->get_current_move())
        return true;
    if (titus_get_attack_kind(boss) == 4) {
        if (target->ai.is_disabled_any())
            return true;
        if (target->ai.is_combat_e_or_sneak_attack() == true)
            return true;
    }
    return false;
}

// 0x4c3130  Confidence: exact. The members of cur_character's group within 6 of the boss (horizontally)
// whose defence fails.
bool titus_collect_area_hits(Character *c)
{
    ai_group *group;
    Character *t;
    int i;

    titus_area_hit_count = 0;
    group = cur_character->ai.group;
    if (group) {
        for (i = 0; i < group->count; i++) {
            t = living_from_handle(group->members[i]);
            if (t == NULL)
                continue;
            float dx = t->pos.x - c->pos.x;
            float dz = t->pos.z - c->pos.z;
            if (dz * dz + dx * dx < 36.0f && living_combat_test_defense(&c->combat, &t->combat))
                titus_area_hits[titus_area_hit_count++] = group->members[i];
        }
    }
    return titus_area_hit_count > 0;
}

// 0x4c31f0  Confidence: exact.
void titus_apply_area_hits(Character *c)
{
    int i;
    Character *t;

    for (i = 0; i < titus_area_hit_count; i++) {
        t = living_from_handle(titus_area_hits[i]);
        if (t)
            living_combat_resolve_hit_vs(&c->combat, &t->combat);
    }
}

// 0x4c3240  Confidence: exact. Sub-mode AIS_TITUS_ROCK_ERUPT: the eruption move (9, 1) and the
// "Titus-RockErrupt.VFX" effect at the boss; either failing ends the attack.
void ai_state::titus_ai_rock_erupt()
{
    if (owner->play_move((int)titus_rock_erupt_move, 1.0f, true, true, false) == -1)
        goto fail;
    titus_rock_erupt_fx_handle = titus_rock_erupt_fx(owner->handle);
    if (titus_rock_erupt_fx_handle == -1) {
fail:
        titus_end_attack(owner);
        return;
    }
    set_mode(AI_STATE_COMBAT, AIS_TITUS_ERUPT_NEXT);
}

// 0x4c32a0  Confidence: exact. Sub-mode AIS_TITUS_ERUPT_NEXT.
void ai_state::titus_start_rock_erupt()
{
    set_mode(AI_STATE_COMBAT, AIS_TITUS_ERUPT_WAIT);
}

// 0x4c32b0  Confidence: exact. Sub-mode AIS_TITUS_ERUPT_WAIT.
void ai_state::titus_ai_wait_rock_erupt_fx()
{
    if (fx_is_finished(titus_rock_erupt_fx_handle, NULL))
        titus_end_attack(owner);
}

// 0x4c32e0  Confidence: exact. boss_cast_special_dispatch case 0xc1: spell 0x4c is cast (a goal in
// multiplayer); 0x3b and the other moves report no cast. The current move is not NULL-checked.
bool titus_cast_special(Character *c, int anim)
{
    int spell = titus_current_move->spell;

    if (spell == -1 || spell == SPELL_KNOCKDOWN_MOVE || spell != SPELL_TITUS_CAST)
        return false;
    if (game_flags & GF_MULTIPLAYER) {
        c->ai.add_cast_at_target(SPELL_TITUS_CAST, c->combat.opponent->owner, 999999);
        return true;
    }
    c->perform_spell_cast(SPELL_TITUS_CAST, false);
    return true;
}

// 0x4c3340  Confidence: exact. Called by fx_frame for the eruption effect (its position and owner): the
// party within 8 (horizontally) loses (0.7 + frand() * 0.6) * 120 HP. The living_from_handle results are
// not NULL-checked (NOTES, game bugs).
void titus_rock_erupt_damage(const vector3 *pos, int source)
{
    ai_group *group;
    Player *p;
    Character *c;
    int i;

    if (game_flags & GF_SINGLE_PLAYER) {
        group = cur_character->ai.group;
        if (group == NULL)
            return;
        for (i = 0; i < group->count; i++) {
            c = living_from_handle(group->members[i]);
            float dx = c->pos.x - pos->x;
            float dz = c->pos.z - pos->z;
            if (dz * dz + dx * dx < 64.0f) {
                float damage = (frand() * 0.6f + 0.7f) * 120.0f;
                c->add_hp(-damage, source, 1, false);
            }
        }
    } else {
        for (p = party_list.next; p != &party_list; p = p->next) {
            c = living_from_handle(p->handle);
            float dx = c->pos.x - pos->x;
            float dz = c->pos.z - pos->z;
            if (dz * dz + dx * dx < 64.0f) {
                float damage = (frand() * 0.6f + 0.7f) * 120.0f;
                c->add_hp(-damage, source, 1, false);
            }
        }
    }
}
