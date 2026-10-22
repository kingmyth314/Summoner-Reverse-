// unnamed_pyrul_0x4b87c0.cpp - Sum.exe v1.40, code 0x4b87c0-0x4b9e62: the Pyrul boss (species 0xc4).
// File name NOT known: a placeholder (src/decomp/README.md). The unit has no assert path and no strings.tbl
// block; it is cut by its static initialiser run (entries 306-308, 0x5884cc-0x5884d4) and its data.
// Written from the Sum.exe disassembly (tools/asm.mjs; jump tables decoded from the raw bytes).
// Target: VC++ 6.0 (cl 12.00.8966), /O2 (no frame pointer, 0x90 padding) like the other boss units.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: static_init_pyrul_timers (0x4b87c0) to random_stun_duration (0x4b9e50-0x4b9e62). The
// previous function, pathvis_bsp_contains (0x4b86e0), is pathfinding_visibility.cpp's last; the next
// unit (Ghost Rider) opens with its initialiser 0x4b9e70 (entry 309).
// Data: .data 0x59d934-0x59d9eb (the two command names / help texts, "Invalid stage for pyrul\n", the
// combat message, "pyrul_boss.tbl" and "$missile01"); .bss 0x247fcf0-0x24802df (ccmd_pyrul_stage first,
// pyrul_move_3_1 last; the Ghost Rider's move pointers start at 0x24802e0). 0x247fd24 is never accessed
// (the 4 bytes after ccmd_pyrul_combat). Floats are pooled .rdata constants.
// random_stun_duration has no data; it sits before the ghost initialiser, so it ends this unit.

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

#define AIS_CLOSE_COMBAT 0x0f         // as in ai.cpp (sub-mode names 0x59ad94)
#define AIS_GET_CLOSE    0x16
#define AIS_GET_FAR      0x17

#define SPELL_METEOR_STORM       0x1c
#define SPELL_WALL_OF_FIRE       0x1d
#define SPELL_PUSHBACK           0x3a
#define SPELL_PYRUL_FIRE_ARROW   0x4d
#define SPELL_PYRUL_FIREBALL     0x4e
#define SPELL_PYRUL_METEOR_STORM 0x4f

// ---- .bss (address order) -----------------------------------------------------------------------------
boss_attack *pyrul_move_2_3;          // 0x247fcfc
boss_attack *pyrul_move_3_3;          // 0x247fd00
boss_attack *pyrul_move_3_2;          // 0x247fd04
boss_attack *pyrul_move_2_2;          // 0x247fd08
boss_attack *pyrul_move_1_0;          // 0x247fd0c
boss_attack *pyrul_move_2_0;          // 0x247fd10
boss_attack *pyrul_move_3_0;          // 0x247fd14
pyrul_boss pyrul;                     // 0x247fd28-0x24802cf: stage .. hits (D6701; fields in boss_ai.h)
boss_attack *pyrul_move_1_2;          // 0x24802d0
boss_attack *pyrul_move_1_1;          // 0x24802d4
boss_attack *pyrul_move_2_1;          // 0x24802d8
boss_attack *pyrul_move_3_1;          // 0x24802dc

// 0x4b87c0  Confidence: exact. static_init_pyrul_timers: "jmp 0x4b87d0" (initialiser entry 306).
// 0x4b87d0  Confidence: likely. init_pyrul_timers: the GameTimer constructor for the five threat records
// (+0x14, stride 0x18), then pyrul_move_timer, pyrul_target_timer, pyrul_timer_a and pyrul_timer_b. The
// compiler's initialiser for the definitions above; that one function constructs the threat timers and
// the four plain timers means they are defined in this order in the original (no atexit: no destructor).

// 0x4b8820  Confidence: exact. static_init_ccmd_pyrul_stage: "jmp 0x4b8830" (entry 307).
// 0x4b8830  Confidence: exact. init_ccmd_pyrul_stage (console_command constructor).
console_command ccmd_pyrul_stage("pyrul_stage", "Set Pyrul's current combat stage", cmd_pyrul_stage);   // 0x247fcf0

