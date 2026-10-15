// unnamed_giant_salamanka_0x4c8030.cpp - Sum.exe v1.40, code 0x4c8030-0x4c8d03: the Giant Salamanka boss of
// the Khosani lab (species 0xac). File name NOT known: a placeholder (src/decomp/README.md). No assert path,
// no strings.tbl block; cut by its static initialiser run (entries 333-335, 0x588538-0x588540) and its data.
// Written from the Sum.exe disassembly (tools/asm.mjs; jump tables decoded from the raw bytes).
// Target: VC++ 6.0 (cl 12.00.8966), /O2 like the other boss units. Confidence per function: exact =
// expect identical code, likely = same semantics, small layout risk, unsure = the source shape is a guess.
//
// Unit range: static_init_giant_salamanka_timers (0x4c8030) to giant_salamanka_stage_attack_enabled
// (0x4c8cc0-0x4c8d03). The Luminar unit opens with its initialiser 0x4c8d10 (entry 336).
// Data: .data 0x59debc-0x59dfcd (giant_salamanka_bitmap = -1, the command names / help texts and message,
// "$boss1".."$boss3", "walk2", "walk3", the flinch error text, "front", "right", "left", "gsal_boss.tbl";
// "walk", "stage", "_flinch_", "area", "status" and "$missile" are pooled with other units' strings);
// .bss 0x248c7b0-0x248cdaf (ccmd_gsal_combat, ccmd_gs_stage, then the state object gsal 0x248c7d0, D3904).
// Return codes of giant_salamanka_ai_select_action: 2 a spell move, 3 a melee move, 4 nothing, 5 busy.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "engine/ai/boss_ai.h"
#include "engine/ai/unnamed_giant_salamanka_0x4c8030.h"
#include "engine/ai/ai.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "engine/effects/fx.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/script_internal.h"
#include "vsdk/os/console.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/ca/character.h"
#include "summoner/summoner.h"

#define AIS_CLOSE_COMBAT  0x0f        // as in ai.cpp (sub-mode names 0x59ad94)
#define AIS_GET_CLOSE     0x16

#define SPELL_GSAL_TIMED  0x38        // the spell move that waits for attack_timer (5 s)

// ---- .data --------------------------------------------------------------------------------------------
int giant_salamanka_bitmap = -1;      // 0x59debc

// ---- .bss (initialiser order; the addresses run the other way) ------------------------------------------
// 0x4c8030  Confidence: exact. static_init_giant_salamanka_timers: "jmp 0x4c8040" (initialiser entry 333).
// 0x4c8040  Confidence: likely. init_giant_salamanka_timers: the compiler's constructor of gsal (the five
// threat timers, then attack_timer). The initialisers run in definition order (333 gsal, 334 gs_stage,
// 335 gsal_combat) while the objects lie in the opposite address order, as Titus's two commands do.
giant_salamanka_boss gsal;            // 0x248c7d0
// 0x4c8070  Confidence: exact. static_init_ccmd_gs_stage: "jmp 0x4c8080" (entry 334).
// 0x4c8080  Confidence: exact. init_ccmd_gs_stage (console_command constructor).
console_command ccmd_gs_stage("gs_stage", "Set Giant salamanka's current combat stage", cmd_gs_stage);   // 0x248c7c0
// 0x4c80d0  Confidence: exact. static_init_ccmd_gsal_combat: "jmp 0x4c80e0" (entry 335).
// 0x4c80e0  Confidence: exact. init_ccmd_gsal_combat (console_command constructor).
console_command ccmd_gsal_combat("gsal_combat", "Sets gsal in combat state", cmd_gsal_combat);   // 0x248c7b0

// 0x4c80a0  Confidence: exact. 1-3 only (no message, and no way back to 0 = by HP).
void cmd_gs_stage()
{
    process_command(ARG_INT);
    if ((arg_type & ARG_INT) && command_argument_int > 0 && command_argument_int < 4)
        giant_salamanka_forced_stage = command_argument_int;
}

// 0x4c8100  Confidence: exact.
void cmd_gsal_combat()
{
    giant_salamanka_combat_enabled = true;
    print_to_console("Setting gsal's ai state to combat\n", 0);
}

