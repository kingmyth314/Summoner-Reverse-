// unnamed_machival_0x4c0c40.cpp - Sum.exe v1.40, code 0x4c0c40-0x4c20d8: the Machival boss (species 0xc5).
// File name NOT known: a placeholder (src/decomp/README.md). No assert path, no strings.tbl block; cut by
// its static initialiser run (entries 318-319, 0x5884fc-0x588500) and its data.
// Written from the Sum.exe disassembly (tools/asm.mjs; jump tables decoded from the raw bytes).
// Target: VC++ 6.0 (cl 12.00.8966), /O2 like the other boss units. Confidence per function: exact =
// expect identical code, likely = same semantics, small layout risk, unsure = the source shape is a guess.
//
// Unit range: static_init_machival_timers (0x4c0c40) to machival_ai_wait_move_end (0x4c2090-0x4c20d8). The
// Titus unit opens with its initialiser 0x4c20e0 (entry 320).
// Data: .data 0x59db8c-0x59dc01 (the command name and help, the combat message, "$tele_0%d",
// "machival_boss.tbl"); .bss 0x248b010-0x248b37b (ccmd_machival_combat first, machival_move_melee last;
// Titus's move pointers start at 0x248b380). Floats are pooled .rdata constants.
// Return codes of machival_ai_select_action: 0 no target (go idle), 1 a spell was cast, 3 a new move,
// 4 nothing to do this think, 5 busy / keep the current move.

#include <stdio.h>
#include <stdlib.h>
#include "engine/ai/boss_ai.h"
#include "engine/ai/ai.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "engine/spells/spells.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/script_internal.h"
#include "vsdk/os/console.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/ca/character.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"

#define AIS_CLOSE_COMBAT     0x0f     // as in ai.cpp (sub-mode names 0x59ad94)
#define AIS_GET_CLOSE        0x16
#define AIS_GET_FAR          0x17
#define AIS_MACHIVAL_TELEPORT 0x33    // teleport out (names ours)
#define AIS_MACHIVAL_ARRIVE  0x34     // teleport in

#define SPELL_MACHIVAL_PARTY 0x5b     // cast at a random party member when nobody is in reach
#define SPELL_MACHIVAL_AT_SELF 0x64   // at the point one unit in front of the boss
#define SPELL_MACHIVAL_TARGET 0x65
#define SPELL_MACHIVAL_AT_FEET 0x66
#define SPELL_KNOCKDOWN_MOVE 0x3b     // the move spell machival_knock_down_party reacts to

// ---- .bss (address order) -----------------------------------------------------------------------------
// ccmd_machival_combat 0x248b010 is defined with its initialiser below.
boss_attack *machival_move_3;         // 0x248b01c &machival_attack_table[0][3] (the far reach move)
boss_attack *machival_move_5;         // 0x248b020 [0][5]
boss_attack *machival_move_4;         // 0x248b024 [0][4]
boss_attack *machival_move_2;         // 0x248b028 [0][2]
boss_attack *machival_move_1;         // 0x248b02c [0][1]
machival_boss machival;               // 0x248b030-0x248b377 (D6510: the fields' old names are macros in boss_ai.h)
boss_attack *machival_move_melee;     // 0x248b378 [0][0]

// 0x4c0c40  Confidence: exact. static_init_machival_timers: "jmp 0x4c0c50" (initialiser entry 318).
// 0x4c0c50  Confidence: likely. init_machival_timers: the GameTimer constructor for the five threat records,
// machival_248b310, the attack, teleport and three spell timers and the party spell timer: the compiler's
// initialiser for the definitions above (no destructor, no atexit).

// 0x4c0cc0  Confidence: exact. static_init_ccmd_machival_combat: "jmp 0x4c0cd0" (entry 319).
// 0x4c0cd0  Confidence: exact. init_ccmd_machival_combat (console_command constructor).
console_command ccmd_machival_combat("machival_combat", "Sets machival in combat state",
                                     cmd_machival_combat);   // 0x248b010