// 0x4b8850  Confidence: exact. No calling_a_command_function test: the parse runs for help and status
// too. Out-of-range values print the message; a missing number is ignored.
void cmd_pyrul_stage()
{
    process_command(ARG_INT);
    if (arg_type & ARG_INT) {
        if (command_argument_int >= 0 && command_argument_int <= 3)
            pyrul_target_stage = command_argument_int;
        else
            print_to_console("Invalid stage for pyrul\n", 0);
    }
}

// 0x4b8890  Confidence: exact. static_init_ccmd_pyrul_combat: "jmp 0x4b88a0" (entry 308).
// 0x4b88a0  Confidence: exact. init_ccmd_pyrul_combat (console_command constructor).
console_command ccmd_pyrul_combat("pyrul_combat", "Sets pyrul in combat state", cmd_pyrul_combat);   // 0x247fd18

// 0x4b88c0  Confidence: exact (the store is scheduled between the two pushes).
void cmd_pyrul_combat()
{
    pyrul_combat_enabled = true;
    print_to_console("Setting pyrul's ai state to combat\n", 0);
}

// 0x4b88e0  Confidence: exact. boss_target_filter_dispatch case 0xc4. Flags: 1 = stage 1, an attacking
// target whose char_info has no flag 0x10; 2 = attacking (or sub-mode 0x16) the boss; 8 = stage 2 / 3,
// more than 3 away by the edges and nearer than 15.
void pyrul_target_filter(Character *boss, boss_target *t)
{
    if (t->character == NULL)
        return;
    if (pyrul_stage == 1 && !(t->character->info->flags & 0x10) && t->character->ai.is_attacking() == true)
        t->flags |= 1;
    if (t->character->ai.is_attacking() == true || t->character->ai.is_combat_sub16()) {
        if (t->character->ai.target == boss->handle)
            t->flags |= 2;
    }
    if (pyrul_stage == 2 || pyrul_stage == 3) {
        if (boss->distance_between_edges(t->character, true, -1) > 3.0f && t->dist < 15.0f)
            t->flags |= 8;
    }
}

// 0x4b89b0  Confidence: exact. Stage 2 entry (pyrul_check_stage pushes the owner, nothing reads it).
void pyrul_timer_a_reset(Character *c)
{
    pyrul_timer_a.SetTimeout(0);
}

// 0x4b89c0  Confidence: exact. Stage 3 entry.
void pyrul_timer_b_reset(Character *c)
{
    pyrul_timer_b.SetTimeout(0);
}

// folded (0x4f13e0, empty_function)  Confidence: likely. Stage 1 entry: pyrul_boss_load and
// pyrul_check_stage push the owner and call empty_function, so the body is empty (identical-code folding).
void pyrul_stage1_reset(Character *c)
{
}

// folded (0x4f13e0, empty_function)  Confidence: likely. boss_ai_unload case 0xc4 (boss_ai.h: folded).
void pyrul_unload()
{
}

// 0x4b89d0  Confidence: exact. boss_ai_idle_dispatch case 0xc4: a finished move clears the combat state
// first; the combat flag (console, khosanilab2 script) starts the fight.
void ai_state::pyrul_ai_idle()
{
    if (owner->get_current_move() == NULL && owner->combat.state != COMBAT_IDLE) {
        owner->combat.state = COMBAT_IDLE;
        return;
    }
    if (pyrul_combat_enabled == true) {
        pyrul_cur_move = NULL;
        enter_combat();
    }
}