// 0x4c8120  Confidence: exact.
void character_set_locomotion_anims(Character *c, int anim)
{
    c->anim_stand = anim;
    c->anim_norm_walk = anim;
    c->anim_ready = anim;
    c->anim_walk = anim;
    c->anim_stand_hurt = anim;
    c->anim_norm_walk_hurt = anim;
    c->anim_ready_hurt = anim;
    c->anim_walk_hurt = anim;
}

// 0x4c8160  Confidence: exact. Stage 1: "walk" and the "$boss1" navpoint (position and heading).
void giant_salamanka_place_boss1(Character *c)
{
    navpoint *nav;

    character_set_locomotion_anims(c, charinfo_find_anim(c->info, "walk"));
    nav = navpoint_find("$boss1");
    if (nav) {
        c->pos = nav->pos;
        c->orient.set_from_angles(0.0f, 0.0f, nav->heading);
    }
}

// 0x4c81c0  Confidence: exact.
void giant_salamanka_place_boss2(Character *c)
{
    navpoint *nav;

    character_set_locomotion_anims(c, charinfo_find_anim(c->info, "walk2"));
    nav = navpoint_find("$boss2");
    if (nav) {
        c->pos = nav->pos;
        c->orient.set_from_angles(0.0f, 0.0f, nav->heading);
    }
}

// 0x4c8220  Confidence: exact.
void giant_salamanka_place_boss3(Character *c)
{
    navpoint *nav;

    character_set_locomotion_anims(c, charinfo_find_anim(c->info, "walk3"));
    nav = navpoint_find("$boss3");
    if (nav) {
        c->pos = nav->pos;
        c->orient.set_from_angles(0.0f, 0.0f, nav->heading);
    }
}

// 0x4c8280  Confidence: likely (the stage digit goes through a two-byte local for atoi). For each move
// "stage<N>_flinch_<kind>": kind area / status / left / right / front fills the table of stage N. The
// action is not checked to start with "stage"; a bad kind is formatted into a local buffer that is never
// shown (NOTES, game bugs).
void giant_salamanka_set_flinch(move_info *moves)
{
    char msg[128];
    char digit[2];
    const char *p;
    move_info *m;
    int stage;

    for (m = moves; m; m = m->next) {
        if (m->action == NULL)
            continue;
        p = m->action + strlen("stage");
        digit[0] = *p;
        digit[1] = 0;
        stage = atoi(digit) - 1;
        p += strlen("_flinch_") + 1;
        if (!_stricmp(p, "area"))
            gsal.flinch_area[stage] = m;
        else if (!_stricmp(p, "status"))
            gsal.flinch_status[stage] = m;
        else if (!_stricmp(p, "left"))
            gsal.flinch_left[stage] = m;
        else if (!_stricmp(p, "right"))
            gsal.flinch_right[stage] = m;
        else if (!_stricmp(p, "front"))
            gsal.flinch_front[stage] = m;
        else
            sprintf(msg, "Unable to set Giant Salamanka flinch for %s -- bad $Action\n", m->anim_file);
    }
}

// 0x4c83a0  Confidence: likely (the byte stores after the memset are in the binary too). Three stages of
// moves; each stage enables some of its eight moves (stage 1: 0-4, 6; stage 2: 0, 2, 3, 5, 7; stage 3: 0,
// 2, 5, 6, 7) for boss_stage_attack_enabled_dispatch.
void ai_state::giant_salamanka_boss_load()
{
    int i;
    int tag;

    memset((void *)&gsal, 0, sizeof(gsal));
    giant_salamanka_combat_enabled = false;
    giant_salamanka_stage2_reached = false;
    giant_salamanka_stage2_ack = false;
    giant_salamanka_stage3_reached = false;
    giant_salamanka_stage3_ack = false;
    giant_salamanka_stage = 1;
    giant_salamanka_forced_stage = 0;
    giant_salamanka_attack_timer.SetTimeout(0);
    giant_salamanka_place_boss1(owner);
    for (i = 0; i < BOSS_MAX_THREATS; i++)
        giant_salamanka_threat_table[i].handle = -1;
    giant_salamanka_offense_point_count = 0;
    for (i = 0; i < 10; i++) {
        tag = owner->instance->character->find_mesh_tag(offense_point_name(i));
        if (tag != -1) {
            gsal.offense_points[giant_salamanka_offense_point_count].tag = tag;
            giant_salamanka_offense_point_count++;
        }
    }
    boss_ai_parse_table("gsal_boss.tbl", 3, gsal.moves, giant_salamanka_offense_point_count);
    gsal.stage_move_id[0] = 0xa7;
    gsal.stage_move_id[1] = 0xac;
    gsal.stage_move_id[2] = 0xaf;
    giant_salamanka_missile_tag = owner->instance->character->find_mesh_tag("$missile");
    giant_salamanka_set_flinch((move_info *)owner->info->moves[4]);
    giant_salamanka_stage1_attacks[6] = true;
    giant_salamanka_stage1_attacks[4] = true;
    giant_salamanka_stage1_attacks[3] = true;
    giant_salamanka_stage1_attacks[2] = true;
    giant_salamanka_stage1_attacks[1] = true;
    giant_salamanka_stage1_attacks[0] = true;
    giant_salamanka_stage2_attacks[7] = true;
    giant_salamanka_stage2_attacks[5] = true;
    giant_salamanka_stage2_attacks[3] = true;
    giant_salamanka_stage2_attacks[2] = true;
    giant_salamanka_stage2_attacks[0] = true;
    giant_salamanka_stage3_attacks[7] = true;
    giant_salamanka_stage3_attacks[6] = true;
    giant_salamanka_stage3_attacks[5] = true;
    giant_salamanka_stage3_attacks[2] = true;
    giant_salamanka_stage3_attacks[0] = true;
    owner->flags |= CF_COMBAT_READY;
}