// 0x4c0cf0  Confidence: exact (the store is scheduled between the two pushes).
void cmd_machival_combat()
{
    machival_combat_enabled = true;
    print_to_console("Setting machival's ai state to combat\n", 0);
}

// 0x4c0d10  Confidence: exact. boss_target_filter_dispatch case 0xc5. Flags: 1 = in close combat with the
// boss and nearer than 4 by the edges; 2 = inside move 3's range; 4 / 8 = in the melee / move 1 cone.
void machival_filter_target(Character *boss, boss_target *t)
{
    float dist;

    if (t->character == NULL)
        return;
    dist = boss->distance_between_edges(t->character, true, -1);
    if (t->character->ai.target == boss->handle && t->character->ai.is_mode(AI_STATE_COMBAT, AIS_CLOSE_COMBAT)
        && dist < 4.0f)
        t->flags |= 1;
    if (dist < machival_move_3->range_max)
        t->flags |= 2;
    if (machival_offense_point_reaches(t->character, machival_move_melee) == true)
        t->flags |= 4;
    if (machival_offense_point_reaches(t->character, machival_move_1) == true)
        t->flags |= 8;
}

// 0x4c0dd0  Confidence: exact (VC6 byte match) (the dot product is summed z, y, x). The horizontal distance from the move's
// offense point within range_max and the direction within the cone (as pyrul_point_in_attack_cone).
bool machival_offense_point_reaches(Character *c, boss_attack *move)
{
    boss_offense_point *pt = &machival_offense_points[move->offense_point];
    vector3 d(c->pos.x - pt->pos.x, 0.0f, c->pos.z - pt->pos.z);

    if (d.normalize_safe_length() > move->range_max)
        return false;
    if (d.x * pt->dir.x + d.y * pt->dir.y + d.z * pt->dir.z < move->cos_max_angle)
        return false;
    return true;
}

// 0x4c0e70  Confidence: exact. Called once, at the first target: teleport after 70 s, the three spells
// after 0-60 s, 0-45 s and 0-30 s.
void machival_reset_timers()
{
    machival_teleport_timer.SetTimeout(70000);
    machival_spell100_timer.SetTimeout(rand() % 1000 * 60);
    machival_spell101_timer.SetTimeout(rand() % 1000 * 45);
    machival_spell102_timer.SetTimeout(rand() % 1000 * 30);
}

// 0x4c0ee0  Confidence: exact. "$tele_01" .. "$tele_06" (NULL when missing).
void machival_find_teleports()
{
    char name[64];
    int i;

    for (i = 0; i < 6; i++) {
        sprintf(name, "$tele_0%d", i + 1);
        machival_teleports[i] = navpoint_find(name);
    }
}

// 0x4c0f20  Confidence: exact. boss_ai_idle_dispatch case 0xc5.
void ai_state::machival_ai_idle()
{
    if (owner->get_current_move() == NULL && owner->combat.state != COMBAT_IDLE) {
        owner->combat.state = COMBAT_IDLE;
        return;
    }
    if (machival_combat_enabled == true) {
        machival_current_move = NULL;
        enter_combat();
    }
}

// 0x4c0f60  Confidence: exact (the offense-point packing loop as in the other loaders; the tag name and the
// ca_character in locals, D7700). One stage in the table.
void ai_state::machival_boss_load()
{
    int i;
    int tag;

    machival_target_handle = -1;
    for (i = 0; i < BOSS_MAX_THREATS; i++)
        machival_threat_table[i].handle = -1;
    machival_num_offense_points = 0;
    for (i = 0; i < 10; i++) {
        const char *name = offense_point_name(i);
        ca_character *ch = owner->instance->character;
        tag = ch->find_mesh_tag(name);
        if (tag != -1) {
            machival_offense_points[machival_num_offense_points].tag = tag;
            machival_num_offense_points++;
        }
    }
    machival_timers_started = false;
    boss_ai_parse_table("machival_boss.tbl", 1, machival_attack_table, machival_num_offense_points);
    machival_init_move_tables();
    machival_casting_spell = -1;
    machival_cast_anim = -1;
    machival_party_spell_timer.SetTimeout(0);
    machival_find_teleports();
    machival_teleport_navpoint = NULL;
}