// 0x4b8a10  Confidence: exact (VC6 byte match, match9; was likely: the offense-point packing loop as in the other
// loaders).
void ai_state::pyrul_boss_load()
{
    int i;
    int tag;

    pyrul_stage = 1;
    pyrul_target_stage = 0;
    pyrul_stage1_reset(owner);
    pyrul_target = -1;
    for (i = 0; i < BOSS_MAX_THREATS; i++)
        pyrul_threat_table[i].handle = -1;
    pyrul_num_offense_points = 0;
    for (i = 0; i < 10; i++) {
        tag = owner->instance->character->find_mesh_tag(offense_point_name(i));
        if (tag != -1) {
            pyrul_offense_points[pyrul_num_offense_points].tag = tag;
            pyrul_num_offense_points++;
        }
    }
    pyrul_missile_bone = owner->instance->character->find_mesh_tag("$missile01");
    pyrul_24802b0 = 0;
    pyrul_24802b4 = 0;
    pyrul_timer_a.invalidate();
    pyrul_timer_b.invalidate();
    pyrul_ground_fx = -1;
    pyrul_24802ac = -1;
    boss_ai_parse_table("pyrul_boss.tbl", 3, pyrul_attack_table, pyrul_num_offense_points);
    pyrul_init_move_ptrs();
}

// 0x4b8b20  Confidence: exact (store order as in the binary).
void pyrul_init_move_ptrs()
{
    pyrul_move_1_0 = &pyrul_attack_table[0][0];
    pyrul_move_1_1 = &pyrul_attack_table[0][1];
    pyrul_move_1_2 = &pyrul_attack_table[0][2];
    pyrul_move_2_0 = &pyrul_attack_table[1][0];
    pyrul_move_2_1 = &pyrul_attack_table[1][1];
    pyrul_move_2_2 = &pyrul_attack_table[1][2];
    pyrul_move_2_3 = &pyrul_attack_table[1][3];
    pyrul_move_3_0 = &pyrul_attack_table[2][0];
    pyrul_move_3_1 = &pyrul_attack_table[2][1];
    pyrul_move_3_2 = &pyrul_attack_table[2][2];
    pyrul_move_3_3 = &pyrul_attack_table[2][3];
}

// 0x4b8b90  Confidence: exact (the forced-stage switch bodies are laid out 3, 2, 1 after the default and
// are written in that order; the HP branches share the stage store through EDI). A forced stage is
// applied once; otherwise HP / max below 0.3 is stage 3, below 0.66 stage 2. Then, at HP <= 0 while a
// cutscene runs (GF_CUTSCENE), the boss goes idle and gets CF_HIDDEN (once).
void ai_state::pyrul_check_stage()
{
    float frac;

    if (pyrul_target_stage != 0) {
        if (pyrul_stage != pyrul_target_stage) {
            switch (pyrul_target_stage) {
            default:
                pyrul_target_stage = 0;
                break;
            case 3:
                pyrul_timer_b_reset(owner);
                pyrul_stage = 3;
                break;
            case 2:
                pyrul_timer_a_reset(owner);
                pyrul_stage = 2;
                break;
            case 1:
                pyrul_stage1_reset(owner);
                pyrul_stage = 1;
                break;
            }
        }
    } else {
        frac = owner->hp / (float)owner->get_max_stat(0);
        if (frac < 0.3f) {
            if (pyrul_stage != 3) {
                pyrul_timer_b_reset(owner);
                pyrul_stage = 3;
            }
        } else if (frac < 0.66f) {
            if (pyrul_stage != 2) {
                pyrul_timer_a_reset(owner);
                pyrul_stage = 2;
            }
        } else {
            if (pyrul_stage != 1) {
                pyrul_stage1_reset(owner);
                pyrul_stage = 1;
            }
        }
    }
    if (owner->hp <= 0.0f && (game_flags & GF_CUTSCENE) && !(owner->flags & CF_HIDDEN)) {
        go_idle();
        owner->flags |= CF_HIDDEN;
    }
}

// 0x4b8ce0  Confidence: exact (the sub-mode switch: an explicit case -1 has its own table slot, 0x4b8d78
// byte 0; the bodies are laid out 15, 22/23; go_idle is the else branch, placed last and falling into the
// shared epilogue, D7700).
void ai_state::pyrul_ai_frame()
{
    int result;
    Character *target;

    result = pyrul_choose_attack();
    if (result != 0) {
        target = living_from_handle(pyrul_target);
        owner->combat.target = pyrul_target;
        if (target && (target->flags & CF_HIDDEN))
            target = NULL;
        if (result == 3)
            combat_approach(target);
        switch (submode) {
        case -1:
            return;
        case AIS_CLOSE_COMBAT:
            combat_melee_attack(target);
            return;
        case AIS_GET_CLOSE:
        case AIS_GET_FAR:
            combat_close_in(target);
            return;
        }
    } else
        go_idle();
}