// 0x4c8540  Confidence: exact. boss_ai_unload case 0xac (nothing loads the bitmap).
void giant_salamanka_free_bitmap()
{
    if (giant_salamanka_bitmap >= 0) {
        bm_release(giant_salamanka_bitmap);
        giant_salamanka_bitmap = -1;
    }
}

// 0x4c8560  Confidence: likely (the forced-stage switch bodies are laid out 1, 3, 2 and shared with the HP
// branches). The HP thresholds 2/3 and 1/3 only arm the "reached" byte; the stage changes when the level
// script has acknowledged it (the Trans cutscenes), and never skips one. A forced stage outside 1-3 would
// set the stage to 0 (cmd_gs_stage cannot store one). At HP <= 0 in a cutscene: idle and CF_HIDDEN.
void ai_state::giant_salamanka_check_stage()
{
    float frac;

    if (giant_salamanka_forced_stage != 0) {
        if (giant_salamanka_stage != giant_salamanka_forced_stage) {
            switch (giant_salamanka_forced_stage) {
            case 1:
                giant_salamanka_stage = 1;
                giant_salamanka_place_boss1(owner);
                break;
            case 2:
                giant_salamanka_stage = 2;
                giant_salamanka_place_boss2(owner);
                break;
            case 3:
                giant_salamanka_stage = 3;
                giant_salamanka_place_boss3(owner);
                break;
            default:
                giant_salamanka_stage = 0;
                break;
            }
        }
    } else {
        frac = owner->hp / (float)owner->get_max_stat(0);
        if (frac < 0.3333f) {
            if (giant_salamanka_stage == 2) {
                if (giant_salamanka_stage3_reached == false) {
                    giant_salamanka_stage3_reached = true;
                    giant_salamanka_stage3_ack = false;
                } else if (giant_salamanka_stage3_ack == true) {
                    giant_salamanka_stage = 3;
                    giant_salamanka_place_boss3(owner);
                }
            }
        } else if (frac < 0.6667f) {
            if (giant_salamanka_stage == 1) {
                if (giant_salamanka_stage2_reached == false) {
                    giant_salamanka_stage2_reached = true;
                    giant_salamanka_stage2_ack = false;
                } else if (giant_salamanka_stage2_ack == true) {
                    giant_salamanka_stage = 2;
                    giant_salamanka_place_boss2(owner);
                }
            }
        }
    }
    if (owner->hp <= 0.0f && (game_flags & GF_CUTSCENE) && !(owner->flags & CF_HIDDEN)) {
        go_idle();
        owner->flags |= CF_HIDDEN;
    }
}

// folded (0x4f13e0, empty_function)  Confidence: likely. boss_ai_idle_dispatch case 0xac (ai.h: folded).
void ai_state::giant_salamanka_ai_idle()
{
}