// 0x4c1030  Confidence: exact (store order as in the binary).
void machival_init_move_tables()
{
    machival_move_melee = &machival_attack_table[0][0];
    machival_move_1 = &machival_attack_table[0][1];
    machival_move_2 = &machival_attack_table[0][2];
    machival_move_3 = &machival_attack_table[0][3];
    machival_move_4 = &machival_attack_table[0][4];
    machival_move_5 = &machival_attack_table[0][5];
}

// 0x4c1070  Confidence: exact (the action result is a switch, D7700; the sub-mode switch: case -1 has its own
// slot, byte table 0x4c1180; the bodies are laid out 15, 22/23, 51, 52). A multiplayer client only plays
// the teleport sub-modes; the host and single player think first.
void ai_state::machival_ai_frame()
{
    Character *target = NULL;
    int action;

    if (MP_IS_CLIENT()) {
        switch (submode) {
        case AIS_MACHIVAL_TELEPORT:
            machival_ai_special_move();
            break;
        case AIS_MACHIVAL_ARRIVE:
            machival_ai_wait_move_end();
            break;
        }
        return;
    }
    action = machival_ai_select_action();
    switch (action) {
    case 0:
        go_idle();
        return;
    case 1:
    case 4:
        return;
    }
    if (machival_casting_spell != -1)
        return;
    if (submode != AIS_MACHIVAL_TELEPORT && submode != AIS_MACHIVAL_ARRIVE) {
        target = living_from_handle(machival_target_handle);
        owner->combat.target = machival_target_handle;
        if (target == NULL || (target->flags & CF_HIDDEN)) {
            go_idle();
            return;
        }
        if (action == 3)
            submode = AIS_CLOSE_COMBAT;
    }
    switch (submode) {
    case -1:
        return;
    case AIS_CLOSE_COMBAT:
        machival_ai_melee(target);
        break;
    case AIS_GET_CLOSE:
    case AIS_GET_FAR:
        combat_close_in(target);
        break;
    case AIS_MACHIVAL_TELEPORT:
        machival_ai_special_move();
        break;
    case AIS_MACHIVAL_ARRIVE:
        machival_ai_wait_move_end();
        break;
    }
}

// 0x4c11c0  Confidence: exact (VC6 byte match, match31; was likely) (the candidates' direction is normalised although only the cone tests use
// it). Counts the candidates inside move 3's range, inside the melee cone and inside move 1's cone; the
// cones are measured around the direction from the boss to its current target, not the offense points.
void machival_count_targets_in_reach(Character *boss, int *in_reach, int *in_melee, int *in_move1,
                                     boss_target *list, int count)
{
    Character *target;
    vector3 dir;
    vector3 d;
    float dist;
    float dot;
    int i;

    *in_melee = 0;
    *in_move1 = 0;
    *in_reach = 0;
    target = living_from_handle(machival_target_handle);
    if (target == NULL)
        return;
    dir = vector3(target->pos.x - boss->pos.x, 0.0f, target->pos.z - boss->pos.z);
    dir.normalize_safe();
    for (i = 0; i < count; i++, list++) {
        if (list->character == NULL)
            continue;
        dist = list->dist;
        d = vector3(list->character->pos.x - boss->pos.x, 0.0f, list->character->pos.z - boss->pos.z);
        d.normalize_safe();
        if (dist < machival_move_3->range_max)
            (*in_reach)++;
        if (dist < machival_move_melee->range_max) {
            dot = dir.y * d.y + dir.x * d.x + d.z * dir.z;
            if (dot > machival_move_melee->cos_max_angle)
                (*in_melee)++;
        }
        if (dist < machival_move_1->range_max) {
            dot = dir.y * d.y + dir.x * d.x + d.z * dir.z;
            if (dot > machival_move_1->cos_max_angle)
                (*in_move1)++;
        }
    }
}