// 0x4b8da0  Confidence: unsure (D15903: the two-weight arrays of stages 2 and 3 are block-scoped, which gives the binary's 0x68 frame with the array over the attackers slot; still differs in the order of the timer / retarget pick blocks; the shape of the three stage blocks: the pick / retarget code is the same
// template three times, and the compiler shares the tails; the order of the tests matches). Returns 0 (no
// target: the caller goes idle), 3 (a new move was chosen) or 5 (the current move goes on). The stage 1
// third weight (attackers * 0.1) only counts when the target has filter flag 1.
int ai_state::pyrul_choose_attack()
{
    boss_target list[BOSS_MAX_THREATS];
    float weights[3];
    Character *target;
    bool moving;
    bool busy;
    bool retarget;
    bool timer_up;
    bool flagged;
    int count;
    int attackers;
    int handle;
    int index;
    int min_ms;
    int max_ms;

    target = living_from_handle(pyrul_target);
    if (target && (target->flags & CF_HIDDEN))
        target = NULL;
    moving = owner->get_current_move() != NULL;
    busy = owner->combat.is_busy();
    if (!moving && busy) {
        owner->combat.state = COMBAT_IDLE;
        busy = false;
    }
    if (target == NULL) {
        pyrul_247fd30 = 0;
        pyrul_cur_move = NULL;
    }
    if (busy == true)
        return pyrul_cur_move ? 5 : 0;
    retarget = false;
    if (pyrul_cur_move) {
        if (pyrul_move_timer.GetElapsedTime() > 8000 && submode != AIS_CLOSE_COMBAT) {
            retarget = true;
            pyrul_cur_move = NULL;
        }
    }
    if (pyrul_cur_move)
        return 5;

    timer_up = pyrul_target_timer.elapsed();
    pyrul_update_attack_points(owner);
    count = boss_target_list_build(owner, list, pyrul_threat_table);
    attackers = count_attackers();

    if (pyrul_stage == 1) {
        bool pick = timer_up || !target;   // D22606: through a bool (the retarget block goes after the pick)
        if (pick) {
            handle = boss_target_list_pick(list, count, pyrul_threat_table, pyrul_target, false, 0);
            if (handle == -1)
                return 0;
            pyrul_target = handle;
            pyrul_timer_set_random(20000, 30000);
        } else if (retarget) {
            handle = boss_target_list_pick(list, count, pyrul_threat_table, pyrul_target, false, 0x80000000);
            if (handle == -1)
                return 0;
            pyrul_target = handle;
            pyrul_timer_set_random(20000, 30000);
        }
        flagged = false;
        index = boss_target_list_find(list, count, pyrul_target);
        if (index != -1 && (list[index].flags & 1))
            flagged = true;
        int nw = 3;                 // D22606: the weight count as an if (mov eax, 3 / jne / mov eax, 2), before the weights
        if (!flagged)
            nw = 2;
        weights[0] = 0.4f;
        weights[1] = 0.4f;
        weights[2] = attackers * 0.1f;
        switch (rand_weighted_index(weights, nw)) {
        case 2:
            pyrul_cur_move = pyrul_move_1_2;
            break;
        case 1:
            pyrul_cur_move = pyrul_move_1_1;
            break;
        case 0:
        default:
            pyrul_cur_move = pyrul_move_1_0;
            break;
        }
    } else if (pyrul_stage == 2) {
        bool pick = timer_up || !target;
        if (pick) {
            if ((attackers == 0 || frand() < 0.3f)
                && (handle = boss_target_list_pick(list, count, pyrul_threat_table, pyrul_target, false, 8)) != -1) {
                min_ms = 5000;
                max_ms = 10000;
            } else {
                handle = boss_target_list_pick(list, count, pyrul_threat_table, pyrul_target, false, 0);
                min_ms = 20000;
                max_ms = 30000;
            }
            pyrul_target = handle;
            if (handle == -1)
                return 0;
            pyrul_timer_set_random(min_ms, max_ms);
        } else if (retarget) {
            handle = boss_target_list_pick(list, count, pyrul_threat_table, pyrul_target, false, 0x80000000);
            if (handle == -1)
                return 0;
            pyrul_target = handle;
            pyrul_timer_set_random(20000, 30000);
        }
        if (pyrul_timer_a.elapsed()) {
            index = boss_target_list_find(list, count, pyrul_target);
            if (index == -1 || !(list[index].flags & 2)) {
                handle = boss_target_list_pick(list, count, pyrul_threat_table, pyrul_target, false, 2);
                if (handle != -1) {
                    pyrul_target = handle;
                    pyrul_timer_set_random(20000, 30000);
                }
            }
            pyrul_timer_a.SetTimeout(30000);
            pyrul_cur_move = pyrul_move_2_3;
        } else {
            index = boss_target_list_find(list, count, pyrul_target);
            if (index != -1 && (list[index].flags & 8)) {
                pyrul_cur_move = pyrul_move_2_2;
            } else {
                float weights2[2];
                weights2[0] = 0.4f;
                weights2[1] = 0.4f;
                index = rand_weighted_index(weights2, 2);
                if (index == -1 || index == 0)
                    pyrul_cur_move = pyrul_move_2_0;
                else
                    pyrul_cur_move = pyrul_move_2_1;
            }
        }
    } else if (pyrul_stage == 3) {
        bool pick = timer_up || !target;
        if (pick) {
            if ((attackers == 0 || frand() < 0.3f)
                && (handle = boss_target_list_pick(list, count, pyrul_threat_table, pyrul_target, false, 8)) != -1) {
                min_ms = 5000;
                max_ms = 10000;
            } else {
                handle = boss_target_list_pick(list, count, pyrul_threat_table, pyrul_target, false, 0);
                min_ms = 20000;
                max_ms = 30000;
            }
            pyrul_target = handle;
            if (handle == -1)
                return 0;
            pyrul_timer_set_random(min_ms, max_ms);
        } else if (retarget) {
            handle = boss_target_list_pick(list, count, pyrul_threat_table, pyrul_target, false, 0x80000000);
            if (handle == -1)
                return 0;
            pyrul_target = handle;
            pyrul_timer_set_random(20000, 30000);
        }
        if (pyrul_timer_b.elapsed()) {
            index = boss_target_list_find(list, count, pyrul_target);
            if (index == -1 || !(list[index].flags & 2)) {
                handle = boss_target_list_pick(list, count, pyrul_threat_table, pyrul_target, false, 2);
                if (handle != -1) {
                    pyrul_target = handle;
                    pyrul_timer_set_random(20000, 30000);
                }
            }
            pyrul_timer_b.SetTimeout(30000);
            pyrul_cur_move = pyrul_move_3_3;
        } else {
            index = boss_target_list_find(list, count, pyrul_target);
            if (index != -1 && (list[index].flags & 8)) {
                // At most three fireballs / walls of fire at a time, then a near target is picked.
                if (fx_count_spells_1a_1d_4e() < 3) {
                    pyrul_cur_move = pyrul_move_3_2;
                    pyrul_move_timer.SetTimeout(0);
                    return 3;
                }
                handle = boss_target_list_pick(list, count, pyrul_threat_table, pyrul_target, false, 2);
                if (handle != -1) {
                    pyrul_target = handle;
                    pyrul_timer_set_random(20000, 30000);
                }
            }
            float weights2[2];
            weights2[0] = 0.5f;
            weights2[1] = 0.5f;
            index = rand_weighted_index(weights2, 2);
            if (index == -1 || index == 0)
                pyrul_cur_move = pyrul_move_3_0;
            else
                pyrul_cur_move = pyrul_move_3_1;
        }
    } else {
        return 0;
    }
    pyrul_move_timer.SetTimeout(0);
    return 3;
}