// 0x4c86b0  Confidence: likely (the result switch is a table of 5, 0x4c87c4). Gated by the combat flag. The
// move id is the stage's default unless a move was chosen (then its variation); the attack runs on the AI
// record's own target.
void ai_state::giant_salamanka_ai_frame()
{
    Character *t;
    int move_id;

    if (!giant_salamanka_combat_enabled)
        return;
    t = living_from_handle(target);
    if (t && (t->flags & CF_HIDDEN))
        t = NULL;
    move_id = gsal.stage_move_id[giant_salamanka_stage - 1];
    switch (giant_salamanka_ai_select_action()) {
    case 0:
        go_idle();
        break;
    case 1:
        break;
    case 2:
    case 3:
        if (giant_salamanka_target_handle != target) {
            target = giant_salamanka_target_handle;
            owner->combat.target = giant_salamanka_target_handle;
            t = living_from_handle(target);
        }
        move_id = giant_salamanka_current_move->variation;
        break;
    case 4:
        return;
    }
    switch (submode) {
    case AIS_CLOSE_COMBAT:
        attack_range = owner->info->attack_radius;
        if (t == NULL) {
            go_idle();
            return;
        }
        ai_move_flags |= 4;
        owner->combat.update(move_id);
        t->ai.claim_attack_slot(t, owner, 0, -1);
        claim_attack_slot(owner, t, 0, -1);
        t->ai.alert_guards_of_target(owner);
        break;
    case AIS_GET_CLOSE:
        set_mode(AI_STATE_COMBAT, AIS_CLOSE_COMBAT);
        break;
    }
}

// 0x4c87e0  Confidence: likely (the offense points are refreshed inline; the move list reuses the
// candidate array in the binary). Candidates: the living threat records scored 5 - (3 * threat[0] +
// threat[1]), then party-type entities scored 5 (at most five); a weighted pick. Then one of the stage's
// enabled moves whose offense point sees the target's centre inside its cone and range (spell 0x38 only
// when attack_timer has run out), at random.
int ai_state::giant_salamanka_ai_select_action()
{
    struct { int handle; float score; } cand[5];
    float cum[5];
    int moves[10];
    matrix orient;
    vector3 pos;
    boss_offense_point *pt;
    boss_attack *m;
    Character *c;
    Character *t;
    float sum;
    float r;
    float dx;
    float dz;
    float dist;
    int count = 0;
    int num_moves;
    int handle;
    int pick;
    int i;
    int j;

    if (owner->combat.is_busy() == true)
        return 5;
    for (i = 0; i < BOSS_MAX_THREATS; i++) {
        if (giant_salamanka_threat_table[i].handle == -1)
            continue;
        if (living_from_handle(giant_salamanka_threat_table[i].handle)->flags & CF_DEAD)
            continue;
        cand[count].handle = giant_salamanka_threat_table[i].handle;
        cand[count].score = 5.0f - (giant_salamanka_threat_table[i].threat[0] * 3.0f
                                    + giant_salamanka_threat_table[i].threat[1]);
        count++;
    }
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->type != 7)
            continue;
        for (j = 0; j < count; j++) {
            if (cand[j].handle == c->handle)
                break;
        }
        if (j < count)
            continue;
        cand[count].handle = c->handle;
        cand[count].score = 5.0f;
        count++;
        if (count >= 5)
            break;
    }
    if (count == 0)
        goto none;
    sum = 0.0f;
    for (i = 0; i < count; i++) {
        sum += cand[i].score;
        cum[i] = sum;
    }
    for (i = 0; i < count; i++)
        cum[i] /= sum;
    r = frand();
    for (i = 0; i < count; i++) {
        if (r <= cum[i])
            break;
    }
    pick = (i < count && i != -1) ? i : 0;
    for (i = 0; i < giant_salamanka_offense_point_count; i++) {
        pt = &gsal.offense_points[i];
        owner->instance->get_bone_world_transform(pt->tag, &owner->orient, &owner->pos, &orient, &pos);
        pt->dir = orient.uvec;
        pt->pos.x = pos.x + owner->centre_offset.x;
        pt->pos.y = pos.y + owner->centre_offset.y;
        pt->pos.z = pos.z + owner->centre_offset.z;
    }
    handle = cand[pick].handle;
    t = living_from_handle(handle);
    if (t == NULL) {
        giant_salamanka_current_move = NULL;
        return 4;
    }
    num_moves = 0;
    for (i = 0; i < BOSS_MAX_OFFENSE_POINTS; i++) {
        m = &gsal.moves[giant_salamanka_stage - 1][i];
        if (!m->enabled)
            continue;
        if (m->spell == SPELL_GSAL_TIMED && !giant_salamanka_attack_timer.elapsed())
            continue;
        pt = &gsal.offense_points[m->offense_point];
        dx = t->pos.x + t->centre_offset.x - pt->pos.x;
        dz = t->centre_offset.z + t->pos.z - pt->pos.z;
        dist = (float)sqrt(dz * dz + dx * dx);
        if ((dz * pt->dir.z + dx * pt->dir.x) / dist > m->cos_max_angle && dist < m->range_max
            && dist > m->range_min)
            moves[num_moves++] = i;
    }
    if (num_moves <= 0)
        goto none;
    if (num_moves == 1)
        pick = moves[0];
    else
        pick = moves[rand() % num_moves];
    if (pick < 0)
        goto none;
    giant_salamanka_target_handle = handle;
    giant_salamanka_current_move = &gsal.moves[giant_salamanka_stage - 1][pick];
    return (giant_salamanka_current_move->spell == -1) + 2;