// 0x4c1330  Confidence: exact. A living, visible party member's handle, -1 none. The local list holds 5
// (no bound check; the party has at most 4).
int machival_pick_random_party_member()
{
    int handles[5];
    int count = 0;
    Player *p;
    Character *c;

    for (p = party_list.next; p != &party_list; p = p->next) {
        c = living_from_handle(p->handle);
        if (c && !(c->flags & CF_HIDDEN))
            handles[count++] = p->handle;
    }
    if (count > 0)
        return handles[rand() % count];
    return -1;
}

// 0x4c13a0  Confidence: likely (the order of the tests matches; the compiler shares the "return 1" tails).
// The spell timers run only after the first target was picked (machival_reset_timers); spells 0x64-0x66 are
// rearmed to 56-104 s, the party spell to 3-5 s. When nobody is in reach of any move, the party spell goes
// to the current target, else the choice is weighted: move 3 by 0.1 per candidate in reach, the melee and
// move 1 by 0.5 per candidate in their cones.
int ai_state::machival_ai_select_action()
{
    boss_target list[BOSS_MAX_THREATS];
    float weights[3];
    int in_reach;
    int in_melee;
    int in_move1;
    Character *target;
    bool busy;
    bool moving;
    bool timer_up;
    int count;
    int handle;
    int index;

    if (machival_casting_spell != -1 || submode == AIS_MACHIVAL_TELEPORT || submode == AIS_MACHIVAL_ARRIVE)
        return 5;
    target = living_from_handle(machival_target_handle);
    if (target && (target->flags & CF_HIDDEN))
        target = NULL;
    moving = owner->get_current_move() != NULL;
    busy = owner->combat.is_busy();
    if (!moving && busy) {
        owner->combat.state = COMBAT_IDLE;
        busy = false;
    }
    if (target == NULL) {
        machival_current_move = NULL;
        machival_248b030 = 0;
    }
    if (busy == true)
        return machival_current_move ? 5 : 0;
    if (machival_current_move)
        return 5;

    timer_up = machival_attack_timer.elapsed();
    machival_update_offense_points(owner);
    count = boss_target_list_build(owner, list, machival_threat_table);
    if (timer_up || !target) {
        handle = boss_target_list_pick(list, count, machival_threat_table, machival_target_handle, false, 0);
        if (handle == -1) {
            if (machival_party_spell_timer.elapsed() == true) {
                machival_party_spell_timer.SetRandomTimeout(3000, 5000);
                handle = machival_pick_random_party_member();
                if (handle != -1) {
                    machival_cast_spell(SPELL_MACHIVAL_PARTY, living_from_handle(handle));
                    return 1;
                }
            }
            return 0;
        }
        if (!machival_timers_started) {
            machival_timers_started = true;
            machival_reset_timers();
        }
        machival_target_handle = handle;
        machival_set_attack_timer(15000, 25000);
    }
    if (machival_teleport_timer.elapsed()) {
        if (machival_teleport(-1) == true) {
            machival_teleport_timer.SetTimeout(70000);
            return 5;
        }
        machival_teleport_timer.SetTimeout(2000);
    }
    if (machival_spell101_timer.elapsed()) {
        index = boss_target_list_find(list, count, machival_target_handle);
        if (index == -1 || !(list[index].flags & 1)) {
            handle = boss_target_list_pick(list, count, machival_threat_table, machival_target_handle, false, 1);
            if (handle == -1)
                goto spell102;
            machival_target_handle = handle;
            machival_set_attack_timer(15000, 25000);
        }
        machival_spell101_timer.SetRandomTimeout(55999, 103999);
        machival_cast_spell(SPELL_MACHIVAL_TARGET, living_from_handle(machival_target_handle));
        return 1;
    }
spell102:
    if (machival_spell102_timer.elapsed()) {
        machival_spell102_timer.SetRandomTimeout(55999, 103999);
        machival_cast_spell_at(SPELL_MACHIVAL_AT_FEET, &owner->pos, owner->layer);
        return 1;
    }
    if (machival_spell100_timer.elapsed()) {
        machival_spell100_timer.SetRandomTimeout(55999, 103999);
        vector3 front(owner->pos.x + owner->orient.fvec.x, owner->pos.y + owner->orient.fvec.y,
                      owner->pos.z + owner->orient.fvec.z);
        machival_cast_spell_at(SPELL_MACHIVAL_AT_SELF, &front, owner->layer);
        return 1;
    }
    machival_count_targets_in_reach(owner, &in_reach, &in_melee, &in_move1, list, count);
    weights[0] = in_reach * 0.1f;
    weights[1] = in_melee * 0.5f;
    weights[2] = in_move1 * 0.5f;
    if (in_reach + in_melee + in_move1 == 0) {
        if (machival_party_spell_timer.elapsed() == true) {
            machival_party_spell_timer.SetRandomTimeout(3000, 5000);
            machival_cast_spell(SPELL_MACHIVAL_PARTY, target);
            return 1;
        }
        return 4;
    }
    index = rand_weighted_index(weights, 3);
    if (index == -1)
        index = 0;
    switch (index) {
    case 0:
        machival_current_move = machival_move_3;
        break;
    case 1:
        machival_current_move = machival_move_melee;
        return 3;
    case 2:
        machival_current_move = machival_move_1;
        return 3;
    }
    return 3;
}