// 0x4b93d0  Confidence: exact (max - min is stored into max's slot for fimul).
void pyrul_timer_set_random(int min_ms, int max_ms)
{
    pyrul_target_timer.SetTimeout(min_ms + (int)(frand() * (max_ms - min_ms)));
}

// 0x4b9400  Confidence: exact (VC6 byte match, match9; was likely: the position sum goes through a temporary).
void pyrul_update_attack_points(Character *c)
{
    int i;
    matrix orient;
    vector3 pos;
    boss_offense_point *pt;

    for (i = 0; i < pyrul_num_offense_points; i++) {
        pt = &pyrul_offense_points[i];
        c->instance->get_bone_world_transform(pt->tag, &c->orient, &c->pos, &orient, &pos);
        pt->dir = orient.uvec;
        pt->pos = pos + c->centre_offset;
    }
}

// 0x4b94a0  Confidence: exact (VC6 byte match, m-1019 w3; was likely; switch on the move's spell; case -1 has its own slot in the byte table
// 0x4b9598; the bodies are laid out 29/58, 78, 77, 79). Pyrul fire arrow / fireball / meteor storm are
// cast as goals in multiplayer and directly otherwise; wall of fire (0x1d) and pushback (0x3a) report a
// special move without a cast.
bool pyrul_cast_special(Character *c, int anim)
{
    if (pyrul_cur_move == NULL)
        return false;
    switch (pyrul_cur_move->spell) {
    case -1:
        return false;
    case SPELL_WALL_OF_FIRE:
    case SPELL_PUSHBACK:
        break;
    case SPELL_PYRUL_FIREBALL:
        if (game_flags & GF_MULTIPLAYER) {
            c->ai.add_cast_at_target(SPELL_PYRUL_FIREBALL, c->combat.opponent->owner, 999999);
            return true;
        }
        c->perform_spell_cast(SPELL_PYRUL_FIREBALL, false);
        return true;
    case SPELL_PYRUL_FIRE_ARROW:
        if (game_flags & GF_MULTIPLAYER) {
            c->ai.add_cast_at_target(SPELL_PYRUL_FIRE_ARROW, c->combat.opponent->owner, 999999);
            return true;
        }
        c->perform_spell_cast(SPELL_PYRUL_FIRE_ARROW, false);
        return true;
    case SPELL_PYRUL_METEOR_STORM:
        if (game_flags & GF_MULTIPLAYER) {
            c->ai.add_cast_at_target(SPELL_PYRUL_METEOR_STORM, c->combat.opponent->owner, 999999);
            return true;
        }
        c->perform_spell_cast(SPELL_PYRUL_METEOR_STORM, false);
        return true;
    default:
        return false;
    }
    return true;
}