none:
    giant_salamanka_current_move = NULL;
    return 4;
}

// 0x4c8b30  Confidence: exact. character_perform_spell_cast's missile origin for the boss.
void giant_salamanka_get_missile_bone(Character *c, matrix *orient, vector3 *pos)
{
    c->instance->get_bone_world_transform(giant_salamanka_missile_tag, &c->orient, &c->pos, orient, pos);
}

// 0x4c8b60  Confidence: exact. boss_cast_special_dispatch case 0xac (the arguments are not read).
bool giant_salamanka_move_has_spell(int handle, int anim)
{
    if (giant_salamanka_current_move && giant_salamanka_current_move->spell != -1)
        return true;
    return false;
}

// 0x4c8b80  Confidence: exact. boss_attack_trigger_dispatch case 0xac: the move's spell as a missile from
// the "$missile" bone; spell 0x38 then waits 5 s.
void giant_salamanka_fire_attack(Character *boss, Character *target)
{
    matrix orient;
    vector3 pos;

    if (giant_salamanka_current_move == NULL || giant_salamanka_current_move->spell == -1)
        return;
    giant_salamanka_get_missile_bone(boss, &orient, &pos);
    fx_create_missile(giant_salamanka_current_move->spell, &pos, &orient, boss->handle, target->handle);
    if (giant_salamanka_current_move->spell == SPELL_GSAL_TIMED)
        giant_salamanka_attack_timer.SetTimeout(5000);
}

// 0x4c8c00  Confidence: exact. boss_attack_stuns_dispatch case 0xac: +Push: (flag 2).
bool giant_salamanka_move_pushes()
{
    if (giant_salamanka_current_move && (giant_salamanka_current_move->flags & BOSS_ATTACK_PUSH))
        return true;
    return false;
}

// 0x4c8c20  Confidence: exact. boss_attack_knocks_down_dispatch case 0xac: +Trip: (flag 1).
bool giant_salamanka_move_knocks_down()
{
    if (giant_salamanka_current_move && (giant_salamanka_current_move->flags & BOSS_ATTACK_TRIP))
        return true;
    return false;
}

// 0x4c8c40  Confidence: likely (an int return: mov eax, 1 / xor eax, eax; D3905). The current move's range +
// 1 from its offense point (horizontal), or without a move the char_info attack radius by the edges.
int giant_salamanka_target_in_reach(Character *boss, Character *target)
{
    boss_offense_point *pt;
    float reach;
    float dist;

    if (giant_salamanka_current_move) {
        reach = giant_salamanka_current_move->range_max + 1.0f;
        pt = &gsal.offense_points[giant_salamanka_current_move->offense_point];
        float dx = target->pos.x - pt->pos.x;
        float dz = target->pos.z - pt->pos.z;
        dist = (float)sqrt(dz * dz + dx * dx);
    } else {
        reach = boss->info->attack_radius;
        dist = boss->distance_between_edges(target, true, -1);
    }
    if (reach > dist)
        return 1;
    return 0;
}

// 0x4c8cc0  Confidence: exact. boss_stage_attack_enabled_dispatch case 0xac.
bool giant_salamanka_stage_attack_enabled(int index)
{
    switch (giant_salamanka_stage) {
    case 1:
        return giant_salamanka_stage1_attacks[index] == true;
    case 2:
        return giant_salamanka_stage2_attacks[index] == true;
    case 3:
        return giant_salamanka_stage3_attacks[index] == true;
    }
    return false;
}