// 0x4c17f0  Confidence: exact (max - min is stored into max's slot for fimul).
void machival_set_attack_timer(int min_ms, int max_ms)
{
    machival_attack_timer.SetTimeout(min_ms + (int)(frand() * (max_ms - min_ms)));
}

// 0x4c1820  Confidence: exact (VC6 byte match) (the position sum goes through a temporary), as pyrul_update_attack_points.
void machival_update_offense_points(Character *c)
{
    int i;
    matrix orient;
    vector3 pos;
    boss_offense_point *pt;

    for (i = 0; i < machival_num_offense_points; i++) {
        pt = &machival_offense_points[i];
        c->instance->get_bone_world_transform(pt->tag, &c->orient, &c->pos, &orient, &pos);
        pt->dir = orient.uvec;
        pt->pos = pos + c->centre_offset;
    }
}

// 0x4c18c0  Confidence: exact. The spell's cast move (category 7, the spell's variation), then a cast goal
// at the point (priority 5000).
void ai_state::machival_cast_spell_at(int spell, vector3 *pos, int layer)
{
    move_info *move = owner->find_move(7, spells[spell].variation);

    if (move == NULL)
        return;
    owner->cur_anim = owner->play_move((int)move, 1.0f, true, true, false);
    if (owner->cur_anim == -1)
        return;
    add_cast_at_point(spell, pos, layer, 5000);
    machival_cast_anim = owner->cur_anim;
    machival_casting_spell = spell;
}

// 0x4c1940  Confidence: exact. Spell 0x5b uses category 0 for its move, the others 7.
void ai_state::machival_cast_spell(int spell, Character *target)
{
    move_info *move;
    int variation = spells[spell].variation;

    if (spell == SPELL_MACHIVAL_PARTY)
        move = owner->find_move(0, variation);
    else
        move = owner->find_move(7, variation);
    if (move == NULL)
        return;
    owner->cur_anim = owner->play_move((int)move, 1.0f, true, true, false);
    if (owner->cur_anim == -1)
        return;
    add_cast_at_target(spell, target, 5000);
    machival_cast_anim = owner->cur_anim;
    machival_casting_spell = spell;
}

// 0x4c19c0  Confidence: exact. boss_ai_check_stage case 0xc5: no HP stages, only the end of a cast.
void ai_state::machival_check_stage()
{
    if (machival_casting_spell != -1 && owner->move_finished(machival_cast_anim))
        machival_casting_spell = -1;
}