// 0x4b95f0  Confidence: exact. Without the "$missile01" tag the boss's own frame is copied, but the bone
// call follows anyway with tag -1 (no else; NOTES.md, game bugs).
void pyrul_get_missile_origin(Character *c, matrix *orient, vector3 *pos)
{
    if (pyrul_missile_bone == -1) {
        *orient = c->orient;
        *pos = c->pos;
    }
    c->instance->get_bone_world_transform(pyrul_missile_bone, &c->orient, &c->pos, orient, pos);
}

// 0x4b9670  Confidence: likely (switch on the move's spell, base 0x1d; bodies laid out 0x3a, 0x4d/0x4e,
// 0x1d, 0x4f). Wall of fire: a point 7 ahead of the boss's centre towards the target's centre; single
// player places the ground effect facing across that line, multiplayer casts wall of fire there and then
// falls through into the meteor storm case (no break; NOTES.md, game bugs).
void pyrul_fire_attack(Character *boss, Character *target)
{
    matrix orient;
    vector3 start;
    vector3 dir;
    vector3 offset;
    vector3 pos;
    effect_object *fx;
    int missile;

    if (pyrul_cur_move == NULL)
        return;
    switch (pyrul_cur_move->spell) {
    case SPELL_PUSHBACK:
        target->ai.disable_2f();
        break;
    case SPELL_PYRUL_FIRE_ARROW:
    case SPELL_PYRUL_FIREBALL:
        if (game_flags & GF_MULTIPLAYER) {
            spell_cast_execute(boss, pyrul_cur_move->spell, target->handle, NULL, -1, 0, 0, 0);
            break;
        }
        pyrul_get_missile_origin(boss, &orient, &start);
        missile = -1;
        for (fx = fx_active_list.fx_next; fx != &fx_active_list; fx = fx->fx_next) {
            if (fx->owner == boss->handle && fx->source == boss->cur_anim) {
                missile = fx->handle;
                break;
            }
        }
        spell_fire_missile(pyrul_cur_move->spell, missile, boss, target, &start, &orient);
        break;
    case SPELL_WALL_OF_FIRE: {
        vector3 from_centre(boss->pos.x + boss->centre_offset.x, boss->pos.y + boss->centre_offset.y,
                            boss->pos.z + boss->centre_offset.z);
        vector3 to_centre(target->pos.x + target->centre_offset.x, target->pos.y + target->centre_offset.y,
                          target->pos.z + target->centre_offset.z);
        dir.x = to_centre.x - from_centre.x;
        dir.y = to_centre.y - from_centre.y;
        dir.z = to_centre.z - from_centre.z;
        dir.normalize_safe();
        offset.x = dir.x * 7.0f;
        offset.y = dir.y * 7.0f;
        offset.z = dir.z * 7.0f;
        vector3 base(boss->pos.x + boss->centre_offset.x, boss->pos.y + boss->centre_offset.y,
                     boss->pos.z + boss->centre_offset.z);
        pos.x = base.x + offset.x;
        pos.y = base.y + offset.y;
        pos.z = base.z + offset.z;
        vector3 spot(pos);            // a dead copy (0x4b98b0)
        if (!(game_flags & GF_MULTIPLAYER)) {
            dir = vector3(dir.y * boss->orient.uvec.z - dir.z * boss->orient.uvec.y,
                          dir.z * boss->orient.uvec.x - dir.x * boss->orient.uvec.z,
                          dir.x * boss->orient.uvec.y - dir.y * boss->orient.uvec.x);
            dir.normalize();
            orient.set_from_forward_up(dir, boss->orient.uvec);
            pyrul_ground_fx = spell_place_ground_effect(pyrul_cur_move->spell, boss->handle, &pos, boss->layer, &orient);
            break;
        }
        spell_cast_execute(boss, SPELL_WALL_OF_FIRE, -1, &pos, boss->layer, 0, 0, 0);
    }
        // fall through
    case SPELL_PYRUL_METEOR_STORM:
        spell_cast_execute(boss, SPELL_METEOR_STORM, -1, NULL, boss->layer, 0, 0, 0);
        break;
    }
}

// 0x4b9a00  Confidence: exact. Reach of the current move (boss_get_move_range case 0xc4).
float pyrul_move_range(Character *c)
{
    int kind = pyrul_is_own_move(c);

    if (kind == 2)
        return 3.5f;
    if (kind == 6)
        return 2.5f;
    if (kind == 3 || kind == 5 || kind == 4)
        return 9.0f;
    return c->info->attack_radius;
}

// 0x4b9a50  Confidence: exact. The class of the current move: 0 the first move of each stage, 2 the second,
// 1 / 3 / 5 the third of stages 1 / 2 / 3, 4 / 6 the stage 2 / 3 specials, -1 none (the argument is not read).
int pyrul_is_own_move(Character *c)
{
    if (pyrul_cur_move == NULL)
        return -1;
    if (pyrul_cur_move == pyrul_move_1_0 || pyrul_cur_move == pyrul_move_2_0 || pyrul_cur_move == pyrul_move_3_0)
        return 0;
    if (pyrul_cur_move == pyrul_move_1_1 || pyrul_cur_move == pyrul_move_2_1 || pyrul_cur_move == pyrul_move_3_1)
        return 2;
    if (pyrul_cur_move == pyrul_move_1_2)
        return 1;
    if (pyrul_cur_move == pyrul_move_2_2)
        return 3;
    if (pyrul_cur_move == pyrul_move_3_2)
        return 5;
    if (pyrul_cur_move == pyrul_move_2_3)
        return 4;
    return pyrul_cur_move == pyrul_move_3_3 ? 6 : -1;
}