// 0x4c19f0  Confidence: exact. Sub-mode AIS_CLOSE_COMBAT: once facing the target (or while a move plays),
// the combat update with the boss move id, both attack slots claimed and the target's guards alerted.
void ai_state::machival_ai_melee(Character *target)
{
    if (combat_face_target(target) || owner->combat.is_busy()) {
        int move_id = boss_get_attack_move_id();
        ai_move_flags |= 4;
        owner->combat.update(move_id);
        claim_attack_slot(owner, target, 0, -1);
        target->ai.claim_attack_slot(target, owner, 0, -1);
        target->ai.alert_guards_of_target(owner);
    }
}

// 0x4c1a80  Confidence: exact (VC6 byte match). boss_attack_trigger_dispatch case 0xc5. For a move with spell 0x3b, every
// party member within 8 of the boss is knocked down for 2-3 s with chance 0.8 (the target always).
void machival_knock_down_party(Character *boss, Character *target)
{
    Player *p;
    Character *c;

    if (machival_current_move == NULL || machival_current_move->spell == -1
        || machival_current_move->spell != SPELL_KNOCKDOWN_MOVE)
        return;
    for (p = party_list.next; p != &party_list; p = p->next) {
        c = living_from_handle(p->handle);
        if (c == NULL)
            continue;
        vector3 d = c->pos - boss->pos;
        if (d.len_sq() < 64.0f && (frand() < 0.8f || c == target) && c->ai.can_be_knocked_down() == true)
            c->ai.knock_down(rand_int_range(2000, 3000));
    }
}

// 0x4c1b60  Confidence: exact. 7 the melee and move 1 (the cone moves), 2..5 moves 2..5, -1 none.
int machival_get_attack_kind()
{
    if (machival_current_move == NULL)
        return -1;
    if (machival_current_move == machival_move_melee || machival_current_move == machival_move_1)
        return 7;
    if (machival_current_move == machival_move_2)
        return 2;
    if (machival_current_move == machival_move_3)
        return 3;
    if (machival_current_move == machival_move_4)
        return 4;
    return machival_current_move == machival_move_5 ? 5 : -1;
}

// 0x4c1bc0  Confidence: exact. The party members in the current move's cone whose defence fails; single
// player walks cur_character's AI group, multiplayer the party list.
bool machival_attack_test(Character *c)
{
    ai_group *group;
    Player *p;
    Character *t;
    int i;

    machival_update_offense_points(c);
    machival_area_hit_count = 0;
    if (game_flags & GF_SINGLE_PLAYER) {
        group = cur_character->ai.group;
        if (group) {
            for (i = 0; i < group->count; i++) {
                t = living_from_handle(group->members[i]);
                if (t && machival_offense_point_reaches(t, machival_current_move)
                    && living_combat_test_defense(&c->combat, &t->combat))
                    machival_area_hits[machival_area_hit_count++] = group->members[i];
            }
        }
    } else {
        for (p = party_list.next; p != &party_list; p = p->next) {
            t = living_from_handle(p->handle);
            if (t && machival_offense_point_reaches(t, machival_current_move)
                && living_combat_test_defense(&c->combat, &t->combat))
                machival_area_hits[machival_area_hit_count++] = p->handle;
        }
    }
    return machival_area_hit_count > 0;
}

// 0x4c1d10  Confidence: exact. The members of cur_character's group within 8 of the boss (horizontally)
// whose defence fails (single player only: no party list branch).
bool machival_collect_area_hits(Character *c)
{
    ai_group *group;
    Character *t;
    int i;

    machival_area_hit_count = 0;
    group = cur_character->ai.group;
    if (group) {
        for (i = 0; i < group->count; i++) {
            t = living_from_handle(group->members[i]);
            if (t == NULL)
                continue;
            float dx = t->pos.x - c->pos.x;
            float dz = t->pos.z - c->pos.z;
            if (dz * dz + dx * dx < 64.0f && living_combat_test_defense(&c->combat, &t->combat))
                machival_area_hits[machival_area_hit_count++] = group->members[i];
        }
    }
    return machival_area_hit_count > 0;
}