// 0x4b9ae0  Confidence: exact. Collects the party members in the current move's cone whose defence roll
// fails. Single player walks cur_character's AI group, multiplayer the party list.
bool pyrul_attack_test_class0(Character *c)
{
    ai_group *group;
    Player *p;
    Character *t;
    int i;

    pyrul_num_hits = 0;
    if (game_flags & GF_SINGLE_PLAYER) {
        group = cur_character->ai.group;
        if (group) {
            for (i = 0; i < group->count; i++) {
                t = living_from_handle(group->members[i]);
                if (t && pyrul_point_in_attack_cone(t, pyrul_cur_move)
                    && living_combat_test_defense(&c->combat, &t->combat))
                    pyrul_hits[pyrul_num_hits++] = group->members[i];
            }
        }
    } else {
        for (p = party_list.next; p != &party_list; p = p->next) {
            t = living_from_handle(p->handle);
            if (t && pyrul_point_in_attack_cone(t, pyrul_cur_move)
                && living_combat_test_defense(&c->combat, &t->combat))
                pyrul_hits[pyrul_num_hits++] = p->handle;
        }
    }
    return pyrul_num_hits > 0;
}

// 0x4b9c20  Confidence: exact (VC6 byte match, match9; was likely: the dot product is summed z, y, x). Horizontal
// distance from the move's
// offense point within range_max and the direction within the cone. The point index is not checked
// (every retail move has -1; bosses.md "offense points").
bool pyrul_point_in_attack_cone(Character *c, boss_attack *move)
{
    boss_offense_point *pt = &pyrul_offense_points[move->offense_point];
    vector3 d(c->pos.x - pt->pos.x, 0.0f, c->pos.z - pt->pos.z);

    if (d.normalize_safe_length() > move->range_max)
        return false;
    if (d.x * pt->dir.x + d.y * pt->dir.y + d.z * pt->dir.z < move->cos_max_angle)
        return false;
    return true;
}

// 0x4b9cc0  Confidence: exact. Byte for byte the body of pyrul_attack_test_class0 (not folded with it).
bool pyrul_attack_test_class2(Character *c)
{
    ai_group *group;
    Player *p;
    Character *t;
    int i;

    pyrul_num_hits = 0;
    if (game_flags & GF_SINGLE_PLAYER) {
        group = cur_character->ai.group;
        if (group) {
            for (i = 0; i < group->count; i++) {
                t = living_from_handle(group->members[i]);
                if (t && pyrul_point_in_attack_cone(t, pyrul_cur_move)
                    && living_combat_test_defense(&c->combat, &t->combat))
                    pyrul_hits[pyrul_num_hits++] = group->members[i];
            }
        }
    } else {
        for (p = party_list.next; p != &party_list; p = p->next) {
            t = living_from_handle(p->handle);
            if (t && pyrul_point_in_attack_cone(t, pyrul_cur_move)
                && living_combat_test_defense(&c->combat, &t->combat))
                pyrul_hits[pyrul_num_hits++] = p->handle;
        }
    }
    return pyrul_num_hits > 0;
}

// 0x4b9e00  Confidence: exact.
void pyrul_resolve_hit(Character *c)
{
    int i;
    Character *t;

    for (i = 0; i < pyrul_num_hits; i++) {
        t = living_from_handle(pyrul_hits[i]);
        if (t)
            living_combat_resolve_hit_vs(&c->combat, &t->combat);
    }
}

// 0x4b9e50  Confidence: exact.
int random_stun_duration()
{
    return rand_int_range(3000, 7000);
}