// 0x4c1dd0  Confidence: exact.
void machival_apply_area_hits(Character *c)
{
    int i;
    Character *t;

    for (i = 0; i < machival_area_hit_count; i++) {
        t = living_from_handle(machival_area_hits[i]);
        if (t)
            living_combat_resolve_hit_vs(&c->combat, &t->combat);
    }
}

// 0x4c1e20  Confidence: exact (VC6 byte match). (index): a client takes the navpoint the host sent (mp_handle_swi_boss
// has set machival_teleport_pos; a bad index becomes 0); the host / single player picks one of the six at
// random and fails when it is the current one. Facing: towards the target, or the navpoint's heading.
// Then the vanish move (category 8, 0x8a) and sub-mode AIS_MACHIVAL_TELEPORT; the host sends SWI_BSS.
bool ai_state::machival_teleport(int index)
{
    navpoint *nav;
    Character *target;

    if (MP_IS_CLIENT()) {
        if (index < 0 || index >= 6)
            index = 0;
        machival_teleport_navpoint = machival_teleports[index];
    } else {
        int r = rand() % 6;
        nav = machival_teleports[r];
        if (nav == machival_teleport_navpoint)
            return false;
        machival_teleport_navpoint = nav;
        target = living_from_handle(machival_target_handle);
        if (target == NULL) {
            matrix m;
            m.set_from_angles(0.0f, 0.0f, machival_teleport_navpoint->heading);
            machival_teleport_pos = m.fvec;
        } else {
            machival_teleport_pos = target->pos - owner->pos;
        }
    }
    move_info *m = owner->find_move(8, 0x8a);
    machival_cast_anim = owner->play_move((int)m, 1.0f, false, false, false);
    set_mode(AI_STATE_COMBAT, AIS_MACHIVAL_TELEPORT);
    priority = 99999;
    owner->set_6ac(0.0f);
    ai_move_flags |= 4;
    if (MP_IS_HOST())
        mp_send_swi_boss(owner, AI_STATE_COMBAT, AIS_MACHIVAL_TELEPORT);
    return true;
}

// 0x4c1f90  Confidence: exact (VC6 byte match). Sub-mode AIS_MACHIVAL_TELEPORT: when the vanish move ends, the boss moves
// to the navpoint, faces machival_teleport_pos, becomes visible (object flag 1, instance alpha 0xff) and
// plays the arrival move (8, 0x8b). Until then the object is hidden from frame 69 of the move on.
void ai_state::machival_ai_special_move()
{
    ai_move_flags |= 4;
    if (owner->move_finished(machival_cast_anim)) {
        owner->pos = machival_teleport_navpoint->pos;
        owner->orient.set_from_forward(machival_teleport_pos);
        owner->obj_flags |= 1;
        owner->instance->_unkb4 = 0xff;
        move_info *m = owner->find_move(8, 0x8b);
        machival_cast_anim = owner->play_move((int)m, 1.0f, false, false, false);
        set_mode(AI_STATE_COMBAT, AIS_MACHIVAL_ARRIVE);
        priority = 99999;
        return;
    }
    owner->move_trigger_fired(6, 0);
    if (owner->instance->get_anim_elapsed(owner->cur_move->anim) >= 69.0f)
        owner->obj_flags &= ~1;
}

// 0x4c2090  Confidence: exact. Sub-mode AIS_MACHIVAL_ARRIVE.
void ai_state::machival_ai_wait_move_end()
{
    ai_move_flags |= 4;
    owner->obj_flags |= 1;
    if (owner->move_finished(machival_cast_anim)) {
        machival_cast_anim = -1;
        go_idle();
    }
}

// folded (0x4f13e0, empty_function)  Confidence: likely. boss_ai_unload case 0xc5 (boss_ai.h: folded).
void machival_unload()
{
}
